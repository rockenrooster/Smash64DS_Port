Dream Land and fighter visual-correctness audit

I compared the supplied DS capture with the N64 reference and traced the relevant stage, material, texture, depth, and fighter-display paths in the read-only Lean snapshot. No files were changed.

The snapshot is still 20260710_060757 and excludes Git metadata, so the live agent tree may already be ahead of some lines below (SNAPSHOT_MANIFEST.txt:1-12).

The N64 reference uses Fox and Kirby while the DS capture uses Fox and Mario, and the captures are from different battle states. It is reliable for stage composition and object appearance, but not for an exact fighter-size or camera comparison.

What the images indicate
Visible problem	Assessment
Dark navy background instead of sky and floating island	Confirmed missing wallpaper/SObj path
Whispy face reduced to misplaced dark fragments	The map GObjs exist, but their material or texture state is wrong
Left bush contains a large pink horizontal patch	Strong texture-window/material-state symptom
Purple canopy star appears as a triangular shard	Either one triangle/state pass is missing, or the alpha texture window is wrong
Two small ponds become huge cyan/white forms	Strong texture-window, alpha, or tile-repeat symptom
Canopy, trunks, and platforms overlap unnaturally	Depth plus opaque/translucent head-order problem
Mario and Fox are recognizable but fragmented	Source triangles are largely present; spatial depth and display-state playback are wrong
No fighter contact shadows	Confirmed stub
Stage seems enlarged/cropped	Not proven from these unsynchronized captures

The screenshot is not showing one residual bug. At least four renderer contracts are independently wrong: source depth, stage material normalization, stage display-head ordering, and N64 texture-window semantics.

1. Blocker: source Z is replaced with submission-order depth

The current hardware path always selects projected submission whenever a triangle is both transformed and Z-buffered:

return ((zbuffered != FALSE) && (transformed_ready != FALSE)) ?
    TRUE : FALSE;

at src/nds/nds_renderer.c:3916-3922.

Once that condition is true, the renderer explicitly disables the source Z behavior:

zbuffered = FALSE;
decal_depth = FALSE;
prim_depth = FALSE;

at src/nds/nds_renderer.c:4136-4142.

It then generates depth from a global decrementing counter at src/nds/nds_renderer.c:3828-3837. The submitted X and Y come from the transformed clip vertex, but Z comes from this synthetic counter at src/nds/nds_renderer.c:3840-3857 and 3964-3990.

The result is effectively:

triangle visibility = submission order

instead of:

triangle visibility = source geometric depth

This affects:

Dream Land layer 1, which is intentionally Z-buffered.
Both fighters, whose source display preamble enables G_ZBUFFER.
Self-occlusion within a fighter.
Occlusion between fighter parts and the stage.
Overlap between tree, platforms, and other stage pieces.

This is the strongest shared explanation for Mario’s stacked-looking body, Fox’s disconnected-looking limbs, and stage pieces appearing in front of geometry that should cover them.

The repository itself acknowledges that the CPU-oracle projected fallback is not independent proof of raw DS matrix/depth correctness at docs/KNOWN_ISSUES.md:1060-1065. The 320/306 source-selected fighter triangles and zero rejects at docs/STATUS.md:65-80 prove that geometry reached the renderer; they do not prove that it was depth-composited correctly.

Required acceptance condition

Ordinary Z-buffered stage and fighter triangles must retain their actual eye-space or clip-space depth. Either the raw DS matrix path must be corrected, or the projected path must preserve mathematically equivalent source depth and homogeneous-W behavior.

A useful metamorphic test is to reverse the submission order of opaque Z-buffered DObjs. A correct opaque image should remain unchanged. The current synthetic-depth path will generally change.

2. Blocker: Dream Land MObjSub records are still lane-swapped

Every relocation payload is blanket-swapped as 32-bit words:

ndsRelocWriteNative32(word, ndsRelocReadBe32(word));

at src/port/reloc_backend_assets.c:2045-2062.

That is insufficient for MObjSub, because it contains packed combinations such as:

u16 pad00;
u8 fmt;
u8 siz;
...
u16 flags;
u8 block_fmt;
u8 block_siz;

at decomp/BattleShip-main/decomp/src/sys/objtypes.h:300-342.

