[CmdletBinding()]
param(
    [string]$Build = 'build-p2-ness-upb-shell-real',
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    [ValidateRange(1, 12)][int]$RunnerSlot = 8,
    [ValidateRange(1, 20)][int]$Attempts = 8,
    [ValidateRange(60, 600)][int]$TimeoutSeconds = 300,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$null = Add-Type -AssemblyName Microsoft.VisualBasic
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

if (-not ('Smash64DSNessShellInput' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSNessShellInput
{
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr window, IntPtr processId);
    [DllImport("kernel32.dll")]
    public static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")]
    public static extern bool AttachThreadInput(uint idAttach, uint idAttachTo, bool attach);
    [DllImport("user32.dll")]
    public static extern bool BringWindowToTop(IntPtr window);
    [DllImport("user32.dll")]
    public static extern IntPtr SetActiveWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern IntPtr SetFocus(IntPtr window);
    [DllImport("user32.dll")]
    public static extern void keybd_event(byte key, byte scan, uint flags, UIntPtr extra);
}
'@
}

$VK_RETURN = [byte]0x0D
$VK_A = [byte]0x41
$VK_S = [byte]0x53
$VK_UP = [byte]0x26
$VK_DOWN = [byte]0x28
$KEYUP = [uint32]2
$KEYEVENTF_EXTENDEDKEY = [uint32]1
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'

$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$configPath = Join-Path (Split-Path -Parent $rom) 'nds_build_config.h'
$config = Get-Content -LiteralPath $configPath -Raw
foreach ($required in @(
    'NDS_DEV_LIVE_INPUT_PREVIEW 1',
    'NDS_HARNESS_FAST_LOGIC 0',
    'NDS_P2_MENU_SHELL 1',
    'NDS_P2_1P_GAME 1',
    'NDS_P2_NESS 1',
    'NDS_P2_PROOF_FIGHTER0 11'
)) {
    if ($config -notmatch ('(?m)^#define ' + [regex]::Escape($required) + '$')) {
        throw "Ness shell proof requires '$required'."
    }
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_ness-upb-shell-real-input.txt')
}

$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' `
    -RunnerSlot $RunnerSlot -NoBuild
$runnerConfigOriginal = [System.IO.File]::ReadAllBytes($context.ConfigPath)
$melonDir = Split-Path -Parent $context.MelonDSPath
$tempDir = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stageDir = Join-Path $tempDir 'ness-upb-shell-stages'
New-Item -ItemType Directory -Force -Path $tempDir,$logDir,$stageDir | Out-Null
Get-ChildItem -LiteralPath $stageDir -File -ErrorAction SilentlyContinue |
    Remove-Item -Force

$gdbScript = Join-Path $tempDir 'ness_upb_shell_real.gdb'
$gdbOut = Join-Path $tempDir 'ness_upb_shell_real.gdb.out'
$gdbErr = Join-Path $tempDir 'ness_upb_shell_real.gdb.err'
$stdout = Join-Path $logDir 'melonds.ness-upb-shell-real.stdout.log'
$stderr = Join-Path $logDir 'melonds.ness-upb-shell-real.stderr.log'

function Stage-Path([string]$Name) { Join-Path $stageDir ($Name + '.ready') }
function Gdb-Path([string]$Name) { ((Stage-Path $Name) -replace '\\', '/') }

function Wait-Stage {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$Emulator,
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$GdbProcess,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    $path = Stage-Path $Name
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path -LiteralPath $path -PathType Leaf) { return }
        if (Test-Path -LiteralPath (Stage-Path 'fault') -PathType Leaf) {
            throw 'ARM exception occurred during the natural-shell Ness Up-B run.'
        }
        $Emulator.Refresh()
        if ($Emulator.HasExited) {
            throw "melonDS exited during the natural-shell Ness Up-B run (code $($Emulator.ExitCode))."
        }
        $GdbProcess.Refresh()
        if ($GdbProcess.HasExited) {
            $out = if (Test-Path $gdbOut) { Get-Content $gdbOut -Raw } else { '' }
            $err = if (Test-Path $gdbErr) { Get-Content $gdbErr -Raw } else { '' }
            throw "GDB observer exited before shell stage '$Name'. OUT=$out ERR=$err"
        }
        Start-Sleep -Milliseconds 20
    }
    throw "Timed out waiting for natural-shell stage '$Name' (freeze/stall)."
}

