#include "domain/memory/process_memory.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
using namespace hexlith::domain::memory;
extern "C" bool LegacyRead32(void*,std::uint32_t,std::uint32_t*);
struct Provider:ExternalMemory {
 std::array<std::uint8_t,8192> bytes{};unsigned reads=0,writes=0;
 bool Read(std::uint32_t a,std::span<std::uint8_t>b)override{++reads;if(a>bytes.size()||b.size()>bytes.size()-a)return false;std::memcpy(b.data(),bytes.data()+a,b.size());return true;}
 bool Write(std::uint32_t a,std::span<const std::uint8_t>b)override{++writes;if(a>bytes.size()||b.size()>bytes.size()-a)return false;std::memcpy(bytes.data()+a,b.data(),b.size());return true;}
};
int main(){
 Provider provider;std::vector<std::uint8_t*> pages(1<<20,nullptr);
 pages[0]=provider.bytes.data();std::array<std::uint8_t,4096> replacement{};
 {
 ProcessMemory m;assert(m.UseExternal(provider));assert(m.BindExternalPages(pages));
 assert(m.Write32(4,0xaabbccdd));assert(provider.writes==0);std::uint32_t v=0;
 assert(LegacyRead32(&m,4,&v)&&v==0xaabbccdd);assert(provider.reads==0);
 // A GPU-owned page loses its direct pointer: even the old generated reader
 // must delegate rather than read a stale RAM copy.
 pages[0]=nullptr;assert(LegacyRead32(&m,4,&v)&&v==0xaabbccdd);assert(provider.reads==1);
 assert(m.Write32(4,123));assert(provider.writes==1);
 // A process remap is visible immediately through the same live table.
 std::uint32_t n=456;std::memcpy(replacement.data()+4,&n,4);pages[0]=replacement.data();
 assert(LegacyRead32(&m,4,&v)&&v==456);assert(provider.reads==1);
 assert(m.Write32(4094,0x12345678));assert(m.Read32(4094)==0x12345678);assert(provider.reads==2&&provider.writes==2);
 assert(!m.BindExternalPages(std::span<std::uint8_t*>(pages.data(),1)));
 assert(m.BindExternalPages({}));assert(LegacyRead32(&m,4,&v)&&v==123);assert(provider.reads==3);
 assert(!m.Map(0,4096,Backing::Allocate(4096),0,Access::ReadWrite));
 }
 // Destruction must not free the borrowed page table or its RAM.
 pages[0]=replacement.data();assert(pages[0]==replacement.data());
 ProcessMemory owned;assert(owned.Map(0,4096,Backing::Allocate(4096),0,Access::ReadWrite));assert(owned.Write32(0,777));assert(owned.Read32(0)==777);
 std::cout<<"PASS: old-reader ABI, live remapping, GPU ownership fallback, crossing access, owned/borrowed lifetime and legacy owned memory\n";
}
