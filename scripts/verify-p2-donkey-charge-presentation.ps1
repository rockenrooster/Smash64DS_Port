param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [string]$Build = 'build-bugfix-modelparts',
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-DonkeyCharge {
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
        'NDS_P2_SAMUS=1' 'NDS_P2_PROOF_FIGHTER0=2'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-DonkeyCharge (Test-Path -LiteralPath $path -PathType Leaf) `
        "Donkey charge-presentation proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_P2_LUIGI 1',
    '#define NDS_P2_DONKEY 1',
    '#define NDS_P2_PROOF_FIGHTER0 2',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1',
    '#define NDS_TASK39_FX_FLASH 1'
)) {
    Assert-DonkeyCharge $configText.Contains($definition) `
        "Donkey charge-presentation build is missing required definition: $definition" $config
}
Assert-DonkeyCharge $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Donkey charge-presentation proof must run the mode-163 battle_playable harness.' $sceneConfig

# BattleShip owns the entire natural path.  The bounded proof presses B, stores
# charge, resumes it, reaches full charge and returns to Wait.  This verifier
# only reads the source-state counters plus the owner-approved ColAnim seam:
# request 48 (FoxSpecialHi, as authored in both Donkey Giant Punch loop scripts)
# must be substituted with 6
# (CommonSpecialNCharge).  No GDB status/input poke is used.
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
        'tbreak osStopThread if gNdsSceneBoundaryResult != 0',
        'continue',
        'printf "DKCHARGE=%u,%d,%d,%u,%u,%u,%u,%#x,%u,%u\n", gNdsDonkeyChargePresentationOverrideCount, gNdsDonkeyChargePresentationRequestedID, gNdsDonkeyChargePresentationAppliedID, gNdsFighterDonkeySpecialsNLoopFrames, gNdsFighterDonkeySpecialsNStoredChargeMax, gNdsFighterDonkeySpecialsNEndFrames, gNdsFighterDonkeySpecialsNPassiveResetFrames, gNdsFighterEffectKindMask2, gNdsFighterNaturalCombatStallCount, gNdsSceneBoundaryResult',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-donkey-charge-presentation.gdb' `
        -TimeoutSeconds 240 | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR `
        'p2-donkey-charge-presentation.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'DKCHARGE=(\d+),(-?\d+),(-?\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+)')
    Assert-DonkeyCharge $match.Success 'Donkey charge-presentation marker is missing.' $stdout
    $count = [uint32]$match.Groups[1].Value
    $requested = [int]$match.Groups[2].Value
    $applied = [int]$match.Groups[3].Value
    $loopFrames = [uint32]$match.Groups[4].Value
    $chargeMax = [uint32]$match.Groups[5].Value
    $endFrames = [uint32]$match.Groups[6].Value
    $resetFrames = [uint32]$match.Groups[7].Value
    $effectMask2 = [Convert]::ToUInt32($match.Groups[8].Value.Substring(2), 16)
    $stalls = [uint32]$match.Groups[9].Value
    $boundary = [uint32]$match.Groups[10].Value

    Assert-DonkeyCharge ($count -gt 0) `
        'BattleShip never requested Donkey SpecialNLoop ColAnim on the natural charge path.' $stdout
    Assert-DonkeyCharge ($requested -eq 48 -and $applied -eq 6) `
        'Owner override did not map source Donkey loop FoxSpecialHi (48) to CommonSpecialNCharge (6).' $stdout
    Assert-DonkeyCharge ($loopFrames -gt 0 -and $chargeMax -ge 10) `
        'Natural Giant Punch did not enter its loop and reach source full-charge level 10.' $stdout
    Assert-DonkeyCharge ($endFrames -gt 0 -and $resetFrames -gt 0) `
        'Natural Giant Punch did not release/settle and reset its passive charge.' $stdout
    Assert-DonkeyCharge (($effectMask2 -band 0x200) -ne 0) `
        'Full-charge source ChargeSparkle effect (kind 73 / mask2 bit 9) was not observed.' $stdout
    Assert-DonkeyCharge ($stalls -eq 0 -and $boundary -ne 0) `
        'Donkey natural-special proof stalled or never reached its bounded scene boundary.' $stdout

    Write-Output ("Donkey owner charge presentation passed: overrides={0} request={1} applied={2} loop={3} chargeMax={4} sparkle=0x{5:x}." -f `
        $count, $requested, $applied, $loopFrames, $chargeMax, $effectMask2)
    Write-Output $match.Value
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
