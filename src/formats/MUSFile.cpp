// ─── formats/MUSFile.cpp ─────────────────────────────────────────────────────
// EA MUS music stream parser for NFS:MW (MW_Music.mus + MW_Music.mpf).
//
// A .mus is a concatenation of independent EA SCHl streams ("segments"),
// each: SCHl (variable header) → SCCl → SCDl... (blocked audio) → SCEl, with
// padding to the next SCHl.  NFSMW music uses a GSTR/"PT" header, version 3,
// codec EA-XA v2 (split mono per channel), 2 or 4 channels.
//
// Block walk: at each block, id = u32be@+0x00, size = u32le@+0x04 (size INCLUDES
//   the 8-byte header); advance by size.  Audio lives in SCDl blocks:
//     [+0x08] block_samples           (big-endian for this platform)
//     [+0x0C] channel start table     (channels × u32be, relative to data base)
//     data base = +0x0C + channels*4; channel c data = [base+start[c], base+start[c+1])
//
// The .mus can be huge (MW_Music.mus ≈ 533 MB / 3257 segments), so segments are
// enumerated lazily (headers only) and each segment's audio is read from disk
// and decoded on demand in DecodePCM().
//
// References: vgmstream meta/ea_schl.c, layout/blocked_ea_schl.c,
//             coding/ea_xa_decoder.c (decode_ea_xa_v2).
// ─────────────────────────────────────────────────────────────────────────────
#include "formats/MUSFile.h"
#include "formats/ABKFile.h"   // DecodeEAXA (EA-XA v2)
#include "formats/AudioCodecs.h"
#include "core/Logger.h"

#include <fstream>
#include <cstring>
#include <algorithm>

