# P7: the four-mirror P4/P6 rosters against the WORK gate

The P7 worst-case search runs four mirrors of every P4/P6 content (the
`p95sweep` four-CPU lab ROM, `gNdsLabP4Content` = content x 4) on every VS
stage. The original cast meets the gate on Dream Land (WORK P95 1,059,456);
the mirror rosters did not. This file records what the census found and what
each pass changed. Gate: WORK P95 <= 1,120,000.

**Owner, 2026-10-18:** the 1,120,000 gate (>= 95% of frames within two
VBlanks) holds for original-cast and mixed rosters; four IDENTICAL heavy
P4/P6 contents get a stress target: P95 <= 1,250,000 and >= 90% of frames
within two VBlanks.

## Dream Land, 1,960 frames, WORK P95

| roster | 10-17 | pass 1 | 1-3 | 1-4 | 1-5 (two-VBlank frames) |
|---|---|---|---|---|---|
| c20 Palutena | 1,395,072 | 1,447,936 * | 1,393,792 | 1,271,616 | 1,268,224 (86.0%) |
| c4 Bowser | 1,202,816 | 1,197,376 | 1,196,608 | 1,184,832 | 1,177,920 (92.7%) |
| c16 Meta Knight | 1,203,200 | 1,210,304 | 1,203,136 | 1,199,936 | 1,198,080 (92.6%) |
| c14 | 1,189,824 | 1,183,808 | | | 1,178,368 (91.6%) |
| c5 | 1,170,688 | 1,172,992 | | | 1,161,408 (92.0%) |
| c6 | 1,166,464 | 1,173,440 | | |
| c19 Ike | 1,166,912 | 1,169,344 | | |
| c9 | 1,165,440 | 1,171,136 | | |
| c17 | 1,124,736 | 1,130,944 | | |
| c13 | 1,113,216 | 1,118,592 | | |

\* the p6ac fixes (longer light wings, two more glow windows) landed between
the 10-17 run and pass 1.

Artifacts: `artifacts/performance/2026-10-17_p7-sweep/` (`dl*`),
`2026-10-18_c20-profile*`, `2026-10-18_c4-profile`, `2026-10-18_lean-diag`.

## What the census found

* **Palutena's wide lean list.** Her list is 3,215 words; a half entry holds
  3,084 (8,840-word slot region, 1,336-word packet header). The list went
  wide, which left no second entry and no variant records, and her eye blink
  (head MObj `texture_id_curr` 0 <-> 1) re-materialized it on every flip: 82
  times a match at ~600K cycles, all in the tail. (gdb dprintf on
  `ndsFtrLeanMaterializeFor`: two keys per slot, differing in key[0] only.)
* **Her smash wings on the CPU.** Six of the wings' eight groups carry seam
  corners loaded under the previous bone's matrix (matrix overrides), and an
  override group was projected and divided on the CPU corner by corner:
  `ndsRendererHardwareClipVertex` +56K in heavy frames.
* **The P4 sidecar scan.** Every Fox-parent content's every draw scanned all
  160 P4 effect roots for a sidecar (46K cycles a frame for four Palutenas).
