# Campaign 08 — DS-Native Texture Formats Everywhere Practical, AOT

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.
>
> **Basis (2026-08-17):** the shipping level is **+26,449** at rank-80 and the fresh per-PC census is `artifacts/performance/2026-08-17_shipping-rebank/v4-c238`. `c200` and `v3-c221` are retired. **Read `SHIPPING_REBANK.md` §7.7 before quoting any figure in this brief** — it lists what the new census contradicts, and mask the census by the GATE's own rank-80 frames.

## Objective

Make ROM assets arrive at runtime already in the final Nintendo DS texel/palette representation required by the renderer.

After GO, static Mario/Fox/Dream Land/P1 VFX should not need texture decode, quantization, palette construction, endian fixup, repacking, or format conversion.

## Current repo anchors

- `src/nds/battle_playable_static_textures.c`
- `src/nds/nds_renderer.c`
- `scripts/generate_battle_playable_static_textures.py`
- `scripts/generate_battle_playable_texture_census.py`
- `scripts/check-battle-playable-static-textures.ps1`
- `scripts/check-battle-playable-texture-census.ps1`
- `scripts/audit-p1-vfx-textures.py`
- `scripts/census-texture-demand-postko.ps1`
- `scripts/census-texture-key-rebuild.ps1`
- particle/VFX generators

## Phase 0 — Post-GO texture-path census

Instrument paths that:

- decode pixels;
- quantize;
- swizzle/repack;
- build palette;
- endian-swap;
- convert source formats;
- allocate converted cache entries;
- upload to VRAM.

Classify every asset:

1. already final DS-native;
2. static but still converted at runtime;
3. genuinely dynamic content;
4. diagnostic/oracle only.

Prioritize class 2.

## Phase 1 — Define one final native asset contract

Generated static texture data should expose only what ARM9 needs:

- width/height;
- DS texture format;
- wrap/flip flags;
- texel byte length/alignment;
- palette count/length;
- final texel bytes;
- final BGR555 palette bytes;
- transparency metadata.

Runtime must not rediscover format by interpreting N64 display-list commands.

## Phase 2 — Move conversion to host

Extend existing generators.

For each class-2 asset:

1. decode source on host;
2. apply exact intended quantization/palette rule;
3. emit DS-native bytes/metadata;
4. compare reference output;
5. add deterministic checker fixture;
6. delete runtime conversion for that asset.

Deduplicate palettes AOT only when semantics permit.

## Phase 3 — Delete normalization/conversion caches

Qualified native asset lookup should return a native descriptor directly.

Delete:

- source-format decode;
- temporary conversion buffers;
- conversion-cache key building;
- repeated palette generation;
- endian/repacking.

If a cache remains, it should represent VRAM residency/bindings, not N64→DS conversion.

## Phase 4 — Treat truly dynamic textures separately

Read `docs/OPTIMIZE_LIST.md`'s billboard observation first (owner, 2026-08-06):
on N64 every VFX except the platform is a camera-facing billboard at the
fighter's own Z, always drawn on top, alpha on some parts. If confirmed against
BattleShip, effect textures are flat billboard sheets — no per-effect 3D
geometry — which simplifies what "native format" means for the whole VFX
class. It is a design input, not a filed bug; confirm before building on it.

Also remember the measured A5I3 lesson (2026-08-03): with a shared 8-entry
palette, A5I3 carries shape+alpha only, and a two-tone asset routed through it
loses its second color entirely. Histogram each asset's palette indices before
choosing its format; the format decision is per-asset, not per-class.

For dynamic content ask whether:

- only palette changes;
- frames can be baked;
- source can remain in native format;
- a compact native delta can be applied.

Document genuine exceptions rather than forcing static assumptions.

## Phase 5 — Integrate GO/DMA

Campaign 14 should assert zero avoidable post-GO texture conversion.

Campaign 10 may benchmark DMA for large final-format VRAM transfers, but do not introduce a staging copy just to use DMA.

## Verification

- generator fixtures;
- class-2 P1 count reaches zero;
- Mario/Fox/Dream Land/VFX visual parity;
- VRAM binding correctness;
- zero post-GO conversion/repack counters;
- ROM/RAM footprint;
- renderer/texture ticks;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law).

## Completion criteria

Every practical P1 static texture is stored in final DS-native texel/palette form and consumed directly. Any remaining runtime conversion is explicitly justified as genuinely dynamic/unavoidable.
