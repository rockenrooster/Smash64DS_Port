[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Build,
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(60, 900)][int]$TimeoutSeconds = 300,
    [ValidateRange(100, 1000)][int]$WalkMilliseconds = 350,
    [ValidateRange(500, 4000)][int]$InhaleMilliseconds = 1800,
    [string]$Artifact = '',
    [string]$Screenshot = ''
)

# P2-3f47 final Kirby copy-hat proof.
#
# Input is ordinary host keyboard input delivered to melonDS using the same
# checked-in mapping as probe-samus-owner.ps1:
#   Left/Right/Up -> DS D-pad
#   S     -> DS B -> source B_BUTTON
#
# GDB only observes state and creates stage files so host input is sent at a
# deterministic time. It never calls a status function and never writes
# status/motion/copy_kind. BattleShip owns walk, Inhale, catch/eat, CopyLink and
# copied Link SpecialN from beginning to end.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\melonds-screenshot.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

if (-not ('Smash64DSKirbyCopyInput' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSKirbyCopyInput
{
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool PostMessage(
        IntPtr window, uint message, UIntPtr wParam, IntPtr lParam);
}
'@
}

$VK_S = [byte]0x53
$VK_LEFT = [byte]0x25
$VK_UP = [byte]0x26
$VK_RIGHT = [byte]0x27
$SCAN_LEFT = [byte]0x4B
$SCAN_UP = [byte]0x48
$SCAN_RIGHT = [byte]0x4D
$WM_KEYDOWN = [uint32]0x0100
$WM_KEYUP = [uint32]0x0101

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$buildConfig = Join-Path (Split-Path -Parent $rom) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "Kirby CopyLink proof requires generated build config: $buildConfig"
}
$config = Get-Content -LiteralPath $buildConfig -Raw
foreach ($required in @(
    'NDS_P2_KIRBY 1',
    'NDS_P2_LINK 1',
    'NDS_P2_KIRBY_COPYLINK_PROOF 1',
    'NDS_NATIVE_OWNER_IMAGE_VERIFY 1'
)) {
    if ($config -notmatch ('(?m)^#define ' + [regex]::Escape($required) + '$')) {
        throw "Kirby CopyLink proof requires build config '$required'."
    }
}
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\visibility\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-kirby-copylink-owner.txt')
}
if ([string]::IsNullOrWhiteSpace($Screenshot)) {
    $Screenshot = Join-Path $root ('artifacts\visibility\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-kirby-copylink-hat.png')
}

$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' `
    -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$tempDir = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root ('artifacts\verifier-temp\slot' + $RunnerSlot)
}
New-Item -ItemType Directory -Force -Path $tempDir,$logDir | Out-Null

$stdout = Join-Path $logDir 'melonds.kirby-copylink.stdout.log'
$stderr = Join-Path $logDir 'melonds.kirby-copylink.stderr.log'
$gdbScript = Join-Path $tempDir 'kirby_copylink_hostkeys.gdb'
$gdbOut = Join-Path $tempDir 'kirby_copylink_hostkeys.gdb.out'
$gdbErr = Join-Path $tempDir 'kirby_copylink_hostkeys.gdb.err'
$stageDir = Join-Path $tempDir 'kirby-copylink-stages'
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Get-ChildItem -LiteralPath $stageDir -File -ErrorAction SilentlyContinue |
    Remove-Item -Force

