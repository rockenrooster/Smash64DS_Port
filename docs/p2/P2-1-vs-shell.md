# P2-1 — Shell, Preview Residency and Natural Loops

Finish the existing native shell without rebuilding its accepted screens. This phase owns VS preview lifetime and reusable preview services; 1P owns its source menu semantics and consumes those services. Current implementation state and owner rejections remain on the board/bug queue.

## Preserve and reuse

The match descriptor, scene manager, native UI kit, title/VS/CSS/SSS surfaces and battle→Results→CSS route already have implementation and historical evidence. Preserve the original title and branding, correct hand/token behavior, source selection/READY semantics, four-slot transfer data and known-good VS menu treatment. Historical two-fighter or green-loop proofs do not qualify today's complete roster.

## Package: bounded preview selection

**Outcome:** One to four visible preview slots remain correct during browsing, selection, cancellation and re-entry, with responsive 30 Hz presentation.

**Prerequisites:** Native fighter preview programs; the scene's admitted RAM/VRAM profile; validated asset identity and lifetime. Use existing compact-preview work rather than introducing another loader. Related blockers in fighter documents link here.

**Implementation boundary:** `nds_menu_shell_css.c`, the imported `mnplayersvs`/`mnplayers1pgame` bridges, and the existing preview load/publish/retire owner. Keep menu selection state authoritative. Each request is identified by slot, kind, costume, requested pose and generation; a completed obsolete request cannot replace the current selection. Reuse an existing equivalent generation mechanism instead of adding a parallel cache.

Separate reading, decoding/normalization, native binding, final publication and retirement. Price the longest indivisible operation; merely decreasing a bytes-per-tick constant is not a bound on finalization. Keep UI/input/audio service alive while preparing a replacement. Do not expose a partially initialized fighter, free a still-referenced asset, or stall the menu in a synchronous full closure load. Longer preparation is preferable to missed presented cadence; any interim presentation follows the source contract and approved loading treatment, not a silently wrong fighter.

**Proof:** Sweep the whole roster with four occupied slots, repeated kinds, all costume choices, rapid reversals and selections made before earlier preparation finishes. Confirm and immediately cancel; move between unlocked/locked cells under both development and real-save masks. Compare requested and displayed identities plus selected-pose animation/voice. Repeat after battle/Results and after leaving the scene. Attribute read versus bind/publish/retire cost on the real-time path.

**Exit:** No stale/missing/mismatched preview or unbounded selection hitch; declared capacity/floor met throughout, with the actual ship configuration's cadence. **Stop:** A shared asset/storage or native-program defect returns to its owner with the exact failed transition, not a separate per-character workaround.

## Package: rules and routes

**Outcome:** Time/Stock, CPU levels, teams/colors/friendly-fire, stage and item settings survive forward and back navigation exactly as the source specifies.

Keep `fighters[4]` instance identity distinct from fighter-kind/native-owner indexing. Exercise human+CPUs, disabled slots, repeated kinds, same-team READY rejection, SSS cancel, random stage selection over the allowed mask, Results START and No Contest. Do not replace source semantics with a convenience test preset. Item Switch belongs to P2-5/P2-7 dependencies; the campaign entry belongs to P2-6.

**Proof:** Read the descriptor at the handoff and verify the actual match, then return and inspect settings. Check move/confirm/back cues and BGM start/stop transitions. Preserve accepted Options/VS visuals while repairing their route or state transfer.

## Package: resource handback

**Outcome:** Each scene relinquishes only resources it owns; repeated natural loops have bounded steady-state usage.

Specify ownership for preview packs and jobs, scene arena, native images, texture handles, palette/OAM/affine state, audio requests and temporary buffers. Drain/cancel outstanding requests before resetting their arena. No background completion callback may publish into the next scene. Reset scene-local latches; retain only source-defined persistent menu/battle/save state.

**Proof:** CSS→SSS→battle→Results→CSS plus title/options detours; repeat until each relevant entry/exit pair has been exercised beyond its first entry. Compare per-scene high-water/floor and live-object counts at corresponding boundaries. Test exit during a preview change. A passing first lap cannot prove cleanup of an outstanding replacement.

## Package: shell qualification

Use the current registry's `p2_shell_loop` for lifecycle and the real-time menu probe for pacing; fast-logic loop ticks are not frame-cost evidence. Preserve `p2_battle_realtime` as the Mario/Fox/Dream Land regression through the shell. Run the widest relevant profile once for the kept checkpoint, then build the intended human-input P2 artifact per `VERIFYING.md`.

Required surfaces: original title animation/music, main/VS rules screens, CSS roster/settings/READY, SSS map art/selection, loading handoff, Results/rematch and correct back navigation. Main-screen menus remain 30 Hz; bottom screen remains static outside battle unless a separately approved scene requires otherwise.

## Acceptance checklist

- [ ] Every visible preview matches the latest source selection and remains animated/correctly costumed.
- [ ] Real settings and natural routes work, including cancellation and repeated entry.
- [ ] Memory/VRAM/handles/jobs are handed back without leaks or stale references.
- [ ] Native-only packaging, real-time menu cadence and relevant shell/battle guards pass.
- [ ] Source-asset comparisons and any required owner review cover the exact candidate.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `src/nds/nds_menu_shell_css.c`.
- `src/nds/nds_menu_shell_sss.c`.
- `src/nds/nds_scene_manager.c`.
- `src/import/battleship_mnplayersvs.c`.
- `decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayersvs.c`.
- `decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1pgame.c`.
- `scripts/menus/probe-p2-shell.ps1`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-1-vs-shell.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-1-vs-shell.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
