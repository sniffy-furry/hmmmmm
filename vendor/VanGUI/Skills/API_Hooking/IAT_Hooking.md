# IAT Hooking (Import Address Table)

## What the IAT Is
When Windows loads a PE, it builds the Import Address Table: an array of function
pointers for every imported function. The compiled code calls through these pointers
(e.g., `call qword ptr [__imp_MessageBoxW]`) rather than hardcoding addresses.
Patching one IAT entry redirects all calls to that import within the module — 
without touching the original function.

```
PE Module in Memory:
  .text section:
    call [__imp_MessageBoxW]  →  IAT entry (ptr) → real MessageBoxW

  IAT (.idata / .rdata):
    __imp_MessageBoxW:  0x00007FF... ← patch this pointer
```

---

## IAT Hook vs. MinHook (Inline)

| Aspect | IAT Hook | Inline Hook (MinHook) |
|---|---|---|
| Scope | Only the target module's calls | All callers process-wide |
| Trampoline needed | No | Yes |
| Detectable | Entry differs from GetProcAddress | Code bytes differ from disk |
| Affects dynamically loaded calls | No (only at load time) | Yes |
| Safe for RTTI / vtable | Yes | Yes |
| Works when target inlines the call | No | No (same limitation) |

Use IAT hooking when you want per-module scope or when inline hooking the target
function is unsafe (e.g., too short, guarded region).

---

## Parsing the IAT at Runtime

```cpp
#include <windows.h>
#include <winternl.h>

// Find the IAT entry for a specific function in a specific module
void** FindIATEntry(HMODULE hMod,           // module whose IAT to patch
                    const char* importDll,  // e.g. "user32.dll"
                    const char* funcName)   // e.g. "MessageBoxW"
{
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
    auto* nt  = reinterpret_cast<IMAGE_NT_HEADERS*>(
        reinterpret_cast<uint8_t*>(hMod) + dos->e_lfanew);

    auto* importDir = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        reinterpret_cast<uint8_t*>(hMod) +
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    for (; importDir->Name; importDir++) {
        const char* dllName = reinterpret_cast<const char*>(
            reinterpret_cast<uint8_t*>(hMod) + importDir->Name);

        if (_stricmp(dllName, importDll) != 0) continue;

        // Walk the OriginalFirstThunk (hints/names) and FirstThunk (IAT) in parallel
        auto* hint = reinterpret_cast<IMAGE_THUNK_DATA*>(
            reinterpret_cast<uint8_t*>(hMod) + importDir->OriginalFirstThunk);
        auto* iat  = reinterpret_cast<IMAGE_THUNK_DATA*>(
            reinterpret_cast<uint8_t*>(hMod) + importDir->FirstThunk);

        for (; hint->u1.AddressOfData; hint++, iat++) {
            if (IMAGE_SNAP_BY_ORDINAL(hint->u1.Ordinal)) continue;

            auto* name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                reinterpret_cast<uint8_t*>(hMod) + hint->u1.AddressOfData);

            if (strcmp(name->Name, funcName) == 0)
                return reinterpret_cast<void**>(&iat->u1.Function);
        }
    }
    return nullptr;
}
```

---

## Patching the IAT Entry

```cpp
bool PatchIAT(void** iatEntry, void* newFunc, void** oldFunc) {
    if (!iatEntry) return false;

    DWORD oldProtect;
    VirtualProtect(iatEntry, sizeof(void*), PAGE_READWRITE, &oldProtect);

    if (oldFunc) *oldFunc = *iatEntry;  // save original
    *iatEntry = newFunc;                 // install detour

    VirtualProtect(iatEntry, sizeof(void*), oldProtect, &oldProtect);
    return true;
}
```

---

## Full Example: Hook MessageBoxW in a Specific Module

```cpp
typedef int (WINAPI* PFN_MsgBoxW)(HWND, LPCWSTR, LPCWSTR, UINT);
PFN_MsgBoxW oMessageBoxW = nullptr;

int WINAPI hk_MessageBoxW(HWND hWnd, LPCWSTR text,
                            LPCWSTR caption, UINT type) {
    OutputDebugStringW(L"[IAT] MessageBoxW intercepted");
    return oMessageBoxW(hWnd, L"[Hooked]", caption, type);
}

void InstallIATHook() {
    HMODULE hTarget = GetModuleHandleW(L"target.exe");  // or any loaded module

    void** entry = FindIATEntry(hTarget, "user32.dll", "MessageBoxW");
    PatchIAT(entry, (void*)hk_MessageBoxW, (void**)&oMessageBoxW);
}

void RemoveIATHook() {
    HMODULE hTarget = GetModuleHandleW(L"target.exe");
    void** entry = FindIATEntry(hTarget, "user32.dll", "MessageBoxW");
    PatchIAT(entry, (void*)oMessageBoxW, nullptr);
}
```

---

## Hooking Delay-Load Imports

Delay-load DLLs are loaded on first call. Their IAT is populated lazily
and lives in a different directory entry:

```cpp
// Check IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT instead
auto* delayDir = reinterpret_cast<ImgDelayDescr*>(
    reinterpret_cast<uint8_t*>(hMod) +
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);

// Same walk logic, but use delayDir->rvaINT and delayDir->rvaIAT
// Must hook AFTER the DLL has been loaded (after first call or LoadLibrary)
```

---

## Watching All Modules (Global IAT Hook)

To intercept a function across every currently loaded module:

```cpp
void HookAllModules(const char* dllName, const char* funcName, void* detour) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
                                            GetCurrentProcessId());
    MODULEENTRY32W me{ sizeof(me) };

    if (Module32FirstW(snap, &me)) {
        do {
            void** entry = FindIATEntry(
                (HMODULE)me.modBaseAddr, dllName, funcName);
            if (entry) PatchIAT(entry, detour, nullptr);
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
}
```

---

## Common Pitfalls

| Issue | Cause | Fix |
|---|---|---|
| Entry not found | Function is ordinal-imported | Handle `IMAGE_SNAP_BY_ORDINAL` case |
| Access violation on patch | IAT page is read-only | Use `VirtualProtect` before write |
| Hook stops working | Module reloaded (LoadLibrary) | Re-hook on `LdrDllNotification` callback |
| Only some calls intercepted | Other modules import the same function | Hook each module's IAT separately |
| Detour never called | Compiler inlined the call | IAT hooks can't catch inlined calls |