function Get-StagePath([string]$Name) {
    Join-Path $stageDir ($Name + '.ready')
}
function Get-GdbStagePath([string]$Name) {
    ((Get-StagePath $Name) -replace '\\', '/')
}
function Wait-Stage {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$GdbProcess,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    $path = Get-StagePath $Name
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path -LiteralPath $path -PathType Leaf) { return }
        $GdbProcess.Refresh()
        if ($GdbProcess.HasExited) {
            $out = if (Test-Path $gdbOut) { Get-Content $gdbOut -Raw } else { '' }
            $err = if (Test-Path $gdbErr) { Get-Content $gdbErr -Raw } else { '' }
            throw "GDB exited before Kirby stage '$Name'. OUT=$out ERR=$err"
        }
        Start-Sleep -Milliseconds 20
    }
    throw "Timed out waiting for Kirby CopyLink stage '$Name'."
}
function Get-MelonWindow {
    param(
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    while ((Get-Date) -lt $Deadline) {
        $Process.Refresh()
        if ($Process.HasExited) {
            throw "melonDS exited before its Kirby proof window appeared (code $($Process.ExitCode))."
        }
        if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
            return $Process.MainWindowHandle
        }
        Start-Sleep -Milliseconds 50
    }
    throw 'Timed out waiting for the melonDS Kirby proof window.'
}
function Send-MelonKey {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Window,
        [Parameter(Mandatory=$true)][byte]$Key,
        [ValidateRange(80, 5000)][int]$HoldMilliseconds = 200,
        [switch]$Extended,
        [byte]$Scan = 0
    )
    # Send the keyboard message to melonDS itself. Global keybd_event requires
    # stealing desktop focus and can hit an unrelated foreground application.
    # Qt receives these as the same ordinary key down/up events while the proof
    # stays isolated to its emulator window.
    [void][Smash64DSKirbyCopyInput]::ShowWindow($Window, 9)
    $downLParam = [int64]1 -bor ([int64]$Scan -shl 16)
    if ($Extended) { $downLParam = $downLParam -bor [int64]0x01000000 }
    if (-not [Smash64DSKirbyCopyInput]::PostMessage(
            $Window, $WM_KEYDOWN, [UIntPtr]$Key, [IntPtr]$downLParam)) {
        throw "Could not deliver key-down 0x$($Key.ToString('x2')) to melonDS."
    }
    try { Start-Sleep -Milliseconds $HoldMilliseconds }
    finally {
        $upLParam = $downLParam -bor [int64]0xC0000000
        if (-not [Smash64DSKirbyCopyInput]::PostMessage(
                $Window, $WM_KEYUP, [UIntPtr]$Key, [IntPtr]$upLParam)) {
            throw "Could not deliver key-up 0x$($Key.ToString('x2')) to melonDS."
        }
    }
}

$commands = @'
set pagination off
set confirm off
set remotetimeout 20
target remote 127.0.0.1:__PORT__
set $kirby_gobj = 0
set $link_gobj = 0
set $walk_done = 0
set $special_staged = 0
set $hat_high = 0
set $hat_low = 0
set $copy_committed = 0
set $copy_program_sets = 0
set $copy_last_program = 0
set $boom_made = 0
set $boom_base = 0
set $hat_load_base = gNdsNativeKirbyHatLoadCount
set $hat_fail_base = gNdsNativeKirbyHatFailCount
set $native_fail_base = gNdsRendererNativeFailure.count
set $fighter_reject_base = gNdsFtrRejectCountBySlot[0]

break ftCommonWaitProcInterrupt
commands
silent
if (((FTStruct *)fighter_gobj->user_data.p)->fkind != 8 || ((FTStruct *)fighter_gobj->user_data.p)->pkind != 0 || ((FTStruct *)fighter_gobj->user_data.p)->is_control_disable != 0 || $kirby_gobj != 0)
continue
end
set $kirby_gobj = fighter_gobj
set $link_gobj = sNdsFighterManagerLiveGObjs[1]
printf "TRACE INPUT_READY kirby=%p link=%p kx=%f lx=%f gap=%f status=%d copy=%d\n", $kirby_gobj, $link_gobj, ((DObj *)$kirby_gobj->obj)->translate.vec.f.x, ((DObj *)$link_gobj->obj)->translate.vec.f.x, ((DObj *)$link_gobj->obj)->translate.vec.f.x - ((DObj *)$kirby_gobj->obj)->translate.vec.f.x, ((FTStruct *)$kirby_gobj->user_data.p)->status_id, ((FTStruct *)$kirby_gobj->user_data.p)->passive_vars.kirby.copy_id
shell cmd /c echo ready>__WALK__
continue
end

