[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RowsCsv,
    [Parameter(Mandatory=$true)][string]$Arm,
    # PROJECT_GOAL.md's per-presented-frame budget. Frames above it are the
    # "hot" set; the rest are "clean".
    [double]$Gate = 1120380,
    # Loading states are excluded from the gate by the owner's stated rule:
    # drop frames whose SRC exceeds this multiple of the arm's OWN SRC median.
    # 0 disables the exclusion.
    [double]$LoadFrameSrcMultiple = 2.0,
    # Also print the lane-ceiling table: the most each lane could ever pay if it
    # were perfectly flattened to its own median. Off by default because the
    # excursion table above answers "who owns the tail" and the ceiling answers
    # the different question "how big is the prize" -- but the two belong in one
    # place, because the c125 ceilings were computed by hand, in a doc, on the
    # WRONG ARM, and nothing could re-derive them when that was discovered.
    [switch]$Ceilings,
    [string]$JsonOut = ''
)

# WHO OWNS THE TAIL -- mean on over-gate frames minus mean on clean frames.
#
# The metric is an EXCURSION, not a mean, and the difference decides lanes: a
# bucket that is large but flat costs P50 and cannot move the gate, while a
# bucket that is small on clean frames and large on hot ones is the gate lever
# however modest its average. Ranking by mean has twice sent this campaign after
# a flat owner.
#
# THE COMPLETENESS CHECK IS THE POINT. The eleven tick-HUD buckets satisfy
#
#   WORK-H = (FTR + STG + BG + AUD + SRC + MISC) + (OTHR - WAIT)
#
# exactly, frame by frame, so the per-owner excursions must sum to the WORK-H
# hot-minus-clean delta. If they do not, the attribution has missed something
# and no share in the table means anything. This script fails on that rather
# than printing an incomplete ranking, because an incomplete ranking is exactly
# what a plausible-looking wrong answer looks like.
#
# OTHR IS NOT A REGION, it is an accounting remainder: taskman_seam.c computes
# it as `all - named` and `named` excludes WAIT, so OTHR still contains VBlank
# idle. Ranking OTHR beside WAIT double-counts the same idle time and already
# cost one retracted headline. Only `OTHR - WAIT`, the true unattributed work,
# appears here -- and it has measured flat (~19,159 ticks/frame) on both arms.
#
# HUD is the instrument's own cost and is excluded from WORK-H by construction,
# which is why WORK-H rather than WORK is the series being explained.

$ErrorActionPreference = 'Stop'

$owners = @('FTR', 'STG', 'BG', 'AUD', 'SRC', 'MISC')
$rows = @(Import-Csv -LiteralPath $RowsCsv)
if ($rows.Count -eq 0) { throw "No rows in $RowsCsv." }

