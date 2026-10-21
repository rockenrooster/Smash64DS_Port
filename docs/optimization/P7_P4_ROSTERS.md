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

## Bounded pipelining, the CPUs' low detail and Hyrule Temple's memory (2026-10-19, `s16`/`s17`)

`s16` (HEAD 082148f0e09, cast on every VS and P5 stage) regressed Hyrule
Temple to 1,477,376 (31% two-VBlank): its stage program stopped loading
(`gNdsP2StageProgReason` 3). An allocation census against `s12` showed why:
the 1P campaign and P7 work grew the resident `.main` by 18,432 B, and the
frontend overlay a VS battle borrows shrank by 32,864 B, so the battle had
19.5 KB less in all; the program had loaded with 3.3 KB to spare. Four-CPU
Hyrule Temple at the program's load: 92,756 B free against the 108,980 B it
asks (71,092 B body and memo plus the 37,888 B it keeps).

Owner 2026-10-19 (two rulings):

- **CPUs keep low detail in 3-4 fighter VS battles** (as contents and the
  campaign's CPUs already did): their high images (19-30 KB each) are not
  loaded and the KO and pause close-ups draw them low; humans keep high, and
  in a wireless match only the host does (only the host pauses; every console
  knows the host's port, so they agree). `ndsFTManagerLowDetailOnly`
  (battleship_ftmanager.c) is the one rule for the image load, the close-up
  clamp (`ftParamSetModelPartDetailAll`) and Kirby's copy hats (a Kirby kept
  low loads low hats only).
- **Bounded pipelining**: a battle frame's next game logic may start up to half
  a VBlank (131 scanlines) before its swap retrace (~8 ms more input latency on
  average), and the gate stays both numbers (WORK P95 and cadence).
  `nds_platform.c` (the block above ndsPlatformPresentLeadEnable): the flush is
  held to the retrace before the scheduled one, EndFrame returns at the lead
  line (a VCount interrupt), and the retrace's commits (deferred palettes, fade,
  wallpaper affine, texture refreshes, OAM planes) run on a present thread one
  priority above the main thread; the next draw waits for that commit
  (`ndsPlatformPresentLeadRetire`). Game order and the replay digest are
  unchanged. A simulation over the `s16` ring CSVs (scratchpad pipe_sim.py)
  gave the cast 96-99% two-VBlank on every failing stage at 0.5 VBlank.
  `gNdsPresentLeadLines` 0 at boot is the old loop on the same ROM.

First look (`s17`, Hyrule Temple cast, 600 frames): the program loads again,
heap low-water 49,760, WORK P95 1,076,096, 590 of 600 frames within two
VBlanks; every frame led and 423 of them ran their logic past the retrace (the
commit had landed when the draw began).

Cast, `s16` -> `s17` (1,960 frames, WORK P95 and frames within two VBlanks;
replay digests identical on every stage; 0 native failures):

| stage | `s16` | `s17` |
|---|---|---|
| Peach's Castle (0) | 1,138,432 (93.3%) | 1,149,440 (98.8%) |
| Sector Z (1) | 1,144,768 (93.5%) | 1,111,936 (98.8%) |
| Congo Jungle (2) | 1,145,216 (93.6%) | 1,120,128 (98.9%) |
| Yoshi's Island (5) | 1,188,544 (91.1%) | 1,176,640 (98.5%) |
| Saffron City (7) | 1,253,440 (85.7%) | 1,213,568 (97.3%) |
| Mushroom Kingdom (8) | 1,185,856 (91.6%) | 1,183,488 (98.0%) |
| Bowser's Stadium (42) | 1,130,048 (93.9%) | 1,127,232 (98.9%) |
| Hyrule Temple (43) | 1,477,376 (31.3%) | 1,221,824 (97.2%) |
| Dinosaur Land (44) | 1,188,800 (92.2%) | 1,173,568 (98.2%) |

Cadence now passes everywhere; WORK P95 still fails on eight. The frames
over the budget are logic frames: against the median, the 93rd-98th
percentile frames carry +190-250K of SRC and +85-113K of draw (MISC).

