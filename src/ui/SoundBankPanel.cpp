// ─── ui/SoundBankPanel.cpp ───────────────────────────────────────────────────
// VanGui panel for browsing and playing EA ABK sound-effect banks and GIN
// engine RPM-curve audio files.
//
// Mirrors AudioPanel architecture: shared miniaudio device, SBPlayState,
// waveform thumbnail, transport controls.  Dispatch between ABK and GIN is
// handled internally via std::variant + BankKind enum.
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/SoundBankPanel.h"
#include "core/Logger.h"

// miniaudio.h — implementation is in AudioPanel.cpp; here we only need the API.
#include "vendor/miniaudio/miniaudio.h"

#include <vangui.h>
#include <vangui_notify.h>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <cmath>

namespace nfsmw {

// ─── WAV write helper (local) ─────────────────────────────────────────────────

static bool WriteWAV(const std::filesystem::path& dst,
                      const std::vector<int16_t>& pcm,
                      uint32_t sampleRate,
                      uint32_t channels)
{
    if (pcm.empty() || channels == 0 || sampleRate == 0) return false;

    std::ofstream f(dst, std::ios::binary);
    if (!f) return false;

    const uint32_t dataBytes    = static_cast<uint32_t>(pcm.size() * 2);
    const uint32_t byteRate     = sampleRate * channels * 2;
    const uint16_t blockAlign   = static_cast<uint16_t>(channels * 2);

    auto w16 = [&](uint16_t v){ f.write(reinterpret_cast<const char*>(&v), 2); };
    auto w32 = [&](uint32_t v){ f.write(reinterpret_cast<const char*>(&v), 4); };

    f.write("RIFF", 4); w32(36 + dataBytes);
    f.write("WAVE", 4);
    f.write("fmt ", 4); w32(16);
    w16(1); w16(static_cast<uint16_t>(channels));
    w32(sampleRate); w32(byteRate);
    w16(blockAlign); w16(16);
    f.write("data", 4); w32(dataBytes);
    f.write(reinterpret_cast<const char*>(pcm.data()),
            static_cast<std::streamsize>(dataBytes));
    return true;
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

SoundBankPanel::SoundBankPanel()
    : playState_(std::make_shared<SBPlayState>()) {}

SoundBankPanel::~SoundBankPanel() {
    ShutdownAudioDevice();
}

// ─── IsLoaded ────────────────────────────────────────────────────────────────

bool SoundBankPanel::IsLoaded() const {
    return bankKind_ != BankKind::None;
}

// ─── Entry accessors — dispatch ABK vs GIN ───────────────────────────────────

size_t SoundBankPanel::EntryCount() const {
    if (bankKind_ == BankKind::ABK) return std::get<ABKFile>(bank_).entries.size();
    if (bankKind_ == BankKind::GIN) return std::get<GINFile>(bank_).clips.size();
    return 0;
}

std::string SoundBankPanel::EntryName(size_t idx) const {
    if (bankKind_ == BankKind::ABK) {
        const auto& e = std::get<ABKFile>(bank_).entries;
        return idx < e.size() ? e[idx].name : "";
    }
    if (bankKind_ == BankKind::GIN) {
        return "Clip " + std::to_string(idx);
    }
    return "";
}

std::string SoundBankPanel::EntryCodec(size_t idx) const {
    if (bankKind_ == BankKind::ABK) {
        const auto& e = std::get<ABKFile>(bank_).entries;
        return idx < e.size() ? e[idx].CodecName() : "";
    }
    if (bankKind_ == BankKind::GIN) return "EA-XA";
    return "";
}

std::string SoundBankPanel::EntryExtra(size_t idx) const {
    if (bankKind_ == BankKind::ABK) {
        const auto& e = std::get<ABKFile>(bank_).entries;
        if (idx >= e.size()) return "";
        const auto& entry = e[idx];
        if (entry.sampleRate == 0) return "0:00";
        // Estimate duration from raw data size (EAXA: ~56 samples per 32-byte block)
        // For display purposes the actual PCM sample count isn't available at browse time.
        return std::string(entry.HasLoop() ? "loop" : "");
    }
    if (bankKind_ == BankKind::GIN) {
        const auto& clips = std::get<GINFile>(bank_).clips;
        if (idx >= clips.size()) return "";
        const auto& c = clips[idx];
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.0f–%.0f rpm", c.rpmMin, c.rpmMax);
        return buf;
    }
    return "";
}

Result<std::vector<int16_t>> SoundBankPanel::DecodeEntry(size_t idx,
                                                           uint32_t& outRate,
                                                           uint32_t& outChannels) const
{
    if (bankKind_ == BankKind::ABK) {
        const ABKFile& abk = std::get<ABKFile>(bank_);
        if (idx >= abk.entries.size())
            return Result<std::vector<int16_t>>::Err("Entry index OOB");
        const ABKEntry& entry = abk.entries[idx];

        auto rawRes = ABKParser::ReadPayload(abk, entry);
        if (!rawRes) return Result<std::vector<int16_t>>::Err(rawRes.error);

        return ABKParser::DecodePCM(rawRes.value, entry, outRate, outChannels);
    }

    if (bankKind_ == BankKind::GIN) {
        const GINFile& gin = std::get<GINFile>(bank_);
        if (idx >= gin.clips.size())
            return Result<std::vector<int16_t>>::Err("Clip index OOB");

        return GINParser::DecodePCM(gin, gin.clips[idx], outRate, outChannels);
    }

    return Result<std::vector<int16_t>>::Err("No bank loaded");
}

// ─── OpenABK ─────────────────────────────────────────────────────────────────

void SoundBankPanel::OpenABK(const std::filesystem::path& path, TaskQueue& tasks) {
    loading_     = true;
    selectedIdx_ = -1;
    lastError_.clear();
    loadedPath_  = path;
    tasks_       = &tasks;
    StopPlayback();

    struct ABKLoadResult { ABKFile bank; std::string error; bool ok = false; };
    auto out = std::make_shared<ABKLoadResult>();
    tasks.Submit("Loading ABK",
        [out, path](nfsmw::ProgressState&) {
            auto res = ABKParser::Load(path);
            if (res) { out->bank = std::move(res.value); out->ok = true; }
            else      { out->error = res.error; }
        },
        [this, out]() {
            loading_ = false;
            if (out->ok) {
                bank_     = std::move(out->bank);
                bankKind_ = BankKind::ABK;
                LOG_INFO("SoundBankPanel: ABK loaded — {} entries",
                         std::get<ABKFile>(bank_).entries.size());
            } else {
                lastError_ = out->error;
                bankKind_  = BankKind::None;
                bank_      = std::monostate{};
                LOG_WARN("SoundBankPanel: ABK load error: {}", out->error);
                VanGui::NotifyError("%s", std::string("ABK load failed: " + out->error).c_str());
            }
        });
}

// ─── OpenGIN ─────────────────────────────────────────────────────────────────

void SoundBankPanel::OpenGIN(const std::filesystem::path& path, TaskQueue& tasks) {
    loading_     = true;
    selectedIdx_ = -1;
    lastError_.clear();
    loadedPath_  = path;
    tasks_       = &tasks;
    StopPlayback();

    struct GINLoadResult { GINFile bank; std::string error; bool ok = false; };
    auto out = std::make_shared<GINLoadResult>();
    tasks.Submit("Loading GIN",
        [out, path](nfsmw::ProgressState&) {
            auto res = GINParser::Load(path);
            if (res) { out->bank = std::move(res.value); out->ok = true; }
            else      { out->error = res.error; }
        },
        [this, out]() {
            loading_ = false;
            if (out->ok) {
                bank_     = std::move(out->bank);
                bankKind_ = BankKind::GIN;
                LOG_INFO("SoundBankPanel: GIN loaded — {} clips",
                         std::get<GINFile>(bank_).clips.size());
            } else {
                lastError_ = out->error;
                bankKind_  = BankKind::None;
                bank_      = std::monostate{};
                LOG_WARN("SoundBankPanel: GIN load error: {}", out->error);
                VanGui::NotifyError("%s", std::string("GIN load failed: " + out->error).c_str());
            }
        });
}

// ─── miniaudio integration ────────────────────────────────────────────────────

void SoundBankPanel::MADataCallback(ma_device* pDevice, void* pOutput,
                                     const void* /*pInput*/, uint32_t frameCount)
{
    auto* state = static_cast<SBPlayState*>(pDevice->pUserData);
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

void SoundBankPanel::InitAudioDevice() {
    if (maDev_.IsInited()) return;

    ma_device_config cfg   = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_s16;
    cfg.playback.channels = playState_->channels;
    cfg.sampleRate        = playState_->sampleRate;
    cfg.dataCallback      = MADataCallback;
    cfg.pUserData         = playState_.get();

    maDev_.Init(cfg, "SoundBankPanel");
}

void SoundBankPanel::ShutdownAudioDevice() {
    StopPlayback();
    maDev_.Shutdown();
}

// ─── Playback ────────────────────────────────────────────────────────────────

void SoundBankPanel::StartPlayback(size_t idx) {
    StopPlayback();

    uint32_t rate = 0, ch = 0;
    auto pcmRes = DecodeEntry(idx, rate, ch);
    if (!pcmRes) {
        VanGui::NotifyError("%s", std::string("Decode failed: " + pcmRes.error).c_str());
        return;
    }

    {
        std::lock_guard<std::mutex> lk(playState_->mtx);
        playState_->pcmData    = std::move(pcmRes.value);
        playState_->sampleRate = rate > 0 ? rate : 22050;
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

void SoundBankPanel::StopPlayback() {
    if (playState_) playState_->playing = false;
    maDev_.Stop();
}

// ─── Waveform thumbnail ───────────────────────────────────────────────────────

void SoundBankPanel::BuildWaveThumb(const std::vector<int16_t>& pcm) {
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

// ─── Export WAV ──────────────────────────────────────────────────────────────

void SoundBankPanel::DoExportWAV(size_t idx, const std::filesystem::path& dstPath) {
    uint32_t rate = 0, ch = 0;
    auto pcmRes = DecodeEntry(idx, rate, ch);
    if (!pcmRes) {
        VanGui::NotifyError("%s", std::string("Export decode failed: " + pcmRes.error).c_str());
        return;
    }
    if (!WriteWAV(dstPath, pcmRes.value, rate, ch)) {
        VanGui::NotifyError("%s", std::string("WAV write failed: " + dstPath.string()).c_str());
        return;
    }
    VanGui::NotifyInfo("%s", std::string("Exported: " + dstPath.filename().string()).c_str());
}

// ─── Import WAV (ABK) ─────────────────────────────────────────────────────────

// Minimal RIFF/WAVE reader: returns mono s16 PCM (stereo is down-mixed).
// Supports 16-bit PCM (the common case). Fills outRate.
static Result<std::vector<int16_t>> ReadWAVMono(const std::filesystem::path& path,
                                                 uint32_t& outRate) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return Result<std::vector<int16_t>>::Err("Cannot open WAV: " + path.string());
    const size_t sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> d(sz);
    f.read(reinterpret_cast<char*>(d.data()), static_cast<std::streamsize>(sz));
    if (sz < 44 || std::memcmp(d.data(), "RIFF", 4) != 0 || std::memcmp(d.data() + 8, "WAVE", 4) != 0)
        return Result<std::vector<int16_t>>::Err("Not a valid WAV file");

    auto u16 = [&](size_t o){ return (uint16_t)(d[o] | d[o+1] << 8); };
    auto u32 = [&](size_t o){ return (uint32_t)(d[o] | d[o+1] << 8 | d[o+2] << 16 | (uint32_t)d[o+3] << 24); };

    uint16_t fmt = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    size_t dataOff = 0, dataLen = 0;
    size_t p = 12;
    while (p + 8 <= sz) {
        const uint32_t id   = u32(p);
        const uint32_t clen = u32(p + 4);
        const size_t   body = p + 8;
        if (id == 0x20746d66 /* "fmt " */ && body + 16 <= sz) {
            fmt = u16(body); channels = u16(body + 2); rate = u32(body + 4); bits = u16(body + 14);
        } else if (id == 0x61746164 /* "data" */) {
            dataOff = body; dataLen = std::min<size_t>(clen, sz - body);
        }
        p = body + clen + (clen & 1);  // chunks are word-aligned
    }
    if (channels == 0 || rate == 0 || dataOff == 0)
        return Result<std::vector<int16_t>>::Err("WAV missing fmt/data chunk");
    if (fmt != 1 || bits != 16)
        return Result<std::vector<int16_t>>::Err("Only 16-bit PCM WAV is supported");

    outRate = rate;
    const size_t frames = dataLen / (2 * channels);
    std::vector<int16_t> mono(frames);
    const uint8_t* s = d.data() + dataOff;
    for (size_t i = 0; i < frames; ++i) {
        int32_t acc = 0;
        for (uint16_t c = 0; c < channels; ++c)
            acc += (int16_t)(s[(i * channels + c) * 2] | s[(i * channels + c) * 2 + 1] << 8);
        mono[i] = (int16_t)(acc / channels);
    }
    return Result<std::vector<int16_t>>::Ok(std::move(mono));
}

void SoundBankPanel::DoImportWAV(size_t idx, const std::filesystem::path& srcWav) {
    if (bankKind_ != BankKind::ABK && bankKind_ != BankKind::GIN) {
        VanGui::NotifyWarning("%s", std::string("No bank loaded.").c_str());
        return;
    }

    uint32_t wavRate = 0;
    auto pcm = ReadWAVMono(srcWav, wavRate);
    if (!pcm) {
        VanGui::NotifyError("%s", std::string("Import failed: " + pcm.error).c_str());
        return;
    }

    StopPlayback();

    if (!tasks_) {
        VanGui::NotifyError("%s", std::string("No task queue available.").c_str());
        return;
    }

    // ── ABK path: EA-XA v2 in-place replace (async, with progress) ───────────
    if (bankKind_ == BankKind::ABK) {
        auto& liveBank = std::get<ABKFile>(bank_);
        if (idx >= liveBank.entries.size()) {
            VanGui::NotifyError("%s", std::string("Sound index out of range.").c_str());
            return;
        }

        // Create the .bak before the first write — and abort if it fails, rather
        // than silently overwriting the original with no way back (the previous
        // code discarded the error code from copy_file).
        std::error_code ec;
        const std::filesystem::path bak = liveBank.path.string() + ".bak";
        if (!std::filesystem::exists(bak)) {
            std::filesystem::copy_file(liveBank.path, bak, ec);
            if (ec) {
                VanGui::NotifyError("%s", std::string("Could not create backup (" + ec.message() +
                                           ") — replace aborted.").c_str());
                return;
            }
        }

        const std::string entryName  = EntryName(idx);
        const uint32_t    numSamples = liveBank.entries[idx].numSamples;
        const std::string bakName    = bak.filename().string();

        // Work on a private copy of the bank so a concurrent file switch can't
        // dangle the reference the worker holds. ABKFile is just path + entry
        // metadata (the audio bytes stay on disk), so the copy is cheap. The UI
        // thread (onDone) applies the result and shows the toast — workers must
        // never touch panel members like toasts_/waveEntryIdx_.
        auto workBank = std::make_shared<ABKFile>(liveBank);
        auto result   = std::make_shared<Result<AudioCodecs::ReplaceReport>>(
                            Result<AudioCodecs::ReplaceReport>::Err("not started"));

        tasks_->Submit("Replacing '" + entryName + "'",
            [workBank, result, pcmVec = std::move(pcm.value), wavRate, idx]
            (ProgressState& ps) mutable {
                AudioCodecs::ProgressFn prog = [&ps](float f) { ps.fraction = f; };
                *result = ABKParser::ReplaceFromPCM(*workBank, workBank->entries[idx],
                                                    pcmVec, wavRate,
                                                    workBank->path.parent_path(), prog);
            },
            [this, workBank, result, entryName, numSamples, bakName, idx]() {
                if (!*result) {
                    VanGui::NotifyError("%s", std::string("Replace failed: " + result->error).c_str());
                    return;
                }
                // Refresh the live bank's entry if that file is still open.
                if (bankKind_ == BankKind::ABK) {
                    auto& live = std::get<ABKFile>(bank_);
                    if (live.path == workBank->path && idx < live.entries.size())
                        live.entries[idx] = workBank->entries[idx];
                }
                waveEntryIdx_ = -2;
                std::string msg = "Replaced '" + entryName + "' (fit to " +
                                  std::to_string(numSamples) + " samples). Backup: " + bakName;
                const std::string summary = result->value.Summary();
                if (!summary.empty()) msg += " [" + summary + "]";
                VanGui::NotifyInfo("%s", std::string(msg).c_str());
            });
        return;
    }

    // ── GIN path: EA-XAS v0 stream rewrite (async, with progress) ────────────
    if (bankKind_ == BankKind::GIN) {
        GINFile& liveGin = std::get<GINFile>(bank_);
        if (idx >= liveGin.clips.size()) {
            VanGui::NotifyError("%s", std::string("Clip index out of range.").c_str());
            return;
        }

        backup_.EnsureFileBak(liveGin.path);   // UI thread, before any write

        const std::string clipName = "Clip " + std::to_string(idx);
        const size_t      nSamples = pcm.value.size();

        // As with ABK: work on a private copy so a concurrent file switch can't
        // dangle the worker's reference, and apply results on the UI thread.
        auto workGin = std::make_shared<GINFile>(liveGin);
        auto result  = std::make_shared<Result<AudioCodecs::ReplaceReport>>(
                           Result<AudioCodecs::ReplaceReport>::Err("not started"));

        tasks_->Submit("Replacing " + clipName,
            [workGin, result, pcmVec = std::move(pcm.value), wavRate, idx]
            (ProgressState& ps) mutable {
                AudioCodecs::ProgressFn prog = [&ps](float f) { ps.fraction = f; };
                *result = GINParser::ReplaceClip(*workGin, idx, pcmVec, wavRate, prog);
            },
            [this, workGin, result, clipName, nSamples]() {
                if (!*result) {
                    VanGui::NotifyError("%s", std::string("GIN replace failed: " + result->error).c_str());
                    return;
                }
                // Refresh in-memory audio so preview/playback reflect the new clip.
                if (bankKind_ == BankKind::GIN) {
                    auto& live = std::get<GINFile>(bank_);
                    if (live.path == workGin->path) live = *workGin;
                }
                waveEntryIdx_ = -2;
                std::string msg = "Replaced " + clipName + " (" +
                                  std::to_string(nSamples) + " samples). Backup saved.";
                const std::string summary = result->value.Summary();
                if (!summary.empty()) msg += " [" + summary + "]";
                VanGui::NotifySuccess("%s", std::string(msg).c_str());
            });
    }
}

// ─── DrawBrowser ─────────────────────────────────────────────────────────────

void SoundBankPanel::DrawBrowser() {
    if (loading_) {
        VanGui::TextDisabled("Loading…");
        return;
    }
    if (!IsLoaded()) {
        if (!lastError_.empty()) {
            VanGui::TextColored(VanVec4(1, 0.4f, 0.4f, 1), "Error: %s", lastError_.c_str());
            return;
        }
        VanGui::TextDisabled("No sound bank loaded.");
        VanGui::TextDisabled("Audio \xe2\x86\x92 Open ABK\xe2\x80\xa6 or Open GIN\xe2\x80\xa6");
        return;
    }
    // Bank is loaded — any prior error is stale; don't show it.

    // ── Bank name / type badge ─────────────────────────────────────────────────
    const char* kindLabel = (bankKind_ == BankKind::ABK) ? "[ABK]" : "[GIN]";
    std::string bankName;
    if (bankKind_ == BankKind::ABK) bankName = std::get<ABKFile>(bank_).name;
    else if (bankKind_ == BankKind::GIN) bankName = std::get<GINFile>(bank_).name;

    VanGui::TextColored(VanGui::GetStyleColorVec4(VanGuiCol_SliderGrab), "%s", kindLabel);
    VanGui::SameLine();
    VanGui::TextUnformatted(bankName.c_str());
    VanGui::Separator();

    // ── Filter bar ────────────────────────────────────────────────────────────
    VanGui::SetNextItemWidth(-1.0f);
    char filterBuf[128];
    snprintf(filterBuf, sizeof(filterBuf), "%s", filter_.c_str());
    if (VanGui::InputTextWithHint("##sbfilter", "Filter…", filterBuf, sizeof(filterBuf)))
        filter_ = filterBuf;

    VanGui::Separator();

    // ── Entry list ────────────────────────────────────────────────────────────
    const float rowH = VanGui::GetTextLineHeightWithSpacing();
    (void)VanGui::BeginChild("##sblist", VanVec2(0, 0), false,
                       VanGuiWindowFlags_HorizontalScrollbar);

    const VanVec4 colPlaying(0.35f, 0.90f, 0.45f, 1.0f); // green = actively playing
    const VanVec4 accent    = VanGui::GetStyleColorVec4(VanGuiCol_SliderGrab);
    const VanVec4 colCodec  (accent.x * 0.80f, accent.y * 0.80f, accent.z * 0.80f, 0.85f);
    const VanVec4 colExtra  (0.65f, 0.65f, 0.65f, 1.0f);

    const size_t count = EntryCount();
    for (size_t i = 0; i < count; ++i) {
        const std::string name  = EntryName(i);
        if (!CaseInsensitiveContains(name, filter_)) continue;

        const bool sel     = (selectedIdx_ == static_cast<int>(i));
        const bool playing = sel && playState_ && playState_->playing;

        if (playing) VanGui::PushStyleColor(VanGuiCol_Text, colPlaying);
        bool clicked = VanGui::Selectable(("##sbrow" + std::to_string(i)).c_str(),
                                          sel, 0, VanVec2(0, rowH));
        if (playing) VanGui::PopStyleColor();

        if (clicked) {
            selectedIdx_ = static_cast<int>(i);
            waveEntryIdx_ = -2; // invalidate waveform
        }

        // Double-click to play immediately
        if (VanGui::IsItemHovered() && VanGui::IsMouseDoubleClicked(0))
            StartPlayback(i);

        VanGui::SameLine(4.0f);
        if (playing) { VanGui::TextColored(colPlaying, "▶ "); VanGui::SameLine(); }
        VanGui::Text("%s", name.c_str());

        // Right-aligned codec + extra columns
        const std::string codec = EntryCodec(i);
        const std::string extra = EntryExtra(i);
        const float availW = VanGui::GetContentRegionAvail().x;
        VanGui::SameLine(availW - 160.0f);
        VanGui::TextColored(colCodec, "%s", codec.c_str());
        VanGui::SameLine(availW - 80.0f);
        VanGui::TextColored(colExtra, "%s", extra.c_str());

        // Right-click context menu
        if (VanGui::BeginPopupContextItem(("##sbctx" + std::to_string(i)).c_str())) {
            if (VanGui::MenuItem("▶ Play"))          StartPlayback(i);
            if (VanGui::MenuItem("Export as WAV…") && fileDialog_) {
                const std::string defaultName = EntryName(i) + ".wav";
                fileDialog_->Show("Export as WAV", FileDialog::Mode::Save,
                                   {".wav"},
                                   [this, i](const std::filesystem::path& p) {
                                       DoExportWAV(i, p);
                                   });
            }
            VanGui::EndPopup();
        }
    }

    VanGui::EndChild();
}

// ─── DrawPreview ─────────────────────────────────────────────────────────────

void SoundBankPanel::DrawPreview(TaskQueue& /*tasks*/) {
    if (!IsLoaded() || selectedIdx_ < 0 ||
        selectedIdx_ >= static_cast<int>(EntryCount()))
    {
        VanGui::TextDisabled("Select a sound to preview.");
        return;
    }

    const size_t idx = static_cast<size_t>(selectedIdx_);

    // ── Metadata card ─────────────────────────────────────────────────────────
    (void)VanGui::BeginChild("##sbmeta", VanVec2(0, 78), true);

    VanGui::TextUnformatted(EntryName(idx).c_str());
    VanGui::Separator();
    VanGui::Columns(3, "sbmetacols", false);
    VanGui::Text("Codec: %s", EntryCodec(idx).c_str());
    VanGui::NextColumn();
    if (bankKind_ == BankKind::ABK) {
        const auto& e = std::get<ABKFile>(bank_).entries[idx];
        VanGui::Text("Rate: %u Hz",  e.sampleRate);
        VanGui::NextColumn();
        VanGui::Text("Ch: %u",       e.channels);
    } else if (bankKind_ == BankKind::GIN) {
        const auto& c = std::get<GINFile>(bank_).clips[idx];
        VanGui::Text("Rate: %u Hz",  c.sampleRate);
        VanGui::NextColumn();
        VanGui::Text("Ch: %u",       c.channels);
    } else {
        VanGui::NextColumn();
    }
    VanGui::NextColumn();
    VanGui::Text("%s", EntryExtra(idx).c_str());
    VanGui::Columns(1);

    VanGui::EndChild();

    // ── Waveform thumbnail (build lazily) ─────────────────────────────────────
    if (waveEntryIdx_ != selectedIdx_) {
        waveThumb_.assign(256, 0.0f);
        waveEntryIdx_ = selectedIdx_;
        // Decode in-place for preview; small entries are fast enough synchronously.
        uint32_t rate = 0, ch = 0;
        auto pcmRes = DecodeEntry(idx, rate, ch);
        if (pcmRes) BuildWaveThumb(pcmRes.value);
    }

    if (!waveThumb_.empty()) {
        const VanVec2 thumbSize(VanGui::GetContentRegionAvail().x, 60.0f);
        VanDrawList* dl = VanGui::GetWindowDrawList();
        const VanVec2 p  = VanGui::GetCursorScreenPos();
        // Background — use theme's frame bg (matches dark/light mode).
        dl->AddRectFilled(p, VanVec2(p.x + thumbSize.x, p.y + thumbSize.y),
                           VanGui::ColorConvertFloat4ToU32(VanGui::GetStyleColorVec4(VanGuiCol_FrameBg)));

        const float midY   = p.y + thumbSize.y * 0.5f;
        const float scaleX = thumbSize.x / static_cast<float>(waveThumb_.size());
        const VanVec4 wa    = VanGui::GetStyleColorVec4(VanGuiCol_SliderGrab);
        const VanU32 waveCol = VanGui::ColorConvertFloat4ToU32(VanVec4(wa.x, wa.y, wa.z, 0.78f));
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
        if (VanGui::Button("⏹ Stop", VanVec2(80, 0))) StopPlayback();
    } else {
        if (VanGui::Button("▶ Play", VanVec2(80, 0))) StartPlayback(idx);
    }

    VanGui::SameLine();
    bool loop = playState_ ? playState_->loop.load() : false;
    if (VanGui::Checkbox("Loop", &loop) && playState_)
        playState_->loop = loop;

    VanGui::SameLine();
    if (VanGui::Button("Export WAV…") && fileDialog_) {
        const std::string defaultName = EntryName(idx) + ".wav";
        fileDialog_->Show("Export as WAV", FileDialog::Mode::Save,
                           {".wav"},
                           [this, idx](const std::filesystem::path& p) {
                               DoExportWAV(idx, p);
                           });
    }

    // Replace from WAV — ABK: EA-XA v2 in-place; GIN: EA-XAS v0 stream rewrite.
    if (bankKind_ == BankKind::ABK || bankKind_ == BankKind::GIN) {
        VanGui::SameLine();
        if (VanGui::Button("Replace from WAV\xe2\x80\xa6") && fileDialog_) {
            fileDialog_->Show("Replace from WAV", FileDialog::Mode::Open,
                               {".wav"},
                               [this, idx](const std::filesystem::path& p) {
                                   DoImportWAV(idx, p);
                               });
        }
        if (VanGui::IsItemHovered()) {
            if (bankKind_ == BankKind::ABK)
                VanGui::SetTooltip("Re-encode a mono 16-bit WAV into this sound (EA-XA v2).\n"
                                  "The clip is fit to the original length; a .bak is made first.");
            else
                VanGui::SetTooltip("Re-encode a mono 16-bit WAV into this GIN clip (EA-XAS v0).\n"
                                  "The stream is rewritten in full; sample rate should match.\n"
                                  "A .bak is created before writing.");
        }
    }

    // ── Whole-file actions (export / replace / revert) ────────────────────────
    // Offered as a power-user escape hatch alongside the per-clip WAV replace above.
    // A .bak is always made first so it's reversible.
    if (!loadedPath_.empty()) {
        const char* ext = (bankKind_ == BankKind::GIN) ? ".gin" : ".abk";
        VanGui::Separator();
        VanGui::TextDisabled("%s", loadedPath_.filename().string().c_str());

        if (VanGui::Button("Export file\xe2\x80\xa6") && fileDialog_) {
            fileDialog_->Show("Export sound bank", FileDialog::Mode::Save, {ext},
                              [this](const std::filesystem::path& dst) {
                                  std::error_code ec;
                                  std::filesystem::copy_file(loadedPath_, dst,
                                      std::filesystem::copy_options::overwrite_existing, ec);
                                  if (ec) VanGui::NotifyError("Export failed");
                                  else    VanGui::NotifySuccess("%s", std::string("Exported " + dst.filename().string()).c_str());
                              }, loadedPath_.filename().string());
        }
        VanGui::SameLine();
        if (VanGui::Button("Replace file\xe2\x80\xa6") && fileDialog_) {
            fileDialog_->Show("Replace sound bank", FileDialog::Mode::Open, {ext},
                              [this](const std::filesystem::path& src) {
                                  StopPlayback();
                                  backup_.EnsureFileBak(loadedPath_);
                                  std::error_code ec;
                                  std::filesystem::copy_file(src, loadedPath_,
                                      std::filesystem::copy_options::overwrite_existing, ec);
                                  if (ec) { VanGui::NotifyError("%s", std::string("Replace failed: " +
                                            ec.message()).c_str()); return; }
                                  VanGui::NotifySuccess("%s", std::string("Replaced " +
                                      loadedPath_.filename().string() + " (backup saved)").c_str());
                                  if (tasks_) {
                                      if (bankKind_ == BankKind::GIN) OpenGIN(loadedPath_, *tasks_);
                                      else                            OpenABK(loadedPath_, *tasks_);
                                  }
                              });
        }
        VanGui::SameLine();
        const bool canRevert = backup_.HasBak(loadedPath_);
        if (!canRevert) VanGui::BeginDisabled();
        if (VanGui::Button("Revert##bank")) {
            StopPlayback();
            std::error_code ec;
            std::filesystem::copy_file(BackupManager::BakPath(loadedPath_), loadedPath_,
                std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) VanGui::NotifyError("Revert failed");
            else    VanGui::NotifySuccess("Reverted to backup");
            if (!ec && tasks_) {
                if (bankKind_ == BankKind::GIN) OpenGIN(loadedPath_, *tasks_);
                else                            OpenABK(loadedPath_, *tasks_);
            }
        }
        if (!canRevert) VanGui::EndDisabled();
    }

    // ── Seek bar ──────────────────────────────────────────────────────────────
    if (playState_ && !playState_->pcmData.empty()) {
        const uint64_t totalFrames = playState_->pcmData.size() /
                                     std::max<uint32_t>(1, playState_->channels);
        float frac = totalFrames > 0
            ? static_cast<float>(playState_->cursor.load()) /
              static_cast<float>(totalFrames)
            : 0.0f;
        VanGui::SetNextItemWidth(-1.0f);
        if (VanGui::SliderFloat("##sbseek", &frac, 0.0f, 1.0f, "")) {
            playState_->cursor = static_cast<uint64_t>(frac * static_cast<float>(totalFrames));
        }
    }
}

} // namespace nfsmw
                                        