import re, sys, os
root = r"D:/Stuff/DevFolder/Smash64DS_Port/src/nds"
sel = open(os.path.join(root, "nds_native_stage_select.inc")).read()

def packet_tail(name):
    m = re.search(r"sNdsNativeStagePacket%s = \{(.*?)\n\};" % name, sel, re.S)
    if not m: return None
    body = re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.S)
    toks = [t.strip() for t in body.replace("\n", " ").split(",")]
    toks = [t for t in toks if t]
    return toks

def parse_table(txt, ctype, pfx):
    m = re.search(r"static const %s sNdsNativeStage%s\w*?\[\d+\] = \{(.*?)\n\};" % (ctype, pfx), txt, re.S)
    if not m: return []
    rows = re.findall(r"\{([^{}]*)\}", m.group(1))
    out = []
    for r in rows:
        vals = [v.strip().rstrip('u').rstrip('U') for v in r.split(",") if v.strip()]
        out.append([int(v, 0) for v in vals])
    return out

stages = [("owner", "", "DreamLand"), ("castle", "Castle", "Castle"), ("hyrule", "Hyrule", "Hyrule"),
          ("inishie", "Inishie", "Inishie"), ("jungle", "Jungle", "Jungle"), ("sector", "Sector", "Sector"),
          ("yamabuki", "Yamabuki", "Yamabuki"), ("yoster", "Yoster", "Yoster"), ("zebes", "Zebes", "Zebes")]
CLS = {0: "raw", 3: "noZ", 6: "range"}
for fn, pfx, pkt in stages:
    txt = open(os.path.join(root, "nds_native_stage_%s.generated.inc" % fn)).read()
    runs = parse_table(txt, "NDSNativeStageRun", pfx + "Runs" if pfx else "Runs")
    verts = parse_table(txt, "NDSNativeStageDenseVertex", pfx + "Vertices" if pfx else "Vertices")
    corners_m = re.search(r"static const u16 sNdsNativeStage%sCorners\[\d+\] = \{(.*?)\};" % pfx, txt, re.S)
    corners = [int(v.strip().rstrip('u'), 0) for v in corners_m.group(1).split(",") if v.strip()]
    segs = parse_table(txt, "NDSNativeStageSegment", pfx + "Segments" if pfx else "Segments")
    tail = packet_tail(pkt)
    rigid = None; cam = None
    if tail:
        # find rigid and camera masks: tokens ending with ULL or 0u after cross counts
        hexes = [t for t in tail if re.match(r"^(0x[0-9a-fA-F]+|\d+)ULL$", t) or t.endswith("RIGID_BINDING_MASK") or t.endswith("CAMERA_BINDINGS")]
    # generic: locate by position: fields after cross_corner_count
    # count known tokens: find index of CROSS_MATRIX_FOREIGN_CORNER_COUNT
    idx = [i for i, t in enumerate(tail) if "CROSS_MATRIX_FOREIGN_CORNER_COUNT" in t][0]
    rigid_tok, cam_tok = tail[idx + 1], tail[idx + 2]
    def ev(t):
        if "DREAMLAND_RIGID" in t: return 0x00000381c00fffff
        if "DREAMLAND_CAMERA" in t: return 0x7ec38
        return int(t.replace("ULL", "").rstrip("u"), 0)
    rigid = ev(rigid_tok); cam = ev(cam_tok); rigid_raw = rigid; rigid = rigid & ~cam
    stat = {}
    per_seg = {}
    for ri, r in enumerate(runs):
        first_corner, tri, bind, epoch, cls, pol, flags = r
        cross = flags & 1
        seg = None
        for si, s in enumerate(segs):
            if s[7] <= ri < s[7] + s[8]:
                seg = si
        # binding rigidity: any corner vertex binding non-rigid?
        rig_run = all((rigid >> verts[corners[first_corner + k]][5]) & 1 for k in range(tri * 3))
        camrun = any((cam >> verts[corners[first_corner + k]][5]) & 1 for k in range(tri * 3)); key = (CLS.get(cls, cls), "cross" if cross else ("rigid" if rig_run else ("cam" if camrun else "dyn")))
        stat[key] = stat.get(key, 0) + tri
        per_seg.setdefault(seg, {}).setdefault(key, 0)
        per_seg[seg][key] += tri
    tot = sum(r[1] for r in runs)
    print("%-9s tris=%d runs=%d rigid=%#x cam=%#x  %s" % (fn, tot, len(runs), rigid, cam,
          " ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(stat.items()))))
    for si in sorted(per_seg, key=lambda x: -1 if x is None else x):
        s = segs[si] if si is not None else None
        print("     seg%s owner=%s link=%s bind=%s+%s runs=%s+%s: %s" % (si, s[2] if s else 0, s[3] if s else 0, s[4] if s else 0, s[5] if s else 0, s[7] if s else 0, s[8] if s else 0,
              " ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(per_seg[si].items()))))
