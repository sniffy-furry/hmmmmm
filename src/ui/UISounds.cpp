// ─── ui/UISounds.cpp ─────────────────────────────────────────────────────────
// Procedural UI chimes using a dedicated miniaudio device at 44100 Hz mono.
//
// Each "blip" is rendered with:
//   • Sine fundamental + optional 2nd/3rd harmonics for glass-chime timbre
//   • Natural exponential-decay envelope (percussive attack, soft tail)
//   • Optional sample delay for sequenced multi-note sounds
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/UISounds.h"
#include "core/Logger.h"

#include "vendor/miniaudio/miniaudio.h"

#include <cmath>
#include <algorithm>
#include <new>

namespace nfsmw {

static constexpr float kTwoPi = 6.28318530717958f;

// ─────────────────────────────────────────────────────────────────────────────

void UISounds::AudioCallback(ma_device* device,
                             void*       output,
                             const void* /*input*/,
                             uint32_t    frameCount)
{
    auto* self = static_cast<UISounds*>(device->pUserData);
    float* out = static_cast<float*>(output);

    for (uint32_t i = 0; i < frameCount; ++i) {
        float sample = 0.0f;

        for (int b = 0; b < kMaxBlips; ++b) {
            Blip& blip = self->blips_[b];

            // Wait out the sample delay before generating audio.
            if (blip.delay > 0) {
                --blip.delay;
                continue;
            }
            if (blip.remaining <= 0) continue;

            // Progress through the total duration: t = 0 at start, 1 at end.
            const int  pos = blip.total - blip.remaining;
            const float t  = static_cast<float>(pos) / static_cast<float>(blip.total);

            // Natural envelope: very short linear attack (1.5%) then expo decay.
            // Mimics the strike-then-ring of a glass or bell.
            float env;
            if (t < 0.015f)
                env = t / 0.015f;
            else
                env = std::exp(-t * 6.0f);

            // Fundamental sine
            float s = std::sin(blip.phase * kTwoPi);

            // Harmonics for richer timbre (2nd: octave above, 3rd: 5th above octave)
            if (blip.harmonics) {
                s += 0.30f * std::sin(blip.phase * kTwoPi * 2.0f);
                s += 0.12f * std::sin(blip.phase * kTwoPi * 3.0f);
            }

            sample += blip.amp * env * s;

            blip.phase += blip.freq / static_cast<float>(kSampleRate);
            if (blip.phase >= 1.0f) blip.phase -= 1.0f;
            --blip.remaining;
        }

        // Gentle soft-clip to prevent inter-blip stacking from clipping hard.
        out[i] = std::tanh(sample * 0.65f) * 0.48f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────

bool UISounds::Init() {
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 1;
    cfg.sampleRate        = kSampleRate;
    cfg.dataCallback      = AudioCallback;
    cfg.pUserData         = this;

    if (!maDev_.Init(cfg, "UISounds")) {
        // MiniaudioDevice already logged the failure reason.
        return false;
    }
    if (!maDev_.Start()) {
        LOG_WARN("UISounds: failed to start audio device — UI sounds disabled");
        maDev_.Shutdown();
        return false;
    }

    initialized_ = true;
    LOG_INFO("UISounds: device opened at {}Hz mono", kSampleRate);
    return true;
}

void UISounds::Shutdown() {
    maDev_.Shutdown();
    initialized_ = false;
}

// ─────────────────────────────────────────────────────────────────────────────

void UISounds::AddBlip(float freqHz, float amp, float durationSec,
                       bool harmonics, float delayMs)
{
    const int samples = static_cast<int>(durationSec * kSampleRate);
    const int delaySamples = static_cast<int>(delayMs * 0.001f * kSampleRate);

    // Find a free slot (remaining == 0 and delay == 0 means inactive).
    for (int b = 0; b < kMaxBlips; ++b) {
        if (blips_[b].remaining <= 0 && blips_[b].delay <= 0) {
            blips_[b] = { freqHz, amp, samples, samples, 0.0f, delaySamples, harmonics };
            return;
        }
    }
    // All slots busy — evict the one furthest from completion.
    int oldest = 0;
    for (int b = 1; b < kMaxBlips; ++b)
        if (blips_[b].remaining > blips_[oldest].remaining)
            oldest = b;
    blips_[oldest] = { freqHz, amp, samples, samples, 0.0f, delaySamples, harmonics };
}

void UISounds::Play(Sound s) {
    if (!initialized_) return;

    switch (s) {
        case Nav:
            // Soft glass tap: a single high note with harmonics and fast decay.
            // Quiet and unobtrusive — shouldn't pull attention.
            AddBlip(780.0f, 0.10f, 0.090f, /*harmonics*/true);
            break;

        case FileLoaded: {
            // Two-note ascending glass chime: G5 → B5
            // The 95ms gap creates a clean "ding… ding" sequence.
            AddBlip(784.0f,  0.14f, 0.200f, true,   0.0f); // G5
            AddBlip(987.8f,  0.12f, 0.180f, true,  95.0f); // B5
            break;
        }

        case Success: {
            // Three-note ascending: C5 → E5 → G5 — a pleasant major triad.
            AddBlip(523.3f, 0.13f, 0.190f, true,   0.0f); // C5
            AddBlip(659.3f, 0.12f, 0.175f, true,  85.0f); // E5
            AddBlip(784.0f, 0.11f, 0.200f, true, 170.0f); // G5
            break;
        }

        case Error: {
            // Soft descending two-tone: E5 → C5.  Lower register, no harmonics
            // so it reads as "wrong" without being alarming.
            AddBlip(659.3f, 0.12f, 0.160f, false,  0.0f); // E5
            AddBlip(523.3f, 0.11f, 0.200f, false, 90.0f); // C5
            break;
        }

        default: break;
    }
}

} // namespace nfsmw