Mid-match first-use script normalization (Dream Land cast, frames 150-700): 64
scripts in 16 frames -- fighter model parts' MObjs (15), the catch and item-get
swirls (9 each), the KO blast (9, 206 commands), damage slashes, Whispy's
anims, the impact wave, the quake and Link's bomb and boomerang.

Two tries that went back out (`s18`/`s19`, same day):

- **Normalizing the common effects' scripts at load** (efManagerInitEffects,
  every NDS_EF_MANAGER_DESCS desc's DObj and MObj scripts): Congo Jungle's
  replay digest diverged from frame 140 (before GO) and its event32 ledger
  filled (3,026 of 3,026); the other stages moved by noise. The tail's
  normalizations were not the effects' first uses.
- **A per-fighter hurtbox cache epoch** (only the cleared fighter's cached
  worlds dropped): Saffron 1,213,568 -> 1,211,584 and a few frames of digest
  drift (Peach's Castle from frame 1388). The global epoch stays.

`s20` (the particle heavy-frame LOD's elapsed time no longer counts the wait
for the led frame's swap, so it marks only frames whose own work is heavy):
every stage's digest identical to `s17`, WORK P95 within noise (Saffron
1,215,232; Hyrule Temple 1,219,648; Peach's Castle 1,146,944).

Where Saffron's tail goes (`s17` profile, 20 costliest frames): ~120 hurtbox
tests a frame against weapons (69.5) and items (51), each building its joint's
fixed-point world (LocalFromDObj, Compose, WorldOf, DamageBox: ~167K ticks a
frame there, ~40K elsewhere); fighter attacks are under one test a frame.
Peach's Castle and Yoshi's Island tails are spread: texture conversions (a
third of the frames), first-use normalization (12-16 of 20), lean
materialization (6), synchronous sound reads (12), pose parse (all 20).

Later 2026-10-19:

- **Stress rosters on `s20`** (pipelined; 1,960 frames, WORK P95 and frames
  within two VBlanks): four Palutenas 1,259,392 (96.3%) on Dream Land,
  1,307,904 (93.2%) on Saffron, 1,328,064 (92.3%) on Hyrule Temple; four
  Bowsers 1,200,384 (97.7%), 1,330,688 (94.8%), 1,305,088 (94.9%). Cadence
  meets the stress target everywhere; WORK P95 misses 1,250,000 on Saffron and
  Hyrule Temple.
- **Hyrule Temple's layers are the temple.** Hiding layer 0 (the "secondary"
  layer: 100 of the template's 177 runs, 195 of its 527 triangles) or layer 3
  leaves the fighters standing on the sky (`artifacts/visibility/p7-ht/`), so
  a layer cannot simply go; the template is 13,196 GX words a frame, 430
  patches (177 material, 104 composed no-Z, 98 no-Z runs, 51 composed).
- **Refuted, `s21`:** deduplicating the stage commit's per-run tail (texture
  stamps of consecutive runs on one entry, one span-state write): every
  digest identical, WORK P95 within +-5K on all nine stages. Reverted.
- Where the remaining tail lives (census, top-20 frames): status changes
  (pose parse/bind, the setter, motion copies), the hurtbox kernel against
  weapons and items (Saffron's Charmander flame and Link's bombs against all
  four fighters every tick), first-use normalization, sound and clip reads;
  each remaining lever measures 2-10K.

Four-mirror rosters on Dream Land, `s21` (pipelined; 1,960 frames; WORK P95,
frames within two VBlanks, heap low-water; 0 native failures on every row):

| content | P95 | 2-VB | heap |
|---|---|---|---|
| c1 Falco | 1,047,168 | 99.4% | 60,848 |
| c2 Ganondorf | 1,077,632 | 99.3% | 38,072 |
| c3 Wolf | 1,114,688 | 99.2% | 49,136 |
| c4 Bowser | 1,198,272 | 97.8% | 38,228 |
| c5 Marth | 1,183,616 | 98.1% | 39,112 |
| c6 Roy | 1,179,072 | 98.3% | 45,184 |
| c7 Wario | 1,052,416 | 99.6% | 34,752 |
| c8 Peach | 1,068,800 | 99.5% | 31,312 |
| c9 Crash | 1,179,776 | 97.7% | 42,580 |
| c10 Lanky | 1,053,632 | 99.3% | 39,256 |
| c11 Sheik | 1,116,736 | 99.4% | 35,848 |
| c13 Sonic | 1,123,264 | 98.7% | 38,028 |
| c14 Dedede | 1,177,536 | 98.3% | 37,136 |
| c15 Mad Piano | 931,712 | 99.9% | 141,212 |
| c16 Meta Knight | 1,216,384 | 97.6% | 42,304 |
| c17 Mr. Game & Watch | 1,128,832 | 98.3% | 47,684 |
| c18 Snake | 1,104,576 | 99.2% | 39,848 |
| c19 Ike | 1,181,056 | 98.1% | 43,124 |
| c20 Palutena | 1,259,904 | 96.2% | 334,552 |

Eighteen of nineteen meet the stress target (P95 <= 1,250,000, >= 90%);
four Palutenas are 9,904 over.

**The event-script ledger refused scripts for the cast** (found 2026-10-19
reading high water against limit in the sweep extras): Congo Jungle 7,
Hyrule Castle 7, Planet Zebes 1 refusals in 1,960-frame cast matches, reason
12, no native failure. The CPUs' low detail had grown the elastic motion
cache and the cast's normalized clips with it. Roster margin 384 -> 896
(`59970c16490`): 0 refusals on the nine stage-bound stages (peaks 3,128 /
2,105 / 5,058, 400-500 to spare), digests identical, P95 noise; Ness, Yoshi,
Pikachu and Purin on Jungle, Zebes, Castle and Yoshi's Island peak 3,084-5,911
with 2,280+ to spare. Probes now read `gNdsAObjEvent32NormalizeFailCount`.
Remaining cast stages on `s21` (pipelined): Zebes 1,083,904, Hyrule Castle
1,020,480, Dream Land 1,074,688, gkind 13 1,110,528, 14 927,104, 16
1,005,376, 41 971,840 -- all under 1,120,000, 98.8-99.8% within two VBlanks.

## Saffron City and Hyrule Temple mirrors (2026-10-19, `s23`)

Four identical CPUs of each content (`build-sweep-s23`, ship layout, pipelined), 1,960
frames, WORK P95 / within two VBlanks / heap low-water:

| content | Saffron City (g7) | Hyrule Temple (g43) |
|---|---|---|
| c1 Falco | 1,126,528 / 98.9% / 59,696 | 1,122,432 / 99.1% / 103,392 |
| c2 Ganondorf | 1,193,280 / 97.8% / 40,500 | 1,186,240 / 98.3% / 45,576 |
| c3 Wolf | 1,213,056 / 97.3% / 47,696 | 1,201,152 / 98.2% / 37,680 |
| c4 Bowser | 1,335,104 / 94.7% / 41,292 **over** | 1,308,672 / 94.9% / 46,580 **over** |
| c5 Marth | 1,208,384 / 97.5% / 45,524 | 1,267,712 / 96.5% / 45,932 **over** |
| c6 Roy | 1,297,152 / 95.7% / 50,316 **over** | 1,357,056 / 93.1% / 31,736 **over** |
| c7 Wario | 1,138,624 / 99.5% / 41,664 | 1,152,704 / 99.3% / 44,240 |
| c8 Peach | 1,155,648 / 98.6% / 36,064 | 1,151,104 / 99.1% / 38,584 |
| c9 Crash | 1,271,936 / 96.1% / 47,648 **over** | 1,322,880 / 95.6% / 32,544 **over** |
| c10 Lanky | 1,105,408 / 98.8% / 41,112 | 1,201,472 / 98.2% / 42,464 |
| c11 Sheik | 1,172,352 / 99.0% / 42,896 | 1,201,216 / 98.6% / 43,832 |
| c13 Sonic | 1,302,208 / 95.1% / 44,356 **over** | 1,281,920 / 96.0% / 44,864 **over** |
| c14 Dedede | 1,138,304 / 98.8% / 45,604 | 1,241,024 / 97.7% / 45,464 |
| c15 Mad Piano | 930,560 / 99.9% / 59,088 | 1,000,960 / 99.9% / 42,736 |
| c16 Meta Knight | 1,320,448 / 95.1% / 24,040 **over** **heap** | 1,319,104 / 95.9% / 32,860 **over** |
| c17 Mr. Game & Watch | 1,120,704 / 99.1% / 36,160 | 1,240,448 / 97.6% / 35,152 |
| c18 Snake | 1,235,328 / 97.1% / 45,160 | 1,368,640 / 91.6% / 45,024 **over** |
| c19 Ike | 1,274,368 / 96.4% / 44,332 **over** | 1,358,720 / 91.6% / 49,572 **over** |
| c20 Palutena | 1,310,528 / 93.3% / 270,992 **over** | 1,327,808 / 92.3% / 221,336 **over** |

Against the stress target (P95 <= 1,250,000, >= 90%): Saffron 12/19 and Hyrule
Temple 10/19 pass; every run is >= 90% within two VBlanks (lowest 91.6%) and the
P95 misses are 1.4-9.5%. Four Meta Knights on Saffron leave 24,040 bytes of heap, under
the 25,600 floor. 0 native failures, 0 lean declines, 0 ledger refusals everywhere.
Over-gate frames on Hyrule Temple (`s20p` cast profile) are first-use costs: event-script
normalization 12% of the premium, texture binds 8%, the lean packet's variant learning
and materialize 18%, pose parse 4%; on Saffron the hurtbox tests (~25%) lead.

## Collision sweep keys and the stage matrix memo (2026-10-19, `s27`/`s28`)

`s27` (committed c8af1ef888b): the floor and ceiling sweeps make their sweep's
compare keys once a group instead of once a line. Exact (digests IDENTICAL
against `s26` on the cast at Hyrule Temple and Saffron, four Bowsers on
Saffron and four Snakes on Hyrule Temple). WORK P95:

| run | before | `s27` |
|---|---|---|
| cast, Hyrule Temple | 1,220,096 (`s25`) | 1,215,616 |
| cast, Saffron | 1,210,688 (`s26`) | 1,214,720 (noise) |
| four Snakes, Hyrule Temple | 1,319,680 (`s26`) | 1,303,872 |
| four Bowsers, Saffron | 1,329,728 (`s25`) | 1,324,544 |

`s28` (refuted, reverted): `ndsStageGxComposedFor`'s memo slot
`((binding << 3) | shift) & 31` keeps only a binding's low two bits, so a
stage's bindings share four slots per shift (~46 rebuilds a frame on Hyrule
Temple). Slotting by `binding + 8 x shift` and building into the slot was
exact (digests IDENTICAL on the same four runs) but moved nothing (P95 -64 to
+1,984), so the rebuilds are not where the frames go. The gate is still open:
the cast sits ~8.5% over 1,120,000 and the heavy mirrors 4-6% over 1,250,000.

