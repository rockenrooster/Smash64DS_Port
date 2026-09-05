#!/usr/bin/env python3
"""Oracle for stage_reloc_file.py: the geometry rows it derives from the o2r
containers must equal the rows the port wrote by hand for the battle HUD
files in src/port/reloc_backend_assets.c (sNdsBattleInterfaceSpriteDescs).
Every derived sprite that has a hand row must match it field for field; the
counts of hand rows without a derived twin (and vice versa) are printed so a
partial hand table reads as partial, not as a mismatch."""
from __future__ import annotations

import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import stage_reloc_file as stage  # noqa: E402

ROOT = HERE.parent.parent
HAND = {"IFCommonTimer": "NDS_RELOC_ASSET_IF_COMMON_TIMER",
        "IFCommonDigits": "NDS_RELOC_ASSET_IF_COMMON_DIGITS",
        "IFCommonPlayerDamage": "NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE",
        "IFCommonPlayerTags": "NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS",
        "IFCommonBattlePause": "NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE",
        "IFCommonGameStatus": "NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS"}


def hand_rows(backend: str, asset: str) -> dict[int, tuple]:
    start = backend.find(stage.BACKEND_TABLE_START)
    end = backend.find("\n};", start)
    table = backend[start:end]
    out = {}
    for off, w, h, n, fmt, siz in re.findall(
            r"\{\s*" + re.escape(asset) + r",\s*(0x[0-9a-fA-F]+)u,\s*(\d+)u,\s*(\d+)u,"
            r"\s*(\d+)u,\s*(G_IM_FMT_\w+),\s*(G_IM_SIZ_\w+)\s*\}", table):
        out[int(off, 16)] = (int(w), int(h), int(n), fmt, siz)
    return out


def main() -> int:
    failures = 0
    for name, expected in (("SC1PStageClear1", "SC1P_STAGE_CLEAR1"),
                           ("IFCommonTimer", "IF_COMMON_TIMER"),
                           ("MNVSRecordMain", "MNVS_RECORD_MAIN"),
                           ("MN1PContinue", "MN1P_CONTINUE")):
        got = stage.macro_token(name)
        if got != expected:
            print(f"macro_token({name}) = {got}, expected {expected}")
            failures += 1
    backend = stage.read(ROOT, stage.BACKEND)
    for name, asset in HAND.items():
        plan = stage.Plan(ROOT, name)
        derived = {off: (w, h, n, stage.FMT[fmt], stage.SIZ[siz])
                   for _s, off, w, h, n, fmt, siz in plan.rows}
        hand = hand_rows(backend, asset)
        common = sorted(set(hand) & set(derived))
        bad = [off for off in common if hand[off] != derived[off]]
        for off in bad:
            print(f"{name} 0x{off:04x}: hand {hand[off]} derived {derived[off]}")
        failures += len(bad)
        print(f"{name}: {len(common)} compared, {len(bad)} mismatched, "
              f"{len(set(hand) - set(derived))} hand-only, "
              f"{len(set(derived) - set(hand))} derived-only")
    print("PASS" if failures == 0 else f"FAIL ({failures})")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
