#!/usr/bin/env python3
"""P4 native owners: feed a donor fighter model to the existing owner generator.

scripts/fighters/generate_nds_native_owners.py compiles a fighter model into
the DS-native owner IR from per-owner tables it keeps for the original cast
(JointTree offsets, setup mask, plan counts, cross-binding slots, census pins).
For a donor the source tables are not BattleShip C files, so this module reads
the same facts from the donor's own main/model files and registers the owner
at run time. Census pins live in scripts/p4/owners/<name>.json (counts only, no
asset bytes); `--learn` re-derives them from the generator's own validators
and must be re-run deliberately when the donor model changes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts" / "fighters"))
import _paths  # noqa: E402,F401
import generate_nds_native_owners as O  # noqa: E402
import ft_layout  # noqa: E402

PINS_DIR = HERE / "owners"
DOBJ_DESC_SIZE = 0x2C
CROSS_SLOT_FIRST = 16

# name -> {"main": file id, "model": file id}. Filled from each content's
# export (scripts/p4/p4_contents.py export_facts) before any owner registers.
DONORS: dict[str, dict] = {}


def load_donor(name: str, export_root: Path) -> dict:
    import p4_contents  # noqa: E402

    facts = p4_contents.export_facts(export_root, name)
    DONORS[name] = {"main": facts["main"], "model": facts["model"],
                    "attributes": facts["attributes"],
                    "script_parts": script_model_parts(export_root / name / "events.json")}
    return DONORS[name]


def script_model_parts(events_path: Path) -> set[tuple[int, int]]:
    """(joint, model part) pairs the donor's own motion scripts set."""
    parts: set[tuple[int, int]] = set()
    if not events_path.exists():
        return parts
    events = json.loads(events_path.read_text(encoding="utf-8"))
    for block in events["blocks"].values():
        for command in block["commands"]:
            if command["op"] != "SetModelPartID":
                continue
            word = command["words"][0]
            part = word & 0x7FFFF
            if part & 0x40000:
                part -= 0x80000
            parts.add(((word >> 19) & 0x7F, part))
    return parts


class O2RFile:
    def __init__(self, path: Path):
        blob = path.read_bytes()
        self.file_id, self.intern, self.extern, n = struct.unpack_from("<IHHI", blob, 0x40)
        self.externs = list(struct.unpack_from(f"<{n}H", blob, 0x4C))
        off = 0x4C + 2 * n
        size = struct.unpack_from("<I", blob, off)[0]
        self.data = blob[off + 4:off + 4 + size]
        self.slots: dict[int, tuple] = {}
        w = self.intern
        while w != 0xFFFF:
            val = struct.unpack_from(">I", self.data, w * 4)[0]
            self.slots[w * 4] = ("intern", self.file_id, (val & 0xFFFF) * 4)
            w = val >> 16
        w, k = self.extern, 0
        while w != 0xFFFF:
            val = struct.unpack_from(">I", self.data, w * 4)[0]
            self.slots[w * 4] = ("extern", self.externs[k], (val & 0xFFFF) * 4)
            k += 1
            w = val >> 16

    def ptr(self, off: int) -> tuple | None:
        return self.slots.get(off)

    def u32(self, off: int) -> int:
        return struct.unpack_from(">I", self.data, off)[0]


def donor_tables(o2r_dir: Path, main_id: int, model_id: int, attr_offset: int) -> dict:
    lay = ft_layout.layout()
    main = O2RFile(o2r_dir / f"{main_id:04x}")
    model = O2RFile(o2r_dir / f"{model_id:04x}")
    container = main.ptr(attr_offset + lay["FTAttributes.commonparts_container"])
    setup = main.ptr(attr_offset + lay["FTAttributes.setup_parts"])
    if container is None or setup is None or container[0] != "intern" or setup[0] != "intern":
        raise SystemExit("donor main: commonparts/setup_parts are not internal pointers")
    trees = []
    # FTCommonPart.flags bit 0: each DObjDesc.dl is a `Gfx *dls[2]` pair
    # (ftdisplaymain.c), Yoshi's form, which Bowser keeps.
    flags = {main.data[container[2] + detail * lay["sizeof(FTCommonPart)"] +
                       lay["FTCommonPart.flags"]] & 1 for detail in range(2)}
    if len(flags) != 1:
        raise SystemExit("donor main: the two details disagree on the DL pair form")
    for detail in range(2):
        ref = main.ptr(container[2] + detail * lay["sizeof(FTCommonPart)"] + lay["FTCommonPart.dobjdesc"])
        if ref is None or ref[0] != "extern" or ref[1] != model_id:
            raise SystemExit(f"donor main: detail {detail} JointTree is not in the model file")
        count = 0
        while True:
            if model.u32(ref[2] + count * DOBJ_DESC_SIZE) == 18:
                count += 1
                break
            count += 1
            if count > 64:
                raise SystemExit("donor JointTree has no depth-18 sentinel")
        trees.append((ref[2], count))
    mask = (main.u32(setup[2]), main.u32(setup[2] + 4))
    return {"trees": trees, "setup": mask, "dl_pairs": flags == {1}}


