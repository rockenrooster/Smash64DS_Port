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

**Isolated owner experiment (2026-09-30): Meta Knight, P4.** Worktree
`.worktrees/meta-knight`, branch `codex/meta-knight`, base `2e093297c5c`.
Owner explicitly requested a new CSS entry and playable match; this branch does
not reassign the main agent's P2 performance work. Worktree lifetime: 2026-10-07.
Phase: VERIFY; native freeplay build passed, gameplay proof is next.
Status: `IMPLEMENTED_NOT_ACCEPTED`, checkpoint `92bddf1b3e4` pushed to
`origin/codex/meta-knight`. Build five session 53064 exited 2, log `build-fifth.log`:
native stage generation passed and C compilation began; static textures rejected
the missing local `ExternDataBank103`. Static-texture census and unconditional
Dream Land water 103/104 readers are repaired; two override/negative fixtures pass.
The remaining legacy CSS and shield-pose chains are repaired; four new and four
existing affected fixtures pass. All prior failure/job history is in the receipt.
All eight credit includes are closed through producer-owned reference read/hash
and local encoded/metadata output receipts. Build eight session 82919 exited 2 at
ARM9 link; all producers completed and ARM7's five-input native-only check passed.
The existing strong export is `ndsBaseFTCommonJumpAerialUpdateModelYaw`; Meta's
two-line binding now uses it. Build nine session 30239 exited 0, log `build-ninth.log`.
ROM: `builds/build-p4-meta-knight/smash64ds-p2-shell-freeplay-hwtri.nds`,
64,928,768 B, SHA256 `C0F93220BBD24477A3E7AE62CEF9FF4B0A69856C4B8D274CFD84BB8FD5D3422F`.
ARM9 native-only passed over 324 actual inputs, ARM7 over five; overlay check passed.
Freeplay config confirms Meta 1, fast logic 0, menu walk 0. Identity: checkpoint
`92bddf1b` plus the documented read-root/Make/credit overlay.
Gameplay/capture source is frozen; root's overlay is documentation.
Prior build failures, source checks and all unique logs are retained in the receipt.
`runtime_integration` is now the **sole integrator/producer/build/timing writer**;
root performs documentation/review only until ownership returns. Accurate emulator
boot-policy check session 6161 exited 0. O2R prerequisites are repaired; no target
emulator has launched. Direct stream producer: 1,570 clips, zero mismatches and
pinned hash preserved. Private Meta 172 resources are staged; final link/package
and native-object checks are still pending.
Capture/bounce/hat source fixtures pass; nine lifecycle copies emitted. Helpers
are frozen. Main collectors block timing/visual acceptance.
Owned PID49456/session97088 exited; main baked6 collector fleet is active.
Title/UI load passes, but next kind60 is automatic Explain, with input ring/count0;
the prior natural Start claim is retracted. Explain has a real data abort; counter
192 is boot sizing-probe failures, not per-scene allocation failures. Per-kind
Mario/Luigi heaps exclude a direct Meta-global-maximum cause; first fault needed.
Private UI profile now caps60FPS (JIT off, same storage/ports); no game state edits.
Owned 60FPS MI session21235/PID29316 localizes input only during the main fleet;
bindings confirmed in UI, instantaneous keypress still lacks a positive witness.
OSK transport failed at a higher Windows integrity barrier; user must close it.
Only owned PID53264/MI93574 remains halted at title with exception breakpoint4.
Seed1, playback0, empty saves/states, backup result3; no game state writes.
User was asked to signal readiness for a brief held Enter; response pending.
Next: coordinated manual input, then natural keyboard
CSS/SSS/match/Results/rematch proof. Target `smash64ds-p2-shell-freeplay-hwtri`,
`BUILD=build-p4-meta-knight`, Meta flag 1; no main writes or P1 changes.
Evidence:
`artifacts/performance/2026-09-30_p4-meta-knight/README.md`.
Checks owed: deterministic import, legacy-ID regression, native model/motions,
specials/jumps/CPU/copy, CSS/HUD/audio/results, natural-input match and Latest.
ROM build passes; no gameplay/runtime/performance PASS yet. Runtime integration owns all
producers/builds; read-root validation and other character helpers are frozen.
Thread-capacity failures recorded. Family 6 alpha remains one native experiment,
not an accepted visual delta. No emulator/cadence proof exists yet.

The main P2 cursor at branch creation is preserved in
`archive/P4_META_KNIGHT_BASE_CURSOR_2026-09-30.md`. Main continues its own
four-fighter performance lane; this worktree executes the owner-requested P4 experiment.


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
