#!/usr/bin/env python3
"""Every DRAWING hidden part a motion installs must have a root program.

THE FAILURE THIS CATCHES is the sibling of
`check_model_part_mutation_coverage.py`, reached through a different source
mechanism. An `ftdata.c` motion row's third field carries `FTANIM_FLAG_*` in its
low half and an anim-desc mask in its high half. A set bit at position `31 - i`
makes `ftMainSetStatus` install `<Fighter>Main.hiddenparts[i]`. If that hidden
part's root joint carries a display list it **adds a root** to the live vector,
and `nds_renderer_assets.c` rejects a native owner on
`owner->root_count != root_count` before it ever compares offsets. No
per-binding variant can represent a root-count change. The owner declines, and
at `NDS_RENDERER_PROFILE_LEVEL 0` a declined owner draws **nothing** -- the whole
fighter disappears for the length of the move.

That is exactly how Yoshi's grab, his B attack and Kirby's copy presented, and
all of them were found by a human watching a fighter vanish rather than by any
check.

A hidden part with NO display list of its own adds a root just the same when
the motion that installed it names it in a `SetModelPartID`: Ness's USmash and
DSmash install joint 30 blank and give it the yo-yo two frames later. This
check skipped every mask whose hidden parts were blank in the JointTree, so it
never saw that shape: Ness drew nothing for the rest of either smash until
P2-2p8 Phase 1 slice 7 (2026-09-24) gave the owner its YoYo program. A blank
hidden part is now followed through each motion that names it.

Indices 0, 1 and 2 are the `TRANSN`/`XROTN`/`YROTN` joints (`kind 3`), which
never carry a display list, so only indices 3 and up can do this.

THE RESOLUTION CHAIN, parsed rather than assumed:

  ftdata.c     `{ &llFT<Title>Anim<Name>FileID, <motion>, <flags|mask> }`
  Main.c       `/* @ 0xNNNN ... hiddenparts target */` gives the table, and
               `FTHiddenPart.root_joint_id - 4` gives the JointTree descriptor
  setup_parts  a descriptor it already selects is canonical, not hidden
  Model.c      the descriptor's display list, with BattleShip's Low fallback
  motion file  that motion's own `SetModelPartID` commands, which change which
               display list each live joint draws

The resulting live root vector must then equal the vector of some emitted root
program for that owner and detail. Nothing else can draw it.

Usage:
    python scripts/fighters/check_hidden_part_root_coverage.py

Exit 0 when every drawing hidden part is covered; 1 otherwise, naming the
fighter, the motions, the mask, the joints and the offsets that no program
carries.
"""
from __future__ import annotations

import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import _paths  # noqa: E402
import generate_nds_native_owners as native  # noqa: E402

REPO = _paths.REPO_ROOT
RELOC = REPO / "decomp/BattleShip-main/decomp/src/relocData"
FTDATA = REPO / "decomp/BattleShip-main/decomp/src/ft/ftdata.c"
O2R = REPO / "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main"

# Every FTANIM_FLAG_* value lives in the low half (ftdef.h: SUBMOTION_SCRIPT
# 0x10 down to ANIMLOCKS 0x1), so the anim-desc mask is the high half alone.
FLAG_BITS = 0x0000FFFF

CMD = re.compile(r"ftMotionCommandSetModelPartID\((\d+),\s*(-?\d+)\)")
# Motion arrays are declared BOTH ways in the reloc data -- 
# `ftMotionCommand dLinkMainMotion_Catch[]` but `u32 dLinkMainMotion_CatchPull[]`.
# Matching only the first silently attributes every u32-declared motion's
# commands to whichever ftMotionCommand-declared motion preceded it.
ARR = re.compile(r"^(?:ftMotionCommand|u32) (d\w+)\[\]")
# Samus's Catch keeps its eight model-part writes in a subroutine, not inline,
# so a checker that reads only the named motion body sees no events at all and
# computes a 16-root vector for a 21-root program.
SUB = re.compile(r"ftMotionCommandSubroutine\((?:\(u32\))?\s*(d\w+)\s*\)")
ROW = re.compile(r"&llFT(\w+?)Anim(\w+?)FileID\s*,([^,]*),([^}]*)\}")
HIDDEN = re.compile(r"/\* @ 0x([0-9A-Fa-f]+),[^*]*hiddenparts target")
HIDDEN_COUNT = re.compile(r"FTHiddenPart \w+_hiddenparts\[(\d+)\]")
CONTAINER = re.compile(
    r"/\* @ 0x([0-9A-Fa-f]+),[^*]*modelparts_container target")

