Read-only Dream Land / fighter audit handoff

Reviewed the Lean snapshot dated 20260710_155855 (SNAPSHOT_MANIFEST.txt:1-12). Git metadata is excluded (SNAPSHOT_MANIFEST.txt:9,24), so this is an exact source-state audit, not a tracked-diff audit.

No files were changed or committed.

Highest-priority conclusions
Priority	Confirmed issue	Smallest useful repair
P0	Dream Land texture origin is scaled when it should be subtracted after scaling	Fix ndsRendererHardwareTexCoord() and add one exact asset fixture
P0	All ongoing AObjEvent32 readers use host bitfields against N64 MSB-first words	Decode opcode/flags/payload from the raw u32 in the central parsers
P0	Fighter high-bit DObj descriptors bypass gcDecideDObj3TransformsKind()	Restore the original high-bit branch in the port setup function
P1	POT upload space does not reproduce N64 mask/shift/clamp/repeat semantics	Add one logical-address helper and populate padded texels through it
P1	Wallpaper/foreground layers are committed directly to visible VRAM and partial layers count as complete	Retain the previous complete layer and commit a finished page at VBlank
P1	Stage DL head identity is discarded and head 0/head 1 are flattened per DObj	Queue the camera pass and replay source head order
P1	Generated segment-0xE commands contain an extra unflagged palette seed and unchecked frame indices	Proof-gate the seed and bounds-check texture/palette lookup
P1	BGM refill is polled once per slow render frame	Separate boundary consumption from file reads with a prepared-half queue
P2	Pixel gates mostly test broad color presence	Add fixed stage-only native crops with reference and smear/flash metrics
P2	Durable portability rules remain incident-specific	Add explicit bitfield, word-lane, units, state-stream, and pixel-proof rules

Three guardrails should prevent wasted work:

The exact five-record Dream Land MObjSub lane-normalization experiment changed 0/49152 pixels and was reverted. It remains data debt, but it is not the active explanation for the visible smearing (docs/PORTING.md:18586-18588, docs/KNOWN_ISSUES.md:1076-1081).
Projected source Z and the no-Z unit domain were already repaired (docs/PORTING.md:18595-18615). The remaining depth finding below is about source draw classes and DL-head order, not redoing projection.
Segment-0xE branch-table order appears faithful. The concrete material divergences are the unflagged palette command and unchecked indices, not the table index itself.
1. Stage smearing: first incorrect value is the tile-origin calculation
Traced Dream Land surface

A useful fixture is the right-side animated layer-1 surface whose material is declared at:

decomp/BattleShip-main/decomp/src/relocData/104_StagePupupuFile2.c:651-679
display list declaration at 104_StagePupupuFile2.c:726-729
DObj placement at 104_StagePupupuFile2.c:731-738
material-animation table at 104_StagePupupuFile2.c:743-751

Its initial material values include:

fmt                  CI
logical dimensions   32 × 64
trau                 0.0066
scau                 1.25
scav                 2.0
flags                0x006B
block format         CI4
block_dxt            0x20

The original material generator calculates the render-tile origin at decomp/BattleShip-main/decomp/src/sys/objdisplay.c:1432-1477 and the gSPTexture scale at objdisplay.c:1478-1499.

For this material:

uls = (((32 × 0.0066) + 64) / 1.25) × 4
    = 205.475... → 205 in N64 10.2 tile units

scaleS = (2097152 / 32) / 1.25
       = 52428.8 → 0xCCCC in gSPTexture 0.16 units

The port generator closely reproduces those two calculations at:

tile origin: src/port/reloc_backend_renderer_dl.c:3296-3340
texture scale: src/port/reloc_backend_renderer_dl.c:3342-3371

The renderer then correctly records:

gSPTexture S/T scales at src/nds/nds_renderer.c:690-705
tile mask and shift fields at src/nds/nds_renderer.c:789-840
LOADBLOCK ULS/ULT/LRS/DXT at src/nds/nds_renderer.c:850-866
tile-size ULS/ULT at src/nds/nds_renderer.c:904-943

The staged payload contains the relevant vertices at:

decomp/BattleShip-main/decomp/assets/us/relocData/104.vpk0.bin @ 0x2168

