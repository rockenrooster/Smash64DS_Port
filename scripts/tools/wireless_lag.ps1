param([Parameter(Mandatory)][string]$Build, [int]$MatchCount = 3, [int]$TimeoutSeconds = 1500, [string]$Tag = 'lag',
      [ValidateRange(2, 4)][int]$Consoles = 2)
# Two to four linked consoles (melonDS-mp, MELONDS_MP_INSTANCES, gdb 3600 + 10 a console) run
# the lab autopilot of a wireless lab ROM in builds/<Build> (NDS_NET_LAB_LOBBY
# + SWEEP). Each console prints its VBlank count at every scene entry, at the
# stage select's commit/START paths, at the battle's first lockstep gates and
# at each battle's end (matches, desyncs, aborts, stalls), so each console's
# stage-load time and the host's wait for the guest read directly. Records:
# builds/probes/<Tag>-<console>.log. Console VBlank clocks drift: compare
# durations, not stamps (memory melonds-mp-vblank-clocks-drift).
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
$melon = Join-Path $S64Repo 'emulators\melonds-mp\melonDS.exe'
$rom = Join-Path $S64Repo "builds\$Build\smash64ds.nds"
$elf = Join-Path $S64Repo "builds\$Build\smash64ds.elf"
$env:MELONDS_MP_INSTANCES = [string]$Consoles
$env:MELONDS_NO_ACTIVATE = '1'
$p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru -WindowStyle Minimized
$gps = @()
try {
    foreach ($i in 0..($Consoles - 1)) {
        $port = 3600 + 10 * $i
        $deadline = (Get-Date).AddSeconds(60)
        while (-not (Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue)) {
            if ((Get-Date) -gt $deadline) { throw "no gdb listener on $port" }
            Start-Sleep -Milliseconds 500
        }
        $cmd = Join-Path $S64Probes "$Tag-$i.gdb"
        $log = Join-Path $S64Probes "$Tag-$i.log"
        @('set pagination off', 'set confirm off', 'set remotetimeout 20', "target remote localhost:$port",
          'set remote X-packet off', 'set breakpoint always-inserted on',
          'tbreak main', 'continue',
          # bp 2: scene entries; arms bp 3 at the battle's entry
          'break ndsSceneManagerEnter', 'commands', 'silent',
          "printf `"S i=$i vb=%u scene=%u role=%u heap=%u\n`", sVBlankCount, gSCManagerSceneData.scene_curr, gNdsNetAutopilotRole, gNdsTaskmanGeneralHeapFreeMin",
          'if gSCManagerSceneData.scene_curr == 22', 'enable 3', 'set $gates = 0', 'end',
          'continue', 'end',
          # bp 3: the battle's first two lockstep gates
          'break ndsNetBattleGate', 'commands', 'silent',
          "printf `"G i=$i vb=%u batch=%u stallmax=%u\n`", sVBlankCount, gNdsNetBatch, gNdsNetStallMaxVBlanks",
          'set $gates = $gates + 1', 'if $gates >= 2', 'disable 3', 'end',
          'continue', 'end', 'disable 3',
          # bp 4: the host's start offer (the SSS commit)
          'break ndsNetLobbyHostStartMatch', 'commands', 'silent',
          "printf `"H i=$i vb=%u start offer\n`", sVBlankCount", 'continue', 'end',
          'break ndsNetLockstepConfigure', 'commands', 'silent',
          "printf `"C i=$i vb=%u lockstep configure\n`", sVBlankCount", 'continue', 'end',
          'break ndsNetBattleEnd', 'commands', 'silent',
          "printf `"END i=$i vb=%u m=%u desync=%u abort=%u stallmax=%u stalls=%u\n`", sVBlankCount, gNdsNetAutopilotMatches, gNdsNetDesyncs, gNdsNetAborted, gNdsNetStallMaxVBlanks, gNdsNetStallBatches",
          "if gNdsNetAutopilotMatches >= $MatchCount", 'detach', 'quit', 'end',
          'continue', 'end',
          'break __arm_excpt_dabt', 'commands', 'silent', "printf `"VEC i=$i fault pc=%#x\n`", `$lr - 8", 'bt 8', 'detach', 'quit', 'end',
          'continue') | Set-Content -LiteralPath $cmd -Encoding ascii
        $gps += Start-Process -FilePath $S64Gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
            -RedirectStandardOutput $log -RedirectStandardError (Join-Path $S64Probes "$Tag-$i.err")
    }
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    foreach ($g in $gps) {
        $left = [int](($deadline - (Get-Date)).TotalMilliseconds)
        if ($left -lt 1 -or -not $g.WaitForExit($left)) { "TIMEOUT gdb $($g.Id)" }
    }
} finally {
    foreach ($g in $gps) { if (-not $g.HasExited) { Stop-Process -Id $g.Id -Force } }
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
}
foreach ($i in 0..($Consoles - 1)) {
    Get-Content (Join-Path $S64Probes "$Tag-$i.log") | Where-Object { $_ -match '^(S|G|H|C|END|VEC) ' }
    Get-Content (Join-Path $S64Probes "$Tag-$i.err") -ErrorAction SilentlyContinue | Select-String 'No symbol|Error|error' |
        Select-Object -First 3 | ForEach-Object { "ERR i=${i}: $($_.Line)" }
}
