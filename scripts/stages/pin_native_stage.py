"""Auto-pin a native stage descriptor from the generator's own falsifiers.

    python scripts/stages/pin_native_stage.py <stage> [--max 14]

Written 2026-09-05 for the 25 bonus boards: each board copies the exemplar
descriptor and then needs a dozen values only the generator can name; agents
spent twenty minutes a board reading falsifiers by hand. Register the stage in
native_stage_descriptors/__init__.py, set its inputs (paths, file ids) and
include_sha="TO_BE_FILLED", then run this.

Runs scripts/stages/generate_nds_native_stage.py for the stage; each time it
falsifies with an "actual != expected" message for a pinnable field, writes
the actual value into the descriptor file and runs again, until it passes or
a falsifier is not pinnable (reported verbatim).  Then runs the checker.
Pinnable: input SHA256 / payload SHA256 / file id / fixup counts, live DObjs,
selected bindings, MODIFYVTX, packet counts, submit classes, segment
partition, state packet counts, cross-matrix counts, include SHA256.
"""
import ast
import hashlib
import os
import re
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(REPO, "scripts", "stages"))
import generate_nds_native_stage as G  # noqa: E402
from pathlib import Path  # noqa: E402

stage = sys.argv[1]
max_iter = int(sys.argv[sys.argv.index("--max") + 1]) if "--max" in sys.argv else 14
DESC = os.path.join(REPO, "scripts", "stages", "native_stage_descriptors", stage + ".py")

COUNT_KEYS = ["callbacks", "dobjs", "bindings", "commands", "vertex_commands",
              "source_vertices", "triangle_commands", "triangles", "runs",
              "texture_epochs", "material_events"]


def read():
    return open(DESC, encoding="utf-8", newline="").read()


def write(s):
    open(DESC, "w", encoding="utf-8", newline="").write(s)


def set_count(s, key, value):
    pat = re.compile(r'("%s":\s*)([-\d]+|\([^)]*\))' % re.escape(key))
    assert pat.search(s), key
    return pat.sub(lambda m: m.group(1) + str(value), s, count=1)


def set_field(s, field, value_repr):
    """Replace a top-level `field=<literal>,` in the StageDescriptor call."""
    m = re.search(r'^(    %s=)' % re.escape(field), s, re.M)
    assert m, field
    start = m.end()
    # find the end of the literal: balanced parens/brackets/quotes up to the
    # comma that closes the argument
    depth = 0
    i = start
    in_str = None
    while i < len(s):
        c = s[i]
        if in_str:
            if c == in_str:
                in_str = None
        elif c in "\"'":
            in_str = c
        elif c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            break
        i += 1
    return s[:start] + value_repr + s[i:]


def set_input(s, path, key, value_repr):
    """Update one key inside the o2r_inputs entry whose "path" is `path`."""
    idx = s.find('"path": "%s"' % path)
    assert idx >= 0, path
    end = s.find("}", idx)
    block = s[idx:end]
    pat = re.compile(r'("%s":\s*)("[^"]*"|[-\w]+)' % re.escape(key))
    assert pat.search(block), (path, key)
    block = pat.sub(lambda m: m.group(1) + value_repr, block, count=1)
    return s[:idx] + block + s[end:]


def run_generator():
    p = subprocess.run([sys.executable, os.path.join(REPO, "scripts", "stages", "generate_nds_native_stage.py"),
                        "--repo-root", REPO.replace("\\", "/"), "--stage", stage],
                       capture_output=True, text=True, cwd=REPO)
    return (p.stdout + p.stderr).strip().splitlines()


def tup(text):
    return tuple(ast.literal_eval(text))


# The runtime adapter rows name the packet's asset ids in o2r_inputs order
# (stage_images, stage_geometry, stage_map -- whichever the descriptor lists);
# the checker only says they "disagree", so derive them up front.
s0 = read()
ids = re.findall(r'"file_id":\s*(\d+)', s0)
m0 = re.search(r'^    adapter_asset_ids=\(([^)]*)\)', s0, re.M)
if ids and m0:
    want = ", ".join("0x%X" % int(i) for i in ids)
    if m0.group(1).replace(" ", "") != want.replace(" ", ""):
        s0 = s0[:m0.start(1)] + want + s0[m0.end(1):]
        write(s0)
        print("pinned adapter_asset_ids = (%s)" % want)

