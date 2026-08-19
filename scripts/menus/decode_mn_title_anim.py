#!/usr/bin/env python3
"""Decode and evaluate the title screen's label pop animation, offline.

P2-1k (d).  `mnTitlePlayAnim` (mntitle.c:729-748) is not a hand-written
curve: it runs `gcPlayAnimAll` over a GObj whose DObjs carry the
`llMNTitleLabelsAnimJoint` streams, then copies each DObj's scale and
translate into the matching SObj.  The obvious port is to load the MNTitle
reloc container at runtime and call the source's own
`gcSetupCommonDObjs`/`gcAddAnimJointAll`/`gcPlayAnimAll` -- and a cycle spent
doing exactly that ended with a title screen that never handed off.

THIS TOOL EXISTS BECAUSE THE ANIMATION DOES NOT NEED A RUNTIME AT ALL.  Every
input is a constant in a read-only container, the whole thing is 51 frames
long, and the port's own contract says so: "precomputed animation data",
"compute once, not every frame" (`PROJECT_GOAL.md`).  Evaluating it here --
with the interpreter transcribed from `objanim.c` rather than approximated --
produces the pose table a bake can ship, and removes the GObj allocation, the
container load and the reloc-registry mutation that the fault lived inside.

It is also the evidence generator for row P2-1k's numbers.  Two independent
oracles check it, and both are printed by `--verify`:

  1. the REST POSE reproduces `dMNTitleCommonSpriteDescs` (mntitle.c:64)
     exactly, for all seven labels -- so the settled frame is the static
     layout the kit already bakes, to the pixel;
  2. all seven streams reach `End` on the SAME frame, and that frame is 51 --
     which is the source's own `mnTitleSetEndLayout` snap at tic 220 against
     labels shown at tic 170 (P2-1i's transcribed timeline).

Neither is a number this tool chose; both fall out of the container.

Usage:
    python scripts/menus/decode_mn_title_anim.py --verify
    python scripts/menus/decode_mn_title_anim.py --disasm
    python scripts/menus/decode_mn_title_anim.py --pose [--frames N]
    python scripts/menus/decode_mn_title_anim.py --cost
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_mn_ui_kit as kit  # noqa: E402


# ---------------------------------------------------------------------------
# The container's own offsets, from the read-only decomp header that owns them
# (`decomp/BattleShip-main/include/reloc_data.us.h`).  Read rather than
# hardcoded so a reference re-fetch cannot silently move them.
# ---------------------------------------------------------------------------

SYMBOLS = ("llMNTitleLabelsDObjDesc", "llMNTitleLabelsAnimJoint")

# objtypes.h:38.  gcSetupCommonDObjs terminates on `dobjdesc->id ==
# ARRAY_COUNT(array_dobjs)`, and that count is DOBJ_ARRAY_MAX.
DOBJ_ARRAY_MAX = 18
# objtypes.h:368 -- s32 id, void *dl, then three Vec3f.
DOBJDESC_BYTES = 4 + 4 + 12 + 12 + 12

# objtypes.h:41-43.  F32_MIN is the most negative finite f32.
F32_MIN = struct.unpack(">f", bytes.fromhex("FF7FFFFF"))[0]
AOBJ_ANIM_NULL = F32_MIN
AOBJ_ANIM_CHANGED = F32_MIN / 2.0
AOBJ_ANIM_END = F32_MIN / 3.0

# objdef.h:189 (opcodes) and :217 (tracks).  Track bit N of the command word's
# 10-bit `flags` field is track N + nGCAnimTrackJointStart.
KIND_NONE, KIND_LINEAR, KIND_CUBIC, KIND_STEP = 0, 1, 2, 3
TRACKS = ("RotX", "RotY", "RotZ", "TraI", "TraX", "TraY", "TraZ",
          "ScaX", "ScaY", "ScaZ")
OPCODES = {
    0: "End", 1: "Jump", 2: "Wait", 3: "SetValBlock", 4: "SetVal",
    5: "SetValRateBlock", 6: "SetValRate", 7: "SetTargetRate",
    8: "SetVal0RateBlock", 9: "SetVal0Rate", 10: "SetValAfterBlock",
    11: "SetValAfter", 12: "Cmd12", 13: "SetInterp", 14: "SetAnim",
    15: "SetFlags", 16: "Cmd16", 17: "Cmd17",
}
# f32 words each opcode consumes per set track bit.
VALUE_WORDS = {3: 1, 4: 1, 5: 2, 6: 2, 7: 1, 8: 1, 9: 1, 10: 1, 11: 1}

# `mnTitleMakeLabels` builds sprite kinds 0..4 into one GObj and then 5..6,
# in `dMNTitleCommonSpriteDescs` order (mntitle.c:64, :1051), and DObj child
# i carries label i.  That pairing is not assumed: --verify checks every
# label's rest pose against the desc centre below.
#
# THE SIZES ARE READ FROM THE CONTAINER, never written here, and the reason is
# that the rest-pose oracle CANNOT catch a wrong one: mnTitlePlayAnim's own
# `pos = translate + 160 - size*scale*0.5` means the centre this tool
# reconstructs is `pos + size*scale*0.5`, so the size cancels and any value
# passes.  Two of seven were wrong when they were typed in from a screenshot
# (Bros 79x45 against the container's 56x52, TMUnk 23x21 against 32x12), and
# the only report that would have shown it is the destination-area cost --
# i.e. exactly the number the mechanism decision turns on.
LABELS = (
    #  name,          desc centre x, y
    ("Cutout",       157,  94),
    ("Smash",        161,  88),
    ("Super",         55,  96),
    ("Bros",         268,  96),
    ("TMUnk",        270, 132),
    ("Copyright",    160, 208),
    ("BorderUpper",  160,  15),
)

ANIM_FRAMES = 51


def f32(value: float) -> float:
    """Round to f32, as every accumulation in objanim.c is."""
    return struct.unpack("<f", struct.pack("<f", value))[0]


# ---------------------------------------------------------------------------
# objanim.c, transcribed
# ---------------------------------------------------------------------------

class AObj:
    """One animated track.  gcAddAObjForDObj (objman.c:1187) is the zero."""

    __slots__ = ("track", "kind", "value_base", "value_target", "rate_base",
                 "rate_target", "length", "length_invert")

    def __init__(self, track: int) -> None:
        self.track = track
        self.kind = KIND_NONE
        self.value_base = 0.0
        self.value_target = 0.0
        self.rate_base = 0.0
        self.rate_target = 0.0
        self.length = 0.0
        self.length_invert = 1.0

    def value(self) -> float:
        """gcPlayDObjAnimJoint's own three cases (objanim.c:735-765)."""
        if self.kind == KIND_LINEAR:
            return f32(self.value_base + (self.length * self.rate_base))
        if self.kind == KIND_CUBIC:
            li2 = self.length_invert * self.length_invert
            l2 = self.length * self.length
            f18 = self.length_invert * l2
            f14 = self.length * l2 * li2
            f20 = 2.0 * f14 * self.length_invert
            f22 = 3.0 * l2 * li2
            f24 = f14 - f18
            return f32((self.value_base * ((f20 - f22) + 1.0)) +
                       (self.value_target * (f22 - f20)) +
                       (self.rate_base * ((f24 - f18) + self.length)) +
                       (self.rate_target * f24))
        if self.kind == KIND_STEP:
            return (self.value_target if self.length_invert <= self.length
                    else self.value_base)
        return 0.0


