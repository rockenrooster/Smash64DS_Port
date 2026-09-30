# Bug D: impact wave sometimes lacks green

Investigator start: 2026-09-30 14:01 UTC. Repository: `D:\Stuff\DevFolder\Smash64DS_Port`.
Status: **INVESTIGATION FINISHED — missing-green symptom NOT REPRODUCED; root cause NOT CONFIRMED.** Twelve bounded launches/attempts used; no build or tracked-file edit. Writes are confined to this report/evidence directory and `builds/codex-impact-wave-green*` scratch paths.

## Result for the integrator

Green/yellow plus white was captured on retained **r54**, retained **r55 with NDL on**, and the initialized **Mario/Kirby/Fox/Yoshi four-CPU lab**. The final same-ROM **r55 NDL-off** control also shows green and actually engages the source display proc. No captured failing colour event supports a regression/pre-existing classification.

The requested colour is already present: all inspected standard collision callers select **index 4**, whose primitive is white but whose texture TLUT contains the green outer band. Do **not** change the maker to index 1. The five baked palettes and the texels are byte-identical in the retained r54/r55 ELFs; measured VRAM palette dumps agree with variant 4.

There is one concrete **unconfirmed state-inheritance candidate**: stage and lean-fighter FIFO producers invalidate the port's texture trackers without invalidating libnds's active texture identity. A same-name `glBindTexture` can therefore elide a necessary hardware rebind. The proposed minimal producer fix below is a reviewable hypothesis, **not a proven repair or permission to close Bug D**. The healthy captured events requested a different active texture, so they did not exercise that failure condition.

| Qualified diagnostic event | Actual roster/arm | Maker/draw window | Pixel / palette result |
|---|---|---|---|
| `lab-image-on` | Mario/Kirby/Fox/Yoshi CPUs, NDL=1 | first maker 290; six native draw observations | Green; exact variant-4 VRAM palette; GX COLOR register `0x7fff` |
| `r55-startup` | shipping Mario-human/Fox-CPU preset, NDL=1 | first maker 340; six draws | Green at capture 344 and fading green at 346; exact palette; COLOR `0x7fff` |
| `r54-image` | same Mario/Fox preset, source display route | first maker 340; six draws | Green in captures 342/344/346; exact palette; COLOR `0x7fff` |
| `r55-ndl-off` | same r55/preset, NDL=0 | first maker 340; six draws and **six source display-proc calls** | Green; exact palette; same visible first-event behaviour as NDL on |

All events are Dream Land. No fighter position/velocity/status was injected in these qualified observations. Published-ROM roster-four and wall/special-move collision coverage remain **unrun**, as do later-match/intermittent and sibling acceptance gates. These are debugger-paused visual diagnostics, not performance qualification: ticks/FPS/P50/P95 were not measured authoritatively; HUD FPS values must not be banked.

## Scope and known facts

- Owner observed intermittent missing green during an r55 VS match with Mario, Kirby, Fox, and Yoshi.
- r55: `builds/p2p8-playtest-r55/smash64ds.nds`, reportedly built from `1246ddfe5a1`.
- r54 accepted reference: `builds/remaining-bugs-playtest-r54/smash64ds.nds`.
- Source effect is `nEFKindImpactWave`; its maker accepts a colour index. The retained ROMs link the strong imported-source maker and native textured geometry. The older substitute description in the shim comment is stale.
- The current uncommitted `src/port/nds_p2_hurtbox_reject.c` edit is outside r55 and is excluded from analysis.
- Runtime work is restricted to accurate repo-local melonDS runner slot 3, GDB port 3330, interpreter with JIT disabled. No builds planned.

## Reproduction and measurements

Twelve launches/attempts, including failed helpers and one cancelled overlap, are recorded below. Qualified captures show green; the owner symptom was not reproduced. Regression classification remains **unknown**.

Binary measurement (`palette-analysis.json`): the 160-byte five-palette table and 256-byte source texel table are identical in the retained r54/r55 ELFs. Of 315 opaque texels, 138 use a green-dominant RGB555 colour in variant 4. The two `*-white-variant-source-texture.png` files visualize decoded binary data, **not emulator captures**. The analysis script is `builds/codex-impact-wave-green-palettes.py`.

