param(
    [int]$Slot = 9,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-p2p8-lab-cw',
    [string]$Kinds = '151388424',
    [string]$Gkind = '1',
    # name=value words poked at main (decimal values)
    [string[]]$Sets = @(),
    [int[]]$Frames = @(450, 600, 800, 1500, 1700),
    [int]$TimeoutSeconds = 900,
    [int]$PokeFrame = 0,
    [string[]]$Pokes = @()
)
# Same-ROM frame captures on the four-CPU lab: the words are poked at main,
# the guest halts at ndsBattlePlayableFrameCompleteMarker on each requested
# presented frame, and gdb's `shell` captures the melonDS window (PrintWindow)
# while the guest is halted.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
$sp = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\31e5c78d-38d0-4d75-b683-91723230d2cc\scratchpad'
$out = Join-Path $root 'artifacts\performance\2026-09-30_p2-2p8-arwing-packet\captures'
New-Item -ItemType Directory -Force $out | Out-Null
$port = 3300 + 10 * $Slot
$melon = Join-Path $root "emulators\melonds-runners\slot$Slot\melonDS.exe"
$rom = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elf = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.elf"
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$cmd = Join-Path $sp "erpcap-$Tag.gdb"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port ($port + 1) -Persistent -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete',
        "set var gNdsLabFourCpuGkind = $Gkind", "set var gNdsLabFourCpuKinds = $Kinds")
    foreach ($s in $Sets) {
        $kv = $s -split '='
        $lines += "set var $($kv[0]) = $($kv[1])"
    }
    if ($PokeFrame -gt 0) {
        $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $PokeFrame"
        $lines += 'continue'
        foreach ($pk in $Pokes) { $lines += $pk }
        $lines += 'delete'
    }
    foreach ($f in $Frames) {
        $png = (Join-Path $out "$Tag-f$f.png")
        $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $f"
        $lines += 'continue'
        $lines += "printf `"AT %u\n`", gNdsBattlePlayablePacingPresentedFrames"
        $lines += "shell pwsh -NoProfile -File `"$sp\capwin.ps1`" -EmulatorProcessId $($p.Id) -Output `"$png`""
        $lines += 'delete'
    }
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "erpcap-$Tag.log") -RedirectStandardError (Join-Path $sp "erpcap-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "erpcap-$Tag.log") | Select-String -Pattern 'AT |Captured|rror' | ForEach-Object { $_.Line }

