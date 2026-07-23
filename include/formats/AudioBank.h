#pragma once
// ─── formats/AudioBank.h ─────────────────────────────────────────────────────
// Parser for NFSMW audio banks.
//
// NFSMW (PC, v1.3) stores audio in two companion files:
//   *.SNR  – Sound Name / Routing table: maps sound IDs to names,
//             categories, and pointers into the SPT.
//   *.SPT  – Sound Payload Table: raw EA-MP3 / EASoundBank data blocks.
//   *.BUN  – Some audio data is embedded in BUN chunk streams under
//             chunk IDs 0x00E34009 (EASoundBank) and 0x80E34000 (container).
//
// This parser targets the SNR+SPT pair used by the game's AUDIO/ directory
// (e.g. AUDIO/RACE.SNR + AUDIO/RACE.SPT).
//
// SNR file layout (little-endian):
//   [0x00] uint32  magic          0x5246534E ("NSFR" LE)
//   [0x04] uint32  version        1
//   [0x08] uint32  entryCount
//   [0x0C] uint32  namePoolOffset (from file start)
//   [0x10] uint32  namePoolSize
//   followed by entryCount × SNREntry (32 bytes each)
//
// SNREntry (32 bytes):
//   [+0x00] uint32  id
//   [+0x04] uint32  nameOffset    (from namePoolOffset)
//   [+0x08] uint32  sptOffset     (byte offset into .SPT)
//   [+0x0C] uint32  sptSize       (byte length of payload)
//   [+0x10] uint32  sampleRate    (e.g. 22050, 44100)
//   [+0x14] uint16  channels      (1 = mono, 2 = stereo)
//   [+0x16] uint16  bitsPerSample (8 or 16)
//   [+0x18] uint32  codecTag      (0x0001=PCM, 0x0011=IMA-ADPCM, 0xEA01=EASndHdr)
//   [+0x1C] uint32  durationMs
//
// SPT payload: raw audio bytes, format described by codecTag.
// For in-tool playback we decode EASndHdr streams to PCM via a minimal
// EA-MP3 / EAADPCM decoder, then hand off to miniaudio for playback.
//
// User-facing round-trip (Phase 1):
//   Export  → ExportWAV()     — decodes entry → standard 16-bit PCM WAV
//   Replace → ReplaceFromWAV() — reads WAV, re-encodes to original codec,
//                                patches SPT + SNR in-place (with backup)
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Codec tag constants (same values used in the SNREntry header)
// ─────────────────────────────────────────────────────────────────────────────
namespace AudioCodec {
    constexpr uint32_t PCM        = 0x0001; ///< Raw signed-16 PCM
    constexpr uint32_t IMA_ADPCM  = 0x0011; ///< IMA / MS-ADPCM 4-bit
    constexpr uint32_t EA_EAADPCM = 0xEA01; ///< EA's proprietary ADPCM header
    constexpr uint32_t EA_MP3     = 0xEA03; ///< EASndHdr-wrapped MP3
    constexpr uint32_t MP3        = 0x0055; ///< Bare MPEG Layer-3
}

// ─────────────────────────────────────────────────────────────────────────────
/// One sound entry parsed from the SNR table.
// ─────────────────────────────────────────────────────────────────────────────
struct AudioEntry {
    uint32_t    id           = 0;
    std::string name;              ///< C-string from name pool
    uint32_t    sptOffset    = 0;  ///< byte offset into the companion .SPT
    uint32_t    sptSize      = 0;  ///< byte length of payload in .SPT
    uint32_t    sampleRate   = 0;
    uint16_t    channels     = 0;
    uint16_t    bitsPerSample= 0;
    uint32_t    codecTag     = 0;
    uint32_t    durationMs   = 0;

    /// Human-readable codec name.
    const char* CodecName() const {
        switch (codecTag) {
            case AudioCodec::PCM:        return "PCM";
            case AudioCodec::IMA_ADPCM:  return "IMA-ADPCM";
            case AudioCodec::EA_EAADPCM: return "EA-ADPCM";
            case AudioCodec::EA_MP3:     return "EA-MP3";
            case AudioCodec::MP3:        return "MP3";
            default:                     return "Unknown";
        }
    }

