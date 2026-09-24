# VS character select: every 3D preview missing on the all-content ROM (2026-09-23)

Owner report, on the lab ROM `build-p2p8-ifc-1p`: "in CSS all fighters are
invisible". The VS character select showed each panel's series emblem and no
fighter; the 1P character select drew its preview normally.

## Cause

`ndsMNPlayersVSPreviewInit` builds the VS preview pools in
`ndsMNPlayersVSPreviewInitResidentPools`: a shared-closure arena, four 80 KiB
closure blocks, the shared trees, and then the animation cache's CSS working
set (`ndsR2AnimCacheReserveCSSWorkingSet`: the row-0 clip of all twelve kinds,
52,000 B). That reservation also keeps `NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE`
(128 KiB) free behind it. When the scene heap cannot, pool init returns FALSE,
`sNdsPlayersVSPreviewActive` stays FALSE, and every VS preview is switched off
for the visit: no halt, and no counter any verifier read.

The keep-free is not oversized for this scene: with previews live the character
select takes 127,492 B more after the reservation (VS-only shell, 18 preview
loads, free 381,136 at the reservation and 201,644 at exit, 52,000 of the
difference being the reservation). Relaxing it would trade the empty panels for
an out-of-memory halt later in the screen.

The all-content configuration (`smash64ds-p2-shell-freeplay-hwtri` and the
published `smash64ds`: one flag block, two output names) carries the 1P campaign
in its static image, 226,848 B more than the VS-only shell, and its boot arena
is smaller by the same amount (892,416 vs 1,117,952 B on the lab ROMs). The
published 2026-09-22 ROM cleared the reservation by 5,296 B; Phase 1's code
(+29,216 B of image) took HEAD under it.

| ROM | image end | free at the reservation | needed | VS previews |
|---|---|---:|---:|---|
| published `smash64ds.nds`, 2026-09-22 17:14 | 0x022ca3e0 | 188,368 | 183,072 | drawn |
| HEAD 94b02906707, all-content (published config) | 0x022d1600 | 163,792 | 183,072 | **none** |
| HEAD + this fix, all-content | 0x022d1620 | **204,752** | 183,072 | drawn |
| `build-p2p8-ifc-1p` (HEAD + menu walk + lab flags) | 0x022d32e0 | 155,600 | 183,072 | **none** (tour drew 0x000 of 0xfff) |
| HEAD + this fix, same lab flags | 0x022d3320 | **196,560** | 183,072 | drawn (tour drew 0xfff of 0xfff) |
| `build-p2p8-s2c-shipcheck` (VS-only shell) | 0x0229bcc0 | 381,136 | 183,072 | drawn |

"Needed" is 52,000 + 131,072 (the working set plus the keep-free), before the
16-byte alignment.

## Fix (`src/import/battleship_mnplayersvs.c`)

- `NDS_PLAYERS_VS_SHARED_RESIDENT_BYTES` 64 KiB -> **24 KiB** for the compact-
  preview configuration. Its content is a fixed list of seven trees
  (`sNdsPlayersVSSharedResidentAssetIDs`) that measure **15,232 B** at VS CSS
  exit on the VS-only shell and on the all-content ROM alike; 24 KiB keeps
  9,344 B of headroom. The raw-tree profiles (which also pin YoshiModel) keep
  64 KiB. The 40,960 B returned come out ahead of the reservation, one for one
  (163,792 -> 204,752). A shared tree that no longer fits halts in
  `ndsSyMallocOverflowHalt` naming arena `'CSS\0'`, so an undersize cannot
  repeat this failure's silence.
- `gNdsPlayersVSPreviewPoolFailCount` / `...PoolFailStage`: boot-lifetime
  witness of a pool-init refusal (1 shared arena, 2 closure block, 3 shared
  tree, 4 animation working set). `scripts/verify-p2-shell-loop.ps1` prints it
  in LOOPDONE (`respool`, `respoolstage`) and requires 0.

Cost: +40 B `.text`, image end +32 B.

## Verification

