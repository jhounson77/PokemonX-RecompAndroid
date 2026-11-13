// Synthetic reader microbenchmark; this does not predict phone gameplay FPS.
#include "domain/memory/process_memory.h"
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
using namespace hexlith::domain::memory;
extern "C" bool LegacyRead32(void*,std::uint32_t,std::uint32_t*);
struct Provider:ExternalMemory{std::array<std::uint8_t,4096>bytes{};std::uint64_t reads=0;bool Read(std::uint32_t a,std::span<std::uint8_t>b)override{++reads;std::memcpy(b.data(),bytes.data()+a,b.size());return true;}bool Write(std::uint32_t a,std::span<const std::uint8_t>b)override{std::memcpy(bytes.data()+a,b.data(),b.size());return true;}};
int main(){Provider p;ProcessMemory m;if(!m.UseExternal(p))return 1;std::vector<std::uint8_t*>pages(1<<20,nullptr);pages[0]=p.bytes.data();for(bool fast:{false,true}){if(fast&&!m.BindExternalPages(pages))return 2;std::uint64_t sum=0;auto before=p.reads;auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<8000000;++i){std::uint32_t value;if(!LegacyRead32(&m,(i%1024)*4,&value))return 3;sum+=value;}auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();std::cout<<(fast?"live_pages":"delegated")<<" ms="<<elapsed<<" callbacks="<<p.reads-before<<" checksum="<<sum<<'\n';if(fast&&p.reads!=before)return 4;}}
