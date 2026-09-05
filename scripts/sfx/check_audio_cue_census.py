#!/usr/bin/env python3
"""Census the audio cues the imported scenes request against what the port ships.

`check-fgm-pack-coverage.py` answers "is every FGM id the sources name packed,
and does the generator tuple equal `ndsAudioFgmIDIsIncluded`".  Nothing answered
the same question for **BGM**, and nothing answered it *per scene* -- so the 33
missing gmMusicID tracks (Results, the whole 1P campaign, the ending, credits,
Training, both item themes) sat silent until a hand census found them on
2026-09-05.  This closes that hole in both directions and adds the third
question a table row cannot answer on its own:

  1. **Requested BGM with no track row.**  Every `nSYAudioBGM*` the compiled
     sources name, by route, against `sNdsAudioBgmTracks` in
     `src/nds/nds_audio_bgm.c`.  Four routes reach `ndsAudioBgmPlay`:
       PLAY    `syAudioPlayBGM(0, nSYAudioBGMx)` in a scene body
       ITEM    `ftParamTryPlayItemMusic(nSYAudioBGMx)` (Hammer / Star)
       DEFAULT `gMPCollisionBGMDefault = nSYAudioBGMx` (a scene overriding the
               stage's own track -- sc1pgame's Final Destination, training)
       STAGE   the `bgm_id` field of a staged stage descriptor.  A stage is
               staged when `include/reloc_data.h` rows `ll<Name>FileID` with a
               file number whose `decomp/.../relocData/<n>_<Name>.c` exists; its
               `bgm_id` is what `mpCollisionSetPlayBGM` plays.
       DATA    a table literal or a switch case (the Sound Test's
               `dMNSoundTestMusicIDs`, `ftParamGetItemMusicLength`).
  2. **Track rows nothing requests.**  A rendered track no code can reach is
     ROM weight; each deliberate one carries a reason in ALLOW.
  3. **Requests that cannot reach the player.**  A row in the table proves the
     *asset* exists, never that the *call* survives.  Every decomp function that
     calls `syAudioPlayBGM` is located; if the port hand-implements it instead of
     importing it (no `#include` of that decomp source from any
     `src/import/battleship_*.c`), the port body must still reach a BGM sink.
     An empty port body is a silent cue, and the checker names its line.
     `ftParamTryPlayItemMusic` and `ftParamTryUpdateItemMusic` were exactly that
     on 2026-09-05: both tracks rendered, both callers compiled, both bodies
     `{ (void)bgm_id; }`.

The FGM half is per-scene attribution over the same as-built read, cross-checked
against the generator tuple and `ndsAudioFgmIDIsIncluded`; the global FGM
verdict stays `check-fgm-pack-coverage.py`'s job and is not duplicated here.

Sources are read the way the build compiles them, through
`scripts/menus/check_reloc_symbol_census.py`: the import-overlay patch when one
exists, then `as_built()` to drop the `REGION_JP` and `!SSB64_TARGET_NDS` arms.
Comments are stripped before any name sweep -- `nds_audio_bgm.c` names every id
twice (row plus provenance comment) and every scene wrapper's header comment
lists the audio entry points it needs.

Usage:
    python scripts/sfx/check_audio_cue_census.py            # report
    python scripts/sfx/check_audio_cue_census.py --strict   # exit 1 on a gap
    python scripts/sfx/check_audio_cue_census.py --scenes   # per-scene table
    python scripts/sfx/check_audio_cue_census.py --scene battleship_mvending.c

This reads sources only.  It never renders, hashes or inspects a pack: a
re-render of the FGM pack moves its byte/hash pins, so the packing decision
stays a human one.
"""
from __future__ import annotations

