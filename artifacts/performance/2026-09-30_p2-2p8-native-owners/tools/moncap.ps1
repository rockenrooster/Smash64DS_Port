param(
    [int]$Slot = 5,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-baked2',
    [string]$Gkind = '6',
    [string]$Kinds = '',
    # Poke Ball Pokemon, 1-based (1 Onix .. 13 Mew): dITManagerForceMonsterKind
    [int]$Force = 0,
    # item_toggles (0 keeps all); 524288 = Poke Balls only
    [int]$Toggles = 524288,
    [int]$Rate = 5,
    # capture at the first frame whose baked draw count passes each threshold
    [int[]]$Thresholds = @(4, 30, 90),
    [int]$TimeoutSeconds = 1200
)
# Owner captures for the baked item roots: Poke Balls only at the highest
# rate, one forced Pokemon, and a PrintWindow capture on the first presented
# frame after the baked draw counter passes each threshold (so the shot has
# the object on screen). The match's last frame ends the watch.
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
$cmd = Join-Path $sp "mcap-$Tag.gdb"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete', "set var gNdsLabFourCpuGkind = $Gkind",
        "set var gNdsLabItemToggles = $Toggles", "set var gNdsLabItemRate = $Rate",
        "set var dITManagerForceMonsterKind = $Force")
    if ($Kinds -ne '') { $lines += "set var gNdsLabFourCpuKinds = $Kinds" }
    foreach ($t in $Thresholds) {
        $png = (Join-Path $out "$Tag-b$t.png")
        $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsItemBakedDrawCount > $t || gNdsBattlePlayablePacingPresentedFrames > 1960"
        $lines += 'continue'
        $lines += "printf `"AT %u baked=%u last=%x fail=%u\n`", gNdsBattlePlayablePacingPresentedFrames, gNdsItemBakedDrawCount, gNdsItemBakedLastRoot, gNdsItemBakedSubmitFailCount"
        $lines += "shell pwsh -NoProfile -File `"$sp\capwin.ps1`" -EmulatorProcessId $($p.Id) -Output `"$png`""
        $lines += 'delete'
    }
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "mcap-$Tag.log") -RedirectStandardError (Join-Path $sp "mcap-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "mcap-$Tag.log") | Select-String -Pattern 'AT |rror' | ForEach-Object { $_.Line }
