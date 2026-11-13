from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'azahar/src/video_core/renderer_vulkan/renderer_vulkan.cpp').read_text()
a=s.index('void RendererVulkan::PrepareRendertarget()');b=s.index('\nvoid RendererVulkan::PrepareDraw',a);body=s[a:b]
code=r'''
#include <array>
#include <cassert>
#include <iostream>
using u32=unsigned;
namespace Settings{enum class StereoRenderOption{Off,On};struct Value{StereoRenderOption value=StereoRenderOption::Off;StereoRenderOption GetValue(){return value;}};struct{Value render_3d;}values;}
struct Fill{bool is_enabled=false;int AsVector()const{return 0;}};
struct Config{int width=400,height=240,color_format=0;};
struct Texture{int width=0,height=0,format=0,image_view=0;};
struct Info{Texture texture;int image_view=0;std::array<float,4>texcoords{};};
struct RendererVulkan{
 struct{struct{std::array<Config,2>framebuffer_config;}regs;struct{Fill color_fill_top,color_fill_bottom;}regs_lcd;}pica;
 std::array<Info,3>screen_infos;int loads=0,fills=0,configs=0;int next=10;
 void FillScreen(int,Texture&t){++fills;if(!t.image_view)t.image_view=++next;}
 void ConfigureFramebufferTexture(Texture&t,const Config&f){++configs;t.width=f.width;t.height=f.height;t.format=f.color_format;t.image_view=++next;}
 void LoadFBToScreenInfo(const Config&,Info&i,bool right){++loads;i.image_view=i.texture.image_view;i.texcoords={float(right),1,2,3};}
 void PrepareRendertarget();
};
'''+body+r'''
int main(){RendererVulkan r;r.pica.regs.framebuffer_config[1].width=320;
 r.PrepareRendertarget();assert(r.loads==2&&r.configs==2);assert(r.screen_infos[1].image_view==r.screen_infos[0].image_view);assert(r.screen_infos[1].texture.image_view==0);assert(r.screen_infos[2].texture.width==320);
 Settings::values.render_3d.value=Settings::StereoRenderOption::On;r.loads=0;r.PrepareRendertarget();assert(r.loads==3&&r.configs==3);auto owned=r.screen_infos[1].texture.image_view;assert(owned!=0&&r.screen_infos[1].image_view!=r.screen_infos[0].image_view);
 Settings::values.render_3d.value=Settings::StereoRenderOption::Off;r.loads=0;r.PrepareRendertarget();assert(r.loads==2&&r.screen_infos[1].texture.image_view==owned);assert(r.screen_infos[1].image_view==r.screen_infos[0].image_view);
 r.pica.regs_lcd.color_fill_top.is_enabled=true;r.pica.regs_lcd.color_fill_bottom.is_enabled=true;r.fills=0;r.loads=0;r.PrepareRendertarget();assert(r.fills==2&&r.loads==0);assert(r.screen_infos[1].image_view==r.screen_infos[0].image_view);
 std::cout<<"PASS production preparation: mono2 instead of3 display loads, stereo3, bottom screen preserved, valid alias descriptors, ownership and color-fill transitions\n";
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(code);out=Path(d)/'test';subprocess.run(['g++','-std=c++20','-O2',str(p),'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