import argparse
import glob
import importlib.util
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RELOC_CENSUS = os.path.join(ROOT, 'scripts', 'menus', 'check_reloc_symbol_census.py')
FGM_COVERAGE = os.path.join(ROOT, 'scripts', 'sfx', 'check-fgm-pack-coverage.py')
BGM_RUNTIME = os.path.join(ROOT, 'src', 'nds', 'nds_audio_bgm.c')
RELOC_DATA = os.path.join(ROOT, 'include', 'reloc_data.h')
DECOMP_SRC = os.path.join(ROOT, 'decomp', 'BattleShip-main', 'decomp', 'src')
RELOC_DATA_SRC = os.path.join(DECOMP_SRC, 'relocData')
IMPORT_GLOB = os.path.join(ROOT, 'src', 'import', 'battleship_*.c')
# Where a hand-ported (not imported) decomp function may be defined.
PORT_DIRS = [os.path.join(ROOT, 'src', 'port'), os.path.join(ROOT, 'src', 'nds')]

BGM_NAME_RE = re.compile(r'\bnSYAudioBGM[A-Za-z0-9_]+\b')
FGM_NAME_RE = re.compile(r'\bnSYAudio(?:FGM|Voice)[A-Za-z0-9_]+\b')
ENUM_RE = re.compile(
    r'^\s*(nSYAudio(?:BGM|FGM|Voice)[A-Za-z0-9_]+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*,?', re.M)
PLAY_RE = re.compile(r'\bsyAudioPlayBGM\s*\(\s*[^,()]*,\s*([^;()]*?)\s*\)')
ITEM_RE = re.compile(r'\bftParamTryPlayItemMusic\s*\(\s*([^;()]*?)\s*\)')
DEFAULT_RE = re.compile(r'\bgMPCollisionBGM(?:Default|Current)\s*=\s*(nSYAudioBGM[A-Za-z0-9_]+)')
FILE_ID_RE = re.compile(r'X\(\s*ll(\w+?)FileID\s*,\s*(0[xX][0-9a-fA-F]+|\d+)\s*\)')
MAP_BGM_RE = re.compile(r'(nSYAudioBGM\w+)\s*,\s*/\*\s*bgm_id\s*\*/')
TRACK_TABLE_RE = re.compile(r'sNdsAudioBgmTracks\[\]\s*=\s*\{(.*?)\n\};', re.S)
# A K&R definition line: a type, a name, an argument list, then `{` on the
# next line.  The import wrappers and the decomp sources both use this form.
DEF_RE = re.compile(r'^([A-Za-z_][\w \t\*]*?\b)(\w+)\s*\(([^;{}]*)\)\s*$')
# What a port body must reach for a BGM request to become sound.
BGM_SINKS = ('syAudioPlayBGM', 'ndsAudioBgmPlay', 'ftParamTryPlayItemMusic',
             'mpCollisionSetPlayBGM')
# Enum terminators and comment shorthand, never played.  Mirrors
# check-fgm-pack-coverage.py's SENTINELS.
SENTINELS = {'nSYAudioFGMVoiceEnd', 'nSYAudioVoiceAnnounce', 'nSYAudioBGMEnd'}

# Deliberately-absent cues and seams.  One line each, quoting the evidence.
# A key here is excused from --strict; delete the key the moment the reason
# stops being true, and never add one to silence an unexplained gap.
ALLOW = {
    'BGM_SINK:dbCubeAudioThreadUpdate':
        'db/dbcube.c is the N64 debug cube; no src/import wrapper includes it '
        'and no port target compiles it.',
    'BGM_SINK:mnModeSelectFuncStart':
        'the VS shell replaces mode select natively: '
        'ndsMenuShellModeSelectPlayBgm (src/nds/nds_menu_shell_router.c:467) '
        'transcribes mnmodeselect.c:882 including its four-scene re-entry '
        'guard, and reaches ndsAudioBgmPlay directly.',
}


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


def strip_comments(text):
    """Blank C comments, keeping line count so reported lines stay true."""
    def blank(m):
        return re.sub(r'[^\n]', ' ', m.group(0))
    return re.sub(r'/\*.*?\*/|//[^\n]*', blank, text, flags=re.S)


