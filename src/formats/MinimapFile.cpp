#include "formats/MinimapFile.h"
#include "patch/BackupManager.h"
#include "core/Logger.h"
// Note: StringHash.h / HashResolver intentionally NOT included here.
// HashResolver mutations must only occur on the UI thread (see MinimapPanel::Load
// onDone callback).  Including it here tempts future callers to Register() from
// background threads, which is an unsynchronised write to the global unordered_map.

// stb_image — declaration only; implementation is compiled in MinimapPanel.cpp.
#include <stb_image.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <algorithm>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// MinimapParser helpers
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ void MinimapParser::ComputeGrid(int n, int& cols, int& rows) {
    if (n <= 0) { cols = rows = 0; return; }
    cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(n))));
    rows = (n + cols - 1) / cols;
}

/*static*/ size_t MinimapParser::FindChunk(const uint8_t* buf, size_t bufLen,
                                            size_t offset, uint32_t targetID) {
    while (offset + 8 <= bufLen) {
        uint32_t id   = 0;
        uint32_t size = 0;
        std::memcpy(&id,   buf + offset,     4);
        std::memcpy(&size, buf + offset + 4, 4);

        // Strip container bit to get the comparable ID
        const uint32_t bareID = id & 0x7FFFFFFFu;
        const uint32_t bareTarget = targetID & 0x7FFFFFFFu;

        if (bareID == bareTarget)
            return offset + 8; // return payload start

        offset += 8 + size;
    }
    return std::string::npos;
}

