package org.pokemonx.recomp;
import android.content.*;
import android.os.*;
import android.widget.TextView;
import java.util.Locale;
import java.util.concurrent.*;
// One sample a second; PSS and battery reads run off the UI thread every 5s.
final class PerformanceHud {
 private final Context context;private final TextView view;
 private ScheduledExecutorService worker;private volatile int generation=0;
 PerformanceHud(Context context,TextView view){this.context=context.getApplicationContext();this.view=view;}
 synchronized void start(){stop();final int token=generation;worker=Executors.newSingleThreadScheduledExecutor(r->{Thread t=new Thread(r,"PerformanceStats");t.setPriority(Thread.MIN_PRIORITY);return t;});worker.scheduleAtFixedRate(new Runnable(){long sample=0;int pss=-1;float temperature=Float.NaN;
  public void run(){if(token!=generation)return;try{double[]performance=MainActivity.nativePerformance();double currentFps=performance[0],currentSpeed=performance[1];if(sample++%5==0){Debug.MemoryInfo memory=new Debug.MemoryInfo();Debug.getMemoryInfo(memory);pss=memory.getTotalPss();Intent battery=context.registerReceiver(null,new IntentFilter(Intent.ACTION_BATTERY_CHANGED));temperature=battery!=null&&battery.hasExtra(BatteryManager.EXTRA_TEMPERATURE)?battery.getIntExtra(BatteryManager.EXTRA_TEMPERATURE,0)/10f:Float.NaN;}
   String fps=Double.isNaN(currentFps)?"—":String.format(Locale.US,"%.1f",currentFps),speed=Double.isNaN(currentSpeed)?"—":String.format(Locale.US,"%.0f",currentSpeed),ram=pss<0?"—":String.format(Locale.US,"%.0f",pss/1024f),temp=Float.isNaN(temperature)?"—":String.format(Locale.US,"%.1f",temperature);String visual=performance.length>2&&!Double.isNaN(performance[2])?String.format(Locale.US,"%.1f",performance[2]):"—";String label=I18n.t("Jogo ")+fps+I18n.t(" · Tela ")+visual+I18n.t("  ·  Vel. ")+speed+"%\nRAM "+ram+I18n.t(" MB  ·  Bat. ")+temp+" °C";view.post(()->{if(token==generation)view.setText(label);});
  }catch(RuntimeException ignored){view.post(()->{if(token==generation)view.setText(I18n.t("Métricas indisponíveis"));});}}
 },0,1,TimeUnit.SECONDS);}
 synchronized void stop(){generation++;if(worker!=null){worker.shutdownNow();worker=null;}}
}