class DObj:
    """One node of the label GObj's DObj tree, with its own script cursor."""

    def __init__(self, container, script_off, translate, rotate, scale):
        self.c = container
        self.pc = script_off
        self.anim_wait = (AOBJ_ANIM_CHANGED if script_off is not None
                          else AOBJ_ANIM_NULL)
        self.anim_frame = 0.0
        self.anim_speed = 1.0
        self.aobjs: list[AObj] = []
        self.translate = list(translate)
        self.rotate = list(rotate)
        self.scale = list(scale)

    def _word(self) -> int:
        return struct.unpack_from(">I", self.c.payload, self.pc)[0]

    def _float(self) -> float:
        return struct.unpack_from(">f", self.c.payload, self.pc)[0]

    def _take_float(self) -> float:
        value = self._float()
        self.pc += 4
        return value

    def _track(self, index: int, table: list) -> AObj:
        if table[index] is None:
            aobj = AObj(index + 1)
            self.aobjs.append(aobj)
            table[index] = aobj
        return table[index]

    def parse(self) -> None:
        """gcParseDObjAnimJoint (objanim.c:268-634)."""
        if self.anim_wait == AOBJ_ANIM_NULL:
            return
        # THE END IS A STOP, and the source treats it as one.  The End case
        # returns WITHOUT advancing the script cursor and leaves anim_wait at
        # AOBJ_ANIM_END (-1.13e38); step the same DObj again and that End
        # re-runs `aobj->length += anim_speed + anim_wait`, so every track's
        # length reaches -inf within four frames and every cubic evaluates to
        # NaN.  The original never gets there: mnTitleSetEndLayout snaps the
        # layout at tic 220 and mnTitleProcUpdate stops calling PlayAnim.
        # Freezing here models that -- and the frame each stream first
        # reaches it is the animation's own length, which --verify checks.
        if self.anim_wait == AOBJ_ANIM_END:
            return
        if self.anim_wait == AOBJ_ANIM_CHANGED:
            self.anim_wait = -self.anim_frame
        else:
            self.anim_wait = f32(self.anim_wait - self.anim_speed)
            self.anim_frame = f32(self.anim_frame + self.anim_speed)
            if self.anim_wait > 0.0:
                return
        table: list = [None] * len(TRACKS)
        for aobj in self.aobjs:
            if 1 <= aobj.track <= len(TRACKS):
                table[aobj.track - 1] = aobj
        while True:
            word = self._word()
            opcode = (word >> 25) & 0x7F
            flags = (word >> 15) & 0x3FF
            payload = float(word & 0x7FFF)
            self.pc += 4        # AObjAnimAdvance is a post-increment
            if opcode in (3, 4):                    # SetVal[Block]: linear
                for i in range(len(TRACKS)):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self._track(i, table)
                        aobj.value_base = aobj.value_target
                        aobj.value_target = self._take_float()
                        aobj.kind = KIND_LINEAR
                        if payload != 0.0:
                            aobj.rate_base = f32(
                                (aobj.value_target - aobj.value_base) /
                                payload)
                        aobj.length = f32(-self.anim_wait - self.anim_speed)
                        aobj.rate_target = 0.0
                    flags >>= 1
                if opcode == 3:
                    self.anim_wait = f32(self.anim_wait + payload)
            elif opcode in (5, 6):                  # SetValRate[Block]: cubic
                for i in range(len(TRACKS)):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self._track(i, table)
                        aobj.value_base = aobj.value_target
                        aobj.value_target = self._take_float()
                        aobj.rate_base = aobj.rate_target
                        aobj.rate_target = self._take_float()
                        aobj.kind = KIND_CUBIC
                        if payload != 0.0:
                            aobj.length_invert = f32(1.0 / payload)
                        aobj.length = f32(-self.anim_wait - self.anim_speed)
                    flags >>= 1
                if opcode == 5:
                    self.anim_wait = f32(self.anim_wait + payload)
            elif opcode in (8, 9):                  # SetVal0Rate[Block]
                for i in range(len(TRACKS)):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self._track(i, table)
                        aobj.value_base = aobj.value_target
                        aobj.value_target = self._take_float()
                        aobj.rate_base = aobj.rate_target
                        aobj.rate_target = 0.0
                        aobj.kind = KIND_CUBIC
                        if payload != 0.0:
                            aobj.length_invert = f32(1.0 / payload)
                        aobj.length = f32(-self.anim_wait - self.anim_speed)
                    flags >>= 1
                if opcode == 8:
                    self.anim_wait = f32(self.anim_wait + payload)
            elif opcode in (10, 11):                # SetValAfter[Block]: step
                for i in range(len(TRACKS)):
                    if flags == 0:
                        break
                    if flags & 1:
                        aobj = self._track(i, table)
                        aobj.value_base = aobj.value_target
                        aobj.value_target = self._take_float()
                        aobj.kind = KIND_STEP
                        aobj.length_invert = payload
                        aobj.length = f32(-self.anim_wait - self.anim_speed)
                        aobj.rate_target = 0.0
                    flags >>= 1
                if opcode == 10:
                    self.anim_wait = f32(self.anim_wait + payload)
            elif opcode == 7:                       # SetTargetRate
                for i in range(len(TRACKS)):
                    if flags == 0:
                        break
                    if flags & 1:
                        self._track(i, table).rate_target = self._take_float()
                    flags >>= 1
            elif opcode == 2:                       # Wait
                self.anim_wait = f32(self.anim_wait + payload)
            elif opcode == 12:                      # track-length accumulator
                for i in range(len(TRACKS)):
                    if flags == 0:
                        break
                    if flags & 1:
                        self._track(i, table).length += payload
                    flags >>= 1
            elif opcode == 15:                      # SetFlags
                self.anim_wait = f32(self.anim_wait + payload)
            elif opcode == 0:                       # End
                for aobj in self.aobjs:
                    if aobj.kind != KIND_NONE:
                        aobj.length = f32(aobj.length + self.anim_speed +
                                          self.anim_wait)
                self.anim_frame = self.anim_wait
                self.anim_wait = AOBJ_ANIM_END
                return
            else:
                raise kit.ConvertError(
                    f"title anim: opcode {opcode} ({OPCODES.get(opcode)}) at "
                    f"{self.pc - 4:#x} has no transcription here")
            if self.anim_wait > 0.0:
                return

    def play(self) -> None:
        """gcPlayDObjAnimJoint (objanim.c:714-800), joint tracks only."""
        if self.anim_wait == AOBJ_ANIM_NULL:
            return
        for aobj in self.aobjs:
            if aobj.kind == KIND_NONE:
                continue
            if self.anim_wait != AOBJ_ANIM_END:
                aobj.length = f32(aobj.length + self.anim_speed)
            value = aobj.value()
            track = aobj.track
            if track in (1, 2, 3):
                self.rotate[track - 1] = value
            elif track in (5, 6, 7):
                self.translate[track - 5] = value
            elif track in (8, 9, 10):
                self.scale[track - 8] = value
            # nGCAnimTrackTraI (4) needs an `interpolate` display list and no
            # title stream sets one; --verify fails if one ever appears.
            elif track == 4:
                raise kit.ConvertError(
                    "title anim: a TraI (cubic path) track appeared; it needs "
                    "syInterpCubic and an interpolate pointer")


