Historical prototype1. Test17 now integrates a separate image-space Vulkan frame-generation mode; see TEST17_FRAME_GENERATION.md. The matrix class below is still standalone. The following notes describe the earlier milestone.

# 60 FPS interpolation: prototype 1 (not integrated)

## Current state

The active APK is still Test16/code51, SHA-256
80863498ea89bb6973915410766cc15e8851a716643c15cdac3615daf34a72d1.
No new APK or live 60 FPS mode is supplied by this prototype. Source is ahead of
that APK only by the files documented here. Native CPU changes and the multiple
PNG mod manager in Test16 remain intact. No GitHub writes are authorized.

The first milestone is a presentation-only matrix history, restoration guard,
synthetic checks and a static audit of the X 1.5 update/draw gate. It does not
include a verified game adapter. No function in the prototype is called by the
shipping host or generated game libraries. Do not advertise game interpolation,
phone performance, saved-game compatibility or a working 60 FPS switch yet.

## Prototype API

`bridge/frame_interpolation.h` keeps bounded copies of previous/current 3x4
matrices, keyed by object identity, lifetime generation and matrix index. The
caller supplies a monotonically increasing original VBlank index and an explicit
continuity decision. Distinct source values establish their own cadence: a
30 Hz source on a60 Hz timeline yields a halfway pose and then the original;
a15 Hz source yields quarter steps. Existing60 Hz changes pass through.

Repeated meshes within the same presentation reuse the same phase. A source
changing multiple times inside one frame is reset rather than treated as a
stable stream. Nonfinite input, rewind, gaps, explicit cuts and new object
lifetimes do not blend stale matrices. Storage has a4096-entry hard cap and
retains no guest pointers. The class is confined to one serialized draw thread;
a future multicore adapter must establish its own synchronization.

The restoration guard is for live native matrix storage that remains valid
through the draw scope. A guest-memory adapter needs checked reads/writes and
restoration on every exit, including faults, scheduling, pause, termination and
exception paths. It must not retain pointers to guest objects. This adapter is
not implemented here. Linear affine blending also needs verified scene/pose
continuity rules; it does not solve rotational discontinuities automatically.

## Static findings for X 1.5

Input digest:
7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74.
The private code was read without modification. The development-only audit
`bridge/tools/probe_frame_loop.py PRIVATE_CODE` rejects a different executable
and emits field/branch facts without executable bytes or disassembly.

The candidate function starts at0x0010E4F4. The mode field is+13 and phase field
+14. A drawing decision is at0x0010E6D0, skip branch0x0010E6DC, drawing return
0x0010E6F4, and candidate post-draw point0x0010E7BC. Time is queried before and
after drawing. A graphics skip field is+494, which differs from the ORAS
reference's+510. These are static candidates; cadence and side effects have
not been observed in our live game. They are not patched or called.

RTTI inspection also found candidate SkeletalModel and WorldMatrixUpdater
vtables (0x00598BF8 and0x00598F0C); no layout or callable hook is inferred from
these names alone. H3dModel metadata is present but its callable path remains
unresolved. Reusing ORAS addresses or object layouts is not permitted.

## Checks and remaining integration gate

The new test was first compiled before the header existed and failed, then
passed with the implementation. Synthetic cases cover nominal120 presentations
and60 unchanged simulation updates, matrix restoration including nested/throwing
draws, repeats,15 Hz cadence, cuts, generations, stale history, nonfinite input,
rewind and bounded storage. This is an artificial simulation, not gameplay.

Run `python3 bridge/tests/run_frame_interpolation_test.py` with g++ and sanitizer
support. AddressSanitizer/UBSan pass in this workspace with
`ASAN_OPTIONS=detect_leaks=0`; LeakSanitizer itself cannot run under its tracing
setup, so leak checking is not claimed. The same test compiles with Android
NDK r27c for ARM64/API26 and strict warnings. The Android object was not run.

Before activating this in an APK:

1. Verify live update/draw cadence for the exact executable, and map camera,
   H3D and NintendoWare model/skeleton paths without guessed layouts.
2. Intercept only render-time reads and restore every temporary matrix before
   the next logic step. Freeze draw-driven UI/fades and reject scene changes.
3. Establish deterministic movement/event/RNG/input and audio comparisons, plus
   original vs intermediate image captures in field, interior and battle.
4. Add explicit original/experimental60 modes with immediate fallback. Budget
   extra drawing separately so a slow GPU drops generated frames rather than
   slowing logic. Measure4x separately; no sustained60 claim is established.

## Research basis

Independent implementation of presentation-history/restoration concepts, not
copied runtime code or executable patches. The ORAS project describes
engine-level matrix interpolation and UI/fade protections; its addresses are
not X addresses. Bergamota offers renderer-level draw replay as a separate,
more invasive alternative. Their own reports are not tests of this port.

- ctr-smooth commit dc019c7e8ea8948b85d9f174fec0c5d2813c7b8b:
  https://github.com/mathis-ait/ctr-smooth/blob/dc019c7e8ea8948b85d9f174fec0c5d2813c7b8b/docs/runtime.md
- Bergamota commit21007338e0655b675ab116273e602ff7d37e28e6:
  https://github.com/ShiromiyaG/Bergamota/blob/21007338e0655b675ab116273e602ff7d37e28e6/src/vk/between.cpp

Game-derived code, source instructions and reference runtime code are not
included in this prototype or the public source tree.
