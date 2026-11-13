#include "application/boot.h"
#include "adapters/filesystem/png_file.h"
#ifdef PX_HOST_GPU
#include "adapters/vulkan/vulkan_backend.h"
#endif
#include "application/live_display.h"
#include "application/gpu_render.h"
#include "adapters/filesystem/file_rom_source.h"
#include "ports/screen_sink.h"
#include "ports/input_source.h"
#include <iostream>
#include <algorithm>
#include <chrono>
using namespace hexlith;
struct Sink: ports::ScreenSink, ports::InputSource {
    std::uint64_t frames = 0;std::string output;std::uint32_t last_width=0;
    core::Status Present(std::uint64_t f, const domain::gpu::Picture& top, const domain::gpu::Picture&) override { frames=f;last_width=top.width; if(f%100==0) std::cout<<"frame "<<f<<" picture "<<top.width<<"x"<<top.height<<" colored "<<std::count_if(top.pixels.begin(),top.pixels.end(),[](auto p){return p.red||p.green||p.blue;})<<std::endl;if(!output.empty()&&(f==100||f==200||f==300||f==350)){auto saved=adapters::filesystem::WritePng(output+"-"+std::to_string(f)+".png",top);if(!saved)return saved;}return {}; }
    bool StopRequested() const override { return false; }
    std::string Describe() const override { return "headless validation"; }
    domain::services::InputState StateAt(std::uint64_t f) override { return {.buttons=((f>=100&&f<104)||(f>=200&&f<204))?1u:0u}; }
};
int main(int argc,char**argv) {
 if(argc<2)return 2;
 adapters::filesystem::FileRomSource rom(argv[1]); auto bytes=rom.Contents();if(!bytes){std::cerr<<bytes.Failure().Message();return 1;}
 auto loaded=application::BootedTitle::FromNcch(bytes.Value()); if(!loaded){std::cerr<<loaded.Failure().Message();return 1;}
 auto&title=*loaded.Value(); Sink sink;title.UseInput(sink);
 auto memory=application::GpuMemoryOf(title);
#ifdef PX_HOST_GPU
 std::unique_ptr<adapters::vulkan::VulkanContext> context;
#endif
 std::unique_ptr<ports::GpuBackend> gpu;
#ifdef PX_HOST_GPU
 if(argc>2&&std::string(argv[2]).starts_with("vulkan")){auto made=adapters::vulkan::VulkanContext::Create({.validation=false});if(!made){std::cerr<<made.Failure().Message();return 1;}context=std::move(made.Value());int scale=std::string(argv[2])=="vulkan2"?2:1;auto built=adapters::vulkan::VulkanGpuBackend::Create(*context,memory,domain::gpu::Sampling::Hardware,adapters::vulkan::VertexShading::GpuWherePossible,adapters::vulkan::FragmentShading::Generic,scale);if(!built){std::cerr<<built.Failure().Message();return 1;}gpu=std::move(built.Value());}
#endif
 if(!gpu)gpu=std::make_unique<application::ReferenceGpuBackend>(memory,domain::gpu::Sampling::Hardware);
 if(argc>3)sink.output=argv[3];
 application::LiveDisplay display(memory,*gpu,sink);
 if(argc>2)display.Watch(title.Gpu());
 title.Gpu().OnVerticalBlank([&](std::uint64_t f){if(f>=350)title.Kernel().RequestStop();});
 auto start=std::chrono::steady_clock::now();auto stop=title.Kernel().Run(2000000000);
 std::cout<<"stop "<<static_cast<int>(stop.reason)<<" frame "<<title.Gpu().Frames()<<" pc "<<std::hex<<stop.address<<" service "<<stop.service<<" detail "<<stop.detail<<std::dec<<" executed "<<title.Kernel().Executed()<<" seconds "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" code "<<title.CodeDigest()<<std::endl;
 auto finished=display.Finish();if(!finished){std::cerr<<finished.Failure().Message();return 1;}
 if(argc>2&&std::string(argv[2])=="vulkan2"&&sink.last_width!=800)return 3;
 return title.Gpu().Frames()>=227?0:1;
}
