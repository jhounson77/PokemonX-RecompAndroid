"""Run the production readonly reader against mapped synthetic guest memory."""
from pathlib import Path
import tempfile,subprocess
root=Path(__file__).resolve().parents[2]
body='\n'.join(l for l in (root/'bridge/battle_data.cpp').read_text().splitlines() if not l.startswith('#include'))
prefix=r'''
#include <array>
#include <chrono>
#include <mutex>
#include <map>
#include <optional>
#include <memory>
#include <thread>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include "common/px_screen_interface.h"
#include "battle_identity.h"
#define LOG_INFO(...) ((void)0)
namespace Memory{struct MemorySystem{std::map<unsigned,unsigned> data;int reads=0;std::optional<unsigned> Read32OrNullopt(unsigned at){if(at&3)throw std::runtime_error("unaligned read");++reads;auto i=data.find(at);if(i==data.end())return{};return i->second;}};}
namespace Core{struct System{Memory::MemorySystem memory;static System& GetInstance(){static System s;return s;}Memory::MemorySystem& Memory(){return memory;}};}
namespace PokemonX{struct Program{bool update=true;bool IsUpdate15(){return update;}};std::shared_ptr<Program> GetProgram(){static auto p=std::make_shared<Program>();return p;}}
'''
suffix=r'''
int main(){auto&m=Core::System::GetInstance().memory;auto byte=[&](unsigned at,unsigned v){auto aligned=at&~3u,shift=(at&3)*8;m.data[aligned]=(m.data[aligned]&~(255u<<shift))|((v&255)<<shift);};auto half=[&](unsigned at,unsigned v){byte(at,v);byte(at+1,v>>8);};
 auto mon=[&](unsigned p){for(unsigned i=0;i<0x160;i+=4)m.data[p+i]=0;half(p+0xc,656);half(p+0xe,22);half(p+0x10,22);for(unsigned i=0;i<3;i++){half(p+0x11c+i*14,i==0?1:i==1?45:145);byte(p+0x11e + i*14,30);byte(p+0x11f + i*14,35);}};
 unsigned p=0x8200000;mon(p);m.data[0x81fb170]=0x40001;for(unsigned i=0;i<6;i++)m.data[0x81fb284+i*4]=0;m.data[0x81fb284]=p;PokemonXUI::battle_loaded=true;PokemonXUI::scene=4;
 auto refresh=[&]{std::this_thread::sleep_for(std::chrono::milliseconds(55));PokemonX::RefreshBattleData();return PokemonX::BattleDataSnapshot();};
 auto a=refresh();assert(a[0]==0&&a[1]==1); // no assumed actor, even for a singleton
 mon(p+0x1000);m.data[0x81fb288]=p+0x1000;a=refresh();assert(!a[0]&&a[1]==2);
 PokemonXUI::SignalBattleMove(0);a=refresh();assert(!a[0]);byte(p+0x1000+0x11e,29);a=refresh();assert(a[0]&&a[2]==656&&a[4]==29);
 ++PokemonXUI::battle_actor_epoch;a=refresh();assert(!a[0]);
 PokemonXUI::SignalBattleMove(1);a=refresh();byte(p+0x12c,28);a=refresh();assert(a[0]&&a[7]==28); // halfword-aligned move slot
 m.data[0x81fb288]=0x41f4;a=refresh();assert(!a[0]);
 PokemonX::GetProgram()->update=false;m.reads=0;a=refresh();assert(!a[0]&&m.reads==0);
 std::cout<<"PASS real reader on synthetic mapped memory: multi-party PP association, duplicate species, aligned reads, actor epoch, garbage fallback and base version gate\n";}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(prefix+body+suffix);binary=Path(d)/'test';subprocess.run(['g++','-std=c++20','-I'+str(root/'azahar/src'),'-I'+str(root/'bridge'),str(p),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