# name -> detail -> [(binding, offset, joint, part, vertices, scripted)] for
# every variant donor_variants found, dropped ones included (learn's choice).
VARIANT_INFO: dict[str, dict[str, list[tuple]]] = {}


def _display_vertices(payload: bytes, offset: int) -> int:
    """Vertices a display list loads (G_VTX counts up to its G_ENDDL)."""
    total = 0
    for at in range(offset, len(payload) - 7, 8):
        op = payload[at]
        if op == 0x01:
            total += (struct.unpack_from(">I", payload, at)[0] >> 12) & 0xFF
        elif op == 0xDF:
            break
    return total


def donor_variants(name: str, o2r_dir: Path, main_id: int, model_id: int,
                   attr_offset: int, dropped: dict | None = None) -> dict:
    """Every alternate model part a donor's joints carry, per detail, as
    (logical binding, display offset) rows for P2_MODEL_PART_ROOT_VARIANTS.

    ftParamSetModelPartID writes modelparts[joint - 4][part][detail].dl into
    the live DObj, and a P4 fighter's parts are set by its own scripts and by
    its parent's code (Bowser's jaw by Yoshi's specials), so the owner carries
    every part, not only the ones a script names. A joint's FTModelPartDesc
    runs from its row to the next descriptor (or the container itself)."""
    lay = ft_layout.layout()
    main = O2RFile(o2r_dir / f"{main_id:04x}")
    container = main.ptr(attr_offset + lay["FTAttributes.modelparts_container"])
    if container is None:
        return {}
    payload = O.load_o2r_payload(REPO, name)
    scripted = DONORS[name].get("script_parts", set())
    dropped = dropped or {}
    result = {}
    VARIANT_INFO[name] = {}
    for detail in ("high", "low"):
        descriptors = O._owner_joint_descriptors(payload, name, detail)[:-1]
        selected = O._owner_selected_descriptor_indices(name, len(descriptors))
        roots = [i for i in selected if descriptors[i][1] is not None]
        rows_by_joint = {}
        for index in range(min(len(descriptors), 37 - 4)):
            ref = main.ptr(container[2] + 4 * index)
            if ref is not None and ref[0] == "intern":
                rows_by_joint[index] = ref[2]
        starts = sorted(set(rows_by_joint.values()) | {container[2]})
        info = []
        for binding, index in enumerate(roots):
            base = rows_by_joint.get(index)
            if base is None:
                continue
            bound = min([s for s in starts if s > base], default=len(main.data))
            part = 1
            while base + (part + 1) * 2 * 20 <= bound:
                ref = main.ptr(base + (part * 2 + (detail == "low")) * 20)
                if (ref is not None and ref[0] == "extern" and ref[1] == model_id and
                        ref[2] != descriptors[index][1] and
                        all(row[1] != ref[2] or row[0] != binding for row in info)):
                    info.append((binding, ref[2], index + 4, part,
                                 _display_vertices(payload, ref[2]),
                                 (index + 4, part) in scripted))
                part += 1
        VARIANT_INFO[name][detail] = info
        rows = tuple((row[0], row[1]) for row in info
                     if row[1] not in set(dropped.get(detail, ())))
        if rows:
            result[detail] = rows
    return result


