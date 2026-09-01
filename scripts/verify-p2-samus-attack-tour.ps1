param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [int]$DelaySeconds = 0,
    [string]$Build = 'build-p2-samus-attack-tour-fast',
    [string]$Artifact = '',
    [switch]$TickHudNative,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-SamusAttackTour {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if (-not $Condition) {
        if ($Evidence) { throw "$Message`n$Evidence" }
        throw $Message
    }
}

$target = if ($TickHudNative) {
    'smash64ds-battle-playable-tickhud-hwtri'
} else {
    'smash64ds-battle-playable-fast-hwtri'
}
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
        'NDS_P2_SAMUS_STATE_TOUR=0' 'NDS_P2_SAMUS_TUMBLE_TOUR=0' `
        'NDS_P2_SAMUS_ATTACK_TOUR=1' `
        $(if ($TickHudNative) { 'NDS_TASK68_FALLBACK_CENSUS=1' } else { 'NDS_TASK68_FALLBACK_CENSUS=0' })
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig, $nm)) {
    Assert-SamusAttackTour (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus attack-tour proof input is missing: $path"
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
    '#define NDS_P2_SAMUS_ATTACK_TOUR 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusAttackTour $configText.Contains($definition) `
        "Samus attack-tour build is missing required definition: $definition" $config
}
Assert-SamusAttackTour $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus attack-tour proof must run the mode-163 battle_playable harness.' $sceneConfig
if ($TickHudNative) {
    Assert-SamusAttackTour $configText.Contains('#define NDS_TICK_HUD 1') `
        'Samus native attack-tour proof requires the tick-HUD target.' $config
    Assert-SamusAttackTour $configText.Contains('#define NDS_TASK68_FALLBACK_CENSUS 1') `
        'Samus native attack-tour proof requires fallback census.' $config
}

$elfSymbols = @(& $nm -a $elf)
Assert-SamusAttackTour ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = $elfSymbols | Where-Object {
        $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$"
    } | Select-Object -First 1
    Assert-SamusAttackTour ($null -ne $line) "ELF symbol not found: $Name"
    $m = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($m.Groups[1].Value, 16))
}
$tourStop = Get-ElfSymbolAddress 'ndsSamusAttackTourProofStop'
if ($TickHudNative) {
    foreach ($symbol in @(
        'gNdsTickHudNativeOwnerFallbackCount',
        'gNdsTickHudNativeOwnerFallbackByReason'
    )) { [void](Get-ElfSymbolAddress $symbol) }
}

