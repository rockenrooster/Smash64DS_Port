[CmdletBinding()]
param(
    [string]$Build = 'build-p2-ness-upb-real',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 12)][int]$RunnerSlot = 8,
    [ValidateRange(1, 20)][int]$Attempts = 8,
    [ValidateRange(30, 300)][int]$TimeoutSeconds = 120,
    [switch]$SelfHit,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$null = Add-Type -AssemblyName Microsoft.VisualBasic
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

if (-not ('Smash64DSNessRealInput' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSNessRealInput
{
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")]
    public static extern void keybd_event(
        byte key, byte scan, uint flags, UIntPtr extra);
}
'@
}

$VK_UP = [byte]0x26
$VK_DOWN = [byte]0x28
$VK_LEFT = [byte]0x25
$VK_RIGHT = [byte]0x27
$VK_X = [byte]0x58
$VK_S = [byte]0x53
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'

$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$configPath = Join-Path (Split-Path -Parent $rom) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) {
    throw "Ness real-input proof requires generated config: $configPath"
}
$config = Get-Content -LiteralPath $configPath -Raw
foreach ($required in @(
    'NDS_DEV_LIVE_INPUT_PREVIEW 1',
    'NDS_HARNESS_FAST_LOGIC 0',
    'NDS_P2_NESS 1',
    'NDS_P2_PROOF_FIGHTER0 11'
)) {
    if ($config -notmatch ('(?m)^#define ' + [regex]::Escape($required) + '$')) {
        throw "Ness real-input proof requires '$required'."
    }
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_ness-upb-real-input.txt')
}

$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' `
    -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$tempDir = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stageDir = Join-Path $tempDir 'ness-upb-real-stages'
New-Item -ItemType Directory -Force -Path $tempDir,$logDir,$stageDir | Out-Null
Get-ChildItem -LiteralPath $stageDir -File -ErrorAction SilentlyContinue |
    Remove-Item -Force

$gdbScript = Join-Path $tempDir 'ness_upb_real.gdb'
$gdbOut = Join-Path $tempDir 'ness_upb_real.gdb.out'
$gdbErr = Join-Path $tempDir 'ness_upb_real.gdb.err'
$stdout = Join-Path $logDir 'melonds.ness-upb-real.stdout.log'
$stderr = Join-Path $logDir 'melonds.ness-upb-real.stderr.log'