A correct mixed-lane repair already exists at src/port/reloc_backend_assets.c:3216-3267, but its table walker rejects every asset except NDS_RELOC_ASSET_MV_COMMON:

loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON

at src/port/reloc_backend_assets.c:3392-3405.

The load dispatch calls that repair only for MV Common at src/port/reloc_backend_assets.c:4614-4617. Dream Land gets only its MPGroundData/map header normalized at src/port/reloc_backend_assets.c:2697-2721.

That leaves material records in the Dream Land asset family untreated:

GRPupupuMap, asset 0xFF
StageDreamLand, asset 0x10058
ExternDataBank104, asset 0x68
MiscDataBank152, asset 0x98

registered at src/nds/nds_reloc_assets.c:63-71.

The original stage path then copies these records without repairing them:

Geometry layers call gcAddMObjAll() at decomp/.../gr/grdisplay.c:182-205.
Whispy map objects do the same at decomp/.../gr/grcommon/grpupupu.c:637-669.
gcAddMObjForDObj() copies the whole source MObjSub at decomp/.../sys/objman.c:1302-1335.

By comparison, fighter material attachment already makes a local copy and calls the mixed-lane normalizer at src/port/reloc_backend_compat_shims.c:7798-7815. This explains why fighter palette colors improved while Dream Land’s face, bushes, and material-driven details remain badly malformed.

Concrete Whispy fixtures

The source symbol table identifies:

Whispy eyes material table: 0x0F00
Whispy mouth material table: 0x13B0
Back-flower DObj tree: 0x2A80
Front-flower DObj tree: 0x31F8

at decomp/BattleShip-main/tools/reloc_data_symbols.us.txt:4002-4009.

The eyes material is defined at decomp/.../relocData/152_StagePupupuFile3.c:204-223 and should decode to:

fmt = 2
siz = 2
logical material dimensions of 32×32
flags = 0x0001
block_fmt = 2
block_siz = 0
material control field unk4C = 0x00002005

The mouth fixture at decomp/.../relocData/152_StagePupupuFile3.c:464-483 is the corresponding 32×16 material.

Without lane repair, the eyes’ first source word:

00 00 02 02

becomes a native structure that looks approximately like:

pad00 = 0x0202
fmt   = 0
siz   = 0

Likewise, the source flags, block_fmt, and block_siz lanes become mixed. The renderer directly consumes these fields at src/port/reloc_backend_renderer_dl.c:2614-2678.

That is a direct code-level explanation for the face being present as geometry but rendered as misplaced or incorrect texture fragments.

Correct repair boundary

Normalize Dream Land material tables once, after relocation pointer fixups and before original gcAddMObjAll() copies them into live MObjs. The normalizer should deduplicate source record addresses and validate that each address belongs to one of the expected loaded stage files. Repairing them opportunistically in the renderer would make lifetime and double-normalization behavior harder to prove.

3. High: source opaque/translucent DL-head ordering is discarded

BattleShip does not draw Dream Land as one flat list of DObjs.

Its source callbacks distinguish:

Layers 0, 2, and 3: no Z-buffering.
Layer 1: Z-buffering.
DL head 0: opaque geometry.
DL head 1: translucent geometry.

The exact source setup is at:

decomp/.../gr/grdisplay.c:52-83
decomp/.../gr/grdisplay.c:86-108
decomp/.../gr/grdisplay.c:111-155

The display links are defined as layer 0 → link 4, layer 1 → link 6, layer 2 → link 13, and layer 3 → link 17 at decomp/.../gr/grdisplay.c:10-42.

The original object traversal appends each DObj’s commands to the appropriate gSYTaskmanDLHeads[dl_link->list_id] at decomp/.../sys/objdisplay.c:1695-1739. This preserves a global opaque stream and a separate global translucent stream.

The DS adapter does not preserve that contract:

It reconstructs only a Z/no-Z decision from the GObj display-link ID at src/port/reloc_backend_movement.c:12735-12741.
It immediately submits each DObj at src/port/reloc_backend_movement.c:12863-12884.
It range-checks DObjDLLink::list_id, but then calls ndsRendererAdapterSubmitStageDL() without passing that list ID at src/port/reloc_backend_renderer_dl.c:3545-3592.
The source gDPSetRenderMode() and geometry preambles written by grDisplayLayer*ProcDisplay() are not interpreted by the hardware adapter. The imported gcDrawDObj* functions only record callbacks at src/port/opening_movie_backend.c:4486-4533.

