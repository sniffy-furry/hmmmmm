#pragma once
//
// MiniaudioDevice — small RAII wrapper around a single miniaudio playback
// device.
//
// Several panels (MusicPanel, VideoPanel, AudioPanel, SoundBankPanel,
// UISounds) each used to carry their own copy of the same
// new ma_device{} / ma_device_init / ma_device_uninit / delete lifecycle.
// That duplication meant any future fix (e.g. handling device-lost
// callbacks, or switching allocation strategy) had to be made in five
// places. This wrapper centralizes it; panels just build a
// ma_device_config and call Init()/Shutdown()/IsInited()/Get().
//
#include "vendor/miniaudio/miniaudio.h"
#include <string>

#include "core/Logger.h"

namespace nfsmw {

class MiniaudioDevice {
public:
    MiniaudioDevice() = default;
    ~MiniaudioDevice() { Shutdown(); }

    MiniaudioDevice(const MiniaudioDevice&)            = delete;
    MiniaudioDevice& operator=(const MiniaudioDevice&) = delete;
    MiniaudioDevice(MiniaudioDevice&&)                 = delete;
    MiniaudioDevice& operator=(MiniaudioDevice&&)      = delete;

    /// Allocates and initializes the device from `cfg`. No-op if already
    /// inited. `logTag` is used only for the warning log line on failure
    /// (e.g. "MusicPanel"). Returns true on success.
    bool Init(const ma_device_config& cfg, const char* logTag) {
        if (inited_) return true;

        device_ = new (std::nothrow) ma_device{};
        if (!device_) {
            LOG_WARN("{}: out of memory allocating miniaudio device", logTag);
            return false;
        }
        if (ma_device_init(nullptr, &cfg, device_) != MA_SUCCESS) {
            LOG_WARN("{}: failed to init miniaudio device", logTag);
            delete device_;
            device_ = nullptr;
            return false;
        }
        inited_ = true;
        return true;
    }

    /// Stops (if running) and tears the device down. Safe to call when not
    /// inited, and safe to call more than once.
    void Shutdown() {
        if (inited_ && device_) {
            ma_device_uninit(device_);
        }
        delete device_;
        device_ = nullptr;
        inited_ = false;
    }

    bool       IsInited() const { return inited_ && device_ != nullptr; }
    ma_device* Get()             { return device_; }

    bool Start() { return IsInited() && ma_device_start(device_) == MA_SUCCESS; }
    void Stop()  { if (IsInited()) ma_device_stop(device_); }

private:
    ma_device* device_ = nullptr;
    bool       inited_ = false;
};

} // namespace nfsmw
