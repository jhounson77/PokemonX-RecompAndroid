package org.pokemonx.recomp;
import java.io.*;
import java.nio.file.*;
import java.util.*;
public class SessionLogsTest {
 static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
 public static void main(String[]args)throws Exception{
  Path root=Files.createTempDirectory("pokemonx-session-test");SessionLogs logs=new SessionLogs(root.toFile());
  File first=logs.begin(true,7,1,"APK versionCode=7; first session");SessionLogs.append(first,"native profile first");
  File export=logs.snapshot(first,2);String frozen=Files.readString(export.toPath());
  SessionLogs.append(first,"after export");File exportAgain=logs.snapshot(first,2);
  check(!export.equals(exportAgain),"Every export has a different provider name");
  check(Files.readString(export.toPath()).equals(frozen),"Export snapshot must not change while game continues");
  File second=logs.begin(true,7,1,"APK versionCode=7; second session");SessionLogs.append(second,"native profile second");
  check(!first.equals(second),"Two starts in one millisecond need different logs");
  check(!Files.readString(second.toPath()).contains("profile first"),"Second session must contain no old data");
  check(logs.restore(second.getName()).equals(second),"Restore last session after process restart");
  check(SessionLogs.exported(root.toFile(),export.getName()).equals(export),"Provider must resolve the actual snapshot");
  check(logs.restore("runtime.log")==null,"Legacy shared log is not a session");
  for(String invalid:new String[]{"../runtime.log","PokemonX-../private.log.txt","PokemonX-%2Fprivate.log.txt","runtime.log","PokemonX-missing.log.txt"}){
   boolean rejected=false;try{SessionLogs.exported(root.toFile(),invalid);}catch(FileNotFoundException expected){rejected=true;}check(rejected,"Reject traversal, legacy and missing export: "+invalid);
  }
  try(var paths=Files.walk(root)){for(Path path:paths.sorted(Comparator.reverseOrder()).toList())Files.delete(path);}
  System.out.println("Session logs: fresh starts, immutable unique exports, restart restore and provider path validation OK");
 }
}
