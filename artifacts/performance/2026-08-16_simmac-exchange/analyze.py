# The warm-MAC exchange rate, from six same-binary shadow arms.
#
# Every arm is build-c209-simmac2 with one .data word poked, so there is no
# placement term and the cross-build floors do not apply. What DOES survive is
# the cartridge-read frames (CAMERA_Q20_12.md 3.2): the paired delta's extremes
# reach +/-150k on frames that are not reproducible between emulator sessions.
# The driving seam fires 0.95 times a frame in BURSTS (a hitbox has to be live),
# so most frames carry no evaluation at all and the paired MEDIAN is structurally
# zero -- the paired MEAN is the statistic here, with a trimmed mean beside it so
# the cartridge frames can be seen not to be carrying it.
import csv, json, math, os, statistics, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ARMS = ['r0', 'r64tc', 'r64t', 'r16tc', 'r1tc', 'rgrade']
REQUIREMENT = 94481          # build-c206-shipgx0, rank-80 1,239,808 raw / 1,214,861 net
APPARATUS = 24947
GATE = 1120380


def load(name):
    with open(os.path.join(HERE, name + '.json'), encoding='utf-8') as fh:
        js = json.load(fh)
    rows = {}
    with open(os.path.join(HERE, name + '-rows.csv'), newline='') as fh:
        for row in csv.DictReader(fh):
            rows[int(row['frame'])] = {k: int(v) for k, v in row.items()
                                       if k != 'frame'}
    return js, rows


def extras(js):
    return {e['name']: e['value'] for e in js['extras']}


def trimmed(values, drop):
    s = sorted(values)
    return statistics.mean(s[drop:len(s) - drop])