## The cast gate at 1,250,000 (owner 2026-10-20, `s35`/`s36p`)

Cast on `s35` (ship layout, pipelined, 1,960 frames), WORK P95: Hyrule
Temple 1,232,640, Saffron 1,230,656, Sector Z 1,117,632, Yoshi's Island
1,117,504; every stage 97-99% within two VBlanks. Profiles of HEAD
(`build-prof-s36`, 384 frames from 200) split the P90-97 band against the
P25-75 control by subsystem (scratchpad `band_groups.py`):

- Hyrule Temple: premium 401K a frame (profile cycles) -- pose and animation
  19%, stage collision 18% (275K a control frame: the wall sweeps), other
  logic 17%, fighter and stage render 14%, clip reads 7%, hurtboxes 6%,
  status changes 5%.
- Saffron: premium 723K -- hurtbox tests 36% (105 reject tests and 60 world
  compositions a band frame, Charmander's flames near the fighters),
  other logic 17%, render 13%, particles 13%.

No subsystem carries the ~9% to 1,120,000, and the digest-identical
candidates found add up to ~4-5%. The owner relaxed the cast gate to the
mirrors' number: **WORK P95 <= 1,250,000 with >=95% of frames within two
VBlanks**; the cast passes it on every stage. The heavy 4-identical mirrors
remain over it (`s23`: Bowser, Roy, Crash, Sonic, Meta Knight, Snake, Ike,
Palutena; `s35`: four Bowsers on Saffron 1,343,936, four Snakes on Hyrule
Temple 1,337,408), so the exact candidates go on: hurtbox worlds reused
across the pose hold's held tick, particle update and draw, clip
prefetch, the pose parser's precomputed segments, the bind's AObj reset
walk.

## Palutena's wide list keeps its variants; the mirrors on `s40` (2026-10-20)

Four Palutenas on Dream Land had gone from WORK P95 1,275,456 (`s36`) to
1,412,608 (`s37`): the evening's look changes (her hair shut at the nape,
the low's hair whole) took her low list from 3,385 to 3,698 words, past the
~3,560 at which a wide list's own learn copy still leaves room in its region
for two variant records. Her records went to 0 and every blink flip
materialized her list again (52 a 900-frame match against 20; frames of four
VBlanks 4 -> 24). Her high list (4,588 words, a duel's or a human player's)
never had room. The learn's copy, hold block and mask live only from
`ndsFtrLeanWideHoldBegin` to `ndsFtrLeanWideLearn` within one draw, so they
now sit in one heap buffer a heap generation for every slot (sized to the
longest list asked for, taken under the spares' floor) and the region keeps
its words for records (`76531c1a500`): `s39` 1,277,632, 24 materializations,
8 records; her duel's high list 8 records, 28 -> 10 materializations in 700
frames. Replay digests identical. A census of every content (four mirrors and
a duel each, `s40`) finds no other list without records.

The heavy mirrors on `s40` (1,960 frames; target P95 <= 1,250,000 with >= 90%
within two VBlanks):

| content | Saffron City (g7) | Hyrule Temple (g43) |
|---|---|---|
| c4 Bowser | 1,307,904 / 95.3% | 1,243,136 / 96.9% |
| c5 Marth | 1,196,288 / 97.6% | 1,257,408 / 96.5% |
| c6 Roy | 1,289,280 / 95.8% | 1,341,056 / 93.4% |
| c9 Crash | 1,268,096 / 96.4% | 1,315,008 / 95.7% |
| c13 Sonic | 1,274,048 / 95.2% | 1,268,160 / 96.2% |
| c16 Meta Knight | 1,312,640 / 95.3% | 1,317,376 / 95.8% |
| c18 Snake | 1,268,672 / 96.2% | 1,294,272 / 95.9% |
| c19 Ike | 1,269,888 / 96.6% | 1,347,712 / 92.4% |
| c20 Palutena | 1,279,040 / 95.6% | 1,379,968 / 89.9% |

0 native failures and 0 lean declines in every run. Lean materializations
are not what the rest of the tail is made of: with every frame that holds one
(+-1) set to the median, four Roys on Hyrule Temple would read 1,328,384
(8% of their top 5% hold one) and four Bowsers on Saffron 1,259,712 (20%).

## Hyrule Temple's mixes: the stage's dead bytes and a program a segment range at a time (2026-10-21, `s42`/`s44`)

The four-different heavy mixes on Hyrule Temple were memory-bound (owner
2026-10-21: free memory, small visual cuts allowed on Hyrule Temple only).
Mix A (Bowser, Crash, Ike, Dedede) left 21,468 B of heap at frame 0, so the
stage's 61,936 B program never loaded and the stage drew uncompiled (~330K
ticks a frame); the heap's low-water was 14,128 B, under the 25,600 B floor
and the source's GObj latch at 25 KiB. The census on `s40`:

* The figatree heaps are no spare buffers here: with 21 KB free the elastic
  motion cache is 8,000 B (45 hits, 608 misses a match), so the fighters'
  clips load straight into those heaps. Merging them into the cache frees
  nothing under this pressure.
* Hyrule Temple's geometry file (0x982, 59,856 B) still carries the display
  lists and vertex arrays its native packet was generated from: 40,224 B in
  three ranges, reached only by the layer DObjs' display-list pointers, which
  the binding check compares and nothing reads. The stage owner rejected 0
  frames, so the source draw that would read them never ran.

What changed:

* `stage_generate.py dead_spans` emits the ranges (`NDS_P5_DEAD_SPAN_ROWS`),
  falsifying on any other pointer into them or an image load inside one.
  Once the packet's assets are resident, a VS battle lends them
  (`ndsNativeStageLendDeadSpans`): `syTaskmanMalloc` takes them after the
  loan, best fit; the reloc registry's containment test stops calling an
  address inside them the file's (`ndsRelocLendDeadSpans`); the draw loop
  skips a lent layer's source draw (`ndsNativeStageLayerLent`, counted in
  every build: 0 in every run). Mix A's frame 0 heap 21,468 -> 60,668 B,
  low-water 14,128 -> 45,904 B, motion cache 8,000 -> 44,288 B, 40,184 of the
  40,224 B served.
