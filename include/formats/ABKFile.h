#pragma once
// ─── formats/ABKFile.h ────────────────────────────────────────────────────────
// Parser for EA ABK (.ABK) sound-effect banks — ABKC/BNKl container format.
//
// ABKC outer container (little-endian):
//   [0x00] char[4]   "ABKC"
//   [0x14] uint32    totalFileSize    (patch on repack)
//   [0x20] uint32    sfxBankOffset    (offset of embedded BNKl)
//   [0x24] uint32    sfxBankSize      (patch on repack)
//   [0x30] uint32    funcFixupOffset  (patch on repack)
//   [0x34] uint32    staticDataFixupOffset
//   [0x38] uint32    interfaceOffset
//
// BNKl inner bank at sfxBankOffset:
//   [+0x00] char[4]   "BNKl"
//   [+0x06] int16     numSounds
//   [+0x08] int32     bnkSize
//   [+0x14] int32[N]  PT offsets (relative to position after each read)
//
// PT chunks (big-endian TLV): SampleRate(0x84), NumSamples(0x85),
//   LoopOffset(0x86), LoopLength(0x87), DataStart(0x88), Channels(0x82),
//   Compression(0x83), End(0xFF).
//
// Audio codec: EA-XA R2 ("eaxa_blk") — 15-byte compressed blocks (28 samples)
//   or 61-byte uncompressed blocks (first byte 0xEE). Shared DecodeEAXA().
//
// Reference: CrabJournal/ABK_Insert Program.cs, FFmpeg adpcm.c ADPCM_EA_R2.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/AudioCodecs.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
namespace ABKCodec {
    constexpr uint16_t PCM       = 0x0001;
    constexpr uint16_t IMA_ADPCM = 0x0011;
    constexpr uint16_t EAXA      = 0xEA01;
}

// ─────────────────────────────────────────────────────────────────────────────
/// EA-XA v2 ("eaxa", PCM-blocks revision) decoder used by NFSMW ABK banks.
/// Mono split stream; persistent ADPCM history across 15-byte (ADPCM) /
/// 61-byte (0xEE PCM) blocks of 28 samples each. Decodes up to `numSamples`
/// samples (0 = decode whole buffer).
/// Ported from vgmstream coding/ea_xa_decoder.c (decode_ea_xa_v2).
std::vector<int16_t> DecodeEAXA(const uint8_t* src, size_t srcSize,
                                  uint32_t channels, uint32_t numSamples = 0);

/// EA-XA v2 encoder (inverse of DecodeEAXA). Encodes mono s16 PCM to 15-byte
/// ADPCM frames (28 samples each); output size = ceil(numSamples/28)*15.
/// Brute-forces predictor/shift per frame; round-trip fidelity ~47-70 dB SNR.
/// `onProgress`, if set, is called periodically with a value in [0, 1] —
/// the brute-force search is the most expensive step in any replace path
/// that uses this codec (ABK, MUS), so callers running on a background
/// task should wire this to their ProgressState::fraction.
std::vector<uint8_t> EncodeEAXA(const int16_t* pcm, size_t numSamples,
                                 const std::function<void(float)>& onProgress = {});

// ─────────────────────────────────────────────────────────────────────────────
struct ABKEntry {
    uint32_t    id         = 0;
    std::string name;
    uint32_t    dataOffset = 0;  ///< byte offset from file start
    uint32_t    dataSize   = 0;
    uint32_t    sampleRate = 0;
    uint32_t    numSamples = 0;  ///< decoded PCM sample count (per channel)
    uint16_t    channels   = 1;
    uint16_t    codecTag   = ABKCodec::PCM;
    uint32_t    loopStart  = 0xFFFFFFFF;
    uint32_t    loopLength = 0;

    bool HasLoop() const { return loopStart != 0xFFFFFFFF && loopLength > 0; }

    const char* CodecName() const {
        switch (codecTag) {
            case ABKCodec::PCM:       return "PCM";
            case ABKCodec::IMA_ADPCM: return "IMA-ADPCM";
            case ABKCodec::EAXA:      return "EA-XA";
            default:                  return "Unknown";
        }
    }

    std::string DurationStr(uint32_t pcmSamples) const {
        if (sampleRate == 0) return "0:00";
        uint32_t s = pcmSamples / sampleRate;
        return std::to_string(s / 60) + ":" +
               (s % 60 < 10 ? "0" : "") + std::to_string(s % 60);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
struct ABKFile {
    std::string           name;
    std::vector<ABKEntry> entries;
    std::filesystem::path path;

    size_t EntryCount() const { return entries.size(); }
};

// ─────────────────────────────────────────────────────────────────────────────
class ABKParser {
public:
    static Result<ABKFile>              Load(const std::filesystem::path& path);

    /// Read the raw compressed payload for one entry directly from disk.
    static Result<std::vector<uint8_t>> ReadPayload(const ABKFile& bank,
                                                     const ABKEntry& entry);

    /// Decode a raw payload to interleaved s16 PCM.
    /// Fills outRate and outChannels.
    static Result<std::vector<int16_t>> DecodePCM(const std::vector<uint8_t>& raw,
                                                    const ABKEntry& entry,
                                                    uint32_t& outRate,
                                                    uint32_t& outChannels);

    /// Replace one entry's payload in the ABK file on disk.
    /// Patches fileSize at bytes 8–11 and preserves all other entries.
    /// Creates a .bak backup via BackupManager on first write.
    static Result<void> ReplacePayload(ABKFile& bank,
                                        ABKEntry& entry,
                                        const std::vector<uint8_t>& newRaw,
                                        const std::filesystem::path& backupDir);

    /// Import mono s16 PCM into an EA-XA entry (game-safe in-place replace).
    /// The PT chunk's SampleRate field is NOT patched on replace, so if
    /// `srcRate` differs from `entry.sampleRate` the PCM is first resampled
    /// to match — otherwise the clip would play back at the wrong pitch/
    /// speed in-game. The (possibly resampled) PCM is then fit to the
    /// entry's original sample count (truncated or zero-padded), EA-XA v2
    /// encoded, and zero-padded to the original byte size, so the file
    /// layout is unchanged (no risky container repack).
    /// `pcmMono` is interleaved-mono s16 at `srcRate` Hz.
    /// `onProgress`, if set, receives [0,1] progress through the EA-XA
    /// brute-force encode (the slow part of this call).
    static Result<AudioCodecs::ReplaceReport> ReplaceFromPCM(
        ABKFile& bank,
        ABKEntry& entry,
        const std::vector<int16_t>& pcmMono,
        uint32_t srcRate,
        const std::filesystem::path& backupDir,
        const AudioCodecs::ProgressFn& onProgress = {});
};

} // namespace nfsmw
