#!/usr/bin/env python3
"""Check that every FGM cue the compiled port can ask for is in the pack.

`render-audio-fgm-phase-pack.py` ships exactly the ids in its `FULL_COVERAGE_IDS`
tuple. The runtime looks a cue up by id at play time (`ndsAudioFgmFindEntry`)
and on a miss increments a counter and plays nothing; it also carries its own
hand-written `switch` of "included" ids in `nds_audio_fgm.c` that decides which
miss counter to bump. Nothing at build time relates the three, so a cue that
landed content asks for and the tuple lacks is silent in the ROM and silent in
the build -- the Bat hit (`nSYAudioFGMBatHit`, 52) shipped that way on
2026-09-04: Luigi's sweetspot and the Home-Run Bat both index hit table row
Bat/Strong, the port table maps it to 52, and 52 was never in the tuple.

`check-audio-ordinals.py` answers a different question (does each declared
NAME carry the right NUMBER). This one answers three:

  1. Is every id the compiled code can reach actually packed. Names are
     collected from the compiled port sources and resolved through the port
     enums; the numeric hit-collision table the fighter runtime indexes
     directly is parsed on its own, since a name sweep cannot see it.
  2. Does the runtime's included-id `switch` equal the generator's tuple.
  3. Informational: packed ids no code references (ROM spent, not a defect).

The generator is imported, not regexed: `FULL_COVERAGE_IDS` splices the
per-fighter audio tables with `*(...)`, so its text is not its value.

Exit status is non-zero on 1 or 2.
"""
import importlib.util
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GENERATOR = os.path.join(ROOT, 'scripts', 'sfx', 'render-audio-fgm-phase-pack.py')
RUNTIME = os.path.join(ROOT, 'src', 'nds', 'nds_audio_fgm.c')
ENUM_FILES = [os.path.join(ROOT, 'include', 'gm', 'gmsound.h'),
              os.path.join(ROOT, 'include', 'sys', 'audio.h')]
HIT_TABLE = os.path.join(ROOT, 'src', 'port', 'reloc_backend_ftmain_runtime.c')
# src/ and include/ plus the one decomp directory the Makefile compiles as-is
# (BATTLESHIP_SYS). The other decomp TUs that reach the ROM do so through the
# overlay patches under scripts/import-overlays/battleship/, applied at build
# time to an ephemeral copy; their targets are read off the patch headers below.
DECOMP = os.path.join(ROOT, 'decomp', 'BattleShip-main', 'decomp')
SOURCE_DIRS = [os.path.join(ROOT, d) for d in ('src', 'include')] + [
    os.path.join(DECOMP, 'src', 'sys')]
OVERLAY_PATCHES = os.path.join(ROOT, 'scripts', 'import-overlays', 'battleship')
# Enum terminators are table sentinels, never played.
SENTINELS = {'nSYAudioFGMVoiceEnd'}

NAME_RE = re.compile(r'\bnSYAudio(?:FGM|Voice)[A-Za-z0-9_]+\b')
ENUM_RE = re.compile(r'^\s*(nSYAudio(?:FGM|Voice)[A-Za-z0-9_]+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*,?', re.M)
CASE_RE = re.compile(r'^\s*case\s+(nSYAudio(?:FGM|Voice)[A-Za-z0-9_]+)\s*:', re.M)


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


