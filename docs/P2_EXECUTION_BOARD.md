# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-15 after particle quad first-row indexing.

**Last integrated Boundary GREEN: `d8660bc2fd9` + owner overlay. Boundary
is blocked by `decomp/alt_assets/`; acceptance RED (P2-2p8).** Four-CPU
stress passes with native failures/rejects 0/0 and heap low-water 111,680 B.

**The only dynamic queue.** Normal restart reads `docs/HANDOFF.md` + this file.
Plans live in `docs/P2_PLAN.md` + `docs/p2/`. Closed row history lives in
`docs/archive/P2_CLOSED_ROWS.md`; measurements in `PERF_LEDGER.md`; chronology in
`PORTING.md`. Those large documents are lookup-only during ordinary work.

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
4. Last recorded published P2 ROM hash (carried forward, not reverified here):

SHA-256 2CB6B86242F9BF2B0CF8D99FF0405C1C4F87DE38F1A03AA51D3514BED421DF99

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **Loop and realtime arms GREEN** | Raw `0x152` pin, owner-image lifetime and CSS particle re-init fixed; laps flat; realtime fenced. Seven previews invisible; cadence/visual acceptance remains. |
| P2-2 Four-fighter engine | **Capacity GREEN; performance RED (P2-2p8)** | Four-kind FPCs use 125,108 B plus a 336 B foreign bank; BPS1 directory is resident. Whole-match low-water 111,680 B; libc reserve 40,960 B and weapon pool 10; every stress gate passes. |
| P2-3 Fighter production | **Acceptance OPEN** | Link Neutral-B/Spin have diagnostic output only. Samus morph proof needs human input. Preserve prior scoped proofs unless contradicted. |
| P2-4 Stage production | **Visual acceptance OPEN** | Collision parity passes. Audit-15 admission fixed/proved in `d8660bc2fd9`; natural Hyrule/Inishie counts/output pass. Three VS captures remain. |
| P2-5 Items | **Native coverage incomplete** | Sword lifetime repair recorded; fidelity-02 landed (`a8b6bd0`: Poke Ball procs, rock member, pool 10; ball/monster witness 09-14). Atlas membership, other kinds/children and interactions remain open. |
| P2-6 1P Game | **TALLY REACHED 09-14** | Guest-side playback wins stage 0 and reaches the source StageClear (score 31,940, three bonuses; `a1209354b`). Intro keeps the full-file path. Open: next-stage Intro OOM (144,640 B asked, 120,164 B free), 41,952 native failures by the tally, DL overflow 3,744 B (in flight); shipping flag stays 0. |
| P2-7 Modes & meta | **Options/Backup Clear accepted; DATA blue screen diagnosed** | Owner (09-06): Options and Backup Clear look good. DATA/VS Record/Sound Test draw only through the retired MAIN text slab (audit 16): bake their surfaces (fix queued). Characters blits a real surface. 1P stays gated. |

## Current integration checkpoint

**Current shared fix:** P2-2p8 particle row indexing after I/O/pose cuts.
Whole-match same-ROM WORK-H P95 **2,380,032 -> 2,376,576**, paired median
**-640** ticks/frame. Final stress **1,635,712/2,377,152**, native fail/reject
**0/0**, heap **111,680 B**; 118/1,973 presents are 2-VBlank. P2-2p8 remains RED.
Boundary stops on owner
`decomp/alt_assets/` after generator staleness.
In flight: menu-rules close, 1P next stage, roster proofs.
Next: remaining P2-2p8 whole-frame/SRC/tail cost, DATA surfaces (P2-7), weapon-pool sizing, remaining
stage captures/visual acceptance, dead code, remaining audit divergences
(BUG_NOTES 2026-09-13 block), P2-3r17, per-target particle outputs.
**Samus morph-ball:** programs 2/3 never reject; a level-3 CPU never rolls or
Bombs (`ftcomputer.c:801-809,4001-4010`); prove with a Samus-human playback
tour on the existing state-tour machinery (`0x9C/0x9D/0xE5/0xE6`).
Shared causes banked 2026-09-12 in `p2/BUG_NOTES.md` have rows below.

Main owns shared outputs and the serialized build. Keep unrelated dirty changes
and local 1P/CSS integration. Independent CSS, item, stage and campaign packages
may advance without waiting for another package's acceptance. Current owner
settings: 30 Hz menus, 1P active. Required final gates remain unchanged.

| Unit | Current evidence / boundary |
|---|---|
| Compact capacity + Link | Pushed `6c75e56f677`. `artifacts/visibility/2026-09-12_link-native-integration.md` pins isolated whole-match capacity and the Samus rejection; diagnostic captures are labeled. |
| Retained native proofs | DamageSlash `f36feff7e21`; Sword `2b60863c492`; Cutter `c3f79cf2801`; Donkey `35ab1a83dfe`; Samus Catch `5389765200f`; Link Catch `f4437339d28`; CopyLink `5e09e477b29`. |
| Options / 1P | Options `68c0e522d3c` / `f33c5aa039f`; compact preview producer `87c6be2549b`. Preserve local loaders/bridge and campaign work; route/preview checks do not close cadence, persistence or campaign acceptance. |

Permanent reports (each pins its own ROM/configuration and proof scope) live
under `artifacts/visibility/2026-09-11_*` (kirby-root-program-recovery,
damage-slash-native, sword-native-vram-lifetime, kirby-cutter-native,
donkey-low-modelpart-binding, samus-catch-native, link-catch-native),
`artifacts/visibility/2026-09-12_*` (kirby-copylink-native,
link-native-integration) and
`artifacts/performance/2026-09-10_pack-skeleton-ceiling/` (BATTLE_CORE_RECOVERY,
older CEILING). HIGH remains reachable; stripping it is not authorized. Current
builds do not replace the published P2 hash above until required gates pass.

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
| P2-2p8 | Four-CPU renderer/performance, target `<1.12m` ticks | **I/O + POSE + PARTICLE CUTS KEEP; RED** | Particle index P95 **-3,456**, paired median **-640**; final **1,635,712/2,377,152**, heap **111,680**. Evidence `2026-09-15_p2-2p8-particle-quad-index`. Continue whole-frame/SRC/tail cost. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries; move closed detail out at once; keep rows short enough to decide the next action.
- After verified progress, update the existing row and permanent evidence; handoff points here. Distinguish local candidates, reproducible commits and accepted scope. Record durable findings from gitignored `builds/` scratch before handoff. Keep owner reports in `BUGS.md` unchanged.
- Worktree audit: 20 auxiliaries outside `.worktrees/`, 17 dirty/ambiguous. Create none; a separate cleanup cycle must hash-migrate evidence first. Clean candidates: `builds/p2-v4-index-worktree`, `.codex-worktrees/startup-oom-run`, sibling `_s64_itcm_measure`.
