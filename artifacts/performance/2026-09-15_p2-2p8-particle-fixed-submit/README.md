# P2-2p8 — fixed generic particle quad submit KEEP

Date: 2026-09-15

## Change

`ndsRendererSubmitParticleQuad()` was the largest attributed soft-float caller in
the current four-CPU caller census. The generic billboard submit used binary32
multiply/add/conversion for every right/up leg and every emitted corner even
though GX ultimately consumes signed v16 coordinates.

The retained path converts only the render-boundary inputs once per quad:

- center and size: Q8;
- camera/right/up basis: Q13;
- leg construction and range selection: integer arithmetic;
- final GX coordinates: sign-aware shifts to world*16 v16;
- 1/4-world-unit extent guard only for scale selection, never vertex position.

Particle simulation, spawn/RNG order, source transforms, alpha, palette/material
selection, UVs, depth/order, camera selection and batching are unchanged. Whispy's
existing Q12/Q14 AOT contract remains unchanged and reuses the same finite
binary32-to-fixed decoder.

The final linked `ndsRendererSubmitParticleQuad` has no `__aeabi_f*` calls. An
intermediate Q12 implementation was rejected because power-of-two `s64` division
lowered to `__aeabi_ldivmod`; the retained implementation uses shifts and one
wide multiply where needed, with no replacement divide helper.

## Numeric bound

A deterministic host differential compared the former binary32 construction
against the retained fixed construction. Over 100,000 random cases each for
basis component ranges ±1 and ±2, the selected particle batch scale was identical
for every case and every emitted coordinate differed by at most 2 v16 units
(0.125 world unit). Stressing basis ranges ±4, ±8 and ±16 exposed one near-rail
underestimate in the unguarded experiment; adding the Q8 value 64 extent guard
(0.25 world unit) eliminated all under-scaling in 100,000 cases per range. The
guard occasionally chooses a coarser safe scale near a boundary and never moves
the submitted vertex itself.

## Focused 128-frame result

Frames 1400..1527, current four-CPU workload:

| metric | prechecked-replay baseline | fixed submit | delta |
|---|---:|---:|---:|
| WORK-H P50 | 1,733,120 | 1,706,496 | -26,624 |
| WORK-H P95 | 2,475,328 | 2,450,560 | -24,768 |
| MISC P50 | 362,112 | 343,872 | -18,240 |
| MISC P95 | 582,272 | 548,416 | -33,856 |

Paired WORK-H improves on 108/128 frames, median **-16,032** and mean
**-18,505** ticks/frame. MISC, which owns the particle pass, improves on 119/128
frames, median **-14,176** and mean **-16,139**. SRC is essentially neutral.
These are synchronized cross-build rows and candidate-sizing evidence, not a
same-ROM release verdict.

## Final Boundary-produced ROM

Final stress ROM SHA-256:

`1CC0BC02C673D3C12E1C685E3E692EE2879E402709A6F105C18BA8F2985C4244`

ELF SHA-256:

`CFB5946D91DFC8906E688E4D5C14D1EB22E8ECB78C41C7E1F26F5135B15A25A8`

Frames 2..1973 (1,972 samples), compared with the prior retained hard-on
prechecked-replay checkpoint `45869C2A...CC203`:

| metric | prior | fixed submit | delta |
|---|---:|---:|---:|
| WORK-H P50 | 1,654,528 | 1,654,208 | -320 |
| WORK-H P95 | 2,389,376 | **2,375,296** | **-14,080** |
| MISC P50 | 260,352 | 253,824 | -6,528 |
| MISC P95 | 503,552 | **481,024** | **-22,528** |
| SRC P95 | 1,064,064 | 1,057,856 | -6,208 |
| SINT P95 | 597,504 | 589,568 | -7,936 |
| 5+ VBlank presents | 235 | **227** | -8 |

Paired MISC improves on **1,678/1,972** frames, median **-3,648**, mean
**-5,795** ticks/frame. Whole-frame WORK-H improves on 1,060/1,972, median
-768 and mean -2,918. The P95 reduction equals the repository's documented
14,080-tick cross-build significance floor; the stronger owner-local MISC
distribution is the attribution evidence.

Final guards:

- native failures/direct rejects: **0/0**;
- four observed CPU slots: Donkey / Samus / Link / Kirby, draw mask `0xF`;
- pose bind-full/track-overflow/run-mask-fallback: **0/0/0**;
- graphics heap overflow/no-room: **0/0**;
- weapon-pool refusals: **0**;
- general heap low-water: **112,192 B**;
- BGM direct fallback: **0**;
- FGM direct fallback / stdio reads: **0/0**;
- VBlank 2/3/4/5+: **104 / 818 / 824 / 227**, max 13.

Full `verify-all.ps1 -Profile Boundary -RunnerSlot 2` is GREEN on the retained
source: shell loop, natural realtime two-fighter battle, and four-CPU stress all
pass. `check-nds-particle-banks.ps1` is also GREEN.

P2-2p8 remains RED against the product target. This checkpoint removes one
complete render-time soft-float consumer chain; it does not claim fixed particle
simulation or full-runtime no-float closure.