Consequently, the order changes from:

all head-0 opaque geometry
then all head-1 translucent geometry

to something closer to:

DObj A opaque
DObj A translucent
DObj B opaque
DObj B translucent
...

That matters particularly because the hardware flush uses manual translucent sorting:

glFlush(GL_TRANS_MANUALSORT | GL_WBUFFERING);

at src/nds/nds_platform.c:953-965.

It can visibly break:

Canopy leaf and star billboards.
Flower and bush alpha edges.
Platform alpha or decal elements.
Tree/platform overlap.
Material planes intended to be drawn after all opaque geometry.

There is also a secondary state-lifetime issue: the stage persistent texture/tile/segment state is zeroed in ndsRendererAdapterBeginStageTraversal() at src/port/reloc_backend_renderer_dl.c:2559-2570, while the movement adapter begins and ends a traversal separately for each stage GObj at src/port/reloc_backend_movement.c:12995-13005. The source DL heads can retain state across multiple GObjs within the camera pass.

Required architecture

The stage adapter should retain at least:

camera pass
GObj display link/layer
DObj identity
DObjDLLink list_id
source preamble state
display list
material and matrix identity

It should replay each source head with its own persistent state, preserving the source head order rather than immediately flattening every DObj.

4. High: texture source-window, DXT, mask, and shift semantics are incomplete

This is the strongest code-level suspect for the huge cyan ponds, the pink bush patch, and possibly the triangular purple star.

The renderer records LOADBLOCK’s origin and DXT at src/nds/nds_renderer.c:850-866, but the hardware uploader treats LOADBLOCK as:

source_origin_s = 0;
source_origin_t = 0;
source_width = width;

at src/nds/nds_renderer.c:3494-3506.

By contrast, LOADTILE uses the image origin and source image width. The LOADBLOCK DXT value is recorded but is not used to reproduce source row stepping during upload.

The tile’s masks, maskt, shifts, and shiftt values are stored at src/nds/nds_renderer.c:830-840, but hardware texture coordinates only subtract the tile origin and apply a scale:

relative = coord - (origin << 3);

at src/nds/nds_renderer.c:2310-2316.

The DS texture parameters implement clamp, wrap, and mirror at src/nds/nds_renderer.c:2660-2689, but they do not implement N64 mask-period and shift arithmetic. Textures are also padded to the next power-of-two upload size at src/nds/nds_renderer.c:3482-3486; wrapping can therefore sample padded texels instead of reproducing the N64 tile period.

This debt is already acknowledged at docs/KNOWN_ISSUES.md:1055-1059.

How to distinguish the purple-star failure

The purple canopy artifact is an excellent one-frame diagnostic:

Render its DObj with a flat untextured color.
If it remains a triangle, the second source triangle is missing, culled, clipped, or submitted under the wrong head.
If it becomes a complete quad or intended polygon, geometry is sound and the defect is the texture window or alpha decode.

The same test applies to the water objects:

A correct flat-colored quad with an incorrect textured pond means tile/alpha semantics.
An oversized flat-colored shape means its matrix, geometry, or selected DL is wrong.
Recommended one-frame draw trace

For every stage submission, record:

owner GObj and source layer
DObj tree path
DL asset ID and offset
DObjDLLink list_id
MObj asset ID and source offset
fmt / siz / flags
logical width and height
LOADBLOCK or LOADTILE
SETTIMG image width
line and TMEM values
uls / ult / lrs / lrt
DXT
maskS / maskT
shiftS / shiftT
palette identity
observed S/T range
projected screen bounding box
texture-cache key

The projected bounding box will make it easy to locate the event producing the cyan foreground object without guessing from asset names.

5. High: the fighter display contract omits source RDP state

The original fighter display path establishes a substantial dynamic preamble:

Two-cycle mode.
Z-buffering, shading, culling, lighting, and smooth shading.
Fog/opaque render mode.
Environment and fog colors.
Per-part NOFOG and TOGGLEFOG decisions.
Opaque versus translucent render mode according to alpha.
Restoration to one-cycle state afterward.