Two useful S coordinates are:

vertex 5: S = 2048
vertex 3: S = 3072

These are N64 vertex S/T values in 10.5 units.

First incorrect operation

The DS coordinate helper is:

static s16 ndsRendererHardwareTexCoord(s16 coord, u32 scale, u32 origin,
                                       s32 offset)
{
    s64 relative = (s64)coord - ((s64)origin << 3);

    return (s16)(((relative * (s64)scale) >> 17) + offset);
}

at src/nds/nds_renderer.c:2310-2316. It is called with the recorded tile ULS/ULT at nds_renderer.c:3947-3987 and nds_renderer.c:4272-4289.

This subtracts the tile origin in vertex 10.5 units and then applies gSPTexture scaling to both the vertex coordinate and the tile origin. The source pipeline scales vertex S/T first; the RDP tile origin remains in its own 10.2 domain and is subtracted afterward.

The required DS t16 conversion is:

scaled vertex t16 = (vertex_ST_10.5 × gSP_scale_0.16) >> 17
tile origin t16   = tile_origin_10.2 << 2
result             = scaled vertex t16 - tile origin t16 + offset

For vertex 5:

current:
    ((2048 - (205 << 3)) × 0xCCCC) >> 17
    = 163 t16
    = 10.1875 texels

source-equivalent:
    ((2048 × 0xCCCC) >> 17) - (205 << 2)
    = 819 - 820
    = -1 t16
    ≈ the left tile edge

For vertex 3:

current: 572 t16 = 35.75 texels
correct: 408 t16 = 25.5 texels

Every S coordinate on this surface receives approximately a +10.25 texel offset. That is enough to turn the right half into repeated or unrelated texture content.

This fixture is already 32×64, so POT expansion is not involved in its first failure. Likewise, LOADBLOCK fields have already been decoded before this point. For the initial material state, the first provably incorrect value is therefore produced by ndsRendererHardwareTexCoord() at src/nds/nds_renderer.c:2313-2315.

Ongoing AObjEvent32 corruption can independently alter trau, scau, or the selected texture frame later. It does not invalidate the static initial-state trace above.

Suggested minimal change

Change only the order and units in the helper:

s64 scaled_t16 = ((s64)coord * (s64)scale) >> 17;
s64 origin_t16 = (s64)origin << 2;

return (s16)(scaled_t16 - origin_t16 + offset);

Add one exact fixture using this asset:

origin ULS = 205
scale S    = 0xCCCC
vertex S   = 2048 → expected approximately -1 t16
vertex S   = 3072 → expected 408 t16

Do not combine this change with mask/shift, LOADBLOCK, or animation repairs. The fixture should make the first boundary independently provable.

2. POT padding: coordinates can sample uninitialized logical space

The hardware uploader expands dimensions to a DS power of two at src/nds/nds_renderer.c:2649-2658 and src/nds/nds_renderer.c:3482-3486.

Its DS texture flags reproduce only broad clamp/wrap and mirror choices:

S wrap/clamp and mirror: src/nds/nds_renderer.c:2660-2680
T wrap/clamp and mirror: src/nds/nds_renderer.c:2681-2689

The N64 masks, maskt, shifts, and shiftt values are retained in the cache key at src/nds/nds_renderer.c:3540-3572, but they do not alter the submitted texture coordinate or upload contents.

The upload buffer is cleared for the full POT dimensions, but only the logical width × height rectangle is decoded into it at src/nds/nds_renderer.c:3648-3668. The complete POT image is then uploaded and recorded as the active profile dimensions at src/nds/nds_renderer.c:3690-3709.

This creates three incorrect cases:

N64 mode	Current DS result
Clamp	Coordinates beyond the logical edge can reach zero-filled padding before the DS POT edge clamps
Repeat	Repetition occurs at the DS POT width/height, not necessarily the N64 1 << mask period
Mirror	Mirroring occurs across the DS POT extent rather than the N64 mask period

The diagnostic sampler also wraps or clamps against the POT profile dimensions at src/nds/nds_renderer.c:3229-3305, so a sample in zero padding can be counted as a valid in-range sample.

There is a second source-layout issue in the same path. LOADTILE uses the source image origin and SETTIMG width, but LOADBLOCK forces:

