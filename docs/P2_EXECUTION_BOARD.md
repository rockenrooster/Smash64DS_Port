# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-16 continuation protocol; recorded runtime evidence unchanged.

**Last integrated Boundary GREEN: N04.08; P2-2p8 acceptance RED.** Figures below.

**The only dynamic queue.** Restart reads `docs/HANDOFF.md` + this file. Plans:
`docs/P2_PLAN.md` + `docs/p2/`. Closed rows: `docs/archive/P2_CLOSED_ROWS.md`.
Measurements: `PERF_LEDGER.md`. Chronology: `PORTING.md`. Those are lookup-only.

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
4. Qualified hard-on P2 ROM after N04.08, runtime-verified 2026-09-16:

SHA-256 E9AF96F8BEA5E2786824B02919DFC89799A70934B0F6F09FD5417ED93F1DFCA6

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **Loop and realtime arms GREEN** | Raw `0x152` pin, owner-image lifetime and CSS particle re-init fixed; laps flat; realtime fenced. Seven previews invisible; cadence/visual acceptance remains. |
| P2-2 Four-fighter engine | **Capacity GREEN; performance RED (P2-2p8)** | Four-kind FPCs use 125,108 B plus a 336 B foreign bank; BPS1 directory is resident. Whole-match low-water 111,680 B; libc reserve 40,960 B and weapon pool 10; scoped correctness/resource guards pass; FPS remains RED. |
| P2-3 Fighter production | **Acceptance OPEN** | Link Neutral-B/Spin have diagnostic output only. Samus morph proof needs human input. Preserve prior scoped proofs unless contradicted. |
| P2-4 Stage production | **Visual acceptance OPEN** | Collision parity passes. Audit-15 admission fixed/proved in `d8660bc2fd9`; natural Hyrule/Inishie counts/output pass. Three VS captures remain. |
| P2-5 Items | **Native coverage incomplete** | Sword lifetime repair recorded; fidelity-02 landed (`a8b6bd0`: Poke Ball procs, rock member, pool 10; ball/monster witness 09-14). Atlas membership, other kinds/children and interactions remain open. |
| P2-6 1P Game | **TALLY REACHED 09-14** | Guest playback wins stage 0 and reaches the source StageClear (31,940, three bonuses; `a1209354b`). Intro keeps the full-file path. Open: next-stage Intro OOM (144,640 B asked, 120,164 B free), 41,952 native failures by the tally, DL overflow 3,744 B (in flight); shipping flag stays 0. |
| P2-7 Modes & meta | **Options/Backup Clear accepted 09-06; DATA blue screen diagnosed** | DATA/VS Record/Sound Test draw only through the retired MAIN text slab (audit 16): bake their surfaces (fix queued). Characters blits a real surface. 1P stays gated. |

## Current integration checkpoint

**Last qualified checkpoint:** N04.08 material-animation stable-zero skip.
WORK-H **1,639,808 / 2,365,120**, FTR **357,312 / 744,640**; native fail/reject
**0/0**, heap **108,096 B**; Boundary and hard-on build GREEN.
### Execution cursor