function Stage-Path([string]$Name) {
    Join-Path $stageDir ($Name + '.ready')
}
function Gdb-Path([string]$Name) {
    ((Stage-Path $Name) -replace '\\', '/')
}
function Wait-Stage {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$Emulator,
        [Parameter(Mandatory=$true)][datetime]$Deadline,
        [ValidateRange(1,1000)][int]$PollMilliseconds = 20
    )
    $path = Stage-Path $Name
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path -LiteralPath $path -PathType Leaf) { return }
        if (Test-Path -LiteralPath (Stage-Path 'fault') -PathType Leaf) {
            throw 'ARM exception occurred during real Ness Up-B input.'
        }
        $Emulator.Refresh()
        if ($Emulator.HasExited) {
            throw "melonDS exited during real Ness Up-B input (code $($Emulator.ExitCode))."
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    throw "Timed out waiting for Ness Up-B stage '$Name' (freeze/stall)."
}
function Wait-SteerOrJibaku {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$Emulator,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    $path = Stage-Path $Name
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path -LiteralPath (Stage-Path 'jibaku') -PathType Leaf) {
            return 'jibaku'
        }
        if (Test-Path -LiteralPath $path -PathType Leaf) { return 'marker' }
        if (Test-Path -LiteralPath (Stage-Path 'fault') -PathType Leaf) {
            throw 'ARM exception occurred during real Ness Up-B input.'
        }
        $Emulator.Refresh()
        if ($Emulator.HasExited) {
            throw "melonDS exited during real Ness Up-B input (code $($Emulator.ExitCode))."
        }
        Start-Sleep -Milliseconds 1
    }
    throw "Timed out waiting for PK Thunder steering marker '$Name'."
}
function Focus-Melon {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Window,
        [Parameter(Mandatory=$true)][int]$ProcessId
    )
    # Use the same held-key path as soak-freeze-watch.ps1. Qt's top-level
    # PostMessage path does not reach melonDS gameplay input reliably; a real
    # foreground key down/up pair does and spans multiple guest samples.
    [void][Smash64DSNessRealInput]::ShowWindow($Window, 9)
    $focused = $false
    for ($focusTry = 0; $focusTry -lt 10; $focusTry++) {
        [void][Microsoft.VisualBasic.Interaction]::AppActivate($ProcessId)
        [void][Smash64DSNessRealInput]::SetForegroundWindow($Window)
        Start-Sleep -Milliseconds 120
        if ([Smash64DSNessRealInput]::GetForegroundWindow() -eq $Window) {
            $focused = $true
            break
        }
    }
    if (-not $focused) {
        throw 'Windows refused to foreground the private melonDS runner.'
    }
}
function Send-HeldKeys {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Window,
        [Parameter(Mandatory=$true)][int]$ProcessId,
        [Parameter(Mandatory=$true)][byte[]]$Keys,
        [Parameter(Mandatory=$true)][int]$HoldMilliseconds,
        [switch]$AlreadyFocused
    )
    if (-not $AlreadyFocused) {
        Focus-Melon -Window $Window -ProcessId $ProcessId
    }
    foreach ($key in $Keys) {
        $flags = if (($key -eq $VK_UP) -or ($key -eq $VK_DOWN) -or
                    ($key -eq $VK_LEFT) -or ($key -eq $VK_RIGHT)) { 1 } else { 0 }
        [Smash64DSNessRealInput]::keybd_event($key, 0, $flags, [UIntPtr]::Zero)
    }
    try { Start-Sleep -Milliseconds $HoldMilliseconds }
    finally {
        for ($i = $Keys.Count - 1; $i -ge 0; $i--) {
            $key = $Keys[$i]
            $flags = if (($key -eq $VK_UP) -or ($key -eq $VK_DOWN) -or
                        ($key -eq $VK_LEFT) -or ($key -eq $VK_RIGHT)) { 3 } else { 2 }
            [Smash64DSNessRealInput]::keybd_event($key, 0, $flags, [UIntPtr]::Zero)
        }
    }
}
function Set-KeysDown {
    param([Parameter(Mandatory=$true)][byte[]]$Keys)
    foreach ($key in $Keys) {
        $flags = if (($key -eq $VK_UP) -or ($key -eq $VK_DOWN) -or
                    ($key -eq $VK_LEFT) -or ($key -eq $VK_RIGHT)) { 1 } else { 0 }
        [Smash64DSNessRealInput]::keybd_event($key, 0, $flags, [UIntPtr]::Zero)
    }
}
function Set-KeysUp {
    param([Parameter(Mandatory=$true)][byte[]]$Keys)
    for ($i = $Keys.Count - 1; $i -ge 0; $i--) {
        $key = $Keys[$i]
        $flags = if (($key -eq $VK_UP) -or ($key -eq $VK_DOWN) -or
                    ($key -eq $VK_LEFT) -or ($key -eq $VK_RIGHT)) { 3 } else { 2 }
        [Smash64DSNessRealInput]::keybd_event($key, 0, $flags, [UIntPtr]::Zero)
    }
}
function Send-UpB {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Window,
        [Parameter(Mandatory=$true)][int]$ProcessId
    )
    Focus-Melon -Window $Window -ProcessId $ProcessId
    [Smash64DSNessRealInput]::keybd_event($VK_UP, 0, 1, [UIntPtr]::Zero)
    try {
        # Keep this a real two-key chord, but do not leave a whole guest input
        # window between Up and B. On a fast/variable-speed emulator that turns
        # the intended Up-B into a plain stick jump before B can be sampled.
        Start-Sleep -Milliseconds 8
        [Smash64DSNessRealInput]::keybd_event($VK_S, 0, 0, [UIntPtr]::Zero)
        try { Start-Sleep -Milliseconds 180 }
        finally {
            [Smash64DSNessRealInput]::keybd_event($VK_S, 0, 2, [UIntPtr]::Zero)
        }
    }
    finally {
        [Smash64DSNessRealInput]::keybd_event($VK_UP, 0, 3, [UIntPtr]::Zero)
    }
}
function Get-MelonWindow {
    param(
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    while ((Get-Date) -lt $Deadline) {
        $Process.Refresh()
        if ($Process.HasExited) { throw 'melonDS exited before opening a window.' }
        if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
            return $Process.MainWindowHandle
        }
        Start-Sleep -Milliseconds 50
    }
    throw 'Timed out waiting for the melonDS window.'
}

