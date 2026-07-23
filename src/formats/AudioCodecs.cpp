// ─── formats/AudioCodecs.cpp ──────────────────────────────────────────────────
// See AudioCodecs.h for rationale. This file is the single home for:
//   • the IMA-ADPCM block coder (previously private to AudioBank.cpp)
//   • the EA-XAS v0 coder (previously private to GINFile.cpp)
//   • PCM16 passthrough
//   • EA-XA v2 is intentionally NOT duplicated here — ABKFile.cpp already
//     exposes EncodeEAXA/DecodeEAXA as shared free functions (GINFile.h and
//     MUSFile.cpp already include ABKFile.h to reuse them), so this module
//     simply forwards to them for Codec::EA_XA_V2.
// ─────────────────────────────────────────────────────────────────────────────
#include "formats/AudioCodecs.h"
#include "formats/ABKFile.h"   // EncodeEAXA / DecodeEAXA (EA-XA v2)
#include "core/Logger.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace nfsmw::AudioCodecs {

// ─────────────────────────────────────────────────────────────────────────────
// Small shared helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

inline int16_t ClampS16(int32_t v) {
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return static_cast<int16_t>(v);
}

inline void WriteU32LE(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}
inline uint32_t ReadU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | static_cast<uint32_t>(p[1]) << 8
         | static_cast<uint32_t>(p[2]) << 16
         | static_cast<uint32_t>(p[3]) << 24;
}

double ComputeRMS(const int16_t* pcm, size_t n) {
    if (n == 0) return 0.0;
    double sumSq = 0.0;
    for (size_t i = 0; i < n; ++i) sumSq += double(pcm[i]) * double(pcm[i]);
    return std::sqrt(sumSq / double(n));
}

// ── PCM16 ───────────────────────────────────────────────────────────────────
std::vector<uint8_t> EncodePCM16(const int16_t* pcm, size_t numSamples) {
    std::vector<uint8_t> out(numSamples * 2);
    if (numSamples) std::memcpy(out.data(), pcm, numSamples * 2);
    return out;
}
std::vector<int16_t> DecodePCM16(const uint8_t* data, size_t size) {
    std::vector<int16_t> out(size / 2);
    if (!out.empty()) std::memcpy(out.data(), data, out.size() * 2);
    return out;
}

