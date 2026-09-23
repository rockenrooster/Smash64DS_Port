"""Slice 4 tools shim: the emitter lives in scripts/fighters."""
import sys
from pathlib import Path

_root = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(_root / 'scripts' / 'fighters'))
sys.path.insert(0, str(_root / 'scripts'))
from fighter_list_emitter import *  # noqa: E402,F401,F403
import fighter_list_emitter as _m  # noqa: E402
ROOT = _m.ROOT
