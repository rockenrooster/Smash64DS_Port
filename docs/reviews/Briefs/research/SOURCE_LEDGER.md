# Source ledger and research boundaries

Inspected repository snapshot: `master` at
`a5c5bc08d8e8661658865216798d600462db948e`.

**Direct code** means the specified source text/excerpt was read. It does not
prove that a particular ROM compiled or exercised it. **Repository record** means
an existing report's own claims and scope, not a test rerun here. Original packed
ROM assets, ignored generated data, the owner's uncommitted overlay, target
compiler and emulator were unavailable in this working environment.

The analysis corrected several stale theories rather than accepting every old
BUG_NOTES paragraph. Supplied ring-asset command decoding remains owner-supplied;
source callback state and the native depth path were independently inspected.
The source-code patches were constructed from inspected preimages; complete
checkout application is a separate check supplied in `tools/check_candidates.py`.

## S01

**Repository operating rules**

`AGENTS.md`

Directly read. Native-only builds, read-only reference trees, shared-build serialization, verification and hygiene.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/AGENTS.md)

## S02

**Current execution board**

`docs/P2_EXECUTION_BOARD.md`

Directly read lines 1-125. Status reported by repository authors as of September 14; no runtime claims independently rerun here.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/P2_EXECUTION_BOARD.md)

## S03

**Bug-fixing and closure policy**

`docs/BUG_FIXING_PROCESS.md`

Directly read. Observable source contract, first divergence, natural-path proof, closure and annotations.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/BUG_FIXING_PROCESS.md)

## S04

**Build and verification procedure**

`docs/VERIFYING.md`

Directly read lines 1-170. Target identities, current profile coverage and timing/evidence limitations.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/VERIFYING.md)

## S05

**Native impact wave implementation**

`src/nds/nds_renderer_native_common.c`

Directly read lines 1030-1375. Transformed X/Y reused; painter-depth counter used per triangle; non-Z comment.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_native_common.c)

## S06

**Stage/effect initial geometry state**

`src/port/reloc_backend_movement.c`

Directly read lines 11800-11885. Current actor-preserves-Z exception and effect admission context; downstream paths are follow-up search targets.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/port/reloc_backend_movement.c)

## S07

**Original impact wave display and update**

`decomp/BattleShip-main/decomp/src/ef/efmanager.c`

Directly read lines 3270-3328. Display selects G_RM_AA_ZB_XLU_SURF and sets dynamic primitive alpha. Asset/camera decode in the supplied diagnosis was not independently repeated.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/ef/efmanager.c)

## S08

**Native DATA owner**

`src/nds/nds_menu_shell_data.c`

Search excerpts inspected. Current board provides the retired-slab diagnosis; trace actual route in the working revision.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_menu_shell_data.c)

## S09

**Existing owner bug list**

`docs/BUGS.md`

Directly read. Older report list differs from the latest user request; no extra old defects silently added. Falcon Punch/Kick are labeled repository-only leads.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/BUGS.md)

## S10

**CSS residency repair and proof history**

`artifacts/visibility/2026-09-13_css-residency-loop.md`

Directly read lines 1-290. Raw 0x152 pin, owner image/arena and particle re-entry repairs, three-lap proof and residuals. Historical proof is not current symptom closure.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/artifacts/visibility/2026-09-13_css-residency-loop.md)

## S11

**Samus morph source/owner tests**

`scripts/fighters/test_native_samus_morph.py`

Directly read. MorphUnfold/MorphBall source roots and program IDs; roll/Bomb/restore source assertions. Test presence is not natural runtime proof.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/fighters/test_native_samus_morph.py)

## S12

**Stage setup repair and bounded proof**

`artifacts/visibility/2026-09-14_stage-hazard-guards.md`

Directly read. Hyrule/Inishie setup guards, natural entry and native output. Does not prove Peach's Castle roof texturing or all other stage visuals.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/artifacts/visibility/2026-09-14_stage-hazard-guards.md)

## S13

**Saffron gate source and native anchors**

`scripts/stages/native_stage_descriptors/yamabuki.py`

Search excerpts inspected together with src/import/battleship_gryamabuki_ground.c and the original gryamabuki.c. Open/close animation assets and source state/proximity/timer routing.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/stages/native_stage_descriptors/yamabuki.py)

## S16

**Product and performance contract**

`PROJECT_GOAL.md`

Directly read. Native-only scope, 30 FPS target, 30 Hz menus, acceptance and custom accuracy-focused melonDS reference.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/PROJECT_GOAL.md)

## S17

**Native entry/effect generator**

`scripts/3d_vfx/generate_nds_entry_effects.py`

Search excerpts inspected. Confirmed producer path; per-effect details must be traced on the actual working tree.

[Inspected snapshot](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/3d_vfx/generate_nds_entry_effects.py)

## S14

**DS texture formats**

Martin Korth / GBATEK, DS 3D Texture Formats. Read the A3I5/A5I3, direct-color and palette-format descriptions; these are hardware representation constraints, not diagnoses of any specific port asset.

