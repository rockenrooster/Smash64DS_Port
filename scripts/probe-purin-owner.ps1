[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Build,
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(60, 600)][int]$TimeoutSeconds = 240,
    [string]$Artifact = '',
    [string]$Screenshot = ''
)

# P2-3f47 final Purin runtime proof.
#
# The only player action is an ordinary S key message delivered to the melonDS
# window (checked-in mapping: S -> DS B -> source B_BUTTON). GDB observes the
# source Wait and Neutral-B transitions and their renderer counters; it never
# calls a status setter or writes fighter status/motion.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\melonds-screenshot.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

if (-not ('Smash64DSPurinInput' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSPurinInput
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
$WM_KEYDOWN = [uint32]0x0100
$WM_KEYUP = [uint32]0x0101
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'

$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$buildConfig = Join-Path (Split-Path -Parent $rom) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "Purin proof requires generated build config: $buildConfig"
}
$config = Get-Content -LiteralPath $buildConfig -Raw
foreach ($required in @(
    'NDS_P2_PURIN 1',
    'NDS_P2_PROOF_FIGHTER0 10',
    'NDS_NATIVE_OWNER_IMAGE_VERIFY 1'
)) {
    if ($config -notmatch ('(?m)^#define ' + [regex]::Escape($required) + '$')) {
        throw "Purin proof requires build config '$required'."
    }
}
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\visibility\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-purin-owner.txt')
}
if ([string]::IsNullOrWhiteSpace($Screenshot)) {
    $Screenshot = Join-Path $root ('artifacts\visibility\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-purin-specialn.png')
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
$stdout = Join-Path $logDir 'melonds.purin-owner.stdout.log'
$stderr = Join-Path $logDir 'melonds.purin-owner.stderr.log'
$gdbScript = Join-Path $tempDir 'purin_owner_hostkey.gdb'
$gdbOut = Join-Path $tempDir 'purin_owner_hostkey.gdb.out'
$gdbErr = Join-Path $tempDir 'purin_owner_hostkey.gdb.err'
$stageDir = Join-Path $tempDir 'purin-owner-stages'
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
            throw "GDB exited before Purin stage '$Name'. OUT=$out ERR=$err"
        }
        Start-Sleep -Milliseconds 20
    }
    throw "Timed out waiting for Purin stage '$Name'."
}
function Get-MelonWindow {
    param(
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    while ((Get-Date) -lt $Deadline) {
        $Process.Refresh()
        if ($Process.HasExited) {
            throw "melonDS exited before its Purin proof window appeared (code $($Process.ExitCode))."
        }
        if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
            return $Process.MainWindowHandle
        }
        Start-Sleep -Milliseconds 50
    }
    throw 'Timed out waiting for the melonDS Purin proof window.'
}
function Send-MelonB {
    param([Parameter(Mandatory=$true)][IntPtr]$Window)
    [void][Smash64DSPurinInput]::ShowWindow($Window, 9)
    if (-not [Smash64DSPurinInput]::PostMessage(
            $Window, $WM_KEYDOWN, [UIntPtr]$VK_S, [IntPtr]1)) {
        throw 'Could not deliver Purin B key-down to melonDS.'
    }
    try { Start-Sleep -Milliseconds 180 }
    finally {
        if (-not [Smash64DSPurinInput]::PostMessage(
                $Window, $WM_KEYUP, [UIntPtr]$VK_S, [IntPtr]0xC0000001)) {
            throw 'Could not deliver Purin B key-up to melonDS.'
        }
    }
}

$commands = @'
set pagination off
set confirm off
set remotetimeout 20
target remote 127.0.0.1:__PORT__
set $purin_gobj = 0
set $special_entered = 0
set $special_hits = 0
set $idle_tri = 0
set $native_fail_base = 0
set $fighter_reject_base = 0
set $image_fail_base = 0
set $image_mismatch_base = 0

