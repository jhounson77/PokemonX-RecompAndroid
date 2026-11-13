"""Exercise a reader built with NDK r27c and the previous memory header."""
import struct,sys
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM64,UC_MODE_ARM
from unicorn.arm64_const import UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2,UC_ARM64_REG_SP,UC_ARM64_REG_LR
with open(sys.argv[1],'rb')as stream:
 elf=ELFFile(stream);assert elf['e_machine']=='EM_AARCH64';uc=Uc(UC_ARCH_ARM64,UC_MODE_ARM);image=0x10000000
 for seg in elf.iter_segments():
  if seg['p_type']!='PT_LOAD':continue
  start=seg['p_vaddr']&~4095;end=(seg['p_vaddr']+seg['p_memsz']+4095)&~4095
  uc.mem_map(image+start,end-start);uc.mem_write(image+seg['p_vaddr'],seg.data())
 entry=image+next(s['st_value']for s in elf.get_section_by_name('.dynsym').iter_symbols()if s.name=='LegacyRead32')
 # The data pointer is the first field; the next vector starts after the
 # pinned three-pointer-sized table prefix. Its ABI is checked in C++ too.
 obj=0x100000;table=0x200000;access=0xa00000;ram=0xb00000;out=0xc00000;stack=0xd00000;done=0xe00000
 for address,size in [(obj,4096),(table,8*1048576),(access,1048576),(ram,8192),(out,4096),(stack,8192),(done,4096)]:uc.mem_map(address,size)
 uc.mem_write(obj,struct.pack('<QQQ',table,0,1048576));uc.mem_write(obj+24,struct.pack('<Q',access))
 uc.mem_write(access+2,b'\x03')
 for page,value in [(ram,0xaabbccdd),(ram+4096,0x12345678)]:
  uc.mem_write(page+4,struct.pack('<I',value));uc.mem_write(table+2*8,struct.pack('<Q',page))
  uc.reg_write(UC_ARM64_REG_X0,obj);uc.reg_write(UC_ARM64_REG_X1,0x2004);uc.reg_write(UC_ARM64_REG_X2,out)
  uc.reg_write(UC_ARM64_REG_SP,stack+4096);uc.reg_write(UC_ARM64_REG_LR,done)
  uc.emu_start(entry,done,count=1000)
  assert uc.reg_read(UC_ARM64_REG_X0)==1
  assert struct.unpack('<I',uc.mem_read(out,4))[0]==value
  assert uc.reg_read(UC_ARM64_REG_SP)==stack+4096
print('PASS: NDK ARM64 previous-header reader accepts the live table prefix and sees remaps immediately')