# Owner name as the generator knows it -> the decomp Main/Model title.
TITLES = {
    "mario": "Mario", "fox": "Fox", "donkey": "Donkey", "samus": "Samus",
    "luigi": "Luigi", "link": "Link", "kirby": "Kirby", "purin": "Purin",
    "captain": "Captain", "ness": "Ness", "pikachu": "Pikachu",
    "yoshi": "Yoshi", "boss": "Boss", "mmario": "MMario", "nmario": "NMario",
    "nfox": "NFox", "ndonkey": "NDonkey", "nsamus": "NSamus",
    "nluigi": "NLuigi", "nlink": "NLink", "nkirby": "NKirby",
    "npurin": "NPurin", "ncaptain": "NCaptain", "nness": "NNess",
    "npikachu": "NPikachu", "nyoshi": "NYoshi",
}


def main_source(title: str) -> Path | None:
    """The numerically prefixed Main file, never a sibling like NYoshiMain."""
    for path in RELOC.glob(f"[0-9]*_{title}Main.c"):
        if re.fullmatch(rf"\d+_{title}Main\.c", path.name):
            return path
    return None


def hiddenpart_table(path: Path) -> tuple[int, int] | None:
    text = path.read_text(encoding="utf-8", errors="replace")
    offset = HIDDEN.search(text)
    count = HIDDEN_COUNT.search(text)
    if offset is None or count is None:
        return None
    return int(offset.group(1), 16), int(count.group(1))


def container_offset(owner: str, path: Path) -> int | None:
    """The modelparts_container offset, from the Main file's own comment.

    Where the generator already pins one, the two must agree -- a free
    falsifier on the comment convention this check leans on for the fighters
    the generator has never needed to open.
    """
    match = CONTAINER.search(path.read_text(encoding="utf-8", errors="replace"))
    if match is None:
        return None
    parsed = int(match.group(1), 16)
    pinned = native.OWNER_ROOT_PROGRAM_SOURCES.get(owner)
    if pinned is not None and pinned[2] != parsed:
        raise SystemExit(
            f"check-hidden-part-root-coverage: {owner} container offset "
            f"0x{parsed:x} from {path.name} disagrees with the generator's "
            f"pinned 0x{pinned[2]:x}")
    return parsed


# The ROM builds -DREGION_US, and the reloc data carries both arms: 215
# `#if defined(REGION_JP)` and 39 `#if defined(REGION_US)` blocks across the
# motion files, holding ten real SetModelPartID commands between Link and Kirby.
# A line-based parser absorbs the JP arm into the US motion and computes a
# vector no US build ever reaches -- that is exactly how a phantom
# dLinkMainMotion_Catch finding appeared, from the stray SetModelPartID(21, 0)
# below its End() in the JP arm. There are no #elif forms and every directive is
# at column 0, so a simple stack is enough.
def region_us_lines(text: str):
    """Yield (line_number, line) for the REGION_US arm only."""
    stack: list[bool] = []
    for number, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if stripped.startswith("#if"):
            if "REGION_JP" in stripped:
                stack.append(False)
            else:
                # REGION_US, and anything else we do not model, stays active.
                stack.append(True)
            continue
        if stripped.startswith("#else"):
            if stack:
                stack[-1] = not stack[-1]
            continue
        if stripped.startswith("#endif"):
            if stack:
                stack.pop()
            continue
        if all(stack):
            yield number, line


