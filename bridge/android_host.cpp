#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <atomic>
#include <algorithm>
#include <functional>
#include <vector>
#include <limits>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include "arm_native.h"
#include "audio_core/aaudio_sink.h"
#include "audio_core/dsp_interface.h"
#include "render_scale.h"
#include "common/px_screen_interface.h"
#include "program.h"
#include "battle_data.h"
#include "core/hle/service/am/am.h"
#include "common/dynamic_library.h"
#include "common/file_util.h"
#include "common/logging/backend.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/applets/default_applets.h"
#include "core/frontend/image_interface.h"
#include "core/frontend/input.h"
#include "core/hle/service/service.h"
#include "video_core/gpu.h"
#include "video_core/renderer_base.h"
namespace {
std::atomic<bool> busy{false},stopping{false},paused{false};
std::atomic<bool> configured_textures{false};
std::atomic<int> buttons{0},cx{0},cy{0},tx{-1},ty{-1},scale{1},configured_scale{1},renderer{0};
std::atomic<std::uint64_t> presented{0},emulated{0};
auto& mode=PokemonXUI::mode;
std::atomic<double> reported_fps{std::numeric_limits<double>::quiet_NaN()},reported_speed{std::numeric_limits<double>::quiet_NaN()};
std::mutex surface_mutex,log_mutex,notify_mutex;
std::function<void()> notify_surface;
ANativeWindow* requested_surface=nullptr;
std::string log_path,driver_dir,driver_library,az_log;
std::uint64_t az_offset=0;
int crash_fd=-1;
void Log(const std::string& line){std::lock_guard lock(log_mutex);__android_log_write(ANDROID_LOG_INFO,"PokemonX",line.c_str());std::ofstream(log_path,std::ios::app)<<line<<'\n';}
void HandleCrash(int signal){const char text[]="\nNATIVE CRASH in Azahar integration. Export this session log.\n";if(crash_fd>=0)write(crash_fd,text,sizeof(text)-1);std::signal(signal,SIG_DFL);raise(signal);}
std::string String(JNIEnv* env,jstring value){auto p=env->GetStringUTFChars(value,nullptr);std::string s=p;env->ReleaseStringUTFChars(value,p);return s;}
void Status(JNIEnv* env,jobject activity,const std::string& value){auto cls=env->GetObjectClass(activity);auto method=env->GetMethodID(cls,"runtimeStatus","(Ljava/lang/String;)V");auto text=env->NewStringUTF(value.c_str());env->CallVoidMethod(activity,method,text);env->DeleteLocalRef(text);env->DeleteLocalRef(cls);if(env->ExceptionCheck())env->ExceptionClear();}
void MirrorLog(){
 std::ifstream file(az_log,std::ios::binary);if(!file)return;file.seekg(0,std::ios::end);const auto size=static_cast<std::uint64_t>(file.tellg());if(size<az_offset)az_offset=0;
 file.seekg(az_offset);std::string line;while(std::getline(file,line))Log("Azahar: "+line);az_offset=size;
}
class Button final:public Input::ButtonDevice{int bit;public:explicit Button(int b):bit(b){}bool GetStatus()const override{return(buttons.load()&(1<<bit))!=0;}};
class Buttons final:public Input::Factory<Input::ButtonDevice>{public:std::unique_ptr<Input::ButtonDevice>Create(const Common::ParamPackage&p)override{return std::make_unique<Button>(p.Get("bit",0));}};
class Analog final:public Input::AnalogDevice{public:std::tuple<float,float>GetStatus()const override{return{std::clamp(cx.load()/156.f,-1.f,1.f),std::clamp(cy.load()/156.f,-1.f,1.f)};}};
class Analogs final:public Input::Factory<Input::AnalogDevice>{public:std::unique_ptr<Input::AnalogDevice>Create(const Common::ParamPackage&)override{return std::make_unique<Analog>();}};
class Touch final:public Input::TouchDevice{public:std::tuple<float,float,bool>GetStatus()const override{int x=tx.load(),y=ty.load();return{x/320.f,y/240.f,x>=0&&y>=0};}};
class Touches final:public Input::Factory<Input::TouchDevice>{public:std::unique_ptr<Input::TouchDevice>Create(const Common::ParamPackage&)override{return std::make_unique<Touch>();}};
void RegisterInput(){
 static bool once=false;if(!once){Input::RegisterFactory<Input::ButtonDevice>("px_native",std::make_shared<Buttons>());Input::RegisterFactory<Input::AnalogDevice>("px_native",std::make_shared<Analogs>());Input::RegisterFactory<Input::TouchDevice>("px_native",std::make_shared<Touches>());once=true;}
 constexpr int bits[]={0,1,10,11,6,7,5,4,9,8,3,2,12,13,14,15,16,17};
 for(int i=0;i<Settings::NativeButton::NumButtons;++i)Settings::values.current_input_profile.buttons[i]="engine:px_native,bit:"+std::to_string(bits[i]);
 Settings::values.current_input_profile.analogs[Settings::NativeAnalog::CirclePad]="engine:px_native";
 Settings::values.current_input_profile.touch_device="engine:px_native";
}
class Context final:public Frontend::GraphicsContext{
 std::shared_ptr<Common::DynamicLibrary>driver;
public:explicit Context(std::shared_ptr<Common::DynamicLibrary>d):driver(std::move(d)){}
 std::shared_ptr<Common::DynamicLibrary>GetDriverLibrary()override{return driver;}
};
class Window final:public Frontend::EmuWindow{
 std::shared_ptr<Common::DynamicLibrary>driver;
 std::vector<ANativeWindow*> retained;
 std::mutex update_mutex;
 int previous_mode=-1,width=0,height=0;
public:
 explicit Window(std::shared_ptr<Common::DynamicLibrary>d):driver(std::move(d)){PollEvents();}
 ~Window(){for(auto* w:retained)ANativeWindow_release(w);}
 void PollEvents()override{
  std::lock_guard update_lock(update_mutex);
  bool changed=false;int w=width,h=height;
  {std::lock_guard lock(surface_mutex);
   if(!requested_surface)return;
   if(requested_surface!=window_info.render_surface){
    if(requested_surface){ANativeWindow_acquire(requested_surface);retained.push_back(requested_surface);w=ANativeWindow_getWidth(requested_surface);h=ANativeWindow_getHeight(requested_surface);}
    window_info.type=Frontend::WindowSystemType::Android;window_info.render_surface=requested_surface;changed=true;
   }else if(requested_surface){w=ANativeWindow_getWidth(requested_surface);h=ANativeWindow_getHeight(requested_surface);}
  }
  if(previous_mode<0)UpdateCurrentFramebufferLayout(std::max(w,1),std::max(h,1),false);
  int selected=PokemonXUI::Effective(mode.load());auto expected=PokemonXUI::Build(selected,std::max(w,1),std::max(h,1));const auto& current=GetFramebufferLayout();
  bool layout_changed=current.top_screen_enabled!=expected.show_top||current.bottom_screen_enabled!=expected.show_bottom;
  if(expected.show_top)layout_changed|=current.top_screen.left!=u32(std::round(expected.top.x))||current.top_screen.top!=u32(std::round(expected.top.y))||current.top_screen.right!=u32(std::round(expected.top.x+expected.top.w))||current.top_screen.bottom!=u32(std::round(expected.top.y+expected.top.h));
  if(expected.show_bottom)layout_changed|=current.bottom_screen.left!=u32(std::round(expected.bottom.x))||current.bottom_screen.top!=u32(std::round(expected.bottom.y))||current.bottom_screen.right!=u32(std::round(expected.bottom.x+expected.bottom.w))||current.bottom_screen.bottom!=u32(std::round(expected.bottom.y+expected.bottom.h));
  if(changed||selected!=previous_mode||w!=width||h!=height||layout_changed){
   width=std::max(w,1);height=std::max(h,1);previous_mode=selected;
   Settings::values.swap_screen.SetValue(false);
   UpdateCurrentFramebufferLayout(width,height,false);
   auto layout=GetFramebufferLayout();auto ui=PokemonXUI::Build(selected,width,height);
   auto rect=[](PokemonXUI::Rect r){return Common::Rectangle<u32>{u32(std::round(r.x)),u32(std::round(r.y)),u32(std::round(r.x+r.w)),u32(std::round(r.y+r.h))};};
   layout.top_screen_enabled=ui.show_top;layout.bottom_screen_enabled=ui.show_bottom;
   layout.top_screen=rect(ui.top);layout.bottom_screen=rect(ui.bottom);layout.additional_screen_enabled=false;
   NotifyFramebufferLayoutChanged(layout);
   Log("Screen interface mode="+std::to_string(selected)+", viewport="+std::to_string(width)+"x"+std::to_string(height)+", pieces="+std::to_string(ui.count));
   auto& system=Core::System::GetInstance();if(changed&&system.IsPoweredOn())system.GPU().Renderer().NotifySurfaceChanged(false);
  }
 }
 std::unique_ptr<Frontend::GraphicsContext>CreateSharedContext()const override{return std::make_unique<Context>(driver);}
 std::shared_ptr<Common::DynamicLibrary>GetDriverLibrary()override{return driver;}
};
std::shared_ptr<Common::DynamicLibrary> Driver(const std::string&libdir){
 if(driver_library.empty()){Log("Azahar Vulkan driver: system");return{};}
 auto*shim=dlopen((libdir+"/libpxdrivers.so").c_str(),RTLD_NOW|RTLD_LOCAL);if(!shim)throw std::runtime_error("could not open custom driver support");
 using Open=void*(*)(const char*,const char*,const char*);auto open=reinterpret_cast<Open>(dlsym(shim,"px_open_custom_driver"));if(!open)throw std::runtime_error("custom driver entry missing");
 auto hooks=libdir+"/",folder=driver_dir+"/";void*handle=open(hooks.c_str(),folder.c_str(),driver_library.c_str());if(!handle)throw std::runtime_error("custom driver failed: select System");
 static std::vector<std::shared_ptr<Common::DynamicLibrary>>keep;
 auto driver=std::make_shared<Common::DynamicLibrary>(handle);keep.push_back(driver);Log("Azahar Vulkan driver: "+driver_library);return driver;
}
}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeSurface(JNIEnv*e,jclass,jobject surface){
 {std::lock_guard lock(surface_mutex);if(requested_surface)ANativeWindow_release(requested_surface);requested_surface=surface?ANativeWindow_fromSurface(e,surface):nullptr;}
 // Wake presentation directly from the surface callback: RunLoop may itself
 // be waiting for a replacement surface and cannot poll events in that state.
 if(surface){std::lock_guard lock(notify_mutex);if(notify_surface)notify_surface();}
}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeTextures(JNIEnv*,jclass,jboolean enabled){configured_textures=enabled;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeConfigure(JNIEnv*e,jclass,jint s,jstring dir,jstring lib){configured_scale=s==0?0:std::clamp(static_cast<int>(s),1,PokemonX::MaximumScale);driver_dir=String(e,dir);driver_library=String(e,lib);}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeRenderer(JNIEnv*,jclass,jint value){renderer=value;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeResidentTextures(JNIEnv*,jclass,jboolean){}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeMode(JNIEnv*,jclass,jint value){++PokemonXUI::generation;PokemonXUI::popup_requested=false;PokemonXUI::battle_menu_full=false;mode=std::clamp(static_cast<int>(value),0,5);tx=-1;ty=-1;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeOpenPopup(JNIEnv*,jclass){PokemonXUI::OpenPopup();}
extern "C" JNIEXPORT jboolean JNICALL Java_org_pokemonx_recomp_MainActivity_nativePopupVisible(JNIEnv*,jclass){return PokemonXUI::popup_requested.load();}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeCommitUI(JNIEnv*,jclass,jint profile,jint index){PokemonXUI::Commit(profile,index);}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeBattleRegions(JNIEnv*e,jclass,jint w,jint h,jfloatArray out){if(!out||e->GetArrayLength(out)<34)return;int requested=mode.load(),effective=PokemonXUI::Effective(requested);auto ui=PokemonXUI::Build(effective,w,h);std::array<jfloat,34> values{};values[0]=float(effective);values[1]=float(ui.count);values[2]=float(requested);for(int i=0;i<ui.count;i++){auto p=ui.pieces[i];int at=3+i*6;values[at]=p.destination.x;values[at+1]=p.destination.y;values[at+2]=p.destination.w;values[at+3]=p.destination.h;values[at+4]=p.image?1.f:0.f;values[at+5]=p.enabled?1.f:0.f;}values[3+ui.count*6]=float(PokemonXUI::command_focus.load());e->SetFloatArrayRegion(out,0,values.size(),values.data());}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeMoveStats(JNIEnv*e,jclass,jintArray out){if(!out||e->GetArrayLength(out)<15)return;auto v=PokemonX::BattleDataSnapshot();if(!PokemonXUI::move_cards)v[0]=0;e->SetIntArrayRegion(out,0,v.size(),v.data());}
extern "C" JNIEXPORT jint JNICALL Java_org_pokemonx_recomp_MainActivity_nativeEffectiveMode(JNIEnv*,jclass){return PokemonXUI::Effective(mode.load());}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeMoveCards(JNIEnv*,jclass,jboolean enabled){PokemonXUI::move_cards=enabled;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeAutomaticUI(JNIEnv*,jclass,jboolean enabled){PokemonXUI::automatic=enabled;}
extern "C" JNIEXPORT jintArray JNICALL Java_org_pokemonx_recomp_MainActivity_nativeMapTouch(JNIEnv*e,jclass,jfloat x,jfloat y,jint w,jint h,jint selected){auto [tx,ty]=PokemonXUI::Touch(selected,w,h,x,y);jint values[]={tx,ty};auto out=e->NewIntArray(2);if(out)e->SetIntArrayRegion(out,0,2,values);return out;}
static PokemonXUI::CommandController command_controller;
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeInput(JNIEnv*,jclass,jint b,jint x,jint y,jint touchx,jint touchy){command_controller.Route(b,x,y,touchx,touchy);int previous=buttons.load();bool changed=previous!=b||((tx.load()>=0&&ty.load()>=0)!=(touchx>=0&&touchy>=0));if((b&2)&&!(previous&2)){if(PokemonXUI::Effective(mode.load())==4)PokemonXUI::Commit(4,4);PokemonXUI::decision_hidden=false;if(PokemonXUI::popup_requested)PokemonXUI::popup_close_requested=true;}if((b&1)&&!(previous&1)){if(PokemonXUI::decision_hidden)PokemonXUI::decision_hidden=false;}buttons=b;cx=x;cy=y;tx=touchx;ty=touchy;if(changed)PokemonXUI::force_probe=true;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativePause(JNIEnv*,jclass,jboolean value){paused=value;command_controller.Reset();AudioCore::SetAndroidOutputMuted(value);if(value){reported_fps=std::numeric_limits<double>::quiet_NaN();reported_speed=std::numeric_limits<double>::quiet_NaN();}buttons=0;cx=0;cy=0;tx=-1;ty=-1;}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeStop(JNIEnv*,jclass){stopping=true;}
extern "C" JNIEXPORT jboolean JNICALL Java_org_pokemonx_recomp_MainActivity_nativePrepare(JNIEnv*,jclass){bool expected=false;if(!busy.compare_exchange_strong(expected,true))return false;stopping=false;paused=false;AudioCore::SetAndroidOutputMuted(false);presented=0;emulated=0;reported_fps=std::numeric_limits<double>::quiet_NaN();reported_speed=std::numeric_limits<double>::quiet_NaN();return true;}
extern "C" JNIEXPORT jlongArray JNICALL Java_org_pokemonx_recomp_MainActivity_nativeCounters(JNIEnv*e,jclass){jlong values[]={static_cast<jlong>(presented.load()),static_cast<jlong>(emulated.load())};auto result=e->NewLongArray(2);if(result)e->SetLongArrayRegion(result,0,2,values);return result;}
extern "C" JNIEXPORT jdoubleArray JNICALL Java_org_pokemonx_recomp_MainActivity_nativePerformance(JNIEnv*e,jclass){
 const jdouble values[]={reported_fps.load(),reported_speed.load()};auto result=e->NewDoubleArray(2);if(result)e->SetDoubleArrayRegion(result,0,2,values);return result;
}
extern "C" JNIEXPORT void JNICALL Java_org_pokemonx_recomp_MainActivity_nativeRun(JNIEnv*e,jobject activity,jstring data,jstring libs,jstring session,jboolean warming){
 auto dir=String(e,data),libdir=String(e,libs);log_path=String(e,session);crash_fd=open(log_path.c_str(),O_WRONLY|O_APPEND|O_CREAT,0600);for(int s:{SIGSEGV,SIGBUS,SIGILL,SIGABRT})std::signal(s,HandleCrash);
 auto& system=Core::System::GetInstance();
 std::unique_ptr<Window> window;
 try{
  Log("Starting Pokemon X native CPU + Azahar kernel/services/Vulkan build 28");
  if(renderer!=0)throw std::runtime_error("Azahar backend requires Vulkan; select Vulkan in settings");
  std::filesystem::create_directories(dir+"/azahar/");FileUtil::SetUserPath(dir+"/azahar/");
  for(const auto* path:{"shaders/vulkan/pipeline","shaders/vulkan/transferable"})std::filesystem::create_directories(dir+"/azahar/"+path);
  az_log=dir+"/azahar/log/runtime-azahar.txt";az_offset=std::filesystem::exists(az_log)?std::filesystem::file_size(az_log):0;
  static bool logging=false;if(!logging){Common::Log::Initialize("runtime-azahar.txt");Common::Log::Start();logging=true;az_offset=0;}
  Common::Log::Filter filter;filter.ParseFilterString("*:Warning Core.ARM11:Info Debug.Emulated:Debug");Common::Log::SetGlobalFilter(filter);
  const int requested_scale=configured_scale.load();int surface_width=400,surface_height=240;
  {std::lock_guard lock(surface_mutex);if(requested_surface){surface_width=ANativeWindow_getWidth(requested_surface);surface_height=ANativeWindow_getHeight(requested_surface);}}
  scale=PokemonX::ChooseRenderScale(requested_scale,surface_width,surface_height);
  Log("Render scale requested="+std::to_string(requested_scale)+", effective="+std::to_string(scale.load())+", main_target="+std::to_string(PokemonX::NativeWidth*scale.load())+"x"+std::to_string(PokemonX::NativeHeight*scale.load()));
  Settings::values.graphics_api.SetValue(Settings::GraphicsAPI::Vulkan);Settings::values.resolution_factor.SetValue(scale.load());
  Settings::values.layout_option.SetValue(Settings::LayoutOption::SingleScreen);Settings::values.swap_screen.SetValue(false);
  Settings::values.output_type.SetValue(AudioCore::SinkType::AAudio);Settings::values.input_type.SetValue(AudioCore::InputType::Null);
  Settings::values.volume.SetValue(1.0f);Settings::values.enable_audio_stretching.SetValue(true);
  Settings::values.use_disk_shader_cache.SetValue(true);
  Settings::values.custom_textures.SetValue(configured_textures.load());Settings::values.preload_textures.SetValue(false);Settings::values.async_custom_loading.SetValue(true);
  Log(configured_textures?"PNG texture pack enabled (asynchronous, no full preload)":"Texture mods disabled");
  Settings::values.async_shader_compilation.SetValue(false);Settings::values.use_cpu_jit=false;Settings::values.use_fastinterp=false;
  Settings::values.frame_limit.SetValue(100);Settings::values.cpu_clock_percentage.SetValue(100);
  for(const auto& service:Service::service_module_map)Settings::values.lle_modules.emplace(service.name,false);
  RegisterInput();Frontend::RegisterDefaultApplets(system);system.RegisterImageInterface(std::make_shared<Frontend::ImageInterface>());
  const auto update_path=Service::AM::GetTitleContentPath(Service::FS::MediaType::SDMC,0x0004000E00055D00ULL);
  const bool update15=std::filesystem::is_regular_file(update_path);
  const auto expected=update15?"7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74":"0f6d3e1d66cbcd55d46fa453db4b6aa489e03c3537c6b24026f2a53af142797a";
  Log(update15?"Selecting verified 1.5 native libraries":"Selecting base native libraries");
  auto program=PokemonX::Program::Load(libdir+(update15?"/libpxmain15.so":"/libpxmain.so"),libdir+(update15?"/libpxmodules15.so":"/libpxmodules.so"),expected);PokemonX::SetProgram(program);
  window=std::make_unique<Window>(Driver(libdir));Status(e,activity,"Iniciando nucleo Azahar…");
  auto result=system.Load(*window,dir+"/game.cxi");MirrorLog();if(result!=Core::System::ResultStatus::Success)throw std::runtime_error("Azahar load status "+std::to_string(static_cast<int>(result)));
  {std::lock_guard lock(notify_mutex);notify_surface=[ptr=window.get()]{ptr->PollEvents();};}
  Status(e,activity,"Preparando shaders conhecidos…");
  const auto preload_begin=std::chrono::steady_clock::now();
  Log("Shader cache preload started; persistent driver and transferable cache enabled");
  system.GPU().Renderer().Rasterizer()->LoadDefaultDiskResources(stopping,
    [e,activity](VideoCore::LoadCallbackStage stage,std::size_t current,std::size_t total,const std::string& name){
      if(current==0||current==total){Status(e,activity,"Preparando shaders conhecidos: "+std::to_string(current)+"/"+std::to_string(total));Log("Shader cache stage="+std::to_string(static_cast<int>(stage))+", "+name+", "+std::to_string(current)+"/"+std::to_string(total));}
    });
  Log("Shader cache preload completed in ms="+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-preload_begin).count()));
  if(warming&&stopping){Log("Shader preparation cancelled");Status(e,activity,"Preparação cancelada. O cache já concluído foi preservado.");}else if(warming){Log("Import/manual shader preparation completed; game CPU was not executed");Status(e,activity,"Preparação concluída. Novos efeitos serão armazenados ao jogar.");}else Status(e,activity,"Pokémon X");

  Log("Azahar loaded with native CPU; internal Vulkan resolution "+std::to_string(scale.load())+"x; kernel, services and graphics supplied by Azahar");
  auto last=std::chrono::steady_clock::now();double game_count=0,system_count=0;bool was_paused=false;
  while(!stopping&&!warming){
   window->PollEvents();if(paused){std::this_thread::sleep_for(std::chrono::milliseconds(20));last=std::chrono::steady_clock::now();was_paused=true;continue;}
   if(was_paused){system.GetAndResetPerfStats();last=std::chrono::steady_clock::now();was_paused=false;}
   result=system.RunLoop();
   if(result!=Core::System::ResultStatus::Success){Log("Azahar run status "+std::to_string(static_cast<int>(result)));break;}
   PokemonX::RefreshBattleData();
   auto now=std::chrono::steady_clock::now();double seconds=std::chrono::duration<double>(now-last).count();
   if(seconds>=1){auto perf=system.GetAndResetPerfStats();reported_fps=perf.game_fps;reported_speed=perf.emulation_speed*100;game_count+=std::max(0.0,perf.game_fps)*seconds;system_count+=std::max(0.0,perf.system_fps)*seconds;presented=static_cast<std::uint64_t>(game_count);emulated=static_cast<std::uint64_t>(system_count);last=now;
    Log("Azahar profile: fps="+std::to_string(perf.game_fps)+", speed_percent="+std::to_string(perf.emulation_speed*100)+", gpu_seconds="+std::to_string(perf.time_gpu)+", svc_seconds="+std::to_string(perf.time_hle_svc)+", ipc_seconds="+std::to_string(perf.time_hle_ipc)+", present_seconds="+std::to_string(perf.time_swap)+", remaining_seconds="+std::to_string(perf.time_remaining));
    if(auto* audio=dynamic_cast<AudioCore::AAudioSink*>(&system.DSP().GetSink()))Log(audio->Diagnostics());
    std::uint64_t entries=0,modules=0,interpreted=0;
    for(u32 i=0;i<system.GetNumCores();++i){auto& cpu=dynamic_cast<PokemonX::ARMNative&>(system.GetCore(i));entries+=cpu.Statistics().entries;modules+=cpu.Statistics().module_entries;interpreted+=cpu.Statistics().interpreted;}
    Log("Native CPU all cores: entries="+std::to_string(entries)+", module_entries="+std::to_string(modules)+", interpreted="+std::to_string(interpreted));MirrorLog();
   }
  }
  {std::lock_guard lock(notify_mutex);notify_surface={};system.Shutdown();window.reset();}PokemonX::SetProgram({});MirrorLog();Log("Azahar session ended");
 }catch(const std::exception&error){{std::lock_guard lock(notify_mutex);notify_surface={};if(system.IsPoweredOn())system.Shutdown();window.reset();}PokemonX::SetProgram({});MirrorLog();Log(std::string("ERROR: ")+error.what());Status(e,activity,"Falha ao iniciar. Exporte os logs.");}
 if(crash_fd>=0){close(crash_fd);crash_fd=-1;}reported_fps=std::numeric_limits<double>::quiet_NaN();reported_speed=std::numeric_limits<double>::quiet_NaN();busy=false;
}
