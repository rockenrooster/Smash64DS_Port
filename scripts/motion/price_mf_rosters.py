#!/usr/bin/env python3
"""Price all four-slot MFP2 roster banks, including mirrors, from an emitted pack.

This is a bank layout budget, not proof that the shipping arena can admit it.
Run check_mf_pack.py first to qualify source identities and decoded bytes.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

import mf_residency as mr
import check_mf_pack as fmt


def load_checked_pack(pack, check):
    data = pack.read_bytes()
    receipt = json.loads(check.read_text())
    sha = hashlib.sha256(data).hexdigest()
    if (receipt.get("pack_sha256") != sha or receipt.get("error_count") != 0 or
            receipt.get("passed", 0) != receipt.get("clips") or
            receipt.get("validator_sha256") != fmt.validator_fingerprint() or
            receipt.get("corpus_inputs_sha256") != mr.mc.corpus_cache_key()):
        raise ValueError("a current successful check for this exact pack is required")
    h = fmt.HEADER.unpack_from(data)
    if h[:3] != (fmt.PACK_MAGIC, fmt.PACK_VERSION, len(mr.mc.KINDS)):
        raise ValueError("not an MFP2 pack for the supported roster")
    rows = []
    for index in range(h[6]):
        e = fmt.ENTRY.unpack_from(data, h[5] + index * fmt.ENTRY.size)
        rows.append(dict(asset_id=e[0], stream_bits=e[2], decoded_bytes=e[3],
                         kind=e[5], cls=e[6], need_mask=e[7], main_user_mask=e[8]))
    raw = []
    for index in range(h[12]):
        e = fmt.RAW_ENTRY.unpack_from(data, h[11] + index * fmt.RAW_ENTRY.size)
        raw.append(dict(asset_id=e[0], payload_bytes=e[1], source_bytes=e[2],
                        main_user_mask=e[4], kind=e[5]))
    tables = data[h[3]:h[3] + h[4]]
    return rows, raw, len(tables), mr.mft1_arm_storage_bytes(tables), sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack", type=Path)
    parser.add_argument("--check", type=Path,
                        help="successful checker receipt (default: adjacent check.json)")
    parser.add_argument("--json", type=Path, required=True)
    args = parser.parse_args()
    check = args.check or args.pack.with_name("check.json")
    rows, raw, tables_bytes, expanded_bytes, sha = load_checked_pack(args.pack, check)

    def price(slots):
        return mr.resident_plan(rows, raw, slots, tables_bytes, expanded_bytes)

    prices = []
    worst = None
    for slots in itertools.combinations_with_replacement(mr.mc.KINDS, 4):
        plan = price(slots)
        prices.append({key: value for key, value in plan.items()
                       if key not in ("compressed_ids", "raw_ids")})
        if worst is None or plan["required_a7_bytes"] > worst["required_a7_bytes"]:
            worst = plan
    result = {
        "pack_sha256": sha,
        "check_sha256": hashlib.sha256(check.read_bytes()).hexdigest(),
        "scope": "MFP2 bank only; excludes existing fighter heaps and other scene resources",
        "layout": "16-aligned bank, packed 4-aligned MF structures, 16-aligned raw assets",
        "roster_count": len(prices),
        "canonical": price(("donkey", "samus", "link", "kirby")),
        "heavy_probe": price(("captain", "link", "pikachu", "kirby")),
        "worst": worst,
        "rosters": prices,
    }
    args.json.write_text(json.dumps(result, indent=2) + "\n")
    print("MF_ROSTERS=%d worst=%s bank=%d B canonical=%d B heavy=%d B" % (
        len(prices), ",".join(worst["slots"]), worst["required_a7_bytes"],
        result["canonical"]["required_a7_bytes"],
        result["heavy_probe"]["required_a7_bytes"],
    ))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
