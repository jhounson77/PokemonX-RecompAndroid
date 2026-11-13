# Pokemon X: native CPU with Azahar platform

This integration uses Azahar 5f3306d13cab40bbd5cf38f2cf3ae8b6e6eedca6 as the
kernel, IPC services, memory/GPU coherence, Vulkan renderer and DSP platform.
Hexlith's generated main and CRO libraries remain the CPU program. It does not
switch the game to Dynarmic. The Hexlith interpreter remains the fallback for
unlifted instructions. No MH4U-specific code, addresses or hooks are used.

`bridge/arm_native.*` implements Azahar's ARM_Interface with a resumable
Hexlith Engine. Registers, CPSR, VFP and TLS transfer through the scheduler;
SVC returns go to Azahar's SVCContext. Instruction ticks go to Azahar's timer.
The adapter does not need fibers because the lifted engine can return at a
budget boundary with its complete state explicit.

`hexlith/src/domain/memory/process_memory.*` retains the pinned field prefix
used by existing generated libraries. Build16 replaces the owned-only pointer
vector with an explicitly owned/borrowed table of the same three-pointer size
and first-pointer position. A runtime STL-prefix check guards compatibility.
The bridge borrows Azahar's live pointer array without copying pages or
modifying vector ownership internals. Ordinary RAM uses the old generated
reader's inline fast path. GPU-cached or unmapped pages have null pointers and
use the coherent external provider; crossing accesses also delegate. A process
remap or GPU ownership change is visible on the next access. The borrowed
array is never freed or written by the adapter's mapping operations.

The NCCH loader checks the uncompressed executable digest before BSS extension
or patches. CRO metadata and code digests are captured before relocation,
but offered to the native registry only after Azahar's entire CRO load succeeds.
Unload removes the binding after successful unmapping. Unknown/mismatched
modules remain interpreted. Executable patching and savestates are not exposed
by this frontend; further code-write invalidation work is required for them.

The custom JNI frontend uses private app storage, Vulkan, one displayed screen,
the existing controls, render scale and driver shim. Saves live in the new
`azahar/` profile and existing Hexlith saves remain in place. This build does
not convert save formats. Build18 connects DSP output to an Android AAudio shared low-latency stereo
PCM16 stream. DSP stretching/resampling uses the negotiated device rate and
volume is enabled. Input capture stays Null. The callback starts only after
DSP rate setup; stream stop/close precedes destruction of callback data.
Pause/backgrounding mutes output atomically; resume unmutes it. AAC shutdown
now closes/resets the FAAD2 decoder. Init/decode remain the existing FAAD2 path.
No new AAC save/load-state emulation is claimed. Per-second logs include device
rate, frames, signal frames, xruns, stream state and the last callback error.
If a route change disconnects AAudio, restart the gameplay session; automatic
stream recovery/audio focus management are not implemented in this build.
Synthetic API tests cover buffer bounds, stereo frames, negotiated sample rate,
mute/resume, errors and cleanup. TimeStretcher separates native/device rate
conversion from tempo correction; 1kHz tone tests at32728/44100/48000Hz preserve
pitch within25Hz with no missing frames in the measured steady-state window. Audible game output requires phone validation. The old renderer's resident-texture toggle does not
apply to Azahar and is removed from the launcher. The FPS HUD uses Azahar's
system/game frame statistics. Logs mirror the core log into each session.

## Build

The original CIA and generated game C++ are not part of this source tree.
Recreate Azahar and dependencies using `fetch_dependencies.py`,
`fetch_nested.py`, and the exact revisions in `azahar-tree.json`. The small
linkernsbypass dependency is the revision recorded by Azahar's libadrenotools.
The source bundle includes the fetched dependencies for an offline rebuild.

