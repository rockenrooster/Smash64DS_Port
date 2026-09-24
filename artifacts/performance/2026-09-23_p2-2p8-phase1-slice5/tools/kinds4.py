"""Per-kind lean counters of one slice 4 run JSON (run-s4.ps1 output).

Usage: kinds4.py ARM [ARM...]   (ARM = run name in the artifact dir, or a path
without .json)
"""
import json
import os
import sys

KINDS = ['Donkey', 'Samus', 'Link', 'Kirby']
DECLINE = ['kind', 'skeleton', 'camera', 'plan', 'validate', 'kirby_head',
           'roots', 'material', 'tables', 'policy', 'texture', 'capacity',
           'topology', 'kernel', 'texgen', 'tint', 'stale', 'inputs', '18',
           '19']
EVENT = ['none', 'first', 'tuple', 'status', 'rebind', 'material', 'preamble',
         'tint_set', 'tint_tile', 'fence', 'kernel']
JCLASS = ['fast', 'nolocal', 'scale', 'warm', 'lock', 'convert', 'xobj',
          'nogobj']
OCLASS = ['structure', 'projection', 'modelview', 'shade', 'light', 'other',
          'tint', 'texgen']
GUARD = ['tuple', 'topology', 'camera', 'identity', 'preamble', 'packet',
         'shuffle', 'event']
PATCH = ['tint_tiles', 'apply_tint', 'matrices', 'light', 'texgen', 'sites']
SUBMIT = ['flush', 'gx+dma_wait', 'dma+invalidate', 'stats']
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load(arm):
    path = arm if arm.endswith('.json') else arm + '.json'
    if not os.path.exists(path):
        path = os.path.join(HERE, path)
    d = json.load(open(path))
    return d, {e['name']: e['value'] for e in d['extras']}


def g(ex, name):
    return ex.get('gNdsFtrLean.' + name, 0) or 0


def per(v, n):
    return '%.0f' % (v / n) if n else '-'


