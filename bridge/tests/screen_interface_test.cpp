#include "common/px_screen_interface.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
using namespace PokemonXUI;
bool Near(float a,float b){return std::abs(a-b)<.0001f;}
int main(){
 automatic=false;move_slots=15;decision_hidden=false;
 auto dual=Build(1,720,240);assert(dual.show_top&&dual.show_bottom&&dual.count==0);
 assert(Near(dual.top.x,0)&&Near(dual.top.w,400)&&Near(dual.bottom.x,400)&&Near(dual.bottom.w,320));
 assert((Touch(1,720,240,400,0)==std::pair<int,int>{0,0}));assert((Touch(1,720,240,719.9f,239.9f)==std::pair<int,int>{319,239}));assert((Touch(1,720,240,399,10)==std::pair<int,int>{-1,-1}));
 assert((Touch(1,720,480,400,120)==std::pair<int,int>{0,0}));assert((Touch(1,720,480,450,119)==std::pair<int,int>{-1,-1}));
 auto commands=Build(3,400,240);assert(commands.count==4);for(int i=0;i<4;i++)assert(!commands.pieces[i].image);
 for(int i=0;i<4;i++){auto p=commands.pieces[i];auto [x,y]=Touch(3,400,240,p.destination.x+p.destination.w*.5f,p.destination.y+p.destination.h*.5f);assert(x>=0&&y>=0);assert(Visible(p,x+.5f,y+.5f));}
 assert((Touch(3,400,240,210,220)==std::pair<int,int>{-1,-1}));
 for(auto [w,h]:std::vector<std::pair<int,int>>{{400,240},{720,480},{1920,1080},{2340,1080},{1817,997}}){for(int m=0;m<=5;m++){auto l=Build(m,w,h);for(int i=0;i<l.count;i++){auto p=l.pieces[i];assert(p.source.x>=0&&p.source.y>=0&&p.source.x+p.source.w<=320&&p.source.y+p.source.h<=240);assert(p.destination.x>=0&&p.destination.y>=0&&p.destination.x+p.destination.w<=w+.01f&&p.destination.y+p.destination.h<=h+.01f);auto [x,y]=Touch(m,w,h,p.destination.x+p.destination.w*.51f,p.destination.y+p.destination.h*.51f);assert(x>=p.source.x&&x<p.source.x+p.source.w&&y>=p.source.y&&y<p.source.y+p.source.h);}}}
 auto moves=Build(4,400,240);assert(moves.count==5);for(int i=0;i<4;i++)assert(moves.pieces[i].destination.y>=170);assert(!moves.pieces[4].image);
 move_slots=7;assert(!Build(4,400,240).pieces[3].enabled);assert((Touch(4,400,240,348,221)==std::pair<int,int>{-1,-1}));move_slots=15;
 assert((Touch(5,400,240,246,132)==std::pair<int,int>{0,0}));assert((Touch(5,400,240,318,186)==std::pair<int,int>{160,120}));
 assert((Touch(1,0,480,2,3)==std::pair<int,int>{-1,-1}));assert((Touch(4,400,240,std::numeric_limits<float>::quiet_NaN(),3)==std::pair<int,int>{-1,-1}));
 auto uv=CropUV({.15f,.2f,.85f,.9f},{0,0,160,120});assert(Near(uv.left,.15f)&&Near(uv.right,.5f)&&Near(uv.bottom,.9f)&&Near(uv.top,.55f));
 std::cout<<"PASS dual mode, original command crops, 2x2 move region, hidden empty moves, bottom-right popup, viewport scaling and UV mapping\n";
}