Android configure: Azahar root, NDK r27c Android26 arm64-v8a, c++_static,
Release, PX_CUSTOM_FRONTEND=ON, ENABLE_VULKAN=ON, ENABLE_OPENGL=OFF,
ENABLE_QT/SDL2/WEB_SERVICE/SCRIPTING/OPENAL/CUBEB/LIBUSB/FFMPEG/TESTS/LTO=OFF,
CITRA_USE_PRECOMPILED_HEADERS=OFF, CITRA_WARNINGS_AS_ERRORS=OFF.
Build target `pxhost`; copy its libpxhost.so to the APK alongside the existing
libpxmain.so and corrected libpxmodules.so (both from APK13).
The module library must retain its direct DT_NEEDED on libpxhost.so.
Package through android/build_apk.py using PX_REUSE_LIBS_FROM_APK=APK13,
PX_HOST_LIBRARY=<new pxhost>, PX_SOURCE_ARCHIVE=<this source bundle>, plus
existing private SDK/JDK/NDK/CryptoConfig/signing inputs. Keep signing private.

## Validation limits

The synthetic HLE fixture invokes native functions and reaches actual Azahar
OutputDebugString and ExitProcess SVCs. Its log must contain AZ_NATIVE_SVC_OK.
External-memory tests cover widths, crossing pages, failed accesses, callbacks
and preventing mixed ownership. These tests contain no game content.
An independent integration read of the supplied NCCH through Azahar returned
the same 5,156,864-byte code section and SHA256 as the existing native library.
No S25 gameplay speed claim is made from host tests. Real Vulkan presentation,
full game compatibility, thermal behavior and sustained speed require device
validation. The switch of kernel/services is architectural, not a version bump.

## Build 15 startup correction

The Android path in Settings::GetWorkingGraphicsAPI invokes the optional
AndroidUtils::IsUsingAngleForOpenGL callback. The custom native frontend has
no original Azahar Java class or initialized JVM/method IDs. The callback
previously entered GetEnvForThread and could dereference an uninitialized
JVM during System::Load, matching the v14 startup failure.

This frontend is Vulkan-only and landscape-only. Both optional ANGLE and
portrait callbacks now return false without touching JNI. The retained ARM64
host receives the source-equivalent exported-function replacements through
bridge/tools/apply_android_frontend_hotfix.py. There is no native rebuild
claim for this patch. Symbol addresses, ELF load layout and all other native
code remain fixed; the ELF build ID and runtime build label are refreshed.
The script assembles and emulates each replacement under ARM64, verifying
false return, unchanged stack and preserved argument register. A full future
rebuild uses the matching C++ return-false implementations. CPU/GPU/services,
main/CRO libraries, controls, rendering scale and save profile are unchanged.

The v14 phone log has no native backtrace, so this fixes a verified unsafe
Android-only call path, not a claim that all startup issues are excluded.
Actual S25 startup and gameplay still require testing.

## Build 16 stability work

The v15 phone session used3x resolution and Turnip. It reached long stretches
near30FPS/100percent speed, with severe title-transition drops and isolated
63percent-speed intervals. The log also reported a pipeline cache write
failure. The frontend had omitted LoadDefaultDiskResources, so it did not
initialize/warm the transferable shader cache. Build16 creates both cache
directories before loading, explicitly enables disk caches and loads known
resources before RunLoop. New shader configurations still compile on demand;
no import-time enumeration of all game shaders or skip-draw async mode is
claimed. Cold and warm sessions must be compared on the phone.

Native CPU statistics now sum all cores instead of reporting the last idle
core's zero counters. The HUD takes Azahar's actual sampled FPS/speed directly,
rather than differentiating a counter updated on a separate one-second clock.
No smoothing conceals real slow intervals. Paused intervals are excluded from
the next profiler sample. Scale, game clock, frame limit and audio stay unchanged.

Live-pointer tests use ASan/UBSan with leak detection disabled because the
runner does not support LeakSanitizer. They verify old-header-reader ABI,
remapping, GPU-owned null-pointer fallback, crossing accesses and ownership.
An additional NDKr27c Android26 ARM64 old-header reader runs under Unicorn and
accepts the same table prefix and remaps. A synthetic microbenchmark measures
the access path only; it is not a phone FPS prediction. This version is a full
native rebuild, including the v15 source fixes, not an ELF hotfix.

## Build 17 rendering presets

