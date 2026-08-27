[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Build,
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(60, 900)][int]$TimeoutSeconds = 240,
    [string]$Artifact = ''
)

# P2-3 Samus owner proof.
#
# GDB is observation-only with respect to fighter/gameplay state. The player
# actions are real host key events delivered to melonDS using its checked-in
# keyboard mapping:
#   S         -> DS B -> source B_BUTTON
#   Q         -> DS L -> port Z_TRIG mapping
#   Down + S  -> DS Down + B -> source stick_y=-80 + B_BUTTON
#
# This avoids both remote writes into ARM9 DTCM and GDB inferior function
# calls. BattleShip owns every fighter status transition observed below.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

if (-not ('Smash64DSSamusOwnerInput' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSSamusOwnerInput
{
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")]
    public static extern void keybd_event(
        byte key, byte scan, uint flags, UIntPtr extra);
}
'@
}

$VK_S = [byte]0x53
$VK_Q = [byte]0x51
$VK_DOWN = [byte]0x28
$SCAN_DOWN = [byte]0x50
$KEYEVENTF_EXTENDEDKEY = [uint32]1
$KEYEVENTF_KEYUP = [uint32]2

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$buildConfig = Join-Path (Split-Path -Parent $rom) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "Samus owner proof requires generated build config: $buildConfig"
}
$buildConfigText = Get-Content -LiteralPath $buildConfig -Raw
if (($buildConfigText -notmatch '(?m)^#define NDS_P2_SAMUS 1$') -or
    ($buildConfigText -notmatch '(?m)^#define NDS_P2_PROOF_FIGHTER0 3$')) {
    throw 'Samus owner proof requires a build made with NDS_P2_SAMUS=1 NDS_P2_PROOF_FIGHTER0=3.'
}
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-samus-owner.txt')
}

$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$tempDir = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root ('artifacts\verifier-temp\slot' + $RunnerSlot)
}
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$stdout = Join-Path $logDir 'melonds.samus-owner.stdout.log'
$stderr = Join-Path $logDir 'melonds.samus-owner.stderr.log'
$gdbScript = Join-Path $tempDir 'samus_owner_hostkeys.gdb'
$gdbOut = Join-Path $tempDir 'samus_owner_hostkeys.gdb.out'
$gdbErr = Join-Path $tempDir 'samus_owner_hostkeys.gdb.err'
$stageDir = Join-Path $tempDir 'samus-owner-stages'
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Get-ChildItem -LiteralPath $stageDir -File -ErrorAction SilentlyContinue |
    Remove-Item -Force

function Get-StagePath([string]$Name) {
    return Join-Path $stageDir ($Name + '.ready')
}

function Get-GdbStagePath([string]$Name) {
    return ((Get-StagePath $Name) -replace '\\', '/')
}

function Wait-Stage {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$GdbProcess,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    $path = Get-StagePath $Name
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            return
        }
        $GdbProcess.Refresh()
        if ($GdbProcess.HasExited) {
            $outText = if (Test-Path $gdbOut) { Get-Content $gdbOut -Raw } else { '' }
            $errText = if (Test-Path $gdbErr) { Get-Content $gdbErr -Raw } else { '' }
            throw "GDB exited before Samus stage '$Name'. OUT=$outText ERR=$errText"
        }
        Start-Sleep -Milliseconds 20
    }
    throw "Timed out waiting for Samus owner stage '$Name'."
}

function Get-MelonWindow {
    param(
        [Parameter(Mandatory=$true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory=$true)][datetime]$Deadline
    )
    while ((Get-Date) -lt $Deadline) {
        $Process.Refresh()
        if ($Process.HasExited) {
            throw "melonDS exited before its owner-proof window appeared (code $($Process.ExitCode))."
        }
        if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
            return $Process.MainWindowHandle
        }
        Start-Sleep -Milliseconds 50
    }
    throw 'Timed out waiting for the melonDS owner-proof window.'
}

function Focus-MelonWindow {
    param([Parameter(Mandatory=$true)][IntPtr]$Window)
    for ($i = 0; $i -lt 5; $i++) {
        [void][Smash64DSSamusOwnerInput]::ShowWindow($Window, 9)
        [void][Smash64DSSamusOwnerInput]::SetForegroundWindow($Window)
        Start-Sleep -Milliseconds 100
        if ([Smash64DSSamusOwnerInput]::GetForegroundWindow() -eq $Window) {
            return
        }
    }
    throw 'Refusing to synthesize Samus input: melonDS did not become the foreground window.'
}

