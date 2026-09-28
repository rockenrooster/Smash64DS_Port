"""Lane 1: parse the P1 static texture corpus metadata (src/nds/generated/battle_playable_static_textures.generated.inc).
Records: (owner_mask, image_asset_id, tlut_asset_id, image_offset, tlut_offset, payload_bytes)."""
import re
import lane1_o2r as o

INC = o.REPO / "src/nds/generated/battle_playable_static_textures.generated.inc"

def records():
    s = INC.read_text()
    body = s.split("sNdsBattleStaticTextureRecords[", 1)[1]
    out = []
    # each record: { owner, image_asset, tlut_asset, reserved, image_off, tlut_off, payload_off, payload_bytes, ...
    for m in re.finditer(r"\{\s*0x([0-9a-f]+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*0x([0-9a-f]+)u,\s*0x([0-9a-f]+)u,\s*(\d+)u,\s*(\d+)u,", body):
        owner, img_a, tl_a, _res, img_o, tl_o, p_off, p_bytes = m.groups()
        out.append(dict(owner=int(owner, 16), image_asset=int(img_a), tlut_asset=int(tl_a),
                        image_off=int(img_o, 16), tlut_off=int(tl_o, 16), payload_off=int(p_off), payload_bytes=int(p_bytes)))
    return out

if __name__ == "__main__":
    r = records()
    print(len(r))
    import collections
    c = collections.Counter((x['image_asset']) for x in r)
    print(c)
    for x in r[:6]: print(x)
