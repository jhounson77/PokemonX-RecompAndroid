package org.pokemonx.recomp;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.view.Gravity;
import android.view.View;
import android.widget.*;
import java.util.function.Consumer;
import java.util.function.IntConsumer;

/** Scrollable native settings, with independent navigation and content columns. */
final class SettingsView extends FrameLayout {
 static final int WHITE=0xfff1f6fc,MUTED=0xff9cb1c8,ACCENT=0xff59d9ff;
 final LinearLayout content;
 final TextView status;
 private final boolean busy;
 private int dp(float n){return Math.round(n*getResources().getDisplayMetrics().density);}
 private TextView label(String value,int size,int color){TextView t=new TextView(getContext());t.setText(value);t.setTextSize(size);t.setTextColor(color);t.setIncludeFontPadding(false);return t;}
 private android.graphics.drawable.Drawable surface(boolean selected){GradientDrawable g=new GradientDrawable();g.setColor(selected?0xc021425c:0xc0102235);g.setCornerRadius(dp(12));g.setStroke(dp(1),selected?0xff59d9ff:0x303e617f);return new RippleDrawable(ColorStateList.valueOf(0x2059d9ff),g,null);}
 SettingsView(Context c,int selected,boolean busy,String message,Runnable back,IntConsumer select){
  super(c);this.busy=busy;addView(new LauncherView.Backdrop(c),new LayoutParams(-1,-1));View shade=new View(c);shade.setBackgroundColor(0x85061425);addView(shade,new LayoutParams(-1,-1));
  LinearLayout shell=new LinearLayout(c);shell.setOrientation(LinearLayout.VERTICAL);shell.setPadding(dp(24),dp(12),dp(24),dp(12));addView(shell,new LayoutParams(-1,-1));
  LinearLayout header=new LinearLayout(c);header.setGravity(Gravity.CENTER_VERTICAL);TextView title=label(I18n.t("Configurações"),23,WHITE);title.setTypeface(Typeface.create("sans-serif-medium",0));header.addView(title,new LinearLayout.LayoutParams(0,dp(46),1));TextView close=label(I18n.t("‹  Voltar"),14,ACCENT);close.setGravity(Gravity.CENTER);close.setBackground(surface(false));close.setFocusable(true);close.setOnClickListener(v->back.run());header.addView(close,new LinearLayout.LayoutParams(dp(100),dp(40)));shell.addView(header);
  LinearLayout body=new LinearLayout(c);LinearLayout.LayoutParams bodyParams=new LinearLayout.LayoutParams(-1,0,1);bodyParams.topMargin=dp(12);shell.addView(body,bodyParams);
  LinearLayout nav=new LinearLayout(c);nav.setOrientation(LinearLayout.VERTICAL);int width=Math.min(dp(176),Math.round(getResources().getDisplayMetrics().widthPixels*.25f));body.addView(nav,new LinearLayout.LayoutParams(width,-1));
  String[] names={I18n.t("Jogo e arquivos"),I18n.t("Imagem e telas"),I18n.t("Controles"),I18n.t("Sistema")};for(int i=0;i<names.length;i++){final int index=i;TextView tab=label(names[i],14,selected==i?ACCENT:MUTED);tab.setGravity(Gravity.CENTER_VERTICAL);tab.setPadding(dp(14),0,dp(8),0);tab.setBackground(surface(selected==i));tab.setFocusable(true);tab.setSelected(selected==i);tab.setOnClickListener(v->select.accept(index));LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,dp(46));p.bottomMargin=dp(8);nav.addView(tab,p);}
  TextView brand=label("POKÉMON X\nRECOMP · V0.1.0 Beta",11,MUTED);brand.setPadding(dp(12),dp(14),0,0);nav.addView(brand);
  ScrollView scroll=new ScrollView(c);scroll.setFillViewport(false);scroll.setClipToPadding(false);scroll.setPadding(dp(18),0,dp(2),dp(8));body.addView(scroll,new LinearLayout.LayoutParams(0,-1,1));content=new LinearLayout(c);content.setOrientation(LinearLayout.VERTICAL);scroll.addView(content,new ScrollView.LayoutParams(-1,-2));
  status=label(message,12,MUTED);status.setPadding(dp(4),0,dp(4),dp(10));status.setVisibility(message.isEmpty()?GONE:VISIBLE);content.addView(status);
 }
 void heading(String title,String description){TextView t=label(title,19,WHITE);t.setTypeface(Typeface.create("sans-serif-medium",0));t.setPadding(dp(4),dp(2),0,dp(8));content.addView(t);note(description);}
 void note(String value){TextView t=label(value,12,MUTED);t.setPadding(dp(4),0,dp(4),dp(12));content.addView(t);}
 private LinearLayout row(String title,String description){LinearLayout row=new LinearLayout(getContext());row.setGravity(Gravity.CENTER_VERTICAL);row.setPadding(dp(14),dp(12),dp(14),dp(12));row.setBackground(surface(false));LinearLayout copy=new LinearLayout(getContext());copy.setOrientation(LinearLayout.VERTICAL);copy.addView(label(title,14,WHITE));if(!description.isEmpty()){TextView d=label(description,11,MUTED);d.setPadding(0,dp(5),dp(6),0);copy.addView(d);}row.addView(copy,new LinearLayout.LayoutParams(0,-2,1));LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,-2);p.bottomMargin=dp(8);content.addView(row,p);return row;}
 void action(String title,String description,boolean available,Runnable run){LinearLayout r=row(title,description);TextView arrow=label("›",23,ACCENT);r.addView(arrow);r.setFocusable(true);r.setEnabled(available);r.setAlpha(available?1f:.42f);r.setOnClickListener(v->run.run());}
 void toggle(String title,String description,boolean checked,Consumer<Boolean> changed){LinearLayout r=row(title,description);Switch control=new Switch(getContext());control.setContentDescription(title);control.setChecked(checked);control.setThumbTintList(new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked},new int[]{}},new int[]{ACCENT,0xff889aad}));control.setTrackTintList(ColorStateList.valueOf(0xff29475d));control.setEnabled(!busy);control.setOnCheckedChangeListener((b,on)->changed.accept(on));r.addView(control);r.setEnabled(!busy);r.setOnClickListener(v->{if(!busy)control.toggle();});r.setFocusable(!busy);}
 void choice(String title,String[] values,int selected,IntConsumer changed){int safe=Math.max(0,Math.min(values.length-1,selected));action(title,values[safe],!busy,()->{AlertDialog d=new AlertDialog.Builder(new android.view.ContextThemeWrapper(getContext(),android.R.style.Theme_Material_Dialog_Alert)).setTitle(title).setSingleChoiceItems(values,safe,(dialog,index)->{changed.accept(index);dialog.dismiss();}).setNegativeButton(I18n.t("Cancelar"),null).create();d.setOnShowListener(v->{d.getButton(-2).setTextColor(ACCENT);});d.show();});}
}
