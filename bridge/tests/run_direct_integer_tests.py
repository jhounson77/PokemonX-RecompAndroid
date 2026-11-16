"""Synthetic native integer lowering/codegen regressions; no game inputs.

Usage: python3 bridge/tests/run_direct_integer_tests.py <bridge/tools build>
"""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
objects = [str(p) for p in (Path(sys.argv[1]) / 'CMakeFiles/px-lift-update.dir').rglob('*.o')
           if p.name != 'lift_update.cpp.o']
assert objects, 'Build bridge/tools first'
include = '-I' + str(root / 'hexlith/src')
with tempfile.TemporaryDirectory(prefix='px-integer-') as temp:
    out = Path(temp)
    for name in ['direct_integer_lower_test', 'media_lower_test', 'direct_integer_aot_test']:
        exe = out / name
        subprocess.run(['g++', '-std=c++20', '-O2', '-ffp-contract=off',
                        '-ffunction-sections', '-rdynamic', include,
                        str(root / 'bridge/tests' / (name + '.cpp')), *objects,
                        '-Wl,--gc-sections', '-lcrypto', '-ldl', '-o', str(exe)], check=True)
        if name != 'direct_integer_aot_test':
            subprocess.run([str(exe)], check=True)
            continue
        generated = out / 'fixture'
        subprocess.run([str(exe), str(generated)], check=True)
        for p in generated.glob('shard*.cpp'):
            assert 'ExecuteHandled' not in p.read_text() and 'ExecuteDecoded' not in p.read_text()
        library = out / 'fixture.so'
        subprocess.run(['g++', '-std=c++20', '-O2', '-ffp-contract=off', '-fPIC',
                        '-shared', '-fvisibility=hidden', include,
                        *map(str, generated.glob('*.cpp')), '-o', str(library)], check=True)
        subprocess.run([str(exe), '--run', str(library)], check=True)
        print('PASS direct-only generated fixture contains no interpreter-handler calls')
