# Android test build 11

Build 04 uses a clean, translucent outlined 3DS control layout. It includes
a captured analog Circle Pad with radial dead zone, an independent digital
directional pad, L/R, SELECT/START, and the Nintendo XYAB diamond.
The persistent bottom action/menu/back bar is removed. A compact top-center
menu icon provides pause, original-screen switching, logs and exit.
Controls remain over the game image by default. The
editor places individual buttons within either half of the game area and
stores normalized positions. Docked controls remain available in settings,
with separate saved positions. New outline profiles begin from the new preset. The launcher,
CIA importer, driver ZIP management, log export and app-private saves remain.

Resolution is now internal Vulkan rasterization scale: 1x (400x240), 2x
(800x480), or 3x (1200x720) for the main screen. It changes render attachments,
depth/stencil images, viewport, scissor and staging dimensions. Fragment
coordinates are mapped back to native PICA coordinates where the game needs
them. Guest buffers and texture addresses retain their native layout;
uploads expand pixels and readbacks select sample centers. Recognized screen
transfers retain a coherent scaled picture for presentation. CPU writes
invalidate that picture. The Android output buffer is no longer resized to
implement resolution selection. Software rendering supports 1x only.

Host Vulkan validation ran 21 synthetic renderer tests, including native
pixel/depth comparisons, high-resolution subpixel coverage at both 2x and
3x, native-memory coherence, and invalidation after CPU writes. A separate
Pokémon X integration run reached frame 350, produced 800x480 pictures at
frames 100 and 200, and saved visible language-selection frames. The Vulkan
context lifetime was also corrected to outlive its backend during shutdown.
Android behavior still requires the owner's device test.

The original simultaneous bottom-screen overlay remains removed. The pause menu
can temporarily replace the central picture with the original menu. Full semantic
integration of battle, PSS, Amie and training menus is not completed. Module
load/unload events are logged to support that work.

Vulkan uses volk. Optional ARM64 driver ZIPs are installed privately and loaded
through libadrenotools; namespace hooks are isolated in a DSO opened only for
custom drivers. The system driver is the default. Archives are bounded and
reject traversal. Driver compatibility still requires real-device testing.

The same application ID and signing key preserve game data on in-place updates.
The main executable is statically lifted; CRO modules retain interpreter
fallback. Audio and broader performance optimization remain unfinished.

Build 04 reuses the exact signed-build-03 native libraries. Synthetic Java tests
cover analog range, dead zone, diagonals, digital directions and layout bounds.
Java compilation, APK signatures, package version and native-library identity
are checked; real Android multi-touch and game response require device testing.

Build 05 adds an optional non-interactive two-line performance HUD below the
upper-left shoulder controls. FPS counts successful native-window posts,
not Android UI refreshes. Speed counts emulated vertical blanks relative to
the runtime's 60 Hz pacing target. RAM is process PSS, sampled with battery
temperature off the UI thread every five seconds; frame rates sample once
per second. Missing readings display a dash. Sampling stops outside gameplay,
on pause, and while the pause menu is open. Settings can disable the HUD.

Native presentation caches horizontal nearest-neighbour indices, copies
repeated rows, and clears only letterbox/padding regions. Synthetic tests
compare every pixel against the previous implementation across native and
scaled source sizes, arbitrary target sizes, clipping and stride padding.
This reduces CPU presentation cost; it does not establish full-speed gameplay.
Cumulative GPU-list, transfer and presentation timings are logged every five
seconds. The user log included an earlier native crash under a custom driver;
there is no backtrace identifying its cause. The DSP service currently fakes
firmware startup/interrupts without mixing PCM or decoding AAC. Audio remains
unimplemented and is explicitly not claimed fixed in this build.

Build 06 reduces CPU-side screen preparation. The Android sink snapshots which
screen it presents. LiveDisplay still synchronizes both guest framebuffer
ranges and runs every GPU command, but composes only the selected picture.
Switching to original menus selects the bottom screen for the next frame;
no game logic or simulation frames are skipped. Other sinks default to both.

A coherent Vulkan scaled framebuffer can now be borrowed read-only during the
synchronous Present call. This avoids copying its pixel vector each frame.
Both read barriers complete before either view is borrowed; CPU-write checks
and invalidation remain in place, and the owning API remains available.
Synthetic display tests cover selection, switching, both read barriers and
pointer identity; Vulkan tests verify borrowed pixels and invalidation after
CPU writes at 2x/3x. Runtime logs now separate guest CPU, GPU list/transfer,
presentation, driver waits, uploads/downloads, pipeline creation, allocation
and interpreted-instruction totals. Five-second FPS/speed intervals restart
while paused. Timers overlap and must not be summed as exclusive CPU stages.
Device gameplay speed remains to be measured with the owner's next log.

