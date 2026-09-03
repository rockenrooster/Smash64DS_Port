# P2 Execution Board

Created: 2026-08-17.
Updated: 2026-09-02 (token-efficiency cleanup; active queue only, historical detail stays with archive/evidence owners. Pikachu and Yoshi landed opt-in -- rows P2-3f34..f45 in the archive; P2-3f46 open; P2-3f47 roster close in progress.)

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
4. The canonical current P2 ROM hash appears on exactly this line:

SHA-256 0636B28D063ADA82F1FBE24F4EABA379469B696ACAA1FB725A2754E5F501CF66

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **automated green; owner visual acceptance pending** | Implementation is closed. Only the final owner presentation re-check remains; history is archived. |
| P2-2 Four-fighter engine | **automated green; owner visual/play acceptance pending** | Four-player source semantics, HUD, camera, Results/Sudden Death, and memory/native-path gates are landed. Owner still owes the four-way presentation/team-feel pass. |
| P2-3 Fighter production | **IN PROGRESS — Link runtime acceptance is active; Pikachu and Yoshi landed opt-in** | Luigi/DK/Falcon/Samus production paths are live; Pikachu (`NDS_P2_PIKACHU`) and Yoshi (`NDS_P2_YOSHI`) are admitted, drawn, voiced and toured behind their flags. Link structural inventory, LinkBomb, entry packets, and current Boomerang/Spin data/native seams are landed; integrated natural gameplay/visual acceptance remains. |
| P2-4 Stage production | **IN PROGRESS — Yoshi's Island gameplay half landed opt-in (`NDS_P2_STAGE_YOSTER`)** | 8 VS stages. Yoster's collision, cloud platforms, camera and blast zones come from source; its presentation (native stage packet, particle banks, stage-select art, music) is row P2-4s1. |
| P2-5 Items | queued | System + 20 items + 13 Pokémon; stress = items ON. |
| P2-6 1P Game | queued | Campaign start-to-credits. |
| P2-7 Modes & meta | queued | Fresh-cart parity and P2 close gate. |

## Queue — acceptance only

These are owner checks, not implementation work unless a current reproduction
fails:

- P2-1: final shell presentation.
- P2-2: four-way camera, lower-screen HUD, Team Battle feel, Results and Sudden Death presentation.
- P2-3: Mario/Luigi pipe (`P2-3r1`), Luigi animation (`P2-3r2`), intro visibility (`P2-3r5`), CSS preview rebuild (`P2-3r7`), plus Falcon/Samus owner-feel passes.

## Queue — P2-3 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-3r17 | Intermittent fighter seams/holes around DK and Mario cap | **DEFERRED BY OWNER; not fixed** | Offline source geometry/vertex/matrix-routing/facing/fixed-point closures are green and culling was falsified. Remaining surface is runtime matrix supply. Cheapest next experiment when resumed: run `NDS_R2_FIGHTER_GX_COMPOSE=2`, inspect `gNdsR2GxComposeVerifyFail`, then compare transformed positions for one affected Mario-cap root. Do not spend current production time here without owner reprioritization. |
| P2-3f33 | Link entry wave/beam native graduation + integrated specials acceptance | **PARTIAL — static/native checks green; natural runtime/visual acceptance open** | Entry compiler/checker pins exact LinkSpecial2 roots and native packets. Current uncommitted extension also has Boomerang/Spin source data, s16 WPAttributes repair, TREE/TREE_DLLINKS submission, and native bake checks green. Next: one integrated natural Link route through entry, Neutral-B and Up-B; verify creation/draw/eject and visuals before closing. |
| P2-3f46 | Yoshi stress arm: the landed argmax moves (YoshiMain 146,928 B) and the Yoshi/Fox/Captain/Samus roster arm halts before its first sample | **OPEN** | `NDS_P2_FOUR_CPU_KIND0..3` build knobs landed (defaults = the P2-3f22 roster; the verifier reads them back). Next: attach at `scVSBattleStartBattle` on `build-yoshi-stress`, read the arena/syMalloc fail counters, size the reservation for the new argmax, bank the run. Detail: `docs/p2/fighters/yoshi.md`. |
| P2-3f47 | Roster close: Ness, Jigglypuff (Purin) and Kirby admitted opt-in in one slice (`NDS_P2_NESS` / `NDS_P2_PURIN` / `NDS_P2_KIRBY`) | **IN PROGRESS — generated, first full-roster build pending** | `scripts/fighters/admit_fighter.py` (SPECS-driven, replaces the hand-run Pikachu/Yoshi loop) + `derive_native_owner_tables.py` (owner tables from the generator's own falsifiers) produced the manifest rows, reloc/effect/entry/HUD/CSS/PlayersVS seams, native owners (slots 9/10/11), status promotion and the three audio banks (two-segment Purin anim stems; 221/203 loop-prefix cues; pack ceiling 3 -> 6 MiB; four 16 kHz snores). Next: build `build-roster` with all ten flags, both-CPU smokes (proof fighter 11/10/8), CSS capture, Boundary, then the stress arm on the new argmax with P2-3f46. Detail: `docs/p2/fighters/{ness,jigglypuff,kirby}.md`. |
| P2-3f48 | ITCommonData (0xfb) residency | **OPEN — scoped, cheap, unblocked** | **3,392 B**, not 82,976: its only dependency, MiscData086, already ships with Yoshi. It carries the art for *every* common, monster and stage item, so it gates all of P2-5. Steps and exact insertion points: `docs/archive/P2_CLOSED_ROWS.md`. |
| P2-3f49 | Arena capacity on the ten-fighter ROM | **NO LONGER BLOCKING; not closed** | The ROM reaches gameplay (1,381 presented frames) since the animation cache started keeping the match's fighter cost free (`c0fcfa6306b`). It now reserves **no cache at all** there and streams every animation, which is real acquisition cost and is owner-parked performance work. Census and the sized static levers (Mario/Fox owner tables ~64,147 B — a frozen combined export, not generator output; prepared-dense to slot scratch ~65,784 B) are in the archive. |

## Queue — P2-4 engineering

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-4s1 | Yoshi's Island (Yoster), first stage through the pipeline | **IN PROGRESS** | Gameplay half landed behind `NDS_P2_STAGE_YOSTER` (default 0): source `gryoster.c` included verbatim, admission arm, reloc payload, ground data, stage-select slot, cloud-vapor particle bank. Remaining: native stage packet (law 8), stage-select art bake, music track, asset rows. Source pins and the Dream Land pipeline trace are in the archive. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance repair, target `<1.12m` ticks | **PARKED BY OWNER** | Source-correct native stage routing and retained-Q20/cache cuts are landed. Corrected four-kind work remains well above target; the remaining owner is source animation acquisition/representation plus diffuse simulation. Resume only when owner un-parks performance work; use `scripts/verify-p2-four-fighter-stress.ps1 -Build` for A/B and bank verbose output. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries here; move closed row detail out immediately.
- Keep each active row short enough to decide the next action without loading its historical investigation.
- Search owner docs/evidence for detail instead of expanding this board.
- After verified progress, update this queue and the owning evidence doc; do not duplicate the same result across restart surfaces.
