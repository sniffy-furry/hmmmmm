// ─── formats/GINFile.cpp ─────────────────────────────────────────────────────
// EA GIN (.GIN) engine-sound parser — "Gnsu" container, EA-XAS v0 codec.
//
// Confirmed header layout (vgmstream meta/gin.c, validated against all 160
// NFSMW .gin files):
//   [0x00]  char[4]  "Gnsu"  (or "Octn" in later EA titles)
//   [0x04]  uint32   "20\0\0" version/size marker (ignored)
//   [0x08]  float32  rpmMin  (informational — engine RPM range)
//   [0x0C]  float32  rpmMax
//   [0x10]  uint32   table0Size  (RPM-up curve point count)
//   [0x14]  uint32   table1Size  (RPM curve point count)
//   [0x18]  uint32   numSamples  (total PCM samples)
//   [0x1C]  uint32   sampleRate  (Hz — varies per file, NOT fixed 22050)
//   [0x20]  uint32[table0Size+1]  table 0
//   ...     uint32[table1Size+1]  table 1
//   start = 0x20 + (table0Size+1)*4 + (table1Size+1)*4
//   [start] EA-XAS v0 audio stream (one mono stream, numSamples total)
//
// Codec: EA-XAS v0 ("xas0") — 0x13-byte frames of 32 samples each:
//   frame header u32le: nibble&0x0F -> CD-XA coef pair; bits>>16&0xF -> shift;
//   two 12-bit hist samples; 30 nibbles (high-nibble-first) follow.
// Ported from vgmstream coding/ea_xas_decoder.c (decode_ea_xas_v0).
// ─────────────────────────────────────────────────────────────────────────────
#include "formats/GINFile.h"
#include "formats/AudioCodecs.h"
#include "core/Logger.h"

#include <fstream>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace nfsmw {

namespace {

Result<std::vector<uint8_t>> ReadFileBytes(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return Result<std::vector<uint8_t>>::Err("Cannot open: " + p.string());
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(sz);
    if (!f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz)))
        return Result<std::vector<uint8_t>>::Err("Read failed: " + p.string());
    return Result<std::vector<uint8_t>>::Ok(std::move(buf));
}

inline uint32_t ReadU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | static_cast<uint32_t>(p[1]) << 8
         | static_cast<uint32_t>(p[2]) << 16
         | static_cast<uint32_t>(p[3]) << 24;
}
inline float ReadF32LE(const uint8_t* p) {
    uint32_t bits = ReadU32LE(p);
    float v;
    std::memcpy(&v, &bits, 4);
    return v;
}
inline int16_t ClampS16(int32_t v) {
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return static_cast<int16_t>(v);
}

// Standard CD-XA K0/K1 filter pairs (only 4 valid; rest assumed 0).
const double kXACoefs[16][2] = {
    { 0.0,       0.0      },
    { 0.9375,    0.0      },
    { 1.796875, -0.8125   },
    { 1.53125,  -0.859375 },
};

