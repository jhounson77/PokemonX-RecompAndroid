# Private 0.1.2 Test 3: intro/logo fallback work

No GitHub write is authorized before phone testing and explicit user approval.

## Device evidence

The supplied Test2/code37 session is from a Samsung SM-S931B, internal scale4,
1600x960, custom Freedreno driver and asynchronous PNG texture pack enabled.
This differs from the previous Xiaomi code35/scale1 session; cross-device FPS
must not be attributed to the decoder cache change.

The new session starts the animated intro around30 FPS/~100% speed. It falls
to roughly12-16 FPS/~40-53% speed, before recovering to60 FPS/~100%. In the
slow phase, sampled interpreted-count deltas are roughly10-16 million per
approximately one-second reporting interval, and decode misses account for
about33-36% of the interpreted count. This indicates substantial fallback work,
not proof that decoder misses alone explain the slowdown. GPU_seconds measures
host GPU-processing scope, not complete GPU device occupancy or fence waits.
Scale4 and the texture pack can add costs and are retained for a fair phone test.

The cache preload takes84ms. The log also contains831 missing replacement
warnings, including many duplicates. Partial texture packs are expected to
leave originals unchanged; a missing replacement is not a failed game texture.

## Changes

- A fixed512-set/two-way decoded cache holds1024 encodings instead of256
  direct-mapped entries. A hit updates the victim way so alternating colliding
  encodings can coexist. Fresh memory fetch, Thumb boundary rules and the
  original StepDecoded semantics are preserved. No game bytes or offsets added.
- Normal texture misses are now debug diagnostics rather than per-draw warnings.
  Replacement lookup still checks the live map, so new entries stay visible.
  No unbounded negative-cache allocation and no texture quality changes.

## Checks and limits

Synthetic differential ARM/Thumb tests cover live flags/PC, code changes, faults,
remapping, complete CPU state, instruction budgets and SVC. A regression first
failed with Test2 when two encodings mapped to the same set, and now needs only
two decodes across1000 alternating executions.

A separate synthetic32-encoding collision fixture changed1,000,000 decodes to32
and host runtime22.25ms to11.81ms; the two-encoding hot set remained similar
(11.13ms vs11.34ms). This is not a replay of the device instruction stream and
is not a phone FPS prediction. Cache memory is63,504 bytes per engine, versus
15,376 before (about188KiB extra across four cores on this host layout).

The production GetMaterial body was exercised with10,000 missing lookups,
existing replacements and a replacement registered after earlier misses.
Misses no longer format warning logs; the map does not grow from misses.
Party cancellation and menu lifecycle regressions remain passing.

Phone validation: install over Test2, keep the same scale4/driver/pack settings,
play from launch through the logo to Press Start, then export the session log.
Compare speed_percent and decode misses per interpreted instruction in the slow
phase. Do not promise60 FPS in sequences that run at30 FPS at normal speed.
