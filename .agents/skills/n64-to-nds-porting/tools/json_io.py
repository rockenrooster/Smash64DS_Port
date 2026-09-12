"""Strict JSON and replace-after-success output for these host teaching tools.

Atomic replacement is a host-filesystem operation, not a DS save/durability claim.
"""
from __future__ import annotations
import json
import os
from pathlib import Path
import tempfile
from typing import Any

MAX_INPUT_BYTES = 8 * 1024 * 1024


def _unique(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result: raise ValueError(f"duplicate JSON key: {key!r}")
        result[key] = value
    return result


def _nonfinite(token: str) -> Any:
    raise ValueError(f"nonfinite JSON number: {token}")


def _finite_float(token: str) -> float:
    import math
    value = float(token)
    if not math.isfinite(value): return _nonfinite(token)
    return value


def read_json(path: Path) -> Any:
    # Bound the actual read, not merely a racy stat before an unbounded read.
    with path.open("rb") as source:
        raw = source.read(MAX_INPUT_BYTES + 1)
    if len(raw) > MAX_INPUT_BYTES: raise ValueError("input exceeds the 8 MiB teaching-tool limit")
    return json.loads(raw.decode("utf-8"), object_pairs_hook=_unique,
                      parse_constant=_nonfinite, parse_float=_finite_float)


def write_json_atomic(path: Path, result: Any) -> None:
    text = json.dumps(result, indent=2, sort_keys=True, allow_nan=False) + "\n"
    temporary: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n",
                                         dir=path.parent, prefix=f".{path.name}.",
                                         suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(text)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
