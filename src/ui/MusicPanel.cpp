// ─── ui/MusicPanel.cpp ────────────────────────────────────────────────────────
// VanGui panel for interactive-music browsing: MPF event graph + MUS sections.
//
// Mirrors SoundBankPanel architecture: shared miniaudio device, MPPlayState,
// waveform thumbnail, transport controls. Two browser sub-tabs ("Events" and
// "Sections") drive selection of a MUSSection, which is decoded and played.
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/MusicPanel.h"
#include "formats/AudioCodecs.h"
#include "core/Logger.h"

// miniaudio.h — implementation is in AudioPanel.cpp; here we only need the API.
#include "vendor/miniaudio/miniaudio.h"

#include <vangui.h>
#include <vangui_notify.h>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace nfsmw {

// ─── Per-section WAV export ───────────────────────────────────────────────────

void MusicPanel::DoExportSectionWAV(int sectionIdx, const std::filesystem::path& dstPath) {
    if (sectionIdx < 0 || sectionIdx >= static_cast<int>(mus_.sections.size())) return;

    const MUSSection& sec = mus_.sections[static_cast<size_t>(sectionIdx)];
    uint32_t rate = 0, ch = 0;
    auto pcmRes = MUSParser::DecodePCM(sec, rate, ch);
    if (!pcmRes) {
        VanGui::NotifyError("%s", std::string("Export failed: " + pcmRes.error).c_str());
        return;
    }
    if (!MUSParser::WriteWAV(dstPath, pcmRes.value, rate, ch)) {
        VanGui::NotifyError("%s", std::string("WAV write failed").c_str());
        return;
    }
    VanGui::NotifySuccess("%s", std::string("Exported " + dstPath.filename().string()).c_str());
}

// ─── Per-section WAV import ───────────────────────────────────────────────────

void MusicPanel::DoImportSectionWAV(int sectionIdx, const std::filesystem::path& srcWav) {
    if (sectionIdx < 0 || sectionIdx >= static_cast<int>(mus_.sections.size())) return;
    if (musPath_.empty()) {
        VanGui::NotifyError("%s", std::string("No MUS path set").c_str());
        return;
    }

    StopPlayback();

    // Read the source WAV on the UI thread (fast — just file I/O and format parse)
    uint32_t srcRate = 0, srcCh = 0;
    auto pcmRes = MUSParser::ReadWAV(srcWav, srcRate, srcCh);
    if (!pcmRes) {
        VanGui::NotifyError("%s", std::string("WAV read failed: " + pcmRes.error).c_str());
        return;
    }

    if (!tasks_) {
        VanGui::NotifyError("%s", std::string("No task queue available.").c_str());
        return;
    }

    // Ensure a .bak exists before modifying (do this on UI thread before async work starts)
    backup_.EnsureFileBak(musPath_);

    const std::string sectionLabel = "Section " + std::to_string(sectionIdx);

    // Work on a private copy of the MUS handle (path + lazily-decoded section
    // metadata — cheap) so the worker never touches mus_ while the UI thread is
    // iterating it every frame. ReplaceSection reloads the handle in place, which
    // reallocates the sections vector — doing that under the UI thread's feet was
    // a data race / use-after-free. Results are applied on the UI thread (onDone).
    auto workMus = std::make_shared<MUSFile>(mus_);
    auto result  = std::make_shared<Result<AudioCodecs::ReplaceReport>>(
                       Result<AudioCodecs::ReplaceReport>::Err("not started"));
    const std::string musPathStr = musPath_.string();

    tasks_->Submit("Replacing " + sectionLabel,
        [workMus, result, sectionIdx, pcmVec = std::move(pcmRes.value),
         srcRate, srcCh](ProgressState& ps) mutable {
            AudioCodecs::ProgressFn prog = [&ps](float f) { ps.fraction = f; };
            *result = MUSParser::ReplaceSection(*workMus, static_cast<size_t>(sectionIdx),
                                                pcmVec, srcRate, srcCh, prog);
        },
        [this, workMus, result, sectionLabel, musPathStr]() {
            if (!*result) {
                VanGui::NotifyError("%s", std::string("Replace failed: " + result->error).c_str());
                return;
            }
            // Adopt the reloaded handle if the same MUS is still open.
            if (musPath_.string() == musPathStr) {
                mus_ = std::move(*workMus);
                waveSectionIdx_ = -2;   // rebuild waveform thumbnail next draw
            }
            const std::string summary = result->value.Summary();
            std::string msg = sectionLabel + " replaced (backup saved)";
            if (!summary.empty()) msg += " [" + summary + "]";
            VanGui::NotifySuccess("%s", std::string(msg).c_str());
        });
}

