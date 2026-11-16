# Private 0.1.2 Test 2

No GitHub publication is authorized. Phone testing and explicit user approval
must precede any GitHub write, including branches, commits, PRs and releases.

## Closing the Pokemon party screen

The battle popup return previously depended on seeing an intermediate menu
through a brightness heuristic. Fast cancellation and flat/dark party screens
could leave the popup selected even after the game returned to battle commands.

The renderer now records departure from the learned battle-command signature
without that heuristic. Back also permits return detection without an intermediate
sample. Both paths still require the learned command signature on two observations;
Back inside nested menus does not close the popup prematurely. Completed actions
retain the existing module lifecycle return path.

The synthetic regression executes the production probe body and covers normal
and fast cancellation without switching, dark party screens, nested Back,
legacy color false positives and completed actions. Phone verification remains
required for the real title, including opening Pokemon from the overworld.

## Intro CPU fallback

A historical v23 device session showed millions of interpreted instructions
during the intro, roughly 20-36 FPS, then 60 FPS with much less interpretation.
This is evidence of CPU work in that session, not a diagnosis of the current
build or a guarantee that other devices have the same bottleneck.

The fallback engine now caches decoded instructions in 256 bounded direct-mapped
entries per engine. It fetches current bytes on every execution, including the
second Thumb halfword only when required. Code changes, remapping and fetch
faults remain observable. Execution uses the original StepDecoded implementation,
so PC-relative behavior, flags, instruction budgets and services remain unchanged.
Native game libraries and their ABI-facing structures are unchanged.

Differential synthetic ARM/Thumb tests compare cached and original execution,
including changing flags and code, page boundaries, faults, remapping, complete
CPU state, memory, engine budgets and SVC. A repeated two-instruction host loop
showed lower fallback time; this is not a gameplay benchmark or phone FPS result.
Session logs now include decode_cache_hits and decodes for comparison.

Test on the same phone/settings from launch to Press Start, then reopen and
cancel Pokemon both without switching and after a switch. Retain session logs
if the intro remains slow or the screen fails to return. Experimental metadata
remains optional and unchanged from Test 1.
