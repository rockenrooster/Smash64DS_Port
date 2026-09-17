# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-17.

**Last integrated Boundary GREEN: N04.08; P2-2p8 acceptance RED.** Figures below.

**The only dynamic queue.** Restart reads `docs/HANDOFF.md` + this file. Plans:
`docs/P2_PLAN.md` + `docs/p2/`. Closed rows: `docs/archive/P2_CLOSED_ROWS.md`.
Measurements: `PERF_LEDGER.md`. Chronology: `PORTING.md`. Lookup-only.

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
| P2-1 VS shell | **Loop and realtime arms GREEN** | Raw `0x152` pin, owner-image lifetime and CSS particle re-init fixed; laps flat; realtime fenced. Seven previews invisible; cadence/visual acceptance open. |
| P2-2 Four-fighter engine | **Capacity GREEN; performance RED (P2-2p8)** | Four-kind FPCs use 125,108 B plus a 336 B foreign bank; BPS1 directory resident. Low-water 111,680 B; libc reserve 40,960 B, weapon pool 10; scoped guards pass; FPS RED. |
| P2-3 Fighter production | **Acceptance OPEN** | Link Neutral-B/Spin have diagnostic output only. Samus morph proof needs human input. Preserve prior scoped proofs unless contradicted. |
| P2-4 Stage production | **Visual acceptance OPEN** | Collision parity passes. Audit-15 admission proved in `d8660bc2fd9`; natural Hyrule/Inishie pass. Three VS captures remain. |
| P2-5 Items | **Native coverage incomplete** | Sword lifetime repair recorded; fidelity-02 landed (`a8b6bd0`). Atlas membership, other kinds/children and interactions remain open. |
| P2-6 1P Game | **TALLY REACHED 09-14** | Guest playback wins stage 0 and reaches source StageClear (31,940, three bonuses; `a1209354b`). Open: next-stage Intro OOM (144,640 B asked, 120,164 B free), 41,952 native failures by the tally, DL overflow 3,744 B; shipping flag stays 0. |
| P2-7 Modes & meta | **Options/Backup Clear accepted 09-06; DATA blue screen diagnosed** | DATA/VS Record/Sound Test draw only through the retired MAIN text slab (audit 16): bake their surfaces (fix queued). Characters blits a real surface. 1P stays gated. |

## Current integration checkpoint

**Last qualified checkpoint:** N04.08 + clean rebuild + the 2026-09-16 cleanup +
the 3,779-line shim trim + wave-1 collapse + 1,024 B arena alignment. **Boundary
GREEN all three arms 2026-09-17.** WORK-H **1,588,544 / 2,301,504**, FTR
**353,280 / 741,376**, STG 345,600; heap low-water 111,680 B; arena 1,355,520 B;
native 0/0; slips 0.
### Execution cursor

