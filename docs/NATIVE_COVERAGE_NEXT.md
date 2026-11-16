# Native coverage work: private Test4

No GitHub writes are authorized before phone tests and explicit user approval.

The supplied code38 Samsung scale4 session still slows in the intro/logo.
Across intervals with over10M fallback instructions, decode misses average8.47%,
versus31.12% in the previous code37 log (different time coverage; not a controlled
FPS benchmark). The cache avoids more decoding but the engine still executes
roughly16-19M fallback instructions per reporting interval during the slow phase.
These counters do not establish a percent-native execution claim: native entries
are calls, while interpreted is an instruction count. They have different units.

The next target is native CPU coverage of the actual slow sites. It is not a
wholesale replacement of kernel/services/PICA-to-Vulkan translation. An AOT-bound
module can still use interpreter handlers, leave uncovered blocks, fail a memory
fast-path check or exhaust its block budget. The current log cannot distinguish
those locations. Increasing caches further does not solve missing native coverage.

Test4 adds a bounded sampled fallback-site sketch to the actual engine fallback
path. It records PC, instruction set and whether a lifted function returned or
no entry was found. It reads no guest code bytes and makes no extra guest memory
access. It uses varying512-1535-instruction strides and32 fixed Space-Saving
slots per engine. Every fifth performance report logs eight estimated heavy sites
per core, with error bounds and total samples. These are cumulative approximate
sample counts, not exact instruction counts or CPU-time percentages. Native-yield
includes both exhausted block budgets and native fast-path bailouts; it is not
proof of unsupported instructions. Helpers called inside AOT blocks are outside
this sketch. No unsupported opcode or game address has been guessed.

Synthetic tests cover bounded storage, instruction-set/cause separation, frequent
site retention and sampling rate. Differential engine tests retain register,
memory, fault, remapping, clock-budget and SVC behavior. The profiler adds some
CPU work; Test4 is for locating recompilation targets, not a promised speedup.

Phone procedure: install over Test3, keep settings, run through the slow logo
for at least10seconds, then export the log. The fallback-site lines can be mapped
to existing private main/module analysis and used to choose the next AOT change.
Private generated game code stays outside the source checkout. Any native lowering
change needs synthetic semantic tests and a same-device comparison. Do not call
the port100% native or promise elimination of all stalls before measured evidence.
