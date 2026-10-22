param(
    [int]$Slot = 9,
    [Parameter(Mandatory)][string]$Tag,
    [string]$Build = 'build-sweep-s61',
    [string]$Kinds = '0x01010101',
    [string]$Content = '0x00000014',
    [string]$Gkind = '6',
    [switch]$Two,
    # name=value words poked at main
    [string[]]$Sets = @(),
    # Schedule rows "frame|action": "in=<hex input>" (sets the forced input on
    # slot 0 and restarts its tick count: a fresh press of `held` ticks;
    # encoding in memory lab-forced-input-encoding), "held=<n>", "shot", or
    # "gdb=<raw line>".
    [string[]]$Plan = @(),
    [int]$TimeoutSeconds = 1200,
    # gdb lines (dprintf ...) set after main and again after each frame's delete
    [string[]]$Persist = @()
)
# Frame captures of a forced-move schedule on a four-CPU ROM (gate or lab):
# words poked at main, the guest halted at the presented-frame marker of each
# scheduled frame; inputs changed there and the melonDS window captured
# (no-activate PrintWindow) with fighter 0's motion and clip frame. Record:
# builds/probes/<Tag>.log ("SHOT"/"END" lines); captures in
# artifacts/visibility/probes/<Tag>/.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
. (Join-Path $S64Repo 'scripts\lib\melonds.ps1')
$out = Join-Path $S64Shots $Tag
New-Item -ItemType Directory -Force $out | Out-Null
$context = Initialize-MelonDSVerifierContext -Root $S64Repo -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$rom = Join-Path $S64Repo "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elf = Join-Path $S64Repo "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.elf"
$cmd = Join-Path $S64Probes "$Tag.gdb"
$log = Join-Path $S64Probes "$Tag.log"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port `
    -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru -WindowStyle Minimized
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $lines = @('set pagination off', 'set confirm off', "target remote localhost:$port", 'set remote X-packet off',
        'break main', 'continue', 'delete',
        "set var gNdsLabFourCpuGkind = $Gkind", "set var gNdsLabFourCpuKinds = $Kinds",
        "set var gNdsLabP4Content = $Content", 'set var gNdsLabForceInputSlots = 3',
        'set var gNdsLabForceInput = 0', 'set var gNdsLabForceInputPeriod = 0x00040800')
    if ($Two) { $lines += 'set var gNdsLabFourCpuTwo = 1' }
    foreach ($s in $Sets) {
        $kv = $s -split '='
        $lines += "set var $($kv[0]) = $($kv[1])"
    }
    $lines += $Persist
    $rows = $Plan | ForEach-Object { $f, $a = $_ -split '\|', 2; [pscustomobject]@{ F = [int]$f; A = $a } } | Sort-Object F
    foreach ($g in ($rows | Group-Object F | Sort-Object { [int]$_.Name })) {
        $f = [int]$g.Name
        $lines += "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $f"
        $lines += 'continue'
        foreach ($r in $g.Group) {
            if ($r.A -like 'in=*') {
                $lines += "set var gNdsLabForceInput = $($r.A.Substring(3))"
                $lines += 'set var sNdsLabForceInputTicks[0] = 0'
                $lines += 'set var sNdsLabForceInputTicks[1] = 0'
            } elseif ($r.A -like 'held=*') {
                $lines += "set var gNdsLabForceInputPeriod = $($r.A.Substring(5))"
            } elseif ($r.A -like 'gdb=*') {
                $lines += $r.A.Substring(4)
            } elseif ($r.A -eq 'shot') {
                $png = Join-Path $out ('{0}-f{1:d4}.png' -f $Tag, $f)
                $lines += 'printf "SHOT f=%u motion=%d clip=%d status=%d\n", gNdsBattlePlayablePacingPresentedFrames, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->motion_id, (int)gSCManagerBattleState->players[0].fighter_gobj->anim_frame, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id'
                $lines += "shell pwsh -NoProfile -File `"$S64Repo\scripts\capture-running-melonds-window.ps1`" -EmulatorProcessId $($p.Id) -Output `"$png`""
            }
        }
        $lines += 'delete'
        $lines += $Persist
    }
    $lines += 'printf "END native=%u lean_declines=%u entry_misses=%u\n", gNdsNativeFailureLabCount, gNdsFtrLeanDeclineCount, gNdsP4EntryEffectMisses'
    $lines += 'kill', 'quit'
    $lines | Set-Content -LiteralPath $cmd -Encoding ascii
    $g2 = Start-Process -FilePath $S64Gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $log -RedirectStandardError (Join-Path $S64Probes "$Tag.err")
    if (-not $g2.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g2.Id -Force; 'TIMEOUT' }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content $log | Select-String -Pattern 'SHOT|END|rror|No symbol' | ForEach-Object { $_.Line }
