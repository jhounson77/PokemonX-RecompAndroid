package org.pokemonx.recomp;
import java.io.*;import java.nio.*;import java.nio.file.*;import java.util.*;import java.util.zip.*;
public final class TextureModsTest {
 static final String A="tex1_8x8_0000000000000001_13_mip0.png",B="tex1_8x8_0000000000000002_13_mip0.png",C="tex1_8x8_0000000000000003_13_mip0.png";
 static byte[] png(int marker){return ByteBuffer.allocate(25).putLong(0x89504e470d0a1a0aL).putInt(13).putInt(0x49484452).putInt(8).putInt(8).put((byte)marker).array();}
 static File zip(Path dir,String label,String[] names,int value)throws Exception{File f=dir.resolve(label+".zip").toFile();try(ZipOutputStream z=new ZipOutputStream(new FileOutputStream(f))){for(String n:names){z.putNextEntry(new ZipEntry(n));z.write(png(value));z.closeEntry();}}return f;}
 static void check(boolean v,String message){if(!v)throw new AssertionError(message);}
 static int value(File files,String name)throws Exception{return Files.readAllBytes(new File(ModInstaller.directory(files),name).toPath())[24];}
 static void restart()throws Exception{java.lang.reflect.Field f=TextureMods.class.getDeclaredField("ready");f.setAccessible(true);((Set<?>)f.get(null)).clear();}
 public static void main(String[] args)throws Exception{
  Path temp=Files.createTempDirectory("texture-mods-test");File files=temp.resolve("files").toFile();
  File first=zip(temp,"first",new String[]{"Nested/"+A,B},1),second=zip(temp,"second",new String[]{A,C},2);
  ModInstaller.extract(first,files,"Legacy");check(TextureMods.list(files).size()==1,"legacy migration");String a=TextureMods.list(files).get(0).id;
  ModInstaller.install(second,files,"Second");List<TextureMods.Pack> list=TextureMods.list(files);String b=list.get(1).id;
  check(list.size()==2&&value(files,A)==2&&value(files,B)==1&&value(files,C)==2,"coexistence and newest priority");
  TextureMods.change(files,b,0);check(value(files,A)==1&&!new File(ModInstaller.directory(files),C).exists(),"disable restores underlying pack");
  TextureMods.change(files,b,0);TextureMods.change(files,a,1);check(value(files,A)==1&&value(files,C)==2,"priority changes overlap only");
  UpdateInstaller.remove(ModInstaller.directory(files));restart();check(TextureMods.list(files).get(1).id.equals(a)&&value(files,A)==1,"persistent order and recovery");
  for(String bad:new String[]{"../"+A,"code.ips"}){try{ModInstaller.install(zip(temp,"bad"+bad.length(),new String[]{bad},9),files,"Bad");throw new AssertionError("unsafe ZIP accepted");}catch(IOException expected){}check(TextureMods.list(files).size()==2&&value(files,A)==1,"failed ZIP changed mods");}
  File indexTemp=new File(files,"texture-mods/index.tmp");check(indexTemp.mkdir(),"fault setup");try{TextureMods.change(files,a,0);throw new AssertionError("fault not injected");}catch(IOException expected){}check(value(files,A)==1&&TextureMods.list(files).get(1).enabled,"failed index commit rollback");indexTemp.delete();
  File source=new File(files,"texture-mods/"+a),hidden=new File(files,"hidden-source");check(source.renameTo(hidden),"source fault setup");try{TextureMods.change(files,b,1);throw new AssertionError("missing source accepted");}catch(IOException expected){}check(value(files,A)==1,"staging failure erased live overlay");check(hidden.renameTo(source),"restore source");
  TextureMods.change(files,a,2);check(value(files,A)==2&&value(files,C)==2&&!new File(ModInstaller.directory(files),B).exists(),"remove restores older mod");
  String alternate="tex1_16x16_0000000000000001_14.png";ModInstaller.install(zip(temp,"hash-conflict",new String[]{alternate},3),files,"Hash owner");check(value(files,alternate)==3&&!new File(ModInstaller.directory(files),A).exists(),"native hash collision priority");
  for(TextureMods.Pack p:TextureMods.list(files))TextureMods.change(files,p.id,0);check(!ModInstaller.installed(files),"all disabled");restart();check(TextureMods.list(files).size()==2&&!ModInstaller.installed(files),"disabled persistence");
  for(TextureMods.Pack p:TextureMods.list(files))TextureMods.change(files,p.id,2);check(TextureMods.list(files).isEmpty()&&!ModInstaller.installed(files),"remove all");
  UpdateInstaller.remove(temp.toFile());System.out.println("PASS multi-pack migration, coexistence, hash priority, toggle/remove restoration, restart persistence, rejected ZIP preservation, commit rollback, staging failure and empty overlay");
 }
}
