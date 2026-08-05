[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RowsCsv,
    [Parameter(Mandatory=$true)][string]$Arm,
    [double]$Gate = 1120380,
    # The GAME SET tail is post-buzzer on both arms (board: frames 1998-2040),
    # so overlap with it is reported rather than assumed away.
    [int]$TailStartFrame = 1998,
    [string]$JsonOut = ''
)

# DOES THE OWNER'S LOAD-FRAME EXCLUSION SELECT LOADING STATES?
#
# The board excludes "loading states" from the gate by dropping frames whose
# SRC exceeds 2x that arm's own SRC median. This script exists because that
# rule is SELF-REFERENTIAL: it thresholds on the very bucket whose excursion
# the campaign is trying to attribute, so applying it necessarily shrinks SRC's
# measured share whether or not a single load occurred. Any SRC ranking taken
# with the exclusion ON is therefore circular, and the size of the distortion
# has to be measured rather than assumed small.
#
# Three questions, none of which the multiplier itself can answer:
#
#   1. SENSITIVITY -- how far do P95 and the gap move across plausible
#      thresholds? A gate that swings on the knob is a fragile gate.
#   2. SHAPE -- a genuine loading state is a CONTIGUOUS multi-frame event.
#      Scattered singletons spread evenly through steady play are gameplay.
#   3. SIGNATURE -- on a real load frame the cost is anomalous across the
#      board. If FTR/STG/MISC are ordinary and only SRC is elevated, the rule
#      is selecting the SRC excursion itself, i.e. the thing being measured.
#
# Cross-arm asymmetry is the strongest single discriminator and is why -Arm is
# mandatory: both arms run the same stage, fighters and assets, so a real
# loading filter must affect them similarly. A filter that guts one arm's P95
# and barely moves the other's is tracking that arm's tail, not its loads.

$ErrorActionPreference = 'Stop'

$rows = @(Import-Csv -LiteralPath $RowsCsv | ForEach-Object {
    [pscustomobject]@{
        frame = [int64]$_.frame
        SRC   = [int64]$_.SRC
        MISC  = [int64]$_.MISC
        FTR   = [int64]$_.FTR
        STG   = [int64]$_.STG
        AUD   = [int64]$_.AUD
        W     = [int64]$_.'WORK-H'
    }
})
if ($rows.Count -eq 0) { throw "No rows in $RowsCsv." }

function Get-Pct { param([int64[]]$V,[double]$Q)
    $s = @($V | Sort-Object); return [int64]$s[[int][Math]::Floor(($s.Count-1)*$Q)] }
function Get-Mean { param($S,[string]$F)
    if ($S.Count -eq 0) { return 0.0 }
    return ([double](($S | Measure-Object -Property $F -Sum).Sum))/$S.Count }

$median = Get-Pct -V @($rows.SRC) -Q 0.50

Write-Host ''
Write-Host "LOAD-FRAME EXCLUSION AUDIT -- arm $Arm"
Write-Host ("  rows {0}   SRC median {1:N0}   gate {2:N0}" -f $rows.Count, $median, $Gate)
Write-Host ''
Write-Host ("  {0,-6} {1,-12} {2,8} {3,12} {4,12} {5,12}" -f 'mult','SRC cut','dropped','WORK-H P95','gap','over-gate')

$sweep = @()
foreach ($m in @(0.0, 1.5, 2.0, 2.5, 3.0, 4.0)) {
    if ($m -le 0) { $kept = $rows; $cut = 0 }
    else { $cut = $median * $m; $kept = @($rows | Where-Object { $_.SRC -le $cut }) }
    $p95 = Get-Pct -V @($kept.W) -Q 0.95
    $over = @($kept | Where-Object { $_.W -gt $Gate }).Count
    $sweep += [pscustomobject]@{
        multiple = $m; srcCut = [int64]$cut; dropped = ($rows.Count - $kept.Count)
        workHP95 = $p95; gap = ($p95 - $Gate); overGate = $over; judged = $kept.Count
    }
    Write-Host ("  {0,-6} {1,-12:N0} {2,8} {3,12:N0} {4,12:N0} {5,12}" -f `
        $(if ($m -le 0) {'OFF'} else {"$($m)x"}), $cut, ($rows.Count-$kept.Count), $p95, ($p95-$Gate), "$over/$($kept.Count)")
}

$gapOff = ($sweep | Where-Object { $_.multiple -le 0 }).gap
$gapMin = ($sweep | Where-Object { $_.multiple -gt 0 } | Measure-Object -Property gap -Minimum).Minimum
$swing = if ($gapMin -ne 0) { [math]::Round($gapOff / $gapMin, 2) } else { 0 }
Write-Host ''
Write-Host ("  SENSITIVITY: gap ranges {0:N0} (off) to {1:N0} (1.5x) -- a {2}x swing on the knob alone." -f $gapOff, $gapMin, $swing)

$cut2 = $median * 2.0
$dropped = @($rows | Where-Object { $_.SRC -gt $cut2 })
$keptRows = @($rows | Where-Object { $_.SRC -le $cut2 })
if ($dropped.Count -eq 0) { Write-Host '  Nothing dropped at 2.0x.'; return }

Write-Host ''
Write-Host ("  THE 2.0x DROPPED POPULATION -- {0} frames (SRC > {1:N0})" -f $dropped.Count, $cut2)
$sig = @()
foreach ($f in @('SRC','FTR','STG','MISC','AUD')) {
    $d = Get-Mean $dropped $f; $k = Get-Mean $keptRows $f
    $ratio = if ($k -ne 0) { $d / $k } else { 0 }
    $sig += [pscustomobject]@{ bucket = $f; droppedMean = [int64]$d; keptMean = [int64]$k; ratio = [math]::Round($ratio,3) }
    Write-Host ("    {0,-5} dropped {1,12:N0}   kept {2,12:N0}   ratio {3,6:N2}x" -f $f, $d, $k, $ratio)
}

$runs = @(); $start = $null; $prev = $null
foreach ($r in ($dropped | Sort-Object frame)) {
    if ($null -eq $start) { $start = $r.frame; $prev = $r.frame; continue }
    if ($r.frame -eq ($prev + 1)) { $prev = $r.frame; continue }
    $runs += ,@($start, $prev); $start = $r.frame; $prev = $r.frame
}
if ($null -ne $start) { $runs += ,@($start, $prev) }
$singletons = @($runs | Where-Object { $_[0] -eq $_[1] }).Count
$longest = (($runs | ForEach-Object { $_[1]-$_[0]+1 } | Measure-Object -Maximum).Maximum)
$inTail = @($dropped | Where-Object { $_.frame -ge $TailStartFrame }).Count

Write-Host ''
Write-Host ("    SHAPE: {0} contiguous runs, {1} of them singletons, longest {2} frames" -f $runs.Count, $singletons, $longest)
Write-Host ("    overlap with the GAME SET tail (frame >= {0}): {1} of {2}" -f $TailStartFrame, $inTail, $dropped.Count)

if ($JsonOut) {
    $payload = [ordered]@{
        analysis = 'load-frame exclusion audit'; arm = $Arm; rowsCsv = $RowsCsv
        gate = $Gate; rows = $rows.Count; srcMedian = $median
        sweep = $sweep; gapSwingRatio = $swing
        droppedAt2x = $dropped.Count; bucketSignature = $sig
        contiguousRuns = $runs.Count; singletonRuns = $singletons
        longestRunFrames = $longest; overlapWithGameSetTail = $inTail
        capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
    }
    $root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
    $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut } else { Join-Path $root $JsonOut }
    $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
    Write-Host "Wrote $jsonPath"
}
Write-Host ''
