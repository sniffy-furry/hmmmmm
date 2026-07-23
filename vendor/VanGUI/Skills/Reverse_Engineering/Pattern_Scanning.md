# Pattern Scanning & Memory Searching

## Why Pattern Scanning
Function addresses change every build due to ASLR and recompilation.
Pattern scanning finds code by its bytes — a sequence of instruction bytes that
is stable across builds (the logic doesn't change even if the base address does).

---

## Building a Signature

1. Find the function in Ghidra/IDA
2. Look at the first N bytes of its prologue
3. Replace operands that encode addresses (absolute or relative jumps/calls, 
   RIP-relative memory references) with wildcards `??`
4. Leave stable opcode bytes as-is

```asm
; Example function prologue:
48 89 5C 24 08           ; mov [rsp+8], rbx           → keep (stack offsets stable)
57                        ; push rdi                    → keep
48 83 EC 30              ; sub rsp, 0x30               → keep
48 8B 05 ?? ?? ?? ??     ; mov rax, [rip+????]         → wildcard (RIP-rel)
48 85 C0                 ; test rax, rax               → keep
74 ??                    ; jz short ????               → wildcard (branch offset)
```

Pattern: `48 89 5C 24 08 57 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ??`

---

## Pattern Scanner Implementation

```cpp
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

struct PatternByte {
    uint8_t  value;
    bool     wildcard;
};

std::vector<PatternByte> ParsePattern(const char* pattern) {
    std::vector<PatternByte> result;
    const char* p = pattern;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        if (p[0] == '?' && (p[1] == '?' || p[1] == ' ' || !p[1])) {
            result.push_back({ 0, true });
            p += (p[1] == '?') ? 2 : 1;
        } else {
            uint8_t byte = (uint8_t)strtoul(p, const_cast<char**>(&p), 16);
            result.push_back({ byte, false });
        }
    }
    return result;
}

uintptr_t ScanRegion(uintptr_t start, size_t size,
                      const std::vector<PatternByte>& pattern) {
    const size_t patLen = pattern.size();
    const uint8_t* mem  = reinterpret_cast<const uint8_t*>(start);

    for (size_t i = 0; i + patLen <= size; i++) {
        bool match = true;
        for (size_t j = 0; j < patLen; j++) {
            if (!pattern[j].wildcard && mem[i + j] != pattern[j].value) {
                match = false;
                break;
            }
        }
        if (match) return start + i;
    }
    return 0;
}

// Scan the .text section of a module
uintptr_t FindPattern(HMODULE hMod, const char* patternStr) {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
    auto* nt  = reinterpret_cast<IMAGE_NT_HEADERS*>(
        (uint8_t*)hMod + dos->e_lfanew);

    auto  pattern = ParsePattern(patternStr);
    auto* sec     = IMAGE_FIRST_SECTION(nt);

    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        // Only scan executable sections
        if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;

        uintptr_t secStart = (uintptr_t)hMod + sec->VirtualAddress;
        size_t    secSize  = sec->Misc.VirtualSize;

        uintptr_t result = ScanRegion(secStart, secSize, pattern);
        if (result) return result;
    }
    return 0;
}
```

---

## Usage

```cpp
// Find the function
uintptr_t addr = FindPattern(
    GetModuleHandleW(L"target.exe"),
    "48 89 5C 24 08 57 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ??");

if (addr) {
    // Hook it with MinHook
    MH_CreateHook((void*)addr, &MyDetour, (void**)&oOriginal);
    MH_EnableHook((void*)addr);
}
```

---

## Resolving RIP-Relative Addresses

Many patterns point to code that references globals via RIP-relative addressing.
Resolve the reference from within the pattern:

```cpp
// After finding the pattern, extract the RIP-relative target at offset `instrOffset`
// instrOffset = offset from match to the 4-byte displacement within the instruction
// instrLen    = total length of the instruction containing the displacement

uintptr_t ResolveRIPRelative(uintptr_t matchAddr,
                               int instrOffset,
                               int instrLen) {
    int32_t disp = *reinterpret_cast<int32_t*>(matchAddr + instrOffset);
    return matchAddr + instrLen + disp;
    // RIP = address of next instruction = matchAddr + instrLen
}

// Example: `48 8B 05 [AA BB CC DD]` where bytes 3-6 are the displacement
// instrOffset=3, instrLen=7
uintptr_t globalPtr = ResolveRIPRelative(addr, 3, 7);
SomeStruct* pGlobal = *reinterpret_cast<SomeStruct**>(globalPtr);
```

---

## Scanning Across All Modules

```cpp
uintptr_t FindPatternInProcess(const char* patternStr) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    MODULEENTRY32W me{ sizeof(me) };

    if (Module32FirstW(snap, &me)) {
        do {
            uintptr_t r = FindPattern((HMODULE)me.modBaseAddr, patternStr);
            if (r) { CloseHandle(snap); return r; }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return 0;
}
```

---

## Unique String Reference Scanning

Sometimes it's easier to find a function by a unique string it references:

```cpp
// 1. Find the string in memory
uintptr_t FindString(HMODULE hMod, const char* str) {
    auto* dos  = (IMAGE_DOS_HEADER*)hMod;
    auto* nt   = (IMAGE_NT_HEADERS*)((uint8_t*)hMod + dos->e_lfanew);
    auto* sec  = IMAGE_FIRST_SECTION(nt);
    size_t len = strlen(str);

    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        uintptr_t start = (uintptr_t)hMod + sec->VirtualAddress;
        for (size_t j = 0; j + len <= sec->Misc.VirtualSize; j++) {
            if (memcmp((void*)(start + j), str, len) == 0)
                return start + j;
        }
    }
    return 0;
}

// 2. Find code that references that string (xref)
// 3. Navigate to the function containing that xref
```

---

## Performance Notes

- Scanning a 50MB module takes ~5ms on a modern CPU — acceptable at init time
- Cache results; never scan on the hot path (per-frame)
- Limit scan to `.text` section; avoid scanning compressed or encrypted sections
- For very large targets, parallelize with `std::async` per section
