#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>
namespace hexlith::adapters::vulkan::cache_store {
constexpr std::size_t MaxBytes=32*1024*1024;
inline std::uint64_t Hash(std::span<const std::uint8_t> bytes){std::uint64_t hash=14695981039346656037ULL;for(auto b:bytes){hash^=b;hash*=1099511628211ULL;}return hash;}
inline void Put(std::vector<std::uint8_t>& out,std::uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));}
inline std::uint64_t Read(std::span<const std::uint8_t> in,std::size_t at,unsigned count){std::uint64_t value=0;for(unsigned i=0;i<count;++i)value|=static_cast<std::uint64_t>(in[at+i])<<(8*i);return value;}
// App-private envelope: magic, payload size, checksum; then the Vulkan blob.
inline std::vector<std::uint8_t> Pack(std::span<const std::uint8_t> payload){std::vector<std::uint8_t> out{'P','X','V','K','C','0','0','1'};Put(out,payload.size(),8);Put(out,Hash(payload),8);out.insert(out.end(),payload.begin(),payload.end());return out;}
inline std::vector<std::uint8_t> Unpack(std::span<const std::uint8_t> in,std::uint32_t vendor,std::uint32_t device,const std::array<std::uint8_t,16>&uuid){
 constexpr std::array<std::uint8_t,8> magic{'P','X','V','K','C','0','0','1'};
 if(in.size()<56||in.size()>MaxBytes+24||!std::equal(magic.begin(),magic.end(),in.begin()))return {};
 auto bytes=in.subspan(24);if(Read(in,8,8)!=bytes.size()||Read(in,16,8)!=Hash(bytes))return {};
 // VkPipelineCacheHeaderVersionOne: explicit little-endian 32-byte header.
 // Source: docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCacheHeaderVersionOne.html
 if(Read(bytes,0,4)!=32||Read(bytes,4,4)!=1||Read(bytes,8,4)!=vendor||Read(bytes,12,4)!=device||!std::equal(uuid.begin(),uuid.end(),bytes.begin()+16))return {};
 return {bytes.begin(),bytes.end()};
}
}
