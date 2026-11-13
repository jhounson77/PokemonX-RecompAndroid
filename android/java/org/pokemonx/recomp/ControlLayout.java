package org.pokemonx.recomp;
final class ControlLayout {
 static final int BUTTON=0,PILL=1,CIRCLE=2,DPAD=3;
 static final class Control {
  final String label;final int kind,bit;float x,y;final float rx,ry;
  Control(String label,int kind,int bit,float x,float y,float rx,float ry){this.label=label;this.kind=kind;this.bit=bit;this.x=x;this.y=y;this.rx=rx;this.ry=ry;}
 }
 static float clamp(float v,float low,float high){return Math.max(low,Math.min(Math.max(low,high),v));}
 static Control c(String label,int kind,int bit,float x,float y,float rx,float ry,float w,float h){return new Control(label,kind,bit,clamp(x,rx+1,w-rx-1),clamp(y,ry+1,h-ry-1),rx,ry);}
 static Control[] defaults(float w,float h,float density,boolean left,boolean overlay,int size){
  float r=Math.min(size*density*.5f,Math.min(h*.075f,w*.16f));
  float shoulderX=overlay?(left?54*density:w-54*density):w*.5f;
  float systemX=overlay?(left?145*density:w-145*density):w*.5f;
  float shoulderY=overlay?32*density:h*.075f,systemY=overlay?32*density:h*.185f;
  if(left){float centerX=overlay?68*density:w*.5f;float circle=Math.min(r*2.5f,Math.min(68*density,Math.min(w*.44f,h*.17f)));float dpad=Math.min(r*2.1f,Math.min(w*.43f,h*.14f));return new Control[]{
   c("L",PILL,512,shoulderX,shoulderY,r*1.7f,r*.8f,w,h),c("SELECT",PILL,4,systemX,systemY,r*1.9f,r*.64f,w,h),
   c("CirclePad",CIRCLE,0,centerX,h*.44f,circle,circle,w,h),c("Dpad",DPAD,0,centerX,h*.79f,dpad,dpad,w,h)};}
  float gap=r*1.65f,cx=overlay?w-16*density-gap-r:w*.5f,cy=h*.53f;
  return new Control[]{c("R",PILL,256,shoulderX,shoulderY,r*1.7f,r*.8f,w,h),c("START",PILL,8,systemX,systemY,r*1.9f,r*.64f,w,h),
   c("X",BUTTON,1024,cx,cy-gap,r,r,w,h),c("Y",BUTTON,2048,cx-gap,cy,r,r,w,h),c("A",BUTTON,1,cx+gap,cy,r,r,w,h),c("B",BUTTON,2,cx,cy+gap,r,r,w,h)};
 }
}
