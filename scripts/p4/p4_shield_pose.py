#!/usr/bin/env python3
"""P4 S15: a native guard-pose package for each content.

The original cast guards with compact NSP1 packages
(scripts/fighters/generate_nds_shield_pose_pack.py, src/nds/nds_shield_pose.c)
in place of their raw ShieldPose files; a content kept its Remix file, 20-42
KB, and the source Event32 path, nine AObjs per joint while it shields. This
builds the same package from the content's own O2R files: Main's nine guard
pointers (FTAttributes.dobj_lookup and shield_anim_joints[8]) name the
DObjDesc rows and the eight 45-degree tables in its ShieldPose file, and the
scripts go through the source generator's encoder and its error oracle (the
0.02 joint-value gate, the stale-AObj check).

Two differences from the original cast's packages, both decoded by the
runtime only where a package uses them: a translate value past Q6's s16
range (Marth's and Roy's guard poses reach 530 units) is stored after the
wide escape token as an s32, and a package whose scripts need more words
than the runtime's stack scratch decodes into a scratch of its own
(NDS_P4_SHIELD_POSE_MAX_*).

A content that guards with its parent's vanilla ShieldPose file at the same
nine targets (Falco with Fox's, Ganondorf with Captain's) uses the parent's
package, as the Polygon Team does; one whose parent has no package (Wario,
Mario's) keeps the raw file.

The package and its row are build output (they derive from the user's ROM).
"""
from __future__ import annotations

import collections
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parents[0]))
sys.path.insert(0, str(HERE.parents[0] / "fighters"))
import ft_layout  # noqa: E402
import generate_nds_shield_pose_pack as SP  # noqa: E402

# include/nds/nds_p4.h: the runtime's limits for content packages.
MAX_BASE_COUNT = 40
MAX_SCRATCH_WORDS = 512
MAX_COMMANDS = 256
# A value token past the package's dictionary: an s32 follows (nds_shield_pose.c).
VALUE_WIDE_ESCAPE = 253
DOBJ_DESC_SIZE = 0x2C
DOBJ_DESC_SENTINEL = 18
# The original cast's packages by vanilla fkind (SP.SPECS).
SPEC_FKINDS = {"Fox": 1, "Donkey": 2, "Samus": 3, "Link": 5, "Captain": 7,
               "Kirby": 8, "Pikachu": 9, "Purin": 10, "Ness": 11}


class ShieldPoseError(Exception):
    pass


def _qround_wide(value: float, frac: int) -> int:
    scaled = value * (1 << frac)
    out = int(scaled + 0.5) if scaled >= 0 else -int(-scaled + 0.5)
    if not -(1 << 23) <= out < (1 << 23):
        raise ShieldPoseError(f"guard value {value!r} Q{frac} exceeds the wide range")
    return out


class _O2R:
    def __init__(self, path: Path):
        blob = path.read_bytes()
        self.file_id, intern, extern, n = struct.unpack_from("<IHHI", blob, 0x40)
        ext = struct.unpack_from(f"<{n}H", blob, 0x4C)
        off = 0x4C + 2 * n
        size = struct.unpack_from("<I", blob, off)[0]
        self.data = blob[off + 4:off + 4 + size]
        self.slots = {}
        for head, deps in ((intern, None), (extern, ext)):
            w, k = head, 0
            while w != 0xFFFF:
                val = struct.unpack_from(">I", self.data, w * 4)[0]
                self.slots[w * 4] = ((self.file_id if deps is None else deps[k]),
                                     (val & 0xFFFF) * 4)
                k += 1
                w = val >> 16

    def u32(self, off: int) -> int:
        return struct.unpack_from(">I", self.data, off)[0]


def guard_targets(o2r: Path, main_id: int, attr: int):
    """Main's nine guard slots and the ShieldPose file and offsets they name."""
    lay = ft_layout.layout()
    main = _O2R(o2r / f"{main_id:04x}")
    lookup_slot = attr + lay["FTAttributes.dobj_lookup"]
    table_slots = [attr + lay["FTAttributes.shield_anim_joints"] + 4 * i for i in range(8)]
    refs = [main.slots.get(s) for s in [lookup_slot] + table_slots]
    if any(r is None for r in refs) or len({r[0] for r in refs}) != 1:
        raise ShieldPoseError("the nine guard pointers do not name one file")
    scales = attr + lay["FTAttributes.translate_scales"]
    if scales in main.slots or main.u32(scales) != 0:
        raise ShieldPoseError("translate_scales is set; the native guard cannot serve it")
    return [lookup_slot] + table_slots, refs[0][0], [r[1] for r in refs]


