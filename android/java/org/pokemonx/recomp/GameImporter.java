package org.pokemonx.recomp;
import android.content.*;
import android.net.Uri;
import java.io.*;
import java.math.BigInteger;
import java.nio.*;
import java.security.*;
import javax.crypto.*;
import javax.crypto.spec.*;
import java.util.*;
final class GameImporter {
 interface Progress {void show(String message);}
 static long le(byte[]b,int o,int n){long v=0;for(int i=0;i<n;i++)v|=(long)(b[o+i]&255)<<(8*i);return v;}
 static long align(long v){return (v+63)&~63L;}
 static byte[] read(RandomAccessFile f,long at,int size)throws IOException{byte[] b=new byte[size];f.seek(at);f.readFully(b);return b;}
 static byte[] hex(String s){byte[]b=new byte[s.length()/2];for(int i=0;i<b.length;i++)b[i]=(byte)Integer.parseInt(s.substring(i*2,i*2+2),16);return b;}
 static final BigInteger MASK=BigInteger.ONE.shiftLeft(128).subtract(BigInteger.ONE);
 static BigInteger rol(BigInteger v,int n){return v.shiftLeft(n).or(v.shiftRight(128-n)).and(MASK);}
 static byte[] fixed(BigInteger v){byte[]a=v.toByteArray(),r=new byte[16];int n=Math.min(16,a.length);System.arraycopy(a,a.length-n,r,16-n,n);return r;}
 static byte[] key(byte[]h){BigInteger x=new BigInteger(CryptoConfig.KEY_X,16),y=new BigInteger(1,Arrays.copyOf(h,16)),c=new BigInteger(CryptoConfig.CONSTANT,16);return fixed(rol(rol(x,2).xor(y).add(c).and(MASK),87));}
 static Cipher cipher(byte[]key,byte[]h,int section)throws GeneralSecurityException{
  byte[]iv=new byte[16];for(int i=0;i<8;i++)iv[i]=h[0x10f-i];iv[8]=(byte)section;
  Cipher c=Cipher.getInstance("AES/CTR/NoPadding");c.init(Cipher.DECRYPT_MODE,new SecretKeySpec(key,"AES"),new IvParameterSpec(iv));return c;
 }
 static boolean hashMatches(byte[]data,int size,byte[]h,int off)throws Exception{
  MessageDigest d=MessageDigest.getInstance("SHA-256");d.update(data,0,size);return MessageDigest.isEqual(d.digest(),Arrays.copyOfRange(h,off,off+32));
 }
 static void importGame(Context ctx,Uri uri,Progress p)throws Exception{
  File temporary=new File(ctx.getFilesDir(),"import.part"),result=new File(ctx.getFilesDir(),"game.cxi"),output=new File(ctx.getFilesDir(),"game.part");
  try{
   p.show(I18n.t("Copiando seu arquivo…"));
   try(InputStream in=ctx.getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(temporary)){
    if(in==null)throw new IOException(I18n.t("Não foi possível abrir o arquivo."));byte[]buffer=new byte[2*1024*1024];long done=0;int n;
    while((n=in.read(buffer))!=-1){out.write(buffer,0,n);done+=n;if(done%(32*1024*1024)<buffer.length)p.show(I18n.t("Copiando: ")+done/(1024*1024)+" MB");}
   }
   prepareFile(temporary,result,p);
  }finally{temporary.delete();output.delete();}
 }
 static void prepareFile(File temporary,File result,Progress p)throws Exception{prepareFile(temporary,result,p,0x0004000000055D00L);}
 static void prepareFile(File temporary,File result,Progress p,long expectedTitle)throws Exception{
  File output=new File(result.getParentFile(),"game.part");
   try(RandomAccessFile in=new RandomAccessFile(temporary,"r");RandomAccessFile out=new RandomAccessFile(output,"rw")){
    byte[]outer=read(in,0,512);long base;
    if(new String(outer,256,4,"US-ASCII").equals("NCCH"))base=0;
    else if(new String(outer,256,4,"US-ASCII").equals("NCSD"))base=le(outer,0x120,4)*512;
    else {long hs=le(outer,0,4);if(hs<0x20||hs>0x100000)throw new IOException(I18n.t("Cabeçalho CIA inválido."));base=align(align(align(align(hs)+le(outer,8,4))+le(outer,12,4))+le(outer,16,4));}
    byte[]h=read(in,base,512);
    if(!new String(h,256,4,"US-ASCII").equals("NCCH"))throw new IOException(I18n.t("Conteúdo NCCH não encontrado. CIA com criptografia externa não é aceito nesta versão."));
    if(le(h,0x118,8)!=expectedTitle)throw new IOException(I18n.t("Este arquivo não é Pokémon X."));
    long length=le(h,0x104,4)*512;if(length<512||base>in.length()-length)throw new IOException(I18n.t("Arquivo truncado."));
    boolean encrypted=(h[0x18f]&4)==0;
    if(encrypted&&((h[0x18b]&255)!=0||(h[0x18f]&0x21)!=0))throw new IOException(I18n.t("Método de criptografia diferente do arquivo testado."));
    byte[]k=key(h);byte[]ex=read(in,base+512,2048);
    if(!hashMatches(ex,(int)le(h,0x180,4),h,0x160)){
     if(!encrypted)throw new IOException(I18n.t("Hash do cabeçalho inválido."));ex=cipher(k,h,1).doFinal(ex);
     if(!hashMatches(ex,(int)le(h,0x180,4),h,0x160))throw new IOException(I18n.t("A chave não corresponde ao arquivo."));
    } else encrypted=false;
    out.setLength(length);in.seek(base);out.seek(0);byte[]buf=new byte[4*1024*1024];long left=length;
    p.show(I18n.t("Preparando o conteúdo…"));while(left>0){int n=in.read(buf,0,(int)Math.min(buf.length,left));if(n<0)throw new EOFException();out.write(buf,0,n);left-=n;}
    if(encrypted){
     convert(in,out,base,512,2048,cipher(k,h,1),p,I18n.t("Cabeçalho"));
     convert(in,out,base,le(h,0x1a0,4)*512,le(h,0x1a4,4)*512,cipher(k,h,2),p,I18n.t("Executável"));
     convert(in,out,base,le(h,0x1b0,4)*512,le(h,0x1b4,4)*512,cipher(k,h,3),p,"Recursos");
    }
    verify(out,le(h,0x1a0,4)*512,le(h,0x1a8,4)*512,h,0x1c0);
    verify(out,le(h,0x1b0,4)*512,le(h,0x1b8,4)*512,h,0x1e0);
    h[0x18f]|=4;h[0x18b]=0;out.seek(0);out.write(h);out.getFD().sync();
   }
   if(!output.renameTo(result))throw new IOException(I18n.t("Não foi possível concluir a importação."));
 }
 static void convert(RandomAccessFile in,RandomAccessFile out,long base,long off,long size,Cipher c,Progress p,String label)throws Exception{
  in.seek(base+off);out.seek(off);byte[]buf=new byte[4*1024*1024];long done=0;
  while(done<size){int n=in.read(buf,0,(int)Math.min(buf.length,size-done));if(n<0)throw new EOFException();out.write(c.update(buf,0,n));done+=n;if(done%(32*1024*1024)<buf.length)p.show(label+": "+(done*100/size)+"%");}out.write(c.doFinal());
 }
 static void verify(RandomAccessFile f,long off,long size,byte[]h,int ho)throws Exception{
  if(size>16*1024*1024||size<=0)throw new IOException(I18n.t("Região de hash inválida."));
  if(!hashMatches(read(f,off,(int)size),(int)size,h,ho))throw new IOException(I18n.t("Falha de integridade na extração."));
 }
}
