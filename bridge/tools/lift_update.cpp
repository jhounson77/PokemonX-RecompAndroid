// Generated game output belongs outside the source tree.
#include "application/analyze.h"
#include "application/analyze_modules.h"
#include "application/lift_emit.h"
#include "application/lift_survey.h"
#include "application/analysis_tables.h"
#include "domain/lift/control_flow.h"
#include "domain/arm/decode_arm.h"
#include <unordered_set>
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
 if(argc==7&&std::string_view(argv[1])=="--emit-main"){
  auto code=Read(argv[2]);auto base=static_cast<std::uint32_t>(std::stoul(argv[3],nullptr,0));
  UnchangedSink sink(argv[6]);auto result=application::EmitLiftedCode({.code=code,.base=base,.functions=Text(argv[4]),.regions=Text(argv[5])},sink);
  if(!result)throw std::runtime_error(result.Failure().Message());std::cout<<application::RenderLiftEmitSummary(result.Value())<<std::endl;return 0;
 }
 if(argc==8&&std::string_view(argv[1])=="--emit-resume-main"){
  auto code=Read(argv[2]);auto base=static_cast<std::uint32_t>(std::stoul(argv[3],nullptr,0));
  auto functions=Text(argv[4]),regions=Text(argv[5]);auto tables=application::ReadLiftTables(functions,regions);if(!tables)throw std::runtime_error(tables.Failure().Message());
  std::vector<std::uint32_t> resumes;std::ifstream input(argv[6]);if(!input)throw std::runtime_error("resume input missing");std::string line;
  while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::size_t end=0;auto pc=std::stoull(line,&end,0);if(end!=line.size()||pc>0xffffffffULL||pc%4||pc<base||pc-base>=code.size())throw std::runtime_error("invalid ARM resume PC");resumes.push_back(static_cast<std::uint32_t>(pc));}
  std::unordered_set<std::uint32_t> remaining(resumes.begin(),resumes.end());std::vector<domain::analysis::FunctionRange> selected;
  auto boundaries=tables.Value().entries;
  const domain::lift::FlowContext flow{.entries=&boundaries,.regions=tables.Value().regions};
  for(const auto& record:tables.Value().functions){if(record.instruction_set!=domain::arm::InstructionSet::Arm)continue;auto fn=domain::lift::BuildFunction({code,base},record.address,record.instruction_set,flow);bool used=false;for(const auto& block:fn.blocks)for(const auto& placed:block.instructions)if(remaining.erase(placed.address))used=true;if(used)selected.push_back(record);if(remaining.empty())break;}
  auto known=tables.Value().functions;unsigned observed_seeds=0;
  while(!remaining.empty()){
   auto pc=*std::min_element(remaining.begin(),remaining.end());auto word=core::ReadU32At(code,pc-base);if(!word||!domain::arm::IsDefined(domain::arm::DecodeArm(word.Value())))throw std::runtime_error("observed PC is not a defined ARM instruction");
   domain::analysis::FunctionRange seed{.address=pc,.instruction_set=domain::arm::InstructionSet::Arm,.source=domain::analysis::Provenance::Observed};known.push_back(seed);selected.push_back(seed);boundaries.insert(pc);++observed_seeds;
   auto fn=domain::lift::BuildFunction({code,base},pc,seed.instruction_set,flow);auto old_size=remaining.size();for(const auto& block:fn.blocks)for(const auto& placed:block.instructions)remaining.erase(placed.address);if(remaining.size()==old_size)throw std::runtime_error("observed seed was not reached");
  }
  auto boundary_text=domain::analysis::RenderFunctions(known);
  auto subset=domain::analysis::RenderFunctions(selected);UnchangedSink sink(argv[7]);auto r=application::EmitLiftedCode({.code=code,.base=base,.functions=subset,.regions=regions,.resume_points=resumes,.known_functions=boundary_text},sink);if(!r)throw std::runtime_error(r.Failure().Message());sink.WriteText("selected-functions.txt",subset);sink.WriteText("known-functions.txt",boundary_text);std::cout<<"Observed new function seeds: "<<observed_seeds<<"\n";std::cout<<application::RenderLiftEmitSummary(r.Value())<<std::endl;return 0;
 }

 if(argc==6&&std::string_view(argv[1])=="--survey"){
  auto code=Read(argv[2]);auto functions=Text(argv[3]),regions=Text(argv[4]);
  auto result=application::SurveyControlFlow({.code=code,.base=static_cast<std::uint32_t>(std::stoul(argv[5],nullptr,0)),.functions=functions,.regions=regions});
  if(!result)throw std::runtime_error(result.Failure().Message());std::cout<<application::RenderLiftSurvey(result.Value());return 0;
 }

 if((argc==6||argc==7)&&(std::string_view(argv[1])=="--emit-module"||std::string_view(argv[1])=="--emit-observed-module")){
  auto bytes=Read(argv[2]);auto header=domain::ctr::ParseCroHeader(bytes);if(!header)throw std::runtime_error(header.Failure().Message());
  auto functions=Text(argv[3]),regions=Text(argv[4]);
  std::vector<std::uint32_t> resumes;
  if(argc==7){std::ifstream f(argv[6]);if(!f)throw std::runtime_error("resume file unreadable");std::string line;while(std::getline(f,line)){if(line.empty()||line[0]=='#')continue;std::size_t end=0;auto value=std::stoull(line,&end,0);if(end!=line.size()||value>0xffffffffULL)throw std::runtime_error("invalid resume address");resumes.push_back(static_cast<std::uint32_t>(value));}}
  if(std::string_view(argv[1])=="--emit-observed-module"){
   auto tables=application::ReadLiftTables(functions,regions);if(!tables)throw std::runtime_error(tables.Failure().Message());auto known=tables.Value().functions;auto boundaries=tables.Value().entries;std::unordered_set<std::uint32_t> remaining(resumes.begin(),resumes.end());
   const domain::lift::FlowContext flow{.entries=&boundaries,.regions=tables.Value().regions};
   for(const auto& record:known){if(record.instruction_set!=domain::arm::InstructionSet::Arm)continue;auto fn=domain::lift::BuildFunction({bytes,0},record.address,record.instruction_set,flow);for(const auto& block:fn.blocks)for(const auto& placed:block.instructions)remaining.erase(placed.address);}
   unsigned seeds=0;
   while(!remaining.empty()){
    auto pc=*std::min_element(remaining.begin(),remaining.end());auto word=core::ReadU32At(bytes,pc);if(pc%4||!word||!domain::arm::IsDefined(domain::arm::DecodeArm(word.Value())))throw std::runtime_error("invalid observed ARM module PC");
    domain::analysis::FunctionRange seed{.address=pc,.instruction_set=domain::arm::InstructionSet::Arm,.source=domain::analysis::Provenance::Observed};known.push_back(seed);boundaries.insert(pc);++seeds;auto fn=domain::lift::BuildFunction({bytes,0},pc,seed.instruction_set,flow);auto old_size=remaining.size();for(const auto& block:fn.blocks)for(const auto& placed:block.instructions)remaining.erase(placed.address);if(old_size==remaining.size())throw std::runtime_error("observed module seed not reached");
   }
   functions=domain::analysis::RenderFunctions(known);std::cout<<"Observed new module seeds: "<<seeds<<"\n";
  }
  application::ModuleSource source{header.Value().name,bytes,functions,regions,resumes};
  UnchangedSink sink(argv[5]);if(std::string_view(argv[1])=="--emit-observed-module")sink.WriteText("selected-functions.txt",functions);auto result=application::EmitLiftedModules({.modules=std::span{&source,1}},sink);
  if(!result)throw std::runtime_error(result.Failure().Message());std::cout<<application::RenderLiftModulesSummary(result.Value())<<std::endl;return 0;
 }

 if(argc<5||argc>7)throw std::runtime_error("code cxi modules out [observed-main-table] [observed-modules-dir]");
 const std::string observed=argc>=6?Text(argv[5]):std::string{};
 std::map<std::string,std::string> observed_modules;std::map<std::string,std::string_view> module_views;
 if(argc==7){for(const auto&e:fs::directory_iterator(argv[6]))if(e.path().extension()==".txt")observed_modules[e.path().stem().string()]=Text(e.path());for(const auto&[name,text]:observed_modules)module_views.emplace(name,text);}
 auto code=Read(argv[1]),cxi=Read(argv[2]),crs=Read(fs::path(argv[3])/"static.crs");
 std::vector<fs::path> paths;for(auto&e:fs::directory_iterator(argv[3]))if(e.path().extension()==".cro")paths.push_back(e.path());std::sort(paths.begin(),paths.end());
 std::vector<std::vector<std::uint8_t>> files;for(auto&p:paths)files.push_back(Read(p));std::vector<core::ByteSpan> spans;for(auto&f:files)spans.push_back(f);
 auto profile=domain::profile::PokemonX();auto u32=[&](std::size_t a){if(a+4>cxi.size())throw std::runtime_error("short header");return std::uint32_t(cxi[a])|(std::uint32_t(cxi[a+1])<<8)|(std::uint32_t(cxi[a+2])<<16)|(std::uint32_t(cxi[a+3])<<24);};
 auto seg=[&](std::size_t a){return domain::profile::SegmentLayout{u32(512+a),u32(512+a+4),u32(512+a+8)};};profile.text=seg(0x10);profile.read_only=seg(0x20);profile.data=seg(0x30);profile.bss_size=u32(512+0x3c);profile.remaster_version=5;
 fs::path out=argv[4];UnchangedSink analysis(out/"analysis"),mainout(out/"main"),moduleout(out/"modules");
 auto a=application::Analyze({.code=code,.static_module=crs,.modules=spans,.profile=&profile,.executed=observed},{},analysis);if(!a)throw std::runtime_error(a.Failure().Message());std::cout<<application::RenderAnalyzeSummary(a.Value())<<std::endl;
 auto m=application::AnalyzeModules({.static_module=crs,.modules=spans,.executed=module_views},{},analysis);if(!m)throw std::runtime_error(m.Failure().Message());std::cout<<application::RenderModulesSummary(m.Value())<<std::endl;
 auto fun=Text(out/"analysis/functions.txt"),reg=Text(out/"analysis/regions.txt");
 auto l=application::EmitLiftedCode({.code=code,.base=profile.text.address,.functions=fun,.regions=reg},mainout);if(!l)throw std::runtime_error(l.Failure().Message());std::cout<<application::RenderLiftEmitSummary(l.Value())<<std::endl;
 std::vector<std::string> names,funcs,regions;for(auto&f:files){auto h=domain::ctr::ParseCroHeader(f);if(!h)throw std::runtime_error(h.Failure().Message());names.push_back(h.Value().name);auto d=out/"analysis/modules"/names.back();funcs.push_back(Text(d/"functions.txt"));regions.push_back(Text(d/"regions.txt"));}
 std::vector<application::ModuleSource> sources;for(std::size_t i=0;i<files.size();i++)sources.push_back({names[i],files[i],funcs[i],regions[i]});
 auto lm=application::EmitLiftedModules({.modules=sources},moduleout);if(!lm)throw std::runtime_error(lm.Failure().Message());std::cout<<application::RenderLiftModulesSummary(lm.Value())<<std::endl;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
