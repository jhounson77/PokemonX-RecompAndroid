# v0.1.2 — cumulative Android release

This release packages Test17 (Android ARM64, versionCode 52). The APK is the exact signed Test17 binary; its internal versionName remains `0.1.2-test17`. The release tag and download name are `v0.1.2`.

## Changes since v0.1.1-beta

- Restore the primary screen after leaving the Bag or Pokémon selection, including cancelling without switching Pokémon. Preserve nested target/summary navigation and resident helper modules.
- Support multiple Citra/Azahar PNG texture ZIP packs, independent toggles/removal and priority ordering. Resolve conflicts by texture hash, restore lower-priority textures, migrate existing packs and recover interrupted overlay updates.
- Add a persistent 30/60 display-frame setting. 30 FPS is the default. 60 FPS uses optional experimental Vulkan motion-compensated image interpolation; native 60 FPS scenes pass through. Game clocks, logic, input, audio and saves retain their original speed.
- Separate original game FPS from host display submissions in the HUD and diagnostics.
- Reduce repeated instruction decoding and operand validation, expand verified native entry/resume coverage for the compatible update 1.5, and directly lower CLZ/REV/REV16 integer instructions. Add CPU, GPU-stage and fallback diagnostics.
- Retain four app languages, save backup/import/export, touch/gamepad controls, screen modes and compatible Vulkan driver selection.

## Validation and limitations

Local synthetic regression checks cover menu lifecycle/cancellation, texture-pack priority/recovery, saves/locales, instruction lowering and presentation cadence. Production Vulkan commands passed four headless GPU cases (RGBA8 UNORM/sRGB at 400x240 and 1600x960) with zero synchronization-validation errors. APK signature/package continuity, ARM64 imports/JNI and 16 KiB alignment were checked.

Test17 still needs gameplay validation on Android devices. Experimental interpolation may add approximately 33 ms of visual latency and produce artifacts around UI, cuts and fast movement. It is display interpolation, not 60 logical game updates per second. Sustained 60 FPS, full native coverage and 100% speed at 4x are not guaranteed. Unsupported texture/ROM patch formats remain unsupported.

Install over the existing app without uninstalling to preserve app data; back up saves first. The public source excludes game dumps, extracted/generated game code, keys and private signing inputs. Rebuilding game-specific libraries requires the external inputs described in BUILD.md.

APK SHA-256: `15561b45a6587b777b02a67f54c89fe06e36d029762681b9925fb0836993bcb3`.

Detailed development notes: TEST2_FIXES.md through TEST17_FRAME_GENERATION.md in this directory (numbering follows the existing development builds).