break ftParamInitPlayerBattleStats if player == 0 && fighter_gobj != 0 && ((FTStruct *)fighter_gobj->user_data.p)->fkind == 10
commands
silent
set $purin_gobj = fighter_gobj
set $native_fail_base = gNdsRendererNativeFailure.count
set $fighter_reject_base = gNdsFtrRejectCountBySlot[0]
set $image_fail_base = gNdsNativeOwnerImageFailCount
set $image_mismatch_base = gNdsNativeOwnerImageMismatchCount
printf "TRACE MADE gobj=%p status=%d detail=%d pkind=%d nativeFail=%u rejects=%u imageFail=%u imageMismatch=%u\n", $purin_gobj, ((FTStruct *)$purin_gobj->user_data.p)->status_id, ((FTStruct *)$purin_gobj->user_data.p)->detail_curr, ((FTStruct *)$purin_gobj->user_data.p)->pkind, $native_fail_base, $fighter_reject_base, $image_fail_base, $image_mismatch_base
continue
end

# ftCommonAppearSetStatus calls ftMainSetStatus first, then this helper.  By the
# time this breakpoint fires the Appear animation force-load has completed, so
# its counters identify the real loader result without a per-frame breakpoint.
break ndsFTCommonAppearInitStatusVars if $r0 == sNdsFighterManagerLiveGObjs[0]
commands
silent
set $appear_fp = (FTStruct *)fighter_gobj->user_data.p
printf "TRACE APPEAR_LOAD status=%d motion=%d anim=%f fig=%p resolve=%u fallback=%u last=0x%x step=%u streamReads=%u streamMisses=%u streamFailures=%u\n", $appear_fp->status_id, $appear_fp->motion_id, fighter_gobj->anim_frame, $appear_fp->figatree, gNdsRelocForceFighterAnimResolveCount, gNdsRelocForceFighterAnimFallbackCount, gNdsRelocForceFighterAnimFallbackLastAsset, gNdsRelocForceFighterAnimFallbackStep, gNdsRelocAssetFighterStreamReads, gNdsRelocAssetFighterStreamMisses, gNdsRelocAssetFighterStreamFailures
disable $_hit_bpnum
continue
end

break ftPurinSpecialNSetStatus if $r0 == sNdsFighterManagerLiveGObjs[0]
commands
silent
set $special_fp = (FTStruct *)fighter_gobj->user_data.p
set $special_entered = 1
set $special_hits = 0
set $idle_tri = gNdsFighterDLAllDrawP0HardwareTriangleCount
printf "TRACE SPECIAL_ENTER pre_status=%d control=%d detail=%d idleTriangles=%u nativeFailDelta=%u fighterRejectDelta=%u imageFailDelta=%u imageMismatchDelta=%u validate=%u\n", $special_fp->status_id, $special_fp->is_control_disable, $special_fp->detail_curr, $idle_tri, gNdsRendererNativeFailure.count - $native_fail_base, gNdsFtrRejectCountBySlot[0] - $fighter_reject_base, gNdsNativeOwnerImageFailCount - $image_fail_base, gNdsNativeOwnerImageMismatchCount - $image_mismatch_base, gNdsNativeFighterValidateRejectCode
shell cmd /c echo ready>__CAPTURE__
continue
end