source_origin_s = 0;
source_origin_t = 0;
source_width = width;

at src/nds/nds_renderer.c:3494-3506. The recorded DXT is included in the cache key but is not used to reconstruct row stepping.

Suggested minimal change

After the texcoord-order fix:

Keep separate fields for:
logical texture dimensions
N64 mask period
DS upload dimensions
Add one N64 address helper that applies:
shift
mask period
mirror
clamp
Populate every POT texel through that helper:
clamp: copy the nearest logical edge texel
repeat: repeat the N64 period
mirror: mirror the N64 period
Make the profile sampler use the same helper rather than the POT dimensions.
For LOADBLOCK, derive source rows from SETTIMG width and recorded DXT instead of setting source_width = width.

A temporary edge-fill-only patch can reduce clamp seams, but it should not be treated as the repeat/mirror fix.

3. Whispy/tree face: ongoing AObjEvent32 decoding is globally wrong
Root defect

AObjEvent32 is represented with host C bitfields at:

u32 opcode : 7;
u32 flags : 10;
u32 payload : 15;

in decomp/BattleShip-main/decomp/src/sys/objtypes.h:94-108.

The source encoding is explicitly documented as N64/IDO MSB-first:

(opcode << 25) | (flags << 15) | payload

at decomp/BattleShip-main/decomp/src/sys/objdef.h:271-284.

The original translation unit is included unchanged by src/import/battleship_sys_objanim.c:1-4. Consequently, ARM GCC reads the low bits as opcode, which is not the source representation.

Complete ongoing parser surface

The relevant attachment and execution paths are:

Role	Entry points
Attach one DObj script	gcAddDObjAnimJoint(), objanim.c:137-149
Attach one MObj script	gcAddMObjMatAnimJoint(), objanim.c:151-163
Attach a DObj tree	gcAddAnimJointAll(), objanim.c:165-188
Attach material scripts	gcAddMatAnimJointAll(), objanim.c:190-219
Attach both	gcAddAnimAll(), objanim.c:221-266
Parse DObj commands	gcParseDObjAnimJoint(), objanim.c:268-609
Parse MObj commands	gcParseMObjMatAnimJoint(), objanim.c:830-1210
Per-frame DObj/MObj traversal	gcPlayAnimAll(), objanim.c:1428-1455
Parse camera commands	gcParseCObjCamAnimJoint(), objanim.c:2488-2812
Per-frame camera traversal	gcPlayCamAnim(), objanim.c:2915-2921
Scan script duration	gcGetAnimTotalLength(), objanim.c:2923-3027

The first direct defective reads are:

DObj opcode/flags/payload: objanim.c:328-335
MObj opcode/flags/payload: objanim.c:896-903
CObj opcode/flags/payload: objanim.c:2547-2554
duration scanner: objanim.c:2938-3002

Fixing only gcPlayAnimAll() would not be broad enough because several systems call the parsers directly, including:

fighter parameter paths: decomp/.../ft/ftparam.c:388-424
fighter animation path: decomp/.../ft/ftanim.c:411
moving collision objects: decomp/.../mp/mpcollision.c:3700-3736
the current port layer-1 bridge: src/port/reloc_backend_compat_shims.c:13007-13091
Dream Land-specific callers

Geometry layers 0, 2, and 3 use gcPlayAnimAll; layer 1 uses mpCollisionPlayYakumonoAnim:

layer descriptors: decomp/.../gr/grdisplay.c:10-42
script attachment and initial playback: grdisplay.c:182-218

The layer-1 DS bridge directly parses every DObj and MObj each update:

DObj parser calls: src/port/reloc_backend_compat_shims.c:13038-13040, 13081-13082
MObj parser calls: src/port/reloc_backend_compat_shims.c:13085-13090

Whispy dynamically replaces and immediately plays four script sets:

eyes DObj/MObj scripts: decomp/.../gr/grcommon/grpupupu.c:567-582
mouth DObj/MObj scripts: grpupupu.c:584-597
back flower/mouth texture DObj scripts: grpupupu.c:599-610
front flower/eyes texture DObj scripts: grpupupu.c:612-624

All four map GObjs then retain gcPlayAnimAll as an ongoing process:

