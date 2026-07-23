---
name: dll-injection-techniques
description: >
  Complete reference for DLL injection — forcing a target process to load your
  DLL using Windows APIs or memory manipulation. Use when: implementing a
  code-execution loader for an authorized target process, selecting among
  injection methods based on detection risk and privilege requirements,
  resolving injection failures (privilege errors, architecture mismatches,
  loader lock deadlocks), implementing manual mapping for stealthy injection
  without a loader-visible module entry, or writing a payload DLL that
  initializes safely after injection. All techniques apply only to processes
  you own, have written authorization to modify, or that fall under applicable
  research/interoperability exemptions.
tags: [dll, injection, createremotethread, loadlibrary, manual-mapping, shellcode, windows, process, privilege]
related_skills: [DLL_Hijacking, DLL_Proxying, IAT_Hooking, VTable_Hooking]
platforms: [Windows x86, Windows x64]
authorization: Authorized targets only — own processes, written permission, or applicable research/interoperability exemption.
---

================================================================================
  DLL INJECTION TECHNIQUES — COMPLETE TECHNICAL SKILL REFERENCE
  Covers: method selection, CreateRemoteThread/LoadLibrary, SetWindowsHookEx,
          NtCreateThreadEx, APC injection, thread hijacking, manual mapping
          (full PE implementation), privilege management, architecture matching,
          payload DLL best practices, detection signals, and pitfall table.
================================================================================

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 1 — WHAT DLL INJECTION IS
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

DLL injection causes a running target process to load and execute a DLL that
was not part of its original launch. The injected DLL runs inside the target's
address space with access to all its memory, handles, and loaded modules.

