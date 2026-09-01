param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [int]$DelaySeconds = 0,
    [string]$Build = 'build-p2-samus-state-tour-fast',
    [switch]$NoBuild,
    [ValidateRange(240,900)][int]$TimeoutSeconds = 600
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-SamusTour {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if (-not $Condition) {
        if ($Evidence) { throw "$Message`n$Evidence" }
        throw $Message
    }
}

$target = 'smash64ds-battle-playable-fast-hwtri'
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
        'NDS_P2_SAMUS_STATE_TOUR=1' 'NDS_TASK68_FALLBACK_CENSUS=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig, $nm)) {
    Assert-SamusTour (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus state-tour proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_P2_SAMUS_STATE_TOUR 1',
    '#define NDS_TASK68_FALLBACK_CENSUS 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusTour $configText.Contains($definition) `
        "Samus state-tour build is missing required definition: $definition" $config
}
Assert-SamusTour $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus state-tour proof must run the mode-163 battle_playable harness.' $sceneConfig

$elfSymbols = @(& $nm -a $elf)
Assert-SamusTour ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = $elfSymbols | Where-Object {
        $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$"
    } | Select-Object -First 1
    Assert-SamusTour ($null -ne $line) "ELF symbol not found: $Name"
    $m = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($m.Groups[1].Value, 16))
}
$battleStart = Get-ElfSymbolAddress 'scVSBattleStartBattle'
$osStopThread = Get-ElfSymbolAddress 'osStopThread'

