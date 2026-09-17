# STG did not regress by definition, and N05.03's falsifier is now sized

Two questions settled from evidence already on disk, no new build.

## 1. "STG was 175K in P1, why is it 350K now?"

**The bucket definition never changed.** `git log -S'gNdsTickHudStageTicks +='`
over all of `src/` returns exactly one commit, `2724b2d8c1b` (2026-07-21). Four
accumulation sites then, four now: `src/port/reloc_backend_movement.c:13667`
(display commit), `:13796` (DObj traversal), `:14091` (prepare owner), `:14134`
(finish owner). `487c774fe69` (2026-07-27) rewrote the *values* those sites
accumulate without changing their number, and its own Task 103 E3 partition
proved the four **exhaustive and non-nesting** — the split closed to 192 ticks,
0.05%, against that build's STG of 393,472.

**It is not the roster.** Two fighters on today's source
(`smash64ds-battle-playable-tickhud-hwtri` built into `builds/build-p1-compare`,
`NDS_R2_ANIM_Q_ITCM_ON=0` because the pack overflows ITCM by 320 B on today's
tree) reads STG **335,296** against the four-fighter **339,968**.

**It is genuinely stage work, not misfiled fighter work.** Summing self cycles
per symbol from `…/2026-09-16_p2-2p8-n0409-profile/arm9-profile.csv` joined to
the linked ELF's symbol ranges, symbols named `*Stage*`/`Task36` carry
**220,519 tk/fr of self time**; the bucket is 339,968 and the difference is
shared leaves (`memset`, `memcpy`, soft float, `MtxMul*`) called from inside the
stage brackets. `ndsRendererAdapterCommitNativeStageDisplay`
(`src/port/renderer_adapter_stage.c:3876`) only commits a display that **is**
one of the workspace's own stage segments; fighter, item and effect displays
fall through the loop and return `FALSE`.

### What actually happened, same two-fighter roster

| bucket | 2026-08-10 (`docs/PERF_LEDGER.md:7242`) | today | delta |
|---|---:|---:|---:|
| FTR P50 | 302,272 | 168,896 | **-133,376** |
| STG P50 | 189,184 | 335,296 | **+146,112** |
| **WORK-H P50** | **952,512** | **879,424** | **-73,088** |
| ALL P50 | 1,118,336 | 1,117,568 | — |

STG roughly doubled and **total frame work fell 73,088**. The 2026-08-16 reading
of 175,424 (`docs/archive/P1_EXECUTION_BOARD.md:936`) sits in the same era; the
2026-07-27 figure of 393,472 is pre-`whole-match` boundary and is not comparable
(see the 2026-08-05 instrument change).

### Leading cause — stated as a hypothesis, not a result

The window 2026-08-17..2026-09-16 is the whole P2 native-draw campaign: **28
commits to `src/nds/nds_renderer_native_owners.c`**, which holds the stage
emitters, plus native stage wallpapers (`a4e157ef528`), 26 item owners, ~12
effect owners, Ness unlit vertex-colour runs and Kirby's copy hat. The stage now
emits geometry the August build did not. Current stage leaves:

```
EmitNoZTriangle 18,198 + EmitNoZVertex 14,155 + LoadNoZMatrix 11,290 = 43,643
BuildPersistentStageWorldMatrix 15,010 · NativeStageBeginRun 16,447
CommitNativeStageSegment 27,835
```

**Not proven:** whether the FTR fall and the STG rise are one transfer or two
independent movements. No August run recorded
`gNdsStageGCDrawAllLoopCapturedDisplayCount`, so the display-count growth
(63.95/frame today, 78.4% of captures non-stage) cannot be compared backwards
without a build against the August owner set.

## 2. N05.03's falsifier, sized from the existing profile

The N05.01 write-up recorded the lever as -12,000..-18,000 with this falsifier:
*"if the recompute pulls the same `FTParts` cache lines anyway, only the issue
slots go — 56% of 20,349 is about -6,700."* The per-PC split settles the first
half of that without a build.

`ndsFTParamsInvalidateSubtree` (`src/port/reloc_backend_compat_shims.c:2955`,
with `FlatWalkFor`/`FlattenDescendants`/`InvalidateFlatParts` all inlined into
it — they have zero PCs of their own):

```
20,030 PCs   insn 938,263   cycles 5,250,005   CPI 5.60   20,348 tk/fr
  issue-only floor (1 cyc/insn)  =  3,636 tk/fr
  stall excess                   = 16,712 tk/fr  (82%)
```

The two hottest PCs are loads, not stores: `0x01ffcc8c` (`0x68b66933`, the
`next`-pointer chase) at **CPI 41.8** and `0x01ffcd6e` (`0x6818`) at **CPI
48.2** — full misses to main RAM, one per part.

So the bucket is a **read-miss walk**, and the lever's value is bounded by:

- **ceiling -19,700** — 20,348 less the ~3% of parts that are recomputed anyway
  (474.5 part-word clears/frame against 14.3 matrix recomputes)
- **floor -3,636** — the issue slots, if every walked part's first cache line is
  pulled by some other consumer in the same frame regardless

`FTParts`'s first 32-byte line holds `transform_update_mode`, the
`unk_dobjtrans_*` union, `next`, `flags`, `joint_id` and `is_have_anim`, so any
other per-part walk in the frame touches it; `mtx_translate` at 0xE0 does not.
**Which end of that range applies is still an A/B question**, and the recorded
experiment — a `volatile u32` route switch giving byte-identical `.text` in both
arms, asserted by `scripts/compare-elf-sections.py --max-diff 1` — is still the
way to answer it. What has changed is that the 56%-of-20,349 figure in the
falsifier was a guess at the store/load split; the real split is 18% issue, 82%
load stall.

## 3. Implementation blocker, now concrete

An O(1) generation stamp has to change how the latches are read, and the reads
are in decomp text: **23 sites** across `gm/gmcollision.c` (16), `ft/ftmanager.c`
(3), `ft/ftparam.c` (3) and `ft/ftcomputer.c` (1). The port compiles
`gmcollision.c` verbatim — `src/import/battleship_gmcollision.c:220` is
`#include "../../decomp/BattleShip-main/decomp/src/gm/gmcollision.c"`, the
pristine path.

The supported route already exists and is used by twelve other files: add
`scripts/import-overlays/battleship/src_gm_gmcollision.patch` (and the three ft
files), then switch each importer to the overlay include form, exactly as
`src/import/battleship_sys_objanim.c:56` does with
`#include <battleship_overlay/src/sys/objanim.c>`. No `decomp/` file is edited;
`check-decomp-pristine.ps1` stays green.
