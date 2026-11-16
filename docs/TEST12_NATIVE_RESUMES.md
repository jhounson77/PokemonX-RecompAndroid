# Test12 native continuation points in Title

Private0.1.2-test12/code47. This is an AOT continuation change, not another operand-cache optimization. Android FPS and gameplay improvements remain unconfirmed.

## Evidence and scope

Test11's comparable-duration phone window averaged19.62FPS/65.64percent speed versus19.47/65.06 in Test10; the difference did not establish a consistent gain. The recent logs contain aggregate execution timings, not current per-address rankings. Earlier code39 PC samples establish address presence only: their Space-Saving error bounds were too large to rank individual sites reliably.

Those earlier samples contain23 ARM offsets in DllTitle. The retained Test11 source has native instruction groups for these positions, but only1 of the23 is a registered entry. Interior positions can therefore execute through the engine interpreter until another block entry is reached. The supplied module was recovered from the retained analysis CXI; its actual code-segment SHA256 is212346daff6b71cf6077d8223c232908fdeba49eac7a832b2bdac356bc9df073, matching the existing native module and loader guard. This does not establish how hot each address remains in the latest run.

## Change

An optional FlowContext.resume_points input splits already reached instructions into basic blocks before assembly. It neither discovers new paths nor decodes arbitrary requested addresses. Duplicate, unreachable and non-instruction-start requests have no effect. The standard emitter then supplies its existing budget checks, bailout refunds and entry registrations for those blocks. ModuleSource exposes the input; px-lift-update --emit-module accepts an optional final file containing numeric continuation offsets. Empty input preserves default behavior byte-for-byte. Runtime Context, EntryTable, module ABI and lifter manifest format remain unchanged; lifter4 is retained for compatibility with the original libraries.

For the verified Title module,23/23 observed offsets are now direct registered entries, versus1/23 before. Unique entries3647->3669, blocks3663->3685; functions558 and decoded instruction count21895 remain unchanged. All3647 original entries and the identical manifest/digest are retained. These are22 new continuation entries, not22 new functions, new executable byte coverage,100percent-native execution or a measured FPS percentage. Short budgets and runtime bailouts can still invoke fallback.

Only the host and supplemental libpxintro15.so change. The other9 packaged libraries, including the main executable and original133-module library, remain byte-identical toTest11. Normal guest time, resolution4x, frame production and required GPU synchronization are preserved. The supplement remains version/hash gated and has the existing first-offer priority.

For the next iteration, existing CPU sampled runs also feed the bounded fallback-PC sketch, with its existing randomized512..1535 stride. The unsampled RunCore<false> path compiles this work out. Log site counts/errors describe sampled presence, not CPU shares, exact invocation counts or an unconditional frequency ranking. No extra guest-memory reads are added by this PC sampling. The separate legacy full fallback-profile option remainsOFF.

## Validation

10100 comparisons of emitted before/after libraries against the interpreter cover ARM/Thumb, main and relocated modules, interior entry, instruction budgets0..100, flags/registers/stores, direct call/return, SVC, write faults and a relocated instruction patched in memory. All old entries remain, invalid/unreachable resume requests and a Thumb BL second halfword are ignored. A targeted fixture executes3 instructions through fallback before,0 after, with identical final state and retired count. This is synthetic evidence of a native continuation change, not phone FPS evidence.

Regenerating Title with no resume input produces every source, registry and manifest byte-identically to the retained Test11 source. Real-source audits verify the23 entries,22 additions and unchanged digest. Sampling ON/OFF integration, menu cancellation/lifecycle, shader preparation and signed package/import/JNI/16KiB checks remain required. Private generated game C++ stays outside the public source bundle and is retained in the private checkpoint.

Install overTest11 without reimporting CIA. Keep4x, the same Freedreno driver and PNG textures. Run the slow logo/PressStart for about60seconds and export the log. Compare speed and fallback counts across similar scenes, allowing for sampling, device conditions and the added block guards. No guaranteed100percent-speed claim and no GitHub writes before phone tests and explicit authorization.
