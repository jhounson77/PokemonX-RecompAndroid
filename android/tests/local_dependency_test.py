"""Reproduce local-provider symbol lookup without any game content."""
import ctypes
import os
from pathlib import Path
import subprocess
import tempfile

with tempfile.TemporaryDirectory() as folder:
    root = Path(folder)
    (root / 'provider.cpp').write_text('extern "C" int px_test_value(){return 73;}\n')
    (root / 'consumer.cpp').write_text('extern "C" int px_test_value(); extern "C" int px_test_run(){return px_test_value();}\n')
    subprocess.run(['c++', '-shared', '-fPIC', 'provider.cpp', '-Wl,-soname,libpx_test_provider.so', '-o', 'libpx_test_provider.so'], cwd=root, check=True)
    subprocess.run(['c++', '-shared', '-fPIC', 'consumer.cpp', '-o', 'unlinked.so'], cwd=root, check=True)
    subprocess.run(['c++', '-shared', '-fPIC', 'consumer.cpp', '-L.', '-lpx_test_provider', '-Wl,--no-undefined', '-Wl,-rpath,$ORIGIN', '-o', 'linked.so'], cwd=root, check=True)
    provider = ctypes.CDLL(str(root / 'libpx_test_provider.so'), mode=os.RTLD_NOW | os.RTLD_LOCAL)
    try:
        ctypes.CDLL(str(root / 'unlinked.so'), mode=os.RTLD_NOW | os.RTLD_LOCAL)
    except OSError as error:
        assert 'px_test_value' in str(error), error
    else:
        raise AssertionError('An undeclared local provider must not resolve')
    consumer = ctypes.CDLL(str(root / 'linked.so'), mode=os.RTLD_NOW | os.RTLD_LOCAL)
    assert consumer.px_test_run() == 73
    print('PASS: local provider reproduces missing symbol; explicit dependency resolves it.')
