#pragma once
// ─── ui/VideoPanel.h ──────────────────────────────────────────────────────────
// Browse, play, export and replace NFSMW movie files (MOVIES/*.vp6 — On2 VP6
// video + EA audio), mirroring the SoundBank/Music panel layout:
//
//   Left   → list of .vp6 files in the opened file's folder
//   Centre → live video preview (GL texture) + transport (play/pause/restart/loop)
//   Right  → metadata, Export raw copy, Replace from file (with .bak revert)
//
// Video is decoded on the UI thread frame-by-frame via formats/VideoFile (FFmpeg)
// and paced by the audio clock (miniaudio). Audio is pre-decoded to S16.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/VideoFile.h"
#include "ui/FileDialog.h"
#include "patch/BackupManager.h"
#include "async/TaskQueue.h"
#include "core/MiniaudioDevice.h"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <atomic>

struct ma_device;

namespace nfsmw {

// Shared between miniaudio's real-time callback and the UI thread.
struct VideoAudioState {
    std::vector<int16_t>  pcm;
    uint32_t              sampleRate = 48000;
    uint32_t              channels   = 2;
    std::atomic<uint64_t> cursor     = 0;     ///< frames played (= audio clock)
    std::atomic<bool>     playing    = false;
    std::atomic<bool>     loop       = true;
};

class VideoPanel {
public:
    VideoPanel();
    ~VideoPanel();
    VideoPanel(const VideoPanel&)            = delete;
    VideoPanel& operator=(const VideoPanel&) = delete;

    /// Load a .vp6 (and scan its folder for siblings). Async open via `tasks`.
    void OpenFile(const std::filesystem::path& path, TaskQueue& tasks);

    void DrawBrowser();
    void DrawPreview(TaskQueue& tasks);

    void SetFileDialog(FileDialog* fd) { fileDialog_ = fd; }

    bool IsLoaded()  const { return video_ && video_->IsOpen(); }
    bool IsLoading() const { return loading_; }

    /// Release GL + audio resources. Call while the GL context is still current.
    void Shutdown();

private:
    // ── Loaded video ─────────────────────────────────────────────────────────
    std::unique_ptr<VideoFile>   video_;
    std::string                  loadedPath_;
    std::vector<std::filesystem::path> folder_;   ///< sibling .vp6 files
    int                          selected_ = -1;
    std::filesystem::path        loadRequest_;    ///< deferred load (browser → preview)
    TaskQueue*                   tasks_    = nullptr; ///< remembered for reload-after-import

    // Async open staging (worker → UI)
    std::unique_ptr<VideoFile>   pending_;
    std::string                  pendingPath_;
    std::atomic<bool>            pendingReady_ = false;
    std::atomic<bool>            loading_      = false;
    std::string                  lastError_;

    // ── Playback clock ───────────────────────────────────────────────────────
    std::shared_ptr<VideoAudioState> audio_;
    MiniaudioDevice               maDev_;
    double                       lastFramePts_ = -1.0;
    double                       clock_   = 0.0;   ///< playback position (seconds)
    double                       lastTick_ = 0.0;  ///< wall time of last advance
    bool                         paused_  = false;
    bool                         ended_   = false;

    // ── GL frame texture ─────────────────────────────────────────────────────
    uint32_t                     frameTex_ = 0;
    int                          texW_ = 0, texH_ = 0;
    std::vector<uint8_t>         frameRGBA_;

    FileDialog*    fileDialog_ = nullptr;
    BackupManager  backup_;

    // helpers
    void PumpPending();
    void StartPlayback();
    void StopPlayback();
    void InitAudioDevice();
    void ShutdownAudioDevice();
    void UploadFrame();
    double ClockSeconds() const;

    void ExportSelected();
    void ReplaceSelected(const std::filesystem::path& src);   ///< raw VP6 swap (escape hatch)
    void DoExportMP4(const std::filesystem::path& dst);       ///< transcode → H.264/AAC MP4
    void DoImportMP4(const std::filesystem::path& src,        ///< transcode MP4 → VP6
                     TaskQueue& tasks);

    static void MADataCallback(ma_device* dev, void* out, const void* in, uint32_t frames);
};

} // namespace nfsmw
