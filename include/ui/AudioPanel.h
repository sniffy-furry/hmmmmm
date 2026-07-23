#pragma once
// ─── ui/AudioPanel.h ──────────────────────────────────────────────────────────
// VanGui panel for audio bank browsing, playback, export, and replacement.
//
// Layout (docked into the standard two-column layout):
//
//  Left panel  → "Audio Browser"  (sound list, filter box, codec badges)
//  Centre panel → "Audio Preview"  (waveform thumb, transport controls,
//                                    export / replace buttons, metadata)
//
// Playback is driven by miniaudio (header-only, vendored at vendor/miniaudio).
// The panel keeps one miniaudio device alive for the lifetime of the loaded
// bank; switching entries stops any current playback and queues a new decode.
//
// User-facing round-trip (Phase 1):
//
//   Export WAV workflow:
//     1. User selects an entry in the browser list.
//     2. Clicks "Export WAV…" → save dialog (default name = entry.name + ".wav").
//     3. AudioBankParser::ExportWAV decodes the SPT payload to PCM and writes
//        a standard 16-bit RIFF WAV.
//     4. For codecs without an in-process decoder (EA-MP3, EA-ADPCM) the panel
//        falls back to a raw .bin export and shows a warning toast.
//
//   Replace WAV workflow:
//     1. User selects the same entry.
//     2. Clicks "Replace WAV…" → open dialog filtered to .wav.
//        (Button is disabled with an explanation for unsupported codecs.)
//     3. AudioBankParser::ReplaceFromWAV reads the WAV, re-encodes to the
//        entry's original codec (PCM → raw s16, IMA-ADPCM → MS-IMA blocks),
//        patches the SPT slot (in-place if same size or smaller, else appended),
//        rewrites the SNR with updated sptOffset/sptSize/rate/channels/duration,
//        and creates a one-time backup in AUDIO/audio_backups/.
//     4. Toast confirms the replacement; waveform thumbnail refreshes.
//
//   Advanced section (collapsed tree node):
//     "Export raw payload…" — saves the original codec bytes as .bin for power
//     users or debugging, regardless of codec support.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/AudioBank.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include "core/MiniaudioDevice.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

// Forward-declare miniaudio device handle so the header stays clean even if
// miniaudio.h is not yet visible to callers.
struct ma_device;

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
/// Audio playback state (shared between the audio thread and the UI thread).
// ─────────────────────────────────────────────────────────────────────────────
struct AudioPlayState {
    std::vector<int16_t>  pcmData;       ///< decoded s16 interleaved samples
    std::vector<uint8_t>  rawData;       ///< original bytes (fallback for ma_decoder)
    uint32_t              sampleRate  = 44100;
    uint32_t              channels    = 1;
    std::atomic<uint64_t> cursor      = 0; ///< current read position in samples (per channel)
    std::atomic<bool>     playing     = false;
    std::atomic<bool>     loop        = false;
    std::mutex            mtx;
};

// ─────────────────────────────────────────────────────────────────────────────
class AudioPanel {
public:
    AudioPanel();
    ~AudioPanel();

    // Non-copyable / non-movable (owns an ma_device)
    AudioPanel(const AudioPanel&) = delete;
    AudioPanel& operator=(const AudioPanel&) = delete;

    /// Open an SNR file (locates companion SPT automatically).
    void OpenSNR(const std::filesystem::path& path, TaskQueue& tasks);

    /// Draw the left-side sound list ("Audio Browser" window).
    void DrawBrowser();

    /// Draw the centre preview / controls ("Audio Preview" window).
    void DrawPreview(TaskQueue& tasks);

    /// Wire in the shared toast manager and file dialog from AppShell.
    void SetFileDialog(FileDialog* fd) { fileDialog_ = fd; }

    bool IsLoaded()  const { return !bank_.entries.empty(); }
    bool IsLoading() const { return loading_; }

    const std::string& LastError() const { return lastError_; }

    void SetTaskQueue(TaskQueue& q) { (void)q; } // compatibility shim

private:
    // ── Data ─────────────────────────────────────────────────────────────────
    AudioBank            bank_;
    int                  selectedIdx_  = -1;
    std::string          filter_;
    std::string          lastError_;
    std::atomic<bool>    loading_      = false;

    // ── Playback ──────────────────────────────────────────────────────────────
    std::shared_ptr<AudioPlayState> playState_;
    MiniaudioDevice                   maDev_;     ///< miniaudio device (RAII)

    // ── UI helpers ────────────────────────────────────────────────────────────
    FileDialog*    fileDialog_ = nullptr;

    // ── Waveform thumbnail ────────────────────────────────────────────────────
    // Cached per selected entry to avoid re-computing every frame.
    std::vector<float> waveThumb_;   ///< normalised [-1,1], 256 samples
    int                waveEntryIdx_ = -2;

    // ── Internal ──────────────────────────────────────────────────────────────
    void  StartPlayback(size_t entryIndex);
    void  StopPlayback();
    void  BuildWaveThumb(const std::vector<int16_t>& pcm);
    void  InitAudioDevice();
    void  ShutdownAudioDevice();

    /// Decode entry to PCM and write a 16-bit WAV. Falls back to raw .bin
    /// export for codecs without an in-process decoder, with a warning toast.
    void  DoExportWAV(size_t entryIndex, const std::filesystem::path& dstPath);

    /// Read srcPath as a 16-bit PCM WAV, re-encode to the entry's original
    /// codec, and patch the SPT + SNR files on disk.
    void  DoReplaceFromWAV(size_t entryIndex, const std::filesystem::path& srcPath,
                            TaskQueue& tasks);

    /// miniaudio data-callback trampoline (static, passed to ma_device_config).
    static void MADataCallback(ma_device* pDevice, void* pOutput,
                               const void* pInput, uint32_t frameCount);
};

} // namespace nfsmw
