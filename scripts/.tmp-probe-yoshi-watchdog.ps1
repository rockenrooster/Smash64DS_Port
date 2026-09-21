[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateRange(1,12)][int]$RunnerSlot = 6,
    [string]$Build = 'build-p2-yoshi-tour-full',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1,5000)][int]$AtTotal = 100,
    [ValidateRange(30,240)][int]$TimeoutSeconds = 120
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir ($Target + '.nds')
$elf = Join-Path $buildDir ($Target + '.elf')
$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom -WorkingDirectory (Split-Path -Parent $ctx.MelonDSPath) -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set breakpoint pending off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        'break __excpt_entry',
        'commands',
        'silent',
        'printf "YOSHI_SNAP_EXCEPTION pc=%#x lr=%#x\n",$pc,$lr',
        'detach',
        'quit 2',
        'end',
        ("tbreak ndsR2HostBattleUpdateOnce if gNdsYoshiBugHostWatchdogTotalUpdates >= {0}" -f $AtTotal),
        'continue',
        'set $yg=sNdsFighterManagerLiveGObjs[0]',
        'set $yf=($yg != 0) ? (FTStruct *)$yg->user_data.p : (FTStruct *)0',
        'printf "YOSHI_SNAP total=%u prepared=%u result=%#x phase=%u done=%u stall=%u game=%d gobj=%p status=%d motion=%d entry=%u shared=%u lay=%u latch=%u/%u/%u gmax=%d gactive=%d heap=%u\n",gNdsYoshiBugHostWatchdogTotalUpdates,gNdsFighterNaturalMotionPrepared,gNdsFighterNaturalMotionResult,gNdsYoshiBugTourPhase,gNdsYoshiBugTourDone,gNdsYoshiBugTourStallPhase,gSCManagerBattleState->game_status,$yg,($yf!=0)?$yf->status_id:-1,($yf!=0)?$yf->motion_id:-1,gNdsYoshiBugProofEntryEggDrawCount,gNdsYoshiBugProofSharedEggDrawCount,gNdsYoshiBugProofEggLayDrawCount,gNdsIFCommonGObjLatchReserveApplyCount,gNdsIFCommonGObjLatchBase,gNdsIFCommonGObjLatchLimit,sGCCommonsMaxNum,sGCCommonsActiveNum,(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $commands -ScriptName 'yoshi-watchdog-snap.gdb' -TimeoutSeconds $TimeoutSeconds | Out-Null
    $temp = $env:SMASH64DS_VERIFY_TEMP_DIR
    if ([string]::IsNullOrWhiteSpace($temp)) { $temp = Join-Path $root ('artifacts\verifier-temp\slot' + $RunnerSlot) }
    Get-Content (Join-Path $temp 'yoshi-watchdog-snap.gdb.out') -Raw
    if (Test-Path (Join-Path $temp 'yoshi-watchdog-snap.gdb.err')) { Get-Content (Join-Path $temp 'yoshi-watchdog-snap.gdb.err') -Raw }
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) { Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
