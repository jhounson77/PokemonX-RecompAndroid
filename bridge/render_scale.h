#pragma once
#include <algorithm>
namespace PokemonX {
// Azahar's surface cache uses integral scales. At 400x240, 4x is the
// highest integral scale within a 1080-line rendering budget (960 lines).
inline constexpr int NativeWidth=400, NativeHeight=240, HeightBudget=1080;
inline constexpr int MaximumScale=HeightBudget/NativeHeight;
constexpr int ChooseRenderScale(int requested,int width,int height){
 if(requested!=0)return std::clamp(requested,1,MaximumScale);
 return std::clamp(std::min(width/NativeWidth,height/NativeHeight),1,MaximumScale);
}
}