See:

decomp/.../ft/ftdisplaymain.c:687-720
decomp/.../ft/ftdisplaymain.c:1164-1203
decomp/.../ft/ftdisplaymain.c:1230-1242

The wrapper currently collapses several render-mode constants to zero and turns numerous static GBI commands into empty entries at src/import/battleship_ftdisplaymain.c:46-86.

Its explicit interception covers only:

selected display list
geometry mode
environment color
primitive color

at src/import/battleship_ftdisplaymain.c:97-111.

Fog color is a no-op at src/import/battleship_ftdisplaymain.c:113-118.

The captured event therefore contains only:

DObj *
matrix DObj *
material DObj *
Gfx *
geometry mode
primitive color
environment color
one Light

at src/port/reloc_backend_renderer_dl.c:5371-5401.

Those fields are copied at src/port/reloc_backend_renderer_dl.c:5548-5579 and used to seed playback at src/port/reloc_backend_renderer_dl.c:6510-6523 and 6562-6568. There is no event representation of the source cycle type, render mode, fog color/state, alpha-mode decision, or comparable dynamic RDP state.

This can cause:

Incorrect transparent part edges.
Wrong fog or color-animation appearance.
Opaque parts treated as translucent, or vice versa.
Loss of per-part state transitions.
Flat or inconsistent fighter shading.

It is probably not the primary cause of the detached body pieces—that is more strongly explained by synthetic depth—but it will remain visible after depth is corrected.

The original source uses one directional light at decomp/.../ft/ftdisplaylights.c:10-26, so merely adding more lights is not the correct first fix. The stronger confirmed losses are depth and render/fog semantics.

The contract should capture semantic state before the wrapper’s placeholder constants erase it. Capturing gDPSetRenderMode(..., 0, 0) after macro reduction cannot recover which source mode was intended.

6. Medium: fighter anim-lock matrix behavior is missing

The DS fighter-parts matrix bridge currently handles:

the cached transform_update_mode matrix
ordinary translate/rotate/scale
ordinary translate/rotate

at src/port/reloc_backend_renderer_dl.c:390-440.

The original matrix callback has a separate branch when fp->is_use_animlocks is set. It calculates cumulative part scale, calls lbCommonMatrixTraRotScaInv(), and updates global traversal scale at decomp/.../lb/lbcommon.c:1369-1441.

Fighter motions toggle this behavior at decomp/.../ft/ftmain.c:4724-4735.

This is motion-specific and may not explain every neutral-frame fragment, but affected motions will produce:

Separated joints.
Incorrectly stretched limbs.
Parent/child scale discontinuities.
Pose-dependent model breakage that disappears in other statuses.

A neutral pose alone is therefore insufficient as a fighter matrix gate. At least one known anim-lock motion must be included.

7. Confirmed missing visual systems

These are separate from the malformed 3D geometry and should not be confused with renderer regressions.

Dream Land wallpaper

The source battle scene creates a wallpaper camera and calls grWallpaperMakeDecideKind() at decomp/.../sc/sccommon/scvsbattle.c:152-159 and 423-431.

Dream Land falls through to grWallpaperMakeCommon() at decomp/.../gr/grwallpaper.c:267-301. The source SObj is positioned around (10,10) and scaled by 1.004 at decomp/.../gr/grwallpaper.c:132-159.

The DS implementation is empty:

void grWallpaperMakeDecideKind(void)
{
}

at src/port/reloc_backend_compat_shims.c:12531-12533.

Therefore, the dark navy clear behind the tree is expected. It is not caused by an incorrect Dream Land model.

Fighter shadows
GObj *ftShadowMakeShadow(GObj *fighter_gobj)
{
    return NULL;
}

at src/port/reloc_backend_compat_shims.c:12228-12232.

The lack of grounding/contact beneath Mario and Fox is therefore confirmed.

Dream Land particles and wind presentation

The particle bank is currently proof-oriented at src/port/reloc_backend_compat_shims.c:12112-12140, while particle creation, transforms, processing, and ejection return NULL or do nothing at 12234-12268.