def _o2r_rel(repo: Path, path: Path) -> Path:
    try:
        return path.resolve().relative_to(repo.resolve())
    except ValueError:
        raise SystemExit(f"{path}: O2R inputs must live under the repository (builds/)")


def register(name: str, o2r_dir: Path, attr_offset: int, pins: dict | None) -> dict:
    """Register `name` with the generator; returns the derived source tables."""
    spec = DONORS[name]
    model_path = o2r_dir / f"{spec['model']:04x}"
    sha = hashlib.sha256(model_path.read_bytes()).hexdigest()
    tables = donor_tables(o2r_dir, spec["main"], spec["model"], attr_offset)
    O.P4_DONOR_OWNERS.add(name)
    if tables["dl_pairs"]:
        O.OWNER_DL_PAIR_MODE = O.OWNER_DL_PAIR_MODE | {name}
    O.P2_O2R_ASSETS[name] = (_o2r_rel(REPO, model_path), spec["model"], sha)
    O.OWNER_JOINT_TREES[name] = tuple(tables["trees"][0])
    O.OWNER_JOINT_TREES_LOW[name] = tuple(tables["trees"][1])
    O.OWNER_SETUP_PARTS[name] = tuple(tables["setup"])
    pins = pins or {}
    O.OWNER_PLAN_COUNTS[name] = tuple(pins.get("plan", (0, 0)))
    O.OWNER_CROSS_BINDING_SLOTS[name] = tuple(tuple(x) for x in pins.get("cross_high", ()))
    if "cross_low" in pins:
        O.OWNER_CROSS_BINDING_SLOTS_LOW[name] = tuple(tuple(x) for x in pins["cross_low"])
    O.OWNER_GX_PLAN_COUNTS[name] = tuple(pins.get("gx", (1, 0, 0, 0, 0)))
    for detail in ("high", "low"):
        O.DETAIL_GX_PLAN_COUNTS[detail][name] = tuple(pins.get(f"gx_{detail}", (1, 0, 0, 0, 0)))
    O.P2_OWNER_MODEL_CENSUS[name] = {d: tuple(pins.get(f"census_{d}", ())) for d in ("high", "low")}
    variants = donor_variants(name, o2r_dir, spec["main"], spec["model"], attr_offset,
                              pins.get("variants_dropped"))
    if variants:
        O.P2_MODEL_PART_ROOT_VARIANTS[name] = variants
    else:
        O.P2_MODEL_PART_ROOT_VARIANTS.pop(name, None)
    return tables


def _cross_bindings(name: str, detail: str) -> list[int]:
    data = O.build_p2_owner_source_export(REPO, name, detail)
    vertex = O.unpack_many("<BBBBIhh", data["vertex"])
    vb = dict(O.unpack_many("<HH", data.get("vertex_bindings", b"")))
    tris = [i[0] for i in O.unpack_many("<H", data["triangles"])]
    runs = O.unpack_many("<HBBI", data["runs"])
    epochs = O.unpack_many("<HHHHBBBBBBBB", data["epochs"])
    roots = O.unpack_many("<IHHHBBBB2x", data[f"{name}_roots"])
    geo = O.build_dense_geometry(vertex, tris, runs, epochs, ((name, roots),), REPO, action_bindings=vb)
    cross: set[int] = set()
    for index, run in enumerate(runs):
        if run[2] == 1:
            cross |= set(geo[8][index])
    return sorted(cross)