function Send-MelonKey {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Window,
        [Parameter(Mandatory=$true)][byte]$Key,
        [ValidateRange(80, 1000)][int]$HoldMilliseconds = 250
    )
    Focus-MelonWindow -Window $Window
    [Smash64DSSamusOwnerInput]::keybd_event($Key, 0, 0, [UIntPtr]::Zero)
    try {
        Start-Sleep -Milliseconds $HoldMilliseconds
    }
    finally {
        [Smash64DSSamusOwnerInput]::keybd_event(
            $Key, 0, $KEYEVENTF_KEYUP, [UIntPtr]::Zero)
    }
}

function Send-MelonDownB {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Window,
        [ValidateRange(80, 1000)][int]$HoldMilliseconds = 250
    )
    Focus-MelonWindow -Window $Window
    # Cursor keys are extended Win32 keys. Without the extended flag Qt sees a
    # keypad-family key instead of the configured Qt::Key_Down binding.
    [Smash64DSSamusOwnerInput]::keybd_event(
        $VK_DOWN, $SCAN_DOWN, $KEYEVENTF_EXTENDEDKEY, [UIntPtr]::Zero)
    [Smash64DSSamusOwnerInput]::keybd_event($VK_S, 0, 0, [UIntPtr]::Zero)
    try {
        Start-Sleep -Milliseconds $HoldMilliseconds
    }
    finally {
        [Smash64DSSamusOwnerInput]::keybd_event(
            $VK_S, 0, $KEYEVENTF_KEYUP, [UIntPtr]::Zero)
        [Smash64DSSamusOwnerInput]::keybd_event(
            $VK_DOWN, $SCAN_DOWN,
            ($KEYEVENTF_EXTENDEDKEY -bor $KEYEVENTF_KEYUP), [UIntPtr]::Zero)
    }
}