* GX template format 9 appends each segment's slices of the run, patch and
  word tables and one hash over them. A heap short of the whole program loads
  the consecutive segment range worth the most that fits under the same
  keep-free (no-Z triangles weigh three), and the other segments draw
  uncompiled. The run memo dropped its world box, which only a culling
  program reads (never a VS battle): Hyrule Temple's program 71,092 -> 66,136
  B with its memos.
* `p5_rules.mk` stages a stage's template when the template changes; the
  stamp used to follow the C rows only, so format 9 left Hyrule Temple's old
  one in NitroFS (`s43`, discarded).

WORK P95 (frames within two VBlanks), 1,960 frames:

| mix (contents) | HT `s40` | HT `s44` (program loaded) | Saffron `s40` | Saffron `s44` |
|---|---|---|---|---|
| 4, 9, 19, 14 (A) | 1,686,208 (62.1%) | 1,648,128 (64.3%, segment 2) | 1,526,144 (86.8%) | 1,496,256 (89.5%, none) |
| 13, 10, 16, 3 | 1,595,136 (77.7%) | 1,381,504 (94.6%, whole) | 1,401,920 (93.8%) | 1,413,696 (93.4%, 0-3) |
| 2, 8, 18, 0 | 1,483,200 (72.8%) | 1,360,256 (89.7%, segment 0) | 1,318,272 (95.1%) | 1,285,120 (95.8%, 0) |
| 5, 6, 11, 17 | 1,529,984 (71.8%) | 1,334,080 (94.5%, whole) | 1,332,288 (94.8%) | 1,344,448 (94.4%, 0-3) |