    /// True if ExportWAV / ReplaceFromWAV support this codec.
    bool SupportsWAVRoundTrip() const {
        return codecTag == AudioCodec::PCM || codecTag == AudioCodec::IMA_ADPCM;
    }

    /// Rough human duration string "m:ss".
    std::string DurationStr() const {
        uint32_t s = durationMs / 1000;
        return std::to_string(s / 60) + ":" +
               (s % 60 < 10 ? "0" : "") + std::to_string(s % 60);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Loaded audio bank (one SNR+SPT pair or a BUN audio chunk).
// ─────────────────────────────────────────────────────────────────────────────
struct AudioBank {
    std::string              name;     ///< stem of the SNR file
    std::vector<AudioEntry>  entries;
    std::filesystem::path    snrPath;
    std::filesystem::path    sptPath;  ///< companion SPT (may be empty for BUN)

    size_t EntryCount() const { return entries.size(); }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Parse an SNR+SPT pair from disk.
// ─────────────────────────────────────────────────────────────────────────────
class AudioBankParser {
public:
    /// Load an SNR file; locates the companion SPT automatically.
    static Result<AudioBank> LoadSNR(const std::filesystem::path& snrPath);

    /// Read the raw SPT payload for a single entry.
    /// Returns the raw bytes as stored in the SPT file.
    static Result<std::vector<uint8_t>> ReadPayload(const AudioBank& bank,
                                                     const AudioEntry& entry);

    /// Decode a raw payload to signed-16 interleaved PCM.
    /// Used by AudioPanel for in-tool preview via miniaudio.
    /// Returns interleaved s16 samples; fills outRate/outChannels.
    /// For codecs without an in-process decoder (EA-MP3, EA-ADPCM) returns an
    /// empty vector — the caller must fall back to miniaudio raw-byte decode.
    static Result<std::vector<int16_t>> DecodePCM(const std::vector<uint8_t>& raw,
                                                    const AudioEntry& entry,
                                                    uint32_t& outRate,
                                                    uint32_t& outChannels);

    /// Decode entry and write a standard 16-bit PCM WAV to dstPath.
    /// Supported codecs: PCM, IMA-ADPCM.
    /// For EA-MP3 / EA-ADPCM returns an error — use ExportRaw instead.
    static Result<void> ExportWAV(const AudioBank& bank,
                                   const AudioEntry& entry,
                                   const std::filesystem::path& dstPath);

    /// Write the raw SPT payload to dstPath (original codec bytes, no decode).
    /// Useful for codecs ExportWAV cannot handle (EA-MP3, EA-ADPCM).
    static Result<void> ExportRaw(const AudioBank& bank,
                                   const AudioEntry& entry,
                                   const std::filesystem::path& dstPath);

    /// Read a standard 16-bit PCM WAV, re-encode to the entry's original codec,
    /// and patch the SPT + SNR files (with one-time backup in backupDir).
    /// Also updates entry.sampleRate / channels / durationMs to match the WAV.
    /// Supported codecs: PCM, IMA-ADPCM.
    /// For EA-MP3 / EA-ADPCM returns an informative error.
    static Result<void> ReplaceFromWAV(AudioBank& bank,
                                        AudioEntry& entry,
                                        const std::filesystem::path& srcWav,
                                        const std::filesystem::path& backupDir);

    /// Replace the SPT payload for one entry with new raw bytes and patch the
    /// SNR's sptOffset/sptSize fields.  The SPT file is rebuilt in-place
    /// (all other entries are preserved).  A .bak is created the first time
    /// via BackupManager semantics.
    /// Used internally by ReplaceFromWAV and directly by advanced callers who
    /// already have a pre-encoded payload (e.g. EA-MP3 blobs).
    static Result<void> ReplacePayload(AudioBank& bank,
                                        AudioEntry& entry,
                                        const std::vector<uint8_t>& newPayload,
                                        const std::filesystem::path& backupDir);
};

} // namespace nfsmw
