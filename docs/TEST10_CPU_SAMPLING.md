# Test10 CPU execution sampling

Private 0.1.2-test10/code45. This is a diagnostic build, not a confirmed FPS improvement or new native coverage.

## Input evidence

Test9/code44 phone log: Samsung SM-S931B, scale4x, same custom Freedreno driver and asynchronous PNG pack. Forty-three wall windows were captured. Last ten windows averaged18.56FPS /62.10percent speed and954.39ms guest-run wall per approximately1.002s. Readbacks were absent in the sustained slow portion. Pipeline lookup summed3.288ms across24256calls in the whole capture, with no recorded pipeline create/build/wait events after warmup. A single display transfer took92.16ms early in startup; this does not explain sustained late slowdown. Host command timing is not hardware GPU utilization. These data justify subdividing Engine.Run, not removing GPU synchronization. No thermal cause is established by this log.

## Measurement

PX_PROFILE_EXECUTION=ON (defaultOFF) samples Engine.Run with a guest-independent nonzero xorshift sequence, targeting1/256runs. Unsampled work uses a separate untimed core function, with the lookup helper explicitly inlined to retain the previous hot-path behavior. A desktop synthetic comparison caught an inlining regression during development; it was corrected before packaging. Alternating OFF/ON host rounds are retained with validation; they are not phone performance evidence. Selected runs measure disjoint lookup, aot_managed and engine_fallback wall scopes, call counts and retired guest instructions. Sampling does not alter guest state or budgets. Selection occurs per run, not per instruction; long runs are not truncated. Accumulators are fixed-size and sampled operation data are published only when the selected run returns. Counters reset after shader preparation and on resume, alongside existing host diagnostics. PX_PROFILE_HOST_TIMINGS=ON is required for log output.

CPU sampled runs reports all runs, selected runs and selected-run wall_ms. CPU sampled stage reports only work in selected runs. Do not compare its raw milliseconds directly against full-interval Host wall scopes, multiply a single window into an exact CPU percentage, or treat instruction ratios as time ratios. AOT-managed includes semantic handlers inside generated code; it does not mean every instruction was lowered natively. Fallback includes live fetch, decode-cache work and execution. Lookup includes main/module resolution. Each category includes OS descheduling; timing overhead perturbs selected work. Atomic exchanges and asynchronous event completion can straddle reporting windows. Total sampled-run time also contains loop, counter and timing overhead. Use several windows, selection counts and retired counts to judge representativeness; an empty category with little sampling is inconclusive.

No frame skipping, scale reduction, clock adjustment, required-wait removal, guest compiler or game data change is introduced. All ten game/driver libraries are reused byte-identically fromTest9. The host is rebuilt from the offline source tree. Source/data ABI exposed to generated libraries is preserved.

## Validation and phone test

The new unit/integration test checks reset, disabled samples, concurrent publication and sample distribution. It compares actual Engine.Run against the interpreter for mixed main AOT/fallback, loaded module, digest mismatch, fetch fault, limits0..6 and SVC exits, with sampling enabled and disabled. CPU register/flag states, stop reasons/details and executed counts must match. Existing decode-cache and screen/menu regressions and signed package/JNI/alignment/source checks remain required.

Install overTest9 without CIA reimport. Keep4x, same driver and PNG pack. Capture startup through the slow logo/PressStart for about45seconds and export the log. The next optimization should target the measured lookup, AOT-managed or engine-fallback cost. No sustained100percent-speed guarantee follows from desktop tests. No GitHub writes before phone tests and explicit authorization.
