#!/usr/bin/env python3
"""The fighter list emitter (P2-2p8 Phase 1 slice 4).

Replays a fighter owner's generated native tables exactly as the production
execute does (src/nds/nds_renderer_native_fighter_production.c ->
src/nds/nds_renderer_native_common.c state spans / shade / run prepare /
emitters) and packs the GX FIFO words the packet recorder
(src/nds/nds_renderer_preamble.c ndsFighterPacketCmd*) records -- or, in the
'lean' layout, the slice 4 list (P' once, the light under an identity vector
matrix at the head, one 12-word LOAD4x3 per root).

Words the tables cannot determine are emitted as tagged placeholders:
matrices, the light vector, texture VRAM words (TEXIMAGE_PARAM/PLTT_BASE),
DIF_AMB (its inputs are recorded on the tag) and texgen coordinates.  Two
structure decisions depend on live material state -- whether an untextured
material epoch takes the tint tile, and whether a texture carries a palette
word -- and are asked of a `decide` object.

scripts/fighters/check_nds_native_owner_packet.py --fighter-lists proves this
model against runtime-recorded packets (scripts/fighters/fighter_list_proof.py).
"""
import sys
from pathlib import Path

_scripts_root = Path(__file__).resolve().parent
while _scripts_root.name != "scripts":
    _scripts_root = _scripts_root.parent
if str(_scripts_root) not in sys.path:
    sys.path.insert(0, str(_scripts_root))
import _paths  # noqa: E402,F401

import generate_nds_native_owners as g  # noqa: E402

ROOT = _scripts_root.parent

# --- constants (nds_renderer.h, nds_renderer_preamble.c, libnds) ----------
GEOM_ZBUFFER = 0x1
GEOM_LIGHTING = 0x00020000
GEOM_TEXTURE_GEN = 0x00040000
GEOM_CULL_BACK = 0x400
GEOM_CULL_FRONT = 0x200
SRC_CULL_FRONT = 0x1000   # NDS_NATIVE_SOURCE_GEOM_CULL_* (checked below)
SRC_CULL_BACK = 0x2000
LIGHT_DIR_1 = 1
LIGHT_COLOR_1 = 1
LIGHT_COLOR_2 = 2
TEX_STATE_SEEN, TEX_STATE_ON, TEX_STATE_SCALE_S, TEX_STATE_SCALE_T = 1, 2, 4, 8
TEXM_SETCOMBINE, TEXM_SETTILE, TEXM_TEXTURE, TEXM_SETTILESIZE = 1, 2, 4, 8
TEXM_SETTIMG, TEXM_LOADBLOCK, TEXM_LOADTILE = 16, 32, 64
RENDER_TILE = 0
LOAD_TILE = 7
IMPLICIT_SCALE = 0xffff
TEXTFILT_MASK = 3 << 12
TF_POINT = 0
FILTER_OFFSET = 1 << 4
POLY_ID_MASK = 0x3f
POLY_LIGHT0 = 1
POLY_CULL_BACK = 2 << 6
POLY_CULL_NONE = 3 << 6
RUN_CLASS_MASK = 0x03
RUN_UNLIT = 0x80
RUN_ALPHA_MASK = 0x7c
RUN_ALPHA_SHIFT = 2
RUN_RAW, RUN_CROSS = 0, 1
POLICY_CULL_NONE = 0x80
DENSE_ID_MASK = 0x7ff
CORNER_MATRIX_SHIFT = 11
GX_CURRENT = 31
GX_SLOT_MAX = 30
GL_TRIANGLE = 0
TINT_DIM = 8
MATERIAL_NONE = 0xff
STATE_NONE = 0xffff
# NDS_NATIVE_STATE_*
ST_OTHERMODE, ST_COMBINE, ST_TEXTURE, ST_GEOMETRY, ST_IMAGE = 2, 3, 4, 5, 6
ST_TILE, ST_LOAD_TLUT, ST_LOAD_BLOCK, ST_TILE_SIZE, ST_PRIM = 7, 8, 9, 10, 11
ST_BLEND, ST_MATERIAL, ST_LIGHT_COLOR, ST_LOAD_TILE = 12, 13, 14, 20
# RDP opcodes
OP_SETOTHERMODE_H, OP_SETOTHERMODE_L, OP_RDPSETOTHERMODE = 0xE3, 0xE2, 0xEF
# MOVEWORD light colour offsets (F3DEX2: index 0x0a, offsets 0x00/0x04,
# 0x18/0x1c ... light n at 0x18*(n-1)).
MW_LIGHTCOL = 0x0A
# direct policy families (generated sNdsNativeFighterDirectPolicies)
POLICY_TEXTURED = {0: 1, 1: 0, 2: 0, 3: 0, 4: 1, 5: 0}
POLICY_USE_MATERIAL = {0: 0, 1: 1, 2: 0, 3: 1, 4: 0, 5: 0}

