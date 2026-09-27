# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-26.

**Last integrated Boundary GREEN: N04.08; P2-2p8 acceptance RED.** Figures below.

**The only dynamic queue.** Restart reads `docs/HANDOFF.md` + this file. Plans:
`docs/P2_PLAN.md` + `docs/p2/`. Closed rows: `docs/archive/P2_CLOSED_ROWS.md`.
Measurements: `PERF_LEDGER.md`. Chronology: `PORTING.md`.

## Standing rules

1. `docs/VERIFYING.md` owns one-minute measurements, coverage and command lifecycle.
   Boundary: `p2_shell_loop`, `p2_battle_realtime`, `p2_fourcpu_stress`.
2. Cadence uses all presents; rank-80 is sizing only. Actual shipping flags in
   `nds_build_config.h` control qualification.
3. **Publish law:** only qualified natural-input `smash64ds.nds`, without menu walk or
   fast logic, after verified batches. Keep P1 frozen.
4. Published P2 ROM after N04.08 + the clean rebuild, 2026-09-16. **Runtime proof
   owed** — the payload changed since the last runtime verification:

SHA-256 C6574420A9FC0E77B670093CE7AE1B595A62583488C5A9D72DD367877B0E9477

5. Permanent evidence: `artifacts/performance` and `artifacts/visibility`.
   Report P50/P95, FPS and 2/3/4/5+ VBlank histogram/maximum.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **Loop/realtime GREEN** | 12/12 roster tour native; FPS/music/dwell open. Detailed scoped proof is archived. |
| P2-2 Four-fighter engine | **Scoped capacity GREEN; performance RED** | FPC/BPS1 guards pass; shipping heavy roster still OOM. Follow current cursor. |
| P2-3 Fighter production | **Acceptance OPEN** | Link diagnostic-only; Samus engagement owed. Queue below. |
| P2-4 Stage production | **Visual acceptance OPEN** | Collision parity and Hyrule/Inishie natural proof pass; three VS captures remain. |
| P2-5 Items | **Native coverage incomplete** | Sword repair/fidelity-02 landed; kinds, children, atlas and interactions remain open. |
| P2-6 1P Game | **TALLY REACHED 09-14** | Guest playback wins stage 0 and reaches source StageClear (31,940, three bonuses; `a1209354b`). Open: next-stage Intro OOM (144,640 B asked, 120,164 B free), 41,952 native failures by the tally, DL overflow 3,744 B; shipping flag stays 0. |
| P2-7 Modes & meta | **Options/Backup Clear accepted** | Bake DATA/VS Record/Sound Test surfaces (retired MAIN slab); Characters works. 1P gated. |

## Current integration checkpoint

**Last qualified integration:** `a4eb24c9a85`, Boundary all three arms GREEN
2026-09-17 (Yoshi/Samus roots). Historical measurements: `PERF_LEDGER.md`.
The newer candidates below do not inherit that qualification.
The two new Samus roots cost +2,880 P50 / +8,768 P95, UNDER the 14,080 floor.
### Execution cursor

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
MF2 check passes 1,570 clips/29 raw exceptions; scoped audit recorded in receipt.
`FB299B25`: 2,043 presents/Results; low-water 9,672 B, reserve RED. Reload passes.
A7 loan 265,216 B. Trace accounts 36,300 B of play allocations; hats 16,188 B.
MF2 worst bank697,760 B unlinked; Kirby copy engagement still owed.
**A8 IMPLEMENTED_NOT_ACCEPTED**:17 host tests pass.
0BD4523E replay exact;141 BGM refills, no audio failures, arena +20,480 B.
AUD P95 6,528; WORK-H P95 2.625M/18.97 FPS, RED. Mixed audio identifies BGM.
HeavyF7443068 gains16,384 B; admission still5,904 B short, plus later play.
A8 FGM: ARM7 fills + 160 KiB arena landed 09-27 (FGM cache below).
**Solo 09-26/27** (receipts `2026-09-2{6,7}_p2-2p8-*`, all replay identical):
compact packet layout reverted (`BC3500EA` P95 1,955,392, was 2,626,368);
edge-only ROM bounce (1,892,160); FGM LRU/aligned/envelopes + ARM7 async fills
(1,852,672); 160 KiB FGM ring arena (-72 KiB); fast cpuGetTiming; stage no-Z
local W columns (STG P50 253K); status path (token index, path formatter,
ForgetRange skip); ARM pose clock; HUD state once per pass; stage witness off +
world pointer chain; reloc lookup memos; A5 hurtbox reject (shadow 0 flips);
ARM memcpy/memset/memcmp in ITCM, FGM id map, matrix leaves, searched flat-walk
cache; lab splits off; libgcc pose clock; HUD div15 (`2026-09-27_p2-2p8-fast-mem`):
MP f32 cache reads; billboard memo; texpool witness; digest word mix; lean phase clocks out; STG span: `FE3AF61F` WORK-H P50/P95/P99 1,131,008/1,584,000/1,984,128.
**Constraint**: CSS reserve >=183,072 B; margin 17,584 B less ~1.4 KB BSS (owed).
After static growth use `artifacts/performance/2026-09-23_css-preview-heap/tools/run-owner-css.ps1`.
Specs: `artifacts/performance/2026-09-23_p2-2p8-phase-specs/`.

