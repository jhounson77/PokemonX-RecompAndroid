package org.pokemonx.recomp;
import android.content.Context;
import android.net.Uri;
import java.io.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;

/** Installs the supported update transactionally, without touching base data/saves. */
final class UpdateInstaller {
 static final long TITLE=0x0004000E00055D00L;
 static final String CODE_HASH="7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74";
 static final String TITLE_PATH="azahar/sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/0004000e/00055d00";
 private static native String unpackRar(String source,String destination);
 static File root(Context c){return new File(c.getFilesDir(),TITLE_PATH);}
 static boolean installed(Context c){return enabled(c)||new File(root(c),"content.disabled/px-update15.txt").isFile();}
 static boolean enabled(Context c){return new File(root(c),"content/px-update15.txt").isFile();}
 static void setEnabled(File root,boolean enable)throws Exception{
  File from=new File(root,enable?"content.disabled":"content"),to=new File(root,enable?"content":"content.disabled"),backup=new File(root,"previous-"+UUID.randomUUID());
  if(!new File(from,"px-update15.txt").isFile()){if(new File(to,"px-update15.txt").isFile())return;throw new IOException(I18n.t("Atualização instalada não encontrada."));}
  boolean moved=false;if(to.exists()){if(!new File(to,"px-update15.txt").isFile())throw new IOException(I18n.t("A pasta de destino contém dados não reconhecidos; nada foi alterado."));if(!to.renameTo(backup))throw new IOException(I18n.t("Não foi possível preservar a instalação anterior."));moved=true;}
  if(!from.renameTo(to)){if(moved)backup.renameTo(to);throw new IOException(I18n.t("Não foi possível alterar a versão ativa."));}
  if(backup.exists())try{remove(backup);}catch(IOException ignored){}
 }
 static long be(byte[]b,int o,int n)throws IOException{if(o<0||n<0||o>b.length-n)throw new IOException(I18n.t("Metadados truncados."));long v=0;for(int i=0;i<n;i++)v=(v<<8)|(b[o+i]&255L);return v;}
 static void put(byte[]b,int o,long v,int n){for(int i=n-1;i>=0;i--){b[o+i]=(byte)v;v>>>=8;}}
 static byte[] digest(File f,long at,long size)throws Exception{MessageDigest d=MessageDigest.getInstance("SHA-256");try(RandomAccessFile in=new RandomAccessFile(f,"r")){if(at<0||size<0||at>in.length()-size)throw new IOException(I18n.t("Conteúdo truncado."));in.seek(at);byte[]buf=new byte[1024*1024];while(size>0){int n=in.read(buf,0,(int)Math.min(buf.length,size));if(n<0)throw new EOFException();d.update(buf,0,n);size-=n;}}return d.digest();}
 static String hex(byte[]b){StringBuilder s=new StringBuilder();for(byte v:b)s.append(String.format(Locale.ROOT,"%02x",v&255));return s.toString();}
 static final class Metadata{byte[]tmd;long data,size;int id,record,body;}
 static Metadata inspect(File file)throws Exception{
  try(RandomAccessFile f=new RandomAccessFile(file,"r")){
   byte[]h=GameImporter.read(f,0,32);long hs=GameImporter.le(h,0,4),cs=GameImporter.le(h,8,4),ts=GameImporter.le(h,12,4),ms=GameImporter.le(h,16,4);
   if(hs<32||hs>1048576||cs>1048576||ts>1048576||ms<0xb34||ms>1048576)throw new IOException(I18n.t("Cabeçalho CIA de atualização inválido."));
   long tmdAt=GameImporter.align(GameImporter.align(GameImporter.align(hs)+cs)+ts),dataAt=GameImporter.align(tmdAt+ms);
   Metadata m=new Metadata();m.tmd=GameImporter.read(f,tmdAt,(int)ms);m.body=be(m.tmd,0,4)==0x10004?0x140:-1;
   if(m.body<0||be(m.tmd,m.body+0x4c,8)!=TITLE)throw new IOException(I18n.t("O arquivo não é uma atualização do Pokémon X."));
   if(be(m.tmd,m.body+0x9c,2)!=5232||be(m.tmd,m.body+0x9e,2)!=1)throw new IOException(I18n.t("Somente o update 1.5 compatível é aceito."));
   m.record=m.body+0x9c4;m.id=(int)be(m.tmd,m.record,4);m.size=be(m.tmd,m.record+8,8);m.data=dataAt;
   if(be(m.tmd,m.record+4,2)!=0||m.size<512||m.size>64L*1024*1024||dataAt>f.length()-m.size)throw new IOException(I18n.t("Conteúdo do update inválido."));
   if(!MessageDigest.isEqual(digest(file,dataAt,m.size),Arrays.copyOfRange(m.tmd,m.record+16,m.record+48)))throw new IOException(I18n.t("O update está corrompido: hash do conteúdo não confere."));
   byte[]ncch=GameImporter.read(f,dataAt+256,4);if(!Arrays.equals(ncch,new byte[]{'N','C','C','H'}))throw new IOException(I18n.t("Criptografia externa do CIA não suportada."));
   return m;
  }
 }
 static String executableHash(File app)throws Exception{
  try(RandomAccessFile f=new RandomAccessFile(app,"r")){
   byte[]h=GameImporter.read(f,0,512);long ex=GameImporter.le(h,0x1a0,4)*512;byte[]entries=GameImporter.read(f,ex,160);int found=-1;
   for(int i=0;i<10;i++)if(new String(entries,i*16,5,"US-ASCII").equals(".code")){found=i*16;break;}
   if(found<0)throw new IOException(I18n.t("Executável do update ausente."));
   long off=GameImporter.le(entries,found+8,4),size=GameImporter.le(entries,found+12,4);if(size<8||size>16*1024*1024)throw new IOException(I18n.t("Tamanho do executável inválido."));
   byte[]src=GameImporter.read(f,ex+512+off,(int)size);byte flags=GameImporter.read(f,512+13,1)[0];byte[]code=src;
   if((flags&1)!=0){
    long extra=GameImporter.le(src,src.length-4,4),packed=GameImporter.le(src,src.length-8,4);if(extra>16*1024*1024||src.length+extra>16*1024*1024)throw new IOException(I18n.t("Executável comprimido inválido."));
    int top=(int)(packed>>>24),bottom=(int)(packed&0xffffff);if(top>src.length||bottom>src.length||top<8)throw new IOException(I18n.t("Rodapé de compressão inválido."));
    code=Arrays.copyOf(src,src.length+(int)extra);int index=src.length-top,stop=src.length-bottom,out=code.length;
    while(index>stop){int control=src[--index]&255;for(int i=0;i<8&&index>stop&&out>0;i++,control=(control<<1)&255){
     if((control&128)!=0){if(index<2)throw new IOException(I18n.t("Compressão truncada."));index-=2;int s=(src[index]&255)|((src[index+1]&255)<<8),count=(s>>>12)+3,d=(s&4095)+2;if(out<count||out+d>=code.length)throw new IOException(I18n.t("Referência de compressão inválida."));for(int j=0;j<count;j++){code[out-1]=code[out+d];out--;}}
     else{if(index<=0)throw new IOException(I18n.t("Compressão truncada."));code[--out]=src[--index];}
    }}
   }
   return hex(MessageDigest.getInstance("SHA-256").digest(code));
  }
 }
 static void remove(File f)throws IOException{if(!f.exists())return;if(f.isDirectory()){File[]files=f.listFiles();if(files==null)throw new IOException(I18n.t("Não foi possível listar os arquivos temporários."));for(File q:files)remove(q);}if(!f.delete())throw new IOException(I18n.t("Não foi possível remover um arquivo temporário."));}
 static void installFile(File cia,File titleRoot,GameImporter.Progress progress)throws Exception{
  Metadata m=inspect(cia);if(!titleRoot.isDirectory()&&!titleRoot.mkdirs())throw new IOException(I18n.t("Não foi possível criar a pasta da atualização."));
  File stage=new File(titleRoot,"stage-"+UUID.randomUUID()),content=new File(titleRoot,"content"),backup=new File(titleRoot,"previous-"+UUID.randomUUID());if(!stage.mkdir())throw new IOException(I18n.t("Falha ao preparar a instalação."));boolean oldMoved=false,committed=false;
  try{
   File app=new File(stage,String.format(Locale.ROOT,"%08x.app",m.id));GameImporter.prepareFile(cia,app,progress,TITLE);
   if(!CODE_HASH.equals(executableHash(app)))throw new IOException(I18n.t("O executável não corresponde à recompilação 1.5 incluída neste APK."));
   // The local decrypted .app is represented consistently in HLE metadata.
   put(m.tmd,m.record+6,be(m.tmd,m.record+6,2)&~1L,2);put(m.tmd,m.record+8,app.length(),8);System.arraycopy(digest(app,0,app.length()),0,m.tmd,m.record+16,32);
   for(int i=0;i<64;i++){int p=m.body+0xc4+i*36,count=(int)be(m.tmd,p+2,2),index=(int)be(m.tmd,p,2);if(count>0){if(index!=0||count!=1)throw new IOException(I18n.t("Tabela de conteúdo não suportada."));MessageDigest d=MessageDigest.getInstance("SHA-256");d.update(m.tmd,m.record,48);System.arraycopy(d.digest(),0,m.tmd,p+4,32);}}
   MessageDigest d=MessageDigest.getInstance("SHA-256");d.update(m.tmd,m.body+0xc4,0x900);System.arraycopy(d.digest(),0,m.tmd,m.body+0xa4,32);
   try(FileOutputStream o=new FileOutputStream(new File(stage,"00000000.tmd"))){o.write(m.tmd);o.getFD().sync();}
   try(FileOutputStream o=new FileOutputStream(new File(stage,"px-update15.txt"))){o.write((CODE_HASH+"\n").getBytes("UTF-8"));o.getFD().sync();}
   if(content.exists()){if(!content.renameTo(backup))throw new IOException(I18n.t("Não foi possível preservar o update anterior."));oldMoved=true;}
   if(!stage.renameTo(content)){if(oldMoved&&!backup.renameTo(content))throw new IOException(I18n.t("Falha na instalação e restauração; os arquivos anteriores foram preservados na pasta de recuperação."));throw new IOException(I18n.t("Falha ao ativar o update; instalação anterior restaurada."));}
   committed=true;
  }finally{if(stage.exists())remove(stage);if(committed&&backup.exists())remove(backup);}
 }
 static void install(Context c,Uri uri,GameImporter.Progress p)throws Exception{
  if(!new File(c.getApplicationInfo().nativeLibraryDir,"libpxmain15.so").isFile()||!new File(c.getApplicationInfo().nativeLibraryDir,"libpxmodules15.so").isFile())throw new IOException(I18n.t("Este APK não contém a recompilação do update."));
  File input=File.createTempFile("update-input-",".part",c.getCacheDir()),cia=File.createTempFile("update-cia-",".part",c.getCacheDir());
  try{
   try(InputStream in=c.getContentResolver().openInputStream(uri);FileOutputStream out=new FileOutputStream(input)){if(in==null)throw new IOException(I18n.t("Arquivo indisponível."));byte[]buf=new byte[1024*1024];long total=0;int n;while((n=in.read(buf))!=-1){total+=n;if(total>96L*1024*1024)throw new IOException(I18n.t("O arquivo excede o tamanho permitido para este update."));out.write(buf,0,n);}out.getFD().sync();}
   byte[]head;try(RandomAccessFile f=new RandomAccessFile(input,"r")){head=GameImporter.read(f,0,7);}
   File source=input;
   if(head[0]=='R'&&head[1]=='a'&&head[2]=='r'&&head[3]=='!'){p.show(I18n.t("Extraindo o CIA da atualização…"));String error=unpackRar(input.getAbsolutePath(),cia.getAbsolutePath());if(error!=null)throw new IOException(error);source=cia;}
   p.show(I18n.t("Validando Pokémon X 1.5…"));installFile(source,root(c),p);
  }finally{input.delete();cia.delete();}
 }
}
