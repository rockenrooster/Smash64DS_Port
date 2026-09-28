# S1: the frame-79 entry crash was an event32 ledger overflow (2026-09-28)

## Symptom

In the full-roster lab (sweep ROM plus the Ness/Yoshi/Pikachu/Purin flags),
Ness/Yoshi/Pikachu/Purin crashed at frame 79 on Yoshi's Island, Saffron and
Sector Z. `gcParseDObjAnimJoint` read `dobj->anim_joint.event32 =
0xc035bfa0`, which is -2.84f. The first S1 note (2026-09-27) had the same
crash with DK/Samus/Link/Kirby on Sector Z.

## How it was found

- A lab joint witness in the fighter proc wrappers recorded each fighter's
  `joints[]` at every proc stage. All joints were valid right before the
  crash window. The window was the fourth fighter's entry on frame 79: its
  AppearR motion (Purin, status 228) was being bound.
- gdb reads of cached RAM and ARM9 WRAM return stale lines on the
  cache-accurate fork. Only values flushed by the ROM itself were trusted.
- The ledger counters at the fault: Yoshi's Island limit 3,328, high-water
  3,328, 3,445 commands requested, 19 normalize failures.

## Mechanism

Every event32 script is normalized in place (a bit permutation of each
command word), and the ledger records which words are already permuted.
Its size is fixed per stage when the scene starts (09-09 static census). When
a script no longer fits, `ndsAObjEvent32NormalizeScript` refuses it and
`gcAddDObjAnimJoint` skipped the bind. The joint then kept its previous
`event32` pointer. For a fighter, that pointer is into the `figatree_heap`
buffer the new motion had just overwritten, so the next parse read motion
data as a command pointer.

The entry motions are event32 clips, and their sizes vary by a factor of
three (commands, counted offline from the O2R files by
`count_ev32.py`, mirroring `ndsAObjEvent32PlanStream`):

| Kind | Commands | Kind | Commands |
|---|---|---|---|
| Pikachu | 758 | Mario / Luigi | 378 |
| Yoshi | 728 | Ness | 373 |
| Captain | 599 | Fox | 318 |
| Link | 557 | DK | 268 |
| Samus | 508 | Kirby | 214 |
| Purin | 498 | | |

## Measured high-water

Full-match `gNdsAObjEvent32NormalizedHighWater`, ledger forced to 5,120 or
8,000 with the lab word `gNdsLabAObjEvent32CapacityOverride` (arms `ev_*` in
`artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/`):

| Stage | Old bound | Ness/Yoshi/Pika/Purin | Pika/Yoshi/Capt/Link | DK/Samus/Link/Kirby | Mario/Fox/Samus/Capt |
|---|---|---|---|---|---|
| Castle | 1,536 | 3,069 | 3,620 | 2,131 | 2,516 |
| Sector Z | 2,560 | 3,622 | 4,069 | 2,752 | 3,216 |
| Jungle | 2,560 | 3,244 | 3,333 | 2,642 | 2,848 |
| Zebes | 4,608 | 5,085 (at frame 1920) | 5,199 | 4,572 | 4,919 |
| Hyrule | 1,536 | 2,221 | 2,310 | 1,619 | 1,825 |
| Yoshi's Island | 3,328 | 4,041 | 4,621 | 3,155 | 3,681 |
| Dream Land | 3,072 | 2,136 | 2,335 | 1,623 | 2,008 |
| Saffron | 2,560 | 3,406 | 3,887 | 2,427 | 2,793 |
| Mushroom | 3,072 | 3,161 | 3,711 | 2,300 | 2,865 |

The stress roster itself overflowed four stages' bounds (Castle, Sector Z,
Jungle, Hyrule). Heap low-water stayed at 41-75K with the forced ledgers.

## Fix

1. `ndsAObjEvent32RosterLimit`: a VS ledger is
   `max(stage bound, stage part + sum of the players' entry-clip commands +
   384)`. Stage part = the largest (high-water - roster clip sum) over the
   four rosters: Castle 978, Sector 1,427, Jungle 1,095, Zebes 3,116,
   Hyrule 72, Yoshi 1,979, Dream Land 205, Saffron 1,245, Mushroom 1,069.
   The gate (Dream Land, DK/Samus/Link/Kirby) keeps its 3,072: its roster
   term is 2,136.
2. A refused script no longer leaves the previous pointer behind:
   `gcAddDObjAnimJoint` and `gcAddMObjMatAnimJoint` bind NULL instead
   (`gNdsAObjEvent32DetachCount`). A roster the table under-sizes loses that
   joint's animation for the motion; it cannot crash.

## Verification

