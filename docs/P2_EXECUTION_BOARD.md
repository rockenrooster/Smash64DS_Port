# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-17.

**Last integrated Boundary GREEN: N04.08; P2-2p8 acceptance RED.** Figures below.

**The only dynamic queue.** Restart reads `docs/HANDOFF.md` + this file. Plans:
`docs/P2_PLAN.md` + `docs/p2/`. Closed rows: `docs/archive/P2_CLOSED_ROWS.md`.
Measurements: `PERF_LEDGER.md`. Chronology: `PORTING.md`.

## Standing rules

1. **Measurement law:** `docs/VERIFYING.md` owns procedure and each entry's
   coverage. Boundary = `p2_shell_loop`, `p2_battle_realtime`,
   `p2_fourcpu_stress`. Gate arms use the one-minute match; soak is a separate flag.
2. Cadence verdicts use all presented frames; the 1,600-frame gameplay rank-80
   remains candidate-sizing evidence. Measure the configuration that actually
   ships (`nds_build_config.h` is truth).
3. **Publish law:** P2 publishes only verifier-covered `smash64ds.nds` from the
   shipping VS shell with human input — no scripted walk, no fast logic. Rebuild
   after each verified fix batch; the frozen P1 artifact is not rebuilt routinely.
4. Published P2 ROM after N04.08 + the clean rebuild, 2026-09-16. **Runtime proof
   owed** — the payload changed since the last runtime verification:

SHA-256 C6574420A9FC0E77B670093CE7AE1B595A62583488C5A9D72DD367877B0E9477

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **Loop and realtime arms GREEN** | Raw `0x152` pin, owner-image lifetime and CSS particle re-init fixed; laps flat; realtime fenced. Final roster tour is 12/12 native (`kind=fff drew=fff`, every triangle bucket nonzero); FPS/music/dwell remain open. |
| P2-2 Four-fighter engine | **Capacity GREEN; performance RED (P2-2p8)** | Four-kind FPCs use 125,108 B plus a 336 B foreign bank; BPS1 directory resident. Low-water 111,680 B; libc reserve 40,960 B, weapon pool 10; scoped guards pass; FPS RED. |
| P2-3 Fighter production | **Acceptance OPEN** | Link Neutral-B/Spin have diagnostic output only. Samus morph proof needs human input. Preserve prior scoped proofs unless contradicted. |
| P2-4 Stage production | **Visual acceptance OPEN** | Collision parity passes. Audit-15 admission proved in `d8660bc2fd9`; natural Hyrule/Inishie pass. Three VS captures remain. |
| P2-5 Items | **Native coverage incomplete** | Sword lifetime repair recorded; fidelity-02 landed (`a8b6bd0`). Atlas membership, other kinds/children and interactions remain open. |
| P2-6 1P Game | **TALLY REACHED 09-14** | Guest playback wins stage 0 and reaches source StageClear (31,940, three bonuses; `a1209354b`). Open: next-stage Intro OOM (144,640 B asked, 120,164 B free), 41,952 native failures by the tally, DL overflow 3,744 B; shipping flag stays 0. |
| P2-7 Modes & meta | **Options/Backup Clear accepted 09-06; DATA blue screen diagnosed** | DATA/VS Record/Sound Test draw only through the retired MAIN text slab (audit 16): bake their surfaces (fix queued). Characters blits a real surface. 1P stays gated. |

## Current integration checkpoint

**Last qualified checkpoint:** `a4eb24c9a85` -- Yoshi + Samus root programs,
both resolvers registered, accounting literal fixed. **Boundary GREEN all three
arms 2026-09-17.** WORK-H **1,600,960 / 2,320,576**, FTR **350,144 / 736,960**,
STG 385,088; heap low-water 111,200 B; arena 1,351,424 B; native 0/0; slips 0.
The two new Samus roots cost +2,880 P50 / +8,768 P95, UNDER the 14,080 floor.
### Execution cursor

Focus (owner 09-22): **P2-2p8 four-fighter 30 FPS -- architecture.**
Plan: `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md` (keep the rules, replace the
machinery; pillars A1-A10, phases 0-7). Owner rulings D1-D7 recorded in its
section 8: gate stays P95; renderer replaced outright; tolerance classes approved,
body-hurtbox hold kept; no run-ahead; visual reserve case by case; every motion
resident via a new compact format (N02.04 stands); custom ARM7 audio. Phase:
**Phase 0 instrument landed** (`3d62c6abf26`): WORK-H P50 1,689,088 / P95
4,207,488 / P99 4,753,152, two-VBlank 5.7%. P95 is owned by a re-record
episode (a texture-VRAM fence storm, per slice 1) and the gate throws on
Link AppearL native failures -- both are Phase 1 scope. MF verdict **yes**
(candidate B, 0.382x on the worst roster, byte-exact; bind cost is the Phase 3
risk). **Phase 0 closed**: the shipping-config census found the heaviest
reachable roster (Captain/Link/Pikachu/Kirby) halts at battle load, ~130 KB
short (`artifacts/performance/2026-09-23_p2-2p8-shipping-heap-census/`); Phase 3's
RAM work is now a correctness prerequisite. **Phase 1 in progress**:
slice 1 landed (`c33274f8345`: Samus LOW lean path, exact oracle, route 1 FTR
P95 -11%); it found the P95 episode is a texture-VRAM fence storm and Link's
entry failure is VRAM exhaustion. Slice 2a (`f1476de32dd`) measured it:
fragmentation in a full A+B, and BG3's bank D empty in VS battles. Slice 2b
(`bd29b08e282`, word `gNdsFtrLeanAdmit`=2, default 0): WORK-H P95 4.21M ->
2.53M, Link entry failures 0. Slice 2c (`b20b8f0f2b2`): whole admission with
no new RAM, uploads after GO 0, WORK-H P95 2.45M at word 2. Slice 3
(`08736558f3b`): lean for all four stress kinds, FTR P50 229K / WORK-H P50
1.51M at route 1. **Slice 4 in progress**: host-generated fighter lists.
Found: the first 1P battle OOMs at load (pre-existing); integrator works the
battle HUD file bake (A7 memory) in parallel. Alongside (disjoint files, integrator): MF host
encoder + C decoder + checker. ARM7 audio spec queued (one subagent at a time). Specs: `artifacts/performance/2026-09-23_p2-2p8-phase-specs/`.
Evidence: `artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/`,
`artifacts/performance/2026-09-23_p2-2p8-phase0-baseline/`.

