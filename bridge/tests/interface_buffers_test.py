from pathlib import Path
import subprocess,tempfile,sys
root=Path(__file__).resolve().parents[2]
old=Path(sys.argv[1]).read_text();new=(root/'bridge/android_host.cpp').read_text()
def function(s,name):
 a=s.index('extern "C"',s.index(name)-130);b=s.index('\n',s.index(name));return s[a:b]
# These JNI entry points are deliberately single-line definitions in the bridge.
def pick(s,name):return next(line for line in s.splitlines() if 'JNICALL Java_org_pokemonx_recomp_MainActivity_'+name+'(' in line)
code=r'''
#include "common/px_screen_interface.h"
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>
static unsigned long allocations=0;
void* operator new(std::size_t n){++allocations;if(auto*p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}
using jint=int;using jfloat=float;using jclass=int;using jfloatArray=std::vector<float>*;using jintArray=std::vector<int>*;
#define JNIEXPORT
#define JNICALL
struct JNIEnv{
 int GetArrayLength(jfloatArray v){return v->size();}int GetArrayLength(jintArray v){return v->size();}
 jfloatArray NewFloatArray(int n){return new std::vector<float>(n);}jintArray NewIntArray(int n){return new std::vector<int>(n);}
 void SetFloatArrayRegion(jfloatArray v,int at,int n,const float*p){std::copy_n(p,n,v->begin()+at);}void SetIntArrayRegion(jintArray v,int at,int n,const int*p){std::copy_n(p,n,v->begin()+at);}
};
namespace PokemonX {std::array<int,15> values{1,1,25,1,2,3};auto BattleDataSnapshot(){return values;}}
using PokemonXUI::mode;
'''
for s,prefix in [(old,'old'),(new,'now')]:
 for name in ['nativeBattleRegions','nativeMoveStats']:
  code+=pick(s,name).replace('Java_org_pokemonx_recomp_MainActivity_'+name,prefix+name)+'\n'
code+=r'''
int main(){JNIEnv env;std::vector<float>out(34);std::vector<int>stats(15);PokemonXUI::automatic=false;PokemonXUI::metadata_overlay=true;
 for(int m=0;m<6;m++){mode=m;auto*previous=oldnativeBattleRegions(&env,0,1920,1080);nownativeBattleRegions(&env,0,1920,1080,&out);assert(std::equal(previous->begin(),previous->end(),out.begin()));for(unsigned i=previous->size();i<out.size();i++)assert(out[i]==0);delete previous;}
 nownativeBattleRegions(&env,0,400,240,nullptr);std::vector<float>shortBuffer(3);nownativeBattleRegions(&env,0,400,240,&shortBuffer);assert(shortBuffer[0]==0);
 for(bool enabled:{false,true}){PokemonXUI::move_cards=enabled;auto*p=oldnativeMoveStats(&env,0);nownativeMoveStats(&env,0,&stats);assert(*p==stats);delete p;}
 PokemonXUI::metadata_overlay=false;nownativeMoveStats(&env,0,&stats);assert(stats[0]==0&&stats[2]==25);
 mode=3;unsigned long begin=allocations;for(int i=0;i<10000;i++){auto*p=oldnativeBattleRegions(&env,0,1920,1080);delete p;}auto before=allocations-begin;begin=allocations;for(int i=0;i<10000;i++)nownativeBattleRegions(&env,0,1920,1080,&out);auto after=allocations-begin;assert(before>0&&after==0);
 std::cout<<"PASS production JNI geometry/focus/stats byte-equivalence, empty tail, short/null bounds; 10000 region polls allocations before="<<before<<" after="<<after<<" (host allocation count, not phone FPS)\n";
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(code);out=Path(d)/'test';subprocess.run(['g++','-std=c++20','-O2','-I'+str(root/'azahar/src'),str(p),'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
