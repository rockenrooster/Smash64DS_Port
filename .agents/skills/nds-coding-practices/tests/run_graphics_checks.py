#!/usr/bin/env python3
"""Compile/run actual pixel producers against host RAM; no SDK/GX/OAM emulation."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def extract(file: str, function: str) -> str:
    text = (ROOT / "examples" / file).read_text(encoding="utf-8")
    match = re.search(r"static void " + re.escape(function) + r"\([^)]*\)\n\{.*?\n\}", text, re.S)
    if match is None: raise RuntimeError(f"missing expected example function: {function}")
    return match.group(0)


def main() -> int:
    source = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define RGB15(r,g,b) ((uint16_t)((r) | ((g) << 5) | ((b) << 10)))
#define REQUIRE(c) do { if (!(c)) { fprintf(stderr, "failed line %d\n", __LINE__); return 1; } } while (0)
static uint16_t cutout_pixels[64];
''' + extract("gx_cutout.c", "make_cutout") + "\n" + extract("sprite_oam.c", "fill_sprite_pixels") + r'''
int main(void) {
    uint16_t guarded[130];
    for (unsigned i = 0; i < 130; ++i) guarded[i] = 0xa55a;
    fill_sprite_pixels(guarded + 1);
    REQUIRE(guarded[0] == 0xa55a && guarded[129] == 0xa55a);
    unsigned visible = 0;
    for (unsigned y = 0; y < 16; ++y) {
        for (unsigned x = 0; x < 16; ++x) {
            unsigned tile = (y / 8) * 2 + x / 8;
            unsigned byte = tile * 64 + (y % 8) * 8 + x % 8;
            unsigned pixel = (guarded[1 + byte / 2] >> ((byte % 2) * 8)) & 255;
            bool wanted = x >= 2 && x < 14 && y >= 2 && y < 14;
            REQUIRE(pixel == (unsigned)wanted);
            visible += pixel != 0;
        }
    }
    REQUIRE(visible == 144);
    make_cutout();
    visible = 0;
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            bool wanted = x != 0 && x != 7 && y != 0 && y != 7 &&
                          !(x >= 3 && x <= 4 && y >= 3 && y <= 4);
            bool alpha = (cutout_pixels[y * 8 + x] & 0x8000) != 0;
            REQUIRE(alpha == wanted);
            visible += alpha;
        }
    }
    REQUIRE(visible == 32);
    REQUIRE(cutout_pixels[2 * 8 + 2] == 0x8000); /* Opaque black. */
    puts("PASS: tiled OBJ mask/packing and guards; direct GX fixture has 32 holes and opaque black");
    return 0;
}
'''
    passed = 0
    with tempfile.TemporaryDirectory(prefix="nds-graphics-contracts-") as tmp:
        path = Path(tmp)/"pixels.c"; path.write_text(source, encoding="utf-8")
        for name in ("gcc", "clang"):
            compiler = shutil.which(name)
            if not compiler:
                print(f"SKIP: {name} unavailable")
                continue
            for variant, flags in (("debug", ["-O0"]), ("optimized", ["-O2"]),
                                   ("release", ["-O2", "-DNDEBUG"]),
                                   ("ubsan", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"])):
                exe = Path(tmp)/(name+"-"+variant)
                subprocess.run([compiler,"-std=c11","-Wall","-Wextra","-Werror",*flags,str(path),"-o",str(exe)],check=True,timeout=45)
                subprocess.run([str(exe)],check=True,timeout=45)
                print(f"PASS: {name} {variant}",flush=True)
                passed += 1
    if not passed: return 2
    print(f"{passed} host configurations passed. No SDK API calls, upload, raster output or teardown were executed.")
    return 0

if __name__ == "__main__": raise SystemExit(main())