$commands = @'
set pagination off
set confirm off
set remotetimeout 20
target remote 127.0.0.1:__PORT__
break efManagerSamusEntryPointMakeEffect
commands
silent
printf "TRACE ENTRY_EFFECT pos=(%f,%f,%f)\n", pos->x, pos->y, pos->z
disable $_hit_bpnum
continue
end
break ftCommonWaitProcInterrupt
commands
silent
if ((((FTStruct *)fighter_gobj->user_data.p)->fkind != 3) || (((FTStruct *)fighter_gobj->user_data.p)->is_control_disable != 0))
continue
end
disable $_hit_bpnum
end
continue
printf "TRACE INPUT_READY status=%d fkind=%d pkind=%d control_disable=%d\n", ((FTStruct *)fighter_gobj->user_data.p)->status_id, ((FTStruct *)fighter_gobj->user_data.p)->fkind, ((FTStruct *)fighter_gobj->user_data.p)->pkind, ((FTStruct *)fighter_gobj->user_data.p)->is_control_disable
tbreak ftSamusSpecialNStartSetStatus
shell cmd /c echo ready>__CHARGE__
continue
printf "TRACE CHARGE_START level=%d\n", ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.samus.charge_level
tbreak ftSamusSpecialNLoopProcUpdate if ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.samus.charge_level >= 2
continue
set $sam = (FTStruct *)fighter_gobj->user_data.p
set $stored = $sam->passive_vars.samus.charge_level
printf "TRACE CHARGE_STORED level=%d charge_gobj=%p\n", $stored, $sam->status_vars.samus.specialn.charge_gobj
break ftCommonWaitSetStatus
commands
silent
if ((FTStruct *)fighter_gobj->user_data.p)->fkind != 3
continue
end
disable $_hit_bpnum
end
shell cmd /c echo ready>__CANCEL__
continue
set $sam = (FTStruct *)fighter_gobj->user_data.p
set $cancel = $sam->passive_vars.samus.charge_level
printf "TRACE CHARGE_CANCEL level=%d pre=%d preserved=%d\n", $cancel, $stored, ($cancel >= $stored)
finish
break ftCommonWaitProcInterrupt
commands
silent
if ((((FTStruct *)fighter_gobj->user_data.p)->fkind != 3) || (((FTStruct *)fighter_gobj->user_data.p)->is_control_disable != 0))
continue
end
disable $_hit_bpnum
end
continue
tbreak ftSamusSpecialNStartSetStatus
shell cmd /c echo ready>__RESUME__
continue
printf "TRACE CHARGE_RESUME_START level=%d cancel=%d preserved=%d\n", ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.samus.charge_level, $cancel, (((FTStruct *)fighter_gobj->user_data.p)->passive_vars.samus.charge_level >= $cancel)
tbreak ftSamusSpecialNLoopSetStatus
continue
set $sam = (FTStruct *)fighter_gobj->user_data.p
finish
printf "TRACE CHARGE_RESUME level=%d cancel=%d preserved=%d charge_gobj=%p\n", $sam->passive_vars.samus.charge_level, $cancel, ($sam->passive_vars.samus.charge_level >= $cancel), $sam->status_vars.samus.specialn.charge_gobj
tbreak ftSamusSpecialNEndSetStatus
shell cmd /c echo ready>__RELEASE__
continue
printf "TRACE CHARGE_RELEASE_STATUS level=%d\n", ((FTStruct *)fighter_gobj->user_data.p)->passive_vars.samus.charge_level
tbreak wpSamusChargeShotLaunch
continue
set $shot = (WPStruct *)weapon_gobj->user_data.p
printf "TRACE CHARGE_LAUNCH size=%d owner_pre=%p full=%d\n", $shot->weapon_vars.charge_shot.charge_size, $shot->weapon_vars.charge_shot.owner_gobj, $shot->weapon_vars.charge_shot.is_full_charge
finish
printf "TRACE CHARGE_LAUNCHED owner_post=%p damage=%d\n", $shot->weapon_vars.charge_shot.owner_gobj, $shot->attack_coll.damage
break ftCommonWaitProcInterrupt
commands
silent
if ((((FTStruct *)fighter_gobj->user_data.p)->fkind != 3) || (((FTStruct *)fighter_gobj->user_data.p)->is_control_disable != 0))
continue
end
disable $_hit_bpnum
end
continue
tbreak ftSamusSpecialLwSetStatus
shell cmd /c echo ready>__BOMB__
continue
set $sam = (FTStruct *)fighter_gobj->user_data.p
finish
printf "TRACE BOMB_STATUS status=%d ga=%d anim=%f figatree=%p resolves=%u fallbacks=%u fallback_last=%#x\n", $sam->status_id, $sam->ga, fighter_gobj->anim_frame, $sam->figatree, gNdsRelocForceFighterAnimResolveCount, gNdsRelocForceFighterAnimFallbackCount, gNdsRelocForceFighterAnimFallbackLastAsset
tbreak wpSamusBombMakeWeapon
continue
printf "TRACE BOMB_MAKE pos_y=%f\n", pos->y
finish
printf "TRACE BOMB_CREATED gobj=%p\n", $r0
break ftSamusSpecialAirLwProcUpdate
commands
silent
printf "TRACE BOMB_JUMP status=%d ga=%d vel_y=%f\n", ((FTStruct *)fighter_gobj->user_data.p)->status_id, ((FTStruct *)fighter_gobj->user_data.p)->ga, ((FTStruct *)fighter_gobj->user_data.p)->physics.vel_air.y
disable $_hit_bpnum
continue
end
tbreak wpSamusBombExplodeProcUpdate
continue
set $bomb = (WPStruct *)weapon_gobj->user_data.p
printf "TRACE BOMB_EXPLODE gobj=%p lifetime=%d size=%f\n", weapon_gobj, $bomb->lifetime, $bomb->attack_coll.size
detach
quit
'@
$commands = $commands.Replace('__PORT__', [string]$context.GdbPort)
$commands = $commands.Replace('__CHARGE__', (Get-GdbStagePath 'charge'))
$commands = $commands.Replace('__CANCEL__', (Get-GdbStagePath 'cancel'))
$commands = $commands.Replace('__RESUME__', (Get-GdbStagePath 'resume'))
$commands = $commands.Replace('__RELEASE__', (Get-GdbStagePath 'release'))
$commands = $commands.Replace('__BOMB__', (Get-GdbStagePath 'bomb'))
Set-Content -LiteralPath $gdbScript -Value $commands

