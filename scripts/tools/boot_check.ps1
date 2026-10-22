param([string]$Build = 'play-p7z', [int]$Slot = 11, [int]$Seconds = 25, [string]$Tag = 'boot')
# Smoke test of a play or walk ROM: boot it on runner slot $Slot for $Seconds,
# then read the scene, the storage counters (requests, reads, failures,
# resends, stale replies), the GX stall count and the ARM7 storage health.
# A healthy boot reaches a scene with fail=0.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
. (Join-Path $S64Repo 'scripts\lib\melonds.ps1')
$context = Initialize-MelonDSVerifierContext -Root $S64Repo -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$romPath = Join-Path $S64Repo "builds\$Build\smash64ds.nds"
$elfPath = Join-Path $S64Repo "builds\$Build\smash64ds.elf"
$cmd = Join-Path $S64Probes "$Tag.gdb"
$log = Join-Path $S64Probes "$Tag.log"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port `
    -Persistent:([bool]$context.PersistentConfig) -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($romPath) -WorkingDirectory (Split-Path $melon) -WindowStyle Minimized -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    Start-Sleep -Seconds $Seconds
    @('set pagination off', 'set confirm off', 'set remotetimeout 20', "target remote localhost:$port",
      'printf "scene=%u reqs=%u reads=%u fail=%u resends=%u stale=%u gxstall=%u\n", gSCManagerSceneData.scene_curr, gNdsAudioStorageRequests, gNdsAudioStorageReads, gNdsAudioStorageFailures, gNdsAudioStorageResends, gNdsAudioStorageStaleReplies, gNdsGxStallCount',
      'printf "A7 received=%u completed=%u current=%#x pending_max=%u vblanks=%u\n", sStorageHealth.received, sStorageHealth.completed, sStorageHealth.current, sStorageHealth.pending_max, sStorageHealth.vblanks',
      'detach', 'quit') | Set-Content -LiteralPath $cmd -Encoding ascii
    & $S64Gdb -batch -x $cmd $elfPath *> $log
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content $log | Select-String -Pattern '^(scene|A7)' | ForEach-Object { $_.Line }
Get-Content $log | Select-String 'No symbol|Error' | Select-Object -First 3 | ForEach-Object { $_.Line }
