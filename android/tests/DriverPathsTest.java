package org.pokemonx.recomp;
import java.io.*;
import java.nio.file.*;
public class DriverPathsTest {
 public static void main(String[]args)throws Exception{
  File root=Files.createTempDirectory("driver-path-test").toFile();
  if(!DriverManager.safeChild(root,"folder/driver.so").getCanonicalPath().startsWith(root.getCanonicalPath()+File.separator))throw new AssertionError();
  for(String path:new String[]{"../escape.so","sub/../../escape.so","."}){boolean rejected=false;try{DriverManager.safeChild(root,path);}catch(IOException expected){rejected=true;}if(!rejected)throw new AssertionError("Accepted traversal: "+path);}
  System.out.println("Driver paths: nested files accepted; traversal and root replacement rejected");
 }
}
