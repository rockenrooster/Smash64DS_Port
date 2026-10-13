#!/usr/bin/env python3
"""P5 T7: render a Smash Remix song on Remix's own music bank.

The vanilla renderer (scripts/sfx/bgm/render-audio-bgm.py) reads the
BattleShip o2r's B1_sounds1 bank, whose instrument array stops before the
programs Remix appends (midi.asm add_instrument), and resolves a program
past it to the percussion instrument -- a wrong instrument and no error.
This runs the same renderer with the bank read from the staged Remix build
(scripts/p5/remix_bank.py): the vanilla instruments are the same bytes, the
appended ones resolve, and a program past even Remix's bank still fails
closed here instead of borrowing percussion.

    python scripts/p5/render_remix_bgm.py --staging <remix staging> \
        [--remix-bgm-id <id>] --sequence-file <song.bin> --output <out.bin> \
        [renderer options]

With --remix-bgm-id the song also takes Remix's per-song overrides from the
assembled tables (midi.asm add_master_volume_override, the master volume
every note of the song plays at, and add_bend_range_override, a bend range
per instrument). Every other option passes through unchanged.
"""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts" / "sfx"))
sys.path.insert(0, str(REPO / "scripts" / "sfx" / "bgm"))
import remix_bank  # noqa: E402

RENDERER = REPO / "scripts" / "sfx" / "bgm" / "render-audio-bgm.py"


def song_overrides(staging: Path, bgm_id: int) -> tuple[int, dict[int, int]]:
    """(master volume or 0, {program: bend range}) from the ROM's tables."""
    import struct
    sys.path.insert(0, str(REPO / "scripts" / "p4"))
    import remix_rom  # noqa: E402
    rom = remix_rom.Rom(staging / "ssb64asm.z64")
    sym, _ = remix_rom.load_symbols(staging / "logfile.log")
    volume = rom.read_ram(sym["MIDI.master_volume_override_table"] + bgm_id, 1)[0]
    bends = {}
    array = rom.u32_ram(sym["MIDI.bend_range_override_table"] + 4 * bgm_id)
    if array:
        for program in range(128):
            value = struct.unpack(">H", rom.read_ram(array + 2 * program, 2))[0]
            if value:
                bends[program] = value
    return volume, bends


def main() -> int:
    argv = list(sys.argv[1:])
    if "--staging" not in argv:
        raise SystemExit("render_remix_bgm: --staging <remix staging> is required")
    at = argv.index("--staging")
    staging = Path(argv[at + 1])
    del argv[at:at + 2]
    bgm_id = None
    if "--remix-bgm-id" in argv:
        at = argv.index("--remix-bgm-id")
        bgm_id = int(argv[at + 1], 0)
        del argv[at:at + 2]
    spec = importlib.util.spec_from_file_location("render_audio_bgm", RENDERER)
    renderer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(renderer)
    ctl, tbl = remix_bank.remix_bank(staging)
    banks = {renderer.BGM_SEQUENCE_BANK_CTL: ctl, renderer.BGM_SEQUENCE_BANK_TBL: tbl}
    vanilla_read = renderer.read_o2r_payload

    def read_o2r_payload(path):
        name = Path(path).name
        return banks[name] if name in banks else vanilla_read(path)

    vanilla_resolve = renderer.resolve_instrument

    def resolve_instrument(bank, program):
        offsets = bank.get("instArray_offs", [])
        if not (0 <= program < len(offsets)):
            raise SystemExit(f"render_remix_bgm: program {program} is past Remix's "
                             f"{len(offsets)}-instrument bank")
        return vanilla_resolve(bank, program)

    renderer.read_o2r_payload = read_o2r_payload
    renderer.resolve_instrument = resolve_instrument
    if bgm_id is not None:
        volume, bends = song_overrides(staging, bgm_id)
        if volume:
            collect = renderer.collect_notes

            def collect_notes(*args, **kwargs):
                notes, tempo = collect(*args, **kwargs)
                for note in notes:
                    note["master_volume"] = volume
                return notes, tempo

            renderer.collect_notes = collect_notes
        if bends:
            load = renderer.load_module

            def load_module(path, name):
                module = load(path, name)
                if name == "decode_ctl":
                    walk = module.walk

                    def bent_walk(data):
                        structs = walk(data)
                        by_off = {s["offset"]: s for s in structs}
                        bank = next(s for s in structs if s.get("kind") == "ALBank")
                        offs = bank["instArray_offs"]
                        for program, value in bends.items():
                            if program < len(offs) and offs[program] in by_off:
                                by_off[offs[program]]["bendRange"] = value
                        return structs

                    module.walk = bent_walk
                return module

            renderer.load_module = load_module
        print(f"render_remix_bgm: bgm {bgm_id:#x} master volume {volume or 'source'}, "
              f"bend overrides {bends or 'none'}")
    sys.argv = [str(RENDERER)] + argv
    return renderer.main()


if __name__ == "__main__":
    sys.exit(main())
