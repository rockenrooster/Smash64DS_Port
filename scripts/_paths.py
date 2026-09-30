"""Make every scripts/ area folder importable by bare module name.

The generators cross-import (`generate_nds_native_owners` imports
`generate_nds_native_stage`; the dreamland family imports both directions),
and they live in per-area folders (`fighters/`, `stages/`, `stages/dreamland/`,
`sfx/`, `2d_vfx/`) while some shared modules stay at the scripts root. Python
only puts the *executed* script's own directory on sys.path, so without this
shim every cross-folder import breaks the moment a file moves.

Usage, at the top of any script that imports a sibling from another folder:

    import sys
    from pathlib import Path
    _p = Path(__file__).resolve().parent
    while _p.name != "scripts":
        _p = _p.parent
    sys.path.insert(0, str(_p))
    import _paths  # noqa: F401

Scripts already at the scripts root only need the final `import _paths` line.
Keep this list in sync with the area folders in scripts/README.md.
"""
import sys
import os
import hashlib
import json
from pathlib import Path

_SCRIPTS_ROOT = Path(__file__).resolve().parent

# Canonical anchors for moved scripts. A file that computed the repo root as
# Path(__file__).resolve().parents[1] silently points somewhere else after a
# move; these do not.
SCRIPTS_ROOT = _SCRIPTS_ROOT
REPO_ROOT = _SCRIPTS_ROOT.parent


def battleship_o2r_root(repo_root: Path) -> Path:
    """Select the read-only O2R corpus without moving source or output roots.

    Make exports BATTLESHIP_O2R. An isolated checkout may explicitly point it
    at a qualified corpus in another checkout; no missing-input fallback is
    performed. Native Windows Python also accepts Make's MSYS /d/... spelling.
    Producer --repo-root continues to own source validation and all outputs.
    NDS_REFERENCE_ROOT is an optional read-only repository root for O2R,
    decompressed assets and ignored decomp/build inputs only; the explicit
    BATTLESHIP_O2R/BATTLESHIP_RELOCDATA roots take precedence.
    """
    value = os.environ.get("BATTLESHIP_O2R")
    if value:
        if (os.name == "nt" and len(value) >= 3 and value[0] == "/" and
                value[1].isalpha() and value[2] == "/"):
            value = value[1].upper() + ":" + value[2:]
        return Path(value).resolve()
    reference = os.environ.get("NDS_REFERENCE_ROOT")
    if reference:
        return _native_reference_root(reference) / "decomp/BattleShip-main/BattleShip_o2r"
    return (repo_root / "decomp/BattleShip-main/BattleShip_o2r").resolve()


def _native_reference_root(value: str) -> Path:
    if (os.name == "nt" and len(value) >= 3 and value[0] == "/" and
            value[1].isalpha() and value[2] == "/"):
        value = value[1].upper() + ":" + value[2:]
    return Path(value).resolve()


def battleship_input_path(repo_root: Path, relative: str | Path) -> Path:
    """Route assets and ignored decomp/build inputs; tracked source stays local."""
    path = Path(relative)
    # A lexical escape must not make an asset override select tracked source.
    if ".." in path.parts:
        return repo_root / path
    prefix = Path("decomp/BattleShip-main/BattleShip_o2r")
    if path.is_relative_to(prefix):
        return battleship_o2r_root(repo_root) / path.relative_to(prefix)
    prefix = Path("decomp/BattleShip-main/decomp/assets/us/relocData")
    if path.is_relative_to(prefix):
        return battleship_relocdata_root(repo_root) / path.relative_to(prefix)
    prefix = Path("decomp/BattleShip-main/decomp/build")
    if path.is_relative_to(prefix) and os.environ.get("NDS_REFERENCE_ROOT"):
        return _native_reference_root(os.environ["NDS_REFERENCE_ROOT"]) / path
    return repo_root / path


def battleship_relocdata_root(repo_root: Path) -> Path:
    """Honor Make's explicit read-only decompressed BATTLESHIP_RELOCDATA root."""
    value = os.environ.get("BATTLESHIP_RELOCDATA")
    if value:
        if (os.name == "nt" and len(value) >= 3 and value[0] == "/" and
                value[1].isalpha() and value[2] == "/"):
            value = value[1].upper() + ":" + value[2:]
        return Path(value).resolve()
    reference = os.environ.get("NDS_REFERENCE_ROOT")
    if reference:
        return (_native_reference_root(reference) /
                "decomp/BattleShip-main/decomp/assets/us/relocData")
    return (repo_root / "decomp/BattleShip-main/decomp/assets/us/relocData").resolve()


def record_reference_input(path: Path, payload: bytes) -> None:
    """Emit the exact read identity when an explicit corpus is selected.

    The caller passes bytes it has already read, so provenance neither rereads
    an input nor writes a receipt into the read-only reference checkout.
    """
    if (os.environ.get("BATTLESHIP_O2R") or os.environ.get("BATTLESHIP_RELOCDATA") or
            os.environ.get("NDS_REFERENCE_ROOT")):
        print("REFERENCE_INPUT " + json.dumps({
            "path": str(path.resolve()), "bytes": len(payload),
            "sha256": hashlib.sha256(payload).hexdigest(),
        }, sort_keys=True), file=sys.stderr)


_AREA_DIRS = (
    "",
    "fighters",
    "fighters/mario",
    "fighters/fox",
    "stages",
    "stages/dreamland",
    "sfx",
    "sfx/bgm",
    "sfx/items",
    "2d_vfx",
    "3d_vfx",
    "menus",
)

for _rel in _AREA_DIRS:
    _dir = str(_SCRIPTS_ROOT / _rel) if _rel else str(_SCRIPTS_ROOT)
    if _dir not in sys.path:
        sys.path.append(_dir)
