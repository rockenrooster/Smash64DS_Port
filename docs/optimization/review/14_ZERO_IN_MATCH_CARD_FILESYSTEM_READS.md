# Campaign 14 — Zero In-Match Card / Filesystem Reads

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.
>
> **Basis (2026-08-17):** the shipping level is **+26,449** at rank-80 and the fresh per-PC census is `artifacts/performance/2026-08-17_shipping-rebank/v4-c238`. `c200` and `v3-c221` are retired. **Read `SHIPPING_REBANK.md` §7.7 before quoting any figure in this brief** — it lists what the new census contradicts, and mask the census by the GATE's own rank-80 frames.

## RESOLVED 2026-08-17 — the reader is `ndsAudioFgmPlayAtPan`, and the lane does not convert

`artifacts/performance/2026-08-17_card-fs-caller/CARD_FS_CALLER.md`. No build,
no emulator: a re-reduction of the existing `c237` bank and `v4-c238` census.

**The reader.** The FGM sound-effect player, two sites in
`src/nds/nds_audio_fgm.c` — `ndsAudioFgmCacheAcquire` `:477-480`
(`fseek(data_offset)` + `fread(data_bytes)` on a sample-cache miss) and
`:1147-1156` (`fseek(envelope_offset)` + `fread(4 × envelope_count)` on every
play of the 9 of 88 cues that carry an envelope). The file is
`nitro:/audio/fgm_phase_pack_ima.bin` (938,996 B, 88 entries, samples median
6,904 B / max 52,108 B, 256-byte envelope tail), held open across the match in
`sNdsAudioFgmFile`. Path: newlib `fseek/fread` → `_nitroFS_*` →
`nitroromReadIter`/`_nitroromFdRead` → `_FAT_seek_r`/`_FAT_read_r` →
`f_lseek`/`f_read` → `get_fat` → `move_window` → `disk_read` → calico
`_dvmDiscCache*`/`blkDev*` → DLDI. Resolved from the linked ELF, not from grep.
**Not** `ndsRelocGetFileData` (conc **1.04**) and **not** the BGM streamer
(`ndsAudioBgmReadPacket` 83 tk/fr gate-80 at 0.11 calls/gate-frame): the
campaign's own closure holds.

**The size, and the retraction.** The stack is 31 symbols, not 3 — `fat` 29,639
+ `stdio` 5,057 + `blk` 4,247 + `nitro` 2,768 = **41,712 tk/fr gate-80** against
4,278 whole-match (COST× 9.75), so the earlier 23,908 undercounted the cost.
But subtracting each frame's own measured I/O and re-ranking the 1,600 rows
moves rank-80 by **−11,003** (requirement +26,449 → +15,446), a conversion of
**0.264** — because the I/O lands on **175 of 1,600 frames**, only **56 of the
80** gate frames carry any, and its heaviest frames are `WORK-H` ranks
**2, 6, 7, 8, 11, 16, 17, 18, 29**, *above* the percentile.
`[[cluster-where-the-percentile-lives]]` in its other direction. **Removing 25%
of the misses buys +0; 50% buys −5,275. There is no incremental version.**
**0 frames change VBlank bucket**, so the cadence arm does not move either.
**"23,908 = 90% of the requirement" is retracted.**

**And it cannot be removed anyway.** Board slice 53 (2026-08-13) measured the
working set at **59 cues / 575,760 B against the 204,800 B `sNdsAudioFgmCache`**
— residency needs ~371 KB that does not exist (heap low-water 53,136) — reverted
its own candidate on a falsifier, and named exactly one reopening condition:
the per-seek unit price. This census measures **424 `get_fat` hops per `f_lseek`
and 18,131 ticks per seek** against that closure's 447-step walk. **Unchanged;
the lane stays closed.**

**Consequence for Phase 4 below:** a verifier that FAILS on a post-GO card read
cannot be armed for the audio path — it would go red today and stay red, because
the FGM path legitimately reads the card ~150 times a match (slice 53 measured
150 misses over 188 plays on `c147`; this census counts 160 `ndsAudioFgmPlayAtPan`
entries over 1,601 frames) and cannot be made resident in the RAM available. Phase 4 remains correct, and armable, for
animation, texture conversion and reloc normalization, which are measured at
zero.

## Status — the core LANDED 2026-08-16

The countdown/loading fix
(`artifacts/verification/2026-08-16_owner-bug-closure/BUG_CLOSURE.md`;
`docs/BUGS.md`) moved DS texture/placement preparation and the animation
warm-up to the **exact pre-BGM seam**: the import shim intercepts
`mpCollisionSetPlayBGM` → `ndsSCVSBattleStartPlayBGM`, which drains scene
textures, placement, and `ndsR2AnimCachePreloadFinish()` before BGM/countdown
begins. The preload is **stepped** (`ndsR2AnimCachePreloadStep`, bounded
16,384 B chunks, ~18 steps) with barrier counters
(`gNdsR2AnimPreloadBarrier*`), and the warm set includes the common
grab/throw animations (`0x231-0x234`, `0x2c0-0x2c3`), all pinned by
`check-gbi-decode-fixtures.ps1`.

**Measured: a 1,600-frame run has 0 post-start animation cache misses and 0
post-start payload reads, with BGM seam/error/overrun all 0.** The seven card
reads (12,736 at rank-80 on `c219`) are claimed — do not re-chase them.

**Scoped 2026-08-17:** that claim is about the **animation** loader
(`ndsRelocGetFileData`, conc 1.04), and it holds. It is *not* a claim that the
frames went quiet: **6 of those same 7 frames still carry card I/O** on `c237`
(456/830/1015/1186/1625/1655, `WORK-H` ranks 32/2/6/8/7/11), because a new
status triggers an animation attach *and* a sound effect, and the sound effect
is the reader named above. Two readers, one set of frames.

## BLOCKED(decision: audio fidelity) — the only route left to the −11,003

The lane is all-or-nothing (25% removed = +0), and full removal needs the FGM
working set resident: **59 cues / 575,760 B against a 204,800 B cache**, i.e.
**~371 KB of RAM that does not exist** (heap low-water 53,136; binary growth
costs the arena 1:1, `[[ram-is-not-free-gobj-cap]]`). The only way to close that
gap is to shrink the cue set or the cues — fewer cues, shorter tails, lower
sample rates, or a coarser ADPCM. That is **sacrifice-order rung 1 (audio
fidelity)**, which `PROJECT_GOAL.md` assigns to the owner. Price it before
asking: **−11,003 of a +26,449 requirement (42%)**, and no partial version of it
pays anything. Nobody should start this without that decision.

## Remaining objective

Make **GO a hard, fail-closed preparation boundary** rather than a currently
true fact:

- the invariant today is proven for **animation** misses/payload reads; extend
  the counters to texture conversion/repack, reloc normalization/fixup, and
  any first-use asset conversion (Phase 4);
- make the one-minute verification **fail** (not report) when a forbidden
  counter increments post-GO, naming the asset;
- price the enlarged warm set against RAM headroom (Phase 5);
- keep the invariant permanent as Campaign 03 replaces legacy normalization
  with native packs (Phase 6).

## Pre-fix measured facts (historical baseline the fix was built against)

From `artifacts/performance/2026-08-16_match-io-audit/IO_AUDIT.md` (basis
`build-c219-animitcm-ship`, gate stress arm `NDS_R2_BOTH_CPU=1`, three
byte-identical whole-match runs — **before** the pre-BGM preload landed):

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
