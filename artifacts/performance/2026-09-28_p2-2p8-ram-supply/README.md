# P2-2p8 battle RAM supply (2026-09-28)

Three read-only investigations sized where battle RAM can come from; the first
result is implemented. Reports:

- `lane1-stage-ground-files.md`: every VS stage's resident ground tree carries
  its 158,928 B wallpaper container, whose pixels the DS never reads (BG2
  streams the native wallpaper). Compact maps reclaim 158,144 B per stage.
- `lane2-common-item-effect-files.md`: 129,300 B more in the IF, effect and
  misc files with no new reader, once the native owners stop comparing source
  offsets and sizes directly.
- `lane3-renderer-retirement.md`, `lane3-r1-p1safe.md`: 163,713 B of old
  renderer machinery; 81,793 B (config-dead modes + the startup diagnostics
  reset) needs no owner decision; the rest waits on the 1P question.

## Banked: T1 compact ground maps

Implementation notes: `lane1-t1-implementation.md` (flag
`NDS_P2_COMPACT_GROUND_MAPS`, default on for the P2 four-CPU, shell, free-play,
shell-loop and published targets; P1 stages the original maps and compiles
byte-identical objects). The generator is
`scripts/stages/generate_compact_ground_maps.py` (19 host tests,
`scripts/stages/test_compact_ground_maps.py`).

Full-content lab ROM (`build-p2p8-s1`), control `b1_*` = same source without
T1, candidate `t1_*`; replay IDENTICAL on all nine:

| Roster / stage | P50 | P95 | P99 | motion cache | reads | model rebuilds | spare refusals |
|---|---|---|---|---|---|---|---|
| DK/Samus/Link/Kirby, Dream Land | 950,848 -> 947,072 | 1,330,880 -> **1,309,568** | 1,628,992 -> 1,505,024 | 68,064 -> 167,008 | 448 -> 306 | 54 -> 39 | 37 -> 0 |
| Fox/Pikachu/Ness/Samus, Saffron | 1,158,272 -> 1,157,632 | 1,583,104 -> **1,574,976** | 1,769,344 -> 1,766,400 | 77,888 -> 236,032 | 447 -> 286 | 35 -> 35 | 0 -> 0 |
| Luigi/Kirby/Ness/Purin, Yoshi's Island | 1,095,040 -> 1,092,928 | 1,474,624 -> **1,463,360** | 1,657,024 -> 1,642,048 | 69,216 -> 191,648 | 461 -> 295 | 28 -> 28 | 6 -> 0 |
| Captain/Yoshi/Kirby/DK, Jungle | 1,083,136 -> **980,096** | 1,465,536 -> **1,338,560** | 1,804,096 -> 1,620,096 | 42,592 -> 84,320 | 531 -> 412 | 61 -> 49 | 47 -> 0 |
| Kirby x4, Dream Land | 903,872 -> 902,016 | 1,204,160 -> 1,204,416 | 1,389,504 -> 1,388,416 | 258,048 (cap) | 82 -> 82 | 36 -> 36 | 0 -> 0 |
| Pikachu x4, Zebes | 1,001,088 -> 1,001,280 | 1,427,008 -> 1,427,392 | 1,646,208 -> 1,642,624 | 258,048 (cap) | 74 -> 74 | 23 -> 23 | 0 -> 0 |
| Fox/Pikachu/Ness/Samus, Dream Land | 985,472 -> 983,040 | 1,407,616 -> **1,399,424** | 1,669,760 -> 1,661,888 | 130,560 -> 258,048 | 392 -> 290 | 28 -> 28 | 0 -> 0 |
| Luigi/Kirby/Ness/Purin, Castle | 1,011,520 -> 1,011,968 | 1,407,936 -> **1,397,056** | 1,617,664 -> 1,605,184 | 106,176 -> 258,048 | 345 -> 224 | 42 -> 42 | 0 -> 0 |
| Captain/Yoshi/Kirby/DK, Mushroom | 1,155,328 -> **1,114,624** | 1,544,832 -> **1,486,272** | 1,775,040 -> 1,737,536 | 71,584 -> 161,760 | 470 -> 375 | 69 -> 55 | 39 -> 0 |

