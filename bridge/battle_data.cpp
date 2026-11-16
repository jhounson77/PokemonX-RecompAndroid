#include "battle_data.h"
#include "battle_identity.h"
#include "program.h"
#include "common/px_screen_interface.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/memory.h"
#include <chrono>
#include <mutex>
namespace PokemonX {
namespace {
std::mutex lock;std::array<int,15> current{};BattleIdentity identity;BattleParty last_party{};
std::chrono::steady_clock::time_point next{};
// Read-only XY1.5 layout: Gen6CTRPFrameworkOverhauled Helpers.cpp/PKHeX.cpp,
// commit16fc26e1000919c6e284f171d05fd25fcbfb9c0d. Never reuse for base XY.
constexpr unsigned State=0x081FB170,Party=0x081FB284;
}
std::array<int,15> BattleDataSnapshot(){std::lock_guard guard(lock);return current;}
void RefreshBattleData(){
 auto now=std::chrono::steady_clock::now();if(now<next)return;next=now+std::chrono::milliseconds(50);
 std::array<int,15> value{};auto program=GetProgram();BattleParty party{};bool complete=true;
 if(program&&program->IsUpdate15()&&PokemonXUI::battle_loaded){
  auto& memory=Core::System::GetInstance().Memory();auto read=[&](unsigned at){return memory.Read32OrNullopt(at);};auto state=read(State);
  if(state&&*state==0x40001){
   for(unsigned i=0;i<party.size();i++){
    auto pointer=read(Party+i*4);if(!pointer){complete=false;break;}
    auto& record=party[i];record.address=*pointer;if(!*pointer)continue;
    if(*pointer<0x07000000||*pointer>=0x0F000000||(*pointer&3)){complete=false;break;}
    for(unsigned j=0;j<i;j++)if(party[j].address==*pointer)complete=false;
    auto data=read(*pointer+0xc),hp=read(*pointer+0x10);if(!data||!hp){complete=false;break;}
    record.species=*data&65535;record.max_hp=*data>>16;record.hp=*hp&65535;
    bool sane=record.species>=1&&record.species<=721&&record.max_hp>=1&&record.max_hp<=1000&&record.hp<=record.max_hp;int occupied=0;
    for(unsigned m=0;m<4;m++){
     unsigned at=*pointer+0x11c+m*0xe;auto lo=read(at&~3u);unsigned word=0;
     if(!lo){sane=false;continue;}
     if((at&3)==0)word=*lo;
     else{auto hi=read((at&~3u)+4);if(!hi){sane=false;continue;}word=(*lo>>16)|(*hi<<16);}
     auto& move=record.moves[m];move={word&65535,(word>>16)&255,word>>24};
     if(move.id==0){if(move.pp||move.max_pp)sane=false;}
     else{occupied++;if(move.id>621||move.max_pp<1||move.max_pp>64||move.pp>move.max_pp)sane=false;}
    }
    record.valid=sane&&occupied>0;if(!record.valid)complete=false;else value[1]++;
   }
  }else complete=false;
 }else complete=false;
 if(party!=last_party){
  if(complete)for(unsigned i=0;i<party.size();i++){const auto& r=party[i];if(r.valid)LOG_INFO(Core_ARM11,"Battle metadata record: slot={} species={} HP={}/{} moves={},{},{},{} PP={}/{},{}/{},{}/{},{}/{}",i,r.species,r.hp,r.max_hp,r.moves[0].id,r.moves[1].id,r.moves[2].id,r.moves[3].id,r.moves[0].pp,r.moves[0].max_pp,r.moves[1].pp,r.moves[1].max_pp,r.moves[2].pp,r.moves[2].max_pp,r.moves[3].pp,r.moves[3].max_pp);}
  last_party=party;
 }
 int actor=-1;if(complete)actor=identity.Observe(party,PokemonXUI::battle_actor_epoch.load(),PokemonXUI::battle_move_event.load());else identity.Reset();
 // This association is experimental: a unique PP spend identifies the actor
 // of the observed action, not every later forced/automatic switch. Opt-in UI.
 if(actor>=0&&PokemonXUI::scene==4&&!PokemonXUI::decision_hidden){const auto& r=party[actor];value[0]=1;value[2]=r.species;for(int m=0;m<4;m++){value[3+m*3]=r.moves[m].id;value[4+m*3]=r.moves[m].pp;value[5+m*3]=r.moves[m].max_pp;}}
 bool changed=false;{std::lock_guard guard(lock);changed=value!=current;current=value;}
 PokemonXUI::native_pp_trusted=PokemonXUI::metadata_overlay&&value[0]!=0;
 if(changed&&(value[0]||value[1]))LOG_INFO(Core_ARM11,"Battle metadata experiment: records={} pp_actor={} species={} moves={},{},{},{} PP={}/{},{}/{},{}/{},{}/{}",value[1],actor,value[2],value[3],value[6],value[9],value[12],value[4],value[5],value[7],value[8],value[10],value[11],value[13],value[14]);
}
}
