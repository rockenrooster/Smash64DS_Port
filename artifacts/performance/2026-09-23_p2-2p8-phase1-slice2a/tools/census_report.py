"""Decode a slice 2a census run (census-a.json) into tables."""
import json, os, subprocess, sys, bisect

ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a'
ELF = r'D:\Stuff\DevFolder\Smash64DS_Port\builds\build-p2-fourcpu-tickhud\smash64ds-p2-fourcpu-tickhud-hwtri.elf'
NM = r'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
FIXED = ['cache:stage', 'cache:static', 'cache:stage_warm', 'cache:slot0', 'cache:slot1', 'cache:slot2',
         'cache:slot3', 'cache:other', 'entry:startup', 'entry:kept', 'tint', 'halo+shield']
SITES = 30
BUCKETS = len(FIXED) + SITES + 1

arm = sys.argv[1] if len(sys.argv) > 1 else 'census-a'
d = json.load(open(os.path.join(ART, arm + '.json')))
ex = {e['name']: e.get('value') for e in d['extras']}

# site PCs -> functions
syms = []
out = subprocess.run([NM, '-S', '--defined-only', ELF], capture_output=True, text=True).stdout
for line in out.splitlines():
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        syms.append((int(p[0], 16), int(p[1], 16), p[3]))
syms.sort()
starts = [s[0] for s in syms]
def func_of(pc):
    i = bisect.bisect_right(starts, pc) - 1
    if i >= 0 and syms[i][0] <= pc < syms[i][0] + syms[i][1]:
        return syms[i][2]
    return '?%08x' % pc
site_count = ex.get('gNdsVramCensusSiteCount') or 0
site_names = []
for i in range(SITES):
    pc = ex.get('gNdsVramCensusSitePc[%d]' % i) or 0
    site_names.append(func_of(pc) if i < site_count else '-')
names = FIXED + ['site%d:%s' % (i, site_names[i]) for i in range(SITES)] + ['untagged/full']

def snap(s):
    g = lambda f: ex.get('gNdsVramCensus.%s.%s' % (s, f))
    r = {f: g(f) for f in ('frame', 'tex_names', 'tex_bytes', 'tex_size_field', 'tex_free', 'tex_free_blocks',
                           'tex_largest_free', 'tex_alloc_blocks', 'tex_alloc_bytes', 'tex_start', 'tex_end',
                           'tex_usable', 'tex_free_usable', 'tex_largest_usable', 'tex_free_runs_usable',
                           'pal_usable', 'pal_free_usable', 'pal_largest_usable', 'pal_free_runs_usable',
                           'pal_names', 'pal_bytes', 'pal_free', 'pal_free_blocks', 'pal_largest_free',
                           'pal_alloc_blocks', 'pal_alloc_bytes', 'pal_start', 'pal_end', 'dispcnt',
                           'vramcnt_abcd', 'vramcnt_efg', 'bg2_opaque', 'bg3_opaque', 'last_request')}
    r['bytes'] = [g('bytes[%d]' % i) or 0 for i in range(BUCKETS)]
    r['count'] = [g('count[%d]' % i) or 0 for i in range(BUCKETS)]
    r['pal'] = [g('pal[%d]' % i) or 0 for i in range(BUCKETS)]
    return r

