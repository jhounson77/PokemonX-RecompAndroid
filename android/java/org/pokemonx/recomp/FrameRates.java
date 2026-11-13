package org.pokemonx.recomp;
final class FrameRates {
 private long time=-1,posted,emulated;
 float fps=Float.NaN,speed=Float.NaN;
 void reset(){time=-1;fps=speed=Float.NaN;}
 void sample(long now,long frames,long ticks){
  if(time>=0&&now>time&&frames>=posted&&ticks>=emulated){double seconds=(now-time)/1e9;fps=(float)((frames-posted)/seconds);speed=(float)((ticks-emulated)/seconds/60.0*100);}
  else fps=speed=Float.NaN;
  time=now;posted=frames;emulated=ticks;
 }
}