def coverage_ids():
    spec = importlib.util.spec_from_file_location('render_audio_fgm_phase_pack', GENERATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    ids = tuple(int(i) for i in module.FULL_COVERAGE_IDS)
    dupes = sorted({i for i in ids if ids.count(i) > 1})
    return set(ids), dupes


def port_enum():
    enum = {}
    for path in ENUM_FILES:
        for name, value in ENUM_RE.findall(read(path)):
            enum.setdefault(name, int(value, 0))
    return enum


def runtime_included(enum):
    """The `switch (id)` in ndsAudioFgmIsIncluded-style code: every `case` names
    an id the runtime believes the pack carries."""
    text = read(RUNTIME)
    names = CASE_RE.findall(text)
    unknown = [n for n in names if n not in enum]
    return {enum[n] for n in names if n in enum}, unknown


def hit_table_ids():
    text = read(HIT_TABLE)
    m = re.search(r'sNdsFighterDashRunHitCollisionFGMs\[[^=]*=\s*\{(.*?)\n\};', text, re.S)
    if not m:
        return {}
    ids = {}
    start = text[:m.start()].count('\n') + 1
    for offset, line in enumerate(m.group(1).split('\n')):
        for value in re.findall(r'\b(\d+)u\b', line):
            ids.setdefault(int(value), (os.path.relpath(HIT_TABLE, ROOT), start + offset))
    return ids


INCLUDE_C_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+\.c)[>"]', re.M)
PATCH_TARGET_RE = re.compile(r'^\+\+\+ b/(\S+\.c)', re.M)


def decomp_source(rel, includer):
    """Resolve an included `.c` path the way the compiler does: relative to the
    including file first (the wrappers use `"../../decomp/.../x.c"`), then the
    -I roots decomp/src (the `<ft/ftcommon/x.c>` form) and decomp itself."""
    for base in (os.path.dirname(includer), os.path.join(DECOMP, 'src'), DECOMP):
        candidate = os.path.normpath(os.path.join(base, rel.replace('/', os.sep)))
        if os.path.isfile(candidate):
            return candidate
    return None


def compiled_files():
    """Every file whose text reaches the compiler: the port trees, BATTLESHIP_SYS,
    the overlay-patched decomp TUs, and every decomp `.c` a src/import wrapper
    #includes under macro renames (the import pattern: the wrapper is a few
    #defines around `#include <ft/ftcommon/foo.c>`, so the cue names live in
    the decomp file, not the wrapper)."""
    seen = {}
    for base in SOURCE_DIRS:
        for dirpath, _dirs, files in os.walk(base):
            for f in files:
                if f.endswith(('.c', '.h', '.inc')):
                    path = os.path.join(dirpath, f)
                    seen.setdefault(os.path.normcase(os.path.abspath(path)), path)
    if os.path.isdir(OVERLAY_PATCHES):
        for f in os.listdir(OVERLAY_PATCHES):
            if f.endswith('.patch'):
                for rel in PATCH_TARGET_RE.findall(read(os.path.join(OVERLAY_PATCHES, f))):
                    path = os.path.join(DECOMP, rel.replace('/', os.sep))
                    if os.path.isfile(path):
                        seen.setdefault(os.path.normcase(os.path.abspath(path)), path)
    queue = [p for p in seen.values() if os.sep + 'import' + os.sep in p]
    while queue:
        path = queue.pop()
        for rel in INCLUDE_C_RE.findall(read(path)):
            target = decomp_source(rel, path)
            if target is None:
                continue
            key = os.path.normcase(os.path.abspath(target))
            if key not in seen:
                seen[key] = target
                queue.append(target)
    return list(seen.values())


def referenced_names():
    refs = {}
    skip = {os.path.normcase(os.path.abspath(p)) for p in ENUM_FILES + [RUNTIME]}
    for path in compiled_files():
        if os.path.normcase(os.path.abspath(path)) in skip:
            continue
        for lineno, line in enumerate(read(path).split('\n'), 1):
            for name in NAME_RE.findall(line):
                if name in SENTINELS:
                    continue
                refs.setdefault(name, (os.path.relpath(path, ROOT), lineno))
    return refs


def main():
    packed, dupes = coverage_ids()
    enum = port_enum()
    included, unknown_cases = runtime_included(enum)
    refs = referenced_names()

    reachable = {}
    unknown = []
    for name, where in sorted(refs.items()):
        if name not in enum:
            unknown.append((name, where))
            continue
        reachable.setdefault(enum[name], (name, where))
    for fgm_id, where in hit_table_ids().items():
        reachable.setdefault(fgm_id, ('hit-collision table', where))

    missing = sorted(i for i in reachable if i not in packed)
    unreferenced = sorted(i for i in packed if i not in reachable)
    only_runtime = sorted(included - packed)
    only_generator = sorted(packed - included)

    print(f'packed={len(packed)} runtime_included={len(included)} reachable={len(reachable)} '
          f'unpacked_reachable={len(missing)} packed_unreferenced={len(unreferenced)}')
    by_id = {v: k for k, v in enum.items()}
    for fgm_id in missing:
        name, (path, line) = reachable[fgm_id]
        print(f'  UNPACKED {fgm_id:4d} {name:40s} {path}:{line}')
    for fgm_id in only_runtime:
        print(f'  RUNTIME-ONLY {fgm_id:4d} {by_id.get(fgm_id, "?"):40s} runtime switch lists it, generator does not pack it')
    for fgm_id in only_generator:
        print(f'  GENERATOR-ONLY {fgm_id:4d} {by_id.get(fgm_id, "?"):40s} packed, but the runtime switch does not list it')
    if dupes:
        print('  duplicate ids in FULL_COVERAGE_IDS: ' + ', '.join(str(i) for i in dupes))
    if unreferenced:
        print('  packed but no reference found (informational): ' +
              ', '.join(str(i) for i in unreferenced))
    for name, (path, line) in unknown:
        print(f'  UNKNOWN NAME {name} at {path}:{line} (not in any port enum)')
    for name in unknown_cases:
        print(f'  UNKNOWN CASE {name} in {os.path.relpath(RUNTIME, ROOT)} (not in any port enum)')

    if missing or unknown or only_runtime or only_generator or unknown_cases:
        print('FGM_PACK_COVERAGE_FAIL')
        return 1
    print('FGM_PACK_COVERAGE_OK every reachable cue is packed and the runtime switch equals the pack')
    return 0


if __name__ == '__main__':
    sys.exit(main())
