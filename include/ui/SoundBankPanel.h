#pragma once
// ─── ui/SoundBankPanel.h ──────────────────────────────────────────────────────
// VanGui panel for browsing and playing ABK sound-effect banks and GIN
// engine RPM-curve audio files.
//
// Layout mirrors AudioPanel:
//   Left  → "Sound Bank Browser" — entry list with filter, codec badges,
//            RPM range column for GIN clips
//   Right → "Sound Bank Preview" — waveform thumbnail, transport, export
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/ABKFile.h"
#include "formats/GINFile.h"
#include "ui/FileDialog.h"
#include "patch/BackupManager.h"
#include "async/TaskQueue.h"
#include "core/MiniaudioDevice.h"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <variant>

struct ma_device;

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
/// Shared playback state (same layout as AudioPanel::AudioPlayState).
// ─────────────────────────────────────────────────────────────────────────────
struct SBPlayState {
    std::vector<int16_t>  pcmData;
    uint32_t              sampleRate = 44100;
    uint32_t              channels   = 1;
    std::atomic<uint64_t> cursor     = 0;
    std::atomic<bool>     playing    = false;
    std::atomic<bool>     loop       = false;
    std::mutex            mtx;
};

// ─────────────────────────────────────────────────────────────────────────────
class SoundBankPanel {
public:
    SoundBankPanel();
    ~SoundBankPanel();

    SoundBankPanel(const SoundBankPanel&) = delete;
    SoundBankPanel& operator=(const SoundBankPanel&) = delete;

    void OpenABK(const std::filesystem::path& path, TaskQueue& tasks);
    void OpenGIN(const std::filesystem::path& path, TaskQueue& tasks);

    /// Draw the left-pane entry browser.
    void DrawBrowser();

    /// Draw the right-pane preview / transport.
    void DrawPreview(TaskQueue& tasks);

    void SetFileDialog(FileDialog* fd) { fileDialog_ = fd; }

    bool IsLoaded()  const;
    bool IsLoading() const { return loading_; }

private:
    // Loaded data — one of ABK or GIN is active at a time.
    std::variant<std::monostate, ABKFile, GINFile> bank_;
    enum class BankKind { None, ABK, GIN } bankKind_ = BankKind::None;

    int               selectedIdx_  = -1;
    std::string       filter_;
    std::string       lastError_;
    std::atomic<bool> loading_      = false;
    std::filesystem::path loadedPath_;     ///< source .abk/.gin on disk
    BackupManager     backup_;
    TaskQueue*        tasks_ = nullptr;     ///< for reload-after-replace

    // Playback
    std::shared_ptr<SBPlayState> playState_;
    MiniaudioDevice               maDev_;

    // UI helpers
    FileDialog*   fileDialog_ = nullptr;

    // Waveform thumbnail
    std::vector<float> waveThumb_;
    int                waveEntryIdx_ = -2;

    // Helpers
    void StartPlayback(size_t idx);
    void StopPlayback();
    void BuildWaveThumb(const std::vector<int16_t>& pcm);
    void InitAudioDevice();
    void ShutdownAudioDevice();
    void DoExportWAV(size_t idx, const std::filesystem::path& dstPath);
    void DoImportWAV(size_t idx, const std::filesystem::path& srcWav);  // ABK only

    // Entry accessors (dispatch ABK vs GIN)
    size_t      EntryCount() const;
    std::string EntryName(size_t idx)  const;
    std::string EntryCodec(size_t idx) const;
    std::string EntryExtra(size_t idx) const;  // RPM range for GIN, duration for ABK
    Result<std::vector<int16_t>> DecodeEntry(size_t idx,
                                              uint32_t& outRate,
                                              uint32_t& outChannels) const;

    static void MADataCallback(ma_device* pDevice, void* pOutput,
                               const void* pInput, uint32_t frameCount);
};

} // namespace nfsmw