def learn(name: str, o2r_dir: Path, attr_offset: int) -> dict:
    """Derive every pin from the donor model and the generator's validators."""
    tables = register(name, o2r_dir, attr_offset, None)
    payload = O.load_o2r_payload(REPO, name)
    selected = O._owner_selected_descriptor_indices(name, tables["trees"][0][1] - 1)
    descriptors = O._owner_joint_descriptors(payload, name, "high")[:-1]
    roots = [descriptors[i][1] for i in selected if descriptors[i][1] is not None]
    pins: dict = {"plan": [1 + len(selected), len(roots)]}
    register(name, o2r_dir, attr_offset, pins)
    for detail in ("high", "low"):
        cross = _cross_bindings(name, detail)
        pins[f"cross_{detail}"] = [[b, CROSS_SLOT_FIRST + i] for i, b in enumerate(cross)]
    if pins["cross_low"] == pins["cross_high"]:
        del pins["cross_low"]
    pins["gx"] = [1, 0, 0, len(pins["cross_high"]), 0]
    for detail in ("high", "low"):
        cross = pins.get(f"cross_{detail}", pins["cross_high"])
        pins[f"gx_{detail}"] = [1, 0, 0, len(cross), 0]
    pins["census_high"] = pins["census_low"] = [0] * 13
    patterns = (
        (r"seed/push/pop=1/(\d+)/(\d+)", lambda m: _set_gx(pins, push=int(m[1]), pop=int(m[2]))),
        (r"(high|low) GX store count (\d+) !=", lambda m: _set_detail(pins, m[1], 3, int(m[2]))),
        (r"(high|low) GX restore count (\d+) !=", lambda m: _set_detail(pins, m[1], 4, int(m[2]))),
        (r"packet store/restore (\d+)/(\d+) !=", None),
        (r"(high|low) (?:native-model|runtime context) census \(([^)]*)\) !=",
         lambda m: pins.__setitem__(f"census_{m[1]}", [int(x) for x in m[2].split(",")])),
        # A model-part variant's cross run names a binding no canonical run
        # crosses: it takes the next physical slot.
        (r"(high|low) (?:current )?binding (\d+) has no (?:restorable )?GX palette slot",
         lambda m: _add_cross(pins, m[1], int(m[2]))),
    )
    building = ["high"]
    for _attempt in range(40):
        register(name, o2r_dir, attr_offset, pins)
        try:
            for detail in ("high", "low"):
                building[0] = detail
                O.build_p2_owner_runtime_context(REPO, name, detail)
            O.build_p2_owner_model_inventory(REPO, name)
            if pins.get("cross_low") == pins["cross_high"]:
                del pins["cross_low"]
            return pins
        except ValueError as error:
            text = str(error)
            if re.search(r"dense IDs exceed the 11-bit direct ABI", text):
                _drop_variant(name, pins, building[0])
                continue
            for pattern, apply in patterns:
                m = re.search(pattern, text)
                if m:
                    if apply is None:
                        raise SystemExit(f"learn: unhandled pin: {text}")
                    apply(m)
                    break
            else:
                m = re.search(r"restore(?:s| count)? (\d+)", text)
                raise SystemExit(f"learn: {text}")
    raise SystemExit("learn: pins did not converge")


def _drop_variant(name: str, pins: dict, detail: str) -> None:
    """The owner's dense vertices exceed the packed corner's 11-bit ID: leave
    out the largest model-part variant no donor script sets (then the largest
    left). A dropped part declines when the parent's code sets it; the pins
    name every one."""
    dropped = pins.setdefault("variants_dropped", {}).setdefault(detail, [])
    kept = [row for row in VARIANT_INFO[name][detail] if row[1] not in dropped]
    if not kept:
        raise SystemExit(f"learn: {name} {detail} exceeds the dense ID ABI without variants")
    victim = max(kept, key=lambda row: (not row[5], row[4]))
    dropped.append(victim[1])


def _set_gx(pins: dict, push: int, pop: int) -> None:
    pins["gx"][1:3] = [push, pop]
    for detail in ("high", "low"):
        pins[f"gx_{detail}"][1:3] = [push, pop]


def _set_detail(pins: dict, detail: str, index: int, value: int) -> None:
    pins[f"gx_{detail}"][index] = value


# The GX matrix stack's last physical slot: 0..15 are the camera seed and the
# hierarchy stack, so cross bindings own 16..30.
CROSS_SLOT_LAST = 30


def _add_cross(pins: dict, detail: str, binding: int) -> None:
    if "cross_low" not in pins:
        pins["cross_low"] = [list(row) for row in pins["cross_high"]]
    rows = pins[f"cross_{detail}"]
    if any(b == binding for b, _slot in rows):
        raise SystemExit(f"learn: {detail} binding {binding} already has a GX slot")
    if CROSS_SLOT_FIRST + len(rows) > CROSS_SLOT_LAST:
        raise SystemExit(f"learn: {detail} cross bindings exceed the GX palette")
    rows.append([binding, CROSS_SLOT_FIRST + len(rows)])


