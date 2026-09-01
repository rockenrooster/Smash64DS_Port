param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [string]$Build = 'build-bugs-samus-roll',
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-SamusRoll {
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
        'NDS_P2_SAMUS=1' 'NDS_P2_PROOF_FIGHTER0=3'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-SamusRoll (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus shield-roll proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_P2_LUIGI 1',
    '#define NDS_P2_DONKEY 1',
    '#define NDS_P2_CAPTAIN 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusRoll $configText.Contains($definition) `
        "Samus shield-roll build is missing required definition: $definition" $config
}
Assert-SamusRoll $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus shield-roll proof must run the mode-163 battle_playable harness.' $sceneConfig

# This proof must remain controller-driven. The production input/status code is
# not patched or poked by GDB: the bounded battle harness supplies Z and a fresh
# +/-80 stick tap, BattleShip owns tap_stick_x and Guard's interrupt chain, and
# these counters merely observe the resulting statuses.
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
        # Do not gate this proof on a symbolic scVSBattleStartBattle stop.  The
        # current accurate-melonDS/GDB pair has an old host-encoding failure on
        # that symbol breakpoint and can wait forever before the proof starts.
        # The scene-boundary stop below is the actual bounded acceptance seam:
        # it is reached only after the battle harness has run and published its
        # result, and every asserted counter is read there.
        'tbreak osStopThread if gNdsSceneBoundaryResult != 0',
        'continue',
        'printf "SAMUS_ROLL=%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%#x\n", gNdsFighterNaturalCombatGuardOnFrames, gNdsFighterNaturalCombatGuardFrames, gNdsFighterNaturalCombatGuardOffFrames, gNdsFighterNaturalCombatRollFrames, gNdsFighterNaturalCombatRollStatus, gNdsFighterNaturalCombatPhase, gNdsFighterNaturalCombatStallCount, gNdsFighterNaturalMovesetMask, gNdsSceneBoundaryResult, gNdsFighterNaturalCombatAppealFrames, gNdsFighterNaturalCombatAppealStatus, gNdsBootSelfTestResult',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-shield-roll.gdb' `
        -TimeoutSeconds 240 | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-shield-roll.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'SAMUS_ROLL=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+)')
    Assert-SamusRoll $match.Success 'Samus shield-roll marker is missing.' $stdout
    $v = @($match.Groups[1..12] | ForEach-Object { $_.Value })
    $rollStatus = [int]$v[4]
    $naturalMask = [Convert]::ToUInt32($v[7].Substring(2), 16)
    $bootSelfTest = [Convert]::ToUInt32($v[11].Substring(2), 16)
    Assert-SamusRoll ([int]$v[0] -gt 0 -and [int]$v[1] -ge 10 -and [int]$v[2] -gt 0) `
        'Samus source GuardOn/Guard/GuardOff lifecycle did not complete.' $stdout
    Assert-SamusRoll ([int]$v[3] -gt 0 -and $rollStatus -in @(156, 157)) `
        'Samus did not enter BattleShip EscapeF/EscapeB from guarded fresh-stick input.' $stdout
    Assert-SamusRoll ([int]$v[6] -eq 0) 'Samus natural combat accumulated a stall.' $stdout
    Assert-SamusRoll ($naturalMask -eq 0x7ff) `
        ('The prerequisite controller-driven common moveset regressed: got 0x{0:x}' -f $naturalMask) $stdout
    Assert-SamusRoll ([int]$v[8] -ne 0) 'Bounded battle did not publish a scene-boundary result.' $stdout
    Assert-SamusRoll ([int]$v[9] -gt 0 -and [int]$v[10] -eq 189) `
        'The common L-trigger input did not enter BattleShip Appeal.' $stdout
    # Boot self-test includes ndsControllerBackendSelfTest.  The production
    # controller backend's Select->L mapping and the guest-driven Appeal proof
    # together close the DS-button-to-source-status path without GDB input.
    Assert-SamusRoll ($bootSelfTest -eq 0x50415353) `
        'Boot/controller self-test failed; DS SELECT -> N64 L is not proven.' $stdout

    Write-Output ('P2-3 Samus shield roll + common taunt passed: rollFrames={0} status={1} ' +
        'guard={2}/{3}/{4} appeal={5}/{6} SELFTEST=PASS NAT_MOVESET=0x{7:x} stalls=0.' -f
        [int]$v[3], $rollStatus, [int]$v[0], [int]$v[1], [int]$v[2],
        [int]$v[9], [int]$v[10], $naturalMask)
    Write-Output $match.Value
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
