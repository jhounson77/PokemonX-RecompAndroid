"""Exercise production module callbacks with synthetic module identities."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
program = (root / 'bridge/program.cpp').read_text()
callbacks = program[program.index('void Program::Loaded('):program.rfind('\n}')]
harness = r'''
#include "common/px_screen_interface.h"
#include <cassert>
#include <set>
#include <string>
#include <cstdint>
#include <iostream>
#define LOG_INFO(...) ((void)0)
namespace hexlith::domain::lift::runtime {
struct LoadedModules {
 enum class Outcome { Lifted, DigestDiffers, NotLifted };
 Outcome Loaded(const std::string&,std::uint32_t,std::uint32_t,std::uint32_t,const std::string&){return Outcome::Lifted;}
 void Unloaded(const std::string&,std::uint32_t){}
};
}
namespace PokemonX {
struct ModuleImage {std::string name,digest;std::uint32_t offset=0,size=16;};
class Program {
 std::set<std::uint32_t> submenu_bases_;
 hexlith::domain::lift::runtime::LoadedModules loaded_;
 public:
 void Loaded(const ModuleImage&,std::uint32_t);
 void Unloaded(const std::string&,std::uint32_t);
};
'''
tests = r'''
}
int main(){
 using namespace PokemonXUI;
 for(const auto* root: {"DllBag","DllBattlePartySelect","DllPokeList","DllPoke3Select"}){
  PokemonX::Program p;
  mode=0;popup_requested=false;battle_menu_full=false;
  p.Loaded({"DllBattle"},0x1000);
  Commit(3,root==std::string("DllBag")?1:3);
  p.Loaded({"DllBag"},0x2000);
  p.Loaded({"DllPokeViewer"},0x3000);
  p.Loaded({root},0x4000);
  p.Unloaded(root,0x4000);
  assert(submenu_loaded); // Cached Bag/viewer are still resident.
  assert(battle_menu_exit);
  BattleMenuExit exit;
  assert(!exit.Observe(generation,true,battle_menu_exit,submenu_loaded));
  // A newly loaded target menu must cancel this pending return.
  p.Loaded({"DllPokeList"},0x5000);
  assert(!battle_menu_exit);
  assert(!exit.Observe(generation,true,battle_menu_exit,submenu_loaded));
  p.Unloaded("DllPokeList",0x5000);
  assert(!exit.Observe(generation,true,battle_menu_exit,submenu_loaded));
  assert(exit.Observe(generation,true,battle_menu_exit,submenu_loaded));
  ReturnFromBattleMenu();
  assert(mode==0&&!popup_requested&&!battle_menu_full);
  assert(Effective(0)==0);
  // Closing a summary viewer alone must not commit an item/switch action.
  mode=0;Commit(3,3);battle_menu_exit=false;
  p.Unloaded("DllPokeViewer",0x3000);
  assert(!battle_menu_exit&&battle_menu_full);
  p.Unloaded("DllBattle",0x1000);
  assert(!battle_loaded&&!popup_requested&&mode==0);
 }
 // A menu loaded before battle is reused without a new load callback.
 PokemonX::Program reused;
 battle_loaded=false;popup_requested=false;mode=0;
 reused.Loaded({"DllBag"},0x6000);
 reused.Loaded({"DllBattle"},0x1000);
 Commit(3,1);
 reused.Unloaded("DllBag",0x6000);
 assert(battle_menu_exit);
 std::cout<<"PASS actual module callbacks: item/party roots, resident helpers, nested targets, viewer-only close, battle teardown\n";
}
'''
with tempfile.TemporaryDirectory() as folder:
 source = Path(folder)/'lifecycle.cpp'
 binary = Path(folder)/'lifecycle'
 source.write_text(harness+callbacks+tests)
 subprocess.run(['g++','-std=c++20','-O2','-I'+str(root/'azahar/src'),str(source),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
