# P2-2p8 Phase 0: shipping-configuration four-fighter heap census (2026-09-23)

The architecture doc (A7) listed the shipping build's four-fighter heap as never
measured. This measures it: the shipping shell configuration
(`smash64ds-p2-shell-hwtri`, the published `smash64ds` flags plus the unattended
menu walk), seeded with the heaviest four-kind roster a player can reach
(`NDS_P2_SHELL_ARGMAX_ROSTER=1`: Captain human, Link / Pikachu / Kirby CPUs,
level 3, Dream Land), built into its own directory
(`builds/build-p2p8-shell-census`), probed with
`scripts/probe-shell-four-kind.ps1` on runner slot 7.

## Result: the match never starts

The battle scene halts in `ndsSyMallocOverflowHalt` while loading the fourth
fighter:

| | value |
|---|---:|
| taskman arena chosen at boot | **1,138,432 B** |
| free at the first fighter's creation | 178,784 B |
| free after Captain / Link / Pikachu | 99,584 / 28,160 / 3,676 B |
| failing request | 35,272 B (Kirby's compact battle pack, `ndsRelocPreviewFighterLoadBegin` <- `ftManagerSetupFilesAllKind(8)` <- `scVSBattleStartBattle`) |
| headroom at the halt | 3,676 B |

The deficit is at least ~31.6 KB to finish loading Kirby, plus Kirby's own
creation (~70 KB, by the other three fighters), plus the 25,600 B GObj floor:
**roughly 130 KB short before the match runs a single frame.** A player who picks
these four in VS mode hits a silent halt today. (`probe-lowwater.txt`)

The only earlier shell four-kind probes (2026-08-25, arena 1,658,880 B; later
1,273,856 B) used a lighter roster, before the 1P game, the remaining fighters
and all nine stages were linked into the shipping image.

## Where the arena goes (battle scene, allocations >= 2 KB; `probe-bigalloc.txt`)

| owner | bytes | note |
|---|---:|---|
| `scVSBattleSetupFiles` | 208,672 | common battle files |
| `mpCollisionInitGroundData` | 202,816 | Dream Land's whole ground file (`mpcollision.c:3963-3975`): gameplay reads its map geometry; the DS renderer draws the stage from native blobs |
| `ndsRelocPreviewFighterLoadBegin` | 121,076 | compact battle packs, 3 of 4 fighters (27,300 / 33,520 / 24,984; Kirby's 35,272 failed) |
| `ndsRendererNativeEnsureOwnerImage` | 108,284 | native fighter owner images (7) -- Phase 1's generated lists replace these |
| `ndsBaseEFManagerInitEffects` | 94,704 | effect pools |
| `itManagerInitItems` | 82,976 | item files |
| unattributed (inlined callers) | 73,808 | 5 allocations |
| `ndsRelocAssetAllocSize` | 72,528 | fighter file trees |
| `ndsBaseFTManagerAllocFighter` | 47,296 | fighter structs |
| `ndsAObjEvent32ConfigureNormalizedCapacity` | 23,552 | |
| other | 45,000 | objman, shield poses, pose handles, map masks |

Before the first fighter, stage + common + items already hold ~546 KB of the
1.14 MB arena.

## Static image (`static-shell.txt`, `static-fourcpu.txt`)

| | shipping shell | four-CPU lab (Phase 0 gate ROM) |
|---|---:|---:|
| `.main` (code + rodata) | 1,568,508 | 1,392,036 |
| `.main.rw` | 228,612 | 202,212 |
| `.main.bss` | 904,560 | 917,136 |
| main RAM image total | 2,713,180 | 2,522,452 |
| largest BSS | FGM cache 237,568; `gSYFramebufferSets` 147,840; texture scratch 32,768; owner materials 28,800 | same set |

The shipping image carries ~190 KB more than the four-CPU gate ROM, almost all
of it code (menus, 1P game, all fighters and stages) that a battle does not run.
**The four-CPU gate ROM therefore measures a machine with ~0.2-0.5 MB more
arena than the one that ships** (its arena was 1,355,520 B in Phase 0).

## What this means for the plan

- "Any four fighters" fails on memory before it fails on time. The RAM work of
  Phase 3 (A7 overlays, A8 ARM7 audio, A2 admission) is a correctness
  prerequisite, not only a residency enabler: the shipping battle needs ~130 KB
  just to load this roster, and the D6 motion bank (up to ~599 KB for the worst
  motion roster, MF experiment) comes on top.
- Candidate sources, each to be measured when its phase lands: a battle-time
  overlay for menu / 1P code (up to the ~190 KB the shell adds, more if the
  battle's own cold code moves too); the ARM9 FGM cache (237,568 B BSS) once the
  ARM7 owns cue fill (D7); production / packet BSS (~98 KB) and owner images
  (~108 KB here) at Phase 1's deletion step; stage replay buffers (~73 KB) at
  Phase 2's; the ground file's render-only parts.
- Every later phase report carries the shipping arena and this roster's load
  margin, not only the four-CPU gate ROM's heap.

## Reproduce

```
make TARGET=smash64ds-p2-shell-hwtri BUILD=build-p2p8-shell-census NDS_P2_SHELL_ARGMAX_ROSTER=1
pwsh scripts/probe-shell-four-kind.ps1 -Build build-p2p8-shell-census -Target smash64ds-p2-shell-hwtri -BattleFrames 3400 -RunnerSlot 7 -Artifact probe-lowwater.txt
pwsh scripts/probe-shell-four-kind.ps1 -Build build-p2p8-shell-census -Target smash64ds-p2-shell-hwtri -BattleFrames 2 -TraceMallocAtLeast 2048 -RunnerSlot 7 -Artifact probe-bigalloc.txt
python ram_census.py <elf> 40
```