[GBATEK texture formats](https://problemkaputt.de/gbatek-ds-3d-texture-formats.htm)

## S15

**DS texture/vertex blending**

Martin Korth / GBATEK, DS 3D Texture Blending. Read per-polygon versus per-vertex alpha and modulation/decal behavior before choosing a gradient representation.

[GBATEK texture blending](https://problemkaputt.de/gbatek-ds-3d-texture-blending.htm)

## R18

`scripts/fighters/test_native_samus_morph.py` — 1–90.

Direct source tests, not a rerun: program IDs, root offsets, model-part helper and roll/Bomb/restoration expectations.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/fighters/test_native_samus_morph.py)

## R39

`decomp/BattleShip-main/decomp/src/sys/rdp.c` — 1–75.

Direct source graphics initialization; inherited geometry state includes G_ZBUFFER.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/sys/rdp.c)

## R41

`src/import/battleship_mnplayersvs.c` — 300–640.

Direct resident ownership/preparation excerpt; current definitions supersede older residency descriptions.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/import/battleship_mnplayersvs.c)

## R42

`src/import/battleship_mnplayersvs.c` — 640–1000.

Direct resident lifetime and acquire support excerpt.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/import/battleship_mnplayersvs.c)

## R48

`src/import/battleship_mnplayersvs.c` — 1000–1360.

Direct acquisition excerpt. Existing action budgeting and blocking compact acquisition must be considered separately.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/import/battleship_mnplayersvs.c)

## R52

`decomp/BattleShip-main/decomp/src/relocData/247_YoshiMain.c` — 1–235.

Direct source model-part / DL-pair declarations. Referenced generated vertex/texture arrays were not independently decoded.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/relocData/247_YoshiMain.c)

## R60

`scripts/stages/native_stage_descriptors/zebes.py` — descriptor.

Direct descriptor/source-contract inspection: acid root and already-present subdivision policy.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/stages/native_stage_descriptors/zebes.py)

## R61

`scripts/3d_vfx/generate_nds_entry_effects.py` — 540–900.

Direct effect texture conversion excerpts: current alpha formats and IA8 handling. Do not infer missing conversion solely from old reports.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/3d_vfx/generate_nds_entry_effects.py)

## R64

`src/nds/nds_renderer_native_common.c` — 4780–5440.

Direct native effect lookup/group ownership excerpts, including already-present Captain/other effect routes.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_native_common.c)

## R65

`src/nds/nds_renderer_native_common.c` — 5520–5820.

Direct entry-effect submit excerpt: inherited geometry, group clear/set state, native material/texture and alpha handling.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_native_common.c)

## R66

`decomp/BattleShip-main/decomp/src/relocData/353_LinkSpecial2.c` — 1–220.

Direct source effect material declarations: entry CI4 wave/column versus the Spin material; packed art not regenerated.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/relocData/353_LinkSpecial2.c)

## R71

`src/nds/nds_renderer_assets.c` — 5600–6035.

Direct native root-resolution and variant-selection excerpts. Distinguish table presence from matching live program/binding coverage.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_assets.c)

## R72

`docs/HANDOFF.md` — file.

Repository handoff record: active 1P/roster/performance work and preserved scoped proofs. Not independently rerun.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/docs/HANDOFF.md)

## R73

`src/import/battleship_gryamabuki_ground.c` — file.

Direct wrapper inspection: original gate state machine imported, with native/accessor integration; historical comments may be stale.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/import/battleship_gryamabuki_ground.c)

## R74

`scripts/stages/native_stage_descriptors/yamabuki.py` — descriptor / related search excerpts.

Native gate source anchors and baked/live distinction; do not conclude frozen animation from a static bake alone.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/stages/native_stage_descriptors/yamabuki.py)

## R75

`src/port/reloc_backend_movement.c` — 12270–12540.

Direct item/weapon route inspection: item Z restore and weapon-specific initial state. Do not broaden R02 into all routes.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/port/reloc_backend_movement.c)

## R76

`src/nds/nds_renderer_textures_effects.c` — 12955–13055.

Direct projected source-depth emitter versus already-NDC helper; R01 follows the existing SourceDepthToV16 mapping.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_textures_effects.c)

## R78

`src/port/reloc_backend_movement.c` — 12645–12712; rechecked 12660–12722.

Direct effect call site and profile/non-profile variants; preimages of R02 rechecked.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/port/reloc_backend_movement.c)

## R83

`src/nds/nds_renderer_assets.c` — 3830–4110.

Direct native image/copy-hat and Kirby-enabled routing excerpts. Link foreign roots already handled in this branch.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_assets.c)

## R84

`src/nds/nds_renderer_assets.c` — 4100–4350.

Direct disabled-Kirby helper definitions: absence of parallel Link foreign-table/preamble routing is R04 target.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_assets.c)

## R88

`scripts/stages/native_stage_descriptors/zebes.py` — descriptor; associated generator source.

Direct acid-subdivision setting and source references; E01 does not assume exact native output counts after integration.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/stages/native_stage_descriptors/zebes.py)

## R90

`scripts/stages/generate_nds_native_stage.py` — 3600–3860; associated run-emission excerpts.