# GX command ids (REG2ID)
C_MTX_MODE, C_MTX_PUSH, C_MTX_POP, C_MTX_STORE, C_MTX_RESTORE = 0x10, 0x11, 0x12, 0x13, 0x14
C_MTX_IDENTITY, C_MTX_LOAD_4x4, C_MTX_LOAD_4x3 = 0x15, 0x16, 0x17
C_COLOR, C_NORMAL, C_TEXCOORD, C_VTX_16 = 0x20, 0x21, 0x22, 0x23
C_POLY_ATTR, C_TEXIMAGE, C_PLTT = 0x29, 0x2A, 0x2B
C_DIF_AMB, C_LIGHT_VECTOR, C_LIGHT_COLOR = 0x30, 0x32, 0x33
C_BEGIN = 0x40
PARAMS = {C_MTX_MODE: 1, C_MTX_PUSH: 0, C_MTX_POP: 1, C_MTX_STORE: 1,
          C_MTX_RESTORE: 1, C_MTX_IDENTITY: 0, C_MTX_LOAD_4x4: 16,
          C_MTX_LOAD_4x3: 12, C_COLOR: 1, C_NORMAL: 1, C_TEXCOORD: 1,
          C_VTX_16: 2, C_POLY_ATTR: 1, C_TEXIMAGE: 1, C_PLTT: 1,
          C_DIF_AMB: 1, C_LIGHT_VECTOR: 1, C_LIGHT_COLOR: 1, C_BEGIN: 1}


class Tag(object):
    """A word the host does not know; `kind` names why."""
    __slots__ = ('kind', 'info')

    def __init__(self, kind, info=None):
        self.kind = kind
        self.info = info

    def __repr__(self):
        return '<%s %r>' % (self.kind, self.info)


class Packer(object):
    """ndsFighterPacketCmd: a header carries up to four opcodes, their
    parameters follow; a header whose commands took no parameter at all gets
    one dummy word before the next header."""

    def __init__(self):
        self.words = []
        self.cmds = []          # (param index, cmd, n)
        self.slot = 4
        self.header = None
        self.header_params = 0
        self.header_valid = False

    def cmd(self, op, params=()):
        params = list(params)
        assert len(params) == PARAMS[op], (hex(op), len(params))
        if self.slot >= 4:
            if self.header_valid and self.header_params == 0:
                self.words.append(0)
            self.header = len(self.words)
            self.words.append(0)
            self.slot = 0
            self.header_params = 0
            self.header_valid = True
        self.words[self.header] |= op << (8 * self.slot)
        self.slot += 1
        self.header_params += len(params)
        first = len(self.words)
        self.words.extend(params)
        self.cmds.append((first, op, len(params)))
        return first


def s16(v):
    v &= 0xffff
    return v - 0x10000 if v & 0x8000 else v


def s32(v):
    v &= 0xffffffff
    return v - 0x100000000 if v & 0x80000000 else v


class Tile(object):
    __slots__ = ('set_seen', 'size_seen', 'format', 'size', 'line', 'tmem',
                 'palette', 'cms', 'cmt', 'masks', 'maskt', 'shifts',
                 'shiftt', 'uls', 'ult', 'lrs', 'lrt', 'width', 'height')

    def __init__(self):
        for k in self.__slots__:
            setattr(self, k, 0)