# SRC SUB-OWNERS (cycle 85). SRC is 68.9% of the both-CPU gate arm's excursion
# and a residual cannot be optimised, so builds carrying the SHDT/SWRM ring
# buckets split it three ways:
#
#   SHDT  ftMainProcSearchHitAll      live-hitbox hit detection
#   SWRM  ndsR2AnimCachePreloadStep   anim-cache warm step (the one load in SRC)
#   SBAS  SRC - SHDT - SWRM           the decomp sim path, derived not ringed
#
# SBAS is a residual on purpose: it costs no bytes on a ROM 96 bytes from a boot
# cliff when this was designed, and SBAS >= 0 on every frame is the proof that
# the two ringed spans really are nested inside SRC. If a frame goes negative the
# bracket is outside SRC and the split is meaningless, so that throws.
#
# Detected, not required: every banked CSV predates these columns and must still
# analyse, because reproducing the banked table is how this script gets trusted.
#
# SBAS SPLIT (cycle 86). SBAS carried 61.5% of the gate arm's WORK-H excursion,
# so it is split in turn. The composition was verified from source before the
# brackets were placed: SRC brackets ndsTask39EffectsUpdate + scVSBattleFuncUpdate,
# and the decomp scene update reaches gcRunAll (ifcommon.c:2970), which is the
# SOLE gateway to the simulation. The fighter proc chain is ftmanager.c:858-863.
#
#   GCRA  gcRunAll                     the entire simulation (ringed)
#   SCPU  ftComputerProcessAll         level-3 CPU AI, inside the interrupt proc
#   SCAT  ftMainProcSearchCatch        grab/catch search, proc priority 2
#   SPRM  ftMainProcParams             anim/event/status proc, priority 0
#   SOUT  SRC - SWRM - GCRA            work in SRC OUTSIDE the simulation (derived)
#   SGCO  GCRA - SCPU - SCAT - SHDT - SPRM   unattributed inside the sim (derived)
#
# The seven owners SHDT/SWRM/SOUT/SCPU/SCAT/SPRM/SGCO are DISJOINT and partition
# SRC exactly, which is what the closure check below verifies. Note SHDT nests
# inside GCRA, so it is subtracted there and never double-counted; the cycle-85
# roll-up SBAS = SRC - SHDT - SWRM is reported as SOUT+SCPU+SCAT+SPRM+SGCO.
# Both derived residuals are non-negative by construction if and only if the
# nesting is real, so both throw -- the same falsifier, one level deeper.
# SGCO SPLIT (cycle 92). SGCO carried 153,291 of the gate arm's WORK-H excursion
# (52.0%, and 73.6% of SRC) and 81.3% of Boundary's SRC excursion. Same reasoning
# one level deeper: it is a residual, and a residual cannot be optimised. The
# three remaining per-fighter procs (ftmanager.c:858-860) are ringed, which
# leaves the non-fighter GObjs as the only unnamed part:
#
#   SINT  ftMainProcUpdateInterrupt    proc priority 5, SCPU nested INSIDE it
#   SPHD  ftMainProcPhysicsMapDefault  proc priority 4, uncaptured physics arm
#   SPHC  ftMainProcPhysicsMapCapture  proc priority 3, captured physics arm
#   SITR  SINT - SCPU                  the interrupt proc less its AI (derived)
#   SOBJ  GCRA - SINT - SPHD - SPHC - SCAT - SHDT - SPRM
#                                      camera/effects/items/weapons/interface
#                                      GObjs + gcRunAll's own two dispatch loops
#
# SGCO = SITR + SPHD + SPHC + SOBJ exactly, so it is still reported (as a
# roll-up) and stays comparable against the banked 153,291 -- that equality is
# the regression check on this script before it is trusted on new data. The
# partition below therefore replaces SGCO with its four parts and stays exact.
$csvColumns = @($rows[0].PSObject.Properties.Name)
$hasSrcSplit = ($csvColumns -contains 'SHDT') -and ($csvColumns -contains 'SWRM')
$hasSbasSplit = $hasSrcSplit -and
    (@('GCRA', 'SCPU', 'SCAT', 'SPRM') |
        Where-Object { $csvColumns -notcontains $_ }).Count -eq 0
$hasSgcoSplit = $hasSbasSplit -and
    (@('SINT', 'SPHD', 'SPHC') |
        Where-Object { $csvColumns -notcontains $_ }).Count -eq 0
$srcSubOwners = if ($hasSgcoSplit) {
    @('SHDT', 'SWRM', 'SOUT', 'SCPU', 'SCAT', 'SPRM',
      'SITR', 'SPHD', 'SPHC', 'SOBJ')
} elseif ($hasSbasSplit) {
    @('SHDT', 'SWRM', 'SOUT', 'SCPU', 'SCAT', 'SPRM', 'SGCO')
} else { @('SHDT', 'SWRM', 'SBAS') }

function Get-Pct {
    param([int64[]]$Values, [double]$Q)
    $sorted = @($Values | Sort-Object)
    return [int64]$sorted[[int][Math]::Floor(($sorted.Count - 1) * $Q)]
}

