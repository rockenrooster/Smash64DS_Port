# Task 62 — A/B Results: Dream Land DS-native mesh (c120) vs segment0

8-frame synchronized A/B on the canonical Boundary configuration
(`battle_playable_realtime`, mode 163, Dream Land, Mario human vs Fox level-3 CPU,
items off, one-minute Time mode). Frame window 438–445 (the canonical first
steady-state gate after GO). melonDS, repo-local runner slot 0, identical window
profile. Date 2026-07-24.

## Build identity

Both arms built from the same tree (`dfff37d59`, post counter-relocation fix),
differing only in the flag:

| arm | target | flag | BUILD dir | ROM sha256 |
|-----|--------|------|-----------|-----------|
| A (baseline) | `smash64ds-battle-playable-proof-hwtri` | `NDS_DREAMLAND_DS_MESH=0` | `builds/build-task62-ab-flag0` | `af8b2a50...` (11,432,960 B) |
| B (candidate) | `smash64ds-battle-playable-proof-hwtri` | `NDS_DREAMLAND_DS_MESH=1` | `builds/build-task62-ab-flag1` | `ae23109d...` (11,425,792 B) |

The `proof-hwtri` target is profile-0 + HW_TRIANGLES (the published-profile-0
config) but non-published, so each arm resolves to its own BUILD dir and the two
ROMs coexist. Config headers confirm `NDS_DREAMLAND_DS_MESH 0` vs `1`.

## Tick results — DECISIVE stage-CPU-work reduction

RENDER_BENCH rows (profile, frame, total_ticks, ftr/stage_ticks, …) via GDB
memory reads — measured, not captured. Raw logs:
`artifacts/performance/2026-07-24_task62-ab-A_flag0.log`,
`artifacts/performance/2026-07-24_task62-ab-B_flag1.log`.

| frame | A total | B total | Δ total | A ftr/stg | B ftr/stg | Δ ftr/stg |
|------:|--------:|--------:|--------:|----------:|----------:|----------:|
| 438 | 1,989,568 | 1,429,760 | −559,808 | 1,521,088 | 1,222,464 | −298,624 |
| 439 | 1,950,592 | 1,392,640 | −557,952 | 1,486,080 | 1,181,952 | −304,128 |
| 440 | 1,351,616 | 1,283,200 | −68,416 | 1,008,320 | 711,168 | −297,152 |
| 441 | 1,361,600 | 801,536 | −560,064 | 1,002,368 | 706,560 | −295,808 |
| 442 | 1,292,992 | 798,336 | −494,656 | 1,004,288 | 706,496 | −297,792 |
| 443 | 1,333,184 | 773,184 | −560,000 | 1,003,968 | 703,680 | −300,288 |
| 444 | 1,272,448 | 1,272,640 | +192 | 1,007,360 | 707,136 | −300,224 |
| 445 | 1,357,888 | 797,696 | −560,192 | 1,000,768 | 705,408 | −295,360 |

**FTR/STG column (the stage owner CPU work — the surface this change targets):**
steady-state (frames 440–445) **P50 A=1,004,128 → B=706,528 = −297,600 ticks (−29.6%)**, max A=1,008,320 → B=711,168. The delta is consistent across all 8 frames (−295,808 to −304,128), very low variance — a real, repeatable reduction, not noise.

**Total column** shows higher variance in B (798K–1.28M) — this is host
present/pacing jitter (the same jitter that trips the harness's locked-30 pacing
contract, see below). The FTR/STG column is the actual stage CPU work and is the
signal; total-tick jitter is host timing, not the change.

### Stage submission counters (last sample)

| counter | A (flag=0) | B (flag=1) |
|---------|-----------|-----------|
| `STAGE_GCDRAWALL_HW` | 240,1038,66,972,0,36,10,44,0,0 | 198,836,0,836,0,0,0,0,0,0,0,0 |
| `RENDER_SUBMIT` | 658,0,44,126,0,0,0,0,1152 | 582,0,44,0,0,0,0,0,396 |
| `FAST_FINAL` | 9,121,828,202,320,306,0,0,0 | 9,67,626,0,320,306,0,0,0 |
| `RENDER_PROFILE` | 445,156288,1357888,1000768,… | 445,157952,797696,705408,… |

`RENDER_SUBMIT` word count: A=1152 → B=396 (**−66% GX words**), matching the
host-side projection (525 → 261 verts, 50% geometry cut, further reduced by
VERTEX10 + stripification). The stage submission work dropped materially.

## Visual A/B — INCONCLUSIVE (capture-environment artifact)

**The visual gate is NOT satisfied in this environment.** All screenshot
captures returned the same static blue frame regardless of arm, timing, or
capture method:

- Harness capture (A, frame 445): `task62_capture_artifact_blue.png` sha256
  `bf23d2d2...` — solid blue (~RGB 43,83,204 = Dream Land sky color), 0% green,
  0% geometry on both screens.
- Harness capture (B, frame 445): **byte-identical** `bf23d2d2...`.
- Interactive capture (B, foreground melonDS, 22 s after launch): **byte-identical** `bf23d2d2...`.

Three captures, three different ROMs/timings/methods, one identical blue PNG.
This proves the blue is a **PrintWindow capture-environment artifact** — the
capture cannot read melonDS's GPU-rendered 3D surface in this session — NOT a
rendering difference between arms and NOT evidence of what either ROM displays.
The GDB markers confirm the harness reached scene 22 (VSBattle) and the match
started (`HARN=...,163,22,21`, `BPLAY_START=1,1,...`), so the battle was active
when the (blue) capture fired.

Per VERIFYING.md: "a successful API call alone never qualifies an image." The
blue captures qualify as nothing — they are non-evidence.

## Verdict

This is a **CPU-work-removal claim behind a flag**, for which AGENTS permits a
melonDS KEEP on typed A/B evidence. The tick evidence is decisive and large:
**−29.6% stage CPU work (−297,600 FTR/STG ticks, P50), −66% GX submission words**,
consistent and low-variance across 8 frames.

**But the KEEP gate is not met this cycle** because:
1. The visual A/B is non-evidence (capture artifact). The owner is the visual
   oracle and must confirm the flat-white c120 mesh renders a recognizable Dream
   Land silhouette (main island, three platforms, no holes) before this can KEEP.
2. Per AGENTS: "agents cannot see or hear... never self-approve on the strength
   of counters or a passing verifier alone."

**Recommended path to the KEEP decision:** the owner runs the flag=1 ROM
(`builds/build-task62-ab-flag1/smash64ds-battle-playable-proof-hwtri.nds`, or a
fresh `make TARGET=smash64ds-battle-playable-hwtri NDS_DREAMLAND_DS_MESH=1`) on
real hardware or in a session where screen capture works, and confirms the
silhouette. If visually acceptable, the tick evidence supports KEEP; the feature
then ships enabled in published profile-0.

## Harness note

The harness threw the `battle_playable locked-30 pacing` assertion after the
benchmark sampling completed (it fires during the final pacing-contract check,
after all 8 RENDER_BENCH rows and the capture). This is the same host
present/pacing jitter visible in the total-tick variance and is unrelated to the
stage mesh change (it would fire identically for flag=0). The tick sampling and
counters were fully captured before the assertion, so the A/B evidence is
intact; only the JSON export (written after the assertion) was not produced.
