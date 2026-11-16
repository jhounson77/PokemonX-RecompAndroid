from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2];src=root/'hexlith/src'
with tempfile.TemporaryDirectory() as d:
 out=Path(d)/'test'
 files=[root/'bridge/tests/decode_cache_test.cpp',*sorted((src/'domain/arm').rglob('*.cpp')),src/'domain/memory/process_memory.cpp',src/'domain/memory/paged_memory.cpp',src/'domain/lift/engine.cpp',src/'core/text.cpp',src/'core/byte_reader.cpp']
 subprocess.run(['g++','-std=c++20','-O2','-ffp-contract=off','-I'+str(src),*[str(f) for f in files],'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
