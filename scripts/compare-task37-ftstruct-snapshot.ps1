<#
.SYNOPSIS
Diff two raw FTStruct snapshots and name the differing member.

.DESCRIPTION
Task 37's exactness gate fails with 692 of 3,892 per-update records differing,
and the region bisect could only narrow it to "FTStruct", which is hashed as one
3,012-byte blob. This diffs the raw bytes captured by NDS_TASK9_FTSTRUCT_SNAPSHOT
and reports the exact offsets, the members they belong to, and -- the part that
decides the verdict -- which memory region each differing word points into.

Two outcomes are being told apart:

  * both values are addresses on opposite sides of the main-RAM/ITCM boundary.
    That is the instrument, not the port: ndsTask9StateCanonicalWord collapses
    main RAM to 0x20000000 so relocation does not register, and gives ITCM its
    own class 0x30000000, so any code moved into ITCM registers as a state
    change by construction.

  * the offset is a gameplay member (position, velocity, status, damage).
    Then Task 37 is a real defect.

Slot 0 is the update before the first divergence, slot 1 is the first diverging
update. Slot 0 matching while slot 1 differs proves the state entering the
divergence was identical, so the reported offset is the origin.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Baseline,
    [Parameter(Mandatory = $true)][string]$Candidate,
    [string]$Header = (Join-Path $PSScriptRoot '..\include\ft\fighter.h'),
    # Emit every differing word rather than the first N per fighter.
    [switch]$All,
    [int]$MaxPerFighter = 40
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-FTStructLayout {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "fighter.h not found: $Path"
    }
    $text = Get-Content -Raw -LiteralPath $Path
    $size = $null
    $sourceSize = $null
    $members = [Collections.Generic.List[object]]::new()
    foreach ($line in ($text -split "`r?`n")) {
        if ($line -match '^\s*#define\s+NDS_FTSTRUCT_LAYOUT_SIZE\s+(\d+)u?\s*$') {
            $size = [int]$Matches[1]
            continue
        }
        if ($line -match '^\s*#define\s+NDS_FTSTRUCT_SOURCE_SIZE\s+(\d+)u?\s*$') {
            $sourceSize = [int]$Matches[1]
            continue
        }
        if ($line -match '^\s*#define\s+NDS_FTSTRUCT_OFF_([A-Z0-9_]+)\s+(\d+)u?\s*$') {
            $members.Add([pscustomobject]@{
                Name   = $Matches[1].ToLowerInvariant()
                Offset = [int]$Matches[2]
            })
        }
    }
    if ($null -eq $size) { throw 'NDS_FTSTRUCT_LAYOUT_SIZE not found in fighter.h' }
    if (-not $members.Count) { throw 'no NDS_FTSTRUCT_OFF_* entries found in fighter.h' }

    return [pscustomobject]@{
        Size       = $size
        SourceSize = $sourceSize
        Members    = @($members | Sort-Object Offset)
    }
}

function Get-MemberAt {
    param($Layout, [int]$Offset)

    $hit = $null
    foreach ($m in $Layout.Members) {
        if ($m.Offset -le $Offset) { $hit = $m } else { break }
    }
    if ($null -eq $hit) {
        return [pscustomobject]@{ Name = '<before first mapped member>'; Delta = $Offset }
    }
    return [pscustomobject]@{ Name = $hit.Name; Delta = ($Offset - $hit.Offset) }
}

# Mirrors the ranges in ndsTask9StateCanonicalWord (nds_task9_state_hash.c).
function Get-Region {
    param([uint32]$Value)

    $v = [uint32]($Value -band 0xFFFFFFFE)
    if ($v -ge 0x02000000 -and $v -lt 0x02400000) { return 'main-RAM' }
    if ($v -ge 0x01FF8000 -and $v -lt 0x02000000) { return 'ITCM' }
    if ($v -ge 0x02FF0000 -and $v -lt 0x03000000) { return 'DTCM' }
    if ($v -ge 0x06000000 -and $v -lt 0x07000000) { return 'VRAM' }
    return '-'
}

$layout = Get-FTStructLayout -Path $Header
$baselineBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $Baseline))
$candidateBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $Candidate))