process attachment: grpupupu.c:637-659
eyes, mouth, rear flowers, front flowers creation: grpupupu.c:663-669

This is therefore directly relevant to the malformed or frozen tree face, not merely a fighter costume issue.

Why the current workaround cannot be extended safely

The current costume-only bridge:

recognizes the endian/bitfield problem at src/port/reloc_backend_compat_shims.c:7809-7814
is limited to 64 commands at compat_shims.c:7814-7821
manually decodes the raw fields at compat_shims.c:7862-7865
accepts only a selected linear opcode set at compat_shims.c:7867-7904
rewrites command words in place at compat_shims.c:7916-7926
restores them at compat_shims.c:7934-7947
is invoked only around the one-shot costume parse at compat_shims.c:7971-7988

Dream Land scripts contain genuine self-loops. One example begins at 104_StagePupupuFile2.c:813-815 and jumps back to itself at 104_StagePupupuFile2.c:870-871. Repacking only a bounded linear stream is unsafe for jumps, shared scripts, repeated playback, and concurrent users.

Suggested minimal change

Decode fields from the raw word in the central readers:

opcode  = (event->u >> 25) & 0x7F;
flags   = (event->u >> 15) & 0x3FF;
payload = event->u & 0x7FFF;

Use these accessors in:

the DObj parser
the MObj parser
the CObj parser
gcGetAnimTotalLength()

Do not mutate script storage. Do not build another stage-specific or Whispy-specific wrapper.

The focused regression fixture should use the self-looping script at 104_StagePupupuFile2.c:813-871 and prove across multiple frames:

opcode sequence
loop target
trau
scrollu
lfrac
current/next texture IDs

Once every reader uses the raw accessors, the costume-only repack can be removed.

4. Background black flashes: source-bracketed pass versus visible single-buffer updates
Original Dream Land wallpaper contract

Dream Land takes the default common wallpaper path:

battle scene creates the clear, wallpaper, and battle cameras in sequence at decomp/.../sc/sccommon/scvsbattle.c:152-159
grWallpaperMakeDecideKind() selects grWallpaperMakeCommon() by default at decomp/.../gr/grwallpaper.c:267-300
the common wallpaper SObj is created on display link 0 at grwallpaper.c:132-159
its camera runs at priority 80 and captures display link 0 at decomp/.../gm/gmcamera.c:1209-1237

The common Dream Land path does not use dGRWallpaperDisplayList; that fill display list is used only by Bonus 3 at grwallpaper.c:250-264.

Instead, the source sprite-camera pass brackets state explicitly:

pipe sync, one-cycle mode, alpha threshold, texture perspective off, bilerp, LUT state, and opaque render mode: decomp/.../lb/lbcommon.c:2897-2915
camera capture of the complete link-0 pass: lbcommon.c:2948-2977
combine, alpha compare, texture perspective, render mode, and LUT restoration: lbcommon.c:2928-2945

The source therefore treats the wallpaper as one complete camera pass with explicit state setup and restoration. It does not expose a half-rendered wallpaper to scanout.

Current DS behavior

BG priorities and blending are initialized at:

BG3 foreground priority 0
BG0 3D priority 1
BG2 wallpaper priority 2
BG0/BG2 alpha blend

in src/nds/nds_platform.c:121-150.

Overlay enablement also changes the GX clear alpha between opaque and transparent at nds_platform.c:444-458.

The compositor has several partial-frame hazards:

A layer is considered committable as soon as one SObj draw succeeds:

src/port/sprite_preview_backend.c:760-768

The wallpaper/foreground split is inferred from GObj identity or Results display-link ID rather than being driven by the source camera pass:

sprite_preview_backend.c:770-790

Every frame clears visible BG3 before the replacement foreground has been rendered:

sprite_preview_backend.c:807-814
src/nds/nds_platform.c:428-438

Layer commits write rows directly into the displayed BG bitmap:

nds_platform.c:392-419

A non-full-screen layer still clears the entire visible 256×256 bitmap before copying its rows:

nds_platform.c:405-417

The project already removed the old full-screen preclear before row-copy (docs/PORTING.md:18611-18613), but the visible BG3 begin-frame clear and direct visible row updates remain.