### Run 1 plan (r55 default arm)

- Missing coverage: actual effect draw colour and bound VRAM palette, not supplied by existing M1 replay/geometry receipts.
- Relocate the byte-identical reserved slot 3 executable/config into `builds/codex-impact-wave-green-slot3`; retain port 3330/3331, interpreter/JIT off, software renderer. This preserves the assigned slot's isolation and confines all automatic writes to scratch. No other slot is run or mutated.
- Scratch ELF-matched r55 ROM; break before `ndsBaseSCManagerRunLoop`. Seed source default scene to VSBattle and source default battle state to Mario/Kirby/Fox/Yoshi four-CPU, Dream Land, one-minute Time, items off. All byte fields are modified through aligned words. This bypasses menus for a diagnostic match; it is not natural menu/lifecycle acceptance.
- Observe natural CPU collisions; break on maker and native draw. Record index, primitive/environment colour, typed material, texture name, palette object/address, and outgoing GX colour registers. Temporarily expose E/F/G to LCD at the first bind, dump the palette, then restore the exact VRAMCNT word before resuming. Capture three presented frames through `capture-melonds.ps1`'s extracted capture/window functions, forcing PrintWindow and holding the shared capture mutex.
- Launcher `builds/codex-impact-wave-green-run.ps1`; GDB template `builds/codex-impact-wave-green-observe.gdb`; status `builds/codex-impact-wave-green-r55-default.status`; stdout/stderr use that same prefix. Timeout 900 s; wrapper kills only its own emulator/GDB processes.

14:41 UTC: run 1 launched detached, wrapper PID 35348. Copied ROM/executable hashes exactly match the retained r55 and assigned slot 3. Pre-run static validation exposed two recipe mistakes immediately after launch: the seeded game-rules byte 2 means Stock (the source Time bit is 1); the optional outgoing-colour breakpoint was at a bounds-counter instruction rather than the colour store. Keep this run as diagnostic Stock evidence if it reaches gameplay; do not claim the intended Time recipe or outgoing-colour coverage. Correct those scratch instructions for the next run after this writer exits. Source inputs and ROM remain unchanged.

Run 1 outcome: **helper failure; no valid runtime evidence**. GDB stdout is empty; stderr reports ELF and `.gdb` paths with unintended whitespace. The wrapper exit 0 was not a valid completion marker. No maker/draw was observed. Also corrected an adjacent-digit regex replacement in the copied TOML and added TOML parse validation before retry. Evidence: `r55-default-gdb-errors.txt`. Scratch launcher now requires the `DONE` marker; captured frames will also require visual inspection.

### Run 2 plan (r55 default, corrected helpers)

Same frozen r55 ROM and reserved slot-3 byte-identical binary/port. Source Time bit 1; otherwise the run-1 intended roster/stage/item recipe. Fixed GDB quoting, actual colour-store breakpoint (+6764), and hidden-window discovery. Prefix `builds/codex-impact-wave-green-r55-natural`; retain the three diagnostic captures and first bound palette. This retry supplies wholly missing runtime coverage; it repeats no valid completed evidence.

Run 2 outcome: **diagnostic startup failure, no wave coverage**. ARM9 exception in `mpCollisionInitGroundData` at `src/port/reloc_backend_compat_shims.c:13136` (LR `0x020a38e0`), before any maker or native draw. Wrapper stopped both owned processes; no corrupt run was continued. The direct default-scene seed therefore does not yet constitute a working recipe on r55. `r55-natural-gdb.txt` preserves the stopped stack; it is not evidence that the reported colour bug itself crashes.

Static draw-state hazard under investigation: `src/nds/nds_stage_gx.exec.inc:354-369` sends a stage FIFO packet and invalidates the renderer bound-name/active-entry trackers, but does not update libnds `glGlobalData.activeTexture/activePalette`. libnds `glBindTexture` (r55 ELF `0x0211f804`) compares its software active texture with the requested name and can return before writing TEXIMAGE/PLTT. A later ImpactWave requesting the still-recorded libnds name could inherit stage hardware state. This is a **hypothesis requiring observed bind/capture or same-ROM repair**, not an accepted cause.

