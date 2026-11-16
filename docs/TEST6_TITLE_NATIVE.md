# Private Test6: Title coverage and specialized byte operations

No GitHub writes before phone tests and explicit user approval.

The Test5/code40 Samsung log shows the intro initially near30FPS/100% speed,
then roughly20-27FPS/~66-89% speed. Same settings: scale4, custom driver, PNG
pack. This remains below normal speed; cross-session timing/thermal conditions
are not controlled, so no precise gain is attributed to one prior change.

The prior Test4 session also recorded23 distinct ARM PCs in DllTitle. They are
converted to CRO-relative offsets using the logged load base; the module analyzer
checks these against the actual code segment. Counts indicate presence only,
not the profiler's uncertain ranking/frequency estimates. Existing main coverage
from Test5 is retained unchanged.

Observed Title analysis adds one function (557->558) and7 block entry points.
All3640 previous Title entries are retained. All23 sampled instructions have
emitted groups:10 inline-native and13 specialized native handler calls. Those13
use ParallelArithmetic/SEL, which previously went through ExecuteDecoded. The
lifter now emits constant decoded operands through ExecuteHandled, using the
existing canonical ARM media handlers (no new arithmetic semantics). The emitter
names both enum values explicitly, preserving prefix/shape/GE fields. This is
specialization of handler calls, not a claim of a rewritten CPU or100% native.

A dedicated libpxintro15.so contains the complete rebuilt Title table, with its
original code-segment hash. For update1.5 only, Program loads it before the retained
module library: LoadedModules deliberately chooses the first offer. All other
modules continue using their original tables. Actual module load still checks
its code digest; a mismatch never executes the supplemental table. The library
and its tables outlive all game CPU instances. The base version does not load it.

Validation: lowering agrees with interpreter semantics for all36 parallel lane
variants and all16 SEL GE masks with randomized operands, full GPR/CPSR/Q results
and invalid-PC rejection. The evaluator fetches from synthetic mapped code, as
its public contract requires. A before-change test rejected specialized lowering.
A compiled synthetic before/after AOT fixture now includes parallel arithmetic
and SEL and matches interpreter registers, flags, stores, budgets0..100, SVC and
write faults. Supplemental module tests check first-offer priority, relocated
PCs/mode, unrelated modules, unload and digest mismatch refusal. Menu and CPU
regressions remain required; native package checks include the new module imports.

Private generated code/observed tables stay outside the source checkout. Retain
original update15 sources and binaries, plus private/intro15-sources.zip and the
observed Title table for reproduction. Build bridge/tools/px-lift-update; normal
mode accepts an observed-modules directory after the observed-main table; the
--emit-module mode generates one module's library from its verified CRO and tables.
Build android/supplemental-aot with PX_SUPPLEMENT_SOURCE_DIR/PX_HOST_LIBRARY and
package PX_SUPPLEMENT_MODULES_LIBRARY; APK builds with update1.5 libraries must
include the supplemental library. Detailed runtime sampling remains disabled.

Phone: install over Test5, keep update1.5 and the same settings, run the full intro
through the slow logo to Press Start and export the log. Confirm visual correctness
and speed; this build has not been run on the target phone. Continue mapping any
remaining fallback after this measured comparison.
