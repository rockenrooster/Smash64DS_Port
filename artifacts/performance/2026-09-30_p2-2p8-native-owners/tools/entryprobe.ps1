param(
    [int]$Slot = 1,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-baked2',
    [string]$Gkind = '6',
    [string]$Kinds = '51054849',
    [int]$FromFrame = 190,
    [int]$EndFrame = 200,
    [int[]]$Lines = @(5832, 5839, 5844, 5870, 5928, 5940, 5947, 5966, 5981, 6008, 6021, 6048, 6057, 6064, 6075, 6101, 6105, 6129, 6164, 6173, 6183, 6198, 6206, 6235, 6240, 6248, 6864, 6963),
    [int]$TimeoutSeconds = 900
)
# Which refusal in ndsRendererSubmitNativeEntryEffect fires: one line
# breakpoint per `return FALSE`, armed from $FromFrame.
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
$cmd = Join-Path $sp "entry-$Tag.gdb"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru -WindowStyle Minimized
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $g = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete', "set var gNdsLabFourCpuGkind = $Gkind")
    if ($Kinds -ne '') { $g += "set var gNdsLabFourCpuKinds = $Kinds" }
    $g += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $FromFrame"
    $g += 'continue'
    $g += 'delete'
    foreach ($l in $Lines) {
        $g += "break nds_renderer_native_common.c:$l"
        $g += 'commands'
        $g += '  silent'
        $g += "  printf `"LINE $l f=%u\n`", gNdsBattlePlayablePacingPresentedFrames"
        $g += '  continue'
        $g += 'end'
    }
    $g += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $EndFrame"
    $g += 'continue'
    $g += 'printf "END f=%u\n", gNdsBattlePlayablePacingPresentedFrames'
    $g += 'kill', 'quit'
    $g | Set-Content -LiteralPath $cmd -Encoding ascii
    $gp = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "entry-$Tag.log") -RedirectStandardError (Join-Path $sp "entry-$Tag.err")
    if (-not $gp.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $gp.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "entry-$Tag.log") | Select-String -Pattern 'LINE|END|rror|Breakpoint [0-9]+ at' | ForEach-Object { $_.Line }
