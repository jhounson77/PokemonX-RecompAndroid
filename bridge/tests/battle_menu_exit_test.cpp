#include "common/px_screen_interface.h"
#include <cassert>
#include <iostream>
using namespace PokemonXUI;
int main(){BattleMenuExit exit;
 assert(!exit.Observe(1,true,false,true)); // Still choosing an item.
 assert(!exit.Observe(1,true,true,true)); // First probe after Bag leaves.
 assert(!exit.Observe(1,true,false,true)); // A newly loaded target-selection menu cancels the exit.
 assert(!exit.Observe(1,true,true,true));
 assert(exit.Observe(1,true,true,true)); // Guest left all auxiliary menus.
 assert(!exit.Observe(2,true,true,true)); // A new presentation invalidates old probes.
 assert(!exit.Observe(2,false,true,true)); // Manual/dual presentation is unaffected.
 // Resident cached helpers cannot hold the presentation on the bottom screen.
 assert(!exit.Observe(3,true,true,true));
 assert(exit.Observe(3,true,true,true));
 // New nested selection invalidates the previous pending return.
 assert(!exit.Observe(4,true,false,true));
 assert(!exit.Observe(4,true,true,true));
 assert(exit.Observe(4,true,true,true));
 mode=0;battle_loaded=true;Commit(3,1);assert(mode==2&&battle_menu_full);
 ReturnFromBattleMenu();assert(mode==0&&!popup_requested&&!battle_menu_full&&scene==0&&decision_hidden);assert(Effective(0)==0);
 Observe(0);assert(Effective(0)==0);Observe(3);assert(!decision_hidden&&Effective(0)==3);
 std::cout<<"PASS item execution restores primary before command redraw; nested target selection cancels exit, stale probes reset, manual mode untouched and next turn rearms\n";
}