def main():
    for arm in sys.argv[1:]:
        d, ex = load(arm)
        print('==', arm, 'rom', d.get('romSha256', '')[:8], 'route',
              ex.get('gNdsFtrLeanRoute'), 'admit', ex.get('gNdsFtrLeanAdmit'),
              'slow', ex.get('gNdsFtrLeanSlow'))
        print('  attempts %s draws %s shadow %s materializations %s entry hits %s '
              'switches %s region takes %s | packet hits %s records %s faults %s '
              'declines %s | native failures %s | draws(all) %s' % (
                  g(ex, 'attempts'), g(ex, 'draws'), g(ex, 'shadow_runs'),
                  g(ex, 'materializations'), g(ex, 'entry_hits'),
                  g(ex, 'entry_switches'), g(ex, 'region_takes'),
                  ex.get('gNdsFighterPacketHits'),
                  ex.get('gNdsFighterPacketRecords'),
                  ex.get('gNdsFighterPacketFaults'),
                  ex.get('gNdsFighterPacketDeclines'),
                  ex.get('gNdsRendererNativeFailure.count'),
                  ex.get('gNdsFighterMarioFoxDLAllDrawCount')))
        mats = g(ex, 'materializations')
        print('  materialize: %s ticks/event path incl. rows (%s list walk), last %s '
              'words %s roots %s textures %s tint binds %s tint pending; max %s words' % (
                  per(g(ex, 'materialize_ticks'), mats),
                  per(g(ex, 'materialize_list_ticks'), mats),
                  g(ex, 'materialize_words'), g(ex, 'materialize_roots'),
                  g(ex, 'materialize_textures'), g(ex, 'materialize_tint_binds'),
                  g(ex, 'materialize_tint_pending'), g(ex, 'materialize_words_max')))
        print('  materialize parts/materialization', {n: per(g(ex, 'materialize_part_ticks[%d]' % i), mats)
                                                     for i, n in enumerate(('head', 'roots', 'spans', 'shade', 'prepare', 'corners', 'bind', 'words'))},
              'key miss', [g(ex, 'materialize_key_miss[%d]' % i) for i in range(6)])
        print('  events', {EVENT[i]: g(ex, 'event[%d]' % i) for i in range(11)
                           if g(ex, 'event[%d]' % i)},
              'event_ticks %s retuples %s | keys: events %s seen before %s distinct %s' % (
                  g(ex, 'event_ticks'), g(ex, 'retuples'), g(ex, 'key_events'),
                  g(ex, 'key_seen_before'), g(ex, 'key_distinct')))
        print('  why', dict(zip(('invalid', 'tint', 'fence', 'key'),
                                [g(ex, 'materialize_why[%d]' % i) for i in range(4)])),
              'variants learned %s switches %s evictions %s reject %s diff words %s max %s | tint repatches %s' % (
                  g(ex, 'variants_learned'), g(ex, 'variant_switches'), g(ex, 'variant_evictions'),
                  dict(zip(('other', 'shape', 'room', 'words'),
                           [g(ex, 'variant_reject[%d]' % i) for i in range(4)])),
                  g(ex, 'variant_diff_words'), g(ex, 'variant_diff_max'), '%s, rerecord resets %s' % (g(ex, 'tint_repatches'), g(ex, 'rerecord_resets'))),
              '| verify runs %s (variant %s) mismatch %s first %s' % (
                  g(ex, 'verify_runs'), g(ex, 'verify_variant_runs'),
                  dict(zip(('shape', 'words', 'textures', 'fences'),
                           [g(ex, 'verify_mismatch[%d]' % i) for i in range(4)])),
                  [hex(g(ex, 'verify_first[%d]' % i)) for i in range(4)]),
              '| arena chosen %s refine %s fails %s' % (
                  ex.get('gNdsTaskmanArenaChosenSize'), ex.get('gNdsTaskmanArenaRefineBytes'),
                  ex.get('gNdsTaskmanArenaAllocFailCount')))
        print('  oracle runs %s words %s mismatch %s clip max %s roots %s donor_memo %s '
              'unconsumed %s source_miss %s rec_under_hit %s record_diff %s first %s' % (
                  g(ex, 'oracle_runs'), g(ex, 'oracle_words'),
                  {OCLASS[i]: g(ex, 'oracle_mismatch[%d]' % i) for i in range(8)
                   if g(ex, 'oracle_mismatch[%d]' % i)},
                  [g(ex, 'oracle_clip_max[%d]' % i) for i in range(2)],
                  g(ex, 'oracle_clip_roots'), g(ex, 'oracle_donor_memo'),
                  g(ex, 'oracle_unconsumed'), g(ex, 'oracle_source_miss'),
                  [g(ex, 'oracle_record_under_hit[%d]' % i) for i in range(2)],
                  {OCLASS[i]: g(ex, 'oracle_record_diff[%d]' % i) for i in range(8)
                   if g(ex, 'oracle_record_diff[%d]' % i)},
                  [hex(g(ex, 'oracle_first[%d]' % i)) for i in range(8)]))
        print('  tint patch binds %s moved %s miss %s white %s shape %s; light %s '
              'texgen %s projection %s; pinned miss %s ident watch miss %s' % (
                  g(ex, 'tint_patch_binds'), g(ex, 'tint_patch_moved'),
                  g(ex, 'tint_patch_miss'), g(ex, 'tint_patch_white'),
                  g(ex, 'tint_patch_shape'), g(ex, 'light_patches'),
                  g(ex, 'texgen_patches'), g(ex, 'projection_patches'),
                  g(ex, 'pinned_residency_miss'), g(ex, 'ident_watch_miss')))
        draws = g(ex, 'draws') + g(ex, 'shadow_runs')
        for k, name in enumerate(KINDS):
            att = g(ex, 'k_attempts[%d]' % k)
            dr = g(ex, 'k_draws[%d]' % k)
            if not att:
                continue
            dec = {DECLINE[i]: g(ex, 'k_decline[%d][%d]' % (k, i))
                   for i in range(20)}
            ev = {EVENT[i]: g(ex, 'k_event[%d][%d]' % (k, i)) for i in range(11)}
            jc = {JCLASS[i]: g(ex, 'k_kernel_class[%d][%d]' % (k, i))
                  for i in range(8)}
            om = {OCLASS[i]: g(ex, 'k_oracle_mismatch[%d][%d]' % (k, i))
                  for i in range(8)}
            prog = {i: g(ex, 'k_program_draws[%d][%d]' % (k, i))
                    for i in range(16)}
            print('  %-6s attempts %5d draws %5d (%.2f%%) materializations %4d '
                  'max words %d high %d oracle_runs %d donor_memo %d' % (
                      name, att, dr, 100.0 * dr / att,
                      g(ex, 'k_materializations[%d]' % k),
                      g(ex, 'k_words_max[%d]' % k),
                      g(ex, 'k_high_draws[%d]' % k),
                      g(ex, 'k_oracle_runs[%d]' % k),
                      g(ex, 'k_oracle_donor_memo[%d]' % k)))
            print('         declines', {a: b for a, b in dec.items() if b})
            print('         events', {a: b for a, b in ev.items() if b},
                  'event ticks/event %s, materialize ticks/materialization %s' % (
                      per(g(ex, 'k_event_ticks[%d]' % k), sum(ev.values())),
                      per(g(ex, 'k_materialize_ticks[%d]' % k),
                          g(ex, 'k_materializations[%d]' % k))))
            print('         oracle mismatch', {a: b for a, b in om.items() if b})
            print('         keys: events %s seen before %s | key miss %s | why %s | variants learned %s '
                  'switches %s' % (
                      g(ex, 'k_key_events[%d]' % k), g(ex, 'k_key_seen_before[%d]' % k),
                      [g(ex, 'k_key_miss[%d][%d]' % (k, i)) for i in range(6)],
                      dict(zip(('invalid', 'tint', 'fence', 'key'),
                               [g(ex, 'k_why[%d][%d]' % (k, i)) for i in range(4)])),
                      g(ex, 'k_variants_learned[%d]' % k), g(ex, 'k_variant_switches[%d]' % k)))
            print('         programs', {a: b for a, b in prog.items() if b})
            jn = g(ex, 'k_kernel_joints[%d]' % k)
            print('         per draw: guard %s kernel %s patch %s submit %s dma_wait %s '
                  '| head %s | kernel/joint %s | classes %s' % (
                      per(g(ex, 'k_guard_ticks[%d]' % k), dr),
                      per(g(ex, 'k_kernel_ticks[%d]' % k), dr),
                      per(g(ex, 'k_patch_ticks[%d]' % k), dr),
                      per(g(ex, 'k_submit_ticks[%d]' % k), dr),
                      per(g(ex, 'k_dma_wait_ticks[%d]' % k), dr),
                      per(g(ex, 'k_head_ticks[%d]' % k), att),
                      per(g(ex, 'k_kernel_ticks[%d]' % k), jn),
                      {a: b for a, b in jc.items() if b}))
        print('  guard parts/draw', {GUARD[i]: per(g(ex, 'guard_part_ticks[%d]' % i), draws)
                                     for i in range(8)})
        print('  patch parts/draw', {PATCH[i]: per(g(ex, 'patch_part_ticks[%d]' % i), draws)
                                     for i in range(6)})
        print('  submit parts/draw', {SUBMIT[i]: per(g(ex, 'submit_part_ticks[%d]' % i), draws)
                                      for i in range(4)},
              'flush bytes/draw', per(g(ex, 'flush_bytes'), draws),
              'book/draw', per(g(ex, 'book_ticks'), g(ex, 'draws')))
        print('  heap free min %s libc top min %s | uploads after go %s admit fail %s' % (
            ex.get('gNdsTaskmanGeneralHeapFreeMin'), ex.get('gNdsTaskmanLibcTopChunkMin'),
            g(ex, 'fighter_uploads_after_go'), ex.get('gNdsFtrLeanAdmitFail')))


if __name__ == '__main__':
    main()
