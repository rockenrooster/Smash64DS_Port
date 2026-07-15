[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [ValidateRange(30, 1800)]
    [int]$TimeoutSeconds = 300,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$target = 'smash64ds-battle-playable-canonical-hwtri'
$build = 'build-battle-playable-canonical-hwtri-harness'
$rom = Join-Path $root "$target.nds"
$elf = Join-Path $root "$target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$runnerSlotActive = Get-MelonDSActiveRunnerSlot
$logDir = Get-MelonDSVerifierLogDir `
    -Root $root -RunnerSlot $runnerSlotActive
$tempDir = Get-MelonDSVerifierTempDir `
    -Root $root -RunnerSlot $runnerSlotActive
$visibilityDir = Join-Path $root 'artifacts\visibility'
$captureStamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$captureRunTag = "${captureStamp}_slot${runnerSlotActive}_p${PID}"
$configState = $null

# BattleShip ifcommon.c:2996 advances horizontal pause orbit by
# stick_x * 0.000333 radians per paused source update. Eleven updates at the
# ordinary controller range of 80 produce 0.29304 rad, or 16.790 degrees.
# gmcamera.c:521-529 independently bounds the normal battle-camera horizontal
# source target to +/-17.5 degrees. These cases therefore distinguish a defect
# inside the normal source envelope from one confined to the optional pause
# orbit without writing fighter, camera, or RNG state.
$normalStickX = 80
$normalMaxDriveFrames = 300
$normalMinYawRatioMilli = 140 # tan(about 8 degrees) * 1000
$normalMinSourceYawMilliDegrees = 8000
$pausePreFrames = 45
$pauseStickX = 80
$expectedPauseYawDegrees = 16.790
$expectedPauseWideYawDegrees = 33.580

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context = '')
    if (-not $Condition) {
        if ([string]::IsNullOrWhiteSpace($Context)) { throw $Message }
        throw "$Message`n$Context"
    }
}

function Convert-MarkerField {
    param([string]$Value)

    if ($Value -match '^0x[0-9a-fA-F]+$') {
        return [int64][uint32]::Parse(
            $Value.Substring(2),
            [System.Globalization.NumberStyles]::HexNumber)
    }
    return [int64]$Value
}

function Get-MarkerRow {
    param(
        [string]$Text,
        [string]$Name,
        [int]$ExpectedFields
    )

    # GDB can append the first printf marker to its stop-location text instead
    # of starting a fresh line. Marker payloads contain no whitespace, so use
    # an identifier boundary and stop at whitespace rather than requiring ^.
    $matches = [regex]::Matches(
        $Text, ('(?<![A-Za-z0-9_])' + [regex]::Escape($Name) + '=(\S+)'))
    Assert-Condition ($matches.Count -eq 1) `
        "Expected exactly one $Name marker, found $($matches.Count)." $Text
    $row = [int64[]]@($matches[0].Groups[1].Value.Split(',') |
        ForEach-Object { Convert-MarkerField $_ })
    Assert-Condition ($row.Count -eq $ExpectedFields) `
        "$Name marker had $($row.Count) fields; expected $ExpectedFields." $Text
    return ,$row
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

function Get-ElfSymbolAddress {
    param([string]$Name)

    $escapedName = [regex]::Escape($Name)
    $line = $script:nmTable |
        Where-Object { $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escapedName$" } |
        Select-Object -First 1
    if (-not $line) { throw "ELF symbol not found: $Name" }
    $match = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($match.Groups[1].Value, 16))
}

function Resolve-VisibilityOutput {
    param([string]$LeafName)

    $resolved = [System.IO.Path]::GetFullPath(
        (Join-Path $visibilityDir $LeafName))
    $prefix = [System.IO.Path]::GetFullPath($visibilityDir).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    Assert-Condition ($resolved.StartsWith(
        $prefix, [System.StringComparison]::OrdinalIgnoreCase)) `
        "Camera-containment screenshots must stay under '$visibilityDir'."
    return $resolved
}

function Wait-ReadyFile {
    param(
        [string]$Path,
        [System.Diagnostics.Process]$GdbProcess,
        [System.Diagnostics.Process]$EmulatorProcess,
        [string]$Phase
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        $GdbProcess.Refresh()
        $EmulatorProcess.Refresh()
        if ($GdbProcess.HasExited) {
            throw "GDB exited before camera-containment $Phase synchronization."
        }
        if ($EmulatorProcess.HasExited) {
            throw "melonDS exited before camera-containment $Phase synchronization."
        }
        if ((Get-Date) -ge $deadline) {
            throw "Timed out waiting for camera-containment $Phase synchronization."
        }
        Start-Sleep -Milliseconds 25
    }
}

function Assert-ControllerOnlyTargetWrites {
    param(
        [string[]]$Commands,
        [uint32[]]$AllowedAddresses
    )

    $allowed = [System.Collections.Generic.HashSet[uint32]]::new()
    foreach ($address in $AllowedAddresses) {
        [void]$allowed.Add($address)
    }
    foreach ($command in $Commands) {
        if ($command -match '^\s*set\s+\{') {
            $addressMatch = [regex]::Match($command, '0x([0-9a-fA-F]+)')
            Assert-Condition $addressMatch.Success `
                "Unparseable target-memory write in GDB command: $command"
            $address = [uint32]([Convert]::ToUInt32(
                $addressMatch.Groups[1].Value, 16))
            Assert-Condition ($allowed.Contains($address)) `
                "Non-controller target-memory write rejected: $command"
        }
        Assert-Condition ($command -notmatch '^\s*set\s+variable\s+') `
            "Named inferior-state write rejected: $command"
    }
}

