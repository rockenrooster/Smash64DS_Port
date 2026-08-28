param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\\emulators\\melonds\\melonDS.exe'),
    [string]$Gdb = 'C:\\devkitPro\\devkitARM\\bin\\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [int]$DelaySeconds = 0,
    [string]$Build = 'build-p2-samus-damagefly-tour-fast',
    [string]$Artifact = '',
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\\gdb-markers.ps1')

function Assert-SamusDamageFlyTour {
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
        'NDS_P2_SAMUS_STATE_TOUR=0' 'NDS_P2_SAMUS_TUMBLE_TOUR=0' `
        'NDS_P2_SAMUS_ATTACK_TOUR=0' 'NDS_P2_SAMUS_DAMAGEFLY_TOUR=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-SamusDamageFlyTour (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus DamageFly-tour proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_P2_SAMUS_STATE_TOUR 0',
    '#define NDS_P2_SAMUS_TUMBLE_TOUR 0',
    '#define NDS_P2_SAMUS_ATTACK_TOUR 0',
    '#define NDS_P2_SAMUS_DAMAGEFLY_TOUR 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusDamageFlyTour $configText.Contains($definition) `
        "Samus DamageFly-tour build is missing required definition: $definition" $config
}
Assert-SamusDamageFlyTour $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus DamageFly-tour proof must run the mode-163 battle_playable harness.' $sceneConfig

