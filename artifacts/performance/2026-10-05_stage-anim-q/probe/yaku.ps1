param([int]$Slot = 9, [Parameter(Mandatory)][string]$Tag, [string]$Build = 'build-lab-clean1005q2',
      [string]$Sets = '', [string]$Frames = '100;400;800;1200', [int]$TimeoutSeconds = 1800)
# Yakumono (moving platform) DObj transforms at given presented frames: the
# stage-anim Q A/B's motion check (run once per arm, compare the dumps).
$ErrorActionPreference = 'Stop'
$SetList = @($Sets -split ';' | Where-Object { $_ })
$FrameList = @($Frames -split ';' | Where-Object { $_ })
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
$sp = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\31e5c78d-38d0-4d75-b683-91723230d2cc\scratchpad'
$context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$target = 'smash64ds-p2-fourcpu-tickhud-hwtri'
$rom = Join-Path $root "builds\$Build\$target.nds"
$elf = Join-Path $root "builds\$Build\$target.elf"
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$cmd = Join-Path $sp "yaku-$Tag.gdb"
$log = Join-Path $sp "yaku-$Tag.log"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru -WindowStyle Minimized
    Wait-MelonDSGdbListener -Process $p -Port $port -Attempts 400 | Out-Null
    $lines = @('set pagination off', 'set confirm off',
        "target remote localhost:$port", 'break main', 'continue', 'delete')
    foreach ($s in $SetList) { $kv = $s -split '='; $lines += "set var $($kv[0]) = $($kv[1])" }
    foreach ($f in $FrameList) {
        $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $f", 'continue', 'delete'
        $lines += "printf `"FRAME $f n=%d\n`", gMPCollisionYakumonosNum"
        for ($i = 0; $i -lt 8; $i++) {
            $d = "gMPCollisionYakumonoDObjs->dobjs[$i]"
            $lines += "if ($i < gMPCollisionYakumonosNum) && ($d != 0)",
                "printf `"Y$i t=%.4f,%.4f,%.4f r=%.4f,%.4f,%.4f s=%.4f\n`", $d->translate.vec.f.x, $d->translate.vec.f.y, $d->translate.vec.f.z, $d->rotate.vec.f.x, $d->rotate.vec.f.y, $d->rotate.vec.f.z, $d->scale.vec.f.x",
                'end'
        }
    }
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $log -RedirectStandardError (Join-Path $sp "yaku-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content $log | Select-String -Pattern '^(FRAME|Y[0-9])' | ForEach-Object { $_.Line }
