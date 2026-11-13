#include "../render_scale.h"
#include <cassert>
#include <iostream>
int main(){using namespace PokemonX;
 static_assert(MaximumScale==4);
 for(int value=1;value<=4;++value){assert(ChooseRenderScale(value,2340,1080)==value);assert(NativeHeight*value<=HeightBudget);}
 assert(NativeHeight*(MaximumScale+1)>HeightBudget);
 assert(ChooseRenderScale(0,2340,1080)==4);
 assert(ChooseRenderScale(0,1280,720)==3);
 assert(ChooseRenderScale(0,800,480)==2);
 assert(ChooseRenderScale(0,200,120)==1);
 assert(ChooseRenderScale(0,3840,2160)==4);
 assert(ChooseRenderScale(99,2340,1080)==4);
 assert(ChooseRenderScale(-9,2340,1080)==1);
 std::cout<<"PASS: manual presets, screen-fit automatic choice, native aspect and 1080-line integer ceiling\n";
}
