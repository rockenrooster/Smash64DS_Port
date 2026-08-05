#!/usr/bin/env python3
"""Pack the original EFCommon particle bank for the DS particle interpreter.

The bank pair is position-independent -- every internal pointer is a
file-relative offset that the source loader pointerises after DMA
(BattleShip lbParticleSetupBankID) -- so the script bank ships byte-identical
and only the textures are re-encoded for DS hardware.

Three things are derived here rather than declared:

* which EFCommon scripts a Dream Land Mario-vs-Fox items-off match can reach,
  from the port's own P1 effect-seam list plus the bytecode spawn graph;
* which textures those scripts name;
* which DS texel format each of those textures needs, chosen by measured
  premultiplied error rather than by a fixed N64->DS table.

Scripts outside the reachable set get NDS_PARTICLE_SCRIPT_UNREACHABLE in the
offset table and textures outside it get sentinel offsets, so an interpreter
that is asked for one fails closed instead of mis-indexing and drawing a
different effect (docs/BUGS.md Coin->Sparkle, Slash->HitNormal).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from collections import Counter
from pathlib import Path

# The census generator moved to scripts/2d_vfx/ in the 2026-07-30 area-folder
# reorganisation, which landed after this branch was cut, so a bare import finds
# nothing and the checker fails with ModuleNotFoundError rather than with
# anything about particle banks. Resolve it by location instead of relying on
# the caller's working directory -- check-nds-particle-banks.ps1 invokes this
# from the repo root, and a future reshuffle should break the path here loudly
# rather than change which module gets imported.
_CENSUS_DIR = Path(__file__).resolve().parent / "2d_vfx"
if str(_CENSUS_DIR) not in sys.path:
    sys.path.insert(0, str(_CENSUS_DIR))

import generate_task39_effect_census as census  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
O2R_PARTICLES = Path("decomp/BattleShip-main/BattleShip_o2r/particles")
EFMANAGER = Path("decomp/BattleShip-main/decomp/src/ef/efmanager.c")

# Same two blobs scripts/generate_task39_hit_sparks.py pins.
SCRIPT_BANK = ("efcommon_particle_scb",
               "4c639924f0c1ce6e4b3d0c5b3d6b49605d237ff7b79816ddd26ff8631ab0eb1d")
TEXTURE_BANK = ("efcommon_particle_txb",
                "8bffc07309693cb79b29f4e4d1faf3fd29cb42a115ccb4ae143d9308480bc860")

# Dream Land's own bank, and the reason BUGS.md's wind row still had an open VFX
# half: `ndsParticleLoadEFCommonBank` covers the common bank and EVERY other
# bank takes `ndsParticleRegisterEmptyBank`, so `grPupupuWhispyLeavesMakeEffect`
# (script 0) and `grPupupuWhispyDustMakeEffect` (script 1) failed closed at
# reject reason 2 before the atlas was ever consulted. A 2026-08-01 both-CPU
# soak caught exactly that pair: `gNdsParticleRejectRing` script 0 and script 1,
# bank 0, reason 2, twice each.
#
# It is small: 416 bytes of bytecode over five scripts, three textures. Scripts
# 0 and 1 both draw TEXTURE 2 -- 16x16 with four frames, 1,024 texels -- and the
# quad sheet has 1,408 texels free after the common set, so Whispy's leaves and
# dust land without touching the 8,192-byte hard bound.
PUPUPU_SCRIPT_BANK = ("grpupupu_particle_scb",
                      "6d7b7769be48e778e1e48db2869a0a8388a7a9fd9795c6c0018429674e125166")
PUPUPU_TEXTURE_BANK = ("grpupupu_particle_txb",
                       "502f9a8eb3142d28e575a367610ac0ea278cf1af072fd221c1c437346e8dd98c")
# The quad frame table is keyed by (texture id, frame), and texture 2 means a
# different image in each bank. Pupupu rows are emitted at this stride so one
# scan still answers both banks and the draw path adds the stride when the
# particle's bank is Dream Land's.
PUPUPU_QUAD_TEXTURE_STRIDE = 64
# The two scripts the wind row needs, named in grpupupu.c:221 and :480. Only
# the textures THESE reference got measured-live standing in the atlas at first
# -- the very first attempt made all three Pupupu textures live and the two that
# scripts 3 and 4 draw (a 32x32 and a 16x16) took the space before the
# four-frame sheet that scripts 0 and 1 actually use, which is the exact failure
# the QUAD_MEASURED_LIVE comment above warns about, reproduced in one commit.
PUPUPU_MEASURED_LIVE_SCRIPTS = frozenset((0, 1))
# ...and then the RUNTIME settled it, which the script closure could not.
# With the bank drawing correctly for the first time (2026-08-01) a both-CPU
# soak reported 3,741 strided draws of which 2,084 missed, at pre-stride ids 0
# and 1 -- so Dream Land draws all three of its textures, not just the one the
# two named scripts reference. Measured beats derived: this set is the live one,
# and it costs the two non-live COMMON textures (3 and 9) their places, neither
# of which any measured match has drawn.
PUPUPU_MEASURED_LIVE_TEXTURES = frozenset((0, 1, 2))

# --------------------------------------------------------------------------
# Source-asset quads -- textures that are NOT in any particle bank
# --------------------------------------------------------------------------
# The shield and the respawn pad are not particles. They are GObj effects the
# port drew as untextured procedural discs, which is why BUGS.md carried
# *"Shield VFX is not correct"* and *"not using correct asset for the revival
# platform"* against them. Both source assets are single small textures sitting
# in ordinary reloc files, and those files are already extracted at build time
# under decomp/.../assets/us/relocData as decompressed .bin -- so the real art
# is reachable here without any new extraction step.
#
# They ride the quad sheet because the sheet is the port's only textured,
# alpha-blended, camera-facing draw, which is exactly what both effects are.
# They are NOT free of consequence and the cell caps below are measured, not
# chosen: at their native 16x32 and 32x16 the shelf packer drops texture 41,
# which IS in QUAD_MEASURED_LIVE -- a silently absent effect, the same class of
# bug as the saturated pools. At the default 16-texel cap both seat with every
# existing row still admitted and 1,344 texels spare, i.e. this addition costs
# the current sheet nothing. If the owner finds 8x16 too coarse the next lever
# is naming non-live common textures to drop, not raising the 8,192-byte bound.
RELOC_ASSET_DIR = Path("decomp/BattleShip-main/decomp/assets/us/relocData")
SOURCE_QUAD_TEXTURE_STRIDE = 128
SOURCE_QUAD_ASSETS = (
    {
        # dFTManagerCommon_Tex_0x0008, relocData/163_FTManagerCommon.c:18.
        # The shield's whole asset is one 4-vertex DL over this texture, so a
        # camera-facing quad IS the source construction, not an approximation
        # of it.
        "name": "SHIELD",
        "file": "163.vpk0.bin",
        "offset": 0x0008,
        "format": "ia8",
        "width": 16,
        "height": 32,
        "symbol": "dFTManagerCommon_Tex_0x0008",
    },
    {
        # THE NAME IS WRONG AND IS KEPT ONLY BECAUSE IT IS ALREADY THE SYMBOL.
        # relocData/85_EFCommonEffects3.c calls 0x2BA8 "Halo glow texture ...
        # I4 32x16, 256 B" and this entry inherited that, but 0x2BA8 is NOT
        # reachable from dEFCommonEffects3_RebirthHalo. Decoded from the file's
        # own relocation chain on 2026-08-04 (artifacts/verification/
        # 2026-08-04-c66-row1-halo-asset-dl.txt): RebirthHalo's DObjDesc @0x2AC0
        # draws 0x2378, 0x2A88 and 0x27E8, whose leaves load images 0x918,
        # 0x9B8 and 0xA40 -- never 0x2BA8. 0x2BA8 sits between RebirthHalo's
        # AnimJoint (0x2B70) and ItemGetSwirl's MObjSub (0x2CA8), and the DLs
        # that do load it (0x2EF0/0x2F80/0x3010/0x30A0) are ItemGetSwirl's.
        # Inert today -- the procedural rebirth stand-in that consumed this
        # atlas entry was deleted on 2026-08-04. Renaming the symbol would
        # rewrite the generated header for a comment, so it stays; read this
        # block, not the name.
        "name": "REBIRTH",
        "file": "85.vpk0.bin",
        "offset": 0x2BA8,
        "format": "i4",
        "width": 32,
        "height": 16,
        "symbol": "dEFCommonEffects3_RebirthHalo_glow",
    },
)

DEFAULT_HEADER = Path("include/nds/generated/nds_particle_banks.generated.h")
DEFAULT_INC = Path("src/nds/generated/nds_particle_banks.generated.inc")
DEFAULT_REPORT = Path("docs/optimization/NDS_PARTICLE_BANKS.generated.json")
DEFAULT_TEXTURE_ASSET = Path("assets/particles/efcommon_particle_textures.ds.bin")

# Texels and palettes ship as a NitroFS payload rather than as linked .rodata.
# This is not a size preference; it is the only place they fit. Linked const
# data comes out of the same 4 MB the boot-time taskman arena search is trying
# to claim (src/port/diagnostics.c searches down from 0x150000 in 0x1000 steps
# and FLOORS at 0x130000), so 82,848 bytes of .rodata costs the arena 82,848
# bytes one-for-one. With the pack linked whole, that search bottomed out at
# the floor and the first battle allocation -- 4,896 bytes against 4,032 free --
# hung the ROM in syTaskmanMalloc's `while (TRUE);`. ROM is the cheap resource
# here (PROJECT_GOAL "ROM and RAM Philosophy"), main RAM is not.
#
# The script bank and the index tables stay linked: the interpreter walks the
# bytecode every frame and indexes the tables by SOURCE id, so paging those
# would put file I/O in a gameplay frame for 12,195 bytes of nothing.
#
# Layout is the raw texture block followed by the palette block, matching the
# offsets already in gNdsParticleTextures: data_offset is a byte offset from 0
# and palette_offset is a u16 ENTRY index from NDS_PARTICLE_PALETTE_ASSET_OFFSET.
TEXTURE_ASSET_NITRO_PATH = "nitro:/particles/efcommon_particle_textures.ds.bin"

# The DS draw path's own payload: ONE RGB555+A1 atlas. See build_quad_sheet for
# why the texels are encoded twice.
#
# 128x128 -> 64x64 on 2026-08-01, and 8,192 bytes is a MEASURED HARD BOUND
# rather than a budget. With a 32,768-byte sheet resident,
# `ndsRendererHardwareResolveStageSourceFrameTexture` failed about one frame in
# ten -- reject site 2, 196 times in a 566-frame match, mask 4096 (TEXIMAGE),
# with the cache census at the first rejection reading Free 7 / Live 41 /
# Pinned 25 / ThisFrame 16 / Evictable 0. Each failure rejected the native
# stage owner and dropped that frame onto the generic renderer at five or more
# VBlanks: 196 of 566.
#
# It is NOT the byte count, and that had to be measured rather than assumed:
# 128x64 freed 16,384 bytes, kept both textures a live match draws, and
# rejected identically -- 196 and 197, to the digit. 64x64 fixes it completely.
# So what matters is where a 16-to-32 KB block lands in libnds's per-bank
# splitting, in the same allocator that has already refused a 4,096-byte upload
# with 268,800 bytes free (PORTING.md). At 64x64 the run comes back to the
# CONTROL's own numbers: StagePrepareBuildCount 2, Reuse 2,041, five-VBlank
# frames 4, and 117,937 quads emitted with ZERO misses.
#
# More coverage cannot come from another allocation either. A second 8 KiB page
# reproduced the exact same violation/rebuild pair: 1 and 197. The corrected KO
# path needs thirteen frames from textures 10/13/18/19/20/21. At 8x8 they use
# 832 texels, fitting the proven sheet's 896 free texels without changing its
# one-name, one-allocation contract.
#
# 128x64 IS BACK, AND IT IS STILL 8,192 BYTES. Everything above is about the
# ALLOCATION, and every refuted variant grew it: 128x64 and 64x128 failed at
# 16,384 bytes, 128x128 at 32,768, and a second page failed at two allocations.
# The sheet is now A5I3 at one byte per texel instead of RGB555+A1 at two, so
# 128x64 asks the allocator for exactly the 8,192-byte block that is already
# proven and gets twice the texels for it. Nothing about the one-name,
# one-allocation contract changes; the palette is sixteen bytes and lives in
# VRAM F/G, which is not the allocator that was refusing.
# 128x256 = 32,768 texels since 2026-08-03, and the ALLOCATION objection that
# capped this at 8,192 is retired rather than overruled. Everything below was
# right: a bigger sheet starved the stage's texture resolve because texture VRAM
# had about 8 KB of headroom and no more. It now has 74,496 more, because the 23
# static battle textures stopped being stored at two bytes a texel when 22 of
# them are sixteen-colour CI4 sources -- see the note on `ds_format` in
# include/nds/battle_playable_static_textures.h. The sheet costs 24,576 of that
# and leaves 49,920 in hand.
#
# What it buys is the owner's row: "ALL VFX look low quality or low resolution.
# VFX should use source quality or 0.8x reduction MAX". At 8,192 the cells were
# 16x16 and 8x8 against 32x32 sources -- 0.5x and 0.25x. QUAD_CELL_MAX is now
# the source's own 32.
QUAD_ATLAS_WIDTH = 128
# 64, and TRIED AT 128 ON 2026-08-02 AT THE OWNER'S REQUEST -- MEASURED, BROKEN,
# REVERTED. Do not spend another round on it without reading this.
#
# The theory was that 16,384 fails for want of a CONTIGUOUS run rather than
# capacity (scvsbattle.c:168: 262,144 texture VRAM, 136,192 static, 57,344 for
# the interface's three atlases), and that the atlas failing closed made the
# experiment cheap. Both halves of that were right and it still broke, because
# the failure is not in the atlas -- the atlas UPLOADED FINE.
# gNdsRendererParticleAtlasFailCount stayed 0 and AtlasBytes read 16,400. What
# broke is everything that allocates AFTER it:
#   gNdsRendererBattleStaticTextureViolationCount  0 -> 1
#   gNdsR2StagePrepareBuildCount                   2 -> 244
# and the Results scoreboard panel (KOs/Pts/Place, the 1P/2P markers) vanished
# from the final frame. Taking the largest free run before the interface asks
# for one is precisely what scvsbattle.c warned about; preparing the atlas last
# only protects it from FAILING, not from succeeding at the interface's expense.
#
# The prize was real and is what makes this tempting: at 128 the sheet admits
# 35 of 36 textures in 148 frames and gNdsParticleQuadMissCount fell 684 -> 0.
# Anyone re-trying it needs a plan for the CONTIGUITY, not for the byte count --
# reserving the atlas block before the interface allocates, or splitting the
# sheet across two smaller allocations, not simply asking for more.
QUAD_ATLAS_HEIGHT = 64
# THE SHEET SIZE IS FIXED AT 8,192 BYTES AND THE SHEET COUNT IS THE VARIABLE.
# That is the whole answer to the note above, which asked for "a plan for the
# CONTIGUITY, not for the byte count -- ... splitting the sheet across two
# smaller allocations, not simply asking for more."
#
# 2026-08-03 measured the other branch to exhaustion. 74,496 bytes were freed
# losslessly from the static battle corpus (22 of its 24 textures are CI4
# sources that were stored at two bytes a texel), so a 32,768-byte sheet was
# affordable by every byte count -- 110,336 free against 32,768 asked. It still
# broke: 27.8 FPS -> 8.6, logic 55.6 Hz -> 17.2, and flat untextured white on
# the stage where the texture resolve had started failing every frame
# (artifacts/visibility/2026-08-03_atlas32k-candidate.png). Free space does not
# buy a contiguous run, because libnds splits texture VRAM per bank and the
# banks are already carved up by the static corpus and the interface atlases.
#
# So this asks for FOUR allocations of the size that has never once been
# refused, instead of one of a size that has now been refused at 16,384 and at
# 32,768. Same 32,768 texels, four separate glTexImage2D calls, each free to
# land in whatever bank has an 8 KiB hole. The runtime binds per sheet and the
# frame table carries which one -- a quad's sheet is known before it is drawn,
# so a sheet change costs one texture bind and a new primitive group, exactly
# what an alpha-bucket change already costs on this path.
QUAD_ATLAS_SHEETS_MAX = 4
# Admitted before anything else. These are the textures a natural single-CPU
# Mario-vs-Fox match was OBSERVED drawing, so they must survive admission
# whatever the packer does with the rest. Regrade this from the use mask after
# any change that adds effects -- an entry here that a match never draws is
# wasted sheet, and one missing from it becomes a QuadMiss. Read the mask as
# BITS, not as a hex digit pattern: 0x08400000 is 22 and 27, and reading it as
# 22/23/26 sent one round of this at textures the pack does not even carry.
# REGRADED 2026-08-01 from a both-CPU soak's own mask, after this list cost
# 127,989 QuadMisses in one match. It read (22, 27) because that was the single-
# CPU mask, and a single-CPU match is Mario standing still. The both-CPU mask is
# 0x08400007 -- bits 0, 1, 2, 22, 27 -- and admitting Pupupu had evicted texture
# 0, which alone carries almost every particle a moving match draws: 130,714
# visible particles produced 2,725 quads. Bits, not hex digits.
# Up-star script 0x5C uses texture 24; a forced source-status run proved that
# its missing cell produced one miss and zero quads. It fits only at 8x8.
# REGRADED AGAIN 2026-08-02, from a 3.5-minute both-CPU soak that reached
# Sudden Death. Two masks, read as BITS:
#   TextureUseMask 0x2B4582C7 / 0x2366 -- drawn at least once:
#     0 1 2 15 16 17 22 24 25 27 29 33 34 37 38 40 41 45
#   QuadMissMask   0x22020000 / 0x2346 -- REFUSED every time, 684 draws lost:
#     17 25 29 33 34 38 40 41 45
# The refused nine are not obscure. Scout-mapped against efmanager.c they are
# 17 DustDash, 25 DamageCoin, 29 SparkleWhiteMulti, 33/34 DamageNormalLight,
# 38 SetOff, 40 DamageFire, 41 DamageNormalHeavy, 45 SparkleWhite -- which is
# BUGS.md's "running foot dust VFX, fireball hit VFX ..." row almost verbatim.
# They are all in the live set now, and QUAD_FRAME_CAP is what makes room.
#
# 684 of 362,759 draws sounds like 0.19% and is the WRONG metric: these nine were
# refused 100% of the time they were asked for, so the effects are binary-absent,
# not slightly thinned. Grade this list by which EFFECTS lose their texture, not
# by share of draws.
#
# Everything demoted out of the list is demoted, not deleted -- the packer still
# admits it if the sheet has room after the live set, which is how 3/4/5/9/11/14
# got on the sheet in the first place. They drew nothing in this match, so they
# lose their places to the nine that did.
# Bits, not hex digits: 0x08400000 is 22 and 27, and reading it as 22/23/26 sent
# one whole round of this at textures the pack does not even carry.
QUAD_KO_LIVE = frozenset((10, 13, 18, 19, 20, 21, 24))
QUAD_MEASURED_LIVE = frozenset(
    (0, 1, 2, 10, 13, 15, 16, 17, 18, 19, 20, 21, 22, 24, 25, 27, 29, 33, 34,
     37, 38, 40, 41, 45))
# A5I3: one byte per texel, 5-bit alpha, 3-bit index into a shared palette.
# Two reasons, and the second is the one that shows on screen.
#
# RGB555+A1 gave particles ONE BIT of alpha -- `alpha >= 128 ? colour : 0` in
# the assembler below -- so every soft-edged particle in the game was drawing as
# a hard-edged blob. Particles are the one thing that needs a smooth fade.
# A5I3 gives 32 levels.
#
# Eight palette entries is not a compromise here: the draw path leaves the
# polygon in modulation mode and calls glColor with pc->primcolor per quad
# (nds_renderer.c, ndsRendererSubmitParticleQuad), so the texture supplies shape
# and the source supplies colour, exactly as the RDP's prim colour did.
# 8 -> 32 on 2026-08-03, which is A3I5 rather than A5I3.
#
# BUGS.md "Fox down B VFX is not correct or using correct asset" is the row that
# forces this, and it is the one asset on the sheet that carries COLOUR instead
# of shape: relocData/346.vpk0.bin is CI4 16x16 using exactly two LUT entries,
# a deep blue (0,8,239) body and a cyan (0,231,247) edge. Eight entries shared
# across the whole sheet cannot hold two specific blues AND the shapes every
# other effect needs, so the reflector came out as whatever the weighted palette
# had nearest -- which is what "not the correct asset" means here.
#
# THE COST IS ALPHA, AND IT IS THE SMALLER LOSS. A3I5 has 8 alpha levels where
# A5I3 has 32. The thing A5I3 was introduced to fix was RGB555+A1's ONE bit --
# soft-edged particles drawing as hard blobs -- and 8 levels is four times what
# that failure had, on cells that are now 32x32 instead of 16x16, so the edge
# has four times the texels to spread a gentler ramp over. Colour cannot be
# recovered any other way; alpha resolution can be spent.
QUAD_ATLAS_PALETTE_ENTRIES = 32
QUAD_SHEET_BUDGET_BYTES = QUAD_ATLAS_WIDTH * QUAD_ATLAS_HEIGHT
# The largest cell the atlas will hold, in texels per axis. This is the
# "halving" the exclusion note below always pointed at, and texture 0 is why it
# finally had to exist: at its source 32x32 it takes a shelf of its own -- the
# packer groups by exact height -- so admitting it costs 32 of the atlas's 64
# rows and wastes half of them, which pushed the 16x16 shelves past the bottom
# edge and dropped the one texture a moving match draws most. At 16x16 it is an
# ordinary cell. Reduced texture resolution is explicitly allowed by
# PROJECT_GOAL.md; a particle that does not draw at all is not.
# SOURCE RESOLUTION. The owner's bar is "source quality or 0.8x reduction MAX"
# and 0.8x of 32 is 25.6, which is worse than 32 in both directions: it is a
# non-integer resample, so it costs a blur the 2:1 halving never did, and it
# saves texels the sheet no longer needs. 1.0x is inside the bar and cheaper to
# produce.
#
# 64 IS "NO REDUCTION": the largest source in the bank is 64x64, so a cap at 64
# passes every texture through at its own resolution. The ladder below steps
# down from it only if four sheets cannot seat the live set, and the owner's bar
# ("source quality or 0.8x reduction MAX") is what makes 64 the entry rather
# than the fallback. 0.8x is not on the ladder because it is not reachable: a
# 2:1 box average is exact and a 0.8 resample is a blur, so 1.0x is both inside
# the bar and cheaper to produce than the thing the bar permits.
QUAD_CELL_MAX = 64
QUAD_CELL_LADDER = (64, 32, 16, 8)
QUAD_KO_CELL_MAX = 64
# A LONG ANIMATION PAYS PER FRAME, so it is sized per frame. After the A5I3
# conversion doubled the texel budget the set that still would not fit was not
# a set of big textures, it was a set of long ones: 28 is twenty frames, 25 is
# fifteen, 17 is ten at 64x64. At 16x16 texture 28 alone wants 5,120 of the
# 8,192 texels for an animation whose individual frames are on screen for one
# or two presented frames each. Halving those to 8x8 costs resolution nobody
# can resolve at that duration and buys the whole rest of the drawn set.
# Same trade the KO cap already makes, keyed on the thing that actually drives
# the cost. PROJECT_GOAL.md allows reduced texture resolution explicitly; it
# does not allow a particle that draws nothing.
QUAD_LONG_ANIMATION_FRAMES = 6
QUAD_LONG_ANIMATION_CELL_MAX = 64
# ...and then DECIMATE what is left, because halving the cell stopped being
# enough. The cap above trades resolution; this one trades animation rate, which
# PROJECT_GOAL.md allows in as many words ("reduced animation update rates",
# "quantized animation poses"). It is the cheaper of the two for these assets:
# 25 is fifteen 8x8 frames of DamageCoin, and nobody resolves fifteen distinct
# poses of a coin that lives well under a second.
#
# WHY IT IS NEEDED, measured 2026-08-02 on a 3.5-minute both-CPU soak rather
# than argued: nine textures were refused every time they were asked for --
# 17, 25, 29, 33, 34, 38, 40, 41, 45 -- and the scout mapping puts the owner's
# own BUGS.md list on exactly those. 17 is efManagerDustDashMakeEffect, i.e.
# "running foot dust VFX". 40 is efManagerDamageFireMakeEffect, i.e. "fireball
# hit VFX". 33/34 are DamageNormalLight and 41 is DamageNormalHeavy, the two
# generic hit flashes. Admitting all nine at full frame counts costs 6,016 bytes
# against 448 free. Decimated to four frames it is 3,328, which the sheet can
# find once the live set below is regraded.
#
# THE RUNTIME HALF IS LOAD-BEARING AND LIVES ELSEWHERE. ndsParticleQuadFrameFor
# (src/import/battleship_lbparticle.c) returns the NEAREST EARLIER packed frame,
# so `frame` in the emitted row stays the SOURCE index and a decimated animation
# plays at reduced rate over its full arc. Before 2026-08-02 that lookup was an
# exact match returning NULL, and packing a subset here would have turned every
# unpacked frame into a QuadMiss -- i.e. this constant would have made the bug
# worse, not better. Do not raise the cap without checking that lookup still
# clamps.
QUAD_FRAME_CAP = 6
# WHICH source frame a one-cell texture holds. The search above settles at cap 1
# and quad_frame_list then returns [0], so every multi-frame animation ships its
# opening frame -- 19 of the 32 admitted textures (commit 3d002c39 names and
# ranks all nineteen). That frame 0 is a FALLBACK, not a choice: nothing ever
# looked at the animation and decided the first frame represented it best.
#
# For texture 25 (DamageCoin, Mario's up-B coin, 16x16 CI4 x15) it is the worst
# of the fifteen -- the most opaque, 148 of 256 texels, which is why the effect
# reads as a flat yellow blob. The owner picked frame 2 by eye from
# artifacts/visibility/2026-08-04_upb-coin-timeline-source-vs-port.png and
# declined the same treatment for the other eighteen, so this table has exactly
# one row and the rest keep frame 0.
#
# THE LOOKUP KEY STAYS 0. ndsParticleQuadFrameFor returns the nearest EARLIER
# packed frame and NULL below the first one, so emitting the row as frame 2
# would make source frames 0 and 1 miss and the coin would not draw at all for
# its first two frames. What the held frame changes is the cell's CONTENT, not
# its index -- hence a separate `source` on the cell.
#
# Inert unless the texture packs exactly one cell: if the sheet ever affords
# more frames, the real animation wins and this table stops applying without
# anyone editing it out.
QUAD_HELD_FRAME = {25: 2}


def quad_cell_dims(width: int, height: int,
                   cell_max: int = QUAD_CELL_MAX) -> tuple[int, int]:
    """Halve both axes until the cell fits cell_max. Aspect is preserved."""
    cell_w, cell_h = width, height
    while (cell_w > cell_max) or (cell_h > cell_max):
        cell_w = max(1, cell_w // 2)
        cell_h = max(1, cell_h // 2)
    return cell_w, cell_h


def quad_frame_list(frames: int, cap: int = QUAD_FRAME_CAP) -> list[int]:
    """SOURCE frame indices to pack, evenly spaced across the animation.

    Always keeps frame 0 and the final frame: the runtime lookup has no earlier
    row to clamp to below the first packed frame, so dropping frame 0 would put
    the start of every animation back on the miss path.
    """
    if frames <= cap:
        return list(range(frames))
    if cap <= 1:
        return [0]
    return sorted({round(i * (frames - 1) / (cap - 1)) for i in range(cap)})


def quad_cell_source_frame(texture: int, frames: int, frame_list: list[int],
                           frame: int) -> int:
    """SOURCE frame whose texels this cell holds. See QUAD_HELD_FRAME."""
    if len(frame_list) != 1:
        return frame
    held = QUAD_HELD_FRAME.get(texture)
    if held is None:
        return frame
    if not (0 <= held < frames):
        raise SystemExit(
            f"quad held frame {held} for texture {texture} is outside its "
            f"{frames} source frames")
    return held
DEFAULT_QUAD_ASSET = Path("assets/particles/efcommon_particle_quads.a5i3.bin")
QUAD_ASSET_NITRO_PATH = "nitro:/particles/efcommon_particle_quads.a5i3.bin"

# The port's efcommon bank handle. Only calls whose bank argument names it can
# reach this bank; the per-fighter and per-stage banks are separate handles.
EFCOMMON_BANK_TOKEN = "gEFManagerParticleBankID"

# BattleShip constructors that take (bank_id, script_id, ...). The 1-based
# index is the position of the script id; lbParticleMakeChildScriptID takes a
# parent first. lbParticleMakeParam is excluded: it carries an explicit
# bytecode pointer rather than a bank script id.
SCRIPT_CONSTRUCTORS = {
    "lbParticleMakeScriptID": 1,
    "lbParticleMakeChildScriptID": 2,
    "lbParticleMakeCommon": 1,
    "lbParticleMakePosVel": 1,
    "lbParticleMakeGenerator": 1,
}

# efManagerShieldMakeEffect is classified SUBSTITUTE by
# generate_task39_effect_census.py through an explicit extra row rather than
# through SUBSTITUTES, so name it here to keep the two lists equivalent.
#
# The four below joined it on 2026-08-01, when ndsFTParamMakeSourceEffect
# started routing the motion-script effect kinds to their source makers. Until
# then ftParamMakeEffect answered nEFKindDustLight, DustHeavy, DustHeavyDouble
# and MusicNote with DS primitives, so those makers were never called and the
# derivation had no reason to consider them P1 -- which meant their scripts
# were never packed. The runtime then rejected script 0x58 forty-nine times a
# match with reason 4 (the slot holds sNdsParticleInertScript), i.e. the hard
# landing drew NOTHING. That is BUGS.md "Correct VFX isn't played for various
# things ... hard landing vfx".
#
# THE RULE THIS ENCODES: routing a maker to source is not finished until this
# list knows about it. A seam that becomes live without being named here fails
# CLOSED and silently -- the reject ring is the only place it shows.
P1_EXTRA_SEAMS = frozenset((
    "efManagerShieldMakeEffect",
    "efManagerDustLightMakeEffect",
    "efManagerDustHeavyMakeEffect",
    "efManagerDustHeavyDoubleMakeEffect",
    "efManagerMusicNoteMakeEffect",
))

# THE SUBSTITUTE LIST IS NOT THE SEAM LIST. This derivation used to seed from
# census.SUBSTITUTES alone, which is the set of effects Task 39 REPLACES with
# 2D sprites -- close to the exact complement of the set that still needs a
# packed script. The runtime settled it: the first full match with the
# interpreter alive started zero scripts, and the reject ring
# (gNdsParticleRejectRing*) named four (bank, script) pairs, of which two were
# efcommon ids marked UNREACHABLE by this very file:
#
#   bank 1 script 0x62 x8  efManagerFoxBlasterGlowMakeEffect  (Fox neutral-B)
#   bank 0 script 0x70 x2  efManagerConfettiMakeEffect        (Results)
#
# -- while not one substituted script was ever asked for, because the sprite
# path takes those calls before they reach the constructor.
#
# So the seed is now "the efmanager seams a Mario-vs-Fox Dream Land items-off
# match can reach", listed explicitly. Membership is decided by the P1
# configuration, not by taste:
#   * fighter-agnostic combat, movement, shield, grab, KO and respawn effects
#     are in -- either fighter can produce them;
#   * Mario-specific and Fox-specific effects are in;
#   * every other fighter's effects are out (no Kirby, Ness, Link, Pikachu,
#     Yoshi, Samus, DK, Captain Falcon, Jigglypuff, Master Ball);
#   * item effects are out (items off);
#   * water ripples are out (Dream Land has no water).
# Keeping the substitutes as well costs nothing measurable and keeps the pack
# correct if a substitute is ever switched back to its original route.
P1_PARTICLE_SEAMS = frozenset((
    # movement
    "efManagerDustExpandLargeMakeEffect",
    "efManagerDustExpandSmallMakeEffect",
    "efManagerDustDashMakeEffect",
    # hit reactions and debris
    "efManagerDamageSpawnOrbsMakeEffect",
    "efManagerDamageSpawnSparksMakeEffect",
    "efManagerDamageSpawnMDustMakeEffect",
    "efManagerImpactWaveMakeEffect",
    "efManagerImpactAirWaveMakeEffect",
    "efManagerSetOffMakeEffect",
    "efManagerFireSparkMakeEffect",
    # sparkles and flashes
    "efManagerSparkleWhiteMultiMakeEffect",
    "efManagerSparkleWhiteMultiExplodeMakeEffect",
    "efManagerFlashSmallMakeEffect",
    "efManagerFlashLargeMakeEffect",
    "efManagerFuraSparkleMakeEffect",
    # defence and grabs
    "efManagerShieldBreakMakeEffect",
    "efManagerCatchSwirlMakeEffect",
    "efManagerReflectBreakMakeEffect",
    # KO, star KO and respawn
    "efManagerStarSplashMakeEffect",
    # fighter-specific: Fox
    "efManagerFoxBlasterGlowMakeEffect",
    "efManagerFoxEntryArwingMakeEffect",
    # fighter-specific: Mario
    "efManagerMarioEntryDokanMakeEffect",
    # match flow
    "efManagerConfettiMakeEffect",
))

O2R_BLOB_MAGIC = b"BLBO"
O2R_BLOB_SIZE_OFFSET = 0x40
O2R_BLOB_DATA_OFFSET = 0x44

LB_SCRIPT_HEADER_BYTES = 0x30
LB_TEXTURE_HEADER_BYTES = 0x18
RODATA_ALIGN = 16

G_IM_FMT_RGBA, G_IM_FMT_CI, G_IM_FMT_IA, G_IM_FMT_I = 0, 2, 3, 4
G_IM_SIZ_BITS = {0: 4, 1: 8, 2: 16, 3: 32}
SOURCE_FORMAT_NAMES = {G_IM_FMT_RGBA: "RGBA", G_IM_FMT_CI: "CI",
                       G_IM_FMT_IA: "IA", G_IM_FMT_I: "I"}

# DS TEXIMAGE_PARAM texture-format field values (libnds GL_RGB32_A3, GL_RGB4,
# GL_RGB16, GL_RGB256, GL_RGB8_A5, GL_RGBA). Emitting the hardware field value
# keeps the runtime from needing a second translation table.
DS_NONE, DS_A3I5, DS_PAL4, DS_PAL16, DS_PAL256, DS_A5I3, DS_DIRECT16 = \
    0, 1, 2, 3, 4, 6, 7
DS_FORMAT_NAMES = {DS_NONE: "NONE", DS_A3I5: "A3I5", DS_PAL4: "PAL4",
                   DS_PAL16: "PAL16", DS_PAL256: "PAL256", DS_A5I3: "A5I3",
                   DS_DIRECT16: "DIRECT16"}
DS_FORMAT_BITS = {DS_A3I5: 8, DS_PAL4: 2, DS_PAL16: 4, DS_PAL256: 8,
                  DS_A5I3: 8, DS_DIRECT16: 16}
# (palette colours excluding the transparent slot, distinct alpha levels)
DS_FORMAT_CAPS = {DS_A3I5: (32, 8), DS_PAL4: (3, 2), DS_PAL16: (15, 2),
                  DS_PAL256: (255, 2), DS_A5I3: (8, 32), DS_DIRECT16: (0, 2)}
# Palette formats reserve entry 0 as the transparent slot so the runtime can
# set the colour-0-transparent bit unconditionally; A3I5/A5I3 carry alpha in
# the texel and use every entry as a colour.
DS_PALETTE_FORMATS = (DS_PAL4, DS_PAL16, DS_PAL256)
DS_ALPHA_FORMATS = (DS_A5I3, DS_A3I5)
# The DS palette base register addresses 16-byte units for every format except
# PAL4, so start each palette on an 8-entry boundary.
DS_PALETTE_ALIGN_ENTRIES = 8
DS_TEXTURE_DATA_ALIGN = 4

SENTINEL_U32 = 0xFFFFFFFF

# The 2026-07-27 sizing in docs/KNOWN_ISSUES.md, restated here so the generator
# reports against it instead of a number someone has to look up.
ESTIMATE = {
    "scripts": 26,
    "textures": 18,
    "texture_bytes": 118856,
    "script_bank_bytes": 10912,
    "grpupupu_bytes": 4896,
    "arena_headroom_bytes": 210320,
}


# --------------------------------------------------------------------------
# source loading
# --------------------------------------------------------------------------
def load_o2r_blob(repo_root: Path, name: str, expected_sha256: str) -> bytes:
    """Return the raw bank payload from an O2R 'BLBO' resource."""
    path = repo_root / O2R_PARTICLES / name
    source = path.read_bytes()
    actual = hashlib.sha256(source).hexdigest()
    if actual != expected_sha256:
        raise SystemExit(f"{path}: SHA-256 {actual} != {expected_sha256}")
    if len(source) < O2R_BLOB_DATA_OFFSET:
        raise SystemExit(f"{path}: truncated O2R resource header")
    if source[4:8] != O2R_BLOB_MAGIC:
        raise SystemExit(f"{path}: {source[4:8]!r} is not an O2R blob resource")
    size = struct.unpack_from("<I", source, O2R_BLOB_SIZE_OFFSET)[0]
    if O2R_BLOB_DATA_OFFSET + size != len(source):
        raise SystemExit(
            f"{path}: declared {size} bytes but the resource holds "
            f"{len(source) - O2R_BLOB_DATA_OFFSET}"
        )
    return source[O2R_BLOB_DATA_OFFSET:]


# --------------------------------------------------------------------------
# .scb -- script bank
# --------------------------------------------------------------------------
# Operand widths taken from the BattleShip dispatcher
# (src/lb/lbparticle.c lbParticleUpdateStruct). Opcodes with a variable-length
# ushort or a channel mask are handled separately below.
OPCODE_FIXED_OPERANDS = {
    0xA1: 1, 0xA2: 4, 0xA3: 4, 0xA4: 2, 0xA5: 2, 0xA6: 4, 0xA7: 1,
    0xA8: 12, 0xA9: 4, 0xAA: 4, 0xAB: 4, 0xAD: 0, 0xAE: 0, 0xAF: 0,
    0xB0: 0, 0xB1: 0, 0xB2: 0, 0xB3: 0, 0xB4: 0, 0xB5: 0, 0xB6: 0,
    0xB7: 1, 0xB8: 5, 0xB9: 2, 0xBA: 4, 0xBB: 4, 0xBC: 2, 0xBD: 8,
    0xBE: 12, 0xBF: 1, 0xFA: 1, 0xFB: 0, 0xFC: 0, 0xFD: 0,
}
OPCODE_VECTOR = (0x80, 0x88, 0x90, 0x98)      # channel mask in the low 3 bits
OPCODE_SIZE_LERP, OPCODE_SIZE_RAND = 0xA0, 0xAC
OPCODE_PRIM_BLEND, OPCODE_ENV_BLEND = 0xC0, 0xD0
OPCODE_TERMINATORS = (0xFE, 0xFF)
# Opcodes that name another script in the same bank.
OPCODE_MAKE_SCRIPT, OPCODE_MAKE_GENERATOR = 0xA4, 0xA5
OPCODE_MAKE_RANDOM, OPCODE_MAKE_ID = 0xAA, 0xB9


def _decode_opcode(command: int) -> int:
    opcode = command & 0xF8
    if opcode > 0x98:
        opcode = command & 0xF0
        if opcode not in (OPCODE_PRIM_BLEND, OPCODE_ENV_BLEND):
            opcode = command
    return opcode


def decode_bytecode(bytecode: bytes, script_id: int) -> list[tuple[int, bytes]]:
    """Return [(opcode, operand_bytes)] up to and including the terminator."""
    commands = []
    cursor = 0
    while cursor < len(bytecode):
        start = cursor
        command = bytecode[cursor]
        cursor += 1
        if command < 0x80:
            if command & 0x20:
                cursor += 1          # long wait, second byte
            if command & 0x40:
                cursor += 1          # explicit frame id
            commands.append((command, bytecode[start:cursor]))
            continue
        opcode = _decode_opcode(command)
        if opcode in OPCODE_VECTOR:
            cursor += 4 * bin(command & 7).count("1")
        elif opcode in (OPCODE_SIZE_LERP, OPCODE_SIZE_RAND):
            cursor += (2 if bytecode[cursor] & 0x80 else 1)
            cursor += 4 if opcode == OPCODE_SIZE_LERP else 8
        elif opcode in (OPCODE_PRIM_BLEND, OPCODE_ENV_BLEND):
            cursor += (2 if bytecode[cursor] & 0x80 else 1)
            cursor += bin(command & 0xF).count("1")
        elif opcode in OPCODE_TERMINATORS:
            commands.append((opcode, bytecode[start:cursor]))
            return commands
        elif opcode in OPCODE_FIXED_OPERANDS:
            cursor += OPCODE_FIXED_OPERANDS[opcode]
        else:
            raise SystemExit(
                f"script {script_id}: unknown opcode 0x{opcode:02x} at "
                f"bytecode offset {start}"
            )
        if cursor > len(bytecode):
            raise SystemExit(
                f"script {script_id}: opcode 0x{opcode:02x} at {start} "
                "reads past the end of its bytecode"
            )
        commands.append((opcode, bytecode[start:cursor]))
    raise SystemExit(f"script {script_id}: bytecode has no DEAD/END terminator")


def parse_script_bank(payload: bytes) -> list[dict]:
    count = struct.unpack_from(">i", payload, 0)[0]
    if count <= 0 or 4 + 4 * count > len(payload):
        raise SystemExit(f"script bank declares {count} scripts")
    offsets = list(struct.unpack_from(f">{count}I", payload, 4))
    if offsets != sorted(offsets) or offsets[0] < 4 + 4 * count:
        raise SystemExit("script bank offsets are not an ascending table")
    scripts = []
    for index, offset in enumerate(offsets):
        end = offsets[index + 1] if index + 1 < count else len(payload)
        if offset + LB_SCRIPT_HEADER_BYTES > end:
            raise SystemExit(f"script {index} is shorter than its header")
        kind, texture_id = struct.unpack_from(">HH", payload, offset)
        bytecode = payload[offset + LB_SCRIPT_HEADER_BYTES:end]
        commands = decode_bytecode(bytecode, index)
        used = sum(len(operands) for _opcode, operands in commands)
        scripts.append({
            "id": index, "offset": offset, "kind": kind,
            "texture_id": texture_id, "bytecode": bytecode,
            "bytecode_used": used, "commands": commands,
        })
    # PARTICLE_BANK_DISCOVERIES.md section 5d: the whole file size is a
    # function of the per-script bytecode lengths. Recomputing it proves the
    # bytecode walk consumed exactly the right number of bytes per script.
    layout = 4 + 4 * count
    for script in scripts:
        layout += LB_SCRIPT_HEADER_BYTES + ((script["bytecode_used"] + 3) & ~3)
    layout = (layout + RODATA_ALIGN - 1) & ~(RODATA_ALIGN - 1)
    if layout != len(payload):
        raise SystemExit(
            f"script bank layout reconstructs to {layout} bytes but the file "
            f"holds {len(payload)}"
        )
    return scripts


def spawned_scripts(script: dict, script_count: int) -> set[int]:
    """Script ids this script's bytecode can instantiate."""
    children = set()
    for opcode, operands in script["commands"]:
        if opcode in (OPCODE_MAKE_SCRIPT, OPCODE_MAKE_GENERATOR,
                      OPCODE_MAKE_ID):
            children.add((operands[1] << 8) | operands[2])
        elif opcode == OPCODE_MAKE_RANDOM:
            base = (operands[1] << 8) | operands[2]
            span = (operands[3] << 8) | operands[4]
            children.update(range(base, base + span + 1))
    return {child for child in children if child < script_count}