# ---------------------------------------------------------------------------
# Container access
# ---------------------------------------------------------------------------

def load(repo_root: Path):
    offsets = kit.load_reloc_offsets(repo_root)
    missing = [s for s in SYMBOLS if s not in offsets]
    if missing:
        raise kit.ConvertError(
            f"{', '.join(missing)} missing from the reloc headers")
    return kit.RelocFile(kit.o2r_path(repo_root, "MNTitle")), offsets


def read_tree(container, offsets):
    """The DObjDesc list and the joint pointer paired with each DObj."""
    base = offsets["llMNTitleLabelsDObjDesc"]
    descs = []
    index = 0
    while True:
        off = base + index * DOBJDESC_BYTES
        ident = struct.unpack_from(">i", container.payload, off)[0]
        if ident == DOBJ_ARRAY_MAX:
            break
        descs.append((
            ident,
            struct.unpack_from(">3f", container.payload, off + 8),
            struct.unpack_from(">3f", container.payload, off + 20),
            struct.unpack_from(">3f", container.payload, off + 32),
        ))
        index += 1
        if index > DOBJ_ARRAY_MAX:
            raise kit.ConvertError("title anim: DObjDesc list has no end")
    joint_base = offsets["llMNTitleLabelsAnimJoint"]
    joints = []
    for i in range(len(descs)):
        slot = joint_base + i * 4
        raw = struct.unpack_from(">I", container.payload, slot)[0]
        if slot in container.pointer_targets:
            joints.append(container.pointer_targets[slot])
        elif raw == 0:
            joints.append(None)
        else:
            raise kit.ConvertError(
                f"title anim: joint slot {i} holds {raw:#x}, which is not in "
                "the container's internal reloc list -- the animation would "
                "need a fixup this file does not carry")
    return descs, joints