The UI exposes native1x400x240,2x800x480,3x1200x720,4x1600x960 and
screen-fit Automatic. Automatic chooses the largest integral scale that fits
both surface dimensions, bounded to1..4. Manual selections remain fixed;
there is no hidden performance-driven scale reduction. Requested and effective
scales/target dimensions are logged separately. An explicit0 selects Automatic;
other invalid inputs clamp to1..4. Existing1x..3x preferences retain their meaning.

Azahar's current surface cache uses integer scales. At the native5:3 aspect,
4x is960lines;5x would be1200lines, above the requested1080-line budget.
This build therefore does not claim an exact1080p/1800x1080 render target.
That would require fractional4.5x rendering support throughout the cache,
framebuffers, transfers and texture coordinate paths. No output upscaling is
mislabeled as internal1080p. Additional GPU/thermal cost at4x is real; equal
FPS across all presets is not guaranteed by changing the menu. Native CPU,
cache warmup, FPS reporting, frame limit, driver and audio paths are unchanged.
S25 validation is still required for4x and Automatic.

## Launcher19

Frontend19 uses a landscape Xerneas background provided by the user, with
Jogar/Configurações/Sair only on the home screen. CIA import, video/HUD options,
Vulkan drivers, control editing and log export are grouped in Settings. File
readiness reflects the local imported game, not a hardcoded success label.
Version0.19-test is the APK version; no unverified game update version is shown.
Background is fit to height on the right without stretching, with gradients
for left-side text contrast. Preview is a rendering of the layout, not a phone
screenshot. Runtime libraries remain byte-identical to build18; audio has
been confirmed audible by the user. Second-screen integration is unchanged.

## Launcher20

Modern blue/white launcher based on the approved visual proposal, with native
accessible text controls and vector icons. The original user-provided artwork
is retained as the background; the generated concept is not baked into the UI.
One translucent cyan selector follows touch-down, hover, focus and directional
keys/gamepad navigation; gamepad A activates. Last selected option is persisted
so returning from Settings can keep that option selected. The green import
indicator and success text are hidden when no committed game.cxi exists.
Import commits only after importer validation and final rename; existing
imports remain recognized after update. Busy status replaces success while
importing/stopping. APK20 changes Java/resources only; native libraries remain
byte-identical to18/19. Device UI validation remains pending.

## Launcher21 layout correction

User screenshot of20 revealed full-screen measured children shifted by new
margins: child LayoutParams were changed in onSizeChanged after FrameLayout
had already measured them.21 configures child bounds in onMeasure BEFORE
super.onMeasure and caches viewport dimensions. Text/action LayoutParams
explicitly use TOP|LEFT gravity. The moving selector is now340x50 design units
instead of405x61, with12-unit gaps and14-unit corner radius. Status/import
behavior and selection navigation are retained. Native runtime remains18,
unchanged from20. Real phone layout confirmation is still required.

## Optional screen-interface mod22

Settings now selects one screen or nonoverlapping side-by-side original
screens. In-game menu > Modo de tela e interface selects manual profiles:
exploration (main only), original dual, enlarged bottom, battle commands,
battle moves, or the original bottom menu panel over the main image.
Profiles3/4 hide overlapping 3DS pads by default; pads can be restored from
the same menu. Touch/pointer release and profile switches clear held touches.
The saved one/two-screen preference is used next launch; manual battle scene
profiles reset to exploration to avoid showing battle controls on the title.

Vulkan composes live bottom-screen texture crops after the main picture,
with no CPU framebuffer readback or invented move names/PP. The source and
destination rectangles share common/px_screen_interface.h with the inverse
touch mapper. Portrait/rotate display is not exposed; the app is landscape.
XY battle command/move rectangles were calibrated from public original-game
screenshots, not from Sun/Moon's different UI. Original artwork/labels come
from the user's imported game during execution. Accuracy on the device and
other menu states still needs confirmation. No automatic scene detector is
implemented or claimed. Party/bag/other menus retain a full original panel
or bottom-screen/dual fallback; they are not completely redesigned. No ROM,
save, combat rules, widescreen FOV, CPU or audio changes are made.