Focus / batch / IDs / owner: P2-2p8 / N04.09 candidate selection / main. Phase: SELECT.
**Owner 2026-09-16: four-CPU optimization runs the four-CPU match and nothing
else** — `p2_fourcpu_stress` alone, no shell loop, realtime arm or extra lab
builds; conditions in `VERIFYING.md`. Boundary is for integration/publication.
Completed/rejected: N04.03, N04.05 and N04.08 KEEP; N04.04, N04.06 and N04.07
REJECTED. Figures in `PERF_LEDGER.md`.
N04.08 closed and published: same-ROM **-8,640** WORK-H P50, canonical re-bank
**-9,920 / -8,512**, engagement 7,892 in four builds, audit SAFE, Boundary GREEN.
Evidence, plus the stale-NitroFS finding:
`artifacts/performance/2026-09-16_p2-2p8-mobj-stable-skip/`.
Rejected without a build, detail in `docs/archive/P2_CLOSED_ROWS.md`: the
`ndsR2CamDiv64` ITCM admission (a hardware-divider busy-wait; ITCM cannot recover
divider latency), any `.text.hot` admission (`linker/nds_hot_text.ld` calls that
list closed in both directions), and N04.09's first premise, the FAT
cluster-chain rewalk, which a fresh profile shows had already shrunk to 0.34%.
Profile: `artifacts/performance/2026-09-16_p2-2p8-n0409-profile/` (129 regions, 0
discontinuities). Non-idle ranking: soft float **6.54%** (fadd 2.67, fmul 2.11,
fdiv 0.94, F32AddBits 0.83) largest theme; pose **3.95%** largest subsystem;
`ndsFighterMarioFoxDLAllDrawForSlot` **2.42%** largest single non-leaf symbol;
memset+memcpy 2.68%. It is a fresh build dir (365 NitroFS files) against the
canonical baseline's 710: rank symbols with it, do not predict gate deltas.
Next: attribute the float leaves to their callers
(`census-softfloat-callers.ps1`, four-CPU target) before sizing a candidate.
Checks: hard-on `smash64ds` rebuilt/published, `NATIVE_ONLY_PASS` 262 inputs,
`check-published-roms.ps1` GREEN, hash in rule 4; nothing owed on N04.08.
Open owner decision: whether to clean-rebuild and re-baseline the canonical
directory, which moves every banked absolute level (deltas within it stay valid).
P2-2p8 stays RED at WORK-H P50 1,639,808.
Falsifier: settled batches reopen only for a recorded invalidator.
P2-2p8 remains RED / `IMPLEMENTED_NOT_ACCEPTED`; N04.05 and N04.08 settled KEEP.
Job: none; main owns all edits, builds and the focused runner.
Review watermark: `Briefs/README.md` inspected 2026-09-16; visual/menu candidates
stay with their owning rows. Owner documentation edits are preserved.

Shared causes banked 2026-09-12 in `p2/BUG_NOTES.md` have rows below.

Main owns shared outputs/builds/timing. Preserve other-owner 1P/CSS work.
Settings remain 30 Hz menus and 1P active; all requirements and coverage stand.

Retained commits and scoped reports: `docs/archive/P2_CLOSED_ROWS.md`, section
"Retained P2 proofs (moved off the board 2026-09-16)".
HIGH stays reachable; stripping it is not authorized. Do not replace the published
P2 artifact above until the candidate's required gates pass.

## Queue — acceptance only

Ask for subjective owner checks only after the required measurable proof. Missing
pixels/audio or unexercised states stay engineering work, not feel-only review.

