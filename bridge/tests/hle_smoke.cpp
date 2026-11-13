#include "arm_native.h"
#include "program.h"
#include "common/file_util.h"
#include "common/logging/backend.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/applets/default_applets.h"
#include "core/frontend/image_interface.h"
#include "core/hle/service/service.h"
#include <iostream>
class Headless final: public Frontend::EmuWindow { void PollEvents() override {} };
static unsigned calls=0;
static hexlith::domain::lift::runtime::Pc Native(hexlith::domain::lift::runtime::Context& ctx, std::uint32_t pc){
 ++calls;--ctx.budget;
 if(calls==1){
  std::uint32_t word=0;
  if(!ctx.memory->ReadInto(0x100200,word)||word!=0x4e5f5a41u)throw std::runtime_error("CPU external RAM read failed");
  if(!ctx.memory->Write32(0x103000,0x12345678)||ctx.memory->Read32(0x103000)!=0x12345678)throw std::runtime_error("CPU external RAM write failed");
  if(!ctx.memory->Write32(0x103fff,0xaabbccdd)||ctx.memory->Read32(0x103fff)!=0xaabbccdd)throw std::runtime_error("CPU crossing-page access failed");
  auto& cpu=dynamic_cast<PokemonX::ARMNative&>(Core::System::GetInstance().GetRunningCore());
  Core::ARM_Interface::ThreadContext saved,changed;cpu.SaveContext(saved);
  auto rw=cpu.GetCP15Register(CP15_THREAD_UPRW),ro=cpu.GetCP15Register(CP15_THREAD_URO);
  cpu.SetReg(7,0x1234);cpu.SetCPSR(0xa80a0030);cpu.SetVFPReg(31,0xdeadbeef);cpu.SetVFPSystemReg(VFP_FPSCR,0x04000000);
  cpu.SetCP15Register(CP15_THREAD_UPRW,123);cpu.SetCP15Register(CP15_THREAD_URO,456);cpu.SaveContext(changed);
  if(changed.cpu_registers[7]!=0x1234||changed.cpsr!=0xa80a0030||changed.fpu_registers[31]!=0xdeadbeef||changed.fpscr!=0x04000000||cpu.GetCP15Register(CP15_THREAD_UPRW)!=123||cpu.GetCP15Register(CP15_THREAD_URO)!=456)throw std::runtime_error("context round trip failed");
  cpu.LoadContext(saved);cpu.SetCP15Register(CP15_THREAD_UPRW,rw);cpu.SetCP15Register(CP15_THREAD_URO,ro);
 }

 if(pc==0x100000){ctx.cpu->registers[0]=0x100200;ctx.cpu->registers[1]=16;ctx.stop={hexlith::domain::arm::StopReason::SupervisorCall,0x3d};}
 else ctx.stop={hexlith::domain::arm::StopReason::SupervisorCall,3};
 ctx.cpu->registers[15]=pc+4;return hexlith::domain::lift::runtime::Stopped;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;
 FileUtil::SetUserPath(argv[2]);Common::Log::Initialize("native-hle-smoke.txt");Common::Log::Start();
 Common::Log::Filter filter;filter.ParseFilterString("*:Warning Debug.Emulated:Debug Kernel:Info");Common::Log::SetGlobalFilter(filter);
 Settings::values.graphics_api.SetValue(Settings::GraphicsAPI::Software);
 Settings::values.output_type.SetValue(AudioCore::SinkType::Null);Settings::values.input_type.SetValue(AudioCore::InputType::Null);
 Settings::values.use_cpu_jit=false;Settings::values.use_fastinterp=false;
 for(const auto& m:Service::service_module_map)Settings::values.lle_modules.emplace(m.name,false);
 hexlith::domain::lift::runtime::EntryTable entries;entries.Add(0x100000,false,Native);entries.Add(0x100004,false,Native);
 PokemonX::SetProgram(PokemonX::Program::FromEntries(std::move(entries)));
 auto& system=Core::System::GetInstance();Frontend::RegisterDefaultApplets(system);system.RegisterImageInterface(std::make_shared<Frontend::ImageInterface>());
 Headless window;auto status=system.Load(window,argv[1]);std::cout<<"load="<<static_cast<unsigned>(status)<<std::endl;
 if(status!=Core::System::ResultStatus::Success){Common::Log::Stop();return 3;}
 for(unsigned i=0;i<100&&calls<2;++i){status=system.RunLoop();if(status!=Core::System::ResultStatus::Success)break;}
 system.Shutdown();PokemonX::SetProgram({});Common::Log::Stop();
 std::cout<<"native_memory_context_checks=passed native_calls="<<calls<<" status="<<static_cast<unsigned>(status)<<std::endl;
 return calls==2?0:4;
}
