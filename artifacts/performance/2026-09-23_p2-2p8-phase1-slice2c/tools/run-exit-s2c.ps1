param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Admit = 0,
    [int]$Route = 0,
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    # unused since the one-shot Results stops (kept for the command lines)
    [int]$Tic = 160,
    [int]$TimeoutSeconds = 2400,
    [int]$BattleTimeoutSeconds = 900,
    # >0: the players[0].score poke that breaks the stress match's tie (see
    # the README: the tie starts a Sudden Death this target never finishes)
    [int]$Score = 0
)
# P2-2p8 Phase 1 slice 2c: the battle-exit proof. One whole four-CPU match
# played out past GAME SET into the VS Results scene on runner slot 9 / GDB
# 3423: the admission word is poked (whole u32) at the first frame-complete
# marker, as the sampler does. Every battle exit (the match and any Sudden
# Death after a tie) and scene entry prints its state; the emulator halts at
# the Results scene start, at the Results tint (tic 180) and at the place row
# (tic 290), prints the exit counters (the lab flushes them at the exit and at
# the bank D return) and the window is captured at tic 290.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
. (Join-Path $root 'scripts\lib\melonds-screenshot.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') -RunnerSlot 9 `
    -GdbPort 3423 -GdbPortExplicit:$true -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot 9
$log = Join-Path $art "$Arm-exit.log"
$png = Join-Path $art "$Arm-results.png"
$configState = $null
$emulator = $null
$start = Get-Date
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'exit-s2c.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'exit-s2c.melonds.err') `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $script = Join-Path $temp 'exit-s2c.gdb'
    $stdout = Join-Path $temp 'exit-s2c.gdb.out'
    $stderr = Join-Path $temp 'exit-s2c.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        "set variable gNdsFtrLeanRoute = $Route",
        "set variable gNdsFtrLeanAdmit = $Admit",
        $(if ($Score -gt 0) { "set variable gSCManagerBattleState->players[0].score = $Score" } else { 'echo' }),
        'printf "POKED route=%u admit=%u frame=%u score0=%d\n", gNdsFtrLeanRoute, gNdsFtrLeanAdmit, gNdsRendererProfileFrameCount, gSCManagerBattleState->players[0].score',
        # Staged stops, no per-frame condition. Every battle exit (the match,
        # then any Sudden Death the tie starts: scvsbattle.c re-enters VSBattle
        # before Results) prints its state and continues; the run halts at
        # the Results scene start, then at the Results frame recorder's
        # ($Tic+1)th hit.
        'set $exits = 0',
        'break *ndsFtrLeanAdmitBattleExit',
        'commands',
        'silent',
        'set $exits = $exits + 1',
        'printf "BATTLE-EXIT n=%d frame=%u status=%u time_remain=%u sudden=%u lent=%u takes=%u returns=%u missed=%u admit_runs=%u admit_frame=%u applied=%u entries=%u fails=%u carved=%u exit_runs=%u released=%u\n", $exits, gNdsRendererProfileFrameCount, gSCManagerBattleState->game_status, gSCManagerBattleState->time_remain, gSCManagerSceneData.is_suddendeath, gNdsVramBankDLent, gNdsVramBankDTakes, gNdsVramBankDReturns, gNdsVramBankDMissedExits, gNdsFtrAdmitLab.runs, gNdsFtrAdmitLab.frame, gNdsFtrAdmitLab.applied, gNdsFtrAdmitLab.entries_admitted, gNdsFtrAdmitLab.fails, gNdsFtrCarveLab.carved[0], gNdsFtrAdmitLab.exit_runs, gNdsFtrAdmitLab.exit_released',
        'continue',
        'end',
        'break ndsSceneManagerEnter',
        'commands',
        'silent',
        'printf "SCENE-ENTER curr=%u prev=%u sudden=%u enters=%u frame=%u lent=%u takes=%u returns=%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.is_suddendeath, gNdsSceneManagerEnterCount, gNdsRendererProfileFrameCount, gNdsVramBankDLent, gNdsVramBankDTakes, gNdsVramBankDReturns',
        'continue',
        'end',
        'tbreak mnVSResultsFuncStart',
        'continue',
        'printf "RESULTS-START frame=%u scene=%u exits=%d returns=%u missed=%u lent=%u exit_runs=%u released=%u\n", gNdsRendererProfileFrameCount, gSCManagerSceneData.scene_curr, $exits, gNdsVramBankDReturns, gNdsVramBankDMissedExits, gNdsVramBankDLent, gNdsFtrAdmitLab.exit_runs, gNdsFtrAdmitLab.exit_released',
        'delete',
        # Per-frame functions cannot be counted with an ignore count on this
        # emulator (a breakpoint on a function already hot never reports), so
        # the Results stops are the source's one-shot tic calls
        # (mnvsresults.c mnVSResultsDrawResultsTimeRoyal): the tint at tic 180,
        # then the place row at tic 290, when every row but it is on screen.
        'tbreak mnVSResultsMakeTint',
        'continue',
        'printf "RESULTS-TIC tics=%d returns=%u missed=%u lent=%u exit_runs=%u released=%u orphans=%u\n", sMNVSResultsTotalTimeTics, gNdsVramBankDReturns, gNdsVramBankDMissedExits, gNdsVramBankDLent, gNdsFtrAdmitLab.exit_runs, gNdsFtrAdmitLab.exit_released, gNdsFtrAdmitLab.exit_orphans',
        'tbreak mnVSResultsMakePlaceRow',
        'continue',
        'delete',
        'printf "EXIT runs=%u released=%u orphans=%u safety=%u done=%u regions=%u\n", gNdsFtrAdmitLab.exit_runs, gNdsFtrAdmitLab.exit_released, gNdsFtrAdmitLab.exit_orphans, gNdsFtrAdmitLab.exit_safety, gNdsFtrAdmitLab.done, gNdsFtrAdmitLab.regions',
        'printf "BANKD takes=%u returns=%u missed=%u lent=%u refused=%u\n", gNdsVramBankDTakes, gNdsVramBankDReturns, gNdsVramBankDMissedExits, gNdsVramBankDLent, gNdsVramBg3RefusedWrites',
        'printf "ADMIT runs=%u frame=%u applied=%u entries=%u fails=%u bytes=%u in_d=%u\n", gNdsFtrAdmitLab.runs, gNdsFtrAdmitLab.frame, gNdsFtrAdmitLab.applied, gNdsFtrAdmitLab.entries_admitted, gNdsFtrAdmitLab.fails, gNdsFtrAdmitLab.bytes_admitted, gNdsFtrAdmitLab.bytes_in_d',
        'printf "CARVE opens=%u carved=%u retires=%u retire_regions=%u region_returns=%u exit_released=%u\n", gNdsFtrCarveLab.opens, gNdsFtrCarveLab.carved[0], gNdsFtrCarveLab.retires, gNdsFtrCarveLab.retire_regions, gNdsFtrCarveLab.region_returns, gNdsFtrCarveLab.exit_released',
        'printf "RESULTS fighters=%u native=%u freemin=%u starts=%u\n", gNdsVSResultsFighterCount, gNdsRendererNativeFailure.count, gNdsTaskmanGeneralHeapFreeMin, gNdsVSResultsStartCount',
        'printf "SCENE curr=%u presented=%u\n", gSCManagerSceneData.scene_curr, gNdsRendererProfileFrameCount',
        'printf "REACHED-TIC=%d\n", sMNVSResultsTotalTimeTics'
    ))
    $gdbProcess = Start-Process -FilePath $gdb -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $battleDeadline = (Get-Date).AddSeconds($BattleTimeoutSeconds)
    $reached = $null
    $noExit = $false
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        $text = ''
        if (Test-Path -LiteralPath $stdout) {
            $text = Get-Content -LiteralPath $stdout -Raw
            if ($text -match 'REACHED-TIC=(\d+)') { $reached = [int]$Matches[1]; break }
        }
        if (((Get-Date) -gt $battleDeadline) -and ($text -notmatch 'BATTLE-EXIT|SCENE-ENTER|RESULTS-START')) {
            $noExit = $true
            break
        }
        if ($gdbProcess.HasExited) { break }
    }
    if (Test-Path -LiteralPath $stdout) { Copy-Item -LiteralPath $stdout -Destination $log -Force }
    if ($noExit) { Add-Content -LiteralPath $log -Value "`nNO BATTLE EXIT within ${BattleTimeoutSeconds}s" }
    if ($null -eq $reached) {
        Add-Content -LiteralPath $log -Value "`nNEVER REACHED Results tic $Tic within ${TimeoutSeconds}s"
        throw "Never reached Results tic $Tic"
    }
    # the capture helper writes only under artifacts/visibility; copy it here
    $visPng = Join-Path $root "artifacts/visibility/p2p8-slice2c/$Arm-results.png"
    & (Join-Path $root 'scripts/capture-running-melonds-window.ps1') `
        -EmulatorProcessId $emulator.Id -Output $visPng
    Copy-Item -LiteralPath $visPng -Destination $png -Force
    Add-Content -LiteralPath $log -Value ("`nCAPTURED $png elapsed=" + ((Get-Date) - $start).TotalSeconds)
    if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
