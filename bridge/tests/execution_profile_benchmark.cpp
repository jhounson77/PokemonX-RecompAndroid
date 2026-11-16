#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include <cassert>
#include <chrono>
#include <iostream>
using namespace hexlith::domain;
lift::runtime::Pc NativeAdd(lift::runtime::Context& c, lift::runtime::Pc pc) {
    ++c.cpu->registers[0]; --c.budget; return pc+4;
}
int main() {
    memory::ProcessMemory memory;
    assert(memory.Map(0x1000,0x1000,memory::Backing::Allocate(0x1000),0,memory::Access::ReadWrite));
    assert(memory.Write32(0x1000,0xe2800001));
    assert(memory.Write32(0x1004,0xeafffffd));
    for(bool native:{false,true}) {
        lift::runtime::EntryTable entries;
        if(native)entries.Add(0x1000,false,NativeAdd);
        lift::Engine engine(entries);
        arm::CpuState state; state.registers[15]=0x1000;
        auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<2000;++i) {
            std::uint64_t executed=0;
            auto stop=engine.Run(state,memory,8192,executed);
            assert(executed==8192 && stop.reason==arm::StopReason::StepLimit);
        }
        assert(state.registers[0]==8192000);
        std::cout<<(native?"mixed-main":"fallback")<<" ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<" retired="<<engine.RetiredStatistics().total<<'\n';
    }
    std::cout<<"Desktop synthetic timing only; not phone FPS or a performance guarantee.\n";
}