snaps = {s: snap(s) for s in ('go', 'burst', 'episode', 'after')}
print('run', arm, 'setGlobals', d.get('setGlobals'), 'rom', d.get('romSha256', '')[:16])
for k in ('gNdsVramCensus.frames', 'gNdsVramCensus.captures', 'gNdsVramCensus.walk_guard_hits',
          'gNdsVramCensus.name_limit', 'gNdsVramCensusNameOverflow', 'gNdsVramCensusSiteCount',
          'gNdsVramCensus.peak_tex_bytes', 'gNdsVramCensus.peak_pal_bytes', 'gNdsVramCensus.min_largest_free',
          'gNdsVramCensus.min_largest_free_frame', 'gNdsVramCensusOverlayMask',
          'gNdsVramCensus.bg_frame[0]', 'gNdsVramCensus.bg2_opaque[0]', 'gNdsVramCensus.bg3_opaque[0]',
          'gNdsVramCensus.bg_frame[1]', 'gNdsVramCensus.bg2_opaque[1]', 'gNdsVramCensus.bg3_opaque[1]',
          'gNdsOriginalSpriteBg2ClearBytes', 'gNdsOriginalSpriteBg2CopyBytes', 'gNdsOriginalSpriteBg2FinalWriteBytes',
          'gNdsOriginalSpriteBg3ClearBytes', 'gNdsOriginalSpriteBg3CopyBytes', 'gNdsOriginalSpriteBg3FinalWriteBytes',
          'gNdsOriginalSpritePreviewCommitCount', 'gNdsEntryEffectStartupTextureReleaseCount',
          'gNdsEntryEffectStartupTextureReleaseBytes', 'gNdsRendererNativeFailure.count',
          'gNdsRendererNativeDirectReject.count', 'gNdsFtrLean.go_frame', 'gNdsFtrLean.fighter_uploads_after_go'):
    print('  %-48s %s' % (k, ex.get(k)))
print()
hdr = ['frame', 'tex_names', 'tex_bytes', 'tex_size_field', 'tex_alloc_bytes', 'tex_alloc_blocks', 'tex_usable', 'tex_free_usable', 'tex_largest_usable', 'tex_free_runs_usable', 'pal_usable', 'pal_free_usable', 'pal_largest_usable', 'pal_free_runs_usable', 'tex_free',
       'tex_free_blocks', 'tex_largest_free', 'tex_start', 'tex_end', 'pal_names', 'pal_bytes', 'pal_alloc_bytes',
       'pal_free', 'pal_free_blocks', 'pal_largest_free', 'pal_start', 'pal_end', 'dispcnt', 'vramcnt_abcd',
       'vramcnt_efg', 'bg2_opaque', 'bg3_opaque', 'last_request']
print('%-18s' % 'field' + ''.join('%14s' % s for s in snaps))
for h in hdr:
    vals = []
    for s in snaps:
        v = snaps[s][h]
        if v is None:
            vals.append('-')
        elif h in ('tex_start', 'tex_end', 'pal_start', 'pal_end', 'dispcnt', 'vramcnt_abcd', 'vramcnt_efg', 'last_request'):
            vals.append('0x%08x' % v)
        else:
            vals.append('{:,}'.format(v))
    print('%-18s' % h + ''.join('%14s' % v for v in vals))
print()
peak = [ex.get('gNdsVramCensus.peak_bytes[%d]' % i) or 0 for i in range(BUCKETS)]
peakc = [ex.get('gNdsVramCensus.peak_count[%d]' % i) or 0 for i in range(BUCKETS)]
print('%-52s' % 'bucket (texel bytes / names / palette bytes)' + ''.join('%22s' % s for s in snaps) + '%18s' % 'peak')
for i in range(BUCKETS):
    if not any(snaps[s]['bytes'][i] or snaps[s]['pal'][i] or snaps[s]['count'][i] for s in snaps) and not peak[i]:
        continue
    print('%-52s' % names[i][:52] + ''.join('%22s' % ('{:,}/{}/{:,}'.format(snaps[s]['bytes'][i], snaps[s]['count'][i], snaps[s]['pal'][i])) for s in snaps)
          + '%18s' % ('{:,}/{}'.format(peak[i], peakc[i])))
print()
print('sites:')
for i in range(site_count):
    print('  %2d 0x%08x %s' % (i, ex.get('gNdsVramCensusSitePc[%d]' % i) or 0, site_names[i]))
# per-stop timeline
stops = d.get('ringStops')
print()
print('ringStops field:', type(stops).__name__, stops if not isinstance(stops, list) else len(stops))
print('keys with stop/perStop:', [k for k in d.keys() if 'top' in k.lower()])