if ($null -eq ('Smash64DSCameraContainmentCapture' -as [type])) {
    Add-Type -AssemblyName System.Drawing
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSCameraContainmentCapture
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
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(
        IntPtr window, IntPtr insertAfter, int x, int y, int width, int height,
        uint flags);
}
'@
}

function Wait-MelonDSWindow {
    param([System.Diagnostics.Process]$Process)

    $deadline = (Get-Date).AddSeconds(20)
    do {
        Start-Sleep -Milliseconds 25
        $Process.Refresh()
    } while (($Process.MainWindowHandle -eq [IntPtr]::Zero) -and
        (-not $Process.HasExited) -and ((Get-Date) -lt $deadline))
    Assert-Condition ((-not $Process.HasExited) -and
        ($Process.MainWindowHandle -ne [IntPtr]::Zero)) `
        'melonDS did not expose a capturable window.'
    return $Process.MainWindowHandle
}

function Set-CaptureWindow {
    param([System.IntPtr]$Handle)

    [void][Smash64DSCameraContainmentCapture]::ShowWindow($Handle, 9)
    # The shared melonDS policy defines this exact natural vertical pair.
    [void][Smash64DSCameraContainmentCapture]::SetWindowPos(
        $Handle, [IntPtr](-1), 0, 0,
        $script:MelonDSCanonicalWindowWidth,
        $script:MelonDSCanonicalWindowHeight, 0x42)
    [void][Smash64DSCameraContainmentCapture]::SetForegroundWindow($Handle)
}

function Save-CaptureWindow {
    param([System.IntPtr]$Handle, [string]$Path)

    $rect = New-Object Smash64DSCameraContainmentCapture+Rect
    Assert-Condition `
        ([Smash64DSCameraContainmentCapture]::GetWindowRect(
            $Handle, [ref]$rect)) `
        'Could not read the synchronized melonDS window bounds.'
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    Assert-Condition ($width -gt 0 -and $height -gt 0) `
        "Invalid synchronized melonDS bounds ${width}x${height}."

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen(
            $rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Get-FrameAdvanceCommands {
    param([int]$Count, [string]$CounterName)

    return @(
        "set `$$CounterName = 0",
        "while (`$$CounterName < $Count)",
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        "set `$$CounterName = `$$CounterName + 1",
        'end'
    )
}

function Get-RenderBaselineCommands {
    return @(
        'set $base_profile_frame = gNdsRendererProfileFrameCount',
        'set $base_hw_frame = gNdsHardwareRendererSubmittedFrameCount',
        'set $base_stage_submit = gNdsStageGCDrawAllLoopHardwareSubmitCount',
        'set $base_stage_tri = gNdsStageGCDrawAllLoopHardwareTriangleCount',
        'set $base_fighter_submit = gNdsStageGCDrawAllLoopHardwareFighterSubmitCount',
        'set $base_fighter_tri = gNdsStageGCDrawAllLoopHardwareFighterTriangleCount',
        'set $base_display_selected = gNdsFighterDisplayContractSelectedCount',
        'set $base_display_submitted = gNdsFighterDisplayContractSubmittedCount',
        'set $base_bounds_fail = gNdsFighterDisplayContractBoundsFailCount',
        'set $base_stage_texture_reject = gNdsStageGCDrawAllLoopHardwareTextureRejectCount',
        'set $base_fighter_texture_reject = gNdsFighterDLAllDrawHardwareTextureRejectCount'
    )
}

function Get-ViewCalculationCommands {
    return @(
        'set $cobj = (CObj *)gGMCameraGObj->obj',
        'set $source_yaw_mdeg = (int)((-$cobj->vec.at.x * 1000.0) / 133.0)',
        'if ($source_yaw_mdeg > 17500)',
        'set $source_yaw_mdeg = 17500',
        'else',
        'if ($source_yaw_mdeg < -17500)',
        'set $source_yaw_mdeg = -17500',
        'end',
        'end',
        'set $actual_dx = $cobj->vec.eye.x - $cobj->vec.at.x',
        'set $actual_dz = $cobj->vec.eye.z - $cobj->vec.at.z',
        'set $actual_ratio_milli = 0',
        'if (($actual_dz > 0.001) || ($actual_dz < -0.001))',
        'set $actual_ratio_milli = (int)(($actual_dx * 1000.0) / $actual_dz)',
        'end'
    )
}

function New-CameraCaseCommands {
    param(
        [pscustomobject]$Case,
        [uint32]$PlaybackPadsAddress,
        [uint32]$PlaybackConnectedAddress,
        [uint32]$PlaybackEnabledAddress,
        [string]$SetupReady,
        [string]$SetupGo,
        [string]$CaptureReady,
        [string]$CaptureGo
    )

    $setupReadyGdb = $SetupReady.Replace('\', '/')
    $setupGoGdb = $SetupGo.Replace('\', '/')
    $captureReadyGdb = $CaptureReady.Replace('\', '/')
    $captureGoGdb = $CaptureGo.Replace('\', '/')
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 10',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        ("set `$case_id = {0}" -f $Case.Id),
        ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$setupReadyGdb' -Value ready; while (-not (Test-Path -LiteralPath '$setupGoGdb')) { Start-Sleep -Milliseconds 25 }`""),
        # ifcommon.c:3167-3179 is the original Wait-to-GO timer transition.
        # Every target-memory write below occurs only after game_status is GO.
        'tbreak ifcommon.c:3175',
        'continue',
        'set $input_reads_base = gNdsControllerPlaybackReadCount',
        'set $input_frames_base = gNdsControllerPlaybackFrameCount',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $PlaybackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($PlaybackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($PlaybackPadsAddress + 3)),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $PlaybackConnectedAddress),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $PlaybackEnabledAddress),
        'set $mario = (GObj *)0',
        'set $fox = (GObj *)0',
        'set $fighter_scan = (GObj *)gGCCommonLinks[3]',
        'set $fighter_count = 0',
        'while (($fighter_scan != 0) && ($fighter_count < 8))',
        'set $scan_fp = (FTStruct *)$fighter_scan->user_data.p',
        'if (($scan_fp != 0) && ($scan_fp->player == 0))',
        'set $mario = $fighter_scan',
        'end',
        'if (($scan_fp != 0) && ($scan_fp->player == 1))',
        'set $fox = $fighter_scan',
        'end',
        'set $fighter_scan = (GObj *)$fighter_scan->link_next',
        'set $fighter_count = $fighter_count + 1',
        'end',
        'set $mfp = (FTStruct *)$mario->user_data.p',
        'set $ffp = (FTStruct *)$fox->user_data.p',
        'set $mroot = (DObj *)$mario->obj',
        'set $froot = (DObj *)$fox->obj'
    )

    if ($Case.Kind -eq 'normal') {
        $commands += @(
            ('set {{signed char}}0x{0:x8} = {1}' -f
                ($PlaybackPadsAddress + 2), $normalStickX),
            'set $drive_frames = 0',
            'set $source_yaw_mdeg = 0',
            'set $actual_ratio_milli = 0',
            ("while ((`$drive_frames < $normalMaxDriveFrames) && " +
                "((((`$actual_ratio_milli > -$normalMinYawRatioMilli) && " +
                "(`$actual_ratio_milli < $normalMinYawRatioMilli))) || " +
                "(((`$source_yaw_mdeg > -$normalMinSourceYawMilliDegrees) && " +
                "(`$source_yaw_mdeg < $normalMinSourceYawMilliDegrees)))))")
        )
        $commands += Get-RenderBaselineCommands
        $commands += @(
            'tbreak ndsBattlePlayableFrameCompleteMarker',
            'continue',
            'set $drive_frames = $drive_frames + 1'
        )
        $commands += Get-ViewCalculationCommands
        $commands += 'end'
    } else {
        $commands += Get-FrameAdvanceCommands `
            -Count $pausePreFrames -CounterName 'pre_pause_frames'
        $commands += @(
            # START_BUTTON enters the original source pause path. It is tapped
            # for one controller frame, then released before orbit input.
            ('set {{unsigned short}}0x{0:x8} = 0x1000' -f
                $PlaybackPadsAddress),
            'tbreak ndsBattlePlayableFrameCompleteMarker',
            'continue',
            ('set {{unsigned short}}0x{0:x8} = 0' -f
                $PlaybackPadsAddress),
            ('set {{signed char}}0x{0:x8} = {1}' -f
                ($PlaybackPadsAddress + 2), $Case.OrbitStickX)
        )
        $commands += Get-FrameAdvanceCommands `
            -Count $Case.OrbitFrames -CounterName 'orbit_frames'
        $commands += @(
            ('set {{signed char}}0x{0:x8} = 0' -f
                ($PlaybackPadsAddress + 2))
        )
        $commands += Get-FrameAdvanceCommands `
            -Count ($Case.SettleFrames - 1) -CounterName 'settle_frames'
        $commands += Get-RenderBaselineCommands
        $commands += @(
            'tbreak ndsBattlePlayableFrameCompleteMarker',
            'continue',
            ("set `$drive_frames = {0}" -f $Case.OrbitFrames)
        )
        $commands += Get-ViewCalculationCommands
    }

    $stateMarker = ('printf "CAMERA_CONTAINMENT_STATE=%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%u,%u,%#x,%u,%u,%#x,%d,%d,%#x,%#x\n", $case_id, gNdsSceneHarnessResult, gNdsSceneHarnessMode, gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_limit, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, gGMCameraStruct.status_default, gGMCameraStruct.status_curr, gGMCameraStruct.status_prev, sIFCommonBattlePauseKindInterface, sIFCommonBattlePausePlayer, *(unsigned int *)0x{0:x8}, *(unsigned int *)0x{1:x8}, gNdsControllerPlaybackReadCount - $input_reads_base, gNdsControllerPlaybackFrameCount - $input_frames_base, gSYControllerDevices[0].button_hold, gSYControllerDevices[0].stick_range.x, gSYControllerDevices[0].stick_range.y, *(unsigned short *)0x{2:x8}, *(unsigned int *)sSYUtilsRandomSeedPtr' -f
        $PlaybackEnabledAddress, $PlaybackConnectedAddress,
        $PlaybackPadsAddress)
    $commands += @(
        $stateMarker,
        'printf "CAMERA_CONTAINMENT_VIEW=%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%d,%d,%#x,%#x\n", $case_id, gNdsRendererProfileFrameCount, $drive_frames, *(unsigned int *)&gGMCameraPauseCameraEyeX, *(unsigned int *)&gGMCameraPauseCameraEyeY, *(unsigned int *)&sIFCommonBattlePauseCameraEyeXOrigin, *(unsigned int *)&sIFCommonBattlePauseCameraEyeYOrigin, *(unsigned int *)&$cobj->vec.at.x, *(unsigned int *)&$cobj->vec.at.y, *(unsigned int *)&$cobj->vec.at.z, *(unsigned int *)&$cobj->vec.eye.x, *(unsigned int *)&$cobj->vec.eye.y, *(unsigned int *)&$cobj->vec.eye.z, $source_yaw_mdeg, $actual_ratio_milli, *(unsigned int *)&gGMCameraStruct.target_dist, *(unsigned int *)&gGMCameraStruct.fovy',
        'printf "CAMERA_CONTAINMENT_FIGHTERS=%u,%u,%d,%d,%d,%d,%u,%d,%d,%#x,%#x,%#x,%u,%d,%d,%u,%d,%d,%u,%d,%d,%#x,%#x,%#x,%u\n", $case_id, $mfp->player, $mfp->fkind, $mfp->pkind, $mfp->status_id, $mfp->motion_id, $mfp->status_total_tics, $mfp->percent_damage, $mfp->stock_count, *(unsigned int *)&$mroot->translate.vec.f.x, *(unsigned int *)&$mroot->translate.vec.f.y, *(unsigned int *)&$mroot->translate.vec.f.z, $ffp->player, $ffp->fkind, $ffp->pkind, $ffp->level, $ffp->status_id, $ffp->motion_id, $ffp->status_total_tics, $ffp->percent_damage, $ffp->stock_count, *(unsigned int *)&$froot->translate.vec.f.x, *(unsigned int *)&$froot->translate.vec.f.y, *(unsigned int *)&$froot->translate.vec.f.z, $fighter_count',
        'printf "CAMERA_CONTAINMENT_RENDER=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", $case_id, gNdsRendererProfileFrameCount - $base_profile_frame, gNdsHardwareRendererSubmittedFrameCount - $base_hw_frame, gNdsStageGCDrawAllLoopHardwareSubmitCount - $base_stage_submit, gNdsStageGCDrawAllLoopHardwareTriangleCount - $base_stage_tri, gNdsStageGCDrawAllLoopHardwareFighterSubmitCount - $base_fighter_submit, gNdsStageGCDrawAllLoopHardwareFighterTriangleCount - $base_fighter_tri, gNdsFighterDisplayContractSelectedCount - $base_display_selected, gNdsFighterDisplayContractSubmittedCount - $base_display_submitted, gNdsFighterDisplayContractBoundsFailCount - $base_bounds_fail, gNdsStageGCDrawAllLoopHardwareTextureRejectCount - $base_stage_texture_reject, gNdsFighterDLAllDrawHardwareTextureRejectCount - $base_fighter_texture_reject, gNdsRendererProfileLevel, gNdsRendererProfileHardwareTriangles, gNdsRendererProfileHardwareVertices, gNdsRendererProfileHardwareOverLimit, gNdsRendererProfileHWVertexSaturateCount, gNdsRendererProfileRawCrossMatrixCount, gNdsRendererProfileProjectedSubmitFallbackCount, gNdsHardwareRendererPolyRamCount, gNdsHardwareRendererVertexRamCount, gNdsMemoryLedgerArenaHeadroom, gNdsHardwareRendererStatus, gNdsHardwareRendererControl',
        ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$captureReadyGdb' -Value ready; while (-not (Test-Path -LiteralPath '$captureGoGdb')) { Start-Sleep -Milliseconds 25 }`""),
        ('set {{unsigned short}}0x{0:x8} = 0' -f $PlaybackPadsAddress),
        ('set {{signed char}}0x{0:x8} = 0' -f ($PlaybackPadsAddress + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($PlaybackPadsAddress + 3)),
        ('set {{unsigned int}}0x{0:x8} = 0' -f $PlaybackEnabledAddress),
        'detach',
        'quit'
    )

    Assert-ControllerOnlyTargetWrites -Commands $commands -AllowedAddresses @(
        $PlaybackPadsAddress,
        ($PlaybackPadsAddress + 2),
        ($PlaybackPadsAddress + 3),
        $PlaybackConnectedAddress,
        $PlaybackEnabledAddress)
    return $commands
}

function Assert-CameraCase {
    param([pscustomobject]$Result)

    $case = $Result.Case
    $state = $Result.State
    $view = $Result.View
    $fighters = $Result.Fighters
    $render = $Result.Render
    $context = $Result.GdbText

    Assert-Condition ($state[0] -eq $case.Id -and
        $view[0] -eq $case.Id -and $fighters[0] -eq $case.Id -and
        $render[0] -eq $case.Id) `
        "Camera-containment case identity drifted for $($case.Name)." $context
    Assert-Condition ($state[1] -eq 0x4841524e -and $state[2] -eq 163 -and
        $state[3] -eq 22 -and $state[4] -eq 21 -and $state[5] -eq 6) `
        "$($case.Name) did not use registered canonical battle_playable mode 163." $context
    Assert-Condition ($state[8] -eq 1 -and $state[9] -gt 0 -and
        $state[10] -ge 0 -and ($state[9] + $state[10]) -eq 3600) `
        "$($case.Name) lost the original one-minute source timer state." $context
    # Direct verifier writes are consumed by the read backend but do not call
    # ndsControllerPlaybackCommitFrame; read delta is the canonical evidence.
    Assert-Condition ($state[16] -eq 1 -and (($state[17] -band 1) -eq 1) -and
        $state[18] -gt 0) `
        "$($case.Name) did not consume controller playback after GO." $context
    Assert-Condition ($fighters[1] -eq 0 -and $fighters[2] -eq 0 -and
        $fighters[3] -eq 0 -and $fighters[12] -eq 1 -and
        $fighters[13] -eq 1 -and $fighters[14] -eq 1 -and
        $fighters[15] -eq 3 -and $fighters[24] -eq 2) `
        "$($case.Name) did not retain human Mario versus level-3 CPU Fox." $context

    Assert-Condition ($render[1] -eq 1 -and $render[2] -eq 1 -and
        $render[3] -ge 42 -and $render[4] -ge 202 -and
        ($render[4] - 202) -eq (2 * ($render[3] - 42)) -and
        $render[5] -ge 1 -and $render[6] -gt 0 -and
        $render[7] -gt 0 -and $render[8] -gt 0 -and
        $render[9] -le 1 -and $render[10] -eq 0 -and
        $render[11] -eq 0) `
        "$($case.Name) did not preserve one complete source stage/fighter hardware traversal." $context
    Assert-Condition ($render[12] -eq 0 -and
        $render[13] -eq ($render[4] + $render[6]) -and
        $render[14] -gt 0 -and $render[15] -eq 0 -and
        $render[16] -eq 0 -and $render[19] -gt 0 -and
        $render[20] -gt 0 -and $render[21] -ge 131072) `
        "$($case.Name) lost shipped-profile hardware geometry or the P1 reserve floor." $context
    Assert-Condition ((Test-Path -LiteralPath $Result.Image -PathType Leaf)) `
        "$($case.Name) completed without its synchronized screenshot."

    $pauseYaw = [double](Convert-UInt32BitsToSingle $view[3]) *
        180.0 / [Math]::PI
    $pausePitch = [double](Convert-UInt32BitsToSingle $view[4]) *
        180.0 / [Math]::PI
    $atX = [double](Convert-UInt32BitsToSingle $view[7])
    $atY = [double](Convert-UInt32BitsToSingle $view[8])
    $atZ = [double](Convert-UInt32BitsToSingle $view[9])
    $eyeX = [double](Convert-UInt32BitsToSingle $view[10])
    $eyeY = [double](Convert-UInt32BitsToSingle $view[11])
    $eyeZ = [double](Convert-UInt32BitsToSingle $view[12])
    foreach ($number in @($pauseYaw, $pausePitch, $atX, $atY, $atZ,
            $eyeX, $eyeY, $eyeZ)) {
        Assert-Condition (Test-FiniteSingle ([single]$number)) `
            "$($case.Name) published a non-finite camera component." $context
    }
    $actualYaw = [Math]::Atan2($eyeX - $atX, $eyeZ - $atZ) *
        180.0 / [Math]::PI
    $sourceYaw = [double]$view[13] / 1000.0
    Assert-Condition ([Math]::Abs($sourceYaw) -le 17.501) `
        "$($case.Name) escaped BattleShip's +/-17.5-degree normal-camera source envelope." $context

    if ($case.Kind -eq 'normal') {
        Assert-Condition ($render[5] -eq 2 -and $render[6] -eq 626 -and
            $render[9] -eq 0) `
            'The normal source-envelope frame did not submit both complete fighters.' $context
        Assert-Condition ($state[6] -eq 1 -and $state[7] -eq 1 -and
            $state[12] -eq 0) `
            'The normal source-envelope sample was not unpaused GO/default camera.' $context
        Assert-Condition ($view[2] -gt 0 -and
            $view[2] -lt $normalMaxDriveFrames -and
            [Math]::Abs($sourceYaw) -ge 7.999 -and
            [Math]::Abs($actualYaw) -ge 7.0 -and
            [Math]::Abs($actualYaw) -le 19.0 -and
            ([Math]::Sign($sourceYaw) -eq [Math]::Sign($actualYaw))) `
            'Controller motion did not reach a rendered off-axis view inside the normal source envelope.' $context
        Assert-Condition ($state[21] -eq $normalStickX -and
            $state[22] -eq 0) `
            'The normal source-envelope frame was not driven only by the declared horizontal stick input.' $context
    } else {
        Assert-Condition ($state[6] -eq 2 -and $state[7] -eq 0 -and
            $state[12] -eq 1 -and $state[14] -eq 1 -and
            $state[15] -eq 0 -and $view[5] -eq 0 -and
            $view[6] -eq 0) `
            "$($case.Name) was not the original player-0 pause/player-zoom path." $context
        Assert-Condition ($state[20] -eq 0 -and $state[21] -eq 0 -and
            $state[22] -eq 0 -and $state[23] -eq 0 -and
            [Math]::Abs($pausePitch) -le 0.05) `
            "$($case.Name) was not captured after neutral controller release." $context
        if ($case.ExpectedYawDegrees -eq 0.0) {
            Assert-Condition ([Math]::Abs($pauseYaw) -le 0.05 -and
                [Math]::Abs($actualYaw) -le 2.0) `
                'Pause-front did not converge on the source straight-on view.' $context
        } else {
            $actualYawTolerance = if (
                [Math]::Abs($case.ExpectedYawDegrees) -gt 20.0
            ) { 5.0 } else { 3.0 }
            Assert-Condition ([Math]::Abs(
                $pauseYaw - $case.ExpectedYawDegrees) -le 0.15 -and
                [Math]::Abs(
                    $actualYaw - $case.ExpectedYawDegrees
                ) -le $actualYawTolerance) `
                "$($case.Name) did not converge on its controller-derived fixed angle." $context
        }
    }

    $Result | Add-Member -NotePropertyName ActualYawDegrees `
        -NotePropertyValue $actualYaw
    $Result | Add-Member -NotePropertyName SourceYawDegrees `
        -NotePropertyValue $sourceYaw
    $Result | Add-Member -NotePropertyName PauseYawDegrees `
        -NotePropertyValue $pauseYaw
}

function Invoke-CameraCase {
    param(
        [pscustomobject]$Case,
        [uint32]$PlaybackPadsAddress,
        [uint32]$PlaybackConnectedAddress,
        [uint32]$PlaybackEnabledAddress
    )

    $token = 'camera-{0}-p{1}-{2}' -f
        $Case.Name, $PID, ([System.Guid]::NewGuid().ToString('N'))
    $gdbScript = Join-Path $tempDir "$token.gdb"
    $gdbStdout = Join-Path $tempDir "$token.gdb.stdout.log"
    $gdbStderr = Join-Path $tempDir "$token.gdb.stderr.log"
    $setupReady = Join-Path $tempDir "$token.setup.ready"
    $setupGo = Join-Path $tempDir "$token.setup.go"
    $captureReady = Join-Path $tempDir "$token.capture.ready"
    $captureGo = Join-Path $tempDir "$token.capture.go"
    $emuStdout = Join-Path $logDir "melonds.$token.stdout.log"
    $emuStderr = Join-Path $logDir "melonds.$token.stderr.log"
    $image = Resolve-VisibilityOutput (
        "${captureRunTag}_mode163_camera_$($Case.Name).png")
    $temporary = @(
        $gdbScript, $gdbStdout, $gdbStderr,
        $setupReady, $setupGo, $captureReady, $captureGo,
        $emuStdout, $emuStderr)
    Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue

    $commands = New-CameraCaseCommands `
        -Case $Case `
        -PlaybackPadsAddress $PlaybackPadsAddress `
        -PlaybackConnectedAddress $PlaybackConnectedAddress `
        -PlaybackEnabledAddress $PlaybackEnabledAddress `
        -SetupReady $setupReady `
        -SetupGo $setupGo `
        -CaptureReady $captureReady `
        -CaptureGo $captureGo
    Set-Content -LiteralPath $gdbScript -Value ($commands -join "`n")

    $emulator = $null
    $gdbProcess = $null
    $gdbText = ''
    try {
        $emulator = Start-Process -FilePath $melonDsPath `
            -ArgumentList $rom `
            -WorkingDirectory $melonDsDir `
            -RedirectStandardOutput $emuStdout `
            -RedirectStandardError $emuStderr `
            -PassThru
        Wait-MelonDSGdbListener -Process $emulator `
            -Port $verifierContext.GdbPort | Out-Null
        $gdbProcess = Start-Process -FilePath $Gdb `
            -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
            -RedirectStandardOutput $gdbStdout `
            -RedirectStandardError $gdbStderr `
            -WindowStyle Hidden `
            -PassThru

        Wait-ReadyFile -Path $setupReady -GdbProcess $gdbProcess `
            -EmulatorProcess $emulator -Phase "$($Case.Name) window setup"
        $window = Wait-MelonDSWindow -Process $emulator
        Set-CaptureWindow -Handle $window
        Start-Sleep -Milliseconds 100
        Set-Content -LiteralPath $setupGo -Value go

        Wait-ReadyFile -Path $captureReady -GdbProcess $gdbProcess `
            -EmulatorProcess $emulator -Phase "$($Case.Name) completed frame"
        Save-CaptureWindow -Handle $window -Path $image
        Set-Content -LiteralPath $captureGo -Value go

        Assert-Condition ($gdbProcess.WaitForExit($TimeoutSeconds * 1000)) `
            "Timed out waiting for $($Case.Name) GDB completion."
        $gdbProcess.WaitForExit()
        $gdbText = ((Get-Content $gdbStdout, $gdbStderr -Raw `
            -ErrorAction SilentlyContinue) -join "`n").Trim()
        Assert-Condition ($gdbProcess.ExitCode -eq 0) `
            "$($Case.Name) GDB failed with exit $($gdbProcess.ExitCode)." `
            $gdbText

        $result = [pscustomobject]@{
            Case = $Case
            Image = $image
            State = Get-MarkerRow -Text $gdbText `
                -Name 'CAMERA_CONTAINMENT_STATE' -ExpectedFields 25
            View = Get-MarkerRow -Text $gdbText `
                -Name 'CAMERA_CONTAINMENT_VIEW' -ExpectedFields 17
            Fighters = Get-MarkerRow -Text $gdbText `
                -Name 'CAMERA_CONTAINMENT_FIGHTERS' -ExpectedFields 25
            Render = Get-MarkerRow -Text $gdbText `
                -Name 'CAMERA_CONTAINMENT_RENDER' -ExpectedFields 24
            GdbText = $gdbText
        }
        Assert-CameraCase -Result $result
        return $result
    } finally {
        Set-Content -LiteralPath $setupGo -Value go `
            -ErrorAction SilentlyContinue
        Set-Content -LiteralPath $captureGo -Value go `
            -ErrorAction SilentlyContinue
        if ($null -ne $gdbProcess) {
            $gdbProcess.Refresh()
            if (-not $gdbProcess.HasExited) {
                if (-not $gdbProcess.WaitForExit(2000)) {
                    Stop-Process -Id $gdbProcess.Id -Force
                    $gdbProcess.WaitForExit()
                }
            }
        }
        if ($null -ne $emulator) {
            $emulator.Refresh()
            if (-not $emulator.HasExited) {
                Stop-Process -Id $emulator.Id -Force
                $emulator.WaitForExit()
            }
        }
        Remove-Item -LiteralPath $temporary -Force `
            -ErrorAction SilentlyContinue
    }
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
    'Canonical battle_playable build did not produce the expected ROM and ELF.'
Assert-Condition (Test-Path -LiteralPath $melonDsPath -PathType Leaf) `
    "melonDS executable not found: $melonDsPath"
Assert-Condition (Test-Path -LiteralPath $Gdb -PathType Leaf) `
    "GDB executable not found: $Gdb"
Assert-Condition (Test-Path -LiteralPath $nm -PathType Leaf) `
    "ELF symbol tool not found: $nm"

$script:nmTable = @(& $nm -a $elf)
if ($LASTEXITCODE -ne 0) { throw 'Could not read canonical ELF symbols.' }
$requiredSymbols = @(
    'sControllerPlaybackPads',
    'sControllerPlaybackConnectedMask',
    'sControllerPlaybackEnabled',
    'gNdsControllerPlaybackFrameCount',
    'gNdsControllerPlaybackReadCount',
    'ndsBattlePlayableFrameCompleteMarker',
    'ifCommonBattleUpdateInterfaceAll',
    'gGCCommonLinks',
    'gSYControllerDevices',
    'gSCManagerSceneData',
    'gSCManagerBattleState',
    'sIFCommonTimerIsStarted',
    'gNdsSceneHarnessResult',
    'gNdsSceneHarnessMode',
    'gGMCameraGObj',
    'gGMCameraStruct',
    'gGMCameraPauseCameraEyeX',
    'gGMCameraPauseCameraEyeY',
    'sIFCommonBattlePauseCameraEyeXOrigin',
    'sIFCommonBattlePauseCameraEyeYOrigin',
    'sIFCommonBattlePauseKindInterface',
    'sIFCommonBattlePausePlayer',
    'sSYUtilsRandomSeedPtr',
    'gNdsRendererProfileFrameCount',
    'gNdsRendererProfileLevel',
    'gNdsRendererProfileHardwareTriangles',
    'gNdsRendererProfileHardwareVertices',
    'gNdsRendererProfileHardwareOverLimit',
    'gNdsRendererProfileHWVertexSaturateCount',
    'gNdsRendererProfileRawCrossMatrixCount',
    'gNdsRendererProfileProjectedSubmitFallbackCount',
    'gNdsHardwareRendererSubmittedFrameCount',
    'gNdsHardwareRendererPolyRamCount',
    'gNdsHardwareRendererVertexRamCount',
    'gNdsHardwareRendererStatus',
    'gNdsHardwareRendererControl',
    'gNdsStageGCDrawAllLoopHardwareSubmitCount',
    'gNdsStageGCDrawAllLoopHardwareTriangleCount',
    'gNdsStageGCDrawAllLoopHardwareFighterSubmitCount',
    'gNdsStageGCDrawAllLoopHardwareFighterTriangleCount',
    'gNdsStageGCDrawAllLoopHardwareTextureRejectCount',
    'gNdsFighterDisplayContractSelectedCount',
    'gNdsFighterDisplayContractSubmittedCount',
    'gNdsFighterDisplayContractBoundsFailCount',
    'gNdsFighterDLAllDrawHardwareTextureRejectCount',
    'gNdsMemoryLedgerArenaHeadroom'
)
foreach ($symbol in $requiredSymbols) {
    [void](Get-ElfSymbolAddress $symbol)
}
$playbackPadsAddress = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$playbackConnectedAddress =
    Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$playbackEnabledAddress = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'

New-Item -ItemType Directory -Force -Path @(
    $logDir, $tempDir, $visibilityDir) | Out-Null
$cases = @(
    [pscustomobject]@{
        Id = 0
        Name = 'normal_source_envelope'
        Kind = 'normal'
        OrbitStickX = 0
        ExpectedYawDegrees = 0.0
    },
    [pscustomobject]@{
        Id = 1
        Name = 'pause_front'
        Kind = 'pause'
        OrbitStickX = 0
        OrbitFrames = 11
        SettleFrames = 30
        ExpectedYawDegrees = 0.0
    },
    [pscustomobject]@{
        Id = 2
        Name = 'pause_plus16p8'
        Kind = 'pause'
        OrbitStickX = $pauseStickX
        OrbitFrames = 11
        SettleFrames = 30
        ExpectedYawDegrees = $expectedPauseYawDegrees
    },
    [pscustomobject]@{
        Id = 3
        Name = 'pause_minus16p8'
        Kind = 'pause'
        OrbitStickX = -$pauseStickX
        OrbitFrames = 11
        SettleFrames = 30
        ExpectedYawDegrees = -$expectedPauseYawDegrees
    },
    [pscustomobject]@{
        Id = 4
        Name = 'pause_plus33p6'
        Kind = 'pause'
        OrbitStickX = $pauseStickX
        OrbitFrames = 22
        SettleFrames = 19
        ExpectedYawDegrees = $expectedPauseWideYawDegrees
    },
    [pscustomobject]@{
        Id = 5
        Name = 'pause_minus33p6'
        Kind = 'pause'
        OrbitStickX = -$pauseStickX
        OrbitFrames = 22
        SettleFrames = 19
        ExpectedYawDegrees = -$expectedPauseWideYawDegrees
    }
)

$results = @()
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath `
        -GdbPort $verifierContext.GdbPort `
        -Persistent:([bool]$verifierContext.PersistentConfig)
    foreach ($case in $cases) {
        $results += Invoke-CameraCase `
            -Case $case `
            -PlaybackPadsAddress $playbackPadsAddress `
            -PlaybackConnectedAddress $playbackConnectedAddress `
            -PlaybackEnabledAddress $playbackEnabledAddress
    }

    $pauseResults = @($results | Where-Object { $_.Case.Kind -eq 'pause' })
    Assert-Condition ($pauseResults.Count -eq 5) `
        'Camera containment did not capture all five synchronized pause cases.'
    $front = $pauseResults | Where-Object { $_.Case.Name -eq 'pause_front' }
    $plus = $pauseResults | Where-Object { $_.Case.Name -eq 'pause_plus16p8' }
    $minus = $pauseResults | Where-Object { $_.Case.Name -eq 'pause_minus16p8' }
    $widePlus = $pauseResults | Where-Object { $_.Case.Name -eq 'pause_plus33p6' }
    $wideMinus = $pauseResults | Where-Object { $_.Case.Name -eq 'pause_minus33p6' }
    $orbitSamples = @($plus, $minus, $widePlus, $wideMinus)

    foreach ($sample in $orbitSamples) {
        Assert-Condition ($sample.View[1] -eq $front.View[1] -and
            $sample.State[9] -eq $front.State[9] -and
            $sample.State[10] -eq $front.State[10] -and
            $sample.State[24] -eq $front.State[24]) `
            "$($sample.Case.Name) did not share pause-front's frame/timer/RNG window." `
            $sample.GdbText
        Assert-Condition ((@($sample.Fighters[1..24]) -join ',') -eq
            (@($front.Fighters[1..24]) -join ',')) `
            "$($sample.Case.Name) changed fighter semantics during the fixed-angle A/B." `
            $sample.GdbText
        Assert-Condition ($sample.View[7] -eq $front.View[7] -and
            $sample.View[8] -eq $front.View[8] -and
            $sample.View[9] -eq $front.View[9] -and
            $sample.View[5] -eq $front.View[5] -and
            $sample.View[6] -eq $front.View[6] -and
            $sample.View[15] -eq $front.View[15] -and
            $sample.View[16] -eq $front.View[16]) `
            "$($sample.Case.Name) changed pause target/FOV semantics rather than only orbit." `
            $sample.GdbText
        $semanticRenderFields = @(1..18) + @(21..23)
        Assert-Condition ((@($sample.Render[$semanticRenderFields]) -join ',') -eq
            (@($front.Render[$semanticRenderFields]) -join ',')) `
            "$($sample.Case.Name) changed the synchronized renderer census." `
            $sample.GdbText
    }
    Assert-Condition ([Math]::Abs(
        ($plus.ActualYawDegrees - $front.ActualYawDegrees) -
        $expectedPauseYawDegrees) -le 1.5 -and [Math]::Abs(
        ($minus.ActualYawDegrees - $front.ActualYawDegrees) +
        $expectedPauseYawDegrees) -le 1.5) `
        'Pause orbit did not move +/-16.8 degrees relative to pause-front.'
    Assert-Condition ([Math]::Abs(
        ($plus.ActualYawDegrees - $front.ActualYawDegrees) +
        ($minus.ActualYawDegrees - $front.ActualYawDegrees)) -le 1.0) `
        'Pause +/-16.8-degree camera views were not symmetric.'
    Assert-Condition ([Math]::Abs(
        ($widePlus.ActualYawDegrees - $front.ActualYawDegrees) -
        $expectedPauseWideYawDegrees) -le 3.0 -and [Math]::Abs(
        ($wideMinus.ActualYawDegrees - $front.ActualYawDegrees) +
        $expectedPauseWideYawDegrees) -le 3.0) `
        'Pause orbit did not move +/-33.6 degrees relative to pause-front.'
    Assert-Condition ([Math]::Abs(
        ($widePlus.ActualYawDegrees - $front.ActualYawDegrees) +
        ($wideMinus.ActualYawDegrees - $front.ActualYawDegrees)) -le 1.5) `
        'Pause +/-33.6-degree camera views were not symmetric.'

    foreach ($result in $results) {
        & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
            -Image $result.Image `
            -MinDifferentFraction 0.01 `
            -MinDominantGreenFraction 0.05 `
            -MinNonWhiteNonGreenFraction 0.01 `
            -MaxSingleColorFraction 0.10 `
            -WindowScaledCapture
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($sample in $orbitSamples) {
        & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
            -Image $sample.Image `
            -CompareImage $front.Image `
            -MinDifferentFraction 0.01 `
            -MinCompareChangedFraction 0.01 `
            -CompareChannelThreshold 20 `
            -WindowScaledCapture
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    $normal = $results | Where-Object {
        $_.Case.Name -eq 'normal_source_envelope'
    }
    Write-Output (
        'battle_playable camera containment passed: ' +
        ('normal={0:N2}deg source={1:N2}deg; ' -f
            $normal.ActualYawDegrees, $normal.SourceYawDegrees) +
        ('pause=front/{0:N2}/{1:N2}/{2:N2}/{3:N2}deg; screenshots={4}' -f
            $plus.ActualYawDegrees, $minus.ActualYawDegrees,
            $widePlus.ActualYawDegrees, $wideMinus.ActualYawDegrees,
            (($results | ForEach-Object { $_.Image }) -join ', ')))
} finally {
    Restore-MelonDSGdbConfig -State $configState
}
