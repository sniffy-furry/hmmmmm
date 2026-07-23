#include "patch/BunRepacker.h"
#include "core/Logger.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <unordered_map>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// DXT size helpers
// ─────────────────────────────────────────────────────────────────────────────

size_t BunRepacker::DxtMipBytes(TexFormat fmt, uint32_t w, uint32_t h) {
    const uint32_t bw = std::max(1u, (w + 3u) / 4u);
    const uint32_t bh = std::max(1u, (h + 3u) / 4u);
    switch (fmt) {
        case TexFormat::DXT1:   return bw * bh * 8u;
        case TexFormat::DXT3:
        case TexFormat::DXT5:   return bw * bh * 16u;
        case TexFormat::ARGB32: return std::max(1u, w) * std::max(1u, h) * 4u;
        default:                return 0u;
    }
}

size_t BunRepacker::DxtChainBytes(TexFormat fmt, uint32_t w, uint32_t h, uint32_t mips) {
    size_t total = 0;
    for (uint32_t i = 0; i < mips; ++i) {
        total += DxtMipBytes(fmt, w, h);
        if (w == 1 && h == 1) break;
        w = std::max(1u, w >> 1);
        h = std::max(1u, h >> 1);
    }
    return total;
}

size_t BunRepacker::Mip0Size(TexFormat fmt, uint32_t w, uint32_t h) {
    return DxtMipBytes(fmt, w, h);
}

// ─────────────────────────────────────────────────────────────────────────────
// Format ↔ compressionType byte (from bun_format.hpp TexCompression enum)
// ─────────────────────────────────────────────────────────────────────────────

uint8_t BunRepacker::FormatToCompType(TexFormat f) {
    switch (f) {
        case TexFormat::ARGB32: return 0x20;
        case TexFormat::DXT1:   return 0x21;
        case TexFormat::DXT3:   return 0x24;
        case TexFormat::DXT5:   return 0x26;
        default:                return 0x00;
    }
}