Previous focus: remaining BUGS sweep / serial integration / main. Phase: **CLOSED
09-22.** Every `docs/BUGS.md` row the owner
opened is either removed by the owner (fixed through r54) or owner-DEFERRED:
C1 CSS hover-to-preview delay (profile + resume plan: `p2/BUG_NOTES.md` "C1")
and S3 Saffron door. Only a descriptionless "-VS options" line remains.
Status table: `docs/p2/REMAINING_BUGS_IMPLEMENTATION_PLAN_2026-09-22.md`.

**ROOT ROM = r54** `C8FC02AA2DF0BB6E` (copy in
`builds/remaining-bugs-playtest-r54/`), pushed through `2a144b426d0`.
Owner-closed this sweep: castle roof (wrap period), eyes (texture-part byte
lane + run-memo fence), Zebes, Yoshi egg roll, MK BGM, Dream Land (r40
revert), Link's CSS boots (preview pack Span T, `538862570e7`).

**Owed:** Boundary/Latest never run on this tree; the r52 memo fence and r54
pack growth (Link +1,336 B, Kirby +1,368 B inside the fixed 80 KiB CSS block)
are proven by probes and host tests only. `test_preview_pack_loader.py` fails
at collection on a pre-existing pin drift (task chip offered).
P2-2p8 policy remains parked below.
**NO CLASS REACHES THE GATE, INCLUDING LOCALITY** (`..._p2-2p8-gate-decision/`):
ceiling **90.6%**, **44,208 OVER**; residual **321,866 unfound**. CLOSED LANES
archived; OWED: per-hat look, captures.
**DTCM HOT SCALARS: -43,200 WORK-H P50 FOR 508 B** (`5e109a47d5d`); 9.2% of gap.
**OWNER: one-line call** on its non-zero exit.
**OWNER 09-17: SRC REOPENED**, **30 Hz still refused**; ~99.7% of the largest
class is gameplay/fidelity gated, so it is a **POLICY call** (detail archived).
**OWNER 09-17 ROSTER CLOSED (P2-3f47).** Detail in the closed-row archive.
**CLEANUP AUDIT** done; proof-fleet REFUTED (20/43); O1 reclaim 5 pages.

Shared causes banked 09-12 in `p2/BUG_NOTES.md` have rows below. Main owns shared
outputs/builds/timing; preserve other-owner 1P/CSS work. Retained proofs:
`docs/archive/P2_CLOSED_ROWS.md`. Do not replace the published artifact until
gates pass.

## Queue — acceptance only

Ask subjective owner checks only after the required measurable proof. Missing
pixels/audio or unexercised states stay engineering work.

- P2-1 shell presentation; P2-2 four-way camera, HUD, Team feel, Results, Sudden Death.
- P2-3: Mario/Luigi pipe (`r1`), Luigi anim (`r2`), intro visibility (`r5`), CSS preview rebuild (`r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3f33 | Link entry wave/beam + specials | **PARTIAL — source programs implemented** | Retain Catch proof. Open: entry beam alpha, SpecialN empty-hand/catch frames, air Spin, ThrowF/ThrowB; Neutral-B/Spin need isolated source-default requalification. |
| P2-3 Samus | Morph-ball closure + **F-smash vanish** | **IMPLEMENTED LOCALLY; engagement owed** | Programs 2/3 use roots `0x8158/0x8708`; Catch stays 1. CPU window 1,536 did not morph. Use source input for roll/Bomb. F-smash is now program 4: `0x00180000` installs drawing hidden parts 11/12 (`0x2c20`/`0x2ce8`), 16 roots vs canonical 14, neither offset was resident. Derived, not observed — confirm on hardware. |
| P2-3f46 | Yoshi stress arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`); pose values stay Q12. Run `test_pose_clock_differential.py` through the ROM oracle and measure cost. |
| P2-3f52 | Yoshi grab + egg lay/throw | **IMPLEMENTED; captures owed** | Two programs carry the 18→19 vector hidden part 4 (joint 9, `0x2800`) forces: Catch (`Catch`/`CatchPull`/`EggLay` 202-206) and Throw (+ joint 7 = `0x7D10`). Grab AND B-attack were ONE bug. Intro is **NOT** this class. OWED: captures. `…_p2-3f52-yoshi-root-programs/`. |
| P2-3f53 | EFDesc effects without native owners | **ALL FOUR RESOLVED: 1 done, 3 blocked, none a wiring change** | **Falcon Punch/Kick DONE** (row was stale). **Yoshi egg** `0xa860` (= invisible intro AND shield) built clean but Boundary RED (arena −4,096, 14 texture-bind rejects); reverted `252a9aa4290`. **Kirby Vulcan Jab** BLOCKED: its state root branches to RGBA32, needs a lossy conversion + fidelity call. **Pikachu down-B Thunder** format CLEAN but `PikachuModel` is no InputSpec, gate `NDS_P2_PIKACHU 0`. **2 of 4 blocked on the SAME resident budget.** `…_p2-3f53-vulcan-jab-blocker/`. |
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