def main():
    data = {}
    for name in ARMS:
        try:
            data[name] = load(name)
        except FileNotFoundError:
            print(f"(missing arm {name})")
    if 'r0' not in data:
        return 1

    print("=== provenance ===")
    shas = set()
    for name, (js, _) in data.items():
        shas.add(js['romSha256'])
        sg = js['setGlobals'][0]
        print(f"{name:8} arm={sg['requested']:6} readback={sg['readback']:6} "
              f"stuck={sg['stuck']}  samples={js['samples']} "
              f"frames={js['startFrame']}..{js['endFrame']} "
              f"slips={js['cadenceViolations']}")
    print(f"romSha256 set size = {len(shas)}  "
          f"({'SAME BINARY, no placement term' if len(shas) == 1 else 'MIXED BINARIES'})")

    print("\n=== the arms must play the SAME FIGHT: whole-match invariants ===")
    inv = ['gNdsBattleTextHudP1Damage', 'gNdsDamageSparkScaleCount',
           'gNdsShieldAnimJointAttachCount', 'gNdsAObjEvent32NormalizedHighWater',
           'gNdsBattlePackHits', 'gNdsObjAnimRunawayCount',
           'gNdsRendererTask36CaptureOutcome', 'gNdsRendererTask36CaptureSegmentMask',
           'gNdsR2FtAnimParseCalls', 'gNdsR2FtAnimParseEarlyOut',
           'gNdsR2FtAnimParseStepped', 'gNdsR2FtAnimNullSkips',
           'gNdsTaskmanGeneralHeapFreeMin', 'gNdsTaskmanArenaChosenSize',
           'gNdsTaskmanArenaAllocFailCount', 'gNdsBattlePackResidentBytes',
           'gNdsBattlePackLoadFails', 'gNdsCfxFighterDamagePhaseCalls',
           'gNdsCfxFighterDamagePhaseHits']
    base = extras(data['r0'][0])
    bad = 0
    for key in inv:
        vals = {n: extras(js)[key] for n, (js, _) in data.items() if key in extras(js)}
        same = len(set(vals.values())) == 1
        if not same:
            bad += 1
            print(f"  !! {key}: {vals}")
    print(f"  {len(inv) - bad}/{len(inv)} invariants bit-identical across "
          f"{len(data)} arms" + ("  -- INSTRUMENT BROKEN" if bad else "  -- OK"))

    print("\n=== engagement ===")
    hdr = ['DriveCalls', 'XfrmCalls', 'XfrmShadow', 'XfrmDecline',
           'CmpsCalls', 'CmpsShadow', 'CmpsDecline', 'XfrmMaxDevQ12']
    print(f"{'arm':8}" + "".join(f"{h:>14}" for h in hdr))
    for name in ARMS:
        if name not in data:
            continue
        ex = extras(data[name][0])
        print(f"{name:8}" + "".join(
            f"{ex.get('gNdsR2SimMac' + h, 0):>14,}" for h in hdr))

    print("\n=== in-window evaluation rate, from the per-ring-stop reads ===")
    rate = {}
    for name in ARMS:
        if name not in data:
            continue
        js, _ = data[name]
        reads = js.get('ringStopReads')
        if not reads:
            print(f"{name:8} (no ringStopReads)")
            continue
        first, last = reads[0], reads[-1]
        span = last['frame'] - first['frame']
        line = f"{name:8} frames {first['frame']}..{last['frame']} (span {span})"
        rate[name] = {}
        for key in ('gNdsR2SimMacDriveCalls', 'gNdsR2SimMacXfrmShadow',
                    'gNdsR2SimMacCmpsShadow'):
            fv = first.get(key)
            lv = last.get(key)
            if (fv is None) or (lv is None) or (span <= 0):
                continue
            rate[name][key] = (lv - fv) / span
            line += f"  {key.replace('gNdsR2SimMac',''):11}={rate[name][key]:9.3f}/fr"
        print(line)

    print("\n=== paired per-frame WORK-H against r0 ===")
    _, r0 = data['r0']
    print(f"{'arm':8}{'pairedMean':>12}{'trimMean40':>12}{'pairedMed':>11}"
          f"{'B>A':>7}{'B<A':>7}{'P50 d':>10}{'rank80 d':>11}")
    for name in ARMS[1:]:
        if name not in data:
            continue
        _, rn = data[name]
        frames = sorted(set(r0) & set(rn))
        a = [r0[f]['WORK-H'] for f in frames]
        b = [rn[f]['WORK-H'] for f in frames]
        d = [y - x for x, y in zip(a, b)]
        ra = sorted(a, reverse=True)[79]
        rb = sorted(b, reverse=True)[79]
        print(f"{name:8}{statistics.mean(d):12,.0f}{trimmed(d, 40):12,.0f}"
              f"{statistics.median(d):11,.0f}"
              f"{sum(1 for x in d if x > 0):7d}{sum(1 for x in d if x < 0):7d}"
              f"{statistics.median(b) - statistics.median(a):10,.0f}"
              f"{rb - ra:11,.0f}")

    print("\n=== cost per evaluation ===")
    print("TWO estimators, and only the second one is quotable.")
    print("  trim-40 is WRONG here and is printed to show that it is: the driving")
    print("  seam fires in BURSTS, so trimming 2.5% off each tail deletes exactly")
    print("  the frames that carry the evaluations. It disagrees with itself across")
    print("  arms by 10x. The price comes from window-regression.txt, whose 96-frame")
    print("  windows each carry their own exact evaluation count.")

    def window_sum(name):
        js, rn = data[name]
        frames = sorted(set(r0) & set(rn))
        total_d = 0
        total_ev = 0
        total_fr = 0
        for rec in (js.get('ringStopReads') or [])[1:]:
            lo, hi = rec['fromFrame'] + 1, rec['frame']
            fs = [f for f in frames if lo <= f <= hi]
            if not fs:
                continue
            total_d += sum(rn[f]['WORK-H'] - r0[f]['WORK-H'] for f in fs)
            total_ev += (rec['gNdsR2SimMacXfrmShadowDelta'] +
                         rec['gNdsR2SimMacCmpsShadowDelta'])
            total_fr += len(fs)
        return total_d, total_ev, total_fr

    def evals(name, key):
        return rate.get(name, {}).get(key, 0.0)

    for name in ARMS[1:]:
        if name not in data:
            continue
        _, rn = data[name]
        frames = sorted(set(r0) & set(rn))
        bad = trimmed([rn[f]['WORK-H'] - r0[f]['WORK-H'] for f in frames], 40)
        d, ev, fr = window_sum(name)
        xf = evals(name, 'gNdsR2SimMacXfrmShadow')
        cm = evals(name, 'gNdsR2SimMacCmpsShadow')
        tot = xf + cm
        print(f"{name:8} windows: {d / fr if fr else 0:9,.0f} tk/fr over "
              f"{tot:7.2f} eval/fr -> {d / ev if ev else float('nan'):8.1f} tk/eval"
              f"   [trim-40 would say {bad / tot if tot else 0:8.1f} -- do not use]")

    print("\n=== exchange rate, gross from SIMSIDE.md section 3 per-call rates ===")
    print("gross transform 289.9 tk/entry (9 fmul @13.23 + 9 fadd @18.98); "
          "SIMSIDE 13,091/45.2 = 289.6")
    print("gross compose   988.8 tk/entry (36 fmul + 27 fadd);            "
          "SIMSIDE 18,759/19.0 = 987.3")
    return 0


sys.exit(main())
