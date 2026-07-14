#!/usr/bin/env python3
"""Generate exact Nintendo DS Dream Land animated-water AOT artifacts.

This host build tool reads the pinned BattleShip O2R
resources, executes the original material-animation command stream with
float32 arithmetic, and reproduces the current DS TEXEL0/TEXEL1 CI4 ordered-
coverage output.  It does not replace or modify the runtime renderer.

Large outputs use the renderer's existing compact representation: first-
occurrence unique rows followed by a 128-byte row map.  Small outputs remain
full 32x64 little-endian DS-u16 images.  Every optimized output is expanded
and compared with a separate slow per-pixel oracle before it is accepted.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Sequence


SIMULATED_FRAMES = 5 * 60 * 60
EXPECTED_CYCLE_FRAMES = 216

BANK103_RELATIVE = Path(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank103"
)
BANK104_RELATIVE = Path(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/ExternDataBank104"
)

BANK103_SHA256 = "a61e74aece06c5f15fa7cd1d6633afd9cc3750c9163caeffe59cab2d157a222a"
BANK104_SHA256 = "3ce7e51da3810dca927521717357a2c44b1c51760bc942b0d4e5bfebe6fd4d52"
BANK103_PAYLOAD_SHA256 = (
    "4109fa4ac31fdf36d25ea228f8475a1efa6758114942ee65b051e56358684afe"
)
BANK104_PAYLOAD_SHA256 = (
    "1d82f9304458528341452b9610f55952a4951a1ead4c41cd0c5ebdab10380ebd"
)

PALETTE_OFFSET = 0x1BB8
TEXTURE_OFFSETS = (0x1BE0, 0x1E10, 0x2040)
CI4_SOURCE_WIDTH = 32
CI4_SOURCE_HEIGHT = 32
CI4_SOURCE_BYTES = CI4_SOURCE_WIDTH * CI4_SOURCE_HEIGHT // 2

LARGE_MOBJ_SUB_OFFSET = 0x1F78
SMALL_MOBJ_SUB_OFFSET = 0x1FF0
LARGE_SCRIPT_OFFSET = 0x2540
SMALL_SCRIPT_OFFSET = 0x2620

TX_MIRROR = 1
TX_CLAMP = 2
TILE0_CMS = TX_MIRROR | TX_CLAMP
TILE0_CMT = TX_CLAMP
TILE1_CMS = TX_CLAMP
TILE1_CMT = TX_CLAMP
TILE_MASK_S = 5
TILE_MASK_T = 5

COMBINE_W0 = 0xFC272C04
COMBINE_W1 = 0x1F0C93FF

OWNER_LARGE = 0
OWNER_SMALL = 1
OWNER_COUNT = 2
LARGE_ROW_CAPACITY = 64

EXPECTED_OWNER_COUNTS = {
    OWNER_LARGE: (165, 101),
    OWNER_SMALL: (157, 105),
}
EXPECTED_FIRST_OUTPUT_HASH = {
    OWNER_LARGE: "f3a908659547f360ec9d3b79f80aa4c5dca829cdb36975a5d3a59667d1fdf532",
    OWNER_SMALL: "61b0bb44aa30033d0c8e07d924f6b38ddbafa23807692eb16aab194e57457efe",
}
EXPECTED_OUTPUT_SET_HASH = {
    OWNER_LARGE: "32b1736cf65a536a7d90e70b7324f54031a8ee23fd4b4a79f458df2f00f79e27",
    OWNER_SMALL: "9c6736d6766b1978f649bc869e9dd2599cbab9655777001576f8829c9e0d3543",
}

# These are the runtime ABI, not merely informative fixture values.  A build
# must stop if the exact payload or sorted lookup table changes without an
# intentional schema/signature update on both the host and DS sides.
EXPECTED_PAYLOAD_BYTES = 1_560_960
EXPECTED_PAYLOAD_SHA256 = (
    "fa8bf472aad6ef1a2423bcec3f28891831b309774ae2f92c9db140cb25121c1e"
)
EXPECTED_INDEX_BYTES = 11_060
EXPECTED_INDEX_SHA256 = (
    "83d2b342bb0a3530589a12af89a0548aa67dbdf6d1f92630bef1798701ac08b6"
)

PAYLOAD_NAME = "pupupu_water_aot_payload.bin"
INDEX_NAME = "pupupu_water_aot_index.bin"
MANIFEST_NAME = "pupupu_water_aot_manifest.json"

INDEX_MAGIC = b"PWA1"
INDEX_VERSION = 1
INDEX_HEADER = struct.Struct("<4sHHHHIIHHHHII")
INDEX_IMAGE = struct.Struct("<BBHIII")
# owner/texture IDs, alignment byte, eight 12-bit tile coordinates,
# fraction/alignment byte, then global output index.
INDEX_KEY = struct.Struct("<BBBB8HBBH")

OUTPUT_KIND_FULL = 0
OUTPUT_KIND_COMPACT_ROWS = 1


class Falsifier(RuntimeError):
    """A source or exact-output assumption failed and must not be papered over."""


def falsify(message: str) -> Falsifier:
    return Falsifier(f"FALSIFIER: {message}")


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def f32(value: float | int) -> float:
    return struct.unpack("<f", struct.pack("<f", float(value)))[0]


def fadd(left: float, right: float) -> float:
    return f32(f32(left) + f32(right))


def fsub(left: float, right: float) -> float:
    return f32(f32(left) - f32(right))


def fmul(left: float, right: float) -> float:
    return f32(f32(left) * f32(right))


def fdiv(left: float, right: float) -> float:
    return f32(f32(left) / f32(right))


def fneg(value: float) -> float:
    return f32(-f32(value))


def read_be_u32(payload: bytes, offset: int, context: str) -> int:
    if offset < 0 or offset + 4 > len(payload):
        raise falsify(f"{context}: u32 read at 0x{offset:x} is out of range")
    return struct.unpack_from(">I", payload, offset)[0]


def read_be_f32(payload: bytes, offset: int, context: str) -> float:
    if offset < 0 or offset + 4 > len(payload):
        raise falsify(f"{context}: f32 read at 0x{offset:x} is out of range")
    return struct.unpack_from(">f", payload, offset)[0]


@dataclass(frozen=True)
class O2RResource:
    relative_path: Path
    source: bytes
    payload: bytes
    source_sha256: str
    payload_sha256: str
    file_id: int
    extern_count: int
    data_offset: int


def load_o2r_resource(
    repo_root: Path,
    relative_path: Path,
    expected_source_hash: str,
    expected_payload_hash: str,
    expected_file_id: int,
    expected_extern_count: int,
) -> O2RResource:
    path = repo_root / relative_path
    if not path.is_file():
        raise falsify(
            f"required BattleShip O2R corpus is absent: {path}. "
            "Pass --repo-root pointing at a checkout that contains decomp/."
        )
    source = path.read_bytes()
    source_hash = sha256(source)
    if source_hash != expected_source_hash:
        raise falsify(
            f"{relative_path}: SHA256 {source_hash} != pinned {expected_source_hash}"
        )
    if len(source) < 0x50 or source[4:8] != b"OLER":
        raise falsify(f"{relative_path}: invalid or truncated O2R header")
    file_id = struct.unpack_from("<I", source, 0x40)[0]
    extern_count = struct.unpack_from("<I", source, 0x48)[0]
    if file_id != expected_file_id:
        raise falsify(
            f"{relative_path}: file ID 0x{file_id:x} != 0x{expected_file_id:x}"
        )
    if extern_count != expected_extern_count:
        raise falsify(
            f"{relative_path}: extern count {extern_count} != {expected_extern_count}"
        )
    data_size_offset = 0x40 + 12 + extern_count * 2
    if data_size_offset + 4 > len(source):
        raise falsify(f"{relative_path}: truncated extern table")
    data_size = struct.unpack_from("<I", source, data_size_offset)[0]
    data_offset = data_size_offset + 4
    data_end = data_offset + data_size
    if data_end != len(source):
        raise falsify(
            f"{relative_path}: payload ends at 0x{data_end:x}, "
            f"file ends at 0x{len(source):x}"
        )
    payload = source[data_offset:data_end]
    payload_hash = sha256(payload)
    if payload_hash != expected_payload_hash:
        raise falsify(
            f"{relative_path}: payload SHA256 {payload_hash} "
            f"!= pinned {expected_payload_hash}"
        )
    return O2RResource(
        relative_path=relative_path,
        source=source,
        payload=payload,
        source_sha256=source_hash,
        payload_sha256=payload_hash,
        file_id=file_id,
        extern_count=extern_count,
        data_offset=data_offset,
    )


@dataclass(frozen=True)
class MObjSpec:
    owner: int
    name: str
    width: int
    height: int
    scroll_width: int
    scroll_height: int
    bias: int
    trau: float
    trav: float
    scau: float
    scav: float
    scrollu: float
    scrollv: float
    flags: int
    prim_l: int
    script_offset: int
    sprite_offsets: tuple[int, int, int]
    materialize_s: bool
    materialize_t: bool


def parse_mobj_spec(
    payload: bytes,
    owner: int,
    name: str,
    offset: int,
    script_offset: int,
    sprite_offsets: tuple[int, int, int],
) -> MObjSpec:
    if offset < 0 or offset + 0x78 > len(payload):
        raise falsify(f"{name}: MObjSub at 0x{offset:x} is out of range")
    fmt = payload[offset + 2]
    size = payload[offset + 3]
    bias, width, height = struct.unpack_from(">HHH", payload, offset + 0x0A)
    unk10 = struct.unpack_from(">i", payload, offset + 0x10)[0]
    trau, trav, scau, scav = struct.unpack_from(">ffff", payload, offset + 0x14)
    flags = struct.unpack_from(">H", payload, offset + 0x30)[0]
    block_fmt = payload[offset + 0x32]
    block_size = payload[offset + 0x33]
    block_dxt, source_width, scroll_width, scroll_height = struct.unpack_from(
        ">HHHH", payload, offset + 0x34
    )
    scrollu, scrollv = struct.unpack_from(">ff", payload, offset + 0x3C)
    prim_l = payload[offset + 0x54]
    if (fmt, size, unk10, flags, block_fmt, block_size, block_dxt, source_width) != (
        2,
        2,
        0,
        0x006B,
        2,
        0,
        0x20,
        0x20,
    ):
        raise falsify(
            f"{name}: MObjSub static signature changed: "
            f"fmt/size={fmt}/{size}, unk10={unk10}, flags=0x{flags:04x}, "
            f"block={block_fmt}/{block_size}/0x{block_dxt:x}, "
            f"source_width={source_width}"
        )
    if prim_l != 0x73:
        raise falsify(f"{name}: prim_l 0x{prim_l:02x} != 0x73")
    return MObjSpec(
        owner=owner,
        name=name,
        width=width,
        height=height,
        scroll_width=scroll_width,
        scroll_height=scroll_height,
        bias=bias,
        trau=f32(trau),
        trav=f32(trav),
        scau=f32(scau),
        scav=f32(scav),
        scrollu=f32(scrollu),
        scrollv=f32(scrollv),
        flags=flags,
        prim_l=prim_l,
        script_offset=script_offset,
        sprite_offsets=sprite_offsets,
        materialize_s=width > CI4_SOURCE_WIDTH,
        materialize_t=height > CI4_SOURCE_HEIGHT,
    )


@dataclass
class AObj:
    kind: int = 0
    value_base: float = field(default_factory=lambda: f32(0.0))
    value_target: float = field(default_factory=lambda: f32(0.0))
    rate_base: float = field(default_factory=lambda: f32(0.0))
    rate_target: float = field(default_factory=lambda: f32(0.0))
    length: float = field(default_factory=lambda: f32(0.0))
    length_invert: float = field(default_factory=lambda: f32(1.0))


@dataclass
class MaterialState:
    payload: bytes
    spec: MObjSpec
    pc: int = field(init=False)
    anim_changed: bool = field(default=True, init=False)
    anim_wait: float = field(default_factory=lambda: f32(0.0), init=False)
    anim_speed: float = field(default_factory=lambda: f32(1.0), init=False)
    anim_frame: float = field(default_factory=lambda: f32(0.0), init=False)
    aobjs: dict[int, AObj] = field(default_factory=dict, init=False)
    texture_id_curr: int = field(default=0, init=False)
    texture_id_next: int = field(default=0, init=False)
    trau: float = field(init=False)
    trav: float = field(init=False)
    scau: float = field(init=False)
    scav: float = field(init=False)
    scrollu: float = field(init=False)
    scrollv: float = field(init=False)
    lfrac: float = field(init=False)

    def __post_init__(self) -> None:
        self.pc = self.spec.script_offset
        self.trau = self.spec.trau
        self.trav = self.spec.trav
        self.scau = self.spec.scau
        self.scav = self.spec.scav
        self.scrollu = self.spec.scrollu
        self.scrollv = self.spec.scrollv
        self.lfrac = fdiv(self.spec.prim_l, 255.0)

    def aobj(self, track: int) -> AObj:
        result = self.aobjs.get(track)
        if result is None:
            result = AObj()
            self.aobjs[track] = result
        return result

    def read_word(self) -> int:
        result = read_be_u32(self.payload, self.pc, self.spec.name)
        self.pc += 4
        return result

    def read_float(self) -> float:
        result = read_be_f32(self.payload, self.pc, self.spec.name)
        self.pc += 4
        return f32(result)

    def set_common_track(
        self,
        aobj: AObj,
        payload: float,
        kind: int,
        target: float,
    ) -> None:
        aobj.value_base = aobj.value_target
        aobj.value_target = target
        aobj.kind = kind
        aobj.length = fsub(fneg(self.anim_wait), self.anim_speed)
        if payload != 0.0 and kind == 2:
            aobj.rate_base = fdiv(
                fsub(aobj.value_target, aobj.value_base), payload
            )

    def parse(self) -> None:
        # gcParseMObjMatAnimJoint(), objanim.c:830.
        if self.anim_changed:
            self.anim_wait = fneg(self.anim_frame)
            self.anim_changed = False
        else:
            self.anim_wait = fsub(self.anim_wait, self.anim_speed)
            self.anim_frame = fadd(self.anim_frame, self.anim_speed)
            if self.anim_wait > 0.0:
                return

        parse_iterations = 0
        while self.anim_wait <= 0.0:
            parse_iterations += 1
            if parse_iterations > 128:
                raise falsify(f"{self.spec.name}: material parser did not block")
            command_word = self.read_word()
            opcode = command_word >> 25
            flags = (command_word >> 15) & 0x3FF
            payload = f32(command_word & 0x7FFF)

            if opcode in (8, 9):  # SetVal0RateBlock / SetVal0Rate
                for track in range(10):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self.aobj(track)
                        aobj.value_base = aobj.value_target
                        aobj.value_target = self.read_float()
                        aobj.rate_base = aobj.rate_target
                        aobj.rate_target = f32(0.0)
                        aobj.kind = 3
                        if payload != 0.0:
                            aobj.length_invert = fdiv(1.0, payload)
                        aobj.length = fsub(fneg(self.anim_wait), self.anim_speed)
                    flags >>= 1
                if opcode == 8:
                    self.anim_wait = fadd(self.anim_wait, payload)

            elif opcode in (3, 4):  # SetValBlock / SetVal
                for track in range(10):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self.aobj(track)
                        self.set_common_track(
                            aobj, payload, 2, self.read_float()
                        )
                        aobj.rate_target = f32(0.0)
                    flags >>= 1
                if opcode == 3:
                    self.anim_wait = fadd(self.anim_wait, payload)

            elif opcode in (5, 6):  # SetValRateBlock / SetValRate
                for track in range(10):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self.aobj(track)
                        aobj.value_base = aobj.value_target
                        aobj.value_target = self.read_float()
                        aobj.rate_base = aobj.rate_target
                        aobj.rate_target = self.read_float()
                        aobj.kind = 3
                        if payload != 0.0:
                            aobj.length_invert = fdiv(1.0, payload)
                        aobj.length = fsub(fneg(self.anim_wait), self.anim_speed)
                    flags >>= 1
                if opcode == 5:
                    self.anim_wait = fadd(self.anim_wait, payload)

            elif opcode == 7:  # SetTargetRate
                for track in range(10):
                    if flags == 0:
                        break
                    if flags & 1:
                        self.aobj(track).rate_target = self.read_float()
                    flags >>= 1

            elif opcode == 2:  # Wait
                self.anim_wait = fadd(self.anim_wait, payload)

            elif opcode in (10, 11):  # SetValAfterBlock / SetValAfter
                for track in range(10):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self.aobj(track)
                        self.set_common_track(
                            aobj, payload, 1, self.read_float()
                        )
                        aobj.length_invert = payload
                        aobj.rate_target = f32(0.0)
                    flags >>= 1
                if opcode == 10:
                    self.anim_wait = fadd(self.anim_wait, payload)

            elif opcode == 14:  # SetAnim
                relocation_word = self.read_word()
                target = (relocation_word & 0xFFFF) * 4
                if target != self.spec.script_offset:
                    raise falsify(
                        f"{self.spec.name}: material loop targets 0x{target:x}, "
                        f"expected self at 0x{self.spec.script_offset:x}"
                    )
                self.pc = target
                self.anim_frame = fneg(self.anim_wait)

            else:
                raise falsify(
                    f"{self.spec.name}: unsupported material opcode {opcode} "
                    f"at 0x{self.pc - 4:x}"
                )

    @staticmethod
    def cubic_value(aobj: AObj) -> float:
        # Preserve the source statement grouping and round after every MIPS
        # single-precision operation.
        temp_f16 = fmul(aobj.length_invert, aobj.length_invert)
        temp_f12 = fmul(aobj.length, aobj.length)
        temp_f18 = fmul(aobj.length_invert, temp_f12)
        temp_f14 = fmul(fmul(aobj.length, temp_f12), temp_f16)
        temp_f20 = fmul(fmul(2.0, temp_f14), aobj.length_invert)
        temp_f22 = fmul(fmul(3.0, temp_f12), temp_f16)
        temp_f24 = fsub(temp_f14, temp_f18)

        term0 = fmul(aobj.value_base, fadd(fsub(temp_f20, temp_f22), 1.0))
        term1 = fmul(aobj.value_target, fsub(temp_f22, temp_f20))
        term2 = fmul(
            aobj.rate_base,
            fadd(fsub(temp_f24, temp_f18), aobj.length),
        )
        term3 = fmul(aobj.rate_target, temp_f24)
        return fadd(fadd(fadd(term0, term1), term2), term3)

    def play(self) -> None:
        # gcPlayMObjMatAnim(), objanim.c:1244.  dict insertion order matches
        # gcAppendAObjToMObj's first-observation list order, though these tracks
        # do not depend on one another during playback.
        for track, aobj in self.aobjs.items():
            if aobj.kind == 0:
                continue
            aobj.length = fadd(aobj.length, self.anim_speed)
            if aobj.kind == 2:
                value = fadd(aobj.value_base, fmul(aobj.length, aobj.rate_base))
            elif aobj.kind == 3:
                value = self.cubic_value(aobj)
            elif aobj.kind == 1:
                value = (
                    aobj.value_target
                    if aobj.length_invert <= aobj.length
                    else aobj.value_base
                )
            else:
                raise falsify(f"{self.spec.name}: invalid AObj kind {aobj.kind}")

            if track == 0:
                self.texture_id_curr = int(value) & 0xFFFF
            elif track == 1:
                self.trau = value
            elif track == 2:
                self.trav = value
            elif track == 3:
                self.scau = value
            elif track == 4:
                self.scav = value
            elif track == 5:
                self.texture_id_next = int(value) & 0xFFFF
            elif track == 6:
                self.scrollu = value
            elif track == 7:
                self.scrollv = value
            elif track == 8:
                self.lfrac = value

    def tick(self) -> None:
        self.parse()
        self.play()


@dataclass(frozen=True, order=True)
class WaterKey:
    owner: int
    texture0: int
    texture1: int
    tile0_uls: int
    tile0_ult: int
    tile0_lrs: int
    tile0_lrt: int
    tile1_uls: int
    tile1_ult: int
    tile1_lrs: int
    tile1_lrt: int
    fraction: int

    def as_list(self) -> list[int]:
        return [
            self.texture0,
            self.texture1,
            self.tile0_uls,
            self.tile0_ult,
            self.tile0_lrs,
            self.tile0_lrt,
            self.tile1_uls,
            self.tile1_ult,
            self.tile1_lrs,
            self.tile1_lrt,
            self.fraction,
        ]


def material_origin(
    spec: MObjSpec, state: MaterialState, scroll: bool
) -> tuple[int, int]:
    source_u = state.scrollu if scroll else state.trau
    source_v = state.scrollv if scroll else state.trav
    width = spec.scroll_width if scroll else spec.width
    height = spec.scroll_height if scroll else spec.height

    uls_float = fmul(
        fdiv(fadd(fmul(width, source_u), spec.bias), state.scau), 4.0
    )
    one_minus_scale = fsub(1.0, state.scav)
    scale_minus_translation = fsub(one_minus_scale, source_v)
    ult_float = fmul(
        fdiv(
            fadd(fmul(scale_minus_translation, height), spec.bias),
            state.scav,
        ),
        4.0,
    )
    # gDPSetTileSize retains the low 12 bits of C's truncation-toward-zero
    # conversion from float to s32.
    return int(uls_float) & 0xFFF, int(ult_float) & 0xFFF


def make_key(spec: MObjSpec, state: MaterialState) -> WaterKey:
    tile0_uls, tile0_ult = material_origin(spec, state, False)
    tile1_uls, tile1_ult = material_origin(spec, state, True)
    width_q = (spec.width - 1) << 2
    height_q = (spec.height - 1) << 2
    fraction = int(fmul(state.lfrac, 255.0)) & 0xFF
    return WaterKey(
        owner=spec.owner,
        texture0=state.texture_id_curr,
        texture1=state.texture_id_next,
        tile0_uls=tile0_uls,
        tile0_ult=tile0_ult,
        tile0_lrs=(tile0_uls + width_q) & 0xFFF,
        tile0_lrt=(tile0_ult + height_q) & 0xFFF,
        tile1_uls=tile1_uls,
        tile1_ult=tile1_ult,
        tile1_lrs=(tile1_uls + width_q) & 0xFFF,
        tile1_lrt=(tile1_ult + height_q) & 0xFFF,
        fraction=fraction,
    )


@dataclass(frozen=True)
class KeyCensus:
    spec: MObjSpec
    sequence: tuple[WaterKey, ...]
    unique_keys: tuple[WaterKey, ...]
    first_frame: dict[WaterKey, int]
    last_new_frame: int


def census_keys(payload: bytes, spec: MObjSpec) -> KeyCensus:
    state = MaterialState(payload, spec)
    sequence: list[WaterKey] = []
    unique_keys: list[WaterKey] = []
    first_frame: dict[WaterKey, int] = {}
    for frame in range(SIMULATED_FRAMES):
        state.tick()
        key = make_key(spec, state)
        sequence.append(key)
        if key not in first_frame:
            first_frame[key] = frame
            unique_keys.append(key)

    if tuple(sequence[:EXPECTED_CYCLE_FRAMES]) != tuple(
        sequence[EXPECTED_CYCLE_FRAMES : EXPECTED_CYCLE_FRAMES * 2]
    ):
        raise falsify(
            f"{spec.name}: live key stream is not {EXPECTED_CYCLE_FRAMES}-frame "
            "periodic from the initial parse/play sample"
        )
    expected_keys, _ = EXPECTED_OWNER_COUNTS[spec.owner]
    if len(unique_keys) != expected_keys:
        raise falsify(
            f"{spec.name}: {len(unique_keys)} live keys != expected {expected_keys}"
        )
    last_new_frame = max(first_frame.values())
    if last_new_frame != EXPECTED_CYCLE_FRAMES - 1:
        raise falsify(
            f"{spec.name}: final new key arrived at frame {last_new_frame}, "
            f"expected {EXPECTED_CYCLE_FRAMES - 1}"
        )
    return KeyCensus(
        spec=spec,
        sequence=tuple(sequence),
        unique_keys=tuple(unique_keys),
        first_frame=first_frame,
        last_new_frame=last_new_frame,
    )


def n64_rgba5551_to_ds(color: int) -> int:
    red = (color >> 11) & 0x1F
    green = (color >> 6) & 0x1F
    blue = (color >> 1) & 0x1F
    alpha = 0x8000 if color & 1 else 0
    return alpha | red | (green << 5) | (blue << 10)


def expand5to8(value: int) -> int:
    value &= 0x1F
    return (value << 3) | (value >> 2)


BAYER4X4 = (
    0,
    8,
    2,
    10,
    12,
    4,
    14,
    6,
    3,
    11,
    1,
    9,
    15,
    7,
    13,
    5,
)

ALPHA_PHASE_PREFIX = (
    0x0000,
    0x0001,
    0x0401,
    0x0405,
    0x0505,
    0x0525,
    0x8525,
    0x85A5,
    0xA5A5,
    0xA5A7,
    0xADA7,
    0xADAF,
    0xAFAF,
    0xAFBF,
    0xEFBF,
    0xEFFF,
    0xFFFF,
)


def blend_value(texel0: int, texel1: int, fraction: int) -> int:
    fraction = min(fraction, 0xFF)
    inverse = 0x100 - fraction
    red = (
        expand5to8(texel0) * inverse + expand5to8(texel1) * fraction
    ) >> 8 >> 3
    green = (
        expand5to8(texel0 >> 5) * inverse
        + expand5to8(texel1 >> 5) * fraction
    ) >> 8 >> 3
    blue = (
        expand5to8(texel0 >> 10) * inverse
        + expand5to8(texel1 >> 10) * fraction
    ) >> 8 >> 3
    alpha_coverage = (
        ((texel0 >> 15) & 1) * 0x100 * inverse
        + ((texel1 >> 15) & 1) * 0x100 * fraction
    ) >> 8
    return red | (green << 5) | (blue << 10) | (alpha_coverage << 15)


def blend_reference(
    texel0: int, texel1: int, fraction: int, x: int, y: int
) -> int:
    value = blend_value(texel0, texel1, fraction)
    alpha_coverage = value >> 15
    threshold = (BAYER4X4[((y & 3) << 2) | (x & 3)] << 4) + 8
    alpha = 1 if alpha_coverage > threshold else 0
    return (value & 0x7FFF) | (alpha << 15)


def build_pair_lut(palette: Sequence[int], fraction: int) -> tuple[int, ...]:
    result: list[int] = []
    for index0 in range(16):
        for index1 in range(16):
            value = blend_value(palette[index0], palette[index1], fraction)
            alpha_coverage = value >> 15
            prefix_count = min((alpha_coverage + 7) >> 4, 16)
            result.append(
                (value & 0x7FFF) | (ALPHA_PHASE_PREFIX[prefix_count] << 16)
            )
    return tuple(result)


def resolve_pair(pair: int, x: int, y: int) -> int:
    phase = ((y & 3) << 2) | (x & 3)
    return (pair & 0x7FFF) | (((pair >> (16 + phase)) & 1) << 15)


def c_divmod(value: int, divisor: int) -> tuple[int, int]:
    quotient = abs(value) // divisor
    if value < 0:
        quotient = -quotient
    return quotient, value - quotient * divisor


def texture_address_coord(
    coord: int,
    logical_extent: int,
    source_extent: int,
    mode: int,
    mask: int,
) -> int:
    if source_extent == 0:
        return 0
    if (
        coord >= 0
        and coord < source_extent
        and (logical_extent == 0 or coord < logical_extent)
        and (mask == 0 or mask >= 31 or coord < (1 << mask))
    ):
        return coord
    if mode & TX_CLAMP:
        if coord < 0:
            coord = 0
        elif logical_extent != 0 and coord >= logical_extent:
            coord = logical_extent - 1
    if mask != 0 and mask < 31:
        mask_extent = 1 << mask
        period, local = c_divmod(coord, mask_extent)
        if local < 0:
            local += mask_extent
            period -= 1
        if mode & TX_MIRROR and period & 1:
            local = mask_extent - 1 - local
        return local if local < source_extent else source_extent - 1
    _, local = c_divmod(coord, source_extent)
    if local < 0:
        local += source_extent
    if mode & TX_CLAMP:
        return coord if coord < source_extent else source_extent - 1
    return local


def masked_address(coord: int, mode: int, mask: int) -> int:
    mask_extent = 1 << mask
    period = coord >> mask
    local = coord & (mask_extent - 1)
    if mode & TX_MIRROR and period & 1:
        local = mask_extent - 1 - local
    return local


def tile_origin_delta(primary: int, secondary: int) -> int:
    delta = (primary - secondary) & 0xFFF
    if delta & 0x800:
        delta -= 0x1000
    return delta


def quarter_to_texel(coord: int) -> int:
    if coord < 0:
        return -((-coord + 2) >> 2)
    return (coord + 2) >> 2


def ci4_index(texture: bytes, logical_index: int) -> int:
    packed = texture[logical_index >> 1]
    return packed >> 4 if logical_index & 1 == 0 else packed & 0x0F


def reference_masked_address(coord: int, mode: int, mask: int) -> int:
    """Straightforward oracle form, independent of masked_address()."""
    extent = 2**mask
    period = coord // extent
    local = coord % extent
    if mode & TX_MIRROR and period % 2:
        local = extent - 1 - local
    return local


def reference_texture_address(
    coord: int,
    logical_extent: int,
    source_extent: int,
    mode: int,
    mask: int,
) -> int:
    """Literal clamp/mask/mirror oracle, separate from the optimized helper."""
    if source_extent <= 0:
        return 0
    if mode & TX_CLAMP:
        coord = max(0, min(coord, logical_extent - 1))
    if mask:
        extent = 2**mask
        period = coord // extent
        local = coord % extent
        if mode & TX_MIRROR and period % 2:
            local = extent - 1 - local
        return min(local, source_extent - 1)
    if mode & TX_CLAMP:
        return min(coord, source_extent - 1)
    return coord % source_extent


@dataclass(frozen=True)
class SourceCorpus:
    bank103: O2RResource
    bank104: O2RResource
    palette: tuple[int, ...]
    textures: tuple[bytes, bytes, bytes]
    specs: tuple[MObjSpec, MObjSpec]


def load_source_corpus(repo_root: Path) -> SourceCorpus:
    bank103 = load_o2r_resource(
        repo_root,
        BANK103_RELATIVE,
        BANK103_SHA256,
        BANK103_PAYLOAD_SHA256,
        0x67,
        0,
    )
    bank104 = load_o2r_resource(
        repo_root,
        BANK104_RELATIVE,
        BANK104_SHA256,
        BANK104_PAYLOAD_SHA256,
        0x68,
        57,
    )
    if PALETTE_OFFSET + 32 > len(bank103.payload):
        raise falsify("Bank103 water palette is out of range")
    palette_n64 = struct.unpack_from(">16H", bank103.payload, PALETTE_OFFSET)
    palette = tuple(n64_rgba5551_to_ds(value) for value in palette_n64)
    textures: list[bytes] = []
    for offset in TEXTURE_OFFSETS:
        end = offset + CI4_SOURCE_BYTES
        if end > len(bank103.payload):
            raise falsify(f"Bank103 CI4 source at 0x{offset:x} is out of range")
        textures.append(bank103.payload[offset:end])

    large = parse_mobj_spec(
        bank104.payload,
        OWNER_LARGE,
        "large",
        LARGE_MOBJ_SUB_OFFSET,
        LARGE_SCRIPT_OFFSET,
        TEXTURE_OFFSETS,
    )
    small = parse_mobj_spec(
        bank104.payload,
        OWNER_SMALL,
        "small",
        SMALL_MOBJ_SUB_OFFSET,
        SMALL_SCRIPT_OFFSET,
        (TEXTURE_OFFSETS[0], TEXTURE_OFFSETS[1], TEXTURE_OFFSETS[1]),
    )
    if (large.width, large.height, large.scroll_width, large.scroll_height) != (
        128,
        128,
        128,
        128,
    ):
        raise falsify(f"large: dimensions changed: {large.width}x{large.height}")
    if (small.width, small.height, small.scroll_width, small.scroll_height) != (
        32,
        64,
        32,
        64,
    ):
        raise falsify(f"small: dimensions changed: {small.width}x{small.height}")
    return SourceCorpus(
        bank103=bank103,
        bank104=bank104,
        palette=palette,
        textures=tuple(textures),  # type: ignore[arg-type]
        specs=(large, small),
    )


def key_sources(
    corpus: SourceCorpus, spec: MObjSpec, key: WaterKey
) -> tuple[bytes, bytes]:
    if key.texture0 >= len(spec.sprite_offsets) or key.texture1 >= len(
        spec.sprite_offsets
    ):
        raise falsify(
            f"{spec.name}: texture IDs {key.texture0}/{key.texture1} are out of range"
        )
    offset0 = spec.sprite_offsets[key.texture0]
    offset1 = spec.sprite_offsets[key.texture1]
    try:
        index0 = TEXTURE_OFFSETS.index(offset0)
        index1 = TEXTURE_OFFSETS.index(offset1)
    except ValueError as exc:
        raise falsify(f"{spec.name}: unknown source texture offset") from exc
    return corpus.textures[index0], corpus.textures[index1]


def render_coordinate_maps(
    spec: MObjSpec, key: WaterKey
) -> tuple[list[int], list[int], list[int], list[int]]:
    delta_s = quarter_to_texel(
        tile_origin_delta(key.tile0_uls, key.tile1_uls)
    )
    delta_t = quarter_to_texel(
        tile_origin_delta(key.tile0_ult, key.tile1_ult)
    )
    source0_s = [
        masked_address(x, TILE0_CMS, TILE_MASK_S) if spec.materialize_s else x
        for x in range(spec.width)
    ]
    source0_t = [
        masked_address(y, TILE0_CMT, TILE_MASK_T) if spec.materialize_t else y
        for y in range(spec.height)
    ]
    source1_s = [
        texture_address_coord(
            x + delta_s,
            spec.width,
            CI4_SOURCE_WIDTH,
            TILE1_CMS,
            TILE_MASK_S,
        )
        for x in range(spec.width)
    ]
    source1_t = [
        texture_address_coord(
            y + delta_t,
            spec.height,
            CI4_SOURCE_HEIGHT,
            TILE1_CMT,
            TILE_MASK_T,
        )
        for y in range(spec.height)
    ]
    return source0_s, source1_s, source0_t, source1_t


def render_reference(
    corpus: SourceCorpus, spec: MObjSpec, key: WaterKey
) -> bytes:
    texture0, texture1 = key_sources(corpus, spec, key)
    delta_s = quarter_to_texel(
        tile_origin_delta(key.tile0_uls, key.tile1_uls)
    )
    delta_t = quarter_to_texel(
        tile_origin_delta(key.tile0_ult, key.tile1_ult)
    )
    output = bytearray(spec.width * spec.height * 2)
    offset = 0
    for y in range(spec.height):
        source0_y = (
            reference_masked_address(y, TILE0_CMT, TILE_MASK_T)
            if spec.materialize_t
            else y
        )
        source1_y = reference_texture_address(
            y + delta_t,
            spec.height,
            CI4_SOURCE_HEIGHT,
            TILE1_CMT,
            TILE_MASK_T,
        )
        row0 = source0_y * CI4_SOURCE_WIDTH
        row1 = source1_y * CI4_SOURCE_WIDTH
        for x in range(spec.width):
            source0_x = (
                reference_masked_address(x, TILE0_CMS, TILE_MASK_S)
                if spec.materialize_s
                else x
            )
            source1_x = reference_texture_address(
                x + delta_s,
                spec.width,
                CI4_SOURCE_WIDTH,
                TILE1_CMS,
                TILE_MASK_S,
            )
            index0 = ci4_index(texture0, row0 + source0_x)
            index1 = ci4_index(texture1, row1 + source1_x)
            color = blend_reference(
                corpus.palette[index0],
                corpus.palette[index1],
                key.fraction,
                x,
                y,
            )
            struct.pack_into("<H", output, offset, color)
            offset += 2
    return bytes(output)


def representative_map(source0: Sequence[int], source1: Sequence[int]) -> list[int]:
    first: dict[tuple[int, int, int], int] = {}
    result: list[int] = []
    for index, (coord0, coord1) in enumerate(zip(source0, source1)):
        class_key = (index & 3, coord1, coord0)
        representative = first.setdefault(class_key, index)
        result.append(representative)
    return result


def compact_exact_rows(expanded: bytes, width: int, height: int) -> tuple[bytes, bytes]:
    """Canonicalize a full image to first-observation exact rows.

    The live renderer's address-class map is intentionally conservative and
    can stage distinct source-coordinate classes whose final palette/blend
    bytes happen to match.  An AOT artifact already has the exact final bytes,
    so it can use the same compact-row + row-map wire format with true byte
    uniqueness.  Expansion remains byte-identical and is checked below.
    """
    row_bytes = width * 2
    expected_bytes = row_bytes * height
    if len(expanded) != expected_bytes:
        raise falsify(
            f"compact rows: expanded bytes {len(expanded)} != {expected_bytes}"
        )
    unique_rows = bytearray()
    row_map = bytearray(height)
    row_index: dict[bytes, int] = {}
    for y in range(height):
        row = expanded[y * row_bytes : (y + 1) * row_bytes]
        index = row_index.get(row)
        if index is None:
            index = len(row_index)
            if index > 0xFF:
                raise falsify("compact rows: more than 256 unique rows")
            row_index[row] = index
            unique_rows.extend(row)
        row_map[y] = index
    return bytes(unique_rows), bytes(row_map)


@dataclass(frozen=True)
class RenderedOutput:
    expanded: bytes
    compact_rows: bytes
    row_map: bytes
    unique_rows: int


def render_optimized(
    corpus: SourceCorpus, spec: MObjSpec, key: WaterKey
) -> RenderedOutput:
    texture0, texture1 = key_sources(corpus, spec, key)
    source0_s, source1_s, source0_t, source1_t = render_coordinate_maps(spec, key)
    pair_lut = build_pair_lut(corpus.palette, key.fraction)

    if spec.width * spec.height >= 4096:
        representative_s = representative_map(source0_s, source1_s)
        representative_t = representative_map(source0_t, source1_t)
        compact_rows = bytearray()
        row_map = bytearray(spec.height)
        row_number_by_y: dict[int, int] = {}
        for y in range(spec.height):
            representative_y = representative_t[y]
            if representative_y != y:
                row_map[y] = row_number_by_y[representative_y]
                continue
            row_number_by_y[y] = len(row_number_by_y)
            row_map[y] = row_number_by_y[y]
            row = [0] * spec.width
            row0 = source0_t[y] * CI4_SOURCE_WIDTH
            row1 = source1_t[y] * CI4_SOURCE_WIDTH
            for x in range(spec.width):
                representative_x = representative_s[x]
                if representative_x != x:
                    row[x] = row[representative_x]
                    continue
                index0 = ci4_index(texture0, row0 + source0_s[x])
                index1 = ci4_index(texture1, row1 + source1_s[x])
                pair = pair_lut[(index0 << 4) | index1]
                row[x] = resolve_pair(pair, x, y)
            compact_rows.extend(struct.pack(f"<{spec.width}H", *row))

        row_bytes = spec.width * 2
        expanded = bytearray(spec.width * spec.height * 2)
        for y, row_number in enumerate(row_map):
            source_offset = row_number * row_bytes
            destination_offset = y * row_bytes
            expanded[destination_offset : destination_offset + row_bytes] = (
                compact_rows[source_offset : source_offset + row_bytes]
            )
        expanded_bytes = bytes(expanded)
        exact_rows, exact_row_map = compact_exact_rows(
            expanded_bytes, spec.width, spec.height
        )
        # Exercise the canonical AOT representation independently of the
        # conservative renderer-class expansion above.
        canonical_expanded = bytearray(len(expanded_bytes))
        exact_row_bytes = spec.width * 2
        for y, row_number in enumerate(exact_row_map):
            source_offset = row_number * exact_row_bytes
            destination_offset = y * exact_row_bytes
            canonical_expanded[
                destination_offset : destination_offset + exact_row_bytes
            ] = exact_rows[source_offset : source_offset + exact_row_bytes]
        if bytes(canonical_expanded) != expanded_bytes:
            raise falsify("large: canonical compact rows failed expansion")
        return RenderedOutput(
            expanded=expanded_bytes,
            compact_rows=exact_rows,
            row_map=exact_row_map,
            unique_rows=len(exact_rows) // exact_row_bytes,
        )

    expanded = bytearray(spec.width * spec.height * 2)
    offset = 0
    for y in range(spec.height):
        row0 = source0_t[y] * CI4_SOURCE_WIDTH
        row1 = source1_t[y] * CI4_SOURCE_WIDTH
        for x in range(spec.width):
            index0 = ci4_index(texture0, row0 + source0_s[x])
            index1 = ci4_index(texture1, row1 + source1_s[x])
            pair = pair_lut[(index0 << 4) | index1]
            struct.pack_into("<H", expanded, offset, resolve_pair(pair, x, y))
            offset += 2
    return RenderedOutput(
        expanded=bytes(expanded), compact_rows=b"", row_map=b"", unique_rows=0
    )


@dataclass
class UniqueImage:
    owner: int
    local_index: int
    first_frame: int
    expanded: bytes
    compact_rows: bytes
    row_map: bytes
    unique_rows: int
    expanded_sha256: str
    global_index: int = -1
    payload_offset: int = -1

    @property
    def output_kind(self) -> int:
        return OUTPUT_KIND_COMPACT_ROWS if self.owner == OWNER_LARGE else OUTPUT_KIND_FULL

    @property
    def staged_bytes(self) -> int:
        return len(self.compact_rows) if self.output_kind == OUTPUT_KIND_COMPACT_ROWS else len(self.expanded)

    @property
    def record(self) -> bytes:
        if self.output_kind == OUTPUT_KIND_COMPACT_ROWS:
            return self.compact_rows + self.row_map
        return self.expanded


@dataclass(frozen=True)
class OwnerOutputs:
    census: KeyCensus
    images: tuple[UniqueImage, ...]
    key_to_local_image: dict[WaterKey, int]
    oracle_pixels: int
    output_set_sha256: str


def find_first_mismatch(left: bytes, right: bytes) -> int:
    for index, (left_byte, right_byte) in enumerate(zip(left, right)):
        if left_byte != right_byte:
            return index
    return min(len(left), len(right))


def build_owner_outputs(corpus: SourceCorpus, census: KeyCensus) -> OwnerOutputs:
    images: list[UniqueImage] = []
    image_by_bytes: dict[bytes, int] = {}
    key_to_image: dict[WaterKey, int] = {}
    oracle_pixels = 0
    for key in census.unique_keys:
        reference = render_reference(corpus, census.spec, key)
        optimized = render_optimized(corpus, census.spec, key)
        oracle_pixels += census.spec.width * census.spec.height
        if optimized.expanded != reference:
            mismatch = find_first_mismatch(optimized.expanded, reference)
            raise falsify(
                f"{census.spec.name}: optimized/oracle mismatch at byte {mismatch} "
                f"for first-live frame {census.first_frame[key]}"
            )
        local_image = image_by_bytes.get(reference)
        if local_image is None:
            local_image = len(images)
            image_by_bytes[reference] = local_image
            images.append(
                UniqueImage(
                    owner=census.spec.owner,
                    local_index=local_image,
                    first_frame=census.first_frame[key],
                    expanded=reference,
                    compact_rows=optimized.compact_rows,
                    row_map=optimized.row_map,
                    unique_rows=optimized.unique_rows,
                    expanded_sha256=sha256(reference),
                )
            )
        key_to_image[key] = local_image

    _, expected_images = EXPECTED_OWNER_COUNTS[census.spec.owner]
    if len(images) != expected_images:
        raise falsify(
            f"{census.spec.name}: {len(images)} exact outputs != expected {expected_images}"
        )
    if images[0].expanded_sha256 != EXPECTED_FIRST_OUTPUT_HASH[census.spec.owner]:
        raise falsify(
            f"{census.spec.name}: first output SHA256 {images[0].expanded_sha256} "
            f"!= expected {EXPECTED_FIRST_OUTPUT_HASH[census.spec.owner]}"
        )
    output_set_hash = sha256(b"".join(image.expanded for image in images))
    if output_set_hash != EXPECTED_OUTPUT_SET_HASH[census.spec.owner]:
        raise falsify(
            f"{census.spec.name}: output-set SHA256 {output_set_hash} "
            f"!= expected {EXPECTED_OUTPUT_SET_HASH[census.spec.owner]}"
        )
    if census.spec.owner == OWNER_LARGE:
        unique_row_counts = {image.unique_rows for image in images}
        if not unique_row_counts or min(unique_row_counts) < 1 or max(unique_row_counts) > 64:
            raise falsify(
                "large: exact compact rows exceeded the researched 64-row capacity: "
                f"{sorted(unique_row_counts)}"
            )
        for image in images:
            if (
                len(image.row_map) != 128
                or len(image.compact_rows) != image.unique_rows * 256
            ):
                raise falsify("large: compact row payload shape changed")
    return OwnerOutputs(
        census=census,
        images=tuple(images),
        key_to_local_image=key_to_image,
        oracle_pixels=oracle_pixels,
        output_set_sha256=output_set_hash,
    )


@dataclass(frozen=True)
class GeneratedArtifacts:
    corpus: SourceCorpus
    owners: tuple[OwnerOutputs, OwnerOutputs]
    payload: bytes
    index: bytes
    manifest: bytes

    def files(self) -> dict[str, bytes]:
        return {
            PAYLOAD_NAME: self.payload,
            INDEX_NAME: self.index,
            MANIFEST_NAME: self.manifest,
        }

    def fixture_summary(self) -> dict[str, object]:
        manifest_hash = sha256(self.manifest)
        return {
            "schema": 1,
            "sources": {
                "bank103_sha256": self.corpus.bank103.source_sha256,
                "bank103_payload_sha256": self.corpus.bank103.payload_sha256,
                "bank104_sha256": self.corpus.bank104.source_sha256,
                "bank104_payload_sha256": self.corpus.bank104.payload_sha256,
            },
            "census": {
                "simulated_frames": SIMULATED_FRAMES,
                "cycle_frames": EXPECTED_CYCLE_FRAMES,
                "large_keys": len(self.owners[0].census.unique_keys),
                "small_keys": len(self.owners[1].census.unique_keys),
                "total_keys": sum(len(owner.census.unique_keys) for owner in self.owners),
                "large_outputs": len(self.owners[0].images),
                "small_outputs": len(self.owners[1].images),
                "total_outputs": sum(len(owner.images) for owner in self.owners),
                "oracle_pixels": sum(owner.oracle_pixels for owner in self.owners),
            },
            "outputs": {
                owner.census.spec.name: {
                    "first_key": owner.census.sequence[0].as_list(),
                    "first_output_sha256": owner.images[0].expanded_sha256,
                    "output_set_sha256": owner.output_set_sha256,
                    "last_new_frame": owner.census.last_new_frame,
                    "compact_unique_rows": (
                        sorted({image.unique_rows for image in owner.images})
                        if owner.census.spec.owner == OWNER_LARGE
                        else []
                    ),
                }
                for owner in self.owners
            },
            "artifacts": {
                "payload_bytes": len(self.payload),
                "payload_sha256": sha256(self.payload),
                "index_bytes": len(self.index),
                "index_sha256": sha256(self.index),
                "runtime_artifact_bytes": len(self.payload) + len(self.index),
                "safe_64_row_payload_bytes": (
                    len(self.owners[0].images)
                    * (LARGE_ROW_CAPACITY * 128 * 2 + 128)
                    + len(self.owners[1].images) * 32 * 64 * 2
                ),
                "max_staged_texture_bytes_per_frame": (
                    max(image.staged_bytes for image in self.owners[0].images)
                    + max(image.staged_bytes for image in self.owners[1].images)
                ),
                "manifest_bytes": len(self.manifest),
                "manifest_sha256": manifest_hash,
            },
        }


def build_payload_and_index(
    owners: Sequence[OwnerOutputs],
) -> tuple[bytes, bytes]:
    payload = bytearray()
    all_images: list[UniqueImage] = []
    for owner in owners:
        for image in owner.images:
            image.global_index = len(all_images)
            image.payload_offset = len(payload)
            payload.extend(image.record)
            all_images.append(image)

    image_entries = bytearray()
    for image in all_images:
        image_entries.extend(
            INDEX_IMAGE.pack(
                image.owner,
                image.output_kind,
                image.unique_rows,
                image.payload_offset,
                image.staged_bytes,
                len(image.record),
            )
        )

    global_base: dict[int, int] = {}
    cursor = 0
    for owner in owners:
        global_base[owner.census.spec.owner] = cursor
        cursor += len(owner.images)

    key_records: list[tuple[WaterKey, int]] = []
    for owner in owners:
        for key in owner.census.unique_keys:
            local_image = owner.key_to_local_image[key]
            key_records.append((key, global_base[key.owner] + local_image))
    key_records.sort(key=lambda item: item[0])

    key_entries = bytearray()
    for key, image_index in key_records:
        key_entries.extend(
            INDEX_KEY.pack(
                key.owner,
                key.texture0,
                key.texture1,
                0,
                key.tile0_uls,
                key.tile0_ult,
                key.tile0_lrs,
                key.tile0_lrt,
                key.tile1_uls,
                key.tile1_ult,
                key.tile1_lrs,
                key.tile1_lrt,
                key.fraction,
                0,
                image_index,
            )
        )

    header = INDEX_HEADER.pack(
        INDEX_MAGIC,
        INDEX_VERSION,
        INDEX_HEADER.size,
        INDEX_KEY.size,
        INDEX_IMAGE.size,
        len(key_records),
        len(all_images),
        len(owners[0].images),
        len(owners[1].images),
        EXPECTED_CYCLE_FRAMES,
        OWNER_COUNT,
        len(payload),
        SIMULATED_FRAMES,
    )
    index = header + bytes(image_entries) + bytes(key_entries)
    payload_bytes = bytes(payload)
    verify_index(index, payload_bytes)
    if (
        len(payload_bytes) != EXPECTED_PAYLOAD_BYTES
        or sha256(payload_bytes) != EXPECTED_PAYLOAD_SHA256
    ):
        raise falsify(
            "runtime payload signature changed: "
            f"bytes={len(payload_bytes)} sha256={sha256(payload_bytes)}"
        )
    if len(index) != EXPECTED_INDEX_BYTES or sha256(index) != EXPECTED_INDEX_SHA256:
        raise falsify(
            "runtime index signature changed: "
            f"bytes={len(index)} sha256={sha256(index)}"
        )
    return payload_bytes, index


def verify_index(index: bytes, payload: bytes) -> None:
    if len(index) < INDEX_HEADER.size:
        raise falsify("generated index is truncated")
    (
        magic,
        version,
        header_size,
        key_size,
        image_size,
        key_count,
        image_count,
        large_count,
        small_count,
        cycle_frames,
        owner_count,
        payload_bytes,
        simulated_frames,
    ) = INDEX_HEADER.unpack_from(index)
    if (
        magic != INDEX_MAGIC
        or version != INDEX_VERSION
        or header_size != INDEX_HEADER.size
        or key_size != INDEX_KEY.size
        or image_size != INDEX_IMAGE.size
        or image_count != large_count + small_count
        or cycle_frames != EXPECTED_CYCLE_FRAMES
        or owner_count != OWNER_COUNT
        or payload_bytes != len(payload)
        or simulated_frames != SIMULATED_FRAMES
    ):
        raise falsify("generated index header failed its round-trip check")
    expected_bytes = header_size + image_count * image_size + key_count * key_size
    if expected_bytes != len(index):
        raise falsify(
            f"generated index size {len(index)} != described {expected_bytes}"
        )
    image_offset = header_size
    previous_end = 0
    for image_index in range(image_count):
        owner, kind, unique_rows, offset, staged_bytes, record_bytes = (
            INDEX_IMAGE.unpack_from(index, image_offset + image_index * image_size)
        )
        if owner >= OWNER_COUNT or kind not in (
            OUTPUT_KIND_FULL,
            OUTPUT_KIND_COMPACT_ROWS,
        ):
            raise falsify(f"generated image entry {image_index} has invalid tags")
        if offset != previous_end or offset + record_bytes > len(payload):
            raise falsify(f"generated image entry {image_index} has invalid bounds")
        if kind == OUTPUT_KIND_COMPACT_ROWS:
            if unique_rows == 0 or staged_bytes + 128 != record_bytes:
                raise falsify(
                    f"generated compact image entry {image_index} has invalid shape"
                )
        elif staged_bytes != record_bytes:
            raise falsify(f"generated full image entry {image_index} has invalid shape")
        previous_end = offset + record_bytes
    if previous_end != len(payload):
        raise falsify("generated image table does not cover the payload")

    key_offset = header_size + image_count * image_size
    previous_key: tuple[int, ...] | None = None
    for key_index in range(key_count):
        values = INDEX_KEY.unpack_from(index, key_offset + key_index * key_size)
        key_sort = values[0:3] + values[4:12] + values[12:13]
        image_index = values[14]
        if image_index >= image_count or values[3] != 0 or values[13] != 0:
            raise falsify(f"generated key entry {key_index} is invalid")
        if previous_key is not None and key_sort <= previous_key:
            raise falsify("generated key table is not strictly sorted")
        previous_key = key_sort


def make_manifest(
    corpus: SourceCorpus,
    owners: Sequence[OwnerOutputs],
    payload: bytes,
    index: bytes,
) -> bytes:
    manifest = {
        "schema": 1,
        "description": "BattleShip Pupupu animated-water exact DS AOT corpus",
        "source": {
            "bank103": {
                "path": BANK103_RELATIVE.as_posix(),
                "sha256": corpus.bank103.source_sha256,
                "payload_sha256": corpus.bank103.payload_sha256,
                "data_offset": corpus.bank103.data_offset,
                "palette_offset": PALETTE_OFFSET,
                "texture_offsets": list(TEXTURE_OFFSETS),
            },
            "bank104": {
                "path": BANK104_RELATIVE.as_posix(),
                "sha256": corpus.bank104.source_sha256,
                "payload_sha256": corpus.bank104.payload_sha256,
                "data_offset": corpus.bank104.data_offset,
                "mobj_sub_offsets": [LARGE_MOBJ_SUB_OFFSET, SMALL_MOBJ_SUB_OFFSET],
                "script_offsets": [LARGE_SCRIPT_OFFSET, SMALL_SCRIPT_OFFSET],
            },
        },
        "renderer_signature": {
            "combine_w0": COMBINE_W0,
            "combine_w1": COMBINE_W1,
            "source_format": "CI4+RGBA5551",
            "output_format": "little-endian DS RGB5A1",
            "coverage": "current DS ordered 4x4 TEXEL0/TEXEL1 oracle",
        },
        "census": {
            "simulated_frames": SIMULATED_FRAMES,
            "cycle_frames": EXPECTED_CYCLE_FRAMES,
            "key_count": sum(len(owner.census.unique_keys) for owner in owners),
            "output_count": sum(len(owner.images) for owner in owners),
            "oracle_pixels": sum(owner.oracle_pixels for owner in owners),
            "oracle_mismatches": 0,
        },
        "owners": [],
        "artifacts": {
            PAYLOAD_NAME: {"bytes": len(payload), "sha256": sha256(payload)},
            INDEX_NAME: {"bytes": len(index), "sha256": sha256(index)},
            "index_layout": {
                "header_bytes": INDEX_HEADER.size,
                "image_entry_bytes": INDEX_IMAGE.size,
                "key_entry_bytes": INDEX_KEY.size,
                "key_order": "owner,tex0,tex1,tile0/1 coordinates,fraction",
            },
        },
        "storage": {
            "tight_payload_bytes": len(payload),
            "safe_64_row_payload_bytes": (
                len(owners[0].images)
                * (LARGE_ROW_CAPACITY * 128 * 2 + 128)
                + len(owners[1].images) * 32 * 64 * 2
            ),
            "runtime_payload_plus_index_bytes": len(payload) + len(index),
            "max_staged_texture_bytes_per_frame": (
                max(image.staged_bytes for image in owners[0].images)
                + max(image.staged_bytes for image in owners[1].images)
            ),
        },
    }
    owner_manifests: list[dict[str, object]] = []
    for owner in owners:
        spec = owner.census.spec
        owner_manifests.append(
            {
                "id": spec.owner,
                "name": spec.name,
                "dimensions": [spec.width, spec.height],
                "source_texture_ids": [0, 1],
                "first_key": owner.census.sequence[0].as_list(),
                "key_count": len(owner.census.unique_keys),
                "last_new_frame": owner.census.last_new_frame,
                "output_count": len(owner.images),
                "output_set_sha256": owner.output_set_sha256,
                "compact_unique_rows": (
                    sorted({image.unique_rows for image in owner.images})
                    if spec.owner == OWNER_LARGE
                    else []
                ),
                "images": [
                    {
                        "index": image.local_index,
                        "first_frame": image.first_frame,
                        "expanded_sha256": image.expanded_sha256,
                        "unique_rows": image.unique_rows,
                    }
                    for image in owner.images
                ],
            }
        )
    manifest["owners"] = owner_manifests
    return (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")


def generate(repo_root: Path) -> GeneratedArtifacts:
    corpus = load_source_corpus(repo_root.resolve())
    censuses = tuple(census_keys(corpus.bank104.payload, spec) for spec in corpus.specs)
    owners = tuple(build_owner_outputs(corpus, census) for census in censuses)
    payload, index = build_payload_and_index(owners)
    manifest = make_manifest(corpus, owners, payload, index)
    return GeneratedArtifacts(
        corpus=corpus,
        owners=owners,  # type: ignore[arg-type]
        payload=payload,
        index=index,
        manifest=manifest,
    )


def write_artifacts(
    artifacts: GeneratedArtifacts,
    output_dir: Path,
    runtime_only: bool = False,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    files = (
        {
            PAYLOAD_NAME: artifacts.payload,
            INDEX_NAME: artifacts.index,
        }
        if runtime_only
        else artifacts.files()
    )
    for name, payload in files.items():
        (output_dir / name).write_bytes(payload)


def summary_lines(artifacts: GeneratedArtifacts) -> Iterable[str]:
    fixture = artifacts.fixture_summary()
    census = fixture["census"]
    outputs = fixture["outputs"]
    artifact_hashes = fixture["artifacts"]
    assert isinstance(census, dict)
    assert isinstance(outputs, dict)
    assert isinstance(artifact_hashes, dict)
    yield (
        "PUPUPU_WATER_AOT_OK "
        f"frames={census['simulated_frames']} cycle={census['cycle_frames']} "
        f"keys={census['total_keys']} outputs={census['total_outputs']} "
        f"oracle_pixels={census['oracle_pixels']} mismatches=0"
    )
    for name in ("large", "small"):
        owner = outputs[name]
        assert isinstance(owner, dict)
        yield (
            f"PUPUPU_WATER_AOT_OWNER name={name} "
            f"keys={census[name + '_keys']} outputs={census[name + '_outputs']} "
            f"compact_unique_rows={owner['compact_unique_rows']} "
            f"first_sha256={owner['first_output_sha256']} "
            f"set_sha256={owner['output_set_sha256']}"
        )
    yield (
        "PUPUPU_WATER_AOT_ARTIFACT "
        f"payload_bytes={artifact_hashes['payload_bytes']} "
        f"payload_sha256={artifact_hashes['payload_sha256']} "
        f"index_bytes={artifact_hashes['index_bytes']} "
        f"index_sha256={artifact_hashes['index_sha256']}"
    )


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="checkout containing the pinned decomp/BattleShip-main O2R corpus",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="optional directory for payload, index, and audit manifest",
    )
    parser.add_argument(
        "--fixture-json",
        action="store_true",
        help="print the compact golden-fixture summary as JSON",
    )
    parser.add_argument(
        "--runtime-only",
        action="store_true",
        help="write only the payload and index consumed by the DS runtime",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        artifacts = generate(args.repo_root)
        if args.output_dir is not None:
            write_artifacts(artifacts, args.output_dir, args.runtime_only)
        if args.fixture_json:
            print(json.dumps(artifacts.fixture_summary(), indent=2, sort_keys=True))
        else:
            for line in summary_lines(artifacts):
                print(line)
        return 0
    except Falsifier as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
