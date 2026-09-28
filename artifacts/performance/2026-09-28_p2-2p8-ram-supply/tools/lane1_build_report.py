#!/usr/bin/env python3
"""Lane 1: assemble ../lane1-stage-ground-files.md from lane1_report_template.md.

Placeholders
  @@SUMMARY@@ @@TABLE1@@ .. @@TABLE5@@ @@APPENDIX@@   tables rendered from ../lane1_data.json
  @@XC_REACHED@@ @@XC_AGREE@@ @@XC_PCT@@ @@XC_DIS@@    walker vs tiling cross-check
  {{L:name}}   `file:line`  (first hit)   {{LL:name}}  bare line number   -- from ../lane1_citations.json
"""
import json
import re
from pathlib import Path

import lane1_render_tables as rt

HERE = Path(__file__).resolve().parent
OUT = HERE.parent
tpl = (HERE / "lane1_report_template.md").read_text(encoding="utf-8")
cit = json.loads((OUT / "lane1_citations.json").read_text())


def L(m):
    name = m.group(1)
    r = cit[name]
    return f"`{r['file']}:{r['lines'][0]}`"


def L2(m):
    r = cit[m.group(1)]
    i = 1 if len(r["lines"]) > 1 else 0
    return f"`{r['file']}:{r['lines'][i]}`"


def LL(m):
    return str(cit[m.group(1)]["lines"][0])


x, dis = rt.xcheck()
S = rt.S
gv = [c["class_total"].get("gfx", 0) + c["class_total"].get("vtx", 0) for c in S]
dl = [c for c in S if c["label"] == "Dream Land"][0]["class_total"]
dl_render = dl["gfx"] + dl["vtx"] + dl["tex"] + dl["pal"]
rep = {
    "@@GV_MIN@@": f"{min(gv) / 1000.0:.1f}",
    "@@GV_MAX@@": f"{max(gv) / 1000.0:.1f}",
    "@@DL_RENDER@@": f"{dl_render:,}",
    "@@SUMMARY@@": rt.summary(),
    "@@TABLE1@@": rt.table1(),
    "@@TABLE2@@": rt.table2(),
    "@@TABLE3@@": rt.table3(),
    "@@TABLE4@@": "### Wallpaper share and what T1 leaves\n\n" + rt.table4(),
    "@@TABLE5@@": rt.table5(),
    "@@APPENDIX@@": rt.appendix(),
    "@@XC_REACHED@@": f"{x['reached']:,}",
    "@@XC_AGREE@@": f"{x['agree']:,}",
    "@@XC_PCT@@": f"{100.0 * x['agree'] / x['reached']:.2f}",
    "@@XC_DIS@@": dis,
}
text = tpl
for k, v in rep.items():
    text = text.replace(k, v)
text = re.sub(r"\{\{LL:([A-Za-z0-9_]+)\}\}", LL, text)
text = re.sub(r"\{\{L2:([A-Za-z0-9_]+)\}\}", L2, text)
text = re.sub(r"\{\{L:([A-Za-z0-9_]+)\}\}", L, text)
left = re.findall(r"@@[A-Z0-9_]+@@|\{\{[^}]*\}\}", text)
assert not left, left
(OUT / "lane1-stage-ground-files.md").write_text(text, encoding="utf-8")
print("wrote", OUT / "lane1-stage-ground-files.md", len(text), "chars")