break ftCommonWaitSetStatus
commands
silent
if fighter_gobj != $kirby_gobj
continue
end
if $walk_done == 0
set $walk_done = 1
printf "TRACE AFTER_WALK kx=%f lx=%f gap=%f lr=%d\n", ((DObj *)$kirby_gobj->obj)->translate.vec.f.x, ((DObj *)$link_gobj->obj)->translate.vec.f.x, ((DObj *)$link_gobj->obj)->translate.vec.f.x - ((DObj *)$kirby_gobj->obj)->translate.vec.f.x, ((FTStruct *)$kirby_gobj->user_data.p)->lr
shell cmd /c echo ready>__INHALE__
continue
end
if $special_staged == 0 && ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.kirby.copy_id == 5 && $hat_high != 0 && $hat_low != 0
set $special_staged = 1
printf "TRACE COPY_READY copy=%d high=%u low=%u programSets=%u lastProgram=%u loads=%u fails=%u validate_latch=%u\n", ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.kirby.copy_id, $hat_high, $hat_low, $copy_program_sets, $copy_last_program, gNdsNativeKirbyHatLoadCount - $hat_load_base, gNdsNativeKirbyHatFailCount - $hat_fail_base, gNdsNativeFighterValidateRejectCode
set $boom_base = gNdsEntryEffectNativeRootDraws[27]
shell cmd /c echo ready>__SPECIAL__
continue
end
continue
end

break ftKirbySpecialNStartSetStatus
commands
silent
if fighter_gobj == $kirby_gobj
printf "TRACE INHALE_START status=%d kx=%f lx=%f gap=%f\n", ((FTStruct *)fighter_gobj->user_data.p)->status_id, ((DObj *)$kirby_gobj->obj)->translate.vec.f.x, ((DObj *)$link_gobj->obj)->translate.vec.f.x, ((DObj *)$link_gobj->obj)->translate.vec.f.x - ((DObj *)$kirby_gobj->obj)->translate.vec.f.x
end
continue
end

break ftKirbySpecialNCatchProcCatch
commands
silent
if fighter_gobj == $kirby_gobj
printf "TRACE CAUGHT search=%p victim_kind=%d kx=%f vx=%f\n", ((FTStruct *)fighter_gobj->user_data.p)->search_gobj, ((FTStruct *)((FTStruct *)fighter_gobj->user_data.p)->search_gobj->user_data.p)->fkind, ((DObj *)fighter_gobj->obj)->translate.vec.f.x, ((DObj *)((FTStruct *)fighter_gobj->user_data.p)->search_gobj->obj)->translate.vec.f.x
end
continue
end

break ftKirbySpecialNWaitSetStatusFromEat
commands
silent
if fighter_gobj != $kirby_gobj
continue
end
printf "TRACE SWALLOWED catch=%p copy_candidate=%d status=%d\n", ((FTStruct *)fighter_gobj->user_data.p)->catch_gobj, ((FTStruct *)fighter_gobj->user_data.p)->status_vars.kirby.specialn.copy_id, ((FTStruct *)fighter_gobj->user_data.p)->status_id
shell cmd /c echo ready>__COPY__
continue
end

break ftKirbySpecialNCopyInitCopyVars
commands
silent
if fighter_gobj == $kirby_gobj && ((FTStruct *)fighter_gobj->user_data.p)->motion_vars.flags.flag1 != 0 && ((FTStruct *)fighter_gobj->user_data.p)->status_vars.kirby.specialn.copy_id == 5
set $copy_committed = 1
printf "TRACE COPY_COMMIT candidate=%d prior=%d\n", ((FTStruct *)fighter_gobj->user_data.p)->status_vars.kirby.specialn.copy_id, ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.kirby.copy_id
end
continue
end

break ndsRendererNativeEnsureKirbyCopyHat
commands
silent
if copy_modelpart_id != 10
continue
end
if use_low_detail == 0
set $hat_high = $hat_high + 1
else
set $hat_low = $hat_low + 1
end
printf "TRACE HAT detail=%u slot=%u modelpart=%u loads=%u fails=%u high=%u low=%u\n", use_low_detail, battle_slot, copy_modelpart_id, gNdsNativeKirbyHatLoadCount - $hat_load_base, gNdsNativeKirbyHatFailCount - $hat_fail_base, $hat_high, $hat_low
continue
end

