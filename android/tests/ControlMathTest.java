package org.pokemonx.recomp;
public class ControlMathTest {
 static void check(boolean ok,String what){if(!ok)throw new AssertionError(what);}
 public static void main(String[]args){
  int[]a=ControlMath.circle(100,0,100);check(a[0]==154&&a[1]==0,"right rim");
  a=ControlMath.circle(0,-100,100);check(a[0]==0&&a[1]==154,"up must be positive HID y");
  a=ControlMath.circle(-100,0,100);check(a[0]==-154&&a[1]==0,"left rim");
  a=ControlMath.circle(4,-3,100);check(a[0]==0&&a[1]==0,"dead zone");
  a=ControlMath.circle(1000,1000,100);check(a[0]>0&&a[1]<0&&Math.hypot(a[0],a[1])<=155,"radial clamp");
  a=ControlMath.circle(50,0,100);check(a[0]>0&&a[0]<154,"analog partial travel");
  check(ControlMath.dpad(0,-60,100)==64,"digital up");check(ControlMath.dpad(60,60,100)==(16|128),"digital diagonal");check(ControlMath.dpad(0,0,100)==0,"digital release");
  for(boolean left:new boolean[]{true,false})for(boolean overlay:new boolean[]{true,false}){
   ControlLayout.Control[] controls=ControlLayout.defaults(overlay?1000:300,900,2.5f,left,overlay,44);
   check(controls.length==(left?4:6),"3DS control groups");
   for(ControlLayout.Control c:controls)check(c.x-c.rx>=0&&c.x+c.rx<=(overlay?1000:300)+.1f&&c.y-c.ry>=0&&c.y+c.ry<=900,"control bounds: "+c.label);
  }
  ControlLayout.Control[]right=ControlLayout.defaults(1000,900,2.5f,false,true,44);
  ControlLayout.Control x=right[2],y=right[3],aButton=right[4],b=right[5];
  check(x.y<y.y&&b.y>y.y&&y.x<x.x&&aButton.x>x.x,"Nintendo XYAB diamond");
  System.out.println("Circle Pad range/deadzone, independent digital pad and 3DS layout bounds: OK");
 }
}
