#include <jni.h>
#include "display_layout.h"
#include "screen_blit.h"
#ifdef PX_CUSTOM_VULKAN
#include <dlfcn.h>
#endif
#ifdef PX_VULKAN
#include "adapters/vulkan/vulkan_backend.h"
#endif
#include <android/native_window_jni.h>
#include <android/log.h>
#include <atomic>
#include <mutex>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include "application/boot.h"
#include "application/live_display.h"
#include "application/gpu_render.h"
#include "application/save_storage.h"
#include "adapters/filesystem/file_rom_source.h"
#include "adapters/filesystem/directory_tree_store.h"
#include "adapters/lifted/lifted_library.h"
#include "adapters/lifted/modules_library.h"
#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include "common/lifted_modules.h"
#include "ports/screen_sink.h"
#include "ports/input_source.h"
JavaVM* PxJavaVM=nullptr;
using namespace hexlith;
namespace {
std::mutex surface_mutex,log_mutex;
ANativeWindow* window=nullptr;
std::atomic<bool> stopping{false},paused{false},busy{false};
std::atomic<int> renderer{0};
std::atomic<int> mode{0},buttons{0},cx{0},cy{0},tx{-1},ty{-1};
std::string log_path,driver_dir,driver_library;
std::atomic<int> render_scale{1};
std::atomic<bool> resident_textures{true};
std::atomic<std::uint64_t> presented_frames{0},emulated_frames{0};
int crash_fd=-1;
void Crash(int signal){const char msg[]="\nNATIVE CRASH: export this log after restarting.\n";if(crash_fd>=0)write(crash_fd,msg,sizeof(msg)-1);std::signal(signal,SIG_DFL);raise(signal);}
void Log(const std::string&s){std::lock_guard lock(log_mutex);__android_log_write(ANDROID_LOG_INFO,"PokemonX",s.c_str());std::ofstream f(log_path,std::ios::app);f<<s<<'\n';}
std::string String(JNIEnv*e,jstring text){auto*p=e->GetStringUTFChars(text,nullptr);std::string s=p;e->ReleaseStringUTFChars(text,p);return s;}
void Status(JNIEnv*e,jobject activity,const std::string&text){auto cls=e->GetObjectClass(activity);auto method=e->GetMethodID(cls,"runtimeStatus","(Ljava/lang/String;)V");auto s=e->NewStringUTF(text.c_str());e->CallVoidMethod(activity,method,s);e->DeleteLocalRef(s);e->DeleteLocalRef(cls);if(e->ExceptionCheck())e->ExceptionClear();}
using pxandroid::Rect;
Rect MainRect(int w,int h){return pxandroid::Fit(w,h,400,240);}
Rect BottomRect(int w,int h,int){return pxandroid::Fit(w,h,320,240);}
struct Display final:ports::ScreenSink,ports::InputSource {
 std::uint64_t shown=0;
 int selected_mode=0;
 unsigned RequestedScreens() override {selected_mode=mode.load();return selected_mode==0?1U:2U;}
 pxandroid::ScreenBlitter blitter;
 core::Status Present(std::uint64_t frame,const domain::gpu::Picture&top,const domain::gpu::Picture&bottom)override{
  shown=frame;if(frame%120==0)Log("Rendered frame "+std::to_string(frame)+"; actual top picture "+std::to_string(top.width)+"x"+std::to_string(top.height));
  std::lock_guard lock(surface_mutex);if(!window)return {};
  ANativeWindow_Buffer b{};if(ANativeWindow_lock(window,&b,nullptr)!=0)return {};
  auto m=selected_mode;const auto&picture=m==0?top:bottom;
  blitter.Draw(static_cast<std::uint32_t*>(b.bits),b.width,b.height,b.stride,picture,m==0?MainRect(b.width,b.height):BottomRect(b.width,b.height,m));
  if(ANativeWindow_unlockAndPost(window)==0)presented_frames.fetch_add(1,std::memory_order_relaxed);return {};
 }
 bool StopRequested()const override{return stopping;}
 std::string Describe()const override{return "Android single display";}
 domain::services::InputState StateAt(std::uint64_t)override{domain::services::InputState s{.buttons=static_cast<std::uint32_t>(buttons.load()),.circle_x=static_cast<std::int16_t>(cx.load()),.circle_y=static_cast<std::int16_t>(cy.load())};int x=tx.load(),y=ty.load();if(x>=0&&x<320&&y>=0&&y<240)s.touch=domain::services::TouchPoint{static_cast<std::uint16_t>(x),static_cast<std::uint16_t>(y)};return s;}
};
}
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*vm,void*){PxJavaVM=vm;return JNI_VERSION_1_6;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeSurface(JNIEnv*e,jclass,jobject surface){std::lock_guard lock(surface_mutex);if(window)ANativeWindow_release(window);window=surface?ANativeWindow_fromSurface(e,surface):nullptr;if(window)ANativeWindow_setBuffersGeometry(window,0,0,WINDOW_FORMAT_RGBA_8888);}

extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeConfigure(JNIEnv*e,jclass,jint scale,jstring dir,jstring library){render_scale=std::clamp(static_cast<int>(scale),1,3);driver_dir=String(e,dir);driver_library=String(e,library);}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeResidentTextures(JNIEnv*,jclass,jboolean enabled){resident_textures=enabled;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeRenderer(JNIEnv*,jclass,jint r){renderer=r;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeMode(JNIEnv*,jclass,jint m){mode=m==2?2:0;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeInput(JNIEnv*,jclass,jint b,jint x,jint y,jint touchx,jint touchy){buttons=b;cx=x;cy=y;tx=touchx;ty=touchy;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativePause(JNIEnv*,jclass,jboolean p){paused=p;buttons=0;cx=0;cy=0;tx=-1;ty=-1;}
extern "C" JNIEXPORT jboolean JNICALL Java_org_pokemonx_recomp_MainActivity_nativePrepare(JNIEnv*,jclass){bool expected=false;if(!busy.compare_exchange_strong(expected,true))return false;stopping=false;paused=false;presented_frames=0;emulated_frames=0;return true;}
extern "C" JNIEXPORT jlongArray JNICALL Java_org_pokemonx_recomp_MainActivity_nativeCounters(JNIEnv*e,jclass){jlong values[]={static_cast<jlong>(presented_frames.load(std::memory_order_relaxed)),static_cast<jlong>(emulated_frames.load(std::memory_order_relaxed))};auto result=e->NewLongArray(2);if(result)e->SetLongArrayRegion(result,0,2,values);return result;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeStop(JNIEnv*,jclass){stopping=true;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeRun(JNIEnv*e,jobject activity,jstring data,jstring libs,jstring session_log){
 auto dir=String(e,data),libdir=String(e,libs);log_path=String(e,session_log);crash_fd=open(log_path.c_str(),O_WRONLY|O_APPEND|O_CREAT,0600);for(int s:{SIGSEGV,SIGBUS,SIGILL,SIGABRT})std::signal(s,Crash);
 try{
  Log("Starting Pokémon X runtime arm64 build 13");
  Log(std::string("Resident GPU textures configured: ")+(resident_textures.load()?"on":"off"));
  Log("Internal resolution: "+std::to_string(render_scale.load())+"x, main "+std::to_string(400*render_scale.load())+"x"+std::to_string(240*render_scale.load()));
  adapters::filesystem::FileRomSource source(dir+"/game.cxi");auto bytes=source.Contents();if(!bytes)throw std::runtime_error(bytes.Failure().Message());
  auto loaded=application::BootedTitle::FromNcch(bytes.Value());if(!loaded)throw std::runtime_error(loaded.Failure().Message());auto&title=*loaded.Value();Log("Executable SHA256 "+title.CodeDigest());
  Display display;title.UseInput(display);
  title.Loader().OnModuleEvent([](const domain::services::RelocatableObjects::ModuleEvent&event){Log(std::string(event.kind==domain::services::RelocatableObjects::ModuleEvent::Kind::Unloaded?"Module unload: ":"Module load: ")+std::string(event.name));});
  std::unique_ptr<adapters::lifted::LiftedLibrary> lifted;
  std::unique_ptr<adapters::lifted::ModulesLibrary> modules_library;
  std::unique_ptr<domain::lift::runtime::LoadedModules> modules;
  std::unique_ptr<domain::lift::Engine> engine;
  const auto path=libdir+"/libpxmain.so";
  if(std::filesystem::exists(path)){
   auto result=adapters::lifted::LiftedLibrary::Open(path,title.CodeDigest());if(!result)throw std::runtime_error(result.Failure().Message());lifted=std::move(result.Value());
   const auto modules_path=libdir+"/libpxmodules.so";
   if(std::filesystem::exists(modules_path)){
    auto opened=adapters::lifted::ModulesLibrary::Open(modules_path);if(!opened)throw std::runtime_error(opened.Failure().Message());modules_library=std::move(opened.Value());
    modules=std::make_unique<domain::lift::runtime::LoadedModules>();modules_library->OfferTo(*modules);
    Log("Native lifted modules loaded: "+std::to_string(modules_library->Count()));
    title.Loader().OnModuleEvent([&modules](const domain::services::RelocatableObjects::ModuleEvent&event){
     using Event=domain::services::RelocatableObjects::ModuleEvent;
     using Outcome=domain::lift::runtime::LoadedModules::Outcome;
     if(event.kind==Event::Kind::Unloaded){modules->Unloaded(event.name,event.address);return;}
     const auto outcome=modules->Loaded(event.name,event.address,event.code_offset,event.code_size,event.code_digest);
     Log("Module execution: "+std::string(event.name)+(outcome==Outcome::Lifted?" native AOT":outcome==Outcome::DigestDiffers?" interpreter (code digest differs)":" interpreter (not lifted)"));
    });
   }else Log("No lifted modules library: CRO modules will run interpreted");
   engine=std::make_unique<domain::lift::Engine>(lifted->Entries(),modules.get());title.Kernel().UseLiftedCode(engine.get());Log("Native lifted main executable loaded");
  }
  else Log("No lifted library: interpreter validation mode");
  auto memory=application::GpuMemoryOf(title);
#ifdef PX_VULKAN
  std::unique_ptr<adapters::vulkan::VulkanContext> vk;
#endif
  std::unique_ptr<ports::GpuBackend> gpu;
#ifdef PX_VULKAN
#ifdef PX_CUSTOM_VULKAN
  // Drivers remain loaded for process lifetime because Android's loader can
  // keep namespace hooks alive after a Vulkan instance is destroyed.
  if(renderer==0){
   if(!driver_library.empty()){
    auto hooks=libdir+"/",custom=driver_dir+"/";
    void*shim=dlopen((libdir+"/libpxdrivers.so").c_str(),RTLD_NOW|RTLD_LOCAL);
    if(!shim)throw std::runtime_error("Custom driver support could not be opened");
    using OpenCustom=void*(*)(const char*,const char*,const char*);
    auto open_custom=reinterpret_cast<OpenCustom>(dlsym(shim,"px_open_custom_driver"));
    if(!open_custom)throw std::runtime_error("Custom driver entry point missing");
    void*handle=open_custom(hooks.c_str(),custom.c_str(),driver_library.c_str());
    if(!handle)throw std::runtime_error("Custom driver could not be loaded. Select System in the launcher.");
    auto proc=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(handle,"vkGetInstanceProcAddr"));
    if(!proc)throw std::runtime_error("Driver exposes no vkGetInstanceProcAddr");
    volkInitializeCustom(proc);Log("Custom driver requested: "+driver_library);
   }else{if(volkInitialize()!=VK_SUCCESS)throw std::runtime_error("System Vulkan loader unavailable");Log("Driver: system");}
  }
#endif

  if(renderer==0){auto created=adapters::vulkan::VulkanContext::Create({.validation=false,.pipeline_cache_directory=dir+"/vulkan-cache"});if(created){vk=std::move(created.Value());auto backend=adapters::vulkan::VulkanGpuBackend::Create(*vk,memory,domain::gpu::Sampling::Hardware,adapters::vulkan::VertexShading::GpuWherePossible,adapters::vulkan::FragmentShading::Specialised,render_scale.load(),resident_textures.load());if(backend)gpu=std::move(backend.Value());else Log("Vulkan backend: "+backend.Failure().Message());}else Log("Vulkan: "+created.Failure().Message());}
#endif
  if(!gpu && render_scale>1)throw std::runtime_error("Internal scaling requires Vulkan; select 1x or a working driver");
  if(!gpu)gpu=std::make_unique<application::ReferenceGpuBackend>(memory,domain::gpu::Sampling::Hardware);
  Log("Renderer: "+gpu->Describe());
#ifdef PX_VULKAN
  if(vk){Log("Vulkan device: "+vk->DeviceName());Log("Pipeline cache loaded bytes: "+std::to_string(vk->LoadedPipelineCacheBytes()));}
#endif
  application::LiveDisplay live(memory,*gpu,display);live.Watch(title.Gpu());
  adapters::filesystem::DirectoryTreeStore saves(dir+"/saves");auto restored=application::RestoreSaves(saves,title.Save(),title.Extra());if(!restored)throw std::runtime_error(restored.Failure().Message());
  title.Kernel().OnDebugOutput([](std::string_view s){Log("Guest: "+std::string(s));});
  title.Kernel().OnServiceRequest([](const domain::kernel::ServiceRequestRecord&r){if(!r.implemented)Log("Unimplemented service "+std::string(r.service)+" command "+std::to_string(r.header));});
  auto next_frame=std::chrono::steady_clock::now();
  title.Gpu().OnVerticalBlank([&](std::uint64_t frame){emulated_frames.store(frame,std::memory_order_relaxed);auto now=std::chrono::steady_clock::now();if(now>next_frame+std::chrono::milliseconds(100))next_frame=now;next_frame+=std::chrono::nanoseconds(16666667);if(next_frame>now)std::this_thread::sleep_until(next_frame);});
  Status(e,activity,"Iniciando Pokémon X…");
  std::uint64_t last_saved=0,last_status=0;
  auto last_profile=std::chrono::steady_clock::now();
  std::uint64_t profile_posted=0,profile_frames=0;
  Log("Performance: selected-screen composition, borrowed coherent scaled framebuffer");
  while(!stopping){if(paused){last_profile=std::chrono::steady_clock::now();profile_posted=presented_frames.load();profile_frames=title.Gpu().Frames();std::this_thread::sleep_for(std::chrono::milliseconds(20));continue;}
   auto stop=title.Kernel().Run(500000);
   auto now=std::chrono::steady_clock::now();if(now-last_profile>=std::chrono::seconds(5)){
    const auto elapsed=std::chrono::duration<double>(now-last_profile).count();last_profile=now;
    const auto posted=presented_frames.load(),frames=title.Gpu().Frames();
    const auto&w=live.Work();const auto stats=gpu->Statistics();
    Log("Profile interval: fps="+std::to_string((posted-profile_posted)/elapsed)+", speed_percent="+std::to_string((frames-profile_frames)/elapsed/60.0*100));
    profile_posted=posted;profile_frames=frames;
    Log("Profile cumulative: emulated="+std::to_string(frames)+", posted="+std::to_string(posted)+", guest_cpu_ms="+std::to_string(title.Kernel().GuestTime().count()/1000000)+", GPU lists_ms="+std::to_string(w.lists_time.count()/1000000)+", transfers_ms="+std::to_string(w.transfers_time.count()/1000000)+", present_ms="+std::to_string(w.present_time.count()/1000000));
    Log("GPU cumulative: draws="+std::to_string(stats.draws)+", fallback_draws="+std::to_string(stats.fallback_draws)+", waits="+std::to_string(stats.waits)+", wait_ms="+std::to_string(stats.times.wait.count()/1000000)+", upload_ms="+std::to_string(stats.times.upload.count()/1000000)+", download_ms="+std::to_string(stats.times.download.count()/1000000)+", pipelines_ms="+std::to_string(stats.times.pipelines.count()/1000000)+", allocations="+std::to_string(stats.allocations));
    Log("GPU stages cumulative: resident_texture_samples="+std::to_string(stats.resident_texture_samples)+", resident_texture_copies="+std::to_string(stats.resident_texture_copies)+", gpu_shaded_draws="+std::to_string(stats.gpu_shaded_draws)+", textures_ms="+std::to_string(stats.times.textures.count()/1000000)+", cpu_vertices_ms="+std::to_string(stats.times.vertices.count()/1000000)+", gpu_total_ms="+std::to_string(stats.times.total.count()/1000000)+", pipelines="+std::to_string(stats.pipelines)+", fragment_configs="+std::to_string(stats.fragment_configurations));
    Log("Texture stages cumulative: sync_ms="+std::to_string(stats.times.texture_sync.count()/1000000)+", decode_ms="+std::to_string(stats.times.texture_decode.count()/1000000)+", pack_ms="+std::to_string(stats.times.texture_pack.count()/1000000));
    if(engine)Log("CPU cumulative: lifted_entries="+std::to_string(engine->Counted().entries)+", module_entries="+std::to_string(engine->Counted().module_entries)+", interpreted_instructions="+std::to_string(engine->Counted().interpreted));
   }
   if(title.Gpu().Frames()>last_status+60){last_status=title.Gpu().Frames();Status(e,activity,"Pokémon X • quadro "+std::to_string(last_status));}
   if(title.Gpu().Frames()>last_saved+1800){last_saved=title.Gpu().Frames();auto saved=application::StoreSaves(saves,title.Save(),title.Extra());if(!saved)Log("Save error: "+saved.Failure().Message());}
   if(stop.reason!=domain::kernel::KernelStopReason::BudgetSpent){auto message="Runtime stopped: reason "+std::to_string(static_cast<int>(stop.reason))+", PC "+std::to_string(stop.address)+", service "+std::string(stop.service)+", detail "+std::to_string(stop.detail);Log(message);Status(e,activity,"Execução interrompida. Exporte os logs.");break;}
  }
  auto finished=live.Finish();if(!finished)Log("Display: "+finished.Failure().Message());
  auto saved=application::StoreSaves(saves,title.Save(),title.Extra());if(!saved)Log("Save: "+saved.Failure().Message());
  Log("Stopped at frame "+std::to_string(title.Gpu().Frames())+"; instructions "+std::to_string(title.Kernel().Executed()));
  if(engine)Log("Lifted entries "+std::to_string(engine->Counted().entries)+"; module entries "+std::to_string(engine->Counted().module_entries)+"; interpreted instructions "+std::to_string(engine->Counted().interpreted));
 }catch(const std::exception&error){Log(std::string("ERROR: ")+error.what());Status(e,activity,"Falha ao iniciar. Exporte os logs.");}
 if(crash_fd>=0){close(crash_fd);crash_fd=-1;}
 busy=false;
}