The cast is unchanged (Hyrule Temple 1,209,152, Dream Land 1,069,440; replay
digests identical to `s40` after the one-frame pre-GO shift). Replay digests
are identical `s42` -> `s44` on every Hyrule Temple mix; mix A's moved from
`s40` because its heap no longer falls under the GObj latch (more effects
spawn: its median WORK +47K), and Saffron's mix A still falls under it
(low-water 20,912 B) and moves with any layout change. 0 native failures,
0 lean declines.

Mix A on Hyrule Temple stays the worst mix: with the latch gone it draws
every effect, and only segment 2 (11,128 B) fits beside the keep-free.

The vanilla VS stages lend theirs too (`s45`,
`scripts/stages/generate_stage_dead_spans.py`, also checking the stage's
other files for pointers into the ranges): Peach's Castle 8,624 B, Sector Z
14,576, Congo Jungle 11,680, Planet Zebes 9,152, Hyrule Castle 12,752,
Saffron City 14,464, Mushroom Kingdom 8,624; Yoshi's Island's geometry file
has external relocations and lends nothing. The cast on all eight: WORK P95
1,016,960-1,212,544, >= 97.3% within two VBlanks, 0 lent-layer skips,
replay digests identical to `s40`. Saffron City's mixes (P95, two VBlanks):

