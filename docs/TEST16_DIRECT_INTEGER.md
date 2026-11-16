# Test16: direct native integer operations

Private APK 0.1.2-test16 / versionCode51 continues CPU integration from Test15.
No GitHub writes are authorized before phone testing and explicit permission.

## Milestone and scope

Replace handler dispatch for CLZ, REV and REV16 with direct native expressions
in the verified update1.5 main executable. Close the step with synthetic semantic
and compiled-code comparisons, unchanged registration/other generated code, and
an update-compatible ARM64 APK. Preserve the multiple-texture-pack manager and
existing menu-return behavior. Texture capture is not part of this step.

The lowerer now emits explicit unary IR operations, reads the decoded source
register and writes the destination without touching flags/memory. The emitter
calls the existing constexpr packing primitives directly; the native compiler
can lower them to ARM64 CLZ/REV/REV16. The evaluator has the same explicit IR
operations. Existing decoder, architectural primitives, unpredictable-PC guards,
conditions, memory, SVC handling and execution budgets are unchanged. No ARM
instruction decoder/interpreter implementation is modified.

This goes beyond the previous constant-operand ExecuteHandled specialization:
the affected generated sites no longer call an interpreter handler at all.
Other operation families and unlifted paths still can. Kernel services,
PICA-to-Vulkan translation and DSP compatibility remain in use. The APK is not
100% native, and no native-execution or FPS percentage follows from these counts.

## Integration facts

The retained update executable SHA-256 is
`7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74`.
The main output is regenerated from its existing function/region tables with
`px-lift-update --emit-main CODE BASE FUNCTIONS REGIONS OUTPUT`.
No new guessed entry, source function or executable byte coverage is claimed.

The source audit finds278 replaced handler occurrences at276 distinct addresses:
CLZ18/17, REV145/144, REV16115/115 (occurrences/distinct sites). Duplicate counts
reflect overlapping analyzed units. Registration/header bytes and all generated
code outside those instruction groups are unchanged. Primary entries226573 and
the retained historical continuation additions60 remain, merged226633. The
continuation output contains none of the affected sites and is retained from
Test15. Existing Title/base/module libraries are retained too.

Only `libpxmain15.so` changes. Eleven other native libraries, including the host,
remain byte-identical to Test15. Lifter identity/runtime ABI are still version4;
this compatible specialization adds no runtime fields or external imports.
The native host banner remains Test13/core48; exported APK header51 and frontend
Test16 identify this package. No binary banner patch is applied.

## Validation

Before the lowerer change, the new direct-lowering assertion fails because the
operations use ExecuteHandled. After the change,20480 seeded ARM/Thumb cases
cover aliases, single-bit/zero edges, conditional execution, all GPR/CPSR/GE/Q
and invalid-PC fallback. A compiled synthetic fixture adds20480 comparisons
against the interpreter, all15 ARM conditions and16 NZCV combinations, budgets
0..7, stores/faults and transition to an SVC. Its generated source has no
ExecuteHandled/ExecuteDecoded calls. A separately cross-compiled synthetic
ARM64 fixture must contain native CLZ, REV and REV16 instructions.

Run `python3 bridge/tests/run_direct_integer_tests.py BUILD_OF_BRIDGE_TOOLS`.
The existing16-family media regression remains applicable: REV/REV16 now expect
direct IR while other families retain their handlers. Existing observed-AOT,
menu lifecycle/cancellation, save, multi-mod and language regressions run too.
APK verification checks package/certificate continuity, ARM64/16KiB alignment,
imports/JNI, the one-library change and current embedded sources. Tests use only
synthetic inputs; private game-derived output stays outside the source tree.

Phone gameplay and speed are not established by these tests. Install overTest15
without CIA reimport, retain the same update/driver/resolution/packs, run through
the slow intro and export the session log. Performance differences need a
same-device comparison before any FPS gain is claimed.
