# Pokemon X update1.5 analysis

The supplied update has title0004000E00055D00, TMD version5232,
remaster version5 and a5169152-byte decompressed executable. Its SHA-256 is
7dc08ac5e2a87adabc31c304e1dfa3120a598ef072441983d09c96768cf67e74.
These facts were measured with the container/exheader/BLZ parsers and checked
against the existing loader. The original executable digest is different;
its lifted library must never be used for the update.

The analysis seeded from the update static module, module imports and data
pointers generated22535 main functions in275 shards, and44054 functions
across133 loadable modules in381 shards. Generated files and bytes stay outside
the source tree, under private workspace inputs.

The update exposed a direct-call eligibility bug: an address present only as an
ARM entry could be selected for a Thumb call, producing an undefined Thumb
function name. Eligibility now includes the instruction set, not just the
address. Unsupported mode combinations remain dispatcher/interpreter work.
bridge/tests/lift_mode_test.cpp reproduces this with synthetic instructions;
it failed before the fix and passes after. No ARM instruction semantics changed.

Experimental readonly battle fields use the XY1.5 references in
Gen6CTRPluginFrameworkOverhauled, commit16fc26e1000919c6e284f171d05fd25fcbfb9c0d,
Sources/Helpers.cpp and Sources/PKHeX.cpp: state081FB170, party table081FB284,
species+0C, maximum/current HP+0E/+10, move IDs+11C and current/maximum PP+11E/+11F
with14-byte move stride. These are not independently device-validated yet.
Only an unambiguous single-battler table with all fields validated can supply
native PP. Multi-Pokemon parties must not guess that slot0 is active; they keep
the real rendered fields. Names/types still come from live game rendering,
not a structured localized string reader. No writes or cheats are performed.