# Project once into plain numeric records; Import-Csv yields strings, and a
# string comparison against the gate silently ranks "999" above "1120380".
$all = @($rows | ForEach-Object {
    $r = [ordered]@{ frame = [int64]$_.frame }
    foreach ($b in @('ALL') + $owners + @('OTHR', 'WAIT', 'WORK', 'WORK-H')) {
        $r[$b] = [int64]$_.$b
    }
    $r['OTHR-WAIT'] = $r['OTHR'] - $r['WAIT']
    if ($hasSrcSplit) {
        $r['SHDT'] = [int64]$_.SHDT
        $r['SWRM'] = [int64]$_.SWRM
        $r['SBAS'] = $r['SRC'] - $r['SHDT'] - $r['SWRM']
    }
    if ($hasSbasSplit) {
        $r['GCRA'] = [int64]$_.GCRA
        $r['SCPU'] = [int64]$_.SCPU
        $r['SCAT'] = [int64]$_.SCAT
        $r['SPRM'] = [int64]$_.SPRM
        $r['SOUT'] = $r['SRC'] - $r['SWRM'] - $r['GCRA']
        $r['SGCO'] = $r['GCRA'] - $r['SCPU'] - $r['SCAT'] -
                     $r['SHDT'] - $r['SPRM']
    }
    if ($hasSgcoSplit) {
        $r['SINT'] = [int64]$_.SINT
        $r['SPHD'] = [int64]$_.SPHD
        $r['SPHC'] = [int64]$_.SPHC
        # SCPU runs inside the interrupt proc, so SINT already contains it.
        # Subtract rather than re-ring, which keeps SCPU's banked series intact.
        $r['SITR'] = $r['SINT'] - $r['SCPU']
        $r['SOBJ'] = $r['GCRA'] - $r['SINT'] - $r['SPHD'] - $r['SPHC'] -
                     $r['SCAT'] - $r['SHDT'] - $r['SPRM']
    }
    [pscustomobject]$r
})

# The nesting proof. A negative residual means a bracketed span ran outside the
# SRC bracket, which makes every share below meaningless.
if ($hasSrcSplit) {
    $negative = @($all | Where-Object { $_.SBAS -lt 0 })
    if ($negative.Count -ne 0) {
        $worst = ($negative | Sort-Object SBAS | Select-Object -First 1)
        throw ("SRC sub-buckets are not nested inside SRC: $($negative.Count) of " +
               "$($all.Count) frames have SRC - SHDT - SWRM < 0 (worst frame " +
               "$($worst.frame): SRC $($worst.SRC), SHDT $($worst.SHDT), " +
               "SWRM $($worst.SWRM), residual $($worst.SBAS)). The split cannot " +
               "be attributed.")
    }
}

# The same falsifier one level deeper. SOUT < 0 means gcRunAll ran outside the
# SRC bracket on that frame; SGCO < 0 means a bracketed fighter proc ran outside
# gcRunAll. Either makes every share below meaningless, so either throws.
if ($hasSbasSplit) {
    foreach ($probe in @(
        @{ col = 'SOUT'
           why = 'gcRunAll (GCRA) is not nested inside SRC: SRC - SWRM - GCRA < 0' }
        @{ col = 'SGCO'
           why = ('a bracketed fighter proc is not nested inside gcRunAll: ' +
                  'GCRA - SCPU - SCAT - SHDT - SPRM < 0') })) {
        $bad = @($all | Where-Object { $_.($probe.col) -lt 0 })
        if ($bad.Count -ne 0) {
            $worst = ($bad | Sort-Object -Property $probe.col | Select-Object -First 1)
            throw ("$($probe.why): $($bad.Count) of $($all.Count) frames " +
                   "(worst frame $($worst.frame): SRC $($worst.SRC), " +
                   "GCRA $($worst.GCRA), SCPU $($worst.SCPU), " +
                   "SCAT $($worst.SCAT), SHDT $($worst.SHDT), " +
                   "SPRM $($worst.SPRM), SWRM $($worst.SWRM), " +
                   "residual $($worst.($probe.col))). The split cannot be " +
                   "attributed.")
        }
    }
}

