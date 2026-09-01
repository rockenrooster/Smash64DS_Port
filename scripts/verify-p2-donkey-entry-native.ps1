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

function Assert-DonkeyEntryNative {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if ($Condition) { return }
    if ($Evidence) { throw "$Message`n$Evidence" }
    throw $Message
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\verification\2026-08-30_bug-donkey-entry-native.txt'
} elseif (-not [System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Assert-DonkeyEntryNative (Test-Path -LiteralPath $config -PathType Leaf) `
    "Donkey entry proof build config is missing: $config"
$configText = Get-Content -LiteralPath $config -Raw
foreach ($definition in @(
    '#define NDS_P2_DONKEY 1',
    '#define NDS_P2_CAPTAIN 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_FOUR_CPU_ROSTER 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-DonkeyEntryNative $configText.Contains($definition) `
        "Donkey entry proof is missing required definition: $definition" $config
}

# BattleShip ftcommonentry.c selects Donkey's AppearR/L status and calls
# efManagerDonkeyEntryTaruMakeEffect.  The four-kind stress roster places Donkey
# in player slot 3; this focused proof observes that natural startup rather than
# injecting a status or effect.  The sparse probe reports the immutable native
# entry root (DonkeySpecial2 + 0x0620), packet state, and texture rejects from
# the same run.
$probe = Join-Path $PSScriptRoot 'probe-p2-fourcpu-sparse.ps1'
& pwsh -NoProfile -File $probe -NoBuild -Build $Build -Frame $Frame `
    -RunnerSlot $RunnerSlot -TimeoutSeconds $TimeoutSeconds -Artifact $Artifact
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$output = Get-Content -LiteralPath $Artifact -Raw
$roster = [regex]::Match($output, 'ROSTER=(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+)')
$packet = [regex]::Match($output,
    'PACKET=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)')
$slot3 = [regex]::Match($output,
    'PKSLOT3=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)')
$texture = [regex]::Match($output, 'TEXTURE=(\d+),(\d+),(\d+)')
$entry = [regex]::Match($output, 'ENTRY_NATIVE=(\d+),(\d+),(\d+),(\d+),(\d+)')

foreach ($pair in @(
    @{ Match=$roster; Name='roster' },
    @{ Match=$packet; Name='packet' },
    @{ Match=$slot3; Name='slot-3 packet' },
    @{ Match=$texture; Name='texture' },
    @{ Match=$entry; Name='entry-native' }
)) {
    Assert-DonkeyEntryNative $pair.Match.Success `
        "Donkey entry proof is missing its $($pair.Name) marker." $output
}

$kindWord = [Convert]::ToUInt32($roster.Groups[1].Value.Substring(2), 16)
$drawMask = [Convert]::ToUInt32($roster.Groups[2].Value.Substring(2), 16)
# One byte per player slot stores fkind + 1.  BattleShip fkind ordering gives
# Samus=3, Fox=1, Captain=7, Donkey=2 -> 04 02 08 03 in slots 0..3.
Assert-DonkeyEntryNative ($kindWord -eq 0x03080204) `
    ('Four-CPU proof did not run Samus/Fox/Captain/Donkey: kindWord=0x{0:x8}' -f $kindWord) $output
Assert-DonkeyEntryNative ($drawMask -eq 0xF) `
    ('Four-CPU proof did not draw all four source fighters: drawMask=0x{0:x}' -f $drawMask) $output

$packetFaults = [int]$packet.Groups[3].Value
$packetDeclines = [int]$packet.Groups[4].Value
Assert-DonkeyEntryNative ($packetFaults -eq 0 -and $packetDeclines -eq 0) `
    "Donkey's four-fighter native packet path faulted or declined." $output

$slot3Valid = [int]$slot3.Groups[1].Value
$slot3Words = [int]$slot3.Groups[2].Value
$slot3Roots = [int]$slot3.Groups[4].Value
$slot3Textures = [int]$slot3.Groups[6].Value
Assert-DonkeyEntryNative `
    ($slot3Valid -eq 1 -and $slot3Words -gt 0 -and $slot3Roots -gt 0 -and $slot3Textures -gt 0) `
    'Donkey slot 3 did not establish a complete production native packet.' $output

$textureRejects = [int]$texture.Groups[3].Value
Assert-DonkeyEntryNative ($textureRejects -eq 0) `
    'Four-fighter production renderer rejected a fighter texture during Donkey entry proof.' $output

$entryDraws = [int]$entry.Groups[1].Value
$entryFallbacks = [int]$entry.Groups[2].Value
$entryPrepares = [int]$entry.Groups[3].Value
$entryBinds = [int]$entry.Groups[4].Value
$donkeyRootDraws = [int]$entry.Groups[5].Value
Assert-DonkeyEntryNative `
    ($entryDraws -gt 0 -and $entryFallbacks -eq 0 -and $entryPrepares -gt 0 -and `
     $entryBinds -gt 0 -and $donkeyRootDraws -gt 0) `
    'Donkey barrel entry did not stay on the DS-native entry-effect path.' $output

Write-Output (("P2 Donkey entry native proof passed: roster=0x{0:x8} drawMask=0x{1:x} " +
    "slot3Words=$slot3Words roots=$slot3Roots textures=$slot3Textures " +
    "entryDraws=$entryDraws donkeyRootDraws=$donkeyRootDraws fallbacks=0 rejects=0.") -f `
    $kindWord, $drawMask)