$configState = $null
$emulator = $null
$gdbProcess = $null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout,$stderr,$gdbOut,$gdbErr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom -WorkingDirectory $melonDir -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $gdbProcess = Start-Process -FilePath $gdb -ArgumentList @('-batch', $elf, '-x', $gdbScript) -RedirectStandardOutput $gdbOut -RedirectStandardError $gdbErr -WindowStyle Hidden -PassThru
    # Attach first. Waiting for the GUI before starting GDB lets this direct-
    # boot ROM run past scVSBattleStartBattle, leaving the observer waiting for
    # a breakpoint that already happened.
    $window = Get-MelonWindow -Process $emulator -Deadline $deadline

    Wait-Stage -Name 'charge' -GdbProcess $gdbProcess -Deadline $deadline
    Start-Sleep -Milliseconds 100
    Send-MelonKey -Window $window -Key $VK_S

    Wait-Stage -Name 'cancel' -GdbProcess $gdbProcess -Deadline $deadline
    Start-Sleep -Milliseconds 100
    Send-MelonKey -Window $window -Key $VK_Q

    Wait-Stage -Name 'resume' -GdbProcess $gdbProcess -Deadline $deadline
    Start-Sleep -Milliseconds 100
    Send-MelonKey -Window $window -Key $VK_S

    Wait-Stage -Name 'release' -GdbProcess $gdbProcess -Deadline $deadline
    Start-Sleep -Milliseconds 100
    Send-MelonKey -Window $window -Key $VK_S

    Wait-Stage -Name 'bomb' -GdbProcess $gdbProcess -Deadline $deadline
    Start-Sleep -Milliseconds 100
    Send-MelonDownB -Window $window

    while ((Get-Date) -lt $deadline) {
        $gdbProcess.Refresh()
        if ($gdbProcess.HasExited) { break }
        Start-Sleep -Milliseconds 50
    }
    $gdbProcess.Refresh()
    if (-not $gdbProcess.HasExited) {
        throw "Samus owner GDB trace exceeded $TimeoutSeconds seconds."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        $errText = if (Test-Path $gdbErr) { Get-Content $gdbErr -Raw } else { '' }
        throw "Samus owner GDB exited $($gdbProcess.ExitCode): $errText"
    }
}
finally {
    foreach ($key in @($VK_S, $VK_Q, $VK_DOWN)) {
        [Smash64DSSamusOwnerInput]::keybd_event(
            [byte]$key, 0, $KEYEVENTF_KEYUP, [UIntPtr]::Zero)
    }
    if ($gdbProcess -and (-not $gdbProcess.HasExited)) {
        Stop-Process -Id $gdbProcess.Id -Force -ErrorAction SilentlyContinue
    }
    if ($emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) {
        Restore-MelonDSGdbConfig -State $configState
    }
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
$hash = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
@(
    "SAMUS_OWNER_HOST_INPUT=melonDS keyboard S/Q/Down+S",
    "ROM_SHA256=$hash",
    "BUILD=$Build",
    "TARGET=$Target"
) | Set-Content -LiteralPath $Artifact
Get-Content -LiteralPath $gdbOut | Add-Content -LiteralPath $Artifact

$text = Get-Content -LiteralPath $Artifact -Raw
foreach ($required in @(
    'TRACE ENTRY_EFFECT pos=\(',
    'TRACE INPUT_READY status=10 fkind=3 pkind=0 control_disable=0',
    'TRACE CHARGE_STORED level=([2-7])',
    'TRACE CHARGE_CANCEL level=[2-7] pre=[2-7] preserved=1',
    'TRACE CHARGE_RESUME_START level=[2-7] cancel=[2-7] preserved=1',
    'TRACE CHARGE_RESUME level=[2-7] cancel=[2-7] preserved=1 charge_gobj=0x[1-9a-fA-F]',
    'TRACE CHARGE_LAUNCHED owner_post=(?:0x0|\(nil\)) damage=[1-9][0-9]*',
    'TRACE BOMB_STATUS status=229 ga=0 .*fallbacks=0 ',
    'TRACE BOMB_MAKE ',
    'TRACE BOMB_CREATED gobj=0x[1-9a-fA-F]',
    'TRACE BOMB_JUMP status=230 ga=1 vel_y=[1-9][0-9]*\.[0-9]+',
    'TRACE BOMB_EXPLODE gobj=0x[1-9a-fA-F][0-9a-fA-F]* lifetime=6 size=180\.000000'
)) {
    if ($text -notmatch $required) {
        throw "Samus owner proof missing required marker: $required"
    }
}

Get-Content -LiteralPath $Artifact |
    Where-Object { $_ -match '^(SAMUS_|ROM_|BUILD=|TARGET=|TRACE )' }
Write-Output "Samus owner proof: PASS ($Artifact)"