for it in range(max_iter):
    lines = run_generator()
    last = lines[-1] if lines else ""
    if "GENERATION_OK" in last:
        print("[%d] generation OK" % it)
        break
    m = re.search(r"M3_STAGE_FALSIFIER: (.*)$", last)
    msg = m.group(1) if m else last
    s = read()
    handled = None
    if (m := re.match(r"packet counts (\(.*?\)) != \(", msg)):
        actual = tup(m.group(1))
        for k, v in zip(COUNT_KEYS, actual):
            s = set_count(s, k, v)
        handled = "packet counts"
    elif (m := re.match(r"live DObjs (\d+) != ", msg)):
        s = set_count(s, "dobjs", int(m.group(1))); handled = "dobjs"
    elif (m := re.match(r"selected bindings (\d+) != ", msg)):
        s = set_count(s, "bindings", int(m.group(1))); handled = "bindings"
    elif (m := re.match(r"MODIFYVTX commands (\d+) != ", msg)):
        s = set_count(s, "modify_vertex_commands", int(m.group(1))); handled = "modify_vertex_commands"
    elif (m := re.match(r"submit classes (\(.*?\)) != ", msg)):
        s = set_count(s, "submit_classes", str(tup(m.group(1)))); handled = "submit_classes"
    elif (m := re.match(r"segment partition (\(.*\)) != \(", msg)):
        s = set_field(s, "segment_partition", repr(tup(m.group(1)))); handled = "segment_partition"
    elif (m := re.match(r"state packet counts \((\d+), (\d+), (\d+), (\d+)\) != ", msg)):
        s = set_count(s, "state_deltas", int(m.group(1)))
        s = set_count(s, "state_events", int(m.group(2)))
        s = set_count(s, "sync_events", int(m.group(4)))
        handled = "state counts"
    elif (m := re.match(r"cross-matrix counts \((\d+), (\d+), (\d+), \d+\) != ", msg)):
        s = set_count(s, "cross_runs", int(m.group(1)))
        s = set_count(s, "cross_tris", int(m.group(2)))
        s = set_count(s, "cross_corners", int(m.group(3)))
        handled = "cross counts"
    elif (m := re.match(r"generated include SHA256 ([0-9a-f]{64}) != ", msg)):
        s = set_field(s, "include_sha", '"%s"' % m.group(1)); handled = "include_sha"
    elif (m := re.match(r"(\S+): SHA256 ([0-9a-f]{64}) != pinned", msg)):
        s = set_input(s, m.group(1), "sha256", '"%s"' % m.group(2)); handled = "input sha"
    elif (m := re.match(r"(\S+): file ID (\d+) != ", msg)):
        s = set_input(s, m.group(1), "file_id", m.group(2)); handled = "file id"
    elif (m := re.match(r"(\S+): internal fixups (\d+) != ", msg)):
        s = set_input(s, m.group(1), "internal_fixups", m.group(2)); handled = "internal fixups"
    elif (m := re.match(r"(\S+): external fixups (\d+) != ", msg)):
        s = set_input(s, m.group(1), "external_fixups", m.group(2)); handled = "external fixups"
    elif (m := re.match(r"(\S+): payload SHA256 changed", msg)):
        path = m.group(1)
        spec = G.InputSpec(path=path, sha256=hashlib.sha256(open(os.path.join(REPO, path), "rb").read()).hexdigest(),
                           file_id=None, internal_fixups=None, external_fixups=None, payload_sha256=None)
        res = G.load_o2r(Path(REPO), spec)
        s = set_input(s, path, "payload_sha256", '"%s"' % hashlib.sha256(res.payload).hexdigest()); handled = "payload sha"
    if handled is None:
        print("[%d] NOT PINNABLE: %s" % (it, msg))
        sys.exit(1)
    write(s)
    print("[%d] pinned %s" % (it, handled))
else:
    print("gave up after %d iterations" % max_iter)
    sys.exit(1)

# The runtime adapter rows the checker compares against the packet: asset
# payload sizes (from the o2r inputs, in descriptor order), and the counts
# the generator has just pinned.
s = read()
paths = re.findall(r'"path":\s*"([^"]+)"', s)
o2r_paths = [p_ for p_ in paths if "/BattleShip_o2r/" in p_]
sizes = []
for path in o2r_paths:
    spec = G.InputSpec(path=path, sha256=hashlib.sha256(open(os.path.join(REPO, path), "rb").read()).hexdigest(),
                       file_id=None, internal_fixups=None, external_fixups=None, payload_sha256=None)
    sizes.append(len(G.load_o2r(Path(REPO), spec).payload))
want = ", ".join("0x%04X" % n for n in sizes)
m = re.search(r'^    adapter_asset_sizes=\(([^)]*)\)', s, re.M)
if m and m.group(1).replace(" ", "") != want.replace(" ", ""):
    s = s[:m.start(1)] + want + s[m.end(1):]
    print("pinned adapter_asset_sizes = (%s)" % want)


def count_of(key):
    return int(re.search(r'"%s":\s*(\d+)' % key, s).group(1))


def get_field(text, field):
    """Return the literal of a top-level `field=<literal>,` argument."""
    m = re.search(r'^    %s=' % re.escape(field), text, re.M)
    assert m, field
    start = m.end()
    depth = 0
    i = start
    in_str = None
    while i < len(text):
        c = text[i]
        if in_str:
            if c == in_str:
                in_str = None
        elif c in "\"'":
            in_str = c
        elif c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            break
        i += 1
    return ast.literal_eval(text[start:i].strip())


# The partitions name owners by constant (OWNER_LAYER0 ...), so read them from
# the registered descriptor object rather than as literals.
_desc = G._get_stage_descriptor(stage)
seg = tuple(_desc.segment_partition)
mat = tuple(_desc.material_command_partition)
derived = {
    "adapter_segment_count": len(seg),
    "adapter_dobj_count": count_of("dobjs"),
    "adapter_binding_count": count_of("bindings"),
    "adapter_asset_count": len(o2r_paths),
    "adapter_material_count": len(mat),
}
for field, value in derived.items():
    m = re.search(r'^    %s=(\d+)' % field, s, re.M)
    if m and int(m.group(1)) != value:
        s = s[:m.start(1)] + str(value) + s[m.end(1):]
        print("pinned %s = %d" % (field, value))
write(s)

for _ in range(3):
    p = subprocess.run([sys.executable, os.path.join(REPO, "scripts", "stages", "check_nds_native_stage.py"), "--stage", stage],
                       capture_output=True, text=True, cwd=REPO)
    last = (p.stdout + p.stderr).strip().splitlines()[-1]
    m = re.search(r"include SHA256 ([0-9a-f]{64}) != descriptor", last)
    if m:
        s = read()
        s = set_field(s, "include_sha", '"%s"' % m.group(1))
        write(s)
        print("pinned include_sha (checker)")
        continue
    print(last[:160])
    break
