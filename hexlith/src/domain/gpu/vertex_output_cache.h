#pragma once
#include "domain/gpu/shader_interpreter.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace hexlith::domain::gpu {
// Same 64-entry FIFO retention as the previous linear cache. Bucket chains
// change lookup cost only, never the set of vertices retained or their order.
class VertexOutputCache {
public:
 static constexpr std::size_t Size=64;
 VertexOutputCache(){heads_.fill(-1);links_.fill(-1);}
 const ShaderRegisters::Bank* Find(std::uint32_t vertex)const{
  for(int slot=heads_[Bucket(vertex)];slot>=0;slot=links_[slot])if(ids_[slot]==vertex)return &outputs_[slot];
  return nullptr;
 }
 // Only called after Find misses, so each retained vertex has one entry.
 void Add(std::uint32_t vertex,const ShaderRegisters::Bank& outputs){
  if(filled_==Size){int* link=&heads_[Bucket(ids_[next_])];while(*link>=0&&*link!=static_cast<int>(next_))link=&links_[*link];if(*link>=0)*link=links_[next_];}
  else ++filled_;
  auto bucket=Bucket(vertex);ids_[next_]=vertex;outputs_[next_]=outputs;links_[next_]=heads_[bucket];heads_[bucket]=static_cast<int>(next_);next_=(next_+1)%Size;
 }
private:
 static std::size_t Bucket(std::uint32_t vertex){return (vertex^(vertex>>6)^(vertex>>12))&(Size-1);}
 // Entries are written in full before their slot is linked into a bucket.
 std::array<std::uint32_t,Size> ids_;
 std::array<ShaderRegisters::Bank,Size> outputs_;
 std::array<int,Size> heads_,links_;
 std::size_t next_=0,filled_=0;
};
}
