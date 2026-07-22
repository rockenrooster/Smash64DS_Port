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
$rows | Format-Table -AutoSize

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
