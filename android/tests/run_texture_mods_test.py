"""Run synthetic Java file tests without Android or game data."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
jdk=Path(os.environ.get('JAVA_HOME','/usr/lib/jvm/java-21-openjdk-amd64'))
compiler=[str(jdk/'bin/javac')] if (jdk/'bin/javac').exists() else [str(jdk/'bin/java'),'com.sun.tools.javac.Main']
with tempfile.TemporaryDirectory(prefix='px-mod-tests-') as temp:
 p=Path(temp);stub=p/'UpdateInstaller.java'
 stub.write_text('''package org.pokemonx.recomp;
import java.io.*;import java.nio.file.*;
final class UpdateInstaller {static void remove(File f)throws IOException{if(f.isDirectory()){File[] a=f.listFiles();if(a==null)throw new IOException("Cannot list");for(File c:a)remove(c);}Files.deleteIfExists(f.toPath());}}
''')
 sources=[root/'android/java/org/pokemonx/recomp'/n for n in ['ModInstaller.java','TextureMods.java','SaveManager.java','I18n.java']]
 tests=[root/'android/tests'/n for n in ['TextureModsTest.java','FileFeaturesTest.java','I18nTest.java']]
 subprocess.run(compiler+['-d',temp,*map(str,sources+tests),str(stub)],check=True)
 for test in tests:subprocess.run([str(jdk/'bin/java'),'-cp',temp,'org.pokemonx.recomp.'+test.stem],check=True)
