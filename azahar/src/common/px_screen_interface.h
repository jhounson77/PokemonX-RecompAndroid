// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <utility>
#include <cstdint>
#include <cstddef>
namespace PokemonXUI {
// Auto scene changes affect presentation only, never generate gameplay inputs.
// 0 main,1 dual,2 bottom,3 battle commands,4 battle moves,5 original menu panel.
inline std::atomic<int> mode{0};
inline std::atomic<unsigned> generation{0};
inline std::atomic<unsigned> battle_actor_epoch{0};
inline std::atomic<std::uint64_t> battle_move_event{0};
inline std::atomic<bool> metadata_overlay{false};
inline void SignalBattleMove(int slot){auto previous=battle_move_event.load();while(!battle_move_event.compare_exchange_weak(previous,((previous&~std::uint64_t{7})+8)|std::uint64_t(slot))){} }

inline std::atomic<bool> battle_menu_full{false},battle_menu_exit{false};
inline std::atomic<bool> move_cards{true},native_pp_trusted{false};
inline std::atomic<bool> automatic{true},battle_loaded{false},submenu_loaded{false};
inline std::atomic<int> scene{0};
inline std::atomic<bool> force_probe{true};
inline std::atomic<bool> decision_hidden{false},saw_execution{false},popup_requested{false},popup_seen_menu{false},popup_close_requested{false};
inline std::atomic<int> committed_scene{0},popup_return{0},move_slots{15};
inline void Commit(int profile,int index);
inline void Observe(int detected){scene=detected;if(decision_hidden){if(detected==0)saw_execution=true;if(detected==3&&(saw_execution||committed_scene==4))decision_hidden=false;}}
inline void OpenPopup(){++battle_actor_epoch;++generation;if(!popup_requested){popup_return=mode.load()==5?0:mode.load();popup_seen_menu=false;popup_close_requested=false;}popup_requested=true;mode=5;force_probe=true;}
inline void ClosePopup(){++generation;battle_menu_exit=false;battle_menu_full=false;mode=popup_return.load();popup_requested=false;popup_seen_menu=false;popup_close_requested=false;force_probe=true;}
inline int Effective(int selected){if(selected!=0||!automatic||!battle_loaded)return selected;return decision_hidden?0:scene.load();}

// Selection and confirmation share the original game's touch targets. This
// avoids depending on an invisible guest cursor after recomposing the menu.
inline std::atomic<int> command_focus{0};
inline void Commit(int profile,int index){
 if(profile==4&&index>=0&&index<4)SignalBattleMove(index);
 ++generation;
 if((profile==3||profile==4)&&index>=0&&index<4)command_focus=index;
 if((profile==3&&index==0)||(profile==4&&index==4)){if(mode==0)scene=0;force_probe=true;return;}
 if(profile==3&&(index==1||index==3)){OpenPopup();battle_menu_exit=false;battle_menu_full=true;mode=2;return;}
 if((profile==4&&index>=0&&index<4)||(profile==3&&index==2)){committed_scene=profile;saw_execution=false;decision_hidden=true;force_probe=true;}
}
struct CommandController {
 int previous=0,held=-1,held_profile=0,last_profile=0;
 static constexpr int navigation=16|32|64|128;
 static std::pair<int,int> Target(int i,int profile=3){
  if(profile==4)return{(i%2)*160+78,40+(i/2)*60+30};
  constexpr std::pair<int,int> p[]={{160,102},{48,207},{160,216},{272,207}};return p[std::clamp(i,0,3)];
 }
 void Reset(){previous=0;held=-1;held_profile=last_profile=0;command_focus=0;}
 void Route(int& b,int& x,int& y,int& tx,int& ty){
  int raw=b,profile=Effective(mode.load());bool choices=profile==3||profile==4;
  int dir=raw&navigation;
  if(std::abs(x)>45||std::abs(y)>45)dir|=std::abs(x)>std::abs(y)?(x>0?16:32):(y>0?64:128);
  int state=(raw&~navigation)|dir;
  if(choices&&profile!=last_profile)command_focus=0;
  int mask=profile==4?move_slots.load():15;
  if(choices&&!(mask&(1<<command_focus.load())))for(int i=0;i<4;i++)if(mask&(1<<i)){command_focus=i;break;}
  if(held>=0){
   b&=~(1|navigation);x=y=0;tx=ty=-1;
   if(raw&2){held=-1;}
   else if(raw&1){auto p=Target(held,held_profile);tx=p.first;ty=p.second;}
   else{int chosen=held,origin=held_profile;held=-1;Commit(origin,chosen);}
  }else if(choices){
   int fresh=dir&~previous,f=command_focus.load();
   if(fresh){int next=f;
    if(profile==3){if(fresh&64)next=0;else if(fresh&128)next=f==0?2:f;else if(fresh&16)next=f==0?3:(f==3?1:f+1);else if(fresh&32)next=f==0?1:(f==1?3:f-1);}
    else{if(fresh&64)next=f>=2?f-2:f;else if(fresh&128)next=f<2?f+2:f;else if(fresh&16)next=(f%2)==0?f+1:f;else if(fresh&32)next=(f%2)?f-1:f;}
    if(mask&(1<<next))command_focus=next;
   }
   if((raw&1)&&!(previous&1)&&tx<0){held=command_focus.load();held_profile=profile;auto p=Target(held,profile);tx=p.first;ty=p.second;}
   b&=~(1|navigation);x=y=0;
  }
  previous=state;last_profile=profile;
 }
};

struct Rect {float x{},y{},w{},h{}; bool Contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;} };
// Input BGRA is a small real GPU render of the bottom screen, never a fabricated UI.
inline int Classify(const std::uint8_t* pixels,int width,int height){
 if(!pixels||width<8||height<8)return 0;
 auto fraction=[&](Rect area,auto predicate){int n=0,yes=0;int x0=std::max(0,int(area.x*width/320)),x1=std::min(width,int((area.x+area.w)*width/320));int y0=std::max(0,int(area.y*height/240)),y1=std::min(height,int((area.y+area.h)*height/240));for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){auto p=pixels+(y*width+x)*4;yes+=predicate(int(p[2]),int(p[1]),int(p[0]));++n;}return n?float(yes)/n:0.f;};
 auto red=[](int r,int g,int b){return r>90&&r>g*1.4f&&r>b*1.35f;};
 auto blue=[](int r,int g,int b){return b>85&&b>r*1.18f&&g>r*1.05f;};
 auto green=[](int r,int g,int b){return g>85&&g>r*1.1f&&g>b*1.1f;};
 if(fraction({105,50,110,65},red)>.45f&&fraction({246,192,60,40},green)>.20f&&fraction({132,205,56,33},blue)>.20f)return 3;
 if(fraction({274,205,44,33},blue)>.25f){
  auto light=[&](Rect area){float total=0;int count=0;fraction(area,[&](int r,int g,int b){total+=(r+g+b)/3.f;++count;return false;});return count?total/count:0.f;};
  int rows=0;for(int i=0;i<4;i++){float x=float((i%2)*160),y=float(40+(i/2)*60);float top=light({x+40,y+9,80,13}),band=light({x+40,y+39,80,13});if(top>35&&band<top*.92f)++rows;}
  if(rows>0)return 4;
 }
 return 0;
}

