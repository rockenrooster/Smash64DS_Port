#!/usr/bin/env python3
"""Build and verify the compact DS guard-pose package for the P2-2 capacity set.

BattleShip's guard code never runs ShieldPose as a free-running animation.  It
selects one of eight 45-degree tables, evaluates the selected joint script at
``angle_f`` in [0, 45), applies the stick-magnitude blend, and immediately
clears the animation.  The base fighters migrated here also have
``translate_scales == NULL``.  This generator therefore keeps the source event
semantics but stores only the authored command templates and quantized operands
needed to reproduce that one-shot pose.

The generated data is deliberately simple to consume on ARM9:

* exact duplicate scripts share one byte-stream record;
* each record starts with a u8 template id;
* templates are source command words factored into a u32 dictionary + u8 ids;
* authored values are Q5, authored cubic rates are Q11;
* the 254 most common Q operands are one-byte dictionary ids; the remaining
  values use token 254 followed by a little-endian s16;
* source AObjEvent32* table entries become u16 byte offsets (0xffff = NULL);
* guard-only DObjDesc base translate/rotate vectors become s16 Q7/Q12 rows.

The verifier independently replays the source Event32 commands and the emitted
quantized representation over the whole migrated corpus and a dense angle
grid.  The accepted error gate is the project's existing 0.02 absolute joint
value bound from ``check_r2_cubic_error_bound.py``.
"""

from __future__ import annotations

import argparse
import collections
import json
import pathlib
import struct
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "fighters"))
import estimate_fighter_pack as pack  # noqa: E402


OUT = ROOT / "src" / "nds" / "generated" / "nds_shield_pose_pack.generated.inc"
ASSET_DIR = ROOT / "assets" / "fighters" / "shield_pose"
ASSET_HEADER = ROOT / "include" / "nds" / "generated" / "nds_shield_pose_assets.generated.h"
ASSET_JSON = ROOT / "docs" / "optimization" / "NDS_SHIELD_POSE_ASSETS.generated.json"

BLOB_MAGIC = 0x3150534E  # "NSP1" little-endian
BLOB_VERSION = 1
BLOB_HEADER_BYTES = 64

VALUE_FRAC = 6
RATE_FRAC = 11
BASE_ROT_FRAC = 12
BASE_TRA_FRAC = 6
ERROR_GATE = 0.02
VALUE_DICT_COUNT = 254
VALUE_ESCAPE = 254
NULL_HANDLE = 0xFFFF
TEMPLATE_ESCAPE = 0xFF

ALLOWED_OPS = frozenset((0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12))
RATE_OPS = frozenset((5, 6))
ONE_VALUE_OPS = frozenset((3, 4, 7, 8, 9, 10, 11))


class Spec:
    def __init__(self, name, main_id, shield_id, dobj_off, table_offs):
        self.name = name
        self.main_id = main_id
        self.shield_id = shield_id
        self.dobj_off = dobj_off
        self.table_offs = tuple(table_offs)


SPECS = (
    Spec("Donkey", 213, 318, 0x0110,
         (0x05E0, 0x0A00, 0x0E30, 0x1260, 0x1690, 0x1AB0, 0x1ED0, 0x22E0)),
    Spec("Samus", 217, 322, 0x00F0,
         (0x0570, 0x09A0, 0x0DE0, 0x1210, 0x1650, 0x1A90, 0x1EE0, 0x2310)),
    Spec("Link", 225, 327, 0x0000,
         (0x0580, 0x0A40, 0x0F30, 0x1450, 0x1990, 0x1EB0, 0x23F0, 0x2910)),
    Spec("Kirby", 229, 329, 0x0000,
         (0x0450, 0x0990, 0x0EF0, 0x1420, 0x19D0, 0x1F00, 0x2480, 0x29A0)),
    Spec("Captain", 236, 334, 0x0000,
         (0x04D0, 0x0980, 0x0E50, 0x1320, 0x17E0, 0x1CA0, 0x2180, 0x2650)),
    Spec("Pikachu", 243, 343, 0x0000,
         (0x0500, 0x0B50, 0x11A0, 0x17F0, 0x1E60, 0x24B0, 0x2B00, 0x3150)),
    Spec("Purin", 233, 331, 0x0000,
         (0x0450, 0x0C40, 0x1480, 0x1CA0, 0x2540, 0x2CD0, 0x3560, 0x3DA0)),
)


def qround(value: float, frac: int) -> int:
    scaled = value * (1 << frac)
    out = int(scaled + 0.5) if scaled >= 0 else -int(-scaled + 0.5)
    if not -32768 <= out <= 32767:
        raise RuntimeError("shield-pose Q value exceeds s16: %r Q%d -> %d" %
                           (value, frac, out))
    return out


def qfloat(value: int, frac: int) -> float:
    return value / float(1 << frac)


def read_u32(payload: bytes, off: int) -> int:
    return struct.unpack_from(">I", payload, off)[0]


def read_f32(payload: bytes, off: int) -> float:
    return struct.unpack_from(">f", payload, off)[0]


