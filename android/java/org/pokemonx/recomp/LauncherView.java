package org.pokemonx.recomp;

import android.content.Context;
import android.graphics.*;
import android.graphics.drawable.*;
import android.util.TypedValue;
import android.view.*;
import android.widget.*;
import java.io.IOException;

/** Native, accessible launcher controls over the original user-supplied artwork. */
final class LauncherView extends FrameLayout {
 private final TextView title,subtitle,platform,state,details,version;
 private final TextView[] actions=new TextView[3];
 private final int white=0xfff5f8fc,cyan=0xff59d9ff;
 private int selected;
 private float unit=1;
 private int layoutWidth=-1,layoutHeight=-1;
 LauncherView(Context context,boolean ready,boolean busy,String message,String build,
              Runnable onPlay,Runnable onSettings,Runnable onExit){
  super(context);setBackgroundColor(0xff061425);
  addView(new Backdrop(context),new LayoutParams(-1,-1));
  title=label("Pokémon X",white,true);subtitle=label("R E C O M P",cyan,true);
  platform=label("Android",0xffd9e4f1,false);
  Runnable[] callbacks={onPlay,onSettings,onExit};String[] labels={I18n.t("Jogar"),I18n.t("Configurações"),I18n.t("Sair")};
  for(int i=0;i<3;i++){
   final int index=i;TextView action=label(labels[i],white,i==0);actions[i]=action;
   action.setId(View.generateViewId());action.setClickable(true);action.setFocusable(true);
   action.setContentDescription(labels[i]);
   action.setOnClickListener(v->{select(index);callbacks[index].run();});
   action.setOnTouchListener((v,event)->{if(event.getActionMasked()==MotionEvent.ACTION_DOWN)select(index);return false;});
   action.setOnHoverListener((v,event)->{if(event.getActionMasked()==MotionEvent.ACTION_HOVER_ENTER&&v.isEnabled())select(index);return false;});
   action.setOnFocusChangeListener((v,focused)->{if(focused)select(index);});
   action.setOnKeyListener((v,key,event)->{
    if(key==KeyEvent.KEYCODE_DPAD_UP||key==KeyEvent.KEYCODE_DPAD_LEFT||key==KeyEvent.KEYCODE_DPAD_DOWN||key==KeyEvent.KEYCODE_DPAD_RIGHT){
     if(event.getAction()==KeyEvent.ACTION_DOWN){int delta=(key==KeyEvent.KEYCODE_DPAD_UP||key==KeyEvent.KEYCODE_DPAD_LEFT)?-1:1;int next=index;do{next=(next+delta+3)%3;}while(!actions[next].isEnabled());select(next);actions[next].requestFocus();}return true;
    }
    if(key==KeyEvent.KEYCODE_BUTTON_A){if(event.getAction()==KeyEvent.ACTION_UP)action.performClick();return true;}return false;
   });
  }
  actions[0].setEnabled(!busy);actions[0].setAlpha(busy?.4f:1f);
  for(int i=0;i<3;i++){actions[i].setNextFocusUpId(actions[(i+2)%3].getId());actions[i].setNextFocusDownId(actions[(i+1)%3].getId());}
  state=label(message.isEmpty()?I18n.t("Arquivos do jogo prontos."):message,message.isEmpty()?0xff8aeab3:0xffffd58a,false);
  // game.cxi is committed only after GameImporter finishes verification/rename.
  // No success dot or success text exists on screen before a committed import.
  state.setVisibility((ready&&!busy)||!message.isEmpty()?VISIBLE:GONE);
  state.setMaxLines(2);
  details=label("Nintendo 3DS · Pokémon X",0xffb4c9dd,false);
  version=label(I18n.t("Versão ")+build,0xffd6dfec,false);version.setGravity(Gravity.RIGHT|Gravity.CENTER_VERTICAL);
  selected=context.getSharedPreferences("settings",0).getInt("launcher_selection",0);if(selected<0||selected>2||!actions[selected].isEnabled())selected=1;
  select(selected);
  final int initialSelection=selected;post(()->{select(initialSelection);if(!isInTouchMode())actions[initialSelection].requestFocus();});
 }
 TextView statusView(){return state;}
 private TextView label(String text,int color,boolean bold){TextView view=new TextView(getContext());view.setText(text);view.setTextColor(color);view.setTypeface(Typeface.create("sans-serif",bold?Typeface.BOLD:Typeface.NORMAL));view.setIncludeFontPadding(false);view.setShadowLayer(2,0,1,0x80000000);view.setGravity(Gravity.CENTER_VERTICAL);addView(view);return view;}
 private void select(int index){selected=index;getContext().getSharedPreferences("settings",0).edit().putInt("launcher_selection",index).apply();
  for(int i=0;i<3;i++){TextView action=actions[i];if(action==null)continue;action.setSelected(i==index);
   GradientDrawable fill=new GradientDrawable(GradientDrawable.Orientation.TL_BR,i==index?new int[]{0xc0275677,0x7430526b}:new int[]{0x00000000,0x00000000});fill.setCornerRadius(14*unit);
   if(i==index)fill.setStroke(Math.max(1,Math.round(1.1f*unit)),cyan);
   action.setBackground(new RippleDrawable(android.content.res.ColorStateList.valueOf(0x25c4edff),fill,null));
  }
 }
 private void place(TextView v,int x,int y,int w,int h,float size){LayoutParams p=new LayoutParams(w,h,Gravity.TOP|Gravity.LEFT);p.leftMargin=x;p.topMargin=y;v.setLayoutParams(p);v.setTextSize(TypedValue.COMPLEX_UNIT_PX,size);}
 private void icon(TextView view,int kind,int size){Glyph glyph=new Glyph(kind);glyph.setBounds(0,0,size,size);view.setCompoundDrawables(glyph,null,null,null);view.setCompoundDrawablePadding(Math.round(16*unit));}
 @Override protected void onMeasure(int widthSpec,int heightSpec){
  // Set bounds BEFORE FrameLayout measures children. Mutating LayoutParams in
  // onSizeChanged happens after measurement and left full-screen measured
  // children offset by their new margins on the initial layout.
  int w=MeasureSpec.getSize(widthSpec),h=MeasureSpec.getSize(heightSpec);
  if(w!=layoutWidth||h!=layoutHeight){layoutWidth=w;layoutHeight=h;configureBounds(w,h);}
  super.onMeasure(widthSpec,heightSpec);
 }
 private void configureBounds(int w,int h){unit=Math.min(w/1280f,h/600f);float u=unit;int x=Math.round(w*.068f),top=Math.round(h*.15f);
  place(title,x,top,Math.round(w*.60f),Math.round(66*u),54*u);
  place(subtitle,x+Math.round(3*u),top+Math.round(66*u),Math.round(300*u),Math.round(28*u),20*u);
  place(platform,x+Math.round(3*u),top+Math.round(106*u),Math.round(220*u),Math.round(30*u),16*u);icon(platform,3,Math.round(22*u));
  int menu=Math.round(h*.43f),row=Math.round(50*u),width=Math.round(340*u),gap=Math.round(12*u);
  for(int i=0;i<3;i++){place(actions[i],x,menu+(row+gap)*i,width,row,(i==0?27:22)*u);actions[i].setPadding(Math.round(20*u),0,Math.round(16*u),0);icon(actions[i],i,Math.round(28*u));}
  int bottom=Math.round(h*.84f);
  place(state,x,bottom,Math.round(w*.57f),Math.round(33*u),15*u);
  if(state.getVisibility()==VISIBLE&&state.getText().toString().equals(I18n.t("Arquivos do jogo prontos.")))icon(state,4,Math.round(12*u));
  place(details,x+Math.round(25*u),bottom+Math.round(30*u),Math.round(w*.56f),Math.round(25*u),13*u);
  place(version,Math.round(w*.70f),bottom+Math.round(30*u),Math.round(w*.24f),Math.round(25*u),13*u);select(selected);
 }
 private static final class Glyph extends Drawable {
  private final int kind;private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
  Glyph(int kind){this.kind=kind;}
  @Override public void draw(Canvas canvas){canvas.save();canvas.translate(getBounds().left,getBounds().top);canvas.scale(getBounds().width()/32f,getBounds().height()/32f);p.setColor(kind==4?0xff0ee694:0xfff5f8fc);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(2.1f);p.setStrokeJoin(Paint.Join.ROUND);p.setStrokeCap(Paint.Cap.ROUND);
   if(kind==0){p.setStyle(Paint.Style.FILL);Path t=new Path();t.moveTo(7,3);t.lineTo(29,16);t.lineTo(7,29);t.close();canvas.drawPath(t,p);}
   if(kind==1){Path gear=new Path();for(int i=0;i<32;i++){double a=i*Math.PI/16;float r=(i%4==0||i%4==3)?15:11.8f;float x=16+(float)Math.cos(a)*r,y=16+(float)Math.sin(a)*r;if(i==0)gear.moveTo(x,y);else gear.lineTo(x,y);}gear.close();canvas.drawPath(gear,p);canvas.drawCircle(16,16,5,p);}
   if(kind==2){Path door=new Path();door.moveTo(14,3);door.lineTo(6,3);door.quadTo(2,3,2,7);door.lineTo(2,25);door.quadTo(2,29,6,29);door.lineTo(14,29);canvas.drawPath(door,p);canvas.drawLine(12,16,30,16,p);canvas.drawLine(23,9,30,16,p);canvas.drawLine(23,23,30,16,p);}
   if(kind==3){p.setStyle(Paint.Style.FILL);canvas.drawRoundRect(5,12,27,26,3,3,p);canvas.drawArc(5,3,27,23,180,180,true,p);p.setStyle(Paint.Style.STROKE);canvas.drawLine(9,5,6,1,p);canvas.drawLine(23,5,26,1,p);p.setColor(0xff061425);canvas.drawPoint(11,10,p);canvas.drawPoint(21,10,p);}
   if(kind==4){p.setStyle(Paint.Style.FILL);canvas.drawCircle(16,16,13,p);}canvas.restore();
  }
  @Override public void setAlpha(int a){p.setAlpha(a);}@Override public void setColorFilter(ColorFilter c){p.setColorFilter(c);}@Override public int getOpacity(){return PixelFormat.TRANSLUCENT;}
 }
 static final class Backdrop extends View {
  private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);private Bitmap art;
  Backdrop(Context c){super(c);try(java.io.InputStream stream=c.getAssets().open("launcher-background.jpg")){art=BitmapFactory.decodeStream(stream);}catch(IOException ignored){}}
  @Override protected void onDraw(Canvas c){super.onDraw(c);float w=getWidth(),h=getHeight();c.drawColor(0xff061425);
   if(art!=null){float width=h*art.getWidth()/art.getHeight();c.drawBitmap(art,null,new RectF(w-width,0,w,h),paint);}
   float left=art==null?0:Math.max(0,1-(h*art.getWidth()/art.getHeight())/w),start=Math.min(.72f,Math.max(.19f,left+.02f)),end=Math.min(.94f,Math.max(.56f,start+.18f));
   paint.setShader(new LinearGradient(0,0,w,0,new int[]{0xff061425,0xff061425,0x65061425,0x00061425},new float[]{0,start,end,1},Shader.TileMode.CLAMP));c.drawRect(0,0,w,h,paint);
   paint.setShader(new LinearGradient(0,0,0,h,new int[]{0x08061425,0x00061425,0xc9061425},new float[]{0,.65f,1},Shader.TileMode.CLAMP));c.drawRect(0,0,w,h,paint);paint.setShader(null);
  }
 }
}
