# MinHook — API Hooking

## What It Is
MinHook is a minimalistic x86/x64 inline API hooking library for Windows.
It patches the first bytes of a target function with a JMP to your detour,
saves the displaced bytes into a trampoline so the original can still be called,
and manages the trampoline lifecycle automatically.

---

## MinHook vs. Alternatives

| Scenario | Best Choice | Reason |
|---|---|---|
| Hook a known exported function | **MinHook** | Simple, stable, well-maintained |
| Hook mid-function (non-prologue) | Detours or manual | MinHook only patches prologues |
| Hook managed (.NET) code | Harmony / dnSpy hooks | MinHook is native only |
| Hook COM vtable method | vtable patching (manual) | No trampoline needed for vtable |
| Hook kernel functions | EasyHook or WDM filter | MinHook is user-mode only |
| Production-grade multi-platform | Microsoft Detours | More complete, but LGPL/commercial |

---

## Setup

```cpp
#include "MinHook.h"
#pragma comment(lib, "libMinHook.x64.lib")  // or .x86 for 32-bit

// Call once at startup
if (MH_Initialize() != MH_OK) { /* handle error */ }

// At shutdown:
MH_Uninitialize();
```

---

## Correct Hook Pattern

```cpp
// 1. Declare a function pointer matching the target's exact signature
typedef BOOL (WINAPI* PFN_MessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT);
PFN_MessageBoxW oMessageBoxW = nullptr;  // will point to trampoline after hook

// 2. Write your detour — exact same signature and calling convention
BOOL WINAPI hk_MessageBoxW(HWND hWnd, LPCWSTR lpText,
                            LPCWSTR lpCaption, UINT uType) {
    // Do your work, then forward to the original via the trampoline
    return oMessageBoxW(hWnd, L"[Intercepted]", lpCaption, uType);
}

// 3. Create and enable
MH_STATUS status = MH_CreateHook(
    &MessageBoxW,                                    // target address
    &hk_MessageBoxW,                                 // your detour
    reinterpret_cast<LPVOID*>(&oMessageBoxW)         // trampoline output
);
if (status != MH_OK) { /* handle */ }
MH_EnableHook(&MessageBoxW);
```

---

## Hooking Non-Exported Functions

```cpp
// Base address of the target module
uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"target.exe"));

// Offset discovered via Ghidra/IDA during RE work
uintptr_t fnAddr = base + 0x12ABC0;
void* target = reinterpret_cast<void*>(fnAddr);

MH_CreateHook(target, &MyDetour, reinterpret_cast<LPVOID*>(&oOriginal));
MH_EnableHook(target);
```

---

## Reentrancy Guard

```cpp
thread_local bool tls_in_hook = false;

BOOL WINAPI hk_MessageBoxW(HWND hWnd, LPCWSTR lpText,
                            LPCWSTR lpCaption, UINT uType) {
    if (tls_in_hook) return oMessageBoxW(hWnd, lpText, lpCaption, uType);
    tls_in_hook = true;

    // ... your hook logic ...

    tls_in_hook = false;
    return result;
}
```

---

## Thread Safety Rules

- **Never** create or enable hooks from `DllMain` — wait until a worker thread is outside loader lock
- `MH_EnableHook` / `MH_DisableHook` use `VirtualProtect` + cache flushing internally; not atomic
- Use `MH_ApplyQueued` with all hooks queued at once to minimize the race window
- To remove: Disable first → wait for threads to exit the trampoline → then remove

---

## Common Failures

| Symptom | Cause | Fix |
|---|---|---|
| Access violation on first call | Wrong function signature or calling convention | Match exactly, including `WINAPI`/`__cdecl` |
| Hook installs but never fires | Compiler inlined a different copy of the function | Hook the actual VA, not the import stub |
| Deadlock on enable | Called from DllMain | Move hook installation to a thread post-attach |
| Target crashes after unhook | Threads in the trampoline when removed | Disable first, wait, then remove |