TexFormat BunRepacker::CompTypeToFormat(uint8_t ct) {
    switch (ct) {
        case 0x20: return TexFormat::ARGB32;
        case 0x21:
        case 0x22: return TexFormat::DXT1;
        case 0x24: return TexFormat::DXT3;
        case 0x26: return TexFormat::DXT5;
        default:   return TexFormat::Unknown;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// BUN buffer walking — collect per-TPK contexts
// ─────────────────────────────────────────────────────────────────────────────

void BunRepacker::CollectTPKs(const uint8_t* buf, size_t sz,
                               std::vector<TpkCtx>& out) const {
    TpkCtx pending{};
    WalkContainer(buf, sz, 0, pending, false, out);
}

void BunRepacker::WalkContainer(const uint8_t* buf, size_t sz,
                                 size_t absBase,
                                 TpkCtx& pending, bool pendingValid,
                                 std::vector<TpkCtx>& out) const
{
    static constexpr size_t kHdr = 8;
    size_t off = 0;

    // We maintain two pieces of state across siblings:
    //   pendingLocal — accumulated TEX_INFO/TEX_FORMAT from the latest CHUNK_TPK_ROOT
    //   When we hit CHUNK_TPK_DATA we marry them and push to `out`.
    TpkCtx pendingLocal = pending;
    bool   havePending  = pendingValid;

    while (off + kHdr <= sz) {
        const uint32_t id = RU32(buf + off);
        const uint32_t s  = RU32(buf + off + 4);
        const size_t childAbs = absBase + off + kHdr;

        if (off + kHdr + s > sz) break;

        if (id == kChunkContainer) {
            WalkContainer(buf + off + kHdr, s, childAbs, pendingLocal, havePending, out);
        }
        else if (id == kTPKRoot) {
            // Walk the TPK_ROOT (0xB3310000) to find TEX_INFO and TEX_FORMAT.
            pendingLocal = {};
            havePending  = false;
            size_t ioff = 0;
            const uint8_t* child = buf + off + kHdr;
            while (ioff + kHdr <= s) {
                const uint32_t tid = RU32(child + ioff);
                const uint32_t ts  = RU32(child + ioff + 4);
                if (ioff + kHdr + ts > s) break;
                const size_t tChildAbs = childAbs + ioff + kHdr;
                if (tid == kTPKTexInfo) {
                    pendingLocal.texInfoOff  = tChildAbs;
                    pendingLocal.texInfoSize = ts;
                    havePending = true;
                } else if (tid == kTPKTexFormat) {
                    pendingLocal.texFmtOff  = tChildAbs;
                    pendingLocal.texFmtSize = ts;
                }
                ioff += kHdr + ts;
            }
        }
        else if (id == kTPKData) {
            // Walk CHUNK_TPK_DATA (0xB3320000) to find TPK_TEX_DATA (0x33320002).
            const size_t tpkDataHdrAbs = absBase + off; // abs offset of this header
            size_t joff = 0;
            const uint8_t* dchild = buf + off + kHdr;
            while (joff + kHdr <= s) {
                const uint32_t did = RU32(dchild + joff);
                const uint32_t ds  = RU32(dchild + joff + 4);
                if (joff + kHdr + ds > s) break;
                if (did == kTPKTexData) {
                    const size_t texDataHdrAbs = childAbs + joff; // abs offset of TPK_TEX_DATA header
                    const size_t rawContentAbs = childAbs + joff + kHdr; // content start
                    // The pixel base is 0x80-aligned from rawContentAbs (engine formula).
                    const size_t pixBase = Align80(rawContentAbs);

                    if (havePending && pendingLocal.texInfoSize > 0) {
                        TpkCtx ctx        = pendingLocal;
                        ctx.pixelBase     = pixBase;
                        ctx.tpkDataHdrOff = tpkDataHdrAbs;
                        ctx.texDataHdrOff = texDataHdrAbs;
                        ctx.valid         = true;
                        out.push_back(ctx);
                        LOG_DEBUG("BunRepacker: TPK ctx: texInfoOff=0x{:X} entries={} pixBase=0x{:X}",
                                  ctx.texInfoOff,
                                  ctx.texInfoSize / sizeof(TexEntry),
                                  ctx.pixelBase);
                    }
                    havePending  = false;
                    pendingLocal = {};
                    break;
                }
                joff += kHdr + ds;
            }
        }

        off += kHdr + s;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PatchAncestorSizes — propagate a size delta up the chunk tree
// ─────────────────────────────────────────────────────────────────────────────
// After the pixel data block grows/shrinks by `delta` bytes starting at
// `modPoint`, every ancestor chunk whose content spans `modPoint` must have its
// size field updated.  We scan the flat chunk list at [off, end) and recurse
// into the one sibling that contains `modPoint`.
// ─────────────────────────────────────────────────────────────────────────────
void BunRepacker::PatchAncestorSizes(uint8_t* buf, size_t off, size_t end,
                                      size_t modPoint, int64_t delta)
{
    static constexpr size_t kHdr = 8;
    while (off + kHdr <= end) {
        const uint32_t s   = RU32(buf + off + 4);
        const size_t   cs  = off + kHdr;
        const size_t   ce  = cs + s;
        if (ce > end) break;

        if (modPoint >= cs && modPoint < ce) {
            const uint32_t newSize = (uint32_t)((int64_t)s + delta);
            WU32(buf + off + 4, newSize);
            const size_t newCe = (size_t)((int64_t)ce + delta);
            PatchAncestorSizes(buf, cs, newCe, modPoint, delta);
            return;
        }
        off = ce;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Algorithm A — In-place patch
// ─────────────────────────────────────────────────────────────────────────────
bool BunRepacker::PatchInPlace(std::vector<uint8_t>& buf,
                                const TpkCtx& ctx,
                                const ReplaceRequest& req,
                                std::vector<uint8_t>& outOriginalBytes,
                                size_t& outFileOffset,
                                std::string& outErr)
{
    // Load the replacement DDS.
    Texture ddsTex;
    ddsTex.name     = "";
    ddsTex.nameHash = req.nameHash;
    auto r = DDSCodec::Import(ddsTex, req.ddsPath);
    if (!r) { outErr = r.error; return false; }

    const int nEntries = (int)(ctx.texInfoSize / sizeof(TexEntry));
    auto* entries = reinterpret_cast<TexEntry*>(buf.data() + ctx.texInfoOff);

    TexEntry* entry = nullptr;
    for (int i = 0; i < nEntries; ++i) {
        if (entries[i].nameHash == req.nameHash) { entry = &entries[i]; break; }
    }
    if (!entry) {
        outErr = "Hash 0x" + [&]{ char b[12]; std::snprintf(b,12,"0x%08X",req.nameHash); return std::string(b); }()
               + " not found in TPK";
        return false;
    }

    const size_t dstOff = ctx.pixelBase + entry->imagePlacement;
    if (dstOff + entry->imageSize > buf.size()) {
        outErr = "Patch region out of buffer bounds";
        return false;
    }
    if (ddsTex.data.size() > entry->imageSize) {
        // Caller should have routed this through RepackAndPatch.
        outErr = "Replacement too large for in-place patch ("
               + std::to_string(ddsTex.data.size()) + " > "
               + std::to_string(entry->imageSize) + ")";
        return false;
    }

    // Capture original bytes for the manifest.
    outFileOffset = dstOff;
    outOriginalBytes.assign(buf.data() + dstOff, buf.data() + dstOff + entry->imageSize);

    // Overwrite pixel bytes (pad remaining slot with zeros if DDS is smaller).
    const size_t copyBytes = ddsTex.data.size();
    std::memcpy(buf.data() + dstOff, ddsTex.data.data(), copyBytes);
    if (copyBytes < entry->imageSize)
        std::memset(buf.data() + dstOff + copyBytes, 0, entry->imageSize - copyBytes);

    // Update TexInfoEntry metadata to match the replacement.
    entry->width            = ddsTex.width;
    entry->height           = ddsTex.height;
    entry->shiftWidth       = ShiftFor(ddsTex.width);
    entry->shiftHeight      = ShiftFor(ddsTex.height);
    entry->compressionType  = FormatToCompType(ddsTex.format);
    entry->numMipLevels     = ddsTex.mipmaps;
    entry->imageSize        = (uint32_t)copyBytes;
    entry->baseImageSize    = (uint32_t)Mip0Size(ddsTex.format, ddsTex.width, ddsTex.height);

    // Update TPK_TEX_FORMAT table if present (D3DFORMAT FourCC per-texture).
    if (ctx.texFmtOff && ctx.texFmtSize >= 4) {
        // Determine which entry index this is.
        const int idx = (int)(entry - entries);
        const int fmtCount = (int)(ctx.texFmtSize / 4);
        if (idx < fmtCount) {
            // Map TexFormat → D3DFORMAT FourCC.
            static constexpr uint32_t kDXT1 = 0x31545844u;
            static constexpr uint32_t kDXT3 = 0x33545844u;
            static constexpr uint32_t kDXT5 = 0x35545844u;
            uint32_t fourCC = 0;
            switch (ddsTex.format) {
                case TexFormat::DXT1:   fourCC = kDXT1; break;
                case TexFormat::DXT3:   fourCC = kDXT3; break;
                case TexFormat::DXT5:   fourCC = kDXT5; break;
                case TexFormat::ARGB32: fourCC = 21u;   break; // D3DFMT_A8R8G8B8
                default: break;
            }
            if (fourCC)
                WU32(buf.data() + ctx.texFmtOff + (size_t)idx * 4, fourCC);
        }
    }

    LOG_INFO("BunRepacker: in-place patch 0x{:08X} <- '{}' [{} B @ buf+0x{:X}]",
             req.nameHash, req.ddsPath.filename().string(), copyBytes, dstOff);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Algorithm B — Full pixel-block repack
// ─────────────────────────────────────────────────────────────────────────────
bool BunRepacker::RepackAndPatch(std::vector<uint8_t>& buf,
                                  const TpkCtx& ctx,
                                  const std::vector<ReplaceRequest>& requests,
                                  std::string& outErr)
{
    const int nEntries = (int)(ctx.texInfoSize / sizeof(TexEntry));
    const TexEntry* entries = reinterpret_cast<const TexEntry*>(buf.data() + ctx.texInfoOff);

    // Build hash→request map.
    std::unordered_map<uint32_t, const ReplaceRequest*> reqMap;
    for (const auto& req : requests) reqMap[req.nameHash] = &req;

    // ── Load DDS files for all matching requests ──────────────────────────────
    struct EntryPatch {
        int                entryIdx;
        Texture            ddsTex;
        bool               needsExpansion;
    };
    std::vector<EntryPatch> patches;

    for (int i = 0; i < nEntries; ++i) {
        auto it = reqMap.find(entries[i].nameHash);
        if (it == reqMap.end()) continue;

        EntryPatch ep{};
        ep.entryIdx     = i;
        ep.ddsTex.name     = "";
        ep.ddsTex.nameHash = entries[i].nameHash;
        auto r = DDSCodec::Import(ep.ddsTex, it->second->ddsPath);
        if (!r) {
            LOG_WARN("BunRepacker::Repack: cannot load '{}': {}", it->second->ddsPath.string(), r.error);
            continue;
        }
        ep.needsExpansion = (ep.ddsTex.data.size() > (size_t)entries[i].imageSize);
        patches.push_back(std::move(ep));
    }

    bool anyExpansion = false;
    for (const auto& ep : patches) anyExpansion |= ep.needsExpansion;
    if (!anyExpansion) {
        outErr = "Repack called but no entry actually needs expansion";
        return false;
    }

    // Build entryIdx → patch map.
    std::unordered_map<int, const EntryPatch*> pmap;
    for (const auto& ep : patches) pmap[ep.entryIdx] = &ep;

    // ── Calculate new pixel data layout ───────────────────────────────────────
    std::vector<uint32_t> newPlacement((size_t)nEntries, 0u);
    uint32_t cursor = 0;
    for (int i = 0; i < nEntries; ++i) {
        newPlacement[(size_t)i] = cursor;
        size_t needed;
        auto it = pmap.find(i);
        if (it != pmap.end())
            needed = it->second->ddsTex.data.size();
        else
            needed = (size_t)entries[i].imageSize;
        cursor = (uint32_t)Align80(cursor + needed);
    }
    const size_t newPixelDataSize = cursor;

    // ── Compute old pixel data extent ─────────────────────────────────────────
    // TPK_TEX_DATA chunk header is at ctx.texDataHdrOff (8B), content starts +8.
    const size_t texDataContentStart = ctx.texDataHdrOff + 8;
    const uint32_t texDataOldSize    = RU32(buf.data() + ctx.texDataHdrOff + 4);
    const size_t oldPixelEnd         = texDataContentStart + (size_t)texDataOldSize;

    // Sanity-check header-derived values against the real buffer before trusting
    // them: a corrupted or hand-edited BUN can make texDataOldSize inconsistent
    // with the actual file size, which would otherwise underflow the size_t
    // subtraction below into a huge value and drive newBufSize to something
    // absurd (allocation failure / crash) instead of a clean failure.
    if (oldPixelEnd < ctx.pixelBase || oldPixelEnd > buf.size()) {
        outErr = "Repack failed: corrupt TPK_TEX_DATA size";
        return false;
    }
    const size_t oldPixelDataSize    = oldPixelEnd - ctx.pixelBase;

    const int64_t delta = (int64_t)newPixelDataSize - (int64_t)oldPixelDataSize;

    if (delta == 0) {
        // Sizes match exactly — just do in-place overwrites (shouldn't normally
        // reach here since we checked needsExpansion, but be safe).
        auto* mEntries = reinterpret_cast<TexEntry*>(buf.data() + ctx.texInfoOff);
        for (const auto& ep : patches) {
            const size_t dstOff = ctx.pixelBase + mEntries[ep.entryIdx].imagePlacement;
            if (dstOff + ep.ddsTex.data.size() <= buf.size())
                std::memcpy(buf.data() + dstOff, ep.ddsTex.data.data(), ep.ddsTex.data.size());
        }
        return true;
    }

    // ── Allocate new buffer ───────────────────────────────────────────────────
    const size_t newBufSize = (size_t)((int64_t)buf.size() + delta);
    std::vector<uint8_t> newBuf(newBufSize, 0u);

    // Copy everything before the pixel data block.
    std::memcpy(newBuf.data(), buf.data(), ctx.pixelBase);

    // ── Write new pixel block ─────────────────────────────────────────────────
    for (int i = 0; i < nEntries; ++i) {
        const size_t dstOff = ctx.pixelBase + newPlacement[(size_t)i];
        auto it = pmap.find(i);
        if (it != pmap.end()) {
            const auto& ep = *it->second;
            const size_t n = ep.ddsTex.data.size();
            if (dstOff + n <= newBufSize)
                std::memcpy(newBuf.data() + dstOff, ep.ddsTex.data.data(), n);
        } else {
            // Original texture: copy from old buffer.
            const size_t srcOff  = ctx.pixelBase + (size_t)entries[i].imagePlacement;
            const size_t copyLen = (size_t)entries[i].imageSize;
            if (srcOff + copyLen <= buf.size() && dstOff + copyLen <= newBufSize)
                std::memcpy(newBuf.data() + dstOff, buf.data() + srcOff, copyLen);
        }
    }

    // Copy everything after the old pixel data block (unchanged suffix).
    if (oldPixelEnd < buf.size()) {
        const size_t suffixLen  = buf.size() - oldPixelEnd;
        const size_t newSufOff  = ctx.pixelBase + newPixelDataSize;
        if (newSufOff + suffixLen <= newBufSize)
            std::memcpy(newBuf.data() + newSufOff, buf.data() + oldPixelEnd, suffixLen);
    }

    // ── Update TexInfoEntry in new buffer ─────────────────────────────────────
    auto* mEntries = reinterpret_cast<TexEntry*>(newBuf.data() + ctx.texInfoOff);
    for (int i = 0; i < nEntries; ++i) {
        mEntries[i].imagePlacement = newPlacement[(size_t)i];
        auto it = pmap.find(i);
        if (it != pmap.end() && it->second->needsExpansion) {
            const auto& ep = *it->second;
            mEntries[i].width           = ep.ddsTex.width;
            mEntries[i].height          = ep.ddsTex.height;
            mEntries[i].shiftWidth      = ShiftFor(ep.ddsTex.width);
            mEntries[i].shiftHeight     = ShiftFor(ep.ddsTex.height);
            mEntries[i].compressionType = FormatToCompType(ep.ddsTex.format);
            mEntries[i].numMipLevels    = ep.ddsTex.mipmaps;
            mEntries[i].imageSize       = (uint32_t)ep.ddsTex.data.size();
            mEntries[i].baseImageSize   = (uint32_t)Mip0Size(ep.ddsTex.format,
                                                               ep.ddsTex.width,
                                                               ep.ddsTex.height);
        }
    }

    // ── Update ancestor chunk size fields ─────────────────────────────────────
    PatchAncestorSizes(newBuf.data(), 0, newBufSize, ctx.pixelBase, delta);

    buf = std::move(newBuf);

    LOG_INFO("BunRepacker: repack complete — delta {:+d} bytes (new size {} bytes)",
             (long long)delta, buf.size());
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// PatchFile — public entry point
// ─────────────────────────────────────────────────────────────────────────────
std::vector<PatchResult> BunRepacker::PatchFile(
    const std::filesystem::path& filePath,
    const std::vector<ReplaceRequest>& requests,
    BackupManager& bm,
    std::function<void(float)> progress)
{
    std::vector<PatchResult> results(requests.size());
    for (size_t i = 0; i < requests.size(); ++i)
        results[i].nameHash = requests[i].nameHash;

    if (requests.empty()) return results;

    if (progress) progress(0.0f);

    // ── Remove any orphaned .tmp_patch from a previously-crashed run ─────
    // Such a file could exist if a prior PatchFile call was killed after the
    // temp-write started but before the rename/copy succeeded.  If left on disk
    // it wastes space and can confuse a subsequent rename on some platforms.
    {
        const auto tmpPath = std::filesystem::path(filePath.string() + ".tmp_patch");
        std::error_code ecOrphan;
        if (std::filesystem::exists(tmpPath, ecOrphan)) {
            LOG_WARN("BunRepacker: removing orphaned temp file '{}'",
                     tmpPath.filename().string());
            std::filesystem::remove(tmpPath, ecOrphan);
        }
    }

    // ── Read whole file into memory ───────────────────────────────────────────
    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) {
        for (auto& r : results) r.error = "Cannot open file";
        return results;
    }
    const auto fileSize = (size_t)in.tellg();
    if (fileSize == 0) {
        for (auto& r : results) r.error = "File is empty";
        return results;
    }
    in.seekg(0);
    std::vector<uint8_t> buf(fileSize);
    in.read(reinterpret_cast<char*>(buf.data()), (std::streamsize)fileSize);
    in.close();

    if (progress) progress(0.10f);

    // ── Ensure .bak before any write ─────────────────────────────────────────
    if (!bm.EnsureFileBak(filePath)) {
        for (auto& r : results) r.error = "Failed to create .bak — aborting";
        return results;
    }

    if (progress) progress(0.15f);

    // ── Pass 1: determine which requests need repack ──────────────────────────
    // We re-collect TPKs after each repack because buffer offsets shift.
    // Iterate until stable (loop borrowed from mwml BunPatcher::patch).
    bool didRepack = true;
    while (didRepack) {
        didRepack = false;
        std::vector<TpkCtx> tpks;
        CollectTPKs(buf.data(), buf.size(), tpks);

        for (auto& ctx : tpks) {
            if (!ctx.valid) continue;
            const int nEntries = (int)(ctx.texInfoSize / sizeof(TexEntry));
            const auto* entries = reinterpret_cast<const TexEntry*>(buf.data() + ctx.texInfoOff);

            // Check whether any request in this TPK needs more space.
            bool needsRepack = false;
            for (const auto& req : requests) {
                for (int i = 0; i < nEntries; ++i) {
                    if (entries[i].nameHash != req.nameHash) continue;
                    // Quick-load to get pixel size.
                    Texture tmp;
                    tmp.nameHash = req.nameHash;
                    if (DDSCodec::Import(tmp, req.ddsPath)) {
                        if (tmp.data.size() > (size_t)entries[i].imageSize) {
                            needsRepack = true;
                        }
                    }
                    break;
                }
                if (needsRepack) break;
            }

            if (needsRepack) {
                std::string err;
                if (RepackAndPatch(buf, ctx, requests, err)) {
                    didRepack = true;
                    // Mark results that were fulfilled by this repack.
                    for (size_t ri = 0; ri < requests.size(); ++ri)
                        results[ri].usedRepack = true;
                    break; // restart with fresh TpkCtxs (offsets changed)
                } else {
                    LOG_ERROR("BunRepacker: repack failed: {}", err);
                    for (auto& r : results) r.error = "Repack failed: " + err;
                    return results;
                }
            }
        }
    }

    if (progress) progress(0.50f);

    // ── Pass 2: in-place pixel copy + metadata update ─────────────────────────
    std::vector<TpkCtx> tpks;
    CollectTPKs(buf.data(), buf.size(), tpks);

    if (tpks.empty()) {
        for (auto& r : results) r.error = "No TPKs found in file";
        return results;
    }

    // Per-request results.
    const float step = 0.40f / (float)std::max<size_t>(1, requests.size());
    for (size_t ri = 0; ri < requests.size(); ++ri) {
        const auto& req = requests[ri];
        bool found = false;
        for (const auto& ctx : tpks) {
            if (!ctx.valid) continue;
            std::vector<uint8_t> origBytes;
            size_t fileOff = 0;
            std::string err;
            if (PatchInPlace(buf, ctx, req, origBytes, fileOff, err)) {
                // Record original region in the manifest.
                bm.RecordRegion(filePath, fileOff, origBytes);
                results[ri].ok = true;
                found = true;
                break;
            }
        }
        if (!found && results[ri].error.empty())
            results[ri].error = "Hash not found in any TPK";
        if (progress) progress(0.50f + (float)(ri + 1) * step);
    }

    if (progress) progress(0.90f);

    // ── Write result back to disk ─────────────────────────────────────────────
    // Write to a temp file beside the original, then rename over it.
    const auto tmpPath = std::filesystem::path(filePath.string() + ".tmp_patch");
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            for (auto& r : results) r.error = "Cannot open temp file for writing";
            return results;
        }
        out.write(reinterpret_cast<const char*>(buf.data()), (std::streamsize)buf.size());
        if (!out) {
            std::error_code ec;
            std::filesystem::remove(tmpPath, ec);
            for (auto& r : results) r.error = "Write error";
            return results;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath, ec);
    if (ec) {
        // rename failed (e.g. cross-device) — fall back to copy + remove.
        std::filesystem::copy_file(tmpPath, filePath,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tmpPath);
        if (ec) {
            for (auto& r : results) r.error = "Failed to write patched file: " + ec.message();
            return results;
        }
    }

    if (progress) progress(1.0f);

    const int ok = (int)std::count_if(results.begin(), results.end(),
                                       [](const PatchResult& r){ return r.ok; });
    LOG_INFO("BunRepacker: {}/{} patch(es) applied to '{}'",
             ok, (int)requests.size(), filePath.filename().string());
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// RevertFile
// ─────────────────────────────────────────────────────────────────────────────
int BunRepacker::RevertFile(const std::filesystem::path& filePath, BackupManager& bm) {
    return bm.RevertAll(filePath);
}

} // namespace nfsmw
