#pragma once
#include <array>
#include <cstdint>
namespace PokemonX {
struct BattleMove { unsigned id=0,pp=0,max_pp=0; bool operator==(const BattleMove&) const=default; };
struct BattleRecord { unsigned address=0,species=0,hp=0,max_hp=0; std::array<BattleMove,4> moves{}; bool valid=false; bool operator==(const BattleRecord&) const=default; };
using BattleParty=std::array<BattleRecord,6>;
// Experimental correlation, not a substitute for the game's active-battler
// pointer. Only a unique PP decrease after an explicit selection is evidence.
class BattleIdentity {
public:
 int Observe(const BattleParty& party,unsigned epoch,std::uint64_t event){
  if(epoch!=epoch_){Reset();epoch_=epoch;event_=event;}
  if(event!=event_){active_=-1;pending_=have_previous_;pending_samples_=0;before_=previous_;slot_=int(event&7);event_=event;}
  if(active_>=0&&(!SameIdentity(confirmed_,party[active_])||party[active_].hp==0))active_=-1;
  if(pending_&&++pending_samples_>200)pending_=false;
  if(pending_&&slot_<4){int candidate=-1;bool ambiguous=false;
   for(int i=0;i<6;i++){
    if(!SameIdentity(before_[i],party[i])){ambiguous=true;continue;}
    if(!party[i].valid)continue;
    for(int m=0;m<4;m++)if(m!=slot_&&before_[i].moves[m]!=party[i].moves[m])ambiguous=true;
    const auto old=before_[i].moves[slot_],now=party[i].moves[slot_];
    if(now.id!=old.id||now.max_pp!=old.max_pp||now.pp>old.pp)ambiguous=true;
    if(now.pp<old.pp){if(candidate>=0||party[i].hp==0)ambiguous=true;candidate=i;}
   }
   if(ambiguous)pending_=false;
   else if(candidate>=0){active_=candidate;confirmed_=party[candidate];pending_=false;}
  }
  previous_=party;have_previous_=true;return active_;
 }
 void Reset(){active_=-1;pending_=false;have_previous_=false;}
private:
 static bool SameIdentity(const BattleRecord& a,const BattleRecord& b){
  if(a.valid!=b.valid)return false;
  if(!a.valid)return a.address==b.address;
  if(a.address!=b.address||a.species!=b.species||a.max_hp!=b.max_hp)return false;
  for(int m=0;m<4;m++)if(a.moves[m].id!=b.moves[m].id||a.moves[m].max_pp!=b.moves[m].max_pp)return false;
  return true;
 }
 BattleParty previous_{},before_{};BattleRecord confirmed_{};
 unsigned epoch_=~0u;std::uint64_t event_=0;int active_=-1,slot_=0,pending_samples_=0;
 bool have_previous_=false,pending_=false;
};
}
