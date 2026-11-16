#include "domain/lift/decoded_instruction_cache.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>
using namespace hexlith::domain;
int main() {
    const std::vector<std::vector<unsigned>> programs={
        {0xe2811001,0xeafffffd}, // ADD; B
        {0xe5901000,0xe2811001,0xe5801000,0xeafffffb}, // LDR; ADD; STR; B
        {0xe6bf1f31,0xeafffffd}, // REV; B
        {0x0290f001,0xeafffffd}, // condition-failed ADDEQS pc; B
    };
    const char* names[]={"alu","load-store","media","condition-failed"};
    for(unsigned which=0;which<programs.size();++which) {
        memory::ProcessMemory memory;
        assert(memory.Map(0x1000,0x1000,memory::Backing::Allocate(0x1000),0,memory::Access::ReadWrite));
        for(unsigned i=0;i<programs[which].size();++i)assert(memory.Write32(0x1000+4*i,programs[which][i]));
        lift::DecodedInstructionCache cache;
        arm::CpuState state;state.registers[15]=0x1000;state.registers[0]=0x1800;
        // Warm the code/data path before timing. Reset semantic state afterwards.
        for(unsigned i=0;i<100000;++i)assert(cache.Step(state,memory).reason==arm::StopReason::None);
        state={};state.registers[15]=0x1000;state.registers[0]=0x1800;
        assert(memory.Write32(0x1800,0));
        constexpr unsigned steps=16000000;
        auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<steps;++i)assert(cache.Step(state,memory).reason==arm::StopReason::None);
        auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        if(which==0)assert(state.registers[1]==steps/2);
        if(which==1)assert(memory.Read32(0x1800)==steps/4);
        assert(cache.Counted().hits>=steps-4);
        std::cout<<names[which]<<" ms="<<elapsed<<" steps="<<steps<<" checksum="<<state.registers[1]<<'\n';
    }
    std::cout<<"Synthetic desktop cache workload; not Android FPS.\n";
}