break ftPurinSpecialNProcMap if $r0 == sNdsFighterManagerLiveGObjs[0] && $special_entered != 0
commands
silent
set $map_fp = (FTStruct *)fighter_gobj->user_data.p
set $special_hits = $special_hits + 1
if $special_hits >= 2 && gNdsFighterDLAllDrawP0HardwareTriangleCount > $idle_tri
printf "TRACE FINAL status=%d detail=%d triDelta=%u nativeFailDelta=%u fighterRejectDelta=%u imageFailDelta=%u imageMismatchDelta=%u validate=%u\n", ((FTStruct *)$purin_gobj->user_data.p)->status_id, ((FTStruct *)$purin_gobj->user_data.p)->detail_curr, gNdsFighterDLAllDrawP0HardwareTriangleCount - $idle_tri, gNdsRendererNativeFailure.count - $native_fail_base, gNdsFtrRejectCountBySlot[0] - $fighter_reject_base, gNdsNativeOwnerImageFailCount - $image_fail_base, gNdsNativeOwnerImageMismatchCount - $image_mismatch_base, gNdsNativeFighterValidateRejectCode
shell cmd /c echo ready>__DONE__
detach
quit
end
continue
end
continue
'@
$commands = $commands.Replace('__PORT__', [string]$context.GdbPort)
$commands = $commands.Replace('__CAPTURE__', (Get-GdbStagePath 'capture'))
$commands = $commands.Replace('__DONE__', (Get-GdbStagePath 'done'))
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
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $gdbProcess = Start-Process -FilePath $gdb `
        -ArgumentList @('-batch', $elf, '-x', $gdbScript) `
        -RedirectStandardOutput $gdbOut -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    $window = Get-MelonWindow -Process $emulator -Deadline $deadline

    # Let the exact 120-frame source Appear2 clip and 3/2/1/GO countdown finish
    # at full emulator speed. Do NOT breakpoint Wait: Fox's CPU also enters Wait
    # frequently, and a host-evaluated GDB condition on that hot common function
    # stretches 120 source frames into minutes. The Special-N setter below is
    # the stronger proof anyway: it must report pre-status Wait (10), control on,
    # and a positive accumulated idle triangle count before it changes status.
    Start-Sleep -Seconds 12
    $capturePath = Get-StagePath 'capture'
    for ($attempt = 0; $attempt -lt 8; $attempt++) {
        Send-MelonB -Window $window
        for ($poll = 0; $poll -lt 40; $poll++) {
            if (Test-Path -LiteralPath $capturePath -PathType Leaf) { break }
            $gdbProcess.Refresh()
            if ($gdbProcess.HasExited) { break }
            Start-Sleep -Milliseconds 25
        }
        if (Test-Path -LiteralPath $capturePath -PathType Leaf) { break }
        $gdbProcess.Refresh()
        if ($gdbProcess.HasExited) { break }
    }
    Wait-Stage -Name 'capture' -GdbProcess $gdbProcess -Deadline $deadline
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Screenshot) |
        Out-Null
    Start-Sleep -Milliseconds 100
    $captureSize = Save-MelonDSWindowCapture -WindowHandle $window `
        -Path $Screenshot -PreferPrintWindow
    Wait-Stage -Name 'done' -GdbProcess $gdbProcess -Deadline $deadline
    while ((Get-Date) -lt $deadline) {
        $gdbProcess.Refresh()
        if ($gdbProcess.HasExited) { break }
        Start-Sleep -Milliseconds 20
    }
    $gdbProcess.Refresh()
    if (-not $gdbProcess.HasExited) { throw 'Purin GDB trace did not exit.' }
    if ($gdbProcess.ExitCode -ne 0) {
        $err = if (Test-Path $gdbErr) { Get-Content $gdbErr -Raw } else { '' }
        throw "Purin GDB exited $($gdbProcess.ExitCode): $err"
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
    'PURIN_HOST_INPUT=melonDS window-targeted S key message only',
    "PURIN_SCREENSHOT=$Screenshot",
    "PURIN_SCREENSHOT_SIZE=$captureSize",
    "ROM_SHA256=$hash",
    "BUILD=$Build",
    "TARGET=$Target"
) | Set-Content -LiteralPath $Artifact
Get-Content -LiteralPath $gdbOut | Add-Content -LiteralPath $Artifact

$text = Get-Content -LiteralPath $Artifact -Raw
foreach ($required in @(
    'TRACE MADE .*nativeFail=0 rejects=0 imageFail=0 imageMismatch=0',
    'TRACE SPECIAL_ENTER pre_status=10 control=0 .*idleTriangles=[1-9][0-9]* nativeFailDelta=0 fighterRejectDelta=0 imageFailDelta=0 imageMismatchDelta=0 validate=0',
    'TRACE FINAL status=230 .*triDelta=[1-9][0-9]* nativeFailDelta=0 fighterRejectDelta=0 imageFailDelta=0 imageMismatchDelta=0 validate=0'
)) {
    if ($text -notmatch $required) {
        throw "Purin proof missing required marker: $required"
    }
}
if (-not (Test-Path -LiteralPath $Screenshot -PathType Leaf)) {
    throw "Purin proof did not create screenshot: $Screenshot"
}
Get-Content -LiteralPath $Artifact |
    Where-Object { $_ -match '^(PURIN_|ROM_|BUILD=|TARGET=|TRACE )' }
Write-Output "Purin owner proof: PASS ($Artifact)"