class Stats(object):
    """The NDSRendererStats fields the fighter emission reads."""

    def __init__(self):
        self.geometry_mode = 0
        self.othermode_h = 0
        self.othermode_l = 0
        self.combine = (0, 0)
        self.combine_count = 0
        self.texture_mask = 0
        self.texture_state_flags = 0
        self.texture_tile = 0
        self.texture_on = 0
        self.scale_s = 0
        self.scale_t = 0
        self.texture_image = 0
        self.texture_format = 0
        self.texture_size = 0
        self.texture_width = 0
        self.load_texels = 0
        self.tiles = [Tile() for _ in range(8)]
        self.tlut = None
        self.prim = 0
        self.env = 0
        self.blend = 0
        self.light1 = 0
        self.light2 = 0
        self.light_color_mask = 0
        self.light_dir = (0, 0, 0)
        self.light_dir_mask = 0
        # The fighter display's preamble state the execute inherits:
        # G_CYC_2CYCLE and G_TF_BILERP (ftdisplaymain.c).
        self.othermode_h = (1 << 20) | (2 << 12)

    def active_tile(self):
        if self.texture_state_flags & TEX_STATE_SEEN:
            return self.texture_tile & 7
        return RENDER_TILE


def apply_othermode(st, w0, w1):
    op = w0 >> 24
    if op == OP_RDPSETOTHERMODE:
        st.othermode_h = w0 & 0x00ffffff
        st.othermode_l = w1
        return
    if op not in (OP_SETOTHERMODE_H, OP_SETOTHERMODE_L):
        return
    bits = (w0 & 0xff) + 1
    pos = (w0 >> 8) & 0xff
    if bits > 32 or pos >= 32 or bits + pos > 32:
        return
    shift = 32 - pos - bits
    mask = 0xffffffff if bits >= 32 else (((1 << bits) - 1) << shift)
    if op == OP_SETOTHERMODE_H:
        st.othermode_h = (st.othermode_h & ~mask) | (w1 & mask)
    else:
        st.othermode_l = (st.othermode_l & ~mask) | (w1 & mask)
    st.othermode_h &= 0xffffffff
    st.othermode_l &= 0xffffffff


def apply_texture(st, w0, w1):
    st.texture_mask |= TEXM_TEXTURE
    st.texture_tile = (w0 >> 8) & 7
    st.texture_on = (w0 >> 1) & 0x7f
    st.scale_s = (w1 >> 16) & 0xffff
    st.scale_t = w1 & 0xffff
    st.texture_state_flags = TEX_STATE_SEEN
    if st.texture_on:
        st.texture_state_flags |= TEX_STATE_ON
    if st.scale_s:
        st.texture_state_flags |= TEX_STATE_SCALE_S
    if st.scale_t:
        st.texture_state_flags |= TEX_STATE_SCALE_T


def apply_settile(st, w0, w1):
    t = st.tiles[(w1 >> 24) & 7]
    st.texture_mask |= TEXM_SETTILE
    t.set_seen = 1
    t.format = (w0 >> 21) & 7
    t.size = (w0 >> 19) & 3
    t.line = (w0 >> 9) & 0x1ff
    t.tmem = w0 & 0x1ff
    t.palette = (w1 >> 20) & 0xf
    t.cmt = (w1 >> 18) & 3
    t.maskt = (w1 >> 14) & 0xf
    t.shiftt = (w1 >> 10) & 0xf
    t.cms = (w1 >> 8) & 3
    t.masks = (w1 >> 4) & 0xf
    t.shifts = w1 & 0xf


def apply_settilesize(st, w0, w1):
    st.texture_mask |= TEXM_SETTILESIZE
    t = st.tiles[(w1 >> 24) & 7]
    t.size_seen = 1
    t.uls = (w0 >> 12) & 0xfff
    t.ult = w0 & 0xfff
    t.lrs = (w1 >> 12) & 0xfff
    t.lrt = w1 & 0xfff
    t.width = ((t.lrs - t.uls) >> 2) + 1 if t.lrs >= t.uls else 0
    t.height = ((t.lrt - t.ult) >> 2) + 1 if t.lrt >= t.ult else 0


def apply_setimage(st, w0, w1):
    st.texture_mask |= TEXM_SETTIMG
    st.texture_format = (w0 >> 21) & 7
    st.texture_size = (w0 >> 19) & 3
    st.texture_width = (w0 & 0xfff) + 1
    st.texture_image = w1


def apply_loadblock(st, w0, w1):
    st.texture_mask |= TEXM_LOADBLOCK
    st.load_texels = ((w1 >> 12) & 0xfff) + 1