Thus, black flashes can occur from:

scanout between visible-layer clear and replacement
scanout during the row-by-row copy
a layer with one successful SObj but missing later SObjs
a frame where wallpaper creation/draw fails but the compositor still changes another layer
mismatched GX clear-alpha or blend state inherited across scene transitions

draw_count != 0 is not a completeness proof.

Suggested minimal change
Stop clearing the visible BG3 page in ndsSObjPreviewBeginFrame().
Compose each layer entirely off-screen.
Mark it complete only after the source camera/display-link pass finishes without a decode failure.
At VBlank, either:
switch to a hidden BG page, or
DMA the complete native layer from RAM in one commit window.
If the current frame is incomplete, retain the previous valid wallpaper or foreground layer.
Save and restore scene-scoped:
BG priorities
REG_BLDCNT
REG_BLDALPHA
GX clear alpha

The minimal diagnostic is a counter for:

layer_started
layer_expected_sobjs
layer_drawn_sobjs
layer_failed_sobjs
layer_committed
layer_retained_previous

That will distinguish a compositor flash from a GX no-submit frame.

5. Fighter fragments: the port drops BattleShip’s high-bit transform path

The source fighter-parts constructor tests the high bit of each DObjDesc::id:

if (dobjdesc->id & 0x8000)
{
    gcDecideDObj3TransformsKind(current_dobj, tk1, tk2, tk3, 0x8000);
}
else
{
    lbCommonInitDObj(current_dobj, tk1, tk2, tk3, arg9);
}

at decomp/BattleShip-main/decomp/src/lb/lbcommon.c:1067-1071.

The same contract is retained for custom trees at lbcommon.c:1134-1138.

gcDecideDObj3TransformsKind() does more than add the literal requested transform kinds. It selects recalculating rotation/scale kinds according to the authored transform combination:

transform analysis: decomp/.../sys/objanim.c:2224-2287
recalc/special-kind selection: objanim.c:2288-2328

The port-owned fighter setup always calls:

lbCommonInitDObj(current_dobj, tk1, tk2, tk3, arg9);

at src/port/reloc_backend_compat_shims.c:7771, irrespective of the high bit.

lbCommonInitDObj() merely adds the literal kinds and initializes transform values at compat_shims.c:7648-7675.

This is a confirmed source divergence. High-bit-authored fighter parts lose their recalc transform behavior and can separate from the body even with correct source depth.

Suggested minimal change

Restore the original branch exactly:

if ((dobjdesc->id & 0x8000) != 0)
{
    gcDecideDObj3TransformsKind(current_dobj, tk1, tk2, tk3, 0x8000);
}
else
{
    lbCommonInitDObj(current_dobj, tk1, tk2, tk3, arg9);
}

Apply the same check to any port-owned custom-tree constructor that currently bypasses gcDecideDObj3TransformsKind().

The fixture should record, for Mario and Fox:

number of selected DObj descriptors with 0x8000
XObj kind sequence created for each
source-versus-port sequence equality

This repair should land before manual fighter-part offsets or pose-specific corrections.

6. Material parity: branch order is sound; palette state and index safety are not
Original segment-0xE behavior

gcDrawMObjForDObj():

installs segment 0xE
counts MObjs
creates one branch-table entry per MObj
emits branches in MObj linked-list order

at decomp/.../sys/objdisplay.c:1204-1259.

Palette loading occurs only when MOBJ_FLAG_PALETTE is set:

objdisplay.c:1261-1287

Texture-frame selection under MOBJ_FLAG_FRAC is:

objdisplay.c:1297-1315

The next and current texture image commands are generated at:

next frame / LOADBLOCK: objdisplay.c:1352-1419
current frame: objdisplay.c:1421-1430

Tile size and gSPTexture commands follow at:

objdisplay.c:1432-1477
objdisplay.c:1478-1499
Port comparison

The current branch-table construction also walks the MObj linked list once and associates each table entry with the corresponding emitted branch:

src/port/reloc_backend_renderer_dl.c:3378-3430

That ordering appears source-faithful.

The first concrete command-stream divergence occurs before the flagged palette path:

if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
    (mobj->sub.palettes != NULL))
{
    ...
    ndsRendererAdapterEmitTextureImage(... palette);
}

