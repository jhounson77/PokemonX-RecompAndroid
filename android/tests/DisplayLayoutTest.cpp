#include "../native/display_layout.h"
#include <stdexcept>
using namespace pxandroid;
int main(){
 auto main=Fit(1000,600,400,240);if(main.x!=0||main.y!=0||main.w!=1000||main.h!=600)throw std::runtime_error("main aspect ratio");
 auto actions=Fit(1000,600,320,240);if(actions.x!=100||actions.y!=0||actions.w!=800||actions.h!=600)throw std::runtime_error("action aspect ratio");
 auto wide=Fit(1600,600,400,240);if(wide.x!=300||wide.w!=1000||wide.h!=600)throw std::runtime_error("wide letterbox");
 auto high=Fit(1000,600,800,480);if(high.x!=main.x||high.y!=main.y||high.w!=main.w||high.h!=main.h)throw std::runtime_error("internal scale changed aspect ratio");
 auto empty=Fit(0,0,400,240);if(empty.w||empty.h)throw std::runtime_error("empty surface");
}
