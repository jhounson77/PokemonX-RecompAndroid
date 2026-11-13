package org.pokemonx.recomp;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;

/** Logical save files only: excludes game data, keys, mods, logs and save states. */
final class SaveManager {
 static File data(File files){return new File(files,"azahar/sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/00040000/00055d00/data");}
 static File latestBackup(File files){File[]list=new File(files,"save-backups").listFiles((d,n)->n.endsWith(".zip"));if(list==null||list.length==0)return null;Arrays.sort(list,Comparator.comparing(File::getName));return list[list.length-1];}
 static void exportSave(File files,OutputStream out)throws Exception{
  File data=data(files),save=new File(data,"00000001");
  try(ZipOutputStream zip=new ZipOutputStream(out)){if(!new File(save,"main").isFile())throw new IOException(I18n.t("Nenhum save encontrado. Salve primeiro pelo menu do jogo."));zip.putNextEntry(new ZipEntry("pokemonx-save.properties"));zip.write("title=0004000000055D00\nformat=1\n".getBytes("UTF-8"));zip.closeEntry();add(zip,save,"save/");File meta=new File(data,"00000001.metadata");if(meta.isFile()){zip.putNextEntry(new ZipEntry("00000001.metadata"));Files.copy(meta.toPath(),zip);zip.closeEntry();}}
 }
 static void add(ZipOutputStream zip,File file,String name)throws Exception{if(Files.isSymbolicLink(file.toPath()))throw new IOException(I18n.t("Link inválido no save."));if(file.isDirectory()){File[]list=file.listFiles();if(list==null)throw new IOException(I18n.t("Não foi possível ler o save."));for(File f:list)add(zip,f,name+f.getName()+(f.isDirectory()?"/":""));}else{zip.putNextEntry(new ZipEntry(name));Files.copy(file.toPath(),zip);zip.closeEntry();}}
 static void importSave(File input,File files)throws Exception{
  File target=data(files),parent=target.getParentFile();if(!parent.isDirectory()&&!parent.mkdirs())throw new IOException(I18n.t("Falha ao preparar save."));File stage=new File(parent,"save-stage-"+UUID.randomUUID()),previous=new File(parent,"save-previous-"+UUID.randomUUID());stage.mkdir();File save=new File(stage,"00000001");save.mkdir();boolean committed=false;File suppliedMeta=new File(stage,"00000001.metadata");
  try{
   boolean zipped;try(FileInputStream in=new FileInputStream(input)){zipped=in.read()==80&&in.read()==75;}
   if(zipped){boolean main=false;int count=0;long total=0;try(ZipInputStream zip=new ZipInputStream(new FileInputStream(input))){ZipEntry e;while((e=zip.getNextEntry())!=null){if(++count>256)throw new IOException(I18n.t("ZIP de save muito grande."));String name=ModInstaller.safe(e.getName());if(e.isDirectory())continue;String base=name.substring(name.lastIndexOf('/')+1);
    File out;if(base.equals("main")){if(main)throw new IOException(I18n.t("O ZIP contém mais de um save main."));main=true;out=new File(save,"main");}else if(name.equals("00000001.metadata")){if(suppliedMeta.exists())throw new IOException(I18n.t("Metadados duplicados."));out=suppliedMeta;}else if(name.equals("pokemonx-save.properties")){ByteArrayOutputStream b=new ByteArrayOutputStream();ModInstaller.copy(zip,b,1024);Properties props=new Properties();props.load(new ByteArrayInputStream(b.toByteArray()));if(!"0004000000055D00".equals(props.getProperty("title")))throw new IOException(I18n.t("Este backup é de outro jogo."));continue;}else if(name.startsWith("save/")&&name.length()>5){out=new File(save,name.substring(5));if(!out.getParentFile().isDirectory()&&!out.getParentFile().mkdirs())throw new IOException(I18n.t("Pasta de save inválida."));if(out.exists())throw new IOException(I18n.t("Arquivo duplicado no save."));}else throw new IOException(I18n.t("ZIP não reconhecido. Selecione o backup do app ou um arquivo main."));
    try(FileOutputStream o=new FileOutputStream(out)){ModInstaller.copy(zip,o,16L*1024*1024);o.getFD().sync();}total+=out.length();if(total>64L*1024*1024)throw new IOException(I18n.t("Backup descompactado muito grande."));
   }}if(!main)throw new IOException(I18n.t("Arquivo main não encontrado no ZIP."));
   }else{if(input.length()<1||input.length()>16L*1024*1024)throw new IOException(I18n.t("Save main inválido."));Files.copy(input.toPath(),new File(save,"main").toPath());}
   File main=new File(save,"main");if(main.length()<1)throw new IOException(I18n.t("Save vazio."));
   if(!suppliedMeta.exists()){File oldMeta=new File(target,"00000001.metadata");if(!oldMeta.isFile())throw new IOException(I18n.t("Inicie o jogo uma vez antes de importar um main, ou importe um backup completo deste app."));Files.copy(oldMeta.toPath(),suppliedMeta.toPath());}if(suppliedMeta.length()!=16)throw new IOException(I18n.t("Metadados do save inválidos."));
   if(new File(target,"00000001/main").isFile()){File backups=new File(files,"save-backups");backups.mkdirs();File backup=new File(backups,"PokemonX-save-"+System.currentTimeMillis()+"-"+UUID.randomUUID()+".zip");try{exportSave(files,new FileOutputStream(backup));}catch(Exception e){backup.delete();throw e;}}
   if(target.exists()&&!target.renameTo(previous))throw new IOException(I18n.t("Não foi possível preservar o save anterior."));if(!stage.renameTo(target)){if(previous.exists())previous.renameTo(target);throw new IOException(I18n.t("Falha ao ativar save; anterior preservado."));}committed=true;
  }finally{if(stage.exists())UpdateInstaller.remove(stage);if(committed&&previous.exists())try{UpdateInstaller.remove(previous);}catch(IOException ignored){}}
 }
}