### Run 3 plan (existing four-CPU lab, no build)

Use the existing `builds/build-p2p8-lab-im/smash64ds-p2-fourcpu-tickhud-hwtri.nds` and its matching ELF (built 2026-09-29 14:43 local). This is explicitly a separate lab identity, not r55. Its shipped direct-battle harness owns setup, avoiding the invalid shipping scene seed. Poke only aligned lab words: `gNdsLabFourCpuKinds=0x06010800` (Mario/Kirby/Fox/Yoshi), `gNdsLabFourCpuGkind=6` (Dream Land); default NDL arm 1. No position/velocity/status injection. Native entry breakpoint corrected to the raw function address so ARM stack arguments are sampled before the prologue; bind/colour instruction offsets are resolved from this ELF. Prefix `builds/codex-impact-wave-green-lab-default`. Timeout 900 s; same captures/palette and completion requirements. Missing coverage remains real colour/binding state; runs 1/2 supplied none.

Run 3 outcome: **positive wave engagement, incomplete colour sampling**. Completed at presented frame 249: maker calls 7 (first frame 225, index 4, position -1377.8/904/0), native submits observed 36; native fallback and maker-null counts 0; five native variants prepared. Outgoing GX COLOR was sampled from its actual store register, `r3=0x7fff` (white), on 25 frames. `lab-default-gdb.txt` preserves the trace. The first stack-derived variant/palette was stale/invalid (5 while actual native bind used variant 4), so `lab-default-palette.bin` is invalid evidence. Initial pre-initialization word reads were also invalid (`gNdsP2Ndl` read 31 before runtime data initialization); intended roster/arm must be re-seeded after initialization and observed. No capture fired because the selected end-frame condition could never hold while waves drew every frame. No missing-green conclusion follows from this run.

### Run 4 plan (r55; earlier seed, register sampler)

Missing questions: working r55 diagnostic startup, actual variant, VRAM palette, and image. Seed defaults at `scManagerRunLoop` **before** `ndsDevSceneHarnessApply` first reads/caches them; run 2 seeded after that read. Read the variant from `r1` immediately after the callee loads it (+164), never from newly written stack. Sample the palette on the third draw and capture at first-wave frame +2/+4/+6. Prefix `builds/codex-impact-wave-green-r55-measured`; default NDL arm observed after runtime initialization. ROM remains `AB60DFA8...`; no build.

Run 4 outcome: same pre-wave ground-setup exception as run 2. No wave/capture coverage; the direct shipping seed is still invalid. Stop this startup detour. Evidence `r55-measured-gdb.txt`.

### Run 5 plan (lab, initialized seed and corrected captures)

Use the already-working frozen lab identity (ROM `EA24D28BD9783A3FC2A4DB1191B809ECEAE35D0C4E023982F2DAAE23458AE8DF`, ELF `706A91028FE50151B0D6074B46332903639C8C3911702ED7490039E6A70EAEBE`). Set the lab words only after SDK/data initialization at `ndsMatchConfigLoadMarioFoxDreamLand` entry. Force/observe `gNdsP2Ndl=1`; fix variant sampling to the loaded register and capture predicate to first-wave +2/+4/+6. Same slot-3 scratch binary/config/port and 900-s timeout. Prefix `builds/codex-impact-wave-green-lab-capture`. This extends missing coverage (valid roster/arm, palette, pixels); run 3's actual white outgoing colour remains useful.

Run 5 outcome: **valid native colour-state measurements; capture helper failed**. Initialized arm reads NDL=1, roster word `0x06010800`, Dream Land=6. First natural maker frame 290, index 4, position 133/1542/0. Actual loaded variant register is 4. Native texture name 61, PAL16 texture format `0x6d105128`, palette index 61, PLTT base `0x90`, palette LCD address `0x06890900`. VRAM dump `lab-capture-palette.bin` is byte-identical to the r54/r55 variant-4 palette (all 16 entries). Actual outgoing COLOR store register `r3=0x7fff`. Completed frame 314, one maker, six observed wave submissions, zero fallback/null, five variant prepares. Incoming libnds active texture was 51, so the suspected same-name elision is **not engaged** by this event. The capture calls fired at 292/294/296 but a scratch PowerShell `catch` syntax error produced no images. Evidence `lab-capture-gdb.txt`, errors file, palette dump.

