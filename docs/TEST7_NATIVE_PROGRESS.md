# Private Test7: wider AOT specialization and honest execution counters

Goal: progressively replace generic guest CPU execution with specialized native
code while maintaining100% normal guest speed where the phone can sustain it.
This is a target, not a promised measurement or a100%-native claim. CIA preparation
already persists game.cxi inside app storage. Native libraries are compiled before
APK delivery; the phone does not compile the whole game when importing or playing.
Shader preparation and compatible kernel/graphics services remain separate work.
No GitHub writes before phone testing and explicit user approval.

## Change

16 additional media/saturating/DSP families use constant-operand ExecuteHandled
calls instead of generic ExecuteDecoded: USAT, PKHBT, SMLAD, UXTB16, REV, USAT16,
REV16, PKHTB, QADD, QDADD, SSAT16, QSUB, SMUSD, SMUAD, UXTAB16 and SSAT. Arithmetic
semantics reuse existing ARM handlers; PC/unpredictable-operand guards remain.
The emitter names the new operation enums explicitly. The verified update1.5
main and supplemental Title library are regenerated and rebuilt, so main also
receives the prior ParallelArithmetic/SEL specialization that Test6 applied only
to Title. Original module/base/driver binaries remain retained.

Static survey of the analyzed main code:1035075 distinct instructions, generic
sites2318->810 (1508 removed).807 remaining sites are SVC service calls;3 other
sites remain generic. These are static sites among analyzed functions, not a
runtime percentage or proof that every code path was discovered. Uncovered
entries, runtime bailouts, budget tails and code outside analyzed functions can
still invoke the engine interpreter. Byte reachability also is not native CPU
coverage. The survey is available through px-lift-update --survey.

## Counters

Engine adds separate lifetime retirement counters without changing the legacy
three-field Statistics ABI or runtime::Context layout. Each Run adds actual
retired guest instructions, based on its existing exact budget accounting.
Fallback attempts that fault/stop without retirement are subtracted; SVC retires
one instruction. No extra counter update is added to the successful interpreter
hot path beyond existing interpreted counting. Logs report Guest CPU retired:
total, aot_managed and engine_fallback. AOT-managed includes semantic handlers
called from generated code: it must not be called pure-native percent or CPU-time
share. Entries/module_entries still count calls, not instructions.

## Validation and reproduction

Synthetic tests cover1024 states across all16 families, with diverse encodings,
passing/failing conditions and full GPR/CPSR/GE/Q comparison against the existing
interpreter. Sampling limits deliberately allow rare REV/REV16 encodings; random
instructions are generated from synthetic ARM encoding spaces only. No ROM bytes
enter tests. A compiled before/after fixture includes five encodings per new
family, ParallelArithmetic and SEL; it compares registers/flags/stores, budgets
0..160, SVC and write faults. Lifetime counters also cover successive slices,
zero budget and non-retiring fetch faults. Existing lane/SEL, decoder-cache,
menu cancellation and metadata checks remain applicable. Phone validation remains
required; host tests do not establish FPS, thermals or GPU-driver behavior.

Private update15 sources contain the rebuilt main and the original retained
module sources; private intro15 sources contain the rebuilt Title and observed
tables. Generated game code stays outside the source checkout. Tools support
observed main/module tables as before. Detailed fallback sampling stays off.

Install over Test6 without reimporting the CIA, keep update1.5 and the same
resolution/driver/texture pack, run through the slow logo to Press Start and
export the session log. Compare actual speed_percent and instruction deltas.
Normal30-FPS scenes should not be treated as needing60FPS when speed is100%.
