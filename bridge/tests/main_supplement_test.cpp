#include "adapters/filesystem/directory_blob_sink.h"
#include "adapters/lifted/lifted_library.h"
#include "application/lift_emit.h"
#include "domain/analysis/tables.h"
#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace hexlith;
namespace arm=domain::arm;namespace lift=domain::lift;namespace memory=domain::memory;
std::vector<std::uint8_t> Code(){
 std::vector<std::uint8_t>b(256);auto put=[&](unsigned at,unsigned w){for(unsigned i=0;i<4;++i)b[at+i]=w>>(8*i);};
 put(0,0xe3a00007);put(4,0xe12fff1e);
 put(0x20,0xe92d4000);put(0x24,0xe2800001);put(0x28,0xe5820000);put(0x2c,0xeb000013);put(0x30,0xe8bd8000);
 put(0x80,0xe2811003);put(0x84,0xe12fff1e);return b;
}
int main(int argc,char**argv){
 assert(argc==3);std::filesystem::path root=argv[2];auto code=Code();
 namespace a=domain::analysis;
 std::vector<a::FunctionRange> original{{0x1000,8,arm::InstructionSet::Arm},{0x1080,8,arm::InstructionSet::Arm}};
 auto all=original;all.push_back({0x1020,20,arm::InstructionSet::Arm});
 auto subset=std::vector<a::FunctionRange>{original[0],all.back()};
 auto regions=a::RenderRegions({{0x1000,256,a::RegionKind::Code,arm::InstructionSet::Arm}});
 std::vector<std::uint32_t> resumes{0x1020,0x1024,0x1028,0x102c,0x1030};
 if(std::string_view(argv[1])=="generate"){
  adapters::filesystem::DirectoryBlobSink before(root/"before"),extra(root/"extra");assert(before.Prepare()&&extra.Prepare());
  auto old=application::EmitLiftedCode({.code=code,.base=0x1000,.functions=a::RenderFunctions(original),.regions=regions},before);assert(old);
  auto now=application::EmitLiftedCode({.code=code,.base=0x1000,.functions=a::RenderFunctions(subset),.regions=regions,.resume_points=resumes,.known_functions=a::RenderFunctions(all)},extra);assert(now&&old.Value().code_hash==now.Value().code_hash);
  std::ofstream(root/"hash.txt")<<old.Value().code_hash;
  std::ofstream(root/"bad.cpp")<<"extern \"C\" const char* hexlith_lifted_manifest(){return \"lifter=4 code="<<std::string(64,'0')<<" functions=0\";}\nextern \"C\" void hexlith_lifted_register(void*){}\n";
  return 0;
 }
 using Library=adapters::lifted::LiftedLibrary;
 std::ifstream h(root/"hash.txt");std::string hash;h>>hash;
 auto plain=Library::Open(root/"before.so",hash),merged=Library::Open(root/"before.so",hash),extra=Library::Open(root/"extra.so",hash);
 assert(plain&&merged&&extra);
 auto old1000=merged.Value()->Entries().Find(0x1000,false),old1080=merged.Value()->Entries().Find(0x1080,false);
 assert(!merged.Value()->Entries().Find(0x1024,false));assert(extra.Value()->Entries().Find(0x1000,false)!=old1000);
 auto count=merged.Value()->Entries().Size();assert(merged.Value()->AddMissingFrom(*extra.Value()));
 assert(merged.Value()->Entries().Size()>count);count=merged.Value()->Entries().Size();
 assert(merged.Value()->Entries().Find(0x1000,false)==old1000&&merged.Value()->Entries().Find(0x1080,false)==old1080);
 assert(merged.Value()->AddMissingFrom(*extra.Value())&&merged.Value()->Entries().Size()==count);
 assert(!Library::Open(root/"extra.so",std::string(64,'0')));
 auto wrong=Library::Open(root/"bad.so",std::string(64,'0'));assert(wrong);assert(!merged.Value()->AddMissingFrom(*wrong.Value()));assert(merged.Value()->Entries().Size()==count);
 unsigned comparisons=0;
 for(auto entry:{0x1000U,0x1020U,0x1024U,0x1028U,0x102cU})for(bool fault:{false,true}){
  memory::ProcessMemory mem[3];for(auto& m:mem){assert(m.Map(0x1000,0x1000,memory::Backing::Allocate(0x1000),0,memory::Access::ReadWrite));for(unsigned i=0;i<code.size();++i)assert(m.Write8(0x1000+i,code[i]));assert(m.Write32(0x1f00,0xef000009));}
  lift::Engine before(plain.Value()->Entries()),after(merged.Value()->Entries());
  for(unsigned limit=0;limit<=80;++limit){
   arm::CpuState states[3];for(unsigned i=0;i<3;++i){states[i].registers[15]=entry;states[i].registers[14]=0x1f00;states[i].registers[13]=entry>0x1020&&entry<0x1040?0x1dfc:0x1e00;states[i].registers[2]=fault?0x3000:0x1800;states[i].registers[0]=limit%7;states[i].carry=limit%2;assert(mem[i].Write32(0x1dfc,0x1f00)&&mem[i].Write32(0x1800,0));}
   std::uint64_t counts[3]{};auto expected=arm::Run(states[0],mem[0],limit,counts[0]);auto a0=before.Counted().interpreted,a1=after.Counted().interpreted;
   auto old=before.Run(states[1],mem[1],limit,counts[1]);auto now=after.Run(states[2],mem[2],limit,counts[2]);
   for(unsigned i=1;i<3;++i){auto stop=i==1?old:now;assert(stop.reason==expected.reason&&stop.detail==expected.detail&&counts[i]==counts[0]);assert(states[i].registers==states[0].registers&&arm::PackCpsr(states[i])==arm::PackCpsr(states[0]));assert(states[i].float_registers==states[0].float_registers&&states[i].fpscr==states[0].fpscr);for(unsigned at=0x1000;at<0x2000;at+=4)assert(mem[i].Read32(at)==mem[0].Read32(at));++comparisons;}
   if(entry==0x1024&&limit==1&&!fault){assert(before.Counted().interpreted-a0==1);assert(after.Counted().interpreted-a1==0);}
  }
 }
 std::cout<<"PASS "<<comparisons<<" main supplement comparisons: same-hash load, wrong-hash refusal, first-wins/idempotent merge, original hidden function dispatch, interior resumes, limits0..80, SVC/write faults and full state/page; targeted fallback1->0\n";
}
