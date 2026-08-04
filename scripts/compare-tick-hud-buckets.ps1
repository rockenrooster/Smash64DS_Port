[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Control,
    [Parameter(Mandatory=$true)][string]$Candidate
)

# Diffs two sample-tick-hud-buckets.ps1 JSON captures on P50 and P95.
#
# Never diff the means. The bursty buckets (AUD, HUD) run p95/p50 spreads of
# 48-310x, so their means describe no frame that ever happens; the percentiles
# are the decision input. See docs/PERF_LEDGER.md.
#
# The battle scene is deterministic, so a matched pair needs no repeat runs --
# but only if both captures cover the SAME frame window. That is asserted here
# rather than assumed, because a shifted window silently compares different work.

$ErrorActionPreference = 'Stop'

$a = Get-Content -Raw -LiteralPath $Control | ConvertFrom-Json
$b = Get-Content -Raw -LiteralPath $Candidate | ConvertFrom-Json

if ($a.startFrame -ne $b.startFrame -or $a.endFrame -ne $b.endFrame) {
    throw ("Frame windows differ: control $($a.startFrame)..$($a.endFrame) vs " +
        "candidate $($b.startFrame)..$($b.endFrame). Re-run both with the same " +
        '-StartFrame and -Samples; a shifted window compares different work.')
}
if ($a.melonDSSha256 -ne $b.melonDSSha256) {
    throw 'Captures used different melonDS builds; cycle counts are not comparable.'
}
if ($a.romSha256 -eq $b.romSha256) {
    throw 'Control and candidate are the same ROM.'
}

Write-Host ("window frames=$($a.startFrame)..$($a.endFrame) samples=$($a.samples) " +
    "melonDS=$($a.melonDSSha256.Substring(0,16)) git=$($a.gitShort)")
Write-Host "control   $($a.romSha256.Substring(0,16))"
Write-Host "candidate $($b.romSha256.Substring(0,16))"
Write-Host ''

$rows = foreach ($bucket in $a.buckets) {
    $other = $b.buckets | Where-Object { $_.bucket -eq $bucket.bucket }
    if ($null -eq $other) { continue }
    $d50 = [int64]$other.p50 - [int64]$bucket.p50
    $d95 = [int64]$other.p95 - [int64]$bucket.p95
    [PSCustomObject]@{
        bucket    = $bucket.bucket
        ctlP50    = '{0,10:N0}' -f $bucket.p50
        candP50   = '{0,10:N0}' -f $other.p50
        dP50      = '{0,10:N0}' -f $d50
        pctP50    = if ($bucket.p50) { '{0,7:N2}' -f (100.0 * $d50 / $bucket.p50) } else { '     -' }
        ctlP95    = '{0,10:N0}' -f $bucket.p95
        candP95   = '{0,10:N0}' -f $other.p95
        dP95      = '{0,10:N0}' -f $d95
        pctP95    = if ($bucket.p95) { '{0,7:N2}' -f (100.0 * $d95 / $bucket.p95) } else { '     -' }
    }
}
# Out-Host, not a bare Format-Table: this script now emits two tables with
# Write-Host between them, and Format-Table streams format objects that the
# default formatter then interleaves out of sequence ("not valid or not in the
# correct sequence", and the FIRST table is the one that disappears). Out-Host
# renders each table where it is written.
$rows | Format-Table -AutoSize | Out-Host