# This proof may stage spacing, percent and post-hit landing geometry. The hit,
# hurtbox placement, damage level, FlyTop angle override and FlyRoll RNG override
# must all remain BattleShip-owned. Fail closed if the driver starts injecting
# any of those outcomes.
$movementPath = Join-Path $root 'src\\port\\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$tourStart = $movementText.IndexOf('void ndsSamusDamageFlyTourProofStop')
$tourEnd = $movementText.IndexOf('#if NDS_P2_SAMUS_ATTACK_TOUR', $tourStart)
Assert-SamusDamageFlyTour (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the Samus DamageFly-tour implementation for injection guards.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
Assert-SamusDamageFlyTour ($tourText -notmatch 'ftMainSetStatus\s*\(') `
    'Samus DamageFly-tour guest setup may not call ftMainSetStatus.'
Assert-SamusDamageFlyTour ($tourText -notmatch 'samus->status_id\s*=(?!=)') `
    'Samus DamageFly-tour guest setup may not assign status_id.'
Assert-SamusDamageFlyTour ($tourText -notmatch 'samus->motion_id\s*=(?!=)') `
    'Samus DamageFly-tour guest setup may not assign motion_id.'
Assert-SamusDamageFlyTour ($tourText -notmatch 'ftMainUpdateDamageStatFighter\s*\(') `
    'Samus DamageFly-tour may not inject fighter damage.'
Assert-SamusDamageFlyTour ($tourText -notmatch 'ftCommonDamage\w*SetStatus\s*\(') `
    'Samus DamageFly-tour may not call a Damage status setter directly.'
Assert-SamusDamageFlyTour ($tourText -notmatch 'syUtilsRand\w*\s*\(') `
    'Samus DamageFly-tour may not sample or manipulate RNG; FlyRoll belongs to BattleShip.'

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
        throw 'DelaySeconds must remain 0 for the fast Samus DamageFly-tour proof.'
    }

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        'tbreak ndsSamusDamageFlyTourProofStop',
        'continue',
        'printf "SAMUS_DAMAGEFLY_TOUR=%u,%u,%u,%u,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u\\n",gNdsSamusDamageFlyTourScenario,gNdsSamusDamageFlyTourStep,gNdsSamusDamageFlyTourFrames,gNdsSamusDamageFlyTourDone,gNdsSamusDamageFlyTourMask,gNdsSamusDamageFlyTourAttackerMask,gNdsSamusDamageFlyTourPlacementPacked,gNdsSamusDamageFlyTourHitCount,gNdsSamusDamageFlyTourRollAttempts,gNdsSamusDamageFlyTourSakuraiHitCount,gNdsSamusDamageFlyTourTopAngle80Count,gNdsSamusDamageFlyTourRollPercent,gNdsSamusDamageFlyTourMismatchCount,gNdsSamusDamageFlyTourStageCount,gNdsSamusDamageFlyTourTerminalCount,gNdsSamusDamageFlyTourStatus,gNdsFighterNaturalMovesetMask,gNdsFighterNaturalCombatStallCount,gNdsFtPoseTrackOverflow',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-damagefly-tour.gdb' `
        -TimeoutSeconds 300 | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-damagefly-tour.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'SAMUS_DAMAGEFLY_TOUR=(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+)')
    Assert-SamusDamageFlyTour $match.Success 'Samus DamageFly-tour marker is missing.' $stdout
    $v = @($match.Groups[1..19] | ForEach-Object { $_.Value })
    $mask = [Convert]::ToUInt32($v[4].Substring(2), 16)
    $attackerMask = [Convert]::ToUInt32($v[5].Substring(2), 16)
    $placements = [Convert]::ToUInt32($v[6].Substring(2), 16)
    $naturalMask = [Convert]::ToUInt32($v[16].Substring(2), 16)
    $hiPlacement = $placements -band 7
    $nPlacement = ($placements -shr 3) -band 7
    $lwPlacement = ($placements -shr 6) -band 7

    Assert-SamusDamageFlyTour ([int]$v[0] -eq 5) `
        'Samus DamageFly tour did not complete all five family scenarios.' $stdout
    Assert-SamusDamageFlyTour ([int]$v[3] -eq 1) `
        'Samus DamageFly tour did not publish a successful Done state.' $stdout
    Assert-SamusDamageFlyTour ($mask -eq 0x1f) `
        ('DamageFly family mask is incomplete: got 0x{0:x}' -f $mask) $stdout
    Assert-SamusDamageFlyTour ($attackerMask -eq 0xf) `
        ('Fox source attack-status mask is incomplete: got 0x{0:x}' -f $attackerMask) $stdout
    Assert-SamusDamageFlyTour (($hiPlacement -eq 2) -and ($nPlacement -eq 1) -and
        ($lwPlacement -eq 0)) `
        ('DamageFly Hi/N/Lw hurtbox placements are not source 2/1/0: got {0}/{1}/{2}' -f
            $hiPlacement, $nPlacement, $lwPlacement) $stdout
    Assert-SamusDamageFlyTour ([int]$v[7] -ge 5) `
        'Each DamageFly family member must originate from a real fighter hit.' $stdout
    Assert-SamusDamageFlyTour (([int]$v[8] -ge 1) -and ([int]$v[8] -le 32)) `
        'FlyRoll must be selected by BattleShip within the bounded real-hit retry window.' $stdout
    Assert-SamusDamageFlyTour ([int]$v[9] -eq (2 + [int]$v[8])) `
        'The head N-air, neutral F-tilt and every Roll attempt must retain the source 361 angle.' $stdout
    Assert-SamusDamageFlyTour ([int]$v[10] -eq 1) `
        'FlyTop must come from exactly one real Fox 80-degree up-smash hit.' $stdout
    Assert-SamusDamageFlyTour ([int]$v[11] -ge 100) `
        'FlyRoll was not selected at or above the source 100-percent gate.' $stdout
    Assert-SamusDamageFlyTour ([int]$v[12] -eq 0) `
        'A non-Roll scenario produced the wrong DamageFly status/placement.' $stdout
    Assert-SamusDamageFlyTour ([int]$v[14] -eq 1) `
        'Samus DamageFly-tour cache-coherent terminal did not execute exactly once.' $stdout
    Assert-SamusDamageFlyTour ($naturalMask -eq 0x7ff) `
        ('The prerequisite controller-driven common moveset regressed: got 0x{0:x}' -f $naturalMask) $stdout
    Assert-SamusDamageFlyTour ([int]$v[17] -eq 0) `
        'Samus DamageFly tour accumulated a natural-combat stall.' $stdout
    Assert-SamusDamageFlyTour ([int]$v[18] -eq 0) `
        'Samus DamageFly tour exhausted the DS fighter-pose track pool.' $stdout

    $summary = ('P2-3 Samus DamageFly family tour passed: ' +
        ('mask=0x{0:x} placements={1}/{2}/{3} hits={4} rollAttempts={5} ' -f
            $mask, $hiPlacement, $nPlacement, $lwPlacement, $v[7], $v[8]) +
        ('NAT_MOVESET=0x{0:x} stalls=0.' -f $naturalMask))
    Write-Output $summary
    Write-Output ($match.Value)
    if (-not [string]::IsNullOrWhiteSpace($Artifact)) {
        $artifactPath = if ([IO.Path]::IsPathRooted($Artifact)) {
            $Artifact
        } else {
            Join-Path $root $Artifact
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifactPath) |
            Out-Null
        Set-Content -LiteralPath $artifactPath -Value @($summary, $match.Value)
    }
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