### Run 6 plan (lab NDL=1, image gap only)

Same exact lab ROM/ELF, initialized roster/stage, first-wave frame anchors. Corrected the capture helper's parser error and validate the whole helper before launch. Prefix `builds/codex-impact-wave-green-lab-image-on`. Existing valid palette/variant/white GX-colour measurements remain valid; this rerun supplies only the missing image evidence. No build.

Run 6 outcome: **green wave captured**. Three PrintWindow images at frames 292/294/296 (`lab-image-on-wave1/2/3.png`) clearly show the green/yellow upper wave and white lower portion, after the frame-290 natural collision on Dream Land's upper platform. Icons/models confirm Mario/Kirby/Fox/Yoshi. Same six native draw observations, same variant 4 and white outgoing COLOR; palette remains exact. This event does not reproduce the missing-green symptom.

### Run 7 plan (r55, preserve startup then source handoff)

Missing coverage: retained shipping r55. Prior direct entry bypassed the normal startup scene. Keep the normal startup scene, seed the default battle descriptor at `scManagerRunLoop` entry, and intercept the first `ndsSceneManagerRequest` at the completed startup handoff to select VSBattle (22). This keeps startup asset/relocation initialization and the normal scene request/arena transition. Game type seed is source Royal=1; one-minute Time; Mario/Kirby/Fox/Yoshi CPUs; Dream Land; items off. Capture/samplers match run 6. Prefix `builds/codex-impact-wave-green-r55-startup`. Still a diagnostic source descriptor/scene request, not a menu/lifecycle acceptance run.

Run 7 outcome: **retained r55 wave is green; actual roster differs from intended seed**. The normal startup-to-battle handoff (`current=27`, requested next=1, redirected to 22) is a working r55 recipe. Startup overwrote the default battle seed with the shipping Mario-human/Fox-CPU preset, so the three images show two fighters. First natural wave frame 340, index/actual variant 4, position 950.515/0/0; six native draws, zero fallback/null; correct VRAM palette and `r3=0x7fff`. Images `r55-startup-wave1/2/3.png` at frames 342/344/346 show strong green at 344 and fading green at 346. The earliest capture has little visible wave behind the fighters; do not label it a palette failure. Evidence `r55-startup-gdb.txt`, palette dump. Missing-green not reproduced in this event.

### Run 8 plan (r55, four-fighter source descriptor producer)

Missing coverage: owner roster on the retained r55. Intercept the first initialized `ndsMatchConfigApply` producer; modify four aligned fighter words to Mario/Kirby/Fox/Yoshi CPUs (level 3/handicap 9), items off. Read this diagnostic descriptor through the DS main-RAM alias at +4 MiB (`r0` only), preventing an already-cached descriptor from hiding debugger writes; the real producer writes the battle state normally. Preserve startup and redirect its first scene request to VSBattle, as the working run 7. Prefix `builds/codex-impact-wave-green-r55-fourcpu`. Check visible roster before attributing results. Same ROM/capture anchors, default NDL arm, no build.

Run 8 outcome: **incomplete descriptor experiment**. The producer breakpoint was eventually reached: `CFG_SEED addr=0x02268a38 word0=0x09030100 kinds=0,8,1,6 ndl=1`. The main-RAM-alias pointer operation then failed to reach the startup handoff; no wave/image or complete result. Stopped only its owned GDB/emulator. Do not use this alias recipe or classify its incomplete state as a colour regression. Evidence `r55-fourcpu-incomplete-gdb.txt`.

Run 9 launch outcome: an overlapping owned scratch-slot launch was detected and cancelled before accepting any output. This was an investigator orchestration error, not a game result. The previous wait had reached its descriptor breakpoint without reaching its handoff. The launcher now also checks for an existing process at the exact owned scratch executable path, because the TCP-listener preflight did not prevent the overlap. Both incomplete processes are stopped; no other runner was touched.

### Run 10 plan (serialized r54 baseline retry)

Same baseline identity and run-9 intended recipe; prefix `builds/codex-impact-wave-green-r54-baseline`. Previous attempt supplies no valid evidence. Owned scratch emulator/GDB have exited. Preserve normal startup; select VSBattle at its first scene request; capture the actual shipping preset and source wave. No aliases, no builds, no NDL symbol access, no other slots.

