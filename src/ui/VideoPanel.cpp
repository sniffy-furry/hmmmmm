#include "ui/VideoPanel.h"
#include "core/Logger.h"
#include "core/StringUtil.h"

#include <vangui.h>
#include <vangui_notify.h>
#include "renderer/GLCompat.h"
#include "vendor/miniaudio/miniaudio.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace nfsmw {

VideoPanel::VideoPanel() {
    audio_ = std::make_shared<VideoAudioState>();
}

VideoPanel::~VideoPanel() {
    // GL teardown must happen in Shutdown() (context current); just stop audio.
    ShutdownAudioDevice();
}

void VideoPanel::Shutdown() {
    StopPlayback();
    ShutdownAudioDevice();
    if (frameTex_) {
        glDeleteTextures(1, &frameTex_);
        frameTex_ = 0;
    }
    texW_ = texH_ = 0;
    video_.reset();
    pending_.reset();
}

// ─── Open ───────────────────────────────────────────────────────────────────

void VideoPanel::OpenFile(const std::filesystem::path& path, TaskQueue& tasks) {
    // Scan the folder for sibling .vp6 files so the browser lists the whole set.
    folder_.clear();
    selected_ = -1;
    std::error_code ec;
    const auto dir = path.parent_path();
    for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
        if (!e.is_regular_file()) continue;
        if (ToLowerAscii(e.path().extension().string()) == ".vp6")
            folder_.push_back(e.path());
    }
    std::sort(folder_.begin(), folder_.end());
    for (int i = 0; i < (int)folder_.size(); ++i)
        if (folder_[i] == path) selected_ = i;

    loading_      = true;
    lastError_.clear();
    pendingReady_ = false;
    tasks_        = &tasks;
    const std::string p = path.string();

    tasks.Submit("Loading " + path.filename().string(),
        [this, p](ProgressState& ps) {
            ps.fraction = -1.0f;
            auto vf = std::make_unique<VideoFile>();
            auto r  = vf->Open(std::filesystem::path(p));
            ps.fraction = 1.0f;
            if (!r) {
                lastError_ = r.error;
                loading_   = false;
                return;
            }
            pending_      = std::move(vf);
            pendingPath_  = p;
            pendingReady_.store(true, std::memory_order_release);
        });
}

void VideoPanel::PumpPending() {
    if (!pendingReady_.exchange(false, std::memory_order_acquire)) return;

    StopPlayback();
    if (video_) video_->Close();
    video_      = std::move(pending_);
    loadedPath_ = pendingPath_;
    loading_    = false;
    lastFramePts_ = -1.0;
    clock_        = 0.0;
    ended_        = false;
    paused_       = false;

    // Hand the pre-decoded audio to the playback state.
    const auto& info = video_->Info();
    {
        audio_->pcm        = video_->AudioPCM();
        audio_->sampleRate = info.hasAudio ? (uint32_t)info.audioSampleRate : 48000;
        audio_->channels   = info.hasAudio ? (uint32_t)info.audioChannels   : 2;
        audio_->cursor     = 0;
        audio_->playing    = false;
    }
    StartPlayback();
}

// ─── Playback clock ──────────────────────────────────────────────────────────

double VideoPanel::ClockSeconds() const {
    if (video_ && video_->Info().hasAudio && !audio_->pcm.empty())
        return (double)audio_->cursor.load(std::memory_order_relaxed) / audio_->sampleRate;
    return clock_;
}

void VideoPanel::StartPlayback() {
    if (!video_) return;
    lastTick_ = VanGui::GetTime();
    paused_   = false;
    ended_    = false;
    if (video_->Info().hasAudio && !audio_->pcm.empty()) {
        if (maDev_.IsInited()) ShutdownAudioDevice();
        InitAudioDevice();
        audio_->cursor  = 0;
        audio_->playing = true;
        maDev_.Start();
    } else {
        clock_ = 0.0;
    }
}

void VideoPanel::StopPlayback() {
    if (audio_) audio_->playing = false;
    maDev_.Stop();
}

// ─── miniaudio ───────────────────────────────────────────────────────────────

