# Private Test5: observed native CPU coverage, XY1.5

No GitHub writes before phone tests and explicit user approval.

## Evidence and change

The code39 session shows sustained fallback on core0 during the intro/logo.
Many sampled PCs are in main executable regions absent from the original
function analysis. Space-Saving per-PC error bounds are nearly their counts:
this profile establishes address presence, not reliable individual rankings or
an exact percentage of CPU time. Other cores report no samples.

33 unique observed ARM PCs are inside the verified XY1.5 text segment. Their
presence, not estimated frequency, is supplied to the analyzer's existing
observed-execution input. It follows reachable control flow and feeds the same
canonical lifter/emitter used for the original build. No guessed instructions,
no hand-written game routines, no altered guest clock or emulation speed.

Main analysis: functions22535->22554; text bytes reached4110936->4113724.
71 additional registered block entry points were generated. Of the33 sampled
PCs,0 were originally registered and14 now are direct entries; interior
instructions are normally entered through their native block, so this count
must not be called33 new native entry points or100% coverage. All33 sampled main PCs have emitted inline-native instruction groups.
The other sampled
module PCs remain outside this main-only change. Module libraries are retained.

The update1.5 main library is rebuilt. Its manifest must keep the verified code
hash7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74.
The base game's native library is unchanged. Detailed per-instruction fallback
sampling is now opt-in at build time with PX_NATIVE_FALLBACK_PROFILE (defaultOFF),
removing the diagnostic sampling cost from the performance build. Existing
aggregate native/interpreted/decode counters remain available.

## Reproduction and validation

Private inputs/generated C++ stay outside the source checkout. The existing
bridge/tools/px-lift-update tool accepts an optional final argument containing
an observed main table. The existing analyzer validates/parses that table;
its address grammar records ISA and presence. The supplied table was filtered
against the verified text segment before use. Counts are1, never the approximate
heavy-hitter estimates. Tool invocation and evidence are retained privately.

A synthetic fixture generates libraries before and after adding an observed
function, compiles and loads both, then compares native/interpreted loops across
budgets0..100, registers/flags/stores, SVC and write faults.
Synthetic instruction-set/direct-call regression and cached/uncached CPU-state,
memory, budget, SVC and fault regressions pass. Party/menu lifecycle and metadata
reader regressions are retained. Package validation checks certificate, version,
ARM64,16KiB alignment, embedded source, imports and exactly which libraries change.
This is an actual AOT-coverage change, not only a cache or diagnostic build.
Phone correctness and FPS still require confirmation. It is not a100%-native
claim, and interpreter handlers used inside AOT functions are not counted by
engine fallback counters.

Install over Test4, keep update1.5 enabled and the same resolution/driver/texture
pack, run through the slow logo to Press Start and export the log. Compare speed,
fallback rate and visible correctness; do not infer speedup from function count.
