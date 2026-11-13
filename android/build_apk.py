#!/usr/bin/env python3
"""Package the Android test build from already compiled native libraries.

PX_SDK, PX_JDK, PX_NDK, PX_MAIN_LIBRARY, PX_MODULES_LIBRARY, PX_CRYPTO_CONFIG, PX_KEYSTORE and
PX_KEYSTORE_PASSWORD_FILE locate private build inputs outside source control.
"""
import os
from pathlib import Path
import shutil
import subprocess
import zipfile
import struct

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / 'build' / 'apk'

def env(name):
    return Path(os.environ[name]).resolve()

def run(*args):
    subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True)

def needed_libraries(path):
    """Read ELF64 DT_NEEDED without relying on a platform readelf binary."""
    data = path.read_bytes()
    if data[:6] != b'\x7fELF\x02\x01':
        raise ValueError(f'{path}: expected little-endian ELF64')
    offset = struct.unpack_from('<Q', data, 32)[0]
    size, count = struct.unpack_from('<HH', data, 54)
    segments = [struct.unpack_from('<IIQQQQQQ', data, offset + index * size) for index in range(count)]
    dynamic = next(segment for segment in segments if segment[0] == 2)
    entries = []
    for position in range(dynamic[2], dynamic[2] + dynamic[5], 16):
        tag, value = struct.unpack_from('<qQ', data, position)
        if tag == 0:
            break
        entries.append((tag, value))
    address = next(value for tag, value in entries if tag == 5)
    load = next(segment for segment in segments if segment[0] == 1 and segment[3] <= address < segment[3] + segment[5])
    strings = load[2] + address - load[3]
    return {data[strings + value:data.index(b'\0', strings + value)].decode() for tag, value in entries if tag == 1}