def load_reloc_census():
    spec = importlib.util.spec_from_file_location('reloc_symbol_census', RELOC_CENSUS)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def as_built_source(census, rel):
    """The decomp source at `rel` as the build compiles it, or None."""
    pristine = os.path.join(DECOMP_SRC, rel.replace('/', os.sep))
    if not os.path.isfile(pristine):
        return None
    return census.as_built(census.patched_source(ROOT, rel, pristine))


def scene_sources(census):
    """{tu basename: {rel or '<wrapper>': as-built text}} over src/import.

    A wrapper's own body counts (it defines port-side helpers that play cues),
    and every decomp `.c` it pulls in, transitively -- an imported source may
    itself `#include` another."""
    scenes = {}
    for path in sorted(glob.glob(IMPORT_GLOB)):
        name = os.path.basename(path)
        wrapper = census.as_built(read(path))
        bodies = {'<wrapper>': wrapper}
        queue = [m.group(1) for m in census.INC_RE.finditer(wrapper)]
        while queue:
            rel = queue.pop()
            if rel in bodies:
                continue
            body = as_built_source(census, rel)
            if body is None:
                continue
            bodies[rel] = body
            queue.extend(m.group(1) for m in census.INC_RE.finditer(body))
        scenes[name] = bodies
    return scenes


def audio_enum():
    """Every nSYAudio* enumerator the port declares, name -> value."""
    enum = {}
    for rel in (('include', 'gm', 'gmsound.h'), ('include', 'sys', 'audio.h')):
        for name, value in ENUM_RE.findall(read(os.path.join(ROOT, *rel))):
            enum.setdefault(name, int(value, 0))
    return enum


def bgm_track_ids():
    """The gmMusicID of every row in sNdsAudioBgmTracks, in table order."""
    text = strip_comments(read(BGM_RUNTIME))
    m = TRACK_TABLE_RE.search(text)
    if m is None:
        raise ValueError('sNdsAudioBgmTracks table not found in ' + BGM_RUNTIME)
    return [n for n in BGM_NAME_RE.findall(m.group(1))]


def staged_stage_bgm():
    """{stage descriptor name: BGM enumerator} for every staged stage.

    `X(llGRLastMapFileID, 0x10a)` stages relocData file 266, whose source
    `266_GRLastMap.c` carries the `bgm_id` mpCollisionSetPlayBGM plays."""
    available = {}
    for path in glob.glob(os.path.join(RELOC_DATA_SRC, '*.c')):
        number, _, _rest = os.path.basename(path).partition('_')
        if number.isdigit():
            available[int(number)] = path
    stages = {}
    for name, value in FILE_ID_RE.findall(read(RELOC_DATA)):
        path = available.get(int(value, 0))
        if path is None or os.path.basename(path) != f'{int(value, 0)}_{name}.c':
            continue
        # Read raw: `/* bgm_id */` is the field marker, so stripping comments
        # here would erase the very anchor the field is identified by.
        m = MAP_BGM_RE.search(read(path))
        if m:
            stages[name] = m.group(1)
    return stages


def function_body(text, index):
    """Brace-matched body starting at the first `{` at or after `index`."""
    start = text.find('{', index)
    if start < 0:
        return ''
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    return text[start:]


def definitions(text):
    """{function name: (line, body)} for every K&R definition in `text`."""
    lines = text.split('\n')
    found = {}
    for i, line in enumerate(lines):
        m = DEF_RE.match(line)
        if not m or not lines[i + 1:i + 2] or not lines[i + 1].lstrip().startswith('{'):
            continue
        if m.group(1).strip() in ('return', 'else', 'case'):
            continue
        offset = sum(len(l) + 1 for l in lines[:i])
        found[m.group(2)] = (i + 1, function_body(text, offset))
    return found


