param(
    [switch]$Build,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Rom = (Join-Path $PSScriptRoot '..\smash64ds.nds'),
    [string]$Output,
    [int]$DelaySeconds = 32,
    [switch]$Unthrottled,
    [switch]$OpenGL4x,
    [switch]$SoftwareRenderer,
    [switch]$Jit,
    [switch]$NoJit,
    [switch]$MaximizeVertical,
    [ValidateRange(-1,8)][int]$RendererFastRunMode = -1,
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [string]$Elf = '',
    [string]$SecondOutput,
    [int]$SecondDelaySeconds = 1,
    [int]$SecondDelayMilliseconds = 0,
    [ValidateRange(-1,1000000)][int]$ExactFirstFrame = -1,
    [ValidateRange(-1,1000000)][int]$ExactSecondFrame = -1,
    # Lock the exact capture on the simulation clock rather than the presented
    # frame counter, capturing at `gSCManagerBattleState->time_remain ==
    # $ExactTimeRemain` and again one tic later. Use this, NOT
    # -ExactFirstFrame/-ExactSecondFrame, whenever the two captures come from
    # DIFFERENT BUILDS: the presented-frame counter drifts against the match
    # clock in proportion to how much faster one arm is, so the faster the
    # candidate the more of the delta is the lock rather than the change.
    # Full rationale on the parameter in capture-cut-g-exact-frames.ps1.
    [ValidateRange(0,3600)][int]$ExactTimeRemain = 0,
    [ValidateRange(-1,1)][int]$FoxCpuMode = -1,
    [ValidateSet('','Fox','Mario','Natural')][string]$FighterAnimAudit = '',
    [ValidateRange(0,218)][int]$FighterAnimStartMotion = 0,
    # BUGS.md #10. gmCameraGetAdjustAtAngle adds these two source globals into
    # the camera pitch and yaw, and nothing rewrites them after camera init, so
    # writing them mid-match orbits the real camera with no ROM change. This is
    # how a bug that only shows from under a fighter becomes reachable from the
    # automated capture instead of costing a manual play test per candidate fix.
    # They only mean anything in the paused player-zoom camera, so pair them
    # with -InGamePause.
    [double]$PauseCameraPitch = 0.0,
    [double]$PauseCameraYaw = 0.0,
    # Presses START (the in-game pause), which switches the camera to
    # player-zoom on the fighter. Sent as a real key to the emulator window,
    # the same thing a player does -- synthesising it by calling
    # gmCameraSetStatusPlayerZoom over GDB crashes the core.
    [switch]$InGamePause,
    [switch]$JumpBeforePause
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
if ($OpenGL4x -and $SoftwareRenderer) {
    throw '-OpenGL4x and -SoftwareRenderer are mutually exclusive.'
}
if ($Jit -and $NoJit) {
    throw '-Jit and -NoJit are mutually exclusive.'
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$melonDsPath = Resolve-MelonDSRepoExecutablePath -Root $root -MelonDS $MelonDS
$melonDsDir = Split-Path -Parent $melonDsPath
$romPath = if ([System.IO.Path]::IsPathRooted($Rom)) {
    $Rom
} else {
    Join-Path $root $Rom
}
$gdbPath = if ([System.IO.Path]::IsPathRooted($Gdb)) {
    $Gdb
} else {
    Join-Path $root $Gdb
}
$elfPath = $null
$rendererSelectionEnabled = ($RendererFastRunMode -ge 0)
$foxSelectionEnabled = ($FoxCpuMode -ge 0)
$fighterAnimAuditEnabled = -not [string]::IsNullOrWhiteSpace($FighterAnimAudit)
$pauseCameraEnabled =
    (($PauseCameraPitch -ne 0.0) -or ($PauseCameraYaw -ne 0.0))
$gdbSelectionEnabled = $rendererSelectionEnabled -or $foxSelectionEnabled
$exactTimeCaptureEnabled = ($ExactTimeRemain -gt 0)
$exactFrameCaptureEnabled =
    (($ExactFirstFrame -ge 0) -or ($ExactSecondFrame -ge 0) -or
     $exactTimeCaptureEnabled)
if ($exactFrameCaptureEnabled) {
    if ($exactTimeCaptureEnabled) {
        # One lock or the other. Accepting both would leave which one actually
        # fired dependent on argument order, and a capture whose lock is
        # ambiguous is worse than no capture -- it looks like evidence.
        if (($ExactFirstFrame -ge 0) -or ($ExactSecondFrame -ge 0)) {
            throw ('-ExactTimeRemain locks on the simulation clock and ' +
                '-ExactFirstFrame/-ExactSecondFrame lock on the presented ' +
                'frame counter; pass one or the other, never both.')
        }
    } else {
        if (($ExactFirstFrame -lt 0) -or ($ExactSecondFrame -lt 0)) {
            throw '-ExactFirstFrame and -ExactSecondFrame must be supplied together.'
        }
        if ($ExactSecondFrame -ne ($ExactFirstFrame + 1)) {
            throw 'Exact Cut G capture frames must be adjacent.'
        }
    }
    if ([string]::IsNullOrWhiteSpace($SecondOutput)) {
        throw '-SecondOutput is required for exact Cut G frame capture.'
    }
    if (-not $SoftwareRenderer) {
        throw 'Exact Cut G frame capture requires -SoftwareRenderer.'
    }
    if ($rendererSelectionEnabled) {
        throw '-RendererFastRunMode cannot be combined with exact Cut G capture.'
    }
    # capture-cut-g-exact-frames.ps1 declares [ValidateRange(0,1)]$FoxCpuMode and
    # then asserts it equals 1, but this script's own "unset" sentinel is -1 and
    # it forwards that value unconditionally. So the default invocation used to
    # boot the emulator, run to the requested frame, and only then die inside the
    # callee's parameter binder with a range error naming a switch the caller
    # never set. Normalise the sentinel here, and reject a deliberate 0 up front
    # where the message can say why. R2-03 E32.
    if ($FoxCpuMode -lt 0) {
        $FoxCpuMode = 1
        $foxSelectionEnabled = $true
        $gdbSelectionEnabled = $true
    } elseif ($FoxCpuMode -ne 1) {
        throw 'Exact frame capture requires -FoxCpuMode 1 (the Boundary configuration).'
    }
}
$gdbControlEnabled = $gdbSelectionEnabled -or $exactFrameCaptureEnabled -or
    $fighterAnimAuditEnabled -or $pauseCameraEnabled
if ($gdbControlEnabled) {
    if (-not (Test-Path -LiteralPath $gdbPath -PathType Leaf)) {
        throw "GDB executable not found: $gdbPath"
    }
    $elfPath = if (-not [string]::IsNullOrWhiteSpace($Elf)) {
        if ([System.IO.Path]::IsPathRooted($Elf)) { $Elf } else { Join-Path $root $Elf }
    } else {
        [System.IO.Path]::ChangeExtension($romPath, '.elf')
    }
    if (-not (Test-Path -LiteralPath $elfPath -PathType Leaf)) {
        throw "Matching ELF not found for mode-specific capture: $elfPath"
    }
}
$config = Join-Path $melonDsDir 'melonDS.toml'
$originalConfig = $null
if ($Build) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C $root
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
if (-not (Test-Path $romPath)) {
    throw "ROM not found: $romPath. Run make first or pass -Build."
}
if ($pauseCameraEnabled -and $foxSelectionEnabled) {
    # The melonDS GDB stub takes one connection. The Fox-mode write already
    # spends it, and the later orbit write then attaches with no live target.
    throw '-PauseCameraPitch/-PauseCameraYaw cannot be combined with -FoxCpuMode; the melonDS GDB stub allows a single session.'
}
if ($fighterAnimAuditEnabled) {
    if ($exactFrameCaptureEnabled -or $rendererSelectionEnabled -or
        $foxSelectionEnabled -or -not [string]::IsNullOrWhiteSpace($SecondOutput)) {
        throw '-FighterAnimAudit cannot be combined with other GDB/capture selectors.'
    }
    if ([string]::IsNullOrWhiteSpace($Output)) {
        $stamp = Get-Date -Format 'yyyy-MM-dd'
        $Output = Join-Path $root "artifacts\visibility\${stamp}_task40-$($FighterAnimAudit.ToLowerInvariant())"
    } elseif (-not [System.IO.Path]::IsPathRooted($Output)) {
        $Output = Join-Path $root $Output
    }
    New-Item -ItemType Directory -Path $Output -Force | Out-Null
} elseif ([string]::IsNullOrWhiteSpace($Output)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $Output = Join-Path $root "artifacts\visibility\melonds-$stamp.png"
} elseif (-not [System.IO.Path]::IsPathRooted($Output)) {
    $Output = Join-Path $root $Output
}
$outputDirectory = Split-Path -Parent $Output
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
if (-not [string]::IsNullOrWhiteSpace($SecondOutput)) {
    if (-not [System.IO.Path]::IsPathRooted($SecondOutput)) {
        $SecondOutput = Join-Path $root $SecondOutput
    }
    $secondOutputDirectory = Split-Path -Parent $SecondOutput
    New-Item -ItemType Directory -Path $secondOutputDirectory -Force | Out-Null
}
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSWindowCapture
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
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool PrintWindow(
        IntPtr window, IntPtr destination, uint flags);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(
        IntPtr window, IntPtr insertAfter, int x, int y, int width, int height,
        uint flags);
}
'@
function Save-MelonDSWindowCapture {
    param(
        [Parameter(Mandatory=$true)]
        [System.IntPtr]$WindowHandle,
        [Parameter(Mandatory=$true)]
        [string]$Path
    )
    $rect = New-Object Smash64DSWindowCapture+Rect
    if (-not [Smash64DSWindowCapture]::GetWindowRect(
            $WindowHandle, [ref]$rect)) {
        throw 'Could not read the melonDS window bounds.'
    }
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) {
        throw "Invalid melonDS window bounds: ${width}x${height}."
    }
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $usedPrintWindow = $false
    try {
        try {
            $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        } catch {
            $screenError = $_.Exception.Message
            $destination = $graphics.GetHdc()
            try {
                if (-not [Smash64DSWindowCapture]::PrintWindow(
                        $WindowHandle, $destination, 2)) {
                    throw "CopyFromScreen failed ($screenError) and PrintWindow failed."
                }
                $usedPrintWindow = $true
            } finally {
                $graphics.ReleaseHdc($destination)
            }
        }
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
    if ($usedPrintWindow) {
        Write-Warning 'CopyFromScreen unavailable; used PrintWindow capture.'
    }
    return "${width}x${height}"
}
function Set-MelonDSCaptureWindow {
    param(
        [Parameter(Mandatory=$true)]
        [System.IntPtr]$WindowHandle
    )
    if ($MaximizeVertical) {
        # Preserve the verifier window's aspect ratio while using the desktop's
        # available vertical resolution for close visual inspection.
        $workArea = [System.Windows.Forms.Screen]::PrimaryScreen.WorkingArea
        $height = $workArea.Height
        $width = [Math]::Round($height * (
            $script:MelonDSCanonicalWindowWidth /
            [double]$script:MelonDSCanonicalWindowHeight))
        [void][Smash64DSWindowCapture]::ShowWindow($WindowHandle, 9)
        [void][Smash64DSWindowCapture]::SetWindowPos(
            $WindowHandle, [IntPtr](-1), $workArea.X, $workArea.Y,
            $width, $height, 0x40)
        return
    }
    [void][Smash64DSWindowCapture]::ShowWindow($WindowHandle, 9)
    # Keep capture geometry stable between samples. melonDS can otherwise
    # auto-resize after the first frame and create a false pixel-delta failure.
    [void][Smash64DSWindowCapture]::SetWindowPos(
        $WindowHandle, [IntPtr](-1),
        $script:MelonDSCanonicalWindowX,
        $script:MelonDSCanonicalWindowY,
        $script:MelonDSCanonicalWindowWidth,
        $script:MelonDSCanonicalWindowHeight, 0x40)
}
function Set-MelonDSCaptureRuntimeMode {
    param(
        [Parameter(Mandatory=$true)][string]$GdbPath,
        [Parameter(Mandatory=$true)][string]$ElfPath,
        [Parameter(Mandatory=$true)][int]$Port,
        [Parameter(Mandatory=$true)][int]$Mode,
        [Parameter(Mandatory=$true)][int]$FoxMode
    )

    $expected = @()
    if ($Mode -ge 0) { $expected += "CAPTURE_FAST_MODE=$Mode" }
    if ($FoxMode -ge 0) { $expected += "CAPTURE_FOX_MODE=$FoxMode" }
    $lastOutput = ''
    for ($attempt = 1; $attempt -le 12; $attempt++) {
        $gdbArgs = @('-q', '-batch', $ElfPath,
                     '-ex', "target remote localhost:$Port")
        if ($Mode -ge 0) {
            $gdbArgs += @('-ex', "set variable gNdsRendererFastRunMode = $Mode",
                          '-ex', 'printf "CAPTURE_FAST_MODE=%u\n", gNdsRendererFastRunMode')
        }
        if ($FoxMode -ge 0) {
            $gdbArgs += @('-ex', 'tbreak scVSBattleStartBattle',
                          '-ex', 'continue',
                          '-ex', "set variable gNdsBattlePlayableFoxCpuEnabled = $FoxMode",
                          '-ex', 'printf "CAPTURE_FOX_MODE=%u\n", gNdsBattlePlayableFoxCpuEnabled')
        }
        $gdbArgs += @('-ex', 'detach', '-ex', 'quit')
        $lastOutput = (& $GdbPath @gdbArgs 2>&1 | Out-String)
        $selected = ($LASTEXITCODE -eq 0)
        foreach ($marker in $expected) {
            $selected = $selected -and $lastOutput.Contains($marker)
        }
        if ($selected) {
            Write-Output "Selected capture runtime modes renderer=$Mode fox=$FoxMode."
            return
        }
        Start-Sleep -Milliseconds 250
    }
    throw "Could not select capture runtime mode through GDB. Last output:`n$lastOutput"
}
function Set-MelonDSPauseCamera {
    param(
        [Parameter(Mandatory=$true)][string]$GdbPath,
        [Parameter(Mandatory=$true)][string]$ElfPath,
        [Parameter(Mandatory=$true)][int]$Port,
        [Parameter(Mandatory=$true)][double]$Pitch,
        [Parameter(Mandatory=$true)][double]$Yaw
    )

    $pitchText = $Pitch.ToString(
        [System.Globalization.CultureInfo]::InvariantCulture)
    $yawText = $Yaw.ToString(
        [System.Globalization.CultureInfo]::InvariantCulture)
    $lastOutput = ''
    # One attempt, not a retry loop: a failed write here is a real fault worth
    # seeing immediately, and retrying it twelve times spawns twelve GDB
    # sessions and turns a 30-second capture into a multi-minute one.
    for ($attempt = 1; $attempt -le 1; $attempt++) {
        $gdbArgs = @('-q', '-batch', $ElfPath,
                     '-ex', "target remote localhost:$Port",
                     # Halt first. The melonDS stub cannot serve memory access
                     # while the core is running -- reads come back "Cannot
                     # access memory" with packet timeouts -- so stop on a
                     # per-update function the way the Fox-mode write does.
                     '-ex', 'tbreak ndsPlatformReadInput',
                     '-ex', 'continue',
                     # Cast through a typed pointer: these are plain BSS
                     # symbols with no DWARF type, and an untyped `set
                     # variable` on them silently does nothing and reads back
                     # zero.
                     '-ex', "set variable *(float *)&gGMCameraPauseCameraEyeY = $pitchText",
                     '-ex', "set variable *(float *)&gGMCameraPauseCameraEyeX = $yawText",
                     '-ex', 'printf "CAPTURE_PAUSE_CAM=%f,%f\n", *(float *)&gGMCameraPauseCameraEyeY, *(float *)&gGMCameraPauseCameraEyeX',
                     # The in-game pause freezes the simulation, so this tick
                     # identifies the frozen pose. Two builds only form a valid
                     # A/B if they paused on the same one -- an unmatched pair
                     # differs in most of the frame from sub-pixel camera drift
                     # and drowns the change being measured.
                     '-ex', 'printf "CAPTURE_PAUSE_TICK=%d\n", gSCManagerBattleState->time_passed',
                     '-ex', 'detach', '-ex', 'quit')
        $lastOutput = (& $GdbPath @gdbArgs 2>&1 | Out-String)
        if (($LASTEXITCODE -eq 0) -and
            $lastOutput.Contains('CAPTURE_PAUSE_CAM=') -and
            -not $lastOutput.Contains('CAPTURE_PAUSE_CAM=0.000000,0.000000')) {
            $line = ($lastOutput -split "`n" |
                Where-Object { $_ -match 'CAPTURE_PAUSE_CAM=' } |
                Select-Object -First 1).Trim()
            Write-Output "Pause camera orbit applied: $line"
            $tick = ($lastOutput -split "`n" |
                Where-Object { $_ -match 'CAPTURE_PAUSE_TICK=' } |
                Select-Object -First 1)
            if ($null -ne $tick) { Write-Output $tick.Trim() }
            return
        }
        Write-Output "PAUSE_CAM_ATTEMPT_${attempt}: $lastOutput"
        Start-Sleep -Milliseconds 250
    }
    throw "Could not set the pause camera through GDB. Last output:`n$lastOutput"
}
$emulator = $null
try {
    if (Test-Path $config) {
        $originalConfig = Get-Content $config -Raw
        $visibleConfig = Set-MelonDSDualScreenLayout -Text $originalConfig
        if ($gdbControlEnabled) {
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'Instance0.Gdb' -Key 'Enable' -Value 'true'
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'Instance0.Gdb' -Key 'Enabled' -Value 'true'
            $arm9BreakOnStartup = if ($exactFrameCaptureEnabled -or
                $foxSelectionEnabled -or $fighterAnimAuditEnabled) {
                'true'
            } else {
                'false'
            }
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'Instance0.Gdb.ARM9' -Key 'BreakOnStartup' `
                -Value $arm9BreakOnStartup
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'Instance0.Gdb.ARM9' -Key 'Port' -Value "$GdbPort"
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'Instance0.Gdb.ARM7' -Key 'BreakOnStartup' -Value 'false'
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'Instance0.Gdb.ARM7' -Key 'Port' -Value "$($GdbPort + 1)"
        } else {
            $visibleConfig = $visibleConfig -replace
                '(?s)(\[Instance0\.Gdb\]\s*Enabled\s*=\s*)true', '${1}false'
            $visibleConfig = $visibleConfig -replace
                '(?s)(\[Instance0\.Gdb\]\s*Enable\s*=\s*)true', '${1}false'
        }
        if ($Unthrottled) {
            $visibleConfig = $visibleConfig -replace
                '(?m)^(LimitFPS\s*=\s*)true\s*$', '${1}false'
            $visibleConfig = $visibleConfig -replace
                '(?ms)(\[JIT\].*?^Enable\s*=\s*)true\s*$', '${1}false'
        }
        if ($Jit) {
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'JIT' -Key 'Enable' -Value 'true'
        } elseif ($NoJit) {
            $visibleConfig = Set-MelonDSTomlValue -Text $visibleConfig `
                -Section 'JIT' -Key 'Enable' -Value 'false'
        }
        if ($SoftwareRenderer) {
            $visibleConfig = $visibleConfig -replace
                '(?m)^(Renderer\s*=\s*)[0-9]+\s*$', '${1}0'
        } elseif ($OpenGL4x) {
            $visibleConfig = $visibleConfig -replace
                '(?m)^(Renderer\s*=\s*)[0-9]+\s*$', '${1}1'
            $visibleConfig = $visibleConfig -replace
                '(?m)^(ScaleFactor\s*=\s*)[0-9]+\s*$', '${1}4'
        }
        if ($visibleConfig -ne $originalConfig) {
            Set-Content $config -Value $visibleConfig -NoNewline
        }
    }
    $emulator = # WindowStyle: visible-by-design -- this harness screenshots the
    $emulator = # emulator window, and a hidden window has neither a MainWindowHandle
    $emulator = # nor desktop pixels to grab (measured 2026-07-29: hiding it returns
    $emulator = # IntPtr.Zero and the capture dies).
    $emulator = Start-Process -FilePath $melonDsPath -ArgumentList "`"$romPath`"" `
        -WorkingDirectory $melonDsDir -PassThru
    $deadline = (Get-Date).AddSeconds([Math]::Max($DelaySeconds, 2) + 10)
    do {
        Start-Sleep -Milliseconds 250
        $emulator.Refresh()
    } while ($emulator.MainWindowHandle -eq [IntPtr]::Zero -and
             -not $emulator.HasExited -and (Get-Date) -lt $deadline)
    if ($emulator.HasExited -or $emulator.MainWindowHandle -eq [IntPtr]::Zero) {
        throw 'melonDS did not expose a capturable window.'
    }
    if ($gdbSelectionEnabled -and -not $exactFrameCaptureEnabled) {
        Set-MelonDSCaptureRuntimeMode -GdbPath $gdbPath -ElfPath $elfPath `
            -Port $GdbPort -Mode $RendererFastRunMode -FoxMode $FoxCpuMode
    }
    if ($fighterAnimAuditEnabled) {
        Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
        [void][Smash64DSWindowCapture]::SetForegroundWindow(
            $emulator.MainWindowHandle)
        Start-Sleep -Milliseconds 100
        Wait-MelonDSGdbListener -Process $emulator -Port $GdbPort | Out-Null
        & (Join-Path $PSScriptRoot 'capture-fighter-animation-audit.ps1') `
            -AuditMode $FighterAnimAudit `
            -Gdb $gdbPath `
            -Elf $elfPath `
            -GdbPort $GdbPort `
            -EmulatorProcessId $emulator.Id `
            -WindowHandle ([long]$emulator.MainWindowHandle) `
            -OutputDirectory $Output `
            -StartMotion $FighterAnimStartMotion
    } elseif ($exactFrameCaptureEnabled) {
        # Establish geometry once while emulation is running. Moving or
        # foregrounding the window after an exact frame pause can capture a Qt
        # resize transition instead of the completed DS presentation.
        Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
        [void][Smash64DSWindowCapture]::SetForegroundWindow(
            $emulator.MainWindowHandle)
        Start-Sleep -Milliseconds 100
        Wait-MelonDSGdbListener -Process $emulator -Port $GdbPort | Out-Null
        & (Join-Path $PSScriptRoot 'capture-cut-g-exact-frames.ps1') `
            -Gdb $gdbPath `
            -Elf $elfPath `
            -GdbPort $GdbPort `
            -EmulatorProcessId $emulator.Id `
            -WindowHandle ([long]$emulator.MainWindowHandle) `
            -Output $Output `
            -SecondOutput $SecondOutput `
            -FirstFrame $(if ($exactTimeCaptureEnabled) { 200 }
                          else { $ExactFirstFrame }) `
            -SecondFrame $(if ($exactTimeCaptureEnabled) { 201 }
                           else { $ExactSecondFrame }) `
            -TimeRemain $ExactTimeRemain `
            -FoxCpuMode $FoxCpuMode
    } else {
        Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
        [void][Smash64DSWindowCapture]::SetForegroundWindow(
            $emulator.MainWindowHandle)
        Start-Sleep -Seconds $DelaySeconds
        if ($InGamePause -or $JumpBeforePause) {
            Add-Type -AssemblyName System.Windows.Forms
            [void][Smash64DSWindowCapture]::SetForegroundWindow(
                $emulator.MainWindowHandle)
            Start-Sleep -Milliseconds 300
            if ($JumpBeforePause) {
                # Tap up to jump, then let him rise before freezing, so the
                # platform is not between the camera and his underside.
                [System.Windows.Forms.SendKeys]::SendWait('{UP}')
                Start-Sleep -Milliseconds 260
            }
            if ($InGamePause) {
                [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
                Start-Sleep -Milliseconds 700
            }
        }
        if (($PauseCameraPitch -ne 0.0) -or ($PauseCameraYaw -ne 0.0)) {
            # Deliberately the only GDB session when orbiting: the melonDS stub
            # is single-connection, so pass -FoxCpuMode -1 with this.
            Wait-MelonDSGdbListener -Process $emulator -Port $GdbPort |
                Out-Null
            Set-MelonDSPauseCamera -GdbPath $gdbPath -ElfPath $elfPath `
                -Port $GdbPort -Pitch $PauseCameraPitch -Yaw $PauseCameraYaw
            Start-Sleep -Milliseconds 500
        }
        $emulator.Refresh()
        Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
        Start-Sleep -Milliseconds 100
        $size = Save-MelonDSWindowCapture `
            -WindowHandle $emulator.MainWindowHandle -Path $Output
        if (-not [string]::IsNullOrWhiteSpace($SecondOutput)) {
            if ($SecondDelayMilliseconds -gt 0) {
                Start-Sleep -Milliseconds $SecondDelayMilliseconds
            } else {
                Start-Sleep -Seconds ([Math]::Max($SecondDelaySeconds, 0))
            }
            $emulator.Refresh()
            Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
            Start-Sleep -Milliseconds 100
            $secondSize = Save-MelonDSWindowCapture `
                -WindowHandle $emulator.MainWindowHandle -Path $SecondOutput
            Write-Output "Captured live melonDS window: $SecondOutput ($secondSize)"
        }
    }
    try {
        [void][Smash64DSWindowCapture]::SetWindowPos(
            $emulator.MainWindowHandle, [IntPtr](-2), 0, 0, 0, 0, 0x43)
    } catch {
        Write-Warning "Could not lower melonDS window: $_"
    }
    if (-not $exactFrameCaptureEnabled -and -not $fighterAnimAuditEnabled) {
        Write-Output "Captured live melonDS window: $Output ($size)"
    }
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    if ($null -ne $originalConfig) {
        Set-Content $config -Value $originalConfig -NoNewline
    }
}
