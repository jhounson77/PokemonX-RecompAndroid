package org.pokemonx.recomp;
import android.content.Context;
import android.net.Uri;
import org.json.JSONObject;
import java.io.*;
import java.util.*;
import java.util.zip.*;
final class DriverManager {
 static final class Driver{final File dir;final String name,library;Driver(File dir,String name,String library){this.dir=dir;this.name=name;this.library=library;}}
 static String text(File f)throws IOException{try(InputStream in=new FileInputStream(f);ByteArrayOutputStream b=new ByteArrayOutputStream()){byte[]buf=new byte[4096];int n;while((n=in.read(buf))!=-1){b.write(buf,0,n);if(b.size()>131072)throw new IOException(I18n.t("Metadados muito grandes."));}return b.toString("UTF-8");}}
 static Driver read(File dir)throws Exception{JSONObject meta=new JSONObject(text(new File(dir,"meta.json")));String lib=meta.optString("libraryName","");if(!lib.matches("[A-Za-z0-9_.+-]+\\.so"))throw new IOException(I18n.t("libraryName inválido."));File binary=new File(dir,lib);if(!binary.isFile())throw new IOException(I18n.t("Biblioteca do driver não encontrada."));try(InputStream in=new FileInputStream(binary)){byte[]h=new byte[20];if(in.read(h)!=20||h[0]!=127||h[1]!='E'||h[2]!='L'||h[3]!='F'||h[4]!=2||h[18]!=(byte)0xb7||h[19]!=0)throw new IOException(I18n.t("O driver precisa ser ELF ARM64."));}return new Driver(dir,meta.optString("name",dir.getName()),lib);}
 static List<Driver> list(Context c){List<Driver> result=new ArrayList<>();File[]dirs=new File(c.getFilesDir(),"drivers").listFiles();if(dirs!=null)for(File d:dirs)try{result.add(read(d));}catch(Exception ignored){}result.sort((a,b)->a.name.compareToIgnoreCase(b.name));return result;}
 static File safeChild(File root,String name)throws IOException{File child=new File(root,name);if(!child.getCanonicalPath().startsWith(root.getCanonicalPath()+File.separator))throw new IOException(I18n.t("Caminho inválido no ZIP."));return child;}
 static void remove(File f){File[]children=f.listFiles();if(children!=null)for(File x:children)remove(x);f.delete();}
 static Driver install(Context c,Uri uri)throws Exception{File root=new File(c.getFilesDir(),"drivers"),temp=new File(root,".import-"+UUID.randomUUID());if(!temp.mkdirs())throw new IOException(I18n.t("Não foi possível criar a pasta do driver."));boolean success=false;
  try{long total=0;int count=0;try(InputStream raw=c.getContentResolver().openInputStream(uri);ZipInputStream zip=new ZipInputStream(raw)){ZipEntry entry;byte[]buf=new byte[1024*1024];while((entry=zip.getNextEntry())!=null){if(++count>512)throw new IOException(I18n.t("ZIP com arquivos demais."));File target=safeChild(temp,entry.getName());if(entry.isDirectory()){target.mkdirs();continue;}target.getParentFile().mkdirs();try(OutputStream out=new FileOutputStream(target)){int n;while((n=zip.read(buf))!=-1){total+=n;if(total>512L*1024*1024)throw new IOException(I18n.t("Driver maior que 512 MB."));out.write(buf,0,n);}}}}
   File actual=temp;if(!new File(actual,"meta.json").exists()){File[]sub=temp.listFiles(File::isDirectory);if(sub!=null&&sub.length==1)actual=sub[0];}Driver checked=read(actual);File result=new File(root,"driver-"+UUID.randomUUID());if(!actual.renameTo(result))throw new IOException(I18n.t("Não foi possível concluir a instalação."));success=true;return read(result);
  }finally{remove(temp);}
 }
}