$commands = @'
set pagination off
set confirm off
set remotetimeout 20
target remote 127.0.0.1:__PORT__
set $ness = 0
set $starts = 0
set $air_starts = 0
set $holds = 0
set $jibaku = 0
set $jibaku_start = 0
set $jibaku_updates = 0
set $steer_frames = 0
set $completed = 0

break __excpt_entry
commands
silent
printf "UPB_EXCEPTION pc=%#x lr=%#x starts=%u holds=%u jibaku=%u completed=%u\n", $pc,$lr,$starts,$holds,$jibaku,$completed
bt 12
shell cmd /c echo fault>__FAULT__
detach
quit
end

set $ness = sNdsFighterManagerLiveGObjs[0]
if $ness != 0
printf "UPB_NESS_READY gobj=%p fkind=%d status=%d\n", $ness, ((FTStruct *)$ness->user_data.p)->fkind, ((FTStruct *)$ness->user_data.p)->status_id
if ((FTStruct *)$ness->user_data.p)->status_id == 10
shell cmd /c echo ready>__IDLE__
end
end

# Full-content builds can still be streaming fighter data eight host seconds
# after boot.  If player 0 does not exist at attach time, observe its natural
# construction and bind the exact Ness GObj there; do not write guest state.
break ftParamInitPlayerBattleStats if player == 0 && fighter_gobj != 0 && ((FTStruct *)fighter_gobj->user_data.p)->fkind == 11
commands
silent
if $ness == 0
set $ness = fighter_gobj
printf "UPB_NESS_READY gobj=%p fkind=%d status=%d late=1\n", $ness, ((FTStruct *)$ness->user_data.p)->fkind, ((FTStruct *)$ness->user_data.p)->status_id
if ((FTStruct *)$ness->user_data.p)->status_id == 10
shell cmd /c echo ready>__IDLE__
end
end
continue
end

break ftNessSpecialHiStartSetStatus if $r0 == $ness
commands
silent
set $starts = $starts + 1
printf "UPB_START count=%u air=0 status=%d\n", $starts, ((FTStruct *)$ness->user_data.p)->status_id
shell cmd /c echo ready>__START__
continue
end
break ftNessSpecialAirHiStartSetStatus if $r0 == $ness
commands
silent
set $starts = $starts + 1
set $air_starts = $air_starts + 1
printf "UPB_START count=%u air=1 status=%d\n", $starts, ((FTStruct *)$ness->user_data.p)->status_id
shell cmd /c echo ready>__START__
shell cmd /c echo ready>__AIRSTART__
continue
end

break ndsBaseFTCommonJumpAerialSetStatus if $r0 == $ness
commands
silent
printf "UPB_DOUBLE_JUMP status=%d jumps=%d\n", ((FTStruct *)$ness->user_data.p)->status_id, ((FTStruct *)$ness->user_data.p)->jumps_used
shell cmd /c echo ready>__AIRJUMP__
continue
end

break ftNessSpecialHiHoldSetStatus if $r0 == $ness
commands
silent
set $holds = $holds + 1
set $steer_frames = 0
printf "UPB_HOLD count=%u air=0\n", $holds
shell cmd /c echo ready>__HOLD__
continue
end
break ftNessSpecialAirHiHoldSetStatus if $r0 == $ness
commands
silent
set $holds = $holds + 1
set $steer_frames = 0
printf "UPB_HOLD count=%u air=1\n", $holds
shell cmd /c echo ready>__HOLD__
continue
end