# The proof may stage fighter spacing and controller release between scenarios.
# Every claimed move must still be selected by BattleShip's ordinary input
# interrupt/status path. Fail closed if the proof driver starts injecting state.
$movementPath = Join-Path $root 'src\port\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$tourStart = $movementText.IndexOf('void ndsSamusAttackTourProofTerminal')
$tourEnd = $movementText.IndexOf('#endif', $tourStart)
Assert-SamusAttackTour (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the Samus attack-tour implementation for injection guards.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
Assert-SamusAttackTour ($tourText -notmatch 'ftMainSetStatus\s*\(') `
    'Samus attack-tour guest setup may not call ftMainSetStatus.'
Assert-SamusAttackTour ($tourText -notmatch 'samus->status_id\s*=(?!=)') `
    'Samus attack-tour guest setup may not assign status_id.'
Assert-SamusAttackTour ($tourText -notmatch 'samus->motion_id\s*=(?!=)') `
    'Samus attack-tour guest setup may not assign motion_id.'
Assert-SamusAttackTour ($tourText -notmatch 'ftCommonAttack\w*SetStatus\s*\(') `
    'Samus attack-tour may not call an attack status setter directly.'
Assert-SamusAttackTour ($tourText -notmatch 'ftCommonThrow\w*SetStatus\s*\(') `
    'Samus attack-tour may not call a throw status setter directly.'

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
        throw 'DelaySeconds must remain 0 for the fast Samus attack-tour proof.'
    }

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        ('tbreak *0x{0:x8}' -f $tourStop),
        'continue',
        'printf "SAMUS_ATTACK_TOUR=%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%#x,%u,%#x,%#x,%u,%#x,%d,%d,%d,%d,%#x,%u,%u\\n",gNdsSamusAttackTourScenario,gNdsSamusAttackTourStep,gNdsSamusAttackTourFrames,gNdsSamusAttackTourDone,gNdsSamusAttackTourMask,gNdsSamusAttackTourStageCount,gNdsSamusAttackTourTerminalCount,gNdsSamusAttackTourStatus,gNdsSamusAttackTourMotion,gNdsSamusAttackTourCatchAttr,gNdsSamusAttackTourGrabInputCount,gNdsSamusAttackTourCatchStatusMask,gNdsSamusAttackTourCatchFrames,gNdsSamusAttackTourCatchActiveFrames,gNdsSamusAttackTourCatchSearchFrames,gNdsSamusAttackTourCatchAttackMask,gNdsSamusAttackTourCatchAnimFrameMaxMilli,gNdsSamusAttackTourVictimGrabbableMask,gNdsSamusAttackTourVictimNormalMask,gNdsSamusAttackTourJoint36SeenCount,gNdsSamusAttackTourJoint36AttackMask,gNdsSamusAttackTourMinGrabDXMilli,gNdsSamusAttackTourGrab0XMilli,gNdsSamusAttackTourGrab1XMilli,gNdsSamusAttackTourFoxXMilli,gNdsFighterNaturalMovesetMask,gNdsFighterNaturalCombatStallCount,gNdsFtPoseTrackOverflow',
        $(if ($TickHudNative) {
            'printf "SAMUS_ATTACK_NATIVE=%u,%u,%u\n", gNdsTickHudNativeOwnerFallbackByReason[0], gNdsTickHudNativeOwnerFallbackByReason[1], gNdsTickHudNativeOwnerFallbackCount'
        } else { 'echo' }),
        $(if ($TickHudNative) {
            'printf "SAMUS_ATTACK_NATIVE_REASONS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTickHudNativeOwnerFallbackByReason[2],gNdsTickHudNativeOwnerFallbackByReason[3],gNdsTickHudNativeOwnerFallbackByReason[4],gNdsTickHudNativeOwnerFallbackByReason[5],gNdsTickHudNativeOwnerFallbackByReason[6],gNdsTickHudNativeOwnerFallbackByReason[7],gNdsTickHudNativeOwnerFallbackByReason[8],gNdsTickHudNativeOwnerFallbackByReason[9],gNdsTickHudNativeOwnerFallbackByReason[10],gNdsTickHudNativeOwnerFallbackByReason[11],gNdsTickHudNativeOwnerFallbackByReason[12],gNdsTickHudNativeOwnerFallbackByReason[13],gNdsTickHudNativeOwnerFallbackByReason[14]'
        } else { 'echo' }),
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-attack-tour.gdb' `
        -TimeoutSeconds 240 | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-attack-tour.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'SAMUS_ATTACK_TOUR=(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(0x[0-9a-fA-F]+),(0x[0-9a-fA-F]+),(\d+),(0x[0-9a-fA-F]+),(-?\d+),(-?\d+),(-?\d+),(-?\d+),(0x[0-9a-fA-F]+),(\d+),(\d+)')
    Assert-SamusAttackTour $match.Success 'Samus attack-tour marker is missing.' $stdout
    $v = @($match.Groups[1..28] | ForEach-Object { $_.Value })
    $mask = [Convert]::ToUInt32($v[4].Substring(2), 16)
    $catchMask = [Convert]::ToUInt32($v[11].Substring(2), 16)
    $catchAttackMask = [Convert]::ToUInt32($v[15].Substring(2), 16)
    $victimGrabbableMask = [Convert]::ToUInt32($v[17].Substring(2), 16)
    $victimNormalMask = [Convert]::ToUInt32($v[18].Substring(2), 16)
    $joint36AttackMask = [Convert]::ToUInt32($v[20].Substring(2), 16)
    $naturalMask = [Convert]::ToUInt32($v[25].Substring(2), 16)

    Assert-SamusAttackTour ([int]$v[0] -eq 23) `
        'Samus attack tour did not complete all twenty-three scenarios.' $stdout
    Assert-SamusAttackTour ([int]$v[1] -eq 3) `
        'Samus attack tour did not finish through the Recover step.' $stdout
    Assert-SamusAttackTour ([int]$v[3] -eq 1) `
        'Samus attack tour did not publish Done.' $stdout
    Assert-SamusAttackTour ($mask -eq 0x00ffffff) `
        ('Samus attack/throw status mask is incomplete: got 0x{0:x}' -f $mask) $stdout
    Assert-SamusAttackTour ([int]$v[5] -eq 23) `
        'Samus attack tour must stage exactly one controller scenario per planned move.' $stdout
    Assert-SamusAttackTour ([int]$v[9] -eq 1) `
        'Samus source FTAttributes no longer advertises her grab.' $stdout
    Assert-SamusAttackTour ([int]$v[10] -gt 0) `
        'Samus attack tour never delivered a processed Z-hold + A-tap grab input.' $stdout
    Assert-SamusAttackTour ($catchMask -eq 0x7) `
        ('Samus grab chain did not visit Catch/CatchPull/CatchWait: got 0x{0:x}' -f $catchMask) $stdout
    Assert-SamusAttackTour ([int]$v[12] -gt 0) `
        'Samus grab never spent a sampled update in Catch.' $stdout
    Assert-SamusAttackTour ([int]$v[13] -gt 0) `
        'Samus Catch never retained the source is_catchstatus owner.' $stdout
    Assert-SamusAttackTour ($catchAttackMask -ne 0) `
        'Samus Catch motion never activated any of its source grapple collision entries.' $stdout
    Assert-SamusAttackTour ($victimGrabbableMask -ne 0) `
        'Fox exposes no grabbable damage-collision entries to the source catch search.' $stdout
    Assert-SamusAttackTour (($victimGrabbableMask -band $victimNormalMask) -ne 0) `
        'Fox has no damage-collision entry that is both normal and grabbable.' $stdout
    Assert-SamusAttackTour ([int]$v[19] -gt 0) `
        'Samus Catch never created source hidden joint 36.' $stdout
    Assert-SamusAttackTour (($joint36AttackMask -band 0x3) -eq 0x3) `
        ('Samus grapple collisions are not attached to live hidden joint 36: got 0x{0:x}' -f $joint36AttackMask) $stdout
    Assert-SamusAttackTour ($naturalMask -eq 0x7ff) `
        ('The prerequisite controller-driven common moveset regressed: got 0x{0:x}' -f $naturalMask) $stdout
    Assert-SamusAttackTour ([int]$v[26] -eq 0) `
        'Samus attack tour accumulated a natural-combat stall.' $stdout
    Assert-SamusAttackTour ([int]$v[27] -eq 0) `
        'Samus attack tour exhausted the DS fighter-pose track pool.' $stdout
    if ($TickHudNative) {
        $native = [regex]::Match($stdout, 'SAMUS_ATTACK_NATIVE=(\d+),(\d+),(\d+)')
        Assert-SamusAttackTour $native.Success `
            'Samus native attack-tour census marker is missing.' $stdout
        Assert-SamusAttackTour ([int]$native.Groups[1].Value -gt 0 -and
                                [int]$native.Groups[2].Value -gt 0) `
            'Samus attack tour did not exercise the production native owner.' $stdout
        Assert-SamusAttackTour ([int]$native.Groups[3].Value -eq 0) `
            'Samus attack/throw tour fell back from the production native owner.' $stdout
    }

    $summary = ('P2-3 Samus exhaustive attack/throw tour passed: ' +
        ('scenarios=23/23 statuses=24/24 mask=0x{0:x} stages=23 ' -f $mask) +
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
