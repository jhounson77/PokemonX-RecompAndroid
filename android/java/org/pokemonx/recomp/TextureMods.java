package org.pokemonx.recomp;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Immutable source packs and an atomic index; the texture folder is a derived overlay. */
final class TextureMods {
 static final class Pack {
  final String id,name; final int count; final boolean enabled;
  Pack(String id,String name,int count,boolean enabled){this.id=id;this.name=name;this.count=count;this.enabled=enabled;}
  Pack enabled(boolean on){return new Pack(id,name,count,on);}
 }
 private static final Set<String> ready=new HashSet<>();
 private static File root(File files){return new File(files,"texture-mods");}
 private static File source(File files,Pack p){return new File(root(files),p.id);}
 private static void mkdir(File f)throws IOException{if(!f.isDirectory()&&!f.mkdirs())throw new IOException(I18n.t("Falha ao criar pasta de mods."));}
 private static void link(File from,File to)throws IOException{
  try{Files.createLink(to.toPath(),from.toPath());}catch(IOException|UnsupportedOperationException error){Files.copy(from.toPath(),to.toPath());}
 }
 private static List<Pack> read(File files)throws IOException{
  List<Pack> result=new ArrayList<>();File index=new File(root(files),"index.properties");if(!index.exists())return result;
  Properties p=new Properties();try(InputStream in=new FileInputStream(index)){p.load(in);}
  for(String id:p.getProperty("order","").split(",")){if(id.isEmpty())continue;if(!id.matches("[0-9a-f-]{36}"))throw new IOException("Invalid mod index");
   try{result.add(new Pack(id,p.getProperty(id+".name","Mod"),Integer.parseInt(p.getProperty(id+".count")),Boolean.parseBoolean(p.getProperty(id+".enabled"))));}catch(NumberFormatException error){throw new IOException("Invalid mod index",error);}
  }return result;
 }
 private static void write(File files,List<Pack> packs)throws IOException{
  Properties p=new Properties();StringJoiner ids=new StringJoiner(",");for(Pack a:packs){ids.add(a.id);p.setProperty(a.id+".name",a.name);p.setProperty(a.id+".count",Integer.toString(a.count));p.setProperty(a.id+".enabled",Boolean.toString(a.enabled));}p.setProperty("order",ids.toString());
  File stage=new File(root(files),"index.tmp");try(FileOutputStream out=new FileOutputStream(stage)){p.store(out,"Texture mods: low to high priority");out.getFD().sync();}
  Files.move(stage.toPath(),new File(root(files),"index.properties").toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
 }
 private static void clean(File f){try{if(f.exists())UpdateInstaller.remove(f);}catch(IOException ignored){}}
 private static void overlay(File files,List<Pack> packs,boolean save)throws IOException{
  File target=ModInstaller.directory(files),parent=target.getParentFile();mkdir(parent);
  File stage=new File(parent,".mods-stage-"+UUID.randomUUID()),backup=new File(parent,".mods-old-"+UUID.randomUUID());mkdir(stage);boolean moved=false,activated=false,done=false;
  try{
   Map<String,File> winners=new TreeMap<>();Map<String,String> owners=new HashMap<>();
   for(Pack p:packs)if(p.enabled){File[] textures=source(files,p).listFiles((d,n)->n.endsWith(".png"));if(textures==null)throw new IOException("Mod source unavailable: "+p.name);Arrays.sort(textures,Comparator.comparing(File::getName));
    // The native texture loader keys materials by hash, not by the full filename.
    for(File f:textures){String name=f.getName().toLowerCase(Locale.ROOT),hash=name.split("_")[2];String old=owners.put(hash,p.id);if(old!=null&&!old.equals(p.id))winners.entrySet().removeIf(e->e.getKey().split("_")[2].equals(hash));winners.put(name,f);}
   }
   for(Map.Entry<String,File> e:winners.entrySet())link(e.getValue(),new File(stage,e.getKey()));
   if(!winners.isEmpty())Files.write(new File(stage,"px-pack.txt").toPath(),("Combined texture mods\n"+winners.size()).getBytes("UTF-8"));
   if(target.exists()){if(!target.renameTo(backup))throw new IOException(I18n.t("Não foi possível preservar o pack anterior."));moved=true;}
   if(!stage.renameTo(target))throw new IOException(I18n.t("Falha ao ativar o pack; anterior preservado."));
   activated=true;if(save)write(files,packs);done=true;
  }finally{
   if(!done&&moved){clean(target);if(!backup.renameTo(target))throw new IOException("Cannot restore previous mod overlay");}
   else if(!done&&activated)clean(target);
   clean(stage);if(done)clean(backup);
  }
 }
 private static void initialize(File files)throws IOException{
  String key=files.getCanonicalPath();if(ready.contains(key))return;mkdir(root(files));File index=new File(root(files),"index.properties");
  if(!index.exists()){
   List<Pack> packs=new ArrayList<>();File legacy=ModInstaller.directory(files);
   if(ModInstaller.installed(files)){
    List<String> label=Files.readAllLines(new File(legacy,"px-pack.txt").toPath());String id=UUID.randomUUID().toString();File dest=new File(root(files),id);mkdir(dest);
    try{File[] textures=legacy.listFiles((d,n)->n.endsWith(".png"));if(textures==null)throw new IOException("Legacy mod unavailable");for(File f:textures)link(f,new File(dest,f.getName()));packs.add(new Pack(id,label.isEmpty()?"Mod":label.get(0),textures.length,true));write(files,packs);}catch(IOException e){clean(dest);throw e;}
   }else write(files,packs);
  }
  // Rebuild from the authoritative index after process death between overlay and index commits.
  List<Pack> packs=read(files);overlay(files,packs,false);
  Set<String> retained=new HashSet<>();for(Pack p:packs)retained.add(p.id);File[] leftovers=root(files).listFiles();if(leftovers!=null)for(File f:leftovers)if(f.isDirectory()&&(f.getName().startsWith(".import-")||f.getName().matches("[0-9a-f-]{36}")&&!retained.contains(f.getName())))clean(f);
  File[] overlays=ModInstaller.directory(files).getParentFile().listFiles();if(overlays!=null)for(File f:overlays)if(f.getName().startsWith(".mods-old-")||f.getName().startsWith(".mods-stage-"))clean(f);
  ready.add(key);
 }
 static synchronized List<Pack> list(File files)throws IOException{initialize(files);return read(files);}
 static synchronized int add(File zip,File files,String label)throws Exception{
  initialize(files);List<Pack> packs=read(files);File staging=new File(root(files),".import-"+UUID.randomUUID());mkdir(staging);File dest=null;boolean committed=false;
  try{int count=ModInstaller.extract(zip,staging,label);String id=UUID.randomUUID().toString();dest=new File(root(files),id);if(!ModInstaller.directory(staging).renameTo(dest))throw new IOException("Cannot retain mod source");
   packs.add(new Pack(id,label.replace('\n',' ').replace('\r',' '),count,true));overlay(files,packs,true);committed=true;return count;
  }finally{clean(staging);if(!committed&&dest!=null)clean(dest);}
 }
 /** action: 0 toggle, 1 highest priority, 2 remove. */
 static synchronized void change(File files,String id,int action)throws IOException{
  initialize(files);List<Pack> packs=read(files);Pack selected=null;int at=-1;for(int i=0;i<packs.size();i++)if(packs.get(i).id.equals(id)){selected=packs.get(i);at=i;break;}
  if(selected==null)throw new IOException("Mod no longer installed");
  if(action==0)packs.set(at,selected.enabled(!selected.enabled));else if(action==1){packs.remove(at);packs.add(selected);}else if(action==2)packs.remove(at);else throw new IllegalArgumentException("action");
  overlay(files,packs,true);if(action==2)clean(source(files,selected));
 }
}
