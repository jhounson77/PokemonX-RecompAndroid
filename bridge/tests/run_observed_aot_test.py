from pathlib import Path
import subprocess,tempfile,sys
root=Path(__file__).resolve().parents[2]
objects=[str(p)for p in (Path(sys.argv[1])/'CMakeFiles/px-lift-update.dir').rglob('*.o')if p.name!='lift_update.cpp.o']
assert objects
with tempfile.TemporaryDirectory()as d:
 out=Path(d);exe=out/'test';inc='-I'+str(root/'hexlith/src')
 subprocess.run(['g++','-std=c++20','-O2','-ffp-contract=off','-ffunction-sections','-rdynamic',inc,str(root/'bridge/tests/observed_aot_test.cpp'),*objects,'-Wl,--gc-sections','-lcrypto','-ldl','-o',str(exe)],check=True)
 subprocess.run([str(exe),d],check=True)
 libraries=[]
 for name in ['before','after']:
  folder=out/name;lib=out/(name+'.so');libraries.append(str(lib))
  subprocess.run(['g++','-std=c++20','-O1','-ffp-contract=off','-fPIC','-shared','-fvisibility=hidden',inc,*map(str,folder.glob('*.cpp')),'-o',str(lib)],check=True)
 subprocess.run([str(exe),*libraries],check=True)
