param(
    [int]$Slot = 5,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-baked2',
    [string]$Gkind = '0',
    [string]$Kinds = '',
    [int]$EndFrame = 400,
    [int]$TimeoutSeconds = 900
)
# Native-failure site probe: at every ndsStageRejectNativeRender call print
# the reason (r2), the GObj kind, the DL address and three return addresses
# (argument registers and the call chain are reliable at entry; stack locals
# are not on this remote).
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
$cmd = Join-Path $sp "rej-$Tag.gdb"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru -WindowStyle Minimized
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete', "set var gNdsLabFourCpuGkind = $Gkind")
    if ($Kinds -ne '') { $lines += "set var gNdsLabFourCpuKinds = $Kinds" }
    $lines += 'break ndsRendererRecordNativeFailure'
    $lines += 'commands'
    $lines += '  silent'
    $lines += '  printf "REJ f=%u ident=%x status=%x root=%x reason=%u lr=%x\n", gNdsBattlePlayablePacingPresentedFrames, $r2, $r3, *(unsigned*)$sp, *(unsigned*)($sp+8), $lr'
    $lines += '  bt 5'
    $lines += '  continue'
    $lines += 'end'
    $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $EndFrame"
    $lines += 'continue'
    $lines += 'printf "END f=%u\n", gNdsBattlePlayablePacingPresentedFrames'
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "rej-$Tag.log") -RedirectStandardError (Join-Path $sp "rej-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "rej-$Tag.log") | Select-String -Pattern 'REJ|gobj=|^#|END|rror' | ForEach-Object { $_.Line }
