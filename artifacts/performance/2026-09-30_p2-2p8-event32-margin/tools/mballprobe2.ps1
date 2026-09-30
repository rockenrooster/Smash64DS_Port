param(
    [int]$Slot = 3,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-exist-fix',
    [string]$Gkind = '2',
    [string]$Kinds = '34080263',
    [int]$EndFrame = 1000,
    [int]$TimeoutSeconds = 1500
)
# Poke Ball open lifecycle on the four-CPU lab: every open/air transition, the
# rays effect the open makes, every ball update while open (multi, the effect
# GObj and its DObj) and every eject of a GObj. Stops at $EndFrame or a fault.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
$sp = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\4aa2b87d-ceb4-40ba-b9bb-cfd56e625cd7\scratchpad'
$context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$rom = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elf = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.elf"
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$cmd = Join-Path $sp "mball-$Tag.gdb"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete', "set var gNdsLabFourCpuGkind = $Gkind", "set var gNdsLabFourCpuKinds = $Kinds",
        'set $rays = 0',
        'break itMBallOpenInitVars',
        'commands', '  silent', '  printf "INIT f=%u ball=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0', '  printf "LEDGER fail=%u reason=%u addr=%08x owner=%u cap=%u count=%u limit=%u holes=%u\n", gNdsAObjEvent32NormalizeFailCount, gNdsAObjEvent32NormalizeLastFailReason, gNdsAObjEvent32NormalizeLastFailAddress, gNdsAObjEvent32NormalizeLastFailOwner, gNdsAObjEvent32CapacityRefusedCount, ''battleship_sys_objanim.c''::sNdsAObjEvent32NormalizedCount, ''battleship_sys_objanim.c''::sNdsAObjEvent32NormalizedLimit, ''battleship_sys_objanim.c''::sNdsAObjEvent32Holes', '  continue', 'end',
        'break itMBallOpenAirSetStatus',
        'commands', '  silent', '  printf "AIR f=%u ball=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0', '  continue', 'end',
        'break itMBallOpenProcUpdate',
        'commands', '  silent',
        '  set $itp = (ITStruct*)((GObj*)$r0)->user_data.p',
        '  set $rays = (unsigned int)$itp->item_vars.mball.effect_gobj',
        '  printf "OPEN f=%u ball=%08x multi=%d fx=%08x", gNdsBattlePlayablePacingPresentedFrames, $r0, $itp->multi, $rays',
        '  if $rays != 0', '    printf " fxobj=%08x\n", (unsigned int)((GObj*)$rays)->obj', '  else', '    printf "\n"', '  end',
        '  continue', 'end',
        'break itMBallOpenAirProcUpdate',
        'commands', '  silent',
        '  set $itp = (ITStruct*)((GObj*)$r0)->user_data.p',
        '  set $rays = (unsigned int)$itp->item_vars.mball.effect_gobj',
        '  printf "OPENAIR f=%u ball=%08x multi=%d fx=%08x", gNdsBattlePlayablePacingPresentedFrames, $r0, $itp->multi, $rays',
        '  if $rays != 0', '    printf " fxobj=%08x\n", (unsigned int)((GObj*)$rays)->obj', '  else', '    printf "\n"', '  end',
        '  continue', 'end',
        'break efManagerNoStructProcUpdate if ($rays != 0) && ((unsigned int)$r0 == $rays)',
        'commands', '  silent',
        '  set $d = (DObj*)((GObj*)$r0)->obj',
        '  printf "FXUPD f=%u gobj=%08x gaf=%f\n", gNdsBattlePlayablePacingPresentedFrames, $r0, ((GObj*)$r0)->anim_frame',
        '  set $n = 0',
        '  while ($d != 0) && ($n < 12)',
        '    printf "  d%d=%08x ev=%08x wait=%f frame=%f speed=%f aobj=%08x child=%08x sib=%08x\n", $n, $d, $d->anim_joint.event32, $d->anim_wait, $d->anim_frame, $d->anim_speed, $d->aobj, $d->child, $d->sib_next',
        '    if $d->child != 0',
        '      set $d = $d->child',
        '    else',
        '      set $d = $d->sib_next',
        '    end',
        '    set $n = $n + 1',
        '  end',
        '  continue', 'end',
        'break gcEjectGObj if ($rays != 0) && ((unsigned int)$r0 == $rays)',
        'commands', '  silent', '  printf "EJECT-FX f=%u gobj=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0', '  bt 4', '  continue', 'end',
        'break __excpt_entry',
        "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $EndFrame",
        'continue',
        'printf "STOP f=%u pc=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $pc',
        'bt 6', '  printf "LEDGER fail=%u reason=%u addr=%08x owner=%u cap=%u count=%u limit=%u holes=%u\n", gNdsAObjEvent32NormalizeFailCount, gNdsAObjEvent32NormalizeLastFailReason, gNdsAObjEvent32NormalizeLastFailAddress, gNdsAObjEvent32NormalizeLastFailOwner, gNdsAObjEvent32CapacityRefusedCount, ''battleship_sys_objanim.c''::sNdsAObjEvent32NormalizedCount, ''battleship_sys_objanim.c''::sNdsAObjEvent32NormalizedLimit, ''battleship_sys_objanim.c''::sNdsAObjEvent32Holes',
        'kill', 'quit')
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "mball-$Tag.log") -RedirectStandardError (Join-Path $sp "mball-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "mball-$Tag.log") | Select-String -Pattern 'INIT|AIR|OPEN|EJECT|STOP|#\d|rror' | ForEach-Object { $_.Line }




