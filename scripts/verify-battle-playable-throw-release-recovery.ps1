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
$contextArgs = @{
    Root = $root
    MelonDS = $MelonDS
    RunnerSlot = $RunnerSlot
    GdbPort = $GdbPort
    GdbPortExplicit = $PSBoundParameters.ContainsKey('GdbPort')
    NoBuild = [bool]$NoBuild
}
$verifierContext = Initialize-MelonDSVerifierContext @contextArgs
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
    Join-Path $root ($target + '.nds')
}
$elf = if ($customArtifacts) {
    [System.IO.Path]::GetFullPath($requestedElf)
} else {
    Join-Path $root ($target + '.elf')
}
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-playable-throw-release-recovery.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-playable-throw-release-recovery.stderr.log'
$scriptName = '_battle_playable_throw_release_recovery.gdb'
$captureScript = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$configState = $null
$emulator = $null

function Assert-Condition {
    param(
        [bool]$Condition,
        [string]$Message,
        [string]$Context = ''
    )

    if ($Condition) { return }
    if ([string]::IsNullOrWhiteSpace($Context)) { throw $Message }
    throw ($Message + [Environment]::NewLine + $Context)
}

function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)

    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $value = $Match.Groups[$i].Value
        if ($value -like '0x*') {
            $values += [int64](Convert-MarkerUInt32 $value)
        } else {
            $values += [int64]$value
        }
    }
    return $values
}

function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)

    $escapedName = [regex]::Escape($Name)
    $line = @(& $nm -a $elf) |
        Where-Object { $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escapedName$" } |
        Select-Object -First 1
    if (-not $line) { throw "ELF symbol not found: $Name" }
    $match = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($match.Groups[1].Value, 16))
}

