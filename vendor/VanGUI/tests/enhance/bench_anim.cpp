#include "vangui.h"
#include "misc/vangui_anim.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
// link stubs (same as the unit test)
struct VanGuiContext {}; static VanGuiContext g_ctx;
namespace VanGui {
  VanGuiContext* GetCurrentContext(){ return &g_ctx; }
  void* MemAlloc(size_t s){ return malloc(s);} void MemFree(void* p){ free(p);}
  alignas(VanGuiIO) static unsigned char io_[sizeof(VanGuiIO)];
  VanGuiIO& GetIO(){ return *reinterpret_cast<VanGuiIO*>(io_);} }
int VanGuiStorage::GetInt(VanGuiID k,int d) const { for(int i=0;i<Data.size();++i) if(Data[i].key==k) return Data[i].val_i; return d;}
void VanGuiStorage::SetInt(VanGuiID k,int v){ for(int i=0;i<Data.size();++i) if(Data[i].key==k){Data[i].val_i=v;return;} Data.push_back(VanGuiStoragePair(k,v)); }
using namespace VanGui::Anim;
using clk = std::chrono::high_resolution_clock;
int main(){
    SetDeltaTimeOverrideForTesting(0.016f);
    // empty-pool early-out cost
    { auto t0=clk::now(); for(int i=0;i<100000;++i) NewFrameUpdate();
      auto t1=clk::now(); double ns=std::chrono::duration<double,std::nano>(t1-t0).count()/100000.0;
      printf("NewFrameUpdate() empty-pool: %.1f ns/call\n", ns); }
    // 256 active tweens, drive them continuously
    const int N=256;
    for(int i=0;i<N;++i){ NewFrameUpdate(); (void)AnimFloat((VanGuiID)(1000+i), 0.0f); }
    // start them moving toward changing targets
    auto t0=clk::now(); int frames=2000;
    for(int f=0; f<frames; ++f){ NewFrameUpdate(); for(int i=0;i<N;++i) (void)AnimFloat((VanGuiID)(1000+i), (float)((f+i)%2)); }
    auto t1=clk::now();
    double total_ns=std::chrono::duration<double,std::nano>(t1-t0).count();
    printf("256 active tweens: %.2f us / frame (update+256 AnimFloat)\n", (total_ns/frames)/1000.0);
    printf("IsAnimating while moving: %d (idle flag works)\n", (int)IsAnimating());
    return 0;
}
