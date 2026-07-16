[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [ValidateRange(30, 3600)]
    [int]$TimeoutSeconds = 900,
    [switch]$NoBuild,
    [string]$Rom = '',
    [string]$Elf = '',
    [string]$Screenshot = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$target = 'smash64ds-battle-playable-hwtri'
$build = 'build-battle-playable-canonical-hwtri-harness'
$requestedRom = $Rom
$requestedElf = $Elf
$customArtifacts = (-not [string]::IsNullOrWhiteSpace($requestedRom)) -or
    (-not [string]::IsNullOrWhiteSpace($requestedElf))
if ($customArtifacts -and (-not $NoBuild)) {
    throw 'Custom -Rom/-Elf artifacts require -NoBuild.'
}
if ($customArtifacts -and
    ([string]::IsNullOrWhiteSpace($requestedRom) -or
     [string]::IsNullOrWhiteSpace($requestedElf))) {
    throw 'Custom recovery verification requires both -Rom and -Elf.'
}
$rom = if ($customArtifacts) {
    [System.IO.Path]::GetFullPath($requestedRom)
} else {
    Join-Path $root "$target.nds"
}
$elf = if ($customArtifacts) {
    [System.IO.Path]::GetFullPath($requestedElf)
} else {
    Join-Path $root "$target.elf"
}
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir `
    -Root $root `
    -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-playable-damagefall-recovery.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-playable-damagefall-recovery.stderr.log'
$scriptName = '_battle_playable_damagefall_recovery.gdb'
$captureScript = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$configState = $null
$emulator = $null
$verificationPassed = $false
$failureRecord = $null

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}

function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') {
            $values += [int64](Convert-MarkerUInt32 $text)
        } else {
            $values += [int64]$text
        }
    }
    return $values
}

function Convert-UInt32BitsToSingle {
    param([int64]$Bits)
    return [BitConverter]::ToSingle(
        [BitConverter]::GetBytes([uint32]$Bits), 0)
}

function Test-FiniteSingle {
    param([single]$Value)
    return (-not [Single]::IsNaN($Value)) -and
        (-not [Single]::IsInfinity($Value))
}

function Resolve-VisibilityOutput {
    param([string]$Path)

    $visibilityDir = [System.IO.Path]::GetFullPath(
        (Join-Path $root 'artifacts\visibility'))
    $resolved = if ([string]::IsNullOrWhiteSpace($Path)) {
        $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff'
        Join-Path $visibilityDir (
            $stamp + '_damagefall-recovery-p' + $PID + '.png')
    } elseif ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $root $Path))
    }
    $visibilityPrefix = $visibilityDir.TrimEnd('\', '/') +
        [System.IO.Path]::DirectorySeparatorChar
    Assert-Condition ($resolved.StartsWith(
            $visibilityPrefix,
            [System.StringComparison]::OrdinalIgnoreCase)) `
        "DamageFall captures must stay under '$visibilityDir': $resolved"
    Assert-Condition ([System.IO.Path]::GetExtension($resolved) -ieq '.png') `
        "DamageFall evidence capture must be a PNG: $resolved"
    return $resolved
}

