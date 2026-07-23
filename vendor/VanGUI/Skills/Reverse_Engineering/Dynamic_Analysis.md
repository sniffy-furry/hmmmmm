# Dynamic Analysis & Debugging

## Core Principle
Dynamic analysis validates static hypotheses. Run in an isolated VM snapshot — always.
Save a clean snapshot before every attach session; roll back after.

---

## x64dbg — Windows User-Mode Debugging

### Essential Setup
```
Plugins to install:
  - ScyllaHide         — hides debugger from anti-debug checks
  - xAnalyzer          — auto-annotates API calls in disassembly
  - OllyDumpEx         — dump + reconstruct imports from a running process

Options → Preferences:
  - Check "Break on DLL Load" for injection analysis
  - Check "System Breakpoint" to catch early execution
```

### Conditional Breakpoints
```
// Break only when rcx points to a specific string
[rcx:s] == "target_value"

// Break when eax equals a magic constant
eax == 0xDEADBEEF

// Log RIP + arg without stopping (trace)
// Right-click BP → Edit → Condition: 0  (always false = log only)
// Log: "Hit at {p:rip}, arg={p:rcx}"
```

### Tracing Techniques
| Technique | x64dbg Command | Use Case |
|---|---|---|
| Trace into | `Ctrl+F7` | Step every instruction, log to file |
| Run to user code | `Alt+F9` | Skip through system DLL frames |
| Run to return | `Ctrl+F9` | Exit current call quickly |
| Hardware BP on write | `HW BP on [addr]` | Catch who modifies a memory address |
| Memory BP | Right-click memory → Break on access | Wider range than HW BP |

### Dump & Reconstruct IAT
```
1. Run target until fully unpacked (OEP reached)
2. Scylla → Dump (saves raw PE to disk)
3. Scylla → IAT Autosearch + Get Imports → Fix Dump
4. Output: a valid PE with reconstructed import table
```

---

## WinDbg — Kernel & Crash Analysis

### Crash Dump Analysis
```
.reload /f                      ; force symbol reload
!analyze -v                     ; auto-analyze crash (most useful first step)
k                               ; stack trace at crash site
lm m <pattern>                  ; list modules matching pattern
!address <addr>                 ; describe memory region (heap/stack/image)
dt nt!_EPROCESS <addr>          ; dump structure at address
```

### Kernel Debugging Setup (VM)
```
// On guest VM (elevated cmd):
bcdedit /debug on
bcdedit /dbgsettings net hostip:192.168.1.X port:50000 key:1.2.3.4

// On host WinDbg:
File → Attach to Kernel → Net → Port 50000, Key 1.2.3.4
```

---

## Process Monitor / API Monitor

### ProcMon Filters for Injection Analysis
```
Filter rules (all "Include"):
  Process Name → is → target.exe
  Operation   → contains → File | Registry | Network | Process

Useful columns to enable:
  Path, Result, Detail, Duration
```

### API Monitor
- Attach to process → select API groups (File I/O, Crypto, Network)
- Records every call with arguments and return values
- Export to XML for offline analysis
- Use "Break on Call" to pause at specific API invocations

---

## rr — Record and Replay (Linux)

```bash
# Record an execution
rr record ./target --args

# Replay (deterministic — same execution every time)
rr replay

# Inside GDB after replay attach:
reverse-continue          # run backward to previous event
reverse-stepi             # step backward one instruction
watch -l variable         # watchpoint fires going backward too

# Jump to specific event
rr replay -e 12345        # event numbers shown in recording output
```
rr is indispensable for non-deterministic bugs (race conditions, heap corruption)
that don't repro under a live debugger.

---

## Memory Forensics During Debugging

### Find a Value in Process Memory (x64dbg)
```
Memory Map → right-click region → Search → Constant / String / Regex
Ctrl+B → binary search across all mapped memory
```

### Watch a Structure Live
```
// In x64dbg Watches panel:
[rcx]:8          // qword at address in rcx
[rsp+0x20]:4     // dword at stack + 0x20
[[rax]+0x10]:8   // double dereference (pointer to pointer)
```

---

## Anti-Debug Bypass Reference

| Technique | Detection Method | Bypass |
|---|---|---|
| `IsDebuggerPresent` | PEB.BeingDebugged flag | ScyllaHide patches PEB |
| `NtQueryInformationProcess` | ProcessDebugPort | ScyllaHide hooks NtQIP |
| Timing checks (`RDTSC`) | Measures instruction latency under debugger | ScyllaHide or NOPing the check |
| Exception-based (`int 3` traps) | Checks if debugger handles exception | Pass exception to program: `Shift+F9` |
| Heap flags | PEB.NtGlobalFlag / heap header flags | ScyllaHide patches heap flags |
| Parent process check | `CreateToolhelp32Snapshot` to find parent | Spoof parent PID at creation |