# PAIRED BY FRAME. The percentile table above compares two SORTED columns, so
# its rows can name different frames in each arm -- and that is not a nuance,
# it is how this campaign has now twice published a verdict the paired view
# reversed. R2-03 E28 read `WORK P95 +73,664` from an excursion frame whose
# placement had moved (paired: FTR better on 128/128). Gate 5 (2026-08-04) read
# flag 1 as 81,344 AHEAD on a 40-sample window; paired over 128 frames the
# over-gate counts were 16 and 17 and the median went the other way.
#
# Both arms run the same deterministic ROM from the same start frame, so frame N
# is the same game state in both and the pairing is free. TASK_STANDING_RULES.md
# has required it since E28; until now nothing in the tooling did it, which is
# why "compare the two JSONs" kept meaning the weaker comparison.
if (($null -ne $a.rows) -and ($null -ne $b.rows) -and
    ($a.rows.Count -eq $b.rows.Count) -and ($a.rows.Count -gt 0)) {
    $names = $a.bucketNames
    $ra = $a.rows
    $rb = $b.rows
    Write-Host 'paired by frame (negative = candidate cheaper):'
    $pairRows = foreach ($i in 0..($names.Count - 1)) {
        $col = $i + 1
        $d = @(foreach ($k in 0..($ra.Count - 1)) {
            [int64]$rb[$k][$col] - [int64]$ra[$k][$col] })
        $sorted = @($d | Sort-Object)
        [PSCustomObject]@{
            bucket   = $names[$i]
            better   = '{0}/{1}' -f @($d | Where-Object { $_ -lt 0 }).Count, $d.Count
            worse    = @($d | Where-Object { $_ -gt 0 }).Count
            median   = '{0,10:N0}' -f $sorted[[int][Math]::Floor(($sorted.Count - 1) * 0.5)]
            best     = '{0,10:N0}' -f $sorted[0]
            worstReg = '{0,10:N0}' -f $sorted[-1]
        }
    }
    # WORK-H is the gated series and is not a raw ring bucket, so it has to be
    # derived per frame -- subtracting one arm's WORK-H percentile from the
    # other's is exactly the mistake this block exists to stop.
    $wi = ([array]::IndexOf($names, 'WORK')) + 1
    $hi = ([array]::IndexOf($names, 'HUD')) + 1
    if (($wi -gt 0) -and ($hi -gt 0)) {
        $dw = @(foreach ($k in 0..($ra.Count - 1)) {
            ([int64]$rb[$k][$wi] - [int64]$rb[$k][$hi]) -
            ([int64]$ra[$k][$wi] - [int64]$ra[$k][$hi]) })
        $sw = @($dw | Sort-Object)
        $pairRows += [PSCustomObject]@{
            bucket   = 'WORK-H'
            better   = '{0}/{1}' -f @($dw | Where-Object { $_ -lt 0 }).Count, $dw.Count
            worse    = @($dw | Where-Object { $_ -gt 0 }).Count
            median   = '{0,10:N0}' -f $sw[[int][Math]::Floor(($sw.Count - 1) * 0.5)]
            best     = '{0,10:N0}' -f $sw[0]
            worstReg = '{0,10:N0}' -f $sw[-1]
        }
    }
    $pairRows | Format-Table -AutoSize | Out-Host

    # The gate is an event, not an average: both arms can share a P50 and differ
    # only in how many frames cross 1.12M. Count them per arm, and name them --
    # an excursion set that moves between arms is a placement artifact, one that
    # shrinks is a result.
    if (($wi -gt 0) -and ($hi -gt 0)) {
        $gate = 1120000
        foreach ($p in @(@{ n = 'control  '; r = $ra }, @{ n = 'candidate'; r = $rb })) {
            $wh = @(foreach ($k in 0..($p.r.Count - 1)) {
                [int64]$p.r[$k][$wi] - [int64]$p.r[$k][$hi] })
            $over = @(0..($wh.Count - 1) | Where-Object { $wh[$_] -gt $gate })
            Write-Host ("  {0}  WORK-H over {1:N0}: {2} of {3} frames{4}" -f
                $p.n, $gate, $over.Count, $wh.Count,
                $(if ($over.Count) {
                    '  at ' + (($over | ForEach-Object { $p.r[$_][0] }) -join ',')
                } else { '' }))
        }
    }
} else {
    # Silence here would read as "the arms are identical per frame". Say why.
    Write-Warning ('No paired-by-frame view: one capture carries no per-frame ' +
        'rows, or the two differ in length. Re-run both with the same ' +
        '-Samples; the percentile table above compares SORTED columns and can ' +
        'name different frames in each arm.')
}

$vbiRow = {
    param($s)
    $t = [double][Math]::Max(1, $s.vbiTotal)
    '{0,5:N1} {1,5:N1} {2,5:N1}  (n={3})' -f (100.0 * $s.vbi3 / $t),
        (100.0 * $s.vbi4 / $t), (100.0 * $s.vbi5plus / $t), $s.vbiTotal
}
Write-Host ''
Write-Host 'VBlank interval share, normalized by sample count (never min-FPS):'
Write-Host ("            3-VBI 4-VBI  5+-VBI")
Write-Host ("  control   " + (& $vbiRow $a))
Write-Host ("  candidate " + (& $vbiRow $b))
Write-Host ''
Write-Host ("named ticks: control $('{0:N0}' -f $a.meanNamed), " +
    "candidate $('{0:N0}' -f $b.meanNamed)")
Write-Host ("cadence violations: control $($a.cadenceViolations), " +
    "candidate $($b.cadenceViolations)")
