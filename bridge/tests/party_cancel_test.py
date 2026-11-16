"""Exercise the production interface consumer without GPU waits or game data."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'azahar/src/video_core/renderer_vulkan/renderer_vulkan.cpp').read_text()
a=s.index('void RendererVulkan::ConsumeInterfaceProbe(');b=s.index('\nvoid RendererVulkan::ProbeBottomInterface()',a)
body=s[a:b]
prefix=r'''
#include "common/px_screen_interface.h"
#include <array>
#include <cassert>
#include <iostream>
#define LOG_INFO(...) ((void)0)
using u8=unsigned char;
struct RendererVulkan {PokemonXUI::CommandSignature ui_commands;PokemonXUI::BattleMenuExit ui_menu_exit;void ConsumeInterfaceProbe(const std::array<u8,80*60*4>&);};
'''
test=r'''
int main(){using namespace PokemonXUI;std::array<u8,80*60*4> commands{},party{},fake{};
 auto fill=[](auto& p,int r,int g,int b){for(int i=0;i<80*60;i++){p[i*4]=b;p[i*4+1]=g;p[i*4+2]=r;p[i*4+3]=255;}};
 auto rect=[](auto& p,int x0,int y0,int x1,int y1,int r,int g,int b){for(int y=y0/4;y<y1/4;y++)for(int x=x0/4;x<x1/4;x++){auto q=p.data()+(y*80+x)*4;q[0]=b;q[1]=g;q[2]=r;}};
 fill(commands,200,200,200);rect(commands,100,40,220,132,220,15,15);rect(commands,240,184,316,240,15,220,15);rect(commands,120,200,196,240,15,80,220);
 fill(party,110,110,110);fake=commands;rect(fake,0,0,320,160,220,5,5);
 assert(Classify(commands.data(),80,60)==3&&!MenuImageVisible(party.data(),80,60));
 auto open=[&](RendererVulkan& renderer){mode=0;automatic=true;battle_loaded=true;popup_requested=false;decision_hidden=false;scene=3;renderer.ui_commands.Learn(commands.data(),80,60);Commit(3,3);assert(mode==2&&battle_menu_full);};
 RendererVulkan renderer;open(renderer);popup_close_requested=true;
 renderer.ConsumeInterfaceProbe(commands);assert(mode==2);renderer.ConsumeInterfaceProbe(commands);assert(mode==0&&!popup_requested&&!decision_hidden&&Effective(0)==3);
 // A dark/flat party screen must count as having left the original commands.
 open(renderer);renderer.ConsumeInterfaceProbe(party);assert(popup_seen_menu);
 renderer.ConsumeInterfaceProbe(commands);assert(mode==2);renderer.ConsumeInterfaceProbe(commands);assert(mode==0&&!popup_requested);
 // Back inside nested menus does not close presentation until verified commands.
 open(renderer);popup_close_requested=true;renderer.ConsumeInterfaceProbe(party);renderer.ConsumeInterfaceProbe(party);assert(mode==2);
 renderer.ConsumeInterfaceProbe(fake);renderer.ConsumeInterfaceProbe(fake);assert(mode==2);
 renderer.ConsumeInterfaceProbe(commands);renderer.ConsumeInterfaceProbe(commands);assert(mode==0);
 // Executing a switch/item still restores the top for the animation.
 open(renderer);battle_menu_exit=true;renderer.ConsumeInterfaceProbe(party);renderer.ConsumeInterfaceProbe(party);assert(mode==0&&decision_hidden);
 std::cout<<"PASS party cancellation, fast Back, dark party screen, nested Back, color false positives and completed actions\n";
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(prefix+body+test);binary=Path(d)/'test'
 subprocess.run(['g++','-std=c++20','-O2','-I'+str(root/'azahar/src'),str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