def label_sizes(container, offsets):
    """Each label sprite's own width/height, out of the container."""
    sizes = []
    for name, _cx, _cy in LABELS:
        symbol = f"llMNTitle{name}Sprite"
        if symbol not in offsets:
            raise kit.ConvertError(f"{symbol} missing from the reloc headers")
        sprite = container.sprite(offsets[symbol])
        sizes.append((int(sprite.width), int(sprite.height)))
    return sizes


def simulate(container, descs, joints, sizes, frames):
    """Every label's pose, frame by frame, in the source's 320x240 frame."""
    dobjs = [DObj(container, joints[i], descs[i][1], descs[i][2], descs[i][3])
             for i in range(len(descs))]
    ended: dict[int, int] = {}
    poses = []
    for frame in range(frames + 1):
        if frame > 0:
            for index, dobj in enumerate(dobjs):
                before = dobj.anim_wait
                dobj.parse()
                if (before != AOBJ_ANIM_END) and \
                        (dobj.anim_wait == AOBJ_ANIM_END):
                    ended[index] = frame
                dobj.play()
        pose = []
        for label_index, dobj in enumerate(dobjs[1:]):
            name = LABELS[label_index][0]
            width, height = sizes[label_index]
            scale_x, scale_y = dobj.scale[0], dobj.scale[1]
            # mnTitlePlayAnim's own two lines (mntitle.c:741-745).
            pos_x = ((dobj.translate[0] + 160.0) -
                     (width * scale_x * 0.5))
            pos_y = ((120.0 - dobj.translate[1]) -
                     (height * scale_y * 0.5))
            pose.append((name, width, height, scale_x, scale_y, pos_x, pos_y))
        poses.append(pose)
    return poses, ended


