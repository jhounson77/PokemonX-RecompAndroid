#include "common/px_screen_interface.h"
#include <cassert>
#include <iostream>
using namespace PokemonXUI;
int main(){
 CommandController c;automatic=true;battle_loaded=true;
 auto reset=[&](int profile){mode=0;scene=profile;decision_hidden=false;popup_requested=false;battle_menu_full=false;move_slots=7;c.Reset();};
 auto input=[&](int raw,int ax=0,int ay=0,int touchx=-1,int touchy=-1){int b=raw,x=ax,y=ay,tx=touchx,ty=touchy;c.Route(b,x,y,tx,ty);return std::array<int,5>{b,x,y,tx,ty};};
 reset(3);input(128);assert(command_focus==2);input(0);input(32);assert(command_focus==1);input(0);
 auto down=input(1);assert(down[0]==0&&down[3]==48&&down[4]==207);assert(!popup_requested);
 scene=0;down=input(1);assert(down[3]==48&&down[4]==207);auto up=input(0);assert(up[3]==-1&&popup_requested&&mode==2&&battle_menu_full);
 auto full=Build(Effective(mode),2340,1080);assert(!full.show_top&&full.show_bottom&&full.count==0);ClosePopup();assert(mode==0&&!battle_menu_full);
 reset(3);input(0,100,0);assert(command_focus==3);input(0);input(1);input(0);assert(mode==2&&popup_requested);ClosePopup();
 reset(3);input(1);input(0);assert(scene==0&&!popup_requested&&!decision_hidden);Observe(4);
 input(0);assert(command_focus==0);input(16);assert(command_focus==1);input(0);input(128);assert(command_focus==1); // slot4 is empty
 input(0);input(32);assert(command_focus==0);input(0);input(128);assert(command_focus==2);input(0);
 down=input(1);assert(down[0]==0&&down[3]==78&&down[4]==130);scene=0;down=input(1);assert(down[3]==78&&down[4]==130);input(0);assert(decision_hidden);
 Observe(0);Observe(3);assert(!decision_hidden);
 reset(4);input(1);input(2);input(0);assert(!decision_hidden); // B cancels held confirmation
 reset(4);Commit(4,4);assert(scene==0&&!decision_hidden);
 reset(3);mode=1;auto dual=input(1,40,20,272,207);assert(dual[0]==1&&dual[1]==40&&dual[2]==20&&dual[3]==272&&dual[4]==207);
 reset(3);auto touch=input(0,0,0,272,207);assert(touch[3]==272&&touch[4]==207);Commit(3,3);assert(popup_requested&&mode==2);
 ClosePopup();mode=0;OpenPopup();assert(mode==5&&!battle_menu_full); // X still uses corner menu
 std::cout<<"PASS command and move navigation, focus, empty slot rejection, held A transition, B cancellation, touch/dual passthrough, full-screen Bag/party and X corner mode\n";
}