def _corpus(sp: _O2R, dobj_off: int, table_offs: list[int]):
    rows = 0
    while sp.u32(dobj_off + rows * DOBJ_DESC_SIZE) != DOBJ_DESC_SENTINEL:
        rows += 1
        if rows >= MAX_BASE_COUNT:
            raise ShieldPoseError(f"more than {MAX_BASE_COUNT} DObjDesc rows")
    # The base rows run through the sentinel row, as the source array's
    # count does (the original cast's packages carry joints + 1).
    base_raw = [struct.unpack_from(">II9f", sp.data, dobj_off + i * DOBJ_DESC_SIZE)[2:8]
                for i in range(rows + 1)]
    corpus = []
    for sector, table in enumerate(table_offs):
        for joint in range(rows):
            ref = sp.slots.get(table + 4 * joint)
            if ref is None:
                if sp.u32(table + 4 * joint) != 0:
                    raise ShieldPoseError(f"table {table:#x} joint {joint} is not a pointer")
                corpus.append((0, sector, joint, None, None))
                continue
            if ref[0] != sp.file_id:
                raise ShieldPoseError("a guard script is outside the ShieldPose file")
            corpus.append((0, sector, joint, ref[1], SP.parse_script(sp.data, ref[1])))
    return rows, base_raw, corpus


def _encode(rows: int, base_raw, corpus) -> dict:
    """SP.build's encoder and oracle for one package, with wide values."""
    saved = SP.qround
    SP.qround = _qround_wide
    try:
        base_rows = [(SP.qround(tx, SP.BASE_TRA_FRAC), SP.qround(ty, SP.BASE_TRA_FRAC),
                      SP.qround(tz, SP.BASE_TRA_FRAC), SP.qround(rx, SP.BASE_ROT_FRAC),
                      SP.qround(ry, SP.BASE_ROT_FRAC), SP.qround(rz, SP.BASE_ROT_FRAC))
                     for tx, ty, tz, rx, ry, rz in base_raw]
        if any(not -32768 <= v <= 32767 for row in base_rows for v in row):
            raise ShieldPoseError("a base vector exceeds s16")
        scripts = collections.OrderedDict()
        for *_x, script in corpus:
            if script is not None:
                if len(script) > MAX_COMMANDS:
                    raise ShieldPoseError(f"a guard script has more than {MAX_COMMANDS} commands")
                scripts.setdefault(script, len(scripts))
        templates = collections.OrderedDict()
        for script in scripts:
            templates.setdefault(SP.template_of(script), len(templates))
        command_dict = collections.OrderedDict()
        template_first, template_tokens = [], []
        for template in templates:
            template_first.append(len(template_tokens))
            for word in template:
                command_dict.setdefault(word, len(command_dict))
                template_tokens.append(command_dict[word])
        if len(command_dict) > 255:
            raise ShieldPoseError("more than 255 distinct guard commands")
        ops = sorted({(w >> 25) & 0x7F for w in command_dict})
        flags = sorted({(w >> 15) & 0x3FF for w in command_dict})
        durations = sorted({w & 0x7FFF for w in command_dict})
        if len(ops) > 16 or len(flags) > 64 or len(durations) > 64:
            raise ShieldPoseError("the command dictionary escapes the u16 key layout")
        oi = {v: i for i, v in enumerate(ops)}
        fi = {v: i for i, v in enumerate(flags)}
        di = {v: i for i, v in enumerate(durations)}
        keys = [oi[(w >> 25) & 0x7F] | (fi[(w >> 15) & 0x3FF] << 4) | (di[w & 0x7FFF] << 10)
                for w in command_dict]
        values = [v for script in scripts for v in SP.quant_values(script)]
        frequency = collections.Counter(values)
        first = {v: i for i, v in enumerate(dict.fromkeys(values))}
        # The wide escape is a token past the dictionary, so it keeps
        # VALUE_WIDE_ESCAPE free; dictionary entries are s16.
        ranked = [v for v in sorted(frequency, key=lambda v: (-frequency[v], first[v]))
                  if -32768 <= v <= 32767][:VALUE_WIDE_ESCAPE]
        small = [v for v in ranked if -128 <= v <= 127]
        large = [v for v in ranked if not -128 <= v <= 127]
        index = {v: i for i, v in enumerate(small + large)}
        script_data = bytearray()
        offsets = {}
        scratch = 0
        for script in scripts:
            if len(script_data) >= SP.NULL_HANDLE:
                raise ShieldPoseError("guard script data exceeds u16 offsets")
            offsets[script] = len(script_data)
            template_id = templates[SP.template_of(script)]
            if template_id < SP.TEMPLATE_ESCAPE:
                script_data.append(template_id)
            else:
                script_data.append(SP.TEMPLATE_ESCAPE)
                script_data += struct.pack("<H", template_id)
            words = len(script)
            for v in SP.quant_values(script):
                words += 1
                if v in index:
                    script_data.append(index[v])
                elif -32768 <= v <= 32767:
                    script_data.append(SP.VALUE_ESCAPE)
                    script_data += struct.pack("<h", v)
                else:
                    script_data.append(VALUE_WIDE_ESCAPE)
                    script_data += struct.pack("<i", v)
            scratch = max(scratch, words)
        if scratch > MAX_SCRATCH_WORDS:
            raise ShieldPoseError(f"a guard script needs {scratch} scratch words")
        handles = [SP.NULL_HANDLE if s is None else offsets[s] for *_x, s in corpus]
        base_offsets, base_data = [], bytearray()
        for row in base_rows:
            base_offsets.append(len(base_data))
            base_data.append(sum(1 << i for i, v in enumerate(row) if v != 0))
            for v in row:
                if v != 0:
                    base_data += struct.pack("<h", v)
        samples = sorted(set([44.999999] + [i * 0.25 for i in range(180)]))
        worst = 0.0
        for _p, sector, joint, _src, script in corpus:
            if script is None:
                continue
            for angle in (0.0, 0.25, 0.5, 1.0, 5.0, 12.5, 22.5, 31.0, 40.0, 44.0, 44.999999):
                if SP.eval_script(script, angle, False) != SP.eval_script(script, angle, False,
                                                                          seed=1234.5):
                    raise ShieldPoseError(f"sector {sector} joint {joint} reads stale AObj state")
            for angle in samples:
                for a, b in zip(SP.eval_script(script, angle, False), SP.eval_script_q(script, angle)):
                    if a is not None:
                        worst = max(worst, abs(a - b))
        if worst >= SP.ERROR_GATE:
            raise ShieldPoseError(f"guard pose error {worst:.6f} exceeds {SP.ERROR_GATE}")
        base_error = 0.0
        for tx, ty, tz, rx, ry, rz in base_raw:
            for v in (tx, ty, tz):
                base_error = max(base_error, abs(v - SP.qfloat(SP.qround(v, SP.BASE_TRA_FRAC),
                                                               SP.BASE_TRA_FRAC)))
            for v in (rx, ry, rz):
                base_error = max(base_error, abs(v - SP.qfloat(SP.qround(v, SP.BASE_ROT_FRAC),
                                                               SP.BASE_ROT_FRAC)))
        if base_error >= SP.ERROR_GATE:
            raise ShieldPoseError(f"base vector error {base_error:.6f} exceeds the gate")
    finally:
        SP.qround = saved
    return {"package_meta": [(0, rows, 0, len(base_rows), 0, 0)], "command_keys": keys,
            "command_ops": ops, "command_flags": flags, "command_durations": durations,
            "template_first": template_first, "template_tokens": template_tokens,
            "value_dict_small": small, "value_dict_large": large,
            "script_data": bytes(script_data), "scratch_words": scratch, "handles": handles,
            "base_offsets": base_offsets, "base_data": bytes(base_data),
            "worst_error": worst, "base_error": base_error}


