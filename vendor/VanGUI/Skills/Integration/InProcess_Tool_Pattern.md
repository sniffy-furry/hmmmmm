# Integration Pattern — In-Process Tool

## Canonical Stack (RE + Hook + UI + Audio)

```
DllMain (attach)
  └─ CreateThread → ToolThread
       ├─ InitFMOD()
       ├─ MH_Initialize()
       ├─ Install hooks (Present, target functions)
       ├─ ImGui context + backend init
       └─ Loop:
            ├─ Process input (toggle UI visibility, hotkeys)
            ├─ RenderFrame()  ← ImGui drawn in hooked Present
            ├─ UpdateFMOD()
            └─ Sleep(1)       ← yield; never spin-wait at 100% CPU

DllMain (detach)
  └─ MH_DisableHook(MH_ALL_HOOKS)
  └─ MH_Uninitialize()
  └─ ImGui cleanup
  └─ ShutdownFMOD()
```

> **Critical Rule:** All initialization belongs on the worker thread, never in DllMain.
> The loader lock is held in DllMain; calling any Win32 API that acquires a lock
> (including heap allocation, which FMOD and ImGui do) will deadlock.

---

## Initialization Order

| Order | Component | Reason |
|---|---|---|
| 1 | FMOD | No dependencies |
| 2 | MinHook (`MH_Initialize`) | Must precede any `MH_CreateHook` |
| 3 | ImGui | Requires D3D device (may need Present hook first) |

---

## Decision Framework

| Task Type | First Question | Recommended Path |
|---|---|---|
| RE task | What is the authorization + goal? | Static analysis → annotate → dynamic validation |
| Hooking task | Exported vs. internal? x86 vs. x64? | MinHook for user-mode native; Detours for mid-function |
| UI task | Standalone tool or in-process overlay? | Pick backend, apply theme, never ship default grey ImGui |
| Audio task | Programmatic control or designer-driven? | Core for code control; Studio for event-driven |
| Integration | Does it touch DllMain? | Move everything to a worker thread |

---

## Sample Worker Thread Skeleton

```cpp
DWORD WINAPI ToolThread(LPVOID) {
    // 1. Audio
    if (!InitFMOD()) return 1;

    // 2. Hooking
    if (MH_Initialize() != MH_OK) return 1;

    // 3. Hook Present to get a D3D11 device + context
    InstallPresentHook();

    // 4. ImGui (initialized inside the first Present call)
    //    See RenderFrame() for deferred init pattern

    // 5. Main loop
    while (!g_should_exit) {
        ProcessHotkeys();
        UpdateFMOD();
        Sleep(1);
    }

    // 6. Cleanup
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    ShutdownFMOD();

    return 0;
}
```

---

## Ethics & Scope

### Covered Use Cases
- Authorized security research
- Tool development for software you own
- Game modding where EULA and jurisdiction permit
- Interoperability engineering
- Audio/UI production work

### Never Assisted
- Bypassing anti-cheat, DRM, or license enforcement
- Unauthorized modification of third-party software
- Any target without ownership or explicit written authorization
