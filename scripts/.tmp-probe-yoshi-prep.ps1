[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 6,
    [int]$TimeoutSeconds = 360
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$build = 'builds\build-p2-yoshi-tour-full'
$target = 'smash64ds-battle-playable-proof-hwtri'
$rom = Join-Path $root "$build\$target.nds"
$elf = Join-Path $root "$build\$target.elf"
$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $ctx.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null
    $cmd = @(
        'set pagination off',
        'set confirm off',
        'set breakpoint pending off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        'break __excpt_entry',
        'commands',
        'silent',
        'printf "YPREP_EXCEPTION pc=%#x lr=%#x\n",$pc,$lr',
        'bt 12',
        'detach',
        'quit 2',
        'end',
        'tbreak ifCommonCountdownMakeInterface',
        'continue',
        'set $yg = sNdsFighterManagerLiveGObjs[0]',
        'set $yf = (FTStruct *)$yg->user_data.p',
        'printf "YPREP_COUNTDOWN state=%u prepared=%u gobj=%p fkind=%d status=%d control=%d entrydraw=%u gmax=%d gactive=%d heap=%u threads=%u/%u sleeps=%u\n",gSCManagerBattleState->game_status,gNdsFighterNaturalMotionPrepared,$yg,$yf->fkind,$yf->status_id,$yf->is_control_disable,gNdsYoshiBugProofEntryEggDrawCount,sGCCommonsMaxNum,sGCCommonsActiveNum,(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr,gNdsOsGObjThreadProvisionCount,gNdsOsGObjThreadProvisionFailCount,gNdsTaskmanGObjThreadSleeps',
        'tbreak *0x021267e8',
        'continue',
        'printf "YPREP_THREAD state=%u prepared=%u status=%d control=%d gmax=%d gactive=%d heap=%u threads=%u/%u sleeps=%u\n",gSCManagerBattleState->game_status,gNdsFighterNaturalMotionPrepared,$yf->status_id,$yf->is_control_disable,sGCCommonsMaxNum,sGCCommonsActiveNum,(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr,gNdsOsGObjThreadProvisionCount,gNdsOsGObjThreadProvisionFailCount,gNdsTaskmanGObjThreadSleeps',
        'tbreak *0x02126728',
        'continue',
        'printf "YPREP_GO before_state=%u prepared=%u status=%d control=%d gmax=%d gactive=%d heap=%u threads=%u/%u sleeps=%u\n",gSCManagerBattleState->game_status,gNdsFighterNaturalMotionPrepared,$yf->status_id,$yf->is_control_disable,sGCCommonsMaxNum,sGCCommonsActiveNum,(unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr,gNdsOsGObjThreadProvisionCount,gNdsOsGObjThreadProvisionFailCount,gNdsTaskmanGObjThreadSleeps',
        'tbreak src/port/reloc_backend_movement.c:6365',
        'continue',
        'printf "YPREP_ARM state=%u fkind=%d status=%d control=%d prepared=%u phase=%u entrydraw=%u\n",gSCManagerBattleState->game_status,$yf->fkind,$yf->status_id,$yf->is_control_disable,gNdsFighterNaturalMotionPrepared,gNdsYoshiBugTourPhase,gNdsYoshiBugProofEntryEggDrawCount',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $cmd `
        -ScriptName 'yoshi-prep.gdb' -TimeoutSeconds $TimeoutSeconds | Out-Null
    $d = $env:SMASH64DS_VERIFY_TEMP_DIR
    if (-not $d) { $d = Join-Path $root ('artifacts\verifier-temp\slot' + $RunnerSlot) }
    foreach ($f in @('yoshi-prep.gdb.out','yoshi-prep.gdb.err')) {
        $p = Join-Path $d $f
        if (Test-Path $p) { Get-Content $p }
    }
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
