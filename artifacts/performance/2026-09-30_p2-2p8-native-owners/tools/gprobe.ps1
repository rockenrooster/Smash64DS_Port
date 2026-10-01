param(
    [int]$Slot = 1,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-baked2',
    # gdb lines run at main (pokes), then the watch lines, then the run to EndFrame
    [string[]]$Boot = @(),
    [string[]]$Watch = @(),
    [int]$EndFrame = 400,
    [string[]]$AtEnd = @(),
    [int]$TimeoutSeconds = 900
)
# Generic gdb probe on the four-CPU lab ROM.
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
$cmd = Join-Path $sp "gp-$Tag.gdb"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru -WindowStyle Minimized
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $g = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete') + $Boot + $Watch
    $g += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $EndFrame"
    $g += 'continue'
    $g += 'printf "END f=%u\n", gNdsBattlePlayablePacingPresentedFrames'
    $g += $AtEnd
    $g += 'kill', 'quit'
    $g | Set-Content -LiteralPath $cmd -Encoding ascii
    $gp = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "gp-$Tag.log") -RedirectStandardError (Join-Path $sp "gp-$Tag.err")
    if (-not $gp.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $gp.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "gp-$Tag.log")
