#pragma once
#include <algorithm>
namespace pxandroid {
struct Rect {int x=0,y=0,w=0,h=0;};
inline Rect Fit(int w,int h,int source_w,int source_h){
 if(w<=0||h<=0||source_w<=0||source_h<=0)return {};
 int dw=std::min(w,h*source_w/source_h),dh=dw*source_h/source_w;
 return {(w-dw)/2,(h-dh)/2,dw,dh};
}
}
