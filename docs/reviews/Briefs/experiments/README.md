# E01 — source-alpha isoband baking experiment

**Executable offline code; not linked into Smash64DS and not an accepted fix.**
Targets the candidate vertex-alpha cause of Zebes acid/light facets. It is not a
universal fix for opaque textures, missing geometry or arbitrary N64 combiners.

The current stage producer already performs one midpoint subdivision on the acid
root, then averages source vertex alpha per triangle (R91). E01 instead clips
triangles at constant source-alpha bands and triangulates the resulting polygons.
It preserves exact rational plane positions, UV/RGB interpolation and matching
intersection positions on shared edges. It rejects unsupported cross-matrix
triangles and enforces an explicit total output-polygon budget. It emits polygon
alpha in 1..31; exactly transparent SHADE-alpha triangles emit no pixels rather
than DS wireframe.

## Run the host test

```powershell
python .\Briefs\tests\test_alpha_isobands.py
```

Eight analytic tests cover area/winding, plane preservation, UV/alpha interpolation,
shared-edge intersections, transparent/uniform input, non-wireframe alpha, reversed
winding and failure on budget/invalid inputs. These use invented mathematical
triangles, not the original acid or light asset.

For a separately exported source fixture:

```powershell
python .\Briefs\experiments\alpha_isobands.py input.json output.json --levels 8 --max-output-triangles 256
```

Input is a JSON list of triangles, each with three vertex records:

```json
{
  "xyz": [0, 0, 0],
  "st": [0, 0],
  "rgba": [255, 255, 255, 0],
  "matrix_binding": 0
}
```

Coordinates/UVs in output remain exact fraction strings. This is deliberate: an
integration must quantize once using the port's verified fixed-point convention,
not introduce separate rounding on adjoining triangles. No source file is changed.

## Integration required before any ROM experiment

First inspect the project's mandated DS reference renderers, then prove that the
selected material really consumes SHADE alpha and that input RGBA is effective
unlit color, not lighting-normal bytes. Preserve the raw source alpha
before the existing averaging/padding rule. Do not add guessed light root offsets
to an allowlist. Texture alpha remains a separate operand; E01 does not replace it.

Feed only qualified roots through the experiment. Translate emitted vertices into
the current `DenseVertex`/run ABI with correct material/matrix ownership. Preserve
UV phase and deduplicate quantized shared edges. Handle exact zero-area/zero-alpha
results at the producer without changing opaque materials that ignore SHADE alpha.
Split native runs on effective polygon alpha and regenerate all counts/certificates,
including compact field widths, packet capacity and native-only linkage checks.

Measure 4/8/16-band candidates against the existing source-derived result. A full
0..255 ramp triangle may grow to 2*levels-1 triangles, so blindly applying 32 bands
would be a major geometry increase. Output-budget rejection must not silently
fall back to missing or flatter required content. Keep the cheapest representation
that passes source-comparable visual and cadence/resource acceptance.

Polygon alpha 31, translucency classification, depth writes, overlapping polygon
IDs, texture modulation, clipping and DS raster seams still require actual target
proof. The exact-arithmetic host tests do not simulate those behaviors. If an
AOT coverage texture represents the same source gradient more cheaply, compare it
under the same material/camera/animation contract rather than preferring geometry
on principle. Do not commit this experiment as a generic runtime tessellator.

## Projection and lighting limitation

E01 interpolates the supplied scalar alpha on the source object-space triangle.
Its tests do not establish the N64 rasterizer's screen-space interpolation rules
under varying clip W. A camera-dependent mismatch can remain even when every
object-space area/UV test passes. Validate the actual stage camera and animation;
reject or redesign the experiment when perspective changes its fade materially.
Raw lit-vertex normal bytes must not be passed as RGBA color. This version has no
normal interpolation/renormalization or lighting implementation.
