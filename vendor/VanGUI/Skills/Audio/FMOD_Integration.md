# FMOD — Audio Integration

## Two Layers
- **FMOD Core** — low-level, programmatic DSP graph control
- **FMOD Studio** — high-level, event-driven, content separated from code

Use Studio for production games with designer-authored content.
Use Core when you need precise programmatic control or are integrating into an existing audio pipeline.

---

## When to Use What

| Need | Choice | Reason |
|---|---|---|
| Full game audio, designer-driven | FMOD Studio | Designers control events without code changes |
| Procedural / generative audio | FMOD Core DSP API | Full graph control in code |
| Simple sound playback in a tool | FMOD Core (Channel API) | Lightweight, no Studio overhead |
| Spatialized 3D audio | FMOD Core 3D attributes | Built-in Doppler, attenuation curves |
| Real-time audio analysis (spectrum) | FMOD Core DSP_FFT | Insert into any channel |
| Audio injection into existing app | Hook + FMOD Core | Hook the host's audio path |

---

## FMOD Core — Init & Lifecycle

```cpp
#include "fmod.hpp"
#pragma comment(lib, "fmod_vc.lib")

FMOD::System* g_fmod = nullptr;

bool InitFMOD() {
    FMOD_RESULT r = FMOD::System_Create(&g_fmod);
    if (r != FMOD_OK) return false;
    r = g_fmod->init(32, FMOD_INIT_NORMAL, nullptr);
    if (r != FMOD_OK) { g_fmod->release(); g_fmod = nullptr; return false; }
    return true;
}

void ShutdownFMOD() {
    if (g_fmod) { g_fmod->close(); g_fmod->release(); g_fmod = nullptr; }
}

// Call once per game loop iteration — processes callbacks, 3D updates
void UpdateFMOD() { if (g_fmod) g_fmod->update(); }
```

---

## Loading & Playing Sounds

```cpp
FMOD::Sound* LoadSound(const char* path, bool streaming = false) {
    FMOD::Sound* sound = nullptr;
    FMOD_MODE mode = streaming ? (FMOD_DEFAULT | FMOD_CREATESTREAM) : FMOD_DEFAULT;
    g_fmod->createSound(path, mode, nullptr, &sound);
    return sound;
}

FMOD::Channel* PlaySound(FMOD::Sound* sound, bool loop = false) {
    if (loop) sound->setMode(FMOD_LOOP_NORMAL);
    FMOD::Channel* ch = nullptr;
    g_fmod->playSound(sound, nullptr, false, &ch);
    return ch;  // retain for control; nullptr = fire-and-forget
}

void SetChannelVolume(FMOD::Channel* ch, float vol) {  // 0.0 – 1.0
    if (ch) ch->setVolume(vol);
}

void PauseChannel(FMOD::Channel* ch, bool pause) {
    if (ch) ch->setPaused(pause);
}
```

---

## 3D Spatialized Audio

```cpp
// Call each frame with camera/player position
void SetListener(const FMOD_VECTOR& pos, const FMOD_VECTOR& vel,
                 const FMOD_VECTOR& fwd, const FMOD_VECTOR& up) {
    g_fmod->set3DListenerAttributes(0, &pos, &vel, &fwd, &up);
}

FMOD::Sound* Load3DSound(const char* path) {
    FMOD::Sound* sound = nullptr;
    g_fmod->createSound(path, FMOD_3D | FMOD_CREATECOMPRESSEDSAMPLE, nullptr, &sound);
    sound->set3DMinMaxDistance(1.0f, 50.0f);  // audible distance range (units)
    return sound;
}

void SetChannelPosition(FMOD::Channel* ch,
                        const FMOD_VECTOR& pos, const FMOD_VECTOR& vel) {
    if (ch) ch->set3DAttributes(&pos, &vel);
}
```

---

## Real-Time FFT Spectrum Analysis

```cpp
FMOD::DSP* AttachSpectrumDSP(FMOD::Channel* ch, int windowSize = 1024) {
    FMOD::DSP* dsp = nullptr;
    g_fmod->createDSPByType(FMOD_DSP_TYPE_FFT, &dsp);
    dsp->setParameterInt(FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_HANNING);
    dsp->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, windowSize);

    FMOD::ChannelGroup* group = nullptr;
    ch->getChannelGroup(&group);
    if (!group) g_fmod->getMasterChannelGroup(&group);
    group->addDSP(0, dsp);
    return dsp;
}

// Call per frame to read spectrum bin magnitudes
void ReadSpectrum(FMOD::DSP* dsp, float* outBins, int numBins) {
    FMOD_DSP_PARAMETER_FFT* data = nullptr;
    unsigned int length = 0;
    dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA,
                          reinterpret_cast<void**>(&data), &length, nullptr, 0);
    if (data && data->numchannels > 0) {
        int count = std::min(numBins, data->length);
        std::memcpy(outBins, data->spectrum[0], count * sizeof(float));
    }
}
```

---

## FMOD Studio (Event-Based)

```cpp
#include "fmod_studio.hpp"
#pragma comment(lib, "fmodstudio_vc.lib")

FMOD::Studio::System* g_studio = nullptr;

bool InitFMODStudio() {
    FMOD::Studio::System::create(&g_studio);
    return g_studio->initialize(512,
                                 FMOD_STUDIO_INIT_NORMAL,
                                 FMOD_INIT_NORMAL, nullptr) == FMOD_OK;
}

void LoadBank(const char* path) {
    FMOD::Studio::Bank* bank = nullptr;
    g_studio->loadBankFile(path, FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
    // Load both .bank and .strings.bank files!
}

FMOD::Studio::EventInstance* PlayEvent(const char* eventPath) {
    FMOD::Studio::EventDescription* desc = nullptr;
    g_studio->getEvent(eventPath, &desc);  // e.g. "event:/Weapons/Gunshot"
    if (!desc) return nullptr;

    FMOD::Studio::EventInstance* inst = nullptr;
    desc->createInstance(&inst);
    inst->start();
    inst->release();  // auto-release on stop; retain only if you need control
    return inst;
}

void UpdateStudio() { if (g_studio) g_studio->update(); }
```

---

## Common Pitfalls

| Problem | Cause | Fix |
|---|---|---|
| No audio output but no errors | `update()` not called each frame | Call `system->update()` every loop |
| Sound plays once then distorts | Freed sound while channel active | Wait for channel stop before `sound->release()` |
| 3D audio not attenuating | Listener never set | Call `set3DListenerAttributes` each frame |
| Studio event not found | Bank not loaded | Load `.bank` AND `.strings.bank` first |
| Init fails in injected DLL | Audio device in exclusive mode by host | Use `FMOD_INIT_NORMAL`; check `FMOD_RESULT` |
