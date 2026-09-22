#!/usr/bin/env python3
"""R01-A: a Results bitmap sprite must never be emitted with alpha 0.

`oamSet`'s fifth argument is the palette bank for paletted sprites but the
ALPHA for a bitmap sprite, and libnds requires that value to be greater than
zero for a bitmap sprite to display at all (nds/arm9/sprite.h:391).
`ndsResultsPaletteForSObj` returns 0 for exactly the formats that bake to
`SpriteColorFormat_Bmp`, because bank 0 is a legitimate answer for a paletted
sprite. Passing that 0 straight through emitted every bitmap sprite fully
transparent -- which is why the first-place badge (`llMNVSResultsWinnerSprite`,
RGBA/16b, the only Bmp SObj in the place row) was missing while its IA8
siblings drew.

It failed silently: prepared, baked, emitted, given an OAM slot. This test is
the standing guard, plus the counter pair that makes it decidable on hardware.

    python scripts/menus/test_results_bitmap_alpha.py
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src" / "nds" / "nds_results_oam.c"
SIBLING = ROOT / "src" / "nds" / "nds_ifcommon_oam.c"

failures: list[str] = []


def check(condition: bool, message: str) -> None:
    if not condition:
        failures.append(message)


def body_of(text: str, signature: str) -> str:
    start = text.index(signature)
    open_brace = text.index("{", start)
    depth = 0
    for i in range(open_brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace:i + 1]
    raise AssertionError(f"unbalanced braces after {signature!r}")


def main() -> int:
    for path in (SOURCE, SIBLING):
        if not path.is_file():
            print(f"FAIL: missing {path}", file=sys.stderr)
            return 1
    text = SOURCE.read_text(encoding="utf-8")

    # The premise this guard rests on: the palette helper still answers 0 for
    # the bitmap formats. If that ever changes the guard below is arguing
    # about the wrong thing and should be revisited, not silently kept.
    palette = body_of(text, "static s32 ndsResultsPaletteForSObj(const SObj *sobj)")
    check("G_IM_FMT_RGBA" in palette and "return 0;" in palette,
          "ndsResultsPaletteForSObj no longer returns 0 for RGBA; re-derive "
          "this guard rather than assuming its premise still holds")

    emit = body_of(text, "static s32 ndsResultsEmitSObj(const SObj *sobj)")

    check("ndsResultsBitmapAlpha" in emit,
          "ndsResultsEmitSObj does not compute a bitmap alpha; every "
          "SpriteColorFormat_Bmp OBJ it emits would be invisible")
    check("sobj->sprite.alpha" in emit,
          "ndsResultsEmitSObj does not read the SObj's own alpha; the source "
          "owns that value, the renderer must not invent one")

    # The emit must SELECT on the colour format -- passing the bitmap alpha
    # unconditionally would corrupt the paletted siblings' bank.
    oam_set = re.search(r"oamSet\(&oamMain,\s*sNdsResultsNextOamId[^;]*;", emit,
                        re.S)
    check(oam_set is not None, "ndsResultsEmitSObj no longer calls oamSet")
    if oam_set is not None:
        call = oam_set.group(0)
        check("SpriteColorFormat_Bmp" in call and "?" in call,
              "ndsResultsEmitSObj's oamSet does not select its fifth argument "
              "on the colour format; that field is a palette bank for "
              "paletted sprites and an alpha for bitmap sprites")
        check("bitmap_alpha" in call and "palette_bank" in call,
              "ndsResultsEmitSObj's oamSet must pass bitmap_alpha for Bmp "
              "cells and palette_bank otherwise")

    # The evidence counters, without which this is undecidable on hardware:
    # every other counter on this path reads healthy either way.
    for counter in ("gNdsResultsOamBmpEmitCount",
                    "gNdsResultsOamBmpZeroAlphaCount"):
        check(text.count(counter) >= 2,
              f"{counter} is not defined and incremented; a zero-alpha bitmap "
              "emit would again be indistinguishable from a healthy one")
    check("gNdsResultsOamBmpZeroAlphaCount++" in emit,
          "the zero-alpha counter is not incremented where the emit happens")

    # The contrast proof: the sibling owner has always done this correctly.
    sibling = SIBLING.read_text(encoding="utf-8")
    check("SpriteColorFormat_Bmp) ?" in sibling.replace("\n", " ").replace(
              "  ", " ") or "tile->color_format == SpriteColorFormat_Bmp"
          in sibling,
          "nds_ifcommon_oam.c no longer selects its OAM alpha on the colour "
          "format; the two Results/interface owners must not diverge")

    if failures:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1
    print("PASS: Results bitmap-sprite OAM alpha "
          "(format-selected, source-owned, counted)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
