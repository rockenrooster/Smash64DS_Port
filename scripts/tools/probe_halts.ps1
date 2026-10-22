param([string]$Build = 'walk-u16', [int]$Slot = 10, [string]$Tag = 'hp1', [string[]]$Pokes = @(),
      [int]$EndScene = 9999, [int]$EndPrev = 9999, [string[]]$EndPrints = @())
# One gdb session: pokes at main, then hardware breakpoints on the halt paths
# (malloc overflow, data/prefetch abort, undefined instruction), each scene
# entry printed, and a backtrace at the first halt. Ends at scene $EndScene
# entered from $EndPrev. Record: builds/probes/<Tag>.log ("x " lines).
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
. (Join-Path $S64Repo 'scripts\lib\melonds.ps1')
$context = Initialize-MelonDSVerifierContext -Root $S64Repo -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$romPath = Join-Path $S64Repo "builds\$Build\smash64ds.nds"
$elfPath = Join-Path $S64Repo "builds\$Build\smash64ds.elf"
$log = Join-Path $S64Probes "$Tag.log"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port `
    -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($romPath) -WorkingDirectory (Split-Path $melon) -WindowStyle Minimized -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $cmd = Join-Path $S64Probes "$Tag.gdb"
    (@('set pagination off', 'set confirm off', 'set remote X-packet off', 'set remotetimeout 30',
       "target remote localhost:$port", 'break main', 'continue', 'delete') + $Pokes + @(
       'break ndsSceneManagerEnter', 'commands', 'silent',
       'printf "x enter scene=%u prev=%u heapmin=%u native=%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsTaskmanGeneralHeapFreeMin, gNdsRendererNativeFailure.count',
       "if (gSCManagerSceneData.scene_curr == $EndScene) && (gSCManagerSceneData.scene_prev == $EndPrev)",
       'printf "x END reached\n"') + $EndPrints + @('detach', 'quit', 'end',
       'continue', 'end',
       'hbreak ndsSyMallocOverflowHalt', 'hbreak __arm_excpt_dabt', 'hbreak __arm_excpt_pabt', 'hbreak __arm_excpt_und',
       'continue',
       'printf "x HALT pc=%x lr=%x scene=%u\n", $pc, $lr, gSCManagerSceneData.scene_curr',
       'printf "x lr-8=%x\n", $lr-8', 'info symbol ($lr-8)', 'x/3i ($lr-8)',
       'printf "x oom req=%u head=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom',
       'bt 14', 'detach', 'quit')) | Set-Content -LiteralPath $cmd -Encoding ascii
    & $S64Gdb -batch -x $cmd $elfPath *> $log
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content $log | Select-String -Pattern '^(x |#)|No symbol|rror|in section' | ForEach-Object { $_.Line }
