# P7: the four-mirror P4/P6 rosters against the WORK gate

The P7 worst-case search runs four mirrors of every P4/P6 content (the
`p95sweep` four-CPU lab ROM, `gNdsLabP4Content` = content x 4) on every VS
stage. The original cast meets the gate on Dream Land (WORK P95 1,059,456);
the mirror rosters did not. This file records what the census found and what
each pass changed. Gate: WORK P95 <= 1,120,000.

**Owner, 2026-10-18:** the 1,120,000 gate (>= 95% of frames within two
VBlanks) holds for the original cast on every stage (Hyrule Temple and the
busy stages included); four heavy P4/P6 contents -- four IDENTICAL ones
(mirrors) or four DIFFERENT ones (mixes, approved that evening) -- get a
stress target: P95 <= 1,250,000 and >= 90% of frames within two VBlanks.
Hyrule Temple's scenery may be simplified so its compiled stage fits, the
owner seeing screenshots first.

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

## Heap across the stages (2026-10-18)

The heaviest mix found (Crash, Sonic, Mr. Game & Watch, Palutena) and two
like it (Captain or Ganondorf in Crash's place) on every VS and P5 stage,
`build-sweep-s4`, 300 frames from the start (heap low-water; the sweep's
arena is 24,832 B under the play ROM's, the tick HUD's cost):

| stage | Crash, Sonic, G&W, Palutena | Captain, ... | Ganondorf, ... |
|---|---|---|---|
| Peach's Castle (0) | 51,236 | 51,024 | 47,272 |
| Sector Z (1) | 6,832 | 16,412 | 21,492 |
| Congo Jungle (2) | out of heap | out of heap | out of heap |
| Planet Zebes (3) | 6,896 | 16,224 | 22,888 |
| Hyrule Castle (4) | 55,696 | 48,336 | 54,348 |
| Yoshi's Island (5) | out of heap | 10,872 | 12,636 |
| Dream Land (6) | 45,240 | 51,412 | 49,468 |
| Saffron City (7) | out of heap | 4,896 | 6,348 |
| Mushroom Kingdom (8) | 38,488 | 47,356 | 49,816 |
| Meta Crystal (13) | 56,864 | 53,300 | 47,932 |
| Duel Zone (14) | 61,744 | 60,056 | 50,608 |
| Final Destination (16) | 10,300 | 25,536 | 26,848 |
| First Destination (41) | 55,472 | 50,736 | 53,988 |
| Bowser's Stadium (42) | 53,592 | 66,584 | 67,464 |
| Hyrule Temple (43) | out of heap | 1,980 | 5,640 |
| Dinosaur Land (44) | 13,384 | 27,184 | 31,072 |

An ordered census of the Jungle run (`census2.ps1`): the overlay loan (all
445,344 B) holds the battle's common files (56 KB), the HUD's (50 KB), the
ground and its collision (95 KB), the stage blob (21 KB), the item pool and
the items' common file (15 + 83 KB), the fighters' pools (36 KB) and one owner
image (38 KB); the heap then takes the stage's own files (66 KB), the effects'
common files (97 KB), and per content its pack (27-64 KB), its other files
(Palutena 67 KB, her wings file 35 KB of it), shield poses, tables, figatree
heap and owner image; the entry corners and the stage's GX list come last and
find the heap empty (the GX list declines; a thread stack overflows).

What changed (2026-10-18), measured on the ship ROM (`play-p7d`, real menus,
the roster and stage poked at the battle start, 900 frames of a 3-minute
match; the heaviest mix):

- **The announcer's letters stay out of a VS battle.** IFCommonAnnounceCommon
  (32,496 B: SUDDEN DEATH!, and the 1P bonus boards' FAILURE and COMPLETE!)
  was resident in every VS battle and read only by a Sudden Death
  (`battleship_scvsbattle.c` `ndsSCVSBattleLoadCommonFiles`). A Sudden Death
  loads it when the battle before it left its bytes plus the 25 KiB latch
  (`ifCommonSetMaxNumGObj`); otherwise SUDDEN DEATH! is announced by voice
  only (`battleship_ifcommon.c` `ifCommonSuddenDeathMakeInterface`), never by
  running the heap out.
- **No battle coroutine left.** The start lamps (`ifCommonCountdownThread`)
  and Sudden Death's start (`ifCommonSuddenDeathThread`) run as function
  processes with the thread's ticks (as the announcer and the entry threads
  already did), so no 4,208 B coroutine block is taken.
- **The entry corners keep 8 KiB free** (`NDS_ENTRY_P4_CORNERS_KEEP_FREE`):
  a block that would take the last bytes before the setup's own allocations
  stays out (its roots draw nothing) instead of halting the match.

| stage | before | after |
|---|---|---|
| Congo Jungle (2) | out of heap | 26,304 |
| Yoshi's Island (5) | 7,728 | 37,376 |
| Saffron City (7) | 4,152 | 29,672 |
| Hyrule Temple (43) | 1,960 | 28,408 |
| Sector Z (1) | 15,520 | 46,048 |
| Planet Zebes (3) | 16,584 | 46,952 |
| Final Destination (16) | 20,472 | 42,064 |
| Dinosaur Land (44) | 21,904 | 40,500 |

(Yoshi's Island through Dinosaur Land were measured before the coroutine
change, Jungle and Hyrule Temple after it.) Congo Jungle was 832 B under
25,600 after the first two; nothing is left idle in the loan
(`sNdsFrontendLoanEnd - sNdsFrontendCursor` 0, 201,216 B spilled to the
heap). The event32 ledger gave that roster 5,786 entries and 2,273 were live
(on Hyrule Temple, a P5 stage under the flat 5,120 bound, 1,328): a roster
of contents alone no longer takes the cast's 384-entry roster margin
(`ndsAObjEvent32RosterMargin`; a content's own allowance is a measured
four-of-it peak with Castle's stage share in it), 5,402 entries, and the
Jungle row above is with it.

The freed heap moved frame cost too (sweep `s6`, Dream Land, 1,960 frames,
replay digests identical to `s4` on all six rosters): Crash, Sonic,
Mr. Game & Watch and Palutena 1,428,032 -> 1,340,032 (77.5% -> 84.9% within
two VBlanks); Palutena, Bowser, Meta Knight and Ike 1,502,656 -> 1,370,432
(70.3% -> 81.2%); the mirrors and the cast unchanged (1,079,872 cast; Bowser
1,189,120; Crash 1,177,088; Palutena 1,246,208).

## The gate ROM's instrument (2026-10-18, `s7`)

The tick HUD's sample ring is battle heap the published ROM has: the arena
follows `.main.bss`, and 34 buckets x 128 samples were 17,408 B of it. The
four-CPU gate ROM now keeps 32 samples a bucket (`NDS_TICK_HUD_WINDOW`,
4,352 B; 128 on every other tick-HUD target), and
`scripts/sample-tick-hud-buckets.ps1` reads the window from the ELF and stops
every 24 presented frames on the universal marker (the four-CPU arm's sparse
marker fires every 32 frames, a whole ring, which reads as an unchanged head).
The gate ROM's data and bss are still 8,800 B over the play ROM's (the ring,
the CYCLE 98 material census's 2 KB, the HUD's percentile rows), and its ITCM
leaves out four functions the play ROM keeps.

Dream Land unless named, 1,960 frames, WORK P95 (frames within two VBlanks):

| roster | `s6` | `s7` |
|---|---|---|
| original cast | 1,079,872 (96.0%) | 1,078,464 (96.0%) |
| original cast, Saffron City | 1,260,480 (87.5%) | 1,218,688 (88.1%) |
| original cast, Hyrule Temple | 1,504,512 (26.6%) | 1,472,896 (31.5%) |
| Crash, Sonic, Mr. Game & Watch, Palutena | 1,340,032 (84.9%) | 1,256,064 (90.2%) |
| Palutena, Bowser, Meta Knight, Ike | 1,370,432 (81.2%) | 1,320,640 (86.8%) |

`scripts/run-task37-profile-census.ps1 -BootSetGlobals` writes the lab ROM's
stage, roster and content words at `main` (one GDB attach, then detach), so
one profile build serves every stage and roster.

## Hyrule Temple's stage program, and the P5 stages' fast path (2026-10-18)

Hyrule Temple's stage program (`hyruletemple.gxp`) is 61,888 B of body and
9,204 B of run memo. The loader kept 62,020 B free beside it (the 25,600 B
floor plus the 36,420 B a match grows after the load) and declined it in both
layouts (`gNdsP2StageProgReason` 3; the play ROM had 122,512 B free at the
first stage draw), so the temple drew uncompiled: 409K ticks of `STG` a frame
at P50 against Dream Land's 129K, most of it each painter triangle loading its
own matrix on the CPU. A census of the cast match's allocations after the load
found 36 KB of growth, all of it draw caches with floors of their own (a lean
spare 17.7 KB, the world caches 13.3 KB, a lean texture block 5.1 KB): the
program now keeps the floor and a 12 KB margin (`NDS_STAGE_GX_HEAP_KEEP_FREE`),
loads, and the caches decline instead.

The P5 stages also drew their programs run by run: the per-run path (run
culling, a patch and DMA a run) is the 1P venues', keyed on gkind >= 9, and
the P5 stages take 1P venues' gkinds (13, 14, 16) or come after them (41-44).
A VS battle now takes the segment's one-DMA path whatever the gkind.

Cast, WORK P95 (frames within two VBlanks), replay digests identical:

| stage | before | `s9`/`s10` |
|---|---|---|
| Hyrule Temple (43) | 1,472,896 (31.5%) | 1,271,424 (84.4%), heap low-water 27,736 |
| Meta Crystal (13) | 1,136,896 (93.8%) | 1,105,600 (95.2%) |
| Bowser's Stadium (42) | 1,136,448 (93.8%) | 1,123,072 (94.3%) |
| Dinosaur Land (44) | 1,218,688 (90.2%) | 1,194,496 (92.5%) |
| Final Destination (16) | 1,032,384 (96.8%) | 1,002,048 (97.4%) |
| Duel Zone (14) | 944,448 | 931,968 |
| First Destination (41) | 998,912 | 967,168 |

Heavy mixes on the tight stages still decline their programs (gate ROM, Crash,
Sonic, Mr. Game & Watch and Palutena): Hyrule Temple 1,666,560 (heap 21,172),
Congo Jungle 1,458,176 (16,816), Saffron City 1,449,344 (24,164), Yoshi's
Island 1,461,952 (31,148). The gate ROM's arena is still ~8.8 KB under the
play ROM's (the tick HUD's ring, rows and the CYCLE 98 material census).

## Where the cast's P95 frames come from (2026-10-18, `s11`/`s12`)

Cast on every VS and P5 stage, `build-sweep-s10` (1,960 frames; target
1,120,000 and 95% within two VBlanks): Peach's Castle 1,143,680 (93.2%),
Sector Z 1,119,616 (94.6%), Congo Jungle 1,140,160 (94.0%), Planet Zebes
1,089,472 (96.1%), Hyrule Castle 1,006,336 (97.5%), Yoshi's Island 1,186,112
(90.9%), Saffron City 1,216,448 (88.2%), Mushroom Kingdom 1,192,000 (91.9%),
Bowser's Stadium 1,123,072 (94.3%), Dinosaur Land 1,194,496 (92.5%), Hyrule
Temple 1,271,424 (84.4%); the rest pass. Over-budget frames are spread
evenly through the match (not warm-up).

The very top frames are lean list materializations: 13 of Dinosaur Land's 20
most expensive profiled frames hold one (`ndsFtrLeanMaterialize` entry counts
in the per-region profile), ~300-400K ticks each (diag ROM, `gNdsFtrLean.
materialize_ticks`). Two fixes (fb12a2d3d45):

- The spare and pool kept 62,020 B free plus every pending spare: Dinosaur
  Land's cast never had either (90,964 B free at the start). They now keep the
  stage body's floor (25,600 + 12,288).
- Only 27 of Dream Land's 80 materializations could copy unchanged root blocks
  (ROOT BLOCKS): Captain Falcon's tables set a blend colour, and a root reading
  the carried colour failed self-containment. The colour is in no list word
  (only the G_AC_THRESHOLD decline reads it), so such a root now keys the
  decline bit and a copy leaves the colour as its walk would. Every plan
  records; the lab verify arm compared 311 re-selected lists with fresh walks:
  no mismatch.

They cut the worst frames (13-14 of the top 120 by 50-300K) but barely move
the 98th: Dinosaur Land 1,194,496 -> 1,170,432, Mushroom Kingdom 1,192,000 ->
1,184,064, the others within a few thousand. The frames around the 95th
percentile (profile ranks 12-32 of 385 against the median third, Dinosaur
Land) are status changes: animation start (`ndsFtPoseParse`/`Play`/
`BindEntry`, the figatree load) ~98K cycles over the median, lean
materialization ~90K, first-use script normalization 27K, audio sample reads
(`ndsAudioStorageReadSync`) 28K, hurtbox tests 30K. Saffron City's extra is the
hurtbox kernel (~240 tests in a heavy frame from the Pokemon's weapons and
items, each composing the victim's joint chain; the decomp's hit_detect_range
box already pre-filters), Hyrule Temple's its collision sweeps (~105K ticks a
frame, ~70K over Dream Land) and its stage program's commit (~50K).

Also found: Link's bomb drew its body and fuse lists through one owner
texture memo (the full resolver twice a frame while a bomb was out), and the
heaviest mix on Hyrule Castle has two pre-existing native failures: a
Bob-omb's texture bind fails (`gNdsItemBombHeiSubmitStep` 3) with four heavy
contents' textures resident.