Run 10 outcome: **r54 draw-state coverage; capture recipe invalidated by a scratch template/output collision**. The prior attempt used a template named like its generated `.gdb` output; that file had substituted the old emulator PID/capture stem. No valid r54 image can be attached to run 10. The baseline maker/draw/palette/register data remain tied to the matching r54 ELF; preserve them as colour-state evidence only (`r54-baseline-gdb.txt`). Fixed the naming and added a launcher collision guard.

### Run 11 plan (r54, missing image)

Fresh template `builds/codex-impact-wave-green-r54-template.gdb` retains placeholders and cannot collide with prefix `builds/codex-impact-wave-green-r54-image`. Exact same retained r54 ROM/ELF, startup/source-handoff recipe and first-wave frame anchors. Missing coverage is the unobscured baseline image; previous valid binary measurements are reused. This is the last r54 retry, followed by at most one bounded hypothesis run and report finalization.

Run 11 outcome: **r54 green wave captured**. Three valid images (`r54-image-wave1/2/3.png`) at frames 342/344/346 show green/yellow and white, with source maker/native draw first frame 340, index/actual variant 4, six draws and no observed fallback/null. This is the same Mario/Fox preset as run 7. Newly written stack position and cached libnds fields read stale on r54 (maker position printed 0/1); ignore those readings. Variant register and outgoing COLOR register are authoritative. The first-image visibility differs across the two builds; these captures stop at function entry before end-frame presentation, so that alone is not proof of a colour regression. Peak images (344) show green in both.

### Run 12 plan (final same-ROM NDL control)

Exact r55 ROM/ELF from run 7; same startup/source-handoff recipe and Mario/Fox preset. Set `gNdsP2Ndl=0` after runtime initialization, observe actual source display-proc engagement and native CPU-projected draw. Prefix `builds/codex-impact-wave-green-r55-ndl-off`, separate template `builds/codex-impact-wave-green-ndl-off-template.gdb`. Capture first-wave +2/+4/+6, preserve colour/palette data. This is the final emulator run; timeout 600 s and investigation ends by 16:31 UTC. Missing coverage is the same-ROM draw-route colour comparison; healthy early-event r54/r55 evidence cannot exclude intermittent later states.

Run 12 outcome: **NDL-off control is engaged and green**. `ARM ndl=0`; six `efManagerImpactWaveProcDisplay` observations; first maker 340, six native draws, zero observed fallback/null; exact variant-4 VRAM palette. Images `r55-ndl-off-wave1/2/3.png` at 342/344/346 show the same first-event visibility and green peak as NDL on. The NDL-only colour-store breakpoint was unengaged in this arm, as expected; do not call that zero a colour proof. Evidence `r55-ndl-off-gdb.txt`, palette, images. Final writer exited with DONE/exit 0; no further emulator runs.

### Run 9 plan (retained r54 baseline)

Reuse the working run-7 startup/handoff recipe, not the unengaged descriptor breakpoint. `builds/remaining-bugs-playtest-r54/smash64ds.nds` and matching ELF; no NDL word exists here. Expect the same shipping Mario/Fox preset to replace the default descriptor seed; observe the actual roster and first natural source wave, palette and COLOR register. Resolve instruction offsets from r54 (variant +152, bind +3832, colour +4324). Prefix `builds/codex-impact-wave-green-r54-startup`. Missing coverage is r54 colour behaviour, required for regression classification; no valid baseline run has yet been completed.

## Source call sites and colour path

Source `decomp/BattleShip-main/decomp/src/ef/efmanager.c:39-54` has five primitive-colour choices: red (0), green (1), blue (2), yellow (3), white (4); all environment colours are black. The display callback at `:3281-3298` reads the stored index and alpha before drawing the animated DObj.

Port hard-landing dispatch `src/port/reloc_backend_compat_shims.c:8322-8331` calls the maker with index 4. This alone is not evidence of a missing-green bug: index 4 is white by source contract. Other collision callers are still being traced.

