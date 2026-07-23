# DLL Proxying

## What It Is
DLL proxying replaces a legitimate DLL with your own DLL that:
1. Forwards all original exports to the real DLL (so the app keeps working)
2. Executes your code at load time or in specific intercepted functions

No `CreateRemoteThread`, no `WriteProcessMemory` — the OS loads your DLL
through the normal loader as part of application startup.

---

## When to Use DLL Proxying

- Application searches for a DLL in its own directory before System32 (DLL search order)
- You want code execution without any injection mechanism
- You need to intercept specific API calls made to a system DLL
- You want a persistent, auto-loading payload tied to one application

---

## DLL Search Order (Windows Default)

```
1. KnownDLLs (HKEY_LOCAL_MACHINE\SYSTEM\...KnownDLLs) — cannot proxy these
2. Application directory            ← primary proxy opportunity
3. System directory (System32)
4. 16-bit system directory
5. Windows directory
6. %PATH% directories
```

Target DLLs that are NOT in KnownDLLs and are loaded by the application directly.
Common candidates: `version.dll`, `winmm.dll`, `dinput8.dll`, `xinput1_3.dll`,
`d3d9.dll`, `dsound.dll`, `iphlpapi.dll`.

---

## Generating Export Forwarders (MSVC Pragma Method)

The simplest proxy: use linker `#pragma comment(linker, "/EXPORT:...")` to forward
each export to the real DLL. No function body needed.

```cpp
// proxy_version.cpp
// Rename to version.dll; place the real dll as version_orig.dll

#pragma comment(linker, "/EXPORT:GetFileVersionInfoA=version_orig.GetFileVersionInfoA")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoW=version_orig.GetFileVersionInfoW")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeA=version_orig.GetFileVersionInfoSizeA")
#pragma comment(linker, "/EXPORT:GetFileVersionInfoSizeW=version_orig.GetFileVersionInfoSizeW")
#pragma comment(linker, "/EXPORT:VerQueryValueA=version_orig.VerQueryValueA")
#pragma comment(linker, "/EXPORT:VerQueryValueW=version_orig.VerQueryValueW")
// ... all exports from the original

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, ToolMain, hMod, 0, nullptr);
    }
    return TRUE;
}
```

---

## Generating Export List Automatically

Use `dumpbin /exports` or a script to extract all exports:

```powershell
# PowerShell: generate pragma lines for all exports of a DLL
$dll    = "C:\Windows\System32\version.dll"
$orig   = "version_orig"
$output = "version_exports.h"

$exports = & dumpbin /exports $dll |
    Select-String '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)' |
    ForEach-Object { $_.Matches[0].Groups[1].Value }

$lines = $exports | ForEach-Object {
    "#pragma comment(linker, `"/EXPORT:$_=$orig.$_`")"
}
$lines | Out-File -Encoding ascii $output
Write-Host "Generated $($lines.Count) exports"
```

---

## Selective Interception (Not Just Forwarding)

To intercept one function while forwarding the rest:

```cpp
// Forward everything EXCEPT the one you want to intercept
#pragma comment(linker, "/EXPORT:GetFileVersionInfoA=version_orig.GetFileVersionInfoA")
// (all others forwarded)
// GetFileVersionInfoW is NOT forwarded — we implement it:

// Load the real DLL once
static HMODULE hReal = nullptr;
typedef BOOL (WINAPI* PFN_GFVIW)(LPCWSTR, DWORD, DWORD, LPVOID);
static PFN_GFVIW oGetFileVersionInfoW = nullptr;

void EnsureReal() {
    if (!hReal) {
        hReal = LoadLibraryW(L"version_orig.dll");
        oGetFileVersionInfoW = (PFN_GFVIW)GetProcAddress(hReal,
                                                           "GetFileVersionInfoW");
    }
}

extern "C" __declspec(dllexport)
BOOL WINAPI GetFileVersionInfoW(LPCWSTR name, DWORD handle,
                                 DWORD len, LPVOID data) {
    EnsureReal();
    // Intercept logic here
    LogCall(name);
    return oGetFileVersionInfoW(name, handle, len, data);
}
```

---

## DEF File Method (Alternative to Pragma)

```def
; version.def
LIBRARY version
EXPORTS
    GetFileVersionInfoA  = version_orig.GetFileVersionInfoA
    GetFileVersionInfoW  = version_orig.GetFileVersionInfoW
    GetFileVersionInfoSizeA = version_orig.GetFileVersionInfoSizeA
    GetFileVersionInfoSizeW = version_orig.GetFileVersionInfoSizeW
    VerQueryValueA       = version_orig.VerQueryValueA
    VerQueryValueW       = version_orig.VerQueryValueW
```

```cmake
target_link_options(proxy PRIVATE /DEF:version.def)
```

---

## Common Proxy Targets and Why

| DLL | Why It Works |
|---|---|
| `version.dll` | Almost no app is in KnownDLLs; loaded by most EXEs for manifest checking |
| `winmm.dll` | Windows multimedia; loaded by games; not in KnownDLLs in older configs |
| `dinput8.dll` | DirectInput; loaded by many games before DirectX hook possible |
| `xinput1_3.dll` | Common gamepad DLL; loaded from app dir on older titles |
| `d3d9.dll` | For very old titles (pre-DX11); hook 3D pipeline from day one |
| `dsound.dll` | DirectSound; audio interception entry point |
| `iphlpapi.dll` | Network utilities; loaded by some launchers |

---

## Detection & Pitfalls

| Pitfall | Mitigation |
|---|---|
| App checks DLL checksum/signature | Place real DLL as `_orig.dll` and load it dynamically |
| App enumerates loaded modules | Your DLL name matches the original — transparent |
| Forward resolves to missing export | Verify the orig DLL has every export you forward |
| Circular load (proxy loads itself) | Use full absolute path in `LoadLibraryW` to orig |
