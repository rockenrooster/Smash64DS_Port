# 10 — Semantic fixtures

These are technical checks to select at the changed boundary, not a mandatory suite for every edit. `docs/BUG_FIXING_PROCESS.md` and `docs/VERIFYING.md` own execution order and acceptance.

Derive expected roots, bindings, equations and extents from BattleShip producers and consumers independently of the converter under test. Generated hashes/counts detect drift, not meaning. Canonicalize pointers/padding/endian before comparison; preserve the first meaningful difference rather than only a checksum.

| Boundary | Discriminating fixture |
|---|---|
| Roots/bindings | Constructor's actual asset and consumer joint; include a source state that deliberately creates no actor. |
| Relocation/compaction | Null/interior pointers, adjacent records, handle pairs, trailing sentinel and complete consumer span. |
| IDs/configuration | Highest and sparse enabled IDs; stable domain versus enabled count; provider/schema agreement. |
| GBI geometry | Inherited child state, tail branches, partial reloads, mixed matrices, overwritten scratch, lighting/winding. |
| Materials | Nonzero transparent TLUT entry with opaque index 0/black; root palette load; same image with alpha used/ignored; sampling, resize and overlap. See [03](03-rdp-materials-textures.md). |
| Numeric/pose | Ranges/rounding, fractional or reverse playback, noncommuting transforms, facing, attachments and consuming tick/space. |
| Gameplay/audio | Observable input/RNG/event ordering, pause/catch-up, late spawns, loop/release timing and transitions. |
| Residency | Complete live roots, instance separation, worst supported overlap, failure cleanup and teardown. |

Use exact equality for guaranteed discrete/serialized properties; use the project's mechanical and approved presentation tolerances elsewhere. Do not turn source availability into a universal bit-exact or pixel-exact requirement. Per-tick comparisons suit unchanged clocks; compensated rate changes need equivalent-event/outcome checks at aligned times.

Coverage needs an expected positive witness: intended state → actual owner → submission → pixels/audio when that source state should produce them. An unvisited zero-reject counter, admission success or triangle count alone cannot establish visible content. Source-intended hidden/offscreen states are not missing-output bugs.

Host fixtures do not certify target rasterization, resource allocation or timing. [Pack tests](../tests/README.md) validate only their documented helper subsets; they are not game acceptance.
