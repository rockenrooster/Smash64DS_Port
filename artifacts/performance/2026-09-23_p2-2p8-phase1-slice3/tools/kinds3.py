"""Per-kind lean counters of one slice 3 run JSON (run-s3.ps1 output).

Usage: kinds3.py ARM [ARM...]   (ARM = run name in the artifact dir, or a path
without .json)
"""
import json
import os
import sys

KINDS = ['Donkey', 'Samus', 'Link', 'Kirby']
DECLINE = ['kind', 'adopt_pending', 'tuple', 'animlock', 'camera', 'material',
           'preamble', 'tint_set', 'residency', 'fence', 'kernel', 'tint',
           'topology', 'skeleton', 'rebind', 'events', 'kirby_head', 'texgen',
           'uncacheable', '19']
REFUSE = ['ok', 'invalid', 'shape', 'texgen', 'tinted', 'capacity', 'topology',
          'projection_index', 'plan', 'roots']
JCLASS = ['fast', 'nolocal', 'scale', 'warm', 'lock', 'convert', 'xobj',
          'nogobj']
OCLASS = ['proj', 'basis', 'row3', 'shade', 'light', 'other', 'tint', 'texgen']
GUARD = ['tuple', 'topology', 'camera', 'identity', 'preamble', 'packet',
         'shuffle', 'texgen_sel']
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
    return ex.get('gNdsFtrLean.' + name, 0)


def per(v, n):
    return '%.0f' % (v / n) if n else '-'


