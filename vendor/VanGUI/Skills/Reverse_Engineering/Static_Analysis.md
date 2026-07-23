# Static Analysis

## Goal
Recover logic, data structures, and control flow from a compiled binary without executing it.
Always work in a copy; never modify the original artifact.

---

## Triage Checklist (Before Opening a Disassembler)

```
1. file <target>           — ELF / PE / Mach-O, arch, bitness
2. pestudio / DIE          — packer, protector, compiler fingerprint, entropy histogram
3. strings -n 8 <target>  — URLs, error messages, registry keys, format strings
4. dumpbin /imports        — or readelf -d; maps the API surface instantly
5. signsrch / yara         — known crypto constants, compression magic bytes
```

High entropy sections → packed/encrypted → must unpack before analysis.
Low entropy + readable strings → start reading immediately.

---

## Ghidra Workflow

### Initial Setup
```
1. New project → Import file (auto-detects format + arch)
2. Analysis → Auto Analyze (all default analyzers ON)
   - Enable: Decompiler Parameter ID, Stack Analysis, Aggressive Instruction Finder
3. Wait for analysis to complete before navigating
```

### Finding Entry Points
- `WinMain` / `DllMain` — check Symbol Tree → Functions → search "main"
- TLS Callbacks — run before `DllMain`; check `.tls` section, often missed
- `DllEntryPoint` for injected DLLs

### Navigation Shortcuts
| Action | Shortcut |
|---|---|
| Go to address | `G` |
| Find references to symbol | `Ctrl+Shift+F` |
| Rename symbol | `L` |
| Retype variable | `Ctrl+L` |
| Open call graph | Right-click → Show Call Trees |
| Search strings | `Search → For Strings` |

### Struct Recovery
```
1. Find an allocation: HeapAlloc, malloc, operator new
2. Note size argument → that's your struct's byte count
3. Trace the returned pointer through the function
4. Observe field offsets: mov [rcx+0x18], rax → field at offset 0x18
5. Data Type Manager → New Structure → add fields at those offsets
6. Apply type to pointer: right-click → Data → chosen struct*
```

---

## IDA Pro Workflow

### Key Differences from Ghidra
- Faster initial analysis on complex binaries
- Better at recovering calling conventions automatically
- Hex-Rays decompiler (licensed separately) is generally cleaner output
- Use `idat64.exe` for headless scripting

### IDAPython Automation
```python
import idc, idautils, idaapi

# Rename all functions matching a string xref pattern
for func_ea in idautils.Functions():
    for xref in idautils.XrefsTo(func_ea):
        name = idc.get_func_name(xref.frm)
        # ... pattern matching logic
        idc.set_name(func_ea, "parsed_name", idc.SN_CHECK)

# Find all calls to a specific import
target = idc.get_name_ea_simple("CreateFileW")
for xref in idautils.XrefsTo(target):
    print(f"Called from: {hex(xref.frm)}")
```

---

## Reading Compiled Code Patterns

### Switch Tables
```asm
; x64 MSVC switch — indirect jump through table
movsx  eax, byte ptr [rdx]     ; load case value
cmp    eax, 7                   ; range check
ja     default_label
lea    rcx, [rip+jump_table]
movsxd rax, dword ptr [rcx+rax*4]
add    rax, rcx
jmp    rax                      ; indirect jump
```

### Virtual Function Calls
```asm
mov    rax, [rcx]               ; load vtable pointer (first qword of object)
mov    rax, [rax + 0x28]        ; load function pointer at vtable[5] (0x28/8=5)
call   rax
; rcx = this pointer throughout
```

### Inlined Functions (don't chase these)
```asm
rep movsb           ; inlined memcpy
pxor xmm0, xmm0    ; inlined memset to zero
```

---

## Frida for Dynamic-Assisted Static

```javascript
// Enumerate all modules + exports at runtime (defeats obfuscation)
Process.enumerateModules().forEach(m => {
    m.enumerateExports().forEach(e => {
        console.log(`${m.name}!${e.name} @ ${e.address}`);
    });
});

// Hook a function discovered statically to log its args
const target = ptr("0x" + (Module.getBaseAddress("target.exe").add(0x12ABC0)).toString(16));
Interceptor.attach(target, {
    onEnter(args) {
        console.log(`arg0=${args[0]}, arg1=${args[1].readUtf8String()}`);
    }
});
```
