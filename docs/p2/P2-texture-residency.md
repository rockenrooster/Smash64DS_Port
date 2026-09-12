# P2 — Deterministic Scene Texture, Palette and Atlas Admission

Required visuals are admitted as a complete scene working set before use. Retain existing fixed arrays, native prepared handles and generators where useful; replace anonymous capacity guesses and silent required-content exclusion, not the whole renderer.

## Policy

Use a deterministic generated scene-residency plan, committed before GO for battle, with stable handles for that residency epoch. A gameplay-time LRU is not the solution to an overfull simultaneous required set. Missing required resources prevent scene entry with the failed constraint identified; they never fall back to generic rendering or silently remove a telegraph.

Mandatory battle, motion and texture demand reads after GO are zero. Texture create/upload/delete/evict/convert operations for required battle demand are zero during the locked epoch. Legitimate palette/material animation may use a pre-admitted native representation; do not ban visual animation by conflating it with unplanned texture residency work.

Streaming BGM is a declared storage client with reserved buffers, bandwidth/service deadlines and measured interference. Required one-shot gameplay cues need resident or demonstrably deadline-safe policy; they do not automatically inherit the BGM allowance. Count undeclared storage clients as failures.

## Package: truthful containment and observability

Carry the complete first failed identity: scene/profile, native owner/run/state, texture view/key, bytes/format/palette, allocator/bank census and inner failure reason. Preserve positive engagement counts. A first-failure latch reporting no error cannot establish coverage of uncalled or no-op owners.

Do not let one independently diagnosable texture miss erase the meaning of every stage-core validity witness. Diagnose the failing local run while preserving the evidence for valid content. This is containment/diagnosis only: required missing pixels still block acceptance, even when most of the scene can be drawn.

Atlas checking compares **exact required, admitted and excluded sets**, not equal counts. Required-set intersection with excluded is empty. Validate a deliberately missing cell and an equal-count replacement: both must fail. Reuse existing counters/checkers rather than add a parallel diagnostic system.

## Package: generated per-scene manifest

Derive required roots from fighters and copies, their reachable materials/animation frames/children, stage static/moving/decorative content, legal item/summon states, HUD/effects and scene-specific UI. Include cold/late states such as KO, respawn, pause/detail, capture, summon and Results transitions. Union simultaneous requirements; do not store the entire P2 union in every scene.

Each resource states canonical source identity, dimensions/format, palette dependency, resident bytes, handle/view identity, usage/lifetime, mapping/alignment and approved representation. Optional presentation is explicitly owner-approved and separately bounded; required content cannot be reclassified optional merely to pass.

Use scene-specific atlas variants where that reduces mandatory union pressure. Do not assume a standalone texture is better: compare handle count, palette storage, atlas geometry and total bytes. Reuse immutable content across instances without aliasing per-instance animation/material state.

## Package: admission solver and bank handoff

Check independent constraints: texture storage; view/key slots; palette bytes/bases; format-specific placement (including compressed-format coupling when used); atlas rectangles/shared-palette representation; and bank ownership/temporary upload windows. Enough total texels alone does not prove a legal atlas or mapping.

`P2-1c-vram-map.md` owns legal per-scene bank/OBJ/BG claims. Battle may reclaim a menu bank only after its actual tenant retires; a proposed remap is not current availability. Count temporary LCD upload windows and restoration, ARM9 scratch/cache/DMA rules and the next scene's requirements. Do not introduce a runtime mode that disables a required layer to free memory.

Commit resources transactionally: validate complete plan; prepare/upload at the legal boundary; publish stable handles only on success; bind native owners; then retire old scene data. A partial failure cannot leave handles from two epochs alive or create a visually incomplete “successful” scene.

## Package: epoch and transition proof

Prove normal battle, cold uncommon states, source-legal items/summons, capture/copy, KO/respawn and source material-frame changes under the admitted profile. Confirm no mandatory demand work occurs after lock, BGM has no underrun/deadline loss under expected interference, and required cues remain audible.

Test CSS→battle→Results→CSS and campaign replacements/scene boundaries. Scene transition loads are legal but their presentation must remain responsive under its own contract. Mid-fight wave replacement cannot silently read unadmitted mandatory motion/texture data; profile the complete legal wave requirement or provide another explicitly qualified boundary representation without changing source behavior.

## Acceptance

- [ ] Complete source-derived required sets, including late/child states, have no unclassified or excluded required member.
- [ ] All independent bank/format/palette/view/atlas constraints and transient peaks pass.
- [ ] Native owners bind stable valid handles and produce required output, not just successful lookup.
- [ ] Post-GO resource-class counters and audio service witnesses pass their actual workloads.
- [ ] Failed admission, cancellation and repeated scene transitions leave no stale handles or hidden content loss.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `docs/reviews/Review_DS_Texture_VRAM_Residency.md`.
- `docs/p2/P2-1c-vram-map.md`.
- `docs/p2/P2-2-pack-estimator.md`.
- `src/nds/nds_renderer_preamble.c`.
- `src/nds/nds_platform.c`.
- `src/port/renderer_adapter_stage.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-texture-residency.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-texture-residency.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