// A guest submenu unload is independent of the bottom-screen image: item
// execution need not redraw the original command menu before the animation.
// Two independent probes allow nested submenu loads to cancel the pending exit.
// Residency is not visibility: helpers can stay loaded after the menu closes.
struct BattleMenuExit {
 unsigned session=0;int confirmations=0;
 bool Observe(unsigned token,bool full,bool exited,bool /* resident */){
  if(token!=session){session=token;confirmations=0;}
  if(!full||!exited){confirmations=0;return false;}
  return ++confirmations>=2;
 }
};
inline void ReturnFromBattleMenu(){ClosePopup();scene=0;committed_scene=3;decision_hidden=true;saw_execution=true;force_probe=true;}

struct CommandSignature {
 std::array<unsigned char,300> cells{};bool valid=false;int return_streak=0;
 static unsigned char Color(const std::uint8_t* p){int r=p[2],g=p[1],b=p[0];if(r>90&&r>g*1.4f&&r>b*1.35f)return 1;if(b>85&&b>r*1.18f&&g>r*1.05f)return 2;if(g>85&&g>r*1.1f&&g>b*1.1f)return 3;if(r>130&&g>90&&b<g*.7f)return 4;return 0;}
 void Reset(){valid=false;return_streak=0;}
 static auto Read(const std::uint8_t* p,int w,int h){std::array<unsigned char,300> a{};for(int y=0;y<15;y++)for(int x=0;x<20;x++)a[y*20+x]=Color(p+((y*h/15)*w+x*w/20)*4);return a;}
 void Learn(const std::uint8_t* p,int w,int h){cells=Read(p,w,h);valid=true;return_streak=0;}
 bool Matches(const std::uint8_t* p,int w,int h)const{if(!valid)return false;auto a=Read(p,w,h);int different=0;for(int i=0;i<300;i++)different+=a[i]!=cells[i];return different<=36;}
 bool Return(int detected,const std::uint8_t* p,int w,int h,bool seen,bool close_requested=false){if((seen||close_requested)&&detected==3&&Matches(p,w,h))return ++return_streak>=2;return_streak=0;return false;}
};

