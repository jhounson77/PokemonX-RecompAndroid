package org.pokemonx.recomp;
import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;
public class ImporterTest {
 static void put(byte[]b,int o,long v,int n){for(int i=0;i<n;i++)b[o+i]=(byte)(v>>(i*8));}
 static byte[] digest(byte[]data,int off,int len)throws Exception{MessageDigest d=MessageDigest.getInstance("SHA-256");d.update(data,off,len);return d.digest();}
 static byte[] image()throws Exception{
  byte[]b=new byte[3584];System.arraycopy(new byte[]{'N','C','C','H'},0,b,256,4);
  put(b,0x104,7,4);put(b,0x118,0x0004000000055D00L,8);put(b,0x180,1024,4);b[0x18f]=4;
  put(b,0x1a0,5,4);put(b,0x1a4,1,4);put(b,0x1a8,1,4);put(b,0x1b0,6,4);put(b,0x1b4,1,4);put(b,0x1b8,1,4);
  System.arraycopy(digest(b,512,1024),0,b,0x160,32);System.arraycopy(digest(b,2560,512),0,b,0x1c0,32);System.arraycopy(digest(b,3072,512),0,b,0x1e0,32);return b;
 }
 static void rejects(Path dir,byte[]bytes,String label)throws Exception{
  Path in=dir.resolve(label+".input"),out=dir.resolve(label+".cxi");Files.write(in,bytes);boolean failed=false;
  try{GameImporter.prepareFile(in.toFile(),out.toFile(),text->{});}catch(Exception expected){failed=true;}
  if(!failed||Files.exists(out))throw new AssertionError(label+" should fail without publishing a result");
 }
 public static void main(String[]args)throws Exception{
  Path dir=Files.createTempDirectory("px-importer-test");byte[]b=image();Path in=dir.resolve("valid.input"),out=dir.resolve("valid.cxi");Files.write(in,b);GameImporter.prepareFile(in.toFile(),out.toFile(),text->{});if(!Arrays.equals(b,Files.readAllBytes(out)))throw new AssertionError("Clear input changed");
  byte[]bad=b.clone();bad[2560]=1;rejects(dir,bad,"corrupt-exefs");bad=b.clone();bad[0x118]=1;rejects(dir,bad,"wrong-title");rejects(dir,Arrays.copyOf(b,b.length-1),"truncated");
  System.out.println("Importer: clear NCCH preserved; corrupt ExeFS, wrong title and truncation rejected");
 }
}