Unlike DLL hijacking (which uses the loader's search order at startup),
injection operates against an already-running process — the injector is a
separate process that reaches into the target using cross-process APIs.

Primary use cases:
  - Tool/mod loading into a running process without restarting it
  - Runtime hooking (install function hooks after the process is live)
  - Instrumentation, tracing, and analysis
  - In-process UI overlays
  - Interoperability tooling (authorized automation, accessibility)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
1.1  METHOD SELECTION MATRIX
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Method                       | Detection Risk | Admin Needed | Complexity | Persistent?
  ─────────────────────────────|────────────────|──────────────|────────────|────────────
  CreateRemoteThread+LLW       | High           | Usually      | Low        | No
  SetWindowsHookEx             | Medium         | No           | Low        | No
  NtCreateThreadEx (direct)    | Medium         | Usually      | Medium     | No
  QueueUserAPC (alertable)     | Medium-Low     | Usually      | Medium     | No
  Thread Hijacking             | Low            | Usually      | High       | No
  Manual Mapping               | Low            | Usually      | Very High  | No
  DLL Hijacking (search order) | Low            | No (usually) | Medium     | Yes → see DLL_Hijacking.md
  AppInit_DLLs (registry)      | Low            | Yes          | Low        | Yes

  Choose based on:
    - Detection requirements (Is the target monitored by an AV/EDR?)
    - Whether the process has a message pump (required for SetWindowsHookEx)
    - Whether you need to appear in the module list or not
    - Available privileges

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 2 — PRIVILEGES AND PROCESS ACCESS
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2.1  REQUIRED PROCESS ACCESS RIGHTS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Access right                 | When needed
  ─────────────────────────────|─────────────────────────────────────────────
  PROCESS_CREATE_THREAD        | CreateRemoteThread, NtCreateThreadEx
  PROCESS_QUERY_INFORMATION    | OpenProcess, querying memory layout
  PROCESS_VM_OPERATION         | VirtualAllocEx, VirtualFreeEx, VirtualProtectEx
  PROCESS_VM_WRITE             | WriteProcessMemory
  PROCESS_VM_READ              | ReadProcessMemory (verify writes, resolve symbols)

  Combined constant for CRT injection:
    DWORD access = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                   PROCESS_VM_OPERATION  | PROCESS_VM_WRITE | PROCESS_VM_READ;

  For APC injection only (no thread creation):
    DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;
    // Plus THREAD_SET_CONTEXT on the target thread handle for QueueUserAPC

  For thread hijacking (suspend/redirect existing thread):
    DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;
    // Plus THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME
    //      on the target thread handle

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2.2  ENABLING SeDebugPrivilege
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Required to open processes belonging to other users or with higher integrity.
  Your process must already be running as admin — SeDebugPrivilege grants access
  to adjust the token; it does not elevate a non-admin process.

  ```cpp
  #include <windows.h>

  bool EnablePrivilege(const wchar_t* privName) {
      HANDLE hToken = nullptr;
      if (!OpenProcessToken(GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
          return false;

      TOKEN_PRIVILEGES tp{};
      tp.PrivilegeCount = 1;
      tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

      if (!LookupPrivilegeValueW(nullptr, privName, &tp.Privileges[0].Luid)) {
          CloseHandle(hToken);
          return false;
      }

      BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);
      DWORD err = GetLastError();
      CloseHandle(hToken);
      // AdjustTokenPrivileges returns TRUE even on partial failure —
      // must check GetLastError() separately
      return ok && (err == ERROR_SUCCESS);
  }

  // Usage:
  EnablePrivilege(SE_DEBUG_NAME);          // "SeDebugPrivilege"
  EnablePrivilege(SE_INCREASE_QUOTA_NAME); // may be needed for some operations
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2.3  ARCHITECTURE MATCHING (x86 vs x64)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  You cannot inject a 32-bit DLL into a 64-bit process or vice versa.
  The injector AND the payload DLL must match the target's architecture.

  Detecting target architecture at runtime:

  ```cpp
  bool IsProcess64Bit(HANDLE hProc) {
      BOOL isWow64 = FALSE;
      // If the function doesn't exist, we're on a 32-bit OS
      using FnIsWow64 = BOOL(WINAPI*)(HANDLE, PBOOL);
      auto fn = (FnIsWow64)GetProcAddress(
          GetModuleHandleW(L"kernel32.dll"), "IsWow64Process");
      if (!fn) return false;  // 32-bit OS, target is 32-bit
      fn(hProc, &isWow64);

      // If the target is NOT WoW64, it's a native 64-bit process (on 64-bit OS)
      // If it IS WoW64, it's a 32-bit process running under the WoW64 emulator
  #ifdef _WIN64
      return !isWow64;  // We are 64-bit; target is 64-bit if NOT WoW64
  #else
      return false;     // We are 32-bit; cannot inject into 64-bit target
  #endif
  }
  ```

  Architecture mismatch consequences:
    - LoadLibraryW with wrong arch DLL → ERROR_BAD_EXE_FORMAT (0xC1)
    - Manual mapping with wrong arch → crash or silent failure
    - Always build both x86 and x64 variants of your payload DLL
      and select based on IsProcess64Bit() at inject time

  WoW64 cross-injection (32-bit injector → 64-bit target):
    Not possible via standard Win32 APIs. Requires heaven's gate technique
    (switching to long mode manually) or a 64-bit stub injector process.
    Practical recommendation: compile your injector to match the target.

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 3 — METHOD 1: CreateRemoteThread + LoadLibraryW
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Classic method. Creates a new thread in the target process whose start
  routine is LoadLibraryW, passing your DLL path as the argument.
  The OS loader runs in the target and loads your DLL normally.

  Detection profile: HIGH. Most AV/EDR products signature this pattern
  (VirtualAllocEx + WriteProcessMemory + CreateRemoteThread with LoadLibraryW
  as the start address). Suitable for authorized targets without monitoring.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3.1  FULL IMPLEMENTATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  #include <windows.h>
  #include <string>

  // Injects dllPath into the process identified by pid.
  // Returns true on success. On failure, GetLastError() is meaningful.
  bool InjectCRT(DWORD pid, const wchar_t* dllPath) {
      // --- 1. Open the target process ---
      HANDLE hProc = OpenProcess(
          PROCESS_CREATE_THREAD |
          PROCESS_QUERY_INFORMATION |
          PROCESS_VM_OPERATION |
          PROCESS_VM_WRITE |
          PROCESS_VM_READ,
          FALSE, pid);
      if (!hProc) return false;

      // --- 2. Allocate remote memory for the DLL path string ---
      const SIZE_T pathBytes = (wcslen(dllPath) + 1) * sizeof(wchar_t);
      void* remotePath = VirtualAllocEx(
          hProc, nullptr, pathBytes,
          MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      if (!remotePath) { CloseHandle(hProc); return false; }

      // --- 3. Write the path into the target ---
      SIZE_T written = 0;
      if (!WriteProcessMemory(hProc, remotePath, dllPath, pathBytes, &written)
          || written != pathBytes) {
          VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
          CloseHandle(hProc);
          return false;
      }

      // --- 4. Resolve LoadLibraryW address ---
      // LoadLibraryW lives in kernel32.dll, which is always at the same
      // virtual address across all processes on the same OS session.
      // This assumption holds for standard 64-bit processes; for WoW64
      // cross-injection this breaks — see Section 2.3.
      auto pfnLLW = reinterpret_cast<LPTHREAD_START_ROUTINE>(
          GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
      if (!pfnLLW) {
          VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
          CloseHandle(hProc);
          return false;
      }

      // --- 5. Create remote thread ---
      HANDLE hThread = CreateRemoteThread(
          hProc, nullptr, 0,
          pfnLLW, remotePath,
          0, nullptr);
      if (!hThread) {
          VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
          CloseHandle(hProc);
          return false;
      }

      // --- 6. Wait for LoadLibraryW to return ---
      // The thread exit code is the HMODULE returned by LoadLibraryW,
      // or 0 on failure (DLL failed to load — check Event Log / OutputDebugString)
      WaitForSingleObject(hThread, 10'000);

      DWORD exitCode = 0;
      GetExitCodeThread(hThread, &exitCode);
      bool success = (exitCode != 0);

      // --- 7. Cleanup ---
      CloseHandle(hThread);
      VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
      CloseHandle(hProc);
      return success;
  }
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3.2  PID DISCOVERY HELPER
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  #include <tlhelp32.h>

  DWORD FindProcessByName(const wchar_t* exeName) {
      HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
      if (snap == INVALID_HANDLE_VALUE) return 0;

      PROCESSENTRY32W pe{ sizeof(pe) };
      DWORD pid = 0;

      if (Process32FirstW(snap, &pe)) {
          do {
              if (_wcsicmp(pe.szExeFile, exeName) == 0) {
                  pid = pe.th32ProcessID;
                  break;
              }
          } while (Process32NextW(snap, &pe));
      }
      CloseHandle(snap);
      return pid;
  }

  // Usage:
  DWORD pid = FindProcessByName(L"target.exe");
  if (pid) InjectCRT(pid, L"C:\\tools\\payload.dll");
  ```

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 4 — METHOD 2: SetWindowsHookEx
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Installs a Windows message hook into a GUI thread of the target process.
  The OS loads your DLL into the target the next time that thread processes
  a message. No CreateRemoteThread, no WriteProcessMemory needed.

  Requirements:
    - Target must have a message pump (GetMessage / PeekMessage loop)
    - Your DLL must export the hook procedure
    - For a specific thread: provide the thread ID to SetWindowsHookEx
    - For all threads in a process: use threadId = 0 (injects into every
      thread that pumps messages in that process)

  Limitation: Only works on threads with a message queue. Console apps and
  background services without a UI cannot be injected this way.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
4.1  PAYLOAD DLL SIDE (must export the hook proc)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  // hook_payload.cpp — compile as hook_payload.dll
  #include <windows.h>

  static HMODULE g_hSelf = nullptr;

  // The hook procedure — called by USER32 in the target thread context
  // Must be exported with C linkage and no name mangling
  extern "C" __declspec(dllexport)
  LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
      // Always call next hook in chain — never swallow messages here
      return CallNextHookEx(nullptr, nCode, wParam, lParam);
  }

  BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
      if (reason == DLL_PROCESS_ATTACH) {
          g_hSelf = hMod;
          DisableThreadLibraryCalls(hMod);
          // Spawn worker thread for real initialization
          CreateThread(nullptr, 0,
              [](LPVOID) -> DWORD { /* tool init */ return 0; },
              nullptr, 0, nullptr);
      }
      return TRUE;
  }
  ```

  DEF file (ensure no decorated name):
  ```def
  LIBRARY hook_payload
  EXPORTS
      HookProc
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
4.2  INJECTOR SIDE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  #include <windows.h>

  struct HookInjection {
      HMODULE hPayload;
      HHOOK   hHook;
  };

  // Inject into a specific thread by thread ID
  // threadId: from GetWindowThreadProcessId or EnumThreadWindows
  HookInjection InjectViaHook(DWORD threadId, const wchar_t* dllPath) {
      HookInjection result{};

      // Load the DLL into our process first — USER32 needs the
      // module handle (hmod) to know where to find HookProc in the target
      result.hPayload = LoadLibraryW(dllPath);
      if (!result.hPayload) return result;

      HOOKPROC proc = (HOOKPROC)GetProcAddress(result.hPayload, "HookProc");
      if (!proc) {
          FreeLibrary(result.hPayload);
          result.hPayload = nullptr;
          return result;
      }

      // WH_GETMESSAGE: fires in target thread when it calls GetMessage/PeekMessage
      // WH_CBT:        fires on window creation/destruction — fires earlier
      // WH_CALLWNDPROC: fires when a message is sent to a window
      result.hHook = SetWindowsHookExW(WH_GETMESSAGE, proc, result.hPayload, threadId);
      if (!result.hHook) {
          FreeLibrary(result.hPayload);
          result.hPayload = nullptr;
      }

      // The DLL has NOT loaded into the target yet at this point.
      // It loads on the NEXT call to GetMessage/PeekMessage in targetThread.
      // PostThreadMessage forces a message pump cycle immediately:
      PostThreadMessageW(threadId, WM_NULL, 0, 0);

      return result;
  }

  void RemoveHookInjection(HookInjection& inj) {
      if (inj.hHook)    { UnhookWindowsHookEx(inj.hHook); inj.hHook = nullptr; }
      if (inj.hPayload) { FreeLibrary(inj.hPayload); inj.hPayload = nullptr; }
      // Note: this does NOT unload the DLL from the target process.
      // The DLL's reference count in the target is managed by the target's loader.
  }

  // Get the thread ID of the thread that owns a window handle:
  DWORD GetWindowThreadId(HWND hWnd) {
      DWORD pid = 0;
      return GetWindowThreadProcessId(hWnd, &pid);
  }

  // Find a window by process name then get its thread:
  DWORD FindGuiThreadByProcess(const wchar_t* exeName) {
      HWND hWnd = nullptr;
      // EnumWindows to find a window owned by the target process
      struct Ctx { const wchar_t* name; HWND found; };
      Ctx ctx{ exeName, nullptr };

      EnumWindows([](HWND hw, LPARAM lp) -> BOOL {
          auto* c = reinterpret_cast<Ctx*>(lp);
          DWORD pid = 0;
          GetWindowThreadProcessId(hw, &pid);
          HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
          if (!hProc) return TRUE;
          wchar_t name[MAX_PATH]{};
          GetModuleFileNameExW(hProc, nullptr, name, MAX_PATH);
          CloseHandle(hProc);
          wchar_t* base = wcsrchr(name, L'\\');
          if (base && _wcsicmp(base + 1, c->name) == 0) {
              c->found = hw;
              return FALSE; // stop
          }
          return TRUE;
      }, (LPARAM)&ctx);

      return ctx.found ? GetWindowThreadProcessId(ctx.found, nullptr) : 0;
  }
  ```

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 5 — METHOD 3: NtCreateThreadEx (DIRECT SYSCALL VARIANT)
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  NtCreateThreadEx is the native API that CreateRemoteThread calls internally.
  Calling it directly bypasses some of CreateRemoteThread's higher-level
  behavior (such as setting the thread's initial access token and calling
  CSRSS for the thread registration), which reduces detection surface.

  Still uses VirtualAllocEx + WriteProcessMemory for the path, so memory
  forensics still applies. Lower detection than CRT, not as stealthy as APC
  or manual mapping.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
5.1  NTCREATETHREADEX DECLARATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  NtCreateThreadEx is undocumented. Resolve at runtime from ntdll.dll:

  ```cpp
  #include <windows.h>

  typedef NTSTATUS (NTAPI* PFN_NtCreateThreadEx)(
      PHANDLE             hThread,
      ACCESS_MASK         DesiredAccess,
      LPVOID              ObjectAttributes,
      HANDLE              ProcessHandle,
      LPTHREAD_START_ROUTINE StartAddress,
      LPVOID              Argument,
      ULONG               CreateFlags,       // 0x1 = suspended; 0x4 = skip loader init
      SIZE_T              ZeroBits,
      SIZE_T              StackSize,
      SIZE_T              MaximumStackSize,
      LPVOID              AttributeList      // PROC_THREAD_ATTRIBUTE_LIST or null
  );

  PFN_NtCreateThreadEx pNtCreateThreadEx = nullptr;

  bool ResolveNtCreateThreadEx() {
      HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
      if (!hNtdll) return false;
      pNtCreateThreadEx = (PFN_NtCreateThreadEx)
          GetProcAddress(hNtdll, "NtCreateThreadEx");
      return pNtCreateThreadEx != nullptr;
  }
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
5.2  INJECTION VIA NtCreateThreadEx
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  bool InjectNtCreateThreadEx(DWORD pid, const wchar_t* dllPath) {
      if (!pNtCreateThreadEx && !ResolveNtCreateThreadEx()) return false;

      HANDLE hProc = OpenProcess(
          PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
          PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
          FALSE, pid);
      if (!hProc) return false;

      SIZE_T pathBytes = (wcslen(dllPath) + 1) * sizeof(wchar_t);
      void* remotePath = VirtualAllocEx(hProc, nullptr, pathBytes,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      if (!remotePath) { CloseHandle(hProc); return false; }

      WriteProcessMemory(hProc, remotePath, dllPath, pathBytes, nullptr);

      auto pfnLLW = (LPTHREAD_START_ROUTINE)
          GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");

      HANDLE hThread = nullptr;
      NTSTATUS status = pNtCreateThreadEx(
          &hThread,
          GENERIC_EXECUTE,    // DesiredAccess
          nullptr,            // ObjectAttributes
          hProc,
          pfnLLW,
          remotePath,
          0,                  // CreateFlags (0 = run immediately, no suspend)
          0, 0, 0,
          nullptr);

      bool ok = false;
      if (NT_SUCCESS(status) && hThread) {
          WaitForSingleObject(hThread, 10'000);
          DWORD exit = 0;
          GetExitCodeThread(hThread, &exit);
          ok = (exit != 0);
          CloseHandle(hThread);
      }

      VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
      CloseHandle(hProc);
      return ok;
  }
  ```

  Flag notes for CreateFlags parameter:
    0x0001 (THREAD_CREATE_FLAGS_CREATE_SUSPENDED) — create suspended, resume manually
    0x0004 (THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH) — skip DLL_THREAD_ATTACH in loaded modules
    0x0010 (THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER) — thread not visible to user-mode debuggers

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 6 — METHOD 4: QueueUserAPC (ASYNCHRONOUS PROCEDURE CALL)
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  APCs are callbacks queued to a thread that execute when the thread enters an
  alertable wait state (SleepEx, WaitForSingleObjectEx, etc. with bAlertable=TRUE).
  No new thread is created — the payload runs in an existing thread of the target.

  Limitation: The target thread MUST enter an alertable wait at some point.
  Threads that never call alertable wait functions will never execute the APC.
  Many UI threads and game loops never call alertable waits — test first.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
6.1  APC INJECTION IMPLEMENTATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  #include <windows.h>
  #include <tlhelp32.h>
  #include <vector>

  // Returns all thread IDs belonging to pid
  std::vector<DWORD> GetProcessThreads(DWORD pid) {
      std::vector<DWORD> tids;
      HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
      if (snap == INVALID_HANDLE_VALUE) return tids;

      THREADENTRY32 te{ sizeof(te) };
      if (Thread32First(snap, &te)) {
          do {
              if (te.th32OwnerProcessID == pid)
                  tids.push_back(te.th32ThreadID);
          } while (Thread32Next(snap, &te));
      }
      CloseHandle(snap);
      return tids;
  }

  // Queue LoadLibraryW as an APC to all threads in the target process.
  // The APC fires when any thread enters an alertable wait.
  bool InjectAPC(DWORD pid, const wchar_t* dllPath) {
      HANDLE hProc = OpenProcess(
          PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
          FALSE, pid);
      if (!hProc) return false;

      // Write DLL path into target
      SIZE_T pathBytes = (wcslen(dllPath) + 1) * sizeof(wchar_t);
      void* remotePath = VirtualAllocEx(hProc, nullptr, pathBytes,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      if (!remotePath) { CloseHandle(hProc); return false; }
      WriteProcessMemory(hProc, remotePath, dllPath, pathBytes, nullptr);

      auto pfnLLW = (PAPCFUNC)
          GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");

      // Queue to ALL threads — we don't know which will next be alertable
      bool anyQueued = false;
      for (DWORD tid : GetProcessThreads(pid)) {
          HANDLE hThread = OpenThread(
              THREAD_SET_CONTEXT, FALSE, tid);
          if (!hThread) continue;

          if (QueueUserAPC(pfnLLW, hThread, (ULONG_PTR)remotePath))
              anyQueued = true;

          CloseHandle(hThread);
      }

      // Note: remotePath is intentionally leaked here — freeing it before
      // the APC fires would cause a crash. For clean injection, signal
      // completion from the payload DLL itself and free from there,
      // or accept the small leak (process will reclaim on exit).

      CloseHandle(hProc);
      return anyQueued;
  }
  ```

  Reliable APC alertable-wait trigger: if you control a thread handle and can
  call SetThreadContext on a suspended thread to redirect RIP to LoadLibraryW
  — that is the thread hijacking technique (Part 7) which is more reliable.

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 7 — METHOD 5: THREAD HIJACKING (GetContext/SetContext)
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Suspends an existing thread in the target, redirects its instruction pointer
  to LoadLibraryW with your DLL path, then resumes. No new thread is created.
  Very low detection profile but fragile — the hijacked thread may be in a
  critical section, holding a mutex, or mid-instruction.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
7.1  THREAD HIJACK SHELLCODE APPROACH (x64)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  The simplest hijack redirects RIP to a shellcode stub that:
    1. Calls LoadLibraryW(remotePath)
    2. Restores original register state
    3. Jumps back to the original RIP

  ```cpp
  #include <windows.h>
  #include <tlhelp32.h>

  // x64 shellcode: sub rsp,28h / mov rcx,[rip+offset] / call [rip+llw_offset] /
  //                add rsp,28h / jmp [rip+orig_rip_offset]
  // In practice, construct this as a struct with relative offsets at runtime.

  #pragma pack(push, 1)
  struct HijackShellcode64 {
      // sub rsp, 0x28 (shadow space + align)
      uint8_t  sub_rsp[4]   = { 0x48, 0x83, 0xEC, 0x28 };
      // mov rcx, <path_ptr>  (absolute 64-bit address)
      uint8_t  mov_rcx[2]   = { 0x48, 0xB9 };
      uint64_t path_addr     = 0;
      // mov rax, <LoadLibraryW addr>
      uint8_t  mov_rax[2]   = { 0x48, 0xB8 };
      uint64_t llw_addr      = 0;
      // call rax
      uint8_t  call_rax[2]  = { 0xFF, 0xD0 };
      // add rsp, 0x28
      uint8_t  add_rsp[4]   = { 0x48, 0x83, 0xC4, 0x28 };
      // mov rax, <orig_rip>
      uint8_t  mov_rax2[2]  = { 0x48, 0xB8 };
      uint64_t orig_rip      = 0;
      // jmp rax
      uint8_t  jmp_rax[2]   = { 0xFF, 0xE0 };
  };
  #pragma pack(pop)

  bool InjectThreadHijack(DWORD tid, HANDLE hProc, const wchar_t* dllPath) {
      HANDLE hThread = OpenThread(
          THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME,
          FALSE, tid);
      if (!hThread) return false;

      // --- Suspend ---
      if (SuspendThread(hThread) == (DWORD)-1) {
          CloseHandle(hThread); return false;
      }

      CONTEXT ctx{};
      ctx.ContextFlags = CONTEXT_FULL;
      if (!GetThreadContext(hThread, &ctx)) {
          ResumeThread(hThread);
          CloseHandle(hThread);
          return false;
      }

      // --- Write DLL path ---
      SIZE_T pathBytes = (wcslen(dllPath) + 1) * sizeof(wchar_t);
      void* remotePath = VirtualAllocEx(hProc, nullptr, pathBytes,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      WriteProcessMemory(hProc, remotePath, dllPath, pathBytes, nullptr);

      // --- Write shellcode ---
      HijackShellcode64 sc;
      sc.path_addr = (uint64_t)remotePath;
      sc.llw_addr  = (uint64_t)GetProcAddress(
          GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
      sc.orig_rip  = ctx.Rip;

      void* remoteSC = VirtualAllocEx(hProc, nullptr, sizeof(sc),
                                       MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
      WriteProcessMemory(hProc, remoteSC, &sc, sizeof(sc), nullptr);
      FlushInstructionCache(hProc, remoteSC, sizeof(sc));

      // --- Redirect RIP ---
      ctx.Rip = (DWORD64)remoteSC;
      SetThreadContext(hThread, &ctx);

      // --- Resume ---
      ResumeThread(hThread);
      CloseHandle(hThread);

      // The shellcode will execute on next thread scheduling slice,
      // call LoadLibraryW, then return to original RIP.
      // remoteSC and remotePath can be freed after a delay or from the payload.
      return true;
  }
  ```

  Risks of thread hijacking:
    - Thread may be inside a system call; RIP may be in ntdll — redirecting
      from kernel-mode return is unsafe on some versions
    - Thread holding the loader lock will deadlock when LoadLibraryW runs
    - Registers (rcx, rdx, r8, r9, rax) are clobbered — may corrupt caller state
      if the original thread was mid-function
    - Check suspend count: if SuspendThread returns > 0, thread was already
      suspended by something else — be careful

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 8 — METHOD 6: MANUAL MAPPING (NO-LOADLIBRARY INJECTION)
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Manual mapping replicates what LoadLibrary does, but entirely from the
  injector side — no call to the Windows loader in the target process.
  The DLL does not appear in the PEB module list, leaving no LoadLibrary
  trace, no entry in EnumProcessModules(), no entry in lm (WinDbg).

  This is the highest-stealth injection technique short of kernel-level rootkits.
  It is also the most complex, requiring full PE parsing and relocation handling.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
8.1  MANUAL MAPPING STAGES
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  Stage 1 — Load DLL into local buffer
  Stage 2 — Allocate SizeOfImage bytes in target (PAGE_EXECUTE_READWRITE initially)
  Stage 3 — Write PE headers into remote allocation
  Stage 4 — Map each section individually (raw offset → virtual offset)
  Stage 5 — Process base relocations (delta between preferred and actual base)
  Stage 6 — Resolve imports (GetProcAddress in injector context + write to remote IAT)
  Stage 7 — Flush instruction cache
  Stage 8 — Apply correct section protections (RX for .text, RW for .data, etc.)
  Stage 9 — Execute DllMain via CreateRemoteThread (or shellcode stub)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
8.2  FULL MANUAL MAPPER IMPLEMENTATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  #include <windows.h>
  #include <vector>
  #include <fstream>
  #include <stdexcept>

  using byte = unsigned char;

  // Read entire file into buffer
  std::vector<byte> ReadFile(const wchar_t* path) {
      std::ifstream f(path, std::ios::binary | std::ios::ate);
      if (!f) throw std::runtime_error("cannot open DLL");
      auto sz = (size_t)f.tellg();
      f.seekg(0);
      std::vector<byte> buf(sz);
      f.read((char*)buf.data(), sz);
      return buf;
  }

  // Translate from file-offset (PointerToRawData) to RVA
  DWORD RawToRVA(IMAGE_NT_HEADERS* nt, DWORD raw) {
      auto* sec = IMAGE_FIRST_SECTION(nt);
      for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
          if (raw >= sec->PointerToRawData &&
              raw <  sec->PointerToRawData + sec->SizeOfRawData) {
              return raw - sec->PointerToRawData + sec->VirtualAddress;
          }
      }
      return raw; // fallback for header data
  }

  bool ManualMap(HANDLE hProc, const wchar_t* dllPath) {
      // --- Stage 1: Load DLL file ---
      auto buf = ReadFile(dllPath);
      byte* local = buf.data();

      auto* dos = (IMAGE_DOS_HEADER*)local;
      if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

      auto* nt = (IMAGE_NT_HEADERS*)(local + dos->e_lfanew);
      if (nt->Signature != IMAGE_NT_SIGNATURE)  return false;

      auto& opt = nt->OptionalHeader;

      // --- Stage 2: Allocate in target ---
      // Try preferred base first; if taken, let the OS choose
      byte* remoteBase = (byte*)VirtualAllocEx(
          hProc, (void*)opt.ImageBase, opt.SizeOfImage,
          MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
      if (!remoteBase) {
          remoteBase = (byte*)VirtualAllocEx(
              hProc, nullptr, opt.SizeOfImage,
              MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
      }
      if (!remoteBase) return false;

      uintptr_t delta = (uintptr_t)remoteBase - (uintptr_t)opt.ImageBase;

      // --- Stage 3: Write PE headers ---
      WriteProcessMemory(hProc, remoteBase, local, opt.SizeOfHeaders, nullptr);

      // --- Stage 4: Map sections ---
      auto* sec = IMAGE_FIRST_SECTION(nt);
      for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
          if (!sec->SizeOfRawData) continue;
          WriteProcessMemory(
              hProc,
              remoteBase + sec->VirtualAddress,
              local + sec->PointerToRawData,
              sec->SizeOfRawData, nullptr);
      }

      // --- Stage 5: Apply base relocations ---
      if (delta != 0 && opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
          auto* reloc = (IMAGE_BASE_RELOCATION*)(local +
              opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

          while (reloc->VirtualAddress) {
              DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2;
              auto* entries = (WORD*)(reloc + 1);

              for (DWORD j = 0; j < count; j++) {
                  int type   = entries[j] >> 12;
                  int offset = entries[j] & 0xFFF;

                  if (type == IMAGE_REL_BASED_DIR64) {
                      // Read the 64-bit value at local copy, add delta, write remotely
                      uintptr_t localVA = (uintptr_t)(local +
                          reloc->VirtualAddress + offset);
                      uintptr_t remoteVA = (uintptr_t)(remoteBase +
                          reloc->VirtualAddress + offset);

                      uintptr_t val;
                      memcpy(&val, (void*)localVA, sizeof(val));
                      val += delta;
                      WriteProcessMemory(hProc, (void*)remoteVA, &val, sizeof(val), nullptr);
                  }
                  // IMAGE_REL_BASED_HIGHLOW for 32-bit (type == 3)
                  // IMAGE_REL_BASED_ABSOLUTE (type == 0) = padding, skip
              }

              reloc = (IMAGE_BASE_RELOCATION*)((byte*)reloc + reloc->SizeOfBlock);
          }
      }

      // --- Stage 6: Resolve imports ---
      if (opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {
          auto* imp = (IMAGE_IMPORT_DESCRIPTOR*)(local +
              opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

          for (; imp->Name; imp++) {
              const char* dllName = (const char*)(local + imp->Name);
              HMODULE hDep = LoadLibraryA(dllName);
              if (!hDep) continue; // dependency not found — acceptable for some cases

              auto* thunk    = (IMAGE_THUNK_DATA*)(local + imp->FirstThunk);
              auto* origThunk = (IMAGE_THUNK_DATA*)(local + imp->OriginalFirstThunk);
              uintptr_t iatBase = (uintptr_t)remoteBase + imp->FirstThunk;

              for (int k = 0; thunk->u1.Function; thunk++, origThunk++, k++) {
                  uintptr_t fnAddr;

                  if (IMAGE_SNAP_BY_ORDINAL(origThunk->u1.Ordinal)) {
                      fnAddr = (uintptr_t)GetProcAddress(
                          hDep, (LPCSTR)IMAGE_ORDINAL(origThunk->u1.Ordinal));
                  } else {
                      auto* ibn = (IMAGE_IMPORT_BY_NAME*)(local +
                          origThunk->u1.AddressOfData);
                      fnAddr = (uintptr_t)GetProcAddress(hDep, ibn->Name);
                  }

                  if (fnAddr) {
                      WriteProcessMemory(
                          hProc,
                          (void*)(iatBase + (uintptr_t)k * sizeof(uintptr_t)),
                          &fnAddr, sizeof(fnAddr), nullptr);
                  }
              }
          }
      }

      // --- Stage 7: Flush instruction cache ---
      FlushInstructionCache(hProc, remoteBase, opt.SizeOfImage);

      // --- Stage 8: Apply section protections ---
      sec = IMAGE_FIRST_SECTION(nt);
      for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
          if (!sec->SizeOfRawData) continue;

          DWORD protect = PAGE_READONLY;
          bool exec  = (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
          bool write = (sec->Characteristics & IMAGE_SCN_MEM_WRITE)   != 0;
          bool read  = (sec->Characteristics & IMAGE_SCN_MEM_READ)    != 0;

          if (exec  && write) protect = PAGE_EXECUTE_READWRITE;
          else if (exec && read)  protect = PAGE_EXECUTE_READ;
          else if (exec)          protect = PAGE_EXECUTE;
          else if (write)         protect = PAGE_READWRITE;

          DWORD old;
          VirtualProtectEx(hProc, remoteBase + sec->VirtualAddress,
                           sec->SizeOfRawData, protect, &old);
      }

      // --- Stage 9: Call DllMain ---
      if (opt.AddressOfEntryPoint) {
          auto entryPoint = (LPTHREAD_START_ROUTINE)(remoteBase + opt.AddressOfEntryPoint);
          HANDLE hThread = CreateRemoteThread(
              hProc, nullptr, 0, entryPoint, nullptr, 0, nullptr);
          if (hThread) {
              WaitForSingleObject(hThread, 10'000);
              CloseHandle(hThread);
          }
      }

      return true;
  }
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
8.3  MANUAL MAPPING LIMITATIONS AND EDGE CASES
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  TLS callbacks not handled:
    IMAGE_DIRECTORY_ENTRY_TLS contains a list of callbacks that the loader
    normally runs before DllMain. If your payload DLL uses TLS (__declspec(thread)
    or TLS callbacks), manually invoke them before calling DllMain.

    ```cpp
    if (opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) {
        auto* tlsDir = (IMAGE_TLS_DIRECTORY*)(local +
            opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
        auto** callbacks = (PIMAGE_TLS_CALLBACK*)tlsDir->AddressOfCallBacks;
        if (callbacks) {
            // Callbacks store absolute VAs in the local image — adjust by delta
            while (*callbacks) {
                auto cb = (PIMAGE_TLS_CALLBACK)((byte*)*callbacks + delta);
                // Call in target via shellcode or remote thread
                callbacks++;
            }
        }
    }
    ```

  Exception directory not registered:
    SEH and C++ exceptions in the manually mapped DLL will not work unless
    RtlAddFunctionTable is called to register the RUNTIME_FUNCTION table
    with the OS exception dispatcher.

    ```cpp
    // After mapping, register exception handlers:
    PRUNTIME_FUNCTION funcTable = (PRUNTIME_FUNCTION)(remoteBase +
        opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress);
    DWORD funcCount = opt.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size /
        sizeof(RUNTIME_FUNCTION);
    // Call RtlAddFunctionTable in target via shellcode:
    // RtlAddFunctionTable(funcTable, funcCount, (DWORD64)remoteBase)
    ```

  Avoid __declspec(dllimport) in manually mapped DLLs:
    Import stubs use indirect calls through the IAT (call [__imp_X]).
    Manual mapping does resolve the IAT, but if the DLL was compiled expecting
    absolute import addresses (e.g. compiled with /FIXED), relocation may fail.
    Always compile with /DYNAMICBASE for manually-mapped payloads.

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 9 — PAYLOAD DLL BEST PRACTICES
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
9.1  DLLMAIN RULES
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  DllMain is invoked while the OS loader lock is held. Blocking or calling
  loader-related APIs here will deadlock the target process.

  NEVER in DllMain:
    ✗ LoadLibrary / FreeLibrary / GetModuleHandle (acquires loader lock)
    ✗ CoInitialize / CoCreateInstance (may trigger LoadLibrary internally)
    ✗ Functions that allocate from the CRT heap in ways that acquire locks
    ✗ WaitForSingleObject on a thread that could call LoadLibrary
    ✗ Any I/O that may block indefinitely

  SAFE in DllMain:
    ✓ DisableThreadLibraryCalls(hMod)   — suppress thread attach/detach
    ✓ Storing hMod to a global variable
    ✓ CreateThread (but do NOT WaitForSingleObject on it)
    ✓ TlsAlloc / TlsSetValue
    ✓ Simple flag/atomic assignments
    ✓ OutputDebugStringW (safe; uses shared memory, not loader)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
9.2  CANONICAL PAYLOAD DLLMAIN
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ```cpp
  #include <windows.h>
  #include <atomic>

  static HMODULE             g_hSelf   = nullptr;
  static HANDLE              g_hWorker = nullptr;
  static std::atomic<bool>   g_running { false };

  static DWORD WINAPI ToolMain(LPVOID) {
      // Safe to do anything here — outside loader lock
      // LoadLibrary, COM, CRT, network, etc. all OK
      g_running.store(true);

      // --- Initialize your tool here ---

      while (g_running.load()) {
          // Main loop if needed
          Sleep(1);
      }

      // Cleanup
      FreeLibraryAndExitThread(g_hSelf, 0); // unload self and exit thread
      return 0;
  }

  BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
      switch (reason) {
      case DLL_PROCESS_ATTACH:
          g_hSelf = hMod;
          DisableThreadLibraryCalls(hMod);
          g_hWorker = CreateThread(nullptr, 0, ToolMain, nullptr, 0, nullptr);
          if (g_hWorker) CloseHandle(g_hWorker); // detach handle; thread runs independently
          break;

      case DLL_PROCESS_DETACH:
          g_running.store(false);
          // Do NOT wait here — process may be in shutdown, deadlock risk
          break;
      }
      return TRUE;
  }
  ```

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
9.3  SELF-UNLOADING (CONTROLLED EJECT)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  To unload the injected DLL from within itself:

  ```cpp
  // Option A: FreeLibraryAndExitThread — unloads DLL and exits current thread atomically
  FreeLibraryAndExitThread(g_hSelf, 0);
  // This is safe because it avoids the race where the DLL is freed while its
  // code is still executing on the thread stack.

  // Option B: Queue unload via a remote thread from the injector:
  // From injector side, call CreateRemoteThread with FreeLibrary as start address
  // and the HMODULE of the payload (obtained from GetExitCodeThread after inject)
  // as the argument.
  ```

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 10 — DETECTION SIGNALS AND MITIGATION
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Signal                                    | Method(s) that emit it    | Mitigation
  ──────────────────────────────────────────|───────────────────────────|─────────────────────────────
  VirtualAllocEx + WPM + CRT triplet        | CRT, NtCTE, APC, hijack   | Use manual mapping
  CreateRemoteThread with LLW start address | CRT                       | Use NtCreateThreadEx
  LoadLibrary in remote thread context      | CRT, NtCTE                | Use manual mapping
  New thread from external process          | CRT, NtCTE                | Use APC / hijack
  PAGE_EXECUTE_READWRITE allocation         | Manual mapping (stage 2)  | Apply final protections (stage 8)
  Module visible in EnumProcessModules      | CRT, NtCTE, hook, APC     | Use manual mapping
  Module name differs from real DLL         | Any LoadLibrary-based     | Use DLL hijacking instead
  QueueUserAPC to foreign thread            | APC                       | Accept; low risk without EDR
  Unexpected RIP redirect in suspended thrd | Thread hijack             | Keep shellcode in RX memory

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 11 — PITFALLS & TROUBLESHOOTING
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  Symptom                              | Root Cause                        | Fix
  ─────────────────────────────────────|───────────────────────────────────|────────────────────────────────────
  OpenProcess returns NULL             | Insufficient privilege            | Enable SeDebugPrivilege or run as admin
  VirtualAllocEx fails                 | Target exited / access denied     | Re-open handle; check target is alive
  WriteProcessMemory fails             | Page protection or PROCESS_VM_WRITE missing | Verify access mask
  CreateRemoteThread returns NULL      | PROCESS_CREATE_THREAD missing     | Add to access mask
  LoadLibraryW returns 0 (exit code 0) | DLL failed to load in target      | Check arch match; verify path is absolute
  DLL loads but DllMain never runs     | Architecture mismatch             | Match DLL arch to target arch
  Target crashes after injection       | Blocking in DllMain               | Move all work to worker thread
  Deadlock on LoadLibraryW call        | Loader lock already held by target| Wait for target to finish loading; inject later
  APC never fires                      | Thread never enters alertable wait| Queue to all threads; or use CRT instead
  Manual map: access violation in DLL  | Relocs not applied / import wrong | Verify delta, re-check reloc loop
  Manual map: C++ exceptions crash     | RtlAddFunctionTable not called    | Register exception directory (see 8.3)
  Manual map: TLS vars are zero        | TLS callbacks not invoked         | Invoke TLS callbacks before DllMain (see 8.3)
  Thread hijack: target hangs          | Hijacked thread held a mutex      | Choose a thread not in a wait state
  Thread hijack: AV on shellcode write | DEP / CFG on RWX alloc            | Use RW alloc + VirtualProtectEx to RX after write
  SetWindowsHookEx fails (0)           | DLL not loaded in injector first  | LoadLibraryW in injector before SetWindowsHookEx
  Hook fires but DLL loads in injector | threadId = 0 with wrong scope     | Verify threadId belongs to target process thread

▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  PART 12 — QUICK REFERENCE
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

  I want to inject into a running process.
  ↓
  Does the target have a message pump (GUI)?
    Yes AND no need for stealth → SetWindowsHookEx (Part 4)
    No  ↓
  Is detection risk acceptable (no EDR/AV monitoring)?
    Yes → CreateRemoteThread + LoadLibraryW (Part 3) — simplest, most reliable
    No  ↓
  Must the DLL be invisible to module enumeration?
    Yes → Manual Mapping (Part 8)
    No  ↓
  Can you avoid creating a new thread in the target?
    Yes AND threads may enter alertable waits → QueueUserAPC (Part 6)
    Yes AND you can find a safe thread to redirect → Thread Hijacking (Part 7)
    No  → NtCreateThreadEx (Part 5) — lower profile than CRT

  Prefer DLL Hijacking (DLL_Hijacking.md) over injection when:
    - The process can be restarted
    - You want persistence across restarts
    - You want zero cross-process API footprint

================================================================================
  END OF DLL_INJECTION SKILL
================================================================================
