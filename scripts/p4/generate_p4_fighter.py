#!/usr/bin/env python3
"""P4 source adapter, stage 2: lower one exported Remix fighter into DS build inputs.

Reads remix_export.py's resolved.json/events.json and the staged reference ROM
and writes, under --out (a build directory, never a tracked one, because every
output is derived from the user's ROM):

  o2r/<id>                 Torch SSB64:RELOC containers the DS reloc loader reads:
                           the fighter's donor-only files, plus a synthesized
                           main-motion file = the parent's motion file followed
                           by every reachable Remix-inserted stream and its data,
                           with each Remix absolute pointer turned into an
                           internal relocation, so ftMainSetStatus's ordinary
                           `file head + offset` path runs the donor scripts.
  nds_p4_<name>.generated.c  FTData, motion and menu-motion descriptors, the
                           status-callback overrides and kind-table rows.
  manifest.json            identities, hashes and every classified deviation.

Remix callbacks must map to a hand-ported DS function in CALLBACK_PORTS; vanilla
callbacks map to their decomp names. Anything else fails the generation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import remix_rom as R  # noqa: E402

NO_SCRIPT = 0x80000000
# DS-synthesized files live above the donor's largest ID (Remix: 5455 files).
SYNTH_FILE_BASE = 0x1600

# Hand-ported native callbacks for donor routines (src/port/nds_p4_*.c).
CALLBACK_PORTS = {
    "Phantasm.ground_subroutine_": "ndsP4FalcoPhantasmGroundInterrupt",
    "Phantasm.air_subroutine_": "ndsP4FalcoPhantasmAirInterrupt",
    "Phantasm.air_physics_": "ndsP4FalcoPhantasmAirPhysics",
    "Phantasm.air_collision_": "ndsP4FalcoPhantasmAirMap",
}

# Per-fighter DS facts that are not in the donor tables: the parent's file
# storage that parent callbacks read through globals (Fox code reaches its
# special files via gFTDataFoxSpecial*), and donor file substitutions proven
# equivalent (see manifest "deviations").
FIGHTERS = {
    "FALCO": {
        "name": "falco", "title": "Falco", "kind_index": 0,
        "parent_kind": "nFTKindFox", "parent_data": "dFTFoxData",
        "shared_storage": {
            "p_file_shieldpose": None,
            "p_file_special1": "gFTDataFoxSpecial1",
            "p_file_special2": "gFTDataFoxSpecial2",
            "p_file_special3": "gFTDataFoxSpecial3",
            "p_file_special4": "gFTDataFoxSpecial4",
        },
        "particle": "gFTDataFoxParticleBankID",
        # Remix rewrote FoxSpecial3 (0xA1) for every Arwing user: three
        # external references to ExternDataBank109+0x19F8 became one internal
        # copy appended at 0x2F80. Same bytes, no new dependency; the DS keeps
        # the vanilla file the parent already ships.
        "equivalent_files": {0xA1: "remix copies ExternDataBank109+0x19F8 inline; vanilla bytes equivalent"},
    },
}

THROW_DATA_BYTES = 28  # FTThrowHitDesc, the larger of the two SetThrow targets


class GenError(SystemExit):
    pass


def parse_key(key: str) -> tuple:
    kind, rest = key.split(":", 1)
    if kind == "ram":
        return ("ram", int(rest, 16))
    fid, off = rest.split(":")
    return ("file", int(fid, 16), int(off, 16))


def command_length(op: str) -> int:
    from remix_export import EVENTS, REMIX_CUSTOM  # noqa: E402
    for name, length in EVENTS:
        if name == op:
            return length
    for name, length in REMIX_CUSTOM.values():
        if name == op:
            return length
    raise GenError(f"unknown op {op}")


def merge_intervals(spans: list[tuple[int, int]], gap: int = 0x40) -> list[tuple[int, int]]:
    out: list[list[int]] = []
    for lo, hi in sorted(spans):
        if out and lo <= out[-1][1] + gap:
            out[-1][1] = max(out[-1][1], hi)
        else:
            out.append([lo, hi])
    return [(a, b) for a, b in out]


class MotionSynth:
    """Parent motion file + appended donor streams, with one rebuilt intern chain."""

    def __init__(self, rom: R.Rom, parent_motion: int, events: dict, synth_id: int):
        self.rom = rom
        self.parent_motion = parent_motion
        self.synth_id = synth_id
        self.blocks = events["blocks"]
        self.data_refs = events["data_refs"]
        base = bytearray(rom.file_bytes(parent_motion))
        if len(base) % 4:
            raise GenError("parent motion file not word sized")
        self.base_len = len(base)
        spans = []
        for key, block in self.blocks.items():
            loc = parse_key(key)
            if loc[0] != "ram":
                continue
            last = block["commands"][-1] if block["commands"] else None
            end = parse_key(last["at"])[1] + command_length(last["op"]) if last else loc[1]
            spans.append((loc[1], end))
        for ref in self.data_refs:
            loc = parse_key(ref["to"])
            if loc[0] == "ram":
                spans.append((loc[1], loc[1] + THROW_DATA_BYTES))
        self.intervals = merge_intervals(spans)
        self.ram_to_off: list[tuple[int, int, int]] = []
        data = base
        for lo, hi in self.intervals:
            lo &= ~3
            hi = (hi + 3) & ~3
            self.ram_to_off.append((lo, hi, len(data)))
            data += rom.read_ram(lo, hi - lo)
        if len(data) >= 0x40000:
            raise GenError("synthesized motion file exceeds the 16-bit word reloc range")
        self.data = data

    def map_ram(self, addr: int) -> int:
        for lo, hi, off in self.ram_to_off:
            if lo <= addr < hi:
                return off + (addr - lo)
        raise GenError(f"RAM {addr:#010x} not inside any copied interval")

    def map_loc(self, loc: tuple) -> int:
        if loc[0] == "ram":
            return self.map_ram(loc[1])
        if loc[1] != self.parent_motion:
            raise GenError(f"pointer into foreign file {loc[1]:#x} needs an extern slot")
        return loc[2]

    def build(self) -> bytes:
        rom = self.rom
        entry = rom.entry(self.parent_motion)
        slots = rom.reloc_slots(self.parent_motion)
        intern = {off: tgt[1] for off, tgt in slots.items() if tgt[0] == "intern"}
        # Remix pointer words: the second word of each pointer command in a
        # copied RAM block, and the RANDOM_SFX table pointer.
        for key, block in self.blocks.items():
            if not key.startswith("ram:"):
                continue
            for cmd in block["commands"]:
                at = parse_key(cmd["at"])
                target = cmd.get("to") or cmd.get("data")
                if target is None:
                    if cmd["op"] in ("Goto", "Subroutine", "SetParallelScript", "SetThrow",
                                     "SetDamageThrown", "RemixRandomSFX"):
                        raise GenError(f"{cmd['at']}: unresolved pointer")
                    if cmd["op"] == "RemixGotoMovesetFile":
                        raise GenError(f"{cmd['at']}: GO_TO_FILE needs a DS lowering")
                    continue
                slot_off = self.map_ram(at[1] + 4)
                intern[slot_off] = self.map_loc(parse_key(target))
        data = bytearray(self.data)
        order = sorted(intern)
        for i, off in enumerate(order):
            nxt = (order[i + 1] // 4) if i + 1 < len(order) else 0xFFFF
            tgt = intern[off]
            if tgt % 4 or tgt // 4 >= 0xFFFF:
                raise GenError(f"intern target {tgt:#x} not encodable")
            struct.pack_into(">I", data, off, (nxt << 16) | (tgt // 4))
        head = (order[0] // 4) if order else 0xFFFF
        ext = rom.extern_ids(self.parent_motion)
        out = bytearray(R.O2R_HEADER)
        out += struct.pack("<IHHI", self.synth_id, head, entry.reloc_extern, len(ext))
        for x in ext:
            out += struct.pack("<H", x)
        out += struct.pack("<I", len(data))
        out += data
        self.intern_count = len(order)
        return bytes(out)


class _SynthRom:
    """Just enough of remix_rom.Rom for EventDecoder to walk one O2R container."""

    def __init__(self, base: R.Rom, blob: bytes):
        self.base = base
        self.file_id, self.intern, self.extern, n = struct.unpack_from("<IHHI", blob, 0x40)
        self.externs = list(struct.unpack_from(f"<{n}H", blob, 0x4C))
        size_off = 0x4C + 2 * n
        size = struct.unpack_from("<I", blob, size_off)[0]
        self.body = blob[size_off + 4:size_off + 4 + size]

    def file_bytes(self, fid):
        return self.body if fid == self.file_id else self.base.file_bytes(fid)

    def entry(self, fid):
        if fid != self.file_id:
            return self.base.entry(fid)
        return R.TableEntry(False, 0, self.intern, 0, self.extern, len(self.body) // 4)

    def extern_ids(self, fid):
        return self.externs if fid == self.file_id else self.base.extern_ids(fid)

    reloc_slots = R.Rom.reloc_slots

    def u32_ram(self, ram):
        raise R.RomError("synthesized file has no RAM space")


def round_trip(rom: R.Rom, blob: bytes, synth: "MotionSynth", roots: list[tuple[str, int]]) -> int:
    """Decode each root inside the synthesized file and compare the op/word
    stream with the donor decode; pointer words are compared by mapped target."""
    from remix_export import EventDecoder, Failures, script_root  # noqa: E402
    fake = _SynthRom(rom, blob)
    fail = Failures()
    dec = EventDecoder(fake, {}, fake.file_id, fail)
    donor = EventDecoder(rom, {}, synth.parent_motion, Failures())
    checked = 0
    for origin, value in roots:
        if value == NO_SCRIPT:
            continue
        src_root = script_root(value, synth.parent_motion)
        donor.decode(src_root, origin)
        new_root = ("file", fake.file_id, synth.map_ram(value) if value & 0x80000000 else value)
        dec.decode(new_root, origin)
        checked += 1
    if fail.items:
        raise GenError("round trip decode failed: " + "; ".join(fail.items[:5]))

    def stream(blocks, key, seen):
        out = []
        while key and key not in seen:
            seen.add(key)
            b = blocks[key]
            for c in b["commands"]:
                words = c["words"][:1]
                out.append((c["op"], tuple(words)))
            nxt = [e["to"] for e in b["edges"] if e["kind"] == "goto"]
            key = nxt[0] if nxt else None
        return out

    for origin, value in roots:
        if value == NO_SCRIPT:
            continue
        a = stream(donor.blocks, donor.key(script_root(value, synth.parent_motion)), set())
        b = stream(dec.blocks, dec.key(("file", fake.file_id,
                                        synth.map_ram(value) if value & 0x80000000 else value)), set())
        if a != b:
            raise GenError(f"round trip mismatch for {origin}")
    if len(dec.blocks) != len(donor.blocks):
        raise GenError(f"round trip block count {len(dec.blocks)} != donor {len(donor.blocks)}")
    return checked


def c_ident_list(values, per_line=4, fmt="{:#010x}"):
    items = [fmt.format(v) for v in values]
    return ",\n    ".join(", ".join(items[i:i + per_line]) for i in range(0, len(items), per_line))


def retarget_o2r_externs(blob: bytes, mapping: dict[int, int]) -> bytes:
    """Rewrite external file IDs in an O2R container (same count, same order)."""
    n = struct.unpack_from("<I", blob, 0x48)[0]
    out = bytearray(blob)
    for i in range(n):
        off = 0x4C + 2 * i
        fid = struct.unpack_from("<H", blob, off)[0]
        if fid in mapping:
            struct.pack_into("<H", out, off, mapping[fid])
    return bytes(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--export", type=Path, required=True, help="remix_export.py --out directory")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    resolved = json.loads((args.export / "resolved.json").read_text(encoding="utf-8"))
    events = json.loads((args.export / "events.json").read_text(encoding="utf-8"))
    fighter = resolved["fighter"]
    spec = FIGHTERS.get(fighter)
    if spec is None:
        raise GenError(f"{fighter}: no DS fighter spec")
    rom = R.Rom(args.staging / "ssb64asm.z64")
    vanilla = R.Rom(args.staging / "roms" / "ssb.rom")
    name, title = spec["name"], spec["title"]
    desc = resolved["descriptor"]
    file_ids = list(desc["file_ids"])
    parent_motion = file_ids[1]

    synth_id = SYNTH_FILE_BASE + spec["kind_index"] * 0x10
    if rom.file_bytes(parent_motion) != vanilla.file_bytes(parent_motion):
        raise GenError(f"parent motion {parent_motion:#x} differs from vanilla")
    synth = MotionSynth(rom, parent_motion, events, synth_id)
    motion_o2r = synth.build()

    deviations = []
    o2r_out = args.out / "o2r"
    o2r_out.mkdir(parents=True, exist_ok=True)
    shipped = {}
    for row in resolved["file_closure"]:
        fid = row["file_id"]
        if row["vanilla_id"]:
            if not row["vanilla_identical"]:
                why = spec["equivalent_files"].get(fid)
                if why is None:
                    raise GenError(f"vanilla ID {fid:#x} carries donor bytes with no classification")
                deviations.append({"file_id": fid, "class": "equivalent", "why": why})
            continue
        blob = rom.o2r(fid)
        # The main file's references into the parent motion file resolve to the
        # same offsets in the synthesized copy, so the parent file never loads.
        blob = retarget_o2r_externs(blob, {parent_motion: synth_id})
        (o2r_out / f"{fid:04x}").write_bytes(blob)
        shipped[fid] = hashlib.sha256(blob).hexdigest()
    (o2r_out / f"{synth_id:04x}").write_bytes(motion_o2r)
    shipped[synth_id] = hashlib.sha256(motion_o2r).hexdigest()

    def script_offset(value: int) -> int:
        if value == NO_SCRIPT:
            return NO_SCRIPT
        if value & 0x80000000:
            return synth.map_ram(value)
        return value

    roots = [(f"motion:{m['index']:#x}", m["script"]) for m in resolved["motions"]]
    roots += [(f"menu:{m['index']:#x}", m["script"]) for m in resolved["menu_motions"]]
    checked = round_trip(rom, motion_o2r, synth, roots)

    motions = resolved["motions"]
    rows = []
    for m in motions:
        rows.append((m["anim_file_id"], script_offset(m["script"]), m["anim_flags"]))
    menus = []
    for m in resolved["menu_motions"]:
        s = m["script"]
        if s != NO_SCRIPT and not (s >= R.REMIX_CODE_RAM):
            raise GenError(f"menu motion {m['index']}: vanilla script pointer {s:#x} needs a symbol")
        menus.append((m["anim_file_id"], script_offset(s), m["anim_flags"]))

    overrides = []
    for s in resolved["statuses"]:
        inh = s.get("inherited")
        if not inh or all(inh.values()):
            continue
        if not all(inh.get(k, True) for k in ("motion_id", "attack_id", "sflags")):
            raise GenError(f"status {s['status_id']:#x}: descriptor (not only callbacks) differs")
        procs = []
        for proc in ("proc_update", "proc_interrupt", "proc_physics", "proc_map"):
            nm = s.get(proc + "_name")
            if s[proc] == 0:
                procs.append("NULL")
            elif nm in CALLBACK_PORTS:
                procs.append(CALLBACK_PORTS[nm])
            elif nm and "." not in nm:
                procs.append(nm)
            else:
                raise GenError(f"status {s['status_id']:#x} {proc}: {nm!r} has no DS port")
        overrides.append((s["status_id"], procs))

    # ftManagerSetupFileSize's three answers, computed the way
    # generate_fighter_production_manifest.extern_alloc_size does for the
    # legacy roster: 16-byte aligned payloads, each dependency counted once.
    def alloc_size(root: int) -> int:
        seen: set[int] = set()

        def visit(fid: int) -> int:
            if fid in seen:
                return 0
            seen.add(fid)
            if fid == synth_id:
                size, deps = len(synth.data), rom.extern_ids(parent_motion)
            else:
                size, deps = len(rom.file_bytes(fid)), rom.extern_ids(fid)
                deps = [synth_id if d == parent_motion and fid == file_ids[0] else d for d in deps]
            total = (size + 0xF) & ~0xF
            for d in deps:
                total = (total + 0xF) & ~0xF
                total += visit(d)
            return total
        return visit(root)

    def largest(rows_):
        best = 0
        for a, _, f in rows_:
            if a and not (f & 0x2):  # FTANIM_FLAG_SHIELDPOSE rows load from the pose file
                best = max(best, alloc_size(a))
        return best

    sizes = (alloc_size(file_ids[0]), largest([(m["anim_file_id"], 0, m["anim_flags"]) for m in resolved["motions"]]),
             largest([(m["anim_file_id"], 0, m["anim_flags"]) for m in resolved["menu_motions"]]))

    own = {row["file_id"] for row in resolved["file_closure"] if not row["vanilla_id"]}
    anim_flags: dict[int, int] = {}
    for m in resolved["motions"] + resolved["menu_motions"]:
        a = m["anim_file_id"]
        if a in own and not (m["anim_flags"] & 0x2):
            anim_flags[a] = anim_flags.get(a, 0) | m["anim_flags"]
    anims = [(a, bool(f & 0x8)) for a, f in sorted(anim_flags.items())]
    if any(a >= 0x8000 for a, _ in anims):
        raise GenError("animation id does not fit the 15-bit table encoding")

    ident = f"NdsP4{title}"
    store = spec["shared_storage"]
    lines = [
        f"/* Generated by scripts/p4/generate_p4_fighter.py for {fighter} from the staged",
        " * Remix donor build. Derived from the user's ROM: build output only, never tracked. */",
        "#include <nds/nds_p4.h>",
        "",
    ]
    proto = sorted({p for _, procs in overrides for p in procs if p != "NULL"})
    for p in proto:
        lines.append(f"void {p}(GObj *fighter_gobj);")
    lines += [
        "",
        f"static FTMotionDesc s{ident}MotionDescs[{len(rows)}] = {{",
    ]
    for a, s, f in rows:
        lines.append(f"    {{ {a:#06x}, (intptr_t){s:#010x}, {{ .word = {f:#010x} }} }},")
    lines += ["};", "", f"/* Script fields are offsets into the synthesized motion file until",
              " * ndsP4BindMenuScripts adds its loaded address (opening statuses read",
              " * them as absolute pointers). */",
              f"static FTMotionDesc s{ident}SubMotionDescs[{len(menus)}] = {{"]
    for a, s, f in menus:
        lines.append(f"    {{ {a:#06x}, (intptr_t){s:#010x}, {{ .word = {f:#010x} }} }},")
    lines += ["};", f"static s32 s{ident}SubMotionCount = {len(menus)};",
              f"static void *s{ident}Main;", f"static void *s{ident}MainMotion;",
              f"static void *s{ident}Model;", ""]
    for _, v in store.items():
        if v:
            lines.append(f"extern void *{v};")
    lines.append(f"extern s32 {spec['particle']};")
    lines += ["", f"FTData g{ident}Data = {{"]
    ids = file_ids[:]
    ids[1] = synth_id
    lines.append("    " + ", ".join(f"{x:#x}" for x in ids) + ",")
    lines.append("    0,")
    lines.append(f"    &s{ident}Main, &s{ident}MainMotion, NULL, &s{ident}Model,")
    sp = store["p_file_shieldpose"]
    lines.append(f"    {('&' + sp) if sp else 'NULL'},")
    lines.append("    " + ", ".join(f"&{store[k]}" for k in
                                     ("p_file_special1", "p_file_special2", "p_file_special3", "p_file_special4")) + ",")
    lines.append(f"    &{spec['particle']}, 0, 0, 0, 0,")
    lines.append(f"    {desc['o_attributes']:#x},")
    lines.append(f"    (FTMotionDescArray *)s{ident}MotionDescs,")
    lines.append(f"    (FTMotionDescArray *)s{ident}SubMotionDescs,")
    lines.append(f"    {len(rows)}, &s{ident}SubMotionCount, 0")
    lines += ["};", "", f"const NDSP4StatusOverride g{ident}StatusOverrides[] = {{"]
    for sid, procs in overrides:
        lines.append(f"    {{ {sid:#x}, {{ {', '.join(procs)} }} }},")
    lines += ["};", f"const u32 g{ident}StatusOverrideCount = {len(overrides)};", "",
              f"const NDSP4RelocAsset g{ident}RelocAssets[] = {{"]
    for fid in sorted(shipped):
        lines.append(f"    {{ {fid:#x}, \"nitro:/reloc/p4/{fid:04x}\" }},")
    lines += ["};", f"const u32 g{ident}RelocAssetCount = {len(shipped)};", "",
              "/* The content's own animation files; bit 15 marks AObjEvent32 (FTANIM_FLAG_ANIMJOINT). */",
              f"const u16 g{ident}Anims[] = {{",
              "    " + ", ".join(f"{a | (0x8000 if ev else 0):#06x}" for a, ev in anims) + ",",
              "};", f"const u32 g{ident}AnimCount = {len(anims)};", "",
              "/* FTFileSize: main closure, largest main-motion and menu-motion figatree. */",
              f"const FTFileSize g{ident}FileSize = {{ {sizes[0]}u, {sizes[1]}u, {sizes[2]}u }};", ""]
    src = "\n".join(lines)
    (args.out / f"nds_p4_{name}.generated.c").write_text(src, encoding="utf-8", newline="\n")

    manifest = {
        "schema": "p4-ds-fighter-v1", "fighter": fighter,
        "reference_rom_sha1": resolved["provenance"]["reference_rom_sha1"],
        "synthesized_motion": {"file_id": synth_id, "parent": parent_motion,
                               "round_trip_roots": checked,
                               "parent_bytes": synth.base_len, "bytes": len(synth.data),
                               "intern_slots": synth.intern_count,
                               "remix_intervals": [[hex(a), hex(b)] for a, b in synth.intervals]},
        "shipped_o2r_sha256": {f"{k:#x}": v for k, v in sorted(shipped.items())},
        "status_overrides": [{"status": hex(s), "procs": p} for s, p in overrides],
        "file_size": {"main": sizes[0], "mainmotion_largest_anim": sizes[1],
                      "submotion_largest_anim": sizes[2]},
        "deviations": deviations,
    }
    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=1), encoding="utf-8")
    print(json.dumps({k: manifest[k] for k in ("synthesized_motion", "status_overrides", "deviations")}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