def apply_loadtile(st, w0, w1):
    st.texture_mask |= TEXM_LOADTILE
    uls, ult = (w0 >> 12) & 0xfff, w0 & 0xfff
    lrs, lrt = (w1 >> 12) & 0xfff, w1 & 0xfff
    st.load_texels = 0
    if lrs >= uls and lrt >= ult:
        st.load_texels = (((lrs - uls) >> 2) + 1) * (((lrt - ult) >> 2) + 1)


def apply_light_moveword(st, w0, w1):
    index = (w0 >> 16) & 0xff
    offset = w0 & 0xffff
    if index != MW_LIGHTCOL:
        return
    if offset in (0x00, 0x04):
        st.light1 = w1
        st.light_color_mask |= LIGHT_COLOR_1
    elif offset in (0x18, 0x1c):
        st.light2 = w1
        st.light_color_mask |= LIGHT_COLOR_2


def apply_delta(st, delta, image_resolver=None):
    w0, w1, effect = delta[0], delta[1], delta[2]
    if effect == ST_OTHERMODE:
        apply_othermode(st, w0, w1)
    elif effect == ST_COMBINE:
        st.texture_mask |= TEXM_SETCOMBINE
        st.combine_count += 1
        st.combine = (w0, w1)
    elif effect == ST_TEXTURE:
        apply_texture(st, w0, w1)
    elif effect == ST_GEOMETRY:
        st.geometry_mode = ((st.geometry_mode & w0) | w1) & 0xffffffff
    elif effect == ST_IMAGE:
        apply_setimage(st, w0, w1 if image_resolver is None else image_resolver(delta))
    elif effect == ST_TILE:
        apply_settile(st, w0, w1)
    elif effect == ST_LOAD_TLUT:
        st.tlut = (w1, st.texture_image)
    elif effect == ST_LOAD_BLOCK:
        apply_loadblock(st, w0, w1)
    elif effect == ST_TILE_SIZE:
        apply_settilesize(st, w0, w1)
    elif effect == ST_PRIM:
        st.prim = w1
    elif effect == ST_BLEND:
        st.blend = w1
    elif effect == ST_LIGHT_COLOR:
        apply_light_moveword(st, w0, w1)
    elif effect == ST_LOAD_TILE:
        apply_loadtile(st, w0, w1)


def apply_material(st, m):
    """ndsRendererNativeApplyMaterial over a host material dict: keys
    'palette_image' (w0, w1), 'palette_tlut' ((tile w0, tile w1), tlut w1),
    'light1', 'light2', 'prim' (w0, w1), 'env', 'blend', 'block_image'
    (w0, w1), 'load_block' (w0, w1), 'current_image' (w0, w1),
    'render_tile_size' (w0, w1), 'scroll_tile_size' (w0, w1),
    'texture' (w0, w1)."""
    if m is None:
        return
    if 'palette_image' in m:
        apply_setimage(st, *m['palette_image'])
    if 'palette_tlut' in m:
        (tw0, tw1), tlut_w1 = m['palette_tlut']
        apply_settile(st, tw0, tw1)
        st.tlut = (tlut_w1, st.texture_image)
    if 'light1' in m:
        st.light1 = m['light1']
        st.light_color_mask |= LIGHT_COLOR_1
    if 'light2' in m:
        st.light2 = m['light2']
        st.light_color_mask |= LIGHT_COLOR_2
    if 'prim' in m:
        st.prim = m['prim'][1]
    if 'env' in m:
        st.env = m['env']
    if 'blend' in m:
        st.blend = m['blend']
    if 'block_image' in m:
        apply_setimage(st, *m['block_image'])
    if 'load_block' in m:
        apply_loadblock(st, *m['load_block'])
    if 'current_image' in m:
        apply_setimage(st, *m['current_image'])
    if 'render_tile_size' in m:
        apply_settilesize(st, *m['render_tile_size'])
    if 'scroll_tile_size' in m:
        apply_settilesize(st, *m['scroll_tile_size'])
    if 'texture' in m:
        apply_texture(st, *m['texture'])


