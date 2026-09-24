# P2-2p8 Phase 3 (A7 memory): compact IFCommonGameStatus (2026-09-23)

The shipping heap census (`../2026-09-23_p2-2p8-shipping-heap-census/`) found the
heaviest four-kind roster ~130 KB short at battle load, and Phase 1 slice 3 found
the first 1P battle ~15-24 KB short. The largest single resident file in every
battle is `IFCommonGameStatus` (152,288 B): the GO / TIME UP / GAME SET letters,
the lamps, rod and frame of the start sequence. After load the DS reads only its
Sprite/Bitmap headers and the lamp/rod/frame pixels -- the native OAM HUD bakes
the letters into OBJ VRAM -- so the letters' pixels are dead weight all match.

## What changed (build flag `NDS_IF_GAMESTATUS_COMPACT`, default 0 in this commit)

| where | what |
|---|---|
| `src/port/reloc_backend_assets.c` | `ndsRelocLoadIfGameStatusCompact`: the file is loaded and finalized in free arena space at the **top** of the heap (nothing owns it; nothing allocates meanwhile); the internal fixup walk records every slot it patches; the GO bank bakes as before; both end messages are pre-baked; then the file is copied **without the twelve letters' pixel payloads** into an ordinary allocation, every internal pointer re-seated from the recorded slots (pointers into dropped payloads become NULL: 39, all letter `Bitmap.buf`); `ndsRelocGetFileData` maps a source offset through the span table (loaded-file mark `0xe7`); `lbRelocGetAllocSize` sizes the entry as a placeholder. Any step that cannot complete moves the whole file down instead (never a partial image). |
| `src/nds/nds_ifcommon_oam.c` | the letter bake can target a RAM image of an OBJ bank (`ndsIFCommonBakeDirectAssetTo`; the VRAM bake is unchanged); `ndsIFCommonNativeOamBakeEndVariants` bakes TIME UP and GAME SET into run-length streams on the scene heap; the announcement expands the stream into the end bank instead of converting pixels; `ndsIFCommonNativeOamRebaseGameStatus` moves the retained Sprite/Bitmap pointers into the image. |
| `Makefile` | `NDS_IF_GAMESTATUS_COMPACT ?= 0` and its build-config define. |

## Results (four-CPU stress lab ROM, `smash64ds-p2-fourcpu-tickhud-hwtri`, same tree, flag 0 vs 1, own build dirs)

| | flag 0 (`ifc0`) | flag 1 (`ifc1`) |
|---|---:|---:|
| GameStatus resident | 152,288 | **21,056** (131,232 dropped) + 22,104 of baked end streams |
| free heap at the first battle frame | 69,052 | **140,256** |
| general-heap low-water, whole match to Results | 54,020 | **122,412** (+68,392) |
| taskman arena chosen at boot | 1,277,696 | 1,253,120 (see below) |
| OBJ VRAM at the first frame (GO bank and the rest, 64 KB) | -- | **byte-identical** (`obj-first.bin`) |
| OBJ end bank after TIME UP's bake (20,736 B) | converted from pixels | **byte-identical**, expanded from the baked stream (`obj-end.bin`; `decodes=1`) |
| announcement / Results frame | 1995 / 2043 | 1995 / 2043 |
| native failures at Results | 293 | 293 |
| compaction counters | -- | count 1, fail stage 0, 75 slots re-seated, 39 nulled |

The static image grew 4,424 B (code + span table), yet the boot arena came out
24,576 B smaller -- an arena-selection effect to look at separately; the
low-water gain above is net of it.

**Replay digest: IDENTICAL over 1,972 frames** (`digest-ifc0-vs-ifc1.json`;
sampler rows, runner slots 8 and 7). Same-ROM-config frame work, admission word 0:
WORK P50 1,706,624 -> 1,637,248, P95 4,415,808 -> 4,320,384, P99 4,963,392 ->
4,946,112; two-VBlank frames 112 -> 135 (cross-build numbers carry layout noise;
the P50 drop is larger than layout noise usually is -- one candidate is the
animation cache, which reserves only above a free-heap threshold; not yet
checked).

Not yet exercised: the GAME SET stream at runtime (this match ends on TIME UP;
it is baked by the same function that produced the byte-identical TIME UP bank),
the 1P battles, and the menu loop. The flag stays 0 until they are.

**GAME SET at runtime (2026-09-23, `tools/run-ifc-vram-gameset.ps1 -ForceGameSet`,
same two ROMs):** the announcement's `game_set` argument is poked to 1 at
`ndsIFCommonNativeOamPrepareAnnouncement` (a register poke; the stress match
never ends on stocks), so both arms bake and show GAME SET at frame 1995. The OBJ
end bank after the bake is **byte-identical** between flag 0 (pixel conversion)
and flag 1 (stream decode; `decodes=1` at Results), and differs from the TIME UP
bank, so the poke took effect (`ifc0-gameset/`, `ifc1-gameset/`, sha256
`6AF19173...` both). GO bank and texture A+B at the first frame identical too.
Native failures at Results read 569 in BOTH arms against 293 on the TIME UP
runs: the forced GAME SET presentation adds 276 in this lab target whether or
not the file is compacted (not investigated here).

## 1P campaign (shipping shell configuration, `smash64ds-p2-shell-freeplay-hwtri`)

Lab ROM in its own dir (`build-p2p8-ifc-1p`, flags `NDS_P2_MENU_WALK=1
NDS_FTR_LEAN_ADMIT_LAB=1 NDS_FTR_LEAN_ADMIT_DEFAULT=2 NDS_IF_GAMESTATUS_COMPACT=1`),
`scripts/menus/probe-p2-campaign.ps1 -TransitionProof` on runner slot 8
(`1p-campaign-probe.txt`, `1p-tally.png`):

- **The first 1P battle (vs Link) now loads and completes** -- without compaction
  it halted in `ndsSyMallocOverflowHalt` during its setup (Phase 1 slice 3, D3).
  In-battle heap low-water 70,832 B; Stage Clear tally reached (score 13,570).
- **Bank D in a 1P battle (slice 2c's open D4c):** taken once in the battle and
  returned at the stage transition; 0 refused BG3 writes, 0 missed exits.
- **Next blocker (pre-existing, P2-6's "next-stage Intro OOM"):** stage 2's intro
  (the Yoshi team, a four-fighter scene) requests Yoshi's full 144,640 B main file
  (`ftManagerSetupFilesMainKind` from `sc1PIntroSetupFighterFiles`) with 50,276 B
  free. The 1P intro loads full fighter main files where battles load compact
  packs.

## Reproduce

```
make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2p8-ifc0 NDS_IF_GAMESTATUS_COMPACT=0
make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2p8-ifc1 NDS_IF_GAMESTATUS_COMPACT=1
pwsh tools/run-ifc-vram.ps1 -Arm ifc0 -Build build-p2p8-ifc0 -RunnerSlot 8
pwsh tools/run-ifc-vram.ps1 -Arm ifc1 -Build build-p2p8-ifc1 -RunnerSlot 7
```

`tools/run-ifc-abort.ps1` (break on calico's exception entry) and
`tools/run-ifc-late-attach.ps1` found the one defect of bring-up: the slot
record was dropped before the copy read it.