// ── IMA-ADPCM ─────────────────────────────────────────────────────────────────
// Moved here verbatim from formats/AudioBank.cpp (was the only consumer; now
// shared so every format dispatches through the same coder). MS-IMA/DVI
// variant: each block begins with (channels × 4-byte header): predictor(s16)
// + index(u8) + pad(u8), followed by interleaved nibble data.
const int kStepTable[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,
    60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,
    337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,
    1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,
    4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,
    15289,16818,18500,20350,22385,24623,27086,29794,32767
};
const int kIndexTable[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

std::vector<int16_t> DecodeIMABlock(const uint8_t* src, size_t blockSize, int channels) {
    std::vector<int16_t> out;
    if (blockSize < static_cast<size_t>(4 * channels)) return out;

    struct Chan { int pred; int idx; };
    std::vector<Chan> ch(static_cast<size_t>(channels));

    size_t pos = 0;
    for (int c = 0; c < channels; ++c) {
        int16_t pred; std::memcpy(&pred, src + pos, 2);
        ch[static_cast<size_t>(c)].pred = pred;
        ch[static_cast<size_t>(c)].idx  = std::clamp<int>(src[pos + 2], 0, 88);
        pos += 4;
        out.push_back(pred);
    }

    while (pos + static_cast<size_t>(4 * channels) <= blockSize) {
        for (int c = 0; c < channels; ++c) {
            auto& state = ch[static_cast<size_t>(c)];
            for (int b = 0; b < 4; ++b) {
                uint8_t byte = src[pos++];
                for (int n = 0; n < 2; ++n) {
                    int nibble = (n == 0) ? (byte & 0xF) : ((byte >> 4) & 0xF);
                    int step   = kStepTable[state.idx];
                    int delta  = step >> 3;
                    if (nibble & 1) delta += step >> 2;
                    if (nibble & 2) delta += step >> 1;
                    if (nibble & 4) delta += step;
                    if (nibble & 8) delta = -delta;
                    state.pred = std::clamp(state.pred + delta, -32768, 32767);
                    state.idx  = std::clamp(state.idx + kIndexTable[nibble & 7], 0, 88);
                    out.push_back(ClampS16(state.pred));
                }
            }
        }
    }
    return out;
}

std::vector<int16_t> DecodeIMAStream(const uint8_t* raw, size_t size, int channels) {
    const size_t blockSize = (channels == 2) ? 1024u : 512u;
    std::vector<int16_t> out;
    size_t pos = 0;
    while (pos + 4 <= size) {
        size_t chunk = std::min(blockSize, size - pos);
        auto block = DecodeIMABlock(raw + pos, chunk, channels);
        out.insert(out.end(), block.begin(), block.end());
        pos += blockSize;
    }
    return out;
}

std::vector<uint8_t> EncodeIMABlock(const int16_t* pcm, size_t numFrames, int channels,
                                     const ProgressFn& onProgress) {
    const size_t blockSize = (channels == 2) ? 1024u : 512u;
    const size_t samplesPerBlock = 1 + ((blockSize / static_cast<size_t>(channels) - 4) * 2);

    std::vector<uint8_t> out;
    out.reserve(((numFrames + samplesPerBlock - 1) / samplesPerBlock) * blockSize);

    struct ChanState { int pred = 0; int idx = 0; };
    std::vector<ChanState> state(static_cast<size_t>(channels));

    const size_t totalBlocks = (numFrames + samplesPerBlock - 1) / std::max<size_t>(1, samplesPerBlock);
    size_t blockIdx = 0;

    size_t frame = 0;
    while (frame < numFrames) {
        const size_t framesThisBlock = std::min(samplesPerBlock, numFrames - frame);
        std::vector<uint8_t> block(blockSize, 0);
        size_t bpos = 0;

        for (int c = 0; c < channels; ++c) {
            const int16_t pred = (framesThisBlock > 0)
                ? pcm[(frame * static_cast<size_t>(channels)) + static_cast<size_t>(c)]
                : 0;
            state[static_cast<size_t>(c)].pred = pred;
            state[static_cast<size_t>(c)].idx  = 0;
            block[bpos + 0] = static_cast<uint8_t>(pred & 0xFF);
            block[bpos + 1] = static_cast<uint8_t>((pred >> 8) & 0xFF);
            block[bpos + 2] = 0;
            block[bpos + 3] = 0;
            bpos += 4;
        }

        size_t f = frame + 1;
        const size_t frameEnd = frame + framesThisBlock;

        while (bpos < blockSize && f < frameEnd) {
            for (int c = 0; c < channels && bpos < blockSize; ++c) {
                auto& st = state[static_cast<size_t>(c)];
                for (int b = 0; b < 4 && bpos < blockSize; ++b) {
                    uint8_t packed = 0;
                    for (int n = 0; n < 2; ++n) {
                        int nib = 0;
                        if (f < frameEnd) {
                            const int16_t sample = pcm[f * static_cast<size_t>(channels) +
                                                       static_cast<size_t>(c)];
                            const int diff = static_cast<int>(sample) - st.pred;
                            const int step = kStepTable[st.idx];
                            nib = (std::abs(diff) * 4) / step;
                            if (nib > 7) nib = 7;
                            if (diff < 0) nib |= 8;

                            int delta = step >> 3;
                            if (nib & 1) delta += step >> 2;
                            if (nib & 2) delta += step >> 1;
                            if (nib & 4) delta += step;
                            if (nib & 8) delta = -delta;
                            st.pred = std::clamp(st.pred + delta, -32768, 32767);
                            st.idx  = std::clamp(st.idx + kIndexTable[nib & 7], 0, 88);
                            ++f;
                        }
                        if (n == 0) packed |= static_cast<uint8_t>(nib & 0xF);
                        else        packed |= static_cast<uint8_t>((nib & 0xF) << 4);
                    }
                    block[bpos++] = packed;
                }
            }
        }

        out.insert(out.end(), block.begin(), block.end());
        frame = frameEnd;
        ++blockIdx;
        if (onProgress && totalBlocks > 0)
            onProgress(static_cast<float>(blockIdx) / static_cast<float>(totalBlocks));
    }
    return out;
}

// ── EA-XAS v0 ─────────────────────────────────────────────────────────────────
// Moved here verbatim from formats/GINFile.cpp (was the only consumer).
// 0x13-byte frames, 32 samples each. See vgmstream coding/ea_xas_decoder.c.
const double kXACoefs[16][2] = {
    { 0.0,       0.0      },
    { 0.9375,    0.0      },
    { 1.796875, -0.8125   },
    { 1.53125,  -0.859375 },
};

std::vector<int16_t> DecodeEAXAS_V0(const uint8_t* src, size_t srcSize, uint32_t numSamples) {
    std::vector<int16_t> out;
    if (srcSize == 0) return out;
    out.reserve(numSamples);

    const size_t cap = numSamples ? numSamples : SIZE_MAX;
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
            int s = (i & 1) ? (nibbles & 0x0F) : ((nibbles >> 4) & 0x0F);
            s = static_cast<int16_t>(s << 12) >> shift;
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

std::vector<uint8_t> EncodeEAXAS_V0(const int16_t* pcm, size_t numSamples,
                                     const ProgressFn& onProgress) {
    const size_t frames = (numSamples + 31) / 32;
    const size_t padded = frames * 32;

    std::vector<int16_t> src(padded, 0);
    if (numSamples) std::memcpy(src.data(), pcm, numSamples * sizeof(int16_t));

    std::vector<uint8_t> out(frames * 0x13, 0);

    for (size_t f = 0; f < frames; ++f) {
        const int16_t* s = src.data() + f * 32;
        uint8_t*       d = out.data() + f * 0x13;

        const int16_t hist2_init = s[0];
        const int16_t hist1_init = s[1];
        const int16_t* res = s + 2;

        int     bestCoef  = 0;
        int     bestShift = 0;
        int64_t bestErr   = INT64_MAX;
        std::vector<int8_t> bestNibs(30);

        for (int ci = 0; ci < 4; ++ci) {
            const double k0 = kXACoefs[ci][0];
            const double k1 = kXACoefs[ci][1];

            for (int sh = 0; sh <= 11; ++sh) {
                int64_t err = 0;
                std::vector<int8_t> nibs(30);
                double h1 = hist1_init, h2 = hist2_init;

                // The decoder reconstructs each residual as
                //   (int16_t)(nibble << 12) >> shift  ==  signext4(nibble) * 2^(12-shift)
                // so the encoder must scale by 2^(12-sh), NOT 2^sh. The previous
                // 2^sh scaling only happened to match the decoder when sh == 6
                // (2^6 == 2^(12-6)); for every other shift it inverted the
                // residual magnitude, producing garbage on any loud/dynamic frame
                // (steady tones encode at shift 6 and sounded fine, which masked
                // the bug). This is the root cause of "choppy" re-encoded audio.
                const int rshift = 12 - sh;                       // decoder right-shift
                for (int i = 0; i < 30; ++i) {
                    const double predicted = h1 * k0 + h2 * k1;
                    double residual = static_cast<double>(res[i]) - predicted;
                    double scaled = residual / static_cast<double>(1 << rshift);
                    int nib = static_cast<int>(std::round(scaled));
                    nib = std::max(-8, std::min(7, nib));
                    nibs[i] = static_cast<int8_t>(nib);
                    // Reconstruct bit-exactly the way DecodeEAXAS_V0 will.
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

        const uint32_t h2bits = static_cast<uint32_t>(hist2_init & 0xFFF0) & 0xFFFF;
        const uint32_t h1bits = (static_cast<uint32_t>(hist1_init & 0xFFF0) & 0xFFFF) << 16;
        const uint32_t hdr = static_cast<uint32_t>(bestCoef)
                           | h2bits
                           | (static_cast<uint32_t>(bestShift) << 16)
                           | h1bits;
        WriteU32LE(d, hdr);

        for (int i = 0; i < 15; ++i) {
            const int hi = bestNibs[i * 2]     & 0x0F;
            const int lo = bestNibs[i * 2 + 1] & 0x0F;
            d[0x04 + i] = static_cast<uint8_t>((hi << 4) | lo);
        }

        if (onProgress && frames > 0)
            onProgress(static_cast<float>(f + 1) / static_cast<float>(frames));
    }
    return out;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Public dispatch
// ─────────────────────────────────────────────────────────────────────────────

const char* CodecName(Codec c) {
    switch (c) {
        case Codec::PCM16:     return "PCM";
        case Codec::IMA_ADPCM: return "IMA-ADPCM";
        case Codec::EA_XA_V2:  return "EA-XA";
        case Codec::EA_XAS_V0: return "EA-XAS";
        default:               return "Unknown";
    }
}

bool IsMonoOnly(Codec c) {
    return c == Codec::EA_XA_V2 || c == Codec::EA_XAS_V0;
}

Result<std::vector<uint8_t>> Encode(Codec codec, const int16_t* pcm, size_t numFrames,
                                     uint32_t channels, const ProgressFn& onProgress) {
    if (numFrames == 0)
        return Result<std::vector<uint8_t>>::Err("Encode: no samples to encode");

    switch (codec) {
        case Codec::PCM16:
            return Result<std::vector<uint8_t>>::Ok(
                EncodePCM16(pcm, numFrames * std::max<uint32_t>(1, channels)));

        case Codec::IMA_ADPCM: {
            const int ch = static_cast<int>(channels == 2 ? 2 : 1);
            auto out = EncodeIMABlock(pcm, numFrames, ch, onProgress);
            if (out.empty())
                return Result<std::vector<uint8_t>>::Err("IMA-ADPCM encode produced no output");
            return Result<std::vector<uint8_t>>::Ok(std::move(out));
        }

        case Codec::EA_XA_V2: {
            if (channels > 1)
                return Result<std::vector<uint8_t>>::Err(
                    "EA-XA v2 is mono-only — use EncodePerChannel for multi-channel sources");
            auto out = EncodeEAXA(pcm, numFrames, onProgress);
            if (out.empty())
                return Result<std::vector<uint8_t>>::Err("EA-XA encode produced no output");
            return Result<std::vector<uint8_t>>::Ok(std::move(out));
        }

        case Codec::EA_XAS_V0: {
            if (channels > 1)
                return Result<std::vector<uint8_t>>::Err(
                    "EA-XAS v0 is mono-only — use EncodePerChannel for multi-channel sources");
            auto out = EncodeEAXAS_V0(pcm, numFrames, onProgress);
            if (out.empty())
                return Result<std::vector<uint8_t>>::Err("EA-XAS encode produced no output");
            return Result<std::vector<uint8_t>>::Ok(std::move(out));
        }
    }
    return Result<std::vector<uint8_t>>::Err("Encode: unhandled codec");
}

Result<std::vector<int16_t>> Decode(Codec codec, const uint8_t* data, size_t size,
                                     uint32_t channels, uint32_t numSamplesHint) {
    if (size == 0)
        return Result<std::vector<int16_t>>::Err("Decode: empty input");

    switch (codec) {
        case Codec::PCM16:
            return Result<std::vector<int16_t>>::Ok(DecodePCM16(data, size));

        case Codec::IMA_ADPCM: {
            const int ch = static_cast<int>(channels == 2 ? 2 : 1);
            return Result<std::vector<int16_t>>::Ok(DecodeIMAStream(data, size, ch));
        }

        case Codec::EA_XA_V2: {
            auto out = DecodeEAXA(data, size, 1, numSamplesHint);
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }

        case Codec::EA_XAS_V0: {
            auto out = DecodeEAXAS_V0(data, size, numSamplesHint);
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }
    }
    return Result<std::vector<int16_t>>::Err("Decode: unhandled codec");
}

Result<std::vector<std::vector<uint8_t>>> EncodePerChannel(Codec codec, const int16_t* pcm,
                                                             size_t numFrames, uint32_t channels,
                                                             const ProgressFn& onProgress) {
    if (!IsMonoOnly(codec))
        return Result<std::vector<std::vector<uint8_t>>>::Err(
            "EncodePerChannel is only meaningful for mono-only codecs");
    if (numFrames == 0 || channels == 0)
        return Result<std::vector<std::vector<uint8_t>>>::Err("EncodePerChannel: no input");

    std::vector<std::vector<int16_t>> chPCM(channels);
    for (uint32_t c = 0; c < channels; ++c) {
        chPCM[c].resize(numFrames);
        for (size_t i = 0; i < numFrames; ++i)
            chPCM[c][i] = pcm[i * channels + c];
    }

    std::vector<std::vector<uint8_t>> out(channels);
    for (uint32_t c = 0; c < channels; ++c) {
        // Sub-divide overall progress evenly across channels so a stereo
        // source still reports smooth 0→1 progress rather than two resets.
        ProgressFn chProgress;
        if (onProgress) {
            chProgress = [&onProgress, c, channels](float f) {
                onProgress((static_cast<float>(c) + f) / static_cast<float>(channels));
            };
        }
        auto res = Encode(codec, chPCM[c].data(), chPCM[c].size(), 1, chProgress);
        if (!res) return Result<std::vector<std::vector<uint8_t>>>::Err(res.error);
        out[c] = std::move(res.value);
    }
    return Result<std::vector<std::vector<uint8_t>>>::Ok(std::move(out));
}

// ─────────────────────────────────────────────────────────────────────────────
// Resampling
// ─────────────────────────────────────────────────────────────────────────────
std::vector<int16_t> Resample(const std::vector<int16_t>& pcm, uint32_t channels,
                               uint32_t srcRate, uint32_t dstRate) {
    if (pcm.empty() || channels == 0 || srcRate == 0 || dstRate == 0 || srcRate == dstRate)
        return pcm;

    const size_t srcFrames = pcm.size() / channels;
    if (srcFrames == 0) return pcm;

    const double ratio     = static_cast<double>(dstRate) / static_cast<double>(srcRate);
    const size_t dstFrames = std::max<size_t>(1,
        static_cast<size_t>(std::llround(static_cast<double>(srcFrames) * ratio)));

    std::vector<int16_t> out(dstFrames * channels);
    for (size_t i = 0; i < dstFrames; ++i) {
        const double srcPos = static_cast<double>(i) / ratio;
        size_t i0 = static_cast<size_t>(srcPos);
        if (i0 >= srcFrames) i0 = srcFrames - 1;
        const size_t i1   = std::min(i0 + 1, srcFrames - 1);
        const double frac = srcPos - static_cast<double>(i0);

        for (uint32_t c = 0; c < channels; ++c) {
            const double a = pcm[i0 * channels + c];
            const double b = pcm[i1 * channels + c];
            const double v = a + (b - a) * frac;
            long lv = std::lround(v);
            lv = std::clamp<long>(lv, -32768L, 32767L);
            out[i * channels + c] = static_cast<int16_t>(lv);
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mismatch detection
// ─────────────────────────────────────────────────────────────────────────────
RateCheck CheckRates(uint32_t srcRate, uint32_t srcChannels,
                      uint32_t dstRate, uint32_t dstChannels) {
    RateCheck r;
    r.srcRate     = srcRate;
    r.dstRate     = dstRate;
    r.srcChannels = srcChannels;
    r.dstChannels = dstChannels;
    r.rateMismatch    = (srcRate != 0 && dstRate != 0 && srcRate != dstRate);
    r.channelMismatch = (srcChannels != 0 && dstChannels != 0 && srcChannels != dstChannels);
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Round-trip verification
// ─────────────────────────────────────────────────────────────────────────────
VerifyResult VerifyRoundTrip(Codec codec, const std::vector<uint8_t>& encoded,
                              uint32_t channels, uint32_t numSamplesHint,
                              const std::vector<int16_t>& refPcm, uint32_t rate) {
    VerifyResult out;
    if (encoded.empty() || refPcm.empty() || channels == 0 || rate == 0) {
        out.ok   = false;
        out.note = "round-trip verification skipped (no reference data)";
        return out;
    }

    auto dec = Decode(codec, encoded.data(), encoded.size(), channels, numSamplesHint);
    if (!dec) {
        out.ok   = false;
        out.note = "round-trip verification failed to decode: " + dec.error;
        return out;
    }
    const auto& got = dec.value;

    const size_t refFrames = refPcm.size() / std::max<uint32_t>(1, channels);
    const size_t gotFrames = got.size()    / std::max<uint32_t>(1, channels);

    const double refDur = static_cast<double>(refFrames) / static_cast<double>(rate);
    const double gotDur = static_cast<double>(gotFrames) / static_cast<double>(rate);
    const double durDiff = std::abs(gotDur - refDur);
    // Block-aligned codecs pad to their frame size, so allow generous slack.
    const double durTolSec = std::max(0.05, refDur * 0.05);

    const double refRMS = ComputeRMS(refPcm.data(), refPcm.size());
    const double gotRMS = ComputeRMS(got.data(), std::min(got.size(), refPcm.size()));

    std::vector<std::string> notes;
    if (durDiff > durTolSec) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "duration drifted %.2fs -> %.2fs", refDur, gotDur);
        notes.emplace_back(buf);
    }
    if (refRMS > 80.0 && gotRMS < refRMS * 0.1) {
        notes.emplace_back("re-encoded audio is far quieter than the source — encode may have failed");
        out.ok = false;
    }

    if (!notes.empty()) {
        out.note = notes.front();
        for (size_t i = 1; i < notes.size(); ++i) out.note += "; " + notes[i];
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// ReplaceReport::Summary
// ─────────────────────────────────────────────────────────────────────────────
std::string ReplaceReport::Summary() const {
    std::vector<std::string> parts;
    if (resampled) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "resampled %u Hz \xE2\x86\x92 %u Hz", srcRate, dstRate);
        parts.emplace_back(buf);
    }
    if (srcChannels != 0 && dstChannels != 0 && srcChannels != dstChannels) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%u ch \xE2\x86\x92 %u ch", srcChannels, dstChannels);
        parts.emplace_back(buf);
    }
    if (!verify.note.empty()) parts.push_back(verify.note);

    if (parts.empty()) return {};
    std::string s = parts.front();
    for (size_t i = 1; i < parts.size(); ++i) s += "; " + parts[i];
    return s;
}

} // namespace nfsmw::AudioCodecs
