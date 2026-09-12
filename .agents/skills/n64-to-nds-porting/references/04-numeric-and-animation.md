# 04 — Numeric boundaries, animation, and attachments

Separate simulation and render values. A source float calculation may remain authoritative while its final pose/material output converts once to native values. Converting fixed and immediately back to float does not remove downstream cost. For each replacement, record range, units, resolution, intermediate width, rounding, overflow and consumers. Saturation can change collision or silhouettes.

N64 `Mtx` is split Q16.16; DS matrices use 12 fractional bits and vertices signed 4.12. Rebase/scale model coordinates before packing. In column-vector notation, if `p'=(p-o)/s`, then `M'=M T(o) S(s)` preserves `M'p'=Mp` before quantization. Derive the actual library's convention rather than copying this multiplication order. Do not scale bone translations twice. Track source-local, actor, world, view, clip and packed-local spaces; handle normals under nonuniform/reflected transforms.

[n64_numeric.h](../examples/n64_numeric.h) supplies checked source-s16 packing and Q16.16→Q20.12 ties-to-even conversion, not whole-matrix equivalence. Test signed extrema/negative ties, largest animated extent, joint tips, near plane, large translations with small scales, noncommuting transforms and repeated loops. Average error cannot certify hit/collision predicates. Preserve source zero/near-zero behavior and enough width for squared terms or cross products.

## Compile tracks, not just more frames

| Channel/path | Native starting representation |
|---|---|
| Constant/default | One value; no per-frame evaluator. |
| Exact discrete tick samples | Packed indexed samples or bounded independent blocks. |
| Fractional/variable-speed playback | Keys/coefficients or samples with the required interpolation. |
| Blend/transition | Live inputs and source composition order. |
| Procedural aiming/recoil/IK | Retained bounded procedural stage over the compact pose. |
| Sound/effect/hitbox events | Ordered stream driven by authoritative simulation time. |

Do not linearize a source cubic channel or bake one playback speed without proving fractional, reverse, wrap and transition behavior. Full-pose precompute costs `poses*joints*bytes_per_joint` plus indexing, events, material tracks, alignment and live decode buffers. Compare CPU, ROM, peak RAM and random-access/stream cost together.

Share a coherent pose only between consumers needing the same tick and space. Rendering, collision, grabs, attachments, audio positions and camera targets can require different sampling points. A lower-rate render pose cannot silently replace an authoritative hitbox pose.

Pose caches include clip/time, blend, procedural input, root/facing, scale and generation; view-dependent outputs include camera. Joint effects consume the required owning joint transform plus local offset, not merely actor origin. Test turning, scaling, pause/orbit, air/ground transitions, blend and owner deletion. Keep local/world pose caching separate when a camera-only change can avoid reevaluation. [Sources](SOURCES.md).