| mix | `s44` | `s45` |
|---|---|---|
| 4, 9, 19, 14 (A) | 1,496,256 (89.5%) | 1,541,760 (80.5%, segment 3) |
| 13, 10, 16, 3 | 1,413,696 (93.4%) | 1,377,728 (94.2%, whole) |
| 2, 8, 18, 0 | 1,285,120 (95.8%) | 1,256,896 (96.5%, 0-1) |
| 5, 6, 11, 17 | 1,344,448 (94.4%) | 1,341,568 (94.3%, whole) |

Saffron's mix A now stays above the GObj latch (low-water 20,912 ->
25,992 B) and pays for every effect it spawns, as Hyrule Temple's did.

## The contents' dead article geometry (2026-10-21, `s47`/`s48`)

The heaviest four-different roster (Crash, Sonic, Mr. Game & Watch,
Palutena; fkinds 0, 1, 0, 1) starved the heap on half the stages on `s46`:
low-water 4,728 B on Congo Jungle, 4,816 on Yoshi's Island, 7,516 on Saffron
City, 11,316 on Final Destination, under the 25,600 B floor, and with no
stage program on eight stages (`P2StageProgReason` 3: neither the whole
program nor any segment range fit).

A content's effects and weapons draw from the entry-effect packet, which
bakes their geometry: the runtime admits a list by comparing a DObj's list
pointer with `*storage + offset` and never reads it (a root that declines
its native draw goes to the stage body, which rejects it as `NO_PROGRAM` and
interprets nothing). The lists and vertices still sit in the special files.