// ─── Case-insensitive substring search ───────────────────────────────────────

static bool CaseInsensitiveContains(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(),   needle.end(),
        [](char a, char b){ return std::toupper((unsigned char)a) ==
                                    std::toupper((unsigned char)b); });
    return it != haystack.end();
}

// ─── Constructor / Destructor ─────────────────────────────────────────────────

MusicPanel::MusicPanel()
    : playState_(std::make_shared<MPPlayState>()) {}

MusicPanel::~MusicPanel() {
    ShutdownAudioDevice();
}

// ─── OpenMPF ─────────────────────────────────────────────────────────────────

void MusicPanel::OpenMPF(const std::filesystem::path& mpfPath,
                          const std::filesystem::path& musPath,
                          TaskQueue& tasks)
{
    loading_         = true;
    selectedEvent_   = -1;
    selectedSection_ = -1;
    lastError_.clear();
    StopPlayback();

    // Locate the companion MUS: explicit path, or same stem/dir as the MPF.
    std::filesystem::path resolvedMus = musPath;
    if (resolvedMus.empty()) {
        resolvedMus = mpfPath;
        resolvedMus.replace_extension(".mus");
        if (!std::filesystem::exists(resolvedMus)) {
            std::filesystem::path alt = mpfPath;
            alt.replace_extension(".MUS");
            if (std::filesystem::exists(alt)) resolvedMus = alt;
        }
    }

    mpfPath_ = mpfPath;
    musPath_ = resolvedMus;
    tasks_   = &tasks;

    tasks.Submit("Loading MPF/MUS", [this, mpfPath, resolvedMus](nfsmw::ProgressState&) {
        auto musRes = MUSParser::Load(resolvedMus);
        if (!musRes) {
            lastError_ = "MUS load failed: " + musRes.error;
            mus_ = MUSFile{};
            mpf_ = MPFFile{};
            LOG_WARN("MusicPanel: {}", lastError_);
            loading_ = false;
            return;
        }
        mus_ = std::move(musRes.value);

        // MPF (event graph) is optional — the section list drives playback, so a
        // missing/unparsed MPF must not block the browser. Only attempt it when
        // an actual .mpf was given.
        mpf_ = MPFFile{};
        const std::string ext = mpfPath.extension().string();
        const bool looksMpf = (ext == ".mpf" || ext == ".MPF");
        if (looksMpf) {
            auto mpfRes = MPFParser::Load(mpfPath);
            if (mpfRes) {
                mpf_ = std::move(mpfRes.value);
                MPFParser::Resolve(mpf_, mus_);
            } else {
                LOG_WARN("MusicPanel: MPF load failed ({}); showing MUS segments only",
                         mpfRes.error);
            }
        }

        LOG_INFO("MusicPanel: loaded {} sections, {} events, {} nodes",
                 mus_.sections.size(), mpf_.events.size(), mpf_.nodes.size());

        if (!mus_.sections.empty()) selectedSection_ = 0;
        loading_ = false;
    });
}

// ─── miniaudio integration ────────────────────────────────────────────────────

