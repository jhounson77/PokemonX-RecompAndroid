#include "domain/memory/process_memory.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
using namespace hexlith::domain::memory;
struct External final: ExternalMemory {
    std::array<std::uint8_t,8192> data{};
    unsigned reads=0,writes=0;
    bool Read(std::uint32_t a,std::span<std::uint8_t> b) override {
        ++reads;if(a>data.size()||b.size()>data.size()-a)return false;
        std::memcpy(b.data(),data.data()+a,b.size());return true;
    }
    bool Write(std::uint32_t a,std::span<const std::uint8_t> b) override {
        ++writes;if(a>data.size()||b.size()>data.size()-a)return false;
        std::memcpy(data.data()+a,b.data(),b.size());return true;
    }
};
int main(){
    External external;ProcessMemory memory;assert(memory.UseExternal(external));
    assert(memory.Write8(3,0x12));assert(memory.Read8(3)==0x12);
    assert(memory.Write16(7,0x1234));assert(memory.Read16(7)==0x1234);
    assert(memory.Write32(10,0x12345678));assert(memory.Read32(10)==0x12345678);
    assert(memory.Write64(30,0x123456789abcdef0ULL));assert(memory.Read64(30)==0x123456789abcdef0ULL);
    assert(memory.Write32(4094,0xaabbccdd));assert(memory.Read32(4094)==0xaabbccdd);
    std::uint32_t value=0;assert(memory.ReadInto(4094,value)&&value==0xaabbccdd);
    assert(!memory.Read32(8190));assert(!memory.Write64(8188,7));
    assert(!memory.Read8(0xffffffff));assert(!memory.Write8(0xffffffff,1));
    assert(!memory.Map(0,4096,Backing::Allocate(4096),0,Access::ReadWrite));
    assert(external.reads==8&&external.writes==7);
    ProcessMemory mapped;assert(mapped.Map(0,4096,Backing::Allocate(4096),0,Access::ReadWrite));
    assert(!mapped.UseExternal(external));assert(mapped.Write32(0,99));assert(mapped.Read32(0)==99);
    std::cout<<"External memory:8/16/32/64bit, unaligned/cross-page, faults, callback coherence and mapping isolation passed\n";
}
