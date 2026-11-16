package org.pokemonx.recomp;
import java.io.*;
public class BattleCatalogTest {
 static void require(boolean ok){if(!ok)throw new AssertionError();}
 public static void main(String[] args)throws Exception{
  BattleCatalog c=BattleCatalog.read(new FileInputStream(args[0]));
  require("Froakie".equals(c.species(656,0)));
  require("Pound".equals(c.move(1,1))&&"Growl".equals(c.move(45,1))&&"Bubble".equals(c.move(145,1)));
  require(c.move(1,2)!=null&&!c.move(1,2).equals(c.move(1,1)));
  require(c.move(1,3)!=null&&!c.move(1,3).equals(c.move(1,1)));
  require(c.move(0,0)==null&&c.move(622,0)==null&&c.species(722,0)==null);
  for(int i=1;i<=621;i++)require(c.move(i,0)!=null);
  for(int i=1;i<=721;i++)require(c.species(i,0)!=null);
  try{BattleCatalog.read(new ByteArrayInputStream("M\t1\ta\tb\tc\nM\t1\tx\ty\tz\n".getBytes("UTF-8")));throw new AssertionError();}catch(IOException expected){}
  try{BattleCatalog.read(new ByteArrayInputStream("M\tx\ta\tb\tc\n".getBytes("UTF-8")));throw new AssertionError();}catch(IOException expected){}
  System.out.println("PASS catalog IDs, complete Gen6 names, localization, unknown fallback and malformed/duplicate rejection");
 }
}