def bgm_requests(scenes, stages):
    """[(id name, route, where)] over every compiled source and staged stage."""
    requests = []
    indirect = []
    for tu, bodies in sorted(scenes.items()):
        for rel, body in sorted(bodies.items()):
            text = strip_comments(body)
            claimed = set()
            for regex, route in ((PLAY_RE, 'PLAY'), (ITEM_RE, 'ITEM')):
                for m in regex.finditer(text):
                    argument = m.group(1).strip()
                    names = BGM_NAME_RE.findall(argument)
                    if names:
                        for name in names:
                            requests.append((name, route, f'{tu}[{rel}]'))
                            claimed.add(name)
                    else:
                        indirect.append((route, argument, f'{tu}[{rel}]'))
            for m in DEFAULT_RE.finditer(text):
                requests.append((m.group(1), 'DEFAULT', f'{tu}[{rel}]'))
                claimed.add(m.group(1))
            for name in BGM_NAME_RE.findall(text):
                if name not in claimed and name not in SENTINELS:
                    requests.append((name, 'DATA', f'{tu}[{rel}]'))
    for stage, name in sorted(stages.items()):
        requests.append((name, 'STAGE', f'relocData/{stage}'))
    return requests, indirect


def bgm_sinks(scenes):
    """[(function, decomp rel, line, imported)] for every decomp function whose
    body calls syAudioPlayBGM.  `imported` is the TU that compiles it, or None
    when the port must hand-implement it."""
    imported = {}
    for tu, bodies in scenes.items():
        for rel in bodies:
            imported.setdefault(rel, tu)
    sinks = []
    for dirpath, _dirs, files in os.walk(DECOMP_SRC):
        if os.path.basename(dirpath) == 'relocData':
            continue
        for name in sorted(files):
            if not name.endswith('.c'):
                continue
            path = os.path.join(dirpath, name)
            text = strip_comments(read(path))
            if 'syAudioPlayBGM' not in text:
                continue
            rel = os.path.relpath(path, DECOMP_SRC).replace(os.sep, '/')
            for function, (line, body) in definitions(text).items():
                if 'syAudioPlayBGM' in body and function != 'syAudioPlayBGM':
                    sinks.append((function, rel, line, imported.get(rel)))
    return sorted(sinks)


def port_definition(function):
    """(path, line, body) where src/port or src/nds defines `function`."""
    for base in PORT_DIRS:
        for dirpath, _dirs, files in os.walk(base):
            for name in sorted(files):
                if not name.endswith(('.c', '.inc')):
                    continue
                path = os.path.join(dirpath, name)
                text = strip_comments(read(path))
                if function not in text:
                    continue
                found = definitions(text).get(function)
                if found:
                    return os.path.relpath(path, ROOT), found[0], found[1]
    return None


def blocked_sinks(sinks):
    """[(function, port path, line, reason)] for sinks the port cannot reach."""
    blocked = []
    for function, rel, line, importer in sorted(set(sinks)):
        if importer is not None:
            continue
        port = port_definition(function)
        if port is None:
            blocked.append((function, '-', 0,
                            f'{rel}:{line} is neither imported nor ported'))
            continue
        path, port_line, body = port
        if not any(sink in body for sink in BGM_SINKS):
            summary = ' '.join(body.split())
            blocked.append((function, path, port_line,
                            f'port body reaches no BGM sink: {summary[:70]}'))
    return blocked


def fgm_by_scene(scenes, enum):
    """{tu: {id: name}} for the FGM/Voice cues each scene's as-built text names."""
    per_scene = {}
    unknown = {}
    for tu, bodies in sorted(scenes.items()):
        ids = {}
        for rel, body in sorted(bodies.items()):
            for name in FGM_NAME_RE.findall(strip_comments(body)):
                if name in SENTINELS:
                    continue
                if name in enum:
                    ids.setdefault(enum[name], name)
                else:
                    unknown.setdefault(name, f'{tu}[{rel}]')
        if ids:
            per_scene[tu] = ids
    return per_scene, unknown


