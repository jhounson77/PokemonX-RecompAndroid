"""Extract already-decrypted update analysis inputs outside the source tree."""
from pathlib import Path
import struct,mmap,sys,hashlib
f=open(sys.argv[1],'rb');m=mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
ex=struct.unpack_from('<I',m,0x1a0)[0]*512;off,size=struct.unpack_from('<II',m,ex+8);src=m[ex+512+off:ex+512+off+size];tb,extra=struct.unpack_from('<II',src,len(src)-8);code=bytearray(src)+bytearray(extra);idx=len(src)-(tb>>24);stop=len(src)-(tb&0xffffff);pos=len(code)
while idx>stop:
 idx-=1;ctrl=src[idx]
 for i in range(8):
  if idx<=stop or pos<=0:break
  if ctrl&128:
   idx-=2;seg=src[idx]|src[idx+1]<<8;count=(seg>>12)+3;dist=(seg&4095)+2
   for j in range(count):code[pos-1]=code[pos+dist];pos-=1
  else:idx-=1;pos-=1;code[pos]=src[idx]
  ctrl=(ctrl<<1)&255
assert hashlib.sha256(code).hexdigest()=='7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74'
(out/'code.bin').write_bytes(code)
rom=struct.unpack_from('<I',m,0x1b0)[0]*512;l3=rom+0x1000;h=struct.unpack_from('<10I',m,l3);assert h[0]==40;dm=l3+h[3];fm=l3+h[7];fd=l3+h[9]
def directory(off,path):
 parent,sibling,child,first,hash,n=struct.unpack_from('<6I',m,dm+off);nm=m[dm+off+24:dm+off+24+n].decode('utf-16le');p=path+('/' if path and nm else '')+nm
 if child!=0xffffffff:directory(child,p)
 k=first
 while k!=0xffffffff:
  parent,nxt,data,size,hash,n=struct.unpack_from('<IIQQII',m,fm+k);nm=m[fm+k+32:fm+k+32+n].decode('utf-16le');fn=p+'/'+nm if p else nm
  if fn.endswith(('.cro','.crs')):
   assert '..' not in Path(fn).parts and not Path(fn).is_absolute();dest=out/'modules'/fn;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(m[fd+data:fd+data+size])
  k=nxt
 if sibling!=0xffffffff:directory(sibling,path)
directory(0,'');print('Verified update executable and extracted module inputs')