inline bool IsPSS(const std::uint8_t* p,int w,int h){
 if(!p||w<=0||h<=0)return false;int yellow=0,body=0,blue=0,head=0;
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){auto q=p+(y*w+x)*4;int b=q[0],g=q[1],r=q[2];if(y>=h/6&&y<h*4/5){body++;yellow+=r>110&&g>80&&b<110&&r>b*1.4f;}if(y<h/8){head++;blue+=b>r*1.2f&&b>50;}}
 return body&&head&&yellow>body*.40f&&blue>head*.20f;
}
inline bool MenuImageVisible(const std::uint8_t* p,int w,int h){if(!p||w<=0||h<=0)return false;int low=0,bright=0;for(int i=0;i<w*h;i++){auto q=p+i*4;int v=(int(q[0])+q[1]+q[2])/3;low+=v<85;bright+=v>155;}return low>w*h*.02f&&bright>w*h*.08f;}
inline void ObservePopup(bool pss,bool visible){if(!popup_requested)return;if(!pss&&visible)popup_seen_menu=true;if(pss&&(popup_seen_menu||popup_close_requested))ClosePopup();}
inline int MoveSlots(const std::uint8_t* p,int w,int h){int mask=0;for(int i=0;i<4;i++){int dark=0,n=0;int x0=(i%2)*160+20,y0=40+(i/2)*60+7;for(int y=y0*h/240;y<(y0+24)*h/240;y++)for(int x=x0*w/320;x<(x0+114)*w/320;x++){auto q=p+(y*w+x)*4;dark+=int(q[0])+q[1]+q[2]<390;++n;}if(n&&dark>n*.025f)mask|=1<<i;}return mask?mask:15;}

