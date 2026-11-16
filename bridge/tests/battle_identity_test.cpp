#include "battle_identity.h"
#include <cassert>
#include <iostream>
using namespace PokemonX;
BattleParty Team(){BattleParty p{};for(int i=0;i<3;i++){auto& r=p[i];r.valid=true;r.address=0x8200000+i*0x1000;r.species=i==2?25:656;r.hp=20;r.max_hp=22;for(auto& m:r.moves)m={1,20,35};}return p;}
int main(){BattleIdentity identity;auto p=Team();assert(identity.Observe(p,1,0)==-1);
 assert(identity.Observe(p,1,8)==-1);p[1].moves[0].pp--;assert(identity.Observe(p,1,8)==1);
 assert(identity.Observe(p,1,8)==1);assert(identity.Observe(p,2,8)==-1);
 assert(identity.Observe(p,2,17)==-1);p[2].moves[1].pp-=2;assert(identity.Observe(p,2,17)==2);
 identity.Reset();identity.Observe(p,3,0);identity.Observe(p,3,8);p[0].moves[0].pp--;p[1].moves[0].pp--;assert(identity.Observe(p,3,8)==-1);
 identity.Reset();identity.Observe(p,4,0);identity.Observe(p,4,8);p[1].moves[0].id=2;p[1].moves[0].pp--;assert(identity.Observe(p,4,8)==-1);
 identity.Reset();identity.Observe(p,5,0);identity.Observe(p,5,8);p[2].moves[0].pp++;assert(identity.Observe(p,5,8)==-1);
 identity.Reset();identity.Observe(p,6,0);identity.Observe(p,6,8);p[1].moves[0].pp--;p[1].hp=0;assert(identity.Observe(p,6,8)==-1);
 identity.Reset();identity.Observe(p,7,0);identity.Observe(p,7,8);assert(identity.Observe(p,7,8)==-1);
 identity.Reset();p=Team();identity.Observe(p,8,0);identity.Observe(p,8,8);
 for(int i=0;i<201;i++)assert(identity.Observe(p,8,8)==-1);
 p[0].moves[0].pp--;assert(identity.Observe(p,8,8)==-1);
 identity.Reset();p=Team();p[0].hp=0;identity.Observe(p,9,0);identity.Observe(p,9,8);p[1].moves[0].pp--;assert(identity.Observe(p,9,8)==1); // an unrelated fainted bench member is not the actor
 std::cout<<"PASS duplicate species, changed actor slot, PP/Pressure, menu epoch, ambiguity, changed moves, healing, faint and no spend\n";
}
