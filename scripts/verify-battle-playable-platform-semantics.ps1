[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 180,
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
    throw 'Custom platform-semantics verification requires both -Rom and -Elf.'
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
$tempDir = Get-MelonDSVerifierTempDir `
    -Root $root `
    -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-playable-platform-semantics.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-playable-platform-semantics.stderr.log'
$scriptName = '_battle_playable_platform_semantics.gdb'
$captureHelper = Join-Path $tempDir '_capture_platform_semantics.ps1'
$configState = $null
$emulator = $null

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context = '')

    if ($Condition) { return }
    if ([string]::IsNullOrWhiteSpace($Context)) { throw $Message }
    throw "$Message`n$Context"
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

function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)

    $escapedName = [regex]::Escape($Name)
    $line = $elfSymbols |
        Where-Object { $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escapedName$" } |
        Select-Object -First 1
    if (-not $line) { throw "ELF symbol not found: $Name" }
    $match = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($match.Groups[1].Value, 16))
}

function Get-LiveLineGeometryGdbCommands {
    param([ValidateRange(0,3)][int]$Line)

    $prefix = '$mp_line{0}' -f $Line
    return @(
        ('set {0}_link_word = ((unsigned int *)gMPCollisionGeometry->vertex_links)[{1}]' -f $prefix, $Line),
        ('set {0}_first_index = ({0}_link_word >> 16) & 0xffff' -f $prefix),
        ('set {0}_vertex_count = {0}_link_word & 0xffff' -f $prefix),
        ('set {0}_last_index = {0}_first_index + {0}_vertex_count - 1' -f $prefix),
        ('set {0}_first_id_word = ((unsigned int *)gMPCollisionGeometry->vertex_id)[{0}_first_index / 2]' -f $prefix),
        ('set {0}_first_id = (({0}_first_index & 1) != 0) ? ({0}_first_id_word & 0xffff) : (({0}_first_id_word >> 16) & 0xffff)' -f $prefix),
        ('set {0}_last_id_word = ((unsigned int *)gMPCollisionGeometry->vertex_id)[{0}_last_index / 2]' -f $prefix),
        ('set {0}_last_id = (({0}_last_index & 1) != 0) ? ({0}_last_id_word & 0xffff) : (({0}_last_id_word >> 16) & 0xffff)' -f $prefix),
        ('set {0}_first_x_half = {0}_first_id * 3' -f $prefix),
        ('set {0}_first_x_word = ((unsigned int *)gMPCollisionGeometry->vertex_data)[{0}_first_x_half / 2]' -f $prefix),
        ('set {0}_first_x = (signed short)((({0}_first_x_half & 1) != 0) ? ({0}_first_x_word & 0xffff) : (({0}_first_x_word >> 16) & 0xffff))' -f $prefix),
        ('set {0}_first_y_half = {0}_first_x_half + 1' -f $prefix),
        ('set {0}_first_y_word = ((unsigned int *)gMPCollisionGeometry->vertex_data)[{0}_first_y_half / 2]' -f $prefix),
        ('set {0}_first_y = (signed short)((({0}_first_y_half & 1) != 0) ? ({0}_first_y_word & 0xffff) : (({0}_first_y_word >> 16) & 0xffff))' -f $prefix),
        ('set {0}_first_flags_half = {0}_first_x_half + 2' -f $prefix),
        ('set {0}_first_flags_word = ((unsigned int *)gMPCollisionGeometry->vertex_data)[{0}_first_flags_half / 2]' -f $prefix),
        ('set {0}_first_flags = (({0}_first_flags_half & 1) != 0) ? ({0}_first_flags_word & 0xffff) : (({0}_first_flags_word >> 16) & 0xffff)' -f $prefix),
        ('set {0}_last_x_half = {0}_last_id * 3' -f $prefix),
        ('set {0}_last_x_word = ((unsigned int *)gMPCollisionGeometry->vertex_data)[{0}_last_x_half / 2]' -f $prefix),
        ('set {0}_last_x = (signed short)((({0}_last_x_half & 1) != 0) ? ({0}_last_x_word & 0xffff) : (({0}_last_x_word >> 16) & 0xffff))' -f $prefix),
        ('set {0}_last_y_half = {0}_last_x_half + 1' -f $prefix),
        ('set {0}_last_y_word = ((unsigned int *)gMPCollisionGeometry->vertex_data)[{0}_last_y_half / 2]' -f $prefix),
        ('set {0}_last_y = (signed short)((({0}_last_y_half & 1) != 0) ? ({0}_last_y_word & 0xffff) : (({0}_last_y_word >> 16) & 0xffff))' -f $prefix)
    )
}

