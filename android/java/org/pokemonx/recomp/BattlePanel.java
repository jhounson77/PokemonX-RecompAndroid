package org.pokemonx.recomp;

import android.content.Context;
import android.graphics.*;
import android.view.*;
import java.util.Arrays;

/** Original battle textures with shared touch geometry and controller focus. */
final class BattlePanel extends View {
 private final MainActivity activity;private final SurfaceView surface;
 private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
 private final int[] surfacePosition=new int[2],viewPosition=new int[2];
 private final Typeface labelTypeface=Typeface.create("sans-serif-medium",Typeface.NORMAL);
 private int[] moveStats=new int[15],nextStats=new int[15];
 private final String[] labels={I18n.t("Lutar"),I18n.t("Bolsa"),I18n.t("Fugir"),"Pokémon"};
 private static final int[] colors={0xe5b5363c,0xe59c7422,0xe52d6eaa,0xe5378048};
 private final RectF[] rectangles={new RectF(),new RectF(),new RectF(),new RectF(),new RectF()};
 private final RectF ring=new RectF();private final Path marker=new Path();
 private float[] regions=new float[34],nextRegions=new float[34];private float offsetX,offsetY;
 private RectF gestureRect=new RectF();
 private int shownMode=0,pointer=-1,gestureMode=-1,pressed=-1;private boolean validEnd;
 private final Runnable tick=new Runnable(){public void run(){if(isAttachedToWindow()){if(refresh())invalidate();postDelayed(this,((int)regions[0]==3||(int)regions[0]==4)?33:100);}}};
 BattlePanel(MainActivity owner,SurfaceView screen){super(owner);activity=owner;surface=screen;setBackgroundColor(Color.TRANSPARENT);setContentDescription(I18n.t("Comandos de batalha"));setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);}
 private boolean refresh(){
  if(surface.getWidth()<=0||surface.getHeight()<=0)return false;
  surface.getLocationInWindow(surfacePosition);getLocationInWindow(viewPosition);
  float x=surfacePosition[0]-viewPosition[0],y=surfacePosition[1]-viewPosition[1];
  boolean changed=x!=offsetX||y!=offsetY;offsetX=x;offsetY=y;
  MainActivity.nativeBattleRegions(surface.getWidth(),surface.getHeight(),nextRegions);
  changed|=!Arrays.equals(nextRegions,regions);float[] old=regions;regions=nextRegions;nextRegions=old;activity.syncScreenMode((int)regions[2]);
  if((int)regions[0]==4)MainActivity.nativeMoveStats(nextStats);else Arrays.fill(nextStats,0);
  changed|=!Arrays.equals(nextStats,moveStats);int[] oldStats=moveStats;moveStats=nextStats;nextStats=oldStats;
  for(int i=0;i<(int)regions[1];i++){int p=3+i*6;rectangles[i].set(offsetX+regions[p],offsetY+regions[p+1],offsetX+regions[p]+regions[p+2],offsetY+regions[p+1]+regions[p+3]);}return changed;
 }

 private RectF rect(int index){return rectangles[index];}
 @Override protected void onDraw(Canvas canvas){super.onDraw(canvas);shownMode=(int)regions[0];int count=(int)regions[1];
  if(shownMode!=3&&shownMode!=4)return;
  int focus=pressed>=0?pressed:(regions.length>3+count*6?(int)regions[3+count*6]:0);
  for(int i=0;i<count;i++){
   int p=3+i*6;if(regions[p+5]==0)continue;RectF r=rect(i);float radius=r.height()*.14f;
   if(regions[p+4]==0){
    paint.setStyle(Paint.Style.FILL);paint.setColor(shownMode==3?colors[i]:0xdd182b41);canvas.drawRoundRect(r,radius,radius,paint);
    paint.setStyle(Paint.Style.STROKE);paint.setColor(0x70ffffff);paint.setStrokeWidth(Math.max(1,r.height()*.012f));canvas.drawRoundRect(r,radius,radius,paint);
    paint.setStyle(Paint.Style.FILL);paint.setColor(Color.WHITE);paint.setTypeface(labelTypeface);paint.setTextAlign(Paint.Align.CENTER);paint.setTextSize(Math.min(r.height()*.36f,r.width()*.18f));Paint.FontMetrics fm=paint.getFontMetrics();canvas.drawText(shownMode==3?labels[i]:I18n.t("B · Voltar"),r.centerX(),r.centerY()-(fm.ascent+fm.descent)/2,paint);
   }
   if(shownMode==4&&i<4&&moveStats!=null&&moveStats.length>=15&&moveStats[0]==1){
    int pp=moveStats[4+i*3],max=moveStats[5+i*3];paint.setStyle(Paint.Style.FILL);paint.setColor(pp==0?0xffff8888:Color.WHITE);paint.setTypeface(labelTypeface);paint.setTextAlign(Paint.Align.CENTER);paint.setTextSize(r.height()*.27f);Paint.FontMetrics fm=paint.getFontMetrics();canvas.drawText("PP "+pp+"/"+max,r.left+r.width()*.69f,r.top+r.height()*.72f-(fm.ascent+fm.descent)/2,paint);
   }
   if(i==focus&&i<4){
    paint.setStyle(Paint.Style.STROKE);paint.setColor(0xffffdf83);paint.setStrokeWidth(Math.max(2,r.height()*.045f));ring.set(r);ring.inset(-2,-2);canvas.drawRoundRect(ring,radius,radius,paint);
    paint.setStyle(Paint.Style.FILL);paint.setColor(0xffffdf83);float d=r.height()*.10f;marker.reset();marker.moveTo(r.left+d,r.centerY()-d);marker.lineTo(r.left+d*2.2f,r.centerY());marker.lineTo(r.left+d,r.centerY()+d);marker.close();canvas.drawPath(marker,paint);
   }
  }
 }

 private int hit(float x,float y){for(int i=(int)regions[1]-1;i>=0;i--)if(regions[3+i*6+5]!=0&&rect(i).contains(x,y))return i;return -1;}
 private void release(boolean commit){activity.touchFromInterface(-1,-1);if(commit&&pressed>=0&&validEnd)MainActivity.nativeCommitUI(gestureMode,pressed);pointer=-1;pressed=-1;gestureMode=-1;validEnd=false;invalidate();}
 @Override public boolean onTouchEvent(android.view.MotionEvent e){int action=e.getActionMasked();
  if(action==MotionEvent.ACTION_DOWN){int previous=shownMode;refresh();int live=(int)regions[0];if(live!=previous){invalidate();return true;}int index=hit(e.getX(),e.getY());if(index<0)return false;int[] point=MainActivity.nativeMapTouch(e.getX()-offsetX,e.getY()-offsetY,surface.getWidth(),surface.getHeight(),live);if(point==null||point[0]<0)return false;pointer=e.getPointerId(0);gestureMode=live;pressed=index;gestureRect.set(rect(index));validEnd=true;activity.touchFromInterface(point[0],point[1]);invalidate();return true;}
  if(pointer<0)return false;
  if(action==MotionEvent.ACTION_CANCEL){release(false);return true;}
  if(action==MotionEvent.ACTION_UP||(action==MotionEvent.ACTION_POINTER_UP&&e.getPointerId(e.getActionIndex())==pointer)){validEnd=validEnd&&gestureRect.contains(e.getX(e.getActionIndex()),e.getY(e.getActionIndex()));release(action==MotionEvent.ACTION_UP);return true;}
  if(action==MotionEvent.ACTION_MOVE){int i=e.findPointerIndex(pointer);if(i>=0){int[] point=MainActivity.nativeMapTouch(e.getX(i)-offsetX,e.getY(i)-offsetY,surface.getWidth(),surface.getHeight(),gestureMode);validEnd=point!=null&&point[0]>=0&&gestureRect.contains(e.getX(i),e.getY(i));activity.touchFromInterface(validEnd?point[0]:-1,validEnd?point[1]:-1);}return true;}return true;
 }
 @Override protected void onAttachedToWindow(){super.onAttachedToWindow();post(tick);}
 @Override protected void onDetachedFromWindow(){removeCallbacks(tick);if(pointer>=0)release(false);super.onDetachedFromWindow();}
 @Override public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);if(!focus&&pointer>=0)release(false);}
}
