#include "common/px_screen_interface.h"
#include <cassert>
#include <iostream>
using namespace PokemonXUI;
int main(){automatic=false;move_slots=15;move_cards=true;
 auto cards=Build(4,400,240);for(int i=0;i<4;i++){auto p=cards.pieces[i];assert(p.image&&p.shape==3&&p.source.w==160);assert(p.source.x+p.source.w<=320);auto hit=Touch(4,400,240,p.destination.x+p.destination.w*.5f,p.destination.y+p.destination.h*.5f);assert(hit.first==(i%2)*160+80&&hit.second==40+(i/2)*60+30);}
 move_cards=false;auto original=Build(4,400,240);for(int i=0;i<4;i++){assert(original.pieces[i].shape==2);assert(original.pieces[i].source.w==160);}
 move_cards=true;move_slots=7;assert(!Build(4,400,240).pieces[3].enabled);
 std::cout<<"PASS live source fields use full160px width (no last-PP-digit crop), identical touch mapping, original-style fallback and empty slot suppression\n";
}
