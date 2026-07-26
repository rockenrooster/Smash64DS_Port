# Task 79 E1 — REVERT. The site memo buys the fighters −716 and costs the stage 6,880

**Date:** 2026-07-26
**Status:** **REVERT.** Source restored byte-identical (`git checkout`, clean
tree). The E0 finding stands; this particular mechanism does not transfer to the
fighter path.
**Inputs:** `artifacts/task79-siteA.json`, `-siteA2.json`, `-siteB.json` — three
128-frame `-RingDump` runs, frames 439–567, both arms built from the same tree
so the refactor nets out.

## 1. What was tried

Task 79 E0 proposed carrying a resolved cache slot and generation stamp per run
so a texture bind could skip rebuilding, hashing and `memcmp`-ing a 236-byte key.

That mechanism turned out to **already exist**: `ndsRendererStageTextureSiteTryBind`
memoises `(source_command_site, texture_source_hash1, texture_source_hash2) →
cache entry`, validates it against `entry->key_generation`, and binds directly.
It was written for the stage and gated to it in two places:

- `sNdsRendererStageTextureSitesEnabled` requires
  `sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_STAGE`
- `ndsRendererTextureSourceHashCommand` returns early for any non-stage owner,
  so the hashes that key the memo are not even accumulated for fighters

So E1 was a small change: one shared eligibility predicate consulted by both
gates, extended to `OWNER_MARIO` and `OWNER_FOX` behind
`NDS_TASK79_FIGHTER_TEXTURE_SITES`. Both gates had to move together — enabling
the memo for an owner whose hashes are never accumulated would compare whatever
the previous owner left behind and bind a stale texture.

## 2. Result

| bucket | A (off) | B (on) | Δ P95 | Δ mean |
|---|---|---|---|---|
| `FTR` P95 | 1,010,816 | 1,020,160 | **+9,344** | −716 |
| `STG` P95 | 388,224 | 397,056 | **+8,832** | **+6,880** |
| `SRC` P95 | 739,008 | 738,688 | −320 | — |
| `WORK` P95 | 1,872,512 | 1,889,088 | +16,576 | — |
| `WORK-H` P95 | 1,871,296 | 1,852,032 | −19,264 | — |

**The fighters gained nothing: −716 ticks/frame mean.** That is the number the
change existed to move, and it did not move.

**The stage lost 6,880 ticks/frame, on all 128 of 128 frames.** A regression
present on every single frame is not placement noise; it is a mechanism. The
site table is 128 entries
(`NDS_RENDERER_STAGE_TEXTURE_SITE_COUNT`) and was sized for the stage alone.
Admitting both fighters to it evicts stage entries, so the stage falls back to
the full key rebuild it used to skip. The change did not add a fast path to the
fighters so much as take one away from the stage.

`WORK-H` P95 improved 19,264, and it is the gate quantity, so it deserves a
direct answer rather than being quietly dropped: it is not supported by anything
else in the table. `WORK` P95 moved the opposite way (+16,576), and both buckets
the change actually touches got worse. Keeping a change on one derived
percentile while two named buckets regress is how a ROM accumulates behaviour
nobody can explain later. `AGENTS.md` calls unexplained differences failures,
and this one is unexplained in the direction of keeping it.

## 3. Why the fighters gained nothing

The memo keys on the source command site pointer plus a running hash of the
texture-state commands preceding it. For the stage that is stable: the same
display list executes from the same address every frame with the same state.

For fighters it evidently is not — the gain would have shown up as a `FTR` mean
drop and it did not. The candidates are that fighter draws reach their binds
through sites that differ frame to frame, that the accumulated state hash
differs because fighter material state genuinely varies, or that the shared
table thrashes so hard that the fighter entries evict each other as well. This
task does not distinguish them, because the stage regression alone is
disqualifying and a further run to separate the three would be spending
measurement on a mechanism already rejected.

## 4. The instrument fact this run established

**Repeat runs of the same ROM are bit-identical.** `siteA` and `siteA2` are the
same ROM sampled twice, and they agree on every bucket, every percentile, and
even the anomalous 6,435,008-tick maximum `ALL` frame — not close, identical.

This is worth writing down because it changes what a third run is for. The
standing rule says to run a third A when the A/B is noisy or surprising. Here a
third A of the *same* ROM is provably worthless: it returns the same numbers to
the digit. Run-to-run variance in this harness is **zero**.

The ±8,000 figure the campaign has been calling a noise floor is therefore not
measurement noise at all — it is **build-to-build placement variance**, real and
deterministic, and it only appears when the binary changes. The consequence for
method: to separate a change's effect from its placement, vary the *build*, not
the run. A repeat run confirms nothing except that the harness is working.

## 5. What survives

Task 79 E0's measurement is untouched: texture and material resolution is
143,434 ticks/frame, it runs at 4.0–10.8 cycles per instruction, and it rebuilds
a 236-byte key per bind. That cost is still there and still worth taking.

What this rules out is reusing the stage's site memo for it. A fighter-side
version would need a key the fighter path actually holds stable — the generated
run index from `sNdsNativeFighterRuns`, not a source command pointer — and its
own table, so the stage's 128 entries stay the stage's. That is a larger change
than this one and it should not start until something establishes *why* the
fighter binds miss, which this task deliberately did not spend a run on.