def _merge_light_preambles(name: str, high: dict, low: dict) -> None:
    """Both details' owners share the one root-light preamble table the high
    pass emits, so it is the union of both, with the low roots re-indexed into
    it (generate_nds_native_owners.py does the same for the original cast:
    Captain's low model has a preamble his high model lacks, as Wario's has)."""
    merged = list(high["light_preambles"])
    for preamble in low["light_preambles"]:
        if preamble not in merged:
            merged.append(preamble)
    if len(merged) > 0xFF:
        raise SystemExit(f"{name}: merged root-light preamble index exceeds u8")
    remap = [merged.index(preamble) for preamble in low["light_preambles"]]
    low["light_preamble_indices"] = [remap[i] for i in low["light_preamble_indices"]]
    high["light_preambles"] = merged
    low["light_preambles"] = merged
    low["high_light_preambles"] = merged


def emit(out_dir: Path, owners: dict[str, tuple[Path, int]]) -> dict[str, list[Path]]:
    """Write the build-local owner products for every enabled donor owner.

    `owners` is name -> (O2R directory, attributes offset) in registry order,
    which is the image slot order. For each owner: its runtime program (.inc,
    the same rows the original cast's owners get in
    src/nds/nds_native_fighter_owner.generated.inc), its two image TUs, and
    one shared header carrying every owner's image ABI rows. Image slots and
    renderer owner slots follow the original cast's (P4 owners append after
    NDS_NATIVE_IMAGE_OWNER_SLOTS and the renderer's base slot)."""
    import generate_nds_native_owner_images as images  # noqa: E402

    out_dir.mkdir(parents=True, exist_ok=True)
    P4_IMAGE_OWNERS = tuple(owners)
    products: dict[str, list[Path]] = {}
    header = [
        "/* Generated by scripts/p4/p4_native_owner.py. Build output: derived from the",
        " * user's ROM through the staged donor build. Do not edit. */",
        "#ifndef NDS_P4_NATIVE_IMAGE_GENERATED_H",
        "#define NDS_P4_NATIVE_IMAGE_GENERATED_H",
        "",
        "#include <nds/generated/nds_native_fighter_image.generated.h>",
        "",
    ]
    inc = ["/* Generated by scripts/p4/p4_native_owner.py. Do not edit. */", ""]
    rows = []
    for index, name in enumerate(P4_IMAGE_OWNERS):
        if name not in owners:
            continue
        o2r_dir, attr = owners[name]
        register(name, o2r_dir, attr, load_pins(name))
        guard = f"NDS_P4_{name.upper()}"
        contexts = {d: O.build_p2_owner_runtime_context(REPO, name, d) for d in ("high", "low")}
        _merge_light_preambles(name, contexts["high"], contexts["low"])
        header += [f"#if {guard}",
                   f"#define NDS_NATIVE_IMAGE_SLOT_{name.upper()} (NDS_NATIVE_IMAGE_OWNER_SLOTS + {index}u)",
                   f"#define NDS_NATIVE_OWNER_IMAGE_{name.upper()} 1"]
        for detail in ("high", "low"):
            header += images.render_owner_image_types(name, detail, contexts[detail])
        header += ["#endif", ""]
        rows += [
            f"#if {guard}",
            f"#define NDS_P4_NATIVE_OWNER_IMAGE_ROW_{name.upper()}(X) \\",
            f"    X(NDS_NATIVE_IMAGE_SLOT_{name.upper()}, \"nitro:/fighters/{name}_high.bin\", "
            f"\"nitro:/fighters/{name}_low.bin\", {images._image_type(name, 'high')}, "
            f"{images._image_type(name, 'low')})",
            "#else",
            f"#define NDS_P4_NATIVE_OWNER_IMAGE_ROW_{name.upper()}(X)",
            "#endif",
        ]
        inc += [f"#if {guard}", f"/* P4: donor-derived {O._owner_title(name)} runtime owner. */", ""]
        for detail in ("high", "low"):
            inc += O.render_p2_owner_runtime_program(contexts[detail])
        # The model-part variants the program emitted (donor_variants), by
        # name for the renderer's resolver; NULL and 0 when it has none.
        for detail, suffix in (("high", ""), ("low", "Low")):
            count = len(O._p2_owner_variant_specs(name, detail))
            macro = f"NDS_P4_NATIVE_{name.upper()}_ROOT_VARIANTS{'_LOW' if detail == 'low' else ''}"
            array = f"sNdsNative{O._owner_title(name)}RootVariants{suffix}" if count else "NULL"
            inc += [f"#define {macro} {array}", f"#define {macro}_COUNT {count}u"]
        inc += [f"#endif  /* {guard} */", ""]
        files = []
        for detail in ("high", "low"):
            text = images.render_image(name, detail, contexts[detail]).replace(
                "#include <nds/generated/nds_native_fighter_image.generated.h>",
                "#include <nds_p4_native_image.generated.h>")
            path = out_dir / f"nds_native_fighter_{name}_{detail}.image.c"
            path.write_text(text, encoding="utf-8", newline="\n")
            files.append(path)
        products[name] = files
    header += ["/* One row per P4 image owner (NDS_NATIVE_OWNER_IMAGE_ROWS's shape). */"]
    header += [r for r in rows]
    header += ["#define NDS_P4_NATIVE_OWNER_IMAGE_ROWS(X) \\"]
    header += [f"    NDS_P4_NATIVE_OWNER_IMAGE_ROW_{n.upper()}(X) \\" for n in P4_IMAGE_OWNERS if n in owners]
    header += ["    /* end */", f"#define NDS_P4_NATIVE_IMAGE_SLOTS {len(P4_IMAGE_OWNERS)}u", "",
               "#endif /* NDS_P4_NATIVE_IMAGE_GENERATED_H */", ""]
    (out_dir / "nds_p4_native_image.generated.h").write_text(
        "\n".join(header), encoding="utf-8", newline="\n")
    (out_dir / "nds_p4_native_owner.generated.inc").write_text(
        "\n".join(inc), encoding="utf-8", newline="\n")
    return products


