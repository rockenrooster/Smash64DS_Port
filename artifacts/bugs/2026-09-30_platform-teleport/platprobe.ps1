param(
    [int]$Slot = 5,
    [Parameter(Mandatory)][string]$Tag,
    [string]$RomDir = 'builds\lab-exist-ctl',
    [string]$Gkind = '5',
    [string]$Kinds = '',
    [string]$FollowProc = 'grYosterProcUpdate',
    [int]$EndFrame = 1950,
    [int]$Follow = 4,
    [int]$TimeoutSeconds = 1500
)
# Platform switch-off probe on the four-CPU lab: whenever the stage switches a
# yakumono off (mpCollisionSetYakumonoOffID), print every fighter's ground
# state and position, then the same on the next $Follow stage ticks. A
# fighter standing on the switched-off line must start falling from where it
# stood (source mpProcessCheckTestFloorCollisionNew), not jump.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
$sp = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\4aa2b87d-ceb4-40ba-b9bb-cfd56e625cd7\scratchpad'
# Per-slot storage/DLDI like the tick-HUD sampler: concurrent runners otherwise
# share one image and all but the first stall at boot.
$context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$rom = Join-Path $root "$RomDir\smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elf = Join-Path $root "$RomDir\smash64ds-p2-fourcpu-tickhud-hwtri.elf"
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$cmd = Join-Path $sp "plat-$Tag.gdb"
$fighters = @(
    '  set $gob = gGCCommonLinks[3]',
    '  while $gob != 0',
    '    set $ftp = (FTStruct*)$gob->user_data.p',
    '    printf "  P%d ga=%d fl=%d st=%d x=%.1f y=%.1f\n", $ftp->player, $ftp->ga, $ftp->coll_data.floor_line_id, $ftp->status_id, $ftp->coll_data.p_translate->x, $ftp->coll_data.p_translate->y',
    '    set $gob = $gob->link_next',
    '  end')
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete', "set var gNdsLabFourCpuGkind = $Gkind")
    if ($Kinds -ne '') { $lines += "set var gNdsLabFourCpuKinds = $Kinds" }
    $lines += 'set $follow = 0'
    $lines += 'break *mpCollisionSetYakumonoOffID'
    $lines += 'commands'
    $lines += '  silent'
    $lines += '  printf "OFF line=%d f=%u\n", $r0, gNdsBattlePlayablePacingPresentedFrames'
    $lines += $fighters
    $lines += "  set `$follow = $Follow"
    $lines += '  continue'
    $lines += 'end'
    $lines += "break $FollowProc if `$follow > 0"
    $lines += 'commands'
    $lines += '  silent'
    $lines += '  printf "TICK f=%u\n", gNdsBattlePlayablePacingPresentedFrames'
    $lines += $fighters
    $lines += '  set $follow = $follow - 1'
    $lines += '  continue'
    $lines += 'end'
    $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $EndFrame"
    $lines += 'continue'
    $lines += 'printf "END f=%u\n", gNdsBattlePlayablePacingPresentedFrames'
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "plat-$Tag.log") -RedirectStandardError (Join-Path $sp "plat-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "plat-$Tag.log") | Select-String -Pattern 'OFF|TICK|  P|END|rror' | ForEach-Object { $_.Line }