# The same falsifier one level deeper again (cycle 92). SITR < 0 means the CPU AI
# ran outside the interrupt proc it is supposed to nest inside; SOBJ < 0 means a
# bracketed fighter proc ran outside gcRunAll -- which is exactly what a botched
# rename would produce, because a proc whose ITCM pin stopped matching still runs
# but no longer where this arithmetic assumes. Either throws.
if ($hasSgcoSplit) {
    foreach ($probe in @(
        @{ col = 'SITR'
           why = ('ftComputerProcessAll (SCPU) is not nested inside the ' +
                  'interrupt proc: SINT - SCPU < 0') }
        @{ col = 'SOBJ'
           why = ('a bracketed fighter proc is not nested inside gcRunAll: ' +
                  'GCRA - SINT - SPHD - SPHC - SCAT - SHDT - SPRM < 0') })) {
        $bad = @($all | Where-Object { $_.($probe.col) -lt 0 })
        if ($bad.Count -ne 0) {
            $worst = ($bad | Sort-Object -Property $probe.col |
                Select-Object -First 1)
            throw ("$($probe.why): $($bad.Count) of $($all.Count) frames " +
                   "(worst frame $($worst.frame): GCRA $($worst.GCRA), " +
                   "SINT $($worst.SINT), SPHD $($worst.SPHD), " +
                   "SPHC $($worst.SPHC), SCPU $($worst.SCPU), " +
                   "SCAT $($worst.SCAT), SHDT $($worst.SHDT), " +
                   "SPRM $($worst.SPRM), residual $($worst.($probe.col))). " +
                   "The split cannot be attributed.")
        }
    }
    # SGCO = SITR + SPHD + SPHC + SOBJ is an identity, not a measurement, so any
    # mismatch is an arithmetic mistake in this script -- precisely the failure a
    # share table would hide. It is also what keeps the cycle-86 SGCO figure
    # comparable across the instrument change.
    $worstSgcoError = 0
    foreach ($r in $all) {
        $err = [math]::Abs($r.SGCO - ($r.SITR + $r.SPHD + $r.SPHC + $r.SOBJ))
        if ($err -gt $worstSgcoError) { $worstSgcoError = $err }
    }
    if ($worstSgcoError -ne 0) {
        throw ("SGCO does not equal SITR + SPHD + SPHC + SOBJ (max per-frame " +
               "error $worstSgcoError). The cycle-92 split is not a partition " +
               "of the cycle-86 residual.")
    }
}

# Verify the identity on every frame BEFORE using it to judge completeness
# later. A CSV from a build whose buckets do not close is not analysable.
$maxIdentityError = 0
foreach ($r in $all) {
    $sum = 0
    foreach ($b in $owners) { $sum += $r.$b }
    $sum += $r.'OTHR-WAIT'
    $err = [math]::Abs($sum - $r.'WORK-H')
    if ($err -gt $maxIdentityError) { $maxIdentityError = $err }
}
if ($maxIdentityError -ne 0) {
    throw ("The bucket identity does not close: max per-frame error " +
           "$maxIdentityError ticks. WORK-H cannot be attributed from this CSV.")
}

# Load-frame exclusion, against this arm's own SRC median rather than a constant.
$srcMedian = Get-Pct -Values @($all.SRC) -Q 0.50
$kept = $all
$dropped = 0
if ($LoadFrameSrcMultiple -gt 0) {
    $srcCut = $srcMedian * $LoadFrameSrcMultiple
    $kept = @($all | Where-Object { $_.SRC -le $srcCut })
    $dropped = $all.Count - $kept.Count
}

$hot = @($kept | Where-Object { $_.'WORK-H' -gt $Gate })
$clean = @($kept | Where-Object { $_.'WORK-H' -le $Gate })
if ($hot.Count -eq 0) { throw "No over-gate frames: nothing to attribute." }
if ($clean.Count -eq 0) { throw "No clean frames: no baseline to difference against." }

function Get-Mean { param($Set, [string]$Field)
    if ($Set.Count -eq 0) { return 0.0 }
    return ([double](($Set | Measure-Object -Property $Field -Sum).Sum)) / $Set.Count
}