def main():
    for arm in sys.argv[1:]:
        d, ex = load(arm)
        print('==', arm, 'rom', d.get('romSha256', '')[:8], 'route',
              ex.get('gNdsFtrLeanRoute'), 'admit', ex.get('gNdsFtrLeanAdmit'),
              'slow', ex.get('gNdsFtrLeanSlow'))
        print('  attempts %s draws %s shadow %s adopts %s | packet hits %s '
              'records %s faults %s | native failures %s | draws(all) %s' % (
                  g(ex, 'attempts'), g(ex, 'draws'), g(ex, 'shadow_runs'),
                  g(ex, 'adopts'), ex.get('gNdsFighterPacketHits'),
                  ex.get('gNdsFighterPacketRecords'),
                  ex.get('gNdsFighterPacketFaults'),
                  ex.get('gNdsRendererNativeFailure.count'),
                  ex.get('gNdsFighterMarioFoxDLAllDrawCount')))
        print('  missword', [ex.get('gNdsFighterPacketMissWord[%d]' % i, 0)
                             for i in range(8)])
        for k, name in enumerate(KINDS):
            att = g(ex, 'k_attempts[%d]' % k)
            dr = g(ex, 'k_draws[%d]' % k)
            if not att:
                continue
            dec = {DECLINE[i]: g(ex, 'k_decline[%d][%d]' % (k, i))
                   for i in range(20)}
            ref = {REFUSE[i]: g(ex, 'k_adopt_refuse[%d][%d]' % (k, i))
                   for i in range(10)}
            jc = {JCLASS[i]: g(ex, 'k_kernel_class[%d][%d]' % (k, i))
                  for i in range(8)}
            om = {OCLASS[i]: g(ex, 'k_oracle_mismatch[%d][%d]' % (k, i))
                  for i in range(8)}
            prog = {i: g(ex, 'k_program_draws[%d][%d]' % (k, i))
                    for i in range(16)}
            print('  %-6s attempts %5d draws %5d (%.1f%%) adopts %4d high %d '
                  'oracle_runs %d' % (
                      name, att, dr, 100.0 * dr / att, g(ex, 'k_adopts[%d]' % k),
                      g(ex, 'k_high_draws[%d]' % k),
                      g(ex, 'k_oracle_runs[%d]' % k)))
            print('         declines', {a: b for a, b in dec.items() if b})
            print('         refusals', {a: b for a, b in ref.items() if b})
            print('         programs', {a: b for a, b in prog.items() if b})
            joints = g(ex, 'k_kernel_joints[%d]' % k)
            print('         joints/draw %s classes %s' % (
                per(joints, dr), {a: b for a, b in jc.items() if b}))
            print('         per draw: head %s guard %s kernel %s patch %s '
                  'submit %s dmawait %s | kernel/joint %s' % (
                      per(g(ex, 'k_head_ticks[%d]' % k), att),
                      per(g(ex, 'k_guard_ticks[%d]' % k), dr),
                      per(g(ex, 'k_kernel_ticks[%d]' % k), dr),
                      per(g(ex, 'k_patch_ticks[%d]' % k), dr),
                      per(g(ex, 'k_submit_ticks[%d]' % k), dr),
                      per(g(ex, 'k_dma_wait_ticks[%d]' % k), dr),
                      per(g(ex, 'k_kernel_ticks[%d]' % k), joints)))
            print('         oracle mismatches', {a: b for a, b in om.items() if b})
        n = g(ex, 'draws') or g(ex, 'shadow_runs')
        print('  guard parts/draw', {GUARD[i]: per(g(ex, 'guard_part_ticks[%d]' % i), n)
                                     for i in range(8)})
        print('  patch parts/draw', {PATCH[i]: per(g(ex, 'patch_part_ticks[%d]' % i), n)
                                     for i in range(6)})
        dn = g(ex, 'draws')
        print('  submit parts/draw', {SUBMIT[i]: per(g(ex, 'submit_part_ticks[%d]' % i), dn)
                                      for i in range(4)},
              'book/draw', per(g(ex, 'book_ticks'), dn),
              'flush B/draw', per(g(ex, 'flush_bytes'), dn))
        print('  oracle: runs %s words %s source_miss %s unconsumed %s fence_rekey %s' % (
            g(ex, 'oracle_runs'), g(ex, 'oracle_words'), g(ex, 'oracle_source_miss'),
            g(ex, 'oracle_unconsumed'), g(ex, 'oracle_fence_rekey')))
        print('         mismatch', [g(ex, 'oracle_mismatch[%d]' % i) for i in range(8)],
              'key_moved', [g(ex, 'oracle_key_moved[%d]' % i) for i in range(8)],
              'rec_under_hit', [g(ex, 'oracle_record_under_hit[%d]' % i) for i in range(2)],
              'rec_diff', [g(ex, 'oracle_record_diff[%d]' % i) for i in range(9)],
              'max_lsb', [g(ex, 'oracle_max_lsb[%d]' % i) for i in range(3)])
        print('  identity: watch_miss %s last adopt watched %s | all-admitted adoptions %s | retuples %s rekeys %s | plan verify runs %s mismatch %s hits %s builds %s' % (
            g(ex, 'ident_watch_miss'), g(ex, 'adopt_watch'), g(ex, 'adopt_all_pinned'), g(ex, 'retuples'), g(ex, 'rekeys'),
            ex.get('gNdsFtrPlanVerifyRuns'), ex.get('gNdsFtrPlanVerifyMismatch'), ex.get('gNdsFtrPlanHit'), ex.get('gNdsFtrPlanBuild')))
        print('  adoption: ticks/adopt %s copy/adopt %s kept %s | projection patches %s | variants learned %s (words %s) switches %s learn-fail %s learn ticks/learn %s | retuple ticks/retuple %s' % (per(g(ex, 'adopt_ticks'), g(ex, 'adopts')), per(g(ex, 'adopt_copy_ticks'), g(ex, 'adopts')), g(ex, 'adopt_kept'), g(ex, 'projection_patches'), g(ex, 'variant_learns'), g(ex, 'variant_words'), g(ex, 'variant_switches'), g(ex, 'variant_learn_fail'), per(g(ex, 'variant_learn_ticks'), g(ex, 'variant_learns') + g(ex, 'variant_learn_fail')), per(g(ex, 'retuple_ticks'), g(ex, 'retuples'))))
        print('  misc: pre_checks %s pre_skips %s light_patches %s texgen_patches %s '
              'pinned_miss %s kernel_fail %s (rebuilt %s) tint binds %s moved %s miss %s white %s shape %s' % (
                  g(ex, 'pre_checks'), g(ex, 'pre_skips'), g(ex, 'light_patches'),
                  g(ex, 'texgen_patches'), g(ex, 'pinned_residency_miss'),
                  g(ex, 'kernel_fail'), g(ex, 'kernel_rebuilds'), g(ex, 'tint_patch_binds'), g(ex, 'tint_patch_moved'),
                  g(ex, 'tint_patch_miss'), g(ex, 'tint_patch_white'), g(ex, 'tint_patch_shape')))
        kj = g(ex, 'kernel_part_joints')
        if kj:
            print('  kernel parts/joint (ktime lab)', {n: per(g(ex, 'kernel_part_ticks[%d]' % i), kj) for i, n in enumerate(['fast_local', 'compose', 'output', 'slow_local'])})
        print('  dma: lean waits %s ticks %s | next-writer waits %s ticks %s | ge busy %s/%s' % (
            g(ex, 'dma_wait_spins'), g(ex, 'dma_wait_ticks'), g(ex, 'packet_dma_waits'),
            g(ex, 'packet_dma_wait_ticks'), g(ex, 'ge_busy_hits'), g(ex, 'ge_busy_samples')))
        print('  heap free min %s libc top min %s uploads after GO %s' % (
            ex.get('gNdsTaskmanGeneralHeapFreeMin'), ex.get('gNdsTaskmanLibcTopChunkMin'),
            g(ex, 'fighter_uploads_after_go')))


if __name__ == '__main__':
    main()