break wpNessPKThunderHeadProcUpdate
commands
silent
set $steer_frames = $steer_frames + 1
if $steer_frames == 7
shell cmd /c echo ready>__STEER1__
end
if $steer_frames == 15
shell cmd /c echo ready>__STEER2__
end
if $steer_frames == 22
shell cmd /c echo ready>__STEER3__
end
if $steer_frames == 30
shell cmd /c echo ready>__STEER4__
end
if $steer_frames == 37
shell cmd /c echo ready>__STEER5__
end
if $steer_frames == 45
shell cmd /c echo ready>__STEER6__
if ((DObj *)((GObj *)$ness)->obj)->translate.vec.f.y >= ((DObj *)((GObj *)$r0)->obj)->translate.vec.f.y
shell cmd /c echo ready>__STEER_HIGH__
else
shell cmd /c echo ready>__STEER_FALL__
end
end
if $steer_frames == 52
shell cmd /c echo ready>__STEER7__
end
if $steer_frames == 60
shell cmd /c echo ready>__STEER8__
end
continue
end

break ftNessSpecialHiJibakuSetStatus if $r0 == $ness
commands
silent
if $jibaku_start < $starts
set $jibaku_start = $starts
set $jibaku = $jibaku + 1
set $jibaku_updates = 0
printf "UPB_JIBAKU count=%u air=0\n", $jibaku
shell cmd /c echo ready>__JIBAKU__
end
continue
end
break ftNessSpecialAirHiJibakuSetStatus if $r0 == $ness
commands
silent
if $jibaku_start < $starts
set $jibaku_start = $starts
set $jibaku = $jibaku + 1
set $jibaku_updates = 0
printf "UPB_JIBAKU count=%u air=1\n", $jibaku
shell cmd /c echo ready>__JIBAKU__
end
continue
end

break ftNessSpecialAirHiJibakuProcUpdate if $r0 == $ness
commands
silent
set $jibaku_updates = $jibaku_updates + 1
if $jibaku_updates == 3
printf "UPB_JIBAKU_LIVE updates=%u\n", $jibaku_updates
shell cmd /c echo ready>__JIBAKU_LIVE__
disable $_hit_bpnum
end
continue
end

break ftCommonWaitSetStatus if $r0 == $ness
commands
silent
if $starts == 0
printf "UPB_IDLE status=%d\n", ((FTStruct *)$ness->user_data.p)->status_id
shell cmd /c echo ready>__IDLE__
end
if $starts > $completed
set $completed = $starts
printf "UPB_COMPLETE count=%u holds=%u jibaku=%u\n", $completed,$holds,$jibaku
shell cmd /c echo ready>__COMPLETE__
if (__SELFHIT__ == 0 && $completed >= __ATTEMPTS__) || (__SELFHIT__ != 0 && $jibaku >= __ATTEMPTS__)
printf "UPB_FINAL starts=%u holds=%u jibaku=%u completed=%u\n", $starts,$holds,$jibaku,$completed
shell cmd /c echo ready>__DONE__
detach
quit
end
end
continue
end
continue
'@
$commands = $commands.Replace('__PORT__', [string]$context.GdbPort)
$commands = $commands.Replace('__FAULT__', (Gdb-Path 'fault'))
$commands = $commands.Replace('__IDLE__', (Gdb-Path 'idle'))
$commands = $commands.Replace('__START__', (Gdb-Path 'start'))
$commands = $commands.Replace('__AIRSTART__', (Gdb-Path 'airstart'))
$commands = $commands.Replace('__AIRJUMP__', (Gdb-Path 'airjump'))
$commands = $commands.Replace('__HOLD__', (Gdb-Path 'hold'))
$commands = $commands.Replace('__JIBAKU__', (Gdb-Path 'jibaku'))
$commands = $commands.Replace('__JIBAKU_LIVE__', (Gdb-Path 'jibaku-live'))
$commands = $commands.Replace('__STEER1__', (Gdb-Path 'steer1'))
$commands = $commands.Replace('__STEER2__', (Gdb-Path 'steer2'))
$commands = $commands.Replace('__STEER3__', (Gdb-Path 'steer3'))
$commands = $commands.Replace('__STEER4__', (Gdb-Path 'steer4'))
$commands = $commands.Replace('__STEER5__', (Gdb-Path 'steer5'))
$commands = $commands.Replace('__STEER6__', (Gdb-Path 'steer6'))
$commands = $commands.Replace('__STEER7__', (Gdb-Path 'steer7'))
$commands = $commands.Replace('__STEER8__', (Gdb-Path 'steer8'))
$commands = $commands.Replace('__STEER_HIGH__', (Gdb-Path 'steer-high'))
$commands = $commands.Replace('__STEER_FALL__', (Gdb-Path 'steer-fall'))
$commands = $commands.Replace('__COMPLETE__', (Gdb-Path 'complete'))
$commands = $commands.Replace('__DONE__', (Gdb-Path 'done'))
$commands = $commands.Replace('__ATTEMPTS__', [string]$Attempts)
$commands = $commands.Replace('__SELFHIT__', $(if ($SelfHit) { '1' } else { '0' }))
Set-Content -LiteralPath $gdbScript -Value $commands