function Get-ElfSymbolAddress {
    param([string]$Name)
    $escapedName = [regex]::Escape($Name)
    $line = @(& $nm -a $elf) |
        Where-Object { $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escapedName$" } |
        Select-Object -First 1
    if (-not $line) { throw "ELF symbol not found: $Name" }
    $match = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($match.Groups[1].Value, 16))
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$nm = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-nm.exe'

if (-not $NoBuild) {
    & make -C $root `
        "TARGET=$target" `
        "BUILD=$build" `
        NDS_DEV_SCENE_HARNESS=battle_playable_realtime `
        NDS_DEV_LIVE_INPUT_PREVIEW=1 `
        NDS_HARNESS_FAST_LOGIC=0 `
        NDS_RENDERER_HW_TRIANGLES=1 `
        NDS_RENDERER_PROFILE_LEVEL=0 `
        NDS_DEBUG_HUD=0 `
        -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if ((-not (Test-Path -LiteralPath $rom -PathType Leaf)) -or
    (-not (Test-Path -LiteralPath $elf -PathType Leaf))) {
    $artifactError = if ($customArtifacts) {
        'Custom recovery verification ROM or ELF artifact is missing.'
    } else {
        'Canonical battle_playable build did not produce the expected ROM and ELF.'
    }
    throw "$artifactError ROM='$rom' ELF='$elf'"
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
if (-not (Test-Path $nm)) { throw "ELF symbol tool not found: $nm" }
if (-not (Test-Path $captureScript)) {
    throw "melonDS capture helper not found: $captureScript"
}

# Resolve the private input storage and require every trace point from this
# exact canonical ELF. This keeps the verifier from silently sampling a stale
# or differently configured binary.
$requiredSymbols = @(
    'sControllerPlaybackPads',
    'sControllerPlaybackConnectedMask',
    'sControllerPlaybackEnabled',
    'ndsBattlePlayableFrameCompleteMarker',
    'ftCommonWalkSetStatusParam',
    'ndsBaseFTCommonDashSetStatus',
    'ndsBaseFTCommonRunSetStatus',
    'ftCommonWaitSetStatus',
    'ndsBaseFTCommonPassCheckInterruptCommon',
    'ndsBaseFTCommonAttackHi4CheckInterruptMain',
    'ndsBaseFTCommonThrownReleaseThrownUpdateStats',
    'ndsBaseFTCommonDamageInitDamageVars',
    'ftCommonDamageFallSetStatusFromDamage',
    'mpCommonCheckFighterCliff',
    'ftCommonDownBounceSetStatus',
    'gGCCommonLinks',
    'gSCManagerSceneData',
    'gSCManagerBattleState',
    'sIFCommonTimerIsStarted',
    'ifCommonAnnounceTimeUpInitInterface',
    'gNdsCollisionRuntimeDiagnostics',
    'gNdsSceneHarnessMode',
    'gNdsRendererProfileLevel',
    'gNdsControllerPlaybackFrameCount',
    'gNdsControllerPlaybackReadCount'
)
$symbolAddresses = @{}
foreach ($symbol in $requiredSymbols) {
    $symbolAddresses[$symbol] = Get-ElfSymbolAddress $symbol
}
$playbackPadsAddress = $symbolAddresses['sControllerPlaybackPads']
$playbackConnectedAddress = $symbolAddresses['sControllerPlaybackConnectedMask']
$playbackEnabledAddress = $symbolAddresses['sControllerPlaybackEnabled']
$frameMarkerAddress = $symbolAddresses['ndsBattlePlayableFrameCompleteMarker']
$walkSetAddress = $symbolAddresses['ftCommonWalkSetStatusParam']
$dashSetAddress = $symbolAddresses['ndsBaseFTCommonDashSetStatus']
$runSetAddress = $symbolAddresses['ndsBaseFTCommonRunSetStatus']
$waitSetAddress = $symbolAddresses['ftCommonWaitSetStatus']
$passCheckAddress =
    $symbolAddresses['ndsBaseFTCommonPassCheckInterruptCommon']
$attackHi4InterruptAddress =
    $symbolAddresses['ndsBaseFTCommonAttackHi4CheckInterruptMain']
$throwReleaseAddress = $symbolAddresses['ndsBaseFTCommonThrownReleaseThrownUpdateStats']
$damageInitAddress = $symbolAddresses['ndsBaseFTCommonDamageInitDamageVars']
$damageFallSetAddress = $symbolAddresses['ftCommonDamageFallSetStatusFromDamage']
$checkCliffAddress = $symbolAddresses['mpCommonCheckFighterCliff']
$downBounceAddress = $symbolAddresses['ftCommonDownBounceSetStatus']
$timeUpAddress = $symbolAddresses['ifCommonAnnounceTimeUpInitInterface']

$screenshotPath = Resolve-VisibilityOutput $Screenshot
$failureStamp = Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff'
$failureLog = Join-Path $logDir (
    "damagefall-recovery-$failureStamp-p$PID.failure.log")
New-Item -ItemType Directory -Path $logDir,
    (Split-Path -Parent $screenshotPath) -Force | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath `
        -GdbPort $verifierContext.GdbPort `
        -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr, $screenshotPath -Force `
        -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList ('"{0}"' -f $rom) `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator `
        -Port $verifierContext.GdbPort | Out-Null

    $captureScriptGdb = $captureScript.Replace('\', '/')
    $screenshotGdb = $screenshotPath.Replace('\', '/')
    $captureCommand =
        'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" -EmulatorProcessId {1} -Output "{2}"' -f
        $captureScriptGdb, $emulator.Id, $screenshotGdb

    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        # This is the source GO transition: game_status is already GO and the
        # original timer is about to start. No playback write happens earlier.
        'tbreak ifcommon.c:3175',
        'continue',
        'set $mario = (GObj *)0',
        'set $fox = (GObj *)0',
        'set $scan = (GObj *)gGCCommonLinks[3]',
        'set $fighter_count = 0',
        'while (($scan != 0) && ($fighter_count < 8))',
        'set $scan_fp = (FTStruct *)$scan->user_data.p',
        'if (($scan_fp != 0) && ($scan_fp->player == 0))',
        'set $mario = $scan',
        'end',
        'if (($scan_fp != 0) && ($scan_fp->player == 1))',
        'set $fox = $scan',
        'end',
        'set $scan = (GObj *)$scan->link_next',
        'set $fighter_count = $fighter_count + 1',
        'end',
        'set $setup_ok = (($mario != 0) && ($fox != 0))',
        'printf "DAMAGEFALL_SETUP=%u,%u\n", $setup_ok, $fighter_count',
        'if ($setup_ok == 0)',
        'detach',
        'quit',
        'end',
        'set $mfp = (FTStruct *)$mario->user_data.p',
        'set $ffp = (FTStruct *)$fox->user_data.p',
        'set $mroot = (DObj *)$mario->obj',
        'set $froot = (DObj *)$fox->obj',
        'set $observed_frames = 0',
        'set $drive_frames = 0',
        'set $drive_phase = 0',
        'set $approach_count = 0',
        'set $approach_route = 0',
        'set $wait_stop_count = 0',
        'set $drop_requests = 0',
        'set $drop_accepts = 0',
        'set $drop_line = -1',
        'set $main_floor_waits = 0',
        'set $upsmash_attempts = 0',
        'set $upsmash_set_count = 0',
        'set $upsmash_active = 0',
        'set $upsmash_damage_count = 0',
        'set $separation_milli = 0',
        'set $throw_release_pending = 0',
        'set $throw_release_count = 0',
        'set $damage_event_count = 0',
        'set $damage_from_throw = 0',
        'set $heavy = 0',
        'set $heavy_status = -1',
        'set $heavy_percent = -1',
        'set $heavy_knockback_bits = 0',
        'set $heavy_hitstun = -1',
        'set $heavy_vx_bits = 0',
        'set $heavy_vy_bits = 0',
        'set $heavy_x_milli = 0',
        'set $heavy_y_milli = 0',
        'set $transition_calls = 0',
        'set $transition_status = -1',
        'set $transition_confirmed = 0',
        'set $cross = 0',
        'set $cross_prev_bottom_milli = 0',
        'set $cross_curr_bottom_milli = 0',
        'set $cross_x_milli = 0',
        'set $cross_pos_diff_y_milli = 0',
        'set $cross_sweep_calls_before = 0',
        'set $cross_sweep_hits_before = 0',
        'set $cross_adj_calls_before = 0',
        'set $cross_adj_hits_before = 0',
        'set $downbounce_calls = 0',
        'set $rejected_direct_count = 0',
        'set $landing_pending = 0',
        'set $completed_frame_checks = 0',
        'set $below_floor_completed = 0',
        'set $below_floor_bottom_milli = 0',
        'set $below_floor_x_milli = 0',
        'set $landing_ga = -1',
        'set $landing_floor_line = -1',
        'set $landing_mask_curr = 0',
        'set $landing_mask_stat = 0',
        'set $landing_is_coll_end = 0',
        'set $landing_bottom_milli = 0',
        'set $landing_sweep_calls_after = 0',
        'set $landing_sweep_hits_after = 0',
        'set $landing_adj_calls_after = 0',
        'set $landing_adj_hits_after = 0',
        'set $route_kind = 0',
        'set $direct_check_delta = 0',
        'set $direct_test_delta = 0',
        'set $direct_hit_delta = 0',
        'set $direct_landing_delta = 0',
        'set $direct_result_delta = 0',
        'set $direct_invalid_delta = 0',
        'set $direct_last_line = -1',
        'set $direct_last_status = -1',
        'set $direct_root_before = 0',
        'set $direct_root_after = 0',
        'set $direct_pos_diff_y = 0',
        'set $direct_mask_curr = 0',
        'set $direct_mask_stat = 0',
        'set $outcome = 0',
        'set $heavy_candidate = 0',
        'set $heavy_candidate_bits = 0',
        'set $phase_heavy_hits = 0',
        'set $phase_damagefalls = 0',
        'set $phase_floor_recoveries = 0',
        'set $terminator_scene = 0',
        'set $terminator_status = 0',
        'set $terminator_limit = 0',
        'set $terminator_remain = 0',
        'set $terminator_passed = 0',
        'set $input_reads_base = gNdsControllerPlaybackReadCount',
        'set $input_frames_base = gNdsControllerPlaybackFrameCount',
        'set $damage_check_base = gNdsCollisionRuntimeDiagnostics.damage_check_calls',
        'set $damage_test_base = gNdsCollisionRuntimeDiagnostics.damage_floor_tests',
        'set $damage_hit_base = gNdsCollisionRuntimeDiagnostics.damage_floor_hits',
        'set $damage_landing_base = gNdsCollisionRuntimeDiagnostics.damage_floor_landings',
        'set $damage_result_base = gNdsCollisionRuntimeDiagnostics.damage_results',
        'set $damage_invalid_base = gNdsCollisionRuntimeDiagnostics.damage_invalid',
        # External player-0 input only. Hold neutral for one completed GO frame
        # before beginning the natural route. If Mario starts on a pass-through
        # side platform, tap Down and let BattleShip's Wait/Squat/Pass path drop
        # him to the main floor. Otherwise, use short source Dash/Walk/Run
        # entries which return naturally to Wait before reevaluating distance.
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $playbackConnectedAddress),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $playbackEnabledAddress),

        # Enabled only while a Down tap is pending. This observes the original
        # pass-input predicate after the live input has been latched, then
        # releases the external pad while the source function proceeds into
        # Squat/Pass. No fighter, status, position, or collision state is set.
        ('break *0x{0:x8}' -f $passCheckAddress),
        'set $pass_breakpoint = $bpnum',
        'commands $pass_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($drive_phase == 5) && ($mfp->input.pl.stick_range.y <= -53) && ($mfp->tap_stick_y < 4) && (($mfp->coll_data.floor_flags & 0x4000) != 0))',
        'set $drop_accepts = $drop_accepts + 1',
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'set $drive_phase = 2',
        'disable $pass_breakpoint',
        'enable $wait_breakpoint',
        'end',
        'continue',
        'end',
        'disable $pass_breakpoint',

        ('break *0x{0:x8}' -f $frameMarkerAddress),
        'set $kick_breakpoint = $bpnum',
        'commands $kick_breakpoint',
        'silent',
        'if ($drive_phase == 0)',
        'if (($mfp->ga == 0) && ($mfp->coll_data.floor_line_id != 3) && (($mfp->coll_data.floor_flags & 0x4000) != 0))',
        'set $drop_requests = $drop_requests + 1',
        'set $drop_line = $mfp->coll_data.floor_line_id',
        'set $drive_phase = 5',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 3)),
        'enable $pass_breakpoint',
        'else',
        'if ($froot->translate.vec.f.x >= $mroot->translate.vec.f.x)',
        ('set {{signed char}}0x{0:x8} = 80' -f ($playbackPadsAddress + 2)),
        'else',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 2)),
        'end',
        'set $drive_phase = 1',
        'enable $walk_breakpoint',
        'enable $dash_breakpoint',
        'enable $run_breakpoint',
        'end',
        'disable $kick_breakpoint',
        'end',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $walkSetAddress),
        'set $walk_breakpoint = $bpnum',
        'commands $walk_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($heavy_candidate == 0) && ($drive_phase == 1) && ($upsmash_attempts < 48))',
        'set $approach_count = $approach_count + 1',
        'set $approach_route = 1',
        'set $drive_phase = 2',
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        'disable $walk_breakpoint',
        'disable $dash_breakpoint',
        'disable $run_breakpoint',
        'enable $wait_breakpoint',
        'end',
        'continue',
        'end',
        'disable $walk_breakpoint',

        # Status-entry observations are deliberately sparse. Only the current
        # driver phase's breakpoint group stays enabled, so unrelated Fox
        # transitions do not churn every hardware breakpoint. Neutralizing the
        # external X stick lets the source status run a short movement step and
        # return naturally to Wait without a per-frame interrupt breakpoint.
        ('break *0x{0:x8}' -f $dashSetAddress),
        'set $dash_breakpoint = $bpnum',
        'commands $dash_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($heavy_candidate == 0) && ($drive_phase == 1) && ($upsmash_attempts < 48))',
        'set $approach_count = $approach_count + 1',
        'set $approach_route = 3',
        'set $drive_phase = 2',
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        'disable $walk_breakpoint',
        'disable $dash_breakpoint',
        'disable $run_breakpoint',
        'enable $wait_breakpoint',
        'end',
        'continue',
        'end',
        'disable $dash_breakpoint',

        ('break *0x{0:x8}' -f $runSetAddress),
        'set $run_breakpoint = $bpnum',
        'commands $run_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($heavy_candidate == 0) && ($drive_phase == 1) && ($upsmash_attempts < 48))',
        'set $approach_count = $approach_count + 1',
        'set $approach_route = 2',
        'set $drive_phase = 2',
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        'disable $walk_breakpoint',
        'disable $dash_breakpoint',
        'disable $run_breakpoint',
        'enable $wait_breakpoint',
        'end',
        'continue',
        'end',
        'disable $run_breakpoint',

        ('break *0x{0:x8}' -f $waitSetAddress),
        'set $wait_breakpoint = $bpnum',
        'commands $wait_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($heavy_candidate == 0) && ($drive_phase != 0) && ($upsmash_attempts < 48))',
        'set $separation = $froot->translate.vec.f.x - $mroot->translate.vec.f.x',
        'set $separation_milli = (int)($separation * 1000.0)',
        # Do not disturb a pending Down tap before the source pass predicate
        # consumes it. Every other Wait starts from a neutral external pad.
        'if ($drive_phase != 5)',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'disable $wait_breakpoint',
        'disable $damageinit_breakpoint',
        'disable $throwrelease_breakpoint',
        'set $upsmash_active = 0',
        'if (($mfp->ga == 0) && ($mfp->coll_data.floor_line_id != 3) && (($mfp->coll_data.floor_flags & 0x4000) != 0))',
        'set $drop_requests = $drop_requests + 1',
        'set $drop_line = $mfp->coll_data.floor_line_id',
        'set $drive_phase = 5',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 3)),
        'enable $pass_breakpoint',
        'else',
        'if (($mfp->ga == 0) && ($mfp->coll_data.floor_line_id == 3))',
        'set $main_floor_waits = $main_floor_waits + 1',
        'end',
        # BattleShip gates AttackHi4 on Mario's live Up+A input, not Fox's
        # kinetics or floor line. Let the original hitbox accept platform/air hits.
        'if (($mfp->ga == 0) && ($mfp->coll_data.floor_line_id == 3) && ($approach_count >= 1) && ($wait_stop_count == $upsmash_attempts) && ($mroot->translate.vec.f.x >= -1800.0) && ($mroot->translate.vec.f.x <= 1800.0) && ($froot->translate.vec.f.x >= -1800.0) && ($froot->translate.vec.f.x <= 1800.0) && ($separation >= -450.0) && ($separation <= 450.0))',
        # A fresh Up+A tap is consumed by the ordinary Wait interrupt on the
        # following frame; no status function is called from the verifier.
        'set $wait_stop_count = $wait_stop_count + 1',
        'set $drive_phase = 3',
        ('set {{unsigned short}}0x{0:x8} = 0x8000' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 80' -f ($playbackPadsAddress + 3)),
        'enable $attackhi4_breakpoint',
        'else',
        'set $drive_phase = 1',
        'set $approach_route = 0',
        'enable $walk_breakpoint',
        'enable $dash_breakpoint',
        'enable $run_breakpoint',
        'if ($mroot->translate.vec.f.x >= 1800.0)',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 2)),
        'else',
        'if ($mroot->translate.vec.f.x <= -1800.0)',
        ('set {{signed char}}0x{0:x8} = 80' -f ($playbackPadsAddress + 2)),
        'else',
        'if ($separation >= 0.0)',
        ('set {{signed char}}0x{0:x8} = 80' -f ($playbackPadsAddress + 2)),
        'else',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 2)),
        'end',
        'end',
        'end',
        'end',
        'end',
        'end',
        'end',
        'if (((GObj *)$r0 == $mario) && ($heavy_candidate == 0) && ($upsmash_attempts >= 48))',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'disable $kick_breakpoint',
        'disable $pass_breakpoint',
        'disable $walk_breakpoint',
        'disable $dash_breakpoint',
        'disable $run_breakpoint',
        'disable $wait_breakpoint',
        'disable $attackhi4_breakpoint',
        'disable $throwrelease_breakpoint',
        'disable $damageinit_breakpoint',
        'end',
        'continue',
        'end',
        'disable $wait_breakpoint',

        # AttackHi4SetStatus is likewise inlined. This source interrupt-main
        # entry is reached only after the natural Up+A input succeeds; r0 is
        # the live FTStruct rather than its fighter GObj.
        ('break *0x{0:x8}' -f $attackHi4InterruptAddress),
        'set $attackhi4_breakpoint = $bpnum',
        'commands $attackhi4_breakpoint',
        'silent',
        'if (((FTStruct *)$r0 == $mfp) && ($heavy_candidate == 0) && ($drive_phase == 3) && ($upsmash_attempts < 48))',
        'set $upsmash_attempts = $upsmash_attempts + 1',
        'set $upsmash_set_count = $upsmash_set_count + 1',
        'set $upsmash_active = 1',
        'set $drive_phase = 4',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'disable $attackhi4_breakpoint',
        'enable $wait_breakpoint',
        'enable $throwrelease_breakpoint',
        'enable $damageinit_breakpoint',
        'end',
        'continue',
        'end',
        'disable $attackhi4_breakpoint',

        # Latch source throw release before damage initialization. Pointer
        # links may already be clear by a later DamageAir update, so this event
        # ordering—not pointer nullness—keeps ordinary hits separate.
        ('break *0x{0:x8}' -f $throwReleaseAddress),
        'set $throwrelease_breakpoint = $bpnum',
        'commands $throwrelease_breakpoint',
        'silent',
        'if ((GObj *)$r0 == $fox)',
        'set $throw_release_pending = 1',
        'set $throw_release_count = $throw_release_count + 1',
        'end',
        'continue',
        'end',
        'disable $throwrelease_breakpoint',
        ('break *0x{0:x8}' -f $damageInitAddress),
        'set $damageinit_breakpoint = $bpnum',
        'commands $damageinit_breakpoint',
        'silent',
        'if ((GObj *)$r0 == $fox)',
        'set $damage_event_count = $damage_event_count + 1',
        'set $damage_from_throw = $throw_release_pending',
        'set $throw_release_pending = 0',
        'if (($damage_from_throw == 0) && ($upsmash_active != 0) && ($drive_phase == 4) && ($mfp->status_id == nFTCommonStatusAttackHi4))',
        'set $upsmash_damage_count = $upsmash_damage_count + 1',
        'end',
        # r3 is the soft-float bit pattern for knockback at this source entry.
        # Positive finite float bits are monotonically ordered. BattleShip
        # derives hitstun as knockback / 1.875, so 60.0 (0x42700000) is the
        # ordinary non-throw threshold for level-3 hitstun >= 32.
        'if (($heavy_candidate == 0) && ($damage_from_throw == 0) && ($upsmash_active != 0) && ($drive_phase == 4) && ($mfp->status_id == nFTCommonStatusAttackHi4) && (((unsigned int)$r3) >= 0x42700000) && (((unsigned int)$r3) < 0x7f800000))',
        'set $heavy_candidate = 1',
        'set $heavy_candidate_bits = (unsigned int)$r3',
        'set $phase_heavy_hits = $phase_heavy_hits + 1',
        'printf "DAMAGEFALL_PHASE=HEAVY_HIT,%u,%#x\n", $phase_heavy_hits, $heavy_candidate_bits',
        # Candidate-local accepted evidence is reset here. A prior direct
        # DamageFly landing may be retained only in rejected_direct_count and
        # the direct diagnostic fields below.
        'set $heavy = 0',
        'set $heavy_status = -1',
        'set $heavy_percent = -1',
        'set $heavy_hitstun = -1',
        'set $transition_calls = 0',
        'set $transition_status = -1',
        'set $transition_confirmed = 0',
        'set $cross = 0',
        'set $cross_prev_bottom_milli = 0',
        'set $cross_curr_bottom_milli = 0',
        'set $cross_x_milli = 0',
        'set $cross_pos_diff_y_milli = 0',
        'set $downbounce_calls = 0',
        'set $landing_pending = 0',
        'set $completed_frame_checks = 0',
        'set $below_floor_completed = 0',
        'set $below_floor_bottom_milli = 0',
        'set $below_floor_x_milli = 0',
        'set $route_kind = 0',
        'set $outcome = 0',
        'set $damage_check_base = gNdsCollisionRuntimeDiagnostics.damage_check_calls',
        'set $damage_test_base = gNdsCollisionRuntimeDiagnostics.damage_floor_tests',
        'set $damage_hit_base = gNdsCollisionRuntimeDiagnostics.damage_floor_hits',
        'set $damage_landing_base = gNdsCollisionRuntimeDiagnostics.damage_floor_landings',
        'set $damage_result_base = gNdsCollisionRuntimeDiagnostics.damage_results',
        'set $damage_invalid_base = gNdsCollisionRuntimeDiagnostics.damage_invalid',
        'enable $damagefall_breakpoint',
        'enable $downbounce_breakpoint',
        'enable $frame_breakpoint',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'disable $kick_breakpoint',
        'disable $pass_breakpoint',
        'disable $walk_breakpoint',
        'disable $dash_breakpoint',
        'disable $run_breakpoint',
        'disable $wait_breakpoint',
        'disable $attackhi4_breakpoint',
        'disable $throwrelease_breakpoint',
        'disable $damageinit_breakpoint',
        'end',
        'end',
        'continue',
        'end',
        'disable $damageinit_breakpoint',
        ('break *0x{0:x8}' -f $damageFallSetAddress),
        'set $damagefall_breakpoint = $bpnum',
        'commands $damagefall_breakpoint',
        'silent',
        'if (($heavy_candidate != 0) && ((GObj *)$r0 == $fox) && ($damage_from_throw == 0) && ($ffp->status_id >= 51) && ($ffp->status_id <= 55))',
        'disable $damagefall_breakpoint',
        'set $phase_damagefalls = $phase_damagefalls + 1',
        'printf "DAMAGEFALL_PHASE=DAMAGEFALL,%u,%d\n", $phase_damagefalls, $ffp->status_id',
        'if ($heavy == 0)',
        'set $heavy = 1',
        'set $heavy_status = $ffp->status_id',
        'set $heavy_percent = $ffp->percent_damage',
        'set $heavy_knockback_bits = $heavy_candidate_bits',
        'set $heavy_hitstun = $ffp->status_vars.common.damage.hitstun_tics',
        'set $heavy_vx_bits = *(unsigned int *)&$ffp->physics.vel_damage_air.x',
        'set $heavy_vy_bits = *(unsigned int *)&$ffp->physics.vel_damage_air.y',
        'set $heavy_x_milli = (int)($froot->translate.vec.f.x * 1000.0)',
        'set $heavy_y_milli = (int)($froot->translate.vec.f.y * 1000.0)',
        'set $drive_frames = (gNdsControllerPlaybackReadCount - $input_reads_base) / 2',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'end',
        'set $transition_calls = $transition_calls + 1',
        'set $transition_status = $ffp->status_id',
        'enable $cliff_breakpoint',
        'end',
        'continue',
        'end',
        'disable $damagefall_breakpoint',
        ('break *0x{0:x8} if (($heavy != 0) && ((GObj *)$r0 == $fox) && ($ffp->status_id == 57))' -f $checkCliffAddress),
        'set $cliff_breakpoint = $bpnum',
        'commands $cliff_breakpoint',
        'silent',
        'set $transition_confirmed = 1',
        'set $map_bottom = $ffp->coll_data.map_coll.bottom',
        'if ($ffp->coll_data.p_map_coll != 0)',
        'set $map_bottom = $ffp->coll_data.p_map_coll->bottom',
        'end',
        'if (($cross == 0) && ($ffp->coll_data.p_translate != 0))',
        'set $prev_bottom = $ffp->coll_data.pos_prev.y + $map_bottom',
        'set $curr_bottom = $ffp->coll_data.p_translate->y + $ffp->coll_data.map_coll.bottom',
        'set $curr_x = $ffp->coll_data.p_translate->x',
        'if (($ffp->coll_data.pos_diff.y < 0.0) && ($prev_bottom >= 0.0) && ($curr_bottom < 0.0) && ($curr_x >= -2318.0) && ($curr_x <= 2318.0))',
        'set $cross = 1',
        'set $cross_prev_bottom_milli = (int)($prev_bottom * 1000.0)',
        'set $cross_curr_bottom_milli = (int)($curr_bottom * 1000.0)',
        'set $cross_x_milli = (int)($curr_x * 1000.0)',
        'set $cross_pos_diff_y_milli = (int)($ffp->coll_data.pos_diff.y * 1000.0)',
        'set $cross_sweep_calls_before = gNdsCollisionRuntimeDiagnostics.floor_sweep_calls',
        'set $cross_sweep_hits_before = gNdsCollisionRuntimeDiagnostics.floor_sweep_hits',
        'set $cross_adj_calls_before = gNdsCollisionRuntimeDiagnostics.floor_adj_calls',
        'set $cross_adj_hits_before = gNdsCollisionRuntimeDiagnostics.floor_adj_direct_hits + gNdsCollisionRuntimeDiagnostics.floor_adjacent_hits',
        # The persistent candidate-only frame breakpoint verifies that this
        # pre-resolution crossing is clamped in the same frame.
        'disable $cliff_breakpoint',
        'end',
        'end',
        'continue',
        'end',
        'disable $cliff_breakpoint',
        ('break *0x{0:x8}' -f $downBounceAddress),
        'set $downbounce_breakpoint = $bpnum',
        'commands $downbounce_breakpoint',
        'silent',
        'if (($heavy_candidate != 0) && ((GObj *)$r0 == $fox) && ($damage_from_throw == 0))',
        'if ($landing_pending != 0)',
        'set $downbounce_calls = $downbounce_calls + 1',
        'else',
        'if (($ffp->status_id >= 51) && ($ffp->status_id <= 55))',
        # Retain the old direct DamageFly landing only as failure diagnostics;
        # it can never satisfy this DamageFall recovery verifier. A first
        # low-percent up-smash is expected to take this route, so record it and
        # rearm natural input for a later higher-knockback DamageFall sample.
        'set $rejected_direct_count = $rejected_direct_count + 1',
        'set $direct_check_delta = gNdsCollisionRuntimeDiagnostics.damage_check_calls - $damage_check_base',
        'set $direct_test_delta = gNdsCollisionRuntimeDiagnostics.damage_floor_tests - $damage_test_base',
        'set $direct_hit_delta = gNdsCollisionRuntimeDiagnostics.damage_floor_hits - $damage_hit_base',
        'set $direct_landing_delta = gNdsCollisionRuntimeDiagnostics.damage_floor_landings - $damage_landing_base',
        'set $direct_result_delta = gNdsCollisionRuntimeDiagnostics.damage_results - $damage_result_base',
        'set $direct_invalid_delta = gNdsCollisionRuntimeDiagnostics.damage_invalid - $damage_invalid_base',
        'set $direct_last_line = gNdsCollisionRuntimeDiagnostics.damage_last_line',
        'set $direct_last_status = gNdsCollisionRuntimeDiagnostics.damage_last_status',
        'set $direct_root_before = gNdsCollisionRuntimeDiagnostics.damage_last_root_y_before_milli',
        'set $direct_root_after = gNdsCollisionRuntimeDiagnostics.damage_last_root_y_after_milli',
        'set $direct_pos_diff_y = gNdsCollisionRuntimeDiagnostics.damage_last_pos_diff_y_milli',
        'set $direct_mask_curr = gNdsCollisionRuntimeDiagnostics.damage_last_mask_curr',
        'set $direct_mask_stat = gNdsCollisionRuntimeDiagnostics.damage_last_mask_stat',
        'if ($outcome == 0)',
        'set $heavy_candidate = 0',
        'set $heavy_candidate_bits = 0',
        'set $upsmash_active = 0',
        'set $damage_from_throw = 0',
        'set $route_kind = 0',
        'set $drive_phase = 0',
        'set $approach_route = 0',
        'disable $damagefall_breakpoint',
        'disable $cliff_breakpoint',
        'disable $downbounce_breakpoint',
        'disable $frame_breakpoint',
        'enable $kick_breakpoint',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'end',
        'else',
        'if (($ffp->status_id == 57) && ($cross != 0))',
        'set $route_kind = 2',
        'set $downbounce_calls = $downbounce_calls + 1',
        'set $phase_floor_recoveries = $phase_floor_recoveries + 1',
        'printf "DAMAGEFALL_PHASE=FLOOR_RECOVERY,%u,%d\n", $phase_floor_recoveries, $ffp->coll_data.floor_line_id',
        'set $landing_pending = 1',
        'set $landing_ga = $ffp->ga',
        'set $landing_floor_line = $ffp->coll_data.floor_line_id',
        'set $landing_mask_curr = $ffp->coll_data.mask_curr',
        'set $landing_mask_stat = $ffp->coll_data.mask_stat',
        'set $landing_is_coll_end = $ffp->coll_data.is_coll_end',
        'set $landing_bottom_milli = (int)(($ffp->coll_data.p_translate->y + $ffp->coll_data.map_coll.bottom) * 1000.0)',
        'set $landing_sweep_calls_after = gNdsCollisionRuntimeDiagnostics.floor_sweep_calls',
        'set $landing_sweep_hits_after = gNdsCollisionRuntimeDiagnostics.floor_sweep_hits',
        'set $landing_adj_calls_after = gNdsCollisionRuntimeDiagnostics.floor_adj_calls',
        'set $landing_adj_hits_after = gNdsCollisionRuntimeDiagnostics.floor_adj_direct_hits + gNdsCollisionRuntimeDiagnostics.floor_adjacent_hits',
        'end',
        'end',
        'end',
        'end',
        'continue',
        'end',
        'disable $downbounce_breakpoint',

        # This breakpoint is disabled until the genuine >=60 up-smash hit.
        # It observes completed frames only; it never drives input. A raw
        # below-floor crossing is allowed inside source map processing, but no
        # completed in-bounds frame may retain Fox below the main floor.
        ('break *0x{0:x8}' -f $frameMarkerAddress),
        'set $frame_breakpoint = $bpnum',
        'commands $frame_breakpoint',
        'silent',
        'if ($heavy_candidate != 0)',
        'set $completed_frame_checks = $completed_frame_checks + 1',
        'if ($ffp->coll_data.p_translate != 0)',
        'set $completed_bottom = $ffp->coll_data.p_translate->y + $ffp->coll_data.map_coll.bottom',
        'set $completed_x = $ffp->coll_data.p_translate->x',
        'if (($outcome == 0) && ($completed_bottom < 0.0) && ($completed_x >= -2318.0) && ($completed_x <= 2318.0))',
        'set $below_floor_completed = $below_floor_completed + 1',
        'set $below_floor_bottom_milli = (int)($completed_bottom * 1000.0)',
        'set $below_floor_x_milli = (int)($completed_x * 1000.0)',
        'set $outcome = 2',
        'end',
        'end',
        'if (($cross != 0) && ($outcome == 0))',
        'if (($route_kind == 2) && ($landing_pending != 0) && ($downbounce_calls == 1))',
        'set $outcome = 1',
        'else',
        'if ($downbounce_calls > 1)',
        'set $outcome = 6',
        'else',
        'set $outcome = 2',
        'end',
        'end',
        'end',
        'if ($outcome != 0)',
        'disable $frame_breakpoint',
        'disable $damagefall_breakpoint',
        'disable $cliff_breakpoint',
        'disable $downbounce_breakpoint',
        'disable $timeup_breakpoint',
        'end',
        'end',
        'if ($outcome == 0)',
        'continue',
        'end',
        'end',
        'disable $frame_breakpoint',

        # Time Up is the explicit no-sample terminator. It avoids a permanent
        # frame breakpoint across the one-minute match.
        ('break *0x{0:x8}' -f $timeUpAddress),
        'set $timeup_breakpoint = $bpnum',
        'commands $timeup_breakpoint',
        'silent',
        'set $terminator_scene = gSCManagerSceneData.scene_curr',
        'set $terminator_status = gSCManagerBattleState->game_status',
        'set $terminator_limit = gSCManagerBattleState->time_limit',
        'set $terminator_remain = gSCManagerBattleState->time_remain',
        'set $terminator_passed = gSCManagerBattleState->time_passed',
        'if (($terminator_scene == 22) && ($terminator_limit == 1) && ($terminator_remain == 0) && ($terminator_passed == 3600))',
        'set $outcome = 3',
        'else',
        'set $outcome = 4',
        'end',
        'end',
        'continue',
        'set $observed_frames = gNdsControllerPlaybackReadCount - $input_reads_base',
        'if ($drive_frames == 0)',
        'set $drive_frames = $observed_frames / 2',
        'end',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 3)),
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u,%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_limit, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed',
        'printf "RENDER_PROFILE_LEVEL=%u\n", gNdsRendererProfileLevel',
        'printf "DAMAGEFALL_ACTORS=%u,%u,%u,%u,%u,%u,%u\n", $mfp->player, $mfp->fkind, $mfp->pkind, $ffp->player, $ffp->fkind, $ffp->pkind, $ffp->level',
        ('printf "DAMAGEFALL_INPUT=%u,%#x,%u,%u,%u,%u,%#x,%d,%d\n", *(unsigned int *)0x{0:x8}, *(unsigned int *)0x{1:x8}, gNdsControllerPlaybackReadCount - $input_reads_base, gNdsControllerPlaybackFrameCount - $input_frames_base, $observed_frames, $drive_frames, *(unsigned short *)0x{2:x8}, *(signed char *)0x{3:x8}, *(signed char *)0x{4:x8}' -f $playbackEnabledAddress, $playbackConnectedAddress, $playbackPadsAddress, ($playbackPadsAddress + 2), ($playbackPadsAddress + 3)),
        'printf "DAMAGEFALL_DRIVER=%u,%u,%u,%u,%u,%d\n", $approach_count, $wait_stop_count, $upsmash_attempts, $upsmash_set_count, $upsmash_damage_count, $separation_milli',
        'printf "DAMAGEFALL_DROP=%u,%u,%d,%u\n", $drop_requests, $drop_accepts, $drop_line, $main_floor_waits',
        'printf "DAMAGEFALL_PHASES=%u,%u,%u\n", $phase_heavy_hits, $phase_damagefalls, $phase_floor_recoveries',
        'printf "DAMAGEFALL_HEAVY=%u,%d,%d,%#x,%d,%#x,%#x,%d,%d\n", $heavy, $heavy_status, $heavy_percent, $heavy_knockback_bits, $heavy_hitstun, $heavy_vx_bits, $heavy_vy_bits, $heavy_x_milli, $heavy_y_milli',
        'printf "DAMAGEFALL_CLASS=%u,%u,%u,%u,%u\n", $route_kind, $throw_release_count, $damage_event_count, $damage_from_throw, $throw_release_pending',
        'printf "DAMAGEFLY_ROUTE=%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%#x,%#x\n", $direct_check_delta, $direct_test_delta, $direct_hit_delta, $direct_landing_delta, $direct_result_delta, $direct_invalid_delta, $direct_last_line, $direct_last_status, $direct_root_before, $direct_root_after, $direct_pos_diff_y, $direct_mask_curr, $direct_mask_stat',
        'printf "DAMAGEFALL_TRANSITION=%u,%d,%u\n", $transition_calls, $transition_status, $transition_confirmed',
        'printf "DAMAGEFALL_CROSS=%u,%d,%d,%d,%d\n", $cross, $cross_prev_bottom_milli, $cross_curr_bottom_milli, $cross_x_milli, $cross_pos_diff_y_milli',
        'printf "DAMAGEFALL_ROUTE=%u,%u,%u,%u,%u,%u,%u,%u,%d,%#x,%#x,%u,%d\n", $downbounce_calls, $cross_sweep_calls_before, $landing_sweep_calls_after, $cross_sweep_hits_before, $landing_sweep_hits_after, $cross_adj_calls_before, $landing_adj_calls_after, $landing_adj_hits_after - $cross_adj_hits_before, $landing_floor_line, $landing_mask_curr, $landing_mask_stat, $landing_is_coll_end, $landing_bottom_milli',
        'printf "DAMAGEFALL_FRAMES=%u,%u,%d,%d,%u,%u\n", $completed_frame_checks, $below_floor_completed, $below_floor_bottom_milli, $below_floor_x_milli, $rejected_direct_count, $landing_pending',
        'printf "DAMAGEFALL_TERMINATOR=%u,%u,%u,%u,%u\n", $terminator_scene, $terminator_status, $terminator_limit, $terminator_remain, $terminator_passed',
        'printf "DAMAGEFALL_POST=%u,%d,%d,%d,%d,%#x,%#x,%u,%d\n", $outcome, $ffp->status_id, $ffp->motion_id, $ffp->ga, $ffp->coll_data.floor_line_id, $ffp->coll_data.mask_curr, $ffp->coll_data.mask_stat, $ffp->coll_data.is_coll_end, (int)(($ffp->coll_data.p_translate->y + $ffp->coll_data.map_coll.bottom) * 1000.0)',
        'printf "MEMARENA=%#x,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom',
        $captureCommand,
        ('set {{unsigned int}}0x{0:x8} = 0' -f $playbackEnabledAddress),
        'detach',
        'quit'
    )

    $gdbStdout = (Invoke-GdbMarkerScript `
        -Gdb $Gdb `
        -Elf $elf `
        -Root $root `
        -Commands $gdbCommands `
        -ScriptName $scriptName `
        -TimeoutSeconds $TimeoutSeconds).Stdout

    $setup = [regex]::Match($gdbStdout, 'DAMAGEFALL_SETUP=([0-9]+),([0-9]+)')
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $profile = [regex]::Match($gdbStdout, 'RENDER_PROFILE_LEVEL=([0-9]+)')
    $actors = [regex]::Match($gdbStdout, 'DAMAGEFALL_ACTORS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'DAMAGEFALL_INPUT=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $driver = [regex]::Match($gdbStdout, 'DAMAGEFALL_DRIVER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $drop = [regex]::Match($gdbStdout, 'DAMAGEFALL_DROP=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $phases = [regex]::Match($gdbStdout, 'DAMAGEFALL_PHASES=([0-9]+),([0-9]+),([0-9]+)')
    $heavy = [regex]::Match($gdbStdout, 'DAMAGEFALL_HEAVY=([0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $class = [regex]::Match($gdbStdout, 'DAMAGEFALL_CLASS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $direct = [regex]::Match($gdbStdout, 'DAMAGEFLY_ROUTE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $transition = [regex]::Match($gdbStdout, 'DAMAGEFALL_TRANSITION=([0-9]+),(-?[0-9]+),([0-9]+)')
    $cross = [regex]::Match($gdbStdout, 'DAMAGEFALL_CROSS=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $route = [regex]::Match($gdbStdout, 'DAMAGEFALL_ROUTE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+)')
    $frames = [regex]::Match($gdbStdout, 'DAMAGEFALL_FRAMES=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $terminator = [regex]::Match($gdbStdout, 'DAMAGEFALL_TERMINATOR=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $post = [regex]::Match($gdbStdout, 'DAMAGEFALL_POST=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+)')
    $memory = [regex]::Match($gdbStdout, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')

    Assert-Condition (Test-Path -LiteralPath $screenshotPath -PathType Leaf) `
        "DamageFall run did not produce its evidence screenshot: $screenshotPath" `
        $gdbStdout
    Assert-Condition ((Get-Item -LiteralPath $screenshotPath).Length -gt 1024) `
        "DamageFall evidence screenshot is unexpectedly small: $screenshotPath" `
        $gdbStdout
    Add-Type -AssemblyName System.Drawing
    $screenshotBitmap = [System.Drawing.Bitmap]::FromFile($screenshotPath)
    try {
        Assert-Condition ($screenshotBitmap.Width -eq
                $script:MelonDSCanonicalWindowWidth -and
                $screenshotBitmap.Height -eq
                $script:MelonDSCanonicalWindowHeight) `
            ("DamageFall evidence screenshot did not use the canonical {0}x{1} window: {2}" -f
                $script:MelonDSCanonicalWindowWidth,
                $script:MelonDSCanonicalWindowHeight,
                $screenshotPath) `
            $gdbStdout
    } finally {
        $screenshotBitmap.Dispose()
    }
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $screenshotPath `
        -MinDifferentFraction 0.01 `
        -MinDominantGreenFraction 0.03 `
        -MinNonWhiteNonGreenFraction 0.25 `
        -WindowScaledCapture
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Assert-Condition ($setup.Success -and [int]$setup.Groups[1].Value -eq 1) 'Canonical Mario/Fox actors were not available after GO.' $gdbStdout
    $hv = Get-Ints $harn
    $sv = Get-Ints $scene
    $av = Get-Ints $actors
    $iv = Get-Ints $input
    $uv = Get-Ints $driver
    $ov = Get-Ints $drop
    $qv = Get-Ints $phases
    $dv = Get-Ints $heavy
    $kv = Get-Ints $class
    $fv = Get-Ints $direct
    $tv = Get-Ints $transition
    $cv = Get-Ints $cross
    $rv = Get-Ints $route
    $wv = Get-Ints $frames
    $xv = Get-Ints $terminator
    $pv = Get-Ints $post
    $mv = Get-Ints $memory

    Assert-Condition ($harn.Success -and $hv[0] -eq 0x4841524e -and $hv[1] -eq 163 -and $hv[2] -eq 22 -and $hv[3] -eq 21 -and $hv[4] -eq 0) 'DamageFall verifier did not use registered battle_playable_realtime mode 163.' $gdbStdout
    Assert-Condition ($profile.Success -and [int64]$profile.Groups[1].Value -eq 0) 'DamageFall verifier did not use the shipped renderer profile.' $gdbStdout
    Assert-Condition ($actors.Success -and $av[0] -eq 0 -and $av[1] -eq 0 -and $av[2] -eq 0 -and $av[3] -eq 1 -and $av[4] -eq 1 -and $av[5] -eq 1 -and $av[6] -eq 3) 'DamageFall verifier did not observe human Mario versus level-3 CPU Fox.' $gdbStdout
    Assert-Condition ($input.Success -and $iv[0] -eq 1 -and $iv[1] -eq 1 -and $iv[2] -gt 0 -and $iv[4] -gt 0 -and $iv[5] -gt 0 -and $iv[6] -eq 0 -and $iv[7] -eq 0 -and $iv[8] -eq 0) 'Post-GO semantic player-0 playback was not consumed or the final pad was not neutral.' $gdbStdout

    if ($post.Success -and $pv[0] -eq 3) {
        Assert-Condition ($terminator.Success -and $xv[0] -eq 22 -and
                $xv[2] -eq 1 -and $xv[3] -eq 0 -and $xv[4] -eq 3600) `
            'The no-sample terminator was not the exact source one-minute expiry.' `
            $gdbStdout
        throw "Mario did not naturally give Fox a non-throw >=60 up-smash DamageFly-to-DamageFall sample before the canonical one-minute match ended.`n$gdbStdout"
    }
    if ($post.Success -and $pv[0] -eq 2) {
        throw "Natural Fox DamageFall either completed a frame below the Pupupu main floor or crossed without a same-frame DownBounce clamp.`n$gdbStdout"
    }
    if ($post.Success -and $pv[0] -eq 4) {
        throw "DamageFall trace stopped at a non-canonical Time Up callback.`n$gdbStdout"
    }
    if ($post.Success -and $pv[0] -eq 6) {
        throw "The accepted Fox DamageFall crossing invoked DownBounce more than once.`n$gdbStdout"
    }

    $knockback = [double](Convert-UInt32BitsToSingle $dv[3])
    Assert-Condition ($driver.Success -and $uv[0] -ge 1 -and $uv[1] -ge 1 -and $uv[2] -ge 1 -and $uv[3] -eq $uv[2] -and $uv[1] -ge $uv[2] -and $uv[4] -ge 1 -and [Math]::Abs($uv[5]) -le 450000) 'Mario did not naturally approach through source Dash/Walk/Run steps, stop in Wait, and enter source up-smash close to Fox.' $gdbStdout
    Assert-Condition ($drop.Success -and $ov[3] -ge 1 -and
            (($ov[0] -eq 0 -and $ov[1] -eq 0 -and $ov[2] -eq -1) -or
             ($ov[0] -ge 1 -and $ov[1] -eq $ov[0] -and $ov[2] -ge 0 -and $ov[2] -ne 3))) `
        'Mario neither began on the main floor nor naturally passed through a side platform before attacking.' `
        $gdbStdout
    Assert-Condition ($phases.Success -and $qv[0] -ge 1 -and
            $qv[1] -ge 1 -and $qv[2] -eq 1 -and
            $qv[0] -ge $qv[1] -and $qv[1] -ge $qv[2]) `
        'The cumulative HEAVY_HIT, DAMAGEFALL, and FLOOR_RECOVERY discriminator did not complete in order.' `
        $gdbStdout
    Assert-Condition ($heavy.Success -and $dv[0] -eq 1 -and $dv[1] -ge 51 -and $dv[1] -le 55 -and $dv[2] -gt 0 -and (Test-FiniteSingle ([single]$knockback)) -and $knockback -ge 60.0) 'The trace did not originate from Mario naturally giving Fox a genuine non-throw >=60 DamageFly hit with up-smash.' $gdbStdout
    Assert-Condition ($class.Success -and $kv[0] -eq 2 -and $kv[2] -ge 1 -and $kv[3] -eq 0 -and $kv[4] -eq 0) 'The accepted Fox damage chain was missing, originated from a throw release, or used direct DamageFly landing.' $gdbStdout
    Assert-Condition ($transition.Success -and $tv[0] -eq 1 -and $tv[1] -ge 51 -and $tv[1] -le 55 -and $tv[2] -eq 1) 'Fox DamageFly did not naturally transition exactly once through source DamageFall status 57.' $gdbStdout
    Assert-Condition ($cross.Success -and $cv[0] -eq 1 -and $cv[1] -ge 0 -and $cv[2] -lt 0 -and $cv[3] -ge -2318000 -and $cv[3] -le 2318000 -and $cv[4] -lt 0) 'Fox DamageFall did not produce a descending main-floor crossing on Pupupu line 3.' $gdbStdout
    Assert-Condition ($route.Success -and $rv[0] -eq 1 -and $rv[8] -eq 3 -and (($rv[9] -band 0x800) -ne 0) -and (($rv[10] -band 0x800) -ne 0) -and $rv[11] -eq 1 -and [Math]::Abs($rv[12]) -le 2) 'Fox DamageFall did not clamp on line 3 and invoke exactly one DownBounce.' $gdbStdout
    Assert-Condition (($rv[2] - $rv[1]) -ge 1 -and
        ($rv[4] - $rv[3]) -ge 1) `
        'Source-shaped Fox DamageFall processing did not drive the shared floor sweep to a hit.' $gdbStdout
    Assert-Condition ($frames.Success -and $wv[0] -ge 1 -and $wv[1] -eq 0 -and $wv[5] -eq 1) 'Fox completed an in-bounds frame below the floor or missed the accepted DamageFall landing callback.' $gdbStdout
    Assert-Condition ($post.Success -and $pv[0] -eq 1 -and
            (($pv[1] -eq 67 -and $pv[2] -eq 58) -or
             ($pv[1] -eq 68 -and $pv[2] -eq 59)) -and
            $pv[3] -eq 0 -and $pv[4] -eq 3 -and
            (($pv[5] -band 0x800) -ne 0) -and
            (($pv[6] -band 0x800) -ne 0) -and $pv[7] -eq 1 -and
            [Math]::Abs($pv[8]) -le 2) `
        'Recovered Fox did not remain in source DownBounce on the same floor after the completed frame.' `
        $gdbStdout
    Assert-Condition ($scene.Success -and $sv[0] -eq 22 -and
            $sv[1] -eq 21 -and $sv[2] -eq 6 -and $sv[3] -eq 1 -and
            $sv[4] -eq 1 -and $sv[5] -eq 1 -and $sv[6] -gt 0 -and
            $sv[7] -gt 0 -and (($sv[6] + $sv[7]) -eq 3600)) `
        'The recovery did not occur naturally before expiry in the running original one-minute match.' `
        $gdbStdout
    Assert-Condition ($memory.Success -and $mv[0] -eq 0x4d4c4544 -and $mv[1] -eq 22 -and $mv[2] -ge 131072) 'DamageFall recovery violated the P1 arena reserve floor.' $gdbStdout

    Write-Output ("battle_playable damage recovery passed: route=DamageFall attacker=Mario move=up-smash attempts={0} victim=Fox kb={1:N2} status={2} floor={3} root_bottom={4} reserve={5} screenshot={6}" -f $uv[2], $knockback, $dv[1], $rv[8], $rv[12], $mv[2], $screenshotPath)
    $verificationPassed = $true
} catch {
    $failureRecord = $_
    throw
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            if (-not (Test-Path -LiteralPath $screenshotPath -PathType Leaf)) {
                try {
                    & $captureScript `
                        -EmulatorProcessId $emulator.Id `
                        -Output $screenshotPath | Write-Output
                } catch {
                    Write-Warning (
                        'DamageFall fallback evidence capture failed: ' +
                        $_.Exception.Message)
                }
            }
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    if ($verificationPassed) {
        Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
        Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    } else {
        $tempDir = if (-not [string]::IsNullOrWhiteSpace(
                $env:SMASH64DS_VERIFY_TEMP_DIR)) {
            $env:SMASH64DS_VERIFY_TEMP_DIR
        } else {
            Join-Path $root 'artifacts\verifier-temp\default'
        }
        $failureLines = @(
            'DamageFall recovery verifier failure evidence',
            "timestamp=$([DateTime]::Now.ToString('o'))",
            "screenshot=$screenshotPath",
            "gdb_temp_dir=$tempDir",
            "gdb_script=$scriptName",
            "melonds_stdout=$stdout",
            "melonds_stderr=$stderr"
        )
        if ($null -ne $failureRecord) {
            $failureLines += 'exception:'
            $failureLines += $failureRecord.Exception.ToString()
        } else {
            $failureLines += 'exception: verifier did not reach PASS.'
        }
        if (Test-Path -LiteralPath $stdout -PathType Leaf) {
            $failureLines += 'melonDS stdout:'
            $failureLines += Get-Content -LiteralPath $stdout
        }
        if (Test-Path -LiteralPath $stderr -PathType Leaf) {
            $failureLines += 'melonDS stderr:'
            $failureLines += Get-Content -LiteralPath $stderr
        }
        Set-Content -LiteralPath $failureLog -Value $failureLines
        Write-Warning "DamageFall failure evidence retained: $failureLog"
    }
}