Build-06 validation: 21 Vulkan renderer cases executed on Mesa lavapipe (none
skipped), including scaled pixel/depth coherence at 2x/3x and CPU-write
invalidation of both owning and borrowed screen access. All 18 display/screen
cases passed. The 300-frame synthetic preparation benchmark removed 600
pixel-vector copies; its timings exclude rendering, driver synchronization,
rotation and Android output and must not be treated as a gameplay speedup.

Build 07 fixes session log identity and export caching. Each game start creates
an empty private session file named with local timestamp, random suffix and
actual APK versionCode. Its header records device/API/PID and selected render
settings. The absolute session file is passed directly into nativeRun; native
logging and crash markers no longer append to the historical runtime.log.
The last game-session filename is retained for export after app restart.
Launcher diagnostics use a separate fresh file and do not modify a past game
session. Export produces a new immutable, length-bounded snapshot each time,
with a distinct provider URI and matching display filename. The provider
resolves only existing snapshots below log-exports and offers read access.
Unfiltered historical logcat tails are no longer appended to the game log.

Synthetic Java tests cover simultaneous timestamp starts, isolation across
sessions, immutable repeated exports, restart restoration and rejection of
legacy/missing/traversal paths. Native build and Java compilation validate
the three-string JNI run signature. APK signing and 16 KiB alignment are
checked; actual Android sharing remains subject to device verification.
Build-06 performance changes, HUD, controls and settings are preserved.

Build 08 targets pipeline creation observed in the owner's fresh build-07 log:
31,585 ms accumulated synchronous pipeline creation, 25,920 ms driver waiting,
18,607 ms guest CPU, 98,283 ms GPU command lists and 3,976 ms presentation.
These timers overlap and are not additive. Reported intervals were about
2-16 presentation FPS at 2x on the system Adreno 830 driver.

VulkanContext now creates a shared VkPipelineCache with default internal
synchronization for pipeline creation, used by generic and asynchronous
specialized pipelines. Its driver-provided binary is saved after backend
workers finish at normal shutdown, and loaded for later sessions. Files are
separated by vendor, device, driver version and pipelineCacheUUID. A private
size/checksum envelope and the official 32-byte little-endian Vulkan header
are validated before loading. Invalid/unreadable data falls back to an empty
cache. Payloads over 32 MiB are not persisted. Atomic file replacement avoids
truncated live cache files. Runtime logs record loaded cache bytes (zero for
a cold session). No rendering settings or game timing were reduced.

Official references:
https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCacheHeaderVersionOne.html
https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCacheCreateFlagBits.html

All 23 Vulkan/store tests pass on Mesa lavapipe, including cold/warm cache
round trips with identical pixels and corrupt-file fallback. This establishes
cache reuse and image correctness in the host environment, not speed on the
S25. First visits still compile new configurations; compare the same scene
in a second session after exiting normally. Session logs, HUD, control layout
and prior presentation optimizations remain. Audio is unchanged.

Build 09 follows the owner's warm-cache v8 log: 9,070,733 cache bytes loaded,
only 24 ms synchronous pipeline creation, but roughly 7 FPS / 23% simulation
speed during much of the intro. Remaining costs included 54,650 ms GPU lists,
20,796 ms waits, 9,390 ms uploads and 5,526 ms downloads at frame 1463.
These cumulative counters overlap. The supplied second v7 attachment was
byte-identical to the earlier v7 log; the v8 session is the current baseline.

Scaled color/depth/stencil uploads now write directly into mapped staging
with row replication, reusing native scratch buffers. This removes expanded
intermediate allocations/copies and per-output-pixel division. Tests compare
all bytes for 1/2/3x and 1/2/3/4-channel layouts. The host synthetic expansion
benchmark excludes actual game rendering, uploads and the phone driver.

Blend constants and stencil reference/read/write masks use Vulkan core
dynamic state rather than creating pipeline variants for each value. A
render-pass command sequence sets its scissor once and rebinds pipelines,
vertex buffers and dynamic values only when changed. Push constants and
viewports still update for each draw. Reference-comparison tests exercise
changing colors and stencil masks and verify pipeline counts stay stable.
GPU logs now expose texture processing, CPU vertex time and actual counts of
GPU-shaded draws, to identify the next sustained bottleneck on the S25.

26 Vulkan/store/scale tests pass on Mesa lavapipe. These changes do not prove
100% speed on Android. Importing a CIA cannot enumerate every runtime shader
configuration; full import-time compilation is not claimed or faked. The
existing driver cache handles encountered configurations across sessions.
Game timing, selected rendering scale, controls, per-session logs and audio
behavior are unchanged.

