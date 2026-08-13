# Anomaly 2 fix — qualification, 2026-08-13

The change: `NDS_AOBJ_EVENT32_NORMALIZED_MAX` 1024 → 2048 plus
`gNdsAObjEvent32NormalizedHighWater`, in `src/import/battleship_sys_objanim.c`.
Attribution and derivation are in `ANOMALIES.md` beside this file.

## Candidate

| | |
|---|---|
| build | `builds/build-c133-aobjcap5`, `smash64ds-battle-playable-tickhud-hwtri` |
| arm | `NDS_R2_BOTH_CPU 1`, **`NDS_R2_SOAK_MATCH_MINUTES 5`**, `NDS_TICK_HUD_DRAW 1` |
| control | `builds/build-c132-stress5` — same target, same arm, pre-change source |
| run | `sample-tick-hud-buckets.ps1 -RingDump -Samples 8448 -StartFrame 438`, DLDI **ON**, frames **439–8887**, `slips=0` |
| artifacts | `soak5-c133.{json,-rows.csv,-run.log}`, `boot-headroom-c133.txt`, `boundary.log` |

**The soak length rides its own flag, not the gate seed**, and the flag is not
left configured: `build-c133-aobjcap5` is a lab build, the published targets and
both root ROMs are untouched.

## The size, priced before it was written and then measured

| | control | candidate | delta |
|---|---:|---:|---:|
| text | 981,348 | 981,356 | **+8** |
| bss | 1,455,560 | 1,463,752 | **+8,192** |
| `fake_heap_start` | `0x02269804` | `0x0226b804` | +8,192 |
| **proven boot headroom** | 176,128 | **167,936** | −8,192 |

+8,192 is exactly 1,024 extra entries × 8 bytes — the predicted figure, not a
rounded one. The +8 of text is the high-water compare.

## The counters — this is what the fix is for

| counter | control (5 min) | **candidate (5 min)** | required |
|---|---:|---:|---|
| `gNdsAObjEvent32NormalizedHighWater` | — (did not exist) | **1,019 of 2,048** | plateau, far from cap |
| `sNdsAObjEvent32NormalizedCount` | 1,019 of 1,024 | 1,019 of **2,048** | — |
| `gNdsAObjEvent32NormalizeFailCount` | 0 | **0** | 0 |
| `gNdsAObjEvent32NormalizeLastFailReason` | 0 | **0** | 0 |
| `gNdsTaskmanGeneralHeapFreeMin` | 70,384 | **70,000** | ≥ 32,768 |
| `gNdsTaskmanArenaAllocFailCount` / `SyMallocOverflowCount` | 0 / 0 | **0 / 0** | 0 |

**The corpus is 1,019 and it is now proven to be 1,019, not merely "at least
1,019".** That distinction is the whole point of the run: at capacity 1,024 the
count and the cap were indistinguishable — a table pinned at 1,019 because the
scene has 1,019 distinct commands looks exactly like a table pinned at 1,019
because it is about to overflow. With 2,048 slots the same match still stops at
1,019, so the demand is real and the margin is **1,029 spare slots (50.1%)**
instead of 5 (0.5%). Heap free-min falls 384 bytes and stays 2.1x the floor.

## Negative control — the other anomaly did NOT move

`gNdsObjAnimRunawayCount` = **50** on the candidate, against **50** on the
control's five-minute match. Byte-identical. The capacity change did not touch
the runaway path, which is what a fix confined to its own seam should look like,
and it independently refutes any coupling between the two anomalies: the ledger
never overflowed, so it was never causing the parser faults. The last recorded
`gNdsObjAnimRunawayScript` is `0x023633EA` — **2 mod 4**, the same `event16`
shape `ANOMALIES.md` attributes.

## Performance — inside the cross-build floor, both percentiles

| bucket | control (c132-stress5) | candidate (c133-aobjcap5) | delta |
|---|---:|---:|---:|
| **`WORK-H` P50** | 929,344 | **932,224** | +2,880 |
| **`WORK-H` P95** | 1,205,760 | **1,208,000** | +2,240 |
| `ALL` P95 | 1,678,720 | 1,678,784 | +64 |
| `FTR` P95 | 325,632 | 325,952 | +320 |
| `STG` P95 | 165,760 | 166,464 | +704 |

`VERIFYING.md` calibrates the cross-build floor at **±14,080 P95** and ~5,700
P50; both deltas are well inside it, so this is placement noise and not a cost.
VBlank histogram over 8,887 presented frames: **2 × 7,412, 3 × 1,388, 4 × 72,
5+ × 15, max 26**, against the control's 2 × 7,415, 3 × 1,394, 4 × 58, 5+ × 19,
max 26 — the same distribution.

No performance A/B was owed here (no active-frame work changed), and the run was
taken anyway because it was the same run that carried the counters.

## Verifier

`scripts/verify-all.ps1 -Profile Boundary` — result recorded in `boundary.log`
beside this file. Boundary is the right width: the change is battle-only and
touches no normal/shared startup path.

## What this fix does NOT claim

- It does not fix, mask, or affect the DObj-parser runaway. That is a separate
  seam with its own evidence, and it is handed forward.
- It does not change a single consumed gameplay value: with
  `NormalizeFailCount` 0 in every run before and after, no attach was ever
  skipped, so the ledger's capacity is unobservable to the game. The change is a
  margin, not a behaviour.
- It does not remove the underlying growth. The ledger still fills with scene
  coverage; it now has room for twice the corpus and an instrument that reports
  the peak instead of the last scene's leftovers.
