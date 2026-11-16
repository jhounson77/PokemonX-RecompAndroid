#include "domain/lift/control_flow.h"
#include "domain/lift/emit.h"
#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include <cassert>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace hexlith::domain;
std::vector<std::uint8_t> Code(bool thumb, bool call=false) {
    std::vector<std::uint8_t> bytes(256);
    auto put=[&](unsigned at,unsigned value,unsigned size){for(unsigned i=0;i<size;++i)bytes[at+i]=value>>(8*i);};
    if(thumb){unsigned words[]={0x2000,0x3001,0x6010,0x2805,0xd1fb,0x4770};for(unsigned i=0;i<6;++i)put(i*2,words[i],2);}
    else{unsigned words[]={0xe2800001,0xe5820000,0xe2911001,0xe3510005,0x1afffffa,0xe12fff1e};for(unsigned i=0;i<6;++i)put(i*4,words[i],4);}
    if(call){put(4,0xeb00000d,4);put(0x40,0xe2877001,4);put(0x44,0xe12fff1e,4);}
    return bytes;
}
int main(int argc,char**argv) {
    assert(argc==3);
    const bool generate=std::string_view(argv[1])=="generate";
    std::filesystem::path root=argv[2];
    unsigned comparisons=0;
    for(unsigned variant=0;variant<5;++variant){
        const bool thumb=variant==2, module=variant!=0, relocated=variant==3;
        auto bytes=Code(thumb,variant==4);
        const auto isa=thumb?arm::InstructionSet::Thumb:arm::InstructionSet::Arm;
        std::vector<std::uint32_t> resumes=thumb?std::vector<std::uint32_t>{0x1002,0x1004,0x1006,0x1008,0x1003,0x1300,0x1002}:std::vector<std::uint32_t>{0x1004,0x1008,0x100c,0x1010,0x1009,0x1300,0x1008};
        lift::EmittedFunction emitted[2];
        for(unsigned stage=0;stage<2;++stage){
            lift::FlowContext flow;
            if(stage)flow.resume_points=resumes;
            auto function=lift::BuildFunction({bytes,0x1000},0x1000,isa,flow);
            assert(function.InstructionCount()==6);
            lift::EmitContext context;context.module=module;
            if(relocated)context.relocated=[](auto pc){return pc==0x1004;};
            emitted[stage]=lift::EmitFunction(function,"fixture",context);
            if(stage){
                for(auto pc:emitted[0].entries)assert(std::ranges::find(emitted[1].entries,pc)!=emitted[1].entries.end());
                for(auto pc:resumes)if(pc% (thumb?2:4)==0 && pc<0x1100)assert(std::ranges::find(emitted[1].entries,pc)!=emitted[1].entries.end());
                assert(std::ranges::find(emitted[1].entries,thumb?0x1003:0x1009)==emitted[1].entries.end());
                assert(std::ranges::find(emitted[1].entries,0x1300)==emitted[1].entries.end());
            }
            if(generate){
                std::ofstream out(root/(std::to_string(variant)+"-"+std::to_string(stage)+".cpp"));
                out<<"#include \"domain/lift/runtime.h\"\nusing namespace hexlith::domain; namespace runtime=lift::runtime; namespace primitives=arm::primitives;\n"<<emitted[stage].text;
                out<<"extern \"C\" __attribute__((visibility(\"default\"))) void reg(void* p){ auto& table=*static_cast<runtime::EntryTable*>(p);\n";
                for(auto pc:emitted[stage].entries)out<<"table.Add("<<pc<<","<<thumb<<",&fixture);\n";
                out<<"}\n";
            }
        }
        if(generate)continue;
        void* handles[2]{};lift::runtime::EntryTable tables[2];
        for(unsigned i=0;i<2;++i){handles[i]=dlopen((root/(std::to_string(variant)+"-"+std::to_string(i)+".so")).c_str(),RTLD_NOW|RTLD_LOCAL);if(!handles[i]){std::cerr<<dlerror();return 2;}auto reg=reinterpret_cast<void(*)(void*)>(dlsym(handles[i],"reg"));assert(reg);reg(&tables[i]);}
        std::vector<unsigned> starts=thumb?std::vector<unsigned>{0x1000,0x1002,0x1004,0x1006,0x1008}:std::vector<unsigned>{0x1000,0x1004,0x1008,0x100c,0x1010};
        const unsigned base=module?variant*0x4000:0;
        for(auto entry:starts)for(bool fault:{false,true}){
            memory::ProcessMemory mem[3];lift::runtime::EntryTable empty;lift::runtime::LoadedModules loaded[2];
            for(unsigned i=0;i<3;++i){assert(mem[i].Map(base+0x1000,0x1000,memory::Backing::Allocate(0x1000),0,memory::Access::ReadWrite));for(unsigned j=0;j<bytes.size();++j)assert(mem[i].Write8(base+0x1000+j,bytes[j]));if(thumb)assert(mem[i].Write16(base+0x1f00,0xdf09));else assert(mem[i].Write32(base+0x1f00,0xef000009));if(relocated)assert(mem[i].Write32(base+0x1004,0xe2877003));}
            for(unsigned i=0;i<2;++i)if(module){loaded[i].Offer("fixture","digest",tables[i]);assert(loaded[i].Loaded("fixture",base,0x1000,256,"digest")==lift::runtime::LoadedModules::Outcome::Lifted);}
            lift::Engine before(module?empty:tables[0],module?&loaded[0]:nullptr),after(module?empty:tables[1],module?&loaded[1]:nullptr);
            for(unsigned limit=0;limit<=100;++limit){
                arm::CpuState states[3];
                for(unsigned i=0;i<3;++i){states[i].thumb=thumb;states[i].registers[15]=base+entry;states[i].registers[14]=base+0x1f00+(thumb?1:0);states[i].registers[2]=base+(fault?0x3000:0x1800);states[i].registers[1]=limit%6;states[i].zero=limit%2;assert(mem[i].Write32(base+0x1800,0));}
                auto old_count=before.Counted().interpreted,new_count=after.Counted().interpreted;
                std::uint64_t counts[3]{};auto expected=arm::Run(states[0],mem[0],limit,counts[0]);
                auto old_stop=before.Run(states[1],mem[1],limit,counts[1]);auto new_stop=after.Run(states[2],mem[2],limit,counts[2]);
                for(unsigned i=1;i<3;++i){auto stop=i==1?old_stop:new_stop;assert(stop.reason==expected.reason&&stop.detail==expected.detail&&counts[i]==counts[0]);assert(states[i].registers==states[0].registers&&arm::PackCpsr(states[i])==arm::PackCpsr(states[0]));assert(states[i].float_registers==states[0].float_registers&&states[i].fpscr==states[0].fpscr);assert(states[i].exclusive_open==states[0].exclusive_open&&states[i].exclusive_address==states[0].exclusive_address);for(unsigned at=base+0x1000;at<base+0x2000;at+=4)assert(mem[i].Read32(at)==mem[0].Read32(at));++comparisons;}
                if(variant==0&&entry==0x1008&&limit==3&&!fault){assert(before.Counted().interpreted-old_count==3);assert(after.Counted().interpreted-new_count==0);}
            }
        }
        for(auto h:handles)dlclose(h);
    }
    if(!generate){
        // A Thumb BL is one four-byte instruction: its second halfword must not
        // become an entry merely because it was requested as a continuation.
        std::vector<std::uint8_t> code{0x00,0xf0,0x0e,0xf8,0x70,0x47};
        std::vector<unsigned> points{0x1202};lift::FlowContext context;context.resume_points=points;
        auto f=lift::BuildFunction({code,0x1200},0x1200,arm::InstructionSet::Thumb,context);assert(!f.BlockAt(0x1202));
        std::cout<<"PASS "<<comparisons<<" old/new vs interpreter comparisons: ARM/Thumb main/modules, interior entry, limits0..100, flags/stores, direct call/return, SVC/faults, relocated word patch, retained entries, ignored invalid/unreached/halfword resumes; targeted fallback3->0\n";
    }
}