Focus / batch / IDs / owner: P2-2p8 / lane selection / N05.04 / main. Phase: SELECT.
**PERFORMANCE: NO CLASS REACHES THE GATE — INCLUDING LOCALITY.**
`…_p2-2p8-gate-decision/`; sizing `…09-17_p2-2p8-locality-sizing/`.
**CORRECTED 09-17:** locality's ceiling was published as 560,739 = **113%** of
the gap; that subtracted ALL data stall. Layout removes only **line fills** =
**424,336 = 90.6%**, so a PERFECT data cache leaves **1,164,208 — 44,208 OVER
the gate**.
Sized subset 15-18% (+11.2% VRAM arena); largest item BANKED below.
`GObj`/`DObj` repack is unblocked: `include/` precedes decomp on `-I` and every
include uses the angle form, so a shadow header needs no decomp edit (HIGH —
forks a decomp type). A scalar's literal-pool base load costs MORE than the
dereference and DTCM does not touch it — packing hot scalars into one anchored
struct is a better-shaped lane. **OWNER DECISION
FORCED:** no structural class closes 468,544 — the gate needs Sacrifice Order
2+3 (fewer joints / fewer transformed objects; `gNdsGCDrawsActiveMax` 203) or
order 4. Placement stays CLOSED (hazard; arena 1,024 B aligned).
Checks: Boundary GREEN 3/3 09-17; four-CPU PASSES 09-17 (hats off).
Any-roster contract now measurable (`…_roster-variance/`).
**KIRBY COPY: implemented, GATED OFF, blocked on BYTES** (`ddf18a57a86`,
`…_p2-3f47-kirby-copy-hats/`). All 10 hats bake, closure GREEN — but the sections
append to KIRBY's resident image (**+28,848 B**) and the arena cannot absorb it.
A/B: hats ON = **151 native failures** (witness root 0x18A60 = Kirby **STONE**),
heap **73,064** vs 111,680, WORK-H **+70,016**; hats OFF = gate PASSES.
`KIRBY_TRIO_ADMIT_COPY_HATS = False` until the sections move into the per-slot
**hat images**; sharing the cross-head-identical arrays gives only ~29%.
**DTCM HOT SCALARS: −43,200 WORK-H P50 FOR 508 BYTES** (`5e109a47d5d`,
`…_p2-2p8-dtcm-hot-scalars/`). **Largest banked win of the campaign** — 3.1x the
14,080 floor, 9.2% of the gap, P95 −43,072, 3 runs agreeing. Linker script only,
**no source change**. Lane was closed on a break-even derived as bytes÷32: right
for a contiguous table, wrong for a scalar owning a whole line — 85.0 tk/fr per
DTCM byte vs the reverted table's 6.4. Native 0/0, triangles identical.
**Per-PC re-profile CONFIRMS the lane**: identical 3,364.0 dereference
accesses/fr in both arms, stall **34,121 → 9,079 (−73.4%)**, literal-pool row
−1.7% — placement cannot move one row and not the other. Direct recovery 25,042;
the other 18,158 reads as eviction relief (inferred). Non-zero exit is a
**window** assertion, NOT correctness: all 21 ring stops at identical presented
frames with identical `PacingLogicFrames` — SAME match — only the stitcher's
first label moves +1; 3 of the assertion's 4 conditions pass.
**OWNER: one-line call** to compare `startFrame` against the recorded label span.
1,484 B DTCM left. `IMPLEMENTED_NOT_ACCEPTED` — owed: that call only.
**OWNER INPUT 09-16:** `docs/optimization/{FTR,STG,SRC,MISC}.md` (2,503 lines).
SRC's top candidate sized NO-GO; FTR/STG/MISC UNSIZED — size before building.
Watermark: `Briefs/README.md` 09-16.

Shared causes banked 2026-09-12 in `p2/BUG_NOTES.md` have rows below. Main owns
shared outputs/builds/timing; preserve other-owner 1P/CSS work. Settings stay
30 Hz menus and 1P active. Retained proofs: `docs/archive/P2_CLOSED_ROWS.md`.
HIGH stays reachable; stripping it is not authorized. Do not replace the
published P2 artifact until the candidate's gates pass.

## Queue — acceptance only

Ask for subjective owner checks only after the required measurable proof. Missing
pixels/audio or unexercised states stay engineering work.

- P2-1 shell presentation; P2-2 four-way camera, lower HUD, Team feel, Results, Sudden Death.
- P2-3: Mario/Luigi pipe (`r1`), Luigi animation (`r2`), intro visibility (`r5`), CSS preview rebuild (`r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Fighter seams/holes around DK and Mario cap | **UN-DEFERRED 09-13; READY** | Raster coverage mismatch, not missing geometry; fix is a bounded AOT guard band in the owner generator. Analysis: `docs/BUGS.md`. |
| P2-3f33 | Link entry wave/beam + specials | **PARTIAL — source programs implemented** | Retain Catch proof. Open: entry beam alpha, SpecialN empty-hand/catch frames, air Spin, ThrowF/ThrowB; Neutral-B/Spin need isolated source-default requalification. |
| P2-3 Samus | Morph-ball source program closure | **IMPLEMENTED LOCALLY; engagement owed** | Programs 2/3 use roots `0x8158/0x8708`; Catch stays 1. CPU window 1,536 did not morph. Use source input for roll/Bomb. |
| P2-3f46 | Yoshi stress arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby | **NDO6 + Kirby hat LANDED `1e80d39`; Kirby/Purin proofs OPEN** | Ness draws natively (nativefail 0). Open: Kirby copy-hat and Purin natural proofs, the image verifier's NORMAL re-bake with the image off (audit 14), alpha-zero guard; then the shell roster flip. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`); pose values stay Q12. Run `test_pose_clock_differential.py` through the ROM oracle and measure cost. |
| P2-3f52 | Yoshi grab, egg lay/throw, entry egg | **OPEN — no Yoshi root programs** | `OWNER_ROOT_PROGRAMS` has only samus/link. Derive from `247_YoshiMain.c` like Link Catch. |
| P2-3f53 | EFDesc effects without native owners | **OPEN** | Falcon Punch/Kick, Pikachu Thunder, Kirby Vulcan Jab, Yoshi shield: `generate_nds_entry_effects.py` roots + lookup + admission + check. |
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
