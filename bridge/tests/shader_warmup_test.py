"""Replay the startup race using unchanged production method bodies and test workers.
No Vulkan driver claims: this verifies startup scheduling, retry and cancellation.
"""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
def function(path,name):
 s=path.read_text();a=s.index(name);start=s.index('{',a);depth=1;b=start+1
 while depth:
  depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
warm=function(root/'azahar/src/video_core/renderer_vulkan/vk_shader_disk_cache.cpp','std::size_t ShaderDiskCache::WarmPipelines')
load=function(root/'azahar/src/video_core/renderer_vulkan/vk_pipeline_cache.cpp','void PipelineCache::LoadCache')
code=r'''
#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#define LOG_INFO(...) ((void)0)
namespace VideoCore {enum class LoadCallbackStage{Build};using DiskResourceLoadCallback=std::function<void(LoadCallbackStage,std::size_t,std::size_t,const std::string&)>;}
struct Worker{std::vector<std::function<void()>> jobs;int drained=0;void Queue(std::function<void()> f){jobs.push_back(f);}void WaitForRequests(){for(auto&f:jobs){f();++drained;}jobs.clear();}};
struct GraphicsPipeline{bool* shaders;Worker* worker;bool done=false,pending=false;int requests=0;bool IsDone(){return done;}bool TryBuild(bool wait){if(pending)return wait;if(!*shaders&&!wait)return false;assert(*shaders);pending=true;++requests;worker->Queue([this]{done=true;pending=false;});return wait;}};
struct ShaderDiskCache{std::unordered_map<unsigned,std::unique_ptr<GraphicsPipeline>> graphics_pipelines;std::size_t WarmPipelines(const std::atomic_bool&);};
struct PipelineCache{
 Worker shader_workers,pipeline_workers;bool ready=false;int cancel_mode=0;
 std::vector<std::shared_ptr<ShaderDiskCache>> disk_caches;
 void LoadDriverPipelineDiskCache(const std::atomic_bool&,const VideoCore::DiskResourceLoadCallback&){}
 void LoadDiskCache(const std::atomic_bool& stop,const VideoCore::DiskResourceLoadCallback&){
  auto cache=std::make_shared<ShaderDiskCache>();disk_caches.push_back(cache);if(stop)return;
  shader_workers.Queue([this,&stop]{ready=true;if(cancel_mode==2)const_cast<std::atomic_bool&>(stop)=true;});
  for(unsigned i=0;i<3;i++){auto p=std::make_unique<GraphicsPipeline>();p->shaders=&ready;p->worker=&pipeline_workers;assert(!p->TryBuild(false));cache->graphics_pipelines.emplace(i,std::move(p));}
 }
 void LoadCache(const std::atomic_bool&,const VideoCore::DiskResourceLoadCallback&);
};
'''+warm+'\n'+load+r'''
int main(){
 for(int scenario=0;scenario<3;scenario++){
  std::atomic_bool stop{scenario==1};PipelineCache c;c.cancel_mode=scenario;int reports=0;
  c.LoadCache(stop,[&](auto,std::size_t current,std::size_t total,auto&){++reports;if(current==total)for(auto&cache:c.disk_caches)for(auto&[id,p]:cache->graphics_pipelines)if(!stop)assert(p->done);});
  assert(c.shader_workers.jobs.empty()&&c.pipeline_workers.jobs.empty());assert(reports==2);
  for(auto&cache:c.disk_caches)for(auto&[id,p]:cache->graphics_pipelines){assert(p->done==(scenario==0));assert(p->requests==(scenario==0?1:0));}
 }
 std::cout<<"PASS production startup methods: pending shader/pipeline retry, completion before return, cancellation before and during load, empty cache\n";
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(code);binary=Path(d)/'test'
 subprocess.run(['g++','-std=c++20','-O1',str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