# The acceptance claim is specifically source-selected states.  Keep the two
# guest staging helpers limited to geometry/damage/facing preconditions: a
# future edit may not turn this into another synthetic status-prime harness.
$movementPath = Join-Path $root 'src\port\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$tourStart = $movementText.IndexOf('static sb32 ndsSamusStateTourPrepareLedge')
$tourEnd = $movementText.IndexOf('#endif', $tourStart)
Assert-SamusTour (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the Samus state-tour implementation for injection guard checks.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
Assert-SamusTour ($tourText -notmatch 'ftMainSetStatus\s*\(') `
    'Samus state-tour guest setup may not call ftMainSetStatus.'
Assert-SamusTour ($tourText -notmatch 'samus->status_id\s*=(?!=)') `
    'Samus state-tour guest setup may not assign status_id.'
Assert-SamusTour ($tourText -notmatch 'samus->motion_id\s*=(?!=)') `
    'Samus state-tour guest setup may not assign motion_id.'

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
    # The fast mode-163 ROM is intentionally unthrottled.  Install the one-shot
    # battle-start breakpoint as soon as the GDB listener exists; a wall-clock
    # delay here can let scVSBattleStartBattle execute before GDB attaches and
    # turn a healthy proof into a timeout waiting for a call that already ran.
    if ($DelaySeconds -gt 0) {
        throw 'DelaySeconds must remain 0 for the fast Samus state-tour proof.'
    }

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        # P2 production proofs must enter the battle before the scene-boundary
        # stop is armed; startup/menu teardowns also publish a boundary result.
        ('tbreak *0x{0:x8}' -f $battleStart),
        'continue',
        ('tbreak *0x{0:x8} if gNdsSceneBoundaryResult != 0' -f $osStopThread),
        'continue',
        'printf "SAMUS_LEDGE_TOUR=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%d,%#x,%u,%u,%u\\n",gNdsSamusStateTourPhase,gNdsSamusStateTourPhaseFrames,sNdsSamusStateTourScenario,sNdsSamusStateTourStep,sNdsSamusStateTourFrames,sNdsSamusStateTourActive,sNdsSamusStateTourDone,gNdsSamusStateTourStageCount,gNdsSamusStateTourMask,gNdsSamusStateTourStatus,gNdsSamusStateTourMotion,gNdsSamusStateTourCliffID,gNdsFighterNaturalMovesetMask,gNdsFighterNaturalCombatStallCount,gNdsFighterNaturalCombatRollFrames,gNdsFighterNaturalCombatRollStatus',
        'printf "SAMUS_NATIVE calls=%u eligible=%u fallback=%u reasons=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\\n",gNdsTickHudNativeOwnerFallbackByReason[0],gNdsTickHudNativeOwnerFallbackByReason[1],gNdsTickHudNativeOwnerFallbackCount,gNdsTickHudNativeOwnerFallbackByReason[2],gNdsTickHudNativeOwnerFallbackByReason[3],gNdsTickHudNativeOwnerFallbackByReason[4],gNdsTickHudNativeOwnerFallbackByReason[5],gNdsTickHudNativeOwnerFallbackByReason[6],gNdsTickHudNativeOwnerFallbackByReason[7],gNdsTickHudNativeOwnerFallbackByReason[8],gNdsTickHudNativeOwnerFallbackByReason[9],gNdsTickHudNativeOwnerFallbackByReason[10],gNdsTickHudNativeOwnerFallbackByReason[11],gNdsTickHudNativeOwnerFallbackByReason[12]',
        'detach',
        'quit'
    )
    # Accurate-cache mode can spend more than 120 s walking CSS/entry on a cold
    # host before mode 163 reaches the battle.  This is a ceiling, not a sleep;
    # successful runs still return as soon as the guest publishes the bounded
    # battle teardown marker.
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-state-tour.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-state-tour.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'SAMUS_LEDGE_TOUR=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+),(-?\d+),(0x[0-9a-fA-F]+),(\d+),(\d+),(\d+)')
    $native = [regex]::Match($stdout,
        'SAMUS_NATIVE calls=(\d+) eligible=(\d+) fallback=(\d+) reasons=([0-9,]+)')
    Assert-SamusTour $match.Success 'Samus ledge-tour marker is missing.' $stdout
    $v = @($match.Groups[1..16] | ForEach-Object { $_.Value })
    $mask = [Convert]::ToUInt32($v[8].Substring(2), 16)
    $naturalMask = [Convert]::ToUInt32($v[12].Substring(2), 16)
    Assert-SamusTour ([int]$v[0] -eq 6) 'Samus ledge tour did not reach Done phase.' $stdout
    Assert-SamusTour ([int]$v[2] -eq 6) 'Samus ledge tour did not complete all six source scenarios.' $stdout
    Assert-SamusTour ([int]$v[3] -eq 4) 'Samus ledge tour did not finish through the Recover step.' $stdout
    Assert-SamusTour ([int]$v[5] -eq 1 -and [int]$v[6] -eq 1) `
        'Samus ledge tour never armed or never published Done.' $stdout
    Assert-SamusTour ([int]$v[7] -eq 12) `
        'Samus ledge tour must perform exactly two guest precondition stages per scenario.' $stdout
    Assert-SamusTour ($mask -eq 0x1ffff) `
        ('Samus ledge tour did not visit the complete Fall/Cliff quick+slow state mask: got 0x{0:x}' -f $mask) $stdout
    Assert-SamusTour ($naturalMask -eq 0x7ff) `
        ('The prerequisite controller-driven common moveset regressed: got 0x{0:x}' -f $naturalMask) $stdout
    Assert-SamusTour ([int]$v[13] -eq 0) 'Samus ledge tour accumulated a natural-combat stall.' $stdout
    Assert-SamusTour ([int]$v[14] -gt 0 -and [int]$v[15] -in @(156, 157)) `
        'Samus prerequisite combat did not enter BattleShip EscapeF/EscapeB from guarded fresh-stick input.' $stdout
    Assert-SamusTour $native.Success 'Samus native/fallback census marker is missing.' $stdout
    Assert-SamusTour ([int]$native.Groups[1].Value -gt 0 -and
        [int]$native.Groups[2].Value -gt 0 -and
        [int]$native.Groups[3].Value -eq 0) `
        'Samus state tour still entered a slow/generic fighter fallback.' $stdout

    Write-Output ('P2-3 Samus natural ledge tour passed: scenarios=6/6 stages=12 ' +
        ('mask=0x{0:x} NAT_MOVESET=0x{1:x} roll={2}/{3} stalls=0.' -f
            $mask, $naturalMask, [int]$v[14], [int]$v[15]))
    Write-Output ($match.Value)
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
