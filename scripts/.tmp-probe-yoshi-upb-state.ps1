[CmdletBinding()]
param([string]$MelonDS='', [string]$Gdb='C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe', [int]$RunnerSlot=6)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$build=Join-Path $root 'builds\build-p2-yoshi-tour-full'
$rom=Join-Path $build 'smash64ds-battle-playable-proof-hwtri.nds'
$elf=Join-Path $build 'smash64ds-battle-playable-proof-hwtri.elf'
$ctx=Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot -NoBuild
$state=$null;$emu=$null
try {
  $state=Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath -GdbPort $ctx.GdbPort -Persistent -MuteAudio
  $emu=Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom -WorkingDirectory (Split-Path -Parent $ctx.MelonDSPath) -WindowStyle Hidden -PassThru
  Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null
  $cmd=@(
    'set pagination off','set confirm off','set breakpoint pending off','set remotetimeout 20',
    ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
    'break __excpt_entry','commands','silent','printf "YUP_EXCEPTION pc=%#x lr=%#x\n",$pc,$lr','bt 12','detach','quit 2','end',
    'tbreak ftYoshiSpecialHiSetStatus','continue',
    'set $yg=(GObj *)$r0','set $yf=(FTStruct *)$yg->user_data.p',
    'finish',
    'printf "YUP_STATE status=%d motion=%d frame=%f fig=%p script=%p wait=%f sid=%d flag2=%d anim=%#x main=%p\n",$yf->status_id,$yf->motion_id,$yg->anim_frame,$yf->figatree,$yf->motion_scripts[0][0].p_script,$yf->motion_scripts[0][0].script_wait,$yf->motion_scripts[0][0].script_id,$yf->motion_vars.flags.flag2,$yf->anim_desc.word,*$yf->data->p_file_mainmotion',
    'set $i=0','while $i < 12','set $w=((unsigned int *)$yf->motion_scripts[0][0].p_script)[$i]','printf "YUP_WORD %u %#x\n",$i,$w','set $i=$i+1','end',
    'detach','quit')
  Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $cmd -ScriptName 'yoshi-upb-state.gdb' -TimeoutSeconds 180 | Out-Null
  $d=$env:SMASH64DS_VERIFY_TEMP_DIR;if(-not $d){$d=Join-Path $root ('artifacts\verifier-temp\slot'+$RunnerSlot)}
  foreach($f in @('yoshi-upb-state.gdb.out','yoshi-upb-state.gdb.err')){$p=Join-Path $d $f;if(Test-Path $p){Get-Content $p}}
} finally {
  if(($null-ne $emu)-and -not $emu.HasExited){Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue}
  if($null-ne $state){Restore-MelonDSGdbConfig -State $state}
}
