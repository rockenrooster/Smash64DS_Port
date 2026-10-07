#!/usr/bin/env python3
"""Fixtures and negative controls for the P4 Remix source adapter.

Needs a local staging build (scripts/p4/stage_remix.py) because every fixture
reads the user's ROM; skips cleanly when it is absent. Run:

    python scripts/p4/test_remix_adapter.py [--staging builds/p4-staging/remix-5e04fe7]
"""
from __future__ import annotations

import argparse
import json
import shutil
import struct
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import remix_rom as R  # noqa: E402
import remix_export as X  # noqa: E402
import generate_p4_fighter as G  # noqa: E402

REPO = HERE.parents[1]
FAILS: list[str] = []


def check(name: str, ok: bool, detail: str = "") -> None:
    print(f"{'ok  ' if ok else 'FAIL'} {name}{(': ' + detail) if detail else ''}")
    if not ok:
        FAILS.append(name)


def expect_raise(name: str, fn, exc=BaseException, needle: str = "") -> None:
    try:
        fn()
    except exc as e:  # noqa: BLE001
        check(name, needle in str(e), str(e)[:120])
        return
    check(name, False, "no failure raised")


def falco(staging: Path) -> tuple[X.Exporter, dict, X.EventDecoder, X.Failures]:
    fail = X.Failures()
    ex = X.Exporter(staging, "FALCO", fail)
    resolved = ex.export()
    dec = X.EventDecoder(ex.rom, ex.sym_by_addr, resolved["descriptor"]["file_ids"][1], fail)
    for m in resolved["motions"] + resolved["menu_motions"]:
        root = X.script_root(m["script"], resolved["descriptor"]["file_ids"][1])
        if root:
            dec.decode(root, f"r{m['index']}")
    dec.mark_fallthrough()
    return ex, resolved, dec, fail


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--staging", type=Path, default=REPO / "builds/p4-staging/remix-5e04fe7")
    args = ap.parse_args()
    if not (args.staging / "ssb64asm.z64").exists():
        print(f"skip: no staging build at {args.staging}")
        return 0

    ex, resolved, dec, fail = falco(args.staging)
    check("falco exports with no explicit failures", not fail.items, "; ".join(fail.items[:3]))
    check("falco is Remix kind 0x1D on Fox", resolved["remix_kind_id"] == 0x1D and resolved["parent"] == "FOX")

    # Inherited Fox rows resolve to Fox's values; Falco's overrides do not.
    by_name = {s["name"]: s for s in resolved["statuses"] if s.get("name")}
    check("0xE1 Phantasm callbacks are overrides",
          by_name["nFTFoxStatusSpecialN"]["proc_interrupt_name"] == "Phantasm.ground_subroutine_")
    check("0xEC reflector stays Fox's", all(by_name["nFTFoxStatusSpecialLwStart"]["inherited"].values()))
    sentinels = [s for s in resolved["statuses"] if s["motion_id"] in (-1, -2)]
    check("no-motion sentinels decode as -1/-2, not indices", len(sentinels) > 0)

    # Event graph fixtures: loops, the USP_GROUND_MOVE fall-through, throw data.
    ram_blocks = {k: b for k, b in dec.blocks.items() if k.startswith("ram:")}
    loops = [k for k, b in ram_blocks.items()
             if b["exit"] == "goto" and b["edges"] and b["edges"][-1]["to"] == k]
    check("RUN/SPARKLE/STUN/ASLEEP style self loops found", len(loops) >= 4, str(len(loops)))
    falls = {k: b["falls_into"] for k, b in ram_blocks.items() if b.get("falls_into")}
    check("USP_GROUND_MOVE falls through into USP_LOOP exactly once",
          len(falls) == 1 and len(next(iter(falls.values()))) == 1, json.dumps(falls))
    throws = [r for r in dec.data_refs if r["kind"] == "throw_data"]
    check("three THROW_DATA pointers (F/B throw, grab release)", len(throws) == 3)

    # Negative control: an unknown vanilla opcode is an explicit failure.
    class PatchedRom:
        def __init__(self, base, addr, word):
            self.base, self.addr, self.word = base, addr, word

        def __getattr__(self, n):
            return getattr(self.base, n)

        def u32_ram(self, ram):
            return self.word if ram == self.addr else self.base.u32_ram(ram)

    run = next(k for k in ram_blocks)
    addr = int(run.split(":")[1], 16)
    bad = X.Failures()
    X.EventDecoder(PatchedRom(ex.rom, addr, 0x3F << 26), {}, 0xD0, bad).decode(("ram", addr), "neg")
    # Opcodes 52..63 all begin 0xD0..0xFF, Remix's custom range, so an unknown
    # top-six-bit opcode surfaces as an unknown custom command.
    check("unknown opcode is rejected", any("unknown" in f for f in bad.items), str(bad.items[:1]))
    bad = X.Failures()
    X.EventDecoder(PatchedRom(ex.rom, addr, 0xDE000000), {}, 0xD0, bad).decode(("ram", addr), "neg")
    check("unported custom command 0xDE is rejected", any("unknown custom" in f for f in bad.items))
    bad = X.Failures()
    X.EventDecoder(PatchedRom(ex.rom, addr + 4, 0x80001234), {}, 0xD0, bad)
    dec2 = X.EventDecoder(PatchedRom(PatchedRom(ex.rom, addr, 0x90000000), addr + 4, 0x80001234), {}, 0xD0, bad)
    dec2.decode(("ram", addr), "neg")
    check("goto into vanilla RAM is rejected", any("outside the Remix region" in f for f in bad.items))

    # Negative control: a missing inherited resource breaks the closure.
    class MissingRom:
        def __init__(self, base, missing):
            self.base, self.missing = base, missing

        def extern_ids(self, fid):
            if fid == self.missing:
                raise R.RomError(f"file {fid:#x} missing")
            return self.base.extern_ids(fid)

    expect_raise("missing inherited resource fails the closure",
                 lambda: X.file_closure(MissingRom(ex.rom, 0x13A), {0x8AB}), R.RomError, "missing")

    # Negative control: an unclassified donor-modified vanilla file.
    spec = G.FIGHTERS["FALCO"]
    saved = dict(spec["equivalent_files"])
    spec["equivalent_files"].clear()
    out = Path(tempfile.mkdtemp(prefix="p4neg"))
    try:
        exp = out / "export"
        exp.mkdir()
        (exp / "resolved.json").write_text(json.dumps(resolved), encoding="utf-8")
        (exp / "events.json").write_text(json.dumps({"blocks": dec.blocks, "data_refs": dec.data_refs}),
                                         encoding="utf-8")
        resolved["file_closure"] = [
            {"file_id": f, "vanilla_id": f < 2132, "bytes": 0, "externs": [],
             "vanilla_identical": f != 0xA1} for f in X.file_closure(ex.rom, {0x8AB, 0xA1})]
        (exp / "resolved.json").write_text(json.dumps(resolved), encoding="utf-8")
        sys.argv = ["g", "--staging", str(args.staging), "--export", str(exp), "--out", str(out / "gen")]
        expect_raise("donor bytes under a vanilla ID need a classification", G.main, SystemExit,
                     "no classification")
    finally:
        spec["equivalent_files"].update(saved)
        shutil.rmtree(out, ignore_errors=True)

    # Negative control: the stager refuses a pin that is not the source lock.
    lock = json.loads((REPO / "docs/P4/source-lock.json").read_text(encoding="utf-8"))
    pin = next(s["commit"] for s in lock["submodules"] if s["path"] == "decomp/smashremix")
    check("source lock pins the staged Remix tree", pin.startswith("5e04fe7"))

    # O2R writer fixture against Torch's own export (all 2,132 vanilla files).
    vanilla = R.Rom(args.staging / "roms/ssb.rom")
    o2r_root = REPO / "decomp/BattleShip-main/BattleShip_o2r"
    if o2r_root.exists():
        res = R.verify_vanilla_o2r(vanilla, o2r_root, REPO / "decomp/BattleShip-main/yamls/us")
        check("O2R writer is byte-identical to Torch", res["different"] == 0 and res["identical"] == 2132,
              json.dumps(res))
    # The appended 0xA1 block is ExternDataBank109's bytes (the classified deviation).
    a1 = ex.rom.file_bytes(0xA1)
    bank = vanilla.file_bytes(109)
    check("Remix 0xA1 tail equals ExternDataBank109+0x19F8",
          a1[0x2F80:0x2F80 + 992] == bank[0x19F8:0x19F8 + 992])

    print(f"{len(FAILS)} failure(s)")
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())