function Resolve-VisibilityOutput {
    param([string]$Path)

    $visibilityDir = [System.IO.Path]::GetFullPath(
        (Join-Path $root 'artifacts\visibility'))
    $resolved = if ([string]::IsNullOrWhiteSpace($Path)) {
        $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff'
        Join-Path $visibilityDir ($stamp + '_throw-release-recovery-p' +
            $PID + '.png')
    } elseif ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $root $Path))
    }
    $visibilityPrefix = $visibilityDir.TrimEnd('\', '/') +
        [System.IO.Path]::DirectorySeparatorChar
    Assert-Condition ($resolved.StartsWith(
            $visibilityPrefix,
            [System.StringComparison]::OrdinalIgnoreCase)) (
        "Throw-release captures must stay under '$visibilityDir': $resolved")
    Assert-Condition ([System.IO.Path]::GetExtension($resolved) -ieq '.png') (
        "Throw-release evidence capture must be a PNG: $resolved")
    return $resolved
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$nm = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-nm.exe'

if (-not $NoBuild) {
    $makeArgs = @(
        '-C', $root,
        ('TARGET=' + $target),
        ('BUILD=' + $build),
        'NDS_DEV_SCENE_HARNESS=battle_playable_realtime',
        'NDS_DEV_LIVE_INPUT_PREVIEW=1',
        'NDS_HARNESS_FAST_LOGIC=0',
        'NDS_RENDERER_HW_TRIANGLES=1',
        'NDS_RENDERER_PROFILE_LEVEL=0',
        'NDS_DEBUG_HUD=0',
        '-j16'
    )
    & make @makeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$artifactError = if ($customArtifacts) {
    'Custom recovery verification ROM or ELF artifact is missing.'
} else {
    'Canonical mode-163 build did not produce the expected ROM and ELF.'
}
Assert-Condition ((Test-Path -LiteralPath $rom -PathType Leaf) -and
    (Test-Path -LiteralPath $elf -PathType Leaf)) (
    "$artifactError ROM='$rom' ELF='$elf'")
Assert-Condition (Test-Path -LiteralPath $melonDsPath -PathType Leaf) (
    "melonDS executable not found: $melonDsPath")
Assert-Condition (Test-Path -LiteralPath $Gdb -PathType Leaf) (
    "GDB executable not found: $Gdb")
Assert-Condition (Test-Path -LiteralPath $nm -PathType Leaf) (
    "ELF symbol tool not found: $nm")
Assert-Condition (Test-Path -LiteralPath $captureScript -PathType Leaf) (
    "melonDS capture helper not found: $captureScript")

$requiredSymbols = @(
    'sControllerPlaybackPads',
    'sControllerPlaybackConnectedMask',
    'sControllerPlaybackEnabled',
    'ndsBattlePlayableFrameCompleteMarker',
    'ftCommonWalkSetStatusParam',
    'ndsBaseFTCommonDashSetStatus',
    'ndsBaseFTCommonRunSetStatus',
    'ndsBaseFTCommonCatchSetStatus',
    'ftCommonWaitSetStatus',
    'ndsBaseFTCommonCatchPullProcCatch',
    'ndsBaseFTCommonCaptureWaitSetStatus',
    'ndsBaseFTCommonCatchWaitSetStatus',
    'ndsBaseFTCommonThrowSetStatus',
    'ndsBaseFTCommonThrownReleaseThrownUpdateStats',
    'ndsBaseFTCommonThrownReleaseFighterLoseGrip',
    'mpCommonRunFighterCollisionDefault',
    'ndsBaseFTCommonDamageInitDamageVars',
    'ftCommonDownBounceSetStatus',
    'ifCommonAnnounceTimeUpInitInterface',
    'gGCCommonLinks',
    'gSCManagerSceneData',
    'gSCManagerBattleState',
    'sIFCommonTimerIsStarted',
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
$playbackConnectedAddress =
    $symbolAddresses['sControllerPlaybackConnectedMask']
$playbackEnabledAddress = $symbolAddresses['sControllerPlaybackEnabled']
$frameMarkerAddress =
    $symbolAddresses['ndsBattlePlayableFrameCompleteMarker']
$walkSetAddress = $symbolAddresses['ftCommonWalkSetStatusParam']
$dashSetAddress = $symbolAddresses['ndsBaseFTCommonDashSetStatus']
$runSetAddress = $symbolAddresses['ndsBaseFTCommonRunSetStatus']
$catchSetAddress = $symbolAddresses['ndsBaseFTCommonCatchSetStatus']
$waitSetAddress = $symbolAddresses['ftCommonWaitSetStatus']
$catchPullAddress = $symbolAddresses['ndsBaseFTCommonCatchPullProcCatch']
$captureWaitAddress =
    $symbolAddresses['ndsBaseFTCommonCaptureWaitSetStatus']
$catchWaitAddress = $symbolAddresses['ndsBaseFTCommonCatchWaitSetStatus']
$throwSetAddress = $symbolAddresses['ndsBaseFTCommonThrowSetStatus']
$releaseUpdateAddress =
    $symbolAddresses['ndsBaseFTCommonThrownReleaseThrownUpdateStats']
$loseGripAddress =
    $symbolAddresses['ndsBaseFTCommonThrownReleaseFighterLoseGrip']
$defaultCollisionAddress =
    $symbolAddresses['mpCommonRunFighterCollisionDefault']
$damageInitAddress =
    $symbolAddresses['ndsBaseFTCommonDamageInitDamageVars']
$downBounceAddress = $symbolAddresses['ftCommonDownBounceSetStatus']
$timeUpAddress = $symbolAddresses['ifCommonAnnounceTimeUpInitInterface']

$screenshotPath = Resolve-VisibilityOutput $Screenshot
New-Item -ItemType Directory -Force -Path $logDir,
    (Split-Path -Parent $screenshotPath) | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath -GdbPort $verifierContext.GdbPort -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    if (Test-Path -LiteralPath $screenshotPath) {
        Remove-Item -LiteralPath $screenshotPath -Force
    }
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList ('"{0}"' -f $rom) `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $verifierContext.GdbPort |
        Out-Null

    $captureScriptGdb = $captureScript.Replace('\', '/')
    $screenshotGdb = $screenshotPath.Replace('\', '/')
    $captureCommand =
        'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" -EmulatorProcessId {1} -Output "{2}"' -f
        $captureScriptGdb, $emulator.Id, $screenshotGdb

    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set breakpoint pending off',
        'set remotetimeout 10',
        ("target remote 127.0.0.1:{0}" -f
            (Get-MelonDSActiveGdbPort)),
        # The source timer starts here. Controller playback stays disabled
        # throughout Wait and the countdown and begins only at GO.
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
        'printf "THROW_SETUP=%u,%u\n", $setup_ok, $fighter_count',
        'if ($setup_ok == 0)',
        'detach',
        'quit',
        'end',
        'set $mfp = (FTStruct *)$mario->user_data.p',
        'set $ffp = (FTStruct *)$fox->user_data.p',
        'set $mroot = (DObj *)$mario->obj',
        'set $froot = (DObj *)$fox->obj',
        'set $outcome = 0',
        'set $drive_phase = 1',
        'set $approach_count = 0',
        'set $approach_route = 0',
        'set $separation_milli = 0',
        'set $catch_attempts = 0',
        'set $catch_pull_count = 0',
        'set $capture_wait_count = 0',
        'set $catch_wait_count = 0',
        'set $throw_set_count = 0',
        'set $release_update_count = 0',
        'set $lose_grip_count = 0',
        'set $default_count = 0',
        'set $damage_init_count = 0',
        'set $down_bounce_count = 0',
        'set $catch_status_before = -1',
        'set $capture_status_seen = -1',
        'set $catch_wait_status_before = -1',
        'set $throw_status_before = -1',
        'set $throw_is_forward = -1',
        'set $throw_wait_before = -1',
        'set $throw_button_tap = 0',
        'set $throw_button_mask_a = 0',
        'set $throw_capture_status = -1',
        'set $throw_status_release = -1',
        'set $thrown_status_release = -1',
        'set $release_script = -1',
        'set $release_proc_status = -1',
        'set $links_at_wait = 0',
        'set $links_at_throw = 0',
        'set $links_at_release = 0',
        'set $links_after = 0',
        'set $in_throw_release = 0',
        'set $damage_from_throw = 0',
        'set $default_args_ok = 0',
        'set $copy_base = 0',
        'set $run_base = 0',
        'set $reset_base = 0',
        'set $fighter_base = 0',
        'set $damage_checks_base = 0',
        'set $damage_results_base = 0',
        'set $damage_hits_base = 0',
        'set $sweep_base = 0',
        'set $sweep_hits_base = 0',
        'set $adj_base = 0',
        'set $adj_hits_base = 0',
        'set $copy_delta = 0',
        'set $run_delta = 0',
        'set $reset_delta = 0',
        'set $fighter_delta = 0',
        'set $pmap_self = 0',
        'set $update_tic_ok = 0',
        'set $post_default_mask = 0',
        'set $post_default_line = -1',
        'set $percent_before = -1',
        'set $percent_after = -1',
        'set $bounce_status_before = -1',
        'set $bounce_ga_before = -1',
        'set $bounce_line = -1',
        'set $bounce_mask_curr = 0',
        'set $bounce_mask_stat = 0',
        'set $bounce_coll_end = 0',
        'set $bounce_bottom_milli = 0',
        'set $input_reads_base = gNdsControllerPlaybackReadCount',
        'set $input_frames_base = gNdsControllerPlaybackFrameCount',

        # Each breakpoint below is a semantic source event. There is no
        # persistent completed-frame breakpoint driving the controller.
        ('break *0x{0:x8}' -f $walkSetAddress),
        'set $walk_breakpoint = $bpnum',
        'commands $walk_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($drive_phase == 1) && ($catch_pull_count == 0) && ($catch_attempts < 48))',
        'set $approach_count = $approach_count + 1',
        'set $approach_route = 1',
        'set $drive_phase = 2',
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 2)),
        'disable $walk_breakpoint',
        'disable $dash_breakpoint',
        'disable $run_breakpoint',
        'enable $wait_breakpoint',
        'end',
        'continue',
        'end',
        'disable $walk_breakpoint',

        # One short source movement entry is enough. Releasing X lets
        # BattleShip return Mario to Wait before distance is reevaluated.
        ('break *0x{0:x8}' -f $dashSetAddress),
        'set $dash_breakpoint = $bpnum',
        'commands $dash_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($drive_phase == 1) && ($catch_pull_count == 0) && ($catch_attempts < 48))',
        'set $approach_count = $approach_count + 1',
        'set $approach_route = 3',
        'set $drive_phase = 2',
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 2)),
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
        'if (((GObj *)$r0 == $mario) && ($drive_phase == 1) && ($catch_pull_count == 0) && ($catch_attempts < 48))',
        'set $approach_count = $approach_count + 1',
        'set $approach_route = 2',
        'set $drive_phase = 2',
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 2)),
        'disable $walk_breakpoint',
        'disable $dash_breakpoint',
        'disable $run_breakpoint',
        'enable $wait_breakpoint',
        'end',
        'continue',
        'end',
        'disable $run_breakpoint',

        ('break *0x{0:x8}' -f $catchSetAddress),
        'commands',
        'silent',
        'if ((GObj *)$r0 == $mario)',
        'set $catch_attempts = $catch_attempts + 1',
        'set $drive_phase = 2',
        ('set {{unsigned short}}0x{0:x8} = 0' -f
            $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 2)),
        'enable $wait_breakpoint',
        'end',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $waitSetAddress),
        'set $wait_breakpoint = $bpnum',
        'commands $wait_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($drive_phase == 2) && ($catch_pull_count == 0) && ($catch_attempts < 48))',
        'set $separation = $froot->translate.vec.f.x - $mroot->translate.vec.f.x',
        'set $separation_milli = (int)($separation * 1000.0)',
        ('set {{unsigned short}}0x{0:x8} = 0' -f
            $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 3)),
        'disable $wait_breakpoint',
        # Mario's source Catch hitbox has size 290 (202_MarioMainMotion.c:878).
        # The broad 450 approach window only times the external button tap;
        # BattleShip's live collision remains the sole authority on a catch.
        'if (($mfp->ga == 0) && ($mfp->coll_data.floor_line_id == 3) && ($ffp->ga == 0) && ($ffp->coll_data.floor_line_id == 3) && ($mroot->translate.vec.f.x >= -1800.0) && ($mroot->translate.vec.f.x <= 1800.0) && ($froot->translate.vec.f.x >= -1800.0) && ($froot->translate.vec.f.x <= 1800.0) && ($separation >= -450.0) && ($separation <= 450.0))',
        'set $drive_phase = 3',
        ('set {{unsigned short}}0x{0:x8} = 0xa000' -f
            $playbackPadsAddress),
        'else',
        'set $drive_phase = 1',
        'set $approach_route = 0',
        'enable $walk_breakpoint',
        'enable $dash_breakpoint',
        'enable $run_breakpoint',
        'if ($mroot->translate.vec.f.x >= 1800.0)',
        ('set {{signed char}}0x{0:x8} = -80' -f
            ($playbackPadsAddress + 2)),
        'else',
        'if ($mroot->translate.vec.f.x <= -1800.0)',
        ('set {{signed char}}0x{0:x8} = 80' -f
            ($playbackPadsAddress + 2)),
        'else',
        'if ($separation >= 0.0)',
        ('set {{signed char}}0x{0:x8} = 80' -f
            ($playbackPadsAddress + 2)),
        'else',
        ('set {{signed char}}0x{0:x8} = -80' -f
            ($playbackPadsAddress + 2)),
        'end',
        'end',
        'end',
        'end',
        'end',
        'continue',
        'end',
        'disable $wait_breakpoint',

        ('break *0x{0:x8}' -f $catchPullAddress),
        'commands',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($mfp->search_gobj == $fox))',
        'set $catch_pull_count = $catch_pull_count + 1',
        'set $catch_status_before = $mfp->status_id',
        'end',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $captureWaitAddress),
        'commands',
        'silent',
        'if (((GObj *)$r0 == $fox) && ($ffp->capture_gobj == $mario))',
        'set $capture_wait_count = $capture_wait_count + 1',
        'set $capture_status_seen = $ffp->status_id',
        'end',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $catchWaitAddress),
        'commands',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($mfp->catch_gobj == $fox))',
        'set $catch_wait_count = $catch_wait_count + 1',
        'set $catch_wait_status_before = $mfp->status_id',
        'set $links_at_wait = (($mfp->catch_gobj == $fox) && ($ffp->capture_gobj == $mario))',
        # A fresh A tap selects the source forward throw from CatchWait.
        ('set {{unsigned short}}0x{0:x8} = 0x8000' -f
            $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 2)),
        'end',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $throwSetAddress),
        'commands',
        'silent',
        'if (((GObj *)$r0 == $mario) && ($mfp->catch_gobj == $fox))',
        'set $throw_set_count = $throw_set_count + 1',
        'set $throw_status_before = $mfp->status_id',
        'set $throw_is_forward = $r1',
        'set $throw_wait_before = $mfp->status_vars.common.catchwait.throw_wait',
        'set $throw_button_tap = $mfp->input.pl.button_tap',
        'set $throw_button_mask_a = $mfp->input.button_mask_a',
        'set $throw_capture_status = $ffp->status_id',
        'set $links_at_throw = (($mfp->catch_gobj == $fox) && ($ffp->capture_gobj == $mario))',
        ('set {{unsigned short}}0x{0:x8} = 0' -f
            $playbackPadsAddress),
        'end',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $releaseUpdateAddress),
        'set $release_update_breakpoint = $bpnum',
        'commands $release_update_breakpoint',
        'silent',
        'if ((GObj *)$r0 == $fox)',
        'set $release_update_count = $release_update_count + 1',
        'if ($release_update_count == 1)',
        'set $in_throw_release = 1',
        'set $throw_status_release = $mfp->status_id',
        'set $thrown_status_release = $ffp->status_id',
        'set $release_script = $r2',
        'set $release_proc_status = $r3',
        'set $links_at_release = (($mfp->catch_gobj == $fox) && ($ffp->capture_gobj == $mario))',
        'set $percent_before = $ffp->percent_damage',
        'set $copy_base = gNdsCollisionRuntimeDiagnostics.default_copy_calls',
        'set $run_base = gNdsCollisionRuntimeDiagnostics.default_run_calls',
        'set $reset_base = gNdsCollisionRuntimeDiagnostics.default_reset_calls',
        'set $fighter_base = gNdsCollisionRuntimeDiagnostics.default_fighter_calls',
        'enable $lose_grip_breakpoint',
        'enable $default_collision_breakpoint',
        'enable $damage_init_breakpoint',
        'end',
        'end',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $loseGripAddress),
        'set $lose_grip_breakpoint = $bpnum',
        'commands $lose_grip_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $fox) && ($in_throw_release != 0))',
        'set $lose_grip_count = $lose_grip_count + 1',
        'end',
        'continue',
        'end',
        'disable $lose_grip_breakpoint',

        ('break *0x{0:x8}' -f $defaultCollisionAddress),
        'set $default_collision_breakpoint = $bpnum',
        'commands $default_collision_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $fox) && ($in_throw_release != 0))',
        'set $default_count = $default_count + 1',
        'set $default_args_ok = (((GObj *)$r0 == $fox) && ((unsigned int)$r1 == (unsigned int)&$mroot->translate.vec.f) && ((unsigned int)$r2 == (unsigned int)&$mfp->coll_data))',
        'end',
        'continue',
        'end',
        'disable $default_collision_breakpoint',

        ('break *0x{0:x8}' -f $damageInitAddress),
        'set $damage_init_breakpoint = $bpnum',
        'commands $damage_init_breakpoint',
        'silent',
        'if ((GObj *)$r0 == $fox)',
        'if ($in_throw_release != 0)',
        'set $damage_init_count = $damage_init_count + 1',
        'set $damage_from_throw = 1',
        'set $copy_delta = gNdsCollisionRuntimeDiagnostics.default_copy_calls - $copy_base',
        'set $run_delta = gNdsCollisionRuntimeDiagnostics.default_run_calls - $run_base',
        'set $reset_delta = gNdsCollisionRuntimeDiagnostics.default_reset_calls - $reset_base',
        'set $fighter_delta = gNdsCollisionRuntimeDiagnostics.default_fighter_calls - $fighter_base',
        'set $pmap_self = ($ffp->coll_data.p_map_coll == &$ffp->coll_data.map_coll)',
        'set $update_tic_ok = ($ffp->coll_data.update_tic == gMPCollisionUpdateTic)',
        'set $post_default_mask = $ffp->coll_data.mask_curr',
        'set $post_default_line = $ffp->coll_data.floor_line_id',
        # Start damage/floor accounting after release reconciliation so the
        # LoseGrip default-collision pass cannot satisfy the landing proof.
        'set $damage_checks_base = gNdsCollisionRuntimeDiagnostics.damage_check_calls',
        'set $damage_results_base = gNdsCollisionRuntimeDiagnostics.damage_results',
        'set $damage_hits_base = gNdsCollisionRuntimeDiagnostics.damage_floor_hits',
        'set $sweep_base = gNdsCollisionRuntimeDiagnostics.floor_sweep_calls',
        'set $sweep_hits_base = gNdsCollisionRuntimeDiagnostics.floor_sweep_hits',
        'set $adj_base = gNdsCollisionRuntimeDiagnostics.floor_adj_calls',
        'set $adj_hits_base = gNdsCollisionRuntimeDiagnostics.floor_adj_direct_hits + gNdsCollisionRuntimeDiagnostics.floor_adjacent_hits',
        'disable $lose_grip_breakpoint',
        'disable $default_collision_breakpoint',
        'disable $damage_init_breakpoint',
        'enable $down_bounce_breakpoint',
        'else',
        'set $damage_from_throw = 0',
        'end',
        'set $in_throw_release = 0',
        'end',
        'continue',
        'end',
        'disable $damage_init_breakpoint',

        ('break *0x{0:x8}' -f $downBounceAddress),
        'set $down_bounce_breakpoint = $bpnum',
        'commands $down_bounce_breakpoint',
        'silent',
        'if (((GObj *)$r0 == $fox) && ($damage_from_throw != 0) && ($release_update_count == 1))',
        'set $down_bounce_count = $down_bounce_count + 1',
        'set $percent_after = $ffp->percent_damage',
        'set $bounce_status_before = $ffp->status_id',
        'set $bounce_ga_before = $ffp->ga',
        'set $bounce_line = $ffp->coll_data.floor_line_id',
        'set $bounce_mask_curr = $ffp->coll_data.mask_curr',
        'set $bounce_mask_stat = $ffp->coll_data.mask_stat',
        'set $bounce_coll_end = $ffp->coll_data.is_coll_end',
        'set $bounce_bottom_milli = (int)(($ffp->coll_data.p_translate->y + $ffp->coll_data.map_coll.bottom) * 1000.0)',
        'set $links_after = (($mfp->catch_gobj == 0) && ($ffp->capture_gobj == 0))',
        'set $outcome = 1',
        'disable $down_bounce_breakpoint',
        # Stop at the first completed frame after source DownBounce has run.
        ('tbreak *0x{0:x8}' -f $frameMarkerAddress),
        'continue',
        'end',
        'continue',
        'end',
        'disable $down_bounce_breakpoint',

        ('break *0x{0:x8}' -f $timeUpAddress),
        'commands',
        'silent',
        'if ($outcome == 0)',
        'set $outcome = 2',
        'else',
        # DownBounce can occur during the expiry frame. Let its planted
        # completed-frame tbreak win instead of terminating here at Time Up.
        'if ($outcome == 1)',
        'continue',
        'end',
        'end',
        'end',

        # Initial natural approach. Short source Walk/Dash/Run entries return
        # naturally to Wait, where distance and both main-floor states are
        # reevaluated before a fresh Z+A grab tap.
        ('set {{unsigned short}}0x{0:x8} = 0' -f
            $playbackPadsAddress),
        'if ($froot->translate.vec.f.x >= $mroot->translate.vec.f.x)',
        ('set {{signed char}}0x{0:x8} = 80' -f
            ($playbackPadsAddress + 2)),
        'else',
        ('set {{signed char}}0x{0:x8} = -80' -f
            ($playbackPadsAddress + 2)),
        'end',
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 3)),
        ('set {{unsigned int}}0x{0:x8} = 1' -f
            $playbackConnectedAddress),
        ('set {{unsigned int}}0x{0:x8} = 1' -f
            $playbackEnabledAddress),
        'enable $walk_breakpoint',
        'enable $dash_breakpoint',
        'enable $run_breakpoint',
        'continue',

        ('set {{unsigned short}}0x{0:x8} = 0' -f
            $playbackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f
            ($playbackPadsAddress + 3)),
        'printf "THROW_RESULT=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", $outcome, $catch_attempts, $catch_pull_count, $capture_wait_count, $catch_wait_count, $throw_set_count, $release_update_count, $lose_grip_count, $default_count, $damage_init_count',
        'printf "THROW_APPROACH=%u,%u,%d\n", $approach_count, $approach_route, $separation_milli',
        'printf "THROW_ACTORS=%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", $mario, $fox, $mfp->fkind, $mfp->pkind, $mfp->player, $ffp->fkind, $ffp->pkind, $ffp->player, $ffp->level',
        'printf "THROW_LIFECYCLE=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u\n", $catch_status_before, $capture_status_seen, $catch_wait_status_before, $throw_status_before, $throw_status_release, $thrown_status_release, $release_script, $release_proc_status, $links_at_wait, $links_at_throw, $links_at_release',
        'printf "THROW_CAUSE=%d,%d,%#x,%#x,%d\n", $throw_is_forward, $throw_wait_before, $throw_button_tap, $throw_button_mask_a, $throw_capture_status',
        'printf "THROW_DEFAULT=%u,%u,%u,%u,%u,%u,%u,%#x,%d\n", $default_args_ok, $copy_delta, $run_delta, $reset_delta, $fighter_delta, $pmap_self, $update_tic_ok, $post_default_mask, $post_default_line',
        'printf "THROW_DAMAGE=%d,%d,%d,%d,%u,%u,%u,%u\n", $percent_before, $percent_after, $bounce_status_before, $bounce_ga_before, $damage_from_throw, gNdsCollisionRuntimeDiagnostics.damage_check_calls - $damage_checks_base, gNdsCollisionRuntimeDiagnostics.damage_results - $damage_results_base, gNdsCollisionRuntimeDiagnostics.damage_floor_hits - $damage_hits_base',
        'printf "THROW_FLOOR=%u,%d,%#x,%#x,%u,%d,%u,%u,%u,%u\n", $down_bounce_count, $bounce_line, $bounce_mask_curr, $bounce_mask_stat, $bounce_coll_end, $bounce_bottom_milli, gNdsCollisionRuntimeDiagnostics.floor_sweep_calls - $sweep_base, gNdsCollisionRuntimeDiagnostics.floor_sweep_hits - $sweep_hits_base, gNdsCollisionRuntimeDiagnostics.floor_adj_calls - $adj_base, (gNdsCollisionRuntimeDiagnostics.floor_adj_direct_hits + gNdsCollisionRuntimeDiagnostics.floor_adjacent_hits) - $adj_hits_base',
        'printf "THROW_LINKS=%u,%#x,%#x,%#x\n", $links_after, $mfp->catch_gobj, $ffp->capture_gobj, $ffp->throw_gobj',
        'printf "THROW_FINAL=%d,%d,%d,%#x,%#x,%u,%d\n", $ffp->status_id, $ffp->ga, $ffp->coll_data.floor_line_id, $ffp->coll_data.mask_curr, $ffp->coll_data.mask_stat, $ffp->coll_data.is_coll_end, (int)(($ffp->coll_data.p_translate->y + $ffp->coll_data.map_coll.bottom) * 1000.0)',
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed',
        'printf "RENDER_PROFILE_LEVEL=%u\n", gNdsRendererProfileLevel',
        ('printf "THROW_INPUT=%u,%#x,%u,%u,%#x,%d,%d\n", *(unsigned int *)0x{0:x8}, *(unsigned int *)0x{1:x8}, gNdsControllerPlaybackReadCount - $input_reads_base, gNdsControllerPlaybackFrameCount - $input_frames_base, *(unsigned short *)0x{2:x8}, *(signed char *)0x{3:x8}, *(signed char *)0x{4:x8}' -f
            $playbackEnabledAddress, $playbackConnectedAddress,
            $playbackPadsAddress, ($playbackPadsAddress + 2),
            ($playbackPadsAddress + 3)),
        'printf "MEMARENA=%#x,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom',
        $captureCommand,
        ('set {{unsigned int}}0x{0:x8} = 0' -f
            $playbackEnabledAddress),
        'detach',
        'quit'
    )

    $gdbResult = Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName -TimeoutSeconds $TimeoutSeconds
    $gdbStdout = $gdbResult.Stdout

    $setup = [regex]::Match(
        $gdbStdout, 'THROW_SETUP=([0-9]+),([0-9]+)')
    $result = [regex]::Match(
        $gdbStdout,
        'THROW_RESULT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $actors = [regex]::Match(
        $gdbStdout,
        'THROW_ACTORS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $approach = [regex]::Match(
        $gdbStdout, 'THROW_APPROACH=([0-9]+),([0-9]+),(-?[0-9]+)')
    $lifecycle = [regex]::Match(
        $gdbStdout,
        'THROW_LIFECYCLE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $cause = [regex]::Match(
        $gdbStdout,
        'THROW_CAUSE=(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+)')
    $default = [regex]::Match(
        $gdbStdout,
        'THROW_DEFAULT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+)')
    $damage = [regex]::Match(
        $gdbStdout,
        'THROW_DAMAGE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $floor = [regex]::Match(
        $gdbStdout,
        'THROW_FLOOR=([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $links = [regex]::Match(
        $gdbStdout,
        'THROW_LINKS=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $final = [regex]::Match(
        $gdbStdout,
        'THROW_FINAL=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+)')
    $harn = [regex]::Match(
        $gdbStdout,
        'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match(
        $gdbStdout,
        'SCENE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $profile = [regex]::Match(
        $gdbStdout, 'RENDER_PROFILE_LEVEL=([0-9]+)')
    $input = [regex]::Match(
        $gdbStdout,
        'THROW_INPUT=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $memory = [regex]::Match(
        $gdbStdout,
        'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')

    Assert-Condition ($setup.Success -and
        [int]$setup.Groups[1].Value -eq 1) (
        'Canonical Mario/Fox actors were unavailable after GO.') $gdbStdout
    Assert-Condition $result.Success (
        'Natural throw verifier did not emit a terminal result.') $gdbStdout
    Assert-Condition (Test-Path -LiteralPath $screenshotPath -PathType Leaf) (
        "Throw-release run did not produce its evidence screenshot: $screenshotPath") $gdbStdout
    Assert-Condition ((Get-Item -LiteralPath $screenshotPath).Length -gt
        1024) (
        "Throw-release evidence screenshot is unexpectedly small: $screenshotPath") $gdbStdout
    Add-Type -AssemblyName System.Drawing
    $screenshotBitmap = [System.Drawing.Bitmap]::FromFile($screenshotPath)
    try {
        Assert-Condition (
            $screenshotBitmap.Width -eq $script:MelonDSCanonicalWindowWidth -and
            $screenshotBitmap.Height -eq $script:MelonDSCanonicalWindowHeight) (
            "Throw-release evidence screenshot did not use the canonical $($script:MelonDSCanonicalWindowWidth)x$($script:MelonDSCanonicalWindowHeight) window: $screenshotPath") $gdbStdout
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

    $rv = Get-Ints $result
    $approachv = Get-Ints $approach
    $av = Get-Ints $actors
    $lv = Get-Ints $lifecycle
    $causev = Get-Ints $cause
    $dv = Get-Ints $default
    $damv = Get-Ints $damage
    $flv = Get-Ints $floor
    $linkv = Get-Ints $links
    $fv = Get-Ints $final
    $hv = Get-Ints $harn
    $sv = Get-Ints $scene
    $iv = Get-Ints $input
    $mv = Get-Ints $memory

    if ($rv[0] -eq 2) {
        throw ('No natural Mario catch/forward-throw floor recovery occurred ' +
            'before the canonical one-minute match ended.' +
            [Environment]::NewLine + $gdbStdout)
    }

    Assert-Condition ($rv[0] -eq 1 -and $rv[1] -ge 1 -and
        $rv[1] -le 48 -and $rv[2] -eq 1 -and $rv[3] -eq 1 -and
        $rv[4] -eq 1 -and $rv[5] -eq 1 -and $rv[6] -eq 1 -and
        $rv[7] -eq 1 -and $rv[8] -eq 1 -and $rv[9] -eq 1) (
        'Catch, throw, release, and source damage-init events were not exactly once.') $gdbStdout
    Assert-Condition ($approach.Success -and $approachv[0] -ge 1 -and
        $approachv[0] -ge $rv[1]) (
        'Mario did not naturally approach Fox through short source Walk/Dash/Run steps before grabbing.') $gdbStdout
    Assert-Condition ($actors.Success -and $av[0] -ne 0 -and
        $av[1] -ne 0 -and $av[2] -eq 0 -and $av[3] -eq 0 -and
        $av[4] -eq 0 -and $av[5] -eq 1 -and $av[6] -eq 1 -and
        $av[7] -eq 1 -and $av[8] -eq 3) (
        'Verifier actors were not human Mario versus level-3 CPU Fox.') $gdbStdout
    Assert-Condition ($lifecycle.Success -and $lv[0] -eq 166 -and
        $lv[1] -eq 171 -and $lv[2] -eq 167 -and $lv[3] -eq 168 -and
        $lv[4] -eq 169 -and $lv[5] -eq 186 -and $lv[6] -eq 0 -and
        $lv[7] -eq 1 -and $lv[8] -eq 1 -and $lv[9] -eq 1 -and
        $lv[10] -eq 1) (
        'BattleShip Catch/Pull/Wait/ThrowF/ThrownCommon lifecycle diverged.') $gdbStdout
    Assert-Condition ($cause.Success -and $causev[0] -eq 1 -and
        $causev[1] -gt 0 -and $causev[3] -ne 0 -and
        (($causev[2] -band $causev[3]) -ne 0) -and
        $causev[4] -eq 172) (
        'Forward throw was not caused by a fresh A tap during live CatchWait/CaptureWait.') $gdbStdout
    Assert-Condition ($default.Success -and $dv[0] -eq 1 -and
        $dv[1] -eq 1 -and $dv[2] -eq 1 -and $dv[3] -eq 1 -and
        $dv[4] -eq 1 -and $dv[5] -eq 1 -and $dv[6] -eq 1 -and
        $dv[7] -eq 0 -and $dv[8] -ge -1) (
        'Throw release did not perform one source copy/run/reset reconciliation.') $gdbStdout
    Assert-Condition ($damage.Success -and $damv[0] -ge 0 -and
        $damv[1] -gt $damv[0] -and $damv[2] -ge 51 -and
        $damv[2] -le 57 -and $damv[3] -eq 1 -and $damv[4] -eq 1 -and
        $damv[5] -gt 0 -and (($damv[2] -eq 57) -or
            (($damv[6] -gt 0) -and ($damv[7] -gt 0)))) (
        'Released Fox did not enter and retain the source throw damage chain.') $gdbStdout
    Assert-Condition ($floor.Success -and $flv[0] -eq 1 -and
        $flv[1] -eq 3 -and (($flv[2] -band 0x800) -ne 0) -and
        (($flv[3] -band 0x800) -ne 0) -and $flv[4] -eq 1 -and
        [Math]::Abs($flv[5]) -le 2 -and $flv[6] -gt 0 -and
        $flv[7] -gt 0) (
        'Thrown Fox did not sweep, clamp, and DownBounce on Pupupu main floor line 3.') $gdbStdout
    Assert-Condition ($links.Success -and $linkv[0] -eq 1 -and
        $linkv[1] -eq 0 -and $linkv[2] -eq 0 -and $linkv[3] -eq 0) (
        'Catch/capture/throw links remained live after throw release.') $gdbStdout
    Assert-Condition ($final.Success -and ($fv[0] -eq 67 -or $fv[0] -eq 68) -and
        $fv[1] -eq 0 -and $fv[2] -eq 3 -and
        (($fv[3] -band 0x800) -ne 0) -and
        (($fv[4] -band 0x800) -ne 0) -and $fv[5] -eq 1 -and
        [Math]::Abs($fv[6]) -le 2) (
        'Fox did not remain grounded on line 3 after the completed recovery frame.') $gdbStdout
    Assert-Condition ($harn.Success -and $hv[0] -eq 0x4841524e -and
        $hv[1] -eq 163 -and $hv[2] -eq 22 -and $hv[3] -eq 21 -and
        $hv[4] -eq 0) (
        'Throw verifier did not use registered battle_playable_realtime mode 163.') $gdbStdout
    Assert-Condition ($profile.Success -and
        [int64]$profile.Groups[1].Value -eq 0) (
        'Throw verifier did not use the shipped renderer profile.') $gdbStdout
    Assert-Condition ($scene.Success -and $sv[0] -eq 22 -and
        $sv[1] -eq 21 -and $sv[2] -eq 6 -and $sv[3] -eq 1 -and
        $sv[4] -eq 1 -and $sv[5] -gt 0 -and
        ($sv[5] + $sv[6]) -eq 3600) (
        'Throw recovery did not occur inside the original one-minute match.') $gdbStdout
    Assert-Condition ($input.Success -and $iv[0] -eq 1 -and
        (($iv[1] -band 1) -eq 1) -and $iv[2] -gt 0 -and
        $iv[3] -eq 0 -and $iv[4] -eq 0 -and $iv[5] -eq 0 -and
        $iv[6] -eq 0) (
        'Semantic player-0 input was not consumed or the final pad was not neutral.') $gdbStdout
    Assert-Condition ($memory.Success -and $mv[0] -eq 0x4d4c4544 -and
        $mv[1] -eq 22 -and $mv[2] -ge 131072) (
        'Throw-release recovery violated the P1 arena reserve.') $gdbStdout

    Write-Output ((
        'battle_playable throw release recovery passed: attempts={0} ' +
        'status=169/186 damage={1}->{2} floor={3} sweeps={4}/{5} ' +
        'reserve={6} screenshot={7}'
    ) -f $rv[1], $damv[0], $damv[1], $flv[1], $flv[6], $flv[7],
        $mv[2], $screenshotPath)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
}