# ---------------------------------------------------------------------------
# Reports
# ---------------------------------------------------------------------------

def report_disasm(container, descs, joints) -> None:
    for index in range(1, len(descs)):
        off = joints[index]
        if off is None:
            print(f"--- joint[{index}] {LABELS[index - 1][0]}: no stream ---")
            continue
        print(f"--- joint[{index}] {LABELS[index - 1][0]} @ {off:#x} ---")
        while True:
            word = struct.unpack_from(">I", container.payload, off)[0]
            opcode = (word >> 25) & 0x7F
            flags = (word >> 15) & 0x3FF
            payload = word & 0x7FFF
            names = [TRACKS[i] for i in range(len(TRACKS))
                     if flags & (1 << i)]
            print(f"  {off:#08x}: {word:#010x}  "
                  f"{OPCODES.get(opcode, opcode):<17s} dur={payload:<4d} "
                  f"{','.join(names)}")
            off += 4
            if opcode == 0:
                break
            for _ in range(VALUE_WORDS.get(opcode, 0) * len(names)):
                value = struct.unpack_from(">f", container.payload, off)[0]
                print(f"       {off:#08x}:   {value:>14.6f}")
                off += 4
        print()


def report_pose(poses, frames) -> None:
    print("frame |            piece  scaleX  scaleY      posX      posY")
    for frame in range(frames + 1):
        for (name, _w, _h, sx, sy, px, py) in poses[frame]:
            print(f"{frame:5d} | {name:>16s} {sx:7.3f} {sy:7.3f} "
                  f"{px:9.2f} {py:9.2f}")


def report_cost(poses) -> None:
    """What a per-frame scaled re-blit into BG2 would actually move."""
    ratio = kit.FRAME_SCALE[0] / kit.FRAME_SCALE[1]
    peak = peak_frame = total = 0
    print(" fr | on-screen texels | union bbox (DS)")
    for frame in range(ANIM_FRAMES + 1):
        texels = 0
        xs0, ys0, xs1, ys1 = [], [], [], []
        for (_n, w, h, sx, sy, px, py) in poses[frame]:
            x0 = max(0.0, px * ratio)
            y0 = max(0.0, py * ratio)
            x1 = min(float(kit.DS_SCREEN_W), (px + w * sx) * ratio)
            y1 = min(float(kit.DS_SCREEN_H), (py + h * sy) * ratio)
            if (x1 > x0) and (y1 > y0):
                texels += int((x1 - x0) * (y1 - y0))
                xs0.append(x0); ys0.append(y0); xs1.append(x1); ys1.append(y1)
        box = ((int(min(xs0)), int(min(ys0)), int(max(xs1)), int(max(ys1)))
               if xs0 else (0, 0, 0, 0))
        total += texels
        if texels > peak:
            peak, peak_frame = texels, frame
        print(f"{frame:3d} | {texels:16d} | {box}")
    screen = kit.DS_SCREEN_W * kit.DS_SCREEN_H
    mean = total // ANIM_FRAMES
    print()
    print(f"PEAK {peak} texels on frame {peak_frame} "
          f"({peak / screen:.2f}x the {screen}-texel screen; the pieces "
          f"overlap), MEAN {mean} over frames 1..{ANIM_FRAMES}")
    for cycles in (6, 10, 14):
        print(f"  at ~{cycles:2d} cyc/texel: peak {peak * cycles:9,d}, "
              f"mean {mean * cycles:9,d} ARM9 ticks "
              f"(60 Hz budget 560,190; 2-VBlank 1,120,380)")
    print("  a full BG2 erase is 49,152 texels on top of the draw.")


