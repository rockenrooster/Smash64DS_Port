#!/usr/bin/env python3
"""Lane 2: the owner code that assumes an UNCOMPACTED file layout.

A reclaim that shrinks a file (span-mapped image or pack) keeps `lbRelocGetFileData`
/ `ndsRelocGetFileData` call sites working (the token resolver maps through the
IF span table or the FPC pack: src/port/reloc_backend_assets.c:16332), but three
kinds of native-owner code read the layout directly:

  raw_pointer_word   `dl[i].words.w1 == base + NDS_NATIVE_*_OFFSET` (the DL word
                     holds a re-seated pointer after compaction; the constant is
                     a source offset)
  raw_size           `loaded->data_size >= NDS_NATIVE_*_FILE_END` or
                     `file_bytes < NDS_NATIVE_*_FILE_END` (loaded size becomes the
                     packed size; the constant is a source extent)
  raw_address        `address == effect_base + 0x...`, `(dl - base) == 0x...`
                     (root identity by raw arithmetic instead of
                     ndsRelocNativeRootOffset's cell decode)
  raw_texel          `asset_base + offset` / `base + NDS_NATIVE_*_OFFSET` handed to
                     a texture prepare/bind (must go through
                     ndsRelocNativeAssetAddress)

This script only COUNTS lines by pattern in the files that consume assets 83-86
(the whole-tree consumer census is in lane2_all_output.txt: assets 83/84/85/86
appear as owner_asset_id / NDS_NATIVE_*_ASSET only in the files below).
"""
from __future__ import annotations

import json
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent

STAGE = REPO / "src/port/renderer_adapter_stage.c"
NATIVE_COMMON = REPO / "src/nds/nds_renderer_native_common.c"
def _asset86_exec():
    """exec.inc files of the owners whose generated header says `_ASSET 86u`."""
    out = []
    for h in sorted((REPO / "include/nds/generated").glob("nds_native_*.generated.h")):
        t = h.read_text(encoding="utf-8", errors="replace")
        if re.search(r"#define NDS_NATIVE_\w+_ASSET 86u", t):
            stem = h.name.replace(".generated.h", "")
            out.append(REPO / "src/nds" / (stem + ".exec.inc"))
    return out


ITEM_EXEC = _asset86_exec()
DMG_EXEC = [REPO / "src/nds/nds_native_damage_slash.exec.inc", REPO / "src/nds/nds_native_damage_fly_mdust.exec.inc"]


def count(path, rx):
    if not path.exists():
        return 0
    rx = re.compile(rx)
    return sum(1 for ln in path.read_text(encoding="utf-8", errors="replace").splitlines() if rx.search(ln))


def main():
    out = {}
    out["stage_item_root_identity_checks"] = count(STAGE, r"ndsRelocNativeRootOffset\(loaded, dl\) == NDS_NATIVE_(ITEM|CASTLE_BUMPER)_")
    out["stage_item_raw_pointer_word_lines"] = count(STAGE, r"\+ NDS_NATIVE_ITEM_[A-Z0-9_]*_OFFSET\)")
    out["stage_item_raw_size_lines"] = count(STAGE, r"data_size >= *\(?NDS_NATIVE_ITEM_[A-Z0-9_]*(FILE_END|_DL_BYTES)")
    out["stage_item_raw_size_lines"] += count(STAGE, r"data_size >= \(NDS_NATIVE_ITEM_")
    out["stage_entry_effect_raw_address_lines"] = count(STAGE, r"address == effect_base \+ 0x[0-9a-f]+u")
    out["stage_ef3_raw_root_lines"] = (count(STAGE, r"\(root_offset == 0x(0440|0518|2ef0|2f80|3010|30a0)u\)") +
                                       count(STAGE, r"rebirth_offset == 0x(2378|2a88|27e8)u"))
    out["stage_slash_raw_pointer_word_lines"] = count(STAGE, r"slash_base \+ (palette|vertex)_offset")
    out["stage_loaded_data_size_uses_total"] = count(STAGE, r"loaded->data_size")
    out["stage_ndsRelocNativeSourceSize_uses"] = count(STAGE, r"ndsRelocNativeSourceSize")
    out["native_common_entry_effect_owner_asset_lines"] = count(NATIVE_COMMON, r"owner_asset_id == (84|85)u")
    out["item_exec_files"] = sum(1 for p in ITEM_EXEC if p.exists())
    out["item_exec_raw_size_lines"] = sum(count(p, r"file_bytes < NDS_NATIVE_") for p in ITEM_EXEC)
    out["item_exec_raw_texel_lines"] = sum(count(p, r"(tlut|image|palette|texel)[a-z0-9_]* = base \+ NDS_NATIVE_") for p in ITEM_EXEC)
    out["damage_exec_raw_texel_lines"] = sum(count(p, r"(asset_base \+ offset|asset_view \+ offset)") for p in DMG_EXEC)
    (HERE / "lane2_sites.json").write_text(json.dumps(out, indent=1))
    for k, v in out.items():
        print(f"{k:48s} {v}")


if __name__ == "__main__":
    main()
