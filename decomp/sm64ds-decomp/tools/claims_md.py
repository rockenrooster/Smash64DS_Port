"""Read the active claims out of CLAIMS.md so schedulers stop handing out taken work.

WHY THIS EXISTS
---------------
CLAIMS.md is how contributors avoid grinding the same function twice, and it was the only
coordination the schedulers did NOT consult. `worklist.py` and `coddog.py` built batches
straight from the unmatched pool, so a function another contributor had marked **active**
went out to an agent anyway. That is not theoretical: a single ov002 batch shipped five
functions held by someone else, three of which came back as duplicate matches of work
already done.

The collision runs both ways, and this file only fixes one direction. It stops US taking
work THEY claimed. It does not announce what we took - that needs the console to write
rows back, which is a separate change.

Matching is by SYMBOL NAME and by ADDRESS, because rows are written by hand and spell
targets inconsistently:

    | ov098 func_ov098_0213b9d8 (0x0213b9d8, size 0x144) | ... | **active** | ...
    | arm9 Stage::InitResources (_ZN5Stage13InitResourcesEv @ 0x0202cc0c, ...) | ... | **active** |

Both the demangled name and the mangled symbol can appear, so an address hit is the
reliable signal and the name is the fallback.

Only rows whose status is **active** or **partial** are treated as held. done, released,
merged and closed are free.

Usage:
    import claims_md
    held = claims_md.held_targets()          # {"names": {...}, "addrs": {...}}
    if claims_md.is_held(held, name, addr):  # skip this target
"""
import pathlib
import re

REPO = pathlib.Path(__file__).resolve().parent.parent
CLAIMS = REPO / "CLAIMS.md"

# A row is held only while it is active/partial; done/released/merged/closed are free.
HELD_STATUS = re.compile(r"\*\*(active|partial)\*\*", re.I)
# Any 0x-prefixed 8-hex-digit address anywhere in the row.
ADDR = re.compile(r"0x([0-9a-fA-F]{6,8})")
# Symbol-shaped tokens: mangled (_Z...), func_<addr>, __sinit_..., or a Class::Method pair.
SYM = re.compile(r"(_Z[A-Za-z0-9_]+|__sinit_[A-Za-z0-9_]+|func_(?:ov\d+_)?[0-9a-fA-F]{6,8}|[A-Za-z_]\w*::[A-Za-z_]\w*)")


def held_targets(path=None):
    """Parse CLAIMS.md. Returns {"names": set[str], "addrs": set[int], "rows": int}."""
    p = pathlib.Path(path) if path else CLAIMS
    names, addrs, rows = set(), set(), 0
    if not p.exists():
        return {"names": names, "addrs": addrs, "rows": 0}
    for line in p.read_text(encoding="utf-8", errors="ignore").splitlines():
        s = line.strip()
        if not s.startswith("|") or "---" in s:
            continue
        if not HELD_STATUS.search(s):
            continue
        rows += 1
        for m in ADDR.finditer(s):
            addrs.add(int(m.group(1), 16))
        for m in SYM.finditer(s):
            names.add(m.group(1))
    return {"names": names, "addrs": addrs, "rows": rows}


def is_held(held, name=None, addr=None):
    """True if this target is claimed by someone. Address is the reliable signal."""
    if addr is not None:
        try:
            a = addr if isinstance(addr, int) else int(str(addr), 0)
        except (TypeError, ValueError):
            a = None
        if a is not None and a in held["addrs"]:
            return True
    if name and name in held["names"]:
        return True
    return False


if __name__ == "__main__":
    h = held_targets()
    print(f"CLAIMS.md: {h['rows']} held rows -> {len(h['addrs'])} addresses, {len(h['names'])} names")
    for a in sorted(h["addrs"])[:20]:
        print(f"   0x{a:08x}")
