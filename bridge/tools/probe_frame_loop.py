"""Audit the verified X 1.5 frame-loop candidate without patching input.

Requires the development-only capstone package. Input must live outside source.
Only addresses/field facts are emitted, never instructions or executable bytes.
"""
from pathlib import Path
import hashlib
import json
import sys
from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM
from capstone.arm import ARM_OP_MEM, ARM_OP_IMM

path=Path(sys.argv[1]).resolve()
root=Path(__file__).resolve().parents[2]
assert not path.is_relative_to(root), 'Keep private input outside source'
code=path.read_bytes()
expected='7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74'
assert hashlib.sha256(code).hexdigest()==expected, 'Unsupported executable'
base=0x100000
md=Cs(CS_ARCH_ARM,CS_MODE_ARM);md.detail=True
instructions={i.address:i for i in md.disasm(code[0xe4f4:0xe830],0x10e4f4)}
def field(at,offset):
 i=instructions[at]
 assert i.mnemonic.startswith('ldrb')
 assert any(o.type==ARM_OP_MEM and o.mem.disp==offset for o in i.operands)
def target(at,destination):
 i=instructions[at];assert i.mnemonic.startswith('b')
 assert any(o.type==ARM_OP_IMM and o.imm==destination for o in i.operands)
field(0x10e514,13);field(0x10e520,14);field(0x10e6d0,13);field(0x10e6e4,494)
target(0x10e51c,0x10e534);target(0x10e530,0x10e558);target(0x10e6dc,0x10e6f4)
assert all(instructions[a].mnemonic=='svc' and instructions[a].operands[0].imm==0x28 for a in [0x10e708,0x10e7c0])
print(json.dumps({'code_sha256':expected,'status':'STATIC CANDIDATE ONLY; not enabled or observed live',
 'function_entry':'0x0010E4F4','mode_field':13,'phase_field':14,
 'drawing_gate':'0x0010E6D0','drawing_skip_branch':'0x0010E6DC',
 'drawing_return':'0x0010E6F4','post_draw_candidate':'0x0010E7BC',
 'graphics_skip_field':494,'time_query_sites':['0x0010E708','0x0010E7C0'],
 'unresolved':['live update/draw cadence','camera/model/skeleton hooks','UI/fade side effects','scene continuity and RNG validation']},indent=2))