* `scripts/p4/p4_dead_spans.py` emits, per file, the ranges the article roots
  of every enabled content reach (`articles.json`, unioned: Marth, Roy and Ike
  share 0xDA7), less what a model-part sidecar reaches (the fighter
  renderer's, Kirby's held weapons included). Only a content's own files
  (ids past 0x852) qualify. A range stays live when a pointer outside the
  ranges -- in the file or in any staged file -- names it other than at a
  root (a display-list call outside them would read it), or when an image
  load overlaps it; an external relocation keeps its 16 bytes, the chain's
  own words. Bowser's 7,584 B, Crash's 0x141A and Lanky's 0x1510 stay: the
  parent's weapon attributes in Main (Yoshi's star, Mario's fireball) name a
  vertex array or a list inside them as data. 22 rows, 9 files, 49,296 B.
* `battleship_ftmanager.c ndsP4LendArticleDeadSpans` runs after each
  content's files load (and from `ndsP4EntryFileEnsure` once Sonic's
  entry-only Tails plane file loads, 21,568 B of its 75,168): a VS battle
  lends every loaded, unlent file's rows. The reloc registry now holds holes
  for up to 12 files (32 ranges) a heap generation and refuses a file that
  is not this scene's own whole copy; the battle's span allocator holds 32.

Heavy roster, WORK P95 (frames within two VBlanks) and heap low-water,
1,960 frames (`s45`/`s46` -> `s47`; 16,224 B of article bytes lent at battle
start, Sonic's file not loaded in these matches):

| stage | P95 before | P95 `s47` | low-water before | low-water `s47` |
|---|---|---|---|---|
| Peach's Castle (0) | 1,331,904 | 1,270,336 (96.3%) | 45,360 | 44,592 |
| Sector Z (1) | 1,544,640 | 1,474,816 (90.3%) | 19,816 | 26,464 |
| Congo Jungle (2) | 1,360,576 | 1,408,000 (93.6%) | 4,728 | 7,936 |
| Planet Zebes (3) | 1,346,560 | 1,415,424 (93.4%) | 15,136 | 27,456 |
| Hyrule Castle (4) | 1,304,448 | 1,279,680 (96.0%) | 39,904 | 41,704 |
| Yoshi's Island (5) | 1,448,000 | 1,484,800 (89.2%) | 4,816 | 14,080 |
| Dream Land (6) | 1,359,104 | 1,336,064 (95.2%) | 24,476 | 37,372 |
| Saffron City (7) | 1,520,000 | 1,483,584 (90.9%) | 7,516 | 20,460 |
| Mushroom Kingdom (8) | 1,457,792 | 1,450,112 (92.2%) | 24,072 | 37,632 |
| Meta Crystal (13) | 1,214,528 | 1,207,232 (96.7%) | 42,164 | 44,044 |
| Duel Zone (14) | 1,070,848 | 1,065,920 (99.0%) | 45,984 | 43,936 |
| Final Destination (16) | 1,315,648 | 1,324,928 (95.7%) | 11,316 | 23,720 |
| First Destination (41) | 1,236,352 | 1,193,792 (98.0%) | 48,312 | 47,592 |
| Bowser's Stadium (42) | 1,211,840 | 1,194,368 (98.0%) | 38,820 | 49,860 |
| Hyrule Temple (43) | 1,708,928 | 1,704,320 (68.4%) | 22,976 | 25,920 |
| Dinosaur Land (44) | 1,465,280 | 1,485,248 (86.7%) | 13,036 | 27,456 |

Where the heap stays under the GObj latch the match keeps fewer effects, so a
stage whose low-water rose past it can read a higher P95 (Planet Zebes):
that is the cost of every effect drawing. Congo Jungle, Yoshi's Island,
Saffron City and Final Destination are still under the floor with this
roster, and
the roster is over the 1,250,000 stress target on ten stages: the next
passes are the roster's heap census and its profile. Hyrule Castle's two
lean declines are Mr. Game & Watch's (`nNDSFtrLeanDeclineStale`, bit 16 of
`gNdsFtrLeanDeclineReasons[41]`): a fresh list whose textures were not
resident at its guard.

