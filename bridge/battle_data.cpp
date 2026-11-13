#include "battle_data.h"
#include "program.h"
#include "common/px_screen_interface.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/memory.h"
#include <chrono>
#include <mutex>
namespace PokemonX {
namespace {std::mutex lock;std::array<int,15> current{};std::chrono::steady_clock::time_point next{};
// XY1.5 references: Gen6CTRPluginFrameworkOverhauled Helpers.cpp/PKHeX.cpp,
// commit16fc26e1000919c6e284f171d05fd25fcbfb9c0d. Read-only, hash-gated.
constexpr unsigned State=0x081FB170,Party=0x081FB284;
}
std::array<int,15> BattleDataSnapshot(){std::lock_guard guard(lock);return current;}
void RefreshBattleData(){
 auto now=std::chrono::steady_clock::now();if(now<next)return;next=now+std::chrono::milliseconds(50);
 std::array<int,15> value{};auto program=GetProgram();
 if(program&&program->IsUpdate15()&&PokemonXUI::battle_loaded&&PokemonXUI::scene==4&&!PokemonXUI::decision_hidden){
  auto& memory=Core::System::GetInstance().Memory();
  auto read=[&](unsigned at){return memory.Read32OrNullopt(at);};
  auto state=read(State);
  if(state&&*state==0x40001){
   int pointers=0,valid=0;unsigned candidate=0;
   for(unsigned i=0;i<6;i++){auto p=read(Party+i*4);if(!p)continue;if(*p==0)continue;++pointers;if(*p<0x07000000||*p>=0x0F000000||(*p&3))continue;auto identity=read(*p+0xc),hp=read(*p+0x10);if(!identity||!hp)continue;unsigned species=*identity&65535,max=*identity>>16,cur=*hp&65535;if(species<1||species>721||max<1||max>1000||cur>max)continue;++valid;candidate=*p;}
   value[1]=valid;
   // Slot0 is NOT assumed to be active in a multi-Pokemon party. Until the
   // active-battler mapping is validated, those cases retain live game fields.
   if(pointers==1&&valid==1){
    auto identity=read(candidate+0xc),hp=read(candidate+0x10);bool sane=identity&&hp&&((*hp&65535)>0);value[2]=identity?(*identity&65535):0;
    int occupied=0;for(unsigned i=0;i<4;i++){// Entries can be halfword-aligned; only aligned optional reads are used.
     unsigned at=candidate+0x11c+i*0xe;auto lo=read(at&~3u),hi=read((at&~3u)+4);unsigned word=0;if((at&3)==0){if(lo)word=*lo;else sane=false;}else if(lo&&hi)word=(*lo>>16)|(*hi<<16);else sane=false;
     unsigned id=word&65535,pp=(word>>16)&255,max=word>>24;value[3+i*3]=id;value[4+i*3]=pp;value[5+i*3]=max;
     if(id==0){if(pp||max)sane=false;}else{++occupied;if(id>621||max<1||max>64||pp>max)sane=false;}
    }
    value[0]=sane&&occupied>0;
   }
  }
 }
 bool changed=false;{std::lock_guard guard(lock);changed=value!=current;current=value;}
 PokemonXUI::native_pp_trusted=value[0]!=0;
 if(changed&&(value[0]||value[1]))LOG_INFO(Core_ARM11,"Battle metadata1.5 trusted={} party_candidates={} species={} moves={},{},{},{} PP={}/{},{}/{},{}/{},{}/{}",value[0],value[1],value[2],value[3],value[6],value[9],value[12],value[4],value[5],value[7],value[8],value[10],value[11],value[13],value[14]);
}
}