* **Status-change spikes (every Remix roster).** A status change reads its
  clip from storage when the anim cache misses (a synchronous ARM7 card read,
  ~30K cycles; the elastic cache gets ~50 KB of a P4 match's heap, about half
  a mirror roster's working set), parses the new clip's keys, and runs a lean
  material event. On four Bowsers 22 of 30 over-gate frames held a card read.
* **Lock fighters.** Bowser's lock chain divided by the parent's accumulated
  scale three times a joint (27% of `ndsFtrLeanLockLocal`), and the hurtbox
  lock path likewise.
* **P4 sound lookups** scanned the whole base FGM pack (16K cycles) before the
  P4 pack's own search; **P4 clip tokens** walked the whole token chain
  (14.5K) when the 64-line memo had lost them.

## The passes

1. Sidecar answers memoized per content and joint; Palutena's light and glow
   effects searched only while one may exist; the light's ring halves hidden
   outside the taunt.
2. An override group that tests Z draws through the GX: the override
   corner's modelview is loaded between the corners of the open batch
   (its A/B word left 2026-10-18).
3. FGM ids past the base map skip its scan when the map holds every base id;
   lock-chain reciprocals memoized per axis (lean builder and hurtbox);
   P4 numeric tokens answered before the token chain (no row below can match
   one).
4. Wide lists keep variant records at the top of their region and learn in
   place (`NDSFtrLeanWideHold`); a wide list's submit cleans its marked lines
   plus the lines past the marks instead of the whole list; Palutena's light
   windows are found once a move, not twice a tic.
5. A variant learn compares only the head and the root blocks whose key,
   place or polygon-ID base differ (a self-contained root's block is a
   function of its key, the premise root-block copying stands on): four
   Bowsers learn 7-8 expressions a fighter, and each learn read both ~12 KB
   lists word by word.

## Memory: four different contents (2026-10-18)

The worst-case search also covers four DIFFERENT contents, and those did
not fit: four heavy ones (Palutena, Bowser, Meta Knight, Ike) ended at a
heap low-water of 17,388 (gate 25,600), and the heaviest estimate
(Palutena, Sonic, Mr. Game & Watch, Ike) halted at setup
(`ndsSyMallocOverflowHalt request=11728 free=7398`, Ike's animation heap).
A per-content census (pack, own externs, low-detail owner image, guard
package, tables, animation heap) found three causes:

* **P6 orphan geometry.** An authored look leaves its base's replaced lists
  and vertices behind, reachable from no pointer. The packs kept them:
  51,232 B of Palutena's battle pack, 38,400 B of Ike's (every Remix
  content: 0). `p4_preview_pack.orphan_spans` drops a run of unreachable
  regions when the live region before it has a known extent (pruned
  geometry, an array of pointers and zeros, or an image no larger than its
  region). Battle packs: Palutena 100,780 -> 49,404 B, Ike 73,324 ->
  32,852; the select's packs shrink the same. Other contents' packs are
  byte-identical.
* **Palutena's raw guard file.** A content with translate_scales kept its
  22 KB ShieldPose file because the native guard did not scale. It now plays
  such a content's guard through the source's scaled player
  (`lbCommonPlayTranslateScaledDObjAnim`, the joint's `translate_scales`
  row), and her package is built like everyone's (4,356 B, the same bytes as
  Peach's: the same file at the same nine targets; worst pose error 0.008).
* **TIME UP / GAME SET.** Their run-length streams (22,104 B) lived on the
  match heap from setup to the end. They now sit in bank H, between the sub
  screen console's font tiles (8 KB) and its map (2 KB at 30 KB): every
  battle gains 22,104 B.

Correctness: the replay digests (DGSA/DGSB) of every sweep run are identical
across the builds (1,960 of 1,960 frames, c20/c4/c16); the wings' GX path
against the CPU path and the blink before/after were captured frame for
frame (`artifacts/visibility/p6-vfx/wings-*`, `blink-*`): the blink frames
are pixel-identical, the wings differ by scattered rounding pixels.

## The shipping layout (2026-10-18)

Every figure above came from the bare four-CPU sweep ROM, whose config has
no menu shell, 1P game or UI kit. Its arena was 978,688 B and its front-end
overlay (a VS battle's loan) 185,056 B; the play ROM's are 641,536 and
445,344 (`gNdsTaskmanArenaChosenSize`). Once the loan is full a VS battle
of the shipping image has ~77 KB less than the lab's, and the heaviest
content mixes ran out of heap at battle start there
(`ifCommonTimerMakeDigits`, the P5 risk note's site): Sonic, Mr. Game &
Watch, Palutena and Crash on Dream Land, any of the Sonic mixes.

The sweep is now built with the shipping flags (`NDS_P2_1P_GAME`,
`NDS_P2_MENU_SHELL`, `NDS_P2_UI_KIT`, freeze diagnostics, ship telemetry);
`scene_harness.c` keeps the lab's boot into the match under
`NDS_LAB_FOURCPU_WORDS`. Its arena is 616,704 B: the tick HUD costs ~23 KB
against the play ROM, so its low-water reads that much low.

Memory in the shipping layout:

* **Sonic's Tails plane** (his special file 3, 75 KB, drawn by his entry
  only) leaves the fighter's load and loads at his entry
  (`ndsP4EntryFileEnsure`): from the loan when it has room, else from the
  heap with 40 KB left over, else the entry runs without the plane. The
  worst mix (Crash, Sonic, Mr. Game & Watch, Palutena) on Dream Land now
  ends at 40,580 B (the plane declined); Sonic beside three of the cast
  loads it from the loan.
* **A VS battle allocates from the loan first** (`syTaskmanMalloc`): the
  stage, items, effects and interface used the heap beside an idle loan in
  any match whose contents left it idle, and the elastic motion cache (the
  heap's free top) was that much smaller (four Bowsers: 18 KB of cache, a
  32% hit rate; 54 KB and 53% in the bare layout).

* **A content never takes the battle pack's carve.** The pack holds the
  Fox kind's clips (`NDS_BATTLEPACK_FIGHTER_KIND`); a content's kind is its
  parent's, so a roster of Fox-parent contents (Falco, Wolf, Peach, Sonic,
  Palutena) took the pack's path, whose 128 KB keep-free declined the
  motion cache and latched: four Palutenas, and the worst mix, played the
  match with no motion cache. `ndsR2BattlePackCarveWorthIt` now skips
  contents. Four Palutenas: low-water 77,752 -> 315,576 (the pack is not
  loaded); the worst mix gains a 37 KB cache (101 hits, was 0).
* **The renderer's range checks accept the loan.** A buffer
  `syTaskmanMalloc` lent from it (Crash's effect lists) failed the stage
  renderer's arena test and drew nothing (`BAD_ASSET`, one native failure
  in four Crashes, `s3`); `ndsFighterDLScanRangeInTaskmanArena` also takes
  the loan's lent bytes (`ndsFrontendOverlayLoanContains`).

Frame cost, shipping layout (Dream Land, 1,960 frames; WORK P95 / frames
within two VBlanks / heap low-water). `s2`: the shipping flags; `s4`: the
loan first, contents off the pack's carve, the loan in the range checks,
thirteen A/B words removed:

| roster | `s2` | `s4` |
|---|---|---|
| original cast (preset) | 1,078,016, 96.0%, 54,576 | 1,079,232, 96.0%, 66,672 |
| c4 Bowser mirror | 1,313,792, 82.0%, 25,188 | 1,187,008, 92.1%, 73,172 |
| c20 Palutena mirror | 1,298,816, 83.0%, 41,200 | 1,250,432, 86.7%, 315,576 |
| c9 Crash mirror | 1,328,896, 90.1%, 57,908 | 1,180,544, 93.2%, 59,636 |
| Palutena, Bowser, Meta Knight, Ike | (`s3` 1,508,800, 71.2%, 43,544) | 1,502,656, 70.3%, 44,824 |
| Crash, Sonic, Mr. Game & Watch, Palutena | | 1,428,032, 77.5%, 39,992 |

Replay digests are identical across `s2`, `s3` and `s4` (1,960 of 1,960
frames, every roster measured on both); 0 native failures in `s4`. Four
Palutenas with 315 KB of free heap run at the same P95 as with no motion
cache: the over-gate frames are drawing (MISC P95 644K against the cast's
543K; four distinct heavy contents 821-955K), not clip reads. Mixes of
heavy contents are the open gap: 1.43-1.50M against the 1.12M gate.
