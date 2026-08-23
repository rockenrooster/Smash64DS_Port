#!/usr/bin/env python3
"""Bake Mario's pipe and Fox's Arwing into DS-native render packets.

The source BattleShip DObj animation remains authoritative at runtime; only the
immutable model/display-list/texture work is moved offline.  The generated
packet contains already-decoded triangle corners, already-resolved texture
images, and DS-native PAL16/A5I3 texture payloads.  Runtime rendering therefore
does not parse an N64 display list or convert an N64 texture for either entry
effect.
"""

from __future__ import annotations

import copy
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import generate_battle_playable_static_textures as static
import generate_battle_playable_texture_census as census


OUTPUT = ROOT / "src/nds/nds_entry_effects.generated.inc"

MARIO = census.InputSpec(
    Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/MarioSpecial2"),
    "42fabaea5d9d93b35f43c531601a477ecd85621cc72afa240df8ab582589d266",
    356,
)
FOX = census.InputSpec(
    Path("decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/FoxSpecial3"),
    "b928c17ea613e047bfa2e9a554456322153e410a80ecb7ec811b5ff96fb651e4",
    161,
)
EXTERN109 = census.InputSpec(
    Path("decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank109"),
    "7670e2e1cd8bd02c895c18e028e57455323a81ee58567312f7f947be38c2f9b7",
    109,
)

MARIO_ROOTS = (0x03C0, 0x04C0)
FOX_ROOTS = (0x1FA0, 0x2920, 0x29D0, 0x29F0, 0x2A20, 0x2868, 0x2A50, 0x2B00)

G_VTX = 0x01
G_MODIFYVTX = 0x02
G_TRI1 = 0x05
G_TRI2 = 0x06
G_TEXTURE = 0xD7
G_GEOMETRYMODE = 0xD9
G_MOVEWORD = 0xDB
G_DL = 0xDE
G_ENDDL = 0xDF
G_SETOTHERMODE_L = 0xE2
G_SETOTHERMODE_H = 0xE3
G_LOADTLUT = 0xF0
G_LOADBLOCK = 0xF3
G_SETTILE = 0xF5
G_SETPRIMCOLOR = 0xFA
G_SETENVCOLOR = 0xFB
G_SETCOMBINE = 0xFC
G_SETTIMG = 0xFD
G_SETTILESIZE = 0xF2

G_MWO_POINT_ST = 0x14
G_MOVEMEM = 0xDC
G_MW_NUMLIGHT = 0x02
G_MW_LIGHTCOL = 0x0A
G_MV_LIGHT = 0x0A
G_TX_DXT_ONE = 2048

FMT_CI = 2
FMT_IA = 3
SIZ_4B = 0
SIZ_16B = 2

TEX_NONE = 0xFF
TEX_PAL16 = 0
TEX_A5I3 = 1


@dataclass(frozen=True)
class Vertex:
    x: int
    y: int
    z: int
    s: int
    t: int
    r: int
    g: int
    b: int
    a: int


@dataclass(frozen=True)
class TextureKey:
    image_asset: int
    image_offset: int
    tlut_asset: int
    tlut_offset: int
    fmt: int
    size: int
    width: int
    height: int
    upload_width: int
    upload_height: int
    cms: int
    cmt: int
    masks: int
    maskt: int


@dataclass
class Texture:
    key: TextureKey
    ds_format: int
    texels: bytes
    palette: tuple[int, ...]


@dataclass(frozen=True)
class GroupState:
    root_index: int
    geometry_mode: int
    geometry_clear: int
    light_color_1: int
    light_color_2: int
    light_mask: int
    combine_w0: int
    combine_w1: int
    othermode_h: int
    othermode_l: int
    prim_color: int
    env_color: int
    texture_key: TextureKey | None
    texture_scale_s: int
    texture_scale_t: int
    texture_origin_s: int
    texture_origin_t: int
    texture_filter_offset: int


@dataclass
class Group:
    state: GroupState
    corners: list[Vertex]


def decode_vertex(payload: bytes, offset: int) -> Vertex:
    x, y, z, _flag, s, t, r, g, b, a = struct.unpack_from(
        ">hhhHhhBBBB", payload, offset
    )
    return Vertex(x, y, z, s, t, r, g, b, a)


