#include "common/px_screen_interface.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
using namespace PokemonXUI;
int main(int argc,char**argv){assert(argc>1);std::ifstream f(std::string(argv[1])+"/commands.bgra",std::ios::binary);std::vector<unsigned char> p((std::istreambuf_iterator<char>(f)),{});assert(p.size()==80*60*4);CommandSignature signature;assert(!signature.Matches(p.data(),80,60));signature.Learn(p.data(),80,60);assert(signature.Matches(p.data(),80,60));assert(!signature.Return(3,p.data(),80,60,false));assert(!signature.Return(3,p.data(),80,60,true));assert(signature.Return(3,p.data(),80,60,true));
 // A different screen can satisfy the old three-color detector. It must not
 // cause the fullscreen auxiliary menu to close or show misleading crops.
 auto fake=p;for(int y=0;y<40;y++)for(int x=0;x<80;x++){auto q=fake.data()+(y*80+x)*4;q[0]=5;q[1]=5;q[2]=220;}
 assert(Classify(fake.data(),80,60)==3);assert(!signature.Matches(fake.data(),80,60));assert(!signature.Return(3,fake.data(),80,60,true));assert(!signature.Return(3,p.data(),80,60,true));assert(signature.Return(3,p.data(),80,60,true));signature.Reset();assert(!signature.valid);
 std::cout<<"PASS verified original command signature, legacy color false-positive regression, two independent return observations, battle reset\n";
}
