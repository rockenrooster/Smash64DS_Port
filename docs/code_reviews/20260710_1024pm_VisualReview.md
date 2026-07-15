Fresh character-focused assessment

The new build has crossed an important threshold: the major facial/UV failures are largely fixed. Mario now has a readable face and cap emblem, while Fox now has a proper tan snout, small nose, eye, and separated cheek region. The characters no longer look as though major pieces of their head textures are missing.

At this point, the remaining mismatch is primarily material color, lighting, and smoothness, plus one conspicuous local problem on Fox’s tail.

Mario
What is now working

Mario’s identity reads much more accurately:

The eye, moustache, ear, sideburn, and hair are now visible.
The cap marking is present rather than appearing as a broad random white wedge.
The facial materials are assigned to the correct general areas.
His red shirt, blue overalls, gloves, skin, and hair are clearly separated.
The overall head and body silhouette is reasonably consistent with the reference model.

The earlier evidence for a badly broken Mario face UV is no longer present.

Remaining differences
Area	Current port	Reference
Face color	Pale, slightly gray/yellow	Warmer peach-brown
Cap	Very solid, saturated red	Softer red with visible light variation
Cap badge	Visible, but reads as a tall white mark	Compact white badge with a clearer red “M”
Moustache	Small, compressed black patch	Broader and more prominent beneath the nose
Gloves	Large, almost flat white mass	Rounded, shaded gray-white
Overalls	Very saturated, nearly uniform blue	More moderate blue with smooth gradients
Shoes	Almost completely black	Clearly dark brown/reddish-brown
General surface	Hard polygonal color blocks	Softer Gouraud shading and filtering

The black shoes remain Mario’s most obvious material-color error. They are dark enough to merge visually with the ground-facing silhouette, whereas the reference preserves their brown coloration even in shadow.

Before changing the shoe material itself, test Mario with lighting disabled. If the shoes turn brown in an unlit pass, their source color is probably correct and the lower-body lighting is simply being crushed too aggressively.

The cap badge is much improved but still does not read as cleanly as the reference. At the current angle, it looks narrow and vertically stretched. This could be residual UV scaling, the badge’s texture-combiner treatment, or simply too few source pixels surviving at DS resolution.

Geometry assessment

Mario’s cap, nose, glove, and shoes appear more angular than the reference, but I would not remodel them yet. Most of that impression can be explained by:

Native DS-scale rasterization.
Lack of the reference emulator’s filtering and antialiasing.
Flat or overly contrasty lighting.
Different animation pose and projection.

The underlying proportions are close enough that lighting and materials should be corrected first.

Fox
What is now working

Fox has made the larger improvement of the two characters.

The previous giant black muzzle defect is gone. His head now has:

An orange-brown crown.
Pale inner ear.
Large white cheek.
Tan snout.
Small black nose.
Readable eye.
Red scarf beneath the head.

This makes the head structure fundamentally coherent. Based on this new image, bad head triangle indices are no longer the leading hypothesis. The previous muzzle problem was more likely caused by UV or material state than by a permanently corrupted mesh.

His cream jacket, olive pants, gloves, boots, and scarf are also assigned to the correct broad areas.

Remaining differences
Area	Current port	Reference
Tail	Dark, strongly banded, dark tip	Mostly orange-brown with a pale cream tip
White cheek	Large, flat, high-contrast polygon	Softer rounded cheek with gradual shading
Snout	Pale and sharply tapered	Warmer and visually rounder
Fur	Darker brown/orange	Brighter orange-brown
Jacket	Slightly gray-green	Warm cream
Pants	Dark, almost uniform olive	Olive with smoother light variation
Boots	Noticeably pale blue/cyan	Neutral white and gray
General surface	Hard transitions between polygons	Smoothly shaded transitions
Fox’s tail is now the highest-priority character defect

The port tail reads as alternating dark and orange bands, and its visible end is dark. In the reference, the tail has a much simpler orange body and a clearly pale tip.

That could result from:

The tail-tip draw using the wrong primitive or material color.
State leaking from the pants or another dark body part.
The cream-tip batch not being submitted.
Incorrect UV scale or wrapping on this particular texture.
Incorrect normals creating alternating dark facets.

A fast isolation test would be to render the tail body and tail tip as two different unlit debug colors. That will establish whether the pale-tip geometry exists and whether the issue is geometry, material assignment, or lighting.

Fox’s face still needs softer value separation

The head is now structurally correct, but the white cheek, orange fur, tan snout, and dark eye form very abrupt blocks. The reference uses essentially the same regions, but their shading connects them into a rounded head.

This is now more of a normal/lighting problem than a texture-placement problem. The white cheek should not be removed or drastically resized based on this view alone.

Shared character-rendering issue

Both characters show the same broad rendering behavior:

