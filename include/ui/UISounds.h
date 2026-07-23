#pragma once
// ─── ui/UISounds.h ───────────────────────────────────────────────────────────
// Lightweight procedural UI feedback sounds using a dedicated miniaudio device.
// No audio files required — tones are generated as short sine-wave PCM bursts.
//
// Usage:
//   UISounds sounds;
//   sounds.Init();
//   sounds.Play(UISounds::Nav);        // subtle click on nav switch
//   sounds.Play(UISounds::FileLoaded); // ascending chime on file open
//   sounds.Play(UISounds::Success);    // short positive chime on patch complete
//   sounds.Shutdown();                 // call before GLFW/miniaudio teardown
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <atomic>
#include <array>

#include "core/MiniaudioDevice.h"

// Forward-declare miniaudio device to avoid pulling in the full header.
struct ma_device;

namespace nfsmw {

class UISounds {
public:
    enum Sound {
        Nav,         // subtle click — nav button pressed
        FileLoaded,  // gentle ascending two-note chime — file opened
        Success,     // three-note ascending chime — patch/export done
        Error,       // short low descending tone — something went wrong
        kCount
    };

    bool Init();
    void Shutdown();
    void Play(Sound s);

    bool IsInitialized() const { return initialized_; }

private:
    static void AudioCallback(ma_device* device,
                              void* output, const void* input,
                              uint32_t frameCount);

    // Each "blip" is a short tone burst with natural exponential-decay envelope.
    struct Blip {
        float    freq      = 0.0f;
        float    amp       = 0.0f;
        int      remaining = 0;    // samples left to render
        int      total     = 0;    // total samples (for envelope)
        float    phase     = 0.0f;
        int      delay     = 0;    // samples to wait before starting
        bool     harmonics = false; // add 2nd+3rd harmonic for bell/chime timbre
    };

    static constexpr int kSampleRate  = 44100;
    static constexpr int kMaxBlips    = 8;

    MiniaudioDevice        maDev_;
    bool                  initialized_ = false;

    // Lock-free blip queue: producer (Play) writes to writeSlot_, consumer (callback) reads.
    std::array<Blip, kMaxBlips> blips_{};
    std::atomic<int>            blipCount_{ 0 };

    void AddBlip(float freqHz, float amp, float durationSec,
                 bool harmonics = false, float delayMs = 0.0f);
};

} // namespace nfsmw
