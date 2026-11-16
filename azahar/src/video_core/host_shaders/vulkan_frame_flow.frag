// Motion estimation on a small grid. Never reads or writes guest state.
#version 450
layout(location=0) in vec2 frag_tex_coord;
layout(location=0) out vec4 color;
layout(set=0,binding=0) uniform sampler2D images[3];
layout(push_constant,std140) uniform Info {
 mat4 model;vec4 input_size;vec4 output_size;
 int left_id;int right_id;int layer;int mode;vec4 clip;
};
vec3 ReadImage(bool current,vec2 uv){return current?texture(images[1],uv).rgb:texture(images[0],uv).rgb;}
float Cost(vec2 uv,vec2 offset,bool backwards){
 float result=0.0;
 const vec2 points[9]=vec2[9](vec2(0),vec2(-6,0),vec2(6,0),vec2(0,-6),vec2(0,6),vec2(-6,-6),vec2(6,-6),vec2(-6,6),vec2(6,6));
 for(int i=0;i<9;i++){
  vec2 p=uv+points[i]*input_size.zw;
  vec3 a=ReadImage(backwards,p),b=ReadImage(!backwards,p+offset);
  result+=dot(abs(a-b),vec3(0.299,0.587,0.114));
 }
 return result;
}
vec2 Find(bool backwards){
 vec2 best=vec2(0);float best_cost=Cost(frag_tex_coord,best,backwards);
 for(float step_size=8.0;step_size>=0.5;step_size*=0.5){
  vec2 center=best;
  for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){
   vec2 v=center+vec2(x,y)*step_size*input_size.zw;
   if(any(greaterThan(abs(v),16.0*input_size.zw)))continue;
   float c=Cost(frag_tex_coord,v,backwards)+dot(v/input_size.zw,v/input_size.zw)*0.00001;
   if(c<best_cost){best_cost=c;best=v;}
  }
 }
 return best/(32.0*input_size.zw)+0.5;
}
void main(){color=vec4(Find(true),Find(false));}