Clarification: a white primitive does **not** make this texture white. The white variant's baked source TLUT starts with RGB555 `0x02c7` (R=7, G=22, B=0) and ends with `0x7bde` (30,30,30), so its outer texture region is green/yellow and its inner region white. Missing that green component can still be a real renderer bug with index 4.

All source collision makers found so far select index 4: `SYVECTOR_AXIS_Z` is `(1 << nSYVectorAxisZ)` with ordinal 2 (`decomp/BattleShip-main/decomp/src/sys/vector.h:7-21`), hence 4. Direct calls are `ftcommonwalldamage.c:32`, `ftcommoncapturekirby.c:434`, and `ftnessspecialhi.c:839`. The generic effect dispatcher at `ftparam.c:1966-1971` also selects 4 (floor-aligned angle when grounded; air maker otherwise). Its triggers include `ftcommondamage.c:543` and `ftcommondownwaitbounce.c:101`.

The port's strong `efManagerImpactWaveMakeEffect` (`src/import/battleship_efmanager.c:2808-2829`) forwards the supplied index/rotation to the renamed imported source maker (`:159`, `:210` include). It overrides the old weak substitute. `ndsEFManagerImpactWaveVariant` (`:363-378`) validates and reads that same EFStruct index; `ndsEFManagerImpactWaveNdlHeader` (`:381-406`) uses the source colour arrays and live alpha. No constant-green source call has been found.

Current port has a resident native ImpactWave path: five texture names/palettes; `src/nds/nds_renderer_textures_effects.c:3754-3778` prepares each variant at scene entry. The queried code is preserved in `codegraph-impact-path.txt` and `codegraph-native-wave.txt`. Source code comments describing an older substitute are not being treated as runtime proof.

### Complete inspected caller mapping

| Caller | Source selection | Port selection |
|---|---|---|
| `ftParamMakeEffect(nEFKindImpactWave)` | `decomp/BattleShip-main/decomp/src/ft/ftparam.c:1966-1971`: index 4; grounded floor-normal rotation, otherwise air maker / rotation 0 | `src/port/reloc_backend_compat_shims.c:8322-8331`: index 4, rotation 0. The constant colour is source-correct; slope rotation differs and is outside this confirmed colour evidence. |
| Damage landing / down bounce | `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondamage.c:543` and `ftcommondownwaitbounce.c:101`: request the dispatcher above | Same dispatcher / strong maker. |
| Wall damage | `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonwalldamage.c:32`: `SYVECTOR_AXIS_Z` = 4, `atan2(-normal.x,normal.y)` | Imported verbatim at `src/import/battleship_ftcommon_walldamage.c:40`; maker forwards index/rotation. |
| Kirby spit-star collision | `decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturekirby.c:434`: index 4, collision-normal rotation | Imported at `src/import/battleship_ftcommon_capturekirby.c:74`; maker forwards. |
| Ness PK Thunder body collision | `decomp/BattleShip-main/decomp/src/ft/ftchar/ftness/ftnessspecialhi.c:839`: index 4, collision-normal rotation | Imported at `src/import/battleship_ness.c:205`; maker declaration `:120` and strong maker preserve arguments. |
| Air wrapper | `decomp/BattleShip-main/decomp/src/ef/efmanager.c:3354-3356`: forwards caller index, rotation 0 | Compiled source wrapper; its internal renamed-maker call can bypass the public instrumentation counter, so maker counters alone are not a complete census. |

Draw contract: `efmanager.c:3281-3298` emits primitive RGBA from the five colour arrays plus live EF alpha, black environment RGBA, then draws the DObj. `:3302-3325` calls `gcPlayAnimAll`, retires at the source animation end, and decays alpha by the maker's `127/11`. The descriptor carries both joint and material animation (`:201-227`). Native NDL uses the same live index/alpha via `src/import/battleship_efmanager.c:381-406` and the MObj material via `src/port/renderer_adapter_stage.c:2978-3006`.