# --------------------------------------------------------------------------
# .txb -- texture bank
# --------------------------------------------------------------------------
def parse_texture_bank(payload: bytes) -> list[dict]:
    count = struct.unpack_from(">i", payload, 0)[0]
    if count <= 0 or 4 + 4 * count > len(payload):
        raise SystemExit(f"texture bank declares {count} textures")
    offsets = list(struct.unpack_from(f">{count}I", payload, 4))
    if offsets != sorted(offsets) or offsets[0] < 4 + 4 * count:
        raise SystemExit("texture bank offsets are not an ascending table")
    textures = []
    accounted = offsets[0]
    for index, offset in enumerate(offsets):
        frames, fmt, siz, width, height, flags = struct.unpack_from(
            ">IiiiiI", payload, offset
        )
        if fmt not in SOURCE_FORMAT_NAMES or siz not in G_IM_SIZ_BITS:
            raise SystemExit(f"texture {index}: fmt {fmt} siz {siz}")
        accounted += LB_TEXTURE_HEADER_BYTES
        images: list[int] = []
        palettes: list[int] = []
        pointer_count = 0
        if frames:
            # data[] length is not a fixed function of frames/flags: the file
            # tells us, because data[0] is the first image and images follow
            # the pointer array directly.
            first = struct.unpack_from(">I", payload, offset +
                                       LB_TEXTURE_HEADER_BYTES)[0]
            span = first - (offset + LB_TEXTURE_HEADER_BYTES)
            if span <= 0 or span % 4:
                raise SystemExit(f"texture {index}: bad data[] span {span}")
            pointer_count = span // 4
            if pointer_count < frames:
                raise SystemExit(
                    f"texture {index}: data[{pointer_count}] cannot hold "
                    f"{frames} images"
                )
            pointers = struct.unpack_from(f">{pointer_count}I", payload,
                                          offset + LB_TEXTURE_HEADER_BYTES)
            images = list(pointers[:frames])
            palettes = list(pointers[frames:])
        image_bytes = width * height * G_IM_SIZ_BITS[siz] // 8
        palette_entries = 0
        palette_count = 0
        if fmt == G_IM_FMT_CI:
            palette_entries = 16 if siz == 0 else 256
            palette_count = 1 if (flags & 1) else frames
            if palette_count > len(palettes):
                raise SystemExit(
                    f"texture {index}: needs {palette_count} palettes but "
                    f"data[] holds {len(palettes)}"
                )
        accounted += 4 * pointer_count + image_bytes * frames \
            + 2 * palette_entries * palette_count
        textures.append({
            "id": index, "offset": offset, "frames": frames, "fmt": fmt,
            "siz": siz, "bits": G_IM_SIZ_BITS[siz], "width": width,
            "height": height, "flags": flags, "images": images,
            "palettes": palettes, "pointer_count": pointer_count,
            "image_bytes": image_bytes, "palette_entries": palette_entries,
            "palette_count": palette_count,
            "source_bytes": LB_TEXTURE_HEADER_BYTES + 4 * pointer_count
            + image_bytes * frames + 2 * palette_entries * palette_count,
        })
    accounted = (accounted + RODATA_ALIGN - 1) & ~(RODATA_ALIGN - 1)
    if accounted != len(payload):
        raise SystemExit(
            f"texture bank accounts for {accounted} bytes but the file holds "
            f"{len(payload)}"
        )
    return textures


