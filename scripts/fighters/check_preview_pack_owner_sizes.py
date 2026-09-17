#!/usr/bin/env python3
"""A CSS preview pack's declared source size must equal its native owner's.

WHY THIS EXISTS. `ndsRendererValidateNativeFighterOwner`
(src/nds/nds_renderer_native_fighter_production.c) compares the loaded asset's
size against the owner's `asset_data_size` and rejects with code 3 **before it
looks at a single root**. For a compact CSS preview the size it is handed is
the pack header's section-1 `source_bytes`
(`ndsRelocNativeSourceSize`, src/port/reloc_preview_pack.c). So two producers
that never mention each other have to agree on one number:

  * `generate_nds_native_owners.py` emits `NDS_NATIVE_<X>_MODEL_DATA_SIZE`
  * `generate_preview_core_packs.py` writes `source_bytes` into the pack

They disagree for exactly one fighter. Yoshi is the only CSS-preview kind in
`OWNER_DL_PAIR_MODE`, so he is the only one whose `load_o2r_payload` result is
longer than the asset the runtime loads -- `_extend_payload_with_pairs` appends
1,232 bytes of welded DL. The owner publishes the raw 44,256; the pack publishes
the extended 45,488; the validator rejects; and at `NDS_RENDERER_PROFILE_LEVEL 0`
a declined owner draws nothing, so Yoshi's 3D preview is absent from the
character select. In-match Yoshi is fine because the battle pack declares the
raw length. Owner-reported 2026-09-17.

IT IS NOT FIXED BY LOWERING `source_bytes`, AND THAT WAS TRIED. The same field
has a second consumer: `reloc_preview_pack.c:401` bounds every span's
`source_offset` by it, and a failure calls `ndsPreviewPackLoadHalt`, a `for(;;)`
spin rather than an abort. Yoshi's third model span ends at 45,488, so a 44,256
bound hangs the character select before it can even enter. One field cannot be
both the span extent and the raw asset size; the pack has to carry the raw size
as its own field. Until it does, KNOWN_MISMATCHES records the live defect by
name so this check still fails on anything new.

Nothing in either producer's text refers to the other, so no grep relates them.
This check does.

Run:
    python scripts/fighters/check_preview_pack_owner_sizes.py
    python scripts/fighters/check_preview_pack_owner_sizes.py --packs DIR
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
OWNER_INC = REPO / "src/nds/nds_native_fighter_owner.generated.inc"
PRODUCTION = REPO / "src/nds/nds_renderer_native_fighter_production.c"
PACK_HEADER = REPO / "include/nds/nds_preview_pack.h"

# FTKind enum order (include/ft/fighter.h); the pack filename is that index.
KINDS = ["mario", "fox", "donkey", "samus", "luigi", "link",
         "yoshi", "captain", "kirby", "pikachu", "purin", "ness"]

# Known live defects, recorded rather than hidden. A kind listed here still
# prints, still explains itself and still shows its delta -- it just does not
# fail the build, because the fix is a pack-format change and not a number
# edit. Anything NOT listed here fails, which is the whole point: a second
# fighter drifting must not be absorbed by the first one's exception.
KNOWN_MISMATCHES = {
    "yoshi": (45488, 44256,
              "pair weld; source_bytes must stay the span extent or "
              "reloc_preview_pack.c:401 spins. Needs its own header field."),
}


def struct_fields(name: str, src: str):
    """Field list of a header struct, read rather than assumed."""
    m = re.search(r"typedef struct %s\s*\{(.*?)\}\s*%s;" % (name, name),
                  src, re.S)
    if not m:
        sys.exit("check_preview_pack_owner_sizes: no struct %s in %s"
                 % (name, PACK_HEADER))
    out = []
    for line in m.group(1).splitlines():
        line = re.sub(r"/\*.*?\*/", "", line).strip()
        fm = re.match(r"(u32|u16|u8)\s+(\w+)(?:\[(\d+)\])?\s*;", line)
        if fm:
            out.append((fm.group(1), fm.group(2), int(fm.group(3) or 1)))
    return out


SIZES = {"u32": 4, "u16": 2, "u8": 1}


def sizeof(fields) -> int:
    return sum(SIZES[t] * n for t, _, n in fields)


def field_offset(fields, name: str):
    off = 0
    for t, nm, n in fields:
        if nm == name:
            if t != "u32" or n != 1:
                sys.exit("check_preview_pack_owner_sizes: %s is not a u32" % nm)
            return off
        off += SIZES[t] * n
    sys.exit("check_preview_pack_owner_sizes: no field %r" % name)


def owner_expected() -> dict[str, int]:
    """What the renderer will demand, from the two emitted C sources."""
    out: dict[str, int] = {}
    inc = OWNER_INC.read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(r"#define NDS_NATIVE_(\w+?)_MODEL_DATA_SIZE\s+"
                         r"(0[xX][0-9a-fA-F]+|\d+)u?", inc):
        out[m.group(1).lower()] = int(m.group(2), 0)
    # Mario and Fox predate the generated defines and carry literals in the
    # validator itself; read them there rather than restating them here.
    prod = PRODUCTION.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"if \(slot == 0u\)\s*\{\s*expected_asset_data_size\s*=\s*"
                  r"(0[xX][0-9a-fA-F]+|\d+)u?;\s*\}\s*else\s*\{\s*"
                  r"expected_asset_data_size\s*=\s*(0[xX][0-9a-fA-F]+|\d+)u?;",
                  prod, re.S)
    if not m:
        sys.exit("check_preview_pack_owner_sizes: Mario/Fox literals moved in %s"
                 % PRODUCTION.name)
    out["mario"], out["fox"] = int(m.group(1), 0), int(m.group(2), 0)
    return out


def find_pack_dirs(explicit: str | None) -> list[Path]:
    if explicit:
        return [Path(explicit)]
    found = sorted({p.parent for p in
                    REPO.glob("builds/*/nitrofs/fighters/preview/00.fpc")} |
                   {p.parent for p in
                    REPO.glob("builds/*/preview-core/00.fpc")} |
                   {p.parent for p in
                    REPO.glob("*/nitrofs/fighters/preview/00.fpc")})
    return found


def pack_source_bytes(path: Path, hdr_fields, sec_fields) -> int:
    blob = path.read_bytes()
    hdr_len, sec_len = sizeof(hdr_fields), sizeof(sec_fields)
    need = hdr_len + 2 * sec_len
    if len(blob) < need:
        sys.exit("%s: shorter than a header plus two sections" % path)
    # Section 1 is the model section by the pack's own validator
    # (ndsPreviewValidateSections requires sections[1].asset_id ==
    # header.model_asset_id).
    off = hdr_len + sec_len + field_offset(sec_fields, "source_bytes")
    return struct.unpack_from("<I", blob, off)[0]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--packs", default=None,
                    help="one directory of NN.fpc packs to check")
    args = ap.parse_args()

    src = PACK_HEADER.read_text(encoding="utf-8", errors="replace")
    hdr_fields = struct_fields("NDSPreviewPackHeader", src)
    sec_fields = struct_fields("NDSPreviewPackSection", src)

    expected = owner_expected()
    missing = [k for k in KINDS if k not in expected]
    if missing:
        print("FAIL: no owner size for %s" % ", ".join(missing))
        return 1

    dirs = find_pack_dirs(args.packs)
    if not dirs:
        # An empty pass is the failure mode this class of checker is prone to.
        print("FAIL: no built preview packs found. Build any configuration "
              "with NDS_P2_MENU_SHELL=1 or NDS_P2_1P_GAME=1, or pass --packs.")
        return 1

    failures = 0
    checked = 0
    knowns = 0
    for d in dirs:
        for index, kind in enumerate(KINDS):
            p = d / ("%02d.fpc" % index)
            if not p.is_file():
                continue
            got = pack_source_bytes(p, hdr_fields, sec_fields)
            want = expected[kind]
            checked += 1
            known = KNOWN_MISMATCHES.get(kind)
            if (got != want) and known and (got, want) == known[:2]:
                knowns += 1
                print("KNOWN %s: pack %d vs owner %d (%+d) -- %s"
                      % (kind, got, want, got - want, known[2]))
                continue
            if got != want:
                failures += 1
                print("FAIL %s: %s declares source_bytes=%d (0x%x) but the "
                      "native owner expects asset_data_size=%d (0x%x), "
                      "delta %+d -- ndsRendererValidateNativeFighterOwner "
                      "rejects with code 3 and the preview draws nothing"
                      % (kind, p.relative_to(REPO) if REPO in p.parents else p,
                         got, got, want, want, got - want))

    if failures:
        print("\n%d of %d preview packs disagree with their native owner."
              % (failures, checked))
        print("If the fighter is in OWNER_DL_PAIR_MODE, the likely cause is a "
              "producer using the pair-EXTENDED payload length where the RAW "
              "length belongs: see owner_asset_data_size() in "
              "scripts/fighters/generate_nds_native_owners.py.")
        return 1

    print("verified preview pack source_bytes against native owner "
          "asset_data_size: %d pack(s) across %d director%s, no NEW drift "
          "(%d known, recorded above)"
          % (checked, len(dirs), "y" if len(dirs) == 1 else "ies", knowns))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
