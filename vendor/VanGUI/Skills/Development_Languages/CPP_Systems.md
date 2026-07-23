# C++ — Systems-Level Best Practices

## Philosophy
Write for the optimizer and the reader simultaneously. Modern C++ (17/20) gives you
expressive tools without sacrificing ABI-level predictability. In hook/tool/game contexts
you often can't use exceptions or RTTI — design accordingly from the start.

---

## Project Configuration Baseline (CMake)

```cmake
cmake_minimum_required(VERSION 3.25)
project(InspectorGadget CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/GR- /EHsc /W4 /WX)
  # /GR- = no RTTI, /EHsc = structured exceptions only
endif()
```

---

## Memory Management in Injected / Embedded Contexts

- Avoid `std::vector`, `std::string` in `DllMain` or early hook callbacks — heap may not be initialized
- Use placement-new into static buffers for pre-initialization allocations
- Prefer `std::array` and stack allocation in hot hook paths — cache locality matters
- Use `HeapCreate` for a private heap at attach time to prevent host heap fragmentation

---

## Calling Conventions

| Convention | Who Cleans Stack | Register Args (x64 Win) | Notes |
|---|---|---|---|
| `__cdecl` | Caller | rcx, rdx, r8, r9 | Default C++ |
| `__stdcall` | Callee | (x86 only) | WinAPI |
| `__fastcall` | Callee | ecx, edx (x86) | Rare in modern code |
| `__thiscall` | Callee | rcx = this | C++ member functions |
| `__vectorcall` | Callee | rcx/xmm0-5 | SIMD-heavy math |

> On x64 Windows all conventions collapse to one; on x86 they diverge.
> If hooking 32-bit code from a 32-bit DLL, explicitly declare detour functions with the correct convention.

---

## RAII Hook Guard

```cpp
struct ScopedHook {
    void* target;
    explicit ScopedHook(void* t, void* detour, void** original)
        : target(t) {
        MH_CreateHook(t, detour, original);
        MH_EnableHook(t);
    }
    ~ScopedHook() { MH_DisableHook(target); MH_RemoveHook(target); }
};
```

---

## Thread-Safe Singleton Init

```cpp
// std::once_flag is lock-free on most platforms after first call
std::once_flag g_init_flag;
void EnsureInit() {
    std::call_once(g_init_flag, []{ /* one-time setup */ });
}
```

---

## Error Handling Without Exceptions

```cpp
template<typename T>
struct Result {
    T value{};
    const char* error = nullptr;   // nullptr = success
    explicit operator bool() const { return error == nullptr; }
};

Result<LPVOID> AllocExecutable(SIZE_T size) {
    auto p = VirtualAlloc(nullptr, size,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_EXECUTE_READWRITE);
    if (!p) return { {}, "VirtualAlloc failed" };
    return { p, nullptr };
}
```
