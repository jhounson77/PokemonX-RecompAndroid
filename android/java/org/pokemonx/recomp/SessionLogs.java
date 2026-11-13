package org.pokemonx.recomp;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.*;
// Session files stay private; exports are immutable copies with unique names.
final class SessionLogs {
 private final File logs,exports;
 SessionLogs(File files)throws IOException{logs=new File(files,"logs");exports=new File(files,"log-exports");if((!logs.isDirectory()&&!logs.mkdirs())||(!exports.isDirectory()&&!exports.mkdirs()))throw new IOException("Cannot create log directories");}
 static boolean safeName(String name){return name!=null&&name.matches("PokemonX-[A-Za-z0-9_.-]+\\.log\\.txt");}
 private static String unique(long millis){return new SimpleDateFormat("yyyyMMdd-HHmmss-SSS",Locale.US).format(new Date(millis))+"-"+UUID.randomUUID().toString().substring(0,8);}
 File begin(boolean gameplay,int version,long now,String header)throws IOException{
  File result=new File(logs,"PokemonX-"+(gameplay?"session-":"launcher-")+unique(now)+"-v"+version+".log.txt");
  if(!result.createNewFile())throw new IOException("Session log already exists");
  append(result,"=== "+result.getName()+" ===\n"+header+"\nSession created: "+new Date(now));return result;
 }
 File restore(String name){if(!safeName(name))return null;File f=new File(logs,name);return f.isFile()?f:null;}
 static synchronized void append(File file,String line)throws IOException{try(FileWriter writer=new FileWriter(file,true)){writer.write(line);writer.write('\n');}}
 File snapshot(File source,long now)throws IOException{
  if(source==null||!source.isFile())throw new IOException("No session log available");
  String stem=source.getName().replace(".log.txt","");File target=new File(exports,stem+"-export-"+unique(now)+".log.txt");
  if(!target.createNewFile())throw new IOException("Export already exists");
  try(InputStream in=new FileInputStream(source);OutputStream out=new FileOutputStream(target)){
   long remaining=source.length();byte[]buffer=new byte[16384];while(remaining>0){int n=in.read(buffer,0,(int)Math.min(buffer.length,remaining));if(n<0)break;out.write(buffer,0,n);remaining-=n;}
  }catch(IOException error){target.delete();throw error;}return target;
 }
 static File exported(File files,String name)throws FileNotFoundException{
  if(!safeName(name))throw new FileNotFoundException("Invalid log name");
  File parent=new File(files,"log-exports"),result=new File(parent,name);
  try{if(!result.getCanonicalFile().getParentFile().equals(parent.getCanonicalFile())||!result.isFile())throw new FileNotFoundException("Log does not exist");}catch(IOException error){throw new FileNotFoundException("Cannot resolve log");}
  return result;
 }
}
