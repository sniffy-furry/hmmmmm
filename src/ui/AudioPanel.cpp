// ─── ui/AudioPanel.cpp ───────────────────────────────────────────────────────
//
// miniaudio is pulled in as a single-header lib vendored at:
//   vendor/miniaudio/miniaudio.h
// Define MA_IMPLEMENTATION exactly once (here).
// ─────────────────────────────────────────────────────────────────────────────
#define MINIAUDIO_IMPLEMENTATION
#include "vendor/miniaudio/miniaudio.h"

#include "ui/AudioPanel.h"
#include "formats/AudioBank.h"
#include "core/Logger.h"

#include <vangui.h>
#include <vangui_notify.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static bool CaseInsensitiveContains(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(hay.begin(), hay.end(), needle.begin(), needle.end(),
        [](char a, char b){ return std::tolower(static_cast<unsigned char>(a)) ==
                                   std::tolower(static_cast<unsigned char>(b)); });
    return it != hay.end();
}

// ─────────────────────────────────────────────────────────────────────────────
// miniaudio data callback — runs on the audio thread
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::MADataCallback(ma_device* pDevice, void* pOutput,
                                 const void* /*pInput*/, uint32_t frameCount) {
    auto* state = static_cast<AudioPlayState*>(pDevice->pUserData);
    if (!state || !state->playing) {
        std::memset(pOutput, 0, frameCount * state->channels * sizeof(int16_t));
        return;
    }

    auto* out     = static_cast<int16_t*>(pOutput);
    const size_t  total  = state->pcmData.size(); // total samples (all channels)
    const uint32_t ch    = state->channels;
    uint64_t cur         = state->cursor.load(std::memory_order_relaxed);

    for (uint32_t f = 0; f < frameCount; ++f) {
        const uint64_t sampleIdx = cur * ch;
        if (sampleIdx + ch > total) {
            // End of stream
            if (state->loop) {
                cur = 0;
            } else {
                state->playing = false;
                for (uint32_t c = 0; c < ch; ++c) *out++ = 0;
                continue;
            }
        }
        for (uint32_t c = 0; c < ch; ++c)
            *out++ = state->pcmData[static_cast<size_t>(cur * ch + c)];
        ++cur;
    }
    state->cursor.store(cur, std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / destructor
// ─────────────────────────────────────────────────────────────────────────────
AudioPanel::AudioPanel()
    : playState_(std::make_shared<AudioPlayState>()) {}

AudioPanel::~AudioPanel() {
    ShutdownAudioDevice();
}

// ─────────────────────────────────────────────────────────────────────────────
// InitAudioDevice / ShutdownAudioDevice
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::InitAudioDevice() {
    if (maDev_.IsInited()) return;

    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_s16;
    cfg.playback.channels = playState_->channels;
    cfg.sampleRate        = playState_->sampleRate;
    cfg.dataCallback      = MADataCallback;
    cfg.pUserData         = playState_.get();

    if (maDev_.Init(cfg, "AudioPanel")) {
        LOG_INFO("AudioPanel: miniaudio device initialised ({} Hz, {} ch)",
                 playState_->sampleRate, playState_->channels);
    }
}

void AudioPanel::ShutdownAudioDevice() {
    StopPlayback();
    maDev_.Shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
// OpenSNR
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::OpenSNR(const std::filesystem::path& path, TaskQueue& tasks) {
    loading_     = true;
    selectedIdx_ = -1;
    lastError_.clear();
    StopPlayback();

    tasks.Submit("Loading SNR", [this, path](nfsmw::ProgressState&) {
        auto res = AudioBankParser::LoadSNR(path);
        if (res) {
            bank_    = std::move(res.value);
        } else {
            lastError_ = res.error;
            LOG_WARN("AudioPanel: {}", res.error);
        }
        loading_ = false;
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// StartPlayback
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::StartPlayback(size_t entryIdx) {
    if (entryIdx >= bank_.entries.size()) return;
    const AudioEntry& entry = bank_.entries[entryIdx];

    StopPlayback();

    // Decode to PCM
    auto rawRes = AudioBankParser::ReadPayload(bank_, entry);
    if (!rawRes) {
        VanGui::NotifyError("%s", std::string("Read failed: " + rawRes.error).c_str());
        return;
    }

    uint32_t rate = 0, ch = 0;
    auto pcmRes = AudioBankParser::DecodePCM(rawRes.value, entry, rate, ch);
    if (!pcmRes) {
        VanGui::NotifyError("%s", std::string("Decode failed: " + pcmRes.error).c_str());
        return;
    }

    {
        std::lock_guard<std::mutex> lk(playState_->mtx);
        playState_->pcmData    = std::move(pcmRes.value);
        playState_->rawData    = std::move(rawRes.value);
        playState_->sampleRate = rate > 0 ? rate : 22050;
        playState_->channels   = ch  > 0 ? ch   : 1;
        playState_->cursor     = 0;
        playState_->playing    = false;
    }

    // Rebuild waveform thumbnail
    BuildWaveThumb(playState_->pcmData);

    // Re-init device if format changed
    if (maDev_.IsInited()) ShutdownAudioDevice();
    InitAudioDevice();

    if (!maDev_.IsInited()) {
        VanGui::NotifyError("%s", std::string("Audio device unavailable").c_str());
        return;
    }

    playState_->playing = true;
    if (!maDev_.Start()) {
        playState_->playing = false;
        VanGui::NotifyError("%s", std::string("Playback start failed").c_str());
    }
}

void AudioPanel::StopPlayback() {
    if (playState_) playState_->playing = false;
    maDev_.Stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// BuildWaveThumb  — 256-bucket peak envelope
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::BuildWaveThumb(const std::vector<int16_t>& pcm) {
    constexpr size_t kBuckets = 256;
    waveThumb_.assign(kBuckets, 0.0f);
    if (pcm.empty()) return;

    const size_t samplesPerBucket = std::max<size_t>(1, pcm.size() / kBuckets);
    for (size_t b = 0; b < kBuckets; ++b) {
        const size_t start = b * samplesPerBucket;
        const size_t end   = std::min(start + samplesPerBucket, pcm.size());
        float peak = 0.0f;
        for (size_t i = start; i < end; ++i)
            peak = std::max(peak, std::abs(static_cast<float>(pcm[i]) / 32768.0f));
        waveThumb_[b] = peak;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DoExportWAV
//
// Decodes the selected entry and writes a standard 16-bit PCM WAV.
// For codecs without an in-process decoder (EA-MP3, EA-ADPCM) falls back to
// DoExportRaw and shows an informative toast.
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::DoExportWAV(size_t entryIdx, const std::filesystem::path& dstPath) {
    if (entryIdx >= bank_.entries.size()) return;
    const AudioEntry& entry = bank_.entries[entryIdx];

    auto res = AudioBankParser::ExportWAV(bank_, entry, dstPath);
    if (res) {
        VanGui::NotifySuccess("%s", std::string("Exported '" + entry.name + "' → " + dstPath.filename().string()).c_str());
        return;
    }

    // ExportWAV failed (codec not supported for in-process decode).
    // Fall back to raw export with a changed extension so the user knows.
    VanGui::NotifyWarning("%s", std::string(entry.name + " (" + entry.CodecName() + "): WAV export not available — "
        "saving raw payload instead.").c_str());

    std::filesystem::path rawPath = dstPath;
    rawPath.replace_extension(".bin");
    auto rawRes = AudioBankParser::ExportRaw(bank_, entry, rawPath);
    if (rawRes) {
        VanGui::NotifyInfo("%s", std::string("Exported raw payload → " + rawPath.filename().string()).c_str());
    } else {
        VanGui::NotifyError("%s", std::string("Export failed: " + rawRes.error).c_str());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DoReplaceFromWAV
//
// Reads the user-supplied WAV, re-encodes it to the entry's original codec,
// and patches the SPT + SNR on disk. On success the in-memory entry and
// the waveform thumbnail are refreshed so the UI reflects the new content
// immediately without requiring a reload.
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::DoReplaceFromWAV(size_t entryIdx, const std::filesystem::path& srcPath,
                                   TaskQueue& tasks) {
    tasks.Submit("Replacing audio", [this, entryIdx, srcPath](nfsmw::ProgressState&) {
        if (entryIdx >= bank_.entries.size()) return;
        AudioEntry& entry = bank_.entries[entryIdx];

        const auto backupDir = bank_.snrPath.parent_path() / "audio_backups";
        auto res = AudioBankParser::ReplaceFromWAV(bank_, entry, srcPath, backupDir);

        if (!res) {
            lastError_ = res.error;
            VanGui::NotifyError("%s", std::string("Replace failed: " + res.error).c_str());
            return;
        }

        // Refresh waveform thumbnail from the newly written entry.
        // Reload from disk so the thumb reflects what the game will actually read.
        auto rawRes = AudioBankParser::ReadPayload(bank_, entry);
        if (rawRes) {
            uint32_t rate = 0, ch = 0;
            auto pcmRes = AudioBankParser::DecodePCM(rawRes.value, entry, rate, ch);
            if (pcmRes && !pcmRes.value.empty())
                BuildWaveThumb(pcmRes.value);
        }
        waveEntryIdx_ = static_cast<int>(entryIdx); // mark thumb as current

        VanGui::NotifySuccess("%s", std::string("Replaced '" + entry.name + "' (" +
            std::to_string(entry.sptSize) + " B, " +
            std::to_string(entry.sampleRate) + " Hz " +
            std::to_string(entry.channels) + "ch). Backup in audio_backups/").c_str());
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawBrowser  — "Audio Browser" docked window
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::DrawBrowser() {
    if (loading_) {
        VanGui::TextDisabled("Loading…");
        return;
    }
    if (bank_.entries.empty()) {
        VanGui::TextDisabled("No audio bank loaded.");
        VanGui::TextDisabled("File → Open Audio (SNR)…");
        return;
    }

    // ── Filter bar ────────────────────────────────────────────────────────────
    VanGui::SetNextItemWidth(-1.0f);
    char filterBuf[128];
    snprintf(filterBuf, sizeof(filterBuf), "%s", filter_.c_str());
    if (VanGui::InputTextWithHint("##audiofilter", "Filter sounds…",
                                  filterBuf, sizeof(filterBuf)))
        filter_ = filterBuf;

    VanGui::Separator();

    // ── Sound list ────────────────────────────────────────────────────────────
    const float rowH = VanGui::GetTextLineHeightWithSpacing();
    (void)VanGui::BeginChild("##audiolist", VanVec2(0, 0), false,
                       VanGuiWindowFlags_HorizontalScrollbar);

    const VanVec4 colPlaying(0.35f, 0.90f, 0.45f, 1.0f);
    const VanVec4 colCodec  (0.55f, 0.65f, 0.90f, 1.0f);
    const VanVec4 colNoWAV  (0.70f, 0.50f, 0.30f, 1.0f); // amber — codec not WAV-capable

    for (int i = 0; i < static_cast<int>(bank_.entries.size()); ++i) {
        const AudioEntry& e = bank_.entries[static_cast<size_t>(i)];
        if (!CaseInsensitiveContains(e.name, filter_)) continue;

        const bool sel     = (selectedIdx_ == i);
        const bool playing = sel && playState_ && playState_->playing;

        if (playing) VanGui::PushStyleColor(VanGuiCol_Text, colPlaying);
        bool clicked = VanGui::Selectable(("##row" + std::to_string(i)).c_str(),
                                          sel, 0, VanVec2(0, rowH));
        if (playing) VanGui::PopStyleColor();

        if (clicked) selectedIdx_ = i;

        VanGui::SameLine(4.0f);
        if (playing) {
            VanGui::TextColored(colPlaying, "▶ ");
            VanGui::SameLine();
        }
        VanGui::Text("%s", e.name.c_str());

        VanGui::SameLine(VanGui::GetContentRegionAvail().x - 70.0f);
        // Dim codec badge for entries that don't yet support WAV round-trip
        VanGui::TextColored(e.SupportsWAVRoundTrip() ? colCodec : colNoWAV,
                           "%s", e.CodecName());
    }

    VanGui::EndChild();
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawPreview  — "Audio Preview" docked window
// ─────────────────────────────────────────────────────────────────────────────
void AudioPanel::DrawPreview(TaskQueue& tasks) {
    if (bank_.entries.empty() || selectedIdx_ < 0 ||
        selectedIdx_ >= static_cast<int>(bank_.entries.size())) {
        VanGui::TextDisabled("Select a sound to preview.");
        return;
    }

    const AudioEntry& entry = bank_.entries[static_cast<size_t>(selectedIdx_)];

    // ── Metadata card ─────────────────────────────────────────────────────────
    VanGui::PushStyleColor(VanGuiCol_ChildBg, VanVec4(0.10f, 0.12f, 0.20f, 1.0f));
    (void)VanGui::BeginChild("##audiometa", VanVec2(0, 90), true);
    VanGui::TextUnformatted(entry.name.c_str());
    VanGui::Separator();
    VanGui::Columns(3, "metacols", false);
    VanGui::Text("Codec: %s",     entry.CodecName());
    VanGui::Text("Rate:  %u Hz",  entry.sampleRate);
    VanGui::NextColumn();
    VanGui::Text("Ch:     %u",    entry.channels);
    VanGui::Text("Bits:   %u",    entry.bitsPerSample);
    VanGui::NextColumn();
    VanGui::Text("Size:  %u B",   entry.sptSize);
    VanGui::Text("Dur:   %s",     entry.DurationStr().c_str());
    VanGui::Columns(1);
    VanGui::EndChild();
    VanGui::PopStyleColor();

    // ── Waveform thumbnail ────────────────────────────────────────────────────
    if (waveEntryIdx_ != selectedIdx_) {
        waveThumb_.assign(256, 0.0f);
        waveEntryIdx_ = selectedIdx_;
    }
    if (!waveThumb_.empty()) {
        const VanVec2 thumbSize(VanGui::GetContentRegionAvail().x, 60.0f);
        VanDrawList* dl = VanGui::GetWindowDrawList();
        const VanVec2 p = VanGui::GetCursorScreenPos();
        dl->AddRectFilled(p, VanVec2(p.x + thumbSize.x, p.y + thumbSize.y),
                           VAN_COL32(14, 16, 28, 255));
        const float midY   = p.y + thumbSize.y * 0.5f;
        const float scaleX = thumbSize.x / static_cast<float>(waveThumb_.size());
        const VanU32 waveCol= VAN_COL32(91, 140, 255, 200);
        for (size_t b = 0; b < waveThumb_.size(); ++b) {
            const float x   = p.x + static_cast<float>(b) * scaleX;
            const float amp = waveThumb_[b] * thumbSize.y * 0.48f;
            dl->AddLine(VanVec2(x, midY - amp), VanVec2(x, midY + amp), waveCol, 1.0f);
        }
        // Playback cursor
        if (playState_ && playState_->playing && !playState_->pcmData.empty()) {
            const float frac = static_cast<float>(playState_->cursor.load()) /
                               static_cast<float>(playState_->pcmData.size() /
                                                  std::max<uint32_t>(1, playState_->channels));
            const float cx = p.x + frac * thumbSize.x;
            dl->AddLine(VanVec2(cx, p.y), VanVec2(cx, p.y + thumbSize.y),
                        VAN_COL32(255, 220, 80, 220), 1.5f);
        }
        VanGui::Dummy(thumbSize);
    }

    VanGui::Spacing();

    // ── Transport controls ────────────────────────────────────────────────────
    const bool playing = playState_ && playState_->playing;

    if (playing) {
        if (VanGui::Button("⏹ Stop", VanVec2(80, 0)))
            StopPlayback();
    } else {
        if (VanGui::Button("▶ Play", VanVec2(80, 0)))
            StartPlayback(static_cast<size_t>(selectedIdx_));
    }

    VanGui::SameLine();
    bool loop = playState_ ? playState_->loop.load() : false;
    if (VanGui::Checkbox("Loop", &loop) && playState_)
        playState_->loop = loop;

    // ── Seek bar ──────────────────────────────────────────────────────────────
    if (playState_ && !playState_->pcmData.empty()) {
        const uint64_t totalFrames = playState_->pcmData.size() /
                                     std::max<uint32_t>(1, playState_->channels);
        float frac = totalFrames > 0
            ? static_cast<float>(playState_->cursor.load()) /
              static_cast<float>(totalFrames)
            : 0.0f;
        VanGui::SetNextItemWidth(-1.0f);
        if (VanGui::SliderFloat("##seek", &frac, 0.0f, 1.0f, "")) {
            playState_->cursor = static_cast<uint64_t>(frac * static_cast<float>(totalFrames));
        }
    }

    VanGui::Spacing();
    VanGui::Separator();
    VanGui::Spacing();

    const int capturedIdx = selectedIdx_;
    const bool wavCapable = entry.SupportsWAVRoundTrip();

    // ── Export WAV ────────────────────────────────────────────────────────────
    if (VanGui::Button("Export WAV…", VanVec2(110, 0))) {
        if (fileDialog_) {
            const std::string defName = entry.name + ".wav";
            fileDialog_->Show("Export Audio as WAV",
                              FileDialog::Mode::Save,
                              {".wav"},
                              [this, capturedIdx](const std::filesystem::path& p) {
                                  DoExportWAV(static_cast<size_t>(capturedIdx), p);
                              }, defName);
        }
    }
    VanGui::SameLine();
    if (wavCapable) {
        VanGui::TextDisabled("16-bit PCM WAV");
    } else {
        VanGui::TextColored(VanVec4(0.8f, 0.6f, 0.2f, 1.0f),
                           "WAV unavailable for %s — raw .bin saved instead",
                           entry.CodecName());
    }

    VanGui::Spacing();

    // ── Replace from WAV ──────────────────────────────────────────────────────
    if (!wavCapable) VanGui::BeginDisabled();
    if (VanGui::Button("Replace WAV…", VanVec2(110, 0))) {
        if (fileDialog_) {
            fileDialog_->Show("Replace Audio — choose WAV",
                              FileDialog::Mode::Open,
                              {".wav"},
                              [this, &tasks, capturedIdx](const std::filesystem::path& p) {
                                  DoReplaceFromWAV(static_cast<size_t>(capturedIdx), p, tasks);
                              });
        }
    }
    if (!wavCapable) {
        VanGui::EndDisabled();
        VanGui::SameLine();
        VanGui::TextColored(VanVec4(0.8f, 0.6f, 0.2f, 1.0f),
                           "%s re-encode not yet supported", entry.CodecName());
    } else {
        VanGui::SameLine();
        VanGui::TextDisabled("16-bit PCM WAV → re-encode → patch ABK");
    }

    VanGui::Spacing();

    // ── Export raw (advanced) ─────────────────────────────────────────────────
    if (VanGui::TreeNode("Advanced")) {
        if (VanGui::Button("Export raw payload…", VanVec2(150, 0))) {
            if (fileDialog_) {
                const std::string defName = entry.name + ".bin";
                fileDialog_->Show("Export raw SPT payload",
                                  FileDialog::Mode::Save,
                                  {".bin", ".raw"},
                                  [this, capturedIdx](const std::filesystem::path& p) {
                                      const AudioEntry& e =
                                          bank_.entries[static_cast<size_t>(capturedIdx)];
                                      auto res = AudioBankParser::ExportRaw(bank_, e, p);
                                      if (res) {
                                          VanGui::NotifyInfo("%s", std::string("Exported raw → " + p.filename().string()).c_str());
                                      } else {
                                          VanGui::NotifyError("%s", std::string("Export failed: " + res.error).c_str());
                                      }
                                  }, defName);
            }
        }
        VanGui::SameLine();
        VanGui::TextDisabled("Original codec bytes, no decode");
        VanGui::TreePop();
    }
}

} // namespace nfsmw