/*static*/ bool MinimapParser::ParseTPK(const std::vector<uint8_t>& decompressed,
                                         MinimapTile& tile) {
    // Walk TPK container (0xB3300000) → TPKInfo (0xB3310000) → TPKEntries (0x33310004)
    // to grab the 124-byte TexInfoEntry, then walk to TPKData (0xB3320000) →
    // TPKDataRaw (0x33320002) for the raw DXT3 pixels.

    const uint8_t* buf    = decompressed.data();
    const size_t   bufLen = decompressed.size();

    if (bufLen < 8) return false;

    // Expect TPK container at offset 0
    uint32_t rootID = 0;
    std::memcpy(&rootID, buf, 4);
    if ((rootID & 0x7FFFFFFFu) != (0xB3300000u & 0x7FFFFFFFu)) {
        LOG_WARN("MinimapParser::ParseTPK: unexpected root chunk 0x{:08X}", rootID);
        return false;
    }

    // ── Locate TPKInfo container ─────────────────────────────────────────────
    size_t infoPayload = FindChunk(buf, bufLen, 8, 0xB3310000u);
    if (infoPayload == std::string::npos) {
        LOG_WARN("MinimapParser::ParseTPK: TPKInfo not found");
        return false;
    }

    // Read TPKInfo container size so we can bound our search
    uint32_t infoContainerSize = 0;
    std::memcpy(&infoContainerSize, buf + (infoPayload - 8) + 4, 4);
    const size_t infoEnd = infoPayload + infoContainerSize;

    // ── Locate TPKEntries (0x33310004) inside TPKInfo ────────────────────────
    size_t entriesPayload = FindChunk(buf, std::min(bufLen, infoEnd), infoPayload, 0x33310004u);
    if (entriesPayload == std::string::npos) {
        LOG_WARN("MinimapParser::ParseTPK: TPKEntries not found");
        return false;
    }

    // ── Parse first 124-byte TexInfoEntry ────────────────────────────────────
    // Layout from header:
    //  +0   pad[12]
    //  +12  name[24]
    //  +36  nameHash (u32)
    //  +40  classNameHash (u32)
    //  +44  imageParentHash (u32)
    //  +48  imagePlacement (u32)
    //  +52  palettePlacement (u32)
    //  +56  imageSize (u32)
    //  +60  paletteSize (u32)
    //  +64  baseImageSize (u32)
    //  +68  width (u16)
    //  +70  height (u16)
    //  +72  shiftWidth (u8)
    //  +73  shiftHeight (u8)
    //  +74  comprType (u8)
    //  +75  palComprType (u8)
    //  +76  numPaletteEntries (u16)
    //  +78  numMipLevels (u8)
    //  +79  pad[45]
    constexpr size_t kEntrySize = 124;
    if (entriesPayload + kEntrySize > bufLen) {
        LOG_WARN("MinimapParser::ParseTPK: TPKEntries too short");
        return false;
    }

    const uint8_t* entry = buf + entriesPayload;

    char name[25] = {};
    std::memcpy(name, entry + 12, 24);
    tile.name = name;

    uint32_t nameHash = 0;
    std::memcpy(&nameHash, entry + 36, 4);
    tile.nameHash = nameHash;

    // NOTE: Do NOT call HashResolver::Instance().Register() here.
    // ParseTPK() runs on a background worker thread; HashResolver::table_ is an
    // unordered_map with no synchronisation.  Name registration is deferred to
    // the MinimapPanel::Load() onDone callback, which executes on the UI thread.

    // ── Locate TPKData container (0xB3320000) ────────────────────────────────
    size_t dataPayload = FindChunk(buf, bufLen, 8, 0xB3320000u);
    if (dataPayload == std::string::npos) {
        LOG_WARN("MinimapParser::ParseTPK: TPKData not found");
        return false;
    }

    // ── Locate TPKDataRaw (0x33320002) inside TPKData ────────────────────────
    uint32_t dataContainerSize = 0;
    std::memcpy(&dataContainerSize, buf + (dataPayload - 8) + 4, 4);
    const size_t dataEnd = dataPayload + dataContainerSize;

    size_t rawPayload = FindChunk(buf, std::min(bufLen, dataEnd), dataPayload, 0x33320002u);
    if (rawPayload == std::string::npos) {
        LOG_WARN("MinimapParser::ParseTPK: TPKDataRaw not found");
        return false;
    }

    // Read pixel data size from chunk header
    uint32_t rawSize = 0;
    std::memcpy(&rawSize, buf + (rawPayload - 8) + 4, 4);

    if (rawPayload + rawSize > bufLen) {
        LOG_WARN("MinimapParser::ParseTPK: TPKDataRaw overruns buffer");
        return false;
    }

    // The raw payload begins with a small header (TPKDataHeader = 0x33320001
    // precedes this in well-formed TPKs, but the raw pixel bytes follow the
    // data-section alignment padding). The game stores pixel data directly.
    // We expect exactly kTilePixelBytes of DXT3 data.
    if (rawSize < kTilePixelBytes) {
        LOG_WARN("MinimapParser::ParseTPK: raw pixel data too small ({} < {})",
                 rawSize, kTilePixelBytes);
        return false;
    }

    tile.dxt3Data.assign(buf + rawPayload, buf + rawPayload + kTilePixelBytes);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapParser::Load
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ Result<MinimapFile> MinimapParser::Load(const std::filesystem::path& path) {
    // ── Read entire file ─────────────────────────────────────────────────────
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        return Result<MinimapFile>::Err("MinimapParser: cannot open '" + path.string() + "'");

    const auto fileSize = static_cast<size_t>(in.tellg());
    in.seekg(0);
    std::vector<uint8_t> fileData(fileSize);
    if (!in.read(reinterpret_cast<char*>(fileData.data()), static_cast<std::streamsize>(fileSize)))
        return Result<MinimapFile>::Err("MinimapParser: read error on '" + path.string() + "'");

    MinimapFile mf;
    mf.path = path;

    // ── Walk outer chunk stream ───────────────────────────────────────────────
    // Each outer chunk:  [u32 id = 0x0003A100] [u32 payloadSize] [payloadSize bytes]
    size_t pos = 0;
    uint32_t tileIndex = 0;

    while (pos + 8 <= fileSize) {
        uint32_t chunkID   = 0;
        uint32_t chunkSize = 0;
        std::memcpy(&chunkID,   fileData.data() + pos,     4);
        std::memcpy(&chunkSize, fileData.data() + pos + 4, 4);

        if (chunkID != kChunkMiniMapTile) {
            LOG_WARN("MinimapParser: unexpected chunk ID 0x{:08X} at offset {}", chunkID, pos);
            // Skip it and try to continue
            pos += 8 + chunkSize;
            continue;
        }

        if (pos + 8 + chunkSize > fileSize)
            return Result<MinimapFile>::Err("MinimapParser: chunk overruns file at offset " +
                                            std::to_string(pos));

        MinimapTile tile;
        tile.index           = tileIndex;
        tile.fileChunkOffset = static_cast<uint64_t>(pos);
        tile.fileChunkSize   = 8 + chunkSize;

        // ── Decompress JDLZ payload ───────────────────────────────────────────
        std::span<const uint8_t> payload(fileData.data() + pos + 8, chunkSize);
        auto decompResult = LZCDecompressor::Decompress(payload);
        if (!decompResult) {
            return Result<MinimapFile>::Err(
                "MinimapParser: tile " + std::to_string(tileIndex) +
                " decompression failed: " + decompResult.error);
        }

        // ── Parse decompressed TPK ────────────────────────────────────────────
        if (!ParseTPK(decompResult.value, tile)) {
            return Result<MinimapFile>::Err(
                "MinimapParser: tile " + std::to_string(tileIndex) + " TPK parse failed");
        }

        mf.tiles.push_back(std::move(tile));
        ++tileIndex;
        pos += 8 + chunkSize;
    }

    if (mf.tiles.empty())
        return Result<MinimapFile>::Err("MinimapParser: no tiles found in '" + path.string() + "'");

    ComputeGrid(static_cast<int>(mf.tiles.size()), mf.gridCols, mf.gridRows);

    LOG_INFO("MinimapParser: loaded {} tiles ({}×{} grid) from '{}'",
             mf.tiles.size(), mf.gridCols, mf.gridRows, path.filename().string());

    return Result<MinimapFile>::Ok(std::move(mf));
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker — JDLZ compression
// ─────────────────────────────────────────────────────────────────────────────
// Implements the same JDLZ format that LZCDecompressor reads.
// Header (16 bytes):
//   [0]  'J','D','L','Z'
//   [4]  0x02 (version)
//   [5]  0x10 (header size)
//   [6]  0x00, 0x00 (reserved)
//   [8]  u32 decompressed size
//   [12] u32 compressed size (payload only, excl. header? — game uses total)
//
// Delegates to the general back-reference JDLZ encoder (LZCDecompressor::
// Compress), which produces a real compressed stream (~0.5x) that the game and
// our own decompressor both accept. Falls back to a valid literal-only stream
// only if the general encoder somehow fails, so a tile is always writable.

/*static*/ std::vector<uint8_t> MinimapRepacker::CompressJDLZ(
    const std::vector<uint8_t>& src) {

    // ── Preferred path: the general JDLZ compressor (smaller, still valid) ────
    if (auto c = LZCDecompressor::Compress(src); c)
        return std::move(c.value);

    // ── Fallback: literal-only stream (no back-references) ────────────────────
    // flags1 bit=0 → literal; flags2 is irrelevant when flags1 bit=0.
    // Emit a flags1 byte of 0x00 (all-literal) every 8 literals.
    std::vector<uint8_t> body;
    body.reserve(src.size() + src.size() / 8 + 2);

    size_t i = 0;
    while (i < src.size()) {
        const size_t batchSize = std::min<size_t>(8, src.size() - i);
        body.push_back(0x00); // flags1: all literals for next 8 bytes
        body.push_back(0x00); // flags2: not used (no back-refs)
        for (size_t j = 0; j < batchSize; ++j)
            body.push_back(src[i++]);
    }

    // ── Assemble 16-byte header + body ───────────────────────────────────────
    const uint32_t decompSize  = static_cast<uint32_t>(src.size());
    const uint32_t compPayload = static_cast<uint32_t>(body.size());
    const uint32_t totalSize   = 16 + compPayload;

    std::vector<uint8_t> out(totalSize);
    out[0] = 'J'; out[1] = 'D'; out[2] = 'L'; out[3] = 'Z';
    out[4] = 0x02; out[5] = 0x10; out[6] = 0x00; out[7] = 0x00;
    std::memcpy(out.data() + 8,  &decompSize,  4);
    std::memcpy(out.data() + 12, &totalSize,   4);
    std::memcpy(out.data() + 16, body.data(), body.size());
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker::PatchTPKPixels
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::vector<uint8_t> MinimapRepacker::PatchTPKPixels(
    const std::vector<uint8_t>& decompressed,
    const std::vector<uint8_t>& newDXT3) {

    // Clone the decompressed TPK then locate and overwrite TPKDataRaw payload.
    std::vector<uint8_t> patched = decompressed;
    const uint8_t* buf    = patched.data();
    const size_t   bufLen = patched.size();

    // Walk to TPKData → TPKDataRaw
    size_t dataPayload = MinimapParser::FindChunk(buf, bufLen, 8, 0xB3320000u);
    if (dataPayload == std::string::npos) return patched; // can't find, return as-is

    uint32_t dataContainerSize = 0;
    std::memcpy(&dataContainerSize, buf + (dataPayload - 8) + 4, 4);
    const size_t dataEnd = dataPayload + dataContainerSize;

    size_t rawPayload = MinimapParser::FindChunk(buf, std::min(bufLen, dataEnd),
                                                  dataPayload, 0x33320002u);
    if (rawPayload == std::string::npos) return patched;

    uint32_t rawSize = 0;
    std::memcpy(&rawSize, buf + (rawPayload - 8) + 4, 4);

    // Overwrite pixel bytes in place (same size guaranteed for same-dimension tiles)
    const size_t copyLen = std::min<size_t>(newDXT3.size(), rawSize);
    std::memcpy(patched.data() + rawPayload, newDXT3.data(), copyLen);
    return patched;
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker::BuildChunk
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::vector<uint8_t> MinimapRepacker::BuildChunk(
    const std::vector<uint8_t>& decompressed) {

    auto compressed = CompressJDLZ(decompressed);
    const uint32_t payloadSize = static_cast<uint32_t>(compressed.size());

    std::vector<uint8_t> chunk(8 + payloadSize);
    std::memcpy(chunk.data(),     &kChunkMiniMapTile, 4);
    std::memcpy(chunk.data() + 4, &payloadSize,       4);
    std::memcpy(chunk.data() + 8, compressed.data(),  payloadSize);
    return chunk;
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker::ReplaceTile
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ TileReplaceResult MinimapRepacker::ReplaceTile(
    MinimapFile& file,
    uint32_t tileIndex,
    const std::vector<uint8_t>& newDXT3,
    BackupManager& bm) {

    TileReplaceResult result;
    result.tileIndex = tileIndex;

    if (tileIndex >= file.tiles.size()) {
        result.error = "tile index out of range";
        return result;
    }
    if (newDXT3.size() != kTilePixelBytes) {
        result.error = "newDXT3 must be exactly " + std::to_string(kTilePixelBytes) + " bytes";
        return result;
    }

    MinimapTile& tile = file.tiles[tileIndex];

    // ── Read the original compressed chunk from disk ──────────────────────────
    std::fstream f(file.path, std::ios::binary | std::ios::in | std::ios::out);
    if (!f) {
        result.error = "cannot open '" + file.path.string() + "' for writing";
        return result;
    }

    f.seekg(static_cast<std::streamoff>(tile.fileChunkOffset + 8)); // skip header
    const uint32_t origPayloadSize = tile.fileChunkSize - 8;
    std::vector<uint8_t> origCompressed(origPayloadSize);
    if (!f.read(reinterpret_cast<char*>(origCompressed.data()),
                static_cast<std::streamsize>(origPayloadSize))) {
        result.error = "read error reading original chunk";
        return result;
    }

    // ── Decompress, patch pixels, recompress ─────────────────────────────────
    auto decompResult = LZCDecompressor::Decompress(
        std::span<const uint8_t>(origCompressed));
    if (!decompResult) {
        result.error = "decompression failed: " + decompResult.error;
        return result;
    }

    auto patched    = PatchTPKPixels(decompResult.value, newDXT3);
    auto newChunk   = BuildChunk(patched);
    const uint32_t newPayloadSize = static_cast<uint32_t>(newChunk.size() - 8);

    // ── Record backup before first write ─────────────────────────────────────
    if (!bm.EnsureFileBak(file.path)) {
        result.error = "backup failed for '" + file.path.string() + "'";
        return result;
    }

    // Record the original bytes for the region we are about to overwrite
    f.seekg(static_cast<std::streamoff>(tile.fileChunkOffset));
    std::vector<uint8_t> origRegion(tile.fileChunkSize);
    f.read(reinterpret_cast<char*>(origRegion.data()),
           static_cast<std::streamsize>(tile.fileChunkSize));
    bm.RecordRegion(file.path, tile.fileChunkOffset, origRegion);

    // ── Algorithm A: in-place overwrite if new payload fits ───────────────────
    if (newPayloadSize <= origPayloadSize) {
        // Overwrite chunk header with new payload size then write payload.
        f.seekp(static_cast<std::streamoff>(tile.fileChunkOffset));
        f.write(reinterpret_cast<const char*>(newChunk.data()),
                static_cast<std::streamsize>(newChunk.size()));
        // Zero-pad any remaining bytes of the original region
        const uint32_t padding = origPayloadSize - newPayloadSize;
        if (padding > 0) {
            std::vector<uint8_t> zeros(padding, 0x00);
            f.write(reinterpret_cast<const char*>(zeros.data()),
                    static_cast<std::streamsize>(padding));
        }
        if (!f) {
            result.error = "write error (in-place)";
            return result;
        }
    } else {
        // ── Algorithm B: full-file rewrite ────────────────────────────────────
        f.close();

        // Read entire file
        std::ifstream fin(file.path, std::ios::binary | std::ios::ate);
        if (!fin) { result.error = "cannot re-open file for full rewrite"; return result; }
        const auto fileSize = static_cast<size_t>(fin.tellg());
        fin.seekg(0);
        std::vector<uint8_t> fileData(fileSize);
        fin.read(reinterpret_cast<char*>(fileData.data()),
                 static_cast<std::streamsize>(fileSize));
        fin.close();

        // Build new file: everything before the chunk, new chunk, everything after
        const size_t chunkEnd = tile.fileChunkOffset + tile.fileChunkSize;
        std::vector<uint8_t> newFile;
        newFile.reserve(fileSize - tile.fileChunkSize + newChunk.size());
        newFile.insert(newFile.end(),
                       fileData.begin(),
                       fileData.begin() + static_cast<std::ptrdiff_t>(tile.fileChunkOffset));
        newFile.insert(newFile.end(), newChunk.begin(), newChunk.end());
        newFile.insert(newFile.end(),
                       fileData.begin() + static_cast<std::ptrdiff_t>(chunkEnd),
                       fileData.end());

        std::ofstream fout(file.path, std::ios::binary | std::ios::trunc);
        if (!fout) { result.error = "cannot open file for full rewrite"; return result; }
        fout.write(reinterpret_cast<const char*>(newFile.data()),
                   static_cast<std::streamsize>(newFile.size()));
        if (!fout) { result.error = "write error (full rewrite)"; return result; }

        // Update chunk metadata for subsequent calls
        tile.fileChunkSize = static_cast<uint32_t>(newChunk.size());
        // Note: offsets of subsequent tiles are now shifted; update them
        const int64_t delta = static_cast<int64_t>(newChunk.size()) -
                              static_cast<int64_t>(origPayloadSize + 8);
        for (uint32_t i = tileIndex + 1; i < static_cast<uint32_t>(file.tiles.size()); ++i)
            file.tiles[i].fileChunkOffset =
                static_cast<uint64_t>(static_cast<int64_t>(file.tiles[i].fileChunkOffset) + delta);
    }

    // ── Update in-memory tile data ────────────────────────────────────────────
    tile.dxt3Data = newDXT3;
    result.ok = true;

    LOG_INFO("MinimapRepacker: replaced tile {} ('{}') in '{}'",
             tileIndex, tile.name, file.path.filename().string());
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker::EncodeDXT3Block
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ void MinimapRepacker::EncodeDXT3Block(
    const uint8_t rgba[4][4][4],
    uint8_t out[16]) {

    // ── Alpha block (8 bytes): 4-bit alpha per pixel, low nibble first ────────
    for (int i = 0; i < 8; ++i) {
        const int p0 = i * 2;
        const int p1 = i * 2 + 1;
        const uint8_t a0 = rgba[p0 / 4][p0 % 4][3] >> 4;
        const uint8_t a1 = rgba[p1 / 4][p1 % 4][3] >> 4;
        out[i] = static_cast<uint8_t>(a0 | (a1 << 4));
    }

    // ── Colour block (8 bytes): find bounding box in RGB space ───────────────
    uint8_t rMin = 255, rMax = 0, gMin = 255, gMax = 0, bMin = 255, bMax = 0;
    for (int py = 0; py < 4; ++py) {
        for (int px = 0; px < 4; ++px) {
            rMin = std::min(rMin, rgba[py][px][0]);
            rMax = std::max(rMax, rgba[py][px][0]);
            gMin = std::min(gMin, rgba[py][px][1]);
            gMax = std::max(gMax, rgba[py][px][1]);
            bMin = std::min(bMin, rgba[py][px][2]);
            bMax = std::max(bMax, rgba[py][px][2]);
        }
    }

    // Quantise to RGB565
    auto to565 = [](uint8_t r, uint8_t g, uint8_t b) -> uint16_t {
        return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    };

    uint16_t c0 = to565(rMax, gMax, bMax);
    uint16_t c1 = to565(rMin, gMin, bMin);

    // Ensure c0 > c1 for the 4-colour interpolation mode
    if (c0 <= c1) std::swap(c0, c1);

    std::memcpy(out + 8,  &c0, 2);
    std::memcpy(out + 10, &c1, 2);

    // Reconstruct the four palette colours from c0/c1
    auto from565 = [](uint16_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = static_cast<uint8_t>(((v >> 11) & 0x1F) * 255 / 31);
        g = static_cast<uint8_t>(((v >>  5) & 0x3F) * 255 / 63);
        b = static_cast<uint8_t>(((v      ) & 0x1F) * 255 / 31);
    };

    uint8_t cr[4], cg[4], cb[4];
    from565(c0, cr[0], cg[0], cb[0]);
    from565(c1, cr[1], cg[1], cb[1]);
    cr[2] = static_cast<uint8_t>((2 * cr[0] + cr[1]) / 3);
    cg[2] = static_cast<uint8_t>((2 * cg[0] + cg[1]) / 3);
    cb[2] = static_cast<uint8_t>((2 * cb[0] + cb[1]) / 3);
    cr[3] = static_cast<uint8_t>((cr[0] + 2 * cr[1]) / 3);
    cg[3] = static_cast<uint8_t>((cg[0] + 2 * cg[1]) / 3);
    cb[3] = static_cast<uint8_t>((cb[0] + 2 * cb[1]) / 3);

    // Assign each pixel to its nearest palette entry
    uint32_t lut = 0;
    for (int i = 0; i < 16; ++i) {
        const int py = i / 4, px = i % 4;
        const uint8_t r = rgba[py][px][0];
        const uint8_t g = rgba[py][px][1];
        const uint8_t b = rgba[py][px][2];

        uint32_t bestDist = UINT32_MAX;
        int bestIdx = 0;
        for (int k = 0; k < 4; ++k) {
            const int dr = static_cast<int>(r) - cr[k];
            const int dg = static_cast<int>(g) - cg[k];
            const int db = static_cast<int>(b) - cb[k];
            const uint32_t dist = static_cast<uint32_t>(dr*dr + dg*dg + db*db);
            if (dist < bestDist) { bestDist = dist; bestIdx = k; }
        }
        lut |= static_cast<uint32_t>(bestIdx) << (i * 2);
    }
    std::memcpy(out + 12, &lut, 4);
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker::EncodeDXT3
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::vector<uint8_t> MinimapRepacker::EncodeDXT3(
    const uint8_t* rgba, uint32_t w, uint32_t h) {

    const uint32_t bw = (w + 3) / 4;
    const uint32_t bh = (h + 3) / 4;
    std::vector<uint8_t> out(bw * bh * 16, 0);

    for (uint32_t by = 0; by < bh; ++by) {
        for (uint32_t bx = 0; bx < bw; ++bx) {
            uint8_t block[4][4][4] = {};
            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    const uint32_t sx = std::min(bx * 4 + static_cast<uint32_t>(px), w - 1);
                    const uint32_t sy = std::min(by * 4 + static_cast<uint32_t>(py), h - 1);
                    const uint8_t* p = rgba + (sy * w + sx) * 4;
                    block[py][px][0] = p[0];
                    block[py][px][1] = p[1];
                    block[py][px][2] = p[2];
                    block[py][px][3] = p[3];
                }
            }
            EncodeDXT3Block(block, out.data() + (by * bw + bx) * 16);
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker::PrepareSourceImage
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ Result<std::vector<uint8_t>> MinimapRepacker::PrepareSourceImage(
    const std::filesystem::path& srcPath) {

    const std::string ext = [&] {
        std::string e = srcPath.extension().string();
        for (auto& c : e) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return e;
    }();

    if (ext == ".dds") {
        // ── DDS: extract raw DXT3 pixel data directly ─────────────────────────
        // DDS header is 128 bytes (4 magic + 124 DDSURFACEDESC2).
        // We trust that PrepareSourceImage is only called with valid 128×128 DXT3.
        std::ifstream f(srcPath, std::ios::binary | std::ios::ate);
        if (!f)
            return Result<std::vector<uint8_t>>::Err(
                "PrepareSourceImage: cannot open '" + srcPath.string() + "'");

        const auto fileSize = static_cast<size_t>(f.tellg());
        constexpr size_t kDDSHeaderSize = 128;
        if (fileSize < kDDSHeaderSize + kTilePixelBytes)
            return Result<std::vector<uint8_t>>::Err(
                "PrepareSourceImage: DDS file too small (expected 128×128 DXT3)");

        // Verify DDS magic
        f.seekg(0);
        uint32_t magic = 0;
        f.read(reinterpret_cast<char*>(&magic), 4);
        if (magic != 0x20534444u) // 'DDS '
            return Result<std::vector<uint8_t>>::Err(
                "PrepareSourceImage: not a DDS file");

        // Read pixel data starting at byte 128
        f.seekg(static_cast<std::streamoff>(kDDSHeaderSize));
        std::vector<uint8_t> dxt3(kTilePixelBytes);
        if (!f.read(reinterpret_cast<char*>(dxt3.data()),
                    static_cast<std::streamsize>(kTilePixelBytes)))
            return Result<std::vector<uint8_t>>::Err(
                "PrepareSourceImage: read error on DDS pixel data");

        return Result<std::vector<uint8_t>>::Ok(std::move(dxt3));

    } else if (ext == ".png") {
        // ── PNG: decode via stb_image then encode to DXT3 ────────────────────
        // STB_IMAGE_IMPLEMENTATION is defined above (once) in this TU.
        const std::string pathStr = srcPath.string();
        int w = 0, h = 0, channels = 0;

        unsigned char* pixels = stbi_load(pathStr.c_str(), &w, &h, &channels, 4);
        if (!pixels)
            return Result<std::vector<uint8_t>>::Err(
                "PrepareSourceImage: stbi_load failed for '" + pathStr + "'");

        if (w != static_cast<int>(kTileWidth) || h != static_cast<int>(kTileHeight)) {
            stbi_image_free(pixels);
            return Result<std::vector<uint8_t>>::Err(
                "PrepareSourceImage: PNG must be " +
                std::to_string(kTileWidth) + "×" + std::to_string(kTileHeight) +
                ", got " + std::to_string(w) + "×" + std::to_string(h));
        }

        auto dxt3 = EncodeDXT3(pixels, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        stbi_image_free(pixels);
        return Result<std::vector<uint8_t>>::Ok(std::move(dxt3));

    } else {
        return Result<std::vector<uint8_t>>::Err(
            "PrepareSourceImage: unsupported format '" + ext +
            "' (expected .dds or .png)");
    }
}

} // namespace nfsmw