def motion_events(title: str) -> dict[str, tuple[tuple[int, int], ...]]:
    """`SetModelPartID` commands per motion symbol, subroutines expanded."""
    # Source order is kept across the subroutine boundary, because a later
    # write to the same joint must win over an earlier one.
    body: dict[str, list[tuple[str, object]]] = {}
    for path in RELOC.glob(f"[0-9]*_{title}MainMotion.c"):
        if not re.fullmatch(rf"\d+_{title}MainMotion\.c", path.name):
            continue
        current = None
        for _number, line in region_us_lines(
                path.read_text(encoding="utf-8", errors="replace")):
            head = ARR.match(line)
            if head:
                current = head.group(1)
                body.setdefault(current, [])
                continue
            if current is None:
                continue
            for joint, part in CMD.findall(line):
                # The event's model part is a signed 19-bit field
                # (check_model_part_mutation_coverage.decode_raw_modelpart):
                # Kirby's motion data spells -1 as 524287.
                part = int(part)
                if part >= 0 and part & 0x40000:
                    part -= 0x80000
                body[current].append(("set", (int(joint), part)))
            for callee in SUB.findall(line):
                body[current].append(("call", callee))

    def expand(symbol: str, seen: frozenset[str]) -> list[tuple[int, int]]:
        if symbol in seen or symbol not in body:
            return []
        rows: list[tuple[int, int]] = []
        for kind, value in body[symbol]:
            if kind == "set":
                rows.append(value)
            else:
                rows.extend(expand(str(value), seen | {symbol}))
        return rows

    return {symbol: tuple(expand(symbol, frozenset())) for symbol in body}


def anim_rows(title: str) -> dict[int, dict[str, set[str]]]:
    """mask -> {motion symbol: {anim names}} for every row naming this fighter."""
    found: dict[int, dict[str, set[str]]] = {}
    for _number, line in region_us_lines(
            FTDATA.read_text(encoding="utf-8", errors="replace")):
        match = ROW.search(line)
        if not match:
            continue
        if match.group(1) != title:
            continue
        motion = match.group(3).strip()
        mask = 0
        for literal in re.findall(r"0x([0-9A-Fa-f]+)", match.group(4)):
            mask |= int(literal, 16)
        mask &= ~FLAG_BITS
        if not mask:
            continue
        if not motion.startswith("d"):
            motion = ""
        found.setdefault(mask, {}).setdefault(motion, set()).add(match.group(2))
    return found


def hiddenpart_ids(mask: int, count: int) -> tuple[int, ...]:
    return tuple(sorted(
        index for index in (31 - bit for bit in range(32) if mask & (1 << bit))
        if 0 <= index < count))


def load_main_payload(title: str) -> bytes:
    payload = (O2R / f"{title}Main").read_bytes()
    header = native.O2R_RESOURCE_HEADER_SIZE
    extern_count = struct.unpack_from("<I", payload, header + 8)[0]
    return payload[header + 12 + extern_count * 2 + 4:]


def emitted_programs(owner: str) -> dict[tuple[str, str], list[tuple[int, ...]]]:
    """Each detail's emitted root-program vectors, from contexts built the way
    the generator builds them: both details, the root-light preamble tables
    merged high-first and low re-indexed through the union (generate()).
    A low context built alone fails the generator's own light-index pins
    (Link's appendix rows), which is why this does not call it per detail."""
    high = native.build_p2_owner_runtime_context(REPO, owner, "high")
    low = native.build_p2_owner_runtime_context(REPO, owner, "low")
    merged = list(high["light_preambles"])
    for preamble in low["light_preambles"]:
        if preamble not in merged:
            merged.append(preamble)
    remap = [merged.index(preamble) for preamble in low["light_preambles"]]
    low["light_preamble_indices"] = [
        remap[index] for index in low["light_preamble_indices"]]
    high["light_preambles"] = merged
    low["light_preambles"] = merged
    return {
        (owner, detail): [
            tuple(program["root_offsets"])
            for program in native.build_owner_root_programs(REPO, context)
        ]
        for detail, context in (("high", high), ("low", low))
    }