$configState = $null
$emulator = $null
$gdbProcess = $null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -MuteAudio
    if ($SelfHit) {
        $runnerConfig = Get-Content -LiteralPath $configState.Config -Raw
        $runnerConfig = Set-MelonDSTomlRootValue -Text $runnerConfig `
            -Key 'TargetFPS' -Value '60.0'
        $runnerConfig = Set-MelonDSTomlRootValue -Text $runnerConfig `
            -Key 'LimitFPS' -Value 'true'
        Set-Content -LiteralPath $configState.Config -Value $runnerConfig -NoNewline
    }
    Remove-Item -LiteralPath $stdout,$stderr,$gdbOut,$gdbErr -Force `
        -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory $melonDir -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Minimized -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    # Let the realtime battle create both fighters before attaching. GDB is an
    # observer only; no guest state is written. This also avoids missing the
    # fighter-construction breakpoint when melonDS outruns host GDB startup.
    Start-Sleep -Seconds 8
    $gdbProcess = Start-Process -FilePath $gdb `
        -ArgumentList @('-batch', $elf, '-x', $gdbScript) `
        -RedirectStandardOutput $gdbOut -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    $window = Get-MelonWindow -Process $emulator -Deadline $deadline

    # Wait for the source Wait setter. This proves the realtime fighter is
    # alive and controllable without depending on an unrelated platform-input
    # helper that is not called every battle frame.
    Wait-Stage -Name 'idle' -Emulator $emulator `
        -Deadline ((Get-Date).AddSeconds(30))
    # The marker is written from the source Wait setter breakpoint before the
    # setter body returns. Let GDB resume and one ordinary controller sample run
    # before issuing the first host chord.
    Start-Sleep -Milliseconds 400
    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        Remove-Item -LiteralPath (Stage-Path 'start'),(Stage-Path 'airstart'),(Stage-Path 'airjump'),(Stage-Path 'hold'),
            (Stage-Path 'jibaku'),(Stage-Path 'jibaku-live'),(Stage-Path 'complete'),
            (Stage-Path 'steer1'),(Stage-Path 'steer2'),(Stage-Path 'steer3'),(Stage-Path 'steer4'),
            (Stage-Path 'steer5'),(Stage-Path 'steer6'),(Stage-Path 'steer7'),(Stage-Path 'steer8'),
            (Stage-Path 'steer-high'),(Stage-Path 'steer-fall') `
            -Force -ErrorAction SilentlyContinue
        # Fox is the real level-3 CPU in this realtime build. If a hit wins the
        # input race, retry the same real host keypress; only a source Up-B
        # setter hit counts as an attempt.
        $started = $false
        for ($press = 1; $press -le 10; $press++) {
            Remove-Item -LiteralPath (Stage-Path 'start'),(Stage-Path 'airstart'),(Stage-Path 'airjump'),
                (Stage-Path 'hold'),(Stage-Path 'complete') -Force -ErrorAction SilentlyContinue
            if ($SelfHit) {
                # Give the PK Thunder return circle enough vertical clearance
                # with two ordinary stick-up jumps. The second one must hit the
                # source aerial-jump setter before Up-B is allowed to count.
                Send-HeldKeys -Window $window -ProcessId $emulator.Id `
                    -Keys ([byte[]]@($VK_UP)) -HoldMilliseconds 80
                Start-Sleep -Milliseconds 220
                Send-HeldKeys -Window $window -ProcessId $emulator.Id `
                    -Keys ([byte[]]@($VK_UP)) -HoldMilliseconds 80
                try {
                    Wait-Stage -Name 'airjump' -Emulator $emulator `
                        -Deadline ((Get-Date).AddMilliseconds(1400))
                } catch {
                    Start-Sleep -Milliseconds 180
                    continue
                }
                Start-Sleep -Milliseconds 80
            }
            Send-UpB -Window $window -ProcessId $emulator.Id
            $pressDeadline = (Get-Date).AddMilliseconds(700)
            while ((Get-Date) -lt $pressDeadline) {
                if ((-not $SelfHit) -and
                    (Test-Path -LiteralPath (Stage-Path 'start') -PathType Leaf)) {
                    $started = $true
                    break
                }
                if ($SelfHit -and
                    (Test-Path -LiteralPath (Stage-Path 'airstart') -PathType Leaf)) {
                    $started = $true
                    break
                }
                if ($SelfHit -and
                    (Test-Path -LiteralPath (Stage-Path 'start') -PathType Leaf)) {
                    Wait-Stage -Name 'complete' -Emulator $emulator `
                        -Deadline ((Get-Date).AddSeconds(5))
                    break
                }
                if (Test-Path -LiteralPath (Stage-Path 'fault') -PathType Leaf) {
                    throw 'ARM exception occurred during real Ness Up-B input.'
                }
                $emulator.Refresh()
                if ($emulator.HasExited) {
                    throw "melonDS exited during real Ness Up-B input (code $($emulator.ExitCode))."
                }
                Start-Sleep -Milliseconds 20
            }
            if ($started) { break }
            Start-Sleep -Milliseconds 180
        }
        if (-not $started) {
            throw "Ten real Up+B presses failed to enter source Up-B for attempt $attempt."
        }
        if ($SelfHit) {
            Wait-Stage -Name 'hold' -Emulator $emulator `
                -Deadline ((Get-Date).AddSeconds(3))
            # Initial head velocity is straight up. Alternate seven/eight guest
            # updates per 45-degree sector until frame 45. At that measured
            # point the live head is moving down-left; releasing the stick lets
            # source velocity carry it through Ness's falling self-hit box near
            # frame 55-60. GDB only counts updates; every steering change is a
            # real host D-pad key transition.
            $sectors = @(
                @{ Keys = [byte[]]@($VK_RIGHT);          Marker = 'steer1' },
                @{ Keys = [byte[]]@($VK_RIGHT,$VK_DOWN); Marker = 'steer2' },
                @{ Keys = [byte[]]@($VK_DOWN);           Marker = 'steer3' },
                @{ Keys = [byte[]]@($VK_DOWN,$VK_LEFT);  Marker = 'steer4' },
                @{ Keys = [byte[]]@($VK_LEFT);           Marker = 'steer5' },
                @{ Keys = [byte[]]@($VK_LEFT,$VK_UP);    Marker = 'steer6' }
            )
            Focus-Melon -Window $window -ProcessId $emulator.Id
            foreach ($sector in $sectors) {
                Set-KeysDown -Keys $sector.Keys
                try {
                    $steerResult = Wait-SteerOrJibaku -Name $sector.Marker `
                        -Emulator $emulator -Deadline ((Get-Date).AddSeconds(5))
                } finally {
                    Set-KeysUp -Keys $sector.Keys
                }
                if ($steerResult -eq 'jibaku') { break }
            }
            if (-not (Test-Path -LiteralPath (Stage-Path 'jibaku') -PathType Leaf)) {
                # On the upper platform the fighter remains above the head at
                # frame 45, so finish the measured full return circle. If Ness
                # is below the head he is still falling; neutral coasting from
                # frame 45 crosses that moving self-hit box instead.
                $routeDeadline = (Get-Date).AddSeconds(2)
                while ((Get-Date) -lt $routeDeadline) {
                    if ((Test-Path -LiteralPath (Stage-Path 'steer-high') -PathType Leaf) -or
                        (Test-Path -LiteralPath (Stage-Path 'steer-fall') -PathType Leaf)) {
                        break
                    }
                    if (Test-Path -LiteralPath (Stage-Path 'fault') -PathType Leaf) {
                        throw 'ARM exception occurred while classifying PK Thunder return route.'
                    }
                    $emulator.Refresh()
                    if ($emulator.HasExited) {
                        throw "melonDS exited while classifying PK Thunder return route (code $($emulator.ExitCode))."
                    }
                    Start-Sleep -Milliseconds 1
                }
                if (Test-Path -LiteralPath (Stage-Path 'steer-high') -PathType Leaf) {
                    $finishSectors = @(
                        @{ Keys = [byte[]]@($VK_UP);           Marker = 'steer7' },
                        @{ Keys = [byte[]]@($VK_UP,$VK_RIGHT); Marker = 'steer8' }
                    )
                    foreach ($sector in $finishSectors) {
                        Set-KeysDown -Keys $sector.Keys
                        try {
                            $steerResult = Wait-SteerOrJibaku -Name $sector.Marker `
                                -Emulator $emulator -Deadline ((Get-Date).AddSeconds(5))
                        } finally {
                            Set-KeysUp -Keys $sector.Keys
                        }
                        if ($steerResult -eq 'jibaku') { break }
                    }
                } elseif (-not (Test-Path -LiteralPath (Stage-Path 'steer-fall') -PathType Leaf)) {
                    throw 'PK Thunder frame-45 route classifier did not publish a state.'
                }
            }
            Wait-Stage -Name 'jibaku' -Emulator $emulator `
                -Deadline ((Get-Date).AddSeconds(3))
            if ($attempt -eq 1) {
                Wait-Stage -Name 'jibaku-live' -Emulator $emulator `
                    -Deadline ((Get-Date).AddSeconds(3))
            }
        }
        Wait-Stage -Name 'complete' -Emulator $emulator `
            -Deadline ((Get-Date).AddSeconds(20))
        Write-Output "REAL_UPB_ATTEMPT=$attempt PASS"
        Start-Sleep -Milliseconds 250
    }
    Wait-Stage -Name 'done' -Emulator $emulator -Deadline $deadline
    while ((Get-Date) -lt $deadline) {
        $gdbProcess.Refresh()
        if ($gdbProcess.HasExited) { break }
        Start-Sleep -Milliseconds 20
    }
    $gdbProcess.Refresh()
    if (-not $gdbProcess.HasExited) { throw 'Ness real-input GDB observer did not exit.' }
    if ($gdbProcess.ExitCode -ne 0) {
        $err = if (Test-Path $gdbErr) { Get-Content $gdbErr -Raw } else { '' }
        throw "Ness real-input GDB observer exited $($gdbProcess.ExitCode): $err"
    }
}
finally {
    if ($gdbProcess -and (-not $gdbProcess.HasExited)) {
        Stop-Process -Id $gdbProcess.Id -Force -ErrorAction SilentlyContinue
    }
    if ($emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}

$trace = Get-Content -LiteralPath $gdbOut -Raw
if ($trace -match 'UPB_EXCEPTION') { throw 'Ness real-input trace contains an ARM exception.' }
$final = [regex]::Match($trace, 'UPB_FINAL starts=(\d+) holds=(\d+) jibaku=(\d+) completed=(\d+)')
if (-not $final.Success) { throw 'Ness real-input trace has no final completion marker.' }
if ($SelfHit) {
    if (([int]$final.Groups[2].Value -lt $Attempts) -or
        ([int]$final.Groups[3].Value -lt $Attempts) -or
        ([int]$final.Groups[4].Value -ne [int]$final.Groups[1].Value)) {
        throw "Ness real-input self-hit did not complete every qualified Up-B: $($final.Value)"
    }
} elseif (([int]$final.Groups[1].Value -ne $Attempts) -or
          ([int]$final.Groups[2].Value -lt $Attempts) -or
          ([int]$final.Groups[4].Value -ne $Attempts)) {
    throw "Ness real-input counts did not complete every Up-B: $($final.Value)"
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
@(
    ('NESS_UPB_INPUT=' + $(if ($SelfHit) {
        'real X jump + VK_UP/S + real D-pad PK Thunder steering'
    } else {
        'real VK_UP + S key input'
    })),
    "SELF_HIT=$([int][bool]$SelfHit)",
    "ATTEMPTS=$Attempts",
    "ROM_SHA256=$((Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash)",
    "BUILD=$Build",
    "TARGET=$Target",
    $final.Value
) | Set-Content -LiteralPath $Artifact
Get-Content -LiteralPath $gdbOut |
    Where-Object { $_ -match '^UPB_' } |
    Add-Content -LiteralPath $Artifact
Get-Content -LiteralPath $Artifact
Write-Output "Ness real-input Up-B proof: PASS ($Artifact)"