- P2-1 shell presentation; P2-2 four-way camera, lower HUD, Team feel, Results and Sudden Death.
- P2-3: Mario/Luigi pipe (`P2-3r1`), Luigi animation (`P2-3r2`), intro visibility (`P2-3r5`), CSS preview rebuild (`P2-3r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Intermittent fighter seams/holes around DK and Mario cap | **UN-DEFERRED 09-13; READY** | N64-to-DS raster coverage mismatch, not missing geometry; production fix is a bounded AOT guard band in the owner generator. Analysis: `docs/BUGS.md`. |
| P2-3f33 | Link entry wave/beam + specials | **PARTIAL — source programs implemented** | Retain Catch proof. Open: entry beam alpha, SpecialN empty-hand/catch frames, air Spin (effect-only), ThrowF/ThrowB programs; Neutral-B/Spin need isolated source-default requalification. |
| P2-3 Samus | Morph-ball source program closure | **IMPLEMENTED LOCALLY; engagement owed** | Programs 2/3 use roots `0x8158/0x8708`; Catch stays 1. CPU window 1,536 did not morph. Use source controller input for roll/Bomb and canonical restoration. |
| P2-3f46 | Yoshi stress arm: the landed argmax moves and the roster arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby | **NDO6 + Kirby hat LANDED `1e80d39`; Kirby/Purin proofs OPEN** | Ness draws natively (nativefail 0). Open: Kirby copy-hat and Purin natural proofs, the image verifier's NORMAL re-bake with the image off (audit 14), alpha-zero guard; then the shell roster flip. |
| P2-3c1 | Exact pose clock | **WIRED; runtime differential/cost owed** | Binary32 clock replaces Q12 timing (`f6f65a…`, `nds_f32_exact.h`); pose values stay Q12. Run `test_pose_clock_differential.py` live set through ROM oracle and measure cost. |
| P2-3f52 | Yoshi grab, egg lay, egg throw, entry egg | **OPEN — no Yoshi root programs** | `OWNER_ROOT_PROGRAMS` has only samus/link; egg weapon, egg-lay and entry egg have no owner. Derive from `247_YoshiMain.c` like Link Catch. |
| P2-3f53 | EFDesc effects without native owners | **OPEN** | Falcon Punch/Kick, Pikachu Thunder head/trail/shock, Kirby Vulcan Jab, Yoshi shield: `generate_nds_entry_effects.py` roots + lookup + admission + check (Kirby cutter is the example). |
| P2-3f54 | Weak stubs shadowing real bodies | **LANDED; runtime proof owed** | Wrappers + `itMainCheckShootNoAmmo` import; all six `T` in the shell ELF; particle atlas 4→5 sheets (110/119 scripts). |

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1..s8 | All eight VS stages | **REOPENED — guard repair proven; visuals open** | BG2 wallpapers draw all eight. Hyrule/Inishie proof: `2026-09-14_stage-hazard-guards.md`; Jungle/Zebes/Yamabuki captures and Castle/Inishie texture defects remain. |
| P2-4n1 | Native stage packet and actors | **38 blob packets plus Dream Land linked; acceptance open** | Host tests pass. Barrel submits but is unproved on screen; Lakitu/Bronto open. Sector Z crash candidates beyond the Arwing basis guard are ranked in `p2/BUG_NOTES.md` (laser spawn matrix, fighter pick, reflector owner). |

## Queue — P2-5 items

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-5i1 | Item manager and twenty common items | **SOURCE PRESENT; Sword tested-lifetime repair recorded** | Blade/hilt and entry-texture lifetime proof linked above. Remaining kinds, children, states, interactions and full natural-path acceptance stay open. |
| P2-5i2 | The 13 Poke Ball Pokemon | **ALL 13 IN THE ROM; draw owners missing** | Dispatch proved (`gNdsItMonsterMakerMask` = `1fff`); a ball opens only when thrown or hit. Saffron monsters' VFX makers (DustLight/DustCollide/MultiExplode) have no native owner. |
| P2-5i3 | Stage-spawned kinds | **8 OF 10 IN THE ROM; two behind the 1P flag** | Native owners exist for 1 of 42 distinct item shapes. `MBallThrown` effect desc is excluded on a false premise (`gITManagerCommonData` links). |
| P2-5i4 | Pick up, throw, shoot and swing | **LANDED; acceptance open** | Pickup animation FileIDs resolved 09-09; `itMainCheckShootNoAmmo` weak stub in P2-3f54. |
| P2-5u1 | Item Switch and VS Options screens | **Entry/row repair committed; acceptance open** | `eafdf226c52`. Switch mask honoured by the spawn law; UI half uncensused. |
| P2-5x1 | Audio cue coverage | **SOURCE WIRED; ROM acceptance pending** | FGM header pins 573 entries over 47 banks; item TU audit clean (09-03/04). |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance, target `<1.12m` ticks | **STRUCTURAL BATCH FOCUS; RED** | Follow the Execution cursor. Retain applicable evidence; all scoped/integrated gates remain due. No universal PASS from one roster. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries; move closed detail out at once; keep rows short enough to decide the next action.
- After verified progress, update the existing row and permanent evidence. Distinguish local candidates, reproducible commits and accepted scope. Bank durable findings out of gitignored `builds/` scratch before handoff; leave owner reports in `BUGS.md` unchanged.
- Worktree audit: 20 auxiliaries outside `.worktrees/`, 17 dirty/ambiguous. Create none; a cleanup cycle must hash-migrate evidence first. Clean candidates: `builds/p2-v4-index-worktree`, `.codex-worktrees/startup-oom-run`, `_s64_itcm_measure`.
