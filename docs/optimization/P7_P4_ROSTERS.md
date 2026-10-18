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
   (`gNdsEntryEffectGxOverride`, 0 = the CPU path).
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

Correctness: the replay digests (DGSA/DGSB) of every sweep run are identical
across the builds (1,960 of 1,960 frames, c20/c4/c16); the wings' GX path
against the CPU path and the blink before/after were captured frame for
frame (`artifacts/visibility/p6-vfx/wings-*`, `blink-*`): the blink frames
are pixel-identical, the wings differ by scattered rounding pixels.