def report_verify(poses, ended, joints, descs, sizes) -> int:
    failures = 0
    print(f"DObjs {len(descs)}, joint pointers "
          f"{sum(1 for j in joints if j is not None)} internal, "
          f"{sum(1 for j in joints if j is None)} null, 0 external "
          f"(read_tree raises on an external one)")
    print("label sprite sizes, from the container: " +
          ", ".join(f"{LABELS[i][0]} {w}x{h}"
                    for i, (w, h) in enumerate(sizes)))

    print("\nORACLE 1 -- rest pose == dMNTitleCommonSpriteDescs (mntitle.c:64)")
    for index, (name, cx, cy) in enumerate(LABELS):
        w, h = sizes[index]
        _n, _w, _h, sx, sy, px, py = poses[0][index]
        got = (px + w * sx * 0.5, py + h * sy * 0.5)
        ok = (abs(got[0] - cx) < 0.001) and (abs(got[1] - cy) < 0.001)
        failures += 0 if ok else 1
        print(f"  {name:>12s} centre ({got[0]:6.1f},{got[1]:6.1f}) vs desc "
              f"({cx:3d},{cy:3d})  {'OK' if ok else 'MISMATCH'}")

    print("\nORACLE 2 -- every stream ends on the frame the source snaps on")
    frames = sorted(set(ended.values()))
    ok = (len(frames) == 1) and (frames[0] == ANIM_FRAMES)
    failures += 0 if ok else 1
    print(f"  End reached on frame(s) {frames}; mnTitleSetEndLayout snaps at "
          f"tic 220 = presented frame {ANIM_FRAMES}  "
          f"{'OK' if ok else 'MISMATCH'}")

    print("\nSETTLED POSE == REST POSE (the animation returns to the layout "
          "the kit bakes)")
    same = all(
        (abs(a[3] - b[3]) < 1e-4) and (abs(a[4] - b[4]) < 1e-4) and
        (abs(a[5] - b[5]) < 1e-3) and (abs(a[6] - b[6]) < 1e-3)
        for a, b in zip(poses[0], poses[ANIM_FRAMES]))
    failures += 0 if same else 1
    print(f"  frame 0 vs frame {ANIM_FRAMES}: "
          f"{'identical' if same else 'DIFFERENT'}")
    print(f"\n{'PASS' if failures == 0 else str(failures) + ' FAILURE(S)'}")
    return 1 if failures else 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=Path(__file__).resolve()
                        .parent.parent.parent, type=Path)
    parser.add_argument("--disasm", action="store_true")
    parser.add_argument("--pose", action="store_true")
    parser.add_argument("--cost", action="store_true")
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--frames", type=int, default=ANIM_FRAMES)
    args = parser.parse_args(argv)
    if not (args.disasm or args.pose or args.cost or args.verify):
        args.verify = True

    repo_root = args.repo_root.resolve()
    container, offsets = load(repo_root)
    descs, joints = read_tree(container, offsets)
    if len(descs) - 1 != len(LABELS):
        raise kit.ConvertError(
            f"title anim: {len(descs) - 1} child DObjs against "
            f"{len(LABELS)} labels")
    sizes = label_sizes(container, offsets)
    frames = max(args.frames, ANIM_FRAMES)
    poses, ended = simulate(container, descs, joints, sizes, frames)

    if args.disasm:
        report_disasm(container, descs, joints)
    if args.pose:
        report_pose(poses, args.frames)
    if args.cost:
        report_cost(poses)
    if args.verify:
        return report_verify(poses, ended, joints, descs, sizes)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except kit.ConvertError as exc:
        print(f"decode_mn_title_anim: {exc}", file=sys.stderr)
        sys.exit(1)
