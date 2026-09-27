"""Per-row (battle slot) lean counters of slice 6 run JSONs (run-s6.ps1 output).

Usage: kinds6.py ARM [ARM...]   (ARM = run name in the slice 6 artifact dir,
s5:NAME / s4:NAME for the slice 5 / 4 dirs, or a path without .json)

Slice 6: the k_* rows are battle slots; k_owner[row] = owner slot + 1 names
the kind each row drew (slice 3-5 ROMs: rows are Donkey, Samus, Link, Kirby).

Prints, per arm: engagement, events and materializations, and the per-draw
table per kind -- head, guard (with the event path shown apart: gNdsFtrLean's
k_guard_ticks include the event paths that ran inside the guard window),
kernel, patch, submit, book -- plus the slice 5 attribution block when the ROM
carries it (gNdsFtrLeanAttr, an NDS_FTR_LEAN_KTIME=1 build).
"""
import json
import os
import sys

OWNERS = ['Mario', 'Fox', 'Luigi', 'Donkey', 'Captain', 'Samus', 'Link',
          'Pikachu', 'Yoshi', 'Ness', 'Purin', 'Kirby']
KINDS = ['Donkey', 'Samus', 'Link', 'Kirby']  # slice 3-5 rows
DECLINE = ['kind', 'skeleton', 'camera', 'plan', 'validate', 'kirby_head',
           'roots', 'material', 'tables', 'policy', 'texture', 'capacity',
           'topology', 'kernel', 'texgen', 'tint', 'stale', 'inputs', 'alpha_test',
           '19']
EVENT = ['none', 'first', 'tuple', 'status', 'rebind', 'material', 'preamble',
         'tint_set', 'tint_tile', 'fence', 'kernel']
JCLASS = ['fast', 'nolocal', 'scale', 'warm', 'lock', 'convert', 'xobj',
          'nogobj']
GUARD = ['tuple', 'topology', 'camera', 'identity', 'preamble', 'packet',
         'shuffle', 'event+retuple']
PATCH = ['tint_tiles', 'apply_tint', 'P\'', 'light', 'texgen', 'refresh',
         'program', 'rest']
SUBMIT = ['flush', 'gx+dma_wait', 'dma+invalidate', 'stats']
HEAD = ['setup+lookat', 'head', 'walk', 'finish']
EVPART = ['resolve', 'program+validate', 'rows+key', 'refresh+ident+rr',
          'held', 'materialize', 'joints', 'watch+proofs']
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load(arm):
    base = HERE
    if arm.startswith('s4:'):
        base = HERE.replace('slice6', 'slice4')
        arm = arm[3:]
    elif arm.startswith('s5:'):
        base = HERE.replace('slice6', 'slice5')
        arm = arm[3:]
    path = arm if arm.endswith('.json') else arm + '.json'
    if not os.path.exists(path):
        path = os.path.join(base, path)
    d = json.load(open(path))
    return d, {e['name']: e['value'] for e in d['extras']}


def g(ex, name):
    return ex.get('gNdsFtrLean.' + name, 0) or 0


def a(ex, name):
    return ex.get('gNdsFtrLeanAttr.' + name, 0) or 0


def per(v, n):
    return '%.0f' % (v / n) if n else '-'


