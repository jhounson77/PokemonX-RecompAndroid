#pragma once
#include "domain/gpu/screen.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>
namespace hexlith::adapters::vulkan {
// Guest buffers keep native dimensions. Only GPU images and their staging
// copies are scaled; uploads replicate native texels, readbacks select centers.
inline void ExpandRenderPixelsInto(std::span<const std::uint8_t> data,
 std::span<std::uint8_t> out,std::uint32_t w,std::uint32_t h,std::uint32_t channels,std::uint32_t scale){
 if(w==0||h==0||channels==0||scale==0)return;
 const std::size_t row_bytes=static_cast<std::size_t>(w)*channels*scale;
 for(std::uint32_t y=0;y<h;++y){
  auto* row=out.data()+static_cast<std::size_t>(y)*scale*row_bytes;
  const auto* source=data.data()+static_cast<std::size_t>(y)*w*channels;
  if(channels==4){for(std::uint32_t x=0;x<w;++x)for(std::uint32_t repeat=0;repeat<scale;++repeat)std::memcpy(row+(static_cast<std::size_t>(x)*scale+repeat)*4,source+x*4,4);}
  else if(channels==1){for(std::uint32_t x=0;x<w;++x)std::fill_n(row+static_cast<std::size_t>(x)*scale,scale,source[x]);}
  else{for(std::uint32_t x=0;x<w;++x)for(std::uint32_t repeat=0;repeat<scale;++repeat)std::memcpy(row+(static_cast<std::size_t>(x)*scale+repeat)*channels,source+x*channels,channels);}
  for(std::uint32_t repeat=1;repeat<scale;++repeat)std::memcpy(row+repeat*row_bytes,row,row_bytes);
 }
}
inline std::vector<std::uint8_t> ExpandRenderPixels(std::span<const std::uint8_t> data,
 std::uint32_t w,std::uint32_t h,std::uint32_t channels,std::uint32_t scale){
 std::vector<std::uint8_t> out(static_cast<std::size_t>(w)*h*channels*scale*scale);
 ExpandRenderPixelsInto(data,out,w,h,channels,scale);return out;
}
inline std::vector<std::uint8_t> ReduceRenderPixels(std::span<const std::uint8_t> data,
 std::uint32_t w,std::uint32_t h,std::uint32_t channels,std::uint32_t scale){
 std::vector<std::uint8_t> out(static_cast<std::size_t>(w)*h*channels);
 for(std::uint32_t y=0;y<h;y++)for(std::uint32_t x=0;x<w;x++)
  std::memcpy(out.data()+(static_cast<std::size_t>(y)*w+x)*channels,
   data.data()+(static_cast<std::size_t>(y*scale+scale/2)*w*scale+x*scale+scale/2)*channels,channels);
 return out;
}
inline domain::gpu::Picture RotateRenderFramebuffer(std::span<const std::uint8_t> rgba,
 std::uint32_t width,std::uint32_t height){
 domain::gpu::Picture p{.width=height,.height=width};p.pixels.resize(static_cast<std::size_t>(width)*height);
 for(std::uint32_t y=0;y<p.height;y++)for(std::uint32_t x=0;x<p.width;x++){
  auto at=(static_cast<std::size_t>(x)*width+width-1-y)*4;
  p.pixels[static_cast<std::size_t>(y)*p.width+x]={rgba[at],rgba[at+1],rgba[at+2],255};
 }
 return p;
}
}
