# Building the Android integration

## Scope

The repository includes the Android frontend, platform bridge, modified Hexlith/Azahar code and vendored build dependencies. **It is not sufficient by itself to reproduce the entire release APK:** generated game-specific native libraries, the matching private cryptographic configuration and signing inputs are external. Do not commit them.

The initial release preserves the tested native CPU tables from the previous development build; it does not claim that the complete base-game generation pipeline has been made into a one-command public build.

## Host/platform library

Use CMake >=3.25, Ninja, an Android SDK with platform/build-tools 35, JDK 21 and NDK r27c. The validated target is `arm64-v8a`, Android API 26, `c++_static`.

Configure `azahar/` in Release with `PX_CUSTOM_FRONTEND=ON`, Vulkan on, OpenGL/Qt/SDL2/web-service/scripting/OpenAL/Cubeb/libusb/FFmpeg/tests/LTO off, precompiled headers off and warnings-as-errors off. Build target `pxhost`. The custom bridge uses a 16 KiB ELF page alignment.

The source tree includes the dependencies used by the integration. The fetch scripts and revision records document their origins; dependency retrieval requires network access if rebuilding a stripped checkout.

## Native game libraries

Use supported, independently prepared game input outside the repository. The update tooling in `bridge/tools/` can extract a **decrypted** supported 1.5 CXI and generate its native source inputs. Generated C++ must stay outside this repository. `android/update-aot/CMakeLists.txt` builds the update libraries with the Android toolchain and requires `PX_UPDATE_SOURCE_DIR` and `PX_HOST_LIBRARY`.

The release uses base `libpxmain.so` / `libpxmodules.so`, and update `libpxmain15.so` / `libpxmodules15.so`. Each modules library must link to `libpxhost.so`. Do not substitute base tables for update tables or disable digest checks.

## Packaging

`android/build_apk.py` takes paths from the environment:

- `PX_SDK`, `PX_JDK`, `PX_NDK`.
- `PX_CRYPTO_CONFIG`: private Java source with the package/class contract shown in `android/examples/CryptoConfig.example.java`.
- `PX_KEYSTORE`, `PX_KEYSTORE_PASSWORD_FILE`: your own signing inputs.
- `PX_REUSE_LIBS_FROM_APK`: optional prior locally supplied compatible binary, used by this development workflow to retain generated native libraries and the driver shim.
- `PX_HOST_LIBRARY`: freshly built host library, when replacing it.
- `PX_UPDATE_MAIN_LIBRARY`, `PX_UPDATE_MODULES_LIBRARY`: optional explicit update libraries.
- `PX_SOURCE_ARCHIVE`: source ZIP included with the APK.
- `PX_AZAHAR_LICENSE`: `azahar/LICENSE.txt`.

A new signing key cannot update installations signed with the original development key. The original private signing key is intentionally not published. The public version name is `0.1.0-beta`; the initial internal version code is 34 to retain the development upgrade path.

## Checks

The workflow under `.github/workflows/` runs public synthetic interface/locale/presentation tests. Additional Android packaging checks verify matching source, JNI exports, native imports, ARM64/16 KiB alignment and signing continuity. Full gameplay still requires the target device.

The production JNI allocation comparison test accepts the previous host source as its first argument. Its baseline is intentionally an external validation input, not game data. Its reported allocation counts are host measurements, not device FPS.
