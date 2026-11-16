# Private battle metadata experiment — 0.1.2 Test 1

Do not publish or push this work before gameplay testing and the maintainer's explicit authorization. The public release remains 0.1.1-beta.

The read-only battle layout is gated by the verified XY1.5 executable digest. Base-game addresses have not been validated and no memory reads use the update layout for that version. No game containers or generated game code are added to public sources.

The reader inspects all six party records. Party order is not interpreted as active order. After an explicit move selection, one unique PP decrease can associate a record with the actor of that action. Duplicate species are supported; ambiguous changes, invalid pointers, changed identities/moves, PP restoration, fainted records and menu/battle epoch changes discard evidence. Pending evidence expires after 200 reader samples. This is experimental correlation, NOT a validated active-battler pointer: forced/automatic switches, transformations, copied moves and double/triple battles still require investigation.

The overlay is opt-in and off by default. It can show catalog names and read PP on the move-selection screen only after association. Otherwise it preserves original game fields. Dynamic type rendering remains the game's own image; catalog types are not substituted. Name localization follows the app language, independently of the game language; Portuguese currently uses English move names, because the source has no official Portuguese set. No full native-data claim is made.

XY1.5 field provenance: samaBR85/Gen6CTRPFrameworkOverhauled commit16fc26e1000919c6e284f171d05fd25fcbfb9c0d, Sources/Helpers.cpp and Sources/PKHeX.cpp. The reference treats slot0 as active in one helper; that assumption is deliberately not adopted.

Offline catalog: PokeAPI/pokeapi data/v2/csv/pokemon_species_names.csv and move_names.csv, fetched 2026-11-13. Only localized names keyed by species/move IDs are retained. Unknown IDs keep the original rendered fields. No network requests happen during gameplay.

Phone procedure (supported update1.5, ordinary single battle): enable Pokémon e golpes — experimental in Display settings. Enter a battle with at least two Pokémon; use one regular move, then reopen Fight. Verify species, four move names and current/max PP. Open Bag and party, switch Pokémon, and verify the original fields remain until a new regular move establishes evidence. Repeat with duplicate species, a failed move and PP-depleted moves. Export a session log if any mismatch appears. Do not use this test as proof of correctness for forced switches or multi-battles.