void MusicPanel::MADataCallback(ma_device* pDevice, void* pOutput,
                                 const void* /*pInput*/, uint32_t frameCount)
{
    auto* state = static_cast<MPPlayState*>(pDevice->pUserData);
    if (!state || !state->playing) {
        const uint32_t ch = state ? state->channels : 1;
        std::memset(pOutput, 0, frameCount * ch * sizeof(int16_t));
        return;
    }

    auto* out           = static_cast<int16_t*>(pOutput);
    const size_t  total = state->pcmData.size();
    const uint32_t ch   = state->channels;
    uint64_t cur        = state->cursor.load(std::memory_order_relaxed);

    for (uint32_t f = 0; f < frameCount; ++f) {
        const uint64_t sampleIdx = cur * ch;
        if (sampleIdx + ch > total) {
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

void MusicPanel::InitAudioDevice() {
    if (maDev_.IsInited()) return;

    ma_device_config cfg   = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_s16;
    cfg.playback.channels = playState_->channels;
    cfg.sampleRate        = playState_->sampleRate;
    cfg.dataCallback      = MADataCallback;
    cfg.pUserData         = playState_.get();

    maDev_.Init(cfg, "MusicPanel");
}

void MusicPanel::ShutdownAudioDevice() {
    StopPlayback();
    maDev_.Shutdown();
}

// ─── Playback ────────────────────────────────────────────────────────────────

void MusicPanel::PlaySection(int sectionIdx) {
    StopPlayback();

    if (sectionIdx < 0 || sectionIdx >= static_cast<int>(mus_.sections.size()))
        return;

    const MUSSection& sec = mus_.sections[static_cast<size_t>(sectionIdx)];

    uint32_t rate = 0, ch = 0;
    auto pcmRes = MUSParser::DecodePCM(sec, rate, ch);
    if (!pcmRes) {
        VanGui::NotifyError("%s", std::string("Decode failed: " + pcmRes.error).c_str());
        return;
    }

    {
        std::lock_guard<std::mutex> lk(playState_->mtx);
        playState_->pcmData    = std::move(pcmRes.value);
        playState_->sampleRate = rate > 0 ? rate : 44100;
        playState_->channels   = ch  > 0 ? ch   : 1;
        playState_->cursor     = 0;
        playState_->playing    = false;
    }

    BuildWaveThumb(playState_->pcmData);

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

void MusicPanel::StopPlayback() {
    if (playState_) playState_->playing = false;
    maDev_.Stop();
}

// ─── Waveform thumbnail ───────────────────────────────────────────────────────

void MusicPanel::BuildWaveThumb(const std::vector<int16_t>& pcm) {
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

// ─── DrawBrowser ─────────────────────────────────────────────────────────────

void MusicPanel::DrawBrowser() {
    if (loading_) {
        VanGui::TextDisabled("Loading…");
        return;
    }
    if (!lastError_.empty()) {
        VanGui::TextColored(VanVec4(1, 0.4f, 0.4f, 1), "Error: %s", lastError_.c_str());
        return;
    }
    if (!IsLoaded()) {
        VanGui::TextDisabled("No music loaded.");
        VanGui::TextDisabled("Audio → Open MPF+MUS…");
        return;
    }

    // ── File name / badge ──────────────────────────────────────────────────────
    VanGui::TextColored(VanVec4(0.55f, 0.65f, 0.90f, 1.0f), "[MUS]");
    VanGui::SameLine();
    VanGui::TextUnformatted(mus_.name.c_str());
    VanGui::SameLine();
    VanGui::TextDisabled("(%zu sections, %zu events)",
                         mus_.sections.size(), mpf_.events.size());
    VanGui::Separator();

    // ── Sub-tab toggle ────────────────────────────────────────────────────────
    if (VanGui::RadioButton("Events", showEventView_))   showEventView_ = true;
    VanGui::SameLine();
    if (VanGui::RadioButton("Sections", !showEventView_)) showEventView_ = false;
    VanGui::Separator();

    if (showEventView_) {
        // ── Filter bar ───────────────────────────────────────────────────────
        VanGui::SetNextItemWidth(-1.0f);
        char filterBuf[128];
        snprintf(filterBuf, sizeof(filterBuf), "%s", eventFilter_.c_str());
        if (VanGui::InputTextWithHint("##mpfilter", "Filter events…", filterBuf, sizeof(filterBuf)))
            eventFilter_ = filterBuf;

        VanGui::Separator();

        // ── Event list ───────────────────────────────────────────────────────
        const float rowH = VanGui::GetTextLineHeightWithSpacing();
        (void)VanGui::BeginChild("##mpevents", VanVec2(0, 0), false,
                           VanGuiWindowFlags_HorizontalScrollbar);

        const VanVec4 colPlaying(0.35f, 0.90f, 0.45f, 1.0f);
        const VanVec4 colSection(0.55f, 0.65f, 0.90f, 1.0f);

        for (size_t i = 0; i < mpf_.events.size(); ++i) {
            const MPFEvent& ev = mpf_.events[i];
            const std::string name = !ev.name.empty()
                ? ev.name
                : ("Event 0x" + [&]{ char b[16]; std::snprintf(b, sizeof(b), "%08X", ev.eventID); return std::string(b); }());

            if (!CaseInsensitiveContains(name, eventFilter_)) continue;

            const bool sel = (selectedEvent_ == static_cast<int>(i));

            // Resolve mapped section index (if any)
            int mappedSection = -1;
            auto it = mpf_.eventToSection.find(ev.eventID);
            if (it != mpf_.eventToSection.end())
                mappedSection = static_cast<int>(it->second);

            const bool playing = sel && playState_ && playState_->playing;

            if (playing) VanGui::PushStyleColor(VanGuiCol_Text, colPlaying);
            bool clicked = VanGui::Selectable(("##mpevrow" + std::to_string(i)).c_str(),
                                              sel, 0, VanVec2(0, rowH));
            if (playing) VanGui::PopStyleColor();

            if (clicked) {
                selectedEvent_ = static_cast<int>(i);
                if (mappedSection >= 0) {
                    selectedSection_ = mappedSection;
                    waveSectionIdx_  = -2; // invalidate waveform
                    PlaySection(mappedSection);
                }
            }

            VanGui::SameLine(4.0f);
            if (playing) { VanGui::TextColored(colPlaying, "▶ "); VanGui::SameLine(); }
            VanGui::Text("%s", name.c_str());

            // Right-aligned "→ section N" indicator
            const float availW = VanGui::GetContentRegionAvail().x;
            VanGui::SameLine(availW - 100.0f);
            if (mappedSection >= 0)
                VanGui::TextColored(colSection, "-> sec %d", mappedSection);
            else
                VanGui::TextDisabled("(unmapped)");
        }

        VanGui::EndChild();
    } else {
        // ── Section list ─────────────────────────────────────────────────────
        const float rowH = VanGui::GetTextLineHeightWithSpacing();
        (void)VanGui::BeginChild("##mpsections", VanVec2(0, 0), false,
                           VanGuiWindowFlags_HorizontalScrollbar);

        const VanVec4 colPlaying(0.35f, 0.90f, 0.45f, 1.0f);
        const VanVec4 colCodec  (0.55f, 0.65f, 0.90f, 1.0f);
        const VanVec4 colExtra  (0.65f, 0.65f, 0.65f, 1.0f);

        for (size_t i = 0; i < mus_.sections.size(); ++i) {
            const MUSSection& sec = mus_.sections[i];
            const bool sel     = (selectedSection_ == static_cast<int>(i));
            const bool playing = sel && playState_ && playState_->playing;

            if (playing) VanGui::PushStyleColor(VanGuiCol_Text, colPlaying);
            bool clicked = VanGui::Selectable(("##mpsecrow" + std::to_string(i)).c_str(),
                                              sel, 0, VanVec2(0, rowH));
            if (playing) VanGui::PopStyleColor();

            if (clicked) {
                selectedSection_ = static_cast<int>(i);
                waveSectionIdx_  = -2; // invalidate waveform
            }

            if (VanGui::IsItemHovered() && VanGui::IsMouseDoubleClicked(0))
                PlaySection(static_cast<int>(i));

            VanGui::SameLine(4.0f);
            if (playing) { VanGui::TextColored(colPlaying, "▶ "); VanGui::SameLine(); }
            VanGui::Text("Section %u", sec.index);

            // Right-aligned codec + sample-rate / duration columns
            const float availW = VanGui::GetContentRegionAvail().x;
            VanGui::SameLine(availW - 200.0f);
            VanGui::TextColored(colCodec, "%s", sec.CodecName());
            VanGui::SameLine(availW - 130.0f);
            VanGui::TextColored(colExtra, "%u Hz", sec.sampleRate);
            VanGui::SameLine(availW - 60.0f);
            const float dur = sec.DurationSec();
            VanGui::TextColored(colExtra, "%d:%02d", static_cast<int>(dur) / 60,
                                static_cast<int>(dur) % 60);
        }

        VanGui::EndChild();
    }
}

// ─── DrawPreview ─────────────────────────────────────────────────────────────

void MusicPanel::DrawPreview(TaskQueue& /*tasks*/) {
    if (!IsLoaded() || selectedSection_ < 0 ||
        selectedSection_ >= static_cast<int>(mus_.sections.size()))
    {
        VanGui::TextDisabled("Select an event or section to preview.");
        return;
    }

    const size_t idx = static_cast<size_t>(selectedSection_);
    const MUSSection& sec = mus_.sections[idx];

    // ── Metadata card ─────────────────────────────────────────────────────────
    VanGui::PushStyleColor(VanGuiCol_ChildBg, VanVec4(0.10f, 0.12f, 0.20f, 1.0f));
    (void)VanGui::BeginChild("##mpmeta", VanVec2(0, 78), true);

    if (selectedEvent_ >= 0 && selectedEvent_ < static_cast<int>(mpf_.events.size())) {
        const MPFEvent& ev = mpf_.events[static_cast<size_t>(selectedEvent_)];
        VanGui::TextUnformatted(!ev.name.empty() ? ev.name.c_str() : "(unnamed event)");
    } else {
        VanGui::Text("Section %u", sec.index);
    }
    VanGui::Separator();
    VanGui::Columns(3, "mpmetacols", false);
    VanGui::Text("Codec: %s", sec.CodecName());
    VanGui::NextColumn();
    VanGui::Text("Rate: %u Hz", sec.sampleRate);
    VanGui::NextColumn();
    VanGui::Text("Ch: %u", sec.channels);
    VanGui::NextColumn();
    if (sec.HasLoop())
        VanGui::Text("Loop: %u + %u", sec.loopStart, sec.loopLength);
    else
        VanGui::TextDisabled("Loop: none");
    VanGui::NextColumn();
    const float dur = sec.DurationSec();
    VanGui::Text("Duration: %d:%02d", static_cast<int>(dur) / 60, static_cast<int>(dur) % 60);
    VanGui::Columns(1);

    VanGui::EndChild();
    VanGui::PopStyleColor();

    // ── Waveform thumbnail (build lazily) ─────────────────────────────────────
    if (waveSectionIdx_ != selectedSection_) {
        waveThumb_.assign(256, 0.0f);
        waveSectionIdx_ = selectedSection_;
        uint32_t rate = 0, ch = 0;
        auto pcmRes = MUSParser::DecodePCM(sec, rate, ch);
        if (pcmRes) BuildWaveThumb(pcmRes.value);
    }

    if (!waveThumb_.empty()) {
        const VanVec2 thumbSize(VanGui::GetContentRegionAvail().x, 60.0f);
        VanDrawList* dl = VanGui::GetWindowDrawList();
        const VanVec2 p  = VanGui::GetCursorScreenPos();
        dl->AddRectFilled(p, VanVec2(p.x + thumbSize.x, p.y + thumbSize.y),
                           VAN_COL32(14, 16, 28, 255));

        const float midY   = p.y + thumbSize.y * 0.5f;
        const float scaleX = thumbSize.x / static_cast<float>(waveThumb_.size());
        const VanU32 waveCol = VAN_COL32(91, 140, 255, 200);
        for (size_t b = 0; b < waveThumb_.size(); ++b) {
            const float x   = p.x + static_cast<float>(b) * scaleX;
            const float amp = waveThumb_[b] * thumbSize.y * 0.48f;
            dl->AddLine(VanVec2(x, midY - amp), VanVec2(x, midY + amp), waveCol, 1.0f);
        }

        // Loop-point markers
        if (sec.HasLoop() && sec.numSamples > 0) {
            const float loopStartFrac = static_cast<float>(sec.loopStart) /
                                         static_cast<float>(sec.numSamples);
            const float loopEndFrac   = static_cast<float>(sec.loopStart + sec.loopLength) /
                                         static_cast<float>(sec.numSamples);
            const VanU32 loopCol = VAN_COL32(255, 160, 60, 200);
            dl->AddLine(VanVec2(p.x + loopStartFrac * thumbSize.x, p.y),
                        VanVec2(p.x + loopStartFrac * thumbSize.x, p.y + thumbSize.y), loopCol, 1.5f);
            dl->AddLine(VanVec2(p.x + loopEndFrac * thumbSize.x, p.y),
                        VanVec2(p.x + loopEndFrac * thumbSize.x, p.y + thumbSize.y), loopCol, 1.5f);
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
        if (VanGui::Button("⏹ Stop", VanVec2(80, 0))) StopPlayback();
    } else {
        if (VanGui::Button("▶ Play", VanVec2(80, 0))) PlaySection(selectedSection_);
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
        if (VanGui::SliderFloat("##mpseek", &frac, 0.0f, 1.0f, "")) {
            playState_->cursor = static_cast<uint64_t>(frac * static_cast<float>(totalFrames));
        }
    }

    // ── Per-section actions: Export WAV / Replace WAV ─────────────────────────
    VanGui::Separator();
    {
        const MUSSection& secSel = mus_.sections[static_cast<size_t>(selectedSection_)];
        const bool isMp3 = (secSel.compression == MUSCodec::EA_MP3);

        // Export selected section as WAV
        if (VanGui::Button("Export Section WAV\xe2\x80\xa6") && fileDialog_) {
            const std::string defaultName =
                "section_" + std::to_string(secSel.index) + ".wav";
            fileDialog_->Show("Export section as WAV", FileDialog::Mode::Save,
                              {".wav"},
                              [this](const std::filesystem::path& dst) {
                                  DoExportSectionWAV(selectedSection_, dst);
                              }, defaultName);
        }

        VanGui::SameLine();

        // Replace selected section with a WAV (not available for EA-MP3 sections)
        if (isMp3) VanGui::BeginDisabled();
        if (VanGui::Button("Replace Section WAV\xe2\x80\xa6") && fileDialog_) {
            fileDialog_->Show("Replace section from WAV", FileDialog::Mode::Open,
                              {".wav"},
                              [this](const std::filesystem::path& src) {
                                  DoImportSectionWAV(selectedSection_, src);
                              });
        }
        if (isMp3) {
            VanGui::EndDisabled();
            if (VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenDisabled))
                VanGui::SetTooltip("EA-MP3 sections cannot be re-encoded (no encoder available).\n"
                                  "Use 'Replace .mus\xe2\x80\xa6' to swap the whole stream file.");
        }
    }

    // ── File actions: whole-file export / replace / revert ────────────────────
    // Whole-file replacement is retained as an escape hatch when the user has a
    // pre-built compatible stream from their own audio pipeline.
    if (!musPath_.empty()) {
        VanGui::Separator();
        VanGui::TextDisabled("%s", musPath_.filename().string().c_str());

        if (VanGui::Button("Export .mus (whole)\xe2\x80\xa6") && fileDialog_) {
            const std::string nm = musPath_.filename().string();
            fileDialog_->Show("Export music stream", FileDialog::Mode::Save,
                              {".mus", ".big"},
                              [this](const std::filesystem::path& dst) {
                                  std::error_code ec;
                                  std::filesystem::copy_file(musPath_, dst,
                                      std::filesystem::copy_options::overwrite_existing, ec);
                                  if (ec) VanGui::NotifyError("Export failed");
                                  else    VanGui::NotifySuccess("%s", std::string("Exported " + dst.filename().string()).c_str());
                              }, nm);
        }
        VanGui::SameLine();
        if (VanGui::Button("Replace .mus (whole)\xe2\x80\xa6") && fileDialog_) {
            fileDialog_->Show("Replace music stream", FileDialog::Mode::Open,
                              {".mus", ".big"},
                              [this](const std::filesystem::path& src) {
                                  StopPlayback();
                                  backup_.EnsureFileBak(musPath_);
                                  std::error_code ec;
                                  std::filesystem::copy_file(src, musPath_,
                                      std::filesystem::copy_options::overwrite_existing, ec);
                                  if (ec) {
                                      VanGui::NotifyError("%s", std::string("Replace failed: " + ec.message()).c_str());
                                      return;
                                  }
                                  VanGui::NotifySuccess("%s", std::string("Replaced " +
                                      musPath_.filename().string() + " (backup saved)").c_str());
                                  if (tasks_) OpenMPF(mpfPath_, musPath_, *tasks_);
                              });
        }
        VanGui::SameLine();
        const bool canRevert = backup_.HasBak(musPath_);
        if (!canRevert) VanGui::BeginDisabled();
        if (VanGui::Button("Revert##mus")) {
            StopPlayback();
            std::error_code ec;
            std::filesystem::copy_file(BackupManager::BakPath(musPath_), musPath_,
                std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) VanGui::NotifyError("Revert failed");
            else    VanGui::NotifySuccess("Reverted to backup");
            if (!ec && tasks_) OpenMPF(mpfPath_, musPath_, *tasks_);
        }
        if (!canRevert) VanGui::EndDisabled();
    }
}

} // namespace nfsmw
                                        