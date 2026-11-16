# Test9 graphics stage diagnostics

Private Android version 0.1.2-test9, code44. Test8 showed steady guest Engine.Run wall cost and occasional GSP command-processing spikes. Surface.Download waits were absent in the slow intervals, with only one readback in the session. That readback hypothesis was not confirmed.

Test9 adds fixed-size atomic counters for GSP DMA, submission, fill, display transfer, texture copy, cache flush and unknown commands. Pipeline lookup, TryBuild, worker wait, shader wait, cache probe and creation API calls report calls, wall_ms and max_ms once per reporting window. Scopes overlap; asynchronous work and counter exchanges can cross window boundaries. OS descheduling is included. Do not sum nested categories or interpret these measurements as physical CPU/GPU utilization. Pipeline creation API calls can use driver caches and are not proof of shader recompilation. Rows with any nonzero field are retained, including boundary-skewed durations. Startup and resume reset counters.

PX_PROFILE_HOST_TIMINGS remains default OFF; this Android diagnostic enables it for six source files. No resolution, frame skipping, guest clock, asynchronous shader policy, required wait, or native game execution changes were introduced. All ten game/driver libraries are reused byte-identically from Test8. There is no new AOT coverage or confirmed FPS gain.

Validation: concurrent counter/reset and command mapping tests; production pipeline path equivalence with instrumentation ON/OFF; shader startup completion/cancellation; party cancellation and battle menu lifecycle; signed ARM64 package, certificate continuity, 16KiB alignment, native imports/JNI and embedded source checks. Phone gameplay remains required.

Install over Test8 without reimporting CIA. Keep 4x, the same driver and PNG textures. Play through the slow logo to Press Start and export the log. Compare command categories and pipeline waits to select the next optimization. No GitHub writes are authorized.
