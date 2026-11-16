"""Synthetic presentation/GPU checks, requiring Vulkan software driver and validation layer.
Usage: python3 bridge/tests/run_frame_generation_tests.py PATH_TO_GLSLANG
Vulkan ICD/layer/library search paths may be supplied through standard environment variables.
"""
from pathlib import Path
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[2]
glslang=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='px-frame-generation-') as temp:
 out=Path(temp)
 common=['g++','-std=c++20','-O1','-I'+str(root/'azahar/src')]
 subprocess.run(common+['-Wall','-Wextra','-Werror',str(root/'bridge/tests/presentation_policy_test.cpp'),'-o',str(out/'policy')],check=True)
 subprocess.run([str(out/'policy')],check=True)
 files=['vulkan_present.vert','vulkan_frame_flow.frag','vulkan_frame_resolve.frag']
 spirv=[]
 for name in files:
  target=out/(name+'.spv');spirv.append(str(target))
  subprocess.run([str(glslang),'-V',str(root/'azahar/src/video_core/host_shaders'/name),'-o',str(target)],check=True)
 subprocess.run(common+['-I'+str(root/'azahar/externals/vulkan-headers/include'),str(root/'bridge/tests/frame_generation_gpu_test.cpp'),'-l:libvulkan.so.1','-o',str(out/'gpu')],check=True)
 subprocess.run([str(out/'gpu'),*spirv],check=True)
