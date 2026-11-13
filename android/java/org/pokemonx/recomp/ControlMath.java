package org.pokemonx.recomp;
final class ControlMath {
 private ControlMath(){}
 static int[] circle(float dx,float dy,float radius){
  if(radius<=0)return new int[]{0,0};double distance=Math.hypot(dx,dy)/radius;
  if(distance<=.10)return new int[]{0,0};double magnitude=Math.min(1,(distance-.10)/.90);
  double length=Math.hypot(dx,dy);return new int[]{(int)Math.round(154*dx/length*magnitude),(int)Math.round(-154*dy/length*magnitude)};
 }
 static int dpad(float dx,float dy,float radius){
  if(radius<=0||Math.hypot(dx,dy)>radius*1.65)return 0;float threshold=radius*.23f;int mask=0;
  if(dx>threshold)mask|=16;if(dx<-threshold)mask|=32;if(dy>threshold)mask|=128;if(dy<-threshold)mask|=64;return mask;
 }
}