def implicit_texture_on(st):
    if st.texture_state_flags & TEX_STATE_ON:
        return False
    req = TEXM_SETTIMG | TEXM_SETTILE | TEXM_SETTILESIZE
    t = st.tiles[st.active_tile()]
    return ((st.texture_mask & req) == req and
            (st.texture_mask & (TEXM_LOADBLOCK | TEXM_LOADTILE)) != 0 and
            st.texture_image != 0 and st.load_texels != 0 and
            t.set_seen and t.size_seen and t.line and t.width and t.height)


def texture_params(st):
    """The run prepare's texture scale / origin / offset (ProductionRunCore)."""
    t = st.tiles[st.active_tile()]
    scale_s, scale_t = st.scale_s, st.scale_t
    if implicit_texture_on(st):
        if not (st.texture_state_flags & TEX_STATE_SCALE_S):
            scale_s = IMPLICIT_SCALE
        if not (st.texture_state_flags & TEX_STATE_SCALE_T):
            scale_t = IMPLICIT_SCALE
    offset = FILTER_OFFSET if (st.othermode_h & TEXTFILT_MASK) != TF_POINT else 0
    return scale_s, scale_t, t.uls, t.ult, offset


def uv_word(dense, params):
    """ndsRendererNativeRebuildProductionRunUv, ordinary UV."""
    scale_s, scale_t, origin_s, origin_t, offset = params
    ds, dt = dense[3], dense[4]
    ss = s32(ds * scale_s) >> 17
    tt = s32(dt * scale_t) >> 17
    s = s16(ss - (origin_s << 2) + offset)
    t = s16(tt - (origin_t << 2) + offset)
    return (s & 0xffff) | ((t & 0xffff) << 16)


def normalize_geometry(mode):
    src = mode & (SRC_CULL_FRONT | SRC_CULL_BACK)
    if src & SRC_CULL_FRONT:
        mode |= GEOM_CULL_FRONT
    if src & SRC_CULL_BACK:
        mode |= GEOM_CULL_BACK
    return mode & ~(SRC_CULL_FRONT | SRC_CULL_BACK)


