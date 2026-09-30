param(
    [int]$Slot = 4,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-lab-exist-fix',
    [string]$Gkind = '2',
    [string]$Kinds = '34080263',
    [int]$Override = 7000,
    [int]$Step = 100,
    [int]$EndFrame = 1900,
    [int]$TimeoutSeconds = 2400
)
# event32 ledger over a whole lab match: live entries, holes, high-water,
# scripts/commands normalized, removals and failures every $Step presented
# frames. A curve that keeps climbing after the entry clips is a leak; one that
# plateaus is a working set.
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
$cmd = Join-Path $sp "ledger-$Tag.gdb"
$row = 'printf "L f=%u count=%u holes=%u hw=%u scripts=%u cmds=%u reuse=%u fail=%u removals=%u compactions=%u limit=%u\n", gNdsBattlePlayablePacingPresentedFrames, ''battleship_sys_objanim.c''::sNdsAObjEvent32NormalizedCount, ''battleship_sys_objanim.c''::sNdsAObjEvent32Holes, gNdsAObjEvent32NormalizedHighWater, gNdsAObjEvent32NormalizeScriptCount, gNdsAObjEvent32NormalizeCommandCount, gNdsAObjEvent32NormalizeReuseCount, gNdsAObjEvent32NormalizeFailCount, gNdsAObjEvent32ForgetHoleRemovals, gNdsAObjEvent32LedgerCompactions, ''battleship_sys_objanim.c''::sNdsAObjEvent32NormalizedLimit'
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'break main', 'continue', 'delete', "set var gNdsLabFourCpuGkind = $Gkind")
    if ($Kinds -ne '') { $lines += "set var gNdsLabFourCpuKinds = $Kinds" }
    if ($Override -gt 0) { $lines += "set var gNdsLabAObjEvent32CapacityOverride = $Override" }
    $lines += 'break __excpt_entry'
    for ($f = $Step; $f -le $EndFrame; $f += $Step) {
        $lines += "tbreak ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $f"
        $lines += 'continue'
        $lines += $row
    }
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $sp "ledger-$Tag.log") -RedirectStandardError (Join-Path $sp "ledger-$Tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $sp "ledger-$Tag.log") | Select-String -Pattern '^L f=|excpt|rror' | ForEach-Object { $_.Line }

