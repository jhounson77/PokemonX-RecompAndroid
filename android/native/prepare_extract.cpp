#include "adapters/filesystem/file_rom_source.h"
#include "adapters/filesystem/directory_blob_sink.h"
#include "application/extract.h"
#include "application/manifest.h"
#include "domain/profile/pokemon_x.h"
#include <iostream>
int main(int argc,char**argv){
 using namespace hexlith;
 if(argc!=4){std::cerr<<"usage: px-extract image output verified-exheader-sha256\n";return 2;}
 const auto parsed=core::ParseDigestHex(argv[3]);if(!parsed)return 2;
 auto profile=domain::profile::PokemonX();profile.exheader_digest=argv[3];
 adapters::filesystem::FileRomSource source(argv[1]);adapters::filesystem::DirectoryBlobSink sink(argv[2]);auto ready=sink.Prepare();if(!ready){std::cerr<<ready.Failure().Message();return 1;}
 auto report=application::Extract(source,sink,{.profile=&profile});if(!report){std::cerr<<report.Failure().Message();return 1;}
 auto saved=sink.WriteText(application::ManifestPath,application::RenderManifest(report.Value()));if(!saved){std::cerr<<saved.Failure().Message();return 1;}
 auto modules=sink.WriteText(application::ModulesPath,application::RenderModules(report.Value()));if(!modules)return 1;
 std::cout<<report.Value().romfs_file_count<<" RomFS files; "<<report.Value().modules.size()<<" CRO modules\n";
}