class Emitter(object):
    """One production execute, root by root, as the recorder packs it.

    emit_root() takes the root's context row, its preamble dict
    (geometry_mode, cycle_type, render_mode, prim, env, light_valid,
    light_dir), its material dicts by slot (None: none applied) and its GX
    store slot (31 = none).  `decide` supplies the material-dependent
    structure (tint route, palette word) where the host cannot know it."""

    def __init__(self, ctx, decide=None, layout='recorded', lit=True,
                 memo=False):
        """layout 'recorded': today's packet (split projection + world-scaled
        LOAD4x4 per root, the light bracket at the first lit epoch).
        layout 'lean': the slice 4 list -- P' (projection with row 3 >> 8)
        loaded once, the light vector under an identity vector matrix before
        the first root, one 12-word LOAD4x3 per root; everything after the
        root's matrix is the same command stream.  `lit`: the list has a lit
        epoch (the head carries the light only then).  `memo`: model
        production's run texture memo (keyed by run index alone) across the
        execute -- the runtime-recorded packets carry it; the lean lists do
        not."""
        self.ctx = ctx
        self.decide = decide
        self.layout = layout
        self.memo = {} if memo else None
        self.pk = Packer()
        self.tags = {}
        self.st = Stats()
        self.light_written = False
        self.prepared_st = {}
        self.pk.cmd(C_LIGHT_COLOR, [0x7fff])
        if layout == 'lean':
            self.pk.cmd(C_MTX_MODE, [0])
            i = self.pk.cmd(C_MTX_LOAD_4x4, [0] * 16)
            for k in range(16):
                self.tag(i + k, 'pproj', k)
            self.pk.cmd(C_MTX_MODE, [2])
            if lit:
                self.pk.cmd(C_MTX_IDENTITY, [])
                i = self.pk.cmd(C_LIGHT_VECTOR, [0])
                self.tag(i, 'light')
            self.light_written = True

    def tag(self, i, kind, info=None):
        self.tags[i] = Tag(kind, info)

    def span(self, ctx, first, count):
        if count == 0 or first == STATE_NONE:
            return
        state, sequence = ctx['state'], ctx['sequence']
        for k in range(count):
            apply_delta(self.st, state[sequence[first + k]])

    def emit_root(self, ri, root_index, pre, materials, slot, ctx=None,
                  row=None, light_index=None):
        """`ctx`/`row`: the table set and root row this binding executes
        (a mixed-file program's donor root lives in its own table set);
        default: this emitter's context, row `root_index`."""
        ctx = self.ctx if ctx is None else ctx
        st, pk, decide = self.st, self.pk, self.decide
        epochs, runs = ctx['epochs'], ctx['runs']
        policies = ctx['direct_epoch_policies']
        dense = ctx['dense_vertices']
        normals = ctx['dense_normals']
        gx = ctx['gx_positions']
        run_meta = ctx.get('run_metadata') or [0] * len(runs)
        (group_first, group_count, group_type, group_first_vertex,
         group_vertex_count, prim_vertices) = ctx['primitive_streams'][2]
        run_first_unique = ctx['run_first_unique']
        run_unique_count = ctx['run_unique_count']
        run_unique_dense = ctx['run_unique_dense']
        root = ctx['roots'][root_index] if row is None else row
        if pre is not None:
            st.geometry_mode = normalize_geometry(pre['geometry_mode'])
            st.othermode_h = (st.othermode_h & ~(3 << 20)) | (pre['cycle_type'] & (3 << 20))
            st.othermode_l = pre['render_mode']
            st.prim = pre['prim']
            st.env = pre['env']
            if pre.get('light_valid'):
                st.light_dir = pre['light_dir']
                st.light_dir_mask = LIGHT_DIR_1
        self.root_first_cmd = len(pk.cmds)
        if self.layout == 'lean':
            i = pk.cmd(C_MTX_LOAD_4x3, [0] * 12)
            for k in range(12):
                self.tag(i + k, 'mv43', (ri, k))
        else:
            pk.cmd(C_MTX_MODE, [0])
            i = pk.cmd(C_MTX_LOAD_4x4, [0] * 16)
            for k in range(16):
                self.tag(i + k, 'proj', (ri, k))
            pk.cmd(C_MTX_MODE, [2])
            i = pk.cmd(C_MTX_LOAD_4x4, [0] * 16)
            for k in range(16):
                self.tag(i + k, 'mv', (ri, k))
        if slot <= GX_SLOT_MAX:
            pk.cmd(C_MTX_STORE, [slot])
        lp = (ctx['light_preamble_indices'][root_index]
              if light_index is None else light_index)
        if lp != 0:
            st.light1, st.light2 = ctx['light_preambles'][lp]
            st.light_color_mask |= LIGHT_COLOR_1 | LIGHT_COLOR_2
        prepare_valid = False
        poly_fmt = 0
        prep = None
        texkey = None
        first_epoch, epoch_count = root[1], root[4]
        for e in range(first_epoch, first_epoch + epoch_count):
            ep = epochs[e]
            (bsf, asf, _fa, first_run, bsc, asc, _bsy, _asy, action_count,
             run_count, mat_slot, _ftc) = ep
            if bsc:
                prepare_valid = False
            self.span(ctx, bsf, bsc)
            if mat_slot != MATERIAL_NONE:
                prepare_valid = False
                if materials is not None and mat_slot < len(materials):
                    apply_material(st, materials[mat_slot])
            if asc:
                prepare_valid = False
            self.span(ctx, asf, asc)
            policy = policies[e]
            family = policy & 7
            textured = POLICY_TEXTURED[family]
            use_material = POLICY_USE_MATERIAL[family]
            material_color = st.prim if use_material else 0
            tint = False
            if action_count != 0:
                lit = (st.geometry_mode & GEOM_LIGHTING) and (st.light_dir_mask & LIGHT_DIR_1)
                if lit:
                    if not self.light_written:
                        pk.cmd(C_MTX_MODE, [2])
                        pk.cmd(C_MTX_PUSH, [])
                        pk.cmd(C_MTX_IDENTITY, [])
                        i = pk.cmd(C_LIGHT_VECTOR, [0])
                        self.tag(i, 'light')
                        pk.cmd(C_MTX_POP, [1])
                        self.light_written = True
                    rgb = (material_color >> 8) & 0xffffff
                    tint = bool(use_material and not textured and rgb != 0xffffff)
                    if decide is not None and use_material and not textured:
                        tint = decide.tint(pk, ri, e, tint)
                    i = pk.cmd(C_DIF_AMB, [0])
                    self.tag(i, 'shade', dict(l1=st.light1, l2=st.light2,
                                              material=0 if tint else material_color,
                                              use_material=0 if tint else use_material,
                                              tinted=tint, epoch=e))
            for r in range(first_run, first_run + run_count):
                run = runs[r]
                meta = run_meta[r]
                cls = run[2] & RUN_CLASS_MASK
                unlit = bool(meta & RUN_UNLIT)
                alpha = ((meta & RUN_ALPHA_MASK) >> RUN_ALPHA_SHIFT) if unlit else 31
                light0 = 0 if unlit else POLY_LIGHT0
                cull = POLY_CULL_NONE if (policy & POLICY_CULL_NONE) else POLY_CULL_BACK
                if not prepare_valid:
                    if textured:
                        if (self.memo is not None) and (r in self.memo):
                            # ndsRendererR2RunTextureMemoApply: the run index
                            # alone keys production's run texture memo, so a
                            # donor-table root replays whatever run of the
                            # same index prepared first in this execute.
                            prep = self.memo[r]
                        else:
                            prep = texture_params(st)
                            if self.memo is not None:
                                self.memo[r] = prep
                        texkey = texture_key(st)
                    else:
                        prep = None
                        texkey = None
                    poly_fmt = cull | (alpha << 16) | light0 | ((st.combine_count & POLY_ID_MASK) << 24)
                    prepare_valid = True
                else:
                    poly_fmt = ((poly_fmt & ~(POLY_CULL_NONE | (31 << 16) | POLY_LIGHT0)) |
                                cull | (alpha << 16) | light0)
                texgen = bool(st.geometry_mode & GEOM_TEXTURE_GEN) and bool(textured)
                if textured or tint:
                    i = pk.cmd(C_TEXIMAGE, [0])
                    self.tag(i, 'teximage', dict(tint=tint, key=texkey, run=r))
                    has_pltt = True if decide is None else decide.pltt(pk, ri, r)
                    if has_pltt:
                        i = pk.cmd(C_PLTT, [0])
                        self.tag(i, 'pltt', dict(tint=tint, key=texkey, run=r))
                else:
                    pk.cmd(C_TEXIMAGE, [0])
                pk.cmd(C_POLY_ATTR, [poly_fmt])
                pk.cmd(C_BEGIN, [GL_TRIANGLE])
                if tint:
                    word = ((TINT_DIM // 2) << 4) | (((TINT_DIM // 2) << 4) << 16)
                    pk.cmd(C_TEXCOORD, [word])
                if textured and not texgen:
                    uf, uc = run_first_unique[r], run_unique_count[r]
                    for k in range(uc):
                        d = run_unique_dense[uf + k]
                        self.prepared_st[d] = uv_word(dense[d], prep)
                gf, gc = group_first[r], group_count[r]
                tex_words = bool(textured) and not tint
                if cls == RUN_CROSS:
                    corners = prim_vertices[group_first_vertex[gf]:group_first_vertex[gf] + run[1] * 3]
                    current = slot
                    active = current
                    for packed in corners:
                        d = packed & DENSE_ID_MASK
                        ps = packed >> CORNER_MATRIX_SHIFT
                        if ps == GX_CURRENT:
                            ps = current
                        if ps != active:
                            pk.cmd(C_MTX_RESTORE, [ps])
                            active = ps
                        self.corner(d, unlit, normals, tex_words, texgen, gx, r)
                    if active != current:
                        pk.cmd(C_MTX_RESTORE, [current])
                else:
                    ctype = GL_TRIANGLE
                    for gi in range(gf, gf + gc):
                        gtype = group_type[gi]
                        if gtype != ctype or gtype != GL_TRIANGLE:
                            pk.cmd(C_BEGIN, [gtype])
                            ctype = gtype
                        v0 = group_first_vertex[gi]
                        for d in prim_vertices[v0:v0 + group_vertex_count[gi]]:
                            self.corner(d, unlit, normals, tex_words, texgen, gx, r)
                    if ctype != GL_TRIANGLE:
                        pk.cmd(C_BEGIN, [GL_TRIANGLE])
        self.span(ctx, root[2], root[5])

    def corner(self, d, unlit, normals, textured, texgen, gx, run):
        pk = self.pk
        pk.cmd(C_COLOR if unlit else C_NORMAL, [normals[d]])
        if textured:
            if texgen:
                i = pk.cmd(C_TEXCOORD, [0])
                self.tag(i, 'texgen', (run, d))
            else:
                i = pk.cmd(C_TEXCOORD, [self.prepared_st.get(d, 0)])
                if d not in self.prepared_st:
                    self.tag(i, 'stale_uv', (run, d))
        xy, z = g.pack_fifo_vertex16_scaled(gx[d][0], gx[d][1], gx[d][2], 'corner')
        pk.cmd(C_VTX_16, [xy, z])

    def finish(self):
        if self.pk.header_valid and self.pk.header_params == 0:
            self.pk.words.append(0)
        return self.pk, self.tags


def texture_key(st):
    t = st.tiles[st.active_tile()]
    return dict(image=st.texture_image, fmt=t.format, siz=t.size, line=t.line,
                tmem=t.tmem, palette=t.palette, cms=t.cms, cmt=t.cmt,
                masks=t.masks, maskt=t.maskt, width=t.width, height=t.height,
                timg_fmt=st.texture_format, timg_siz=st.texture_size,
                tlut=st.tlut)


_CTX = {}


def owner_contexts(owner):
    """(high, low) exactly as generate() prepares them: one light-preamble
    table, the union of both details, and each detail's root programs."""
    if owner in _CTX:
        return _CTX[owner]
    high = g.build_p2_owner_runtime_context(ROOT, owner, 'high')
    low = g.build_p2_owner_runtime_context(ROOT, owner, 'low')
    merged = list(high['light_preambles'])
    for pre in low['light_preambles']:
        if pre not in merged:
            merged.append(pre)
    remap = [merged.index(pre) for pre in low['light_preambles']]
    low['light_preamble_indices'] = [remap[i] for i in low['light_preamble_indices']]
    trio = low.get('kirby_trio_bodies')
    if owner == 'kirby' and trio:
        for entry in trio.values():
            entry['root']['light_index'] = remap[entry['root']['light_index']]
    high['light_preambles'] = merged
    low['light_preambles'] = merged
    low['high_light_preambles'] = merged
    if owner in g.OWNER_ROOT_PROGRAMS or owner == 'kirby':
        high['root_programs'] = g.build_owner_root_programs(ROOT, high)
        low['root_programs'] = g.build_owner_root_programs(ROOT, low)
        for ctx in (high, low):
            for prog in ctx['root_programs']:
                for view in prog.get('verification_contexts') or []:
                    # Kirby's copy-hat table sets are verification views
                    # without the baked shade words; their images carry the
                    # generator's own derivation of them.
                    if (('dense_normals' not in view) and
                            ('dense_vertices' in view) and
                            ('run_unique_dense' in view)):
                        view['dense_normals'] = g._build_dense_shade_words(
                            view['dense_vertices'], view['runs'],
                            view['run_first_unique'],
                            view['run_unique_count'],
                            view['run_unique_dense'],
                            view.get('run_metadata') or
                            [0] * len(view['runs']))
    _CTX[owner] = (high, low)
    return _CTX[owner]


def context(owner, detail):
    high, low = owner_contexts(owner)
    return high if detail == 'high' else low


class GuidedDecider(object):
    """Takes the material-dependent structure decisions (tint route, palette
    word present) from a reference packet at the aligned command position:
    the words themselves stay masked, only their presence is read."""

    def __init__(self, ref_cmds):
        self.ref = ref_cmds

    def _at(self, pk, ahead=0):
        k = len(pk.cmds) + ahead
        return self.ref[k] if k < len(self.ref) else None

    def tint(self, pk, ri, e, default):
        # The epoch's DIF_AMB is next; look for the first run's TEXIMAGE after it.
        k = len(pk.cmds)
        while k < len(self.ref) and self.ref[k][0] != C_TEXIMAGE:
            k += 1
        if k >= len(self.ref):
            return default
        return self.ref[k][1][0] != 0

    def pltt(self, pk, ri, r):
        nxt = self._at(pk)
        return nxt is not None and nxt[0] == C_PLTT
