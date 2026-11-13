#include "common/file_util.h"
#include "common/logging/backend.h"
#include "core/core.h"
#include "core/loader/loader.h"
#include "core/sha256.h"
#include <iostream>
int main(int argc,char**argv){
 FileUtil::SetUserPath(argv[2]);Common::Log::Initialize("inspect.txt");Common::Log::Start();
 auto loader=Loader::GetLoader(argv[1]);if(!loader)return 2;
 std::vector<u8> code;auto result=loader->ReadCode(code);
 std::cout<<"ReadCode status="<<static_cast<int>(result)<<" size="<<code.size()<<" SHA256="<<hexlith::core::Sha256::Of(code).ToHex()<<std::endl;
 Common::Log::Stop();return result==Loader::ResultStatus::Success?0:3;
}