- Owner route on the all-content configuration without the walk
  (`tools/run-owner-css.ps1`, the 2026-09-06 CSS feed: Title -> VS -> CSS,
  select Link, Yoshi, Pikachu): cumulative preview triangles 12,162 / 23,140 /
  33,550 on the published ROM, **0 / 0 / 0** on HEAD, **12,162 / 23,140 /
  33,550** on HEAD + fix. Screenshots:
  `artifacts/visibility/2026-09-23_css-bisect/owner-route-*-{published-0922,head-ship-before,head-ship-after}.png`
  and `pikachu-before-after.png`.
- Lab ROM with the fix (HEAD + `NDS_P2_MENU_WALK=1 NDS_FTR_LEAN_ADMIT_LAB=1
  NDS_FTR_LEAN_ADMIT_DEFAULT=2 NDS_IF_GAMESTATUS_COMPACT=1`, the configuration
  the owner played), `tools/run-css-heap.ps1`: reservation granted (52,000),
  walk tour `kind=0xfff drew=0xfff`, 19 acquisitions, 0 failures, 0 capacity
  failures, shared arena 15,232 B used. Window captures
  `artifacts/visibility/2026-09-23_css-bisect/cssfix-lab1p/` (sheet:
  `cssfix-lab1p-sheet.png`) show Link, Luigi, Samus and CP Fox drawn.
- Free heap at VS CSS exit on that ROM is 15,772 B: the screen took 128,788 B
  after its reservation began (the VS-only shell took 127,492 with 18 loads),
  so the 128 KiB keep-free is what stands between this screen and an
  out-of-memory halt, with ~2 KB to spare for this workload. On the published
  configuration the same arithmetic gives ~24 KB at exit after the fix, against
  ~7.6 KB on the 2026-09-22 ROM.

## Tools

| tool | use |
|---|---|
| `tools/run-owner-css.ps1 -Rom -Elf -Tag` | owner CSS route on a no-walk ROM, heap at the reservation |
| `tools/run-css-heap.ps1 -Build [-Target]` | walk ROM: free at the reservation, at CSS exit, tour masks, shared/block usage |
| `tools/run-css-probe.ps1 -Build [-Target] -DelaySeconds` | walk ROM: late attach, all preview/loader counters |
| `tools/shots-over-time.ps1 -Build -Tag` | walk ROM: window captures at fixed offsets |
| `tools/build-wt.ps1 -Build [-MakeArgs]` | build in the detached HEAD worktree used here (the main tree carried an agent's in-flight edits) |

gdb reads taken right after `finish` in `run-css-heap.ps1` (`RESERVE_DONE`) are
stale on this melonDS fork (dirty dcache lines); only the entry reads and the
CSS-exit reads are used above.

## Where the rest of main RAM goes: the pre-arena census

The taskman arena is the largest block libc can still give when the chooser first
runs, so everything libc allocated before that point is arena the character
select (and every battle) lacks. On the all-content configuration the arena starts
~210 KB above the static image's end. `tools/run-prearena-census.ps1` (breaks at the
entries of `malloc`/`calloc`/`aligned_alloc`/`realloc` for >= 2 KiB until the first
`ndsTaskmanArenaBytes` call; `prearena-census.txt`) names them:

| caller | bytes |
|---|---:|
| `dvmDiscCacheCreate` (calico disc cache), 2 x 65,536 | 131,072 |
| `portCoroutineCreate`, 7 service-thread stacks x 16,384 (`NDS_OS_SERVICE_STACK_SIZE`, `src/port/libultra_os.c`) | 114,688 |
| `nitroromOpen` | 8,416 |

Two measurable levers for Phase 3 (A7), neither taken here: per-thread stack sizes
from a current stack high-water profile (`NDS_TASK20_STACK_PROFILE`; the last one is
P1-era, 2026-07-18) instead of one 16 KiB size for every service thread, and the
disc-cache size weighed against CSS pack and BGM stream read cost. Each byte either
returns goes to the arena one for one.

## Not fixed here

- Nothing checks the walk tour's `kind == drew` automatically:
  `scripts/menus/probe-p2-shell.ps1` prints `CSSTOUR` for a reader, and the shell
  loop verifier runs the VS-only shell, where the heap never gets close. The
  all-content configuration has no walk arm that would have caught this.
- The all-content CSS margin is now 21,680 B and is spent by static image
  growth one for one.
- The post-reservation figure (128,788 B) is the walk's workload: slot 0 touring
  all twelve kinds with CP Fox in slot 1. Four different heavy fighters held in
  all four slots at once was not measured.