// EA-XAS v0 decoder: external interleave, mono. 0x13-byte frame -> 32 samples.
std::vector<int16_t> DecodeEAXAS_V0(const uint8_t* src, size_t srcSize, uint32_t numSamples) {
    std::vector<int16_t> out;
    if (srcSize == 0) return out;
    out.reserve(numSamples);

    const size_t cap = numSamples;
    size_t pos = 0;
    while (out.size() < cap && pos + 0x13 <= srcSize) {
        uint32_t hdr = ReadU32LE(src + pos);
        double  coef1 = kXACoefs[hdr & 0x0F][0];
        double  coef2 = kXACoefs[hdr & 0x0F][1];
        int16_t hist2 = static_cast<int16_t>((hdr >>  0) & 0xFFF0);
        int16_t hist1 = static_cast<int16_t>((hdr >> 16) & 0xFFF0);
        uint8_t shift = (hdr >> 16) & 0x0F;

        if (out.size() < cap) out.push_back(hist2);
        if (out.size() < cap) out.push_back(hist1);

        for (int i = 0; i < 0x0F * 2 && out.size() < cap; ++i) {
            uint8_t nibbles = src[pos + 0x04 + i / 2];
            int s = (i & 1) ? (nibbles & 0x0F) : ((nibbles >> 4) & 0x0F);  // high nibble first
            s = static_cast<int16_t>(s << 12) >> shift;                    // 16b sign-extend + scale
            double acc = static_cast<double>(s)
                       + static_cast<double>(hist1) * coef1
                       + static_cast<double>(hist2) * coef2;
            int16_t sv = ClampS16(static_cast<int32_t>(acc));
            out.push_back(sv);
            hist2 = hist1;
            hist1 = sv;
        }
        pos += 0x13;
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// EA-XAS v0 encoder (inverse of DecodeEAXAS_V0).
// Each 0x13-byte frame encodes exactly 32 samples:
//   bytes [0x00..0x03]  frame header (uint32 LE):
//       bits  [3: 0]  coef index  (0..3, CD-XA K0/K1 pair)
//       bits  [7: 4]  shift       (right-shift for scaled residual)
//       bits [15: 8]  reserved (0)
//       bits [31:16]  hist1 (s16, stored in upper 12 bits — i.e. hist1 & 0xFFF0)
//       *** NOTE: the decoder reads hist2 from bits [15:0] and hist1 from [31:16].
//           We store hist1=first output sample and hist2=second, matching the
//           decoder's layout: hdr bits [15:0] => hist2, [31:16] => hist1.
//   bytes [0x04..0x12]  15 nibble-bytes — 30 nibbles, HIGH nibble first,
//                        encoding 30 residual samples (total = 2 hist + 30 = 32).
//
// Encoder strategy: brute-force coef index (0..3) and shift (0..11) per frame,
// pick the combination that minimises total squared error against the input.
// ─────────────────────────────────────────────────────────────────────────────

// Write uint32 LE into a byte buffer
static void WriteU32LE(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

[[maybe_unused]] static std::vector<uint8_t> EncodeEAXAS_V0(const int16_t* pcm, size_t numSamples) {
    // Pad to a multiple of 32 samples
    const size_t frames    = (numSamples + 31) / 32;
    const size_t padded    = frames * 32;

    // Work buffer padded with silence
    std::vector<int16_t> src(padded, 0);
    std::memcpy(src.data(), pcm, numSamples * sizeof(int16_t));

    std::vector<uint8_t> out(frames * 0x13, 0);

    for (size_t f = 0; f < frames; ++f) {
        const int16_t* s = src.data() + f * 32;
        uint8_t*       d = out.data() + f * 0x13;

        // The first two samples become the initial history pair.
        const int16_t hist2_init = s[0];
        const int16_t hist1_init = s[1];
        // Remaining 30 samples are encoded as residual nibbles.
        const int16_t* res = s + 2;

        int   bestCoef  = 0;
        int   bestShift = 0;
        int64_t bestErr = INT64_MAX;
        std::vector<int8_t> bestNibs(30);

        for (int ci = 0; ci < 4; ++ci) {
            const double k0 = kXACoefs[ci][0];
            const double k1 = kXACoefs[ci][1];

            for (int sh = 0; sh <= 11; ++sh) {
                int64_t err = 0;
                std::vector<int8_t> nibs(30);
                double h1 = hist1_init, h2 = hist2_init;

                // NOTE: the active EA-XAS v0 encoder lives in AudioCodecs.cpp;
                // this copy is kept in sync to avoid a latent trap. The decoder
                // reconstructs residuals as (int16_t)(nib<<12) >> shift, i.e.
                // scaled by 2^(12-sh) — NOT 2^sh — so we must match that here.
                const int rshift = 12 - sh;
                for (int i = 0; i < 30; ++i) {
                    const double predicted = h1 * k0 + h2 * k1;
                    double residual = static_cast<double>(res[i]) - predicted;
                    // Scale residual into nibble range [-8..7]
                    double scaled = residual / (double)(1 << rshift);
                    int nib = static_cast<int>(std::round(scaled));
                    nib = std::max(-8, std::min(7, nib));
                    nibs[i] = static_cast<int8_t>(nib);
                    // Reconstruct exactly what the decoder would produce
                    const int32_t resExt =
                        static_cast<int32_t>(static_cast<int16_t>((nib & 0xF) << 12)) >> sh;
                    const double recon = predicted + static_cast<double>(resExt);
                    const int16_t sv = ClampS16(static_cast<int32_t>(recon));
                    const int64_t diff = static_cast<int64_t>(res[i]) - sv;
                    err += diff * diff;
                    h2 = h1;
                    h1 = static_cast<double>(sv);
                }

                if (err < bestErr) {
                    bestErr   = err;
                    bestCoef  = ci;
                    bestShift = sh;
                    bestNibs  = nibs;
                }
            }
        }

        // Build 32-bit header
        // Decoder reads: coef = hdr & 0x0F, hist2 = (int16_t)(hdr & 0xFFF0),
        //                shift = (hdr >> 16) & 0x0F, hist1 = (int16_t)((hdr >> 16) & 0xFFF0)
        const uint32_t h2bits = static_cast<uint32_t>(hist2_init & 0xFFF0) & 0xFFFF;
        const uint32_t h1bits = (static_cast<uint32_t>(hist1_init & 0xFFF0) & 0xFFFF) << 16;
        const uint32_t hdr = static_cast<uint32_t>(bestCoef)
                           | h2bits
                           | (static_cast<uint32_t>(bestShift) << 16)
                           | h1bits;
        WriteU32LE(d, hdr);

        // Pack 30 nibbles into 15 bytes, high nibble first
        for (int i = 0; i < 15; ++i) {
            const int hi = bestNibs[i * 2]     & 0x0F;
            const int lo = bestNibs[i * 2 + 1] & 0x0F;
            d[0x04 + i] = static_cast<uint8_t>((hi << 4) | lo);
        }
    }
    return out;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
Result<GINFile> GINParser::Load(const std::filesystem::path& path) {
    auto readRes = ReadFileBytes(path);
    if (!readRes) return Result<GINFile>::Err(readRes.error);
    auto data = std::move(readRes.value);
    const size_t fileSize = data.size();

    if (fileSize < 0x24)
        return Result<GINFile>::Err("GIN file too small");

    // Magic: "Gnsu" (original) or "Octn" (later EA titles)
    if (std::memcmp(data.data(), "Gnsu", 4) != 0 &&
        std::memcmp(data.data(), "Octn", 4) != 0) {
        std::string got(reinterpret_cast<const char*>(data.data()),
                        std::min<size_t>(4, fileSize));
        return Result<GINFile>::Err("Not a GIN file (bad magic: \"" + got + "\")");
    }

    const float    rpmMin     = ReadF32LE(data.data() + 0x08);
    const float    rpmMax     = ReadF32LE(data.data() + 0x0C);
    const uint32_t table0Size = ReadU32LE(data.data() + 0x10);
    const uint32_t table1Size = ReadU32LE(data.data() + 0x14);
    const uint32_t numSamples = ReadU32LE(data.data() + 0x18);
    uint32_t       sampleRate = ReadU32LE(data.data() + 0x1C);

    if (sampleRate == 0 || sampleRate > 192000) sampleRate = 22050;
    if (numSamples == 0)
        return Result<GINFile>::Err("GIN: zero samples");

    const size_t startOffset = 0x20
        + (static_cast<size_t>(table0Size) + 1) * 4
        + (static_cast<size_t>(table1Size) + 1) * 4;
    if (startOffset >= fileSize)
        return Result<GINFile>::Err("GIN: audio start offset past EOF");

    GINFile gin;
    gin.name     = path.stem().string();
    gin.path     = path;
    gin.fileData = std::move(data);

    // GIN is a single mono engine stream; we expose it as one clip.
    GINClip clip;
    clip.rpmMin     = rpmMin;
    clip.rpmMax     = rpmMax;
    clip.dataOffset = static_cast<uint32_t>(startOffset);
    clip.dataSize   = static_cast<uint32_t>(fileSize - startOffset);
    clip.numSamples = numSamples;
    clip.sampleRate = sampleRate;
    clip.channels   = 1;
    gin.clips.push_back(clip);

    LOG_INFO("[GIN] Loaded '{}' — {} samples @ {} Hz, {:.0f}–{:.0f} RPM",
             gin.name, numSamples, sampleRate, rpmMin, rpmMax);

    return Result<GINFile>::Ok(std::move(gin));
}

// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<int16_t>> GINParser::DecodePCM(const GINFile& gin,
                                                    const GINClip& clip,
                                                    uint32_t& outRate,
                                                    uint32_t& outChannels) {
    const auto& data = gin.fileData;
    const size_t end = static_cast<size_t>(clip.dataOffset) + clip.dataSize;
    if (clip.dataSize == 0 || end > data.size())
        return Result<std::vector<int16_t>>::Err("GIN: clip data out of bounds");

    outRate     = clip.sampleRate;
    outChannels = clip.channels;

    auto pcm = DecodeEAXAS_V0(data.data() + clip.dataOffset, clip.dataSize, clip.numSamples);
    if (pcm.empty())
        return Result<std::vector<int16_t>>::Err("GIN: EA-XAS decode produced no samples");

    return Result<std::vector<int16_t>>::Ok(std::move(pcm));
}

// ─── GINParser::ReplaceClip ───────────────────────────────────────────────────
// Strategy: GIN is a single-clip file; the audio stream is the tail of the file
// starting at clip.dataOffset. We:
//   1. Re-encode the new PCM to EA-XAS v0.
//   2. Rewrite the file = header (bytes 0..dataOffset-1, unchanged) + new stream.
//   3. Patch numSamples at offset 0x18 with the new sample count.
//   4. Update gin.fileData, clip.dataSize, and clip.numSamples in memory.
//
// The RPM range, sample rate, and all header/table bytes are preserved.
// Callers must ensure a .bak exists before calling.

Result<AudioCodecs::ReplaceReport> GINParser::ReplaceClip(GINFile& gin,
                                     size_t clipIdx,
                                     const std::vector<int16_t>& pcmMono,
                                     uint32_t pcmRate,
                                     const AudioCodecs::ProgressFn& onProgress)
{
    if (clipIdx >= gin.clips.size())
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceClip: clip index out of range");
    if (pcmMono.empty())
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceClip: empty PCM input");
    if (gin.path.empty())
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceClip: GINFile has no path");

    GINClip& clip = gin.clips[clipIdx];
    const size_t hdrSize = clip.dataOffset;  // bytes to preserve verbatim

    if (hdrSize > gin.fileData.size())
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceClip: header region exceeds file data");

    // ── Build report and resample if the source rate differs from the clip's fixed rate
    AudioCodecs::ReplaceReport report;
    report.srcRate     = pcmRate;
    report.dstRate     = clip.sampleRate;
    report.srcChannels = 1;
    report.dstChannels = 1;

    const std::vector<int16_t>* working = &pcmMono;
    std::vector<int16_t> resampled;
    if (pcmRate != 0 && clip.sampleRate != 0 && pcmRate != clip.sampleRate) {
        resampled = AudioCodecs::Resample(pcmMono, 1, pcmRate, clip.sampleRate);
        working   = &resampled;
        report.resampled = true;
    }

    // Re-encode to EA-XAS v0 via centralised dispatch (threads progress)
    auto encRes = AudioCodecs::Encode(AudioCodecs::Codec::EA_XAS_V0,
                                       working->data(), working->size(), 1, onProgress);
    if (!encRes)
        return Result<AudioCodecs::ReplaceReport>::Err("ReplaceClip: " + encRes.error);
    const std::vector<uint8_t>& encoded = encRes.value;

    // Round-trip verification
    report.verify = AudioCodecs::VerifyRoundTrip(
        AudioCodecs::Codec::EA_XAS_V0, encoded, 1,
        static_cast<uint32_t>(working->size()), *working,
        report.dstRate > 0 ? report.dstRate : pcmRate);

    // Build new file = header + encoded stream
    std::vector<uint8_t> newFile(hdrSize + encoded.size());
    std::memcpy(newFile.data(), gin.fileData.data(), hdrSize);
    std::memcpy(newFile.data() + hdrSize, encoded.data(), encoded.size());

    // Patch numSamples at offset 0x18 (uint32 LE)
    const uint32_t newNumSamples = static_cast<uint32_t>(working->size());
    if (hdrSize >= 0x1C) {
        WriteU32LE(newFile.data() + 0x18, newNumSamples);
    } else {
        LOG_WARN("[GIN] ReplaceClip: header too small to patch numSamples");
    }

    // Write to disk
    {
        std::ofstream f(gin.path, std::ios::binary);
        if (!f)
            return Result<AudioCodecs::ReplaceReport>::Err(
                "ReplaceClip: cannot open '" + gin.path.string() + "' for writing");
        if (!f.write(reinterpret_cast<const char*>(newFile.data()),
                     static_cast<std::streamsize>(newFile.size())))
            return Result<AudioCodecs::ReplaceReport>::Err("ReplaceClip: write failed");
    }

    // Update in-memory state
    gin.fileData         = std::move(newFile);
    clip.dataSize        = static_cast<uint32_t>(encoded.size());
    clip.numSamples      = newNumSamples;

    LOG_INFO("[GIN] ReplaceClip: clip {} rewritten — {} samples, {}-byte stream",
             clipIdx, newNumSamples, encoded.size());
    return Result<AudioCodecs::ReplaceReport>::Ok(std::move(report));
}

} // namespace nfsmw
