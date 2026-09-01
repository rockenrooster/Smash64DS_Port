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

function Assert-SamusEntryNative {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if ($Condition) { return }
    if ($Evidence) { throw "$Message`n$Evidence" }
    throw $Message
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\verification\2026-08-31_bug-samus-entry-native.txt'
} elseif (-not [System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Assert-SamusEntryNative (Test-Path -LiteralPath $config -PathType Leaf) `
    "Samus entry proof build config is missing: $config"
$configText = Get-Content -LiteralPath $config -Raw
foreach ($definition in @(
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_FOUR_CPU_ROSTER 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusEntryNative $configText.Contains($definition) `
        "Samus entry proof is missing required definition: $definition" $config
}

# BattleShip ftcommonentry.c selects Samus AppearR/L and creates the
# SamusSpecial2 EntryPoint effect naturally. The four-fighter stress descriptor
# puts Samus in slot 0, so this observes the real startup path with no injected
# status/effect. Generated entry roots 11/12 are the exact source Gfx at 0x0930
# and 0x0AD0; the live DObj and EntryPoint AnimJoint still own timing/transforms.
$probe = Join-Path $PSScriptRoot 'probe-p2-fourcpu-sparse.ps1'
& pwsh -NoProfile -File $probe -NoBuild -Build $Build -Frame $Frame `
    -RunnerSlot $RunnerSlot -TimeoutSeconds $TimeoutSeconds -Artifact $Artifact
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$output = Get-Content -LiteralPath $Artifact -Raw
$roster = [regex]::Match($output, 'ROSTER=(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+)')
$entry = [regex]::Match($output, 'ENTRY_NATIVE=(\d+),(\d+),(\d+),(\d+),(\d+)')
$samus = [regex]::Match($output, 'ENTRY_NATIVE_SAMUS=(\d+),(\d+)')
$texture = [regex]::Match($output, 'TEXTURE=(\d+),(\d+),(\d+)')

foreach ($pair in @(
    @{ Match=$roster; Name='roster' },
    @{ Match=$entry; Name='entry-native' },
    @{ Match=$samus; Name='Samus entry roots' },
    @{ Match=$texture; Name='texture' }
)) {
    Assert-SamusEntryNative $pair.Match.Success `
        "Samus entry proof is missing its $($pair.Name) marker." $output
}

$kindWord = [Convert]::ToUInt32($roster.Groups[1].Value.Substring(2), 16)
$drawMask = [Convert]::ToUInt32($roster.Groups[2].Value.Substring(2), 16)
Assert-SamusEntryNative ($kindWord -eq 0x03080204) `
    ('Four-CPU proof did not run Samus/Fox/Captain/Donkey: kindWord=0x{0:x8}' -f $kindWord) $output
Assert-SamusEntryNative ($drawMask -eq 0xF) `
    ('Four-CPU proof did not draw all four source fighters: drawMask=0x{0:x}' -f $drawMask) $output

$entryDraws = [int]$entry.Groups[1].Value
$entryFallbacks = [int]$entry.Groups[2].Value
$entryPrepares = [int]$entry.Groups[3].Value
$entryBinds = [int]$entry.Groups[4].Value
$samusMainDraws = [int]$samus.Groups[1].Value
$samusPostDraws = [int]$samus.Groups[2].Value
$textureRejects = [int]$texture.Groups[3].Value

Assert-SamusEntryNative `
    ($entryDraws -gt 0 -and $entryFallbacks -eq 0 -and $entryPrepares -gt 0 -and `
     $entryBinds -gt 0 -and $samusMainDraws -gt 0 -and $samusPostDraws -gt 0) `
    'Samus EntryPoint did not submit both exact SamusSpecial2 source lists through the DS-native owner.' $output
Assert-SamusEntryNative ($textureRejects -eq 0) `
    'Production renderer rejected a fighter texture during Samus entry proof.' $output

Write-Output (("P2 Samus entry native proof passed: roster=0x{0:x8} drawMask=0x{1:x} " +
    "entryDraws=$entryDraws samusRoots=$samusMainDraws/$samusPostDraws " +
    "fallbacks=0 rejects=0.") -f $kindWord, $drawMask)
