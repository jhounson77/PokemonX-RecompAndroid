"""Run the presentation-only interpolation prototype on synthetic matrices."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='px-smoothing-') as temp:
 exe=Path(temp)/'test'
 subprocess.run(['g++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
                 '-fsanitize=address,undefined','-fno-omit-frame-pointer',
                 '-I'+str(root/'bridge'),str(root/'bridge/tests/frame_interpolation_test.cpp'),
                 '-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
