# Main P2 cursor at Meta Knight branch creation

Preserved from base `2e093297c5c` on September 30, 2026. This is historical
reference for the isolated experiment; the main checkout retains its live queue.

Focus (owner 09-22): **P2-2p8 four-fighter 30 FPS -- architecture.**
Plan: `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`, D1-D9, log section 6.
Phase 0 closed; Phase 1 through `98ebd1e2e51`.
**Phase 3 IMPLEMENT**, owner reprioritization 09-24: >=95% four-concurrent VS
performance; 1P later, pre-stage intros static images. Live-Intro edits reverted.
Receipt: `artifacts/performance/2026-09-24_p2-2p8-phase2-stage/README.md`.
Completed scoped checks (not phase acceptance): B3 conserves exactly; MINS is
off by default. M1 ImpactWave/DamageSlash has replay/Task49 proof. Pooled GObj
serials remove stale item binding; Link Bomb, clouds/TaruCann and Fox Blaster
have focused positive owner proofs. Reuse their receipts:

- M1: `artifacts/performance/2026-09-25_p2-2p8-phase2-m1/README.md`.
- Lifetime/integrated: `artifacts/performance/2026-09-26_p2-2p8-phase2-item-linkbomb-fixed/README.md`.
- Bomb: `artifacts/performance/2026-09-26_p2-2p8-phase2-link-bomb/link-bomb-proof.txt`.
- Ground: `artifacts/performance/2026-09-26_p2-2p8-phase2-ground/` (mixed timing).
- Fox: `artifacts/verification/2026-09-26_p2-2p8-phase2-fox-ndl.txt`.

**Engagement owed:** Fireball playback reaches GO but never Special-N; no state
injection. Lakitu/Bronto slots remain unengaged in the canonical 59 s match
(Bronto's first wait is 6,000..15,999 source updates). Their host/replay proof:
`artifacts/performance/2026-09-26_p2-2p8-phase2-efground/README.md`.

**Particle/Dust (09-26):** view-space particles `608C79AC` and dust `8B4D66EE`
replay-identical, native failures 39->0; KEEP/IMPLEMENTED_NOT_ACCEPTED. Particle
selector retired, NDL default on. Shipping-like `8062C536` CSS previews exact.
Receipts: `2026-09-26_p2-2p8-{phase3-residency,phase2-particle-ab}`. Owed: particle
Task49/lifecycle, world-cache retirement, integrated gates. All-VS `9F69CA39` KEEP.
`7E0B1C7F` CSS free 200,656 B. Owner 09-26: no subagents.
MF2 check passes (1,570 clips/29 raw exceptions); audit in receipt.
`FB299B25`: 2,043 presents/Results; low-water 9,672 B, reserve RED. Reload passes.
A7 loan 265,216 B. Trace accounts 36,300 B of play allocations; hats 16,188 B.
MF2 worst bank697,760 B unlinked; Kirby copy engagement still owed.
**A8 IMPLEMENTED_NOT_ACCEPTED**:17 host tests pass.
0BD4523E replay exact;141 BGM refills, no audio failures, arena +20,480 B.
AUD P95 6,528; WORK-H P95 2.625M/18.97 FPS, RED. Mixed audio identifies BGM.
HeavyF7443068 gains16,384 B; admission still5,904 B short, plus later play.
A8 FGM: ARM7 fills + 160 KiB arena landed 09-27.
**Solo 09-26/27** (receipts `2026-09-2{6,7}_p2-2p8-*`, all replay identical):
compact packet layout reverted (`BC3500EA` P95 1,955,392, was 2,626,368);
edge-only ROM bounce (1,892,160); FGM LRU/aligned/envelopes + ARM7 async fills
(1,852,672); 160 KiB FGM ring arena (-72 KiB); fast cpuGetTiming; stage no-Z
local W columns (STG P50 253K); status path (token index, path formatter,
ForgetRange skip); ARM pose clock; HUD state once per pass; stage witness off +
world pointer chain; reloc lookup memos; A5 hurtbox reject (shadow 0 flips);
ARM memcpy/memset/memcmp in ITCM, FGM id map, matrix leaves, searched flat-walk
cache; lab splits off; libgcc pose clock; HUD div15 (`2026-09-27_p2-2p8-fast-mem`):
MP f32 cache reads; billboard memo; STG span; ITCM x4; witnesses; Results guard; key words; HW div; lean spare; stage memo + data pin; ForgetRange; HUD digest; lab clocks; DTCM hot stack/sine/scalars; stage GX + packs in ITCM; i2f convs; DTCM-stack subtrees; objs+pose in WRAM; 09-28: FNT, BPS1 x12, 1P overlay, arena cliff, YI clouds, S1-S4, elastic cache, maps, R1, hit fetch; 09-29: spline, GX overlap, pose step, int trig, hurtbox x4, Arwing x6, GX memo, bboard, objs, SMULL, AObj hash, anim idx; 09-30: MPgrp, digest, anim, segx, clip IO x3, ledger, HUD: gate 899K/1.212M. Owe 6 owners
**Constraint**: CSS reserve >=183,072 B; owner route 253,904 free (09-27).
After static growth use `artifacts/performance/2026-09-23_css-preview-heap/tools/run-owner-css.ps1`.
Specs: `artifacts/performance/2026-09-23_p2-2p8-phase-specs/`.

Closed 09-22 bug sweep: owner fixes/deferred rows and remaining obligations are
in `p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-22.md` and
`docs/archive/P2_CLOSED_ROWS.md`. Last historical publication r54
`C8FC02AA2DF0BB6E` is saved in `builds/remaining-bugs-playtest-r54/`.
Boundary/Latest, per-hat captures and preview-pack test pin drift remain owed.

Preserve owner 1P/CSS work and the published ROM until gates pass. Older scoped
proofs: `p2/BUG_NOTES.md`, `docs/archive/P2_CLOSED_ROWS.md`.