function Get-MelonWindow {
    param([System.Diagnostics.Process]$Process, [datetime]$Deadline)
    while ((Get-Date) -lt $Deadline) {
        $Process.Refresh()
        if ($Process.HasExited) { throw 'melonDS exited before opening a window.' }
        if ($Process.MainWindowHandle -ne [IntPtr]::Zero) { return $Process.MainWindowHandle }
        Start-Sleep -Milliseconds 50
    }
    throw 'Timed out waiting for the melonDS window.'
}

function Focus-Melon([IntPtr]$Window, [int]$ProcessId) {
    for ($attempt = 0; $attempt -lt 6; $attempt++) {
        [void][Smash64DSNessShellInput]::ShowWindow($Window, 9)
        try { [void][Microsoft.VisualBasic.Interaction]::AppActivate($ProcessId) } catch { }
        $ours = [Smash64DSNessShellInput]::GetCurrentThreadId()
        $theirs = [Smash64DSNessShellInput]::GetWindowThreadProcessId($Window, [IntPtr]::Zero)
        $attached = ($theirs -ne 0) -and ($ours -ne $theirs) -and
            [Smash64DSNessShellInput]::AttachThreadInput($ours, $theirs, $true)
        try {
            [void][Smash64DSNessShellInput]::BringWindowToTop($Window)
            [void][Smash64DSNessShellInput]::SetForegroundWindow($Window)
            [void][Smash64DSNessShellInput]::SetActiveWindow($Window)
            [void][Smash64DSNessShellInput]::SetFocus($Window)
        } finally {
            if ($attached) {
                [void][Smash64DSNessShellInput]::AttachThreadInput($ours, $theirs, $false)
            }
        }
        Start-Sleep -Milliseconds 250
        if ([Smash64DSNessShellInput]::GetForegroundWindow() -eq $Window) {
            return
        }
        Start-Sleep -Milliseconds 250
    }
    throw 'Windows refused to foreground the private melonDS runner after retries.'
}