Closed 09-22 bug sweep: owner fixes/deferred rows and remaining obligations are
in `p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-22.md` and
`docs/archive/P2_CLOSED_ROWS.md`. Last historical publication r54
`C8FC02AA2DF0BB6E` is saved in `builds/remaining-bugs-playtest-r54/`.
Boundary/Latest, per-hat captures and preview-pack test pin drift remain owed.

Preserve owner 1P/CSS work and the published ROM until gates pass. Older scoped
proofs: `p2/BUG_NOTES.md`, `docs/archive/P2_CLOSED_ROWS.md`.

## Queue — acceptance only

Ask subjective owner checks only after the required measurable proof. Missing
pixels/audio or unexercised states stay engineering work.

- P2-1 shell presentation; P2-2 four-way camera, HUD, Team feel, Results, Sudden Death.
- P2-3: Mario/Luigi pipe (`r1`), Luigi anim (`r2`), intro visibility (`r5`), CSS preview rebuild (`r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3f33 | Link entry wave/beam + specials | **PARTIAL — source programs implemented** | Retain Catch proof. Open: entry beam alpha, SpecialN empty-hand/catch frames, air Spin, ThrowF/ThrowB; Neutral-B/Spin need isolated source-default requalification. |
| P2-3 Samus | Morph-ball closure + **F-smash vanish** | **IMPLEMENTED; engagement owed** | Programs 2/3 morph; 4 F-smash hidden parts. Source-input roll/Bomb/F-smash proof remains; prior 1,536-frame CPU window did not morph. Details: `p2/BUG_NOTES.md`. |
| P2-3f46 | Yoshi stress arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`); pose values stay Q12. Run `test_pose_clock_differential.py` through the ROM oracle and measure cost. |
| P2-3f52 | Yoshi grab + egg lay/throw | **IMPLEMENTED; captures owed** | Two programs carry the 18→19 vector hidden part 4 (joint 9, `0x2800`) forces: Catch (`Catch`/`CatchPull`/`EggLay` 202-206) and Throw (+ joint 7 = `0x7D10`). Grab AND B-attack were ONE bug. Intro is **NOT** this class. OWED: captures. `…_p2-3f52-yoshi-root-programs/`. |
| P2-3f53 | EFDesc effects without native owners | **1 done; 3 blocked** | Falcon Punch/Kick done. Yoshi egg reverted (`252a9aa4290`: arena -4,096, 14 bind rejects). Kirby Vulcan: RGBA32/fidelity and residency. Pikachu Thunder: missing InputSpec/flag off. Two share the resident-budget blocker; `…_p2-3f53-vulcan-jab-blocker/`. |
| P2-3f54 | Weak stubs shadowing real bodies | **LANDED; runtime proof owed** | Wrappers + `itMainCheckShootNoAmmo` import; all six `T` in the shell ELF; atlas 4→5 sheets. |

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1..s8 | All eight VS stages | **REOPENED — guard repair proven; visuals open** | BG2 wallpapers draw all eight. Hyrule/Inishie proof: `2026-09-14_stage-hazard-guards.md`; Jungle/Zebes/Yamabuki captures and Castle/Inishie texture defects remain. |
| P2-4n1 | Native stage packet and actors | **38 blob packets plus Dream Land linked; acceptance open** | Host tests pass. Barrel submits but is unproved on screen; Lakitu/Bronto open. Sector Z crash candidates ranked in `p2/BUG_NOTES.md`. |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager and twenty common items | **SOURCE PRESENT; Sword lifetime repair recorded** | Remaining kinds, children, states, interactions and full natural-path acceptance stay open. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM; draw owners missing** | Dispatch proved (`gNdsItMonsterMakerMask` = `1fff`). Saffron monsters' VFX makers have no native owner. |
| P2-5i3 | Stage-spawned kinds | **8 OF 10 IN THE ROM; two behind the 1P flag** | Native owners exist for 1 of 42 item shapes. `MBallThrown` effect desc excluded on a false premise. |
| P2-5i4 | Pick up, throw, shoot and swing | **LANDED; acceptance open** | Pickup animation FileIDs resolved 09-09; `itMainCheckShootNoAmmo` weak stub in P2-3f54. |
| P2-5u1 | Item Switch and VS Options screens | **Entry/row repair committed** | `eafdf226c52`. Switch mask honoured by the spawn law; UI half uncensused. |
| P2-5x1 | Audio cue coverage | **SOURCE WIRED; ROM acceptance pending** | FGM header pins 573 entries over 47 banks; item TU audit clean (09-03/04). |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance, target `<1.12m` ticks | **STRUCTURAL BATCH FOCUS; RED** | Follow the Execution cursor. Retain applicable evidence; all scoped/integrated gates remain due. No universal PASS from one roster. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries; move closed detail out at once; keep rows short enough to decide the next action. The 12,288-byte cap is enforced by `check-docs.ps1`: when it trips, archive, do not raise it.
- After verified progress, update the existing row and permanent evidence. Distinguish local candidates, reproducible commits and accepted scope. Bank durable findings out of gitignored `builds/` scratch before handoff; leave owner reports in `BUGS.md` unchanged.
- Worktree audit: 20 auxiliaries outside `.worktrees/`, 17 dirty/ambiguous. Create none; a cleanup cycle must hash-migrate evidence first. Clean candidates: `builds/p2-v4-index-worktree`, `.codex-worktrees/startup-oom-run`, `_s64_itcm_measure`.