Reference studied: mamesj/pokemon-fulldex at
e45308b19febb361426547feae0c2d0560d423c0, frontend/merged.h (Element and
MergedTouch) and frontend/gpucompose.h. Adopted the general crop/compose and
inverse-touch approach, independently implemented for Azahar Vulkan/3DS; no
DS emulation/recomp code was copied. The reference does not yet implement3DS.
Sources consulted:
https://github.com/mamesj/pokemon-fulldex/tree/e45308b19febb361426547feae0c2d0560d423c0
https://image.jeuxvideo.com/images-sm/3d/p/o/pokemon-x-nintendo-3ds-1380810710-180.jpg
https://www.gavas.jp/upload/save_image/15465_3.jpg

Validation: ASan/UBSan synthetic layout/input tests cover both screens,
letterboxing, golden command/move touch coordinates, outside/NaN rejection,
multiple viewport sizes including2340x1080, and rotated texture UV cropping.
Device battle correctness/performance and manual-profile ergonomics pending.

## Interface23 automatic battle presentation and quick popup

Single-screen auto mode0 now uses successful DllBattle load/unload events
plus an80x60 BGRA GPU render of the bottom screen to classify command/move
menus. Unknown/fade/animation images produce scene0 (no battle overlays).
DllField clears battle state; bag/party/viewer module scopes suppress battle
classification until closed. Original mode1 dual and manual profiles remain.
Classifier probes occur250ms apart during battles only, with a forced probe
on actual button/touch press-state changes and a60ms follow-up. No synthetic
player inputs are generated by recognition. The existing staging screenshot
path is reused for the small probe, so GPU synchronization/allocation overhead
exists and must be measured on the phone. Recognition can be disabled in
Settings. UI scene transitions and probe duration are logged. Recognizer is
experimental and calibrated to the supplied original English XY battle.
No claim of all battle/menu states, animations or themes being covered.

Vulkan presentation push constants are128bytes; all four matching present
shaders were compiled with NDK glslc. Ellipse and rounded-corner masks remove
grey crop corners. CPU hit testing uses the same geometric masks; partial
side-button art present in the source remains partial. Original move names
and PP are still live texture samples. No additional copyrighted art added.

Touch controls now stay visible when switching battle profiles. User X key
is forwarded normally and reveals a larger centered live second-screen popup
over the main picture. Swap toggles that popup and restores the previous
presentation, without sending any game command or silently closing submenus.
B retains original game behavior. Popup does not automatically disappear on B
because that key may only return one level inside nested menus. Manual full
bottom and dual modes remain available. Other menus are accessible through
this original popup, not completely redesigned.

Validation includes layout/masked input tests, unknown/fade and state-gating
tests, plus actual displayed source crops reconstructed from the user's v22
screenshots: commands=>3, moves=>4, PSS=>0. These are not raw full-framebuffer
captures. User fixtures are external to source, retained in private checkpoint
validation for reproducibility. Native/Java builds and APK checks passed;
phone appearance, automatic timing and frame-rate overhead still need testing.

Each touch gesture latches the displayed effective profile on ACTION_DOWN.
Moving a held finger cannot retarget a command as a move when automatic
recognition changes scenes; a new gesture is required. A touch starting on
the main picture stays noninteractive if a menu appears underneath it.

## Interface24 grouped decisions and persistent corner menu

Replaces scattered battle artwork with a compact lower-right command group:
Fight above Bag/Run/Pokemon, using clear native touch labels. Live move names
and PP use a2x2 grid in the same bottom-right decision area. No invented move
metadata: move graphics remain sampled from the game. Back is in the same
group; empty slots are suppressed when detected, with a conservative fallback
showing all slots if label recognition is inconclusive. Shared native geometry
determines both rendering bounds and inverse touch targets. Native command
labels and interactive hit targets are drawn by BattlePanel above controller
overlays; all normal touch controls remain present.

