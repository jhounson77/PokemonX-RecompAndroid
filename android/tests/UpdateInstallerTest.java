package org.pokemonx.recomp;
import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;
public class UpdateInstallerTest{
 static void be(byte[]a,int p,long v,int n){for(int i=n-1;i>=0;i--){a[p+i]=(byte)v;v>>>=8;}}
 static void le(byte[]a,int p,long v,int n){for(int i=0;i<n;i++){a[p+i]=(byte)v;v>>>=8;}}
 static byte[] cia(long title,int version)throws Exception{byte[]b=new byte[2944+512];le(b,0,32,4);le(b,16,0xb34,4);int t=64,body=t+0x140,record=body+0x9c4;be(b,t,0x10004,4);be(b,body+0x4c,title,8);be(b,body+0x9c,version,2);be(b,body+0x9e,1,2);be(b,record+8,512,8);b[2944+256]='N';b[2944+257]='C';b[2944+258]='C';b[2944+259]='H';le(b,2944+0x118,title,8);le(b,2944+0x104,1,4);byte[]h=MessageDigest.getInstance("SHA-256").digest(Arrays.copyOfRange(b,2944,b.length));System.arraycopy(h,0,b,record+16,32);return b;}
 static void reject(File f,byte[]b)throws Exception{Files.write(f.toPath(),b);try{UpdateInstaller.inspect(f);throw new AssertionError("invalid CIA accepted");}catch(IOException expected){}}
 public static void main(String[]args)throws Exception{Path dir=Files.createTempDirectory("px-update-test");File f=dir.resolve("synthetic.cia").toFile();byte[]good=cia(0x0004000E00055D00L,5232);Files.write(f.toPath(),good);UpdateInstaller.Metadata m=UpdateInstaller.inspect(f);if(m.data!=2944||m.size!=512||m.id!=0)throw new AssertionError();reject(f,cia(0x0004000E00055E00L,5232));reject(f,cia(0x0004000E00055D00L,0));reject(f,Arrays.copyOf(good,good.length-1));byte[]bad=good.clone();bad[bad.length-1]^=1;reject(f,bad);
 File root=dir.resolve("installed").toFile(),content=new File(root,"content");content.mkdirs();Path sentinel=new File(content,"existing-update.txt").toPath();Files.write(sentinel,new byte[]{7,8,9});Files.write(f.toPath(),good);try{UpdateInstaller.installFile(f,root,msg->{});throw new AssertionError("invalid NCCH activated");}catch(IOException expected){}if(!Arrays.equals(Files.readAllBytes(sentinel),new byte[]{7,8,9})||root.list().length!=1)throw new AssertionError("old installation changed");
 File toggle=dir.resolve("toggle").toFile(),active=new File(toggle,"content"),disabled=new File(toggle,"content.disabled");active.mkdirs();Files.write(new File(active,"px-update15.txt").toPath(),new byte[]{1});Files.write(new File(active,"sentinel").toPath(),new byte[]{2});
 UpdateInstaller.setEnabled(toggle,false);UpdateInstaller.setEnabled(toggle,false);if(active.exists()||!new File(disabled,"sentinel").isFile())throw new AssertionError("disable failed");
 UpdateInstaller.setEnabled(toggle,true);UpdateInstaller.setEnabled(toggle,true);if(disabled.exists()||!new File(active,"sentinel").isFile())throw new AssertionError("enable failed");
 disabled.mkdirs();Files.write(new File(disabled,"unknown").toPath(),new byte[]{3});try{UpdateInstaller.setEnabled(toggle,false);throw new AssertionError("unknown destination overwritten");}catch(IOException expected){}if(!new File(active,"sentinel").isFile()||!new File(disabled,"unknown").isFile())throw new AssertionError("toggle modified unknown data");
 System.out.println("PASS synthetic CIA title/version/hash/truncation rejection and failed preparation preserves previous installation; reversible/idempotent toggle preserves unknown destinations");UpdateInstaller.remove(dir.toFile());}
}
