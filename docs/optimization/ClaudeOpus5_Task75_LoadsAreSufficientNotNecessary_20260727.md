# Task 75 E0 — Loads are sufficient for an `SRC` excursion, and not necessary

**Date:** 2026-07-27
**Status:** E0 complete. **Preload is worth ~103,488 of P95, not the ~170,000
Task 71 estimated, and it hits a second wall immediately.** No runtime change;
`NDS_TASK75_LOAD_CENSUS` defaults to 0.
**Input:** `artifacts/task75-load-census.json`, 32 frames from 440.
**Discharges:** Task 71 §5's open obligation.

## 1. The obligation

Task 71 profiled one expensive `SRC` frame, found a NitroFS open, a cartridge
read, a relocation and a figatree parse inside the frame that needed the
animation, and closed with:

> "It also has not been shown that every one of the 26 high-`SRC` frames is a
> load; one frame has been profiled. The cheap confirmation is a counter on the
> load entry point sampled per frame through the ring."

Task 106 made that question the gate's. Halving the update rate leaves the `SRC`
excursion unchanged (+518,016 against +522,720), so the tail is loading — if
Task 71's single frame generalises.

## 2. Instrument

`gNdsTask75AssetLoadCount` increments in `ndsRelocFinalizeLoadedFile`, which is
the one point every load path funnels through exactly once per file; the
`fixups_applying` early return is the re-entrant case and is deliberately not
counted.

Its per-frame delta rides the **same census ring** Task 70's fallback counter
uses, selected by `NDS_TICK_HUD_CENSUS_RING_SOURCE`. The two censuses ask the
same shape of question about different counters and are never built together, so
one ring and one selected source leaves `scripts/sample-tick-hud-buckets.ps1`
untouched — its two-stop baseline path is proven, and editing a `.ps1` has
corrupted these files before.

**Cross-validated.** Built with both censuses on, the load counter reports **7**
loads over the window and the independent native-owner counter reports
`animLoad:7`. Two separately implemented instruments, at different seams, agreeing
exactly — every completed file load in steady battle is an animation load, and
each causes one native-owner `animLoad` fallback.

## 3. The result

`SRC` median 316,096. Frames running `SRC` above 1.5× that:

| frame | `SRC` | `WORK-H` | loads |
|---|---|---|---|
| 450 | 835,648 | 1,801,216 | 2 |
| 465 | 691,136 | 1,650,880 | 2 |
| 470 | 688,768 | 1,656,896 | 1 |
| **453** | **636,096** | **1,593,664** | **0** |
| **454** | **598,656** | **1,553,408** | **0** |
| 445 | 594,176 | 1,556,352 | 1 |
| 469 | 518,912 | 1,481,856 | 1 |

- **5 of 5 load frames are excursions.** A load is *sufficient*.
- **2 of 7 excursions carry no load at all.** A load is *not necessary*.

`SRC` median on loaded frames 688,768; on clean frames 314,048.

**Task 71 §5's question is answered: no, the high-`SRC` frames are not all
loads.** Frames 453 and 454 run 2.0× and 1.9× the median `SRC` with zero
cartridge activity, and nothing in the campaign has profiled a load-free `SRC`
excursion — Task 71's window (469–470) contained a load.

## 4. The ceiling, and the wall behind it

| | `WORK-H` P95 |
|---|---|
| all 32 frames | 1,656,896 |
| load-free frames only | **1,553,408** |
| **difference** | **103,488** |

Removing on-demand loading entirely does not delete those frames, it makes them
cheap, so the surviving distribution is the load-free one. **The preload is worth
about 103,488 of P95** — 19% of the 536,896 gap, and well under Task 71 §5's
~170,000 estimate, which was extrapolated from one frame's excursion rather than
measured against the distribution.

Worse for the plan: the new P95 would be **frame 454, a load-free excursion**.
Preloading buys 103,488 and then hands the gate straight to a cause nobody has
identified.

## 5. What this authorizes

**Does not authorize starting the preload bridge as a subsystem.** It is real,
it is the largest identified single item, and `PROJECT_GOAL.md` endorses the
trade — but at 103,488 against a 536,896 gap it cannot be the plan, and building
it first means discovering the second cause afterwards from a red gate.

**Does authorize, and makes urgent, profiling a load-free `SRC` excursion.**
Frame 453 (`SRC` 636,096, `WORK-H` 1,593,664, zero loads) is the cleanest
subject: a single-frame spike with no cartridge activity, no fallback, and
`FTR`/`STG` at median. The instrument is the one Task 71 used — a per-PC census
windowed on the frame — and it is the only way to find out whether the residual
excursion shares a cause with the loads (relocation, figatree parse) or is
something else entirely.

If both causes turn out to be the animation pipeline arriving late, one fix
serves both and the preload's ceiling is higher than 103,488. If they are
unrelated, the gate needs both and neither alone is sufficient. Nothing in the
record distinguishes those today.

## 6. Cost

One build, one census run, no A/B. It corrected a sizing estimate by 40% and
kept a subsystem-sized task from being started against the wrong number.