def parse_script(payload: bytes, start: int):
    pc = start
    out = []
    for _ in range(256):
        word = read_u32(payload, pc)
        opcode = (word >> 25) & 0x7F
        flags = (word >> 15) & 0x3FF
        duration = word & 0x7FFF
        pc += 4
        if opcode not in ALLOWED_OPS:
            raise RuntimeError("ShieldPose escaped closed opcode set: %d @ %#x" %
                               (opcode, pc - 4))
        per = 2 if opcode in RATE_OPS else (1 if opcode in ONE_VALUE_OPS else 0)
        values = []
        for track in range(10):
            if flags & (1 << track):
                if track == 3:
                    raise RuntimeError(
                        "ShieldPose TraI/SYInterpDesc track is not scalar-packable @ %#x"
                        % (pc - 4))
                words = []
                for _i in range(per):
                    words.append(read_f32(payload, pc))
                    pc += 4
                values.append((track, tuple(words)))
        out.append((word, opcode, flags, duration, tuple(values)))
        if opcode == 0:
            return tuple(out)
    raise RuntimeError("ShieldPose script exceeded 256 commands")


def template_of(script):
    return tuple(row[0] for row in script)


def quant_values(script):
    out = []
    for _word, opcode, _flags, _duration, values in script:
        for _track, vals in values:
            for index, value in enumerate(vals):
                out.append(qround(value, RATE_FRAC if opcode in RATE_OPS and index == 1
                                  else VALUE_FRAC))
    return tuple(out)


def eval_script(script, angle: float, quantized: bool, seed=None):
    # kind, vb, vt, rb, rt, length, length_invert.  This is objanim.c's
    # Event32 DObj parser state, restricted to the closed ShieldPose opcode set.
    if seed is None:
        state = [[0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0] for _ in range(10)]
    else:
        # gcAddDObjAnimJoint resets only `kind`; its existing AObj payload is
        # otherwise stale.  Seed every payload field differently to prove the
        # one-shot ShieldPose result does not depend on a previous motion.
        state = [[0,
                  seed + 11.0 * i,
                  seed + 17.0 * i + 3.0,
                  seed + 19.0 * i + 5.0,
                  seed + 23.0 * i + 7.0,
                  seed + 29.0 * i + 11.0,
                  seed + 31.0 * i + 13.0]
                 for i in range(10)]
    anim_wait = -angle
    speed = 1.0
    ended = False

    for _word, opcode, flags, duration, rows in script:
        if opcode == 0:
            for a in state:
                if a[0] != 0:
                    a[5] += speed + anim_wait
            ended = True
            break
        for track, source_values in rows:
            vals = []
            for index, value in enumerate(source_values):
                if quantized:
                    frac = RATE_FRAC if opcode in RATE_OPS and index == 1 else VALUE_FRAC
                    value = qfloat(qround(value, frac), frac)
                vals.append(value)
            a = state[track]
            if opcode in (3, 4):
                a[1], a[2] = a[2], vals[0]
                a[0] = 1
                if duration:
                    a[3] = (a[2] - a[1]) / duration
                a[5] = -anim_wait - speed
                a[4] = 0.0
            elif opcode == 7:
                a[4] = vals[0]
            elif opcode in (8, 9):
                a[1], a[2] = a[2], vals[0]
                a[3], a[4] = a[4], 0.0
                a[0] = 2
                if duration:
                    a[6] = 1.0 / duration
                a[5] = -anim_wait - speed
            elif opcode in (5, 6):
                a[1], a[2] = a[2], vals[0]
                a[3], a[4] = a[4], vals[1]
                a[0] = 2
                if duration:
                    a[6] = 1.0 / duration
                a[5] = -anim_wait - speed
            elif opcode in (10, 11):
                a[1], a[2] = a[2], vals[0]
                a[0] = 3
                a[6] = float(duration)
                a[5] = -anim_wait - speed
                a[4] = 0.0

        if opcode in (3, 8, 5, 10):
            anim_wait += duration
        elif opcode == 12:
            for track in range(10):
                if flags & (1 << track):
                    state[track][5] += duration
        elif opcode == 2:
            anim_wait += duration
        if anim_wait > 0.0:
            break

    result = [None] * 10
    for track, a in enumerate(state):
        kind, vb, vt, rb, rt, length, inv = a
        if kind == 0:
            continue
        if not ended:
            length += speed
        if kind == 1:
            value = vb + (length * rb)
        elif kind == 3:
            value = vt if inv <= length else vb
        else:
            t = length * inv
            t2 = t * t
            t3 = t2 * t
            value = (vb * ((2.0 * t3) - (3.0 * t2) + 1.0) +
                     vt * ((3.0 * t2) - (2.0 * t3)) +
                     rb * length * (1.0 - t) * (1.0 - t) +
                     rt * length * (t2 - t))
        result[track] = value
    return tuple(result)


