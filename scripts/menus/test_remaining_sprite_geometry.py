#!/usr/bin/env python3
"""Live oracle for the menu-audit sprite geometry rows OUTSIDE vs_results in
sNdsBattleInterfaceSpriteDescs (src/port/reloc_backend_assets.c).

scripts/menus/audit_mn_screen_coverage.py reported NODRAW sprites on ten
imported screens (1p_mode_select, 1p_continue, 1p_css, 1p_bonus_css,
training_css, options, sound_test, data_menu, vs_record, unlock_message):
staged, offset-rowed, but with no Sprite geometry row, so the blanket u32
endian pass left each header swapped and ndsSObjPreviewBasicSupported
rejected it. The vs_results 35 (IFCommonAnnounceCommon letters/symbols,
IFCommonPlayerTags, MNPlayersGameModes labels) live under
test_vs_results_sprite_geometry.py; 7 of them also appear on 1p_continue.

SCOPE
-----
This test checks the LIVE backend, not a patch file: every backend geometry
row for these 8 containers must decode field-for-field from the source o2r
container at that offset with a reloc_data.us.h symbol owning that offset,
and the audit's non-vs_results nodraw sets must be empty because the rows
are live. Containers decode more sprites than any audited screen draws
(digits, unchosen CSS variants), so the test validates landed rows rather
than demanding rows for every decoded sprite.

NATIVE-ROUTE RULING (no false positives, no staged==drawable)
-------------------------------------------------------------
Staged + rowed is NOT drawable -- only a manifest row or a native normalizer
counts. For these 8 containers there is no normalizer: each asset name
appears in reloc_backend_assets.c only in its #define, its token->asset line,
its ledger case, and manifest rows -- never inside an
ndsRelocNormalize* body. The test asserts exactly that allowlist of mention
kinds, so a future native route fails loudly instead of silently
double-covering.

USAGE
  python scripts/menus/test_remaining_sprite_geometry.py
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
ANNOUNCE_ASSET = "NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE"

# The 7 1p_continue NODRAW letters live in the vs_results 35-row set
# (A/E/G/M/O/R/V), owned by test_vs_results_sprite_geometry.py.
PRIOR_SYMBOLS = {
    "llIFCommonAnnounceCommonLetterASprite",
    "llIFCommonAnnounceCommonLetterESprite",
    "llIFCommonAnnounceCommonLetterGSprite",
    "llIFCommonAnnounceCommonLetterMSprite",
    "llIFCommonAnnounceCommonLetterOSprite",
    "llIFCommonAnnounceCommonLetterRSprite",
    "llIFCommonAnnounceCommonLetterVSprite",
}

# Container -> hand asset name (backend #define authority, not macro_token:
# IFCommonAnnounceCommon taught us the hand short name wins). File ids are
# asserted against the decomp symbol table at runtime.
CONTAINERS = (
    ("FTEmblemSprites", "NDS_RELOC_ASSET_FT_EMBLEM_SPRITES"),
    ("FTStocksZako", "NDS_RELOC_ASSET_FT_STOCKS_ZAKO"),
    ("IFCommonBattlePause", "NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE"),
    ("MNCommon", "NDS_RELOC_ASSET_MN_COMMON"),
    ("MNCommonFonts", "NDS_RELOC_ASSET_MN_COMMON_FONTS"),
    ("MNPlayersCommon", "NDS_RELOC_ASSET_MN_PLAYERS_COMMON"),
    ("MNPlayersPortraits", "NDS_RELOC_ASSET_MN_PLAYERS_PORTRAITS"),
    ("MNSelectCommon", "NDS_RELOC_ASSET_MN_SELECT_COMMON"),
)

ROW_RE = re.compile(
    r"\{\s*(NDS_RELOC_ASSET_\w+),\s*(0x[0-9a-fA-F]+)u,\s*(\d+)u,\s*(\d+)u,"
    r"\s*(\d+)u,\s*(G_IM_FMT_\w+),\s*(G_IM_SIZ_\w+)\s*\}")


def fail(msg: str, failures: list[str]) -> None:
    failures.append(msg)
    print(f"FAIL: {msg}")


def main() -> int:
    failures: list[str] = []

    # 1. Every live backend row for these containers decodes field-for-field
    #    from the source o2r container, and its offset is owned by a
    #    reloc_data.us.h symbol of that container.
    file_ids, symbols = stage.parse_decomp(ROOT)
    backend = stage.read(ROOT, BACKEND)
    start = backend.find(stage.BACKEND_TABLE_START)
    table = backend[start:backend.find("\n};", start)]
    live: dict[tuple[str, int], tuple] = {}
    for g in ROW_RE.finditer(table):
        live[(g.group(1), int(g.group(2), 16))] = (
            int(g.group(3)), int(g.group(4)), int(g.group(5)),
            g.group(6), g.group(7))
    off_owner: dict[int, str] = {}
    for sym, off in symbols.items():
        if sym.endswith("FileID") or not sym.startswith("ll"):
            continue
        owners = [f for f in file_ids if sym.startswith("ll" + f)]
        if owners:
            off_owner.setdefault(off, max(owners, key=len))
    checked = 0
    for container, asset in CONTAINERS:
        if container == "MNSelectCommon":
            # stage.Plan(MNSelectCommon) refuses: the decomp table carries a
            # stale llMNSelectCommonPlayersSpotlightDObjDesc (0x568) past the
            # container payload (0x490). That entry is dead (not in any
            # screen inventory); the one live sprite decodes directly.
            reloc = stage.Container(stage.find_container(ROOT, container))
            if reloc.file_id != file_ids[container]:
                fail("MNSelectCommon container id drift", failures)
            sprite = reloc.sprite(0x0440)
            if not (sprite.width == 64 and sprite.height == 32
                    and sprite.nbitmaps == 1 and sprite.bmfmt == 2
                    and sprite.bmsiz == 0
                    and reloc.s16(0x0440 + 42) == 36):
                fail("MNSelectCommon stone decode drift", failures)
            want = (64, 32, 1, "G_IM_FMT_CI", "G_IM_SIZ_4b")
            if live.get((asset, 0x0440)) != want:
                fail(f"MNSelectCommon stone backend {live.get((asset, 0x0440))} "
                     f"!= source {want}", failures)
            else:
                checked += 1
            continue
        plan = stage.Plan(ROOT, container)
        if plan.file_id != file_ids[container]:
            fail(f"{container}: container id 0x{plan.file_id:x} != table "
                 f"0x{file_ids[container]:x}", failures)
        hand = stage.hand_asset_name(backend, plan.file_id) or plan.asset
        if hand != asset:
            fail(f"{container}: hand asset {hand} != {asset}", failures)
        if plan.bad_displist:
            fail(f"{container}: bad displist {plan.bad_displist}", failures)
        rows = [r for r in plan.rows if r[0] not in PRIOR_SYMBOLS]
        landed = [(sym, off) for sym, off, _w, _h, _n, _f, _s in rows
                  if (asset, off) in live]
        if not landed:
            fail(f"{container}: no live backend rows at all", failures)
        for sym, off, w, h, n, fmt, siz in rows:
            got = live.get((asset, off))
            if got is None:
                continue  # decoded but no audited screen draws it; not owed
            want = (w, h, n, stage.FMT[fmt], stage.SIZ[siz])
            if got != want:
                fail(f"{sym}: backend {got} != o2r decode {want}", failures)
            else:
                checked += 1
            if symbols.get(sym) != off:
                fail(f"{sym}: offset 0x{off:04x} != reloc_data.us.h "
                     f"{symbols.get(sym)}", failures)
    print(f"live rows validated against o2r decode: {checked}")

    # 2. Coverage landed: no NODRAW left outside vs_results, and the 7
    #    prior-scope letters draw through their own manifest rows.
    sys.path.insert(0, str(HERE))
    import audit_mn_screen_coverage as audit  # noqa: E402
    _deltas, _allow, report = audit.run_audit(ROOT)
    leftover: dict[str, list[str]] = {}
    for key, info in report["screens"].items():
        if info.get("kind") != "imported" or key == "vs_results":
            continue
        if info.get("nodraw"):
            leftover[key] = info["nodraw"]
    if leftover:
        fail(f"NODRAW remains outside vs_results: "
             f"{ {k: v[:3] for k, v in leftover.items()} }", failures)
    for sym in sorted(PRIOR_SYMBOLS):
        off = symbols.get(sym)
        if (ANNOUNCE_ASSET, off) not in live and off is not None:
            fail(f"prior-scope {sym} not drawable", failures)

    # 3. Native-route guard: these assets occur only as #define, token line,
    #    ledger case, or manifest row -- never in an ndsRelocNormalize* body.
    lines = backend.split("\n")
    in_normalize = [False] * len(lines)
    for i, line in enumerate(lines):
        if re.match(r".*\bndsRelocNormalize\w+\(.*\)\s*$", line) and \
                not line.strip().startswith("if"):
            depth = 0
            for j in range(i, len(lines)):
                depth += lines[j].count("{") - lines[j].count("}")
                if j > i:
                    in_normalize[j] = depth > 0 or lines[j].strip() == "}"
                if depth == 0 and j > i and "{" in "".join(lines[i:j + 1]):
                    break
    for _container, asset in CONTAINERS:
        mention = re.compile(r"\b" + asset + r"\b")
        for i, line in enumerate(lines):
            if mention.search(line) is None:
                continue
            ok_kind = (line.strip().startswith("#define")
                       or "ndsRelocFileID(&ll" in line
                       or line.strip().startswith("case ")
                       or ROW_RE.search(line) is not None
                       or re.search(r"\{\s*" + asset + r",", line) is not None
                       or "sNdsMNCommonSymbols" in line
                       or "loaded->asset_id" in line
                       or "/*" in line)
            if not ok_kind:
                fail(f"{asset}:{i + 1} unexpected mention: "
                     f"{line.strip()[:80]}", failures)
            if in_normalize[i]:
                fail(f"{asset}:{i + 1} mentioned inside a normalizer",
                     failures)

    print("PASS" if not failures else f"FAIL ({len(failures)})")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