Native pixels are source CI4 (16×32), not a flat procedural substitute: source texels/TLUT are baked at `src/nds/nds_renderer_preamble.c:3079-3152`; the five palettes incorporate `(PRIM-ENV)*TEXEL0+ENV`. `ndsRendererHardwarePrepareIFCommonPal16Atlas` uses PAL16 plus colour-zero transparency (`src/nds/nds_renderer_textures_effects.c:3632-3644`); each variant has a separate resident GL texture name. `src/nds/nds_renderer_native_common.c:1042-1075` binds that variant and clears the generic cache active entry. Alpha/UV/material are still read live (`:1255-1363`). Outgoing vertex RGB is white in measured active native draws, so green comes from the palette. No runtime colour mutation is needed for the band.

## Root cause

**Not isolated.** No missing-green event was captured. Healthy events establish the correct caller index, colour-bearing texels/palette, actual variant, and native output; they do not establish later-match or all-roster correctness.

Measured exclusions are scoped: the captured events had five resident variant textures, a correct variant-4 VRAM palette and native draws, with no observed fallback/maker-null. Their incoming active texture differed from the wave name, so a necessary rebind was issued. This does not exclude another event's allocation failure, cache/lifetime problem, caps, mat-animation/UV problem or state inheritance.

The strongest static candidate is **libnds state left stale by direct FIFO producers**:

1. `src/nds/nds_stage_gx.exec.inc:350-369` sends texture/palette words directly, then resets `sNdsRendererHardwareBoundTextureName` and `sNdsRendererHardwareActiveTextureEntry` only. This producer arrived after r54 in commit `3eefca0fd42d` (2026-09-24).
2. Lean fighter packet DMA has the same invalidation gap at `src/nds/nds_renderer_native_common.c:14853-14861`.
3. The ordinary bind (`src/nds/nds_renderer_textures_effects.c:5900-5905`, `src/nds/nds_renderer_preamble.c:3228-3239`) delegates to libnds. r55 ELF `glBindTexture` at `0x0211f804`, offsets +6..+10, compares software `activeTexture` to the requested name and branches straight to its return at +44 on equality. That path writes neither TEXIMAGE nor PLTT.
4. If an earlier wave remains libnds's active name while a direct packet has selected different hardware texture/palette words, the next wave can sample that packet's state despite requesting its correct resident name. Another intervening ordinary bind prevents it, explaining why it would be conditional.

This chain identifies a real bookkeeping hazard, but **there is no observed bad-wave/same-name-elision pair and no repair A/B**. Do not state `3eefca0fd42d`, M1, clip I/O or event32 holes as the proven owner-bug regression. The same-ROM NDL comparison did not change colour in the sampled event.

## Minimal proposed fix

Conditional candidate: invalidate libnds's software texture/palette identity beside the existing producer invalidations, after submitting direct FIFO words. This changes no palette, geometry, colour index or content. It forces the next ordinary bind to write the correct hardware words. **Not applied; not validated; apply only as a measured candidate, not a declared Bug D fix.** The integrator should also check any other still-linked direct FIFO producer for the same contract.

```diff
diff --git a/src/nds/nds_stage_gx.exec.inc b/src/nds/nds_stage_gx.exec.inc
--- a/src/nds/nds_stage_gx.exec.inc
+++ b/src/nds/nds_stage_gx.exec.inc
@@ -363,5 +363,8 @@ static void ndsStageGxFlush(void)
     gNdsP2StageProgDmas++;
     ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
+    /* The FIFO packet also bypassed libnds's active-object state. */
+    glGlobalData.activeTexture = 0;
+    glGlobalData.activePalette = 0;
     sNdsRendererHardwareBoundTextureName = 0u;
     sNdsRendererHardwareActiveTextureEntry = NULL;
     sNdsR2GxLastProjection = NULL;
diff --git a/src/nds/nds_renderer_native_common.c b/src/nds/nds_renderer_native_common.c
--- a/src/nds/nds_renderer_native_common.c
+++ b/src/nds/nds_renderer_native_common.c
@@ -14856,6 +14856,9 @@
     sNdsFighterPacketDmaPending = 1u;
 
     ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
+    /* The FIFO packet also bypassed libnds's active-object state. */
+    glGlobalData.activeTexture = 0;
+    glGlobalData.activePalette = 0;
     sNdsRendererHardwareBoundTextureName = 0u;
     sNdsRendererHardwareActiveTextureEntry = NULL;
     sNdsR2GxLastProjection = NULL;
```

## Integrator verification

