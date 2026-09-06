"""Small C-source extraction helpers shared by host regression tests."""
import re
from pathlib import Path


DECOMP = Path(__file__).resolve().parents[2] / "decomp/BattleShip-main/decomp/src"


def clean(source):
    return re.sub(r"/\*.*?\*/|//[^\n]*", lambda m: " " * len(m[0]),
                  source, flags=re.S)


def braced(source, pattern, semicolon=False):
    plain = clean(source)
    start = re.search(pattern, plain, re.M)
    if start is None:
        raise AssertionError(f"Missing source declaration: {pattern}")
    opening = plain.index("{", start.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (plain[end] == "{") - (plain[end] == "}")
        end += 1
    if semicolon:
        end = plain.index(";", end) + 1
    return source[start.start():end]


def function(source, name):
    return braced(source, rf"^[\w *]+\b{name}\([^;]*?\)\s*\{{")


def original_enum(path, name):
    return braced((DECOMP / path).read_text(), rf"typedef enum {name}\s*\{{", True)