at reloc_backend_renderer_dl.c:3175-3186.

The command counter explicitly reserves space for this extra command at reloc_backend_renderer_dl.c:2749-2758.

The source emits no corresponding SETTIMG when MOBJ_FLAG_PALETTE is clear. This extra command can mutate inherited texture-image state without a matching TLUT load, particularly when the following source DL is partial and expects prior state.

The project documents this as an intentional temporary compatibility path at docs/KNOWN_ISSUES.md:1066-1068; it should therefore be removed only after a semantic command proof, not casually.

A second confirmed problem is index safety:

return items[index];

has only null and negative-index checks at reloc_backend_renderer_dl.c:2895-2903. There is no upper bound for:

palette_id
texture_id_curr
texture_id_next

The current CI uploader properly rejects missing or undersized TLUT state at src/nds/nds_renderer.c:3613-3643, so an incorrect frame or palette ID can become a missing texture, unrelated palette, or out-of-range pointer.

The FRAC update itself matches the source closely at reloc_backend_renderer_dl.c:3224-3240. If lfrac or texture IDs are wrong at runtime, the ongoing AObjEvent32 defect is the earlier cause.

Suggested minimal changes
Add semantic command fixtures for:
no-palette material
CI4 with a palette and TLUT load
two-frame FRAC material
multiple MObjs and branch indices
Compare command opcode and parameters, excluding pointer addresses.
Once the no-palette fixture proves the source stream, remove or narrowly gate reloc_backend_renderer_dl.c:3175-3186.
Store validated sprite and palette counts in relocation-side metadata and bounds-check every MObj index before items[index].
On an invalid index, reject that material deterministically and report:
asset ID
MObj source offset
requested index
validated count

Do not rewrite branch-table ordering unless a fixture actually shows a mismatch.

7. Depth classes: flattening source DL heads changes floor/tree behavior

The source stage layer definitions are:

layer 0: link 4, no Z
layer 1: link 6, Z-buffered
layer 2: link 13, no Z
layer 3: link 17, no Z

at decomp/.../gr/grdisplay.c:10-42.

Their source display callbacks establish distinct opaque and translucent preambles:

layer 0 no-Z opaque/head 0 and XLU/head 1: grdisplay.c:52-83
layer 1 Z opaque/head 0 and Z XLU/head 1: grdisplay.c:86-108
layer 2 no-Z opaque/head 0 and XLU/head 1: grdisplay.c:111-131
layer 3 no-Z opaque/head 0 and XLU/head 1: grdisplay.c:133-155

gcDrawDObjTreeDLLinksForGObj() appends each DObjDLLink to its global gSYTaskmanDLHeads[list_id], retaining segment state and source head separation across the pass:

decomp/.../sys/objdisplay.c:1695-1760

The current stage adapter validates DObjDLLink::list_id, but submits the display list immediately without passing the ID to the renderer:

src/port/reloc_backend_renderer_dl.c:3621-3674

The movement path performs those submissions inline during each DObj traversal:

src/port/reloc_backend_movement.c:12863-12885

It begins and ends persistent stage state separately for every stage GObj:

reloc_backend_movement.c:12995-13007

This changes the ordering from:

all source head-0 opaque commands for the camera pass
then all source head-1 translucent commands

to approximately:

DObj A head 0
DObj A head 1
DObj B head 0
DObj B head 1
...

The DS renderer then infers decal behavior from combiner state at src/nds/nds_renderer.c:1884-1911, derives polygon state at nds_renderer.c:2201-2224, and flushes with manual translucent sorting at src/nds/nds_platform.c:1042-1058.

Consequences include:

a translucent canopy or flower plane being submitted before later opaque geometry
DObj-local state resets replacing source camera-pass inheritance
source no-Z overlays being confused with normal Z geometry
floor/tree decals classified by inferred combiner output rather than source render mode
head-1 order no longer matching BattleShip

This is distinct from the corrected source projected-Z path.

Suggested minimal change

Queue stage submissions for the complete camera pass with:

source layer
DObjDLLink list_id
GObj sequence
DObj sequence
display list
source Z/no-Z preamble
observed render/Z mode

Replay in this order:

source opaque head 0, preserving layer Z/no-Z setup
source decal state where explicitly selected by the source command stream
source translucent head 1 in source order

Begin persistent RSP/RDP state once per camera/head stream rather than once per GObj.

A strong regression is:

reverse opaque head-0 DObj submission order
the rendered Z-buffered opaque image must remain unchanged
restore source order for translucent head 1 and require a fixed crop match
8. Audio breakup: current refill cadence is render-bound

The stream is:

22,050 Hz
mono PCM16
65,536-byte ring
two 32,768-byte halves
44,100 bytes per second

at include/nds/nds_audio_bgm.h:8-13.

The half-buffer deadline is:

32768 / 44100 = 0.7430 seconds

The timer calculation is encoded at src/nds/nds_audio_bgm.c:9-15.

At roughly 1.2 fps, a render takes approximately:

1 / 1.2 = 0.8333 seconds

which is already longer than one half-buffer interval.

The refill performs synchronous fread() into the active ring storage at src/nds/nds_audio_bgm.c:124-163. It only checks whether the selected write half equals the currently calculated playback half at nds_audio_bgm.c:198-217.

If polling arrives one full half late, it seeks and resynchronizes at:

overrun detection: nds_audio_bgm.c:423-441
seek/refill resync: nds_audio_bgm.c:219-238

The poll occurs once during the gameplay update, before the long render:

src/port/taskman_seam.c:4105-4113

The render and presentation happen much later at taskman_seam.c:4262-4278. Cooperative OS threads are not resumed until after rendering and VBlank at taskman_seam.c:4276-4278 and src/port/libultra_os.c:233-252.

Creating another cooperative “audio thread” would therefore remain render-bound.

Suggested minimal render-independent path

Use a producer/consumer design without changing the PCM asset:

Maintain a queue of at least four prepared 32 KiB halves in main RAM.
Main/file-side code performs fread() only into free queue entries. It should fill all available entries before entering another expensive render.
A hardware timer boundary handler must not call fread(). It should only:
advance the consumed-half counter
select a prepared queue entry
start a fixed DMA copy or hand the prepared buffer to the audio owner
record an underrun if no entry is ready
Prefer having ARM7 own the playback-boundary service and accept prepared buffer addresses/indices over FIFO. ARM9 remains the file producer.
Keep playback progression based on the hardware timer, not rendered frame count.

Add these diagnostics:

minimum prepared queue depth
underrun count
maximum boundary lateness
maximum file-read duration
number of resync seeks

An interim call to ndsAudioBgmUpdate() at several renderer safe points may reduce symptoms, but it is not render-independent and should not be considered the completed architecture.

9. Pixel gates: proposed native stage-only crops

The current realtime verifier defines broad regions for bushes, fighter center, and stage body at scripts/verify-battle-playable-realtime-harness.ps1:26-31.

It checks broad green/detail/fighter-colored fractions at verify-battle-playable-realtime-harness.ps1:54-81.

assert-melonds-top-visible.ps1 can already calculate named-region measurements at scripts/assert-melonds-top-visible.ps1:63-128, but named regions are only printed at assert-melonds-top-visible.ps1:285-290; they do not have independent pass/fail thresholds.

The current adjacent-frame checks are global at assert-melonds-top-visible.ps1:261-283, so a moving fighter can hide a wallpaper flash or stage smear.

Capture window geometry is stabilized at scripts/capture-melonds.ps1:137-163, but the gate should consume the extracted native 256×192 top screen, not emulator-window coordinates.

Proposed fixed native crops

These are starting crop definitions for a frozen Dream Land stage-only frame and should be calibrated once against an accepted native reference:

Crop	Native rectangle
Wallpaper upper-left	x=0, y=0, w=64, h=48
Wallpaper upper-right	x=192, y=0, w=64, h=48
Left bush	x=34, y=70, w=59, h=52
Tree face	x=103, y=52, w=56, h=68
Right bush	x=199, y=70, w=57, h=52
Floor/path left	x=36, y=128, w=53, h=30
Floor/path right	x=168, y=128, w=55, h=30

Fighters should be excluded structurally by using a stage-only/frozen harness, not by trying to classify and mask fighter colors.