Direct native stage producer inspection. Actual affected roof/floor texture roots still require decoded source fixtures.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/stages/generate_nds_native_stage.py)

## R91

`scripts/stages/generate_nds_native_stage.py` — 2870–3150.

Direct vertex-load, midpoint subdivision, source-alpha averaging, run splitting and source-Z classification code.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/stages/generate_nds_native_stage.py)

## R92

`scripts/menus/generate_mn_ui_kit.py` — 4100–4350.

Direct main generator flow and appended-surface order; existing surface IDs must remain stable.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/menus/generate_mn_ui_kit.py)

## R93

`scripts/menus/audit_mn_screen_coverage.py` — surface table inventory search excerpt.

Direct excerpt of appended-surface table enumeration; R05 adds DATA to this existing inventory.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/menus/audit_mn_screen_coverage.py)

## R97

`decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c` — 70–315.

Direct DATA unlock-dependent row positions, lrs16 tab geometry, source color pairs and labels.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c)

## R98

`decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c` — 309–450.

Direct DATA Sound Test label, background tint, logo and DATA header positions.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c)

## R99

`decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c` — 438–508.

Direct DATA collage/papers/icon source placements and colors.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/mn/mndata/mndata.c)

## R100

`src/nds/nds_menu_shell_data.c` — file.

Complete DATA fragment inspected: retired text consumers, existing unlock/routing behavior and current redraw function.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_menu_shell_data.c)

## R102

`src/import/battleship_mnplayersvs.c` — 1090–1220.

Direct current acquire/reset preimages for R03; BGM suspension brackets and existing failure cleanup.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/import/battleship_mnplayersvs.c)

## R103

`scripts/menus/generate_mn_ui_kit.py` — 3450–3500.

Direct Options tab/source-surface helper and its bounding box. R05 keeps the same native surface scheme with DATA-specific dimensions.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/menus/generate_mn_ui_kit.py)

## R104

`src/nds/nds_menu_shell_option.c` — 1–220.

Direct established native blit/cache/control pattern used as R05 reference.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_menu_shell_option.c)

## R105

`src/nds/nds_renderer_textures_effects.c` — 12830–12945.

Direct painter counter and EnterProjectedForeground implementation. Transition only changes no-Z ordering bookkeeping.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_textures_effects.c)

## R106

`decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c` — 130–290.

Direct gate update/monster-clear/SetClosedWait/animation-process source. Closing is linked to monster retirement, not an invented fixed oscillator.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c)

## R107

`src/nds/nds_audio_bgm.c` — SuspendForBlockingLoad search excerpt; related header/callers.

Direct function/comment excerpt says resume continues rather than restarting. An actual perceived reset needs its own start/seek evidence.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_audio_bgm.c)

## R108

`src/nds/nds_renderer_assets.c` — 1–165.

Direct traversal/state/image-ABI declarations; not a full generated-image compile proof.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/src/nds/nds_renderer_assets.c)

## HW01

**devkitPro libnds `videoGL.h`**, current official repository documentation read.
`POLY_ALPHA` has range 0–31 and zero selects wireframe; `POLY_ID` has range 0–63.
These are hardware API constraints, not evidence for a particular game bug.

[Official source](https://github.com/devkitPro/libnds/blob/master/include/nds/arm9/videoGL.h)

## HW02

**BlocksDS libnds videoGL reference**, published documentation read. Blend and
alpha-test controls are distinct; edge antialiasing does not antialias translucent
pixels or all interior/intersection edges. No claim that enabling antialiasing
alone fixes missing coverage.

[Library reference](https://blocksds.skylyrac.net/libnds/videoGL_8h.html)

## Deliberate limits

A GitHub source read cannot identify the user's unpublished build flags, effective
asset bytes or current dirty overlay. The ignored O2R/ROM-derived inputs were not
present locally. No new original-game screenshots, audio capture, melonDS run,
ARM target build, full image bake or whole-repository apply was performed. The
entire two DS reference trees were not audited; inspect their relevant renderer
and asset owners before substantial E01/streaming architecture integration.

Failed source-directory/large-file discovery attempts are not counted as inspected
code. A comment about a generic fallback is not permission to compile one; current
project policy requires native-only output and unsupported content remains open.

## R114

`scripts/menus/generate_mn_ui_kit.py`, lines 780–960. Direct placement API and
source-sprite-to-surface conversion contract; source color/coverage differs from
one-bit OBJ cells.

[Inspected revision](https://github.com/rockenrooster/Smash64DS_Port/blob/a5c5bc08d8e8661658865216798d600462db948e/scripts/menus/generate_mn_ui_kit.py)

## R115

Same producer, lines 956–1048. Direct placement field definitions, full-bleed
background policy and source primitive/environment color convention.

## R116

Same producer, lines 1040–1145. Direct `fill`, `size`, `env` fields and
`place_raster` handling, checked against R05's added placements.

## R117

Same producer, lines 3380–3455. Direct Options background and tab constants.
Confirms the single Placement COLLAGE_FULL_BLEED pattern reused by R05.
