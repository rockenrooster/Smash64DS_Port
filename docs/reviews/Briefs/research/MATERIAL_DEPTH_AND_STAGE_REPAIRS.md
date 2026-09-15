# Materials, depth, stage geometry and hazard lifetimes

Source anchors: R61/R64/R65/R66/R75/R76/R78/R90/R91/R105/R106; HW01/HW02.
These are conditional repair contracts. Only R01/R02/R05 supply target diffs in
this family; E01 supplies a separately tested offline gradient experiment.

## Keep depth, coverage, opacity and texture ownership distinct

The impact ring's problem is spatial depth: R01 stops substituting a fresh nearer
painter value for every triangle. It keeps the existing real source-depth mapping
used elsewhere. R02 repairs the inherited G_ZBUFFER value at the effect-model
caller, not stage sky layers or all weapons. Source lists can still explicitly
clear the bit. Render-mode depth compare/write and geometry state must be audited
together; a source-Z default is not a declaration that all effects have one policy.

A source material contract should retain:

```text
geometry clear/set masks and live inherited geometry state
render-mode compare/write/blend intent
source alpha equation (TEXEL, SHADE, PRIM, ENV; cycle dependencies)
source texture/palette/frame ownership and effective coverage
primitive/environment/shade color and alpha operands
UV origin, scale, tile selection, wrap/clamp/mirror and filtering intent
native sampler format, polygon mode/alpha/ID and depth policy
cache identity for every changing operand above
```

This is a review checklist, not a request for a new per-frame N64 interpreter.
Bake immutable combinations in the producer and keep only genuinely changing
values in the native owner. A stale prepared-material cache may need one missing
key/invalidation, not a replacement renderer.

The DS API defines polygon alpha zero as wireframe, not invisibility. Never solve
transparent content by blindly submitting POLY_ALPHA(0). Whole transparent source
geometry may produce no pixels, but distinguish that from source zero vertex
padding on a material whose alpha does not read SHADE. Full-alpha depth behavior
needs separate testing; a global clamp to 30 changes appearance/ordering and is
not an established fix.

## Opaque cards and hard edges

Inspect coverage numerically in the original decoded source and in the generated
native texture. Intermediate source coverage lost during conversion is different
from correct texels drawn with a non-blending or wrong combiner mode. If source
coverage is binary and the reference softness comes from filtering, preserve the
coverage first, then evaluate a bounded source-derived AOT filter/resampling
solution; do not mislabel all filtering differences as alpha-channel corruption.

The current entry-effect converter already includes alpha-capable IA paths and an
IA8 source-byte correction. Do not reapply obsolete XOR lane swaps or replace the
working shield format. Choose A3I5/A5I3 based on actual color/alpha demands and
native budget, not globally. White RGB can be opaque art, transparent texel RGB or
an intensity operand; a white-color key destroys valid content.

Link's entry wave/column use CI4 materials with primitive alpha, while Spin uses
an IA texture. Their alpha fixes need not be the same. Preserve palette coverage
AND primitive opacity, and test the exact source alpha equation at the group draw.

## Roofs and platform textures

For Peach's newly visible roof and Mushroom Kingdom's left platform, preserve the
geometry repair and compare an affected run with a correctly textured adjacent
run. Carry source texture state across the SAME inherited boundaries. Inspect
material-segment branch, IMAGE/TLUT ownership, active tile and UV phase; emit the
minimal missing state/owner information in the native packet and invalidate stale
prepared state. Do not enable texturing indiscriminately or paint intentionally
untextured source faces.

Distinguish a static map binding from a moving stage-actor binding. Shared immutable
geometry is fine, but the actor requires its own current transform/material epoch.
A valid total packet count does not prove that a particular root is textured.

## Missing Yoster structure

Classify main platforms and main floor/path separately. Compare the source-root
census with generated bindings, then per-root submission, projected coordinates,
culling/clipping, depth and alpha. Patch the first divergence. A missing producer
root, wrong live matrix and an incorrectly transparent material are not one fix.
Preserve source transform-only parents. Do not restore a separately approved haze
omission while claiming it is the missing structure. Collision parity alone does
not establish rendering.

## Saffron door: state versus animation versus pixels

Original `grYamabukiGateUpdateOpen` waits for `monster_gobj == NULL` before calling
`grYamabukiGateSetClosedWait`; that routine sets source waits/position and adds the
closing animation. The gate object has a `gcPlayAnimAll` process. These already
exist in the imported source.

The repair decision is therefore specific:

```text
monster should be retired but pointer survives -> repair monster cleanup owner
state changes, animation does not -> repair process/animation resource ownership
animation changes, submitted matrix does not -> repair native binding/memo lifetime
matrix changes, pixels stay open -> repair material/depth/geometry submission
```

Keep source timer/proximity behavior and collision synchronization. A new fixed
open/close timer is not equivalent, and a baked static packet pose does not by
itself prove the runtime path ignores the live gate DObj.

## Sector Z and Thunder crashes

Classify the first stop: allocator spin, native admission refusal, guest abort,
GX fault, IRQ/wait state or merely an expensive live frame. Record the first
unsafe address, owning source object/native program, generation and prior input.
Potential repairs involve custom-matrix context, child/parent lifetime, foreign
image ownership, indexing or legitimate allocation handling; the fault evidence
selects which. Refuted older theories are not permission for another speculative
stack. Disabling the hazard/move or turning a crash into a fail-closed halt does
not close content or behavior acceptance.