break ndsRendererNativeFighterSetRootProgram
commands
silent
if slot == 11 && $copy_committed != 0 && program != 0
set $copy_program_sets = $copy_program_sets + 1
set $copy_last_program = program
end
continue
end

break ftKirbyCopyLinkSpecialNSetStatus
commands
silent
if fighter_gobj == $kirby_gobj
printf "TRACE COPYLINK_SPECIAL status=%d copy=%d\n", ((FTStruct *)fighter_gobj->user_data.p)->status_id, ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.kirby.copy_id
end
continue
end

break wpLinkBoomerangMakeWeapon
commands
silent
if fighter_gobj != $kirby_gobj
continue
end
set $boom_made = $boom_made + 1
printf "TRACE BOOMERANG_MAKE made=%u root27=%u base=%u\n", $boom_made, gNdsEntryEffectNativeRootDraws[27], $boom_base
continue
end

break ndsPlatformEndFrame
commands
silent
if $boom_made == 0 || gNdsEntryEffectNativeRootDraws[27] <= $boom_base
continue
end
printf "TRACE FINAL copy=%d high=%u low=%u programSets=%u lastProgram=%u boom=%u root27delta=%u hatLoads=%u hatFails=%u nativeFailDelta=%u fighterRejectDelta=%u validate_latch=%u\n", ((FTStruct *)$kirby_gobj->user_data.p)->passive_vars.kirby.copy_id, $hat_high, $hat_low, $copy_program_sets, $copy_last_program, $boom_made, gNdsEntryEffectNativeRootDraws[27] - $boom_base, gNdsNativeKirbyHatLoadCount - $hat_load_base, gNdsNativeKirbyHatFailCount - $hat_fail_base, gNdsRendererNativeFailure.count - $native_fail_base, gNdsFtrRejectCountBySlot[0] - $fighter_reject_base, gNdsNativeFighterValidateRejectCode
detach
quit
end
continue
'@
$commands = $commands.Replace('__PORT__', [string]$context.GdbPort)
$commands = $commands.Replace('__WALK__', (Get-GdbStagePath 'walk'))
$commands = $commands.Replace('__INHALE__', (Get-GdbStagePath 'inhale'))
$commands = $commands.Replace('__COPY__', (Get-GdbStagePath 'copy'))
$commands = $commands.Replace('__SPECIAL__', (Get-GdbStagePath 'special'))
Set-Content -LiteralPath $gdbScript -Value $commands

