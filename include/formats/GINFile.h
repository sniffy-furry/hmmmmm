#pragma once
// ─── formats/GINFile.h ────────────────────────────────────────────────────────
// Parser for EA GIN (.GIN) engine RPM-curve audio files.
//
// GIN files encode multiple short looping audio clips, each covering a
// range of engine RPM values, using EAXA encoding (same as ABK).
//
// High-level structure (partially opaque — based on id-daemon's research):
//   [0x00]  Assembly/header block (opaque, ~0x1C bytes)
//   [0x1C]  uint32  sampleCount
//   [0x20]  float   rpmMin
//   [0x24]  float   rpmMax
//   ...     sample offset table (one uint32 per clip)
//   ...     EAXA-encoded audio blocks (~900 samples each, looped)
//
// We parse enough to enumerate clips and their RPM ranges, then share the
// EAXA decoder from ABKFile.cpp for playback.
//
// NOTE: The exact header layout should be validated by hex-dumping real GIN
// files. The constants below reflect community research and may need tweaking.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/ABKFile.h"  // for DecodeEAXA
#include "formats/AudioCodecs.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
struct GINClip {
    float    rpmMin     = 0.f;
    float    rpmMax     = 0.f;
    uint32_t dataOffset = 0;   ///< byte offset from file start
    uint32_t dataSize   = 0;
    uint32_t numSamples = 0;   ///< decoded PCM sample count
    uint32_t sampleRate = 22050;
    uint16_t channels   = 1;

    std::string RPMRangeStr() const {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.0f–%.0f RPM", rpmMin, rpmMax);
        return buf;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
struct GINFile {
    std::string           name;
    std::vector<GINClip>  clips;
    std::filesystem::path path;
    std::vector<uint8_t>  fileData;   ///< entire file cached for payload reads

    size_t ClipCount() const { return clips.size(); }
};

// ─────────────────────────────────────────────────────────────────────────────
class GINParser {
public:
    static Result<GINFile> Load(const std::filesystem::path& path);

    /// Decode one clip to interleaved s16 PCM (via shared DecodeEAXA).
    /// Fills outRate and outChannels.
    static Result<std::vector<int16_t>> DecodePCM(const GINFile& gin,
                                                    const GINClip& clip,
                                                    uint32_t& outRate,
                                                    uint32_t& outChannels);

    /// Replace the audio stream of a GIN clip with new mono s16 PCM.
    ///
    /// The PCM is re-encoded to EA-XAS v0 (the native GIN codec), the file
    /// header's numSamples field is patched, and the stream tail is rewritten.
    /// The header block and RPM tables are preserved verbatim.
    ///
    /// Callers must create a .bak via BackupManager before calling this.
    /// `gin.fileData` and `clip` are updated in-place on success so the
    /// in-memory view stays consistent with the file on disk.
    /// Replace one clip in the GIN file with new audio.
    /// `pcmMono` is mono s16 PCM at `pcmRate` Hz.  If `pcmRate` differs from
    /// the clip's fixed sample rate the PCM is resampled before encoding so
    /// pitch and duration stay correct in-game.
    /// `onProgress`, if set, receives [0,1] progress through the EA-XAS encode.
    /// Callers must have already called BackupManager::EnsureFileBak.
    static Result<AudioCodecs::ReplaceReport> ReplaceClip(
        GINFile& gin,
        size_t clipIdx,
        const std::vector<int16_t>& pcmMono,
        uint32_t pcmRate,
        const AudioCodecs::ProgressFn& onProgress = {});
};

} // namespace nfsmw
