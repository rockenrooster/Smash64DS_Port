# Campaign 14 — Zero In-Match Card / Filesystem Reads

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Make **GO a hard preparation boundary**.

After player control unlocks:

- **0 filesystem/card reads**;
- **0 asset conversions/normalization**;
- **0 avoidable first-use cache fills**.

The remaining seven Mario/Fox animation misses all land in the top twelve measured frames and are worth about **12,736 ticks at rank-80** despite there being only seven. (Sized on the `c219` basis — re-size on the current basis before banking, per the campaign rule.)

## Current measured facts

From `artifacts/performance/2026-08-16_match-io-audit/IO_AUDIT.md` (basis
`build-c219-animitcm-ship`, gate stress arm `NDS_R2_BOTH_CPU=1`, three
byte-identical whole-match runs):

- `349` animation acquisitions;
- `215` BattlePack;
- `127` RAM-cache hits;
- **7 card reads**;
- payload reads `101 -> 108`;
- header reads `1593 -> 1600`;
- short reads `0`;
- cache overflows/rejects `0`;
- animation arena about `415,984 / 451,776` bytes.

Card-read frames/ranks:

- 456 — rank 4
- 830 — rank 1
- 1015 — rank 7
- 1186 — rank 10
- 1625 — rank 12
- 1655 — rank 8
- 1886 — rank 9

The identical seven-frame list was independently reproduced on the `c220`/
`c221` basis (board `SITR_EXCURSION` entry), so the load events belong to the
match, not to one binary.

Do not solve this by excluding those frames from measurement.

**Scope boundary, so this campaign does not absorb its neighbor:** the larger
`SITR` excursion around these frames (72,768 at rank-80 ceiling, 288
attach/force-load frames) is **attach-driven re-parse**, not I/O — the attach
(r=+0.623) and stepped parse (r=+0.650) outrank the force-load (r=+0.487) and
the card read (r=+0.549), and 10 of the 25 cluster frames carry no force-load
at all (`ATTACH_LANE.md`/`SITR_EXCURSION.md`). That work belongs to Campaign
03's representation replacement. This campaign's own fully-diagnosed prize is
the seven card reads plus the GO-boundary invariant.

## Current repo anchors

- `src/port/reloc_backend_assets.c`
- `src/nds/nds_battlepack_anim.c`
- `scripts/generate_battlepack_anim.py`
- `scripts/probe-battlepack-pacing.ps1`
- `scripts/analyze-io-lane-series.py`
- `scripts/analyze-load-frame-exclusion.ps1`
- one-minute match harness
- Campaign 03

## Phase 0 — Identify exact seven assets/motions

Log each post-GO miss with:

- fighter;
- motion/asset ID;
- source file/reloc token;
- requested bytes;
- reason BattlePack did not satisfy;
- cache state;
- first request frame.

Produce a deterministic seven-entry work list. Do not infer asset identity from frame timing alone.

## Phase 1 — Find root cause

The arena no longer overflows/rejects, so classify each miss:

- absent from generated manifest;
- late motion dependency not enumerated;
- key/identity mismatch;
- generator coverage gap;
- preload sequencing gap;
- generated but not retained.

Fix root cause rather than adding a huge “load everything” step.

## Phase 2 — Extend BattlePack/AOT manifest

Teach generators to include all Mario/Fox P1 post-GO motion dependencies.

Prefer build-time dependency discovery from fighter/motion metadata over a hand-maintained list of seven IDs.

Add a checker equivalent to:

`all post-GO Mario/Fox animation acquisitions ⊆ BattlePack/preload manifest`

for qualified P1 content.

## Phase 3 — Preload before GO

Preparation should complete:

1. locate native/BattlePack asset;
2. read card;
3. normalize legacy data if still required;
4. build cache/native descriptor;
5. verify mandatory entries resident;
6. unlock gameplay.

Campaign 03 should eventually remove legacy normalization by storing native animation packs.

## Phase 4 — Hard GO-gated counters

Track after GO:

- file/card opens/reads;
- payload/header reads touching card;
- animation cache misses/fills;
- texture conversion/repack;
- reloc normalization/fixup;
- any first-use asset conversion.

One-minute verification should **fail** when a forbidden counter increments and name the asset.

## Phase 5 — Protect RAM headroom

Do not preload the entire ROM.

Price:

- BattlePack bytes;
- arena used/free;
- heap low-water;
- entry count;
- duplicated content.

Load only the qualified match set and deduplicate.

## Phase 6 — Integrate Campaign 03

Once compact native animation lands:

- the seven legacy misses disappear structurally;
- acquisitions resolve to native descriptors;
- generic FIGATREE normalization is not used for qualified P1 motions.

Keep the GO invariant checker permanently.

## Verification

- multiple whole-match runs;
- exactly zero post-GO card reads;
- intended cache-fill count;
- identical gameplay invariants;
- no prep freeze/hang;
- RAM/arena headroom;
- corrected re-ranking of full frame series;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- no frame exclusion trick.

## Completion criteria

The one-minute Mario/Fox P1 match performs zero card/filesystem reads after GO, zero avoidable conversions/fills after GO, and automated verification prevents future regressions.
