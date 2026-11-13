#include <adrenotools/driver.h>
#include <dlfcn.h>
// Kept in a separate DSO so namespace hooks are never initialized when
// the user chooses the normal Android driver.
extern "C" __attribute__((visibility("default")))
void* px_open_custom_driver(const char* hooks,const char* directory,const char* library){
 return adrenotools_open_libvulkan(RTLD_NOW|RTLD_LOCAL,ADRENOTOOLS_DRIVER_CUSTOM,nullptr,hooks,directory,library,nullptr,nullptr);
}
