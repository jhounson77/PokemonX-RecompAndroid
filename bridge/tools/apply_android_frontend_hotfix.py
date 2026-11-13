"""Apply the source-equivalent native-frontend boolean callbacks to APK14.

APK14 was built without LTO. These exported, out-of-line callbacks consult
Azahar's original Java frontend, which this Vulkan-only frontend does not use.
The replacement implements the matching C++ return-false bodies, preserves
all ELF addresses/layout and is checked with AArch64 assembly/emulation.
"""
from pathlib import Path
import hashlib,struct,sys
from elftools.elf.elffile import ELFFile
from keystone import Ks,KS_ARCH_ARM64,KS_MODE_LITTLE_ENDIAN
from capstone import Cs,CS_ARCH_ARM64,CS_MODE_ARM
from unicorn import Uc,UC_ARCH_ARM64,UC_MODE_ARM
from unicorn.arm64_const import UC_ARM64_REG_PC,UC_ARM64_REG_SP,UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_LR

src,dst=map(Path,sys.argv[1:3]);data=bytearray(src.read_bytes())
original=hashlib.sha256(data).hexdigest()
assert original=='67978f34f3766a12fb9b67ee5509dcd8ab8fdd86e4d1c11bea13d8fa7a31d6bf', 'expected APK14 host'
elf=ELFFile(src.open('rb'));symbols=list(elf.get_section_by_name('.dynsym').iter_symbols())
code=bytes(Ks(KS_ARCH_ARM64,KS_MODE_LITTLE_ENDIAN).asm('mov w0, #0; ret')[0])
assert len(code)==8
for name in ['IsUsingAngleForOpenGL','IsPortraitMode']:
 matches=[s for s in symbols if name in s.name and s['st_size']>0];assert len(matches)==1,name
 symbol=matches[0];address=symbol['st_value'];size=symbol['st_size']
 segment=next(p for p in elf.iter_segments() if p['p_type']=='PT_LOAD' and p['p_vaddr']<=address<p['p_vaddr']+p['p_filesz'])
 offset=segment['p_offset']+address-segment['p_vaddr']
 dis=Cs(CS_ARCH_ARM64,CS_MODE_ARM);first=next(dis.disasm(bytes(data[offset:offset+4]),address))
 # Retain a BTI landing pad when the input binary has one.
 prefix=4 if first.mnemonic=='bti' else 0
 assert size>=prefix+len(code)
 data[offset+prefix:offset+prefix+len(code)]=code
 patched=bytes(data[offset:offset+size])
 for initial in [0,1,0x123456789abcdef,0xffffffffffffffff]:
  uc=Uc(UC_ARCH_ARM64,UC_MODE_ARM);base=address&~4095;uc.mem_map(base,8192);uc.mem_write(address,patched)
  stack=0x70000000;uc.reg_write(UC_ARM64_REG_SP,stack);uc.reg_write(UC_ARM64_REG_LR,base+0x1000)
  uc.reg_write(UC_ARM64_REG_X0,initial);uc.reg_write(UC_ARM64_REG_X1,initial)
  uc.emu_start(address,base+0x1000,count=4)
  assert uc.reg_read(UC_ARM64_REG_PC)==base+0x1000
  assert uc.reg_read(UC_ARM64_REG_X0)==0 and uc.reg_read(UC_ARM64_REG_X1)==initial
  assert uc.reg_read(UC_ARM64_REG_SP)==stack
 print(name,'returns false without JVM/memory access; ARM64 emulation passed')
old=b'kernel/services/Vulkan build 14';new=b'kernel/services/Vulkan build 15';assert data.count(old)==1
data=data.replace(old,new)
# Refresh the build identity after modifying executable bytes.
section=elf.get_section_by_name('.note.gnu.build-id');offset=section['sh_offset']
namesz,descsz,kind=struct.unpack_from('<III',data,offset);assert kind==3 and descsz==20
start=offset+12+((namesz+3)&~3);data[start:start+20]=bytes(20);data[start:start+20]=hashlib.sha1(data).digest()
dst.write_bytes(data)
print('input SHA256',original);print('output SHA256',hashlib.sha256(data).hexdigest())