def main():
    for arm in sys.argv[1:]:
        d, ex = load(arm)
        attr = any(k.startswith('gNdsFtrLeanAttr.') for k in ex)
        print('==', arm, 'rom', d.get('romSha256', '')[:8], 'route',
              ex.get('gNdsFtrLeanRoute'), 'admit', ex.get('gNdsFtrLeanAdmit'),
              'slow', ex.get('gNdsFtrLeanSlow'), '(attribution ROM)' if attr else '')
        print('  attempts %s draws %s shadow %s materializations %s entry hits %s '
              'switches %s | packet hits %s records %s faults %s declines %s | '
              'native failures %s | draws(all) %s' % (
                  g(ex, 'attempts'), g(ex, 'draws'), g(ex, 'shadow_runs'),
                  g(ex, 'materializations'), g(ex, 'entry_hits'),
                  g(ex, 'entry_switches'), ex.get('gNdsFighterPacketHits'),
                  ex.get('gNdsFighterPacketRecords'),
                  ex.get('gNdsFighterPacketFaults'),
                  ex.get('gNdsFighterPacketDeclines'),
                  ex.get('gNdsRendererNativeFailure.count'),
                  ex.get('gNdsFighterMarioFoxDLAllDrawCount')))
        mats = g(ex, 'materializations')
        print('  materialize: %s ticks/materialization (%s list walk); max %s words; parts %s' % (
            per(g(ex, 'materialize_ticks'), mats), per(g(ex, 'materialize_list_ticks'), mats),
            g(ex, 'materialize_words_max'),
            {n: per(g(ex, 'materialize_part_ticks[%d]' % i), mats)
             for i, n in enumerate(('head', 'roots', 'spans', 'shade', 'prepare', 'corners', 'bind', 'words'))}))
        print('  events', {EVENT[i]: g(ex, 'event[%d]' % i) for i in range(11) if g(ex, 'event[%d]' % i)},
              'event_ticks %s retuples %s | variants learned %s switches %s reject %s | tint repatches %s '
              'rerecord resets %s | verify runs %s (variant %s) mismatch %s' % (
                  g(ex, 'event_ticks'), g(ex, 'retuples'), g(ex, 'variants_learned'),
                  g(ex, 'variant_switches'),
                  [g(ex, 'variant_reject[%d]' % i) for i in range(4)],
                  g(ex, 'tint_repatches'), g(ex, 'rerecord_resets'), g(ex, 'verify_runs'),
                  g(ex, 'verify_variant_runs'), [g(ex, 'verify_mismatch[%d]' % i) for i in range(4)]))
        print('  tint re-records: old path per slot %s, lean mirrored %s | fence re-records mirrored %s' % (
            [g(ex, 'tint_rerecords[%d]' % i) for i in range(4)], g(ex, 'tint_rerecords_mirrored'),
            g(ex, 'fence_rerecords_mirrored')))
        if 'gNdsFtrLean.plan_reuses' in ex:
            print('  kept plans (material events): reuses %s restarts %s | verify plan runs %s mismatch %s' % (
                g(ex, 'plan_reuses'), g(ex, 'plan_reuse_restarts'), g(ex, 'verify_plan_runs'),
                [g(ex, 'verify_plan_mismatch[%d]' % i) for i in range(3)]))
        print('  oracle runs %s words %s mismatch %s clip max %s donor_memo %s unconsumed %s '
              'source_miss %s rec_under_hit %s record_diff %s' % (
                  g(ex, 'oracle_runs'), g(ex, 'oracle_words'),
                  [g(ex, 'oracle_mismatch[%d]' % i) for i in range(8)],
                  [g(ex, 'oracle_clip_max[%d]' % i) for i in range(2)],
                  g(ex, 'oracle_donor_memo'), g(ex, 'oracle_unconsumed'),
                  g(ex, 'oracle_source_miss'),
                  [g(ex, 'oracle_record_under_hit[%d]' % i) for i in range(2)],
                  [g(ex, 'oracle_record_diff[%d]' % i) for i in range(8)]))
        print('  light patches %s texgen %s projection %s pre checks %s skips %s flush bytes %s '
              'ident watch miss %s kernel fail %s rebuilds %s' % (
                  g(ex, 'light_patches'), g(ex, 'texgen_patches'), g(ex, 'projection_patches'),
                  g(ex, 'pre_checks'), g(ex, 'pre_skips'), g(ex, 'flush_bytes'),
                  g(ex, 'ident_watch_miss'), g(ex, 'kernel_fail'), g(ex, 'kernel_rebuilds')))
        draws = g(ex, 'draws') + g(ex, 'shadow_runs')
        print('  per draw, all kinds: guard parts', {GUARD[i]: per(g(ex, 'guard_part_ticks[%d]' % i), draws)
                                                     for i in range(8)})
        print('                       patch parts', {PATCH[i]: per(g(ex, 'patch_part_ticks[%d]' % i), draws)
                                                     for i in range(5)},
              '| submit parts', {SUBMIT[i]: per(g(ex, 'submit_part_ticks[%d]' % i), draws) for i in range(4)},
              '| book', per(g(ex, 'book_ticks'), g(ex, 'draws')))
        print('  %-7s %6s %7s | %6s %6s %6s %6s %6s %6s %6s | %6s | %7s %7s %7s' % (
            'kind', 'draws', 'mats', 'head', 'guard', 'event', 'g-net', 'kernel', 'patch', 'submit',
            'sum*', 'k/joint', 'joints', 'ev/draw'))
        for k in range(4):
            own = g(ex, 'k_owner[%d]' % k)
            name = OWNERS[own - 1] if 1 <= own <= 12 else (KINDS[k] if 'gNdsFtrLean.k_owner[0]' not in ex else 'slot%d' % k)
            dr = g(ex, 'k_draws[%d]' % k)
            att = g(ex, 'k_attempts[%d]' % k)
            if not att:
                continue
            jn = g(ex, 'k_kernel_joints[%d]' % k)
            head = g(ex, 'k_head_ticks[%d]' % k) / att
            guard = g(ex, 'k_guard_ticks[%d]' % k) / dr if dr else 0
            ev = g(ex, 'k_event_ticks[%d]' % k) / dr if dr else 0
            kern = g(ex, 'k_kernel_ticks[%d]' % k) / dr if dr else 0
            patch = g(ex, 'k_patch_ticks[%d]' % k) / dr if dr else 0
            sub = g(ex, 'k_submit_ticks[%d]' % k) / dr if dr else 0
            print('  %-7s %6d %7d | %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f %6.0f | %6.0f | %7s %7.1f %7.3f' % (
                name, dr, g(ex, 'k_materializations[%d]' % k), head, guard, ev, guard - ev, kern, patch, sub,
                guard + kern + patch + sub, per(g(ex, 'k_kernel_ticks[%d]' % k), jn), jn / dr if dr else 0,
                sum(g(ex, 'k_event[%d][%d]' % (k, i)) for i in range(11)) / dr if dr else 0))
            print('          events', {EVENT[i]: g(ex, 'k_event[%d][%d]' % (k, i)) for i in range(11)
                                       if g(ex, 'k_event[%d][%d]' % (k, i))},
                  'event ticks/event', per(g(ex, 'k_event_ticks[%d]' % k),
                                           sum(g(ex, 'k_event[%d][%d]' % (k, i)) for i in range(11))),
                  'materialize ticks/mat', per(g(ex, 'k_materialize_ticks[%d]' % k),
                                               g(ex, 'k_materializations[%d]' % k)),
                  'declines', {DECLINE[i]: g(ex, 'k_decline[%d][%d]' % (k, i)) for i in range(20)
                               if g(ex, 'k_decline[%d][%d]' % (k, i))},
                  'classes', {JCLASS[i]: g(ex, 'k_kernel_class[%d][%d]' % (k, i)) for i in range(8)
                              if g(ex, 'k_kernel_class[%d][%d]' % (k, i))},
                  'variants', g(ex, 'k_variants_learned[%d]' % k), g(ex, 'k_variant_switches[%d]' % k),
                  'high draws', g(ex, 'k_high_draws[%d]' % k),
                  'programs', {i: g(ex, 'k_program_draws[%d][%d]' % (k, i)) for i in range(16)
                               if g(ex, 'k_program_draws[%d][%d]' % (k, i))},
                  'words max', g(ex, 'k_words_max[%d]' % k))
            if g(ex, 'k_oracle_runs[%d]' % k) or any(g(ex, 'k_oracle_mismatch[%d][%d]' % (k, i)) for i in range(8)):
                print('          oracle runs', g(ex, 'k_oracle_runs[%d]' % k), 'mismatch',
                      [g(ex, 'k_oracle_mismatch[%d][%d]' % (k, i)) for i in range(8)],
                      'donor_memo', g(ex, 'k_oracle_donor_memo[%d]' % k))
            if attr:
                print('          head parts', {HEAD[i]: per(a(ex, 'head_part_ticks[%d][%d]' % (k, i)), att)
                                               for i in range(4)})
                print('          guard parts', {GUARD[i]: per(a(ex, 'guard_part_ticks[%d][%d]' % (k, i)), dr)
                                                for i in range(8)})
                pp = [a(ex, 'patch_part_ticks[%d][%d]' % (k, i)) for i in range(8)]
                pp[7] = g(ex, 'k_patch_ticks[%d]' % k) - sum(pp[:7])
                print('          patch parts', {PATCH[i]: per(pp[i], dr) for i in range(8)})
                print('          submit parts', {SUBMIT[i]: per(a(ex, 'submit_part_ticks[%d][%d]' % (k, i)), dr)
                                                 for i in range(4)},
                      'book', per(a(ex, 'book_ticks[%d]' % k), dr))
                ad = a(ex, 'kernel_attr_draws[%d]' % k)
                aj = a(ex, 'kernel_attr_joints[%d]' % k)
                if ad:
                    print('          kernel re-runs: draws %d, cold %s/joint (whole pass), warm %s/joint, '
                          'dcold %s/joint (first %d joints/draw)' % (
                              ad, per(a(ex, 'kernel_cold_ticks[%d]' % k), jn * ad / dr if dr else 0),
                              per(a(ex, 'kernel_warm_ticks[%d]' % k), aj),
                              per(a(ex, 'kernel_dcold_ticks[%d]' % k), aj), aj // ad))
                qd = a(ex, 'quiet_draws[%d]' % k)
                if qd:
                    print('          quiet: draws %d, DMA drain %s/draw' % (
                        qd, per(a(ex, 'quiet_wait_ticks[%d]' % k), qd)))
                pj = a(ex, 'kernel_part_joints[%d]' % k)
                if pj:
                    print('          kernel parts/joint', {n: per(a(ex, 'kernel_part_ticks[%d][%d]' % (k, i)), pj)
                                                         for i, n in enumerate(('fast local', 'compose', 'output',
                                                                                'slow local'))})
        if attr:
            print('  event path steps (ticks/step, count)',
                  {EVPART[i]: (per(a(ex, 'event_part_ticks[%d]' % i), a(ex, 'event_part_count[%d]' % i)),
                               a(ex, 'event_part_count[%d]' % i)) for i in range(8)})
        print('  heap free min %s libc top min %s arena %s | memo hits %s fills %s bypass %s | uploads after go %s '
              'admit fail %s' % (
                  ex.get('gNdsTaskmanGeneralHeapFreeMin'), ex.get('gNdsTaskmanLibcTopChunkMin'),
                  ex.get('gNdsTaskmanArenaChosenSize'), ex.get('gNdsFtrDrawMemoHits'),
                  ex.get('gNdsFtrDrawMemoFills'), ex.get('gNdsFtrDrawMemoBypass'),
                  g(ex, 'fighter_uploads_after_go'), ex.get('gNdsFtrLeanAdmitFail')))
        print('  * sum = guard (incl. event paths) + kernel + patch + submit, per lean draw')


if __name__ == '__main__':
    main()
