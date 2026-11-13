#include "adapters/vulkan/render_scale.h"
#include <chrono>
#include <iostream>
using namespace hexlith::adapters::vulkan;
int main(){constexpr unsigned w=400,h=240,channels=4,scale=3;std::vector<std::uint8_t> input(w*h*channels),out(w*h*channels*scale*scale);for(unsigned i=0;i<input.size();++i)input[i]=i*37;std::uint64_t sum=0;
 auto run=[&](bool optimized){auto start=std::chrono::steady_clock::now();for(unsigned iteration=0;iteration<80;++iteration){if(optimized)ExpandRenderPixelsInto(input,out,w,h,channels,scale);else{std::vector<std::uint8_t> expanded(out.size());for(unsigned y=0;y<h*scale;++y)for(unsigned x=0;x<w*scale;++x)std::memcpy(expanded.data()+(y*w*scale+x)*channels,input.data()+((y/scale)*w+x/scale)*channels,channels);std::copy(expanded.begin(),expanded.end(),out.begin());}sum+=out[iteration];}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
 auto baseline=run(false),direct=run(true);std::cout<<"Synthetic 80 uploads at 3x: baseline "<<baseline<<" ms; direct row reuse "<<direct<<" ms; checksum "<<sum<<"\nCPU upload expansion only, not S25/game FPS.\n";}
