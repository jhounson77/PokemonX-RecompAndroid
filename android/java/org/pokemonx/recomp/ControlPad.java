package org.pokemonx.recomp;
import android.content.*;
import android.graphics.*;
import android.view.*;
import java.util.*;
final class ControlPad extends View {
 interface Listener{void changed(int mask,int circleX,int circleY);}
 final boolean left,editing,overlay;final SharedPreferences prefs;final Listener listener;
 final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);final Map<Integer,Touch> touches=new HashMap<>();
 static final class Touch{int control;float x,y;Touch(int control,float x,float y){this.control=control;this.x=x;this.y=y;}}
 int dragging=-1;float downX,downY;float[]origin;
 ControlPad(Context context,boolean left,boolean editing,boolean overlay,Listener listener){super(context);this.left=left;this.editing=editing;this.overlay=overlay;this.listener=listener;prefs=context.getSharedPreferences("controls",0);setBackgroundColor(overlay?Color.TRANSPARENT:0xff0b1119);setContentDescription(editing?I18n.t("Arraste os controles para reposicionar"):I18n.t("Controles Nintendo 3DS"));}
 float dp(float n){return n*getResources().getDisplayMetrics().density;}
 String prefix(){return "outline_v1_"+(overlay?"overlay_":"docked_")+(left?"left_":"right_");}
 String key(ControlLayout.Control c){return prefix()+c.label;}
 ControlLayout.Control[] layout(){ControlLayout.Control[]controls=ControlLayout.defaults(getWidth(),getHeight(),getResources().getDisplayMetrics().density,left,overlay,prefs.getInt("size",44));for(ControlLayout.Control c:controls){c.x=ControlLayout.clamp(prefs.getFloat(key(c)+"_x",getWidth()>0?c.x/getWidth():.5f)*getWidth(),c.rx+1,getWidth()-c.rx-1);c.y=ControlLayout.clamp(prefs.getFloat(key(c)+"_y",getHeight()>0?c.y/getHeight():.5f)*getHeight(),c.ry+1,getHeight()-c.ry-1);}return controls;}
 boolean held(int index){for(Touch t:touches.values())if(t.control==index)return true;return false;}
 int hit(ControlLayout.Control[]controls,float x,float y){for(int i=controls.length-1;i>=0;i--){ControlLayout.Control c=controls[i];float nx=(x-c.x)/c.rx,ny=(y-c.y)/c.ry;if(c.kind==ControlLayout.PILL?(Math.abs(nx)<=1.08&&Math.abs(ny)<=1.16):(nx*nx+ny*ny<=1.15))return i;}return -1;}
 void outline(Canvas canvas,ControlLayout.Control c,boolean pressed){paint.setStyle(Paint.Style.FILL);paint.setColor(pressed?0x28ffffff:0x08000000);if(c.kind==ControlLayout.PILL)canvas.drawRoundRect(c.x-c.rx,c.y-c.ry,c.x+c.rx,c.y+c.ry,c.ry*.85f,c.ry*.85f,paint);else canvas.drawCircle(c.x,c.y,c.rx,paint);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(dp(1.15f));paint.setColor(editing?0xc86cdddd:pressed?0xeeffffff:0x98ffffff);if(c.kind==ControlLayout.PILL)canvas.drawRoundRect(c.x-c.rx,c.y-c.ry,c.x+c.rx,c.y+c.ry,c.ry*.85f,c.ry*.85f,paint);else canvas.drawCircle(c.x,c.y,c.rx,paint);paint.setStyle(Paint.Style.FILL);}
 @Override protected void onDraw(Canvas canvas){super.onDraw(canvas);ControlLayout.Control[]controls=layout();paint.setTypeface(Typeface.create("sans-serif-light",Typeface.NORMAL));
  for(int i=0;i<controls.length;i++){ControlLayout.Control c=controls[i];boolean pressed=held(i);outline(canvas,c,pressed);
   if(c.kind==ControlLayout.CIRCLE){float knobX=c.x,knobY=c.y;for(Touch t:touches.values())if(t.control==i){double d=Math.hypot(t.x-c.x,t.y-c.y),fraction=d>0?Math.min(1,d/c.rx)/d:0;knobX+=(float)((t.x-c.x)*fraction*c.rx*.55);knobY+=(float)((t.y-c.y)*fraction*c.ry*.55);break;}paint.setColor(pressed?0x25ffffff:0x12ffffff);canvas.drawCircle(knobX,knobY,c.rx*.43f,paint);paint.setStyle(Paint.Style.STROKE);paint.setColor(0xaaffffff);paint.setStrokeWidth(dp(1.1f));canvas.drawCircle(knobX,knobY,c.rx*.43f,paint);paint.setStyle(Paint.Style.FILL);continue;}
   if(c.kind==ControlLayout.DPAD){int bits=0;for(Touch t:touches.values())if(t.control==i)bits|=ControlMath.dpad(t.x-c.x,t.y-c.y,c.rx);for(int direction=0;direction<4;direction++){canvas.save();canvas.rotate(direction*90,c.x,c.y);paint.setColor((bits&(new int[]{64,16,128,32})[direction])!=0?0xd0ffffff:0x60ffffff);Path triangle=new Path();triangle.moveTo(c.x,c.y-c.ry*.78f);triangle.lineTo(c.x-c.rx*.27f,c.y-c.ry*.31f);triangle.lineTo(c.x+c.rx*.27f,c.y-c.ry*.31f);triangle.close();canvas.drawPath(triangle,paint);canvas.restore();}continue;}
   paint.setColor(pressed?Color.WHITE:0xc8ffffff);paint.setTextAlign(Paint.Align.CENTER);paint.setTextSize(c.kind==ControlLayout.PILL?(c.label.length()>2?c.ry*.95f:c.ry*1.3f):c.ry*.95f);Paint.FontMetrics fm=paint.getFontMetrics();canvas.drawText(c.label,c.x,c.y-(fm.ascent+fm.descent)/2,paint);
  }
 }
 void report(){int mask=0,cx=0,cy=0;ControlLayout.Control[]controls=layout();for(Touch t:touches.values()){if(t.control<0||t.control>=controls.length)continue;ControlLayout.Control c=controls[t.control];if(c.kind==ControlLayout.CIRCLE){int[]axis=ControlMath.circle(t.x-c.x,t.y-c.y,c.rx);cx=axis[0];cy=axis[1];}else if(c.kind==ControlLayout.DPAD)mask|=ControlMath.dpad(t.x-c.x,t.y-c.y,c.rx);else mask|=c.bit;}listener.changed(mask,cx,cy);invalidate();}
 @Override public boolean onTouchEvent(MotionEvent e){int action=e.getActionMasked(),index=e.getActionIndex(),id=e.getPointerId(index);ControlLayout.Control[]controls=layout();int selected=hit(controls,e.getX(index),e.getY(index));if(overlay&&!editing&&action==MotionEvent.ACTION_DOWN&&selected<0)return false;
  if(editing){if(action==MotionEvent.ACTION_DOWN){dragging=selected;downX=e.getX();downY=e.getY();if(dragging>=0)origin=new float[]{controls[dragging].x,controls[dragging].y};}if(action==MotionEvent.ACTION_MOVE&&dragging>=0){ControlLayout.Control c=controls[dragging];float x=ControlLayout.clamp(origin[0]+e.getX()-downX,c.rx+1,getWidth()-c.rx-1),y=ControlLayout.clamp(origin[1]+e.getY()-downY,c.ry+1,getHeight()-c.ry-1);prefs.edit().putFloat(key(c)+"_x",x/getWidth()).putFloat(key(c)+"_y",y/getHeight()).apply();invalidate();}if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL)dragging=-1;return true;}
  if(action==MotionEvent.ACTION_DOWN||action==MotionEvent.ACTION_POINTER_DOWN){if(selected>=0)touches.put(id,new Touch(selected,e.getX(index),e.getY(index)));}
  if(action==MotionEvent.ACTION_MOVE){for(int j=0;j<e.getPointerCount();j++){int pid=e.getPointerId(j);Touch t=touches.get(pid);if(t==null)continue;t.x=e.getX(j);t.y=e.getY(j);ControlLayout.Control c=controls[t.control];if(c.kind==ControlLayout.BUTTON||c.kind==ControlLayout.PILL){int next=hit(controls,t.x,t.y);if(next>=0&&(controls[next].kind==ControlLayout.BUTTON||controls[next].kind==ControlLayout.PILL))t.control=next;else touches.remove(pid);}}}
  if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_POINTER_UP)touches.remove(id);if(action==MotionEvent.ACTION_CANCEL)touches.clear();report();return true;
 }
 @Override public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);if(!focus){touches.clear();dragging=-1;report();}}
 @Override protected void onDetachedFromWindow(){touches.clear();listener.changed(0,0,0);super.onDetachedFromWindow();}
}