Build 10 follows the v9 phone log: intro intervals reached 13-19 FPS and a
64% speed peak, while later intervals dropped to 10-14 FPS. At frame 1696,
CPU vertices accumulated 14,557 ms, texture work 12,912 ms and driver waits
18,160 ms. Texture work includes synchronization, so it is now split into
sync/decode/packing counters before drawing conclusions about the decoder.

CPU shader arithmetic dispatches component operations once per instruction
instead of once per channel, and unary operations skip unused source reads.
That translation unit uses -O3 and loop unrolling with -ffp-contract=off;
fast-math, approximate arithmetic and fused multiply-add remain disabled.
2,000 deterministic random synthetic programs were compared to the previous
implementation in both float precisions, including NaN/Inf inputs. A separate
host-only synthetic benchmark compares O2 baseline and the new shader code;
its result is not a claim of phone or gameplay FPS.

Indexed CPU vertex caching retains the exact previous 64-entry FIFO set and
insertion age but uses bucket chains for lookup and borrows hit results.
Unused output slots are not zeroed; a complete output bank is assigned before
its slot becomes visible. Retained data is fully initialized before any read.
Random collision/eviction tests compare the same FIFO set and output values.
Rendering scale, game timing, AOT main, driver settings, HUD, session logs and
audio remain unchanged. Real-device impact still requires a v10 session log.

Build 11 studies MH4U for Android (khorne-hub/4u-native), repository head
1512d52dbdf5810c1ace12dca2d1633d3a6f752f observed during this work.
Its runtime's resident-image architecture and compiled CPU shader programs
were compared with Hexlith. No MH4U-specific hooks, addresses, game programs,
application code or renderer implementation were copied into this project.
The v10 phone log confirms that 19,466 of 21,491 texture milliseconds were
synchronization, whereas actual decoding was 1,619 ms (counters overlap).

The independently implemented GPU resident-texture path handles exact whole
RGBA8 target matches. It snapshots the GPU image into a GPU-readable buffer,
selecting native texel centers with a nearest blit when scaled, then copies
those texels directly into the draw's shader buffer. No fence wait or CPU
pixel conversion is needed on that path. Unchanged targets reuse their
snapshot by unique slot ID and write revision. Submissions retain shared
ownership until the ring slot completes; snapshot storage is bounded by
32 records and 64 MiB. CPU-write changes, unsupported layouts/formats and
pending overlapping transfers fall back to the existing coherent path.

The new Settings switch 'Reutilizar imagens na GPU' defaults to on. Session
logs report configured mode and resident texture samples/copies. This does
not eliminate other paths' CPU shaders or frame readbacks. No 100% speed
claim is made until the phone is measured. Synthetic comparisons cover
1x/2x/3x, changing GPU output, repeated snapshots and announced CPU writes;
pixels match the old path and eligible cases wait less. Tests run with
Khronos Vulkan validation enabled. All 76 test cases pass.
Reference: https://github.com/khorne-hub/4u-native

Build 12 corrects a gap in the Android integration: the main executable was
lifted, but the CRO modules were not offered to the lifted engine. The v11
phone session ended with 496,283,078 interpreter instructions. Its texture
optimization was active, but did not address this missing CPU coverage.

All 133 modules with executable code were analyzed and lifted from the
user's verified local image: 44,032 functions in 381 translation units.
Generated game sources and libraries live outside this checkout. Package
the ARM64 module library using PX_MODULES_LIBRARY; it is installed as
libpxmodules.so. The runtime offers the tables to a per-session LoadedModules
registry, follows CRO load/unload events, and attaches it to the same engine
as the main executable. Each module must match its code digest before it can
run natively. A missing or different module retains the interpreter fallback.
Logs report module binding outcomes and cumulative module entry counts.

This build still uses Hexlith's kernel, services, graphics and audio paths.
It does not integrate Azahar. The earlier MH4UltimateDC architecture combines
recompiled game functions with an Azahar ARM_Interface adapter, page-table
memory callbacks, system-call forwarding and host fibers for guest threads.
Hexlith's existing generated libraries use another Context/memory ABI and
cannot be plugged into that backend unchanged. Moving to that architecture
requires a memory/context/scheduling adapter and independent validation;
MH4U-specific hooks and addresses are not reusable for Pokemon X.

Reference architecture:
https://github.com/CompaLarios18/MH4UltimateDC/blob/main/docs/PORT_ARCHITECTURE.md

The additional host test target reuses upstream synthetic module-loader and
runtime tests. Device speed, rendering correctness over a full playthrough
and 100 percent speed remain unverified until measured on the phone.
