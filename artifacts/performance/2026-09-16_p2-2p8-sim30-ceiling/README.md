# P2-2p8: pricing the 30 Hz simulation ceiling

Owner ruled 2026-09-16 that **30 FPS at four players is required**. This prices
the lever `PROJECT_GOAL.md` already nominates for exactly this situation.

**This is a ceiling measurement, not a candidate.** The build is uncompensated:
the match advances one logical tick per present and plays at half speed. It
measures what the work is worth and nothing else.

## Why this lever

`PROJECT_GOAL.md` "Sacrifice Order" — compromise in this order when the DS
cannot afford everything:

> 1. Audio fidelity 2. Visual fidelity 3. Gameplay fidelity
> 4. **Original 60 Hz simulation implementation** 5. **Stable 30 FPS**
>
> "Stable 30 FPS is the most protected requirement."

The port presents at 30 FPS but runs the simulation **twice per presented
frame**: `NDS_TASK106_UPDATES_PER_PRESENT 2u`
(`src/port/taskman_seam_battle_host.c:41`), confirmed in the profile by
`ndsBaseGcRunAll` at **1.98 calls/frame** against `ndsR2HostBattlePresent` at
**0.99**. The 60 Hz simulation is ranked *below* frame rate, and it is already a
build define whose own comment says building with 1 "prices what a 30 Hz
simulation would save".

## Result

`builds/build-p2p8-sim30`, `NDS_TASK106_UPDATES_PER_PRESENT=1`, four-CPU stress,
1,972 samples, against the clean post-deletion baseline:

| bucket | baseline P50 | 30 Hz P50 | dP50 | baseline P95 | 30 Hz P95 | dP95 |
|---|---|---|---|---|---|---|
| **WORK-H** | 1,575,168 | **1,281,152** | **-294,016** | 2,308,032 | **1,798,400** | **-509,632** |
| SRC | 553,984 | 297,088 | -256,896 | 1,059,712 | 649,792 | -409,920 |
| GCRA | 548,480 | 294,208 | -254,272 | 1,054,208 | 642,176 | -412,032 |
| FTR | 356,032 | 357,504 | +1,472 | 750,144 | 465,280 | -284,864 |
| STG | 336,640 | 335,808 | -832 | 380,480 | 382,656 | +2,176 |
| ALL | 1,678,016 | 1,677,824 | -192 | 2,798,144 | **2,238,208** | **-559,936** |

The prediction from the profile was -289,824. The measurement is **-294,016**,
1.4% apart.

The shape is exactly right and is its own check: SRC and GCRA halve, STG (fixed
stage work) does not move at all (-832 and +2,176, both noise), and FTR P50 is
flat while FTR **P95** falls 284,864 because fewer logical ticks mean fewer
state transitions in the tail.

**Cadence moves further than the medians do.** VBlank 2/3/4/5+ goes
**128/947/706/192 -> 517/1177/247/32**: frames landing in two VBlank intervals
rise from 128 to **517**, and 5+ collapses from 192 to 32. `ALL` P95 drops a
whole quantum, 2,798,144 -> 2,238,208.

## Against the gate

| | ticks | vs 1,120,000 | gap |
|---|---|---|---|
| baseline | 1,575,168 | 1.41x | 455,168 |
| 30 Hz simulation | **1,281,152** | **1.14x** | **161,152** |

**It closes 64.6% of the gap and leaves 161,152 tk/fr.** It is not sufficient on
its own, and no honest reading makes it sufficient.

What can cover the remaining 161,152: the stage lane. STG is 336,640 tk/fr, flat
(spread 1.13), on geometry that does not move, and
`src/port/reloc_backend_movement.c:13549` records that **238,254 tk/fr of it has
never been profiled by any task**. 161,152 is 68% of that unprofiled block.

## Caveats, both real

1. **The harness refused this run as a whole-match measurement** and it is right
   to: "P2-2 four-CPU timing window is not whole-match: 43.33% of the guest's
   configured match (26s of 60s)." At half speed 1,972 presented frames cover
   26 seconds instead of 60, so these figures describe an earlier slice of the
   match, not the same window as the baseline. The direction and magnitude are
   unambiguous and the bucket shape corroborates them, but the exact figure has
   to be re-measured on a **compensated** build before it is banked.
2. **Uncompensated is not shippable.** Per the define's own comment, the real
   work is "advancing timers, physics integration and animation by two frames
   per tick", and that "is the part that needs the owner's 'substantially the
   same gameplay experience' call."

## What this establishes

A path to 30 FPS at four players exists, and it is the one the project's own
sacrifice order nominates:

- 30 Hz compensated simulation: **-294,016** (64.6% of the gap)
- the unprofiled stage block: **238,254 tk/fr available**, 161,152 needed

Together they exceed the requirement. Neither alone reaches it.

The engineering is the compensation work plus the stage lane. The decision that
is not mine is whether a 30 Hz simulation, compensated to advance two frames per
tick, is still "substantially the same gameplay experience".
