# JP build: Torch SSB64:RELOC factory hardcoded the US reloc-table address

**Date:** 2026-05-18
**Area:** `torch/` submodule — `src/factories/ssb64/RelocFactory.{h,cpp}`
(branch `ssb64`), JP asset extraction
**Symptom:** `cmake --build … --target ExtractAssets` against
`baserom.jp.z64` aborted almost immediately:

```
> Country: [jp]   Hash: 4b71f0e0…   Assets: …/yamls/jp
- [SSB64:RELOC] Processing reloc_animations/FTMarioAnimWait at 0x53AFC0
terminate called after throwing an instance of 'std::runtime_error'
  what():  ROM too small to read file data for file 474
```

(File 474 is JP's first *uncompressed* file — 0–473 are the 474 VPK0
files — so the failure is right where the table walk first needs a
correct data base.)

## Root cause

`config.yml` SHA1 routing and the Phase-2 Python yaml generator were both
made version-aware, but the Torch C++ `SSB64:RELOC` factory reads the
reloc **table directly from the ROM** (the YAML `offset`/`size` are only
documentation). `RelocFactory.h` hardcoded the **US** layout:

```cpp
RELOC_TABLE_ROM_ADDR = 0x001AC870;  // US (NALE)
RELOC_FILE_COUNT     = 2132;        // US
RELOC_DATA_START     = ROM_ADDR + (FILE_COUNT+1)*12;
```

The JP ROM's reloc table is at `0x001ACAF0` with `2107` files
(`smashbrothers.jp.yaml`, `relocFileDescriptions.jp.txt`). Parsing the JP
ROM with the US base yields garbage table entries and an out-of-ROM data
address → the runtime_error.

## Fix

Replaced the hardcoded constants with a `RelocLayout` resolved per ROM,
keyed on the N64 header country byte at `0x3E` ('E' = NALE/US,
'J' = NALJ/JP). The factory already receives the full normalised ROM
buffer, so the layout is derived locally — no Companion/cartridge
plumbing. All six `RELOC_*` use-sites in `RelocFactory.cpp` now use the
resolved `L.tableRomAddr / fileCount / entrySize / dataStart`.

- **US:** country 'E' → identical constants → byte-identical extraction.
- **JP:** country 'J' → `0x001ACAF0` / 2107 → extraction completes
  cleanly; `BattleShip.o2r` contains 2108 entries (2107 reloc files +
  version), distribution matching `yamls/jp`.

## Notes

This change lives in the `torch` submodule (branch `ssb64`,
`JRickey/Torch`). The outer-tree submodule pointer must be bumped when
this lands (see CLAUDE.md submodule workflow). The reloc table addresses
trace to the upstream decomp splat configs (`smashbrothers.<v>.yaml`),
the same authoritative source used for the Python generator in Phase 2.
