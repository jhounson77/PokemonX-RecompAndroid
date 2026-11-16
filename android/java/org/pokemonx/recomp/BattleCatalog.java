package org.pokemonx.recomp;
import java.io.*;
import java.nio.charset.StandardCharsets;
/** Offline names indexed by live IDs; unknown IDs retain original game fields. */
final class BattleCatalog {
 private final String[][] species=new String[722][],moves=new String[622][];
 static BattleCatalog read(InputStream input)throws IOException{
  BattleCatalog c=new BattleCatalog();
  try(BufferedReader reader=new BufferedReader(new InputStreamReader(input,StandardCharsets.UTF_8))){
   String line;while((line=reader.readLine())!=null){String[] v=line.split("\t",-1);if(v.length!=5)throw new IOException("Invalid catalog row");
    int id;try{id=Integer.parseInt(v[1]);}catch(NumberFormatException bad){throw new IOException("Invalid catalog ID",bad);}
    String[][] table=v[0].equals("P")?c.species:v[0].equals("M")?c.moves:null;
    if(table==null||id<1||id>=table.length||table[id]!=null)throw new IOException("Invalid or duplicate catalog ID");
    table[id]=new String[]{v[2],v[3],v[4]};
   }
  }return c;
 }
 private static String name(String[][] table,int id,int language){return id>0&&id<table.length&&table[id]!=null?table[id][language==2?1:language==3?2:0]:null;}
 String species(int id,int language){return name(species,id,language);}
 String move(int id,int language){return name(moves,id,language);}
}
