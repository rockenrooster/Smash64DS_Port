"""P8 V0: which frames of each efcommon particle texture the reachable
scripts actually show, and what keeping them would cost in the quad atlas.

A script draws its header's texture; its frame changes only where the
bytecode says so (the generator's script_frames: a wait command with the
explicit-frame bit, or opcode 0xBC's random span).
Reachable scripts are the generator's (derive_reachable_scripts: the seeds the
source code spawns and everything they make). Costs are texels a frame at the
source size and at 0.75x (the atlas is one byte a texel).

usage: python scripts/2d_vfx/particle_frame_ledger.py [--json OUT] [--all]
--all lists every texture, not only those whose scripts show >1 frame and
whose atlas row holds fewer.
"""
import json
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "scripts"))
import generate_nds_particle_banks as G  # noqa: E402


def main():
    scripts = G.parse_script_bank(G.load_o2r_blob(REPO, *G.SCRIPT_BANK))
    textures = G.parse_texture_bank(G.load_o2r_blob(REPO, *G.TEXTURE_BANK))
    reach = G.derive_reachable_scripts(REPO, scripts)
    reachable = set(reach["reachable"])
    report = json.load(open(REPO / "docs/optimization/archive/NDS_PARTICLE_BANKS.generated.json"))
    rows_by_tex = {r["texture"]: r for r in report["textures"]}
    # A cell row's "frames" is the source's count; the packed count is its
    # bytes over its cell size (the atlas is one byte a texel, A3I5/A5I3).
    cells = defaultdict(int)
    for cell in report["quads"]["admitted_cells"]:
        packed = cell["bytes"] // max(1, cell["width"] * cell["height"])
        cells[cell["texture"]] = max(cells[cell["texture"]], packed)
    shown = defaultdict(set)
    users = defaultdict(list)
    for s in scripts:
        if s["id"] not in reachable:
            continue
        f = G.script_frames(s) or {0}
        shown[s["texture_id"]] |= f
        users[s["texture_id"]].append(s["id"])
    out = []
    total_1x = total_075 = 0
    for tex in sorted(shown):
        row = rows_by_tex.get(tex, {})
        n = row.get("frames", textures[tex].get("frames", 0) if tex < len(textures) else 0)
        w, h = row.get("width", 0), row.get("height", 0)
        frames = sorted(f for f in shown[tex] if f < max(n, 1))
        have = cells.get(tex, 0)
        per = 1.0  # the atlas is one byte a texel whatever the source format
        w75, h75 = max(1, round(w * 0.75)), max(1, round(h * 0.75))
        cost_1x = int(len(frames) * w * h * per)
        cost_075 = int(len(frames) * w75 * h75 * per)
        entry = {"texture": tex, "size": f"{w}x{h}", "format": row.get("ds_format"), "source_frames": n,
                 "scripted_frames": frames, "atlas_frames": have, "scripts": sorted(users[tex]),
                 "bytes_1x": cost_1x, "bytes_075": cost_075}
        out.append(entry)
        if len(frames) > 1 and have < len(frames):
            total_1x += cost_1x
            total_075 += cost_075
    print(f"{len(reachable)} reachable scripts over {len(shown)} textures")
    print(f"{'tex':>4} {'size':>6} {'fmt':>6} {'src':>4} {'shown':>6} {'atlas':>6} {'1x B':>7} {'0.75x B':>8}  frames")
    for e in out:
        if "--all" not in sys.argv and not (len(e["scripted_frames"]) > 1 and e["atlas_frames"] < len(e["scripted_frames"])):
            continue
        fr = e["scripted_frames"]
        span = f"{fr[0]}..{fr[-1]}" if fr == list(range(fr[0], fr[-1] + 1)) else ",".join(map(str, fr))
        print(f"{e['texture']:>4} {e['size']:>6} {str(e['format']):>6} {e['source_frames']:>4} {len(fr):>6} "
              f"{e['atlas_frames']:>6} {e['bytes_1x']:>7,} {e['bytes_075']:>8,}  {span}")
    print(f"to animate every texture above: {total_1x:,} B at 1x, {total_075:,} B at 0.75x "
          f"({total_075 / 8192:.1f} sheets of 8 KiB)")
    if "--json" in sys.argv:
        json.dump(out, open(sys.argv[sys.argv.index("--json") + 1], "w"), indent=1)


if __name__ == "__main__":
    main()
