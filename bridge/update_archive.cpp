#include <jni.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>

extern "C" JNIEXPORT jstring JNICALL Java_org_pokemonx_recomp_UpdateInstaller_unpackRar(JNIEnv* env,jclass,jstring source,jstring destination){
 const char* s=env->GetStringUTFChars(source,nullptr);const char* d=env->GetStringUTFChars(destination,nullptr);
 std::string input=s,output=d,error;env->ReleaseStringUTFChars(source,s);env->ReleaseStringUTFChars(destination,d);
 archive* a=archive_read_new();archive_read_support_filter_none(a);archive_read_support_format_rar(a);archive_read_support_format_rar5(a);
 int fd=-1,count=0,found=0;std::int64_t total=0;
 auto fail=[&](const char* fallback){const char* text=archive_error_string(a);error=text?text:fallback;};
 if(archive_read_open_filename(a,input.c_str(),65536)!=ARCHIVE_OK)fail("Não foi possível abrir o RAR.");
 archive_entry* e=nullptr;std::array<char,65536> buffer{};
 while(error.empty()){
  int status=archive_read_next_header(a,&e);if(status==ARCHIVE_EOF)break;if(status!=ARCHIVE_OK){fail("RAR inválido.");break;}
  auto size=archive_entry_size(e);if(++count>32||size<0||size>64*1024*1024||(total+=size)>128*1024*1024){error="O RAR excede os limites de uma atualização suportada.";break;}
  const char* path=archive_entry_pathname(e);std::string name=path?path:"";std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
  if(name.size()<4||name.substr(name.size()-4)!=".cia"){archive_read_data_skip(a);continue;}
  if(++found!=1||archive_entry_filetype(e)!=AE_IFREG||archive_entry_is_encrypted(e)){error="O RAR deve conter somente um CIA sem senha.";break;}
  fd=open(output.c_str(),O_WRONLY|O_TRUNC|O_CREAT,0600);if(fd<0){error="Não foi possível criar o CIA temporário.";break;}
  std::int64_t done=0;for(;;){auto n=archive_read_data(a,buffer.data(),buffer.size());if(n==0)break;if(n<0){fail("Falha de integridade no RAR.");break;}done+=n;if(done>size){error="Tamanho descomprimido inválido.";break;}ssize_t at=0;while(at<n){auto written=write(fd,buffer.data()+at,n-at);if(written<=0){error="Não foi possível gravar o CIA temporário.";break;}at+=written;}if(!error.empty())break;}
  if(error.empty()&&done!=size)error="CIA truncado no RAR.";if(error.empty()&&fsync(fd)!=0)error="Falha ao confirmar o CIA temporário.";close(fd);fd=-1;
 }
 if(fd>=0)close(fd);archive_read_free(a);if(error.empty()&&found!=1)error="Nenhum CIA foi encontrado no RAR.";
 if(!error.empty()){unlink(output.c_str());return env->NewStringUTF(error.c_str());}return nullptr;
}
