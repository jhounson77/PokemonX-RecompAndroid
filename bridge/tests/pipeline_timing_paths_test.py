from pathlib import Path
import subprocess,tempfile,sys
root=Path(__file__).resolve().parents[2]
old=Path(sys.argv[1]).read_text();new=(root/'azahar/src/video_core/renderer_vulkan/vk_pipeline_cache.cpp').read_text()
def prefix(s,name):
 a=s.index('bool PipelineCache::BindPipeline(');b=s.index('    const bool is_dirty',a)
 return s[a:b].replace('PipelineCache::',name+'::')+' return true;\n}\n'
def wait(s,name):
 a=s.index('        if (pipeline_dirty) {',s.index('bool PipelineCache::BindPipeline('));p=s.index('{',a);depth=1;i=p+1
 while depth:
  depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return 'void '+name+'(bool pipeline_dirty,GraphicsPipeline* pipeline,Cmd&cmdbuf){'+s[a:i]+'}\n'
code=r'''
#include "common/px_host_timings.h"
#include <array>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
using u32=unsigned;constexpr unsigned MAX_SHADER_STAGES=3;
#define MICROPROFILE_SCOPE(...) ((void)0)
struct PipelineInfo{struct {std::array<int,3>shader_ids{};}state;};
struct GraphicsPipeline{bool done,result;std::vector<std::string>*trace;
 bool IsDone(){trace->push_back("done");return done;}bool TryBuild(bool){trace->push_back("try");return result;}void WaitDone(){trace->push_back("wait");done=true;}unsigned Handle(){trace->push_back("handle");return 7;}};
struct Cache{GraphicsPipeline*p;std::vector<std::string>*trace;GraphicsPipeline*GetPipeline(PipelineInfo&){trace->push_back("lookup");return p;}};
struct Base{std::array<int,3>shader_hashes{1,2,3};Cache*curr_disk_cache;};
struct Old:Base{bool BindPipeline(PipelineInfo&,bool);};struct New:Base{bool BindPipeline(PipelineInfo&,bool);};
namespace vk{enum class PipelineBindPoint{eGraphics};}
struct Cmd{std::vector<std::string>*trace;void bindPipeline(vk::PipelineBindPoint,unsigned){trace->push_back("bind");}};
'''+prefix(old,'Old')+prefix(new,'New')+wait(old,'OldWait')+wait(new,'NewWait')+r'''
int main(){for(bool done:{false,true})for(bool result:{false,true})for(bool wait:{false,true}){
 std::vector<std::string>a,b;GraphicsPipeline pa{done,result,&a},pb{done,result,&b};Cache ca{&pa,&a},cb{&pb,&b};Old before;New after;before.curr_disk_cache=&ca;after.curr_disk_cache=&cb;PipelineInfo ia,ib;
 PokemonXTiming::TakeStages();bool x=before.BindPipeline(ia,wait),y=after.BindPipeline(ib,wait);assert(x==y&&a==b&&ia.state.shader_ids==ib.state.shader_ids);
#ifdef PX_PROFILE_HOST_TIMINGS
 auto stages=PokemonXTiming::TakeStages();assert(stages[unsigned(PokemonXTiming::Stage::PipelineLookup)].calls==1);assert(stages[unsigned(PokemonXTiming::Stage::PipelineTryBuild)].calls==unsigned(!done));
#endif
 }
 for(bool dirty:{false,true})for(bool done:{false,true}){
 std::vector<std::string>a,b;GraphicsPipeline pa{done,true,&a},pb{done,true,&b};Cmd ca{&a},cb{&b};PokemonXTiming::TakeStages();OldWait(dirty,&pa,ca);NewWait(dirty,&pb,cb);assert(a==b);
#ifdef PX_PROFILE_HOST_TIMINGS
 auto stages=PokemonXTiming::TakeStages();assert(stages[unsigned(PokemonXTiming::Stage::PipelineWorkerWait)].calls==unsigned(dirty&&!done));
#endif
 }
 std::cout<<"PASS production pipeline lookup/build/early-return and worker wait/bind traces unchanged\n";}
'''
with tempfile.TemporaryDirectory()as d:
 p=Path(d)/'test.cpp';p.write_text(code)
 for profile in [False,True]:
  out=Path(d)/('profile'if profile else'plain')
  subprocess.run(['g++','-std=c++20','-O2','-I'+str(root/'azahar/src'),*(['-DPX_PROFILE_HOST_TIMINGS=1']if profile else[]),str(p),'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
