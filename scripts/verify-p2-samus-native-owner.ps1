[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [string]$Build = 'build-bugs-samus-native',
    [switch]$NoBuild,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-SamusNativeOwner {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if ($Condition) { return }
    if ($Evidence) { throw "$Message`n$Evidence" }
    throw $Message
}

$target = 'smash64ds-battle-playable-tickhud-hwtri'
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir "$target.nds"
$elf = Join-Path $buildDir "$target.elf"
$config = Join-Path $buildDir 'nds_build_config.h'
$sceneConfig = Join-Path $buildDir 'nds_scene_harness_config.h'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_SAMUS=1' 'NDS_P2_PROOF_FIGHTER0=3' `
        'NDS_TASK68_FALLBACK_CENSUS=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig, $nm)) {
    Assert-SamusNativeOwner (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus native-owner proof input is missing: $path"
}
$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_TICK_HUD 1',
    '#define NDS_P2_LUIGI 1',
    '#define NDS_P2_DONKEY 1',
    '#define NDS_P2_CAPTAIN 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_TASK68_FALLBACK_CENSUS 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusNativeOwner $configText.Contains($definition) `
        "Samus native-owner build is missing required definition: $definition" $config
}
Assert-SamusNativeOwner $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus native-owner proof must run the mode-163 BattleShip battle harness.' $sceneConfig

$elfSymbols = @(& $nm -a $elf)
Assert-SamusNativeOwner ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = $elfSymbols | Where-Object {
        $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$"
    } | Select-Object -First 1
    Assert-SamusNativeOwner ($null -ne $line) "ELF symbol not found: $Name"
    $m = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($m.Groups[1].Value, 16))
}

$osStopThread = Get-ElfSymbolAddress 'osStopThread'
foreach ($symbol in @(
    'gNdsTickHudNativeOwnerFallbackCount',
    'gNdsTickHudNativeOwnerFallbackByReason',
    'gNdsFighterNaturalMovesetMask',
    'gNdsFighterNaturalCombatStallCount',
    'gNdsFighterModelPartSetCount',
    'gNdsFighterModelPartOnCount',
    'gNdsFighterModelPartResetCount',
    'gNdsSceneBoundaryResult'
)) { [void](Get-ElfSymbolAddress $symbol) }

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\verification\2026-08-30_bug-samus-native-owner.txt'
} elseif (-not [System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

# BattleShip owns every status transition and every SetModelPartID event. This
# proof never writes status, motion, DObj, matrix or input state. It waits for
# the existing controller-driven mode-163 battle, then checks the renderer's
# complete native-owner census. A model-part count >0 proves the battle actually
# crossed a live source mutation seam rather than only rendering static Wait.
$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path $ctx.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        # Do not gate this proof on scVSBattleStartBattle.  The current
        # accurate-melonDS/GDB pair can stall before that symbolic/source seam
        # (the focused shield-roll verifier has the same constraint).  The
        # scene-boundary stop below is the actual acceptance seam: it is only
        # reached after the mode-163 battle has run and published its result,
        # and every native-owner/source-behavior counter is read there.
        ('tbreak *0x{0:x8} if gNdsSceneBoundaryResult != 0' -f $osStopThread),
        'continue',
        'printf "SAMUS_NATIVE=%u,%u,%u,%u,%#x,%u,%u,%u,%u\n", gNdsTickHudNativeOwnerFallbackByReason[0], gNdsTickHudNativeOwnerFallbackByReason[1], gNdsTickHudNativeOwnerFallbackCount, gNdsFighterNaturalCombatStallCount, gNdsFighterNaturalMovesetMask, gNdsFighterModelPartSetCount, gNdsFighterModelPartOnCount, gNdsFighterModelPartResetCount, gNdsSceneBoundaryResult',
        'printf "SAMUS_NATIVE_REASONS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTickHudNativeOwnerFallbackByReason[2],gNdsTickHudNativeOwnerFallbackByReason[3],gNdsTickHudNativeOwnerFallbackByReason[4],gNdsTickHudNativeOwnerFallbackByReason[5],gNdsTickHudNativeOwnerFallbackByReason[6],gNdsTickHudNativeOwnerFallbackByReason[7],gNdsTickHudNativeOwnerFallbackByReason[8],gNdsTickHudNativeOwnerFallbackByReason[9],gNdsTickHudNativeOwnerFallbackByReason[10],gNdsTickHudNativeOwnerFallbackByReason[11],gNdsTickHudNativeOwnerFallbackByReason[12],gNdsTickHudNativeOwnerFallbackByReason[13],gNdsTickHudNativeOwnerFallbackByReason[14]',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-native-owner.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-native-owner.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $stdout
    $match = [regex]::Match($stdout,
        'SAMUS_NATIVE=(\d+),(\d+),(\d+),(\d+),((?:0x[0-9a-fA-F]+)|\d+),(\d+),(\d+),(\d+),(\d+)')
    Assert-SamusNativeOwner $match.Success 'Samus native-owner marker is missing.' $stdout
    $calls = [int]$match.Groups[1].Value
    $eligible = [int]$match.Groups[2].Value
    $fallback = [int]$match.Groups[3].Value
    $stalls = [int]$match.Groups[4].Value
    $movesetText = $match.Groups[5].Value
    $moveset = if ($movesetText.StartsWith('0x')) {
        [Convert]::ToUInt32($movesetText.Substring(2), 16)
    } else {
        [Convert]::ToUInt32($movesetText, 10)
    }
    $modelPartSet = [int]$match.Groups[6].Value
    $boundary = [int]$match.Groups[9].Value
    Assert-SamusNativeOwner ($calls -gt 0 -and $eligible -eq $calls) `
        'Samus battle never exercised the production native-owner path.' $stdout
    Assert-SamusNativeOwner ($fallback -eq 0) `
        'Samus source-driven battle fell back from the production native owner.' $stdout
    # This tick-HUD census target does not run the fast-logic natural-moveset
    # tour; that behavior is covered by the shield-roll and weapon-transform
    # proofs.  Here the source-owned renderer contract is the subject: no stall,
    # every eligible draw stays native, and a real model-part mutation occurs.
    Assert-SamusNativeOwner ($stalls -eq 0) `
        'Samus native-owner census accumulated a battle stall.' $stdout
    Assert-SamusNativeOwner ($modelPartSet -gt 0) `
        'Samus proof never crossed a live SetModelPartID mutation.' $stdout
    Assert-SamusNativeOwner ($boundary -ne 0) `
        'Samus battle did not publish a scene-boundary result.' $stdout

    Write-Output ("P2 Samus native-owner proof passed: calls=$calls eligible=$eligible fallback=0 " +
        "modelpart=$modelPartSet censusMoveset=0x{0:x} stalls=0." -f $moveset)
    Write-Output $match.Value
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
