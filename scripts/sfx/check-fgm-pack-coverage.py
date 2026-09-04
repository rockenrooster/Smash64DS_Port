#!/usr/bin/env python3
"""Check FGM source coverage in the generator and runtime allowlist.

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

  1. Is every id the port sources reference registered for packing. Names are
     collected from the compiled port sources and resolved through the port
     enums; the numeric hit-collision table the fighter runtime indexes
     directly is parsed on its own, since a name sweep cannot see it.
  2. Does the runtime's included-id `switch` equal the generator's tuple.
  3. Informational: registered ids no code references.

The generator is imported, not regexed: `FULL_COVERAGE_IDS` splices the
per-fighter audio tables with `*(...)`, so its text is not its value.
This does not inspect a rendered pack, its byte/hash pins, or playable audio.

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

# Cues the compiled code names that the generator deliberately does not pack.
# Each reason quotes render-audio-fgm-phase-pack.py. 246 is the documented
# example: the Samus Charge0..7 family loops forever, and Charge7 exists in
# the source attribute table but "neither Samus nor copied Samus can enter a
# held-charge loop at level 7" -- Start sets is_release when level == MAX, so
# it goes directly to End/launch ("source_start_releases_full_charge_without_
# entering_loop", SAMUS_CHARGE_UNREACHABLE_FULL_ID). The reference the sweep
# finds is the weapon's charge-level table entry, never a play call.
KNOWN_UNPACKED = {
    246: "source_start_releases_full_charge_without_entering_loop: level 7 enters immediate release, never a held loop",
}

NAME_RE = re.compile(r'\bnSYAudio(?:FGM|Voice)[A-Za-z0-9_]+\b')
ENUM_RE = re.compile(r'^\s*(nSYAudio(?:FGM|Voice)[A-Za-z0-9_]+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*,?', re.M)
CASE_RE = re.compile(r'^\s*case\s+(nSYAudio(?:FGM|Voice)[A-Za-z0-9_]+)\s*:', re.M)
# The switch also carries ids with no port-enum declaration: 188
# (nSYAudioFGMFoxSpecialLwHit) and 435 (nSYAudioVoiceMarioJump) are numeric
# `case 188u:` / `case 435u:` because include/gm/gmsound.h never declared
# them, and 673 is `case NDS_AUDIO_FGM_SAMUS_CHARGE_AUX_ID:` (the internal
# Samus Charge fork, not a public gmFGMVoiceID at all). A `case` names an id
# either way, so all three count as listed.
NUMERIC_CASE_RE = re.compile(r'^\s*case\s+(\d+)u?\s*:', re.M)
MACRO_CASE_RE = re.compile(r'^\s*case\s+([A-Z_][A-Za-z0-9_]*)\s*:', re.M)
INTEGER_DEFINE_RE = re.compile(
    r'^\s*#define\s+([A-Z_]\w*)\s+(0[xX][0-9a-fA-F]+|\d+)[uUlL]*\s*$', re.M)


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


def coverage_ids():
    spec = importlib.util.spec_from_file_location('render_audio_fgm_phase_pack', GENERATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    ids = tuple(int(i) for i in module.FULL_COVERAGE_IDS)
    dupes = sorted({i for i in ids if ids.count(i) > 1})
    selectors = {int(row['id']) for row in (
        *module.SELECTED, *module.EXCLUDED_SOURCE_CUES, *module.ATTACK_CUE_AUDIT)}
    for table in (module.SAMUS_NON_CHARGE_AUDIO, module.SAMUS_CHARGE_AUDIO,
                  *(row[1] for row in module.FINITE_BANK_SELECTOR_SPECS)):
        selectors.update(int(fgm_id) for fgm_id, _name in table)
    selectors.update((module.SAMUS_CHARGE_AUX_PROGRAM_ID,
                      module.PIKACHU_JOLT_LOOP_ID, *module.LOOP_PREFIX_IDS))
    return set(ids), dupes, sorted(set(ids) - selectors)


def port_enum():
    enum = {}
    for path in ENUM_FILES:
        for name, value in ENUM_RE.findall(read(path)):
            enum.setdefault(name, int(value, 0))
    return enum


def runtime_included(enum):
    """Read only the inclusion function, resolving its actual numeric defines."""
    source = read(RUNTIME)
    function = re.search(
        r'\bndsAudioFgmIDIsIncluded\s*\([^)]*\)\s*\{(.*?)^\}',
        source, re.M | re.S)
    if function is None:
        raise ValueError('ndsAudioFgmIDIsIncluded definition is absent')
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', function.group(1), flags=re.S)
    defines = {name: int(value, 0)
               for name, value in INTEGER_DEFINE_RE.findall(source)}
    names = CASE_RE.findall(text)
    unknown = [n for n in names if n not in enum]
    values = [enum[n] for n in names if n in enum]
    values.extend(int(v) for v in NUMERIC_CASE_RE.findall(text))
    for macro in MACRO_CASE_RE.findall(text):
        if macro in defines:
            values.append(defines[macro])
        else:
            unknown.append(macro)
    duplicates = sorted({value for value in values if values.count(value) > 1})
    return set(values), unknown, duplicates


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
    packed, dupes, missing_selectors = coverage_ids()
    enum = port_enum()
    included, unknown_cases, runtime_dupes = runtime_included(enum)
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
    known_unpacked = sorted(i for i in missing if i in KNOWN_UNPACKED)
    missing = [i for i in missing if i not in KNOWN_UNPACKED]
    unreferenced = sorted(i for i in packed if i not in reachable)
    only_runtime = sorted(included - packed)
    only_generator = sorted(packed - included)

    print(f'generator_ids={len(packed)} runtime_included={len(included)} referenced={len(reachable)} '
          f'unregistered_references={len(missing)} registered_unreferenced={len(unreferenced)}')
    by_id = {v: k for k, v in enum.items()}
    for fgm_id in missing:
        name, (path, line) = reachable[fgm_id]
        print(f'  UNPACKED {fgm_id:4d} {name:40s} {path}:{line}')
    for fgm_id in known_unpacked:
        name, (path, line) = reachable[fgm_id]
        print(f'  KNOWN-UNPACKED {fgm_id:4d} {name:40s} {path}:{line} '
              f'({KNOWN_UNPACKED[fgm_id]})')
    for fgm_id in only_runtime:
        print(f'  RUNTIME-ONLY {fgm_id:4d} {by_id.get(fgm_id, "?"):40s} runtime switch lists it, generator does not pack it')
    for fgm_id in only_generator:
        print(f'  GENERATOR-ONLY {fgm_id:4d} {by_id.get(fgm_id, "?"):40s} packed, but the runtime switch does not list it')
    if dupes:
        print('  duplicate ids in FULL_COVERAGE_IDS: ' + ', '.join(str(i) for i in dupes))
    if runtime_dupes:
        print('  duplicate runtime case values: ' + ', '.join(str(i) for i in runtime_dupes))
    if missing_selectors:
        print('  no generator selector factory for ids: ' +
              ', '.join(str(i) for i in missing_selectors))
    if unreferenced:
        print('  registered but no reference found (informational): ' +
              ', '.join(str(i) for i in unreferenced))
    for name, (path, line) in unknown:
        print(f'  UNKNOWN NAME {name} at {path}:{line} (not in any port enum)')
    for name in unknown_cases:
        print(f'  UNKNOWN CASE {name} in {os.path.relpath(RUNTIME, ROOT)} (not resolved by a port enum or numeric define)')

    if (missing or unknown or only_runtime or only_generator or unknown_cases or
            dupes or runtime_dupes or missing_selectors):
        print('FGM_PACK_COVERAGE_FAIL')
        return 1
    print('FGM_PACK_COVERAGE_OK source references covered; generator IDs equal runtime inclusion IDs')
    return 0


if __name__ == '__main__':
    sys.exit(main())
