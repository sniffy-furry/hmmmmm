#pragma once
// ─── formats/AudioCodecs.h ────────────────────────────────────────────────────
// Phase 5 — cross-cutting audio re-encode infrastructure shared by AudioBank,
// ABKFile, GINFile, and MUSFile.
//
// Problem this solves:
//   • The re-encode logic was scattered: EA-XA v2 lived in ABKFile and was
//     reused ad-hoc; EA-XAS v0 was private to GINFile; the IMA-ADPCM coder
//     was private to AudioBank. Each format re-implemented its own "pick a
//     codec, encode it" switch statement.
//   • None of the four replace paths actually checked the *source* WAV's
//     sample rate / channel count against the *target* slot's declared
//     values before re-encoding. For ABK (PT chunk), GIN (file header), and
//     MUS (SCHl header) the container's rate field is fixed and is NOT
//     patched on replace — so importing a WAV recorded at a different rate
//     silently changes pitch/speed in-game. (AudioBank's SNR is the one
//     format where the rate field is adjustable, so it can legitimately
//     adopt the WAV's rate instead of resampling to it.)
//   • Large re-encode jobs (EA-XA / EA-XAS brute-force predictor search)
//     gave no progress feedback beyond an indeterminate spinner.
//   • Nothing verified that a re-encode actually preserved the audio — a
//     silent encoder bug could ship corrupted or near-silent output.
//
// This module centralises the codec math behind one dispatch enum, adds a
// linear-interpolation resampler so any input rate can be brought in line
// with a fixed target rate before encoding, and gives every replace path a
// single source of truth for mismatch messaging and post-encode
// verification via the shared ReplaceReport.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace nfsmw::AudioCodecs {

// ─────────────────────────────────────────────────────────────────────────────
// Codec identifiers — distinct from each format's on-disk tag enum
// (AudioCodec::*, ABKCodec::*, MUSCodec::*), which differ in numbering and
// width between formats. Every format's replace path maps its tag to one of
// these before calling Encode/Decode, so the actual codec math lives in
// exactly one place.
// ─────────────────────────────────────────────────────────────────────────────
enum class Codec {
    PCM16,       ///< Raw signed-16 little-endian, multi-channel interleaved.
    IMA_ADPCM,   ///< MS-IMA ADPCM, block-interleaved, multi-channel.
    EA_XA_V2,    ///< EA-XA v2 ("eaxa"). Mono only — used by ABK and MUS.
    EA_XAS_V0,   ///< EA-XAS v0 ("xas0"). Mono only — used by GIN.
};

const char* CodecName(Codec c);

/// True for codecs that only encode/decode a single channel at a time
/// (callers must de-interleave multi-channel sources themselves, e.g. via
/// EncodePerChannel).
bool IsMonoOnly(Codec c);

/// Progress in [0, 1]. May be an empty std::function (no-op) — every
/// function below treats an empty callback as "don't bother reporting".
using ProgressFn = std::function<void(float)>;

// ─────────────────────────────────────────────────────────────────────────────
// Encode / Decode
// ─────────────────────────────────────────────────────────────────────────────

/// Encode interleaved PCM (numFrames × channels samples) to `codec`'s native
/// byte layout. PCM16 / IMA_ADPCM accept any channel count. EA_XA_V2 /
/// EA_XAS_V0 are mono-only — pass channels=1, or use EncodePerChannel for
/// multi-channel sources with those codecs.
Result<std::vector<uint8_t>> Encode(Codec codec,
                                     const int16_t* pcm,
                                     size_t numFrames,
                                     uint32_t channels,
                                     const ProgressFn& onProgress = {});

/// Decode raw bytes back to interleaved PCM. `channels` must match what was
/// passed to Encode. `numSamplesHint` caps output length for codecs that
/// don't self-terminate on a sample count (0 = decode all available data).
Result<std::vector<int16_t>> Decode(Codec codec,
                                     const uint8_t* data,
                                     size_t size,
                                     uint32_t channels,
                                     uint32_t numSamplesHint = 0);

/// De-interleave `pcm` into `channels` mono streams and encode each one
/// independently with a mono-only codec (EA_XA_V2 / EA_XAS_V0). Used by
/// formats that pack per-channel streams into their own container layout
/// (e.g. MUS's SCDl channel-start-offset table).
Result<std::vector<std::vector<uint8_t>>> EncodePerChannel(
    Codec codec,
    const int16_t* pcm,
    size_t numFrames,
    uint32_t channels,
    const ProgressFn& onProgress = {});

// ─────────────────────────────────────────────────────────────────────────────
// Resampling
// ─────────────────────────────────────────────────────────────────────────────

/// Linear-interpolation resampler. Not broadcast-quality, but artifact-free
/// for the short sound-effect / engine-clip / music-section lengths this
/// tool deals with, and enough to keep re-encoded audio at the correct pitch
/// and duration when the user's WAV doesn't match a format's fixed target
/// rate. `pcm` is interleaved; `channels` is preserved. Returns `pcm`
/// unchanged if srcRate == dstRate or either rate is 0.
std::vector<int16_t> Resample(const std::vector<int16_t>& pcm,
                               uint32_t channels,
                               uint32_t srcRate,
                               uint32_t dstRate);

// ─────────────────────────────────────────────────────────────────────────────
// Mismatch detection
// ─────────────────────────────────────────────────────────────────────────────

struct RateCheck {
    bool     rateMismatch    = false;
    bool     channelMismatch = false;
    uint32_t srcRate         = 0;
    uint32_t dstRate         = 0;
    uint32_t srcChannels     = 0;
    uint32_t dstChannels     = 0;

    bool AnyMismatch() const { return rateMismatch || channelMismatch; }
};

RateCheck CheckRates(uint32_t srcRate, uint32_t srcChannels,
                      uint32_t dstRate, uint32_t dstChannels);

// ─────────────────────────────────────────────────────────────────────────────
// Round-trip verification
// ─────────────────────────────────────────────────────────────────────────────

struct VerifyResult {
    bool        ok = true;
    std::string note;   ///< empty when ok and nothing notable to report
};

/// Decode `encoded` back with `codec` and compare its duration and RMS level
/// against `refPcm` — the PCM that was actually fed into Encode (i.e.
/// post-resample, if resampling occurred). Flags large discrepancies, which
/// usually indicate an encoder bug silently dropping or corrupting audio
/// rather than a benign quantisation difference.
VerifyResult VerifyRoundTrip(Codec codec,
                              const std::vector<uint8_t>& encoded,
                              uint32_t channels,
                              uint32_t numSamplesHint,
                              const std::vector<int16_t>& refPcm,
                              uint32_t rate);

// ─────────────────────────────────────────────────────────────────────────────
// Replace report
//
// Returned by every format's Replace*/Import* entry point (AudioBankParser::
// ReplaceFromWAV, ABKParser::ReplaceFromPCM, GINParser::ReplaceClip,
// MUSParser::ReplaceSection) so panels can surface consistent toast text
// regardless of which format they happen to be patching.
// ─────────────────────────────────────────────────────────────────────────────
struct ReplaceReport {
    bool         resampled   = false;  ///< true if the source PCM was resampled to match the target
    uint32_t     srcRate     = 0;
    uint32_t     dstRate     = 0;
    uint32_t     srcChannels = 0;
    uint32_t     dstChannels = 0;
    VerifyResult verify;

    /// Short, toast-ready summary of anything noteworthy (resampling,
    /// channel mismatch, verification concerns). Empty when the replace was
    /// a clean, unremarkable match.
    std::string Summary() const;
};

} // namespace nfsmw::AudioCodecs
