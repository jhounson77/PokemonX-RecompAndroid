#include "support/synthetic_ctr_image.h"
#include "support/synthetic_romfs.h"
#include "domain/ctr/ncsd.h"
#include <fstream>
int main(int argc,char**argv){
 auto spec=hexlith::test::DefaultImageSpec();
 std::vector<std::uint8_t> code((spec.text.pages+spec.read_only.pages+spec.data.pages)*4096);
 const std::uint32_t words[]={0xef00003d,0xef000003};
 for(unsigned i=0;i<2;++i)for(unsigned b=0;b<4;++b)code[i*4+b]=words[i]>>(8*b);
 const std::string msg="AZ_NATIVE_SVC_OK\n";std::copy(msg.begin(),msg.end(),code.begin()+0x200);
 spec.exefs_files={{.name=".code",.data=code}};spec.romfs=hexlith::test::BuildSyntheticRomFs(hexlith::test::DefaultRomFsSpec());
 auto image=hexlith::test::BuildSyntheticImage(spec).bytes;
 auto header=hexlith::domain::ctr::ParseNcsdHeader(image);
 auto partition=hexlith::domain::ctr::PartitionContents(image,header.Value(),0);
 std::ofstream out(argv[1],std::ios::binary);out.write(reinterpret_cast<const char*>(partition.Value().data()),partition.Value().size());
}