def build(o2r: Path, main_id: int, attr: int, parent_fkind: int):
    """(blob or None, row): the content's own package, or the parent's to
    alias (alias_fkind), or neither (the raw file stays)."""
    slots, shield_id, targets = guard_targets(o2r, main_id, attr)
    row = {"main_asset": main_id, "shield_asset": shield_id, "main_fixup_slots": slots,
           "dobj_offset": targets[0], "table_offsets": targets[1:], "alias_fkind": -1,
           "blob_bytes": 0, "base_count": 0, "scratch_words": 0}
    if any(s > 0xFFFF for s in slots) or any(t > 0xFFFF for t in targets):
        raise ShieldPoseError("a guard slot or target exceeds u16")
    path = o2r / f"{shield_id:04x}"
    if not path.exists():
        spec = next((s for s in SP.SPECS if s.shield_id == shield_id), None)
        if (spec is not None and SPEC_FKINDS[spec.name] == parent_fkind and
                [spec.dobj_off] + list(spec.table_offs) == targets):
            row["alias_fkind"] = SPEC_FKINDS[spec.name]
        return None, row
    sp = _O2R(path)
    rows, base_raw, corpus = _corpus(sp, targets[0], targets[1:])
    data = _encode(rows, base_raw, corpus)
    blob = SP.render_blob(data)
    row.update(blob_bytes=len(blob), base_count=rows + 1, scratch_words=data["scratch_words"],
               source_bytes=len(sp.data), worst_error=data["worst_error"])
    return blob, row
