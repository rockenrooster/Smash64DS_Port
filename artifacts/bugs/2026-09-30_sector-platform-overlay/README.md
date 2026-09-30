# Sector Z: the orange "platform overlay" on the Arwing (2026-09-30)

Owner (r58): "time to remove the arwing visual platform debug overlay ... it's
orange ... it also gets left behind when the arwing does a barrel roll ... I
think it's the platform collision?"

## What it was

Stage layer 1's yakumono DObj 1 -- the Arwing's wing-platform collision proxy,
which `grSectorArwingUpdateCollisions` (grsector.c:991-1019) moves to the
Arwing every tick with `mpCollisionSetYakumonoPosID`. The source draws stage
layer 1 with `grDisplayLayer1SecProcDisplay` (grdisplay.c:98-108), so this
DObj's list is drawn like any layer DObj. Decoded from the O2R payload of file
109 (`decode_platform_dl.py`; `decomp/.../BattleShip_o2r/reloc_extern_data/
ExternDataBank109`): DObjDesc entry 1 -> DLLink 0x85E0 -> list 0x75F0, which
sets light colours (G_MOVEWORD LIGHTCOL b3b3b3 / 808080) and calls 0x7638:
`G_CC_SHADE`-style combine, eight vertices, six triangles -- a flat strip
x -680..626, y -5..168, z +-116. Its vertex RGBA fields are normals
(29,124,0 / 245,127,0 / 226,124,0, alpha 0). The port's stage program draws it
unlit, so the normals read as orange vertex colour, and as a flat collision
strip it never rolls with the Arwing.

(The relocData C file's offsets for file 109 -- DL 0x2EE0, DLLink 0x3ED0 -- do
not match the O2R payload; the payload's own DObjDesc pointer is the truth.)

## Evidence (four-CPU lab ROM, Sector Z, owner roster, presented frame 1700)

- `hid1-f1700.png`: before (the orange strip along the top of the Arwing).
- `yak2-f1700.png`: same ROM, `DOBJ_FLAG_NOTEXTURE` poked onto
  `gMPCollisionYakumonoDObjs->dobjs[1]` at frame 100 through gdb
  (`erpcap2.ps1 -PokeFrame 100`); the strip is gone and nothing else changed
  (100 pixels, all inside the strip's box).
- `plat1-f1700.png`: the fix build; **0 pixels differ from `yak2`**.
- `before-after-crop-f1700.png`: the Arwing, before and after, 4x.

## Fix

`ndsGRSectorPlatformDObj()` (battleship_grsector_ground.c) names that DObj on
Sector Z, and the native stage owner leaves its binding out of the frame
(`hidden_binding_mask`, renderer_adapter_stage.c), the same mask
`DOBJ_FLAG_NOTEXTURE` uses. The DObj, its collision and its flags stay as the
source has them. Removed at the owner's request: the source draws this strip
lit (grey), coplanar with the Arwing's wing.

Found alongside: the ground-actor scan (`ndsStageGCDrawAllLoopScanDObjs`)
ignored `DOBJ_FLAG_HIDDEN` and `DOBJ_FLAG_NOTEXTURE`, so the Arwing's
alternately hidden DObjs 7/9 (grsector.c:457-467) both drew; it now follows
objdisplay.c:1563/1707 (a hidden DObj hides its subtree, siblings still draw).
