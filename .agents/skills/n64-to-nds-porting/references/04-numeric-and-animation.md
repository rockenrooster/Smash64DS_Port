# 04 — Numeric boundaries, animation, and joint transforms

## Do not turn a float-heavy source port into a float-heavy renderer

Separate simulation values from render values. It is often possible to preserve
a source float calculation while converting its final pose/material output into
native target values once per required update. The conversion boundary should
eliminate downstream float work; converting to fixed and immediately back to
float does not help.

For a proposed fixed-point replacement, record the real range, resolution,
intermediate width, rounding, overflow policy, and consumers. A representation
that fits positions might not fit squared distances, cross products, accumulated
translation, inverse scales, or camera depth. Saturation avoids a crash but can
silently change collision or a silhouette.

Use the companion for target arithmetic helpers, divider/sqrt ownership, and
code-generation details. This chapter concerns source-to-target equivalence.

## Concrete source/target mismatches

| Value | Source consideration | Target boundary |
|---|---|---|
| Integer vertex coordinates | Source model units can be large | Quantize/rebase to target vertex range with compensating transforms |
| Standard N64 `Mtx` | Split integer/fraction blocks, source order | Decode logical elements, then convert precision/convention |
| Texture ST | Source fixed units plus texture/tile operations | Compose sampling transform before native UV quantization |
| Angles | Source binary angles, radians, degrees, LUT conventions | Convert explicitly; preserve wrap and interpolation rule |
| Normal/color union | Meaning depends on source state | Emit the actual normal or resulting color required by the selected recipe |
| Gameplay float | Rounding can affect branches/events | Retain or prove a substitute at a stated semantic boundary |

libnds names its vertex type `v16` (4.12), texture type `t16` (12.4), and matrix
format `f32` (20.12). These names do not imply that all source values fit; a signed
16-bit 4.12 vertex is limited to [-8, 8). [Pinned target formats][formats]

## Model scaling without broken joints

For column-vector notation, suppose source geometry uses `p` and transform `M`,
and a target stored vertex is `p' = (p - o) / s`. Then:

```text
p = T(o) S(s) p'
M' = M T(o) S(s)
M' p' = M p
```

This is algebra, not a prescription for a library's multiplication order.
Transpose/reorder appropriately for the actual source and target conventions.
Quantization adds an error term; test that separately. Normals and lighting need
consistent treatment under nonuniform scales and reflected transforms.

Do not divide every bone translation by `s` as well as multiplying the final
object matrix by `s` without deriving the hierarchy. That can scale translations
twice or detach effects. Keep an explicit unit/space label at boundaries: source
local, actor local, world, view, clip, and target packed local.

The [numeric helpers](../examples/n64_numeric.h) provide checked integer-to-v16
packing and one explicit Q16.16-to-Q20.12 rounding policy. They do not implement
whole-matrix conversion or certify the source RSP's arithmetic. A quantization
policy should be approved/tested rather than hidden inside a type cast.

## Range and error analysis that earns its cost

For a conversion with step `q`, nearest rounding bounds a single scalar error by
`q/2`; floor rounding has a different directional bias. Composition, skinning,
projection, and long-running integration can amplify that error. Test joint tips,
near-plane geometry, large translations, small scales, and repeated loops rather
than only individual packed numbers.

Where exact branch decisions matter, preserve the reference predicate or prove
its replacement over the actual bounded input domain. “Small average error” is
not enough for ground contact, hit detection, or a timer threshold. Squared
comparisons can avoid square roots only with the same sign/domain behavior and
sufficient product/accumulator width. For normalization, zero and near-zero
vectors need the source's behavior, not an arbitrary divide guard.

A 64-bit multiply intermediate is sometimes the correct efficient implementation.
Do not replace it with a narrower overflow because “DS is 32-bit.” Conversely,
variable 64-bit divide or hidden floating conversions in an inner loop deserve
inspection. Prefer eliminating repeated divisions and invariant work first.

## Compile animation representations, not just more sampled frames

A useful default pipeline is:

```text
source tracks/scripts -> classify channels and topology -> compact native tracks
                         + event stream + attachment metadata
```

Eliminate constant channels, common default transforms, repeated script decode,
unchanging interpolation coefficients, and repeated source-pointer resolution.
Choose quantization and storage per channel. Precompute an entire pose only when
its storage and decode/cache cost beat evaluating the live channels.

Distinguish these cases:

| Case | Efficient starting representation |
|---|---|
| Constant transform/material channel | One value, no per-frame evaluator |
| Discrete exact tick samples | Packed samples with direct index or bounded block decode |
| Variable-speed fractional animation | Keyframes/coefficients or samples plus the required interpolation |
| Blends/transitions | Live blend inputs and source composition order |
| Procedural aiming/IK/recoil | Retained procedural stage over a compact pose |
| Events | Separate ordered event stream driven by simulation time |

Do not use linear interpolation for a source cubic channel merely because the
keyframes match. Prebaking samples at one speed may miss fractional-time playback,
loops, reverse motion, or action transitions. Store enough information for all
supported paths. Compression without independently accessible blocks can make
random animation transitions expensive or require a large decoded working set.

A rough precompute cost is `poses * joints * bytes_per_joint` plus events,
material channels, indexing, alignment, and active decode buffers. Compare ROM,
main-RAM working set, bandwidth, and runtime CPU together. “ROM is cheap” does not
mean the expanded data fits in RAM or arrives without a frame stall.

## Evaluate one coherent pose and name its consumers

Consumers can include drawing, hitboxes, grabs, joint-attached effects, audio
positions, and camera targets. Share calculations that truly have the same pose,
space, and required tick; do not give every consumer an independent traversal.
But a render pose sampled less often cannot replace the authoritative collision
pose without permission.

Pose cache dependencies may include clip ID, fractional time, blend amount,
procedural inputs, root/facing transform, scale, and generation. View-dependent
geometry/UVs additionally require camera state. Cache intermediate local/world
poses separately where that avoids redoing work for a camera-only change.

An attached effect should generally consume the required joint transform from
the owning pose plus its source local offset, not just the actor origin. Resolve
hierarchy and space once; preserve when the source samples the joint. Tests need
turning, scaling, air/ground transitions, blending, pause/orbit, and owner deletion.
An effect can look attached at the idle pose and still use the wrong transform.

[formats]: https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/videoGL.h