function Tap-Key {
    param([IntPtr]$Window, [int]$ProcessId, [byte]$Key, [switch]$Extended)
    Focus-Melon -Window $Window -ProcessId $ProcessId
    $down = if ($Extended) { $KEYEVENTF_EXTENDEDKEY } else { 0u }
    $up = $down -bor $KEYUP
    [Smash64DSNessShellInput]::keybd_event($Key, 0, $down, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [Smash64DSNessShellInput]::keybd_event($Key, 0, $up, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 180
}

function Send-UpB {
    param([IntPtr]$Window, [int]$ProcessId)
    Focus-Melon -Window $Window -ProcessId $ProcessId
    [Smash64DSNessShellInput]::keybd_event($VK_UP, 0, $KEYEVENTF_EXTENDEDKEY, [UIntPtr]::Zero)
    try {
        Start-Sleep -Milliseconds 70
        [Smash64DSNessShellInput]::keybd_event($VK_S, 0, 0, [UIntPtr]::Zero)
        try { Start-Sleep -Milliseconds 180 }
        finally { [Smash64DSNessShellInput]::keybd_event($VK_S, 0, $KEYUP, [UIntPtr]::Zero) }
    }
    finally {
        [Smash64DSNessShellInput]::keybd_event($VK_UP, 0, ($KEYEVENTF_EXTENDEDKEY -bor $KEYUP), [UIntPtr]::Zero)
    }
}

$commands = @'
set pagination off
set confirm off
set remotetimeout 20
target remote 127.0.0.1:__PORT__
set $ness = 0
set $starts = 0
set $holds = 0
set $completed = 0

break __excpt_entry
commands
silent
printf "SHELL_UPB_EXCEPTION pc=%#x lr=%#x starts=%u holds=%u completed=%u\n", $pc,$lr,$starts,$holds,$completed
bt 12
shell cmd /c echo fault>__FAULT__
detach
quit
end

set $scene = (int)gSCManagerSceneData.scene_curr
printf "SHELL_CURRENT scene=%d\n", $scene
printf "SHELL_HEAP overflow=%u arena=%u request=%u align=%u headroom=%u caller=%08x heap=%08x/%08x/%08x aobj=%u/%u bytes=%u declines=%u anim=%u/%u rejects=%u pack=%u/%u arenaChosen=%u refine=%u libcHigh=%u topMin=%u heapMin=%u frame=%u loop=%u sleeps=%u thread=%u/%u noentry=%u createfail=%u\n", gNdsSyMallocOverflowCount, gNdsSyMallocOverflowArenaID, gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowAlignment, gNdsSyMallocOverflowHeadroom, gNdsSyMallocOverflowCallerLR, gSYTaskmanGeneralHeap.start, gSYTaskmanGeneralHeap.ptr, gSYTaskmanGeneralHeap.end, sGCAnimsActiveNum, gNdsR2AObjPoolCount, gNdsR2AObjPoolBytes, gNdsR2AObjPoolDeclines, gNdsR2AnimCacheArenaUsedBytes, gNdsR2AnimCacheArenaReservedBytes, gNdsR2AnimCacheRejects, gNdsBattlePackCarveMatchKinds, gNdsBattlePackCarveDeclineCount, gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaRefineBytes, gNdsTaskmanLibcRuntimeHighWater, gNdsTaskmanLibcTopChunkMin, gNdsTaskmanGeneralHeapFreeMin, gNdsFrameCounter, gNdsTaskmanLoopReached, gNdsTaskmanGObjThreadSleeps, gNdsOsGObjThreadProvisionCount, gNdsOsGObjThreadProvisionFailCount, gNdsOsStartThreadNoEntryCount, gNdsOsStartThreadCreateFailCount
if gNdsSyMallocOverflowCount != 0
info symbol gNdsSyMallocOverflowCallerLR
bt 16
shell cmd /c echo oom>__OOM__
detach
quit 3
end
if $scene != 22
printf "SHELL_NAV_FAIL scene=%d\n", $scene
detach
quit 2
end
set $ness = sNdsFighterManagerLiveGObjs[0]
if $ness == 0
printf "SHELL_NAV_FAIL no player-0 fighter\n"
detach
quit 2
end
printf "SHELL_NESS_READY gobj=%p fkind=%d status=%d cfg0=%d/%d\n", $ness, ((FTStruct *)$ness->user_data.p)->fkind, ((FTStruct *)$ness->user_data.p)->status_id, gNdsMatchConfig.fighters[0].fkind, gNdsMatchConfig.fighters[0].pkind
if ((FTStruct *)$ness->user_data.p)->fkind != 11
printf "SHELL_NAV_FAIL player0=%d\n", ((FTStruct *)$ness->user_data.p)->fkind
detach
quit 2
end
shell cmd /c echo ready>__BATTLE__

break ftCommonAppearSetStatus if $r0 == $ness
commands
silent
printf "SHELL_APPEAR status=%d frame=%u sleeps=%u\n", ((FTStruct *)$ness->user_data.p)->status_id, gNdsFrameCounter, gNdsTaskmanGObjThreadSleeps
shell cmd /c echo ready>__APPEAR__
continue
end

break ftNessSpecialHiStartSetStatus if $r0 == $ness
commands
silent
set $starts = $starts + 1
printf "SHELL_UPB_START count=%u air=0\n", $starts
shell cmd /c echo ready>__START__
continue
end
break ftNessSpecialAirHiStartSetStatus if $r0 == $ness
commands
silent
set $starts = $starts + 1
printf "SHELL_UPB_START count=%u air=1\n", $starts
shell cmd /c echo ready>__START__
continue
end
break ftNessSpecialHiHoldSetStatus if $r0 == $ness
commands
silent
set $holds = $holds + 1
printf "SHELL_UPB_HOLD count=%u air=0\n", $holds
continue
end
break ftNessSpecialAirHiHoldSetStatus if $r0 == $ness
commands
silent
set $holds = $holds + 1
printf "SHELL_UPB_HOLD count=%u air=1\n", $holds
continue
end
break ftCommonWaitSetStatus if $r0 == $ness
commands
silent
if $starts == 0
printf "SHELL_UPB_IDLE status=%d\n", ((FTStruct *)$ness->user_data.p)->status_id
shell cmd /c echo ready>__IDLE__
end
if $starts > $completed
set $completed = $starts
printf "SHELL_UPB_COMPLETE count=%u holds=%u\n", $completed,$holds
shell cmd /c echo ready>__COMPLETE__
if $completed >= __ATTEMPTS__
printf "SHELL_UPB_FINAL starts=%u holds=%u completed=%u libcHigh=%u topMin=%u heapMin=%u arenaChosen=%u aobjPool=%u aobjDeclines=%u\n", $starts,$holds,$completed,gNdsTaskmanLibcRuntimeHighWater,gNdsTaskmanLibcTopChunkMin,gNdsTaskmanGeneralHeapFreeMin,gNdsTaskmanArenaChosenSize,gNdsR2AObjPoolCount,gNdsR2AObjPoolDeclines
shell cmd /c echo ready>__DONE__
detach
quit
end
end
continue
end
continue
'@

$replace = @{
    '__PORT__' = [string]$context.GdbPort
    '__FAULT__' = Gdb-Path 'fault'
    '__OOM__' = Gdb-Path 'oom'
    '__BATTLE__' = Gdb-Path 'battle'
    '__APPEAR__' = Gdb-Path 'appear'
    '__IDLE__' = Gdb-Path 'idle'
    '__START__' = Gdb-Path 'start'
    '__COMPLETE__' = Gdb-Path 'complete'
    '__DONE__' = Gdb-Path 'done'
    '__ATTEMPTS__' = [string]$Attempts
}
foreach ($key in $replace.Keys) { $commands = $commands.Replace($key, $replace[$key]) }
Set-Content -LiteralPath $gdbScript -Value $commands

$configState = $null
$emulator = $null
$gdbProcess = $null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    # Human menu taps need hardware-rate sampling. The runner normally has its
    # host FPS limiter off for automated probes, which burns through the Title
    # attract timeout before a host can press START and can repeat one D-pad
    # press across many guest ticks. Cap only this private run at 60 FPS and
    # restore the runner config byte-for-byte in finally.
    $runnerConfigText = Get-Content -LiteralPath $context.ConfigPath -Raw
    $runnerConfigText = [regex]::Replace(
        $runnerConfigText, '(?m)^LimitFPS\s*=\s*false\s*$', 'LimitFPS = true')
    [System.IO.File]::WriteAllText(
        $context.ConfigPath, $runnerConfigText,
        [System.Text.UTF8Encoding]::new($false))
    Remove-Item -LiteralPath $stdout,$stderr,$gdbOut,$gdbErr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory $melonDir -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $window = Get-MelonWindow -Process $emulator -Deadline $deadline

    # Natural human path. Give Startup enough time to reach Title, then use the
    # exact source menu defaults: Title START; Mode starts on 1P so DOWN+A picks
    # VS; VS starts on VS START so A opens CSS; CSS was seeded with Ness by the
    # ordinary match descriptor; START accepts it; Stage Select starts on Dream
    # Land, where A enters the battle. No guest state is written.
    Start-Sleep -Seconds 2
    Tap-Key $window $emulator.Id $VK_RETURN
    Start-Sleep -Seconds 2
    Tap-Key $window $emulator.Id $VK_DOWN -Extended
    Tap-Key $window $emulator.Id $VK_A
    Start-Sleep -Seconds 2
    Tap-Key $window $emulator.Id $VK_A
    Start-Sleep -Seconds 4
    Tap-Key $window $emulator.Id $VK_RETURN
    # Scene asset loads can hold the prior frame for several seconds. Repeated
    # A taps are harmless during CSS's accepted-START delay and guarantee a
    # real confirmation once Stage Select is actually sampling input.
    for ($i = 0; $i -lt 8; $i++) {
        Start-Sleep -Seconds 2
        Tap-Key $window $emulator.Id $VK_A
    }
    # Leave entry/countdown completely uninstrumented.  The previous verifier
    # attached while Ness was still in common Entry and then put a breakpoint
    # on common entry traffic, which could distort the very transition under
    # test. Eighteen realtime seconds comfortably covers the source 90-tick
    # entry delay, 120-frame entry wait/animation and countdown on this runner.
    Start-Sleep -Seconds 18

    # Attach only after natural menu navigation. This avoids debugger stops
    # perturbing the shell's coroutine/scene hand-offs while still trapping the
    # crash window the user reported.
    $gdbProcess = Start-Process -FilePath $gdb `
        -ArgumentList @('-batch', $elf, '-x', $gdbScript) `
        -RedirectStandardOutput $gdbOut -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    Wait-Stage battle $emulator $gdbProcess ((Get-Date).AddSeconds(30))
    # Source entry is coroutine/tick driven. The accurate interpreter can run
    # the all-content image well below wall-clock realtime, so do not label a
    # slow 90-tick source sleep a freeze. First prove the entry thread reaches
    # Ness, then wait for the actual Wait transition.
    Wait-Stage appear $emulator $gdbProcess ((Get-Date).AddSeconds(240))
    Wait-Stage idle $emulator $gdbProcess ((Get-Date).AddSeconds(240))
    Start-Sleep -Milliseconds 250

    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        Remove-Item -LiteralPath (Stage-Path 'start'),(Stage-Path 'complete') -Force -ErrorAction SilentlyContinue
        $started = $false
        for ($press = 1; $press -le 20; $press++) {
            Send-UpB $window $emulator.Id
            $pressDeadline = (Get-Date).AddMilliseconds(900)
            while ((Get-Date) -lt $pressDeadline) {
                if (Test-Path -LiteralPath (Stage-Path 'start') -PathType Leaf) { $started = $true; break }
                if (Test-Path -LiteralPath (Stage-Path 'fault') -PathType Leaf) { throw 'ARM exception during real shell Up-B input.' }
                $emulator.Refresh()
                if ($emulator.HasExited) { throw 'melonDS exited during real shell Up-B input.' }
                Start-Sleep -Milliseconds 20
            }
            if ($started) { break }
            Start-Sleep -Milliseconds 180
        }
        if (-not $started) { throw "Twenty real Up+B presses failed to enter source Up-B for shell attempt $attempt." }
        Wait-Stage complete $emulator $gdbProcess ((Get-Date).AddSeconds(15))
        Write-Output "SHELL_REAL_UPB_ATTEMPT=$attempt PASS"
        Start-Sleep -Milliseconds 250
    }
    Wait-Stage done $emulator $gdbProcess $deadline
}
finally {
    if ($gdbProcess -and (-not $gdbProcess.HasExited)) { Stop-Process -Id $gdbProcess.Id -Force -ErrorAction SilentlyContinue }
    if ($emulator) { Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
    [System.IO.File]::WriteAllBytes($context.ConfigPath, $runnerConfigOriginal)
}

$trace = Get-Content -LiteralPath $gdbOut -Raw
if ($trace -match 'SHELL_UPB_EXCEPTION') { throw 'Natural-shell Ness Up-B trace contains an ARM exception.' }
$final = [regex]::Match($trace, 'SHELL_UPB_FINAL starts=(\d+) holds=(\d+) completed=(\d+)')
if (-not $final.Success) { throw 'Natural-shell Ness Up-B trace has no final completion marker.' }
if (([int]$final.Groups[1].Value -ne $Attempts) -or
    ([int]$final.Groups[2].Value -lt $Attempts) -or
    ([int]$final.Groups[3].Value -ne $Attempts)) {
    throw "Natural-shell Ness Up-B counts are incomplete: $($final.Value)"
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
@(
    'NESS_UPB_PATH=real Title->VS->CSS->DreamLand shell navigation plus real VK_UP+S battle input',
    "ATTEMPTS=$Attempts",
    "ROM_SHA256=$((Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash)",
    "BUILD=$Build",
    "TARGET=$Target",
    $final.Value
) | Set-Content -LiteralPath $Artifact
Get-Content -LiteralPath $gdbOut | Where-Object { $_ -match '^(SHELL_|SHELL_UPB_)' } | Add-Content -LiteralPath $Artifact
Get-Content -LiteralPath $Artifact
Write-Output "Natural-shell Ness real-input Up-B proof: PASS ($Artifact)"
