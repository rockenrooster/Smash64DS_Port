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
from pathlib import Path

_SCRIPTS_ROOT = Path(__file__).resolve().parent

# Canonical anchors for moved scripts. A file that computed the repo root as
# Path(__file__).resolve().parents[1] silently points somewhere else after a
# move; these do not.
SCRIPTS_ROOT = _SCRIPTS_ROOT
REPO_ROOT = _SCRIPTS_ROOT.parent
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
