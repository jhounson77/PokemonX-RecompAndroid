# Architecture and provenance

This Android integration combines native static recompilation with emulated platform services. It is not a fully rewritten game engine.

| Layer | Implementation |
| --- | --- |
| Main executable and supported CRO code | Hexlith generated native ARM64 libraries |
| Untranslated instructions / unmatched modules | Hexlith interpreter fallback |
| Kernel, IPC, scheduling and filesystem | Azahar |
| PICA200 graphics and presentation | Azahar Vulkan renderer plus Android integration changes |
| DSP and Android output | Azahar DSP, resampling/stretching, custom AAudio sink |
| Launcher, configuration, controls, imports and exports | Native Java/JNI frontend |

## Pinned origins

- Hexlith: https://github.com/KoyFC/Hexlith — `6f0a78d3033a7186f6c445b1d1ecb826a2ddb711`.
- Azahar: https://github.com/azahar-emu/azahar — `5f3306d13cab40bbd5cf38f2cf3ae8b6e6eedca6`.
- libarchive: https://github.com/libarchive/libarchive — v3.8.11, `bb39e54db6cfa582c345535de7b3f3ee3b6e9cd1`.
- Other vendored dependency revisions are recorded in `azahar-tree.json`, `.gitmodules` files and `.source-revision` records.

## Version-specific code

Base title: `0004000000055D00`. Supported update title: `0004000E00055D00`, TMD version 5232 (1.5).

The loader verifies the decompressed executable SHA-256 before binding native tables. The base and update have separate main/CRO libraries. A module whose digest differs from the generated table cannot silently use that table.

The memory adapter keeps the existing three-pointer table prefix ABI used by generated libraries. GPU-cached/unmapped pages and crossing accesses retain coherent fallback paths. Do not change that ABI without rebuilding and validating all generated libraries.

## One-screen interface

Original guest inputs are retained. Presentation geometry remaps touch targets rather than inventing game actions. The reduced asynchronous probe does not wait for the GPU. Menu lifecycle events and learned command signatures control return transitions. No game action is generated merely to detect a screen.

## Distribution boundaries

The source tree contains integration code, modified upstream sources, dependencies, tests and UI artwork. It does not contain the user's CIA, decrypted CXI, RomFS, saves, generated game C++, native game build inputs, signing material or private CryptoConfig source.

The APK is a binary distribution that includes generated native CPU libraries; it is not equivalent to this source-only tree. Reproducing those libraries requires separate, compatible game-derived inputs. This repository does not provide a download service for those inputs and does not claim ownership or a new license over game-derived material.
