#define VANGUI_ENABLE_SIGNALS
#include "misc/vangui_signals.h"
#include <cstdio>
using namespace VanGui;
static int g_fail=0;
#define CHECK(c,m) do{ if(!(c)){printf("  FAIL: %s\n",m);++g_fail;} else printf("  ok  : %s\n",m);}while(0)
int main(){
    printf("[1] emit calls connected slots\n");
    {
        VanSignal<int> sig;
        int sum=0;
        auto c1 = sig.connect([&](int v){ sum+=v; });
        auto c2 = sig.connect([&](int v){ sum+=v*10; });
        sig.emit(3);
        CHECK(sum==33, "both slots fired (3 + 30)");
        CHECK(sig.slot_count()==2, "two live slots");
    }
    printf("[2] RAII disconnect on scope exit\n");
    {
        VanSignal<> sig;
        int hits=0;
        { auto c = sig.connect([&]{ ++hits; }); sig.emit(); CHECK(hits==1,"fires while in scope"); }
        sig.emit();
        CHECK(hits==1, "no fire after connection destroyed");
        CHECK(sig.slot_count()==0, "slot compacted away");
    }
    printf("[3] explicit disconnect()\n");
    {
        VanSignal<int> sig; int hits=0;
        auto c = sig.connect([&](int){ ++hits; });
        c.disconnect();
        sig.emit(1);
        CHECK(hits==0, "disconnected slot does not fire");
        CHECK(!c.connected(), "connection reports disconnected");
    }
    printf("[4] connection outliving the signal is safe\n");
    {
        VanConnection c;
        { VanSignal<> sig; c = sig.connect([]{}); }   // signal dies first
        c.disconnect();                                // must not crash
        CHECK(true, "no crash disconnecting after signal destroyed");
    }
    printf("[5] release() keeps slot, drops handle\n");
    {
        VanSignal<int> sig; int hits=0;
        { auto c = sig.connect([&](int){ ++hits; }); c.release(); }
        sig.emit(5);
        CHECK(hits==1, "released slot still fires");
    }
    printf("[6] disconnect_all\n");
    {
        VanSignal<> sig; int hits=0;
        auto a=sig.connect([&]{++hits;}); auto b=sig.connect([&]{++hits;});
        sig.disconnect_all(); sig.emit();
        CHECK(hits==0,"no slots after disconnect_all");
        a.release(); b.release();
    }
    printf("\n%s (%d failure%s)\n", g_fail?"TESTS FAILED":"ALL TESTS PASSED", g_fail, g_fail==1?"":"s");
    return g_fail?1:0;
}