Confirmed move/Run selection hides decisions immediately. Recognition rearms
when execution/nondecision ends and original commands return; B cancels the
hidden state for recovery. Bag/party module lifetime shows the original menu
in a smaller bottom-right panel and resets scene on final menu unload. Other
combat states/battle variants remain experimental and need device validation.

Swap removed entirely. X opens the game's menu normally and reveals a persistent
lower-right live bottom panel. While it is open, pressing X again forwards B
for one back-navigation step; normal B behaves the same. The panel remains
through nested menus and closes only when the original PSS interface returns
after observed menu content/explicit back intent. During battle, explicit back
plus return to a recognized battle decision also closes it. This image-based
close detection is calibrated to the original PSS page, not every alternative
bottom-screen app. Manual screen profiles remain recovery options.

BattlePanel latches both geometry and profile per touch gesture, cancels on
window focus loss, and rejects a press across an unseen profile transition.
No timed fake combat actions or altered game rules. Shader/CPU/audio/driver
base retained; geometry/UI lifecycle require the new24 host.


## Android interface test 25

Battle commands now use crops from the original bottom framebuffer, with rounded
Fight corners and ellipse masks on Bag/Run/Pokémon. The lower-right group keeps
the native touch inverse map. D-pad/stick navigation gets an explicit focus ring;
A sends the selected original touch coordinate until release, never both A and a
touch. Confirmation of Bag/Pokémon opens the live corner panel directly, even if
the game reuses a resident CRO. The panel closes when an observed submenu returns
to a recognized command/move screen. Original X/world-menu behavior is retained.
Physical Android gamepad keys, hat and left stick are handled during gameplay.
Two-screen mode and move selection retain guest input behavior. Source-level
controller/lifecycle/geometry tests are meaningful but do not establish device
visual correctness or performance. Test these interactions on the S25.


## Android interface test 26

Restores the native labeled command group from24, with explicit focus markers.
Moves keep their original names/PP from the game and use the same explicit
D-pad/stick/A confirmation routing; empty move slots are skipped. Bag/Pokemon
switch presentation to the full original bottom screen, not a second overlay.
Return requires two samples matching a coarse color signature learned from the
real pre-menu command screen; the old three-color false positive is tested.
The X world menu retains its corner-panel behavior. Classic dual-screen remains
available and does not run automatic interface probes.

Interface readback now reuses one80x60 image and persistently mapped staging
buffer. It queues GPU work and consumes it only after a completed scheduler tick;
there is no detector Finish/Wait. It invalidates noncoherent host memory and
rejects snapshots from an obsolete mode generation. Minimum80ms probe spacing
bounds forced input requests; regular idle choices use250ms, transitions120ms.
The Android decision overlay redraws only when geometry/focus changes, rather
than invalidating the whole overlay30times per second. These remove identified
frontend stalls; no100% speed claim is made for all4x scenes or devices. Vulkan
runtime validation and gameplay accuracy still require S25 testing.


## Android interface/shader test 27

Move cards reskin the outer original button into a blue/navy card while sampling
name, type and PP fields from the current bottom-screen texture every rendered
frame. This is deliberately a hybrid presentation, NOT a validated structured
battle-memory reader or native string metadata API. No fixed move list or OCR is
used. The full160px slot width avoids clipping a final PP digit. The original
skin is a Settings fallback. Empty slots and controller selection remain26.
Changing Pokémon or learning a move uses the game's newly rendered fields;
actual device cases and other languages still need visual validation.

Known shader preparation now waits for shader workers, retries cached pipelines
that were skipped while shader modules were pending, then drains pipeline workers
before gameplay. This closes a startup race; it does not discover all unseen game
pipelines. Driver cache writes use a flushed temporary file + rename so the last
good cache survives a failed write. A real Surface-backed warm-only mode loads
the title/renderer and caches without executing the guest RunLoop; shutdown saves
its driver cache. Settings can run this for an existing import, and by default
a successful CIA import starts it automatically. Back cancels preparation;
errors remain visible. No JIT/CPU/memory ABI, audio or async draw-skipping change.
The first import can have no game cache; new effects still compile on first use.
The warm-only mode and actual performance need Android device validation.