$workDelta = (Get-Mean $hot 'WORK-H') - (Get-Mean $clean 'WORK-H')
$table = @()
foreach ($b in ($owners + @('OTHR-WAIT'))) {
    $exc = (Get-Mean $hot $b) - (Get-Mean $clean $b)
    $table += [pscustomobject]@{
        owner = $b
        excursion = [math]::Round($exc, 0)
        share = $(if ($workDelta -ne 0) { $exc / $workDelta } else { 0 })
    }
}
$ownerSum = ($table | Measure-Object -Property excursion -Sum).Sum
$closureError = [math]::Abs($ownerSum - $workDelta)
# Rounding each owner to whole ticks is the only slack allowed; anything larger
# means a bucket is missing from $owners.
if ($closureError -gt ($table.Count + 1)) {
    throw ("Owner excursions sum to $ownerSum but WORK-H moved $workDelta " +
           "(error $closureError). The attribution is incomplete.")
}

$p50 = Get-Pct -Values @($kept.'WORK-H') -Q 0.50
$p95 = Get-Pct -Values @($kept.'WORK-H') -Q 0.95
$cleanP95 = Get-Pct -Values @($clean.'WORK-H') -Q 0.95

Write-Host ''
Write-Host "EXCURSION ATTRIBUTION -- arm $Arm"
Write-Host ("  rows {0}, load-frames dropped {1} (SRC > {2:N0} = {3}x median {4:N0})" -f
    $all.Count, $dropped, ($srcMedian * $LoadFrameSrcMultiple), $LoadFrameSrcMultiple, $srcMedian)
Write-Host ("  WORK-H P50 {0:N0}   P95 {1:N0}   over gate {2}/{3} ({4:P1})" -f
    $p50, $p95, $hot.Count, $kept.Count, ($hot.Count / $kept.Count))
Write-Host ("  clean-frame P95 {0:N0}   gap to gate {1:N0}" -f $cleanP95, ($p95 - $Gate))
Write-Host ("  hot mean - clean mean (WORK-H) = {0:N0}   over {1} hot / {2} clean" -f
    $workDelta, $hot.Count, $clean.Count)
Write-Host ''
foreach ($t in ($table | Sort-Object -Property excursion -Descending)) {
    Write-Host ("    {0,-10} {1,12:N0}  {2,7:P1}" -f $t.owner, $t.excursion, $t.share)
}
Write-Host ("    {0,-10} {1,12:N0}  (identity closes, per-frame max error {2})" -f
    'SUM', $ownerSum, $maxIdentityError)
Write-Host ''

