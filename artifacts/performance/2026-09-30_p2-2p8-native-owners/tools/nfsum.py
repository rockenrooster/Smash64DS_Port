"""Native-failure lab table per arm: identity (GObj kind << 16 | asset id),
status, reason, count, first presented frame."""
import json
import sys

BASE = "D:/Stuff/DevFolder/Smash64DS_Port/artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/"
KINDS = {0: "?", 1: "Ground?", 3: "Fighter", 4: "Weapon?", 5: "Item?", 6: "Effect?"}
for arm in sys.argv[1].split(","):
    try:
        d = json.load(open(BASE + arm + ".json"))
    except OSError:
        print(f"{arm}: no json")
        continue
    ex = {e["name"]: e["value"] for e in d["extras"]}
    n = int(ex.get("gNdsNativeFailureLabCount") or 0)
    print(f"== {arm}: {n} rows, total {ex.get('gNdsRendererNativeFailure.count')}")
    for i in range(min(n, 16)):
        row = [int(ex.get(f"gNdsNativeFailureLab[{i}][{j}]") or 0) & 0xffffffff for j in range(5)]
        ident, status, reason, count, first = row
        print(f"   kind {ident >> 16:3d} asset {ident & 0xffff:5d} (0x{ident & 0xffff:04x})  root {status:#06x}  reason {reason}  count {count}  first f{first}")