$configState = $null
$emulator = $null
$gdbProcess = $null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout,$stderr,$gdbOut,$gdbErr -Force `
        -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory $melonDir -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $gdbProcess = Start-Process -FilePath $gdb `
        -ArgumentList @('-batch', $elf, '-x', $gdbScript) `
        -RedirectStandardOutput $gdbOut -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    $window = Get-MelonWindow -Process $emulator -Deadline $deadline

    Wait-Stage -Name 'walk' -GdbProcess $gdbProcess -Deadline $deadline
    $readyText = Get-Content -LiteralPath $gdbOut -Raw
    $readyMatch = [regex]::Match($readyText, 'TRACE INPUT_READY .*gap=(-?[0-9]+(?:\.[0-9]+)?)')
    if (-not $readyMatch.Success) {
        throw "Kirby CopyLink proof could not read the measured spawn gap:`n$readyText"
    }
    $spawnGap = [double]$readyMatch.Groups[1].Value
    $walkKey = if ($spawnGap -lt 0.0) { $VK_LEFT } else { $VK_RIGHT }
    $walkScan = if ($spawnGap -lt 0.0) { $SCAN_LEFT } else { $SCAN_RIGHT }
    Start-Sleep -Milliseconds 100
    Send-MelonKey -Window $window -Key $walkKey -Scan $walkScan -Extended `
        -HoldMilliseconds $WalkMilliseconds

    Wait-Stage -Name 'inhale' -GdbProcess $gdbProcess -Deadline $deadline
    # Dream Land spawns player 1 on the left platform. The first walk gets
    # Kirby under its right edge; approach a little farther, then use the
    # ordinary Up-stick jump so Inhale is attempted on the same platform as
    # Link instead of from the floor below him.
    Send-MelonKey -Window $window -Key $walkKey -Scan $walkScan -Extended `
        -HoldMilliseconds ([Math]::Max(80, [int]($WalkMilliseconds / 2)))
    Start-Sleep -Milliseconds 100
    Send-MelonKey -Window $window -Key $VK_UP -Scan $SCAN_UP -Extended `
        -HoldMilliseconds 250
    Start-Sleep -Milliseconds 900
    Start-Sleep -Milliseconds 100
    Send-MelonKey -Window $window -Key $VK_S -HoldMilliseconds $InhaleMilliseconds

    Wait-Stage -Name 'copy' -GdbProcess $gdbProcess -Deadline $deadline
    Start-Sleep -Milliseconds 150
    Send-MelonKey -Window $window -Key $VK_S -HoldMilliseconds 180

    Wait-Stage -Name 'special' -GdbProcess $gdbProcess -Deadline $deadline
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Screenshot) |
        Out-Null
    $captureSize = Save-MelonDSWindowCapture -WindowHandle $window `
        -Path $Screenshot -PreferPrintWindow
    Start-Sleep -Milliseconds 150
    Send-MelonKey -Window $window -Key $VK_S -HoldMilliseconds 180

    while ((Get-Date) -lt $deadline) {
        $gdbProcess.Refresh()
        if ($gdbProcess.HasExited) { break }
        Start-Sleep -Milliseconds 50
    }
    $gdbProcess.Refresh()
    if (-not $gdbProcess.HasExited) {
        throw "Kirby CopyLink GDB trace exceeded $TimeoutSeconds seconds."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        $err = if (Test-Path $gdbErr) { Get-Content $gdbErr -Raw } else { '' }
        throw "Kirby CopyLink GDB exited $($gdbProcess.ExitCode): $err"
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

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
$hash = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
@(
    'KIRBY_COPYLINK_HOST_INPUT=melonDS window-targeted Left/Right/Up/S key messages only',
    "KIRBY_COPYLINK_WALK_MS=$WalkMilliseconds",
    "KIRBY_COPYLINK_SPAWN_GAP=$spawnGap",
    "KIRBY_COPYLINK_INHALE_MS=$InhaleMilliseconds",
    "KIRBY_COPYLINK_SCREENSHOT=$Screenshot",
    "KIRBY_COPYLINK_SCREENSHOT_SIZE=$captureSize",
    "ROM_SHA256=$hash",
    "BUILD=$Build",
    "TARGET=$Target"
) | Set-Content -LiteralPath $Artifact
Get-Content -LiteralPath $gdbOut | Add-Content -LiteralPath $Artifact

$text = Get-Content -LiteralPath $Artifact -Raw
foreach ($required in @(
    'TRACE INPUT_READY .*copy=8',
    'TRACE AFTER_WALK ',
    'TRACE SWALLOWED .*copy_candidate=5',
    'TRACE COPY_COMMIT candidate=5',
    'TRACE HAT detail=0 .*modelpart=10',
    'TRACE HAT detail=1 .*modelpart=10',
    'TRACE COPY_READY copy=5 .*fails=0',
    'TRACE COPYLINK_SPECIAL .*copy=5',
    'TRACE BOOMERANG_MAKE made=[1-9]',
    'TRACE FINAL copy=5 high=[1-9][0-9]* low=[1-9][0-9]* programSets=[1-9][0-9]* lastProgram=[1-9][0-9]* boom=[1-9][0-9]* root27delta=[1-9][0-9]* hatLoads=[2-9][0-9]* hatFails=0 nativeFailDelta=0 fighterRejectDelta=0'
)) {
    if ($text -notmatch $required) {
        throw "Kirby CopyLink proof missing required marker: $required"
    }
}
if (-not (Test-Path -LiteralPath $Screenshot -PathType Leaf)) {
    throw "Kirby CopyLink proof did not create screenshot: $Screenshot"
}

Get-Content -LiteralPath $Artifact |
    Where-Object { $_ -match '^(KIRBY_|ROM_|BUILD=|TARGET=|TRACE )' }
Write-Output "Kirby CopyLink proof: PASS ($Artifact)"