def load_pins(name: str) -> dict | None:
    path = PINS_DIR / f"{name}.json"
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() else None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--export-root", type=Path, required=True,
                    help="remix_export.py output root (each content's main/model ids)")
    ap.add_argument("--owner", help="one owner, for --learn or the pin check")
    ap.add_argument("--o2r", type=Path, help="generate_p4_fighter.py o2r/ directory (--owner)")
    ap.add_argument("--contents", help="--emit: the enabled contents, space separated")
    ap.add_argument("--gen-root", type=Path, help="--emit: the build's P4 directory (<name>/o2r)")
    ap.add_argument("--learn", action="store_true")
    ap.add_argument("--emit", type=Path, help="write the build-local owner products here")
    args = ap.parse_args()
    if args.emit:
        import p4_contents  # noqa: E402

        if args.contents is None or args.gen_root is None:
            ap.error("--emit needs --contents and --gen-root")
        owners = {}
        for row in p4_contents.enabled_rows(args.contents.split()):
            donor = load_donor(row["name"], args.export_root)
            owners[row["name"]] = (args.gen_root / row["name"] / "o2r", donor["attributes"])
        products = emit(args.emit, owners)
        for name, files in products.items():
            print(f"{name}: {[str(p.name) for p in files]}")
        return 0
    if args.owner is None or args.o2r is None:
        ap.error("--owner and --o2r are required without --emit")
    args.attributes = load_donor(args.owner, args.export_root)["attributes"]
    if args.learn:
        pins = learn(args.owner, args.o2r, args.attributes)
        PINS_DIR.mkdir(exist_ok=True)
        (PINS_DIR / f"{args.owner}.json").write_text(json.dumps(pins, indent=1) + "\n",
                                                     encoding="utf-8", newline="\n")
        print(json.dumps(pins))
        return 0
    pins = load_pins(args.owner)
    if pins is None:
        raise SystemExit(f"no pins for {args.owner}; run --learn once")
    register(args.owner, args.o2r, args.attributes, pins)
    for detail in ("high", "low"):
        O.build_p2_owner_runtime_context(REPO, args.owner, detail)
    O.build_p2_owner_model_inventory(REPO, args.owner)
    print(f"{args.owner}: native owner verified against pins")
    return 0


if __name__ == "__main__":
    sys.exit(main())