struct Piece {Rect source,destination;int shape=0;Rect ellipse{};bool image=true,enabled=true;};
inline bool Visible(const Piece& p,float x,float y){
 if(p.shape==1){float a=(x-p.ellipse.x)/p.ellipse.w,b=(y-p.ellipse.y)/p.ellipse.h;return a*a+b*b<=1.f;}
 if(p.shape==2||p.shape==3){float nx=(x-p.source.x)/p.source.w,ny=(y-p.source.y)/p.source.h;float qx=std::max(std::abs((nx-.5f)*2.6f)-1.1f,0.f),qy=std::max(std::abs(ny-.5f)-.3f,0.f);return qx*qx+qy*qy<=.04f;}
 return true;
}
struct UV {float left,top,right,bottom;};
inline UV CropUV(UV original,Rect src){return{
 original.left+(original.right-original.left)*src.x/320.f,
 original.bottom+(original.top-original.bottom)*(src.y+src.h)/240.f,
 original.left+(original.right-original.left)*(src.x+src.w)/320.f,
 original.bottom+(original.top-original.bottom)*src.y/240.f};}
inline Rect Snap(Rect r){float x=std::round(r.x),y=std::round(r.y);return{x,y,std::round(r.x+r.w)-x,std::round(r.y+r.h)-y};}
struct Layout {Rect top,bottom;bool show_top=true,show_bottom=false;std::array<Piece,5> pieces{};int count=0;};
inline Rect Fit(float x,float y,float width,float height,float sw,float sh){float scale=std::min(width/sw,height/sh);return{x+(width-sw*scale)/2,y+(height-sh*scale)/2,sw*scale,sh*scale};}
inline Layout Build(int selected,int width,int height){
 Layout l;float w=std::max(width,1),h=std::max(height,1);l.top=Snap(Fit(0,0,w,h,400,240));
 if(selected==1){float scale=std::min(w/720,h/240);float x=(w-720*scale)/2,y=(h-240*scale)/2;l.top=Snap({x,y,400*scale,240*scale});l.bottom=Snap({x+400*scale,y,320*scale,240*scale});l.show_bottom=true;return l;}
 if(selected==2){l.show_top=false;l.show_bottom=true;l.bottom=Snap(Fit(0,0,w,h,320,240));return l;}
 auto add=[&](Rect src,Rect dst,int shape=0,Rect ellipse=Rect{},bool image=true,bool enabled=true){float k=l.top.w/400;l.pieces[l.count++]={src,{l.top.x+dst.x*k,l.top.y+dst.y*k,dst.w*k,dst.h*k},shape,ellipse,image,enabled};};
 if(selected==3){
  add({160,102,1,1},{215,170,175,31},0,{},false);
  add({48,207,1,1},{215,205,55,35},0,{},false);
  add({160,216,1,1},{274,205,55,35},0,{},false);
  add({272,207,1,1},{333,205,57,35},0,{},false);
 }else if(selected==4){
  for(int i=0;i<4;i++)add({float((i%2)*160),float(40+(i/2)*60),160,60},{215+(i%2)*89.5f,float(170+(i/2)*35),85.5f,85.5f*60/160},move_cards?3:2,{},true,(move_slots.load()&(1<<i))!=0);
  add({300,220,1,1},{215,151,55,15},0,{},false);
 }else if(selected==5){add({0,0,320,240},{246,132,144,108});}

 return l;
}
inline std::pair<int,int> Touch(int selected,int width,int height,float x,float y){
 if(width<=0||height<=0||!std::isfinite(x)||!std::isfinite(y))return{-1,-1};
 auto l=Build(Effective(selected),width,height);
 auto map=[&](Rect src,Rect dst)->std::pair<int,int>{if(!dst.Contains(x,y))return{-1,-1};return{std::clamp(int(src.x+(x-dst.x)*src.w/dst.w),0,319),std::clamp(int(src.y+(y-dst.y)*src.h/dst.h),0,239)};};
 for(int i=l.count-1;i>=0;--i)if(l.pieces[i].enabled&&l.pieces[i].destination.Contains(x,y)){auto point=map(l.pieces[i].source,l.pieces[i].destination);if(Visible(l.pieces[i],point.first+.5f,point.second+.5f))return point;}
 if(l.show_bottom)return map({0,0,320,240},l.bottom);
 return{-1,-1};
}
}