def main():
    sdk, jdk = env('PX_SDK'), env('PX_JDK')
    reuse = os.environ.get('PX_REUSE_LIBS_FROM_APK')
    tools = sdk / 'build-tools' / '35.0.0'
    platform = sdk / 'platforms' / 'android-35' / 'android.jar'
    for folder in ['classes', 'dex', 'res', 'stage/lib/arm64-v8a', 'stage/assets']:
        (BUILD / folder).mkdir(parents=True, exist_ok=True)
    compiler = [jdk / 'bin/javac'] if (jdk / 'bin/javac').exists() else [jdk / 'bin/java', 'com.sun.tools.javac.Main']
    run(*compiler, '-source', '11', '-target', '11', '-classpath', platform,
        '-d', BUILD / 'classes', *(ROOT / 'android/java').rglob('*.java'), env('PX_CRYPTO_CONFIG'))
    run(tools / 'd8', '--min-api', '26', '--lib', platform, '--output', BUILD / 'dex',
        *(BUILD / 'classes').rglob('*.class'))
    run(tools / 'aapt2', 'compile', '--dir', ROOT / 'android/res', '-o', BUILD / 'res/resources.zip')
    run(tools / 'aapt2', 'link', '-o', BUILD / 'base.apk', '--manifest', ROOT / 'android/AndroidManifest.xml',
        '-I', platform, BUILD / 'res/resources.zip')
    native = BUILD / 'stage/lib/arm64-v8a'
    if reuse:
        with zipfile.ZipFile(reuse) as previous:
            for name in previous.namelist():
                if name.startswith('lib/arm64-v8a/') and name.endswith('.so'):
                    (native / Path(name).name).write_bytes(previous.read(name))
    else:
        ndk = env('PX_NDK')
        shutil.copy2(ROOT / 'build/android-arm64/libpxhost.so', native / 'libpxhost.so')
        shutil.copy2(env('PX_MAIN_LIBRARY'), native / 'libpxmain.so')
        if os.environ.get('PX_MODULES_LIBRARY'):
            shutil.copy2(env('PX_MODULES_LIBRARY'), native / 'libpxmodules.so')
        shutil.copy2(ROOT / 'build/android-arm64/libpxdrivers.so', native / 'libpxdrivers.so')
        for path in (ROOT / 'build/android-arm64/adrenotools/src/hook').glob('*.so'):
            shutil.copy2(path, native / path.name)
        strip = ndk / 'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip'
        for path in native.glob('*.so'):
            run(strip, '--strip-unneeded', path)
    if os.environ.get('PX_HOST_LIBRARY'):
        shutil.copy2(env('PX_HOST_LIBRARY'), native / 'libpxhost.so')
        if not os.environ.get('PX_HOST_ALREADY_STRIPPED'):
            run(env('PX_NDK') / 'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip', '--strip-unneeded', native / 'libpxhost.so')
    if os.environ.get('PX_UPDATE_MAIN_LIBRARY'):
        for variable, filename in [('PX_UPDATE_MAIN_LIBRARY','libpxmain15.so'),('PX_UPDATE_MODULES_LIBRARY','libpxmodules15.so')]:
            shutil.copy2(env(variable), native / filename)
            run(env('PX_NDK') / 'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip', '--strip-unneeded', native / filename)
    assets = BUILD / 'stage/assets' 
    modules = native / 'libpxmodules.so'
    if modules.exists() and 'libpxhost.so' not in needed_libraries(modules):
        raise ValueError('libpxmodules.so must declare libpxhost.so as DT_NEEDED; build with android/lifted-modules')
    shutil.copy2(ROOT / 'LICENSE', assets / 'LICENSE-Hexlith.txt')
    shutil.copy2(ROOT / 'THIRD_PARTY_NOTICES.md', assets / 'THIRD_PARTY_NOTICES.md')
    if not os.environ.get('PX_SOURCE_ARCHIVE'):
        # Include all current project sources, including new files not committed
        # yet. Build products and private inputs are excluded by gitignore.
        names = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
            '--exclude-standard', '-z'], cwd=ROOT).decode().split('\0')
        with zipfile.ZipFile(assets / 'android-source.zip', 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as source:
            for name in sorted(set(names)):
                path = ROOT / name
                if name and path.is_file():
                    source.write(path, name)

    if (ROOT / 'android/assets').is_dir():
        shutil.copytree(ROOT / 'android/assets', assets, dirs_exist_ok=True)

    if os.environ.get('PX_SOURCE_ARCHIVE'):
        shutil.copy2(env('PX_SOURCE_ARCHIVE'), assets / 'android-source.zip')
    if os.environ.get('PX_AZAHAR_LICENSE'):
        shutil.copy2(env('PX_AZAHAR_LICENSE'), assets / 'LICENSE-Azahar.txt')
    unsigned = BUILD / 'unsigned.apk' 
    shutil.copy2(BUILD / 'base.apk', unsigned)
    with zipfile.ZipFile(unsigned, 'a', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        archive.write(BUILD / 'dex/classes.dex', 'classes.dex')
        for path in (BUILD / 'stage').rglob('*'):
            if path.is_file():
                archive.write(path, str(path.relative_to(BUILD / 'stage')))
    aligned = BUILD / 'aligned.apk'
    run(tools / 'zipalign', '-P', '16', '-f', '4', unsigned, aligned)
    result = BUILD / 'PokemonX_Recomp_v0.1.0_Beta.apk'
    run(tools / 'apksigner', 'sign', '--ks', env('PX_KEYSTORE'), '--ks-pass',
        'file:' + str(env('PX_KEYSTORE_PASSWORD_FILE')), '--v1-signing-enabled', 'false',
        '--v2-signing-enabled', 'true', '--v3-signing-enabled', 'true',
        '--v4-signing-enabled', 'false', '--out', result, aligned)
    run(tools / 'apksigner', 'verify', '--verbose', result)
    run(tools / 'zipalign', '-c', '-P', '16', '4', result)
    print(result)

if __name__ == '__main__':
    main()
