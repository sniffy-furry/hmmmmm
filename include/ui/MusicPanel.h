#pragma once
// ─── ui/MusicPanel.h ──────────────────────────────────────────────────────────
// VanGui panel for interactive-music browsing: MPF event graph + MUS sections.
//
// Layout:
//   Left pane  → two sub-tabs:
//                  "Events"   — MPF event list (name + maps-to section index)
//                  "Sections" — raw MUS section list (index, SR, duration, codec)
//   Centre     → waveform thumbnail of selected section
//   Bottom     → play / stop / loop transport
//
// Selecting an event in the Events tab auto-selects and plays its MUS section.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/MPFFile.h"
#include "formats/MUSFile.h"
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

struct ma_device;

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
struct MPPlayState {
    std::vector<int16_t>  pcmData;
    uint32_t              sampleRate = 44100;
    uint32_t              channels   = 1;
    std::atomic<uint64_t> cursor     = 0;
    std::atomic<bool>     playing    = false;
    std::atomic<bool>     loop       = false;
    std::mutex            mtx;
};

// ─────────────────────────────────────────────────────────────────────────────
class MusicPanel {
public:
    MusicPanel();
    ~MusicPanel();

    MusicPanel(const MusicPanel&) = delete;
    MusicPanel& operator=(const MusicPanel&) = delete;

    /// Load a MPF+MUS pair.  The MUS is located automatically if musPath is
    /// empty (same stem, same directory as mpfPath).
    void OpenMPF(const std::filesystem::path& mpfPath,
                 const std::filesystem::path& musPath,
                 TaskQueue& tasks);

    /// Draw the left browser pane ("Music Browser").
    void DrawBrowser();

    /// Draw the right preview pane ("Music Preview").
    void DrawPreview(TaskQueue& tasks);

    void SetFileDialog(FileDialog* fd) { fileDialog_ = fd; }

    bool IsLoaded()  const { return !mus_.sections.empty(); }
    bool IsLoading() const { return loading_; }

private:
    MPFFile           mpf_;
    MUSFile           mus_;
    std::filesystem::path mpfPath_;   ///< source .mpf (may be empty)
    std::filesystem::path musPath_;   ///< source .mus/.big actually loaded
    BackupManager     backup_;
    TaskQueue*        tasks_ = nullptr;  ///< remembered for reload-after-replace
    int               selectedEvent_    = -1;
    int               selectedSection_  = -1;
    bool              showEventView_    = true;  ///< toggle: event tree vs raw section list
    std::string       eventFilter_;
    std::string       lastError_;
    std::atomic<bool> loading_          = false;

    // Playback
    std::shared_ptr<MPPlayState> playState_;
    MiniaudioDevice               maDev_;

    // Waveform thumbnail
    std::vector<float> waveThumb_;
    int                waveSectionIdx_ = -2;

    FileDialog*   fileDialog_ = nullptr;

    void PlaySection(int sectionIdx);
    void StopPlayback();
    void BuildWaveThumb(const std::vector<int16_t>& pcm);
    void InitAudioDevice();
    void ShutdownAudioDevice();

    /// Export selected section as a WAV file (decodes to PCM, writes RIFF/WAVE).
    void DoExportSectionWAV(int sectionIdx, const std::filesystem::path& dstPath);

    /// Import a WAV file and replace the selected section in the MUS file.
    /// Re-encodes to EA-XA v2 and rebuilds the SCHl/SCDl/SCEl block sequence.
    void DoImportSectionWAV(int sectionIdx, const std::filesystem::path& srcWav);

    static void MADataCallback(ma_device* pDevice, void* pOutput,
                               const void* pInput, uint32_t frameCount);
};

} // namespace nfsmw