$slots = 2
$fighters = 2
$expected = $slots * $fighters * $layout.Size
foreach ($pair in @(@('baseline', $baselineBytes), @('candidate', $candidateBytes))) {
    if ($pair[1].Length -ne $expected) {
        throw ("$($pair[0]) snapshot is $($pair[1].Length) bytes, expected " +
            "$expected ($slots slots x $fighters fighters x $($layout.Size))")
    }
}

Write-Host "FTStruct size          $($layout.Size) bytes (source part $($layout.SourceSize))"
Write-Host "mapped members         $($layout.Members.Count)"
Write-Host "baseline               $Baseline"
Write-Host "candidate              $Candidate"
Write-Host ''

$slotNames = @('slot 0 (update N-1, last identical)', 'slot 1 (update N, first divergence)')
$anyDiff = $false
$regionCrossings = [Collections.Generic.List[string]]::new()

for ($slot = 0; $slot -lt $slots; $slot++) {
    for ($f = 0; $f -lt $fighters; $f++) {
        $base = (($slot * $fighters) + $f) * $layout.Size
        $diffs = [Collections.Generic.List[int]]::new()
        for ($i = 0; $i -lt $layout.Size; $i++) {
            if ($baselineBytes[$base + $i] -ne $candidateBytes[$base + $i]) {
                $diffs.Add($i)
            }
        }
        $label = "$($slotNames[$slot])  fighter $f"
        if (-not $diffs.Count) {
            Write-Host "$label  IDENTICAL ($($layout.Size) bytes)" -ForegroundColor Green
            continue
        }
        $anyDiff = $true
        Write-Host "$label  $($diffs.Count) of $($layout.Size) bytes differ" -ForegroundColor Yellow

        # Collapse to distinct aligned words so a 4-byte pointer reports once.
        $words = [Collections.Generic.SortedSet[int]]::new()
        foreach ($d in $diffs) { [void]$words.Add([int]([Math]::Floor($d / 4) * 4)) }

        $shown = 0
        Write-Host ('    {0,6}  {1,-26} {2,-11} {3,-11} {4}' -f
            'offset', 'member (+delta)', 'baseline', 'candidate', 'regions')
        foreach ($w in $words) {
            if (-not $All -and $shown -ge $MaxPerFighter) {
                Write-Host "    ... $($words.Count - $shown) more words (use -All)"
                break
            }
            $bw = [BitConverter]::ToUInt32($baselineBytes, $base + $w)
            $cw = [BitConverter]::ToUInt32($candidateBytes, $base + $w)
            $m = Get-MemberAt -Layout $layout -Offset $w
            $memberText = if ($m.Delta -eq 0) { $m.Name } else { "$($m.Name)+$($m.Delta)" }
            $br = Get-Region -Value $bw
            $cr = Get-Region -Value $cw
            $regionText = if ($br -eq $cr) { $br } else { "$br -> $cr" }
            $colour = 'Gray'
            if ($br -ne $cr -and $br -ne '-' -and $cr -ne '-') {
                $colour = 'Cyan'
                $regionCrossings.Add(
                    "slot $slot fighter $f offset $w ($memberText): $br -> $cr")
            }
            Write-Host ('    {0,6}  {1,-26} 0x{2:X8}  0x{3:X8}  {4}' -f
                $w, $memberText, $bw, $cw, $regionText) -ForegroundColor $colour
            $shown++
        }
        Write-Host ''
    }
}

Write-Host '=== verdict inputs ==='
if (-not $anyDiff) {
    Write-Host 'Both snapshots are byte-identical.' -ForegroundColor Green
    Write-Host ('The divergence is NOT in FTStruct at this update, or the ' +
        'snapshot update index does not match the first differing record.')
    exit 0
}
if ($regionCrossings.Count) {
    Write-Host ("$($regionCrossings.Count) word(s) change memory region " +
        'between the two builds:') -ForegroundColor Cyan
    foreach ($c in $regionCrossings) { Write-Host "  $c" }
    Write-Host ''
    Write-Host ('A main-RAM -> ITCM crossing is the canonicalizer artifact: ' +
        'ndsTask9StateCanonicalWord gives ITCM its own class, so relocating ' +
        'code into ITCM registers as a state change by construction.')
} else {
    Write-Host ('No word changes memory region. The difference is not a ' +
        'pointer crossing the ITCM boundary -- check the members named above ' +
        'against the gameplay/render split before drawing any conclusion.')
}