1. Reproduce the owner's intermittent event on the **published r55 four-player roster**, through normal startup/VS, and include hard ground and wall bounces. The four-CPU lab can use initialized words `gNdsLabFourCpuKinds=0x06010800`, `gNdsLabFourCpuGkind=6` at `ndsMatchConfigLoadMarioFoxDreamLand` entry. Use the assigned accurate interpreter slot and matched ELF. Preserve normal startup; the direct pre-startup VSBattle seed failed at ground setup and is not a working recipe. Do not reuse the failed descriptor alias experiment.
2. Use loaded registers for variant (`native submit +164` r55 / +152 r54), maker index, and outgoing colour. For position use registers **after the source maker's LDMIA**, not newly written stack. R54 libnds global readbacks can be stale. At `glBindTexture+10`, sample requested name in `r5` and actual compared active name in `r3`; at +28/+72 inspect actual outgoing texture/palette store registers. Save the preceding packet's last texture/palette words to distinguish software identity from hardware state.
3. When green is absent, collect matching RGB555 palette bytes (temporarily map the relevant palette bank to LCD with an aligned whole-word VRAMCNT poke, restore before resuming), actual texture format/PLTT, vertex RGB/alpha, UV origin/scale, and animation age. Compare with the saved variant-4 bytes. Capture **after the relevant end-frame flush/present**, not merely at `ndsPlatformEndFrame` entry, to avoid an older front buffer. Current early/peak captures prove healthy output, not exact displayed-phase equality.
4. If the same-name-elision condition is observed with a bad wave, A/B the candidate producer invalidation in the same ROM/run recipe. Prove an actual TEXIMAGE/PLTT rebind and restored source green with unchanged gameplay/animation state. If it is not observed, leave this candidate unaccepted and follow the measured bad state instead. Do not change the source's index 4 to 1 or introduce a palette override.
5. Qualify the final hard-on candidate under the task's required battle/shared profile, sibling effects and roster/stage/lifecycle coverage, native-only/resource checks and isolated timing (ticks, FPS, P50/P95). Performance and owner acceptance are owed; debugger screenshots and zero counters do not close the bug.

Working retained-ROM short diagnostic commands are the launcher/template pairs for `r55-startup`, `r54-image`, and `r55-ndl-off` above. Templates must remain distinct from generated `.gdb` output, and one owned emulator/GDB may occupy port 3330 at a time. Source-proc engagement (six calls) qualifies the NDL-off control. All reported healthy images and binary measurements are retained under this evidence directory.

## Execution notes

- 14:01 UTC: read repository rules and confirmed `.codegraph/` exists. Read-only git inventory initiated. CodeGraph will be tried before source searches/reads.
- Initial helper failures: the default `rg.exe` Windows link cannot execute; use its resolved package executable. The devkitPro/MSYS git status command stalled; the installed Windows Git works. No tracked edits or builds performed.
- ROM/ELF SHA-256: r55 ROM `AB60DFA8ABF50E04BECFBF4FCA2F08FC6A14B2B6064C0C9F15DEFEB7C9162CE8`, ELF `5E06F75C5FDF5060137AEFD47851FE782BCD3B09517B997545C2D9B070AEEB57`; r54 ROM `C8FC02AA2DF0BB6E84C2E0AF4EF2BF6F8C731A95EA1E7D8F129BABFC8B863121`, ELF `F5BBD5F8077CD62FAAADB62B2E0A330FBCB9F1405AFC84926BE5EE5B98170BDB`.
- Runtime preparation: slot 3 already has GDB port 3330 and `[JIT] Enable=false`. An asynchronous question offered scratch relocation versus automatic writes in slot 3. With no answer after more than ten minutes, use the conservative scratch relocation of this same binary/port; no second slot/port is allocated. `--help` exited without launching a ROM; output was empty. Repo source and slot 3 executable SHA-256 both `DE80E46BDCF1FD986162DE6AFFD9EE1148F8C40565DBAF667B9C2B3EF5475715`.
- Static comparison: the five baked palettes are present with identical values in r54 source (`2a144b426d0`) and r55. M1 changes the submission route; it does not change the palette values. `native-common-r54-r55.diff` preserves the relevant source diff.
