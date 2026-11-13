// Generated game output belongs outside the source tree.
#include "application/analyze.h"
#include "application/analyze_modules.h"
#include "application/lift_emit.h"
#include "adapters/filesystem/directory_blob_sink.h"
#include "domain/profile/pokemon_x.h"
#include "domain/ctr/cro.h"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
namespace fs=std::filesystem;
using namespace hexlith;
std::vector<std::uint8_t> Read(const fs::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("read failed "+p.string());return{std::istreambuf_iterator<char>(f),{}};}
std::string Text(const fs::path&p){auto b=Read(p);return{b.begin(),b.end()};}
class UnchangedSink final:public ports::BlobSink {
 fs::path root;adapters::filesystem::DirectoryBlobSink delegate;
public:
 explicit UnchangedSink(fs::path p):root(p),delegate(p){}
 core::Status Write(std::string_view p,core::ByteSpan data)override{auto file=root/std::string(p);if(fs::is_regular_file(file)&&fs::file_size(file)==data.size()){auto old=Read(file);if(std::equal(old.begin(),old.end(),data.begin()))return{};}return delegate.Write(p,data);}
 core::Status WriteText(std::string_view p,std::string_view text)override{return Write(p,{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});}
 core::Status MakeDirectory(std::string_view p)override{return delegate.MakeDirectory(p);}
 std::string Describe()const override{return delegate.Describe();}
};
int main(int argc,char**argv){try{
 if(argc!=5)throw std::runtime_error("code cxi modules out");
 auto code=Read(argv[1]),cxi=Read(argv[2]),crs=Read(fs::path(argv[3])/"static.crs");
 std::vector<fs::path> paths;for(auto&e:fs::directory_iterator(argv[3]))if(e.path().extension()==".cro")paths.push_back(e.path());std::sort(paths.begin(),paths.end());
 std::vector<std::vector<std::uint8_t>> files;for(auto&p:paths)files.push_back(Read(p));std::vector<core::ByteSpan> spans;for(auto&f:files)spans.push_back(f);
 auto profile=domain::profile::PokemonX();auto u32=[&](std::size_t a){if(a+4>cxi.size())throw std::runtime_error("short header");return std::uint32_t(cxi[a])|(std::uint32_t(cxi[a+1])<<8)|(std::uint32_t(cxi[a+2])<<16)|(std::uint32_t(cxi[a+3])<<24);};
 auto seg=[&](std::size_t a){return domain::profile::SegmentLayout{u32(512+a),u32(512+a+4),u32(512+a+8)};};profile.text=seg(0x10);profile.read_only=seg(0x20);profile.data=seg(0x30);profile.bss_size=u32(512+0x3c);profile.remaster_version=5;
 fs::path out=argv[4];UnchangedSink analysis(out/"analysis"),mainout(out/"main"),moduleout(out/"modules");
 auto a=application::Analyze({.code=code,.static_module=crs,.modules=spans,.profile=&profile},{},analysis);if(!a)throw std::runtime_error(a.Failure().Message());std::cout<<application::RenderAnalyzeSummary(a.Value())<<std::endl;
 auto m=application::AnalyzeModules({.static_module=crs,.modules=spans},{},analysis);if(!m)throw std::runtime_error(m.Failure().Message());std::cout<<application::RenderModulesSummary(m.Value())<<std::endl;
 auto fun=Text(out/"analysis/functions.txt"),reg=Text(out/"analysis/regions.txt");
 auto l=application::EmitLiftedCode({.code=code,.base=profile.text.address,.functions=fun,.regions=reg},mainout);if(!l)throw std::runtime_error(l.Failure().Message());std::cout<<application::RenderLiftEmitSummary(l.Value())<<std::endl;
 std::vector<std::string> names,funcs,regions;for(auto&f:files){auto h=domain::ctr::ParseCroHeader(f);if(!h)throw std::runtime_error(h.Failure().Message());names.push_back(h.Value().name);auto d=out/"analysis/modules"/names.back();funcs.push_back(Text(d/"functions.txt"));regions.push_back(Text(d/"regions.txt"));}
 std::vector<application::ModuleSource> sources;for(std::size_t i=0;i<files.size();i++)sources.push_back({names[i],files[i],funcs[i],regions[i]});
 auto lm=application::EmitLiftedModules({.modules=sources},moduleout);if(!lm)throw std::runtime_error(lm.Failure().Message());std::cout<<application::RenderLiftModulesSummary(lm.Value())<<std::endl;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
