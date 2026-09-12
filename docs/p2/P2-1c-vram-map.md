# P2 — Scene VRAM, BG, OBJ and Palette Ownership

This is the ownership/admission contract for both display engines. The pre-revision map and allocation history remain source evidence, not a promise that the same banks are still free in the current configuration. Read the actual platform setup and active native scene tenants before reallocating a bank.

## One claim ledger per scene profile

For each native scene profile, derive or record bank mapping, address/length/alignment, lifetime, owner, layer priority, palette range, OBJ IDs/affine entries and upload/retirement rule. Include hidden but allocated tenants and diagnostic surfaces in the measured build. Track capacity in the generator/scene owner already responsible for allocation; do not add another manually maintained global memory table.

Shared ownership is legal only when lifetimes or disjoint address ranges are proved. A layer being unused is not proof its bank storage is free. A CPU buffer, BG bitmap and texture in the same size class are different resources.

## Baseline to inspect, not blindly reuse

The old source-derived map gave A/B to GX textures, C/D to two full main bitmap layers, E to main OBJ, F/G to texture palettes, H to sub BG and I to sub OBJ when needed. Later native wallpaper/menu/Results work can change those claims. Confirm `ndsPlatformInit`, native wallpaper, UI kit, battle HUD and Results owners in the actual candidate.

A 256×256 16-bit bitmap consumes 131,072 bytes: no unallocated tail exists in a same-sized bank. Reclaiming that bank therefore means retiring/replacing its complete tenant, not merely drawing fewer visible pixels. Loading a native baked bitmap is not a license to restore a software scene compositor into the ROM.

## Scene responsibilities

| Profile | Ownership work and proof |
|---|---|
| Title and menus | Original art/animation plus input/text/cursors; static bottom screen; explicit active layers and source blend semantics |
| VS/1P CSS | Menu surfaces plus native previews, costumes and temporary replacement data; partial/cancelled requests cannot leave old handles alive |
| SSS | Full source map art/name/preview and cursor changes; keep background and foreground ownership distinct |
| Battle | Native fighter/stage/item/effect textures, top-screen telegraphs, lower-screen four-slot HUD; no mandatory late demand |
| Results | Native fighters/emblem plus player tags, text/table and required tint/fill layers; release or share battle/UI tenants only with a real lifetime proof |
| 1P/Training/bonus/tail | Their source scene requirements—not automatic aliases of a menu or battle bank profile |

## Legal allocation and transfer

Use hardware-supported OBJ dimensions/modes from the configured libnds headers. A single 64×16 OBJ is invalid; a composed rectangle must charge each legal cell. Account for padded cell area, 1D mapping granularity, palette entries, OBJ/affine limits and scanline work. Large blank cell regions still consume storage and may affect sprite processing.

Respect VRAM access width/alignment, upload/DMA/cache rules and temporary bank mappings already established by the backend. Palette transfers cannot rely on unsupported byte stores. Perform uploads in the declared safe preparation phase; restore mapping before consumption. Prevent publication while an image/palette is incomplete.

Define entry/exit operations as a pair: suspend former tenant; release/retire handles; switch validated profile; prepare static data; initialize native owners; publish. On failure/cancellation, retain or restore a valid complete state. Neither a blank frame nor a stale palette from the previous scene counts as a valid transition.

## Work packages

**Audit actual claims:** Extract the selected configuration's claims and cross-check sizes/mappings against linked code and generators. Any unresolved ownership conflict is a named blocker, not free capacity.

**Reclaim/reassign only what is needed:** Choose the smallest profile change solving a demonstrated deficit. Preserve required visible layers and their depth/blend meaning. Price both old/new overlap and the reverse transition.

**Prove the round trip:** Source-derived screen/output comparison, collision-free address/ID claims, safe transfer and no stale handles for title/CSS/SSS/battle/Results round trips and the affected campaign/Training scenes. Capture at transition and steady state; a stable single screen does not prove handback.

## Exit

The actual generated configuration—not this prose—proves every bank/OBJ/palette claim and temporary peak fits. All required layers, blend order, native output and 30 Hz presentation survive normal entry/exit. Update the owning scene allocator/checker and relevant evidence when a claim changes; do not append another contradictory “current free bytes” paragraph.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `src/nds/nds_platform.c: ndsPlatformInit`.
- `src/nds/nds_ui_kit.c`.
- `src/nds/nds_native_wallpaper.c`.
- `src/nds/nds_ifcommon_oam.c`.
- `src/nds/nds_results_oam.c`.
- `docs/p2/RESULTS_OAM_DESIGN.md`.
- `docs/p2/P2-texture-residency.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-1c-vram-map.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-1c-vram-map.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
