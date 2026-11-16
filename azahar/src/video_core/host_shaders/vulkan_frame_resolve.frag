// Display-only motion-compensated intermediate picture, with conservative pixel fallback.
#version 450
layout(location=0) in vec2 frag_tex_coord;
layout(location=0) out vec4 color;
layout(set=0,binding=0) uniform sampler2D images[3];
layout(push_constant,std140) uniform Info {
 mat4 model;vec4 input_size;vec4 output_size;
 int left_id;int right_id;int layer;int mode;vec4 clip;
};
vec2 Decode(vec2 value){return (round((value-0.5)*64.0)*0.5)*input_size.zw;}
bool Inside(vec2 p){return all(greaterThanEqual(p,vec2(0)))&&all(lessThanEqual(p,vec2(1)));}
void main(){
 vec2 uv=frag_tex_coord;
 if(mode==0){color=texture(images[0],uv);return;}
 vec2 a=uv,b=uv;
 for(int i=0;i<2;i++){
  vec2 flow_a=Decode(texture(images[2],a).rg);
  vec2 flow_b=Decode(texture(images[2],b).ba);
  a=uv-0.5*flow_a;b=uv-0.5*flow_b;
 }
 vec4 current=texture(images[1],a),previous=texture(images[0],b);
 float disagreement=dot(abs(current.rgb-previous.rgb),vec3(0.299,0.587,0.114));
 color=Inside(a)&&Inside(b)&&disagreement<0.12 ? mix(previous,current,0.5):texture(images[1],uv);
}
