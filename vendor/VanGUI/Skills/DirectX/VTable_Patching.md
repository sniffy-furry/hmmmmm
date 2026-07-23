# VTable Patching (COM & C++ Virtual Dispatch)

## What VTable Patching Is
Every polymorphic C++ object and every COM interface starts with a vtable pointer.
Patching an entry in that vtable redirects virtual function calls without modifying
the original function's code. No trampoline is created — call the original through
the saved pointer.

---

## VTable Layout in Memory (x64)

```
Object instance:
  [0x00]  vtable_ptr  ──────►  vtable array:
  [0x08]  field_1               [0]  virtual_func_0
  [0x10]  field_2               [1]  virtual_func_1
  ...                           [2]  virtual_func_2
                                [3]  your_detour   ← after patch
```

The vtable is shared among all instances of the same class.
Patching it affects every instance, everywhere in the process.

---

## Finding the vtable at Runtime

### From a Known Instance
```cpp
// If you have an object pointer `obj`:
void** GetVTable(void* obj) {
    return *reinterpret_cast<void***>(obj);
}

// Get the address of a specific entry:
void* GetVTableEntry(void* obj, int index) {
    return GetVTable(obj)[index];
}
```

### From a Symbol (if PDB available or debug build)
```cpp
// IDA / Ghidra: search for "vtable for ClassName" in the symbol tree
// At runtime: GetProcAddress won't work for vtables — use offset from module base
uintptr_t base    = (uintptr_t)GetModuleHandleW(L"target.exe");
void**    vtable  = reinterpret_cast<void**>(base + 0x1234560);  // from RE
```

### From TypeInfo (RTTI, MSVC)
```cpp
// If RTTI is present, vtables have a type_info pointer at vtable[-1]
struct VTableMeta {
    void* rtti;  // __RTTICompleteObjectLocator*
    // vtable entries follow
};
// Scan for specific class names via the demangled type_info::name()
```

---

## Patching a Single Entry

```cpp
bool PatchVTableEntry(void** vtable, int index,
                       void* newFunc, void** oldFunc) {
    void** entry = &vtable[index];

    DWORD oldProtect;
    if (!VirtualProtect(entry, sizeof(void*),
                         PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    if (oldFunc) *oldFunc = *entry;
    *entry = newFunc;

    VirtualProtect(entry, sizeof(void*), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof(void*));
    return true;
}
```

---

## COM Interface Patching Example

Hook `IDirect3DDevice9::EndScene` (DX9 overlay entry point):

```cpp
// IDirect3DDevice9 vtable indices (0-indexed from IUnknown):
// [0]  QueryInterface
// [1]  AddRef
// [2]  Release
// --- D3D9 methods ---
// [42] EndScene   ← hook for overlay

typedef HRESULT (WINAPI* PFN_EndScene)(IDirect3DDevice9*);
PFN_EndScene oEndScene = nullptr;

HRESULT WINAPI hk_EndScene(IDirect3DDevice9* pDev) {
    RenderOverlay(pDev);
    return oEndScene(pDev);
}

void HookD3D9EndScene(IDirect3DDevice9* pDevice) {
    void** vtable = GetVTable(pDevice);
    PatchVTableEntry(vtable, 42, (void*)hk_EndScene, (void**)&oEndScene);
}
```

---

## Patching a Specific Instance Only

If you only want to intercept calls from one particular object (not all instances),
give that object its own private vtable copy:

```cpp
struct PerInstanceHook {
    void**  originalVTable;
    void*   privateVTable[MAX_VTABLE_ENTRIES];
    void*   targetObj;

    void Install(void* obj, int index, void* detour) {
        targetObj      = obj;
        void** shared  = GetVTable(obj);
        originalVTable = shared;

        // Copy the shared vtable into private storage
        memcpy(privateVTable, shared,
               MAX_VTABLE_ENTRIES * sizeof(void*));

        // Patch private copy
        privateVTable[index] = detour;

        // Point object's vtable pointer at private copy
        DWORD old;
        VirtualProtect(&(*(void***)obj), sizeof(void*),
                        PAGE_EXECUTE_READWRITE, &old);
        *(void***)obj = privateVTable;
        VirtualProtect(&(*(void***)obj), sizeof(void*), old, &old);
    }

    void Restore() {
        DWORD old;
        VirtualProtect(&(*(void***)targetObj), sizeof(void*),
                        PAGE_EXECUTE_READWRITE, &old);
        *(void***)targetObj = originalVTable;
        VirtualProtect(&(*(void***)targetObj), sizeof(void*), old, &old);
    }
};
```

---

## Determining vtable Indices Without Symbols

### Method 1: Count Header Declarations
```cpp
// In the header (IUnknown = 0,1,2; then interface methods in order):
class IMyInterface : public IUnknown {  // starts at index 3
    virtual HRESULT Method_A() = 0;     // index 3
    virtual HRESULT Method_B() = 0;     // index 4
    virtual void    Method_C() = 0;     // index 5
};
```

### Method 2: Print vtable at Runtime
```cpp
void DumpVTable(void* obj, int count) {
    void** vtbl = GetVTable(obj);
    char sym[256];
    for (int i = 0; i < count; i++) {
        // On Windows with PDBs loaded: SymFromAddr
        printf("[%2d]  %p\n", i, vtbl[i]);
    }
}
```

### Method 3: Ghidra / IDA
- Find the vtable in the data segment
- Look at the cross-reference to the constructor (`lea rax, [vtable]`)
- Count entries from the top

---

## Pitfalls

| Issue | Cause | Fix |
|---|---|---|
| AV on call through detour | Wrong vtable index | Verify index via DumpVTable |
| Crash after object deleted | Detour called on freed memory | Track object lifetime |
| Other instances unaffected | Per-instance vtable (unlikely but possible) | Check if vtable pointer differs per instance |
| Anti-tamper reverts vtable | Integrity check re-reads vtable from disk | Use a background thread to re-patch |
