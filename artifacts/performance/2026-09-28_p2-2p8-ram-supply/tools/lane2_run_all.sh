#!/usr/bin/env bash
# Lane 2 reproduction: read-only over the repo; writes only into this folder.
# Usage (repo root):  bash artifacts/performance/2026-09-28_p2-2p8-ram-supply/tools/lane2_run_all.sh
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../../../.." && pwd)"
cd "$here"
log="$here/lane2_all_output.txt"
: > "$log"
run() { echo "### $*" | tee -a "$log"; "$@" 2>&1 | tee -a "$log"; echo | tee -a "$log"; }
# item closure (existing script, output redirected into this folder; the packed files are not needed)
( cd "$repo" && python scripts/items/item_memory_closure.py --out "artifacts/performance/2026-09-28_p2-2p8-ram-supply/tools/item-closure-run" ) > "$here/item-closure-run.stdout" 2>&1 || true
rm -f "$here/item-closure-run/compact86.bin" "$here/item-closure-run/compact86.reloc" "$here/item-closure-run/compact251.reloc"
run python lane2_struct_sizes.py
run python lane2_inventory.py
run python lane2_sprites.py IFCommonPlayerDamage IFCommonTimer IFCommonDigits IFCommonBattlePause IFCommonPlayerTags IFCommonAnnounceCommon IFCommonGameStatus IFCommonItem
python -c "import lane2_sprites as s, json; print(json.dumps(s.gamestatus_compact(), indent=1)); print(s.write_counts())" | tee -a "$log"
run python lane2_ifcommon.py
run python lane2_items_offsets.py
run python lane2_items.py
run python lane2_partition.py
run python lane2_reach.py
run python lane2_effects.py
run python lane2_roots.py
run python lane2_prefix.py
run python lane2_static_keys.py
run python lane2_xrefs.py
run python lane2_structural.py
run python lane2_nm_check.py
run python lane2_4c_check.py
run python lane2_sites.py
run python lane2_pools.py
run python lane2_estimate.py
run python lane2_tables.py
run python lane2_cites.py
run python lane2_perfile.py
rm -rf "$here/__pycache__"
