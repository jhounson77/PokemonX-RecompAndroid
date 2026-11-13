#include "application/live_display.h"
#include "domain/gpu/gx_command.h"
#include <chrono>
#include <iostream>
using namespace hexlith;
struct Bench final:ports::ScreenSink,ports::GpuBackend {
 bool optimized=false;std::size_t copies=0;std::uint64_t checksum=0;
 domain::gpu::Picture top{.width=800,.height=480,.pixels=std::vector<domain::gpu::Color>(800*480,{23,71,113,255})};
 domain::gpu::Picture bottom{.width=640,.height=480,.pixels=std::vector<domain::gpu::Color>(640*480,{127,32,19,255})};
 unsigned RequestedScreens()override{return optimized?1:3;}
 core::Status Present(std::uint64_t,const domain::gpu::Picture&a,const domain::gpu::Picture&)override{checksum+=a.pixels.front().red;return {};}
 const domain::gpu::Picture* BorrowHighResolutionScreen(const domain::gpu::MemoryRange&r)override{return optimized?&(r.start==domain::gpu::GpuMemory::VramBase?top:bottom):nullptr;}
 std::optional<domain::gpu::Picture> HighResolutionScreen(const domain::gpu::MemoryRange&r)override{++copies;return r.start==domain::gpu::GpuMemory::VramBase?top:bottom;}
 bool StopRequested()const override{return false;}
 core::Status RunCommandList(std::span<const std::uint8_t>)override{return {};}
 ports::GpuBackendStatistics Statistics()const override{return {};}
 std::string Describe()const override{return "synthetic cached framebuffers";}
};
int main(){domain::gpu::GpuMemory memory;for(bool optimized:{false,true}){Bench bench;bench.optimized=optimized;application::LiveDisplay display(memory,bench,bench);display.Framebuffer(1,0,{0,0x1f000000,0,960,0,0,0});display.Framebuffer(1,1,{0,0x1f080000,0,960,0,0,0});auto began=std::chrono::steady_clock::now();for(unsigned i=1;i<=300;++i){auto result=display.Command(i,{static_cast<std::uint32_t>(domain::gpu::GxCommandId::ProcessCommandList),domain::gpu::GpuMemory::OldLinearHeapBase,0,0,0,0,0,0});if(!result)return 1;}auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();std::cout<<(optimized?"Selected/borrowed":"Both/owning")<<": "<<ms<<" ms, "<<bench.copies<<" pixel-vector copies; checksum "<<bench.checksum<<"\n";}std::cout<<"300 synthetic 2x frames, preparation only; not game/device FPS.\n";}
