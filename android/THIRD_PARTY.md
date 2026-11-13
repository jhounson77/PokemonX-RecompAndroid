# Android build attributions

This Android adapter is based on KoyFC/Hexlith, commit
6f0a78d (full revision recorded with the development checkpoint), under
GPL-3.0-or-later. The app includes its corresponding project sources in
assets/android-source.zip and its licence in assets/LICENSE-Hexlith.txt.

The Vulkan shader compiler statically links KhronosGroup/glslang 15.1.0.
Its complete upstream copyright and licence notices are reproduced in
assets/LICENSE-glslang.txt. No glslang source was modified.

The Android build uses Android's MessageDigest instead of OpenSSL. SDL and
GoogleTest are not linked into the APK. Android framework, libc, libm,
libdl, liblog, libandroid and libvulkan are platform libraries. libc++ is
provided by Android NDK r27c under its upstream LLVM notices, reproduced
in assets/LICENSE-NDK.txt.

Game code is generated locally from the user's supplied copy. It is not
part of the open-source project or its licence grant. No game resources
or original CIA are bundled; the user imports their own game.

Test build 02 adds:

- zeux/volk (MIT): dynamic Vulkan dispatch; LICENSE-volk.txt.
- bylaws/libadrenotools (BSD-2-Clause): optional driver loading;
  LICENSE-adrenotools.txt.
- bylaws/liblinkernsbypass (BSD-2-Clause), pinned by libadrenotools:
  Android linker namespaces; LICENSE-linkernsbypass.txt.

Exact dependency revisions are recorded in the development checkpoint.
