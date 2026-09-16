# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-16 continuation protocol; recorded runtime evidence unchanged.

**Last integrated Boundary GREEN: `5bfb784f8ec`; acceptance RED (P2-2p8).**
Four-CPU native failures/rejects 0/0; heap low-water 112,192 B.

**The only dynamic queue.** Normal restart reads `docs/HANDOFF.md` + this file.
Plans live in `docs/P2_PLAN.md` + `docs/p2/`. Closed row history lives in
`docs/archive/P2_CLOSED_ROWS.md`; measurements in `PERF_LEDGER.md`; chronology in
`PORTING.md`. Large documents are lookup-only during ordinary work.

## Standing rules

1. **Measurement law:** `docs/VERIFYING.md` owns procedure. Boundary is
   `p2_shell_loop`, `p2_battle_realtime`, and `p2_fourcpu_stress`.
   `p2_battle_realtime` is mode 163: shell-driven Mario human vs level-3 Fox,
   Dream Land, one-minute Time, items off. Gate arms use the one-minute match;
   long soak length is a separate flag.
2. Cadence verdicts use all presented frames; the 1,600-frame gameplay rank-80
   remains candidate-sizing evidence. Measure the configuration that actually
   ships (`nds_build_config.h` is truth).
3. **Publish law:** P2 publishes only verifier-covered `smash64ds.nds`, from the
   shipping VS shell with human input, no scripted walk, and no fast logic.
   Rebuild it after each verified fix batch. The frozen P1 artifact is not
   rebuilt routinely.
4. Published P2 ROM after `5bfb784f8ec`, runtime-verified 2026-09-15:

SHA-256 6BF344EF097A9237A5AD6AADE44E9F1FF601F503250DA80B73957B0B64C9C703

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
| P2-6 1P Game | **TALLY REACHED 09-14** | Guest-side playback wins stage 0 and reaches the source StageClear (score 31,940, three bonuses; `a1209354b`). Intro keeps the full-file path. Open: next-stage Intro OOM (144,640 B asked, 120,164 B free), 41,952 native failures by the tally, DL overflow 3,744 B (in flight); shipping flag stays 0. |
| P2-7 Modes & meta | **Options/Backup Clear accepted; DATA blue screen diagnosed** | Owner (09-06): Options and Backup Clear look good. DATA/VS Record/Sound Test draw only through the retired MAIN text slab (audit 16): bake their surfaces (fix queued). Characters blits a real surface. 1P stays gated. |

## Current integration checkpoint

**Last recorded qualified checkpoint (not the live candidate):** fixed generic particle submit (`5bfb784f8ec`).
Recorded stress WORK-H **P50 1,654,208 / P95 2,375,296**, MISC **P50 253,824 / P95 481,024**;
native fail/reject **0/0**, heap **112,192 B**. Full Boundary GREEN; P2-2p8 RED.
### Execution cursor

Focus / batch / IDs / owner: P2-2p8 / particle-camera reuse / N04 / main. Phase: RECORD.
Identity/receipt: HEAD `941f4daab56` + camera overlay; hard-on ROM
`77A047D1...EEFEBB`, ELF `2A83B39B...CB75A`; see
`artifacts/performance/2026-09-15_p2-2p8-particle-camera-reuse/README.md`.
Completed/rejected: camera reuse KEEP; pose draw/topology rejected; packet live-flush
left no retained delta; current profile: `2026-09-15_p2-2p8-current-profile/`.
Next: commit/push this coherent KEEP, then SELECT the next P2-2p8 batch.
Checks/status: particle-bank + Boundary + hard-on native-only build GREEN. Stress WORK-H
P50 1,660,224 / P95 2,380,288; reuse 5,919; native fail/reject 0/0; heap 108,096 B.
P2-2p8 remains RED / `IMPLEMENTED_NOT_ACCEPTED`.
Job: none; hard-on build + Boundary r5 GREEN. Config hash `BDFE5951...F0E0`.
Capability limits: none relevant. Review watermark: `Briefs/README.md` inspected
2026-09-16; visual/menu candidates preserved for their owning rows.

Shared causes banked 2026-09-12 in `p2/BUG_NOTES.md` have rows below.

Main owns shared outputs/builds/timing. Preserve other-owner 1P/CSS work.
Settings remain 30 Hz menus and 1P active; all requirements and coverage stand.

Retained: compact/Link `6c75e56f677` + `2026-09-12_link-native-integration.md`;
native proofs `f36feff7e21`, `2b60863c492`, `c3f79cf2801`, `35ab1a83dfe`,
`5389765200f`, `f4437339d28`, `5e09e477b29`; Options/1P `68c0e522d3c`,
`f33c5aa039f`, preview `87c6be2549b`. Scoped reports stay under
`artifacts/visibility/2026-09-11_*`, `2026-09-12_*` and
`artifacts/performance/2026-09-10_pack-skeleton-ceiling/`.
HIGH stays reachable; stripping it is not authorized. Do not replace the published
P2 artifact above until the candidate's required gates pass.

## Queue — acceptance only

Request subjective owner checks only after required measurable proof. Missing
pixels/audio or unexercised states remain engineering work, not feel-only review.

- P2-1 shell presentation; P2-2 four-way camera, lower HUD, Team feel, Results and Sudden Death.
- P2-3: Mario/Luigi pipe (`P2-3r1`), Luigi animation (`P2-3r2`), intro visibility (`P2-3r5`), CSS preview rebuild (`P2-3r7`), Falcon/Samus feel.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Intermittent fighter seams/holes around DK and Mario cap | **UN-DEFERRED 09-13; READY** | N64-to-DS raster coverage mismatch, not missing geometry; production fix is a bounded AOT guard band in the owner generator. Analysis: `docs/BUGS.md`. |
| P2-3f33 | Link entry wave/beam + specials | **PARTIAL — source programs implemented** | Retain Catch proof. Open: entry beam alpha, SpecialN empty-hand/catch frames, air Spin (effect-only), ThrowF/ThrowB programs; Neutral-B/Spin need isolated source-default requalification. |
| P2-3 Samus | Morph-ball source program closure | **IMPLEMENTED LOCALLY; engagement owed** | Programs 2/3 use roots `0x8158/0x8708`; Catch stays 1. CPU window 1,536 did not morph. Use source controller input for roll/Bomb and canonical restoration. |
| P2-3f46 | Yoshi stress arm: the landed argmax moves and the roster arm halts before its first sample | **BLOCKED behind P2-2p8** | Same tick-HUD ceiling as the four-CPU arm; resume with it. |
| P2-3f47 | Roster close: Ness, Jigglypuff and Kirby | **NDO6 + Kirby hat LANDED `1e80d39`; Kirby/Purin proofs OPEN** | Hidden-part `0x116`, Cutter and CopyLink `0x122..0x127` have reports above. Ness draws natively (nativefail 0). Open: Kirby copy-hat and Purin natural proofs, the image verifier's NORMAL re-bake with the image off (audit 14), alpha-zero guard; then the shell roster flip. |
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
- After verified progress, update the existing row and permanent evidence; handoff points here. Distinguish local candidates, reproducible commits and accepted scope. Record durable findings from gitignored `builds/` scratch before handoff. Keep owner reports in `BUGS.md` unchanged.
- Worktree audit: 20 auxiliaries outside `.worktrees/`, 17 dirty/ambiguous. Create none; a separate cleanup cycle must hash-migrate evidence first. Clean candidates: `builds/p2-v4-index-worktree`, `.codex-worktrees/startup-oom-run`, sibling `_s64_itcm_measure`.
