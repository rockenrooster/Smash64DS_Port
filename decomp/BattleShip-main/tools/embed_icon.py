#!/usr/bin/env python3
"""Embed a PNG file as a C++ uint8_t array header.

Usage: embed_icon.py <input.png> <output.h> <symbol_name>

Linux SDL doesn't pick up an app icon from filesystem paths the way
Windows reads the .ico from a .rc resource or macOS reads .icns from
the .app bundle. The only path is `SDL_SetWindowIcon()` on an
SDL_Surface at runtime, which means the PNG bytes have to be available
to the binary. Embedding via this header avoids any runtime
filesystem-layout coupling (dev tree vs AppImage vs install dir).
"""
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 1
    src, dst, symbol = sys.argv[1:]

    data = Path(src).read_bytes()
    rows = []
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        rows.append(", ".join(f"0x{b:02x}" for b in chunk))
    arr = ",\n    ".join(rows)

    header = f"""// Auto-generated from {Path(src).name} by tools/embed_icon.py.
// Do not edit by hand. See port/port_window_icon.cpp for the consumer.
#pragma once

#include <cstddef>
#include <cstdint>

inline constexpr std::size_t {symbol}_len = {len(data)}u;
inline constexpr std::uint8_t {symbol}[{len(data)}] = {{
    {arr}
}};
"""
    out = Path(dst)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(header)
    return 0


if __name__ == "__main__":
    sys.exit(main())
