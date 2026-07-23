// Standalone runtime test for vangui_anim (Phase 0 verification).
// Stubs only the link symbols the substrate needs; uses the test dt override
// so VanGui::GetIO() is never called at runtime.
#include "vangui.h"
#include "misc/vangui_anim.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>

// --- link stubs --------------------------------------------------------------
struct VanGuiContext {};
static VanGuiContext g_ctx;
namespace VanGui {
    VanGuiContext* GetCurrentContext() { return &g_ctx; }
    void* MemAlloc(size_t size) { return ::malloc(size); }
    void  MemFree(void* ptr)    { ::free(ptr); }
    // Never actually invoked: the test forces dt via SetDeltaTimeOverrideForTesting,
    // so the override path is taken before GetIO() would be reached. Backed by raw
    // storage to avoid pulling in VanGuiIO's constructor (defined in vangui.cpp).
    alignas(VanGuiIO) static unsigned char g_io_storage[sizeof(VanGuiIO)];
    VanGuiIO& GetIO() { return *reinterpret_cast<VanGuiIO*>(g_io_storage); }
}

int  VanGuiStorage::GetInt(VanGuiID key, int default_val) const {
    for (int i = 0; i < Data.size(); ++i) if (Data[i].key == key) return Data[i].val_i;
    return default_val;
}
void VanGuiStorage::SetInt(VanGuiID key, int val) {
    for (int i = 0; i < Data.size(); ++i) if (Data[i].key == key) { Data[i].val_i = val; return; }
    Data.push_back(VanGuiStoragePair(key, val));
}

// --- tiny assert harness -----------------------------------------------------
static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("  FAIL: %s\n", msg); ++g_fail; } \
                              else { printf("  ok  : %s\n", msg); } } while (0)
static bool approx(float a, float b, float eps = 1e-3f) { return fabsf(a - b) <= eps; }

using namespace VanGui::Anim;

int main()
{
    SetDeltaTimeOverrideForTesting(0.1f);   // 100 ms per frame, deterministic

    printf("[1] Easing endpoints\n");
    for (int f = 0; f < VanEasing_COUNT; ++f) {
        VanEasing e = (VanEasing)f;
        CHECK(approx(Ease(e, 0.0f), 0.0f), "Ease(.,0)==0");
        CHECK(approx(Ease(e, 1.0f), 1.0f), "Ease(.,1)==1");
    }

    printf("[2] AnimFloat linear convergence (Duration=1.0)\n");
    {
        const VanGuiID id = 1001u;
        VanAnimParams p; p.Duration = 1.0f; p.Easing = VanEasing_Linear;
        NewFrameUpdate(); float v = AnimFloat(id, 0.0f, p);   // seed at 0
        CHECK(approx(v, 0.0f), "seed returns initial value (0)");
        float mid = 0.0f;
        for (int i = 0; i < 10; ++i) { NewFrameUpdate(); mid = AnimFloat(id, 10.0f, p); if (i == 4) break; }
        // after 5 frames * 0.1s / 1.0s = t=0.5 -> linear -> 5.0
        CHECK(approx(mid, 5.0f, 0.2f), "halfway ~= 5.0 (linear)");
        for (int i = 0; i < 12; ++i) { NewFrameUpdate(); v = AnimFloat(id, 10.0f, p); }
        CHECK(approx(v, 10.0f), "reaches target 10.0");
        NewFrameUpdate(); (void)AnimFloat(id, 10.0f, p);
        NewFrameUpdate();
        CHECK(IsAnimating() == false, "IsAnimating() false after settle");
    }

    printf("[3] AnimBool stays within [0,1] and reaches 1\n");
    {
        const VanGuiID id = 2002u;
        VanAnimParams p; p.Duration = 0.5f; p.Easing = VanEasing_CubicOut;
        NewFrameUpdate(); float a = AnimBool(id, false, p);   // seed closed
        bool in_range = true;
        for (int i = 0; i < 12; ++i) { NewFrameUpdate(); a = AnimBool(id, true, p); if (a < 0.0f || a > 1.0001f) in_range = false; }
        CHECK(in_range, "AnimBool output stayed within [0,1]");
        CHECK(approx(a, 1.0f), "AnimBool reaches 1.0 when open");
    }

    printf("[4] SpringFloat settles to target\n");
    {
        const VanGuiID id = 3003u;
        VanSpringParams sp;  // defaults: stiffness 170, damping 26
        NewFrameUpdate(); float v = SpringFloat(id, 0.0f, sp);   // seed 0
        for (int i = 0; i < 200; ++i) { NewFrameUpdate(); v = SpringFloat(id, 1.0f, sp); }
        CHECK(approx(v, 1.0f, 1e-2f), "spring settles near target 1.0");
    }

    printf("[5] Eviction frees untouched slots\n");
    {
        const VanGuiID id = 4004u;
        SetEvictionFrames(3);
        VanAnimParams p; p.Duration = 1.0f; p.Easing = VanEasing_Linear;
        NewFrameUpdate(); (void)AnimFloat(id, 0.0f, p);          // seed at 0
        NewFrameUpdate(); float partial = AnimFloat(id, 100.0f, p); // begin moving toward 100
        CHECK(partial > 0.0f && partial < 100.0f, "slot is mid-animation before idle");
        for (int i = 0; i < 6; ++i) NewFrameUpdate();            // idle > eviction window
        NewFrameUpdate(); float after = AnimFloat(id, 100.0f, p); // re-seen after eviction
        CHECK(approx(after, 100.0f), "evicted+reseeded id snaps to current target");
        SetEvictionFrames(60);
    }

    printf("\n%s (%d failure%s)\n", g_fail ? "TESTS FAILED" : "ALL TESTS PASSED",
           g_fail, g_fail == 1 ? "" : "s");
    return g_fail ? 1 : 0;
}
