param(
    [int]$Slot = 5,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-baked',
    [string]$Kinds = '',
    [string]$Gkind = '6',
    [int[]]$Frames = @(450, 600),
    [int]$TimeoutSeconds = 900
)
# Same-ROM frame captures on the four-CPU lab (platprobe's per-slot context
# launch): the words are poked at main, the guest halts at
# ndsBattlePlayableFrameCompleteMarker on each requested presented frame, and
# gdb's `shell` captures the melonDS window (PrintWindow) while it is halted.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
$sp = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\4aa2b87d-ceb4-40ba-b9bb-cfd56e625cd7\scratchpad'
$out = Join-Path $root 'artifacts\performance\2026-09-30_p2-2p8-native-owners\captures'
New-Item -ItemType Directory -Force $out | Out-Null
$context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$rom = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elf = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.elf"
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$cmd = Join-Path $sp "bcap-$Tag.gdb"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete', "set var gNdsLabFourCpuGkind = $Gkind")
    if ($Kinds -ne '') { $lines += "set var gNdsLabFourCpuKinds = $Kinds" }
    foreach ($f in $Frames) {
        $png = (Join-Path $out "$Tag-f$f.png")
        $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $f"
        $lines += 'continue'
        $lines += "printf `"AT %u baked=%u last=%x\n`", gNdsBattlePlayablePacingPresentedFrames, gNdsItemBakedDrawCount, gNdsItemBakedLastRoot"
        $lines += "shell pwsh -NoProfile -File `"$sp\capwin.ps1`" -EmulatorProcessId $($p.Id) -Output `"$png`""
        $lines += 'delete'
    }
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "bcap-$Tag.log") -RedirectStandardError (Join-Path $sp "bcap-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "bcap-$Tag.log") | Select-String -Pattern 'AT |Captured|rror' | ForEach-Object { $_.Line }