def _eval_q_kernel(length, inv, vb, vt, rb, rt, kind):
    """Host mirror of include/nds/nds_anim_fixed.h:ndsR2AnimEvalQ."""
    bone = 1 << 16
    if kind == 5:  # NDS_R2_AQ_KIND_STEP
        return vt if inv <= length else vb
    if kind == 6:  # unused by ShieldPose, retained to keep the mirror complete
        return vb + ((length * rb + (1 << 15)) >> 16)
    length = max(-(1024 << 12), min(1024 << 12, length))
    t = (length * inv + (1 << 25)) >> 26
    t = max(-(2 * bone), min(2 * bone, t))
    t2 = (t * t + (bone // 2)) >> 16
    t3 = (t2 * t + (bone // 2)) >> 16
    omt2 = t2 - (2 * t) + bone
    h_vb = (2 * t3) - (3 * t2) + bone
    h_vt = (3 * t2) - (2 * t3)
    h_rb = (length * omt2 + (1 << 11)) >> 12
    h_rt = (length * (t2 - t) + (1 << 11)) >> 12
    return ((vb * h_vb + vt * h_vt + rb * h_rb + rt * h_rt +
             (bone // 2)) >> 16)


def eval_script_q(script, angle: float):
    """The generated ARM9 path: Q12 phase/state, Q30 reciprocal, shared kernel."""
    one = 1 << 12
    scaled_angle = angle * (1 << 12)
    angle_q = (int(scaled_angle + 0.5) if scaled_angle >= 0
               else -int(-scaled_angle + 0.5))
    anim_wait = -angle_q
    ended = False
    # kind, vb(Q12), vt(Q12), rb(Q12), rt(Q12), length(Q12), inv(Q30/Q12)
    state = [[0, 0, 0, 0, 0, 0, 1 << 30] for _ in range(10)]
    for _word, opcode, flags, duration, rows in script:
        if opcode == 0:
            for a in state:
                if a[0] != 0:
                    a[5] += one + anim_wait
            ended = True
            break
        for track, source_values in rows:
            vals = [qround(value, RATE_FRAC if opcode in RATE_OPS and index == 1
                           else VALUE_FRAC)
                    for index, value in enumerate(source_values)]
            a = state[track]
            if opcode in (3, 4):
                a[1], a[2] = a[2], vals[0] << (12 - VALUE_FRAC)
                delta_q12 = a[2] - a[1]
                if duration:
                    numerator = delta_q12 << 4
                    mag = abs(numerator)
                    rate = (mag + duration // 2) // duration
                    a[3] = -rate if numerator < 0 else rate
                a[0] = 6
                a[5] = -anim_wait - one
                a[4] = 0
            elif opcode == 7:
                a[4] = vals[0] << (12 - VALUE_FRAC)
            elif opcode in (8, 9):
                a[1], a[2] = a[2], vals[0] << (12 - VALUE_FRAC)
                a[3], a[4] = a[4], 0
                a[0] = 7
                if duration:
                    a[6] = ((1 << 30) + duration // 2) // duration
                a[5] = -anim_wait - one
            elif opcode in (5, 6):
                a[1], a[2] = a[2], vals[0] << (12 - VALUE_FRAC)
                a[3], a[4] = a[4], vals[1] << (12 - RATE_FRAC)
                a[0] = 7
                if duration:
                    a[6] = ((1 << 30) + duration // 2) // duration
                a[5] = -anim_wait - one
            elif opcode in (10, 11):
                a[1], a[2] = a[2], vals[0] << (12 - VALUE_FRAC)
                a[0] = 5
                a[6] = duration << 12
                a[5] = -anim_wait - one
                a[4] = 0
        if opcode in (3, 8, 5, 10):
            anim_wait += duration << 12
        elif opcode == 12:
            for track in range(10):
                if flags & (1 << track):
                    state[track][5] += duration << 12
        elif opcode == 2:
            anim_wait += duration << 12
        if anim_wait > 0:
            break

    result = [None] * 10
    for track, a in enumerate(state):
        kind, vb, vt, rb, rt, length, inv = a
        if kind == 0:
            continue
        value = _eval_q_kernel(length + (0 if ended else one),
                               inv, vb, vt, rb, rt, kind)
        result[track] = value / 4096.0
    return tuple(result)


def load_corpus():
    types = pack.TypeTable()
    types.load_dirs(pack.HEADER_DIRS)
    corpus = []
    base_rows = []
    package_meta = []
    source_script_bytes = 0

    for spec_index, spec in enumerate(SPECS):
        idx, _entry = pack.index_closure(spec.name, types)
        pf = next(pf for pf in idx.files if pf.file_id == spec.shield_id)
        by_offset = {row.offset: row for row in pf.objects}
        dobj = by_offset[spec.dobj_off]
        if dobj.type_name != "DObjDesc":
            raise RuntimeError("%s ShieldPose DObjDesc moved" % spec.name)
        base_first = len(base_rows)
        for i in range(dobj.count):
            off = spec.dobj_off + i * 44
            row = struct.unpack_from(">II9f", pf.source["payload"], off)
            # `dobj_lookup` is not the fighter's render tree. Guard1 reads only
            # its base translate/rotate fields when blending stick magnitude;
            # the source's pointer/id fields are deliberately outside this
            # compact package (usage audit: ftcommonguard1.c only).
            tx, ty, tz, rx, ry, rz = row[2:8]
            base_rows.append((qround(tx, BASE_TRA_FRAC),
                              qround(ty, BASE_TRA_FRAC),
                              qround(tz, BASE_TRA_FRAC),
                              qround(rx, BASE_ROT_FRAC),
                              qround(ry, BASE_ROT_FRAC),
                              qround(rz, BASE_ROT_FRAC)))

        handle_first = len(corpus)
        table_count = None
        reached = set()
        for sector, table_off in enumerate(spec.table_offs):
            table = by_offset[table_off]
            if table.pointer_depth != 1:
                raise RuntimeError("%s ShieldPose table %#x lost pointer shape" %
                                   (spec.name, table_off))
            if table_count is None:
                table_count = table.count
            elif table.count != table_count:
                raise RuntimeError("%s ShieldPose table counts differ" % spec.name)
            for joint in range(table.count):
                target = pf.source["pointers"].get(table_off + joint * 4)
                if target is None:
                    corpus.append((spec_index, sector, joint, None, None))
                    continue
                if target[0] != spec.shield_id:
                    raise RuntimeError("%s ShieldPose script escaped its file" % spec.name)
                script = parse_script(pf.source["payload"], target[1])
                corpus.append((spec_index, sector, joint, target[1], script))
                reached.add(target[1])
        for off in reached:
            source_script_bytes += by_offset[off].size
        package_meta.append((handle_first, table_count, base_first, dobj.count,
                             spec.main_id, spec.shield_id))

    return corpus, base_rows, package_meta, source_script_bytes


def build():
    corpus, base_rows, package_meta, source_script_bytes = load_corpus()
    unique_scripts = collections.OrderedDict()
    for _package, _sector, _joint, _source, script in corpus:
        if script is not None and script not in unique_scripts:
            unique_scripts[script] = len(unique_scripts)

    templates = collections.OrderedDict()
    for script in unique_scripts:
        template = template_of(script)
        if template not in templates:
            templates[template] = len(templates)

    command_dict = collections.OrderedDict()
    template_first = []
    template_tokens = []
    for template in templates:
        template_first.append(len(template_tokens))
        for word in template:
            if word not in command_dict:
                command_dict[word] = len(command_dict)
            template_tokens.append(command_dict[word])

    command_ops = sorted({(word >> 25) & 0x7F for word in command_dict})
    command_flags = sorted({(word >> 15) & 0x3FF for word in command_dict})
    command_durations = sorted({word & 0x7FFF for word in command_dict})
    if len(command_ops) > 16 or len(command_flags) > 64 or len(command_durations) > 64:
        raise RuntimeError("ShieldPose command dictionary escaped u16 key layout")
    op_index = {value: i for i, value in enumerate(command_ops)}
    flag_index = {value: i for i, value in enumerate(command_flags)}
    duration_index = {value: i for i, value in enumerate(command_durations)}
    command_keys = []
    for word in command_dict:
        opcode = (word >> 25) & 0x7F
        flags = (word >> 15) & 0x3FF
        duration = word & 0x7FFF
        command_keys.append(op_index[opcode] |
                            (flag_index[flags] << 4) |
                            (duration_index[duration] << 10))

    all_values = []
    for script in unique_scripts:
        all_values.extend(quant_values(script))
    frequency = collections.Counter(all_values)
    first_seen = {value: i for i, value in enumerate(dict.fromkeys(all_values))}
    value_dict_ranked = sorted(frequency,
                               key=lambda v: (-frequency[v], first_seen[v]))[:VALUE_DICT_COUNT]
    # Keep token lookup O(1) while halving storage for the common small values:
    # reorder the selected dictionary so all signed-byte entries come first.
    # The token assignment is generated after this reorder, so frequency
    # selection is unchanged; only the resident representation changes.
    value_dict_small = [v for v in value_dict_ranked if -128 <= v <= 127]
    value_dict_large = [v for v in value_dict_ranked if not -128 <= v <= 127]
    value_dict = value_dict_small + value_dict_large
    value_index = {value: i for i, value in enumerate(value_dict)}

    script_data = bytearray()
    script_offsets = {}
    scratch_words = 0
    for script in unique_scripts:
        offset = len(script_data)
        if offset >= NULL_HANDLE:
            raise RuntimeError("ShieldPose script byte offset exceeds u16")
        script_offsets[script] = offset
        template_id = templates[template_of(script)]
        if template_id < TEMPLATE_ESCAPE:
            script_data.append(template_id)
        else:
            script_data.append(TEMPLATE_ESCAPE)
            script_data += struct.pack("<H", template_id)
        words = len(script)
        for value in quant_values(script):
            words += 1
            token = value_index.get(value)
            if token is not None:
                script_data.append(token)
            else:
                script_data.append(VALUE_ESCAPE)
                script_data += struct.pack("<h", value)
        scratch_words = max(scratch_words, words)

    handles = []
    for _package, _sector, _joint, _source, script in corpus:
        handles.append(NULL_HANDLE if script is None else script_offsets[script])

    base_offsets = []
    base_data = bytearray()
    for row in base_rows:
        base_offsets.append(len(base_data))
        mask = sum((1 << i) for i, value in enumerate(row) if value != 0)
        base_data.append(mask)
        for value in row:
            if value != 0:
                base_data += struct.pack("<h", value)

    # Dense source-vs-candidate pose oracle. Include every integer transition,
    # half-step and quarter-step; 44.999999 covers the open upper endpoint.
    samples = sorted(set([44.999999] +
                         [i * 0.25 for i in range(180)]))
    worst = (0.0, None)
    checked = 0
    history_checked = 0
    history_angles = (0.0, 0.25, 0.5, 1.0, 5.0, 12.5, 22.5,
                      31.0, 40.0, 44.0, 44.999999)
    for package, sector, joint, _source, script in corpus:
        if script is None:
            continue
        for angle in history_angles:
            clean = eval_script(script, angle, False)
            stale = eval_script(script, angle, False, seed=1234.5)
            history_checked += 1
            if clean != stale:
                raise RuntimeError(
                    "ShieldPose depends on stale AObj state: package=%d "
                    "sector=%d joint=%d angle=%g" %
                    (package, sector, joint, angle))
        for angle in samples:
            ref = eval_script(script, angle, False)
            got = eval_script_q(script, angle)
            for track, (a, b) in enumerate(zip(ref, got)):
                if a is None:
                    continue
                error = abs(a - b)
                checked += 1
                if error > worst[0]:
                    worst = (error, (package, sector, joint, track, angle, a, b))
    if worst[0] >= ERROR_GATE:
        raise RuntimeError("ShieldPose Q5/Q11 error %.9g exceeds %.9g: %r" %
                           (worst[0], ERROR_GATE, worst[1]))

    # Base range-blend error is independent and convex with the pose error.
    # Quantize source base vectors and assert their own absolute error is below
    # the same gate. Every migrated base fighter has no translate scales.
    max_base_error = 0.0
    types = pack.TypeTable()
    types.load_dirs(pack.HEADER_DIRS)
    for spec in SPECS:
        idx, _entry = pack.index_closure(spec.name, types)
        pf = next(pf for pf in idx.files if pf.file_id == spec.shield_id)
        row = next(row for row in pf.objects if row.offset == spec.dobj_off)
        for i in range(row.count):
            raw = struct.unpack_from(">II9f", pf.source["payload"], spec.dobj_off + i * 44)
            for value in raw[2:5]:
                max_base_error = max(max_base_error,
                                     abs(value - qfloat(qround(value, BASE_TRA_FRAC), BASE_TRA_FRAC)))
            for value in raw[5:8]:
                max_base_error = max(max_base_error,
                                     abs(value - qfloat(qround(value, BASE_ROT_FRAC), BASE_ROT_FRAC)))
    if max_base_error >= ERROR_GATE:
        raise RuntimeError("ShieldPose base-vector error exceeds gate")

    source_pointer_replacement = len(handles) * 2
    # Mirror the estimator's pre-native DObjDesc charge mechanically instead
    # of pinning the original four-fighter subtotal. Retained joint trees cost
    # their source bytes plus u16 refs for every pointer initializer.
    old_joint_tree_keep = 0
    max_base_count = 0
    for spec in SPECS:
        idx, _entry = pack.index_closure(spec.name, types)
        pf = next(pf for pf in idx.files if pf.file_id == spec.shield_id)
        row = next(row for row in pf.objects if row.offset == spec.dobj_off)
        ev = pack.initializer_evidence(row.init_text)
        old_joint_tree_keep += row.size + ev.symbolish * pack.REPL_PTR_REF
        max_base_count = max(max_base_count, row.count)
    dummy_bytes = max_base_count * 44
    base_bytes = len(base_offsets) * 2 + len(base_data)
    command_bytes = (len(command_keys) * 2 + len(command_ops) +
                     len(command_flags) * 2 + len(command_durations) * 2)
    script_bytes = (command_bytes + len(template_tokens) +
                    len(template_first) * 2 + len(value_dict_small) +
                    len(value_dict_large) * 2 + len(script_data))
    old_total = source_script_bytes + source_pointer_replacement + old_joint_tree_keep
    # Runtime metadata that did not exist in the raw source pack: the compact
    # package directory (10 B x N) plus the N DObjDesc and 8*N angle-table source
    # offsets used by the relocation fixup seam (u16 each).  Count it here so
    # the recovery figure is an honest replacement cost, not just payload size.
    metadata_bytes = ((10 * len(SPECS)) + (2 * len(SPECS)) +
                      (2 * 8 * len(SPECS)) + (scratch_words * 4))
    new_total = (script_bytes + source_pointer_replacement + dummy_bytes +
                 base_bytes + metadata_bytes)
    recovery = old_total - new_total

    return {
        "corpus": corpus,
        "base_rows": base_rows,
        "base_offsets": base_offsets,
        "base_data": bytes(base_data),
        "package_meta": package_meta,
        "command_dict": list(command_dict),
        "command_keys": command_keys,
        "command_ops": command_ops,
        "command_flags": command_flags,
        "command_durations": command_durations,
        "template_first": template_first,
        "template_tokens": template_tokens,
        "value_dict": value_dict,
        "value_dict_small": value_dict_small,
        "value_dict_large": value_dict_large,
        "script_data": bytes(script_data),
        "scratch_words": scratch_words,
        "handles": handles,
        "source_script_bytes": source_script_bytes,
        "script_bytes": script_bytes,
        "old_total": old_total,
        "new_total": new_total,
        "metadata_bytes": metadata_bytes,
        "recovery": recovery,
        "worst_error": worst,
        "checked": checked,
        "history_checked": history_checked,
        "max_base_error": max_base_error,
    }


def _rows(values, per=12, fmt=str):
    for i in range(0, len(values), per):
        yield "    " + ", ".join(fmt(v) for v in values[i:i + per]) + ","


def render(data) -> str:
    a = []
    add = a.append
    add("/* Generated by scripts/fighters/generate_nds_shield_pose_pack.py. */")
    add("#define NDS_SHIELD_POSE_PACKAGE_COUNT %du" % len(SPECS))
    add("#define NDS_SHIELD_POSE_TEMPLATE_COUNT %du" % len(data["template_first"]))
    add("#define NDS_SHIELD_POSE_COMMAND_COUNT %du" % len(data["command_dict"]))
    add("#define NDS_SHIELD_POSE_HANDLE_COUNT %du" % len(data["handles"]))
    add("#define NDS_SHIELD_POSE_BASE_COUNT %du" % len(data["base_rows"]))
    add("#define NDS_SHIELD_POSE_SCRIPT_DATA_BYTES %du" % len(data["script_data"]))
    add("#define NDS_SHIELD_POSE_SCRATCH_WORDS %du" % data["scratch_words"])
    add("#define NDS_SHIELD_POSE_SOURCE_SCRIPT_BYTES %du" % data["source_script_bytes"])
    add("#define NDS_SHIELD_POSE_PACK_SCRIPT_BYTES %du" % data["script_bytes"])
    add("#define NDS_SHIELD_POSE_MODELED_OLD_BYTES %du" % data["old_total"])
    add("#define NDS_SHIELD_POSE_MODELED_NEW_BYTES %du" % data["new_total"])
    add("#define NDS_SHIELD_POSE_MODELED_RECOVERY_BYTES %du" % data["recovery"])
    add("#define NDS_SHIELD_POSE_VALUE_ESCAPE 254u")
    add("#define NDS_SHIELD_POSE_TEMPLATE_ESCAPE 255u")
    add("#define NDS_SHIELD_POSE_VALUE_SMALL_COUNT %du" %
        len(data["value_dict_small"]))
    add("#define NDS_SHIELD_POSE_NULL_HANDLE 0xffffu")
    add("#define NDS_SHIELD_POSE_VALUE_FRAC %du" % VALUE_FRAC)
    add("#define NDS_SHIELD_POSE_RATE_FRAC %du" % RATE_FRAC)
    add("#define NDS_SHIELD_POSE_BASE_ROT_FRAC %du" % BASE_ROT_FRAC)
    add("#define NDS_SHIELD_POSE_BASE_TRA_FRAC %du" % BASE_TRA_FRAC)
    add("")
    add("typedef struct NDSShieldPosePackageData {")
    add("    u16 handle_first; u16 base_first; u8 joint_count; u8 base_count;")
    add("    u16 main_asset; u16 shield_asset;")
    add("} NDSShieldPosePackageData;")
    add("")
    add("static const NDSShieldPosePackageData sNdsShieldPosePackages[%d] = {" % len(SPECS))
    for handle_first, joint_count, base_first, base_count, main_id, shield_id in data["package_meta"]:
        add("    { %du, %du, %du, %du, %du, %du }," %
            (handle_first, base_first, joint_count, base_count, main_id, shield_id))
    add("};")
    add("static const u16 sNdsShieldPoseDObjSourceOffset[%d] = {" % len(SPECS))
    add("    " + ", ".join("0x%04xu" % spec.dobj_off for spec in SPECS) + ",")
    add("};")
    add("static const u16 sNdsShieldPoseTableSourceOffset[%d][8] = {" % len(SPECS))
    for spec in SPECS:
        add("    { " + ", ".join("0x%04xu" % off for off in spec.table_offs) + " },")
    add("};")
    add("")
    add("static const u16 sNdsShieldPoseCommandKeys[%d] = {" % len(data["command_keys"]))
    add("\n".join(_rows(data["command_keys"], 12, lambda v: "0x%04xu" % v)))
    add("};")
    add("static const u8 sNdsShieldPoseCommandOps[%d] = {" % len(data["command_ops"]))
    add("\n".join(_rows(data["command_ops"], 16, lambda v: "%du" % v)))
    add("};")
    add("static const u16 sNdsShieldPoseCommandFlags[%d] = {" % len(data["command_flags"]))
    add("\n".join(_rows(data["command_flags"], 12, lambda v: "0x%03xu" % v)))
    add("};")
    add("static const u16 sNdsShieldPoseCommandDurations[%d] = {" %
        len(data["command_durations"]))
    add("\n".join(_rows(data["command_durations"], 12, lambda v: "%du" % v)))
    add("};")
    add("static const u16 sNdsShieldPoseTemplateFirst[%d] = {" % len(data["template_first"]))
    add("\n".join(_rows(data["template_first"], 12, lambda v: "%du" % v)))
    add("};")
    add("static const u8 sNdsShieldPoseTemplateTokens[%d] = {" % len(data["template_tokens"]))
    add("\n".join(_rows(data["template_tokens"], 20, lambda v: "%du" % v)))
    add("};")
    add("static const s8 sNdsShieldPoseValueDictSmall[%d] = {" %
        len(data["value_dict_small"]))
    add("\n".join(_rows(data["value_dict_small"], 16, str)))
    add("};")
    add("static const s16 sNdsShieldPoseValueDictLarge[%d] = {" %
        len(data["value_dict_large"]))
    add("\n".join(_rows(data["value_dict_large"], 12, str)))
    add("};")
    add("static const u8 sNdsShieldPoseScriptData[%d] = {" % len(data["script_data"]))
    add("\n".join(_rows(list(data["script_data"]), 20, lambda v: "0x%02xu" % v)))
    add("};")
    add("static const u16 sNdsShieldPoseHandles[%d] = {" % len(data["handles"]))
    add("\n".join(_rows(data["handles"], 12, lambda v: "0x%04xu" % v)))
    add("};")
    add("static const u16 sNdsShieldPoseBaseOffset[%d] = {" % len(data["base_offsets"]))
    add("\n".join(_rows(data["base_offsets"], 12, lambda v: "%du" % v)))
    add("};")
    add("static const u8 sNdsShieldPoseBaseData[%d] = {" % len(data["base_data"]))
    add("\n".join(_rows(list(data["base_data"]), 20, lambda v: "0x%02xu" % v)))
    add("};")
    add("")
    return "\n".join(a) + "\n"


def _build_for_spec(spec):
    """Run the full source/error oracle for one independently resident blob."""
    global SPECS
    saved = SPECS
    try:
        SPECS = (spec,)
        return build()
    finally:
        SPECS = saved


def _align_blob(blob, alignment=2):
    while len(blob) & (alignment - 1):
        blob.append(0)


def _u16_bytes(values):
    if not values:
        return b""
    return struct.pack("<%dH" % len(values), *values)


def _s16_bytes(values):
    if not values:
        return b""
    return struct.pack("<%dh" % len(values), *values)


def _s8_bytes(values):
    if not values:
        return b""
    return struct.pack("<%db" % len(values), *values)


def render_blob(data):
    """Self-describing, bounds-checkable little-endian resident fighter blob."""
    if len(data["package_meta"]) != 1:
        raise RuntimeError("resident ShieldPose blob must contain exactly one fighter")
    _handle_first, joint_count, _base_first, base_count, _main_id, _shield_id = (
        data["package_meta"][0])
    sections = (
        _u16_bytes(data["command_keys"]),
        bytes(data["command_ops"]),
        _u16_bytes(data["command_flags"]),
        _u16_bytes(data["command_durations"]),
        _u16_bytes(data["template_first"]),
        bytes(data["template_tokens"]),
        _s8_bytes(data["value_dict_small"]),
        _s16_bytes(data["value_dict_large"]),
        bytes(data["script_data"]),
        _u16_bytes(data["handles"]),
        _u16_bytes(data["base_offsets"]),
        bytes(data["base_data"]),
    )
    blob = bytearray(BLOB_HEADER_BYTES)
    offsets = []
    for section in sections:
        _align_blob(blob, 2)
        offsets.append(len(blob))
        blob += section
    if len(blob) > 0xFFFF:
        raise RuntimeError("ShieldPose resident blob exceeds u16 size")
    header_words = (
        BLOB_VERSION,
        BLOB_HEADER_BYTES,
        len(blob),
        joint_count,
        base_count,
        data["scratch_words"],
        len(data["command_keys"]),
        len(data["command_ops"]),
        len(data["command_flags"]),
        len(data["command_durations"]),
        len(data["template_first"]),
        len(data["template_tokens"]),
        len(data["value_dict_small"]),
        len(data["value_dict_large"]),
        len(data["handles"]),
        len(data["base_offsets"]),
        len(data["script_data"]),
        len(data["base_data"]),
        *offsets,
    )
    if len(header_words) != 30:
        raise RuntimeError("ShieldPose blob header field count drifted")
    header = struct.pack("<I30H", BLOB_MAGIC, *header_words)
    if len(header) != BLOB_HEADER_BYTES:
        raise RuntimeError("ShieldPose blob header size drifted")
    blob[:BLOB_HEADER_BYTES] = header
    return bytes(blob)


def render_asset_header(rows, max_base_count, max_scratch_words):
    out = [
        "/* Generated by scripts/fighters/generate_nds_shield_pose_pack.py. */",
        "#pragma once",
        "#define NDS_SHIELD_POSE_BLOB_MAGIC 0x%08xu" % BLOB_MAGIC,
        "#define NDS_SHIELD_POSE_BLOB_VERSION %du" % BLOB_VERSION,
        "#define NDS_SHIELD_POSE_BLOB_HEADER_BYTES %du" % BLOB_HEADER_BYTES,
        "#define NDS_SHIELD_POSE_VALUE_ESCAPE %du" % VALUE_ESCAPE,
        "#define NDS_SHIELD_POSE_TEMPLATE_ESCAPE %du" % TEMPLATE_ESCAPE,
        "#define NDS_SHIELD_POSE_NULL_HANDLE 0xffffu",
        "#define NDS_SHIELD_POSE_VALUE_FRAC %du" % VALUE_FRAC,
        "#define NDS_SHIELD_POSE_RATE_FRAC %du" % RATE_FRAC,
        "#define NDS_SHIELD_POSE_BASE_ROT_FRAC %du" % BASE_ROT_FRAC,
        "#define NDS_SHIELD_POSE_BASE_TRA_FRAC %du" % BASE_TRA_FRAC,
        "#define NDS_SHIELD_POSE_ASSET_COUNT %du" % len(rows),
        "#define NDS_SHIELD_POSE_MAX_BASE_COUNT %du" % max_base_count,
        "#define NDS_SHIELD_POSE_MAX_SCRATCH_WORDS %du" % max_scratch_words,
        "#define NDS_SHIELD_POSE_ASSET_ROWS(X) \\",
    ]
    for index, row in enumerate(rows):
        suffix = " \\" if index + 1 != len(rows) else ""
        args = [
            "nFTKind%s" % row["fighter"],
            "%du" % row["main_asset"],
            "%du" % row["shield_asset"],
            "%du" % row["blob_bytes"],
            "0x%04xu" % row["dobj_offset"],
        ] + ["0x%04xu" % v for v in row["table_offsets"]]
        out.append("    X(%s)%s" % (", ".join(args), suffix))
    out.append("")
    return "\n".join(out)


def build_assets(write=False):
    rows = []
    blobs = {}
    max_base_count = 0
    max_scratch_words = 0
    for spec in SPECS:
        data = _build_for_spec(spec)
        blob = render_blob(data)
        row = {
            "fighter": spec.name,
            "main_asset": spec.main_id,
            "shield_asset": spec.shield_id,
            "blob_bytes": len(blob),
            "old_w_bytes": data["old_total"],
            "net_w_recovery_before_shared_runtime": data["old_total"] - len(blob),
            "dobj_offset": spec.dobj_off,
            "table_offsets": list(spec.table_offs),
            "joint_count": data["package_meta"][0][1],
            "base_count": data["package_meta"][0][3],
            "scratch_words": data["scratch_words"],
            "source_script_bytes": data["source_script_bytes"],
            "compact_script_bytes": data["script_bytes"],
            "max_pose_error": data["worst_error"][0],
            "max_base_error": data["max_base_error"],
            "pose_samples": data["checked"],
            "history_samples": data["history_checked"],
        }
        rows.append(row)
        blobs[spec.name.lower()] = blob
        max_base_count = max(max_base_count, row["base_count"])
        max_scratch_words = max(max_scratch_words, row["scratch_words"])

    header = render_asset_header(rows, max_base_count, max_scratch_words)
    manifest = json.dumps({
        "format": "NSP1",
        "version": BLOB_VERSION,
        "header_bytes": BLOB_HEADER_BYTES,
        "max_base_count": max_base_count,
        "max_scratch_words": max_scratch_words,
        "fighters": rows,
    }, indent=2, sort_keys=True) + "\n"

    if write:
        ASSET_DIR.mkdir(parents=True, exist_ok=True)
        ASSET_HEADER.parent.mkdir(parents=True, exist_ok=True)
        ASSET_JSON.parent.mkdir(parents=True, exist_ok=True)
        for stem, blob in blobs.items():
            path = ASSET_DIR / ("%02d.bin" % next(
                i for i, spec in enumerate(SPECS) if spec.name.lower() == stem))
            # Runtime paths are keyed by fighter kind rather than list ordinal;
            # replace the ordinal path below after the generated row lookup.
            spec = next(s for s in SPECS if s.name.lower() == stem)
            fkind_by_name = {
                "Donkey": 2, "Samus": 3, "Link": 5, "Captain": 7,
                "Kirby": 8, "Pikachu": 9, "Purin": 10,
            }
            path = ASSET_DIR / ("%02d.bin" % fkind_by_name[spec.name])
            path.write_bytes(blob)
        ASSET_HEADER.write_text(header, encoding="utf-8", newline="\n")
        ASSET_JSON.write_text(manifest, encoding="utf-8", newline="\n")
    else:
        for stem, blob in blobs.items():
            spec = next(s for s in SPECS if s.name.lower() == stem)
            fkind_by_name = {
                "Donkey": 2, "Samus": 3, "Link": 5, "Captain": 7,
                "Kirby": 8, "Pikachu": 9, "Purin": 10,
            }
            path = ASSET_DIR / ("%02d.bin" % fkind_by_name[spec.name])
            if not path.exists() or path.read_bytes() != blob:
                raise RuntimeError("generated ShieldPose asset is stale: %s" % path)
        if (not ASSET_HEADER.exists() or
                ASSET_HEADER.read_text(encoding="utf-8") != header):
            raise RuntimeError("generated ShieldPose asset header is stale")
        if (not ASSET_JSON.exists() or
                ASSET_JSON.read_text(encoding="utf-8") != manifest):
            raise RuntimeError("generated ShieldPose asset manifest is stale")
    return rows, max_base_count, max_scratch_words


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--emit-assets", action="store_true")
    ap.add_argument("--check-assets", action="store_true")
    args = ap.parse_args()
    if args.emit_assets or args.check_assets:
        rows, max_base_count, max_scratch_words = build_assets(
            write=args.emit_assets)
        print("NDS_SHIELD_POSE_ASSETS_OK fighters=%d blobs=%d old=%d recovery=%d "
              "max_base=%d max_scratch=%d" %
              (len(rows), sum(r["blob_bytes"] for r in rows),
               sum(r["old_w_bytes"] for r in rows),
               sum(r["net_w_recovery_before_shared_runtime"] for r in rows),
               max_base_count, max_scratch_words))
        return 0
    data = build()
    text = render(data)
    if args.emit:
        OUT.parent.mkdir(parents=True, exist_ok=True)
        OUT.write_text(text, encoding="utf-8", newline="\n")
    if args.check or not args.emit:
        if not OUT.exists() or OUT.read_text(encoding="utf-8") != text:
            raise RuntimeError("generated ShieldPose pack is stale; run with --emit")
    print("NDS_SHIELD_POSE_PACK_OK source_scripts=%d pack_scripts=%d old=%d new=%d "
          "recovery=%d max_error=%.9g base_error=%.9g samples=%d history=%d" %
          (data["source_script_bytes"], data["script_bytes"], data["old_total"],
           data["new_total"], data["recovery"], data["worst_error"][0],
           data["max_base_error"], data["checked"], data["history_checked"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