Lit areas approach pure source color.
Shadowed areas become extremely dark.
Large polygons remain almost internally uniform.
Adjacent polygons change brightness abruptly.
Neutral whites acquire gray or cyan casts.
Lower parts such as shoes, pants, and tail facets lose most of their original color.

That points toward checking the character lighting pipeline:

Verify signed normal decoding and normalization.
Confirm normals are transformed by the model rotation without translation.
Check whether smooth/Gouraud interpolation is enabled on all character polygons.
Compare the ambient term against the reference; the port appears to have insufficient ambient light.
Verify that texture or primitive color is multiplied by shade only where the original material expects it.
Check for primitive, environment-color, or material-state leakage between limbs.

A full-ambient, no-directional-light capture would be especially diagnostic. If Mario’s shoes become brown, Fox’s boots become neutral white, and Fox’s tail becomes orange/cream, then the asset colors are already mostly correct and lighting is the main remaining cause.

What the environment progress tells us

The corrected canopy, flowering bushes, grass, and diamond-patterned island side show that the earlier global texture-coordinate failure has largely been resolved. Mario’s restored face and Fox’s restored muzzle support the same conclusion.

Therefore, another broad rewrite of the global UV path is probably not the best next step. The remaining character problems look more localized:

Fox tail body/tip material or UV state.
Mario shoe color or excessive lower-body darkness.
Character normal and ambient-light handling.
Fox boot color and jacket warmth.
Mario cap-badge presentation.
Final filtering and color tuning.
Current verdict

Mario: broadly correct and recognizable, with the biggest remaining errors being black shoes, overly flat primary colors, cool skin, and a still-imperfect cap badge.

Fox: face and muzzle are now fundamentally correct; the tail remains visibly incorrect, while the rest of the mismatch is dominated by harsh shading and cool/dark material rendering.

The best next technical test is an unlit flat-color character pass. It will cleanly separate remaining asset/material errors from lighting errors before any further geometry changes are made.


Stage assessment

The stage has improved substantially. The earlier one-axis UV collapse appears largely fixed: the canopy now has full two-dimensional foliage detail, the shrubs have distributed flowers instead of horizontal bands, and the island side has its proper diamond pattern. The remaining problems are now concentrated in specific meshes and material paths rather than the entire stage conversion.

What is now broadly correct
Asset	Current state
Canopy foliage	Correct underlying leaf texture and mapping are now recognizable.
Flowering shrubs	Major improvement; flower placement, leaf pattern, and overall silhouette are close.
Island cliff	Diamond texture, yellow-to-brown progression, and repetition now look fundamentally correct.
Tree bark	Correct texture and highlight pattern are present on the main and secondary trunks.
Grass and path	Correct texture families are now mapped across the ground.
Background	Sky, clouds, rainbow, and distant scenery remain a good match.
Overall construction	Tree placement, shrub placement, platform arrangement, ponds, fences, and floating-island structure are all present.

The cliff and shrubs are probably the strongest current stage assets. I would not make major geometry or UV changes to either based on these images.

Most important remaining discrepancies
1. The ground flowers are still absent

This is now the largest missing component of the scene. In the reference, dense rows of blue, pink, yellow, and purple flowers occupy much of the path border and foreground. In the port, those areas are bare grass.

Their absence affects more than detail: it changes the entire visual balance of the stage and makes the path, tree roots, and shrub bases look overly exposed.

This likely belongs to a specific billboard/alpha-material path. A useful test is to force the flower draw calls to:

Use an opaque solid debug texture.
Disable culling.
Set maximum polygon alpha.
Ignore their normal combiner and alpha-test state.

If quads appear, the problem is texture format, palette alpha, or tex-edge handling. If nothing appears, the geometry or its display list is being skipped.

2. The canopy ornaments are still triangles rather than stars (Outdated)

Their positions and color assignments are approximately right, but their geometry is not. Every ornament appears as a shaded triangular wedge or pyramid, while the reference uses flat five-point stars.

Because all of them fail in the same way, this looks deterministic. Possibilities include:

Only part of a triangle fan being submitted.
Incorrect vertex-cache indices.
A primitive opcode being decoded incorrectly.
Most star triangles being rejected by backface culling.
A transformation or matrix state changing partway through the mesh.

A wireframe pass with culling disabled should distinguish these immediately. This is not a filtering or palette problem.

3. The pond rendering is incomplete

The large pond currently appears as a nearly uniform pale-blue polygon. The reference contains visible moving or streaked water detail across its whole surface.

The small pond shows more texture, but its appearance is inconsistent: it has a dark teal region and a sharp pale section rather than one continuous softly rippled surface. That suggests the ponds may involve multiple passes or separate material states.

The current result looks like the large pond is showing only a base-color pass, with its animated/translucent texture overlay missing. Test the water by rendering every pass as opaque texture-replace:

