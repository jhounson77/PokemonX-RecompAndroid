package org.pokemonx.recomp;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;

/** Installs legacy Azahar/Citra PNG texture packs; executable patches are unsupported. */
final class ModInstaller {
 static File directory(File files){return new File(files,"azahar/load/textures/0004000000055D00");}
 static boolean installed(File files){return new File(directory(files),"px-pack.txt").isFile();}
 static String safe(String name)throws IOException{String s=name.replace('\\','/');if(s.startsWith("/")||s.contains(":"))throw new IOException(I18n.t("Caminho inválido no ZIP."));for(String p:s.split("/"))if(p.equals("..")||p.equals("."))throw new IOException(I18n.t("Caminho inválido no ZIP."));return s;}
 static void copy(InputStream in,OutputStream out,long max)throws IOException{byte[]b=new byte[65536];long total=0;int n;while((n=in.read(b))!=-1){total+=n;if(total>max)throw new IOException(I18n.t("Arquivo excede o limite permitido."));out.write(b,0,n);}}
 static int install(File zip,File files,String label)throws Exception{
  File target=directory(files),parent=target.getParentFile();if(!parent.isDirectory()&&!parent.mkdirs())throw new IOException(I18n.t("Falha ao criar pasta de mods."));
  File stage=new File(parent,".stage-"+UUID.randomUUID()),backup=new File(parent,".previous-"+UUID.randomUUID());stage.mkdir();int count=0,entries=0;long total=0;Set<String>names=new HashSet<>();boolean committed=false;
  try(ZipInputStream in=new ZipInputStream(new FileInputStream(zip))){ZipEntry e;while((e=in.getNextEntry())!=null){if(++entries>10000)throw new IOException(I18n.t("Muitos arquivos no pacote."));String path=safe(e.getName());if(e.isDirectory())continue;String name=path.substring(path.lastIndexOf('/')+1);
   if(name.toLowerCase(Locale.ROOT).endsWith(".txt")){copy(in,new OutputStream(){public void write(int b){} public void write(byte[]b,int off,int len){}},1048576);continue;}
   if(!name.matches("tex1_[0-9]+x[0-9]+_[0-9a-fA-F]{16}_[0-9]+(?:_mip[0-9]+)?\\.png"))throw new IOException(I18n.t("Formato não suportado. Use um pack PNG de texturas Citra/Azahar; patches de código não são aceitos."));
   if(!names.add(name.toLowerCase(Locale.ROOT)))throw new IOException(I18n.t("Texturas duplicadas no ZIP."));File out=new File(stage,name);try(FileOutputStream o=new FileOutputStream(out)){copy(in,o,32L*1024*1024);o.getFD().sync();}total+=out.length();if(total>512L*1024*1024)throw new IOException(I18n.t("Pack descompactado muito grande."));
   try(DataInputStream h=new DataInputStream(new FileInputStream(out))){if(h.readLong()!=0x89504e470d0a1a0aL||h.readInt()!=13||h.readInt()!=0x49484452)throw new IOException(I18n.t("PNG inválido."));int w=h.readInt(),v=h.readInt();if(w<1||v<1||w>8192||v>8192||(long)w*v>32L*1024*1024)throw new IOException(I18n.t("Dimensões de textura inválidas."));}count++;
  }if(count==0)throw new IOException(I18n.t("Nenhuma textura compatível encontrada."));
   Files.write(new File(stage,"px-pack.txt").toPath(),(label.replace('\n',' ') +"\n"+count).getBytes("UTF-8"));
   if(target.exists()&&!target.renameTo(backup))throw new IOException(I18n.t("Não foi possível preservar o pack anterior."));
   if(!stage.renameTo(target)){if(backup.exists())backup.renameTo(target);throw new IOException(I18n.t("Falha ao ativar o pack; anterior preservado."));}committed=true;
  }finally{if(stage.exists())UpdateInstaller.remove(stage);if(committed&&backup.exists())try{UpdateInstaller.remove(backup);}catch(IOException ignored){}}
  return count;
 }
}
