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
| P2-1 VS shell | **Loop and realtime arms GREEN** | Raw `0x152` pin, owner-image lifetime and CSS particle re-init fixed; laps flat; realtime fenced. Owner's 7 CSS defects, 4 causes: Yoshi preview FIXED; walk now selects ALL fighters; FPS/music/dwell diagnosed, unfixed. |
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

Focus / batch / IDs / owner: P2-2p8 / lane selection / N05.04 / main. Phase: OWNER.
**NO CLASS REACHES THE GATE, INCLUDING LOCALITY**
(`…_p2-2p8-gate-decision/`): ceiling **90.6%**, leaving **44,208 OVER**.
**RESIDUAL: 321,866 UNFOUND**; banked+sized **146,678 = 31.3%**.
**CLOSED LANES** (detail archived): joint-cap placement, Kirby copy (OWED:
per-hat look), hidden-part class — 26 owners swept, 3 exist, all covered
(`…_p2-3f-hidden-part-sweep/`). OWED: captures.
**DTCM HOT SCALARS: −43,200 WORK-H P50 FOR 508 BYTES** (`5e109a47d5d`,
`…_p2-2p8-dtcm-hot-scalars/`). **Largest banked win**, 9.2% of gap. **OWNER:
one-line call** on the non-zero exit — it is a window assertion, not
correctness (detail archived).
**OWNER 09-17: SRC REOPENED** (was NO-GO), **30 Hz simulation still refused**.
Docs now committed (`cd270b735dc`). **SIZED** (`…_p2-2p8-ftr-stg-misc-sizing/`):
no one of FTR/STG/MISC closes the gap even deleted whole (STG 80.1%, FTR 72.8%,
MISC 49.6%); lanes sum to 124% of ALL so they do NOT add. **SRC DISTRIBUTION**
(`…_p2-2p8-src-distribution/`): **no big rocks** — 1,108 symbols under 5,000
carry 571,666, more than the whole gap. Largest class is soft-float 125,369
(26.1%), **~99.7% behind gameplay or fidelity gates. Closing the gap is a
POLICY call before it is an optimization.**
**LEVER SIZED: 8,458 B of entry-effect texels belong to fighters the four-CPU
build cannot run**, **0 shared** = **2.1 arena pages**
(`…_p2-2p8-entry-effect-roster-residency/`). Buys arena headroom, NOT ticks.
**OWNER 09-17 CSS BATCH** (`…_p2-css-owner-bug-list/`). Yoshi preview FIXED:
pack `source_bytes` 45,488 vs owner `asset_data_size` 44,256 declined the owner
(reject 3, before any root); the 1,232 is the pair weld. Same pass caught today's
Catch/Throw programs publishing un-aliased weld offsets past the asset end —
fixed, generator now asserts. **WALK REPLACED**: tours every admitted kind, 24
tics each, presses its own START; `CSSTOUR kind=/drew=` must be EQUAL. The old
walk covered 4 of 9, which is why Yoshi went unseen.
**ROSTER: rung 10 = +82,040 B image = −86,016 arena (21 pages) → SIGILL on CSS
exit; rung 7 control clean.** Rung 8 (Jigglypuff) is only **+9,440 B**;
bisecting 8/9. Kirby is the expensive member.

Shared causes banked 09-12 in `p2/BUG_NOTES.md` have rows below. Main owns shared
outputs/builds/timing; preserve other-owner 1P/CSS work. Settings stay 30 Hz
menus and 1P active. Retained proofs: `docs/archive/P2_CLOSED_ROWS.md`. Do not
replace the published P2 artifact until gates pass.

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
| P2-3f47 | Roster close: Ness, Purin, Kirby | **NDO6 + Kirby hat LANDED `1e80d39`; Kirby/Purin proofs OPEN** | Ness draws natively (nativefail 0). Open: Kirby copy-hat and Purin natural proofs, the image verifier's NORMAL re-bake with the image off (audit 14), alpha-zero guard; then the shell roster flip. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`); pose values stay Q12. Run `test_pose_clock_differential.py` through the ROM oracle and measure cost. |
| P2-3f52 | Yoshi grab + egg lay/throw | **IMPLEMENTED; captures owed** | Two programs carry the 18→19 vector hidden part 4 (joint 9, `0x2800`) forces: Catch (`Catch`/`CatchPull`/`EggLay` 202-206) and Throw (+ joint 7 = `0x7D10`). Grab AND B-attack were ONE bug. Intro is **NOT** this class. OWED: captures. `…_p2-3f52-yoshi-root-programs/`. |
| P2-3f53 | EFDesc effects without native owners | **ALL FOUR RESOLVED: 1 done, 3 blocked, none a wiring change** | **Falcon Punch/Kick DONE** (checked + registered; row was stale). **Yoshi egg** `0xa860` (= invisible intro AND shield): built clean, checker GREEN, Boundary RED — arena −4,096, AllocFail 84→85, 14 texture-bind rejects; reverted `252a9aa4290`. **Kirby Vulcan Jab**: BLOCKED, its state root branches to **RGBA32** and the DS has no 32-bit format — needs a lossy conversion + fidelity call, not a `case`. **Pikachu down-B Thunder**: format CLEAN (IA16/IA8, would compile) but `PikachuModel` is no InputSpec and gate is `NDS_P2_PIKACHU 0`. **2 of 4 blocked on the SAME resident budget; the per-roster emitter is the lever.** `…_p2-3f53-vulcan-jab-blocker/`, `…_p2-3f53-thunder-assessment/`. |
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
