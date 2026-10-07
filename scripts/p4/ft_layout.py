"""Field offsets of BattleShip fighter structs, measured by the DS compiler.

The DS reads donor main files through the same FTAttributes type it uses for
the original cast (byte-swapped words, normalized u16 lanes), so the offsets
the ARM compiler assigns to include/ft/fighter.h ARE the N64 file offsets.
Asking the compiler avoids a hand-maintained second description of the layout.
"""
from __future__ import annotations

import os
import struct
import subprocess
import tempfile
from functools import lru_cache
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

FIELDS = {
    "FTAttributes": (
        "commonparts_container", "setup_parts", "animlock", "hiddenparts",
        "dobj_lookup", "modelparts_container", "accesspart",
        "textureparts_container", "sprites", "skeleton", "thrown_status",
        "shield_anim_joints", "translate_scales", "damage_coll_descs",
        "gravity", "shade_color", "fog_color",
    ),
    "FTCommonPart": ("dobjdesc", "p_mobjsubs", "p_costume_matanim_joints", "flags"),
    "FTModelPart": ("dl", "mobjsubs", "costume_matanim_joints", "main_matanim_joints", "flags"),
}
SIZES = ("FTAttributes", "FTCommonPart", "FTCommonPartContainer", "FTModelPart", "DObjDesc")


def _gcc() -> str:
    root = os.environ.get("DEVKITARM", "C:/devkitPro/devkitARM")
    return str(Path(root) / "bin" / "arm-none-eabi-gcc")


@lru_cache(maxsize=1)
def layout() -> dict[str, int]:
    names: list[str] = []
    lines = ["#include <stddef.h>", "#include <ft/fighter.h>", "const unsigned gP4Layout[] = {"]
    for typ, fields in FIELDS.items():
        for f in fields:
            lines.append(f"    offsetof({typ}, {f}),")
            names.append(f"{typ}.{f}")
    for typ in SIZES:
        lines.append(f"    sizeof({typ}),")
        names.append(f"sizeof({typ})")
    lines.append("};")
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "layout.c"
        obj = Path(tmp) / "layout.o"
        src.write_text("\n".join(lines) + "\n", encoding="utf-8")
        devkitpro = os.environ.get("DEVKITPRO", "C:/devkitPro")
        cmd = [
            _gcc(), "-c", "-O0", "-std=gnu11", "-march=armv5te", "-DARM9",
            "-D_LANGUAGE_C", "-DSSB64_TARGET_NDS", "-DREGION_US",
            f"-I{REPO / 'include'}",
            f"-I{REPO / 'decomp/BattleShip-main/decomp/src'}",
            f"-I{REPO / 'decomp/BattleShip-main/decomp/src/sys'}",
            f"-isystem{devkitpro}/libnds/include", f"-isystem{devkitpro}/calico/include",
            "-include", str(_config_header(Path(tmp))),
            str(src), "-o", str(obj),
        ]
        subprocess.run(cmd, check=True, capture_output=True)
        binary = Path(tmp) / "layout.bin"
        objcopy = str(Path(_gcc()).with_name("arm-none-eabi-objcopy"))
        subprocess.run([objcopy, "-O", "binary", "--only-section=.rodata", str(obj), str(binary)],
                       check=True, capture_output=True)
        data = binary.read_bytes()
    values = struct.unpack(f"<{len(names)}I", data[:4 * len(names)])
    return dict(zip(names, values))


def _config_header(tmp: Path) -> Path:
    """A minimal nds_build_config.h: layouts must not depend on feature flags."""
    path = tmp / "nds_build_config.h"
    path.write_text("#define NDS_P4 0\n", encoding="utf-8")
    return path


if __name__ == "__main__":
    for k, v in layout().items():
        print(f"{k:45s} {v:#x}")
