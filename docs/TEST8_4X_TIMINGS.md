# Private Test8: diagnose the remaining4x slowdown

Goal remains100% normal guest speed at4x, not a guarantee or a100%-native claim.
No GitHub writes before phone tests and explicit user approval.

The supplied Test7 sessions use the same phone/driver and keep the PNG pack
active. The1x session runs more of the slow phase near30FPS/~100% but later
falls near23-29FPS/~76-96%. The prior4x session falls as low as18-25FPS/~60-83%.
These are separate runs with unmeasured thermal/scheduler/scene timing effects;
resolution contributes to the observed difference but cannot be assigned a
precise causal speedup. Texture-pack cost has not been independently isolated.

Surface::Download retains its required scheduler.Finish and mapped-buffer Commit
before CPU consumption. On scaled surfaces it may first schedule a scale-down.
Removing the wait would risk stale data. Test8 only measures this path and the
guest CPU engine; it does not change resolution, clocks, frame skipping or AOT.
The intro does not normally run the asynchronous battle/popup interface probe,
so that probe is not implicated by the inspected source path.

PX_PROFILE_HOST_TIMINGS defaultsOFF; Test8 builds withON on exactly three source
files. Counters allocate no event buffers and record no per-instruction events.
GuestScope surrounds engine.Run, excluding subsequent kernel SVC work. Readback
scope surrounds Finish plus buffer Commit, including completion of any earlier
queued work it waits for. It is not the isolated GPU cost of one transfer.

Host wall scopes logs report an interval and guest_runs/guest_run_ms, readbacks,
gpu_readback_sync_ms, readback_inside_guest_ms, staged_bytes, scaled_readbacks.
Same-thread nested readback time is part of guest_run_ms: do not add overlapping
scopes or call these CPU/GPU utilization percentages. Wall duration also includes
OS descheduling. Staged bytes are the staging allocation count, not bus traffic.
Other costs (DSP, rendering, driver queue work outside these scopes) remain
unclassified. Individual atomic counters are exact over lifetime, but an event
can straddle log windows. Paused residual counters are discarded on resume.

Synthetic tests verify nested attribution/TLS, reset windows and exact totals
while four threads concurrently record and a consumer drains. Native compilation,
source/ABI/package checks and menu regressions remain required. Vulkan behavior
and phone timings are unverified; Test8 is a diagnostic build, not an FPS fix.
Ten existing game/driver libraries stay byte-identical to Test7; only the host
and build labels/embedded source change. Prior main/Title specialization remains.

Phone: install over Test7 without CIA reimport, select4x again (the saved setting
may still be1x), keep the existing driver/texture pack, run through the slow logo
to Press Start and export the log. The added scopes guide the next optimization;
if neither scope explains the delay, profile the remaining host/DSP/render work.