def main() -> int:
    failures: list[str] = []
    swept = 0
    drawing_cases = 0
    program_cache: dict[tuple[str, str], list[tuple[int, ...]]] = {}

    for owner in sorted(TITLES):
        title = TITLES[owner]
        source = main_source(title)
        if source is None or not (O2R / f"{title}Main").exists():
            continue
        table = hiddenpart_table(source)
        if table is None:
            continue
        rows = anim_rows(title)
        if not rows:
            continue
        hidden_offset, hidden_count = table
        container = container_offset(owner, source)
        if container is None:
            failures.append(
                f"{owner}: {source.name} has no modelparts_container comment, "
                f"so motion commands cannot be resolved")
            continue
        try:
            main_payload = load_main_payload(title)
            model_payload = native.load_o2r_payload(REPO, owner)
        except Exception as exc:                                 # noqa: BLE001
            failures.append(f"{owner}: cannot load payloads: {exc}")
            continue
        swept += 1
        events_by_motion = motion_events(title)

        for mask, motions in sorted(rows.items()):
            ids = hiddenpart_ids(mask, hidden_count)
            if not ids:
                continue
            for detail in ("high", "low"):
                descriptors = native._owner_joint_descriptors(
                    model_payload, owner, detail)[:-1]
                selected = set(native._owner_selected_descriptor_indices(
                    owner, len(descriptors)))
                installed: list[tuple[int, int, int]] = []
                hidden_live: dict[int, tuple[int, int]] = {}
                live = set(selected)
                for hidden_id in ids:
                    row_offset = hidden_offset + hidden_id * 16
                    if row_offset + 16 > len(main_payload):
                        failures.append(
                            f"{owner}: hidden-part table is truncated at "
                            f"index {hidden_id}")
                        continue
                    root_joint = struct.unpack_from(
                        ">i", main_payload, row_offset)[0]
                    index = root_joint - 4
                    if index < 0 or index >= len(descriptors):
                        continue
                    if index in selected:
                        continue
                    live.add(index)
                    hidden_live[index] = (hidden_id, root_joint)
                    if descriptors[index][1] is not None:
                        installed.append(
                            (hidden_id, root_joint, descriptors[index][1]))
                if not hidden_live:
                    continue
                key = (owner, detail)
                counted = False

                for motion, names in sorted(motions.items()):
                    # The generic resolver cannot be reused here: it treats a
                    # command on a setup_parts-omitted joint as a source no-op,
                    # which is right in general and wrong for exactly this
                    # case, because ftMainSetStatus installed that joint from
                    # this same mask before the motion ran. Samus Catch is the
                    # proof -- its six modelpart-0 writes all land on hidden
                    # joints, and dropping them reads 16 roots instead of 21.
                    #
                    # A hidden part with no display list of its own draws only
                    # if this motion gives it one (Ness's yo-yo: hidden joint
                    # 30, then SetModelPartID(30, 0)); a motion that never
                    # names one adds no root through this mask.
                    if not installed and not any(
                            (joint_id - 4) in hidden_live
                            for joint_id, _part in
                            events_by_motion.get(motion, ())):
                        continue
                    overrides: dict[int, int | None] = {}
                    for joint_id, modelpart_id in events_by_motion.get(motion, ()):
                        index = joint_id - 4
                        if index < 0 or index >= len(descriptors):
                            continue
                        if index not in live:
                            continue
                        overrides[index] = \
                            native._owner_modelpart_display_offset(
                                main_payload, container, joint_id,
                                modelpart_id, detail)
                    live_descriptors = native._owner_joint_descriptors(
                        model_payload, owner, detail, overrides or None)[:-1]
                    vector = tuple(
                        live_descriptors[index][1]
                        for index in sorted(live)
                        if live_descriptors[index][1] is not None)
                    drawing = [
                        (hid, joint, live_descriptors[index][1])
                        for index, (hid, joint) in sorted(hidden_live.items())
                        if live_descriptors[index][1] is not None]
                    if not drawing:
                        continue
                    if not counted:
                        drawing_cases += 1
                        counted = True
                    if key not in program_cache:
                        program_cache.update(emitted_programs(owner))
                    if vector in program_cache[key]:
                        continue
                    shown = ", ".join(
                        f"hidden part {hid} joint {joint} 0x{offset:x}"
                        for hid, joint, offset in drawing)
                    failures.append(
                        f"{owner} {detail} mask 0x{mask:08x} "
                        f"({', '.join(sorted(names)[:4])}"
                        f"{' and more' if len(names) > 4 else ''}): "
                        f"{shown} makes the live vector {len(vector)} roots, "
                        f"and no emitted root program carries it. The owner "
                        f"will decline and the fighter will not draw.")

    print(f"  hidden-part root coverage: {swept} fighters swept, "
          f"{drawing_cases} owner/detail cases install a drawing hidden part")
    if failures:
        print("HIDDEN_PART_ROOT_COVERAGE_FAIL")
        for line in failures:
            print(f"  {line}")
        return 1
    print("HIDDEN_PART_ROOT_COVERAGE_OK every drawing hidden part a motion "
          "installs is carried by a native root program")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
