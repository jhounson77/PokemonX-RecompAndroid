# Test17: selectable 30/60 display output

APK0.1.2-test17/code52 adds Settings > Display > Display frame limit:
30 FPS (default) and60 FPS / experimental interpolation. Changes apply next
session. This is image-space motion-compensated frame generation in Vulkan,
not an unlock of the game's logic or the ORAS patch. The earlier matrix-history
prototype remains standalone and unused by the game.

## Behavior and isolation

The guest's clocks, CPU budgets, update/draw calls, input, RNG, saves and audio
speed are unchanged. Host frame limiting and CPU clock remain100%. Original
VBlank frequency is retained. Only composed host display images are captured;
no generated image is written into a guest framebuffer or guest memory. ROM
and AOT instruction bytes are not patched. Native AOT libraries are retained
byte-for-byte fromTest16; only the host library is rebuilt.

The30 option limits display submissions to about30 per original60 VBlanks;
slower content is not padded with duplicates. Native60-FPS scenes still simulate
at their original cadence while their display is capped. The60 option passes
natural60 updates through. After a stable30-source cadence, it presents the
preceding captured image and then a motion-compensated intermediate picture.
This introduces approximately one30-FPS source-frame of visual latency (about
33 ms), and does not turn30 logical updates into60 logical updates.

Motion is estimated bidirectionally at100x60 and used to warp full-size images.
The search is bounded to16 reference pixels per axis, at a400x240 reference.
Sampling uses the same128-byte push layout and descriptor layout as presentation.
Pixel disagreement and out-of-bounds samples fall back to current content.
Image-space estimation can produce artifacts around UI, occlusion, rapid motion
and cuts; this is an experimental optional mode, not a promise of clean60 FPS
in every scene. There is no AI model or external frame-generation dependency.

History resets on layout/surface/pause epochs and on cadence changes. Popup and
full battle-menu states use original output. A VBlank gap over24 ms suppresses
interpolation for a recovery window; a slow host therefore drops generated
pictures rather than adding game updates or changing the guest clock. This
heuristic is not a guarantee of100% speed, nor a full GPU-time measurement.
Pipeline/format/allocation failures disable interpolation with a log message.
Mode30 does not compile the experimental shaders or allocate history images.

The HUD shows Game (original game-frame statistics) separately from Display
(host submissions including intermediates). Display is not a measurement of
physical panel scanout. Session logs include target, submitted FPS and generated
frame counts; existing speed_percent remains the original speed statistic.

## Implementation

`common/px_frame_generation.h` is a bounded output-cadence policy and counters.
`renderer_frame_generation.inc` integrates private history images into Vulkan
presentation; `frame_generation_commands.h` shares production command recording
with the headless GPU fixture. Two fragment shaders perform block matching and
resolve. All resources are reclaimed after GPU/presentation shutdown. Host frame
copies explicitly synchronize the render-pass final layout transition.

The engine-level camera/skeleton approach in ctr-smooth remains a possible later
alternative. This release deliberately uses display-only images to avoid the
unverified game-specific matrix/UI/fade hooks identified in prototype1. No
reference runtime code, executable patch or game-derived bytes enter source.

## Verification

The cadence tests cover30 capping without inventing slower-source frames,
30-to60 output scheduling, natural60 pass-through, epoch/rewind handling,
UI ineligibility and slow-host suppression. They have no simulation-clock API.

The actual production GPU commands and shaders run on Mesa llvmpipe with
Khronos synchronization validation, RGBA8 UNORM and sRGB, at400x240 and1600x960.
The moving synthetic object must appear near the midpoint rather than either
endpoint. Tests also reuse sampled history/flow/output resources, verify static
images do not shimmer, and check fallback on an unrelated bright cut. Final
runs report four cases and zero Vulkan validation errors. Earlier failing
midpoint/synchronization cases are kept in the private validation record.
These synthetic tests are not tests of the real game's scenes or a phone GPU.

Run `python3 bridge/tests/run_frame_generation_tests.py PATH_TO_GLSLANG` with a
Vulkan driver and VK_LAYER_KHRONOS_validation available. Standard Vulkan ICD,
layer and dynamic-library environment variables select a development driver.
The test never reads game content. Existing menu cancellation, multi-mod,
save-roundtrip/security and locale checks remain applicable. APK checks verify
package/certificate, native imports/JNI, ARM64/16KiB, eleven retained libraries
and current embedded source. CPU main remainsTest16; native host is nowTest17.

Phone validation: install overTest16 without reinstalling the CIA, begin with30,
then stop the game and choose60. Compare field, interiors, battle, menus,
transitions and audio at the same resolution/driver/packs. Export a session log
and, for visual quality, record a short video. The4x GPU fixture demonstrates
correct synthetic output at that size; it establishes no Android4x FPS result.
No GitHub publication is authorized before phone tests and explicit approval.