def decode_texture_frame(payload: bytes, texture: dict,
                         frame: int) -> list[tuple[int, int, int, int]]:
    """Decode one frame to RGBA8888.

    Images are stored linearly. The RDP's odd-line TMEM word swap is applied
    on both the load and the fetch, so it cancels and the RDRAM image is plain
    row-major -- measured on every efcommon texture, and the reason this
    generator must not apply the swizzle that
    scripts/generate_task39_hit_sparks.py applies.
    """
    width, height, bits = texture["width"], texture["height"], texture["bits"]
    base = texture["images"][frame]
    fmt = texture["fmt"]
    palette: list[tuple[int, int, int, int]] = []
    if fmt == G_IM_FMT_CI:
        palette_index = 0 if (texture["flags"] & 1) else frame
        palette_offset = texture["palettes"][palette_index]
        for entry in range(texture["palette_entries"]):
            palette.append(_rgba5551(struct.unpack_from(
                ">H", payload, palette_offset + 2 * entry)[0]))
    pixels = []
    for index in range(width * height):
        if bits == 4:
            byte = payload[base + index // 2]
            value = (byte >> 4) if (index % 2 == 0) else (byte & 0xF)
            if fmt == G_IM_FMT_CI:
                pixels.append(palette[value])
            elif fmt == G_IM_FMT_I:
                level = value * 0x11
                pixels.append((level, level, level, level))
            else:                                   # IA4: 3-bit I, 1-bit A
                level = ((value >> 1) & 7) * 255 // 7
                pixels.append((level, level, level, (value & 1) * 255))
        elif bits == 8:
            byte = payload[base + index]
            if fmt == G_IM_FMT_CI:
                pixels.append(palette[byte])
            elif fmt == G_IM_FMT_I:
                pixels.append((byte, byte, byte, byte))
            else:                                   # IA8: 4-bit I, 4-bit A
                level = (byte >> 4) * 0x11
                pixels.append((level, level, level, (byte & 0xF) * 0x11))
        elif bits == 16:
            if fmt == G_IM_FMT_IA:                  # IA16: 8-bit I, 8-bit A
                level, alpha = payload[base + 2 * index: base + 2 * index + 2]
                pixels.append((level, level, level, alpha))
            else:
                pixels.append(_rgba5551(struct.unpack_from(
                    ">H", payload, base + 2 * index)[0]))
        else:
            red, green, blue, alpha = payload[base + 4 * index:
                                              base + 4 * index + 4]
            pixels.append((red, green, blue, alpha))
    return pixels


def _rgba5551(value: int) -> tuple[int, int, int, int]:
    return (((value >> 11) & 31) * 255 // 31,
            ((value >> 6) & 31) * 255 // 31,
            ((value >> 1) & 31) * 255 // 31,
            255 if (value & 1) else 0)


# --------------------------------------------------------------------------
# reachability
# --------------------------------------------------------------------------
def strip_comments_and_literals(text: str) -> str:
    out = []
    index, size = 0, len(text)
    while index < size:
        char = text[index]
        if char == "/" and text.startswith("//", index):
            newline = text.find("\n", index)
            index = size if newline < 0 else newline
        elif char == "/" and text.startswith("/*", index):
            end = text.find("*/", index + 2)
            index = size if end < 0 else end + 2
            out.append(" ")
        elif char in "\"'":
            quote, cursor = char, index + 1
            while cursor < size and text[cursor] != quote:
                cursor += 2 if text[cursor] == "\\" else 1
            out.append(" ")
            index = cursor + 1
        else:
            out.append(char)
            index += 1
    return "".join(out)


def split_arguments(text: str) -> list[str]:
    arguments, depth, start = [], 0, 0
    for index, char in enumerate(text):
        if char in "([":
            depth += 1
        elif char in ")]":
            depth -= 1
        elif char == "," and depth == 0:
            arguments.append(text[start:index])
            start = index + 1
    arguments.append(text[start:])
    return [argument.strip() for argument in arguments]


def find_calls(text: str, name: str) -> list[list[str]]:
    calls = []
    cursor = 0
    token = name
    while True:
        found = text.find(token, cursor)
        if found < 0:
            return calls
        cursor = found + len(token)
        if found and (text[found - 1].isalnum() or text[found - 1] == "_"):
            continue
        open_paren = cursor
        while open_paren < len(text) and text[open_paren] in " \t\n":
            open_paren += 1
        if open_paren >= len(text) or text[open_paren] != "(":
            continue
        depth, index = 1, open_paren + 1
        while index < len(text) and depth:
            depth += text[index] == "("
            depth -= text[index] == ")"
            index += 1
        calls.append(split_arguments(text[open_paren + 1:index - 1]))


def parse_integer(token: str):
    token = token.strip().rstrip("uUlL")
    try:
        return int(token, 0)
    except ValueError:
        return None


def function_body(text: str, name: str):
    cursor = 0
    while True:
        found = text.find(name, cursor)
        if found < 0:
            return None
        cursor = found + len(name)
        if found and (text[found - 1].isalnum() or text[found - 1] == "_"):
            continue
        brace = text.find("{", cursor)
        paren = text.find("(", cursor)
        if paren < 0 or brace < 0 or paren > brace:
            continue
        if ";" in text[cursor:brace]:
            continue
        depth, index = 0, brace
        while index < len(text):
            depth += text[index] == "{"
            depth -= text[index] == "}"
            if depth == 0:
                return text[found:index + 1]
            index += 1
        return None


def byte_arrays(text: str) -> dict[str, list[int]]:
    """Every `u8 name[...] = { literals };` in the translation unit."""
    arrays: dict[str, list[int]] = {}
    cursor = 0
    while True:
        found = text.find("u8 ", cursor)
        if found < 0:
            return arrays
        cursor = found + 3
        if found and (text[found - 1].isalnum() or text[found - 1] == "_"):
            continue
        bracket = text.find("[", cursor)
        if bracket < 0:
            continue
        name = text[cursor:bracket].strip()
        if not name.isidentifier():
            continue
        close = text.find("]", bracket)
        equals = text.find("=", close)
        brace = text.find("{", close)
        end = text.find("}", brace)
        if -1 in (close, equals, brace, end) or equals > brace:
            continue
        values = [parse_integer(item)
                  for item in text[brace + 1:end].split(",") if item.strip()]
        if values and all(value is not None for value in values):
            arrays[name] = values


def resolve_script_ids(expression: str, arrays: dict[str, list[int]],
                       where: str) -> set[int]:
    expression = expression.strip()
    value = parse_integer(expression)
    if value is not None:
        return {value}
    if "?" in expression and ":" in expression:
        question = expression.index("?")
        depth = 0
        for index in range(question + 1, len(expression)):
            if expression[index] in "([":
                depth += 1
            elif expression[index] in ")]":
                depth -= 1
            elif expression[index] == ":" and depth == 0:
                return (resolve_script_ids(expression[question + 1:index],
                                           arrays, where)
                        | resolve_script_ids(expression[index + 1:],
                                             arrays, where))
    bracket = expression.find("[")
    if bracket > 0:
        name = expression[:bracket].strip()
        if name in arrays:
            return set(arrays[name])
    raise SystemExit(f"{where}: cannot resolve script id `{expression}`")


def _called_ef_functions(body: str) -> list[str]:
    """`ef*` identifiers this body calls, for the intra-module call closure."""
    names, index = [], 0
    while True:
        found = body.find("ef", index)
        if found < 0:
            return names
        index = found + 2
        if found and (body[found - 1].isalnum() or body[found - 1] == "_"):
            continue
        end = found
        while end < len(body) and (body[end].isalnum() or body[end] == "_"):
            end += 1
        cursor = end
        while cursor < len(body) and body[cursor] in " \t\n":
            cursor += 1
        if cursor < len(body) and body[cursor] == "(":
            names.append(body[found:end])
        index = end


def derive_reachable_scripts(repo_root: Path, scripts: list[dict]) -> dict:
    """Close the P1 effect seams over the bank's own spawn graph."""
    text = strip_comments_and_literals(
        (repo_root / EFMANAGER).read_text(encoding="utf-8", errors="replace")
    )
    arrays = byte_arrays(text)
    seams = sorted((census.SUBSTITUTES | P1_EXTRA_SEAMS | P1_PARTICLE_SEAMS)
                   - census.SKIPPED_OVERRIDES)
    seeds: dict[int, list[str]] = {}
    helpers: dict[str, list[str]] = {}
    for seam in seams:
        # A seam may reach the bank through an efmanager-internal helper --
        # efManagerDamageSpawnOrbsRandomMakeEffect just forwards to
        # efManagerDamageSpawnOrbsMakeEffect -- so close over the module's own
        # call graph instead of reading one body.
        visited: set[str] = set()
        pending = [seam]
        while pending:
            name = pending.pop()
            if name in visited:
                continue
            visited.add(name)
            body = function_body(text, name)
            if body is None:
                if name == seam:
                    raise SystemExit(f"{EFMANAGER}: no body for P1 seam {seam}")
                continue
            if name != seam:
                helpers.setdefault(seam, []).append(name)
            for constructor, argument in SCRIPT_CONSTRUCTORS.items():
                for call in find_calls(body, constructor):
                    if len(call) <= argument:
                        continue
                    if EFCOMMON_BANK_TOKEN not in call[argument - 1]:
                        continue
                    for script_id in resolve_script_ids(
                            call[argument], arrays,
                            f"{seam}->{name}/{constructor}"):
                        seeds.setdefault(script_id, []).append(seam)
            for callee in sorted(set(_called_ef_functions(body)) - visited):
                pending.append(callee)
    if not seeds:
        raise SystemExit(f"{EFMANAGER}: P1 seams named no efcommon script")
    unknown = sorted(sid for sid in seeds if sid >= len(scripts))
    if unknown:
        raise SystemExit(f"P1 seams name out-of-range scripts {unknown}")

    reachable: set[int] = set()
    pending = sorted(seeds)
    spawn_edges: dict[int, list[int]] = {}
    while pending:
        script_id = pending.pop()
        if script_id in reachable:
            continue
        reachable.add(script_id)
        children = sorted(spawned_scripts(scripts[script_id], len(scripts)))
        if children:
            spawn_edges[script_id] = children
        pending.extend(children)
    return {
        "seams": seams,
        "seam_helpers": {seam: sorted(set(names))
                         for seam, names in helpers.items()},
        "seeds": {sid: sorted(set(names)) for sid, names in seeds.items()},
        "reachable": sorted(reachable),
        "spawn_edges": spawn_edges,
    }


# --------------------------------------------------------------------------
# DS texel conversion
# --------------------------------------------------------------------------
def expand_5bit(value: int) -> int:
    """The one 5-bit -> 8-bit expansion. Source decode, palette fitting and the
    decode-back check must all use it or the self-check reports phantom error."""
    return value * 255 // 31


def quantise_channel_5bit(value: int) -> int:
    return expand_5bit((value * 31 + 127) // 255)


def to_bgr555(colour: tuple[int, int, int]) -> int:
    return (((colour[0] * 31 + 127) // 255)
            | (((colour[1] * 31 + 127) // 255) << 5)
            | (((colour[2] * 31 + 127) // 255) << 10))


def build_palette(colours: Counter, budget: int) -> list[tuple[int, int, int]]:
    """Deterministic weighted k-means over 5-bit-quantised source colours."""
    unique = sorted({tuple(quantise_channel_5bit(channel)
                           for channel in colour) for colour in colours})
    if not unique:
        return []
    weights: Counter = Counter()
    for colour, count in colours.items():
        weights[tuple(quantise_channel_5bit(channel)
                      for channel in colour)] += count
    if len(unique) <= budget:
        return unique
    centres = [max(unique, key=lambda colour: (weights[colour], colour))]
    while len(centres) < budget:
        best, best_distance = None, -1
        for colour in unique:
            distance = min(sum((a - b) ** 2 for a, b in zip(colour, centre))
                           for centre in centres)
            if distance > best_distance or (distance == best_distance
                                            and colour < best):
                best, best_distance = colour, distance
        if best_distance <= 0:
            break
        centres.append(best)
    for _round in range(32):
        buckets: list[list[tuple[int, int, int]]] = [[] for _ in centres]
        for colour in unique:
            index = min(range(len(centres)),
                        key=lambda i: (sum((a - b) ** 2 for a, b
                                           in zip(colour, centres[i])), i))
            buckets[index].append(colour)
        moved = False
        for index, bucket in enumerate(buckets):
            if not bucket:
                continue
            total = sum(weights[colour] for colour in bucket)
            centre = tuple(
                quantise_channel_5bit(
                    round(sum(colour[channel] * weights[colour]
                              for colour in bucket) / total))
                for channel in range(3)
            )
            if centre != centres[index]:
                centres[index] = centre
                moved = True
        if not moved:
            break
    return sorted(set(centres))


def quantise_alpha(alpha: int, levels: int) -> int:
    if levels == 2:
        return 255 if alpha >= 128 else 0
    step = levels - 1
    return (alpha * step + 127) // 255 * 255 // step


def evaluate_format(frames: list[list[tuple[int, int, int, int]]],
                    ds_format: int):
    """Return (palette, mean_error, max_error) in premultiplied 0..255 units."""
    max_colours, alpha_levels = DS_FORMAT_CAPS[ds_format]
    histogram: Counter = Counter()
    for frame in frames:
        histogram.update(frame)
    opaque: Counter = Counter()
    for (red, green, blue, alpha), count in histogram.items():
        if alpha:
            opaque[(red, green, blue)] += count
    palette = ([] if ds_format == DS_DIRECT16
               else build_palette(opaque, max_colours))
    if len(palette) > max_colours:
        return None
    total = error_sum = 0
    error_max = 0.0
    for (red, green, blue, alpha), count in histogram.items():
        packed_alpha = quantise_alpha(alpha, alpha_levels)
        if ds_format == DS_DIRECT16:
            colour = tuple(quantise_channel_5bit(c)
                           for c in (red, green, blue))
        elif packed_alpha == 0 and ds_format in DS_PALETTE_FORMATS:
            colour = (0, 0, 0)
        elif palette:
            colour = min(palette,
                         key=lambda entry: sum((a - b) ** 2 for a, b in
                                               zip(entry, (red, green, blue))))
        else:
            colour = (0, 0, 0)
        error = max(abs(red * alpha - colour[0] * packed_alpha),
                    abs(green * alpha - colour[1] * packed_alpha),
                    abs(blue * alpha - colour[2] * packed_alpha)) / 255.0
        error_sum += error * count
        error_max = max(error_max, error)
        total += count
    return palette, error_sum / total, error_max


def choose_ds_format(texture: dict,
                     frames: list[list[tuple[int, int, int, int]]]):
    """Cheapest DS format that keeps the source's alpha resolution.

    Graded source alpha may only land in A5I3/A3I5; the palette formats carry
    one transparency bit and would flatten a particle's soft edge into a hard
    stencil. Within the surviving candidates the smallest image wins, and
    measured premultiplied error breaks ties -- which is what puts the
    RGBA32 explosion frames in A3I5 rather than the 48-bytes-cheaper A5I3.

    A palette format whose capacity is smaller than the source's own colour
    count is refused outright, for the same reason graded alpha refuses the
    palette formats: it is a resolution loss, not a rounding error. Without
    that test the "no candidate is exact, so take the cheapest" fallback picks
    the WORST survivor -- when the P1 seam list grew on 2026-07-31 it put two
    six- and four-grey IA4 sources (textures 9 and 22) into three-colour PAL4
    at max error 28, where one step up to PAL16 costs 128 bytes each of NitroFS
    payload and lands on 4, the irreducible BGR555 rounding floor.
    """
    alphas = {pixel[3] for frame in frames for pixel in frame}
    graded = not alphas <= {0, 255}
    candidates = DS_ALPHA_FORMATS if graded else \
        (DS_PAL4, DS_PAL16, DS_PAL256, DS_DIRECT16)
    # Counted after 5-bit quantisation: two source colours that land on the same
    # BGR555 entry are merged by the hardware's colour depth, not by the format
    # choice, and charging that to the palette would over-promote every texture.
    needed = len({tuple(quantise_channel_5bit(channel) for channel in pixel[:3])
                  for frame in frames for pixel in frame if pixel[3]})
    results = []
    for ds_format in candidates:
        if ds_format in DS_PALETTE_FORMATS and \
                DS_FORMAT_CAPS[ds_format][0] < needed:
            continue
        evaluated = evaluate_format(frames, ds_format)
        if evaluated is None:
            continue
        palette, mean_error, max_error = evaluated
        image_bytes = (texture["width"] * texture["height"]
                       * DS_FORMAT_BITS[ds_format] // 8)
        results.append((DS_FORMAT_BITS[ds_format], ds_format, palette,
                        mean_error, max_error,
                        image_bytes * texture["frames"]))
    if not results:
        raise SystemExit(f"texture {texture['id']}: no DS format fits")
    exact = [row for row in results if row[4] == 0.0]
    pool = exact or results
    cheapest_bits = min(row[0] for row in pool)
    pool = [row for row in pool if row[0] == cheapest_bits]
    return min(pool, key=lambda row: (row[3], row[4], -row[1]))


def encode_frame(pixels: list[tuple[int, int, int, int]], ds_format: int,
                 palette: list[tuple[int, int, int]], width: int) -> bytes:
    bits = DS_FORMAT_BITS[ds_format]
    if width * bits % 8:
        raise SystemExit(f"width {width} is not a whole number of bytes "
                         f"in {DS_FORMAT_NAMES[ds_format]}")
    _max_colours, alpha_levels = DS_FORMAT_CAPS[ds_format]
    base = 1 if ds_format in DS_PALETTE_FORMATS else 0
    out = bytearray()
    accumulator = shift = 0
    for red, green, blue, alpha in pixels:
        if ds_format == DS_DIRECT16:
            value = to_bgr555((red, green, blue))
            out += struct.pack("<H", value | (0x8000 if alpha >= 128 else 0))
            continue
        if ds_format in DS_PALETTE_FORMATS and alpha < 128:
            index = 0
        else:
            index = base + min(
                range(len(palette)),
                key=lambda i: (sum((a - b) ** 2 for a, b in
                                   zip(palette[i], (red, green, blue))), i)
            )
        if ds_format == DS_A3I5:
            out.append(((alpha * 7 + 127) // 255 << 5) | (index & 0x1F))
        elif ds_format == DS_A5I3:
            out.append(((alpha * 31 + 127) // 255 << 3) | (index & 0x07))
        elif ds_format == DS_PAL256:
            out.append(index)
        else:
            accumulator |= (index & ((1 << bits) - 1)) << shift
            shift += bits
            if shift == 8:
                out.append(accumulator)
                accumulator = shift = 0
    if shift:
        raise SystemExit("frame did not pack into whole bytes")
    if len(out) != len(pixels) * bits // 8:
        raise SystemExit("encoded frame size mismatch")
    return bytes(out)


def decode_ds_frame(data: bytes, ds_format: int, palette_entries: list[int],
                    count: int) -> list[tuple[int, int, int, int]]:
    """Decode packed DS texels back to RGBA8888.

    The encoder is only trustworthy if reading its own output reproduces the
    error the format chooser measured, so build_pack decodes every frame it
    emits and compares. A wrong nibble order or alpha shift moves that number.
    """
    bits = DS_FORMAT_BITS[ds_format]
    colours = [(expand_5bit(entry & 31), expand_5bit((entry >> 5) & 31),
                expand_5bit((entry >> 10) & 31)) for entry in palette_entries]
    pixels = []
    for index in range(count):
        if ds_format == DS_DIRECT16:
            value = struct.unpack_from("<H", data, 2 * index)[0]
            pixels.append((expand_5bit(value & 31),
                           expand_5bit((value >> 5) & 31),
                           expand_5bit((value >> 10) & 31),
                           255 if (value & 0x8000) else 0))
            continue
        if bits == 8:
            byte = data[index]
        else:
            per_byte = 8 // bits
            byte = (data[index // per_byte]
                    >> (bits * (index % per_byte))) & ((1 << bits) - 1)
        if ds_format == DS_A3I5:
            alpha = (byte >> 5) * 255 // 7
            colour = colours[byte & 0x1F]
        elif ds_format == DS_A5I3:
            alpha = (byte >> 3) * 255 // 31
            colour = colours[byte & 0x07]
        else:
            alpha = 0 if byte == 0 else 255
            colour = (0, 0, 0) if byte == 0 else colours[byte]
        pixels.append((colour[0], colour[1], colour[2], alpha))
    return pixels


def measure_error(source: list[list[tuple[int, int, int, int]]],
                  decoded: list[list[tuple[int, int, int, int]]]):
    total = 0
    error_sum = 0.0
    error_max = 0.0
    for source_frame, decoded_frame in zip(source, decoded):
        for (red, green, blue, alpha), (dr, dg, db, da) in zip(source_frame,
                                                               decoded_frame):
            error = max(abs(red * alpha - dr * da),
                        abs(green * alpha - dg * da),
                        abs(blue * alpha - db * da)) / 255.0
            error_sum += error
            error_max = max(error_max, error)
            total += 1
    return error_sum / total, error_max


# --------------------------------------------------------------------------
# pack
# --------------------------------------------------------------------------
def shelf_pack(cells: list[dict], width: int, height: int,
               sheets: int = 1):
    """Shelf-pack fixed cells top-down. Returns placements, or None if it fails.

    Every cell here is a power of two on both axes (16x8 up to 64x64), which is
    what makes a shelf packer optimal rather than merely convenient: sorted by
    descending height, each shelf is filled by cells of exactly its height, so
    the only waste is the tail of a row.

    `sheets` is how many SEPARATE sheets of width x height may be used, and a
    placement is (sheet, x, y). Overflowing one sheet starts the next rather
    than failing; only running out of sheets fails. See QUAD_ATLAS_SHEETS_MAX
    for why the sheet count is the free variable and the sheet SIZE is not.
    """
    order = sorted(range(len(cells)),
                   key=lambda i: (-cells[i]["h"], -cells[i]["w"], i))
    placed = {}
    sheet = 0
    shelf_y = 0
    shelf_h = 0
    cursor_x = 0
    for index in order:
        cell = cells[index]
        if (cell["w"] > width) or (cell["h"] > height):
            return None
        if (shelf_h != cell["h"]) or (cursor_x + cell["w"] > width):
            if shelf_h != cell["h"] or cursor_x != 0:
                shelf_y += shelf_h
            shelf_h = cell["h"]
            cursor_x = 0
        if shelf_y + cell["h"] > height:
            sheet += 1
            if sheet >= sheets:
                return None
            shelf_y = 0
            shelf_h = cell["h"]
            cursor_x = 0
        placed[index] = (sheet, cursor_x, shelf_y)
        cursor_x += cell["w"]
    return placed


def build_quad_sheet(textures: list[dict], report_rows: list[dict],
                     frames_by_texture: dict[int, list[list]],
                     extra_candidates: list[dict] | None = None) -> dict:
    """The DS draw path's payload: one 64x64 RGB555+A1 atlas.

    A SECOND encoding of the same texels, on purpose. The pack above chooses a
    format per texture by measured error and gets the whole reachable set into
    137,152 bytes, which is right for a payload that only has to exist -- but
    those formats are paletted (PAL4/PAL16/PAL256) or alpha-indexed
    (A3I5/A5I3), and the renderer's texture cache uploads GL_RGBA and has no
    palette slot in its key. Teaching it palettes to save bytes we are not
    short of would be the expensive way round.

    One texture per frame would exhaust GL names and break the triangle batch.
    Any second or larger allocation was measured unsafe. The KO-only frames are
    therefore box-averaged to 8x8 so their exact closure fits in the 8 KiB sheet
    already proven by the stage-cache gate.

    Admission is by ascending texture size, keeping each candidate only if the
    whole set still packs -- the packer decides, not a byte budget, because
    shelf waste is real and a byte count would admit a set that cannot be laid
    out.

    Excluded textures are NAMED in the report rather than silently dropped: the
    runtime fails closed on a missing texture, so a BUGS.md row that needs one
    of the big multi-frame animations back reads the exclusion list, and the
    honest answer for those is halving 64x64x10 rather than growing the atlas.
    """
    def build_candidates(rung: int) -> list[dict]:
        rows = []
        for report in report_rows:
            if not report["packed"]:
                continue
            texture = textures[report["texture"]]
            cell_max = QUAD_CELL_MAX
            if texture["id"] in QUAD_KO_LIVE:
                cell_max = QUAD_KO_CELL_MAX
            elif texture["frames"] >= QUAD_LONG_ANIMATION_FRAMES:
                cell_max = QUAD_LONG_ANIMATION_CELL_MAX
            # The ladder scales the whole sheet together rather than one class of
            # texture, so a rung is a single readable statement about the atlas
            # ("everything at source", "everything at half") instead of three
            # interacting caps. The per-class caps still bound their own rows.
            cell_max = min(cell_max, rung)
            cell_w, cell_h = quad_cell_dims(texture["width"],
                                            texture["height"], cell_max)
            frame_list = quad_frame_list(texture["frames"])
            rows.append({
                "texture": texture["id"],
                "width": cell_w,
                "height": cell_h,
                "source_width": texture["width"],
                "source_height": texture["height"],
                "frames": texture["frames"],
                "frame_list": frame_list,
                "packed_frames": len(frame_list),
                "bytes": cell_w * cell_h * len(frame_list),
            })
        return rows

    # Dream Land's leaves and dust are measured-live too -- a 2026-08-01
    # both-CPU soak caught the game asking for scripts 0 and 1 of that bank and
    # being refused -- so they sort with the common bank's live set rather than
    # behind it. They are 16x16x4 = 1,024 texels against the 1,408 the common
    # set leaves free.
    live = set(QUAD_MEASURED_LIVE)
    for candidate in (extra_candidates or ()):
        if candidate.get("live"):
            live.add(candidate["texture"])

    def candidate_set(rung: int) -> list[dict]:
        rows = build_candidates(rung)
        rows.extend(dict(row) for row in (extra_candidates or ()))
        # Measured-live first, then ascending size. Admission is greedy and the
        # sheet is now half what it was, so "smallest first" alone would fill it
        # with whatever happens to be small and let a texture a match actually
        # draws fall off the end.
        #
        # Ranked on SOURCE cost, not on the cell or the capped cost, so the
        # admission order is the same at every (cell cap, frame cap) the search
        # below tries. Ranking on the trial cost would reshuffle priorities as
        # the search moved and make one setting's result depend on the ceiling
        # it started from -- a very quiet way for the sheet to change when
        # somebody edits an unrelated constant.
        rows.sort(key=lambda row: (row["texture"] not in live,
                                   row.get("source_width", row["width"]) *
                                   row.get("source_height", row["height"]) *
                                   row["frames"], row["texture"]))
        return rows

    def cells_for(rows):
        cells = []
        for row in rows:
            # SOURCE frame indices, which may be a decimated subset -- see
            # quad_frame_list and the runtime's nearest-earlier lookup. Rows
            # without the key are the Pupupu extras, which are short enough
            # that the cap never bites.
            frame_list = row.get("frame_list")
            if frame_list is None:
                frame_list = quad_frame_list(row["frames"])
            for frame in frame_list:
                cells.append({"w": row["width"], "h": row["height"],
                              "src_w": row.get("source_width", row["width"]),
                              "src_h": row.get("source_height", row["height"]),
                              "texture": row["texture"], "frame": frame,
                              # Lookup key vs texels -- the same for every
                              # texture but the one QUAD_HELD_FRAME names.
                              "source": quad_cell_source_frame(
                                  row["texture"], row["frames"], frame_list,
                                  frame)})
        return cells

    def admit_at(rows: list[dict], cap: int) -> tuple[list[dict], list[dict]]:
        """Greedy admission with every animation decimated to `cap` frames."""
        admitted: list[dict] = []
        excluded: list[dict] = []
        for candidate in rows:
            trial = dict(candidate)
            trial["frame_list"] = quad_frame_list(candidate["frames"], cap)
            trial["packed_frames"] = len(trial["frame_list"])
            trial["bytes"] = (candidate["width"] * candidate["height"] *
                              trial["packed_frames"])
            if shelf_pack(cells_for(admitted + [trial]), QUAD_ATLAS_WIDTH,
                          QUAD_ATLAS_HEIGHT, QUAD_ATLAS_SHEETS_MAX) is None:
                excluded.append(trial)
            else:
                admitted.append(trial)
        return admitted, excluded

    # SELF-TUNING, and the reason is that a hand-picked setting is a number
    # somebody has to re-derive every time an effect is added. The constraint is
    # not byte count -- at cap 3 the sheet had 896 free bytes and STILL could not
    # seat texture 66's four 16x16 cells, because shelf packing groups by exact
    # height and the 16-tall shelves were full. So the only honest way to choose
    # is to try the good settings and keep the best that refuses nothing live.
    #
    # RESOLUTION IS TRIED BEFORE FRAME RATE, and that order is the owner's, not a
    # preference. The 2026-08-02 decimation pass got every missing effect drawing
    # and his verdict on the row was "better but looks EXTREMELY PIXELATED and low
    # quality" -- so when two settings both seat the whole live set, the sheet is
    # better spent on texels than on frames. Swept on the host before any ROM
    # (scratchpad atlas_policy_sweep), inside the unchanged 8,192-byte sheet:
    #
    #   long cell  cap  admitted  texels  named effects
    #           8    2        33   8,000  DustDash/DamageFire/DamageNormal* 8x8
    #          16    1        32   6,592  ...all of them 16x16
    #
    # i.e. FOUR TIMES the texels per frame on exactly the effects BUGS.md names
    # ("running foot dust VFX, fireball hit VFX ..."), for one frame instead of
    # two. No cap from 6 down to 2 seats the live set at 16x16, so the choice is
    # genuinely 8x8x2f against 16x16x1f and not a free lunch: what is spent is the
    # animation, which PROJECT_GOAL.md allows in as many words ("static
    # substitutes for expensive animation", "reduced animation update rates"),
    # and what is bought is the resolution the owner actually complained about.
    # Two frames of a ten-frame animation was a strobe between frame 0 and frame
    # 9, not motion.
    #
    # The KO cap stays 8: at 16 the sheet drops texture 25 (DamageCoin), which is
    # live. If the sheet ever grows, this loop takes the better setting on its own
    # without anyone editing a constant.
    admitted, excluded = [], []
    chosen_cap = QUAD_FRAME_CAP
    chosen_rung = QUAD_CELL_LADDER[0]
    for rung in QUAD_CELL_LADDER:
        rows = candidate_set(rung)
        seated = False
        for cap in range(QUAD_FRAME_CAP, 0, -1):
            admitted, excluded = admit_at(rows, cap)
            chosen_cap = cap
            chosen_rung = rung
            if not any(row["texture"] in live for row in excluded):
                seated = True
                break
        if seated:
            break
    admitted.sort(key=lambda row: row["texture"])

    cells = cells_for(admitted)
    placement = shelf_pack(cells, QUAD_ATLAS_WIDTH, QUAD_ATLAS_HEIGHT,
                           QUAD_ATLAS_SHEETS_MAX)
    if placement is None:
        raise SystemExit("quad atlas failed to pack its own admitted set")
    sheets_used = max(sheet for sheet, _, _ in placement.values()) + 1
    # NDSParticleQuadFrame.sheet is a u8 and the runtime indexes a fixed array
    # with it, so an out-of-range sheet would bind another sheet's texels under
    # this cell's coordinates rather than fail. Cheap to assert here; impossible
    # to see on screen.
    if not (0 < sheets_used <= QUAD_ATLAS_SHEETS_MAX) or sheets_used > 255:
        raise SystemExit(f"quad atlas packed into {sheets_used} sheets, "
                         f"outside 1..{QUAD_ATLAS_SHEETS_MAX}")

    # Two passes: the shared palette has to see every admitted texel first.
    box_averaged = []
    for cell in cells:
        pixels = frames_by_texture[cell["texture"]][cell["source"]]
        if len(pixels) != cell["src_w"] * cell["src_h"]:
            raise SystemExit(
                f"quad atlas texture {cell['texture']} frame {cell['source']} "
                f"has {len(pixels)} pixels, expected "
                f"{cell['src_w'] * cell['src_h']}")
        step_x = cell["src_w"] // cell["w"]
        step_y = cell["src_h"] // cell["h"]
        grid = []
        for row in range(cell["h"]):
            for column in range(cell["w"]):
                red = green = blue = alpha = 0
                for sub_y in range(step_y):
                    source = ((row * step_y + sub_y) * cell["src_w"] +
                              column * step_x)
                    for sub_x in range(step_x):
                        texel = pixels[source + sub_x]
                        red += texel[0]
                        green += texel[1]
                        blue += texel[2]
                        alpha += texel[3]
                taps = step_x * step_y
                grid.append((red // taps, green // taps, blue // taps,
                             alpha // taps))
        box_averaged.append(grid)

    # One palette for the whole sheet. Weight by texel so a texture that covers
    # more of the atlas has more say, and ignore fully transparent texels --
    # their colour is arbitrary and would otherwise pull an entry towards black.
    colours = Counter()
    for grid in box_averaged:
        for red, green, blue, alpha in grid:
            if alpha > 0:
                colours[(quantise_channel_5bit(red),
                         quantise_channel_5bit(green),
                         quantise_channel_5bit(blue))] += 1
    palette = build_palette(colours, QUAD_ATLAS_PALETTE_ENTRIES)
    if not palette:
        palette = [(255, 255, 255)]

    # Sheets are concatenated in order, so sheet N starts at
    # N * QUAD_SHEET_BUDGET_BYTES and the runtime reads one sheet per upload
    # without any table.
    atlas = bytearray(QUAD_SHEET_BUDGET_BYTES * sheets_used)
    frame_rows = []
    for index, cell in enumerate(cells):
        sheet, origin_x, origin_y = placement[index]
        encoded = encode_frame(box_averaged[index], DS_A3I5, palette,
                               cell["w"])
        sheet_base = sheet * QUAD_SHEET_BUDGET_BYTES
        for row in range(cell["h"]):
            base = sheet_base + (origin_y + row) * QUAD_ATLAS_WIDTH + origin_x
            atlas[base:base + cell["w"]] = \
                encoded[row * cell["w"]:(row + 1) * cell["w"]]
        frame_rows.append({
            "texture": cell["texture"], "frame": cell["frame"],
            "sheet": sheet,
            "x": origin_x, "y": origin_y, "w": cell["w"], "h": cell["h"],
        })
    frame_rows.sort(key=lambda row: (row["texture"], row["frame"]))
    used = sum(row["bytes"] for row in admitted)
    palette_payload = b"".join(
        struct.pack("<H", to_bgr555(colour)) for colour in palette)
    return {
        "payload": bytes(atlas) + palette_payload,
        "admitted": admitted,
        "excluded": excluded,
        "frame_cap": chosen_cap,
        "cell_cap": chosen_rung,
        "frames": frame_rows,
        "held_frames": {str(cell["texture"]): cell["source"]
                        for cell in cells if cell["source"] != cell["frame"]},
        "bytes": used,
        "atlas_bytes": len(atlas),
        "palette_offset": len(atlas),
        "palette_entries": len(palette),
        "width": QUAD_ATLAS_WIDTH,
        "height": QUAD_ATLAS_HEIGHT,
        "sheets": sheets_used,
        "sheet_bytes": QUAD_SHEET_BUDGET_BYTES,
        "budget_bytes": QUAD_SHEET_BUDGET_BYTES * sheets_used,
    }


def build_pupupu_bank(repo_root: Path,
                      frames_by_texture: dict[int, list[list]]) -> dict:
    """Dream Land's own particle bank, packed whole.

    Whole rather than reachability-closed on purpose: the bank is 416 bytes of
    bytecode over five scripts, so closing it over its spawn graph would cost
    more analysis than it can possibly save, and a script the closure got wrong
    would fail closed and be invisible. `derive_reachable_scripts` exists for
    the common bank because that one is 10,912 bytes over 119 scripts.

    The two the wind row needs are 0 (`grPupupuWhispyLeavesMakeEffect`) and 1
    (`grPupupuWhispyDustMakeEffect`), both drawing TEXTURE 2 -- 16x16, four
    frames. Their cells are handed to the shared quad sheet under
    PUPUPU_QUAD_TEXTURE_STRIDE so one frame table still answers both banks.
    """
    script_payload = load_o2r_blob(repo_root, *PUPUPU_SCRIPT_BANK)
    texture_payload = load_o2r_blob(repo_root, *PUPUPU_TEXTURE_BANK)
    scripts = parse_script_bank(script_payload)
    textures = parse_texture_bank(texture_payload)

    wanted = sorted({script["texture_id"] for script in scripts})
    out_of_range = [tid for tid in wanted if tid >= len(textures)]
    if out_of_range:
        raise SystemExit(f"pupupu scripts name absent textures {out_of_range}")

    live_textures = ({script["texture_id"] for script in scripts
                      if script["id"] in PUPUPU_MEASURED_LIVE_SCRIPTS} |
                     set(PUPUPU_MEASURED_LIVE_TEXTURES))
    quad_candidates = []
    for texture in textures:
        if texture["id"] not in wanted or texture["frames"] <= 0:
            continue
        frames = [decode_texture_frame(texture_payload, texture, frame)
                  for frame in range(texture["frames"])]
        key = PUPUPU_QUAD_TEXTURE_STRIDE + texture["id"]
        frames_by_texture[key] = frames
        cell_w, cell_h = quad_cell_dims(texture["width"], texture["height"])
        # Same decimation the common bank gets, and `bytes` has to agree with it
        # or the admission sort ranks this candidate by a cost it does not pay.
        # It read the full frame count until 2026-08-02, which over-priced the
        # four-frame Pupupu sheet and sorted it last among the live set.
        frame_list = quad_frame_list(texture["frames"])
        quad_candidates.append({
            "texture": key,
            "width": cell_w,
            "height": cell_h,
            "source_width": texture["width"],
            "source_height": texture["height"],
            "frames": texture["frames"],
            "frame_list": frame_list,
            "packed_frames": len(frame_list),
            "bytes": cell_w * cell_h * len(frame_list),
            "live": texture["id"] in live_textures,
        })

    return {
        "script_payload": script_payload,
        "offsets": [script["offset"] for script in scripts],
        "scripts": scripts,
        "textures": textures,
        "texture_rows": [(texture["width"], texture["height"],
                          texture["frames"]) for texture in textures],
        "quad_candidates": quad_candidates,
        "wanted": wanted,
    }


def decode_source_asset_texels(payload: bytes, asset: dict
                               ) -> list[tuple[int, int, int, int]]:
    """One N64 texture out of a reloc file, as RGBA the sheet encoder accepts.

    ALL SHAPE GOES IN ALPHA; THE COLOUR IS A FLAT WHITE. That is not a shortcut,
    it is what A5I3 is for: five bits of alpha per texel against THREE bits of
    index into ONE eight-entry palette shared by the entire sheet. Both of these
    assets are greyscale on the N64 and are tinted at draw time, so encoding
    their intensity as colour makes them compete for palette entries against a
    sheet full of fire and sparks -- and lose.

    Measured, first attempt: the halo's grey ramp put 88 of its 128 texels on
    palette index 0, r24 g32 b16, near black, and the owner reported the respawn
    platform as *invisible*. The shield spread across three entries and read
    muddy. Encoding white and letting the quad's own colour do the tinting puts
    every texel on the brightest entry and spends the whole 5-bit alpha channel
    on the thing that actually varies.

    * IA8 -- one byte per texel, 4 bits intensity then 4 bits alpha. Both matter
      and both are coverage-like once the colour is gone, so they multiply: the
      bubble's soft rim lives in its intensity ramp and would be lost if only
      the alpha nibble survived.
    * I4  -- one nibble per texel, high nibble first, no alpha channel at all.
      Intensity IS the coverage for an additive glow.

    Nibbles expand by *17, which maps 0..15 onto 0..255 exactly and is the same
    expansion the pack path uses elsewhere.
    """
    width = asset["width"]
    height = asset["height"]
    count = width * height
    pixels: list[tuple[int, int, int, int]] = []

    if asset["format"] == "ia8":
        raw = payload[asset["offset"]:asset["offset"] + count]
        if len(raw) != count:
            raise SystemExit(
                f"{asset['name']}: wanted {count} IA8 bytes at "
                f"0x{asset['offset']:x}, file holds {len(raw)}")
        for byte in raw:
            intensity = (byte >> 4) * 17
            coverage = (byte & 0xF) * 17
            pixels.append((255, 255, 255, (intensity * coverage) // 255))
    elif asset["format"] == "i4":
        raw = payload[asset["offset"]:asset["offset"] + (count // 2)]
        if len(raw) != (count // 2):
            raise SystemExit(
                f"{asset['name']}: wanted {count // 2} I4 bytes at "
                f"0x{asset['offset']:x}, file holds {len(raw)}")
        for byte in raw:
            for nibble in ((byte >> 4) & 0xF, byte & 0xF):
                pixels.append((255, 255, 255, nibble * 17))
    else:
        raise SystemExit(f"{asset['name']}: unhandled format "
                         f"{asset['format']!r}")
    return pixels


def build_source_asset_quads(repo_root: Path,
                             frames_by_texture: dict[int, list[list]]
                             ) -> list[dict]:
    """Quad candidates for the non-particle source textures.

    Single-frame by construction -- neither asset animates its texture; the
    halo's motion is a rotation its AObj script drives on the DObj, not a frame
    sequence. They are marked live because a shield or a respawn pad that
    silently fails to seat is precisely the bug this is fixing.
    """
    candidates = []
    for index, asset in enumerate(SOURCE_QUAD_ASSETS):
        path = repo_root / RELOC_ASSET_DIR / asset["file"]
        if not path.is_file():
            raise SystemExit(f"{path}: missing reloc payload for "
                             f"{asset['name']} ({asset['symbol']})")
        payload = path.read_bytes()
        pixels = decode_source_asset_texels(payload, asset)
        key = SOURCE_QUAD_TEXTURE_STRIDE + index
        frames_by_texture[key] = [pixels]
        cell_w, cell_h = quad_cell_dims(asset["width"], asset["height"])
        candidates.append({
            "texture": key,
            "width": cell_w,
            "height": cell_h,
            "source_width": asset["width"],
            "source_height": asset["height"],
            "frames": 1,
            "frame_list": [0],
            "packed_frames": 1,
            "bytes": cell_w * cell_h,
            "live": True,
            "source_asset": asset["name"],
        })
    return candidates


def build_pack(repo_root: Path) -> dict:
    script_payload = load_o2r_blob(repo_root, *SCRIPT_BANK)
    texture_payload = load_o2r_blob(repo_root, *TEXTURE_BANK)
    scripts = parse_script_bank(script_payload)
    textures = parse_texture_bank(texture_payload)
    reach = derive_reachable_scripts(repo_root, scripts)
    reachable = reach["reachable"]

    wanted = sorted({scripts[sid]["texture_id"] for sid in reachable})
    out_of_range = [tid for tid in wanted if tid >= len(textures)]
    if out_of_range:
        raise SystemExit(f"reachable scripts name absent textures "
                         f"{out_of_range}")

    texture_data = bytearray()
    palette_data: list[int] = []
    frames_by_texture: dict[int, list[list]] = {}
    rows = []
    report_rows = []
    for texture in textures:
        packed = texture["id"] in wanted and texture["frames"] > 0
        if not packed:
            rows.append((texture["width"] if texture["id"] in wanted else 0,
                         texture["height"] if texture["id"] in wanted else 0,
                         DS_NONE, 0, SENTINEL_U32, SENTINEL_U32, 0))
            report_rows.append({
                "texture": texture["id"],
                "source_format":
                    f"{SOURCE_FORMAT_NAMES[texture['fmt']]}{texture['bits']}",
                "frames": texture["frames"],
                "width": texture["width"], "height": texture["height"],
                "source_bytes": texture["source_bytes"],
                "ds_format": "NONE", "ds_bytes": 0,
                "packed": False,
                "reason": "zero-frame source stub" if texture["id"] in wanted
                          else "not reachable from a P1 effect seam",
            })
            continue
        frames = [decode_texture_frame(texture_payload, texture, frame)
                  for frame in range(texture["frames"])]
        frames_by_texture[texture["id"]] = frames
        graded = not {pixel[3] for frame in frames
                      for pixel in frame} <= {0, 255}
        _bits, ds_format, palette, mean_error, max_error, image_bytes = \
            choose_ds_format(texture, frames)
        entries = len(palette) + (1 if ds_format in DS_PALETTE_FORMATS else 0)
        if entries > 0xFF:
            raise SystemExit(
                f"texture {texture['id']}: {entries} palette entries do not "
                "fit NDSParticleTexture.palette_entries"
            )
        while len(texture_data) % DS_TEXTURE_DATA_ALIGN:
            texture_data.append(0)
        data_offset = len(texture_data)
        encoded = [encode_frame(frame, ds_format, palette, texture["width"])
                   for frame in frames]
        for frame_bytes in encoded:
            texture_data += frame_bytes
        packed_entries = ([0] if ds_format in DS_PALETTE_FORMATS else []) \
            + [to_bgr555(colour) for colour in palette]
        decoded = [decode_ds_frame(frame_bytes, ds_format, packed_entries,
                                   texture["width"] * texture["height"])
                   for frame_bytes in encoded]
        round_trip_mean, round_trip_max = measure_error(frames, decoded)
        if (abs(round_trip_mean - mean_error) > 1e-6
                or abs(round_trip_max - max_error) > 1e-6):
            raise SystemExit(
                f"texture {texture['id']}: {DS_FORMAT_NAMES[ds_format]} "
                f"decode-back error {round_trip_mean:.4f}/{round_trip_max:.4f} "
                f"!= modelled {mean_error:.4f}/{max_error:.4f}"
            )
        if entries:
            while len(palette_data) % DS_PALETTE_ALIGN_ENTRIES:
                palette_data.append(0)
        palette_offset = len(palette_data) if entries else SENTINEL_U32
        palette_data.extend(packed_entries)
        rows.append((texture["width"], texture["height"], ds_format, entries,
                     data_offset, palette_offset, texture["frames"]))
        report_rows.append({
            "texture": texture["id"],
            "source_format":
                f"{SOURCE_FORMAT_NAMES[texture['fmt']]}{texture['bits']}",
            "frames": texture["frames"],
            "width": texture["width"], "height": texture["height"],
            "source_bytes": texture["source_bytes"],
            "ds_format": DS_FORMAT_NAMES[ds_format],
            "ds_palette_entries": entries,
            "ds_bytes": image_bytes + 2 * entries,
            "packed": True,
            "source_graded_alpha": graded,
            "mean_error": round(mean_error, 4),
            "max_error": round(max_error, 4),
        })
    while len(palette_data) % DS_PALETTE_ALIGN_ENTRIES:
        palette_data.append(0)

    # Fail-closed invariant: a reachable script whose texture is absent would
    # draw with whatever the runtime finds instead. The zero-frame source stub
    # is the only legitimate DS_NONE a reachable script may name.
    for script_id in reachable:
        texture_id = scripts[script_id]["texture_id"]
        if rows[texture_id][2] == DS_NONE and textures[texture_id]["frames"]:
            raise SystemExit(
                f"script {script_id} is reachable but texture {texture_id} "
                "was not packed"
            )

    live = set(reachable)
    offsets = [scripts[sid]["offset"] if sid in live else SENTINEL_U32
               for sid in range(len(scripts))]

    table_bytes = b"".join((
        b"".join(struct.pack("<I", value) for value in offsets),
        b"".join(struct.pack("<HHBBxxII", *row[:6]) for row in rows),
        bytes(row[6] for row in rows),
        bytes(texture_data),
        b"".join(struct.pack("<H", entry) for entry in palette_data),
    ))
    source_checksum = int(hashlib.sha256(
        script_payload + texture_payload).hexdigest()[:8], 16)
    table_checksum = int(hashlib.sha256(table_bytes).hexdigest()[:8], 16)

    packed_texture_bytes = sum(row["ds_bytes"] for row in report_rows
                               if row["packed"])
    source_texture_bytes = sum(row["source_bytes"] for row in report_rows
                               if row["packed"])
    # The whole pack, and then the half of it that actually competes with the
    # taskman arena. Only linked_bytes is charged against the measured arena
    # headroom -- texture_asset is a NitroFS payload and costs ROM, not RAM
    # (see TEXTURE_ASSET_NITRO_PATH for why that distinction decides whether
    # the ROM boots at all).
    texture_asset = (bytes(texture_data)
                     + b"".join(struct.pack("<H", entry)
                                for entry in palette_data))
    payload_bytes = len(script_payload) + len(texture_asset)
    # Matches `arm-none-eabi-size -A` on the compiled pack: index tables plus
    # the two exported scalars.
    table_bytes_resident = (4 * len(scripts)          # script offsets
                            + 16 * len(textures)      # texture rows
                            + len(textures)           # frame counts
                            + 8)                      # exported scalars
    linked = len(script_payload) + table_bytes_resident
    pupupu = build_pupupu_bank(repo_root, frames_by_texture)
    source_quads = build_source_asset_quads(repo_root, frames_by_texture)
    quads = build_quad_sheet(textures, report_rows, frames_by_texture,
                             pupupu["quad_candidates"] + source_quads)
    return {
        "pupupu": pupupu,
        "source_quads": source_quads,
        "quads": quads,
        "scripts": scripts, "textures": textures, "reach": reach,
        "script_payload": script_payload, "rows": rows,
        "texture_data": bytes(texture_data), "palette_data": palette_data,
        "texture_asset": texture_asset,
        "offsets": offsets, "report_rows": report_rows,
        "source_checksum": source_checksum, "table_checksum": table_checksum,
        "packed_texture_ids": wanted,
        "packed_texture_bytes": packed_texture_bytes,
        "source_texture_bytes": source_texture_bytes,
        "payload_bytes": payload_bytes,
        "table_bytes": table_bytes_resident,
        "pack_bytes": payload_bytes + table_bytes_resident,
        "asset_bytes": len(texture_asset),
        "linked_bytes": linked,
    }


# --------------------------------------------------------------------------
# emit
# --------------------------------------------------------------------------
def _hex_rows(data: bytes, per_row: int = 16) -> str:
    return "\n".join(
        "    " + ", ".join(f"0x{byte:02x}" for byte in data[index:index + per_row]) + ","
        for index in range(0, len(data), per_row)
    )


def render_header(pack: dict) -> str:
    scripts, textures = pack["scripts"], pack["textures"]
    admitted_keys = {row["texture"] for row in pack["quads"]["admitted"]}
    source_quad_defines = "\n".join(
        f"#define NDS_PARTICLE_QUAD_{asset['name']}_TEXTURE "
        f"{SOURCE_QUAD_TEXTURE_STRIDE + index}u"
        f"  /* {asset['symbol']}, {asset['format'].upper()} "
        f"{asset['width']}x{asset['height']}"
        f"{'' if (SOURCE_QUAD_TEXTURE_STRIDE + index) in admitted_keys else ', NOT SEATED'} */"
        for index, asset in enumerate(SOURCE_QUAD_ASSETS)
    )
    return f"""/* Generated by scripts/generate_nds_particle_banks.py. */
#ifndef SSB64_NDS_PARTICLE_BANKS_GENERATED_H
#define SSB64_NDS_PARTICLE_BANKS_GENERATED_H

#include <PR/ultratypes.h>

/*
 * gNdsParticleScriptBank is the original efcommon .scb, byte for byte. It is
 * BIG-ENDIAN N64 data: every u16/u32/f32 inside it needs a byte swap on read.
 * Internal offsets are file-relative, which is why no relocation is involved.
 *
 * gNdsParticleScriptOffsets is indexed by SOURCE script id. A reachable script
 * holds its offset into gNdsParticleScriptBank (the LBScript header; bytecode
 * starts 0x30 later). Anything else holds NDS_PARTICLE_SCRIPT_UNREACHABLE and
 * must not be instantiated -- its texture is not resident, so drawing it would
 * show a different effect.
 *
 * gNdsParticleTextures is indexed by SOURCE texture id, so
 * gNdsParticleTextures[script->texture_id] needs no remapping. An unpacked row
 * carries ds_format NDS_PARTICLE_FORMAT_NONE and sentinel offsets.
 *   data_offset    -- byte offset into the texel block of frame 0; frames are
 *                     contiguous, stride width*height*bits/8.
 *   palette_offset -- ENTRY index into the palette block.
 *   palette_entries-- entries owned by this texture. For PAL4/PAL16/PAL256
 *                     entry 0 is the transparent slot, so the runtime sets the
 *                     colour-0-transparent bit unconditionally; A3I5/A5I3 use
 *                     every entry as a colour and carry alpha in the texel.
 * Each palette starts on an {DS_PALETTE_ALIGN_ENTRIES}-entry boundary so the DS palette base
 * register can address it, and each image block is {DS_TEXTURE_DATA_ALIGN}-byte aligned.
 *
 * BOTH BLOCKS LIVE IN NDS_PARTICLE_TEXTURE_ASSET_PATH, NOT IN THE ARM9 IMAGE.
 * Texels start at 0 and the palette at NDS_PARTICLE_PALETTE_ASSET_OFFSET, so a
 * loader reads the file once and hands glTexImage2D/glColorTableEXT slices of
 * it. Linking them instead costs the boot-time taskman arena search the same
 * {pack["asset_bytes"]} bytes one-for-one and hangs the ROM before the first
 * battle allocation -- the reason for the split is in the generator, above
 * TEXTURE_ASSET_NITRO_PATH. Do not "simplify" this back into an array.
 */

#define NDS_PARTICLE_SCRIPT_COUNT {len(scripts)}u
#define NDS_PARTICLE_SCRIPT_REACHABLE_COUNT {len(pack["reach"]["reachable"])}u
#define NDS_PARTICLE_SCRIPT_UNREACHABLE 0x{SENTINEL_U32:08x}u
#define NDS_PARTICLE_SCRIPT_HEADER_BYTES 0x{LB_SCRIPT_HEADER_BYTES:02x}u
#define NDS_PARTICLE_SCRIPT_BANK_BYTES {len(pack["script_payload"])}u

#define NDS_PARTICLE_TEXTURE_COUNT {len(textures)}u
#define NDS_PARTICLE_TEXTURE_PACKED_COUNT {len(pack["packed_texture_ids"])}u
#define NDS_PARTICLE_TEXTURE_UNPACKED 0x{SENTINEL_U32:08x}u
#define NDS_PARTICLE_TEXTURE_DATA_BYTES {len(pack["texture_data"])}u
#define NDS_PARTICLE_PALETTE_ENTRIES {len(pack["palette_data"])}u

/* The NitroFS texel/palette payload. */
#define NDS_PARTICLE_TEXTURE_ASSET_PATH "{TEXTURE_ASSET_NITRO_PATH}"
#define NDS_PARTICLE_TEXTURE_ASSET_BYTES {pack["asset_bytes"]}u
#define NDS_PARTICLE_PALETTE_ASSET_OFFSET {len(pack["texture_data"])}u

/* .rodata in the ARM9 image, and therefore charged against the arena search:
 * script bank {len(pack["script_payload"])} + index tables {pack["table_bytes"]}. The other
 * {pack["asset_bytes"]} bytes of the {pack["pack_bytes"]}-byte pack are in the file above. */
#define NDS_PARTICLE_LINKED_BYTES {pack["linked_bytes"]}u

/* THE DRAW PATH'S PAYLOAD: one A5I3 atlas, a second encoding of the same
 * texels. The pack above chooses a DS format per texture by measured error and
 * gets the whole reachable set into the NitroFS payload; this sheet is what the
 * hardware quad path actually binds.
 *
 * ATLAS SHEETS, NOT ONE TEXTURE PER FRAME. GL names are a binding constraint
 * too: the cache holds 48 and the battle's static set pins 24, while the
 * admitted set is {len(pack["quads"]["frames"])} individual frames. {pack["quads"]["sheets"]} sheets keep
 * every particle in {pack["quads"]["sheets"]} binds instead of {len(pack["quads"]["frames"])}.
 *
 * 8,192 BYTES IS THE MEASURED-SAFE ALLOCATION, and it is the ALLOCATION that is
 * fixed here, not the texel count -- so coverage grows by asking for more of
 * them rather than for a bigger one. 16,384 and 32,768 both broke stage texture
 * resolves with VRAM free; each sheet here is {pack["quads"]["width"]}x{pack["quads"]["height"]} A3I5 at one byte
 * per texel, which is the block size that has never been refused.
 *
 * A3I5 also fixed a fidelity bug rather than only a capacity one: RGB555+A1
 * gave every particle ONE BIT of alpha, so soft-edged sprites drew as hard
 * blobs. Three bits is 8 levels. Five index bits carry the assets that encode
 * COLOUR rather than shape -- Fox's reflector is two flat blues and nothing
 * else -- while the draw path stays in modulation mode and calls glColor with
 * the source's primcolor for the rest.
 *
 * A texture the runtime asks for and does not find here draws nothing; it does
 * not draw something else. gNdsParticleTextureUseMask says which ones a real
 * match reached, and the excluded list is in the generated JSON report by name
 * so growing the atlas is an informed decision rather than a hopeful one. */
#define NDS_PARTICLE_QUAD_ASSET_PATH "{QUAD_ASSET_NITRO_PATH}"
#define NDS_PARTICLE_QUAD_ATLAS_WIDTH {pack["quads"]["width"]}u
#define NDS_PARTICLE_QUAD_ATLAS_HEIGHT {pack["quads"]["height"]}u
#define NDS_PARTICLE_QUAD_ATLAS_SHEETS {pack["quads"]["sheets"]}u
#define NDS_PARTICLE_QUAD_SHEET_BYTES {pack["quads"]["sheet_bytes"]}u
#define NDS_PARTICLE_QUAD_CELL_CAP {pack["quads"]["cell_cap"]}u
#define NDS_PARTICLE_QUAD_ASSET_BYTES {len(pack["quads"]["payload"])}u
#define NDS_PARTICLE_QUAD_TEXEL_ASSET_BYTES {pack["quads"]["atlas_bytes"]}u
#define NDS_PARTICLE_QUAD_PALETTE_OFFSET {pack["quads"]["palette_offset"]}u
#define NDS_PARTICLE_QUAD_PALETTE_ENTRIES {pack["quads"]["palette_entries"]}u
#define NDS_PARTICLE_QUAD_TEXEL_BYTES {pack["quads"]["bytes"]}u
#define NDS_PARTICLE_QUAD_COUNT {len(pack["quads"]["admitted"])}u
#define NDS_PARTICLE_QUAD_FRAME_COUNT {len(pack["quads"]["frames"])}u

/* One row per (SOURCE texture id, frame). Sorted by both, so a lookup is a
 * scan; the runtime holds pc->texture_id and pc->frame_id and needs nothing
 * else. Coordinates are atlas texels, which is what glTexCoord2t16 takes. */
typedef struct NDSParticleQuadFrame
{{
    u8 texture_id;
    u8 frame;
    /* Which of the NDS_PARTICLE_QUAD_ATLAS_SHEETS allocations holds this cell.
     * x/y are texels WITHIN that sheet, so the draw path binds by sheet and the
     * coordinates never have to encode which one. */
    u8 sheet;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
}} NDSParticleQuadFrame;

extern const NDSParticleQuadFrame
    gNdsParticleQuadFrames[NDS_PARTICLE_QUAD_FRAME_COUNT];

/* DS TEXIMAGE_PARAM texture-format field values. */
#define NDS_PARTICLE_FORMAT_NONE {DS_NONE}u
#define NDS_PARTICLE_FORMAT_A3I5 {DS_A3I5}u
#define NDS_PARTICLE_FORMAT_PAL4 {DS_PAL4}u
#define NDS_PARTICLE_FORMAT_PAL16 {DS_PAL16}u
#define NDS_PARTICLE_FORMAT_PAL256 {DS_PAL256}u
#define NDS_PARTICLE_FORMAT_A5I3 {DS_A5I3}u
#define NDS_PARTICLE_FORMAT_DIRECT16 {DS_DIRECT16}u

#define NDS_PARTICLE_BANKS_SOURCE_CHECKSUM 0x{pack["source_checksum"]:08x}u
#define NDS_PARTICLE_BANKS_TABLE_CHECKSUM 0x{pack["table_checksum"]:08x}u

typedef struct NDSParticleTexture
{{
    u16 width;
    u16 height;
    u8 ds_format;
    u8 palette_entries;
    u32 data_offset;
    u32 palette_offset;
}} NDSParticleTexture;

/* NOT const: ndsParticleLoadEFCommonBank byte-swaps this bank to
 * little-endian IN PLACE, once, and points scripts[] straight at it. The
 * alternative was a syTaskmanMalloc + memcpy of the whole thing, which spent
 * 10,912 bytes of taskman arena to obtain somewhere writable -- on a target
 * where that is three arena steps and the battle boots or does not by about
 * that margin. Same image bytes either way; .data instead of .rodata. */
extern u8 gNdsParticleScriptBank[NDS_PARTICLE_SCRIPT_BANK_BYTES];
extern const u32 gNdsParticleScriptBankBytes;
extern const u32 gNdsParticleScriptOffsets[NDS_PARTICLE_SCRIPT_COUNT];
extern const NDSParticleTexture gNdsParticleTextures[NDS_PARTICLE_TEXTURE_COUNT];
extern const u32 gNdsParticleTextureCount;
/* NDSParticleTexture has no frame count; animation needs one. */
extern const u8 gNdsParticleTextureFrames[NDS_PARTICLE_TEXTURE_COUNT];

/* ------------------------------------------------------------------------
 * Dream Land's own bank. Whispy's leaves (script 0) and dust (script 1) live
 * here, and until 2026-08-01 every non-common bank registered EMPTY, so both
 * failed closed at reject reason 2 before the atlas was consulted.
 *
 * Carried far more cheaply than the common bank: 416 bytes of bytecode and a
 * width/height pair per texture, with no NitroFS texel payload at all. The
 * common bank ships one because its pack "only has to exist"; the DRAW path
 * reads the quad atlas, and these textures are in it.
 *
 * Quad rows for this bank are emitted at NDS_PARTICLE_QUAD_PUPUPU_STRIDE +
 * texture id, because texture 2 names a different image in each bank and one
 * frame table has to answer both. */
#define NDS_PUPUPU_SCRIPT_COUNT {len(pack["pupupu"]["scripts"])}u
#define NDS_PUPUPU_SCRIPT_BANK_BYTES {len(pack["pupupu"]["script_payload"])}u
#define NDS_PUPUPU_TEXTURE_COUNT {len(pack["pupupu"]["textures"])}u
#define NDS_PARTICLE_QUAD_PUPUPU_STRIDE {PUPUPU_QUAD_TEXTURE_STRIDE}u

/* Source-asset quads: textures that belong to no particle bank at all. The
 * shield and the respawn pad are GObj effects whose source art is a single
 * small texture in an ordinary reloc file, and the quad sheet is the port's
 * only textured alpha-blended camera-facing draw -- which is what both effects
 * are. Addressed by these keys, not by a bank id, because there is no bank. */
{source_quad_defines}

typedef struct NDSPupupuTexture
{{
    u8 width;
    u8 height;
    u8 frames;
}} NDSPupupuTexture;

extern u8 gNdsPupupuScriptBank[NDS_PUPUPU_SCRIPT_BANK_BYTES];
extern const u32 gNdsPupupuScriptBankBytes;
extern const u32 gNdsPupupuScriptOffsets[NDS_PUPUPU_SCRIPT_COUNT];
extern const NDSPupupuTexture gNdsPupupuTextures[NDS_PUPUPU_TEXTURE_COUNT];

#endif
"""


def render_inc(pack: dict) -> str:
    offset_rows = "\n".join(
        "    " + ", ".join(f"0x{value:08x}u"
                           for value in pack["offsets"][index:index + 6]) + ","
        for index in range(0, len(pack["offsets"]), 6)
    )
    texture_rows = "\n".join(
        f"    {{ {row[0]:3d}, {row[1]:3d}, {row[2]}, {row[3]:3d}, "
        f"0x{row[4]:08x}u, 0x{row[5]:08x}u }}, /* texture {index} */"
        for index, row in enumerate(pack["rows"])
    )
    frame_rows = "\n".join(
        "    " + ", ".join(f"{row[6]:3d}" for row in
                           pack["rows"][index:index + 12]) + ","
        for index in range(0, len(pack["rows"]), 12)
    )
    quad_rows = "\n".join(
        f"    {{ {row['texture']:3d}, {row['frame']:3d}, {row['sheet']:2d}, "
        f"{row['x']:3d}, {row['y']:3d}, {row['w']:3d}, {row['h']:3d} }},"
        for row in pack["quads"]["frames"]
    )
    pupupu_offset_rows = "\n".join(
        "    " + ", ".join(f"0x{value:08x}u"
                           for value in pack["pupupu"]["offsets"][index:index + 6]) + ","
        for index in range(0, len(pack["pupupu"]["offsets"]), 6)
    )
    pupupu_texture_rows = "\n".join(
        f"    {{ {row[0]:3d}, {row[1]:3d}, {row[2]:3d} }}, /* texture {index} */"
        for index, row in enumerate(pack["pupupu"]["texture_rows"])
    )
    return f"""/* Generated by scripts/generate_nds_particle_banks.py. */
/* efcommon source SHA256-lo 0x{pack["source_checksum"]:08x}, table 0x{pack["table_checksum"]:08x}. */

#include <nds/generated/nds_particle_banks.generated.h>

const u32 gNdsParticleScriptBankBytes = NDS_PARTICLE_SCRIPT_BANK_BYTES;
const u32 gNdsParticleTextureCount = NDS_PARTICLE_TEXTURE_COUNT;

const u32 gNdsParticleScriptOffsets[NDS_PARTICLE_SCRIPT_COUNT] = {{
{offset_rows}
}};

const NDSParticleTexture gNdsParticleTextures[NDS_PARTICLE_TEXTURE_COUNT] = {{
{texture_rows}
}};

const u8 gNdsParticleTextureFrames[NDS_PARTICLE_TEXTURE_COUNT] = {{
{frame_rows}
}};

/* Ordered by (SOURCE texture id, frame); the admission that built it was by
 * size and the layout by shelf packing, so neither order survives here. */
const NDSParticleQuadFrame
    gNdsParticleQuadFrames[NDS_PARTICLE_QUAD_FRAME_COUNT] = {{
{quad_rows}
}};

u8 gNdsParticleScriptBank[NDS_PARTICLE_SCRIPT_BANK_BYTES]
    __attribute__((aligned(4))) = {{
{_hex_rows(pack["script_payload"])}
}};

/* Dream Land's bank. Same big-endian-in-place contract as the common one
 * above, and non-const for the same reason. */
const u32 gNdsPupupuScriptBankBytes = NDS_PUPUPU_SCRIPT_BANK_BYTES;

const u32 gNdsPupupuScriptOffsets[NDS_PUPUPU_SCRIPT_COUNT] = {{
{pupupu_offset_rows}
}};

const NDSPupupuTexture gNdsPupupuTextures[NDS_PUPUPU_TEXTURE_COUNT] = {{
{pupupu_texture_rows}
}};

u8 gNdsPupupuScriptBank[NDS_PUPUPU_SCRIPT_BANK_BYTES]
    __attribute__((aligned(4))) = {{
{_hex_rows(pack["pupupu"]["script_payload"])}
}};

/* The {pack["asset_bytes"]}-byte texel and palette blocks are NOT here. They ship as
 * NDS_PARTICLE_TEXTURE_ASSET_PATH because linked .rodata is taken out of the
 * boot-time taskman arena search one-for-one, and this pack is large enough to
 * push that search past its 0x130000 floor. */
"""


def render_report(pack: dict) -> dict:
    reach = pack["reach"]
    estimate_total = (ESTIMATE["texture_bytes"] + ESTIMATE["script_bank_bytes"])
    return {
        "source": {
            "script_bank": SCRIPT_BANK[0], "script_bank_sha256": SCRIPT_BANK[1],
            "texture_bank": TEXTURE_BANK[0],
            "texture_bank_sha256": TEXTURE_BANK[1],
            "script_bank_bytes": len(pack["script_payload"]),
            "script_count": len(pack["scripts"]),
            "texture_count": len(pack["textures"]),
        },
        "reach": {
            "p1_seams": reach["seams"],
            # A seam with no seed script reaches the display-list effect path
            # (efManagerMakeEffect*) rather than the particle bank, so packing
            # the bank cannot change what it draws.
            "p1_seams_without_bank_scripts": sorted(
                set(reach["seams"])
                - {seam for names in reach["seeds"].values() for seam in names}
            ),
            "seam_helpers": reach["seam_helpers"],
            "seed_scripts": {str(sid): names
                             for sid, names in sorted(reach["seeds"].items())},
            "reachable_scripts": reach["reachable"],
            "spawn_edges": {str(parent): children for parent, children
                            in sorted(reach["spawn_edges"].items())},
            "packed_textures": pack["packed_texture_ids"],
        },
        "bytes": {
            "source_texture_bytes": pack["source_texture_bytes"],
            "ds_texture_bytes": pack["packed_texture_bytes"],
            "ds_texture_data_bytes": len(pack["texture_data"]),
            "ds_palette_bytes": 2 * len(pack["palette_data"]),
            "script_bank_bytes": len(pack["script_payload"]),
            "payload_bytes": pack["payload_bytes"],
            "index_table_bytes": pack["table_bytes"],
            "pack_bytes": pack["pack_bytes"],
            "asset_bytes": pack["asset_bytes"],
            "linked_bytes": pack["linked_bytes"],
            "arena_headroom_bytes": ESTIMATE["arena_headroom_bytes"],
            "spare_bytes": (ESTIMATE["arena_headroom_bytes"]
                            - pack["linked_bytes"]),
            "estimate_2026_07_27_bytes": estimate_total,
            "estimate_2026_07_27_scripts": ESTIMATE["scripts"],
            "estimate_2026_07_27_textures": ESTIMATE["textures"],
        },
        # The draw path's own payload. `quad_excluded` is the point of this
        # block: a texture that is not here draws nothing, so a BUGS.md row
        # that needs one back reads this list rather than guessing at the
        # budget. The honest fix for the big entries is halving 64x64x10, not
        # raising 65,536.
        "quads": {
            "budget_bytes": pack["quads"]["budget_bytes"],
            "atlas_bytes": pack["quads"]["atlas_bytes"],
            # Reported so check-nds-particle-banks.ps1 can DERIVE the asset
            # size instead of carrying its own copy of the entry count -- that
            # copy is what had to be retyped when the sheet went A5I3 to A3I5.
            "palette_entries": pack["quads"]["palette_entries"],
            "atlas_width": pack["quads"]["width"],
            "atlas_height": pack["quads"]["height"],
            # Reported for the same reason as palette_entries: the checker
            # derives the payload size from sheets x sheet_bytes + palette
            # rather than pinning a total that changes whenever coverage does.
            "sheets": pack["quads"]["sheets"],
            "sheet_bytes": pack["quads"]["sheet_bytes"],
            "cell_cap": pack["quads"]["cell_cap"],
            "frame_count": len(pack["quads"]["frames"]),
            "frame_cap": pack["quads"]["frame_cap"],
            "bytes": pack["quads"]["bytes"],
            # Textures whose single cell holds a source frame other than 0.
            # Everything absent from here is frozen on frame 0 -- see
            # QUAD_HELD_FRAME and docs/BACKLOG.md.
            "held_frames": pack["quads"]["held_frames"],
            "admitted": [row["texture"] for row in pack["quads"]["admitted"]],
            "admitted_cells": [
                {"texture": row["texture"], "bytes": row["bytes"],
                 "width": row["width"], "height": row["height"],
                 "frames": row["frames"]}
                for row in pack["quads"]["admitted"]
            ],
            "excluded": [{"texture": row["texture"], "bytes": row["bytes"],
                          "width": row["width"], "height": row["height"],
                          "frames": row["frames"]}
                         for row in pack["quads"]["excluded"]],
        },
        "checksums": {
            "source_sha256_lo": f"0x{pack['source_checksum']:08x}",
            "table_sha256_lo": f"0x{pack['table_checksum']:08x}",
        },
        "textures": pack["report_rows"],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--out-header", type=Path, default=DEFAULT_HEADER)
    parser.add_argument("--out-inc", type=Path, default=DEFAULT_INC)
    parser.add_argument("--out-json", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--out-texture-asset", type=Path,
                        default=DEFAULT_TEXTURE_ASSET)
    parser.add_argument("--out-quad-asset", type=Path,
                        default=DEFAULT_QUAD_ASSET)
    parser.add_argument("--check", action="store_true",
                        help="rebuild in memory and compare existing outputs")
    args = parser.parse_args()
    repo_root = args.repo_root.resolve()
    outputs = []
    for path in (args.out_header, args.out_inc, args.out_json,
                 args.out_texture_asset, args.out_quad_asset):
        outputs.append(path if path.is_absolute() else repo_root / path)
    header_path, inc_path, json_path, asset_path, quad_path = outputs

    pack = build_pack(repo_root)
    header = render_header(pack).encode("ascii")
    inc = render_inc(pack).encode("ascii")
    report = (json.dumps(render_report(pack), indent=2, sort_keys=True)
              + "\n").encode("ascii")

    if args.check:
        # The header and the report are committed, so they must always match.
        # The .inc is a build product under the gitignored src/nds/generated/
        # and the texture payload one under the gitignored assets/, so absence
        # means "not built yet" rather than "drifted".
        stale = [str(path) for path, wanted
                 in ((header_path, header), (json_path, report))
                 if not path.is_file() or path.read_bytes() != wanted]
        for path, wanted in ((inc_path, inc),
                             (asset_path, pack["texture_asset"]),
                             (quad_path, pack["quads"]["payload"])):
            if path.is_file() and path.read_bytes() != wanted:
                stale.append(str(path))
        if stale:
            print("stale particle bank pack: " + ", ".join(stale),
                  file=sys.stderr)
            return 1
    else:
        for path, wanted in ((header_path, header), (inc_path, inc),
                             (json_path, report),
                             (asset_path, pack["texture_asset"]),
                             (quad_path, pack["quads"]["payload"])):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(wanted)

    print(f"NDS_PARTICLE_BANKS=PASS "
          f"scripts={len(pack['reach']['reachable'])}/{len(pack['scripts'])} "
          f"textures={len(pack['packed_texture_ids'])}/{len(pack['textures'])} "
          f"tex_n64={pack['source_texture_bytes']} "
          f"tex_ds={pack['packed_texture_bytes']} "
          f"pack={pack['pack_bytes']} "
          f"linked={pack['linked_bytes']} "
          f"asset={pack['asset_bytes']} "
          f"atlas={pack['quads']['width']}x{pack['quads']['height']}"
          f"x{pack['quads']['sheets']} "
          f"quads={len(pack['quads']['admitted'])}/"
          f"{len(pack['quads']['admitted']) + len(pack['quads']['excluded'])}"
          f" {len(pack['quads']['frames'])}frames "
          f"cap={pack['quads']['frame_cap']} "
          f"cell={pack['quads']['cell_cap']} "
          f"headroom={ESTIMATE['arena_headroom_bytes']} "
          f"spare={ESTIMATE['arena_headroom_bytes'] - pack['linked_bytes']} "
          f"source=0x{pack['source_checksum']:08x} "
          f"table=0x{pack['table_checksum']:08x}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
