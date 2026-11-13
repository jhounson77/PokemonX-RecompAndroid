package org.pokemonx.recomp;
public class FrameRatesTest {
 static void check(boolean ok){if(!ok)throw new AssertionError();}
 public static void main(String[]args){FrameRates r=new FrameRates();r.sample(0,0,0);check(Float.isNaN(r.fps));r.sample(1000000000L,30,60);check(r.fps==30&&r.speed==100);r.sample(3000000000L,60,120);check(r.fps==15&&r.speed==50);r.reset();r.sample(10000000000L,60,120);check(Float.isNaN(r.fps));r.sample(11000000000L,60,120);check(r.fps==0&&r.speed==0);r.sample(12000000000L,0,0);check(Float.isNaN(r.fps));System.out.println("Frame rates: presentation/simulation separation, pause reset, stalls and new sessions OK");}
}
