"""Run the production cache-initialization body with cancel-at-stage readers."""
from pathlib import Path
import tempfile,subprocess
root=Path(__file__).resolve().parents[2]
s=(root/'azahar/src/video_core/renderer_vulkan/vk_shader_disk_cache.cpp').read_text();a=s.index('void ShaderDiskCache::Init(');b=s.index('{',a);depth=1;end=b+1
while depth:
 depth+=(s[end]=='{')-(s[end]=='}');end+=1
body=s[a:end]
code=r'''
#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <vector>
namespace VideoCore {using DiskResourceLoadCallback=std::function<void()>;}
namespace Settings {struct Values{bool use_disk_shader_cache=true;} values;}
auto MakeThrottledCallback(VideoCore::DiskResourceLoadCallback c){return c;}
struct ShaderDiskCache{
 enum class CacheFileType{VS_CACHE,FS_CACHE,GS_CACHE,PL_CACHE};
 int vs_cache=0,fs_cache=1,gs_cache=2,pl_cache=3,cancel_stage=-2,read=0;std::vector<int> recreated;
 bool Probe(int stage,const std::atomic_bool&stop){++read;if(stage==cancel_stage){const_cast<std::atomic_bool&>(stop)=true;return false;}return cancel_stage!=-1;}
 bool InitVSCache(const std::atomic_bool&s,const VideoCore::DiskResourceLoadCallback&){return Probe(0,s);}
 bool InitFSCache(const std::atomic_bool&s,const VideoCore::DiskResourceLoadCallback&){return Probe(1,s);}
 bool InitGSCache(const std::atomic_bool&s,const VideoCore::DiskResourceLoadCallback&){return Probe(2,s);}
 bool InitPLCache(const std::atomic_bool&s,const VideoCore::DiskResourceLoadCallback&){return Probe(3,s);}
 void RecreateCache(int stage,CacheFileType){recreated.push_back(stage);}
 void Init(const std::atomic_bool&,const VideoCore::DiskResourceLoadCallback&);
};
'''+body+r'''
int main(){for(int stage=0;stage<4;stage++){std::atomic_bool stop=false;ShaderDiskCache c;c.cancel_stage=stage;c.Init(stop,{});assert(stop&&c.recreated.empty()&&c.read==stage+1);}
std::atomic_bool stop=false;ShaderDiskCache c;c.cancel_stage=-1;c.Init(stop,{});assert(c.recreated.size()==4);
ShaderDiskCache valid;valid.Init(stop,{});assert(valid.recreated.empty()&&valid.read==4);
std::cout<<"PASS production cache init: cancel in each read stage preserves existing cache; genuine invalid data still recreated\n";}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(code);binary=Path(d)/'test';subprocess.run(['g++','-std=c++20',str(p),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