Same lab ROM, no override (arms `fx_*`), every configuration that crashed or
overflowed before:

| Arm | Stage, roster | Limit | High-water | Fails / detaches | Heap low-water |
|---|---|---|---|---|---|
| `fx_l_g5` | Yoshi's Island, Ness/Yoshi/Pikachu/Purin | 4,720 | 4,041 | 0 / 0 | 63,780 |
| `fx_l_g7` | Saffron, same | 3,986 | 3,406 | 0 / 0 | 64,344 |
| `fx_l_g1` | Sector Z, same | 4,168 | 3,622 | 0 / 0 | 65,084 |
| `fx_l_g0` | Castle, same | 3,719 | 3,069 | 0 / 0 | 68,360 |
| `fx_s_g1` | Sector Z, DK/Samus/Link/Kirby | 3,358 | 2,752 | 0 / 0 | 50,048 |
| `fx_s_g0` | Castle, same | 2,909 | 2,131 | 0 / 0 | 66,076 |
| `fx_s_g2` | Jungle, same | 3,026 | 2,642 | 0 / 0 | 47,448 |
| `fx_s_g4` | Hyrule, same | 2,003 | 1,619 | 0 / 0 | 63,032 |

- All eight run the full match. Replay is IDENTICAL to the forced-capacity
  runs of the same configuration (`ev_l_g5`, `ev_l_g7`, `ev_s_g1`, `ev_s_g2`).
- Gate target (`gs1`, Dream Land, DK/Samus/Link/Kirby): limit stays 3,072
  (roster term 2,136), high-water 1,623, heap low-water 69,340. Against
  `glm1` P50/P95 932,416/1,298,816 -> 933,760/1,301,824 (the tree also
  carries the day's other commits), replay IDENTICAL.

## Also found

S2 (BUG_NOTES): with the ledger forced large, Ness/Yoshi/Pikachu/Purin on
Zebes data-aborted at frame 1920 in `ndsRendererAdapterBuildItemAttachMtx`
(a held item's attach joint is the fourth fighter's WRAM TopN slot).

## S3: libnds palette table growth failed in the published configuration

Lane A's report (1P overlay receipt): the published configuration with
Captain/Link/Pikachu/Kirby (`smash64ds-p2-shell-freeplay-hwtri` +
`NDS_P2_MENU_WALK=1 NDS_P2_SHELL_ARGMAX_ROSTER=1`, `probe-shell-four-kind.ps1
-ThroughResults`) aborts in `glBindTexture(name=80)`. It still aborts after the
S1 fix (`fourkind-argmax-fix.txt`) and with the owner texture memo off
(`fourkind-argmax-nomemo.txt`).

At the abort (`fourkind-argmax-s3.txt`, frame 418): libnds held 259 palette
names in a 256-slot `palettePtrs`, texture 80 had `palIndex` 257, and libc's
top chunk was 953 B. `glColorTableEXT` writes the palette index into the
texture before `DynamicArraySet` grows the array. The growth (a 2 KB realloc)
failed, and `glBindTexture` then read the NULL entry. In battle, libc also
carries every GObj thread's coroutine stack (`coroutine.c`).

Fix: `ndsPlatformReserveGlNameTables` grows the four libnds name arrays to 512
entries right after `glInit`. `glResetTextures` keeps their capacity, so no
scene grows them again.

| Build | Result |
|---|---|
| published config, before | abort at frame 418 |
| published config, 1,024 entries | full match, Results; arena -16,128 B |
| published config, 512 entries (`fourkind-argmax-s3fix512.txt`) | full match (2,043 presents), Results; libc top low-water 14,440 B; texture/palette names at match end 216/208 |
| gate (`gs2` vs `gs1`) | arena -7,680 B, heap low-water 69,340 both, libc low-water 20,376; P50/P95 928,768/1,295,616; replay identical after one sampling resync |

`tools/probe-shell-four-kind-s3.ps1` is the probe with the GL/libc prints.

## S2: a held item's freed attach joint (render guard)

Ness/Yoshi/Pikachu/Purin on Zebes (`fx_l_g3`, with the S1 fix) still faulted at
frame 1920 in `ndsRendererAdapterBuildItemAttachMtx`. A flushed witness showed
the item (GObj kind 1013) attached to a DObj now owned by an Effect (kind 1011):
the fighter joint had been freed and recycled. Its `user_data`, read as
FTParts, was a WRAM DObj at 0x03007F78, and the matrix read ran past the 32 KB
WRAM MPU region. The renderer now returns FALSE (fallback matrix) when the
attach joint's GObj is not a fighter. `fx2_l_g3`: full match, 66 guarded draws,
native failures 0.