def pack_ids():
    """(FULL_COVERAGE_IDS, ndsAudioFgmIDIsIncluded, KNOWN_UNPACKED) as id sets.

    Imported, never regexed: `FULL_COVERAGE_IDS` splices the per-fighter tables
    with `*(...)`, so its text is not its value.  KNOWN_UNPACKED comes from
    check-fgm-pack-coverage.py so the two checkers cannot disagree about which
    unpacked cue is deliberate."""
    spec = importlib.util.spec_from_file_location('fgm_coverage', FGM_COVERAGE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    packed, _dupes, _selectors = module.coverage_ids()
    included, _unknown, _runtime_dupes = module.runtime_included(module.port_enum())
    return packed, included, dict(module.KNOWN_UNPACKED)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--strict', action='store_true',
                    help='exit 1 when a requested cue is unshippable')
    ap.add_argument('--scenes', action='store_true',
                    help='print the per-scene BGM/FGM table')
    ap.add_argument('--scene', help='one src/import basename to detail')
    args = ap.parse_args(argv)

    census = load_reloc_census()
    scenes = scene_sources(census)
    enum = audio_enum()
    rows = bgm_track_ids()
    stages = staged_stage_bgm()
    requests, indirect = bgm_requests(scenes, stages)
    sinks = bgm_sinks(scenes)
    blocked = [b for b in blocked_sinks(sinks) if f'BGM_SINK:{b[0]}' not in ALLOW]
    packed, included, known_unpacked = pack_ids()
    per_scene, unknown_fgm = fgm_by_scene(scenes, enum)

    requested = {}
    for name, route, where in requests:
        requested.setdefault(name, []).append((route, where))
    row_set = set(rows)
    missing = sorted(n for n in requested if n not in row_set)
    unrequested = [n for n in rows
                   if n not in requested and f'BGM_UNREQUESTED:{enum.get(n)}' not in ALLOW]
    excused = [n for n in rows
               if n not in requested and f'BGM_UNREQUESTED:{enum.get(n)}' in ALLOW]
    unknown_bgm = sorted(n for n in requested if n not in enum)
    # A row named only by a table literal has no scene that starts it: the
    # Sound Test can play it, ordinary play cannot.  Informational, never a
    # --strict failure -- a scene deferred by the owner (the opening
    # cinematic, P2-7) leaves its track exactly this way.
    start_routes = {'PLAY', 'DEFAULT', 'STAGE', 'ITEM'}
    data_only = [n for n in rows
                 if n in requested
                 and not {r for r, _w in requested[n]} & start_routes]

    fgm_ids = set()
    for ids in per_scene.values():
        fgm_ids |= set(ids)
    unpacked = sorted(i for i in fgm_ids if i not in packed)
    fgm_known = [i for i in unpacked if i in known_unpacked]
    fgm_missing = [i for i in unpacked if i not in known_unpacked]
    switch_drift = sorted(packed.symmetric_difference(included))

    print(f'bgm_rows={len(rows)} bgm_requested={len(requested)} '
          f'bgm_missing={len(missing)} bgm_unrequested={len(unrequested)} '
          f'staged_stages={len(stages)} bgm_sinks={len(set(sinks))} '
          f'blocked_sinks={len(blocked)} fgm_packed={len(packed)} '
          f'fgm_referenced={len(fgm_ids)} fgm_missing={len(fgm_missing)}')

    for name in missing:
        route, where = requested[name][0]
        print(f'  BGM-MISSING  {enum.get(name, -1):3d} {name:32s} '
              f'{route} {where} (+{len(requested[name]) - 1} more)')
    for name in unrequested:
        print(f'  BGM-UNREQUESTED {enum.get(name, -1):3d} {name:32s} '
              'track row that no compiled source or staged stage names')
    for name in excused:
        print(f'  BGM-ALLOWED  {enum.get(name, -1):3d} {name:32s} '
              f'{ALLOW["BGM_UNREQUESTED:" + str(enum.get(name))]}')
    for name in data_only:
        where = ', '.join(sorted({w for _r, w in requested[name]}))
        print(f'  BGM-DATA-ONLY {enum.get(name, -1):3d} {name:32s} '
              f'named only by a table ({where}); no scene starts it')
    for function, path, line, reason in blocked:
        print(f'  BGM-BLOCKED  {function:32s} {path}:{line} {reason}')
    for name in unknown_bgm:
        print(f'  BGM-UNKNOWN  {name} (named by a compiled source, absent from '
              'include/gm/gmsound.h)')
    for fgm_id in fgm_missing:
        print(f'  FGM-MISSING  {fgm_id:4d} named by a scene, absent from '
              'FULL_COVERAGE_IDS')
    for fgm_id in fgm_known:
        print(f'  FGM-KNOWN-UNPACKED {fgm_id:4d} {known_unpacked[fgm_id]}')
    for name, where in sorted(unknown_fgm.items()):
        print(f'  FGM-UNKNOWN  {name} at {where}')
    if switch_drift:
        print('  SWITCH-DRIFT ndsAudioFgmIDIsIncluded and FULL_COVERAGE_IDS '
              'differ on: ' + ', '.join(str(i) for i in switch_drift))
    for route, argument, where in sorted(set(indirect)):
        print(f'  BGM-INDIRECT {route} {where} plays `{argument}` '
              '(resolved through DEFAULT/STAGE/DATA above)')

    if args.scenes or args.scene:
        print('--- per scene: BGM verdict / ids requested / FGM ids named ---')
        blocked_names = {function for function, _p, _l, _r in blocked}
        # A route is live when the seam it goes through reaches the player.
        # PLAY and DEFAULT ride syAudioPlayBGM, which forwards to
        # ndsAudioBgmPlay unconditionally (reloc_backend_compat_shims.c:1345,
        # no scene-kind gate); STAGE rides mpCollisionSetPlayBGM, likewise.
        live = {'PLAY', 'PLAY-INDIRECT', 'DEFAULT', 'STAGE'}
        if 'ftParamTryPlayItemMusic' not in blocked_names:
            live.add('ITEM')
        for tu in sorted(scenes):
            if args.scene and tu != args.scene:
                continue
            mine = [(n, r) for n, r, w in requests if w.startswith(tu + '[')]
            if not mine and not args.scene:
                continue
            routes = sorted({r for _n, r in mine})
            # A `syAudioPlayBGM(0, <table lookup>)` is still a live start; the
            # ids it can reach are the DATA names counted above (the Sound
            # Test's dMNSoundTestMusicIDs is exactly this shape).
            if any(w.startswith(tu + '[') and r == 'PLAY' for r, _a, w in indirect):
                routes.append('PLAY-INDIRECT')
            names = sorted({n for n, _r in mine}, key=lambda n: enum.get(n, -1))
            if set(routes) & live:
                verdict = 'REACHES'
            elif set(routes) & {'ITEM'}:
                verdict = 'BLOCKED'
            elif routes:
                verdict = 'DATA-ONLY'
            else:
                verdict = 'INHERITS'
            print(f'  {tu:44s} {verdict:9s} bgm={len(names):2d} '
                  f'fgm={len(per_scene.get(tu, {})):3d} [{"/".join(routes)}] '
                  f'{", ".join(names[:6])}{" ..." if len(names) > 6 else ""}')

    failed = bool(missing or unrequested or blocked or unknown_bgm or
                  fgm_missing or unknown_fgm or switch_drift)
    if failed:
        print('AUDIO_CUE_CENSUS_FAIL')
        return 1 if args.strict else 0
    print('AUDIO_CUE_CENSUS_OK every requested cue has a track row or pack entry '
          'and every hand-ported BGM sink reaches the player')
    return 0


if __name__ == '__main__':
    sys.exit(main())
