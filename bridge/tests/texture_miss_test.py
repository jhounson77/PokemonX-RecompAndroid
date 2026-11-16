from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'azahar/src/video_core/custom_textures/custom_tex_manager.cpp').read_text()
a=s.index('Material* CustomTexManager::GetMaterial(');b=s.index('\nbool CustomTexManager::Decode(',a)
body=s[a:b]
code=r'''
#include <unordered_map>
#include <memory>
#include <cassert>
#include <iostream>
using u64=unsigned long long;
static unsigned formatted_logs=0;
#define LOG_WARNING(...) (++formatted_logs)
#define LOG_DEBUG(...) ((void)0)
struct Material {int value=42;};
struct CustomTexManager {std::unordered_map<u64,std::unique_ptr<Material>> material_map;Material* GetMaterial(u64);};
'''+body+r'''
int main(){CustomTexManager manager;manager.material_map[123]=std::make_unique<Material>();auto*expected=manager.material_map[123].get();
for(unsigned i=0;i<10000;i++){assert(manager.GetMaterial(777)==nullptr);assert(manager.GetMaterial(123)==expected);}
assert(manager.material_map.size()==1);assert(formatted_logs==0);
// A newly registered replacement must still become visible after prior misses.
manager.material_map[777]=std::make_unique<Material>();assert(manager.GetMaterial(777)==manager.material_map[777].get());
std::cout<<"PASS sparse texture pack: 10000 misses produce no warning formatting, present/replaced material remains visible, no negative-cache growth\n";}
'''
with tempfile.TemporaryDirectory()as d:
 p=Path(d)/'test.cpp';p.write_text(code);out=Path(d)/'test'
 subprocess.run(['g++','-std=c++20','-O2',str(p),'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
