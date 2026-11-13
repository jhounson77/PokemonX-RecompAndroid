#include "common/px_screen_interface.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
using namespace PokemonXUI;
int main(int argc,char**argv){
 std::array<std::uint8_t,80*60*4> p{};assert(Classify(p.data(),80,60)==0);p.fill(255);assert(Classify(p.data(),80,60)==0);p.fill(100);assert(Classify(p.data(),80,60)==0);assert(Classify(nullptr,80,60)==0);
 automatic=true;battle_loaded=false;scene=3;assert(Effective(0)==0);battle_loaded=true;assert(Effective(0)==3);submenu_loaded=true;assert(Effective(0)==3);submenu_loaded=false;
 scene=4;Commit(4,0);assert(Effective(0)==0);Observe(4);assert(Effective(0)==0);Observe(0);assert(Effective(0)==0);Observe(3);assert(Effective(0)==3);
 Commit(3,2);Observe(3);assert(Effective(0)==0);Observe(0);Observe(3);assert(Effective(0)==3);
 Commit(3,0);assert(!decision_hidden);Commit(3,1);assert(!decision_hidden&&popup_requested&&mode==2&&battle_menu_full);ClosePopup();
 mode=0;OpenPopup();assert(mode==5&&popup_requested);ObservePopup(true,false);assert(popup_requested);ObservePopup(false,true);assert(popup_seen_menu);ObservePopup(false,true);assert(popup_requested);ObservePopup(true,true);assert(!popup_requested&&mode==0);
 mode=3;OpenPopup();popup_close_requested=true;ObservePopup(true,false);assert(!popup_requested&&mode==3);mode=0;
 if(argc>1){int expected[]={3,4,0};int i=0;for(auto name:{"commands","moves","pss"}){std::ifstream f(std::string(argv[1])+"/"+name+".bgra",std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});assert(b.size()==p.size());int detected=Classify(b.data(),80,60);std::cout<<name<<"="<<detected<<" PSS="<<IsPSS(b.data(),80,60)<<" slots="<<MoveSlots(b.data(),80,60)<<std::endl;assert(detected==expected[i++]);assert(IsPSS(b.data(),80,60)==(std::string(name)=="pss"));if(std::string(name)=="moves")assert(MoveSlots(b.data(),80,60)==7);}}
 std::cout<<"PASS decision hiding/rearm, item submenu routing, menu persistence across nested screens and real PSS return, classifier fixtures\n";
}
