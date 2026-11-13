#include "audio_core/time_stretch.h"
#include "audio_core/audio_types.h"
#include <vector>
#include <cmath>
#include <cassert>
#include <iostream>
int main(){for(unsigned rate : {32728u,44100u,48000u}){
 AudioCore::TimeStretcher stretcher;stretcher.SetOutputSampleRate(rate);
 std::vector<s16> in(2048),out(1024);std::size_t offset=0,previous=0,crossings=0,count=0,zeros=0;
 double phase=0;
 // 12 seconds: let the backlog/tempo controller settle before measuring pitch.
 for(unsigned frame=0;frame<rate*12/512;++frame){
  std::size_t target=static_cast<std::size_t>((frame+1)*512.0*AudioCore::native_sample_rate/rate);
  auto n=target-offset;offset=target;
  for(std::size_t i=0;i<n;++i){s16 v=static_cast<s16>(12000*std::sin(phase));phase+=2*3.141592653589793*1000/AudioCore::native_sample_rate;in[2*i]=v;in[2*i+1]=v;}
  auto got=stretcher.Process(in.data(),n,out.data(),512);assert(got<=512);
  if(frame>rate*8/512){for(std::size_t i=0;i<got;++i){if(static_cast<s16>(previous)<=0&&out[2*i]>0)++crossings;previous=static_cast<uint16_t>(out[2*i]);assert(out[2*i]==out[2*i+1]);}count+=got;zeros+=512-got;}
 }
 double frequency=double(crossings)*rate/count;
 std::cout<<"rate="<<rate<<" pitch="<<frequency<<"Hz missing_frames="<<zeros<<"\n";
 assert(std::abs(frequency-1000)<25);assert(zeros<rate/10);
}}
