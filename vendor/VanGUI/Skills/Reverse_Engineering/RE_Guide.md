# Reverse Engineering (Authorized Targets Only)

## Scope
All guidance applies only to software you **own**, have **written authorization** to analyze,
or that falls under applicable exemptions (research, interoperability, accessibility).

---

## Tool Selection

| Situation | Tool | Why |
|---|---|---|
| Static analysis, first pass | Ghidra (free) or IDA Pro | Full decompilation, cross-references |
| Quick function signatures | Binary Ninja | Cleaner IL, scriptable |
| Dynamic tracing, Windows | x64dbg + ScyllaHide | Open source, solid plugin ecosystem |
| Dynamic tracing, Linux | GDB + pwndbg or rr | Record-replay with rr for non-determinism |
| Syscall tracing | Process Monitor / strace | Lower noise than a full debugger |
| Protocol/network RE | Wireshark + Frida | Intercept before encryption in-process |
| Automated deobfuscation | Frida + custom scripts | Runtime values defeat static obfuscation |
| Binary diffing (patch analysis) | BinDiff / Diaphora | Identify what changed between versions |

---

## Methodology

1. **Triage** — file type, packer/protector detection (`Detect-It-Easy`, `pestudio`),
   imports/exports, strings, entropy histogram. Never run unknown binaries on
   your host; use a snapshot VM or sandbox.

2. **Static first** — map the call graph, identify interesting string xrefs,
   locate initialization routines (`DllMain`, `WinMain`, TLS callbacks).

3. **Annotate continuously** — rename every function as you understand it.
   `sub_140012345` → `parse_config_json`.

4. **Identify data structures** — find allocations, trace pointer arithmetic,
   reconstruct structs. In Ghidra: Edit → Data Type Manager.

5. **Dynamic validation** — attach a debugger to an isolated VM snapshot.
   Set breakpoints on functions identified statically.

6. **Frida for scripting** — when you need to call arbitrary functions at runtime
   or hook managed code.

---

## Key Disassembly Patterns

```asm
; Typical vtable dispatch (x64, MSVC)
mov  rax, [rcx]          ; load vtable pointer from `this`
call qword ptr [rax+18h] ; call virtual function at index 3 (0x18 / 8)

; Stack frame with shadow space (x64 Windows calling convention)
sub  rsp, 28h            ; 0x20 = 32 bytes shadow + 8-byte alignment
...
add  rsp, 28h
ret

; Inlined memcpy pattern — recognize, don't chase
rep movsb
```

## C++ Name Mangling
- MSVC mangled names start with `?`
- Use `undname.exe` (Windows SDK) or `c++filt` (GCC/Clang) to demangle
- Ghidra and IDA demangle automatically in the Symbol Tree
- Always demangle before reading an import table

---

## Ethics & Authorization Checklist
- [ ] You own the software, OR
- [ ] You have written authorization from the owner, OR
- [ ] The work falls under an applicable legal exemption in your jurisdiction
      (e.g., DMCA §1201(f) interoperability, security research safe harbor)
