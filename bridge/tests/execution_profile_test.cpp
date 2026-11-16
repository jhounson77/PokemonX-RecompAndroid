#include "domain/lift/execution_profile.h"
#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
using namespace hexlith::domain;
namespace p = lift::profile;
lift::runtime::Pc NativeAdd(lift::runtime::Context& c, lift::runtime::Pc pc) {
    ++c.cpu->registers[0];
    --c.budget;
    return pc + 4;
}
int main() {
    p::Take();
    { p::Run r(false); p::Scope<> s(r,p::Stage::Fallback); s.Retired(9); }
    assert(p::Take().sampled_runs == 0);
    std::vector<std::thread> threads;
    for (unsigned i=0;i<4;++i) threads.emplace_back([] {
        for(unsigned j=0;j<1000;++j) {
            p::Run r(true); p::Scope<> s(r,p::Stage::Native); s.Retired(7);
        }
    });
    for(auto& t:threads)t.join();
    auto snap=p::Take();
    assert(snap.sampled_runs==4000 && snap.stages[1].calls==4000 && snap.stages[1].retired==28000);
    assert(snap.sampled_ns>=snap.stages[1].ns);
    assert(p::Take().sampled_runs==0);
    std::uint32_t random=0xa341316c;
    unsigned selected=0;
    for(unsigned i=0;i<1000000;++i)selected+=p::Select(random);
    assert(selected>3500 && selected<4300 && p::Take().runs==1000000);
    memory::ProcessMemory memory;
    assert(memory.Map(0x1000,0x1000,memory::Backing::Allocate(0x1000),0,memory::Access::ReadWrite));
    assert(memory.Map(0x8000,0x1000,memory::Backing::Allocate(0x1000),0,memory::Access::ReadWrite));
    for(unsigned base:{0x1000,0x8000}) {
        assert(memory.Write32(base,0xe2800001)); // ADD r0,r0,#1
        assert(memory.Write32(base+4,0xe2811001)); // ADD r1,r1,#1
        assert(memory.Write32(base+8,0xef000009)); // SVC 9
    }
    std::uint64_t checksum=0;
    for(unsigned variant=0;variant<4;++variant) {
        lift::runtime::EntryTable main, module;
        lift::runtime::LoadedModules modules;
        if(variant==0)main.Add(0x1000,false,NativeAdd);
        module.Add(0,false,NativeAdd);
        modules.Offer("fixture","correct",module);
        modules.Loaded("fixture",0x8000,0,0x1000,variant==2?"wrong":"correct");
        lift::Engine engine(main,&modules);
        for(unsigned i=0;i<8192;++i) {
            arm::CpuState expected,actual;
            expected.registers[15]=actual.registers[15]=variant==0?0x1000:variant==3?0xdead0000:0x8000;
            const auto limit=i%7;
            std::uint64_t a=0,b=0;
            const auto x=arm::Run(expected,memory,limit,a);
            const auto y=engine.Run(actual,memory,limit,b);
            assert(x.reason==y.reason && x.detail==y.detail && a==b);
            assert(expected.registers==actual.registers && arm::PackCpsr(expected)==arm::PackCpsr(actual));
            checksum+=b+actual.registers[0]+actual.registers[1]+actual.registers[15];
        }
    }
#if defined(PX_PROFILE_EXECUTION)
    // Collect enough selected fallback runs to exercise the bounded PC sketch.
    // This synthetic loop must retain exactly the same instruction accounting.
    {
        lift::runtime::EntryTable empty;
        lift::Engine fallback(empty);
        assert(memory.Write32(0x1000,0xe2800001));
        assert(memory.Write32(0x1004,0xeafffffd));
        arm::CpuState state;state.registers[15]=0x1000;
        for(unsigned i=0;i<2000;++i){std::uint64_t count=0;auto stop=fallback.Run(state,memory,8192,count);assert(stop.reason==arm::StopReason::StepLimit&&count==8192);}
        assert(state.registers[0]==8192000 && fallback.RetiredStatistics().engine_fallback==16384000);
        assert(fallback.FallbackStatistics().Samples()>0);
        for(const auto& site:fallback.FallbackStatistics().Snapshot())if(site.count)assert((site.pc==0x1000||site.pc==0x1004)&&!site.thumb&&!site.native_yield);
    }
#endif
    snap=p::Take();
#if defined(PX_PROFILE_EXECUTION)
    assert(snap.runs==34768 && snap.sampled_runs>70);
    assert(snap.stages[0].calls>0 && snap.stages[1].calls>0 && snap.stages[2].calls>0);
    assert(snap.stages[0].retired==0 && snap.stages[1].retired>0 && snap.stages[2].retired>0);
    assert(snap.sampled_ns>=snap.stages[0].ns+snap.stages[1].ns+snap.stages[2].ns);
#else
    assert(snap.runs==0 && snap.sampled_runs==0);
#endif
    std::cout<<"PASS sampling/reset/concurrency; main+module+digest-mismatch+fetch-fault execution, budgets and SVC match interpreter; checksum="<<checksum<<'\n';
}