$srcTable = @()
if ($hasSrcSplit) {
    $srcDelta = (Get-Mean $hot 'SRC') - (Get-Mean $clean 'SRC')
    foreach ($b in $srcSubOwners) {
        $hotMean = Get-Mean $hot $b
        $cleanMean = Get-Mean $clean $b
        $exc = $hotMean - $cleanMean
        $srcTable += [pscustomobject]@{
            owner = $b
            excursion = [math]::Round($exc, 0)
            shareOfSrc = $(if ($srcDelta -ne 0) { $exc / $srcDelta } else { 0 })
            shareOfWorkH = $(if ($workDelta -ne 0) { $exc / $workDelta } else { 0 })
            hotMean = [math]::Round($hotMean, 0)
            cleanMean = [math]::Round($cleanMean, 0)
            # The column that decides whether an owner is a GATE lever or only a
            # P50 lever, computed here because it has been a hand calculation on
            # the board and that is how SCPU nearly got nominated: 72,512 ticks
            # at p50 looks like the prize, but it switches only 1.33x hot-vs-
            # clean, so capping it cannot move a P95. Absolute size and switching
            # behaviour are different questions and only the second owns a tail.
            hotOverClean = $(if ($cleanMean -ne 0) {
                [math]::Round($hotMean / $cleanMean, 2) } else { $null })
            p95 = Get-Pct -Values @($kept.$b) -Q 0.95
        }
    }
    # SBAS is defined as SRC - SHDT - SWRM, so the three excursions must sum to
    # SRC's exactly. This can only fail on an arithmetic mistake in this script,
    # which is precisely the failure a share table would hide.
    $srcSum = ($srcTable | Measure-Object -Property excursion -Sum).Sum
    $srcClosure = [math]::Abs($srcSum - $srcDelta)
    if ($srcClosure -gt ($srcTable.Count + 1)) {
        throw ("SRC sub-owner excursions sum to $srcSum but SRC moved " +
               "$srcDelta (error $srcClosure).")
    }
    # The cycle-81 audit closed this as "never rank SRC with the exclusion on":
    # the rule thresholds on the very bucket being attributed, so it shrinks
    # SRC's share whether or not a load happened. The default is still 2.0, so
    # the wrong ranking is one omitted argument away -- on the banked c86 gate
    # arm it reports SGCO 81,595 instead of 153,291, understated 1.88x, and both
    # tables look equally plausible. Say so on the table itself rather than
    # trusting the reader to have carried the caveat.
    if ($LoadFrameSrcMultiple -ne 0) {
        Write-Warning ("SRC is being ranked with the load-frame exclusion ON " +
            "(-LoadFrameSrcMultiple $LoadFrameSrcMultiple, $dropped frames " +
            'dropped). That rule thresholds on SRC itself, so this SRC split ' +
            'is CIRCULAR and understates every sub-owner. Re-run with ' +
            '-LoadFrameSrcMultiple 0 before quoting any figure below.')
    }
    Write-Host ("  SRC SPLIT -- SRC excursion {0:N0} is {1:P1} of WORK-H" -f
        $srcDelta, $(if ($workDelta -ne 0) { $srcDelta / $workDelta } else { 0 }))
    Write-Host ("    {0,-6} {1,12} {2,8} {3,8} {4,12} {5,12} {6,10}" -f
        'owner', 'excursion', '%SRC', '%WORK-H', 'clean mean', 'hot mean',
        'hot/clean')
    foreach ($t in ($srcTable | Sort-Object -Property excursion -Descending)) {
        Write-Host ("    {0,-6} {1,12:N0} {2,8:P1} {3,8:P1} {4,12:N0} {5,12:N0} {6,10}" -f
            $t.owner, $t.excursion, $t.shareOfSrc, $t.shareOfWorkH,
            $t.cleanMean, $t.hotMean,
            $(if ($null -ne $t.hotOverClean) { '{0:N2}x' -f $t.hotOverClean }
              else { 'n/a' }))
    }
    Write-Host ("    {0,-6} {1,12:N0}  (closes against SRC, error {2})" -f
        'SUM', $srcSum, $srcClosure)
    if ($hasSgcoSplit) {
        # The cycle-86 residual, still reported as a roll-up of its four parts so
        # the new instrument stays comparable against the banked 153,291 figure.
        $sgcoHot = Get-Mean $hot 'SGCO'
        $sgcoClean = Get-Mean $clean 'SGCO'
        Write-Host ("    {0,-6} {1,12:N0} {2,8:P1} {3,8:P1} {4,12:N0} {5,12:N0} {6,10}" -f
            'SGCO*', [math]::Round($sgcoHot - $sgcoClean, 0),
            $(if ($srcDelta -ne 0) { ($sgcoHot - $sgcoClean) / $srcDelta } else { 0 }),
            $(if ($workDelta -ne 0) { ($sgcoHot - $sgcoClean) / $workDelta } else { 0 }),
            [math]::Round($sgcoClean, 0), [math]::Round($sgcoHot, 0),
            $(if ($sgcoClean -ne 0) { '{0:N2}x' -f ($sgcoHot / $sgcoClean) }
              else { 'n/a' }))
        Write-Host ('    * SGCO is the cycle-86 roll-up SITR+SPHD+SPHC+SOBJ, ' +
            'NOT a partition member -- shown for comparison against the ' +
            'banked figure only.')
    }
    Write-Host ''
}

