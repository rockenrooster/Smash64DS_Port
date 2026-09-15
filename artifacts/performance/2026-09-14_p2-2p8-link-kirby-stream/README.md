# P2-2p8 Link/Kirby source-normalized animation stream coverage

Date: 2026-09-14

Verdict: **KEEP. Source/interrupt tail cost drops materially; overall four-CPU
performance remains RED.**

The four-kind Donkey/Samus/Link/Kirby stress already used the BPS1
source-normalized animation stream for Donkey and Samus, but Link and Kirby were
absent from the pack generator and NitroROM prune manifest. Their animation
misses therefore took the raw O2R acquisition/normalize path even though the DS
stream representation already preserves the same AObj16 command semantics.

This checkpoint extends the existing generator and build manifest to Link and
Kirby, including Kirby's two live `FTKirbyCopyAnim` IDs. The four source AObj32
exceptions are named explicitly and continue through their existing loader;
Samus's two spline clips remain explicit O2R exceptions. The runtime reader,
cache size, 128 KiB scene-ledger reserve and gameplay code are unchanged.

## Generator equivalence

`generate_battlepack_anim.py --verify` passes over the expanded corpus:

- 1,139 AObj16 clips / 20,844 entry-point scripts;
- 298,270 commands and 403,507 target words compared;
- BPS1 28,561 slots checked, 0 mismatches;
- Link AObj32 `0x4DE/0x4DF` and Kirby AObj32 `0x584/0x585` are explicit skips;
- Samus spline `0x3F5/0x3F6` remains on the source O2R path.

BPS1 grows from 1,772,784 B to 2,630,496 B (+857,712 B ROM). It does not add
ARM9 residency.

## One-minute four-distinct-CPU stress

Configuration-exact Donkey/Samus/Link/Kirby Dream Land stress, frames 2..1973,
source clock 60 -> 1 (59/60 seconds), item overrides zero. The retained
split-packet checkpoint is the immediately preceding whole-match reference.

| Bucket | Split-packet checkpoint P50/P95 | Link/Kirby stream P50/P95 | P95 delta |
|---|---:|---:|---:|
| ALL | 2,237,952 / 3,358,528 | **2,237,888 / 3,358,080** | -448 |
| FTR | 640,000 / 781,056 | **631,552 / 771,840** | -9,216 |
| SRC | 564,736 / 1,629,184 | **566,592 / 1,267,136** | **-362,048** |
| GCRA | 558,848 / 1,623,360 | **560,576 / 1,261,056** | **-362,304** |
| SINT | 260,608 / 1,150,080 | **260,992 / 838,144** | **-311,936** |
| WORK-H | 1,907,200 / 3,013,056 | **1,901,568 / 2,721,408** | **-291,648** |

The median is intentionally almost unchanged: this removes repeated animation
miss normalization from the tail rather than changing steady-state simulation.
Engagement confirms the route change: direct animation reads fall 359 -> 5,
stream reads rise 308 -> 662, stream misses fall 359 -> 5, and stream failures
remain 0.

Resource/correctness evidence remains stable: general-heap low-water 118,752 B
(93,152 B above the 25,600 B floor), libc runtime high-water 32,896 B, animation
arena 5,216 B reserved / 4,208 B used, native failures/direct rejects 0/0,
graphics-heap overflow/no-room 0/0, weapon pool 2/10 with zero refusals, pose
bind-full 0, all four fighter slots draw (`0xF`), and cadence violations are 0.

The final configuration-exact `verify-p2-four-fighter-stress.ps1 -NoBuild`
replay exits 0 and reports that the four-CPU standing stress passed correctness,
cadence, native-owner and memory gates. Candidate ROM SHA-256:
`71236C5FA06C1C07365A159894D336BF1AE72BBD067B357A66AD76983FFB110F`.

## Boundary umbrella

`verify-all.ps1 -Profile Boundary` passes toolchain/docs/untracked-dependency,
native-owner wiring, compact-pack and generator-staleness stages, then stops in
`check-architecture.ps1` on the pre-existing read-only working-tree condition
`?? decomp/alt_assets/`. The exact four-CPU runtime child above is therefore the
retained runtime gate for this checkpoint.

## Permanent files

- `candidate-stress.json` / `candidate-stress.csv`: final whole-match timing.
- `candidate-stress-coverage.json`: same-run roster, clock and natural-input proof.
- `candidate-stress-memory.json`: same-run native/resource/engagement ledger.

The final exact replay's exit code was 0; its transient console log is not part
of permanent evidence.
