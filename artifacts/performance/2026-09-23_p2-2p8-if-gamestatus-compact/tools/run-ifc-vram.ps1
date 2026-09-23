param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$RunnerSlot = 7,
    [int]$TimeoutSeconds = 1800
)
# P2-2p8 Phase 3 (A7): the compact IFCommonGameStatus proof. One four-CPU
# match on a lab ROM: at the first frame-complete marker the OBJ VRAM (GO bank
# and the rest) and texture VRAM A+B are dumped and the tie-breaking score is
# poked (the stress match otherwise ends in a Sudden Death this target never
# finishes); at the end message the OBJ end bank is dumped after its bake; at
# the Results scene the heap and the compaction counters are printed. The same
# script on a compaction-off and a compaction-on build of the same tree must
# produce byte-identical dumps.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-if-gamestatus-compact'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') `
    -RunnerSlot $RunnerSlot -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$out = Join-Path $art $Arm
New-Item -ItemType Directory -Force -Path $out | Out-Null
$fwd = $out.Replace('\', '/')
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'ifc.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'ifc.melonds.err') `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $script = Join-Path $temp 'ifc.gdb'
    $stdout = Join-Path $temp 'ifc.gdb.out'
    $stderr = Join-Path $temp 'ifc.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $counters = 'printf "IFC count=%u move=%u fail=%u src=%u bytes=%u dropped=%u slots=%u nulled=%u baked=%u bakefail=%u decodes=%u prepfail=%u prepok=%u\n", gNdsRelocIfCompactCount, gNdsRelocIfCompactMoveCount, gNdsRelocIfCompactFailStage, gNdsRelocIfCompactSourceBytes, gNdsRelocIfCompactBytes, gNdsRelocIfCompactDropped, gNdsRelocIfCompactSlots, gNdsRelocIfCompactNulled, gNdsIFCommonEndBakedBytes, gNdsIFCommonEndBakedFailCount, gNdsIFCommonEndBakedDecodeCount, gNdsIFCommonNativeOamPrepareFailCount, gNdsIFCommonNativeOamPrepareSuccessCount'
    $countersOff = 'printf "IFC prepfail=%u prepok=%u\n", gNdsIFCommonNativeOamPrepareFailCount, gNdsIFCommonNativeOamPrepareSuccessCount'
    $symbols = & 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe' $elf | ForEach-Object { ($_ -split '\s+')[-1] }
    $hasCompact = $symbols -contains 'gNdsRelocIfCompactCount'
    $ctr = $(if ($hasCompact) { $counters } else { $countersOff })
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'set variable gSCManagerBattleState->players[0].score = 100',
        'printf "FIRST frame=%u freemin=%u free=%u arena=%u\n", gNdsRendererProfileFrameCount, gNdsTaskmanGeneralHeapFreeMin, (unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr, gNdsTaskmanArenaChosenSize',
        $ctr,
        "dump binary memory $fwd/obj-first.bin 0x06400000 0x06410000",
        "dump binary memory $fwd/tex-ab-first.bin 0x06800000 0x06840000",
        'tbreak ndsIFCommonNativeOamPrepareAnnouncement',
        'continue',
        'printf "ANNOUNCE game_set=%u frame=%u\n", $r0, gNdsRendererProfileFrameCount',
        'finish',
        "dump binary memory $fwd/obj-end.bin 0x06404400 0x06409500",
        $ctr,
        'tbreak mnVSResultsFuncStart',
        'continue',
        'printf "RESULTS frame=%u freemin=%u native=%u\n", gNdsRendererProfileFrameCount, gNdsTaskmanGeneralHeapFreeMin, gNdsRendererNativeFailure.count',
        $ctr,
        'printf "DONE\n"'
    ))
    $gdbProcess = Start-Process -FilePath $gdb -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $done = $false
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        if ((Test-Path -LiteralPath $stdout) -and ((Get-Content -LiteralPath $stdout -Raw) -match 'DONE')) { $done = $true; break }
        if ($gdbProcess.HasExited) { break }
    }
    if (Test-Path -LiteralPath $stdout) { Copy-Item -LiteralPath $stdout -Destination (Join-Path $out 'gdb.log') -Force }
    if (Test-Path -LiteralPath $stderr) { Copy-Item -LiteralPath $stderr -Destination (Join-Path $out 'gdb.err') -Force }
    if (-not $done) { throw "ifc run $Arm did not reach Results within ${TimeoutSeconds}s" }
    if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }
    Get-Content -LiteralPath (Join-Path $out 'gdb.log') | Where-Object { $_ -match '^(FIRST|IFC|ANNOUNCE|RESULTS)' }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