void VideoPanel::MADataCallback(ma_device* dev, void* outPtr, const void*, uint32_t frames) {
    auto* st = static_cast<VideoAudioState*>(dev->pUserData);
    auto* out = static_cast<int16_t*>(outPtr);
    const uint32_t ch = st ? st->channels : 2;
    if (!st || !st->playing) { std::memset(outPtr, 0, frames * ch * sizeof(int16_t)); return; }

    const size_t total = st->pcm.size();
    uint64_t cur = st->cursor.load(std::memory_order_relaxed);
    for (uint32_t f = 0; f < frames; ++f) {
        const uint64_t base = cur * ch;
        if (base + ch > total) {           // past end: clamp + silence (loop handled in UI)
            for (uint32_t c = 0; c < ch; ++c) *out++ = 0;
            continue;
        }
        for (uint32_t c = 0; c < ch; ++c) *out++ = st->pcm[(size_t)(base + c)];
        ++cur;
    }
    st->cursor.store(cur, std::memory_order_relaxed);
}

void VideoPanel::InitAudioDevice() {
    if (maDev_.IsInited()) return;
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_s16;
    cfg.playback.channels = audio_->channels;
    cfg.sampleRate        = audio_->sampleRate;
    cfg.dataCallback      = MADataCallback;
    cfg.pUserData         = audio_.get();
    maDev_.Init(cfg, "VideoPanel");
}

void VideoPanel::ShutdownAudioDevice() {
    StopPlayback();
    maDev_.Shutdown();
}

// ─── GL frame upload ─────────────────────────────────────────────────────────