Whispy’s wind velocity push is also a counter-only shim at src/port/reloc_backend_compat_shims.c:12043-12049.

This removes leaves, dust/wind effects, and associated presentation. It does not explain the malformed static Whispy face; the face is a map/material issue.

8. Camera/framing remains unproven

The DS image appears more cropped and enlarged than the N64 reference, but the comparison is not controlled:

Different fighters.
Different world positions.
Different timer/frame.
Potentially different dynamic battle-camera zoom.
The N64 image includes HUD and wallpaper, which strongly changes perceived framing.

The adapter can build matrices from the source CObj at src/port/reloc_backend_renderer_dl.c:794-960. It uses a default battle camera only when no source camera matrix was recovered at 1022-1033.

Do not compensate by changing the FOV or adding an arbitrary stage scale yet. First capture the same deterministic frame on both systems and record:

eye / at / up
fovy / aspect
near / far / scale
viewport
fighter world positions
projected screen positions of fixed stage landmarks

The N64 viewport is explicitly established as (10,10)..(310,230) at decomp/.../sc/sccommon/scvsbattle.c:155-158. A DS mapping should account for that logical viewport rather than simply forcing the scene to fill 256×192.

Fastest isolation sequence
Create one deterministic same-state capture. Freeze both fighters, battle camera, Whispy animation, and stage animation on a known frame. The current N64 and DS images are not suitable for exact projection comparisons.
Run Dream Land stage-only with textures disabled. Give each DObj or DL event a unique flat color. This immediately separates geometry/matrix/triangle loss from texture/material errors. Pay special attention to the purple star and the two ponds.
Capture head 0 and head 1 separately. Then capture the restored source order. This identifies objects being rendered through the wrong opaque/translucent path.
Run a real-depth versus reversed-order test. Reverse opaque DObj or triangle submission while preserving source matrices. Any changed opaque overlap is evidence that submission order is still substituting for Z.
Render fighters in two fixed poses. Use one ordinary neutral pose and one motion known to enable anim locks. Use flat per-part colors before enabling materials, fog, and textures.
Recommended repair order
Restore source-consistent depth. Do this before attempting to tune fighter transforms or manually reorder individual body parts.
Normalize all Dream Land MObjSub records at the relocation boundary. Add exact assertions for Whispy eyes and mouth before drawing.
Preserve stage DL-head identity and source layer preambles. Keep opaque and translucent state streams separate across the entire camera pass.
Implement exact LOADBLOCK source addressing and DXT behavior, then mask/shift and padding-safe repetition.
Expand the fighter display contract to preserve semantic cycle, render, fog, alpha, and depth state.
Implement the source anim-lock matrix branch.
Add contact shadows, wallpaper composition, and Dream Land particles after the core 3D frame is correct.

Performance caching should follow correctness. Caching the currently flattened or synthetic state risks making the wrong behavior harder to remove.

Visual acceptance gate

A Dream Land visual proof should require all of the following, not merely triangle counts:

Whispy has two correctly positioned eyes and a correctly positioned animated mouth.
Both small foreground ponds are bounded and proportioned like the source; no giant cyan/white foreground shape remains.
Canopy stars have complete silhouettes and transparent surroundings.
Left and right bushes contain correctly masked flower details rather than rectangular color patches.
Front and back flower objects are visible and animate through valid frames.
Platforms, tree trunks, foliage, and island body have correct opaque/translucent overlap.
Mario and Fox each form a connected, plausible silhouette in a fixed pose.
Limbs correctly pass in front of or behind the torso according to source depth.
An anim-lock pose does not separate or stretch joints.
Contact shadows are visible beneath both fighters.
The Dream Land sky/wallpaper is present without altering the 3D camera.
No stage or fighter result depends on opaque triangle submission order.

The current broad screenshot gate only checks green/detail/fighter-region coverage at scripts/verify-battle-playable-realtime-harness.ps1:26-67. The stage harness checks that expected GObjs, DObjs, and MObjs exist at scripts/verify-battle-pupupu-stage-harness.ps1:61-65. The hardware harness similarly proves submissions, textures, counts, and zero rejects at scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1:449-458. All of those can pass while Whispy, the ponds, stars, and fighter self-occlusion remain visibly wrong.