def triangle_indices(op: int, w0: int, w1: int) -> list[tuple[int, int, int]]:
    packed = [w0 & 0xFFFFFF]
    if op == G_TRI2:
        packed.append(w1 & 0xFFFFFF)
    return [
        (((word >> 16) & 0xFF) // 2, ((word >> 8) & 0xFF) // 2, (word & 0xFF) // 2)
        for word in packed
    ]


def apply_othermode(current: int, w0: int, w1: int) -> int:
    bits = (w0 & 0xFF) + 1
    pos = (w0 >> 8) & 0xFF
    if bits > 32 or pos >= 32 or bits + pos > 32:
        return current
    shift = 32 - pos - bits
    mask = 0xFFFFFFFF if bits == 32 else ((1 << bits) - 1) << shift
    return (current & ~mask) | (w1 & mask)


def source_ref(resource: census.O2RResource, command_offset: int, word: int) -> census.PointerRef:
    ref = resource.pointer_at(command_offset + 4)
    if ref is not None:
        return ref
    return census.PointerRef(resource.file_id, (word & 0xFFFF) * 4)


def resolve_geometry_any(state: static.DisplayState):
    tile_index = state.texture_tile if state.texture_seen else static.RENDER_TILE
    tile = state.tiles[tile_index]
    if not tile.set_seen or not tile.size_seen:
        raise SystemExit("entry texture has incomplete render-tile state")
    load = static.active_load(state, tile)
    fmt = tile.format
    size = tile.size
    if fmt == FMT_CI and size != SIZ_4B:
        if state.tlut_count <= 16:
            size = SIZ_4B
        else:
            raise SystemExit("entry CI texture is no longer CI4")
    if (fmt, size) not in ((FMT_CI, SIZ_4B), (FMT_IA, SIZ_16B)):
        raise SystemExit(f"entry texture format escaped CI4/IA16: {fmt}/{size}")

    loaded_bytes = load.load_texels * (4 if size == static.SIZ_32B else 2)
    width = tile.width
    height = tile.height
    if (
        width == 0
        or height == 0
        or width > static.MAX_TEXTURE_DIMENSION
        or height > static.MAX_TEXTURE_DIMENSION
        or static.source_bytes(fmt, size, width * height) > loaded_bytes
    ):
        width = static.line_pixels(size, tile.line)
        texels = load.load_texels * 2
        if size == SIZ_4B:
            texels *= 2
        elif size in (static.SIZ_16B, static.SIZ_32B):
            texels //= 2
        height = texels // width if width else 0
    if not (0 < width <= static.MAX_TEXTURE_DIMENSION and 0 < height <= static.MAX_TEXTURE_DIMENSION):
        raise SystemExit(f"invalid entry texture dimensions {width}x{height}")
    source_extent_width = width
    source_extent_height = height
    materialize_s = static.materializes_masked_clamp(
        tile.cms, tile.masks, source_extent_width, tile.width
    )
    materialize_t = static.materializes_masked_clamp(
        tile.cmt, tile.maskt, source_extent_height, tile.height
    )
    if materialize_s:
        width = tile.width
    if materialize_t:
        height = tile.height
    upload_width = static.next_pow2(width)
    upload_height = static.next_pow2(height)
    if upload_width > static.MAX_TEXTURE_DIMENSION or upload_height > static.MAX_TEXTURE_DIMENSION:
        raise SystemExit("entry texture cannot be padded")
    return (
        tile_index, tile, load, fmt, size, width, height,
        upload_width, upload_height, materialize_s, materialize_t,
        source_extent_width, source_extent_height,
    )


def texture_key(state: static.DisplayState) -> TextureKey:
    (
        _tile_index, tile, load, fmt, size, width, height,
        upload_width, upload_height, _materialize_s, _materialize_t,
        _source_extent_width, _source_extent_height,
    ) = resolve_geometry_any(state)
    tlut = state.tlut_image or census.PointerRef(0, 0)
    return TextureKey(
        load.image.asset_id, load.image.offset, tlut.asset_id, tlut.offset,
        fmt, size, width, height, upload_width, upload_height,
        tile.cms, tile.cmt, tile.masks, tile.maskt,
    )


def source_coords(state: static.DisplayState, x: int, y: int):
    (
        _tile_index, tile, load, _fmt, size, width, height,
        _upload_width, _upload_height, materialize_s, materialize_t,
        source_extent_width, _source_extent_height,
    ) = resolve_geometry_any(state)
    if load.load_kind == static.LOAD_KIND_TILE:
        origin_s = load.load_uls >> 2
        origin_t = load.load_ult >> 2
        source_width = load.image_width * 2 if size == SIZ_4B and load.image_size == static.SIZ_8B else load.image_width
    else:
        origin_s = 0
        origin_t = 0
        source_width = source_extent_width
        if load.load_dxt:
            qwords = (G_TX_DXT_ONE + load.load_dxt - 1) // load.load_dxt
            source_width = static.line_pixels(size, qwords)
    sx = static.masked_address(x, tile.cms, tile.masks) if materialize_s else x
    sy = static.masked_address(y, tile.cmt, tile.maskt) if materialize_t else y
    return origin_s + sx, origin_t + sy, source_width, width, height


def convert_texture(state: static.DisplayState, resources: dict[int, census.O2RResource]) -> Texture:
    (
        _tile_index, tile, load, fmt, size, width, height,
        upload_width, upload_height, materialize_s, materialize_t,
        _source_extent_width, _source_extent_height,
    ) = resolve_geometry_any(state)
    image = resources[load.image.asset_id]
    key = texture_key(state)

    if fmt == FMT_CI:
        if state.tlut_image is None or state.tlut_image.asset_id != load.image.asset_id:
            raise SystemExit("entry CI texture/TLUT crossed assets")
        blocks = [{
            "identity": {"asset_id": image.file_id, "offset": 0},
            "source_bytes": len(image.payload),
        }]
        record = static.capture_record(1, 0, copy.deepcopy(state), image, blocks)
        ds_format, packed, palette = static.repack_paletted(record.pixels)
        if ds_format != static.DS_FORMAT_PAL16:
            raise SystemExit("entry CI4 texture stopped fitting PAL16")
        palette = tuple(palette) + (0,) * (16 - len(palette))
        return Texture(key, TEX_PAL16, packed, palette)

    # IA16 -> A5I3.  The eight palette entries are a uniform grayscale ramp;
    # the 8-bit source intensity is quantised to the nearest 3-bit level while
    # source alpha maps to DS's five alpha bits.  This is the same DS-native
    # representation used by the particle AOT generator for IA16 assets.
    palette = tuple(
        ((i * 31 // 7) | ((i * 31 // 7) << 5) | ((i * 31 // 7) << 10))
        for i in range(8)
    )
    out = bytearray(upload_width * upload_height)
    for y in range(height):
        for x in range(width):
            sx, sy, source_width, _w, _h = source_coords(state, x, y)
            source_index = sy * source_width + sx
            # O2R payload is word-swapped; a 16-bit logical texel therefore
            # occupies physical halfword (index ^ 1), matching the runtime
            # resolver's lane correction for 16-bit sources.
            physical = load.image.offset + ((source_index ^ 1) * 2)
            intensity = image.payload[physical]
            alpha = image.payload[physical + 1]
            i3 = (intensity * 7 + 127) // 255
            a5 = (alpha * 31 + 127) // 255
            out[y * upload_width + x] = (a5 << 3) | i3
    return Texture(key, TEX_A5I3, bytes(out), palette)


class Compiler:
    def __init__(self, resource: census.O2RResource, resources: dict[int, census.O2RResource]):
        self.resource = resource
        self.resources = resources
        self.display = static.DisplayState()
        self.vertex_cache: dict[int, Vertex] = {}
        self.geometry_mode = 0
        self.geometry_clear = 0
        # gSPLightColor state.  RSP state persists across the roots of one
        # source tree in draw order (MARIO_ROOTS/FOX_ROOTS are listed in that
        # order), so a root that sets no colour of its own -- the pipe's
        # second list -- draws with the colours its predecessor left.
        self.light_color_1 = 0
        self.light_color_2 = 0
        self.light_mask = 0
        self.combine_w0 = 0
        self.combine_w1 = 0
        self.othermode_h = 0
        self.othermode_l = 0
        self.prim_color = 0xFFFFFFFF
        self.env_color = 0xFFFFFFFF
        self.texture_scale_s = 0xFFFF
        self.texture_scale_t = 0xFFFF
        self.groups: list[Group] = []
        self.textures: dict[TextureKey, Texture] = {}

    def group_state(self, root_index: int) -> GroupState:
        key = None
        origin_s = origin_t = 0
        if self.display.texture_on and self.display.loads:
            key = texture_key(self.display)
            if key not in self.textures:
                self.textures[key] = convert_texture(self.display, self.resources)
            tile = self.display.tiles[self.display.texture_tile if self.display.texture_seen else static.RENDER_TILE]
            origin_s, origin_t = tile.uls, tile.ult
        filter_offset = 8 if (self.othermode_h & 0x00003000) != 0 else 0
        return GroupState(
            root_index, self.geometry_mode, self.geometry_clear,
            self.light_color_1, self.light_color_2, self.light_mask,
            self.combine_w0, self.combine_w1,
            self.othermode_h, self.othermode_l, self.prim_color, self.env_color,
            key, self.texture_scale_s, self.texture_scale_t,
            origin_s, origin_t, filter_offset,
        )

    @staticmethod
    def final_uv(vertex: Vertex, state: GroupState) -> Vertex:
        s = ((vertex.s * state.texture_scale_s) >> 17) - (state.texture_origin_s << 2) + state.texture_filter_offset
        t = ((vertex.t * state.texture_scale_t) >> 17) - (state.texture_origin_t << 2) + state.texture_filter_offset
        return Vertex(vertex.x, vertex.y, vertex.z, s, t, vertex.r, vertex.g, vertex.b, vertex.a)

    def compile_roots(self, roots: tuple[int, ...], root_base: int) -> None:
        for local_index, root in enumerate(roots):
            self.walk(root, root_base + local_index, ())

    def walk(self, start: int, root_index: int, stack: tuple[int, ...]) -> None:
        if start in stack:
            raise SystemExit(f"entry DL recursion at 0x{start:x}")
        stack = stack + (start,)
        pc = start
        for _guard in range(4096):
            w0, w1 = struct.unpack_from(">II", self.resource.payload, pc)
            op = w0 >> 24
            if op == G_DL:
                ref = source_ref(self.resource, pc, w1)
                if ref.asset_id != self.resource.file_id:
                    raise SystemExit("entry DL branch crossed assets")
                self.walk(ref.offset, root_index, stack)
            elif op == G_ENDDL:
                return
            elif op == G_VTX:
                count = (w0 >> 12) & 0xFF
                end = (w0 >> 1) & 0x7F
                first = end - count
                ref = source_ref(self.resource, pc, w1)
                source = self.resources[ref.asset_id]
                for i in range(count):
                    self.vertex_cache[first + i] = decode_vertex(source.payload, ref.offset + i * 16)
            elif op == G_MODIFYVTX:
                where = (w0 >> 16) & 0xFF
                if where != G_MWO_POINT_ST:
                    raise SystemExit(f"entry MODIFYVTX field 0x{where:x} is unsupported")
                index = (w0 & 0xFFFF) >> 1
                s, t = struct.unpack(">hh", w1.to_bytes(4, "big"))
                old = self.vertex_cache[index]
                self.vertex_cache[index] = Vertex(old.x, old.y, old.z, s, t, old.r, old.g, old.b, old.a)
            elif op in (G_TRI1, G_TRI2):
                state = self.group_state(root_index)
                if not self.groups or self.groups[-1].state != state:
                    self.groups.append(Group(state, []))
                group = self.groups[-1]
                for tri in triangle_indices(op, w0, w1):
                    for index in tri:
                        if index not in self.vertex_cache:
                            raise SystemExit(f"entry vertex cache miss root={root_index} pc=0x{pc:x} slot={index}")
                        v = self.vertex_cache[index]
                        group.corners.append(self.final_uv(v, state) if state.texture_key else v)
            elif op == G_SETTIMG:
                self.display.image = source_ref(self.resource, pc, w1)
                self.display.image_format = (w0 >> 21) & 7
                self.display.image_size = (w0 >> 19) & 3
                self.display.image_width = (w0 & 0xFFF) + 1
            elif op == G_SETTILE:
                index = (w1 >> 24) & 7
                tile = self.display.tiles[index]
                tile.set_seen = True
                tile.format = (w0 >> 21) & 7
                tile.size = (w0 >> 19) & 3
                tile.line = (w0 >> 9) & 0x1FF
                tile.tmem = w0 & 0x1FF
                tile.palette = (w1 >> 20) & 0xF
                tile.cmt = (w1 >> 18) & 3
                tile.maskt = (w1 >> 14) & 0xF
                tile.shiftt = (w1 >> 10) & 0xF
                tile.cms = (w1 >> 8) & 3
                tile.masks = (w1 >> 4) & 0xF
                tile.shifts = w1 & 0xF
            elif op == G_LOADTLUT:
                self.display.tlut_image = self.display.image
                self.display.tlut_count = ((w1 >> 14) & 0x3FF) + 1
            elif op == G_LOADBLOCK:
                index = (w1 >> 24) & 7
                tile = self.display.tiles[index]
                self.display.loads.append(static.LoadState(
                    self.display.image, self.display.image_format,
                    self.display.image_size, self.display.image_width,
                    static.LOAD_KIND_BLOCK, index,
                    (w0 >> 12) & 0xFFF, w0 & 0xFFF,
                    (w1 >> 12) & 0xFFF, w1 & 0xFFF,
                    ((w1 >> 12) & 0xFFF) + 1, tile.tmem,
                ))
            elif op == G_SETTILESIZE:
                index = (w1 >> 24) & 7
                tile = self.display.tiles[index]
                tile.size_seen = True
                tile.uls = (w0 >> 12) & 0xFFF
                tile.ult = w0 & 0xFFF
                tile.lrs = (w1 >> 12) & 0xFFF
                tile.lrt = w1 & 0xFFF
                tile.width = ((tile.lrs - tile.uls) >> 2) + 1 if tile.lrs >= tile.uls else 0
                tile.height = ((tile.lrt - tile.ult) >> 2) + 1 if tile.lrt >= tile.ult else 0
            elif op == G_TEXTURE:
                self.display.texture_seen = True
                self.display.texture_on = (w0 & 0xFF) != 0
                self.display.texture_tile = (w0 >> 8) & 7
                self.texture_scale_s = (w1 >> 16) & 0xFFFF
                self.texture_scale_t = w1 & 0xFFFF
            elif op == G_GEOMETRYMODE:
                # w0's low 24 bits are the AND mask (the bits to clear, inverted)
                # and w1 the bits to set.  Both lists run from whatever geometry
                # mode the battle display left in place -- scVSBattleFuncLights
                # sets G_LIGHTING before every display proc -- so a bit neither
                # cleared nor set here is INHERITED at runtime, not zero.  Mario's
                # pipe never mentions G_LIGHTING and is lit by exactly that
                # inheritance (its vertex r,g,b are unit normals).  Carry the
                # cleared set beside the set one; the runtime resolves
                # (initial & ~clear) | set per group.
                self.geometry_mode = (self.geometry_mode & w0) | w1
                self.geometry_clear = (self.geometry_clear | (~w0 & 0x00FFFFFF)) & (~w1 & 0xFFFFFFFF)
            elif op == G_SETCOMBINE:
                self.combine_w0, self.combine_w1 = w0, w1
            elif op == G_SETOTHERMODE_H:
                self.othermode_h = apply_othermode(self.othermode_h, w0, w1)
            elif op == G_SETOTHERMODE_L:
                self.othermode_l = apply_othermode(self.othermode_l, w0, w1)
            elif op == G_SETPRIMCOLOR:
                self.prim_color = w1
            elif op == G_SETENVCOLOR:
                self.env_color = w1
            elif op == G_MOVEWORD:
                # Light COLOURS are the lists' own (pipe: diffuse white over
                # ambient 0x99; Arwing: white over 0x80) and travel in the
                # group row.  The light DIRECTION and count are the battle
                # display's (scVSBattleFuncLights / the fighter's own
                # ftDisplayLightsDrawReflect), which the runtime seeds, so a
                # direction or count word here would be silently dropped --
                # refuse those instead.
                index = (w0 >> 16) & 0xFF
                if index == G_MW_LIGHTCOL:
                    # gSPLightColor writes each colour twice (the a/b copies at
                    # +0x00/+0x04 for light 1 and +0x18/+0x1c for light 2).
                    offset = w0 & 0xFFFF
                    if offset in (0x00, 0x04):
                        self.light_color_1 = w1
                        self.light_mask |= 1
                    elif offset in (0x18, 0x1C):
                        self.light_color_2 = w1
                        self.light_mask |= 2
                    else:
                        raise SystemExit(f"entry DL light colour offset 0x{offset:x} at 0x{pc:x} is not light 1/2")
                elif index == G_MW_NUMLIGHT:
                    raise SystemExit(f"entry DL carries gSPNumLights at 0x{pc:x}; the runtime light seed no longer matches")
            elif op == G_MOVEMEM:
                if (w0 & 0xFF) == G_MV_LIGHT:
                    raise SystemExit(f"entry DL carries gSPLight at 0x{pc:x}; the runtime light seed no longer matches")
            pc += 8
        raise SystemExit(f"entry display-list guard expired at 0x{start:x}")


def c_bytes(name: str, data: bytes, per_line: int = 16) -> list[str]:
    out = [f"static const u8 {name}[{len(data)}] = {{"]
    for i in range(0, len(data), per_line):
        out.append("    " + ", ".join(f"0x{x:02x}u" for x in data[i:i + per_line]) + ",")
    out.append("};")
    return out


def emit(mario: Compiler, fox: Compiler) -> str:
    groups = mario.groups + fox.groups
    textures_by_key: dict[TextureKey, Texture] = {}
    for compiler in (mario, fox):
        textures_by_key.update(compiler.textures)
    texture_keys = list(textures_by_key)
    texture_slot = {key: i for i, key in enumerate(texture_keys)}

    roots = list(MARIO_ROOTS) + list(FOX_ROOTS)
    root_groups: list[list[int]] = [[] for _ in roots]
    flat_vertices: list[Vertex] = []
    group_rows = []
    lit_inherit = lit_clear = lit_set = 0
    for index, group in enumerate(groups):
        if group.state.geometry_mode & 0x00020000:
            lit_set += 1
        elif group.state.geometry_clear & 0x00020000:
            lit_clear += 1
        else:
            lit_inherit += 1
        if len(group.corners) % 3:
            raise SystemExit("entry group corner count is not triangular")
        root_groups[group.state.root_index].append(index)
        first = len(flat_vertices)
        flat_vertices.extend(group.corners)
        group_rows.append((first, len(group.corners) // 3, group))

    lines = [
        "/* Generated by scripts/3d_vfx/generate_nds_entry_effects.py. Do not edit. */",
        "/* Mario pipe + Fox Arwing: no runtime N64 DL/vertex/texture decoding. */",
        f"/* G_LIGHTING per group: {lit_inherit} inherit the battle display's, {lit_clear} clear it, {lit_set} set it. */",
        "",
        f"#define NDS_ENTRY_EFFECT_ROOT_COUNT {len(roots)}u",
        f"#define NDS_ENTRY_EFFECT_GROUP_COUNT {len(groups)}u",
        f"#define NDS_ENTRY_EFFECT_VERTEX_COUNT {len(flat_vertices)}u",
        f"#define NDS_ENTRY_EFFECT_TEXTURE_COUNT {len(texture_keys)}u",
        f"#define NDS_ENTRY_EFFECT_MARIO_ROOT_COUNT {len(MARIO_ROOTS)}u",
        f"#define NDS_ENTRY_EFFECT_FOX_ROOT_FIRST {len(MARIO_ROOTS)}u",
        "",
    ]
    lines.append("static const NDSRendererInputVertex sNdsEntryEffectVertices[NDS_ENTRY_EFFECT_VERTEX_COUNT] = {")
    for v in flat_vertices:
        lines.append(f"    {{ {v.x}, {v.y}, {v.z}, {v.s}, {v.t}, {v.r}, {v.g}, {v.b}, {v.a} }},")
    lines.append("};")
    lines.append("")

    lines.append("static const NDSEntryEffectGroup sNdsEntryEffectGroups[NDS_ENTRY_EFFECT_GROUP_COUNT] = {")
    for first, triangles, group in group_rows:
        s = group.state
        slot = TEX_NONE if s.texture_key is None else texture_slot[s.texture_key]
        key = s.texture_key
        cms = key.cms if key else 0
        cmt = key.cmt if key else 0
        masks = key.masks if key else 0
        maskt = key.maskt if key else 0
        lines.append(
            "    { "
            f"{first}u, {triangles}u, {slot}u, {s.root_index}u, "
            f"0x{s.geometry_mode:08x}u, 0x{s.geometry_clear:08x}u, 0x{s.combine_w0:08x}u, 0x{s.combine_w1:08x}u, "
            f"0x{s.othermode_h:08x}u, 0x{s.othermode_l:08x}u, "
            f"0x{s.prim_color:08x}u, 0x{s.env_color:08x}u, "
            f"0x{s.light_color_1:08x}u, 0x{s.light_color_2:08x}u, "
            f"{cms}u, {cmt}u, {masks}u, {maskt}u, {s.light_mask}u }},"
        )
    lines.append("};")
    lines.append("")

    lines.append("static const NDSEntryEffectRoot sNdsEntryEffectRoots[NDS_ENTRY_EFFECT_ROOT_COUNT] = {")
    for root_index, root in enumerate(roots):
        indices = root_groups[root_index]
        first = indices[0] if indices else 0
        count = len(indices)
        lines.append(f"    {{ 0x{root:04x}u, {first}u, {count}u, 0u }},")
    lines.append("};")
    lines.append("")

    for slot, key in enumerate(texture_keys):
        texture = textures_by_key[key]
        lines.extend(c_bytes(f"sNdsEntryEffectTexture{slot}Texels", texture.texels))
        lines.append(f"static const u16 sNdsEntryEffectTexture{slot}Palette[{len(texture.palette)}] = {{")
        lines.append("    " + ", ".join(f"0x{x & 0x7fff:04x}u" for x in texture.palette) + ",")
        lines.append("};")
        lines.append("")

    lines.append("static const NDSEntryEffectTexture sNdsEntryEffectTextures[NDS_ENTRY_EFFECT_TEXTURE_COUNT] = {")
    for slot, key in enumerate(texture_keys):
        texture = textures_by_key[key]
        lines.append(
            "    { "
            f"sNdsEntryEffectTexture{slot}Texels, sizeof(sNdsEntryEffectTexture{slot}Texels), "
            f"sNdsEntryEffectTexture{slot}Palette, {len(texture.palette)}u, "
            f"{key.upload_width}u, {key.upload_height}u, {texture.ds_format}u, 0u }},"
        )
    lines.append("};")
    lines.append("")
    return "\n".join(lines) + "\n"


def main() -> None:
    resources = {spec.file_id: census.load_o2r(ROOT, spec) for spec in (MARIO, FOX, EXTERN109)}
    mario = Compiler(resources[MARIO.file_id], resources)
    mario.compile_roots(MARIO_ROOTS, 0)
    fox = Compiler(resources[FOX.file_id], resources)
    fox.compile_roots(FOX_ROOTS, len(MARIO_ROOTS))
    OUTPUT.write_text(emit(mario, fox), encoding="ascii")
    print(
        f"wrote {OUTPUT.relative_to(ROOT)}: "
        f"groups={len(mario.groups) + len(fox.groups)} "
        f"triangles={sum(len(g.corners) // 3 for g in mario.groups + fox.groups)} "
        f"textures={len(set(mario.textures) | set(fox.textures))}"
    )


if __name__ == "__main__":
    main()