Stable metrics

For each crop, use a frozen accepted-reference crop and calculate:

Normalized mean absolute RGB error or SSIM.
Meaningful residual fraction, where max-channel delta exceeds 25.
Largest connected residual component, which catches one large smear while tolerating animation noise.

Add crop-specific metrics:

Wallpaper:
near-black pixel fraction
largest near-black connected component
adjacent-frame meaningful delta
Floor/path and bushes:
longest horizontal residual run
horizontal-to-vertical edge-energy ratio
largest saturated pink/cyan residual component
Tree face:
reference similarity
connected-component/topology check for two eye regions and one mouth region
bounded face-region centroid movement across adjacent frames

Exact image hashes are too fragile unless camera, Whispy state, stage animation, and all random sources are frozen.

Suggested minimal script change

Add a stage-only assertion script, or extend the existing image script so each named region can receive:

reference crop
maximum mean delta
maximum changed fraction
maximum residual component
maximum black fraction

Make named regions assert rather than merely print. Retain the current broad gate as a smoke test, but do not treat it as texture or topology proof.

10. Project learnings: durable rules missing from the architecture

The current documentation records the incidents:

costume AObjEvent32 case: docs/ARCHITECTURE.md:1098-1104
arbitrary-PC counters can observe frame-start reset: ARCHITECTURE.md:1115-1119
stage material/head/fighter boundaries: ARCHITECTURE.md:1121-1127
blanket word swap and selected mixed-width normalizers: docs/KNOWN_ISSUES.md:1006-1034
current pixel, AObj, texture, head, and audio debt: KNOWN_ISSUES.md:1047-1095
historical texture/depth/AObj experiments: docs/PORTING.md:18563-18634

Those sections are project-status descriptions rather than reusable portability rules.

Suggested minimal documentation addition

Add one small Cross-platform binary and rendering invariants section to ARCHITECTURE.md, with backlinks from KNOWN_ISSUES.md.

Use rules with this level of specificity:

1. N64 packed bitfields

Persistent N64/O2R words are decoded by documented source bit positions. Host C bitfield layout is never used for serialized data.

Include the AObjEvent32 formula as the canonical example.

2. O2R word lanes

A blanket 32-bit byte swap fixes word byte order only. Structures containing mixed u16 and u8 lanes require a schema-aware repair exactly once per loaded-file generation, with range validation and an idempotence marker.

This prevents treating word swapping and structure normalization as equivalent.

3. Source-versus-target units ledger

Every adapter equation names the source unit, target unit, and conversion order.

The immediate texture example should list:

Vtx S/T       N64 signed 10.5
tile ULS/ULT  N64 10.2
gSPTexture    N64 0.16
DS texcoord   t16, 1/16 texel
clip/depth    signed fixed-point domain explicitly named

The order of conversions is part of the contract, not an implementation detail.

4. CPU counters are not pixel proof

Call counts, triangle counts, zero oracle mismatches, and valid asset pointers prove execution or arithmetic only. They do not prove framebuffer ordering, texture addressing, alpha, palette, or visual topology.

Native framebuffer crops or equivalent pixel evidence are required for visual acceptance.

5. Source state streams are not self-contained DObjs

Partial N64 display lists inherit RSP/RDP state. They may be flattened or cached only after camera-pass ordering, DL-head identity, and state inheritance are proven equivalent.

This rule covers the current stage-head, palette-seed, and wallpaper-partial-layer problems.

Suggested landing order for the sole writer
Fix ndsRendererHardwareTexCoord() and land its exact Dream Land fixture.
Replace host AObjEvent32 bitfield reads with raw-word accessors and land the self-looping stage fixture.
Restore the high-bit fighter DObj transform branch and verify Mario/Fox XObj kinds.
Make wallpaper/foreground commits complete and non-visible until VBlank.
Add N64 mask/shift/address handling and populate POT padding correctly.
Preserve camera-wide stage head 0/head 1 ordering.
Add material semantic-command fixtures, index bounds, then proof-gate the unflagged palette seed.
Move BGM boundary consumption to a timer-driven prepared-buffer queue.
Add the stage-only native crop gates.
Record the five durable portability rules before caching or performance work makes the current assumptions harder to remove.