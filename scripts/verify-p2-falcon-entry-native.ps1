[CmdletBinding()]
param(
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [int]$RunnerSlot = -1,
    [ValidateRange(32,2048)][int]$Frame = 128,
    [ValidateRange(30,900)][int]$TimeoutSeconds = 300,
    [switch]$NoBuild,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-p2-fourcpu-tickhud-hwtri'
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$config = Join-Path $buildDir 'nds_build_config.h'

function Assert-FalconEntryNative {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if ($Condition) { return }
    if ($Evidence) { throw "$Message`n$Evidence" }
    throw $Message
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\verification\2026-08-31_bug-falcon-entry-native.txt'
} elseif (-not [System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Assert-FalconEntryNative (Test-Path -LiteralPath $config -PathType Leaf) `
    "Falcon entry proof build config is missing: $config"
$configText = Get-Content -LiteralPath $config -Raw
foreach ($definition in @(
    '#define NDS_P2_CAPTAIN 1',
    '#define NDS_P2_FOUR_CPU_ROSTER 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-FalconEntryNative $configText.Contains($definition) `
        "Falcon entry proof is missing required definition: $definition" $config
}

# The four-fighter stress descriptor naturally starts Captain Falcon in slot 2.
# BattleShip owns EntryCar creation plus its 0x6200/0x6518/0x6598 AnimJoints;
# generated roots 13..22 correspond exactly to the ten immutable source Gfx
# submitted by dCaptainSpecial2_EntryCar's DObjDLLinks.
$probe = Join-Path $PSScriptRoot 'probe-p2-fourcpu-sparse.ps1'
& pwsh -NoProfile -File $probe -NoBuild -Build $Build -Frame $Frame `
    -RunnerSlot $RunnerSlot -TimeoutSeconds $TimeoutSeconds -Artifact $Artifact
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$output = Get-Content -LiteralPath $Artifact -Raw
$roster = [regex]::Match($output, 'ROSTER=(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+)')
$entry = [regex]::Match($output, 'ENTRY_NATIVE=(\d+),(\d+),(\d+),(\d+),(\d+)')
$captain = [regex]::Match(
    $output,
    'ENTRY_NATIVE_CAPTAIN=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)')
$texture = [regex]::Match($output, 'TEXTURE=(\d+),(\d+),(\d+)')

Assert-FalconEntryNative ($roster.Success -and $entry.Success -and $captain.Success -and $texture.Success) `
    'Falcon EntryCar proof is missing required runtime markers.' $output

$kindWord = [Convert]::ToUInt32($roster.Groups[1].Value.Substring(2), 16)
$drawMask = [Convert]::ToUInt32($roster.Groups[2].Value.Substring(2), 16)
Assert-FalconEntryNative ($kindWord -eq 0x03080204) `
    ('Four-CPU proof did not run Samus/Fox/Captain/Donkey: kindWord=0x{0:x8}' -f $kindWord) $output
Assert-FalconEntryNative ($drawMask -eq 0xF) `
    ('Four-CPU proof did not draw all four source fighters: drawMask=0x{0:x}' -f $drawMask) $output

$entryFallbacks = [int]$entry.Groups[2].Value
$entryPrepares = [int]$entry.Groups[3].Value
$entryBinds = [int]$entry.Groups[4].Value
$rootDraws = for ($i = 1; $i -le 10; $i++) { [int]$captain.Groups[$i].Value }
$textureRejects = [int]$texture.Groups[3].Value

Assert-FalconEntryNative ($entryFallbacks -eq 0 -and $entryPrepares -gt 0 -and $entryBinds -gt 0) `
    'Falcon EntryCar did not stay entirely on the prepared DS-native entry owner.' $output
Assert-FalconEntryNative (($rootDraws | Where-Object { $_ -le 0 }).Count -eq 0) `
    ('Falcon EntryCar did not draw all ten exact CaptainSpecial2 roots: ' + ($rootDraws -join ',')) $output
Assert-FalconEntryNative ($textureRejects -eq 0) `
    'Production renderer rejected a fighter/entry texture during Falcon entry proof.' $output

Write-Output ('P2 Falcon entry native proof passed: ten CaptainSpecial2 roots=' +
    ($rootDraws -join '/') + '; fallbacks=0 rejects=0.')
