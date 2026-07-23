#pragma once
// ─── formats/MUSFile.h ────────────────────────────────────────────────────────
// Parser for EA MUS music stream files used in NFS:MW.
//
// A MUS file is a sequence of DWORD-aligned EA-stream (ASF) sections:
//   SCHl  header block   → EACSHeader + codec metadata
//   SCCl  continuation   → optional extra format info (skipped)
//   SCDl  data block(s)  → compressed audio payload (accumulated)
//   SCEl  end block      → finalize section, advance to DWORD boundary
//
// Codec tags in EACSHeader::compression:
//   0x00  PCM (raw s16)
//   0x02  EA-ADPCM (EA's IMA variant, HIGH nibble first)
//   0xEA  EA-MP3 (EASndHdr-wrapped MP3, requires minimp3)
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/AudioCodecs.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
/// Compression codec identifiers used in MUS SCHl headers.
// ─────────────────────────────────────────────────────────────────────────────
namespace MUSCodec {
    constexpr uint8_t PCM       = 0x00;
    constexpr uint8_t EA_ADPCM  = 0x02;
    constexpr uint8_t MICROTALK = 0x04;  ///< EA MicroTalk-family speech (copspeech.big; codec2 tag 0x04)
    constexpr uint8_t EA_XA     = 0x0A;  ///< EA-XA v2 (NFSMW music/NIS; SCHl blocked)
    constexpr uint8_t EA_MP3    = 0xEA;
}

// ─────────────────────────────────────────────────────────────────────────────
/// One parsed section from a MUS file.
// ─────────────────────────────────────────────────────────────────────────────
struct MUSSection {
    uint32_t             index       = 0;
    uint32_t             sampleRate  = 0;
    uint8_t              bits        = 2;        ///< 1=8-bit, 2=16-bit
    uint8_t              channels    = 1;
    uint8_t              compression = MUSCodec::PCM;
    uint32_t             numSamples  = 0;
    uint32_t             loopStart   = 0xFFFFFFFF;
    uint32_t             loopLength  = 0;
    std::vector<uint8_t> payload;                ///< (legacy/unused for SCHl streams; decoded lazily)
    size_t               fileOffset  = 0;        ///< byte offset of SCHl in MUS file
    size_t               blockSpan   = 0;        ///< bytes from SCHl to end of SCEl (segment size)
    std::filesystem::path srcPath;               ///< source .mus for on-demand decode

    bool HasLoop() const { return loopStart != 0xFFFFFFFF && loopLength > 0; }

    const char* CodecName() const {
        switch (compression) {
            case MUSCodec::PCM:       return "PCM";
            case MUSCodec::EA_ADPCM:  return "EA-ADPCM";
            case MUSCodec::MICROTALK: return "EA MicroTalk";
            case MUSCodec::EA_XA:     return "EA-XA";
            case MUSCodec::EA_MP3:    return "EA-MP3";
            default:                  return "Unknown";
        }
    }

    /// Approximate duration in seconds (best-effort).
    float DurationSec() const {
        if (sampleRate == 0) return 0.f;
        if (numSamples > 0)  return static_cast<float>(numSamples) / sampleRate;
        // Estimate from payload for EA-ADPCM: each block yields ~1014 samples
        if (compression == MUSCodec::EA_ADPCM && channels > 0) {
            const size_t blockSize = (channels == 2) ? 36u : 20u; // header + 16 bytes nibbles
            size_t blocks = payload.size() / blockSize;
            return static_cast<float>(blocks * 28u) / sampleRate; // 28 nibble-pairs per block
        }
        return 0.f;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Loaded MUS file: a collection of sequenced audio sections.
// ─────────────────────────────────────────────────────────────────────────────
struct MUSFile {
    std::string               name;
    std::vector<MUSSection>   sections;
    std::filesystem::path     path;

    size_t SectionCount() const { return sections.size(); }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Parse and decode MUS files.
// ─────────────────────────────────────────────────────────────────────────────
class MUSParser {
public:
    /// Walk the SCHl/SCDl/SCEl block structure and populate sections.
    static Result<MUSFile> Load(const std::filesystem::path& path);

    /// Decode one section to interleaved signed-16 PCM.
    /// Fills outRate and outChannels.
    static Result<std::vector<int16_t>> DecodePCM(const MUSSection& sec,
                                                    uint32_t& outRate,
                                                    uint32_t& outChannels);

    /// Read a standard RIFF/WAVE file (16-bit PCM) into interleaved s16 PCM.
    /// Multi-channel input is preserved (outChannels is set accordingly).
    /// Fills outRate and outChannels.
    static Result<std::vector<int16_t>> ReadWAV(const std::filesystem::path& path,
                                                  uint32_t& outRate,
                                                  uint32_t& outChannels);

    /// Write interleaved s16 PCM to a standard RIFF/WAVE file.
    static bool WriteWAV(const std::filesystem::path& dst,
                          const std::vector<int16_t>& pcm,
                          uint32_t sampleRate,
                          uint32_t channels);

    /// Replace one section's audio in the MUS file on disk.
    ///
    /// The supplied interleaved s16 PCM (at the section's original sample rate)
    /// is re-encoded to EA-XA v2 per-channel, the SCHl/SCDl/SCEl block sequence
    /// is rebuilt with updated numSamples and sizes, and the MUS file is rewritten
    /// Replace one MUS section with new audio supplied as interleaved 16-bit PCM.
    ///
    /// `pcmRate` is the source WAV's sample rate.  If it differs from the
    /// section's fixed rate the PCM is resampled before encoding so pitch and
    /// duration stay correct in-game (the SCHl rate tag is NOT patched —
    /// resampling to the original rate is the correct fix).
    ///
    /// `onProgress`, if set, receives [0,1] encode progress (useful for long
    /// sections submitted via TaskQueue).
    ///
    /// Rebuilds the SCHl/SCDl/SCEl block sequence, patches numSamples in the
    /// SCHl header, and rewrites the MUS file with all following section
    /// offsets adjusted accordingly.
    ///
    /// EA-MP3 sections fall back to a raw-swap error (no encoder available).
    /// Callers must have already created a .bak via BackupManager.
    static Result<AudioCodecs::ReplaceReport> ReplaceSection(
        MUSFile& mus,
        size_t sectionIdx,
        const std::vector<int16_t>& pcmInterleaved,
        uint32_t pcmRate,
        uint32_t pcmChannels,
        const AudioCodecs::ProgressFn& onProgress = {});
};

} // namespace nfsmw