### Their images, Yoshi's Island, and the entry roots' corners (`s49`/`s50`)

* The packet bakes the articles' texels too (`sNdsEntryEffectTexels*`, and
  `nitro:/p4/entry_texels.bin` for the contents), so the images an article
  root's lists load are dead with the lists: `p4_dead_spans.py` adds them,
  keeping any image an MObj's image or palette table names (a material root
  takes its TLUT from the live MObj). 66 rows, 12 files, 89,888 B; Crash's
  14,304, Palutena's 18,848, Sonic's 28,224 when his file loads. The
  registry holds 16 files / 64 ranges a generation, ascending, and the span
  allocator 64.
* Yoshi's Island's geometry file lends its dead lists after all: an external
  relocation keeps its 16 bytes instead of excluding the file (4,816 B).
* `generate_nds_entry_effects.py --p4` emits the entry roots
  (articles.json "entry": the vehicles, gates and waves, named by no other
  row) and the runs of each content's corner block only they draw.
  `ndsRendererEntryP4RetireEntries` marks a content's entry roots retired and
  lends the runs' position, s, t and colour spans: at GO once no entry root
  drew for two presented frames (scvsbattle.c, beside the startup-only
  texture retirement), and at once when an entry-only special file does not
  fit (Sonic's Tails plane: his roots cannot draw). A retired root's draw is
  nothing. Sonic 9,840 B, Crash 2,904, Palutena 576 (13,320 for the heavy
  roster).

Heavy roster on `s50`, WORK P95 (two-VBlank share) and low-water:

| stage | P95 | low-water |
|---|---|---|
| Peach's Castle (0) | 1,269,184 (96.3%) | 51,056 |
| Sector Z (1) | 1,426,304 (92.4%) | 36,316 |
| Congo Jungle (2) | 1,396,224 (93.1%) | 31,108 |
| Planet Zebes (3) | 1,403,392 (93.4%) | 52,972 |
| Hyrule Castle (4) | 1,253,312 (96.8%) | 50,784 |
| Yoshi's Island (5) | 1,449,792 (91.6%) | 36,780 |
| Dream Land (6) | 1,322,368 (95.5%) | 51,360 |
| Saffron City (7) | 1,411,776 (92.4%) | 37,180 |
| Mushroom Kingdom (8) | 1,417,344 (93.0%) | 50,528 |
| Meta Crystal (13) | 1,203,520 (96.8%) | 51,152 |
| Duel Zone (14) | 1,059,776 (99.1%) | 53,808 |
| Final Destination (16) | 1,306,816 (95.7%) | 49,860 |
| First Destination (41) | 1,176,896 (98.2%) | 59,392 |
| Bowser's Stadium (42) | 1,181,952 (98.5%) | 58,832 |
| Hyrule Temple (43) | 1,706,112 (68.2%) | 42,636 |
| Dinosaur Land (44) | 1,446,848 (88.9%) | 39,268 |

Every stage now stays above the 25,600 B floor with this roster (it was
under on six), 0 native failures. The P95 is the roster's own: the heavy
frames are lean materializations (27 of the 43 over-gate frames in the
`s48` profile on Congo Jungle), audio card reads and the effects, and on the
stages whose program still does not fit (Congo Jungle, Final Destination)
the uncompiled stage path, ~290K cycles a frame.
