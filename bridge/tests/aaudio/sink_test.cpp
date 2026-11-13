// Synthetic device API: tests the real sink implementation without a phone.
#include "audio_core/aaudio_sink.h"
#include <cassert>
#include <array>
#include <iostream>
#include <stdexcept>
static AAudioStream stream; static AAudioStreamBuilder builder;
static DataCallback data; static ErrorCallback error; static void *data_user,*error_user;
static int open_result=0,start_result=0,rate=44100,channels=2,closed=0,stopped=0,deleted=0,started=0;
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder** out){*out=&builder;return 0;}
#define SETTER(name) void name(AAudioStreamBuilder*,int){}
SETTER(AAudioStreamBuilder_setDirection) SETTER(AAudioStreamBuilder_setFormat)
SETTER(AAudioStreamBuilder_setChannelCount) SETTER(AAudioStreamBuilder_setSampleRate)
SETTER(AAudioStreamBuilder_setSharingMode) SETTER(AAudioStreamBuilder_setPerformanceMode)
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder*,DataCallback fn,void* u){data=fn;data_user=u;}
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder*,ErrorCallback fn,void* u){error=fn;error_user=u;}
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder*,AAudioStream** out){if(!open_result)*out=&stream;return open_result;}
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder*){++deleted;return 0;}
int AAudioStream_getFormat(AAudioStream*){return AAUDIO_FORMAT_PCM_I16;}
int AAudioStream_getChannelCount(AAudioStream*){return channels;}
int AAudioStream_getSampleRate(AAudioStream*){return rate;}
int AAudioStream_getFramesPerBurst(AAudioStream*){return 192;}
int AAudioStream_setBufferSizeInFrames(AAudioStream*,int n){assert(n==576);return n;}
int AAudioStream_requestStart(AAudioStream*){++started;return start_result;}
int AAudioStream_requestStop(AAudioStream*){++stopped;return 0;}
int AAudioStream_close(AAudioStream*){++closed;return 0;}
int AAudioStream_getXRunCount(AAudioStream*){return 3;}
int AAudioStream_getState(AAudioStream*){return 4;}
int main(){
 {AudioCore::AAudioSink sink("Auto");assert(started==0);assert(sink.GetNativeSampleRate()==44100);
  std::array<s16,10> pcm;pcm.fill(77);
  data(&stream,data_user,pcm.data()+1,4);
  assert(pcm[0]==77&&pcm[9]==77);for(int i=1;i<9;++i)assert(pcm[i]==0);
  sink.SetCallback([](s16* out,std::size_t n){assert(n==4);for(std::size_t i=0;i<n;++i){out[i*2]=123;out[i*2+1]=-456;}});
  data(&stream,data_user,pcm.data()+1,4);
  assert(pcm[0]==77&&pcm[9]==77);for(int i=1;i<9;i+=2){assert(pcm[i]==123&&pcm[i+1]==-456);}
  assert(sink.Diagnostics().find("frames=8, signal_frames=4, xruns=3")!=std::string::npos);
  AudioCore::SetAndroidOutputMuted(true);data(&stream,data_user,pcm.data()+1,4);
  for(int i=1;i<9;++i)assert(pcm[i]==0);
  AudioCore::SetAndroidOutputMuted(false);data(&stream,data_user,pcm.data()+1,4);assert(pcm[1]==123);
  error(&stream,error_user,-899);assert(closed==0&&stopped==0);
  assert(sink.Diagnostics().find("error=-899")!=std::string::npos);
 }
 assert(closed==1&&stopped==1);
 open_result=-1;try{AudioCore::AAudioSink sink("Auto");assert(false);}catch(const std::runtime_error&){}assert(closed==1&&deleted==2);
 open_result=0;channels=1;try{AudioCore::AAudioSink sink("Auto");assert(false);}catch(const std::runtime_error&){}assert(closed==2);
 channels=2;start_result=-2;try{AudioCore::AAudioSink sink("Auto");sink.SetCallback({});assert(false);}catch(const std::runtime_error&){}assert(closed==3&&stopped==2);
 std::cout<<"PASS stereo frame boundaries, negotiated rate, silence, signal diagnostics, error callback, open/format/start failure cleanup and teardown\n";
}