void VideoPanel::UploadFrame() {
    if (!video_ || frameRGBA_.empty()) return;
    const int w = video_->Info().width, h = video_->Info().height;
    if (w <= 0 || h <= 0) return;

    if (!frameTex_) {
        glGenTextures(1, &frameTex_);
        glBindTexture(GL_TEXTURE_2D, frameTex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        texW_ = texH_ = 0;
    }
    glBindTexture(GL_TEXTURE_2D, frameTex_);
    if (w != texW_ || h != texH_) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     frameRGBA_.data());
        texW_ = w; texH_ = h;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                        frameRGBA_.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ─── Browser ─────────────────────────────────────────────────────────────────

void VideoPanel::DrawBrowser() {
    PumpPending();

    if (folder_.empty()) {
        VanGui::TextDisabled("No videos.\nUse \"Open Video (VP6)\xe2\x80\xa6\".");
        return;
    }

    VanGui::TextDisabled("%zu video(s) in folder", folder_.size());
    VanGui::Separator();

    (void)VanGui::BeginChild("##VidList", VanVec2(0, 0), false);
    for (int i = 0; i < (int)folder_.size(); ++i) {
        const bool sel = (i == selected_);
        if (VanGui::Selectable(folder_[i].filename().string().c_str(), sel)) {
            selected_ = i;
            // OpenFile needs the TaskQueue, which DrawBrowser doesn't have — defer
            // the actual load to DrawPreview (called the same frame).
            loadRequest_ = folder_[i];
        }
        if (VanGui::IsItemHovered())
            VanGui::SetTooltip("%s", folder_[i].string().c_str());
    }
    VanGui::EndChild();
}

// ─── Preview + transport ─────────────────────────────────────────────────────

void VideoPanel::DrawPreview(TaskQueue& tasks) {
    PumpPending();

    // Deferred load request from the browser (needs the TaskQueue).
    if (!loadRequest_.empty()) {
        auto req = loadRequest_;
        loadRequest_.clear();
        OpenFile(req, tasks);
    }

    if (loading_) {
        const int dot = (int)(VanGui::GetTime() * 4.0) % 4;
        VanGui::TextColored(VanVec4(0.98f, 0.55f, 0.23f, 1.0f), "Loading video%.*s", dot, "...");
        return;
    }
    if (!lastError_.empty() && !IsLoaded()) {
        VanGui::TextColored(VanVec4(1, 0.4f, 0.4f, 1), "Error: %s", lastError_.c_str());
        return;
    }
    if (!IsLoaded()) {
        VanGui::TextDisabled("Open a .vp6 movie to play it.");
        return;
    }

    const VideoInfo& info = video_->Info();

    // ── Advance the video to the current clock time ──────────────────────────
    const double now = VanGui::GetTime();
    if (!paused_ && !ended_) {
        if (!(info.hasAudio && !audio_->pcm.empty()))
            clock_ += (now - lastTick_);   // wall-clock pacing when silent
    }
    lastTick_ = now;

    const double clock = ClockSeconds();
    bool newFrame = false;
    if (!paused_ && !ended_) {
        int guard = 0;
        // Decode forward until the displayed frame matches the clock (cap catch-up).
        while ((lastFramePts_ < clock || lastFramePts_ < 0.0) && guard++ < 8) {
            double pts = 0.0;
            if (video_->NextFrame(frameRGBA_, pts)) {
                lastFramePts_ = pts;
                newFrame = true;
            } else {
                // End of stream.
                if (audio_->loop.load()) {
                    video_->Restart();
                    lastFramePts_ = -1.0;
                    clock_ = 0.0;
                    audio_->cursor = 0;
                } else {
                    ended_ = true;
                    StopPlayback();
                }
                break;
            }
        }
    }
    if (newFrame) UploadFrame();

    // ── Draw the frame, scaled to fit the available area ─────────────────────
    VanVec2 avail = VanGui::GetContentRegionAvail();
    const float reserveH = 86.0f;            // transport + metadata
    avail.y = std::max(avail.y - reserveH, 60.0f);
    if (frameTex_ && info.width > 0 && info.height > 0) {
        const float ar  = (float)info.width / (float)info.height;
        float w = avail.x, h = w / ar;
        if (h > avail.y) { h = avail.y; w = h * ar; }
        VanGui::SetCursorPosX(VanGui::GetCursorPosX() + (avail.x - w) * 0.5f);
        VanGui::Image((VanTextureID)(uintptr_t)frameTex_, VanVec2(w, h));
    } else {
        VanGui::Dummy(VanVec2(avail.x, avail.y));
        VanGui::TextDisabled("Decoding first frame\xe2\x80\xa6");
    }

    // ── Transport ────────────────────────────────────────────────────────────
    VanGui::Separator();
    if (VanGui::Button(paused_ ? "\xe2\x96\xb6 Play" : "\xe2\x8f\xb8 Pause", VanVec2(90, 0))) {
        paused_ = !paused_;
        if (info.hasAudio && !audio_->pcm.empty()) {
            audio_->playing = !paused_;
            if (maDev_.IsInited()) {
                if (paused_) maDev_.Stop();
                else         maDev_.Start();
            }
        }
        if (!paused_) ended_ = false;
        lastTick_ = VanGui::GetTime();
    }
    VanGui::SameLine();
    if (VanGui::Button("\xe2\x9f\xb2 Restart", VanVec2(90, 0))) {
        video_->Restart();
        lastFramePts_ = -1.0;
        clock_ = 0.0;
        ended_ = false; paused_ = false;
        audio_->cursor = 0;
        audio_->playing = info.hasAudio && !audio_->pcm.empty();
        maDev_.Start();
        lastTick_ = VanGui::GetTime();
    }
    VanGui::SameLine();
    bool loop = audio_->loop.load();
    if (VanGui::Checkbox("Loop", &loop)) audio_->loop.store(loop);

    // Time read-out.
    VanGui::SameLine();
    VanGui::Text("  %5.1f / %5.1f s%s", clock, info.durationSec, ended_ ? "  (end)" : "");

    // ── Metadata + actions ───────────────────────────────────────────────────
    VanGui::Separator();
    VanGui::Text("%s", std::filesystem::path(loadedPath_).filename().string().c_str());
    std::string codecs = info.videoCodec;
    if (info.hasAudio) codecs += " / " + info.audioCodec;
    VanGui::TextDisabled("%dx%d  %.2f fps  %s  %.0f KB",
                        info.width, info.height, info.fps, codecs.c_str(),
                        info.fileSize / 1024.0);

    if (VanGui::Button("Export as MP4\xe2\x80\xa6")) {
        if (fileDialog_) {
            const std::string stem =
                std::filesystem::path(loadedPath_).stem().string() + ".mp4";
            fileDialog_->Show("Export video as MP4", FileDialog::Mode::Save, {".mp4"},
                              [this](const std::filesystem::path& dst) {
                                  DoExportMP4(dst);
                              }, stem);
        }
    }
    VanGui::SameLine();
    if (VanGui::Button("Replace from MP4\xe2\x80\xa6")) {
        if (fileDialog_)
            fileDialog_->Show("Replace video from MP4", FileDialog::Mode::Open, {".mp4"},
                              [this, &tasks](const std::filesystem::path& src) {
                                  DoImportMP4(src, tasks);
                              });
    }
    VanGui::SameLine();
    if (VanGui::Button("Replace raw VP6\xe2\x80\xa6")) {
        if (fileDialog_)
            fileDialog_->Show("Replace video (raw VP6)", FileDialog::Mode::Open, {".vp6"},
                              [this](const std::filesystem::path& src) { ReplaceSelected(src); });
    }
    VanGui::SameLine();
    const bool canRevert = backup_.HasBak(loadedPath_);
    if (!canRevert) VanGui::BeginDisabled();
    if (VanGui::Button("Revert")) {
        std::error_code ec;
        std::filesystem::copy_file(BackupManager::BakPath(loadedPath_), loadedPath_,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) VanGui::NotifyError("Revert failed");
        else    VanGui::NotifySuccess("Reverted to backup");
        if (!ec) OpenFile(loadedPath_, tasks);
    }
    if (!canRevert) VanGui::EndDisabled();
}

// ─── Replace ─────────────────────────────────────────────────────────────────

void VideoPanel::ReplaceSelected(const std::filesystem::path& src) {
    if (loadedPath_.empty()) return;
    backup_.EnsureFileBak(loadedPath_);
    std::error_code ec;
    std::filesystem::copy_file(src, loadedPath_,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        VanGui::NotifyError("%s", std::string("Replace failed: " + ec.message()).c_str());
        return;
    }
    VanGui::NotifySuccess("%s", std::string("Replaced " +
        std::filesystem::path(loadedPath_).filename().string() +
        " (backup saved)").c_str());
    loadRequest_ = loadedPath_;   // reload via DrawPreview's TaskQueue next frame
}

// ─── MP4 Export ──────────────────────────────────────────────────────────────

void VideoPanel::DoExportMP4(const std::filesystem::path& dst) {
    if (loadedPath_.empty()) return;

    // Transcode runs on a background task so the UI stays responsive.
    const std::filesystem::path src = loadedPath_;
    VanGui::NotifyInfo("%s", std::string("Exporting MP4\xe2\x80\xa6").c_str());

    if (tasks_) {
        // Result is reported on the main thread in onDone — never call VanGui from
        // a worker thread.
        auto ok   = std::make_shared<bool>(false);
        auto err  = std::make_shared<std::string>();
        auto name = dst.filename().string();
        tasks_->Submit("Exporting MP4",
            [src, dst, ok, err](ProgressState& ps) {
                ps.fraction = -1.0f;   // indeterminate
                auto res = VideoFile::ExportMP4(src, dst);
                ps.fraction = 1.0f;
                *ok = static_cast<bool>(res);
                if (!res) *err = res.error;
            },
            [ok, err, name]() {
                if (*ok) VanGui::NotifySuccess("%s", std::string("Exported " + name).c_str());
                else     VanGui::NotifyError("%s", std::string("Export failed: " + *err).c_str());
            });
    } else {
        // Synchronous fallback (should not normally happen)
        auto res = VideoFile::ExportMP4(src, dst);
        if (res) VanGui::NotifySuccess("%s", std::string("Exported " + dst.filename().string()).c_str());
        else     VanGui::NotifyError("%s", std::string("Export failed: " + res.error).c_str());
    }
}

// ─── MP4 Import ──────────────────────────────────────────────────────────────

void VideoPanel::DoImportMP4(const std::filesystem::path& src, TaskQueue& tasks) {
    if (loadedPath_.empty()) return;

    const std::filesystem::path dst = loadedPath_;
    backup_.EnsureFileBak(dst);

    VanGui::NotifyInfo("Transcoding to VP6\xe2\x80\xa6");

    auto ok   = std::make_shared<bool>(false);
    auto err  = std::make_shared<std::string>();
    auto name = dst.filename().string();

    tasks.Submit("Transcoding VP6",
      [this, src, dst, ok, err](ProgressState& ps) {
        ps.fraction = -1.0f;

        // Probe the original VP6 for its resolution and fps so we can match them.
        int   origW = 0, origH = 0;
        double origFps = 0.0;
        {
            VideoFile probe;
            auto r = probe.Open(dst.parent_path() / (dst.stem().string() + ".bak"));
            if (!r) {
                // .bak not readable — just probe the current (soon-overwritten) target.
                VideoFile p2;
                if (p2.Open(dst)) {
                    origW   = p2.Info().width;
                    origH   = p2.Info().height;
                    origFps = p2.Info().fps;
                }
            } else {
                origW   = probe.Info().width;
                origH   = probe.Info().height;
                origFps = probe.Info().fps;
            }
        }

        auto res = VideoFile::ImportMP4(src, dst, origW, origH, origFps);
        ps.fraction = 1.0f;

        *ok = static_cast<bool>(res);
        if (!res) { *err = res.error; return; }
        // Reload the newly written VP6 on the next frame
        loadRequest_ = dst;
      },
      [ok, err, name]() {
        if (*ok) VanGui::NotifySuccess("%s", std::string("Replaced " + name + " (backup saved)").c_str());
        else     VanGui::NotifyError("%s", std::string("Import failed: " + *err).c_str());
      });
}

} // namespace nfsmw