output = TEXEL0
alpha  = maximum
blending disabled

If the ripples appear, the problem is blending, combiner state, or polygon alpha. If they remain absent, inspect texture selection, texture scrolling, UVs, and whether all water display lists are submitted.

4. Platform end caps are much too large

The reference platforms are mostly brown wood, with very narrow yellow caps at the ends. In the port, the yellow regions occupy a large fraction of every platform, leaving a much shorter brown center.

This is especially clear on the central platform, where perspective is least ambiguous. The cap regions appear several times wider than they should.

There are two likely cases:

If the platform is one textured mesh, the S range or tile window is still wrong for this material.
If the caps are separate geometry, their local scale or segment transform is wrong.

Assigning a unique flat color to each platform draw call will reveal whether the oversized yellow area follows separate geometry or comes from one texture.

The wood itself is recognizable, although it is darker and more heavily banded than the reference.

5. The tree face still has incorrect secondary geometry

The eyes and mouth are now correctly recognizable as dark oval openings. The nose, however, still does not match:

The reference has a small rounded branch-like nose. (Tyler notes: nose looks fine)
The port has a longer, sharply tapered protrusion.
Thin striped or fragmented slivers remain around the nose and beneath the facial openings. (Tyler notes: yes this is a finding)

Those slivers may be unintended internal faces, duplicate geometry, incorrect indices, backfaces, or depth fighting between nearly coplanar pieces. This deserves a wireframe and flat-color inspection before changing the face textures.

The bark and primary facial openings themselves look good, so this is localized to the nose/face assembly.

6. The foreground fences appear incomplete (Depth issue? the Floor path renders in front of the foreground fence)

The rear-center fence is recognizable and reasonably close. The two front fence sections appear much thinner than in the reference, with several rails or pickets apparently absent.

Camera angle accounts for some of this, particularly in the close screenshot, but the centered wide image still shows less fence structure than the reference. This is a medium-confidence issue rather than a definite one.

A no-culling test is appropriate because these thin fence polygons could easily be using winding or double-sided behavior that differs from the original.

Assets that mainly need rendering polish
Canopy

The foliage mapping is now correct, but the port is:

Sharper and more pixelated.
Darker and more saturated.
More visibly segmented by individual polygons.
Chunkier around the alpha-cutout lower edge.

The reference benefits from N64-style texture filtering and softer output scaling, while the DS result is effectively nearest-neighbor. The underlying asset does not appear fundamentally wrong anymore.

The canopy may eventually benefit from an offline-prefiltered texture, but that should come after the missing meshes and materials are fixed.

Shrubs

The shrubs are now close to the reference. Their major remaining differences are:

Harder diagonal leaf strokes.
More angular silhouettes.
Stronger green contrast.
More visible trunks and lower edges, partly because the foreground flower sprites are missing.

I would classify these as filtering and scene-dressing differences rather than an incorrect texture conversion.

Cliff

The diamond-patterned island side is a major success. Compared with the reference, it is somewhat:

More contrasty.
More yellow near the top.
Darker near the bottom.
More visibly faceted.

Those are secondary color/filtering concerns. The reference image does not expose the full lower silhouette, so I would not alter the serrated bottom geometry based only on these captures.

Grass and path

Both are correctly mapped now. The remaining visual mismatch is that the port grass is fluorescent and very high-frequency, while the path is more orange and sharply patterned than the softly filtered reference.

Much of that may come from:

Nearest-neighbor sampling.
Palette quantization.
Stronger shade multiplication.
Different gamma/output scaling.
Insufficiently soft ambient lighting.

Do not tune those colors aggressively until the flowers are restored, because the missing flowers currently expose far more grass and path than the intended scene does.

Current technical picture

The broad texture-coordinate problem is no longer the dominant issue. The outstanding stage defects divide into clearer local categories:

Problem category	Likely affected assets
Missing alpha/billboard path	Ground flowers
Primitive assembly or culling	Canopy stars; possibly face pieces and fences
Local UV/tile or segment scaling	Platform end caps
Multipass, alpha, or texture animation	Ponds
Filtering, lighting, and palette differences	Canopy, shrubs, grass, bark, cliff, path
Recommended stage fix order
Restore the ground flowers. They cover a large visual area and will make the stage immediately resemble the reference more closely.
Fix the star geometry. Their triangular silhouettes remain one of the most visible unmistakable errors. (Outdated)
Restore the complete water material, especially the large pond’s ripple layer.
Correct the platform cap proportions.
Resolve the tree-face slivers.
Verify the foreground fences with culling disabled (depth issue?).
Finish with global filtering, saturation, ambient-light, and contrast adjustments (not sure if actually needed).

The core stage conversion is now in good shape. Once the flowers, stars, water, and platform caps are corrected, most of the remaining difference should be the expected rendering-style gap between the DS pipeline and the filtered N64 reference.