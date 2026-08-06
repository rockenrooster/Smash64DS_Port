# BTT/BTP Arrow Pink Outline + Per-Character Inner Color (issue #4)

**Date:** 2026-04-29
**Issue:** [#4 Arrow texture in Break The Targets/Board The Platforms renders weird depending on character](https://github.com/JRickey/BattleShip/issues/4)
**Status:** Resolved.

## Symptom

On every BTT/BTP stage *except* Link's, the small directional-arrow sign at the
start of the stage rendered with a pink/magenta outline (instead of black) and
a per-character inner-fill color (Fox brownish, Yoshi green, Pikachu yellow,
Samus orange). Link's stage rendered correctly.

## Root cause — twofold

The arrow's CI4 image samples palette indices 0, 1, 2, AND 3. Each
character's BTT data file contains a 4-entry palette laid out at e.g.
`ExternDataBank120 off=0x598`:

```
29 4A A5 0D | 00 01 7B C1
pal[0]=0x294A | pal[1]=0xA50D | pal[2]=0x0001 (BLACK) | pal[3]=0x7BC1 (olive)
```

Each character's per-frame DL issues a `gsDPLoadTLUTCmd(LOADTILE, 2)` —
`high_index=2`, **count=3 entries**, byteCount=6 — to update palette[0..2].
The 4th entry (palette[3]) isn't named in the cmd. On real N64 hardware, the
4th entry is loaded *anyway* because the RDP's DRAM→TMEM transfer is aligned
to 8-byte (qword) boundaries — count=3's 6-byte transfer rounds up to 8 bytes
and pal[3] comes along for the ride. The developers relied on this
DMA-alignment quirk; Link's data uses count=4 explicitly, so it didn't
depend on the quirk and rendered correctly on the port.

Two compounding bugs in our port:

### Bug 1 — `chain_fixup_settimg` only fixes `count*2` rounded down to a u32 word

`port/bridge/lbreloc_byteswap.cpp:chain_fixup_settimg` walks the reloc chain at
file-load time and BSWAPs the bytes a CI palette LOADTLUT will sample. For
`count=3` it computed `tex_bytes = (3*2 + 3) & ~3 = 8`, fixing 2 words. That's
correct on its own. But the **runtime** fixup used by `GfxDpLoadTlut` had its
own `num_bytes &= ~3u` round-DOWN — so when the runtime path was reached for
the same TLUT (e.g., when chain didn't catch it), it processed only 4 bytes
(the first u32 word). Word 1 (palette[2..3]) stayed in pass1-BSWAP state, with
palette[2] reading as `0xC17B` = magenta-pink. **That's where the pink
outline came from** — palette[2] is the arrow's outline index.

Additionally, `chain_fixup_settimg` recorded its work only in the catch-all
`sStructU16Fixups` set. The runtime fixup's early-skip checked that set and
bailed out as "already fixed", so the second word never got a chance to be
fixed at runtime even with the round-up applied.

### Bug 2 — `GfxDpLoadTlut` did exact `memcpy(byteCount)` without DMA alignment

`libultraship/src/fast/interpreter.cpp:GfxDpLoadTlut` copied exactly
`byteCount = entryCount * 2` bytes into `palette_staging`. For Mario's
count=3 (6 bytes), that left `palette_staging[0][6..7]` (= pal[3]) holding
whatever the *previous* LOADTLUT call had put there — typically Mario's
character TLUT (`MarioModel+0x65c8`), which has `0x003D` (deep blue) at byte
6..7. **That's where the per-character inner color came from** — the arrow's
nibble-3 pixels rendered with the prior frame's TLUT entry, which differed
across characters because each character render had loaded its own model
palette before the arrow's count=3 fired.

## Fix

Three coordinated changes:

1. **`port/bridge/lbreloc_byteswap.cpp:chain_fixup_settimg`** — also record
   per-word entries in `sTexFixupWords` and the texture extent in
   `sTexFixupExtent[target]`. This lets the runtime fixup's early-skip
   correctly recognize chain-fixed targets, fall through to the per-word loop,
   and BSWAP only the words chain didn't cover.

2. **`port/bridge/lbreloc_byteswap.cpp:portRelocFixupTextureAtRuntime`** —
   round `num_bytes` UP to a 4-byte boundary instead of down. A count=3
   LOADTLUT's 6-byte `num_bytes` becomes 8, so word 1 (containing pal[2..3])
   gets fixed and the outline reads as black.

3. **`libultraship/src/fast/interpreter.cpp:GfxDpLoadTlut`** — round
   `byteCount` UP to an 8-byte boundary before the memcpy. This matches N64
   RDP DMA alignment: count=3's 6-byte transfer expands to 8 bytes, loading
   pal[3] from the source data instead of inheriting it from a prior load.
   The 4th entry is `0x7BC1` (olive) for every affected character's stage,
   and the chevron interior renders correctly across the board.

## Why "Link works" was the smoking-gun fingerprint

Link's BTT data uses `gsDPLoadTLUTCmd(LOADTILE, 3)` — count=4, byteCount=8 —
which is already DMA-aligned and explicitly names palette[3]. So Link's run
hit none of the bugs above:

- `chain_fixup_settimg` saw count=4 → tex_bytes=8 → both words fixed.
- The runtime fixup wasn't called for an under-sized `num_bytes`.
- `GfxDpLoadTlut`'s exact memcpy of 8 bytes covered all 4 entries.

This is why Link's stage was the only one rendering correctly, and why
matching N64 DMA semantics (Bug 2's fix) closes the loop for all 11 affected
characters at once.

## Reproduction

Pre-fix:
```
SSB64_GBI_TRACE=1 ./build/BattleShip
# 1P → BTT → Mario → arrow on screen
# Arrow renders with pink outline + character-specific inner.
```

Post-fix:
```
./build/BattleShip
# Same path; arrow renders with black outline + olive interior.
```

## Investigation method

The decisive step was a `SSB64_TLUT_DEBUG=1`-gated dumper (in
`libultraship/src/fast/interpreter.cpp`) that wrote each `ImportTextureCi4`'s
post-decode RGBA buffer as a TGA AND logged the actual 32-byte palette block
the GPU was sampling at draw time. The TGA showed the arrow visually (pink
outline + magenta interior); the palette dump showed it didn't match any
LOADTLUT log entry — a 6-byte count=3 from `ExternDataBank120+0x598` left the
last 26 bytes of palette block 0 holding stale Mario character bytes.

The diagnostic hooks were removed before commit; the file-scope ROM extractor
(`debug_tools/reloc_extract/reloc_extract.py extract <rom> 120 /tmp/file120.bin`)
and the `tex_fixup.log` env-var (`SSB64_TEX_FIXUP_LOG=1`) remain available for
future runs. The IDO+rabbitizer struct-layout audit confirmed the `Sprite`
struct's `nTLUT` field reads correctly on both targets — the cmd's `count=3`
is genuinely what the developers wrote in ROM, and the N64 DMA quirk is what
makes that work. The port had to learn the same quirk.

## Affected stages

All 11 BTT character stages other than Link, and likely the analogous BTP
stages (Fox BTP was confirmed in the issue report). The fix is global to the
LOADTLUT path, not stage-specific, so anywhere else in the game that relied
on N64 DMA-aligned partial palette loads benefits too.
