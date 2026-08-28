param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [int]$DelaySeconds = 0,
    [string]$Build = 'build-p2-samus-tumble-tour-fast',
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-SamusTumbleTour {
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

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_SAMUS=1' 'NDS_P2_PROOF_FIGHTER0=3' `
        'NDS_P2_SAMUS_STATE_TOUR=0' 'NDS_P2_SAMUS_TUMBLE_TOUR=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-SamusTumbleTour (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus tumble-tour proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_P2_SAMUS_STATE_TOUR 0',
    '#define NDS_P2_SAMUS_TUMBLE_TOUR 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusTumbleTour $configText.Contains($definition) `
        "Samus tumble-tour build is missing required definition: $definition" $config
}
Assert-SamusTumbleTour $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus tumble-tour proof must run the mode-163 battle_playable harness.' $sceneConfig

# The proof may stage geometry, damage percent, pose orientation and controller
# preconditions, but every claimed DamageFly/DamageFall/Passive/Down transition
# must still be selected by the real BattleShip hit/map/status code.
$movementPath = Join-Path $root 'src\port\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$tourStart = $movementText.IndexOf('static void ndsSamusTumbleTourRecord')
$tourEnd = $movementText.IndexOf('#endif', $tourStart)
Assert-SamusTumbleTour (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the Samus tumble-tour implementation for injection guard checks.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
Assert-SamusTumbleTour ($tourText -notmatch 'ftMainSetStatus\s*\(') `
    'Samus tumble-tour guest setup may not call ftMainSetStatus.'
Assert-SamusTumbleTour ($tourText -notmatch 'samus->status_id\s*=(?!=)') `
    'Samus tumble-tour guest setup may not assign status_id.'
Assert-SamusTumbleTour ($tourText -notmatch 'samus->motion_id\s*=(?!=)') `
    'Samus tumble-tour guest setup may not assign motion_id.'
Assert-SamusTumbleTour ($tourText -notmatch 'ftMainUpdateDamageStatFighter\s*\(') `
    'Samus tumble-tour may not inject fighter damage through ftMainUpdateDamageStatFighter.'
Assert-SamusTumbleTour ($tourText -notmatch 'ftCommonDamage\w*SetStatus\s*\(') `
    'Samus tumble-tour may not call a Damage status setter directly.'

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
    if ($DelaySeconds -gt 0) {
        throw 'DelaySeconds must remain 0 for the fast Samus tumble-tour proof.'
    }

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        # NAT_MOVESET can only become 0x7ff inside the mode-163 battle, so this
        # final-stop condition excludes startup/menu teardowns without relying
        # on a one-shot battle-start breakpoint that an unthrottled ROM can race.
        'tbreak osStopThread if (gNdsSceneBoundaryResult != 0) && (gNdsFighterNaturalMovesetMask == 0x7ff)',
        'continue',
        'printf "SAMUS_TUMBLE_TOUR=%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%#x,%u\\n",gNdsSamusTumbleTourScenario,gNdsSamusTumbleTourStep,gNdsSamusTumbleTourFrames,gNdsSamusTumbleTourDone,gNdsSamusTumbleTourMask,gNdsSamusTumbleTourDamageFlyMask,gNdsSamusTumbleTourHitCount,gNdsSamusTumbleTourStageCount,gNdsSamusTumbleTourStatus,gNdsSamusTumbleTourMotion,gNdsFighterNaturalMovesetMask,gNdsFighterNaturalCombatStallCount',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-tumble-tour.gdb' `
        -TimeoutSeconds 240 | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-tumble-tour.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'SAMUS_TUMBLE_TOUR=(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+)')
    Assert-SamusTumbleTour $match.Success 'Samus tumble-tour marker is missing.' $stdout
    $v = @($match.Groups[1..12] | ForEach-Object { $_.Value })
    $mask = [Convert]::ToUInt32($v[4].Substring(2), 16)
    $damageFlyMask = [Convert]::ToUInt32($v[5].Substring(2), 16)
    $naturalMask = [Convert]::ToUInt32($v[10].Substring(2), 16)

    Assert-SamusTumbleTour ([int]$v[0] -eq 11) `
        'Samus tumble tour did not complete all eleven recovery scenarios.' $stdout
    Assert-SamusTumbleTour ([int]$v[1] -eq 6) `
        'Samus tumble tour did not finish through the Recover step.' $stdout
    Assert-SamusTumbleTour ([int]$v[3] -eq 1) `
        'Samus tumble tour did not publish Done.' $stdout
    Assert-SamusTumbleTour ($mask -eq 0x1ffff) `
        ('Samus tumble/down/tech recovery mask is incomplete: got 0x{0:x}' -f $mask) $stdout
    Assert-SamusTumbleTour ($damageFlyMask -eq 0x8) `
        ('The real Fox up-smash entry no longer produces the source DamageFlyTop sample: got 0x{0:x}' -f $damageFlyMask) $stdout
    Assert-SamusTumbleTour ([int]$v[6] -eq 11) `
        'Every tumble-tour scenario must originate from its own real Fox hit.' $stdout
    Assert-SamusTumbleTour ([int]$v[7] -eq 22) `
        'Samus tumble tour must perform exactly hit + landing precondition staging per scenario.' $stdout
    Assert-SamusTumbleTour ($naturalMask -eq 0x7ff) `
        ('The prerequisite controller-driven common moveset regressed: got 0x{0:x}' -f $naturalMask) $stdout
    Assert-SamusTumbleTour ([int]$v[11] -eq 0) `
        'Samus tumble tour accumulated a natural-combat stall.' $stdout

    Write-Output ('P2-3 Samus tumble/down/tech recovery tour passed: ' +
        ('scenarios=11/11 hits=11 stages=22 mask=0x{0:x} fly=0x{1:x} ' -f $mask, $damageFlyMask) +
        ('NAT_MOVESET=0x{0:x} stalls=0.' -f $naturalMask))
    Write-Output ($match.Value)
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