Jungle and Mushroom gain most because the stage GX template (which needs
its body plus 62,020 B of heap) now loads: Jungle STG median 266K -> 164K,
Mushroom 177K -> 150K. Elsewhere the gain is the elastic motion cache and the
lean spare buffers taking the freed bytes. Rosters already at the cache's
258,048 B cap are flat; raising the cap is the next step.

Gate ROM (`t1_gate` vs `el1_gate`): engaged (compact map count 1, wallpaper
keyed on all 1,973 frames, Dream Land reloc PASS with mask 0x1F, native
failures 0), true free low-water 183,564 -> 341,708 B, replay identical after
one resync; P50/P95 929,728/1,290,176 -> 930,304/1,294,784 (the gate's cache
was already large and is now capped; T1 adds two per-frame lookups).

Published configuration (`fourkind-argmax-t1.txt`, argmax roster, menu walk):
full match (2,043 presents), Results, allocation overflow 0, relocation and
storage failures 0, general-heap low-water 59,048 -> 168,504 B.

UNPROVEN: Training Mode (1P) with compact maps -- lane 1 added a
Training-only wallpaper region; no device probe exists for Training yet. The
owner CSS reserve tool failed to open its GDB listener on four runner slots;
the static image grew ~0.8 KB against a ~70 KB CSS reserve margin.

## Banked: R1 retirement (legacy fighter executors out of P2 targets)

Lane 3's P1-safe patch (`lane3_r1_p1safe_sources.patch`,
`lane3_r1_p1safe_makefile.patch`, `lane3-r1-p1safe.md`) adds
`NDS_FIGHTER_LEGACY_EXEC`: 0 for the P2 targets (four-CPU, shell, free-play,
shell-loop, scene-walk, published), 1 everywhere else. At 0 the fighter draw
path drops hierarchy mode 7, the per-root hardware executor and CPU triangle
rasteriser behind it, and the resident raw-path corner tables; an `#error`
refuses 0 outside the production fighter configuration. A poked lab mode 5-7
takes the fail-closed reject instead of an executor.

Image: gate ELF text -38,912 B, bss -9,984 B, heap start -48,896 B; lab ELF
heap start -139,520 B.

| Run | P50 | P95 | P99 | heap low-water | replay |
|---|---|---|---|---|---|
| Gate (`t2_gate` -> `t3r_gate`, same tree +/- R1) | 935,360 -> 934,592 | 1,294,208 -> **1,289,280** | 1,519,296 -> 1,461,568 | 337,612 -> 386,508 | IDENTICAL after one resync |

Gate: native failures 0, compact map engaged, motion cache 329,824 -> 378,720 B,
reads 224 -> 217.

Lab (nine rosters, `t2_*` control vs `su_*` = R1 plus the lab-only status
timers of `2026-09-28_p2-2p8-status-change`, which cost ~+2.7K at P50): replay
IDENTICAL on all nine, heap low-water +47,872 to +48,048 B on every roster,
motion cache +47,872 B (to the 458,752 B cap on Kirby x4 and Pikachu x4),
reads down on every roster; P95 within -5.8K..+10.8K, i.e. the timers' band.

Published configuration (`fourkind-argmax-r1.txt`): full match (2,043
presents), Results reached, allocation overflow 0, relocation/storage failures
0, general-heap low-water 168,504 -> 215,352 B.

Note: the lab sweep ROM must be built with `NDS_P2_PIKACHU/YOSHI/NESS/PURIN=1`
(the four-CPU target leaves them 0); without them every roster naming one of
those kinds wedges melonDS before frame 98. Six lab rosters report native
failures (36-268 a match) identically with and without R1 -- a pre-existing
lab-roster defect, logged for its own investigation.
