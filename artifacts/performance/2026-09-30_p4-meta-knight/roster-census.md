# Meta Knight roster census — 2026-09-30

Status: source census only; no runtime, build, resource or performance pass.
Scope: isolated `codex/meta-knight` worktree; main checkout and `decomp/` unchanged.
All line references below were checked in this worktree on this date. CodeGraph
was consulted before code search/read in the original checkout.

## Preserve identity boundaries

`include/ft/fighter.h:73` mirrors the original FTKind enum: playable kinds 0–11,
Boss 12, MMario 13, polygons 14–25, GDonkey 26, EnumCount 27, Null 28. Keep those
values, the playable range (`:87`) and both sentinels (`:105`, `:106`) unchanged.
Inserting Meta Knight before Boss would relabel source tables and save identities.

Smallest safe extension: a generated native registry with separate stable content
ID, extended runtime kind, native owner slot, CSS position and legacy record ID
(absent for Meta Knight). Reserve an extended kind beyond 28; treat membership
explicitly rather than increasing the legacy playable endpoint. Registry rows
must also own source profile, six-costume/team mapping, preview/result actions,
announcer/UI keys, native callback/CPU/copy contracts and resource fragment IDs.
An incomplete dev row stays opt-in and cannot be described as accepted content.

## Verified consumers requiring deliberate routes

| Seam | Verified source | Required change |
|---|---|---|
| FTData lookup | `src/import/battleship_ftdata.c:9`; `include/ft/fighter.h:4162` | Source TU includes the legacy 27-entry FTData array. Add native generated data and a lookup route; do not edit BattleShip. |
| Status lookup | `decomp/BattleShip-main/decomp/src/ft/ftmain.c:4556`, `:4566` | Opening and special status tables index raw fkind. Intercept extended kind in `src/import/battleship_ftmain.c` and use its resolved native descriptors. |
| Damage script domain | `include/ft/fighter.h:2817` | Legacy `p_script[2][27]` cannot receive extended kind. Resolve intentional victim/copy mapping or native extended table. |
| Manager/owner | `src/import/battleship_ftmanager.c:189`, `:541`, `:701` | Preserve legacy source-size census; supply new native size/resource metadata and owner-image mapping separately. Audit entry/passive init and teardown. |
| CPU | `decomp/BattleShip-main/decomp/src/ft/ftcomputer.c:3991` | Raw attack-list indexing needs an extended route in `src/import/battleship_ftcomputer.c`; use donor CPU semantics. |
| Match admission | `src/port/nds_match_config.c:55`, `:160` | Proof selector only permits kinds <=11; four-CPU membership lists legacy kinds. Use registry membership for new-kind proof and normal descriptors. |
| Preview membership | `src/import/battleship_mnplayersvs.c:360`, `:362`, `:380`, `:486`, `:770`, `:1013`, `:1051`, `:1380`, `:1596`, `:1655`, `:2245`, `:2428` | Replace new-kind-facing range assumptions with registry lookup, preserving original campaign range. Telemetry must map a selection index separately from runtime kind. |
| Preview owner | `src/import/battleship_mnplayersvs.c:398` | Map registry owner slot and scene-specific native preview resources through the existing resumable/cancelable loader. |
| Preview telemetry | `include/nds/nds_menu_shell.h:15`, `:357` | Twelve-entry arrays currently assume kind equals selection index. Extend admitted selection capacity with explicit mapping. |
| CSS layout/picking | `src/nds/nds_menu_shell_css.c:73`, `:149`, `:157`, `:413`, `:449` | Twelve portraits, six columns and two rows are hardcoded. Registry position and geometry must agree across drawing, token centering, hit tests and tours. |
| CSS identity/audio/random | `src/nds/nds_menu_shell_css.c:142`, `:329`, `:374`, `:1857` | Direct kind-indexed voice arrays, kind<12 guards and random range exclude a sparse new kind. Route each through admitted registry rows. |
| HUD | `src/nds/nds_battle_hud.c:356`, `:429`, `:603`, `:675` | Unknown kinds fall to Mario stock/name routing. Add Meta Knight stock palettes, name and emblem metadata explicitly. |
| Save/records | `include/sc/scene.h:926`, `:931`, `:933` | Twelve legacy VS records, saved character kind, u16 unlock mask. Never index/store extended kind under unchanged legacy byte meanings; use versioned extension only when that tier is implemented. |

Further mandatory interaction census includes victim offsets (Captain capture,
throws, egg/inhale), source costume selectors, jump-kind branches, Kirby copy
mapping, result naming/demo poses and Training selection. This report does not
claim those domains are exhausted.

## Existing producer contracts

| Producer | Current constraint / adapter seam |
|---|---|
| `scripts/fighters/generate_fighter_production_manifest.py:47`, `:622` | Twelve bootstrap fighters; source FTData census must have 27 rows. Consume resolved EXTRA metadata alongside the legacy provider instead of expanding that assertion. |
| `scripts/fighters/generate_nds_fighter_admission.py` | Requires typed object/file closure metadata and pointer semantics; loose donor binaries are insufficient. |
| `scripts/fighters/generate_nds_native_owners.py:311`, `:1714`, `:3463` | Named, hashed O2R owner resources with geometry/root/binding discovery. Adapter must provide validated payload, semantic roots, materials and relocations to this owner compiler. |
| `scripts/fighters/generate_nds_native_owner_images.py` | Reuse native owner image ABI/output and scene loader rather than a parallel Meta Knight runtime asset loader. |
| `scripts/menus/generate_mn_ui_kit.py:2407`, `:2408` | Explicit twelve-kind built roster. Add native donor portrait/name/emblem input and registry layout through this producer. |
| `scripts/menus/generate_battle_hud.py` | Owns stock/name assets; add resolved source stock palettes/art through producer. |
| `scripts/fighters/admit_fighter.py:1105` | P2 BattleShip-specific source/header/asset patcher. Cannot admit EXTRA by adding a name; it expects BattleShip FTData/motion/status/typed sources. |

## Implementation order

1. Freeze donor revision, profile, supported source ROM/hash and staging tools;
   resolve EXTRA request lists, append/merge operations, symbols and sound IDs.
2. Emit typed model/animation/material/script/callback inventory. Reject unknown
   objects and donor function pointers; unresolved dependencies stay explicit.
3. Feed native geometry/owner, animation, residency, UI/HUD and audio producers;
   derive allocation/loading peaks rather than treating file size as RAM size.
4. Add opt-in registry entry and native runtime wrapper routes, then CSS/preview,
   normal match descriptor and lifecycle integration. Preserve legacy IDs/saves.
5. Qualify source-equivalent normal/special/jump/CPU/copy/item behavior, native
   visuals and natural-input CSS-to-match/results/rematch with original regressions.
   Compilation or a selectable entry alone does not close the playable request.

Current dependency: P4 source adapter is proposed, not implemented. Meta Knight's
zero standalone special-file slots do not eliminate special animation/effect
resources. Its config selects Jigglypuff inheritance but supplies own jumps,
wing/sword/cloaking rules, CPU and specials; inheriting Purin wholesale is invalid.
