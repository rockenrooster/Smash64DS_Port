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

SHA-256 E8BFE8DF3DF0DBDD3DBC0BDAAFD16DA51468CB907B39D039AE417F7BEEB13766

5. Performance/visibility evidence is permanent under `artifacts/performance`
   and `artifacts/visibility`. Device A/B reports include 2/3/4/5+ VBlank
   histogram, max interval, and P50/P95.

## Phase status

| Phase | State | Gate summary |
|---|---|---|
| P2-1 VS shell | **automated green; owner visual acceptance pending** | Implementation is closed. Only the final owner presentation re-check remains; history is archived. |
| P2-2 Four-fighter engine | **automated green; owner visual/play acceptance pending** | Four-player source semantics, HUD, camera, Results/Sudden Death, and memory/native-path gates are landed. Owner still owes the four-way presentation/team-feel pass. |
| P2-3 Fighter production | **IN PROGRESS — Link runtime acceptance is active; Pikachu and Yoshi landed opt-in** | Luigi/DK/Falcon/Samus production paths are live; Pikachu (`NDS_P2_PIKACHU`) and Yoshi (`NDS_P2_YOSHI`) are admitted, drawn, voiced and toured behind their flags. Link structural inventory, LinkBomb, entry packets, and current Boomerang/Spin data/native seams are landed; integrated natural gameplay/visual acceptance remains. |
| P2-4 Stage production | queued | 8 VS stages. |
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
| P2-3f48 | ITCommonData residency: pack and load `llITCommonDataFileID` (0xfb) so the item-common effects resolve | **OPEN** | `dEFManagerCaptureKirbyStar/LoseKirbyStar/MBallThrown` read their DObj descs from `gITManagerCommonData`, which no ROM loads (efmanager.c:1285/1592/1622). Kirby's spit and lose-copy stars are stubbed to NULL (`battleship_kirby_common.h`, accepted visual delta) and the Pikachu/Purin Master Ball entry article is still missing for the same reason; P2-5 items need the file regardless. Steps: reloc row for 0xfb (size/alloc from the corpus), pack it, load at battle start where the fighter extern closures load, drop the two stubs, add the three descs to the roster list. |
| P2-3f49 | The ten-flag ROM exhausts the arena before its first battle frame | **OPEN — measured 2026-09-02, blocks Kirby's smoke** | `probe-arena-overflow.ps1` on `build-roster` (all ten fighter flags, Ness vs Fox, both CPU): arena `chosen=1,347,584`, **93 failed allocations**, halt on a 1,332-byte request with **272 bytes** left, caller inside fighter pose binding (`artifacts/verification/2026-09-02_arena-overflow.txt`). The ROM links and clears the boot cliff (`check-boot-headroom.ps1`: 118,464 bytes of proven headroom), so this is arena exhaustion during battle setup, not the boot freeze. **Measured cause:** static footprint grew, and binary growth costs the arena 1:1 -- the arena fell from **1,515,520** (2026-08-25 four-CPU lab, `allocfail=8`) to 1,347,584, a loss of 167,936 bytes, because this is the first ROM ever built with all seven opt-in fighters at once. Do NOT assume every motion file is resident: `ndsRelocAssetLoadFighterStreamClip` streams animation clips per use (Task 75), so the true residency is core files plus the match's working set and must be READ, not summed from the manifest. Kirby's smoke requires this configuration (Copy links against all ten neutral-Bs), so this row gates it; P2-3f46 (the four-CPU arm halting before frame 98) is the same defect with four fighters. **Sized 2026-09-02 against a Mario control built from the same commit:** the control's arena is **1,622,016** and it runs 965 presented frames; the ten-flag ROM's is **1,347,584** and it halts. The 274,432-byte loss matches the ten flags' static growth of **259,387 bytes** (text +138,112, data +97,176, bss +28,224) to the arena granule, confirming binary growth costs the arena 1:1. The growth is a long tail of per-fighter precomputed render tables -- `sNdsNative<X>FighterPreparedDense`/`...Low`/`...DenseNormals` for the five new owners are about 60 KB of it, `sNdsP2FighterAnimTokens` another 10,384 -- so there is no single lever. Direction: those tables are per-owner precomputed data that every build carries whether or not the owner is in the match, which is what the NitroFS owner images (`nitro:/fighters/<x>_high.bin`, loaded by `ndsRendererNativeEnsureOwnerImage`) already solve for the model data. Move them the same way rather than trimming. |
| P2-3f50 | PurinMain's external pointer fixups fail, so his parts container is never resolved | **OPEN — cause confirmed 2026-09-02; the failure was silent until now** | `probe-battle-progress.ps1` (new) on `build-purin-cpu` (`NDS_P2_PURIN=1` alone, proof fighter 10, both CPU): `presented=0`, and the ARM9 is parked in calico's `__excpt_entry` with `cpsr=0x90000097` (abort mode). Banked registers name the faulting caller: `lr_usr=lbCommonSetupFighterPartsDObjs+86`, `r7=0xeff9ff80` (Purin's own setup-parts mask, so that word read correctly), `r0=r3=0x4f640000` (an unrelocated-looking word). **Not resources:** arena `chosen=1,597,440` (larger than the ten-flag ROM's), `allocfail=32`, **`openfail=0`, `streamfail=0`** -- the animation-segment fix in 1fa52c906f9 cleared the earlier resolution failures. The suspect is a pointer fixup on PurinMain/PurinModel reaching the common-parts DObjDesc array. **CAVEAT, read before re-measuring:** the working tree at build time (16:32) carried another agent's uncommitted P2-3f47 work, written 15:14-15:25 and compiled into this ROM: owner-image-size arms for all five new owners in `nds_renderer_assets.c`, and a block of Ness "admission witness" diagnostic globals in `renderer_adapter_fighter.c` whose own comment says it is chasing an unassigned admission failure. That is in-flight debugging in the fighter-owner path, not corruption, but it is not the committed tree either. Isolate on a clean checkout of 1fa52c906f9 before attributing the abort to Purin, and coordinate with whoever owns that witness. |

## Queue — P2-2 performance debt

| ID | Slice | Status | Next / evidence |
|---|---|---|---|
| P2-2p8 | Four-CPU renderer/performance repair, target `<1.12m` ticks | **PARKED BY OWNER** | Source-correct native stage routing and retained-Q20/cache cuts are landed. Corrected four-kind work remains well above target; the remaining owner is source animation acquisition/representation plus diffuse simulation. Resume only when owner un-parks performance work; use `scripts/verify-p2-four-fighter-stress.ps1 -Build` for A/B and bank verbose output. |

## Queue discipline

- Keep only red/current/deferred/owner-acceptance summaries here; move closed row detail out immediately.
- Keep each active row short enough to decide the next action without loading its historical investigation.
- Search owner docs/evidence for detail instead of expanding this board.
- After verified progress, update this queue and the owning evidence doc; do not duplicate the same result across restart surfaces.