namespace nfsmw {

namespace {

inline uint32_t ReadU32LE(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
inline uint32_t ReadU32BE(const uint8_t* p) {
    return ((uint32_t)p[0]<<24) | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | (uint32_t)p[3];
}
inline uint16_t ReadU16LE(const uint8_t* p) { return (uint16_t)(p[0] | p[1]<<8); }
inline bool IsId(const uint8_t* p, const char* id) { return std::memcmp(p, id, 4) == 0; }

// EA read_patch (see ABKFile.cpp / vgmstream ea_schl.c::read_patch).
uint32_t ReadPatch(const uint8_t* d, size_t n, size_t& off) {
    if (off >= n) return 0;
    uint8_t bc = d[off++];
    if (bc == 0xFF) { if (off + 4 > n) { off = n; return 0; } uint32_t sz = ReadU32BE(d + off); off += 4 + sz; return 0; }
    if (bc > 4) { off += bc; return 0; }
    uint32_t r = 0;
    for (; bc > 0 && off < n; --bc) r = (r << 8) + d[off++];
    return r;
}

struct SchlInfo {
    uint16_t platform   = 0xFFFF;
    int      version    = -1;
    int      codec1     = -1;
    int      codec2     = -1;
    uint32_t channels   = 0;
    uint32_t sampleRate = 0;
    uint32_t numSamples = 0;
    uint32_t loopStart  = 0;
    uint32_t loopEnd    = 0;
};

// Parse an SCHl variable header. `buf` points at the SCHl block start, `size` is
// the SCHl block size (includes 8-byte header). Returns false on bad magic.
bool ParseSchlHeader(const uint8_t* buf, size_t size, SchlInfo& out) {
    if (size < 0x10 || !IsId(buf, "SCHl")) return false;
    size_t p = 0x08;  // variable header begins after id+size
    if (p + 4 <= size && IsId(buf + p, "GSTR")) { out.platform = 8 /*generic*/; p += 8; }
    else if (p + 4 <= size && buf[p] == 'P' && buf[p+1] == 'T') { out.platform = ReadU16LE(buf + p + 2); p += 4; }
    else return false;

    bool end = false;
    while (!end && p < size) {
        uint8_t tag = buf[p++];
        switch (tag) {
            case 0xFF: case 0xFE: end = true; break;
            case 0xFC: case 0xFD: break;
            case 0x80: out.version    = (int)ReadPatch(buf, size, p); break;
            case 0x82: out.channels   = ReadPatch(buf, size, p); break;
            case 0x83: out.codec1     = (int)ReadPatch(buf, size, p); break;
            case 0xA0: out.codec2     = (int)ReadPatch(buf, size, p); break;
            case 0x84: out.sampleRate = ReadPatch(buf, size, p); break;
            case 0x85: out.numSamples = ReadPatch(buf, size, p); break;
            case 0x86: out.loopStart  = ReadPatch(buf, size, p); break;
            case 0x87: out.loopEnd    = ReadPatch(buf, size, p) + 1; break;
            default:   ReadPatch(buf, size, p); break;
        }
    }
    if (out.channels == 0) out.channels = 1;
    if (out.sampleRate == 0) out.sampleRate = 48000;  // generic default
    return true;
}

// Resolve to a MUSCodec value (subset; NFSMW music is EA-XA).
uint8_t ResolveMusCodec(const SchlInfo& s) {
    int c2 = s.codec2;
    if (c2 < 0 && s.codec1 >= 0) {
        if (s.codec1 == 0x07) c2 = 0x0A;  // EAXA
        else if (s.codec1 == 0x00) c2 = 0x00; // PCM
    }
    if (c2 < 0) c2 = 0x0A;  // generic/PC default = EA-XA
    switch (c2) {
        case 0x00: case 0x01:                       return MUSCodec::PCM;
        case 0x04: case 0x07: case 0x08:            return MUSCodec::MICROTALK; // EA MicroTalk speech (copspeech.big)
        case 0x03: case 0x0A:                       return MUSCodec::EA_XA;
        case 0x0F: case 0x10: case 0x17:            return MUSCodec::EA_MP3;
        default:                                    return MUSCodec::EA_XA;
    }
}

// Read `len` bytes at `off` from an open stream into buf (resized).
bool ReadAt(std::ifstream& f, std::streamoff off, size_t len, std::vector<uint8_t>& buf) {
    buf.resize(len);
    f.clear();
    f.seekg(off);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(len));
    return static_cast<size_t>(f.gcount()) == len;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// MUSParser::Load — enumerate SCHl segments (lazy: headers only).
// ─────────────────────────────────────────────────────────────────────────────
Result<MUSFile> MUSParser::Load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return Result<MUSFile>::Err("Cannot open MUS: " + path.string());
    const std::streamoff fileSize = f.tellg();
    f.seekg(0);

    MUSFile mus;
    mus.path = path;
    mus.name = path.stem().string();

    // Locate first SCHl (skip any leading header, e.g. FACE A58C + padding).
    std::vector<uint8_t> window;
    if (!ReadAt(f, 0, std::min<std::streamoff>(0x1000, fileSize), window))
        return Result<MUSFile>::Err("MUS read failed");
    size_t firstSchl = SIZE_MAX;
    for (size_t i = 0; i + 4 <= window.size(); ++i)
        if (IsId(window.data() + i, "SCHl")) { firstSchl = i; break; }
    if (firstSchl == SIZE_MAX)
        return Result<MUSFile>::Err("MUS: no SCHl segment found");

    std::vector<uint8_t> hdr, b8(8);
    std::streamoff off = static_cast<std::streamoff>(firstSchl);
    uint32_t index = 0;

    while (off + 8 <= fileSize) {
        if (!ReadAt(f, off, 8, b8)) break;
        if (!IsId(b8.data(), "SCHl")) {
            // Realign: scan forward for the next SCHl within a small window.
            std::vector<uint8_t> w;
            std::streamoff scanLen = std::min<std::streamoff>(0x2000, fileSize - off);
            if (scanLen < 4 || !ReadAt(f, off, scanLen, w)) break;
            size_t fnd = SIZE_MAX;
            for (size_t i = 0; i + 4 <= w.size(); ++i)
                if (IsId(w.data() + i, "SCHl")) { fnd = i; break; }
            if (fnd == SIZE_MAX) break;
            off += static_cast<std::streamoff>(fnd);
            continue;
        }
        uint32_t schlSize = ReadU32LE(b8.data() + 4);
        if (schlSize < 0x10 || off + schlSize > fileSize) break;

        // Read + parse the SCHl variable header.
        if (!ReadAt(f, off, schlSize, hdr)) break;
        SchlInfo si;
        if (!ParseSchlHeader(hdr.data(), schlSize, si)) break;

        // Walk this segment's blocks to find its SCEl (segment end).
        std::streamoff bpos = off;
        std::streamoff segEnd = off;
        bool sawEnd = false;
        size_t guard = 0;
        while (bpos + 8 <= fileSize && guard++ < 2'000'000) {
            if (!ReadAt(f, bpos, 8, b8)) break;
            uint32_t bsz = ReadU32LE(b8.data() + 4);
            if (bsz < 8 || bpos + bsz > fileSize) break;
            bpos += static_cast<std::streamoff>(bsz);
            if (IsId(b8.data(), "SCEl")) { segEnd = bpos; sawEnd = true; break; }
        }
        if (!sawEnd) segEnd = bpos;

        MUSSection sec;
        sec.index       = index++;
        sec.fileOffset  = static_cast<size_t>(off);
        sec.blockSpan   = static_cast<size_t>(segEnd - off);
        sec.srcPath     = path;
        sec.sampleRate  = si.sampleRate;
        sec.channels    = static_cast<uint8_t>(std::min<uint32_t>(si.channels, 8));
        sec.numSamples  = si.numSamples;
        sec.compression = ResolveMusCodec(si);
        bool loop       = (si.loopEnd > 0);
        sec.loopStart   = loop ? si.loopStart : 0xFFFFFFFFu;
        sec.loopLength  = (loop && si.loopEnd > si.loopStart) ? (si.loopEnd - si.loopStart) : 0;
        mus.sections.push_back(std::move(sec));

        off = segEnd;
    }

    if (mus.sections.empty())
        return Result<MUSFile>::Err("MUS: no valid SCHl segments parsed");

    LOG_INFO("[MUS] Loaded {} — {} segments", mus.name, mus.sections.size());
    return Result<MUSFile>::Ok(std::move(mus));
}

// ─────────────────────────────────────────────────────────────────────────────
// MUSParser::DecodePCM — read one segment from disk, de-block, decode.
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<int16_t>> MUSParser::DecodePCM(const MUSSection& sec,
                                                    uint32_t& outRate,
                                                    uint32_t& outChannels) {
    outRate     = sec.sampleRate;
    outChannels = sec.channels;

    if (sec.blockSpan == 0 || sec.srcPath.empty())
        return Result<std::vector<int16_t>>::Err("MUS: section has no source data");

    std::ifstream f(sec.srcPath, std::ios::binary);
    if (!f) return Result<std::vector<int16_t>>::Err("Cannot open MUS: " + sec.srcPath.string());

    std::vector<uint8_t> seg;
    if (!ReadAt(f, static_cast<std::streamoff>(sec.fileOffset), sec.blockSpan, seg))
        return Result<std::vector<int16_t>>::Err("MUS: failed to read segment data");

    const uint32_t channels = sec.channels ? sec.channels : 1;

    // De-block: gather each channel's contiguous compressed bytes across SCDl blocks.
    std::vector<std::vector<uint8_t>> chData(channels);
    size_t pos = 0;
    while (pos + 8 <= seg.size()) {
        uint32_t bsz = ReadU32LE(seg.data() + pos + 4);
        if (bsz < 8 || pos + bsz > seg.size()) break;
        if (IsId(seg.data() + pos, "SCEl")) break;
        if (IsId(seg.data() + pos, "SCDl")) {
            const size_t base = pos + 0x0C + 4 * (size_t)channels;
            if (base <= pos + bsz) {
                // block_samples (LE on PC) is the per-channel sample count for
                // THIS block; every channel encodes the same number of samples.
                const uint32_t blockSamples = ReadU32LE(seg.data() + pos + 0x08);
                for (uint32_t c = 0; c < channels; ++c) {
                    // Channel-start offsets are little-endian on the PC platform
                    // (verified against 6-channel NIS streams: 0x0000, 0x06B2,
                    // 0x0D64, … — big-endian would give absurd offsets). Mono
                    // segments have ch0=0, which is why this was masked before.
                    uint32_t startC = ReadU32LE(seg.data() + pos + 0x0C + 4 * c);
                    uint32_t startN = (c + 1 < channels)
                                    ? ReadU32LE(seg.data() + pos + 0x0C + 4 * (c + 1))
                                    : (uint32_t)((pos + bsz) - base);
                    size_t a = base + startC;
                    size_t e = base + startN;
                    if (e > pos + bsz) e = pos + bsz;

                    // Each channel's per-block payload is aligned/padded (verified:
                    // NISAudio.big stores every EA-XA sub-block's 1575 real bytes
                    // as 1576 — a 1-byte pad to an even boundary). Those pad bytes
                    // are NOT codec data; concatenating them between blocks shifts
                    // the EA-XA frame stream by a byte at every block boundary, so
                    // the ADPCM decoder desyncs and playback is choppy/gapped.
                    // Trim to the exact frame span: block_samples -> ceil(n/28)
                    // frames of 15 bytes (or 61 for an uncompressed 0xEE block).
                    // If the count doesn't add up (e.g. a non-EA-XA layout), leave
                    // the slice untouched so nothing regresses.
                    if (sec.compression == MUSCodec::EA_XA && blockSamples > 0 && a < e) {
                        const size_t frames = (static_cast<size_t>(blockSamples) + 27u) / 28u;
                        size_t walk  = 0;
                        bool   exact = true;
                        for (size_t fr = 0; fr < frames; ++fr) {
                            if (a + walk >= e) { exact = false; break; }
                            const size_t fb = (seg[a + walk] == 0xEE) ? 61u : 15u;
                            if (a + walk + fb > e) { exact = false; break; }
                            walk += fb;
                        }
                        if (exact) e = a + walk;  // drop the trailing alignment pad
                    }

                    if (a < e && e <= seg.size())
                        chData[c].insert(chData[c].end(), seg.begin() + a, seg.begin() + e);
                }
            }
        }
        pos += bsz;
    }

    switch (sec.compression) {
        case MUSCodec::EA_XA: {
            std::vector<std::vector<int16_t>> dec(channels);
            size_t minLen = SIZE_MAX;
            for (uint32_t c = 0; c < channels; ++c) {
                dec[c] = DecodeEAXA(chData[c].data(), chData[c].size(), 1, sec.numSamples);
                minLen = std::min(minLen, dec[c].size());
            }
            if (minLen == SIZE_MAX || minLen == 0)
                return Result<std::vector<int16_t>>::Err("MUS: EA-XA decode produced no samples");
            std::vector<int16_t> out;
            out.reserve(minLen * channels);
            for (size_t i = 0; i < minLen; ++i)
                for (uint32_t c = 0; c < channels; ++c)
                    out.push_back(dec[c][i]);
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }
        case MUSCodec::PCM: {
            // Interleaved/contiguous PCM16 per channel.
            std::vector<std::vector<int16_t>> dec(channels);
            size_t minLen = SIZE_MAX;
            for (uint32_t c = 0; c < channels; ++c) {
                const size_t n = chData[c].size() / 2;
                dec[c].resize(n);
                std::memcpy(dec[c].data(), chData[c].data(), n * 2);
                minLen = std::min(minLen, n);
            }
            if (minLen == SIZE_MAX || minLen == 0)
                return Result<std::vector<int16_t>>::Err("MUS: no PCM samples");
            std::vector<int16_t> out;
            out.reserve(minLen * channels);
            for (size_t i = 0; i < minLen; ++i)
                for (uint32_t c = 0; c < channels; ++c)
                    out.push_back(dec[c][i]);
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }
        case MUSCodec::MICROTALK:
            // copspeech.big streams are EA MicroTalk (variable-bitrate speech,
            // codec2 tag 0x04 - verified ~1.4-1.8 bits/sample, not EA-XA). MWTools
            // has no MicroTalk decoder yet; decoding it as EA-XA (the old default)
            // produced the loud/choppy garbage. Fail clearly instead.
            return Result<std::vector<int16_t>>::Err(
                "MUS: EA MicroTalk speech (codec 0x04, e.g. copspeech.big) needs a "
                "MicroTalk decoder that is not yet implemented");
        default:
            return Result<std::vector<int16_t>>::Err(
                std::string("MUS: codec '") + sec.CodecName() + "' not supported for playback yet");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// MUSParser::ReadWAV — read a standard RIFF/WAVE (16-bit PCM) file.
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<int16_t>> MUSParser::ReadWAV(const std::filesystem::path& path,
                                                  uint32_t& outRate,
                                                  uint32_t& outChannels)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return Result<std::vector<int16_t>>::Err("Cannot open WAV: " + path.string());
    const size_t sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> d(sz);
    f.read(reinterpret_cast<char*>(d.data()), static_cast<std::streamsize>(sz));

    if (sz < 44 || std::memcmp(d.data(), "RIFF", 4) != 0 || std::memcmp(d.data() + 8, "WAVE", 4) != 0)
        return Result<std::vector<int16_t>>::Err("Not a valid WAV file");

    auto u16 = [&](size_t o){ return (uint16_t)(d[o] | d[o+1] << 8); };
    auto u32 = [&](size_t o){ return (uint32_t)(d[o] | d[o+1] << 8 | d[o+2] << 16 | (uint32_t)d[o+3] << 24); };

    uint16_t fmt = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    size_t dataOff = 0, dataLen = 0;
    size_t p = 12;
    while (p + 8 <= sz) {
        const uint32_t id   = u32(p);
        const uint32_t clen = u32(p + 4);
        const size_t   body = p + 8;
        if (id == 0x20746d66u /* "fmt " */ && body + 16 <= sz) {
            fmt = u16(body); channels = u16(body + 2); rate = u32(body + 4); bits = u16(body + 14);
        } else if (id == 0x61746164u /* "data" */) {
            dataOff = body; dataLen = std::min<size_t>(clen, sz - body);
        }
        p = body + clen + (clen & 1);  // word-aligned chunks
    }

    if (channels == 0 || rate == 0 || dataOff == 0)
        return Result<std::vector<int16_t>>::Err("WAV missing fmt/data chunk");
    if (fmt != 1 || bits != 16)
        return Result<std::vector<int16_t>>::Err("Only 16-bit PCM WAV is supported");

    outRate     = rate;
    outChannels = channels;

    const size_t frames = dataLen / (2u * channels);
    std::vector<int16_t> pcm(frames * channels);
    std::memcpy(pcm.data(), d.data() + dataOff, frames * channels * 2);
    return Result<std::vector<int16_t>>::Ok(std::move(pcm));
}

// ─────────────────────────────────────────────────────────────────────────────
// MUSParser::WriteWAV — write interleaved s16 PCM to a standard RIFF/WAVE.
// ─────────────────────────────────────────────────────────────────────────────
bool MUSParser::WriteWAV(const std::filesystem::path& dst,
                          const std::vector<int16_t>& pcm,
                          uint32_t sampleRate,
                          uint32_t channels)
{
    if (pcm.empty() || channels == 0 || sampleRate == 0) return false;
    std::ofstream f(dst, std::ios::binary);
    if (!f) return false;

    const uint32_t dataBytes  = static_cast<uint32_t>(pcm.size() * 2);
    const uint32_t byteRate   = sampleRate * channels * 2;
    const uint16_t blockAlign = static_cast<uint16_t>(channels * 2);

    auto w16 = [&](uint16_t v){ f.write(reinterpret_cast<const char*>(&v), 2); };
    auto w32 = [&](uint32_t v){ f.write(reinterpret_cast<const char*>(&v), 4); };

    f.write("RIFF", 4); w32(36 + dataBytes);
    f.write("WAVE", 4);
    f.write("fmt ", 4); w32(16);
    w16(1); w16(static_cast<uint16_t>(channels));
    w32(sampleRate); w32(byteRate);
    w16(blockAlign); w16(16);
    f.write("data", 4); w32(dataBytes);
    f.write(reinterpret_cast<const char*>(pcm.data()), static_cast<std::streamsize>(dataBytes));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// MUSParser::ReplaceSection — re-encode one section and patch the MUS file.
//
// Strategy:
//   1. De-interleave the caller's PCM into per-channel mono streams.
//   2. Re-encode each channel with EncodeEAXA (EA-XA v2, 15 bytes / 28 samples).
//   3. Build a new SCHl block (clone the original, update numSamples tag 0x85).
//   4. Pack all encoded channel data into a single SCDl block with the standard
//      channel-start-offset table at [+0x0C].
//   5. Append a zero-payload SCEl block (8 bytes).
//   6. Read the MUS file, splice out the old segment bytes, insert the new
//      segment, write the result back to disk.
//   7. Reload the MUSFile in-place so callers get updated offsets/numSamples.
// ─────────────────────────────────────────────────────────────────────────────
Result<AudioCodecs::ReplaceReport> MUSParser::ReplaceSection(MUSFile& mus,
                                        size_t sectionIdx,
                                        const std::vector<int16_t>& pcmInterleaved,
                                        uint32_t pcmRate,
                                        uint32_t pcmChannels,
                                        const AudioCodecs::ProgressFn& onProgress)
{
    if (sectionIdx >= mus.sections.size())
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceSection: index out of range");

    const MUSSection& sec = mus.sections[sectionIdx];

    if (sec.compression == MUSCodec::EA_MP3)
        return Result<AudioCodecs::ReplaceReport>::Err(
            "EA-MP3 sections cannot be re-encoded (no MP3 encoder available). "
            "Use whole-file replace for this section.");

    if (sec.compression != MUSCodec::EA_XA && sec.compression != MUSCodec::PCM)
        // Only EA-XA and PCM sections have encoders. Re-encoding e.g. a MicroTalk
        // (copspeech) section as EA-XA would rewrite it in the wrong codec and
        // corrupt the stream, so refuse rather than silently mangle it.
        return Result<AudioCodecs::ReplaceReport>::Err(
            std::string("ReplaceSection: '") + sec.CodecName() +
            "' sections cannot be re-encoded - no matching encoder");

    const uint32_t channels = pcmChannels > 0 ? pcmChannels : 1;

    if (pcmInterleaved.empty())
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceSection: empty PCM input");

    // ── Build report and resample if the source rate differs from the section's fixed rate ──
    AudioCodecs::ReplaceReport report;
    report.srcRate     = pcmRate;
    report.dstRate     = sec.sampleRate;
    report.srcChannels = channels;
    report.dstChannels = channels;

    const std::vector<int16_t>* working = &pcmInterleaved;
    std::vector<int16_t> resampled;
    if (pcmRate != 0 && sec.sampleRate != 0 && pcmRate != sec.sampleRate) {
        resampled = AudioCodecs::Resample(pcmInterleaved, channels, pcmRate, sec.sampleRate);
        working   = &resampled;
        report.resampled = true;
    }

    const size_t frames = working->size() / channels;

    if (frames == 0)
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceSection: empty PCM input after resample");

    // ── 1. De-interleave into per-channel mono buffers ───────────────────────
    std::vector<std::vector<int16_t>> chPCM(channels);
    for (uint32_t c = 0; c < channels; ++c) {
        chPCM[c].resize(frames);
        for (size_t i = 0; i < frames; ++i)
            chPCM[c][i] = (*working)[i * channels + c];
    }

    // ── 2. Encode each channel to EA-XA v2 via centralised dispatch ──────────
    std::vector<std::vector<uint8_t>> chEnc(channels);
    for (uint32_t c = 0; c < channels; ++c) {
        // Sub-divide progress evenly across channels
        AudioCodecs::ProgressFn chProg;
        if (onProgress) {
            chProg = [&onProgress, c, channels](float f) {
                onProgress((static_cast<float>(c) + f) / static_cast<float>(channels));
            };
        }
        auto encRes = AudioCodecs::Encode(AudioCodecs::Codec::EA_XA_V2,
                                           chPCM[c].data(), chPCM[c].size(), 1, chProg);
        if (!encRes)
            return Result<AudioCodecs::ReplaceReport>::Err(
                "ReplaceSection encode ch" + std::to_string(c) + ": " + encRes.error);
        chEnc[c] = std::move(encRes.value);
    }

    // Round-trip verification on channel 0 as representative sample
    report.verify = AudioCodecs::VerifyRoundTrip(
        AudioCodecs::Codec::EA_XA_V2, chEnc[0], 1,
        static_cast<uint32_t>(chPCM[0].size()), chPCM[0],
        report.dstRate > 0 ? report.dstRate : pcmRate);

    // ── 3. Read the original SCHl block to use as a template ─────────────────
    std::ifstream fin(mus.path, std::ios::binary | std::ios::ate);
    if (!fin) return Result<AudioCodecs::ReplaceReport>::Err("Cannot open MUS for reading: " + mus.path.string());
    const std::streamoff fileSize = fin.tellg();

    std::vector<uint8_t> origSchl;
    {
        const size_t schlOff = sec.fileOffset;
        std::vector<uint8_t> hdr8(8);
        fin.seekg(static_cast<std::streamoff>(schlOff));
        fin.read(reinterpret_cast<char*>(hdr8.data()), 8);
        const uint32_t schlSize = ReadU32LE(hdr8.data() + 4);
        if (!ReadAt(fin, static_cast<std::streamoff>(schlOff), schlSize, origSchl))
            return Result<AudioCodecs::ReplaceReport>::Err("ReplaceSection: cannot read original SCHl");
    }

    // ── 4. Patch numSamples (tag 0x85) in the SCHl variable header ──────────
    // We scan for the 0x85 patch tag and rewrite its value in-place.
    // The value encoding used by NFSMW is always 4-byte (bc=4, value big-endian).
    {
        size_t p = 0x08;
        // Skip platform marker (GSTR 8 bytes, or PT 4 bytes)
        if (p + 4 <= origSchl.size()) {
            if (std::memcmp(origSchl.data() + p, "GSTR", 4) == 0) p += 8;
            else if (origSchl[p] == 'P' && origSchl[p+1] == 'T') p += 4;
        }
        bool patched = false;
        while (p < origSchl.size()) {
            uint8_t tag = origSchl[p++];
            if (tag == 0xFF || tag == 0xFE) break;
            if (tag == 0xFC || tag == 0xFD) continue;
            if (p >= origSchl.size()) break;
            uint8_t bc = origSchl[p];
            if (bc == 0xFF) { p++; if (p + 4 <= origSchl.size()) { uint32_t skip = ReadU32BE(origSchl.data() + p); p += 4 + skip; } break; }
            if (bc > 4)     { p += 1 + bc; continue; }
            if (tag == 0x85 && bc == 4 && p + 5 <= origSchl.size()) {
                // Overwrite the 4-byte big-endian sample count
                const uint32_t ns = static_cast<uint32_t>(frames);
                origSchl[p+1] = (ns >> 24) & 0xFF;
                origSchl[p+2] = (ns >> 16) & 0xFF;
                origSchl[p+3] = (ns >>  8) & 0xFF;
                origSchl[p+4] = (ns      ) & 0xFF;
                patched = true;
                break;
            }
            p += 1 + bc;  // skip tag byte + value bytes
        }
        if (!patched) {
            // Tag not present or not 4-byte; we can still proceed — the decoder
            // uses numSamples only as a cap, so a missing/stale value is safe.
            LOG_WARN("[MUS] ReplaceSection: could not patch numSamples in SCHl header");
        }
    }

    // ── 5. Build new SCDl block ───────────────────────────────────────────────
    // Layout: [4 id][4 size][4 blockSamples][ch×4 startOffsets][ch data...]
    // All multi-byte values in the channel-start table are little-endian (PC).
    const size_t headerBytes = 0x0C + channels * 4u;  // id+size+blockSamples + table
    size_t totalChData = 0;
    for (uint32_t c = 0; c < channels; ++c) totalChData += chEnc[c].size();
    const size_t scdlPayload = headerBytes - 8u + totalChData;  // body (excluding id+size)
    const uint32_t scdlSize  = static_cast<uint32_t>(8u + scdlPayload);

    std::vector<uint8_t> scdl(scdlSize, 0);
    std::memcpy(scdl.data(), "SCDl", 4);
    // size field (LE)
    scdl[4] = scdlSize & 0xFF; scdl[5] = (scdlSize >> 8) & 0xFF;
    scdl[6] = (scdlSize >> 16) & 0xFF; scdl[7] = (scdlSize >> 24) & 0xFF;
    // block_samples field [+0x08] (LE) — total frames
    const uint32_t bs32 = static_cast<uint32_t>(frames);
    scdl[8] = bs32 & 0xFF; scdl[9] = (bs32 >> 8) & 0xFF;
    scdl[10] = (bs32 >> 16) & 0xFF; scdl[11] = (bs32 >> 24) & 0xFF;

    // Channel-start table [+0x0C]: relative to data base (after table)
    size_t chOffset = 0;
    for (uint32_t c = 0; c < channels; ++c) {
        const size_t tableOff = 0x0C + c * 4u;
        const uint32_t off32 = static_cast<uint32_t>(chOffset);
        scdl[tableOff+0] = off32 & 0xFF; scdl[tableOff+1] = (off32 >> 8) & 0xFF;
        scdl[tableOff+2] = (off32 >> 16) & 0xFF; scdl[tableOff+3] = (off32 >> 24) & 0xFF;
        chOffset += chEnc[c].size();
    }

    // Copy channel data
    size_t writePos = headerBytes;
    for (uint32_t c = 0; c < channels; ++c) {
        std::memcpy(scdl.data() + writePos, chEnc[c].data(), chEnc[c].size());
        writePos += chEnc[c].size();
    }

    // ── 6. Build SCEl block (8 bytes, zero payload) ───────────────────────────
    std::vector<uint8_t> scel(8, 0);
    std::memcpy(scel.data(), "SCEl", 4);
    scel[4] = 8; // size = 8 (just the header, LE)

    // ── 7. Assemble new segment bytes ─────────────────────────────────────────
    // Update SCHl size field to reflect the actual parsed size (already has it).
    // New segment = origSchl + scdl + scel
    std::vector<uint8_t> newSeg;
    newSeg.reserve(origSchl.size() + scdl.size() + scel.size());
    newSeg.insert(newSeg.end(), origSchl.begin(), origSchl.end());
    newSeg.insert(newSeg.end(), scdl.begin(),     scdl.end());
    newSeg.insert(newSeg.end(), scel.begin(),     scel.end());

    // DWORD-align to 4 bytes (MUS segments are padded)
    while (newSeg.size() % 4 != 0) newSeg.push_back(0x00);

    // ── 8. Read entire MUS file into memory, splice, write back ──────────────
    std::vector<uint8_t> musData;
    {
        const size_t fsz = static_cast<size_t>(fileSize);
        musData.resize(fsz);
        fin.seekg(0);
        fin.read(reinterpret_cast<char*>(musData.data()), static_cast<std::streamsize>(fsz));
        fin.close();
    }

    const size_t oldOff  = sec.fileOffset;
    const size_t oldSpan = sec.blockSpan;

    if (oldOff + oldSpan > musData.size())
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceSection: segment range exceeds file size");

    // Build the new file bytes: head + newSeg + tail
    std::vector<uint8_t> newMusData;
    newMusData.reserve(musData.size() - oldSpan + newSeg.size());
    newMusData.insert(newMusData.end(), musData.begin(), musData.begin() + oldOff);
    newMusData.insert(newMusData.end(), newSeg.begin(),  newSeg.end());
    newMusData.insert(newMusData.end(), musData.begin() + oldOff + oldSpan, musData.end());

    {
        std::ofstream fout(mus.path, std::ios::binary | std::ios::trunc);
        if (!fout) return Result<AudioCodecs::ReplaceReport>::Err("Cannot write MUS: " + mus.path.string());
        fout.write(reinterpret_cast<const char*>(newMusData.data()),
                   static_cast<std::streamsize>(newMusData.size()));
    }

    LOG_INFO("[MUS] ReplaceSection: section {} rewritten ({} frames, {} ch, {}-byte new seg)",
             sectionIdx, frames, channels, newSeg.size());

    // ── 9. Reload section offsets so the in-memory MUSFile stays valid ────────
    auto reloaded = MUSParser::Load(mus.path);
    if (reloaded) {
        mus = std::move(reloaded.value);
    } else {
        LOG_WARN("[MUS] ReplaceSection: reload after replace failed: {}", reloaded.error);
    }

    return Result<AudioCodecs::ReplaceReport>::Ok(std::move(report));
}

} // namespace nfsmw