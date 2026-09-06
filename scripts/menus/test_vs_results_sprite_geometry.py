#!/usr/bin/env python3
"""Live oracle for the 35 VS-results sprite geometry rows in
sNdsBattleInterfaceSpriteDescs (src/port/reloc_backend_assets.c).

The 28 IFCommonAnnounceCommon letter/symbol + 5 IFCommonPlayerTags + 2
MNPlayersGameModes sprites the audit's `vs_results` screen draws are staged
and offset-rowed, but without a Sprite geometry row the blanket u32 endian
pass leaves each header swapped and ndsSObjPreviewBasicSupported rejects it.
This test pins the live backend rows field-for-field against the read-only
authorities (reloc_data.us.h offsets x BattleShip_o2r Sprite records), and
asserts the audit's vs_results nodraw set is empty because the rows are live.

NATIVE-ROUTE RULING (no false positives)
----------------------------------------
src/port/reloc_backend_assets.c has ndsRelocNormalizeIFAnnounceMarioSprites,
covering A-Z + Period by offset. It is NOT an alternate route for this
screen: its single call site is the opening-room loader (beside
ndsRelocNormalizeVSResultsSprites/TitleFireSprites in that function), while
every load path funnels through ndsRelocFinalizeLoadedFile, whose sprite
pass is ndsRelocNormalizeBattleInterfaceSprites (manifest-only). The
shell-loop VS-results runtime never passes through the opening loader, so
all 28 announce symbols need manifest rows here; Exclaim (0x7d98) additionally
has no native coverage at all. PlayerTags/GameModes have no native
normalizer.

USAGE
  python scripts/menus/test_vs_results_sprite_geometry.py
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import stage_reloc_file as stage  # noqa: E402

ROOT = HERE.parent.parent
BACKEND = stage.BACKEND

ANNOUNCE = "NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE"
TAGS = "NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS"
MODES = "NDS_RELOC_ASSET_MN_PLAYERS_GAME_MODES"

# Symbol -> (asset, offset, w, h, nbitmaps, fmt, siz), decoded from the o2r
# containers with stage_reloc_file.Container (the bake's own RELO parser)
# and cross-checked against decomp reloc_data.us.h offsets.
EXPECTED: dict[str, tuple[str, int, int, int, int, str, str]] = {
    **{f"llIFCommonAnnounceCommonLetter{c}Sprite": (ANNOUNCE, o, w, h, 1,
        "G_IM_FMT_IA", "G_IM_SIZ_8b")
        for c, o, w, h in [
            ("A", 0x05e0, 35, 37), ("B", 0x09a8, 24, 36),
            ("C", 0x0d80, 24, 37), ("D", 0x1268, 28, 36),
            ("E", 0x1628, 22, 36), ("F", 0x1a00, 20, 37),
            ("G", 0x1f08, 31, 37), ("H", 0x2408, 27, 37),
            ("I", 0x26b8, 9, 37), ("J", 0x2a90, 20, 37),
            ("K", 0x2f98, 27, 37), ("L", 0x3358, 20, 36),
            ("M", 0x3980, 37, 37), ("N", 0x3e88, 29, 37),
            ("O", 0x44b0, 34, 37), ("P", 0x4890, 24, 37),
            ("Q", 0x4f10, 37, 39), ("R", 0x5418, 27, 37),
            ("S", 0x57f0, 24, 37), ("T", 0x5bd0, 24, 37),
            ("U", 0x60d8, 26, 37), ("V", 0x65d8, 28, 37),
            ("W", 0x6c00, 39, 37), ("X", 0x7108, 31, 37),
            ("Y", 0x7608, 29, 37), ("Z", 0x7ae8, 30, 36)]},
    "llIFCommonAnnounceCommonSymbolExclaimSprite":
        (ANNOUNCE, 0x7d98, 10, 37, 1, "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "llIFCommonAnnounceCommonSymbolPeriodSprite":
        (ANNOUNCE, 0x7e50, 8, 11, 1, "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "llIFCommonPlayerTags1PSprite": (TAGS, 0x0258, 19, 24, 1,
        "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "llIFCommonPlayerTags2PSprite": (TAGS, 0x04f8, 20, 24, 1,
        "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "llIFCommonPlayerTags3PSprite": (TAGS, 0x0798, 20, 24, 1,
        "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "llIFCommonPlayerTags4PSprite": (TAGS, 0x0a38, 21, 24, 1,
        "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "llIFCommonPlayerTagsCPSprite": (TAGS, 0x0cd8, 20, 24, 1,
        "G_IM_FMT_IA", "G_IM_SIZ_8b"),
    "llMNPlayersGameModesFreeForAllTextSprite": (MODES, 0x0280, 112, 11, 1,
        "G_IM_FMT_I", "G_IM_SIZ_4b"),
    "llMNPlayersGameModesTeamBattleTextSprite": (MODES, 0x04e0, 110, 9, 1,
        "G_IM_FMT_I", "G_IM_SIZ_4b"),
}

ROW_RE = re.compile(
    r"\{\s*(NDS_RELOC_ASSET_\w+),\s*(0x[0-9a-fA-F]+)u,\s*(\d+)u,\s*(\d+)u,"
    r"\s*(\d+)u,\s*(G_IM_FMT_\w+),\s*(G_IM_SIZ_\w+)\s*\}")


def fail(msg: str, failures: list[str]) -> None:
    failures.append(msg)
    print(f"FAIL: {msg}")


def main() -> int:
    failures: list[str] = []
    if len(EXPECTED) != 35:
        fail(f"EXPECTED has {len(EXPECTED)} entries, want 35", failures)

    # 1. Every EXPECTED row matches the o2r decode and the decomp offset, has
    #    an offset row in the header, and is LIVE in the backend with exact
    #    source container dimensions/format.
    file_ids, symbols = stage.parse_decomp(ROOT)
    backend = stage.read(ROOT, BACKEND)
    header = stage.read(ROOT, stage.HEADER)
    start = backend.find(stage.BACKEND_TABLE_START)
    table = backend[start:backend.find("\n};", start)]
    live = {}
    for g in ROW_RE.finditer(table):
        live[(g.group(1), int(g.group(2), 16))] = (
            int(g.group(3)), int(g.group(4)), int(g.group(5)),
            g.group(6), g.group(7))
    derived: dict[str, tuple] = {}
    for name in ("IFCommonAnnounceCommon", "IFCommonPlayerTags",
                 "MNPlayersGameModes"):
        plan = stage.Plan(ROOT, name)
        # IFCommonAnnounceCommon was hand-staged under the short asset name
        # NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE (backend :46, token line :4077),
        # while macro_token would coin ..._ANNOUNCE_COMMON. The hand name is
        # the authority the backend must use.
        asset = (stage.hand_asset_name(backend, plan.file_id)
                 or plan.asset)
        for sym, off, w, h, n, fmt, siz in plan.rows:
            derived[sym] = (asset, off, w, h, n,
                            stage.FMT[fmt], stage.SIZ[siz])
    for sym, exp in EXPECTED.items():
        if derived.get(sym) != exp:
            fail(f"{sym}: EXPECTED {exp} != o2r decode {derived.get(sym)}",
                 failures)
        if symbols.get(sym) != exp[1]:
            fail(f"{sym}: EXPECTED offset 0x{exp[1]:04x} != "
                 f"reloc_data.us.h {symbols.get(sym)}", failures)
        if re.search(r"\b" + sym + r"\b", header) is None:
            fail(f"{sym}: no offset row in {stage.HEADER}", failures)
        got = live.get((exp[0], exp[1]))
        if got != exp[2:]:
            fail(f"{sym}: backend row {got} != source {exp[2:]}", failures)

    # 2. Coverage landed: the audit's vs_results nodraw set is empty.
    sys.path.insert(0, str(HERE))
    import audit_mn_screen_coverage as audit  # noqa: E402
    _deltas, _allow, report = audit.run_audit(ROOT)
    nodraw = report["screens"]["vs_results"]["nodraw"]
    if nodraw:
        fail(f"vs_results still NODRAW ({len(nodraw)}): {nodraw[:5]}",
             failures)

    # 3. Native-route guard: the announce normalizer stays a single
    #    opening-loader call site, outside the Finalize funnel.
    calls = [i for i, l in enumerate(backend.split("\n"))
             if "ndsRelocNormalizeIFAnnounceMarioSprites(" in l
             and "static void" not in l]
    fin = backend.find("static s32 ndsRelocFinalizeLoadedFile")
    fin_end = backend.find("\n}\n", backend.find(
        "ndsRelocNormalizeBattleInterfaceSprites(loaded) == FALSE"))
    fin_body = backend[fin:fin_end]
    if len(calls) != 1:  # 1 definition (filtered) + this 1 call site
        fail(f"announce normalizer call sites changed ({len(calls)} lines "
             f"mention it); re-verify the native-route ruling", failures)
    if "ndsRelocNormalizeIFAnnounceMarioSprites" in fin_body:
        fail("announce normalizer entered ndsRelocFinalizeLoadedFile; the "
             "NODRAW set may now draw without manifest rows", failures)

    print("PASS" if not failures else f"FAIL ({len(failures)})")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
