# Task 40 fighter-animation audit — Phase 1 checkpoint

Date: 2026-07-21

## Verdict

The source-backed Mario/Fox animation bank and visual/load sweep are complete.
All 195 non-null Mario motions and 209 non-null Fox motions produced asserted
non-clear screenshots. Tyler approved the Mario strip; missing Mario sounds are
Task 38/audio scope. The Fox strip remains pending Tyler's visual verdict.

This is not a green full-duration matrix. The first complete captures predate a
checker fix that bounded joint inspection with `ndsFTStructJointLoopLimit`.
Tyler explicitly asked not to replay known-good motions. The coverage CSV
therefore records those rows as full-play visual captures while keeping their
numeric duration evidence provisional. A post-fix spot run proves Mario motion
203 at 83/83 frames; the resumed Fox 198-218 CSV still contains invalid expected
lengths and false early-loop results and is not presented as correctness proof.

## Root-cause fixes

- The Phase-0 census found 102 Fox motion symbols absent from the port map. The
  completed bank maps all Mario/Fox source animation assets; source-null table
  rows remain null.
- Fox motion 198 (`FTFoxAnimAppear`, asset `0x309`) is AObjEvent32, as are Fox
  `0x30A` and Mario `0x279`/`0x27A`. Treating them as AObjEvent16 and then
  excluding them from the force loader left stale heap data and caused the
  observed hangs. Format classification now separates AObj16/AObj32 while the
  shared force loader admits every Mario/Fox animation asset.
- The missing `T` in `DEATH` was an unrelated presentation omission found
  during the sweep. The existing announce-sprite normalizer now includes its
  source symbol.
- No `decomp/` file or gameplay rule was changed. The animation-loader repair
  is still gameplay-adjacent because animated joints own collision attachments.

## Coverage

| Fighter | Table rows | Source-null | Captured non-null | Natural requested | Natural never requested | Visual verdict |
|---|---:|---:|---:|---:|---:|---|
| Mario | 204 | 9 | 195 | 21 | 183 | Tyler approved; sounds excluded |
| Fox | 219 | 10 | 209 | 42 | 177 | pending Tyler |

The natural match ran with Fox AI enabled; both cyclers disabled Fox AI so it
could not interrupt Mario. Natural evidence reports zero load fallback,
external-fixup, figatree-invalid, or unsafe-animation flags. Cycler resume used
the existing 0-197 Fox screenshots and captured only 198-218 after the loader
fix; no known-good motion was replayed.

## Shipping and memory surface

- Audit counters/cycler: profile 1 only; profile-0 symbol hits: `0`.
- Profile-0 ROM: 14,958,592 bytes,
  `AEE10EB3912A9954090A40EE151406255FD22AA94C7EDF04DE3BB399DECF0415`.
- ROM growth from the pre-task 14,692,352-byte candidate: 266,240 bytes.
- Runtime remains one force-loaded motion in the existing fighter heap; no
  resident cache or per-frame shipping interpreter was added.
- Current net reserve is 166,672 bytes (232,208-byte taskman headroom minus the
  65,536-byte audio ring), above the task's 100 KiB floor; full arena allocation
  remains 1,376,256 bytes with zero fallback.
- A steady-state performance A/B is not meaningful for this checkpoint: the
  shipping change occurs only at motion load, adds no per-frame interpreter,
  and the pre-fix control hangs at Fox motion 198.

## Evidence

- Coverage table: `2026-07-21_task40-fighter-animation-coverage.csv` (423 rows).
- Raw audit exports: `2026-07-21_task40-{mario,fox,natural}-audit.csv`.
- Mario contact sheet: 696,330 bytes,
  `5E8A76D387D69C8BCF99D6BCCBF35CCF67C05C9F95069FD7E7D3538EFB37098A`.
- Fox contact sheet: 739,136 bytes,
  `6B4A15849E53BEA7442A5E196232F9FF8E3948DF574FF1A63629AFA898C42FB5`.
- Phase-0 ownership/seam map:
  `2026-07-21_task40-fighter-animation-phase0.md`.

## Verification

- DevFast preflight/build checks passed. Its first runtime leaf exposed a stale-
  ROM-calibrated Task-36 water assertion; a fresh build correctly reported the
  replay's all-zero live-water counter. The fail-closed mode-2 assertion and
  source fixture were restored, then the focused DevFast runtime passed against
  the unchanged ROM. Already-green unrelated fixtures were not replayed.
- `verify-boundary.ps1 -NoBuild -DelaySeconds 3`: pass.
- Profile-0 audit-symbol census: zero hits.
- The historical four-shard Regression fleet is retired; current repository
  policy exposes only Latest and Boundary and forbids reviving it.