function Resolve-VisibilityOutput {
    param([string]$Path)

    $visibilityDir = [System.IO.Path]::GetFullPath(
        (Join-Path $root 'artifacts\visibility'))
    $resolved = if ([string]::IsNullOrWhiteSpace($Path)) {
        $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff'
        Join-Path $visibilityDir "${stamp}_platform-semantics-p$PID.png"
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
        "Platform-semantics captures must stay under '$visibilityDir': $resolved"
    return $resolved
}

function Write-ExactCaptureHelper {
    param([Parameter(Mandatory=$true)][string]$Path)

    $helper = @'
param(
    [Parameter(Mandatory=$true)][int]$EmulatorProcessId,
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if ($null -eq ('Smash64DSPlatformSemanticsCapture' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Smash64DSPlatformSemanticsCapture
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr window, IntPtr insertAfter,
        int x, int y, int width, int height, uint flags);
}
"@
}

$process = Get-Process -Id $EmulatorProcessId -ErrorAction Stop
$deadline = (Get-Date).AddSeconds(10)
do {
    $process.Refresh()
    if ($process.HasExited) { throw 'melonDS exited before evidence capture.' }
    if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 50
} while ((Get-Date) -lt $deadline)
$handle = $process.MainWindowHandle
if ($handle -eq [IntPtr]::Zero) {
    throw 'melonDS did not expose a window for evidence capture.'
}

# Match scripts/lib/melonds.ps1's canonical natural stacked-screen window.
[void][Smash64DSPlatformSemanticsCapture]::ShowWindow($handle, 9)
[void][Smash64DSPlatformSemanticsCapture]::SetWindowPos(
    $handle, [IntPtr](-1), 24, 24, 488, 675, 0x40)
[void][Smash64DSPlatformSemanticsCapture]::SetForegroundWindow($handle)
Start-Sleep -Milliseconds 250

$rect = New-Object Smash64DSPlatformSemanticsCapture+Rect
if (-not [Smash64DSPlatformSemanticsCapture]::GetWindowRect(
        $handle, [ref]$rect)) {
    throw 'Could not read the melonDS evidence window bounds.'
}
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if (($width -le 0) -or ($height -le 0)) {
    throw "Invalid melonDS evidence window bounds ${width}x${height}."
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Output) |
    Out-Null
$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
try {
    $graphics.CopyFromScreen(
        $rect.Left, $rect.Top, 0, 0, $bitmap.Size)
    $bitmap.Save($Output, [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
    [void][Smash64DSPlatformSemanticsCapture]::SetWindowPos(
        $handle, [IntPtr](-2), 0, 0, 0, 0, 0x43)
}
'@
    Set-Content -LiteralPath $Path -Value $helper
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

Assert-Condition ((Test-Path -LiteralPath $rom -PathType Leaf) -and
    (Test-Path -LiteralPath $elf -PathType Leaf)) `
    'Canonical mode-163 build did not produce the expected ROM and ELF.'
Assert-Condition (Test-Path -LiteralPath $melonDsPath -PathType Leaf) `
    "melonDS executable not found: $melonDsPath"
Assert-Condition (Test-Path -LiteralPath $Gdb -PathType Leaf) `
    "GDB executable not found: $Gdb"
Assert-Condition (Test-Path -LiteralPath $nm -PathType Leaf) `
    "ELF symbol tool not found: $nm"
$elfSymbols = @(& $nm -a $elf)
Assert-Condition ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
$usesLiveMPProcess = [bool]($elfSymbols -match
    '\sndsBaseMPProcessSetLandingFloor$')

# These are private controller-backend objects. Resolve them from the exact ELF
# instead of encoding build-specific addresses; osContGetReadData remains the
# only path that transfers the externally supplied pad into fighter input.
$playbackPadsAddress = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$playbackConnectedAddress = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$playbackEnabledAddress = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
foreach ($symbol in @(
    'ndsBattlePlayableFrameCompleteMarker',
    'mpProcessSetLandingFloor',
    'ndsBaseFTCommonSquatCheckGotoPass',
    'gNdsCollisionRuntimeDiagnostics',
    'gNdsControllerPlaybackReadCount',
    'gNdsBootSelfTestResult',
    'gMPCollisionGeometry',
    'gMPCollisionVertexInfo',
    'gSCManagerBattleState',
    'sIFCommonTimerIsStarted'
)) {
    [void](Get-ElfSymbolAddress $symbol)
}
$liveLineGeometryGdbCommands = @(
    foreach ($line in 0..3) {
        Get-LiveLineGeometryGdbCommands -Line $line
    }
)

$screenshotPath = Resolve-VisibilityOutput $Screenshot
New-Item -ItemType Directory -Force -Path $logDir, $tempDir,
    (Split-Path -Parent $screenshotPath) | Out-Null
Write-ExactCaptureHelper -Path $captureHelper

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath `
        -GdbPort $verifierContext.GdbPort `
        -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator `
        -Port $verifierContext.GdbPort | Out-Null

    $captureHelperGdb = $captureHelper.Replace('\', '/')
    $screenshotGdb = $screenshotPath.Replace('\', '/')
    $captureCommand =
        'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" -EmulatorProcessId {1} -Output "{2}"' -f
        $captureHelperGdb, $emulator.Id, $screenshotGdb

    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set breakpoint pending off',
        'set remotetimeout 10',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        # BattleShip ifcommon.c starts the original timer here. Keep playback
        # neutral until the first fully completed post-GO frame.
        'tbreak ifcommon.c:3175',
        'continue',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{unsigned int}}0x{0:x8} = 0' -f ($playbackPadsAddress + 4)),
        ('set {{unsigned int}}0x{0:x8} = 3' -f $playbackConnectedAddress),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $playbackEnabledAddress),
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'set $failure = 0',
        'set $phase = 1',
        'set $total_frames = 0',
        'set $phase_frames = 0',
        'set $release_pad = 1',
        'set $mario_gobj = gSCManagerBattleState->players[0].fighter_gobj',
        'set $fox_gobj = gSCManagerBattleState->players[1].fighter_gobj',
        'set $mario = (FTStruct *)$mario_gobj->user_data.p',
        'set $fox = (FTStruct *)$fox_gobj->user_data.p',
        'set $mario_coll = &($mario->coll_data)',
        'set $start_percent = $mario->percent_damage',
        'set $start_time_passed = gSCManagerBattleState->time_passed',
        'set $start_reads = gNdsControllerPlaybackReadCount',
        'set $pass_rejects_base = gNdsCollisionRuntimeDiagnostics.floor_adj_pass_rejects',
        ('set $uses_live_mpprocess = {0}' -f [int]$usesLiveMPProcess),
        'set $mario_asc_sweep_mask = 0',
        'set $mario_asc_sweep_count = 0',
        'set $mario_asc_sweep_misses = 0',
        'set $mario_asc_accepts = 0',
        'set $mario_asc_accept_line = -1',
        'set $mario_reverse_hit_mask = 0',
        'set $mario_reverse_hit_count = 0',
        'set $mario_land_count = 0',
        'set $last_land_line = -1',
        'set $last_land_flags = 0',
        'set $last_land_prev_foot = 0.0',
        'set $last_land_current_foot = 0.0',
        'set $last_land_vy = 0.0',
        'set $last_land_ga = -1',
        'set $last_land_crossed_down = 0',
        'set $flight_bit = 0',
        'set $flight_line = -1',
        'set $flight_plane = 0.0',
        'set $flight_rise_frames = 0',
        'set $flight_descending_seen = 0',
        'set $continued_ascent_mask = 0',
        'set $strict_descent_mask = 0',
        'set $downward_cross_mask = 0',
        'set $mario_pass_rejects = 0',
        'set $pass_reject_line = -1',
        'set $pass_reject_flags = 0',
        'set $pass_fall_transitions = 0',
        'set $pass_fall_ignore = -2',
        'set $pass_fall_ga = -1',
        'set $seen_pass_fall_cleanup = 0',
        'set $main_land_after_pass = 0',
        'set $main_land_after_pass_line = -1',
        'set $main_land_after_pass_flags = 0',
        # Each bit represents one of Dream Land's pass-through floors:
        # center line 0, right line 1, and left line 2. Every semantic mask
        # must finish at exactly 0x7 after one continuous input-only route.
        'set $approach_mask = 0',
        'set $up_mask = 0',
        'set $land_wait_mask = 0',
        'set $jump_reland_mask = 0',
        'set $pass_mask = 0',
        'set $cleanup_mask = 0',
        'set $main_floor_mask = 0',
        'set $target_line = -1',
        'set $target_bit = 0',
        'set $target_y = 0.0',
        'set $target_x_min = 0.0',
        'set $target_x_max = 0.0',
        'set $cycle_initialized = 0',
        'set $cycle_start_x = 0.0',
        'set $cycle_approach_x = 0.0',
        'set $cycle_seen_knee_1 = 0',
        'set $cycle_seen_jump_air_1 = 0',
        'set $cycle_apex_1 = 0',
        'set $cycle_stable_1 = 0',
        'set $cycle_seen_knee_2 = 0',
        'set $cycle_seen_jump_air_2 = 0',
        'set $cycle_apex_2 = 0',
        'set $cycle_stable_2 = 0',
        'set $cycle_seen_down_tap = 0',
        'set $cycle_seen_squat = 0',
        'set $cycle_seen_allow_pass = 0',
        'set $cycle_squat_checks = 0',
        'set $cycle_squat_wait_1 = -1',
        'set $cycle_squat_wait_2 = -1',
        'set $cycle_squat_wait_3 = -1',
        'set $cycle_pass_post_calls = 0',
        'set $cycle_pass_post_ignore = -1',
        'set $cycle_pass_post_tap = 0',
        'set $cycle_pass_post_air_ready = 0',
        'set $cycle_pass_rejects = 0',
        'set $cycle_pass_reject_line = -1',
        'set $cycle_pass_reject_flags = 0',
        'set $cycle_pass_fall_transitions = 0',
        'set $cycle_seen_pass_fall_cleanup = 0',
        'set $side_approaches = 0',
        'set $side_up_crossings = 0',
        'set $side_first_landings = 0',
        'set $side_stable_waits = 0',
        'set $side_second_jumps = 0',
        'set $side_second_landings = 0',
        'set $side_pass_crossings = 0',
        'set $side_cleanups = 0',
        'set $side_main_landings = 0',
        'set $side_cycles_completed = 0',
        'set $side_pass_rejects_total = 0',
        'set $line1_approach_delta = 0.0',
        'set $line2_approach_delta = 0.0',
        'set $seen_knee_1 = 0',
        'set $seen_jump_air_1 = 0',
        'set $jump_aerial_pulsed_1 = 0',
        'set $seen_jump_aerial_1 = 0',
        'set $up_crossed = 0',
        'set $up_cross_frame = 0',
        'set $up_prev_foot = 0.0',
        'set $up_foot = 0.0',
        'set $apex_1 = 0',
        'set $land_1 = 0',
        'set $land_1_line = -1',
        'set $land_1_flags = 0',
        'set $stable_1 = 0',
        'set $stable_1_max = 0',
        'set $seen_knee_2 = 0',
        'set $seen_jump_air_2 = 0',
        'set $jump_2_departed = 0',
        'set $apex_2 = 0',
        'set $land_2 = 0',
        'set $land_2_line = -1',
        'set $land_2_flags = 0',
        'set $stable_2 = 0',
        'set $stable_2_max = 0',
        'set $seen_down_tap = 0',
        'set $seen_squat = 0',
        'set $seen_allow_pass = 0',
        'set $squat_checks = 0',
        'set $squat_wait_1 = -1',
        'set $squat_wait_2 = -1',
        'set $squat_wait_3 = -1',
        'set $pass_post_calls = 0',
        'set $pass_post_ignore = -1',
        'set $pass_post_tap = 0',
        'set $pass_post_air_ready = 0',
        'set $down_crossed = 0',
        'set $down_prev_foot = 0.0',
        'set $down_foot = 0.0',
        'set $foot = $mario->coll_data.p_translate->y + $mario->coll_data.map_coll.bottom',
        'set $prev_foot = $foot',
        'set $prev_vy = $mario->physics.vel_air.y',
        'set $prev_status = $mario->status_id',
        'set $land_base = 0',
        # The first nonzero input occurs only after the completed GO frame and
        # is consumed by the ordinary live controller path on the next frame.
        ('set {{unsigned short}}0x{0:x8} = 0x0008' -f $playbackPadsAddress),
        # Validate the exact natural starting scene before observing behavior.
        'if (gNdsSceneHarnessResult != 0x4841524e) || (gNdsSceneHarnessMode != 163)',
        'set $failure = 10',
        'end',
        'if (gNdsSceneHarnessSceneCurr != 22) || (gNdsSceneHarnessScenePrev != 21)',
        'set $failure = 11',
        'end',
        'if (gSCManagerSceneData.scene_curr != 22) || (gSCManagerSceneData.scene_prev != 21) || (gSCManagerSceneData.gkind != 6)',
        'set $failure = 12',
        'end',
        'if (gSCManagerBattleState->game_status != 1) || (sIFCommonTimerIsStarted != 1) || (gSCManagerBattleState->time_remain == 0)',
        'set $failure = 13',
        'end',
        'if ($mario->fkind != 0) || ($mario->pkind != 0) || ($mario->player != 0)',
        'set $failure = 14',
        'end',
        'if ($fox->fkind != 1) || ($fox->pkind != 1) || ($fox->player != 1) || ($fox->level != 3)',
        'set $failure = 15',
        'end',
        'if ($mario->status_id != 10) || ($mario->ga != 0) || ($mario->coll_data.floor_line_id != 3) || (($mario->coll_data.mask_stat & 0x800) == 0)',
        'set $failure = 16',
        'end',
        'if ($mario->coll_data.p_translate->x < -570.0) || ($mario->coll_data.p_translate->x > 570.0)',
        'set $failure = 17',
        'end',
        # ndsOsSelfTest includes ndsControllerBackendSelfTest, whose physical
        # KEY_X/KEY_Y -> U_CBUTTONS and KEY_DOWN -> -80 checks cover the live
        # DS mapping that private N64-pad playback intentionally bypasses.
        'if gNdsBootSelfTestResult != 0x50415353',
        'set $failure = 18',
        'end',
        'if $failure == 0',
        'set $approach_mask = $approach_mask | 1',
        'end',
        # The public landing entry is stable in both backends and runs before
        # either implementation clamps Mario to the selected floor plane.
        'break mpProcessSetLandingFloor if ($mario != 0) && ($r0 == $mario_coll)',
        'commands',
        'silent',
        'set $mario_land_count = $mario_land_count + 1',
        'set $last_land_line = $mario_coll->floor_line_id',
        'set $last_land_flags = $mario_coll->floor_flags',
        'set $last_land_crossed_down = 0',
        'if ($last_land_line >= 0) && ($last_land_line <= 2) && (($last_land_flags & 0x4000) != 0)',
        'set $mario_reverse_hit_mask = $mario_reverse_hit_mask | (1 << $last_land_line)',
        'set $mario_reverse_hit_count = $mario_reverse_hit_count + 1',
        'end',
        'if (($phase == 2) || ($phase == 5) || ($phase == 12) || ($phase == 15)) && ($flight_bit != 0) && ($mario_coll->p_map_coll != 0) && ($mario_coll->p_translate != 0)',
        'set $last_land_prev_foot = $mario_coll->pos_prev.y + $mario_coll->p_map_coll->bottom',
        'set $last_land_current_foot = $mario_coll->p_translate->y + $mario_coll->map_coll.bottom',
        'set $last_land_vy = $mario->physics.vel_air.y',
        'set $last_land_ga = $mario->ga',
        'if ($last_land_line == $flight_line) && (($last_land_flags & 0x4000) != 0) && ($last_land_ga == 1) && ($last_land_vy < 0.0) && ($last_land_prev_foot > $flight_plane) && ($last_land_current_foot <= $flight_plane) && ($last_land_current_foot < $last_land_prev_foot)',
        'set $last_land_crossed_down = 1',
        'end',
        'end',
        'continue',
        'end',
        # Capture BattleShip's exact 3,2,1 Squat pass countdown before each
        # decrement, isolated to the same Mario GObj.
        'break ndsBaseFTCommonSquatCheckGotoPass if ($mario_gobj != 0) && ($r0 == $mario_gobj)',
        'commands',
        'silent',
        'if $phase >= 10',
        'set $cycle_squat_checks = $cycle_squat_checks + 1',
        'if $cycle_squat_checks == 1',
        'set $cycle_squat_wait_1 = $mario->status_vars.common.squat.pass_wait',
        'end',
        'if $cycle_squat_checks == 2',
        'set $cycle_squat_wait_2 = $mario->status_vars.common.squat.pass_wait',
        'end',
        'if $cycle_squat_checks == 3',
        'set $cycle_squat_wait_3 = $mario->status_vars.common.squat.pass_wait',
        'end',
        'else',
        'set $squat_checks = $squat_checks + 1',
        'if $squat_checks == 1',
        'set $squat_wait_1 = $mario->status_vars.common.squat.pass_wait',
        'end',
        'if $squat_checks == 2',
        'set $squat_wait_2 = $mario->status_vars.common.squat.pass_wait',
        'end',
        'if $squat_checks == 3',
        'set $squat_wait_3 = $mario->status_vars.common.squat.pass_wait',
        'end',
        'end',
        'continue',
        'end',
        # One completed-frame state machine observes only natural runtime
        # state. It never calls an inferior function or writes fighter/map data.
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        'set $total_frames = $total_frames + 1',
        'set $phase_frames = $phase_frames + 1',
        'set $phase_start = $phase',
        # Hold the first jump input through KneeBend so this is BattleShip's
        # full jump rather than the one-frame short hop that cannot reach the
        # center platform. Later jump/down pulses still release after one tick.
        'if ($release_pad != 0) && ((($phase != 1) && ($phase != 11)) || ($mario->ga == 1))',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $playbackPadsAddress),
        'set $release_pad = 0',
        'end',
        'set $foot = $mario->coll_data.p_translate->y + $mario->coll_data.map_coll.bottom',
        'set $root_x = $mario->coll_data.p_translate->x',
        'set $vy = $mario->physics.vel_air.y',
        # Global stop conditions keep the proof inside one uninterrupted
        # source match and reject CPU interference rather than hiding it.
        'if $total_frames > 1200',
        'set $failure = 20',
        'end',
        'if (gSCManagerBattleState->players[0].fighter_gobj != $mario_gobj) || (gSCManagerBattleState->players[1].fighter_gobj != $fox_gobj)',
        'set $failure = 21',
        'end',
        'if (gSCManagerSceneData.scene_curr != 22) || (gSCManagerBattleState->game_status != 1) || (sIFCommonTimerIsStarted != 1) || (gSCManagerBattleState->time_remain == 0)',
        'set $failure = 22',
        'end',
        'if ($mario->percent_damage != $start_percent) || ($mario->hitlag_tics != 0) || ($mario->capture_gobj != 0)',
        'set $failure = 27',
        'end',
        'if ($phase < 10) && (($root_x < -570.0) || ($root_x > 570.0))',
        'set $failure = 24',
        'end',
        'if ($phase >= 10) && (($root_x < -2200.0) || ($root_x > 2200.0))',
        'set $failure = 103',
        'end',
        'if $mario_asc_accepts != 0',
        'set $failure = 25',
        'end',
        'if (gNdsCollisionRuntimeDiagnostics.floor_adj_ambiguous != 0) || (gNdsCollisionRuntimeDiagnostics.topology_ambiguous_endpoints != 0) || (gNdsCollisionRuntimeDiagnostics.topology_invalid != 0) || (gNdsCollisionRuntimeDiagnostics.topology_getter_invalid != 0)',
        'set $failure = 26',
        'end',
        # Every platform flight must continue upward for two completed Air
        # frames, expose a later strictly negative Air frame, then enter the
        # landing clamp while its feet cross the exact plane downward.
        'if ($failure == 0) && (($phase_start == 2) || ($phase_start == 5) || ($phase_start == 12) || ($phase_start == 15))',
        'if $mario_land_count <= $land_base',
        'if ($mario->ga == 1) && (($mario->coll_data.mask_stat & 0x800) == 0)',
        'if ($vy > 0.0) && ($foot > ($flight_plane + 0.25))',
        'set $flight_rise_frames = $flight_rise_frames + 1',
        'if $flight_rise_frames >= 2',
        'set $continued_ascent_mask = $continued_ascent_mask | $flight_bit',
        'end',
        'end',
        'if ($vy < 0.0) && ($flight_descending_seen == 0)',
        'if ($continued_ascent_mask & $flight_bit) == 0',
        'set $failure = 28',
        'else',
        'set $flight_descending_seen = 1',
        'set $strict_descent_mask = $strict_descent_mask | $flight_bit',
        'if $phase_start == 2',
        'set $apex_1 = 1',
        'end',
        'if $phase_start == 5',
        'set $apex_2 = 1',
        'end',
        'if $phase_start == 12',
        'set $cycle_apex_1 = 1',
        'end',
        'if $phase_start == 15',
        'set $cycle_apex_2 = 1',
        'end',
        'end',
        'end',
        'end',
        'else',
        'if (($continued_ascent_mask & $flight_bit) == 0) || (($strict_descent_mask & $flight_bit) == 0) || ($last_land_crossed_down == 0)',
        'set $failure = 29',
        'else',
        'set $downward_cross_mask = $downward_cross_mask | $flight_bit',
        'end',
        'end',
        'end',
        # Phase 1: jump from main floor and cross upward through line 0.
        'if ($failure == 0) && ($phase_start == 1)',
        'if $mario->status_id == 20',
        'set $seen_knee_1 = 1',
        'end',
        'if (($mario->status_id == 22) || ($mario->status_id == 23)) && ($mario->ga == 1) && ($vy > 0.0)',
        'set $seen_jump_air_1 = 1',
        'end',
        # Dream Land line 0 is the high center platform. Mario's grounded jump
        # alone does not reach y=1542, so issue one natural aerial-jump tap at
        # the first apex and let the ordinary interrupt/status path consume it.
        'if (($mario->status_id == 24) || ($mario->status_id == 25)) && ($mario->ga == 1)',
        'set $seen_jump_aerial_1 = 1',
        'end',
        'if ($seen_jump_air_1 != 0) && ($jump_aerial_pulsed_1 == 0) && ($mario->ga == 1) && ($prev_vy > 0.0) && ($vy <= 0.0)',
        ('set {{unsigned short}}0x{0:x8} = 0x0008' -f $playbackPadsAddress),
        'set $release_pad = 1',
        'set $jump_aerial_pulsed_1 = 1',
        'end',
        'if ($prev_foot < 1541.75) && ($foot > 1542.25)',
        'if ($seen_knee_1 == 0) || ($seen_jump_air_1 == 0) || ($seen_jump_aerial_1 == 0) || ($mario->ga != 1) || ($vy <= 0.0) || (($mario->coll_data.mask_stat & 0x800) != 0)',
        'set $failure = 30',
        'else',
        'set $mario_asc_sweep_mask = $mario_asc_sweep_mask | 1',
        'set $mario_asc_sweep_count = $mario_asc_sweep_count + 1',
        'if gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_sweeps < $mario_asc_sweep_count',
        'set $failure = 31',
        'else',
        'set $up_crossed = 1',
        'set $up_mask = $up_mask | 1',
        'set $up_cross_frame = $total_frames',
        'set $up_prev_foot = $prev_foot',
        'set $up_foot = $foot',
        'set $phase = 2',
        'set $phase_frames = 0',
        'set $land_base = $mario_land_count',
        'set $flight_bit = 1',
        'set $flight_line = 0',
        'set $flight_plane = 1542.0',
        'set $flight_rise_frames = 0',
        'set $flight_descending_seen = 0',
        'set $last_land_crossed_down = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 90',
        'set $failure = 32',
        'end',
        'end',
        # Phase 2: require the fully ordered first flight and landing on line 0.
        'if ($failure == 0) && ($phase_start == 2)',
        'if $mario_land_count > $land_base',
        'if ($last_land_line != 0) || (($last_land_flags & 0x4000) == 0)',
        'set $failure = 40',
        'else',
        'if ($apex_1 != 0) && (($downward_cross_mask & 1) != 0) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 0) && (($mario->coll_data.mask_stat & 0x800) != 0)',
        'set $land_1 = 1',
        'set $land_1_line = $last_land_line',
        'set $land_1_flags = $last_land_flags',
        'set $phase = 3',
        'set $phase_frames = 0',
        'set $stable_1 = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 120',
        'set $failure = 41',
        'end',
        'end',
        # Phase 3: require eight consecutive source Wait/Ground frames on the
        # exact line-0 plane before requesting the platform jump.
        'if ($failure == 0) && ($phase_start == 3)',
        'if ($mario->status_id == 10) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 0) && (($mario->coll_data.floor_flags & 0x4000) != 0) && (($mario->coll_data.mask_stat & 0x800) != 0) && ($foot >= 1541.95) && ($foot <= 1542.05)',
        'set $stable_1 = $stable_1 + 1',
        'else',
        'set $stable_1 = 0',
        'end',
        'if $stable_1 > $stable_1_max',
        'set $stable_1_max = $stable_1',
        'end',
        'if $stable_1 >= 8',
        'set $land_wait_mask = $land_wait_mask | 1',
        ('set {{unsigned short}}0x{0:x8} = 0x0008' -f $playbackPadsAddress),
        'set $release_pad = 1',
        'set $phase = 4',
        'set $phase_frames = 0',
        'set $land_base = $mario_land_count',
        'end',
        'if $phase_frames > 120',
        'set $failure = 50',
        'end',
        'end',
        # Phase 4: prove a second natural KneeBend -> Jump departure from the
        # platform, with the floor mask released and upward velocity intact.
        'if ($failure == 0) && ($phase_start == 4)',
        'if ($mario->status_id == 20) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 0)',
        'set $seen_knee_2 = 1',
        'end',
        'if (($mario->status_id == 22) || ($mario->status_id == 23)) && ($mario->ga == 1) && ($vy > 0.0)',
        'set $seen_jump_air_2 = 1',
        'end',
        'if ($foot > 1542.25) && ($seen_knee_2 != 0) && ($seen_jump_air_2 != 0)',
        'if (($mario->coll_data.mask_stat & 0x800) != 0) || ($mario_land_count != $land_base)',
        'set $failure = 60',
        'else',
        'set $jump_2_departed = 1',
        'set $phase = 5',
        'set $phase_frames = 0',
        'set $flight_bit = 2',
        'set $flight_line = 0',
        'set $flight_plane = 1542.0',
        'set $flight_rise_frames = 0',
        'set $flight_descending_seen = 0',
        'set $last_land_crossed_down = 0',
        'end',
        'end',
        'if $phase_frames > 90',
        'set $failure = 61',
        'end',
        'end',
        # Phase 5: require the fully ordered second flight onto line 0.
        'if ($failure == 0) && ($phase_start == 5)',
        'if $mario_land_count > $land_base',
        'if ($last_land_line != 0) || (($last_land_flags & 0x4000) == 0)',
        'set $failure = 70',
        'else',
        'if ($apex_2 != 0) && (($downward_cross_mask & 2) != 0) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 0) && (($mario->coll_data.mask_stat & 0x800) != 0)',
        'set $land_2 = 1',
        'set $jump_reland_mask = $jump_reland_mask | 1',
        'set $land_2_line = $last_land_line',
        'set $land_2_flags = $last_land_flags',
        'set $phase = 6',
        'set $phase_frames = 0',
        'set $stable_2 = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 120',
        'set $failure = 71',
        'end',
        'end',
        # Phase 6: establish stable Wait again, then supply one natural down tap.
        'if ($failure == 0) && ($phase_start == 6)',
        'if ($mario->status_id == 10) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 0) && (($mario->coll_data.floor_flags & 0x4000) != 0) && (($mario->coll_data.mask_stat & 0x800) != 0) && ($foot >= 1541.95) && ($foot <= 1542.05)',
        'set $stable_2 = $stable_2 + 1',
        'else',
        'set $stable_2 = 0',
        'end',
        'if $stable_2 > $stable_2_max',
        'set $stable_2_max = $stable_2',
        'end',
        'if $stable_2 >= 4',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 3)),
        'set $release_pad = 1',
        'set $phase = 7',
        'set $phase_frames = 0',
        'end',
        'if $phase_frames > 120',
        'set $failure = 80',
        'end',
        'end',
        # Phase 7: require the original down threshold, Squat countdown, Pass
        # initialization, Mario-specific ignore rejection, and downward crossing.
        'if ($failure == 0) && ($phase_start == 7)',
        'if ($mario->input.pl.stick_range.y <= -53) && ($mario->tap_stick_y < 4)',
        'set $seen_down_tap = 1',
        'end',
        'if $mario->status_id == 28',
        'set $seen_squat = 1',
        'if $mario->status_vars.common.squat.is_allow_pass != 0',
        'set $seen_allow_pass = 1',
        'end',
        'end',
        # The imported setter is inlined into the original Squat countdown in
        # optimized canonical ELFs. Observe its first completed Pass frame:
        # Air, same-line ignore, tap reset, and nonpositive post-gravity Y.
        'if ($mario->status_id == 33) && ($prev_status != 33)',
        'set $pass_post_calls = $pass_post_calls + 1',
        'set $pass_post_ignore = $mario->coll_data.ignore_line_id',
        'set $pass_post_tap = $mario->tap_stick_y',
        'set $pass_post_air_ready = (($mario->ga == 1) && ($vy <= 0.0))',
        'end',
        'if ($prev_foot >= 1541.75) && ($foot < 1541.75)',
        'if (($mario->status_id != 33) && ($mario->status_id != 26)) || ($mario->ga != 1) || (($mario->coll_data.mask_stat & 0x800) != 0) || ($mario->coll_data.ignore_line_id != 0)',
        'set $failure = 90',
        'else',
        'set $mario_pass_rejects = $mario_pass_rejects + 1',
        'set $pass_reject_line = $mario->coll_data.ignore_line_id',
        'set $pass_reject_flags = $mario->coll_data.floor_flags',
        'if ($seen_down_tap == 0) || ($seen_squat == 0) || ($seen_allow_pass == 0) || ($squat_checks != 3) || ($squat_wait_1 != 3) || ($squat_wait_2 != 2) || ($squat_wait_3 != 1)',
        'set $failure = 91',
        'else',
        'if ($pass_post_calls != 1) || ($pass_post_ignore != 0) || ($pass_post_tap != 254) || ($pass_post_air_ready == 0)',
        'set $failure = 92',
        'else',
        'if ($mario_pass_rejects < 1) || ($pass_reject_line != 0) || (($pass_reject_flags & 0x4000) == 0)',
        'set $failure = 93',
        'else',
        'set $down_crossed = 1',
        'set $pass_mask = $pass_mask | 1',
        'set $down_prev_foot = $prev_foot',
        'set $down_foot = $foot',
        'set $phase = 8',
        'set $phase_frames = 0',
        'set $land_base = $mario_land_count',
        'end',
        'end',
        'end',
        'end',
        'end',
        'if $phase_frames > 120',
        'set $failure = 94',
        'end',
        'end',
        # Phase 8: remain neutral until the source Pass animation hands off to
        # Fall. ftMainSetStatus must clear the same-line ignore installed by
        # ftCommonPassSetStatusParam, after which Mario naturally lands on the
        # main floor. This closes the cleanup hole left by stopping immediately
        # below line 0.
        'if ($failure == 0) && ($phase_start == 8)',
        'if ($prev_status == 33) && ($mario->status_id == 26)',
        'set $pass_fall_transitions = $pass_fall_transitions + 1',
        'set $pass_fall_ignore = $mario->coll_data.ignore_line_id',
        'set $pass_fall_ga = $mario->ga',
        'if ($mario->ga != 1) || ($mario->coll_data.ignore_line_id != -1)',
        'set $failure = 95',
        'else',
        'set $seen_pass_fall_cleanup = 1',
        'set $cleanup_mask = $cleanup_mask | 1',
        'end',
        'end',
        'if $mario_land_count > $land_base',
        'if ($seen_pass_fall_cleanup == 0) || ($last_land_line != 3) || (($last_land_flags & 0x4000) != 0)',
        'set $failure = 96',
        'else',
        'if ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 3) && (($mario->coll_data.mask_stat & 0x800) != 0) && ($mario->coll_data.ignore_line_id == -1)',
        'set $main_land_after_pass = 1',
        'set $main_floor_mask = $main_floor_mask | 1',
        'set $main_land_after_pass_line = $last_land_line',
        'set $main_land_after_pass_flags = $last_land_flags',
        # Continue from the untouched main-floor landing into a reusable
        # natural route for right platform line 1, then left platform line 2.
        'set $target_line = 1',
        'set $target_bit = 2',
        'set $target_y = 907.0',
        'set $target_x_min = 1100.0',
        'set $target_x_max = 1750.0',
        'set $cycle_initialized = 0',
        'set $phase = 10',
        'set $phase_frames = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 180',
        'set $failure = 97',
        'end',
        'end',
        # Phase 10 is shared by the two side-platform cycles. Mario approaches
        # each target naturally on main-floor line 3; only the external pad is
        # written. The inner X bands come directly from the BattleShip Pupupu
        # geometry and leave margin at both platform edges.
        'if ($failure == 0) && ($phase_start == 10)',
        'if ($mario->ga != 0) || ($mario->coll_data.floor_line_id != 3) || (($mario->coll_data.mask_stat & 0x800) == 0)',
        'set $failure = 100',
        'else',
        'if $cycle_initialized == 0',
        'set $cycle_initialized = 1',
        'set $cycle_start_x = $root_x',
        'set $cycle_approach_x = $root_x',
        'set $cycle_seen_knee_1 = 0',
        'set $cycle_seen_jump_air_1 = 0',
        'set $cycle_apex_1 = 0',
        'set $cycle_stable_1 = 0',
        'set $cycle_seen_knee_2 = 0',
        'set $cycle_seen_jump_air_2 = 0',
        'set $cycle_apex_2 = 0',
        'set $cycle_stable_2 = 0',
        'set $cycle_seen_down_tap = 0',
        'set $cycle_seen_squat = 0',
        'set $cycle_seen_allow_pass = 0',
        'set $cycle_squat_checks = 0',
        'set $cycle_squat_wait_1 = -1',
        'set $cycle_squat_wait_2 = -1',
        'set $cycle_squat_wait_3 = -1',
        'set $cycle_pass_post_calls = 0',
        'set $cycle_pass_post_ignore = -1',
        'set $cycle_pass_post_tap = 0',
        'set $cycle_pass_post_air_ready = 0',
        'set $cycle_pass_rejects = 0',
        'set $cycle_pass_reject_line = -1',
        'set $cycle_pass_reject_flags = 0',
        'set $cycle_pass_fall_transitions = 0',
        'set $cycle_seen_pass_fall_cleanup = 0',
        'end',
        'if $root_x < $target_x_min',
        ('set {{signed char}}0x{0:x8} = 80' -f ($playbackPadsAddress + 2)),
        'else',
        'if $root_x > $target_x_max',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 2)),
        'else',
        ('set {{signed char}}0x{0:x8} = 0' -f ($playbackPadsAddress + 2)),
        'if $mario->status_id == 10',
        'set $cycle_approach_x = $root_x',
        'if (($target_line == 1) && (($cycle_approach_x - $cycle_start_x) < 600.0)) || (($target_line == 2) && (($cycle_approach_x - $cycle_start_x) > -600.0))',
        'set $failure = 101',
        'else',
        'set $approach_mask = $approach_mask | $target_bit',
        'set $side_approaches = $side_approaches + 1',
        'if $target_line == 1',
        'set $line1_approach_delta = $cycle_approach_x - $cycle_start_x',
        'else',
        'set $line2_approach_delta = $cycle_approach_x - $cycle_start_x',
        'end',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $playbackPadsAddress),
        ('set {{unsigned short}}0x{0:x8} = 0x0008' -f $playbackPadsAddress),
        'set $release_pad = 1',
        'set $phase = 11',
        'set $phase_frames = 0',
        'end',
        'end',
        'end',
        'end',
        'end',
        'if $phase_frames > 300',
        'set $failure = 102',
        'end',
        'end',
        # Phase 11: a full source jump passes upward through the selected side
        # platform. No aerial jump is needed for the y=907/y=904 planes.
        'if ($failure == 0) && ($phase_start == 11)',
        'if ($mario->status_id == 20) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 3)',
        'set $cycle_seen_knee_1 = 1',
        'end',
        'if (($mario->status_id == 22) || ($mario->status_id == 23)) && ($mario->ga == 1) && ($vy > 0.0)',
        'set $cycle_seen_jump_air_1 = 1',
        'end',
        'if ($prev_foot < ($target_y - 0.25)) && ($foot > ($target_y + 0.25))',
        'if ($cycle_seen_knee_1 == 0) || ($cycle_seen_jump_air_1 == 0) || ($mario->ga != 1) || ($vy <= 0.0) || (($mario->coll_data.mask_stat & 0x800) != 0)',
        'set $failure = 110',
        'else',
        'set $mario_asc_sweep_mask = $mario_asc_sweep_mask | $target_bit',
        'set $mario_asc_sweep_count = $mario_asc_sweep_count + 1',
        'if gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_sweeps < $mario_asc_sweep_count',
        'set $failure = 111',
        'else',
        'set $up_mask = $up_mask | $target_bit',
        'set $side_up_crossings = $side_up_crossings + 1',
        'set $phase = 12',
        'set $phase_frames = 0',
        'set $land_base = $mario_land_count',
        'if $target_line == 1',
        'set $flight_bit = 4',
        'else',
        'set $flight_bit = 16',
        'end',
        'set $flight_line = $target_line',
        'set $flight_plane = $target_y',
        'set $flight_rise_frames = 0',
        'set $flight_descending_seen = 0',
        'set $last_land_crossed_down = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 120',
        'set $failure = 112',
        'end',
        'end',
        # Phase 12: require the ordered first side-platform flight and landing.
        'if ($failure == 0) && ($phase_start == 12)',
        'if $mario_land_count > $land_base',
        'if ($last_land_line != $target_line) || (($last_land_flags & 0x4000) == 0)',
        'set $failure = 120',
        'else',
        'if ($cycle_apex_1 != 0) && (($downward_cross_mask & $flight_bit) != 0) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == $target_line) && (($mario->coll_data.mask_stat & 0x800) != 0)',
        'set $side_first_landings = $side_first_landings + 1',
        'set $cycle_stable_1 = 0',
        'set $phase = 13',
        'set $phase_frames = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 150',
        'set $failure = 121',
        'end',
        'end',
        # Phase 13: require eight consecutive Wait/Ground frames at the exact
        # selected side-platform plane before the second jump.
        'if ($failure == 0) && ($phase_start == 13)',
        'if ($mario->status_id == 10) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == $target_line) && (($mario->coll_data.floor_flags & 0x4000) != 0) && (($mario->coll_data.mask_stat & 0x800) != 0) && ($foot >= ($target_y - 0.05)) && ($foot <= ($target_y + 0.05))',
        'set $cycle_stable_1 = $cycle_stable_1 + 1',
        'else',
        'set $cycle_stable_1 = 0',
        'end',
        'if $cycle_stable_1 >= 8',
        'set $land_wait_mask = $land_wait_mask | $target_bit',
        'set $side_stable_waits = $side_stable_waits + 1',
        ('set {{unsigned short}}0x{0:x8} = 0x0008' -f $playbackPadsAddress),
        'set $release_pad = 1',
        'set $land_base = $mario_land_count',
        'set $phase = 14',
        'set $phase_frames = 0',
        'end',
        'if $phase_frames > 150',
        'set $failure = 130',
        'end',
        'end',
        # Phase 14: require the second KneeBend -> Jump departure with the
        # source floor mask released and no immediate relanding.
        'if ($failure == 0) && ($phase_start == 14)',
        'if ($mario->status_id == 20) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == $target_line)',
        'set $cycle_seen_knee_2 = 1',
        'end',
        'if (($mario->status_id == 22) || ($mario->status_id == 23)) && ($mario->ga == 1) && ($vy > 0.0)',
        'set $cycle_seen_jump_air_2 = 1',
        'end',
        'if ($foot > ($target_y + 0.25)) && ($cycle_seen_knee_2 != 0) && ($cycle_seen_jump_air_2 != 0)',
        'if (($mario->coll_data.mask_stat & 0x800) != 0) || ($mario_land_count != $land_base)',
        'set $failure = 140',
        'else',
        'set $side_second_jumps = $side_second_jumps + 1',
        'set $phase = 15',
        'set $phase_frames = 0',
        'if $target_line == 1',
        'set $flight_bit = 8',
        'else',
        'set $flight_bit = 32',
        'end',
        'set $flight_line = $target_line',
        'set $flight_plane = $target_y',
        'set $flight_rise_frames = 0',
        'set $flight_descending_seen = 0',
        'set $last_land_crossed_down = 0',
        'end',
        'end',
        'if $phase_frames > 120',
        'set $failure = 141',
        'end',
        'end',
        # Phase 15: require the ordered second flight onto the same side line.
        'if ($failure == 0) && ($phase_start == 15)',
        'if $mario_land_count > $land_base',
        'if ($last_land_line != $target_line) || (($last_land_flags & 0x4000) == 0)',
        'set $failure = 150',
        'else',
        'if ($cycle_apex_2 != 0) && (($downward_cross_mask & $flight_bit) != 0) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == $target_line) && (($mario->coll_data.mask_stat & 0x800) != 0)',
        'set $jump_reland_mask = $jump_reland_mask | $target_bit',
        'set $side_second_landings = $side_second_landings + 1',
        'set $cycle_stable_2 = 0',
        'set $phase = 16',
        'set $phase_frames = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 150',
        'set $failure = 151',
        'end',
        'end',
        # Phase 16: establish stable Wait once more, then supply one down tap.
        'if ($failure == 0) && ($phase_start == 16)',
        'if ($mario->status_id == 10) && ($mario->ga == 0) && ($mario->coll_data.floor_line_id == $target_line) && (($mario->coll_data.floor_flags & 0x4000) != 0) && (($mario->coll_data.mask_stat & 0x800) != 0) && ($foot >= ($target_y - 0.05)) && ($foot <= ($target_y + 0.05))',
        'set $cycle_stable_2 = $cycle_stable_2 + 1',
        'else',
        'set $cycle_stable_2 = 0',
        'end',
        'if $cycle_stable_2 >= 4',
        ('set {{signed char}}0x{0:x8} = -80' -f ($playbackPadsAddress + 3)),
        'set $release_pad = 1',
        'set $phase = 17',
        'set $phase_frames = 0',
        'end',
        'if $phase_frames > 150',
        'set $failure = 160',
        'end',
        'end',
        # Phase 17: require the original Squat 3,2,1 countdown, exact Pass
        # initialization, target-line ignore rejection, and downward crossing.
        'if ($failure == 0) && ($phase_start == 17)',
        'if ($mario->input.pl.stick_range.y <= -53) && ($mario->tap_stick_y < 4)',
        'set $cycle_seen_down_tap = 1',
        'end',
        'if $mario->status_id == 28',
        'set $cycle_seen_squat = 1',
        'if $mario->status_vars.common.squat.is_allow_pass != 0',
        'set $cycle_seen_allow_pass = 1',
        'end',
        'end',
        'if ($mario->status_id == 33) && ($prev_status != 33)',
        'set $cycle_pass_post_calls = $cycle_pass_post_calls + 1',
        'set $cycle_pass_post_ignore = $mario->coll_data.ignore_line_id',
        'set $cycle_pass_post_tap = $mario->tap_stick_y',
        'set $cycle_pass_post_air_ready = (($mario->ga == 1) && ($vy <= 0.0))',
        'end',
        'if ($prev_foot >= ($target_y - 0.25)) && ($foot < ($target_y - 0.25))',
        'if (($mario->status_id != 33) && ($mario->status_id != 26)) || ($mario->ga != 1) || (($mario->coll_data.mask_stat & 0x800) != 0) || ($mario->coll_data.ignore_line_id != $target_line)',
        'set $failure = 170',
        'else',
        'set $cycle_pass_rejects = $cycle_pass_rejects + 1',
        'set $cycle_pass_reject_line = $mario->coll_data.ignore_line_id',
        'set $cycle_pass_reject_flags = $mario->coll_data.floor_flags',
        'if ($cycle_seen_down_tap == 0) || ($cycle_seen_squat == 0) || ($cycle_seen_allow_pass == 0) || ($cycle_squat_checks != 3) || ($cycle_squat_wait_1 != 3) || ($cycle_squat_wait_2 != 2) || ($cycle_squat_wait_3 != 1)',
        'set $failure = 171',
        'else',
        'if ($cycle_pass_post_calls != 1) || ($cycle_pass_post_ignore != $target_line) || ($cycle_pass_post_tap != 254) || ($cycle_pass_post_air_ready == 0)',
        'set $failure = 172',
        'else',
        'if ($cycle_pass_rejects < 1) || ($cycle_pass_reject_line != $target_line) || (($cycle_pass_reject_flags & 0x4000) == 0)',
        'set $failure = 173',
        'else',
        'set $pass_mask = $pass_mask | $target_bit',
        'set $side_pass_crossings = $side_pass_crossings + 1',
        'set $side_pass_rejects_total = $side_pass_rejects_total + $cycle_pass_rejects',
        'set $land_base = $mario_land_count',
        'set $phase = 18',
        'set $phase_frames = 0',
        'end',
        'end',
        'end',
        'end',
        'end',
        'if $phase_frames > 150',
        'set $failure = 174',
        'end',
        'end',
        # Phase 18: Pass must hand off to Fall with ignore_line_id=-1 before
        # the natural main-floor landing. Then reuse phase 10 for line 2.
        'if ($failure == 0) && ($phase_start == 18)',
        'if ($prev_status == 33) && ($mario->status_id == 26)',
        'set $cycle_pass_fall_transitions = $cycle_pass_fall_transitions + 1',
        'if ($mario->ga != 1) || ($mario->coll_data.ignore_line_id != -1)',
        'set $failure = 180',
        'else',
        'set $cycle_seen_pass_fall_cleanup = 1',
        'set $cleanup_mask = $cleanup_mask | $target_bit',
        'set $side_cleanups = $side_cleanups + 1',
        'end',
        'end',
        'if $mario_land_count > $land_base',
        'if ($cycle_seen_pass_fall_cleanup == 0) || ($cycle_pass_fall_transitions != 1) || ($last_land_line != 3) || (($last_land_flags & 0x4000) != 0)',
        'set $failure = 181',
        'else',
        'if ($mario->ga == 0) && ($mario->coll_data.floor_line_id == 3) && (($mario->coll_data.mask_stat & 0x800) != 0) && ($mario->coll_data.ignore_line_id == -1)',
        'set $main_floor_mask = $main_floor_mask | $target_bit',
        'set $side_main_landings = $side_main_landings + 1',
        'set $side_cycles_completed = $side_cycles_completed + 1',
        'if $target_line == 1',
        'set $target_line = 2',
        'set $target_bit = 4',
        'set $target_y = 904.0',
        'set $target_x_min = -1700.0',
        'set $target_x_max = -1100.0',
        'set $cycle_initialized = 0',
        'set $phase = 10',
        'else',
        'set $phase = 19',
        'end',
        'set $phase_frames = 0',
        'end',
        'end',
        'end',
        'if $phase_frames > 220',
        'set $failure = 182',
        'end',
        'end',
        'set $prev_foot = $foot',
        'set $prev_vy = $vy',
        'set $prev_status = $mario->status_id',
        'if ($failure == 0) && ($phase != 19)',
        'continue',
        'end',
        'end',
        'continue',
        # The frame breakpoint returns only on a hard failure or complete pass.
        ('set {{unsigned int}}0x{0:x8} = 0' -f $playbackPadsAddress)
    )
    # Decode the live O2R geometry with the backend's high-half/low-half rule.
    # These commands only read target memory into debugger convenience values.
    $gdbCommands += $liveLineGeometryGdbCommands
    $gdbCommands += @(
        'printf "PLATFORM_RESULT=%d,%u,%u,%u,%u\n", $failure, $phase, $total_frames, $start_time_passed, gSCManagerBattleState->time_passed',
        'printf "PLATFORM_ACTORS=%#x,%#x,%u,%u,%u,%u,%u,%u\n", $mario_gobj, $fox_gobj, $mario->fkind, $mario->pkind, $mario->player, $fox->fkind, $fox->pkind, $fox->level',
        'printf "PLATFORM_UP=%u,%u,%u,%u,%u,%u,%d,%d\n", $seen_knee_1, $seen_jump_air_1, $up_crossed, $up_cross_frame, $mario_asc_sweep_count, $mario_asc_accepts, (int)($up_prev_foot * 1000.0), (int)($up_foot * 1000.0)',
        'printf "PLATFORM_SWEEP=%#x,%u,%u,%u,%d,%#x,%u\n", $mario_asc_sweep_mask, $mario_asc_sweep_count, $mario_asc_sweep_misses, $mario_asc_accepts, $mario_asc_accept_line, $mario_reverse_hit_mask, $mario_reverse_hit_count',
        'printf "PLATFORM_LAND=%u,%d,%#x,%u,%u,%u,%d,%#x\n", $apex_1, $land_1_line, $land_1_flags, $land_1, $stable_1_max, $mario_land_count, $land_2_line, $land_2_flags',
        'printf "PLATFORM_JUMP=%u,%u,%u,%u,%u,%u\n", $seen_knee_2, $seen_jump_air_2, $jump_2_departed, $apex_2, $land_2, $stable_2_max',
        'printf "PLATFORM_PASS=%u,%u,%u,%u,%d,%d,%d,%u,%d,%u,%u,%u,%d,%#x,%u,%d,%d\n", $seen_down_tap, $seen_squat, $seen_allow_pass, $squat_checks, $squat_wait_1, $squat_wait_2, $squat_wait_3, $pass_post_calls, $pass_post_ignore, $pass_post_tap, $pass_post_air_ready, $mario_pass_rejects, $pass_reject_line, $pass_reject_flags, $down_crossed, (int)($down_prev_foot * 1000.0), (int)($down_foot * 1000.0)',
        'printf "PLATFORM_CLEANUP=%u,%u,%d,%d,%u,%d,%#x\n", $seen_pass_fall_cleanup, $pass_fall_transitions, $pass_fall_ignore, $pass_fall_ga, $main_land_after_pass, $main_land_after_pass_line, $main_land_after_pass_flags',
        'printf "PLATFORM_MASKS=%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", $approach_mask, $up_mask, $land_wait_mask, $jump_reland_mask, $pass_mask, $cleanup_mask, $main_floor_mask',
        'printf "PLATFORM_FLIGHT=%#x,%#x,%#x,%#x,%u,%d,%d,%d,%d,%u\n", $continued_ascent_mask, $strict_descent_mask, $downward_cross_mask, $flight_bit, $flight_rise_frames, (int)($last_land_prev_foot * 1000.0), (int)($last_land_current_foot * 1000.0), (int)($last_land_vy * 1000.0), $last_land_ga, $uses_live_mpprocess',
        'printf "PLATFORM_SIDE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u\n", $side_cycles_completed, $side_approaches, $side_up_crossings, $side_first_landings, $side_stable_waits, $side_second_jumps, $side_second_landings, $side_pass_crossings, $side_cleanups, $side_main_landings, $side_pass_rejects_total, (int)($line1_approach_delta * 1000.0), (int)($line2_approach_delta * 1000.0), $mario_land_count',
        'printf "PLATFORM_FINAL=%u,%u,%d,%#x,%#x,%d,%d,%d,%u,%u\n", $mario->status_id, $mario->ga, $mario->coll_data.floor_line_id, $mario->coll_data.floor_flags, $mario->coll_data.mask_stat, $mario->coll_data.ignore_line_id, (int)($foot * 1000.0), (int)($root_x * 1000.0), $mario->percent_damage, $mario->hitlag_tics',
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed',
        'printf "MP_TOPOLOGY=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x\n", gNdsCollisionRuntimeDiagnostics.topology_build_attempts, gNdsCollisionRuntimeDiagnostics.topology_build_successes, gNdsCollisionRuntimeDiagnostics.topology_rebuilds, gNdsCollisionRuntimeDiagnostics.topology_lines, gNdsCollisionRuntimeDiagnostics.topology_shared_directed, gNdsCollisionRuntimeDiagnostics.topology_orphan_endpoints, gNdsCollisionRuntimeDiagnostics.topology_reversed_lines, gNdsCollisionRuntimeDiagnostics.topology_ambiguous_endpoints, gNdsCollisionRuntimeDiagnostics.topology_invalid, gNdsCollisionRuntimeDiagnostics.topology_getter_invalid, gNdsCollisionRuntimeDiagnostics.topology_inactive, gNdsCollisionRuntimeDiagnostics.topology_hash',
        'printf "MP_LINE0=%u,%d,%d,%d,%d,%#x,%u\n", gMPCollisionVertexInfo->vertex_info[0].line_type, $mp_line0_first_x, $mp_line0_first_y, $mp_line0_last_x, $mp_line0_last_y, $mp_line0_first_flags, $mp_line0_vertex_count',
        'printf "MP_LINE1=%u,%d,%d,%d,%d,%#x,%u\n", gMPCollisionVertexInfo->vertex_info[1].line_type, $mp_line1_first_x, $mp_line1_first_y, $mp_line1_last_x, $mp_line1_last_y, $mp_line1_first_flags, $mp_line1_vertex_count',
        'printf "MP_LINE2=%u,%d,%d,%d,%d,%#x,%u\n", gMPCollisionVertexInfo->vertex_info[2].line_type, $mp_line2_first_x, $mp_line2_first_y, $mp_line2_last_x, $mp_line2_last_y, $mp_line2_first_flags, $mp_line2_vertex_count',
        'printf "MP_LINE3=%u,%d,%d,%d,%d,%#x,%u\n", gMPCollisionVertexInfo->vertex_info[3].line_type, $mp_line3_first_x, $mp_line3_first_y, $mp_line3_last_x, $mp_line3_last_y, $mp_line3_first_flags, $mp_line3_vertex_count',
        'printf "MP_FLOOR=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsCollisionRuntimeDiagnostics.floor_sweep_calls, gNdsCollisionRuntimeDiagnostics.floor_sweep_hits, gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_sweeps, gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_accepts, gNdsCollisionRuntimeDiagnostics.floor_reverse_endpoint_hits, gNdsCollisionRuntimeDiagnostics.floor_adj_calls, gNdsCollisionRuntimeDiagnostics.floor_adj_direct_hits, gNdsCollisionRuntimeDiagnostics.floor_adjacent_hits, gNdsCollisionRuntimeDiagnostics.floor_adj_pass_rejects - $pass_rejects_base, gNdsCollisionRuntimeDiagnostics.floor_adj_ambiguous',
        ('printf "INPUT=%u,%#x,%u,%u,%u,%#x\n", *(unsigned int*)0x{0:x8}, *(unsigned int*)0x{1:x8}, gNdsControllerPlaybackFrameCount, gNdsControllerPlaybackReadCount, gNdsControllerPlaybackReadCount - $start_reads, *(unsigned int*)0x{2:x8}' -f $playbackEnabledAddress, $playbackConnectedAddress, $playbackPadsAddress),
        'printf "MEMARENA=%#x,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom',
        # Capture both DS screens while this exact success/failure frame is
        # paused. The output is always kept under artifacts/visibility.
        $captureCommand,
        'detach',
        'quit'
    )

    $gdbResult = Invoke-GdbMarkerScript `
        -Gdb $Gdb `
        -Elf $elf `
        -Root $root `
        -Commands $gdbCommands `
        -ScriptName $scriptName `
        -TimeoutSeconds $TimeoutSeconds
    $gdbStdout = $gdbResult.Stdout

    $result = [regex]::Match($gdbStdout, 'PLATFORM_RESULT=(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $actors = [regex]::Match($gdbStdout, 'PLATFORM_ACTORS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $up = [regex]::Match($gdbStdout, 'PLATFORM_UP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $sweep = [regex]::Match($gdbStdout, 'PLATFORM_SWEEP=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $land = [regex]::Match($gdbStdout, 'PLATFORM_LAND=([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0)')
    $jump = [regex]::Match($gdbStdout, 'PLATFORM_JUMP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $pass = [regex]::Match($gdbStdout, 'PLATFORM_PASS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $cleanup = [regex]::Match($gdbStdout, 'PLATFORM_CLEANUP=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0)')
    $masks = [regex]::Match($gdbStdout, 'PLATFORM_MASKS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $flight = [regex]::Match($gdbStdout, 'PLATFORM_FLIGHT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $side = [regex]::Match($gdbStdout, 'PLATFORM_SIDE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $final = [regex]::Match($gdbStdout, 'PLATFORM_FINAL=([0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $topology = [regex]::Match($gdbStdout, 'MP_TOPOLOGY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $line0 = [regex]::Match($gdbStdout, 'MP_LINE0=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $line1 = [regex]::Match($gdbStdout, 'MP_LINE1=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $line2 = [regex]::Match($gdbStdout, 'MP_LINE2=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $line3 = [regex]::Match($gdbStdout, 'MP_LINE3=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $floor = [regex]::Match($gdbStdout, 'MP_FLOOR=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'INPUT=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $memory = [regex]::Match($gdbStdout, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')

    Assert-Condition $result.Success `
        'Platform-semantics state machine did not emit a result.' $gdbStdout
    Assert-Condition (Test-Path -LiteralPath $screenshotPath -PathType Leaf) `
        "Platform-semantics run did not produce its evidence screenshot: $screenshotPath" `
        $gdbStdout
    Assert-Condition ((Get-Item -LiteralPath $screenshotPath).Length -gt 1024) `
        "Platform-semantics evidence screenshot is unexpectedly small: $screenshotPath" `
        $gdbStdout
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $screenshotPath `
        -MinDifferentFraction 0.01 `
        -MinDominantGreenFraction 0.03 `
        -MinNonWhiteNonGreenFraction 0.25 `
        -WindowScaledCapture
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $rv = Get-Ints $result
    $av = Get-Ints $actors
    $uv = Get-Ints $up
    $sweepv = Get-Ints $sweep
    $lv = Get-Ints $land
    $jv = Get-Ints $jump
    $pv = Get-Ints $pass
    $cv = Get-Ints $cleanup
    $maskv = Get-Ints $masks
    $flightv = Get-Ints $flight
    $sidev = Get-Ints $side
    $fv = Get-Ints $final
    $hv = Get-Ints $harn
    $sv = Get-Ints $scene
    $tv = Get-Ints $topology
    $line0v = Get-Ints $line0
    $line1v = Get-Ints $line1
    $line2v = Get-Ints $line2
    $line3v = Get-Ints $line3
    $flv = Get-Ints $floor
    $iv = Get-Ints $input
    $mv = Get-Ints $memory

    $failureNames = @{
        10 = 'wrong harness result/mode'; 11 = 'wrong harness scene';
        12 = 'wrong live scene/stage'; 13 = 'timer not naturally running';
        14 = 'P0 is not human Mario'; 15 = 'P1 is not level-3 Fox CPU';
        16 = 'Mario did not begin Wait/Ground on main floor line 3';
        17 = 'Mario began outside center-platform span';
        18 = 'boot controller mapping self-test did not pass';
        20 = '1200-frame watchdog'; 21 = 'fighter GObj identity changed';
        22 = 'scene/timer ended';
        24 = 'Mario left center-platform span';
        25 = 'Mario target-line ascending sweep was accepted';
        26 = 'collision topology/adjacency became invalid or ambiguous';
        27 = 'Fox CPU interfered with Mario by damage, hitlag, or capture';
        28 = 'platform flight did not continue upward before strict Air descent';
        29 = 'landing did not cross the exact platform plane downward from Air';
        30 = 'upward crossing was grounded, clamped, or not a natural jump';
        31 = 'upward crossing did not exercise the flat-platform sweep';
        32 = 'first upward-crossing watchdog';
        40 = 'first landing selected the wrong floor';
        41 = 'first descending-landing watchdog';
        50 = 'first stable-platform dwell watchdog';
        60 = 'platform jump immediately relanded or retained floor mask';
        61 = 'platform-jump departure watchdog';
        70 = 'second landing selected the wrong floor';
        71 = 'second descending-landing watchdog';
        80 = 'second stable-platform dwell watchdog';
        90 = 'downward crossing retained floor or wrong Pass/Fall state';
        91 = 'down tap/Squat 3,2,1 sequence diverged';
        92 = 'BattleShip Pass initialization diverged';
        93 = 'Mario-specific same-line PASS rejection was not observed';
        94 = 'intentional down-pass watchdog';
        95 = 'Pass-to-Fall did not clear same-line ignore';
        96 = 'post-Pass descent did not land on the main floor';
        97 = 'post-Pass cleanup/landing watchdog';
        100 = 'side-platform approach left main-floor line 3';
        101 = 'side-platform approach did not cover natural floor distance';
        102 = 'side-platform natural-approach watchdog';
        103 = 'Mario left the safe side-platform/main-floor route span';
        110 = 'side-platform upward crossing was grounded or clamped';
        111 = 'side-platform upward crossing missed the flat-floor sweep';
        112 = 'side-platform upward-crossing watchdog';
        120 = 'side-platform first landing selected the wrong line';
        121 = 'side-platform first landing watchdog';
        130 = 'side-platform stable Wait watchdog';
        140 = 'side-platform second jump retained floor or relanded';
        141 = 'side-platform second-jump watchdog';
        150 = 'side-platform second landing selected the wrong line';
        151 = 'side-platform second-landing watchdog';
        160 = 'side-platform second stable Wait watchdog';
        170 = 'side-platform downward crossing retained floor or wrong state';
        171 = 'side-platform down tap/Squat 3,2,1 sequence diverged';
        172 = 'side-platform BattleShip Pass initialization diverged';
        173 = 'side-platform target-line PASS rejection was not observed';
        174 = 'side-platform intentional down-pass watchdog';
        180 = 'side-platform Pass-to-Fall did not clear same-line ignore';
        181 = 'side-platform post-Pass descent missed the main floor';
        182 = 'side-platform cleanup/main-floor watchdog'
    }
    $failureText = if ($failureNames.ContainsKey([int]$rv[0])) {
        $failureNames[[int]$rv[0]]
    } else {
        'unknown state-machine failure'
    }
    Assert-Condition ($rv[0] -eq 0 -and $rv[1] -eq 19 -and
        $rv[2] -le 1200 -and ($rv[4] - $rv[3]) -eq $rv[2]) `
        "Platform-semantics natural sequence failed at phase $($rv[1]) with code $($rv[0]) ($failureText)." `
        $gdbStdout
    Assert-Condition ($actors.Success -and $av[0] -ne 0 -and $av[1] -ne 0 -and
        $av[2] -eq 0 -and $av[3] -eq 0 -and $av[4] -eq 0 -and
        $av[5] -eq 1 -and $av[6] -eq 1 -and $av[7] -eq 3) `
        'The complete natural trace did not retain human Mario and level-3 Fox CPU.' `
        $gdbStdout
    Assert-Condition ($up.Success -and $uv[0] -eq 1 -and $uv[1] -eq 1 -and
        $uv[2] -eq 1 -and $uv[3] -gt 0 -and $uv[4] -eq 3 -and
        $uv[5] -eq 0 -and $uv[6] -lt 1541750 -and $uv[7] -gt 1542250) `
        'Mario did not naturally jump upward through line 0 without floor acceptance.' `
        $gdbStdout
    Assert-Condition ($sweep.Success -and $sweepv[0] -eq 0x7 -and
        $sweepv[1] -eq 3 -and $sweepv[2] -eq 0 -and
        $sweepv[3] -eq 0 -and $sweepv[4] -eq -1 -and
        $sweepv[5] -eq 0x7 -and $sweepv[6] -ge 6) `
        'Mario-specific target sweeps or reversed-endpoint floor hits were incomplete.' `
        $gdbStdout
    Assert-Condition ($land.Success -and $lv[0] -eq 1 -and $lv[1] -eq 0 -and
        (($lv[2] -band 0x4000) -ne 0) -and $lv[3] -eq 1 -and
        $lv[4] -ge 8 -and $lv[5] -ge 2 -and $lv[6] -eq 0 -and
        (($lv[7] -band 0x4000) -ne 0)) `
        'Mario did not descend onto line 0 twice and remain stably grounded.' `
        $gdbStdout
    Assert-Condition ($jump.Success -and $jv[0] -eq 1 -and $jv[1] -eq 1 -and
        $jv[2] -eq 1 -and $jv[3] -eq 1 -and $jv[4] -eq 1 -and
        $jv[5] -ge 4) `
        'Mario did not naturally jump from the platform and return to it.' `
        $gdbStdout
    Assert-Condition ($pass.Success -and $pv[0] -eq 1 -and $pv[1] -eq 1 -and
        $pv[2] -eq 1 -and $pv[3] -eq 3 -and $pv[4] -eq 3 -and
        $pv[5] -eq 2 -and $pv[6] -eq 1 -and $pv[7] -eq 1 -and
        $pv[8] -eq 0 -and $pv[9] -eq 254 -and $pv[10] -eq 1 -and
        $pv[11] -ge 1 -and $pv[12] -eq 0 -and
        (($pv[13] -band 0x4000) -ne 0) -and $pv[14] -eq 1 -and
        $pv[15] -ge 1541750 -and $pv[16] -lt 1541750) `
        'Mario did not execute BattleShip Wait/Squat/Pass semantics through line 0.' `
        $gdbStdout
    Assert-Condition ($cleanup.Success -and $cv[0] -eq 1 -and $cv[1] -eq 1 -and
        $cv[2] -eq -1 -and $cv[3] -eq 1 -and $cv[4] -eq 1 -and
        $cv[5] -eq 3 -and (($cv[6] -band 0x4000) -eq 0)) `
        'Pass did not hand off to Fall with ignore cleared before main-floor landing.' `
        $gdbStdout
    Assert-Condition ($masks.Success -and
        $maskv[0] -eq 0x7 -and $maskv[1] -eq 0x7 -and
        $maskv[2] -eq 0x7 -and $maskv[3] -eq 0x7 -and
        $maskv[4] -eq 0x7 -and $maskv[5] -eq 0x7 -and
        $maskv[6] -eq 0x7) `
        'All-three-platform semantic masks did not finish at exactly 0x7.' `
        $gdbStdout
    Assert-Condition ($flight.Success -and
        $flightv[0] -eq 0x3f -and $flightv[1] -eq 0x3f -and
        $flightv[2] -eq 0x3f -and $flightv[3] -eq 0x20 -and
        $flightv[4] -ge 2 -and $flightv[5] -gt 904000 -and
        $flightv[6] -le 904000 -and $flightv[6] -lt $flightv[5] -and
        $flightv[7] -lt 0 -and $flightv[8] -eq 1 -and
        $flightv[9] -eq [int]$usesLiveMPProcess) `
        'All six platform flights did not prove ascent, strict Air descent, and exact downward plane crossing.' `
        $gdbStdout
    Assert-Condition ($side.Success -and
        $sidev[0] -eq 2 -and $sidev[1] -eq 2 -and
        $sidev[2] -eq 2 -and $sidev[3] -eq 2 -and
        $sidev[4] -eq 2 -and $sidev[5] -eq 2 -and
        $sidev[6] -eq 2 -and $sidev[7] -eq 2 -and
        $sidev[8] -eq 2 -and $sidev[9] -eq 2 -and
        $sidev[10] -ge 2 -and $sidev[11] -ge 600000 -and
        $sidev[12] -le -600000 -and $sidev[13] -ge 9) `
        'Lines 1/2 did not complete two exact natural approach/platform/Pass cycles.' `
        $gdbStdout
    Assert-Condition ($final.Success -and $fv[1] -eq 0 -and
        $fv[2] -eq 3 -and (($fv[4] -band 0x800) -ne 0) -and
        $fv[5] -eq -1 -and $fv[8] -eq 0 -and
        $fv[9] -eq 0) `
        'Final post-Pass state was not clean Ground on the main floor.' `
        $gdbStdout
    Assert-Condition ($harn.Success -and $hv[0] -eq 0x4841524e -and
        $hv[1] -eq 163 -and $hv[2] -eq 22 -and $hv[3] -eq 21 -and
        $hv[4] -eq 0) `
        'Platform-semantics verifier did not use canonical mode 163.' $gdbStdout
    Assert-Condition ($scene.Success -and $sv[0] -eq 22 -and $sv[1] -eq 21 -and
        $sv[2] -eq 6 -and $sv[3] -eq 1 -and $sv[4] -eq 1 -and
        $sv[5] -gt 0 -and ($sv[5] + $sv[6]) -eq 3600 -and
        $sv[6] -eq $rv[4]) `
        'Platform-semantics sequence did not remain inside the original one-minute match.' `
        $gdbStdout
    Assert-Condition ($topology.Success -and $tv[0] -eq 1 -and $tv[1] -eq 1 -and
        $tv[2] -eq 0 -and $tv[3] -eq 7 -and $tv[4] -eq 8 -and
        $tv[5] -eq 6 -and $tv[6] -eq 5 -and $tv[7] -eq 0 -and
        $tv[8] -eq 0 -and $tv[9] -eq 0 -and $tv[10] -eq 0 -and
        $tv[11] -eq 3903148810) `
        'Live Pupupu topology diverged from BattleShip shared-vertex construction.' `
        $gdbStdout
    Assert-Condition ($line0.Success -and $line0v[0] -eq 0 -and
        $line0v[1] -eq 570 -and $line0v[2] -eq 1542 -and
        $line0v[3] -eq -570 -and $line0v[4] -eq 1542 -and
        $line0v[5] -eq 0x4000 -and $line0v[6] -eq 3) `
        'Live Pupupu line 0 geometry/kind/flags diverged from BattleShip.' `
        $gdbStdout
    Assert-Condition ($line1.Success -and $line1v[0] -eq 0 -and
        $line1v[1] -eq 1892 -and $line1v[2] -eq 907 -and
        $line1v[3] -eq 951 -and $line1v[4] -eq 907 -and
        $line1v[5] -eq 0x4000 -and $line1v[6] -eq 3) `
        'Live Pupupu line 1 geometry/kind/flags diverged from BattleShip.' `
        $gdbStdout
    Assert-Condition ($line2.Success -and $line2v[0] -eq 0 -and
        $line2v[1] -eq -951 -and $line2v[2] -eq 904 -and
        $line2v[3] -eq -1841 -and $line2v[4] -eq 904 -and
        $line2v[5] -eq 0x4000 -and $line2v[6] -eq 3) `
        'Live Pupupu line 2 geometry/kind/flags diverged from BattleShip.' `
        $gdbStdout
    Assert-Condition ($line3.Success -and $line3v[0] -eq 0 -and
        $line3v[1] -eq -2318 -and $line3v[2] -eq 0 -and
        $line3v[3] -eq 2318 -and $line3v[4] -eq 0 -and
        $line3v[5] -eq 0x8000 -and $line3v[6] -eq 2) `
        'Live Pupupu main-floor line geometry/kind/flags diverged from BattleShip.' `
        $gdbStdout
    $floorBackendOk = if ($usesLiveMPProcess) {
        $flv[5] -eq 0 -and $flv[6] -eq 0 -and $flv[7] -eq 0 -and
            $flv[8] -eq 0
    } else {
        $flv[5] -gt 0 -and $flv[6] -gt 0 -and $flv[8] -ge 1
    }
    Assert-Condition ($floor.Success -and $flv[0] -gt 0 -and $flv[1] -gt 0 -and
        $flv[2] -ge $sweepv[1] -and $flv[3] -eq 0 -and
        $flv[4] -ge $sweepv[6] -and $flv[9] -eq 0 -and $floorBackendOk) `
        'Floor diagnostics did not agree with upward pass, landings, and down-pass.' `
        $gdbStdout
    # The canonical realtime loop consumes the externally supplied pad state
    # twice per completed frame.  Pin that exact ratio instead of accepting a
    # global read counter that Fox or setup work could inflate.
    Assert-Condition ($input.Success -and $iv[0] -eq 1 -and
        (($iv[1] -band 3) -eq 3) -and $iv[2] -eq 0 -and
        $iv[3] -gt 0 -and $iv[4] -eq (2 * $rv[2]) -and
        $iv[3] -ge $iv[4] -and $iv[5] -eq 0) `
        'External controller playback was not consumed and released naturally.' `
        $gdbStdout
    Assert-Condition ($memory.Success -and $mv[0] -eq 0x4d4c4544 -and
        $mv[1] -eq 22 -and $mv[2] -ge 131072) `
        'Platform-semantics run violated the P1 arena reserve.' $gdbStdout

    Write-Output ((
        'battle_playable platform semantics passed: frames={0} masks=0x{1:x} ' +
        'flights=0x{2:x} upSweep={3} landings={4} sideCycles={5} passReject={6} reserve={7} screenshot={8}'
    ) -f
        $rv[2], $maskv[0], $flightv[2], $uv[4], $sidev[13], $sidev[0],
        ($pv[11] + $sidev[10]), $mv[2],
        $screenshotPath)
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
    Remove-Item $captureHelper, $stdout, $stderr -Force `
        -ErrorAction SilentlyContinue
}
