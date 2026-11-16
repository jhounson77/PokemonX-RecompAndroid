# Third-party notices

Origins and attributions for code and documentation Hexlith takes from elsewhere.

## Android integration

This distribution includes and links modified upstream code. Integration changes are offered under GPL-3.0-or-later; each upstream component retains its copyright notices and license terms. The GPL text at the repository root does not relicense game-derived inputs, trademarks or UI artwork.

| Component | Origin / license | Role |
| --- | --- | --- |
| Hexlith | https://github.com/KoyFC/Hexlith / GPL-3.0-or-later | Native recompilation runtime and tooling |
| Azahar | https://github.com/azahar-emu/azahar / GPL-2.0-or-later | Kernel, services, memory, Vulkan and DSP |
| libarchive 3.8.11 | https://github.com/libarchive/libarchive / BSD-style, see COPYING | Local RAR extraction |
| Vendored dependencies | Per-directory license and revision files | Graphics, audio, compression, driver support and build tools |

Upstream revisions and integration boundaries are documented in docs/ARCHITECTURE.md. The launcher background was supplied for this project and the app icon was adapted with generative assistance; Pokémon artwork and marks are not represented as GPL-licensed original game assets. Game ROMs, generated game C++, private keys and signing files are not part of this source repository.

The following original Hexlith notices are retained for provenance. Their statements about Hexlith alone do not describe the larger Android integration.

## Original Hexlith notices

## Linked dependencies

Libraries Hexlith links against rather than vendors. No source from them is present in this repository, and each comes from the platform's package manager.

| Library | Licence | Used for |
| --- | --- | --- |
| [OpenSSL](https://www.openssl.org/) libcrypto | `Apache-2.0` | SHA-256, through the EVP interface. See `docs/adr/0009-sha256-from-openssl.md` |
| [GoogleTest](https://github.com/google/googletest) | `BSD-3-Clause` | The unit tests, which are not part of any distributed binary |

Apache-2.0 and BSD-3-Clause both permit a combined work to be conveyed under GPL-3.0-or-later, so neither restricts Hexlith's own licensing.

## Reference material

Hexlith's format documentation is written against these sources. Reading documentation to learn a file layout creates no licensing obligation, but the sources are recorded because the project rule is that no format or address is asserted without one.

| Source | Used for |
| --- | --- |
| [3dbrew](https://www.3dbrew.org/) | The NCSD, NCCH, extended header, ExeFS, RomFS and CRO0 formats |
| [Azahar](https://github.com/azahar-emu/azahar), `GPL-2.0-or-later` | The CRO header field layout from `src/core/hle/service/ldr_ro/cro_helper.h`, and the RomFS level 3 offset from `src/core/file_sys/ncch_container.cpp` |
| [TriAevum](https://github.com/coccofresco/TriAevum), `GPL-3.0-or-later` | Architecture reference. Audited in `docs/triaevum-audit.md` |

## Format for an entry

When code is adopted, it gets an entry here before the commit that adopts it, and the files keep their original copyright headers and SPDX identifiers.

```
### <Upstream project>

Upstream:  <URL>
Licence:   <SPDX identifier>
Copyright: <holders as stated upstream>
Files:     <paths in this repository>
Taken at:  <upstream commit or release>
Changes:   <what was modified, and why>
```

Donor code is vendored under a `third_party/` directory with its own licence file rather than being mixed into Hexlith's sources, so that the boundary stays visible in the file tree and not only in a notice.

## Adopted

### Azahar

Upstream:  https://github.com/azahar-emu/azahar
Licence:   GPL-2.0-or-later
Copyright: Citra Emulator Project / Azahar Emulator Project
Files:     `third_party/azahar/golden-trace.patch`
Taken at:  219d743a1, the version of the Flatpak build in use
Changes:   A patch of Hexlith's own against five Azahar files, whose context lines are Azahar's code. It makes Azahar log the events of a golden trace; `third_party/azahar/README.md` says what it changes.

## Not covered

Nothing derived from the game is adopted from any project under any licence. See `LICENSE_SCOPE.md`.

Launcher background (android/assets/launcher-background.jpg): user-supplied
Pokémon X/Y Xerneas illustration, used as requested. Original artwork and
Pokémon marks remain the property of their respective owners; no ownership
of the artwork is claimed by this project.

## libarchive3.8.11

RAR extraction for the local update installer uses libarchive, tagv3.8.11
(commitbb39e54db6cfa582c345535de7b3f3ee3b6e9cd1).
The BSD license and upstream notices are retained in
azahar/externals/libarchive/COPYING and the bundled source archive.
No archive pathname is used as an output path; only the CIA payload is extracted.

## Experimental battle name catalog

`android/assets/battle-catalog.tsv` maps live species/move IDs to localized names from PokeAPI/pokeapi, `data/v2/csv/pokemon_species_names.csv` and `move_names.csv`, fetched 2026-11-13. This is a local lookup table, not an online service. Unknown IDs keep original game-rendered fields. Portuguese uses English move names. Dynamic move types and forms are not inferred from this name catalog.
