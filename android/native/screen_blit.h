#pragma once
#include "display_layout.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
namespace pxandroid {
// Nearest-neighbour presentation, preserving the old byte packing and sampling.
// Cache the horizontal map and copy repeated output rows instead of scaling
// each output pixel again. Only letterbox areas need clearing.
class ScreenBlitter {
 std::vector<unsigned> columns_;
 int width_=0;unsigned source_width_=0;
 public:
 template<class Picture> void Draw(std::uint32_t* dst,int w,int h,int stride,const Picture& p,Rect r){
  constexpr std::uint32_t background=0xff080808u;
  if(!dst||w<=0||h<=0||stride<w)return;
  const int left=std::max(0,r.x),right=std::min(w,r.x+r.w),top=std::max(0,r.y),bottom=std::min(h,r.y+r.h);
  if(p.width==0||p.height==0||p.pixels.size()<static_cast<std::size_t>(p.width)*p.height||r.w<=0||r.h<=0||left>=right||top>=bottom){std::fill(dst,dst+stride*h,background);return;}
  if(width_!=r.w||source_width_!=p.width){width_=r.w;source_width_=p.width;columns_.resize(r.w);for(int x=0;x<r.w;++x)columns_[x]=static_cast<unsigned>(x)*p.width/r.w;}
  std::fill(dst,dst+top*stride,background);std::fill(dst+bottom*stride,dst+h*stride,background);
  unsigned previous=~0u;
  for(int y=top;y<bottom;++y){auto* row=dst+y*stride;std::fill(row,row+left,background);std::fill(row+right,row+stride,background);unsigned sy=static_cast<unsigned>(y-r.y)*p.height/r.h;
   if(sy==previous)std::memcpy(row+left,row-stride+left,(right-left)*sizeof(*row));
   else for(int x=left;x<right;++x){auto c=p.pixels[sy*p.width+columns_[x-r.x]];row[x]=c.red|(c.green<<8)|(c.blue<<16)|0xff000000u;}
   previous=sy;
  }
 }
};
}
