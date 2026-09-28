#!/usr/bin/env python3
"""Lane 2: identity-only keys the static battle texture corpus holds INTO the
files under study.  The corpus builds each texture key from the resident file's
addresses (ndsRendererHardwareBuildBattleStaticTextureKey,
src/nds/nds_renderer_textures_effects.c:5960-6065: "This builds identity only;
upload reads the offline DS payload"), so these offsets must stay addressable
(and inside data_size) but their BYTES are not read.
"""
import json
import re
from collections import defaultdict
from pathlib import Path

import lane2_o2r as o2r

INC = o2r.REPO / "src/nds/generated/battle_playable_static_textures.generated.inc"
ASSETS = {82: "IFCommonGameStatus", 83: "EFCommonEffects1", 84: "EFCommonEffects2",
          85: "EFCommonEffects3", 86: "MiscData086", 87: "IFCommonItem", 251: "ITCommonData",
          164: "IFCommonPlayerDamage", 165: "IFCommonTimer", 36: "IFCommonDigits",
          197: "IFCommonBattlePause", 38: "IFCommonPlayerTags", 37: "IFCommonAnnounceCommon",
          166: "IFCommonPlayer"}


def main():
    t = INC.read_text()
    rec = re.compile(r"\{\s*0x([0-9a-f]+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*0x([0-9a-f]+)u,\s*0x([0-9a-f]+)u,", re.S)
    per = defaultdict(list)
    n = 0
    for m in rec.finditer(t):
        n += 1
        owner, img_asset, tlut_asset, _res, img_off, tlut_off = m.groups()
        img_asset, tlut_asset = int(img_asset), int(tlut_asset)
        if img_asset in ASSETS:
            per[ASSETS[img_asset]].append(("image", hex(int(img_off, 16))))
        if tlut_asset in ASSETS:
            per[ASSETS[tlut_asset]].append(("tlut", hex(int(tlut_off, 16))))
    print("records parsed:", n)
    for k, v in sorted(per.items()):
        print(k, sorted(set(v)))
    Path(__file__).with_name("lane2_static_keys.json").write_text(json.dumps(per, indent=1))


if __name__ == "__main__":
    main()