$ceilingTable = @()
if ($Ceilings) {
    # THE ONLY HONEST METHOD, and the reason it is written down: for each lane,
    # subtract max(0, lane - median(lane)) from that row's WORK-H, then re-take
    # the 80th largest of 1,600. Flattening a lane to its own median is the best
    # any optimization of it could do without also cutting the frames that were
    # already cheap.
    #
    # It is NOT computed by subtracting medians. Medians do not add, and doing it
    # that way invented a 110,336-tick lane worth +9,472 a row in c122.
    #
    # The baseline uses the SAME rank statistic rather than the harness's
    # percentile, so it sits a little above the banked P95 (1,856 in c125). Read
    # the DELTAS as the result; the absolute column is only their arithmetic.
    #
    # OTHR and WAIT are absent on purpose: OTHR is an accounting remainder that
    # CONTAINS WAIT, so its "ceiling" is mostly VBlank idle and is not spendable.
    # HUD is excluded from WORK-H by construction, so its ceiling is zero by
    # definition, not by measurement.
    $laneNames = @()
    foreach ($c in @('SRC', 'GCRA', 'SGCO', 'SITR', 'SINT', 'SHDT', 'MISC',
                     'SPHD', 'SOBJ', 'SBAS', 'SPRM', 'AUD', 'SCPU', 'STG',
                     'FTR', 'SCAT', 'SWRM', 'SPHC', 'SOUT', 'BG',
                     'OTHR-WAIT')) {
        if ($null -ne $all[0].PSObject.Properties[$c]) { $laneNames += $c }
    }
    $rank = [math]::Max(1, [int][math]::Ceiling($all.Count * 0.05))
    $sortedWorkH = @($all.'WORK-H' | Sort-Object -Descending)
    $baseline = $sortedWorkH[$rank - 1]
    foreach ($lane in $laneNames) {
        $laneMedian = Get-Pct -Values @($all.$lane) -Q 0.50
        $adjusted = @(foreach ($r in $all) {
            $over = $r.$lane - $laneMedian
            if ($over -lt 0) { $over = 0 }
            $r.'WORK-H' - $over
        })
        $flatP95 = @($adjusted | Sort-Object -Descending)[$rank - 1]
        $ceilingTable += [pscustomobject]@{
            lane = $lane
            ceiling = $baseline - $flatP95
            flatP95 = $flatP95
            median = $laneMedian
        }
    }
    Write-Host ("  LANE CEILINGS -- most each lane could pay, arm {0}" -f $Arm)
    Write-Host ("    baseline (rank {0} of {1}) {2:N0}   gate {3:N0}" -f
        $rank, $all.Count, $baseline, $Gate)
    Write-Host ("    {0,-9} {1,10} {2,12} {3,12}" -f
        'lane', 'ceiling', 'P95 if flat', 'median')
    foreach ($t in ($ceilingTable | Sort-Object -Property ceiling -Descending)) {
        Write-Host ("    {0,-9} {1,10:N0} {2,12:N0} {3,12:N0}" -f
            $t.lane, $t.ceiling, $t.flatP95, $t.median)
    }
    Write-Host ('    Lanes NEST -- SRC contains GCRA contains SGCO contains ' +
        'SITR/SPHD/SPHC/SOBJ. These ceilings do NOT add.')
    Write-Host ''
}

if ($JsonOut) {
    $payload = [ordered]@{
        analysis = 'tick-hud excursion attribution'; arm = $Arm; rowsCsv = $RowsCsv
        laneCeilings = $ceilingTable
        gate = $Gate; rows = $all.Count; loadFramesDropped = $dropped
        srcMedian = $srcMedian; loadFrameSrcMultiple = $LoadFrameSrcMultiple
        workHP50 = $p50; workHP95 = $p95; cleanFrameP95 = $cleanP95
        overGate = $hot.Count; judged = $kept.Count
        workHHotMinusClean = $workDelta; ownerExcursionSum = $ownerSum
        identityMaxErrorTicks = $maxIdentityError; closureErrorTicks = $closureError
        owners = $table
        srcSplitPresent = $hasSrcSplit
        srcSubOwners = $srcTable
        capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
    }
    $root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
    $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut } else { Join-Path $root $JsonOut }
    $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
    Write-Host "Wrote $jsonPath"
}
