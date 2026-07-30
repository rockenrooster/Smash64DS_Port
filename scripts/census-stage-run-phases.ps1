[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4621,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task103-census',
    [switch]$NoBuild,
    [ValidateRange(1,1000000)][int]$StartFrame = 439,
    [ValidateRange(2,600)][int]$WindowFrames = 60,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# Task 103. Splits the stage replay run into its three spans, to attribute the
# ~331,300 ticks/frame that Task 99 left as "89% fixed" and Task 100 proved are
# not pixels.
#
# The two live explanations separate cleanly here:
#   push  large  -> the cost is GX FIFO stall. That is geometry-side
#                   backpressure and is real, but it would mean Task 55 E2's
#                   "-355 words -> +64" needs re-reading, because stall would
#                   then scale with drain time rather than with word count.
#   begin/tail large -> the cost is per-run scaffolding. Run count is the
#                   currency, exactly as Task 99 §4's refined rule predicts,
#                   and the lever is run structure.
#
# Measures in place. Task 99 arm C varied run count by culling 27 of 54 runs and
# measured +109,888 because that disarms the Task 36 capture-once replay, so
# this instrument never removes a run.
#
# Counting/timing build: the four cpuGetTiming reads per run perturb the ITCM
# body of ndsRendererTask36ReplayRun, so the ROM it produces must never be used
# for A/B timing. Read the partition, not the totals.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

$counters = @(
    'gNdsTask103BeginTicks',
    'gNdsTask103PushTicks',
    'gNdsTask103TailTicks',
    'gNdsTask103RunCount',
    'gNdsTask103WordCount',
    'gNdsTask103GenericTicks',
    'gNdsTask103GenericRunCount',
    'gNdsTask103GenericTriangles',
    'gNdsTask103IterTicks',
    'gNdsTask103IterCount',
    'gNdsTask103CommitTicks',
    'gNdsTask103CommitCount',
    'gNdsTask103MaterialTicks',
    'gNdsTask103PrepareTicks',
    'gNdsTask103PrepareCount',
    'gNdsTask103TraversalTicks',
    'gNdsTask103TraversalCount',
    'gNdsTask103DisplayTicks',
    'gNdsTask103DisplayCount',
    'gNdsTask103FinishTicks',
    'gNdsTask103FinishCount',
    'gNdsTask103PrepAdmitTicks',
    'gNdsTask103PrepValidateTicks',
    'gNdsTask103PrepMatrixTicks',
    'gNdsTask103PrepMaterialTicks',
    'gNdsTask103PrepConfigTicks',
    'gNdsTask103PrepOwnerTicks',
    'gNdsTask103PrepCalls',
    'gNdsTask103OwnValidateTicks',
    'gNdsTask103OwnInitTicks',
    'gNdsTask103OwnInitCount',
    'gNdsTask103OwnReuseTicks',
    'gNdsTask103OwnReuseCount',
    'gNdsTask103OwnReuseMissCount',
    'gNdsTask103OwnStateSpanTicks',
    'gNdsTask103OwnStateSpanCount',
    'gNdsTask103OwnPrepareRunTicks',
    'gNdsTask103OwnPrepareRunCount',
    'gNdsTask103RunHeadTicks',
    'gNdsTask103RunDenseTicks',
    'gNdsTask103RunDenseCount',
    'gNdsTask103RunNearCount'
)

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'stagerun.gdb'
$gdbOut = Join-Path $temp 'stagerun.gdb.out'
$gdbErr = Join-Path $temp 'stagerun.gdb.err'
$emulatorOut = Join-Path $temp 'stagerun.melonds.out'
$emulatorErr = Join-Path $temp 'stagerun.melonds.err'
$configState = $null
$emulator = $null

function New-SampleCommands {
    param([string]$Tag, [int]$Frame)

    $fields = $counters -join ', '
    $format = (, '%u' * $counters.Count) -join ','
    # `while`, not `if`: a top-level `if ... continue ... end` resumes exactly
    # once, so the sample lands one frame past the previous stop rather than at
    # the requested frame (Task 96 standing rule).
    @(
        "while gNdsBattlePlayablePacingPresentedFrames < $Frame",
        'continue',
        'end',
        ("printf `"STGRUN=$Tag,%u,$format\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, $fields")
    )
}

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" NDS_TASK103_STAGE_RUN_PHASE=1
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required stage-run census file is missing: $path"
        }
    }

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $endFrame = $StartFrame + $WindowFrames
    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        'end',
        'continue'
    ) + (New-SampleCommands -Tag 'A' -Frame $StartFrame) + @(
        'continue'
    ) + (New-SampleCommands -Tag 'B' -Frame $endFrame) + @('detach')

    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "Stage-run census exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "Stage-run census GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $out = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $out | Where-Object { $_ -match "^STGRUN=$tag," } | Select-Object -First 1
        if (-not $line) {
            $out | Write-Host
            throw "Stage-run census produced no sample $tag."
        }
        $parts = ($line -replace "^STGRUN=$tag,", '') -split ','
        $s = [ordered]@{ frame = [uint32]$parts[0] }
        for ($i = 0; $i -lt $counters.Count; $i++) {
            $s[$counters[$i]] = [uint32]$parts[$i + 1]
        }
        $samples[$tag] = $s
    }

    $frames = [int]$samples['B'].frame - [int]$samples['A'].frame
    if ($frames -le 0) {
        throw "Stage-run census window is $frames frames; the two stops did not advance."
    }
    $delta = [ordered]@{}
    foreach ($c in $counters) {
        $delta[$c] = [int64]$samples['B'][$c] - [int64]$samples['A'][$c]
    }

    $runs = [double]$delta['gNdsTask103RunCount']
    if ($runs -le 0) {
        throw 'Stage-run census recorded no replay runs; the replay path did not execute.'
    }
    $begin = [double]$delta['gNdsTask103BeginTicks']
    $push = [double]$delta['gNdsTask103PushTicks']
    $tail = [double]$delta['gNdsTask103TailTicks']
    $words = [double]$delta['gNdsTask103WordCount']
    $total = $begin + $push + $tail

    Write-Host ""
    Write-Host ("Task 103 -- stage replay run partition over $frames presented frames")
    Write-Host ("(frames {0} .. {1})" -f $samples['A'].frame, $samples['B'].frame)
    Write-Host ""
    Write-Host ("runs        {0,10:N1} / frame" -f ($runs / $frames))
    Write-Host ("words       {0,10:N1} / frame   ({1:N1} per run)" -f `
        ($words / $frames), ($words / $runs))
    Write-Host ""
    Write-Host ("span            ticks/frame   ticks/run     share")
    Write-Host ("begin-run     {0,12:N0} {1,11:N0} {2,9:P1}" -f `
        ($begin / $frames), ($begin / $runs), ($begin / $total))
    Write-Host ("word push     {0,12:N0} {1,11:N0} {2,9:P1}" -f `
        ($push / $frames), ($push / $runs), ($push / $total))
    Write-Host ("tail+endbatch {0,12:N0} {1,11:N0} {2,9:P1}" -f `
        ($tail / $frames), ($tail / $runs), ($tail / $total))
    Write-Host ("TOTAL         {0,12:N0} {1,11:N0}" -f `
        ($total / $frames), ($total / $runs))
    Write-Host ""
    if ($words -gt 0) {
        Write-Host ("push cost per GX word   {0:N2} ticks" -f ($push / $words))
    }

    $generic = [double]$delta['gNdsTask103GenericTicks']
    $genericRuns = [double]$delta['gNdsTask103GenericRunCount']
    $genericTris = [double]$delta['gNdsTask103GenericTriangles']
    $iter = [double]$delta['gNdsTask103IterTicks']
    $iterCount = [double]$delta['gNdsTask103IterCount']

    Write-Host ""
    Write-Host ("the other branch of the same loop")
    Write-Host ("generic runs  {0,10:N1} / frame" -f ($genericRuns / $frames))
    if ($genericRuns -gt 0) {
        Write-Host ("generic tris  {0,10:N1} / frame   ({1:N1} per run)" -f `
            ($genericTris / $frames), ($genericTris / $genericRuns))
        Write-Host ("generic emit  {0,10:N0} ticks/frame  ({1:N0} per run, {2:N0} per triangle)" -f `
            ($generic / $frames), ($generic / $genericRuns),
            ($(if ($genericTris -gt 0) { $generic / $genericTris } else { 0 })))
    }
    Write-Host ""
    Write-Host ("whole run-loop body")
    Write-Host ("iterations    {0,10:N1} / frame" -f ($iterCount / $frames))
    Write-Host ("loop total    {0,10:N0} ticks/frame" -f ($iter / $frames))
    Write-Host ("  replay {0:N0} + generic {1:N0} = {2:N0}; loop overhead {3:N0}" -f `
        ($total / $frames), ($generic / $frames),
        (($total + $generic) / $frames), (($iter - $total - $generic) / $frames))
    Write-Host ""
    $commit = [double]$delta['gNdsTask103CommitTicks']
    $commits = [double]$delta['gNdsTask103CommitCount']
    $material = [double]$delta['gNdsTask103MaterialTicks']
    if ($commits -gt 0) {
        Write-Host ("segment commit")
        Write-Host ("commits       {0,10:N1} / frame" -f ($commits / $frames))
        Write-Host ("material prep {0,10:N0} ticks/frame  ({1:N0} per commit)" -f `
            ($material / $frames), ($material / $commits))
        Write-Host ("commit total  {0,10:N0} ticks/frame  ({1:N0} per commit)" -f `
            ($commit / $frames), ($commit / $commits))
        Write-Host ("  of which run loop {0:N0}; per-segment scaffolding {1:N0}" -f `
            ($iter / $frames), (($commit - $iter) / $frames))
        Write-Host ""
    }
    $prep = [double]$delta['gNdsTask103PrepareTicks']
    $trav = [double]$delta['gNdsTask103TraversalTicks']
    $disp = [double]$delta['gNdsTask103DisplayTicks']
    $fin = [double]$delta['gNdsTask103FinishTicks']
    $stgSum = $prep + $trav + $disp + $fin
    if ($stgSum -gt 0) {
        Write-Host ("the whole STG bucket, by accumulation site (no added timer reads)")
        Write-Host ("site              ticks/frame     calls/frame   share")
        Write-Host ("prepare owner   {0,12:N0} {1,13:N1} {2,8:P1}" -f `
            ($prep / $frames), ($delta['gNdsTask103PrepareCount'] / $frames), ($prep / $stgSum))
        Write-Host ("dobj traversal  {0,12:N0} {1,13:N1} {2,8:P1}" -f `
            ($trav / $frames), ($delta['gNdsTask103TraversalCount'] / $frames), ($trav / $stgSum))
        Write-Host ("display commit  {0,12:N0} {1,13:N1} {2,8:P1}" -f `
            ($disp / $frames), ($delta['gNdsTask103DisplayCount'] / $frames), ($disp / $stgSum))
        Write-Host ("finish owner    {0,12:N0} {1,13:N1} {2,8:P1}" -f `
            ($fin / $frames), ($delta['gNdsTask103FinishCount'] / $frames), ($fin / $stgSum))
        Write-Host ("SUM             {0,12:N0}" -f ($stgSum / $frames))
        Write-Host ""
        Write-Host ("The four sites are the only writers of gNdsTickHudStageTicks, so")
        Write-Host ("SUM should equal the STG bucket. If it exceeds STG the spans nest")
        Write-Host ("and the bucket double-counts; compare against the ring dump.")
        Write-Host ""
    }
    $prepCalls = [double]$delta['gNdsTask103PrepCalls']
    if ($prepCalls -gt 0) {
        $steps = [ordered]@{
            'admit / revalidate' = [double]$delta['gNdsTask103PrepAdmitTicks']
            'validate task36 world' = [double]$delta['gNdsTask103PrepValidateTicks']
            'prepare matrices' = [double]$delta['gNdsTask103PrepMatrixTicks']
            'prepare materials' = [double]$delta['gNdsTask103PrepMaterialTicks']
            'config / frame setup' = [double]$delta['gNdsTask103PrepConfigTicks']
            'renderer prepare owner' = [double]$delta['gNdsTask103PrepOwnerTicks']
        }
        $prepSum = ($steps.Values | Measure-Object -Sum).Sum
        Write-Host ("inside prepare owner ({0:N1} calls/frame)" -f ($prepCalls / $frames))
        Write-Host ("step                       ticks/frame     share")
        foreach ($k in $steps.Keys) {
            Write-Host ("{0,-24} {1,12:N0} {2,9:P1}" -f `
                $k, ($steps[$k] / $frames), ($steps[$k] / $prepSum))
        }
        Write-Host ("{0,-24} {1,12:N0}" -f 'SUM', ($prepSum / $frames))
        Write-Host ""
    }
    $own = [double]$delta['gNdsTask103PrepOwnerTicks']
    if ($own -gt 0) {
        $ownSteps = [ordered]@{
            'validate topology' = @([double]$delta['gNdsTask103OwnValidateTicks'], 1)
            'init stats+traversal' = @([double]$delta['gNdsTask103OwnInitTicks'], [double]$delta['gNdsTask103OwnInitCount'])
            'task36 reuse check' = @([double]$delta['gNdsTask103OwnReuseTicks'], [double]$delta['gNdsTask103OwnReuseCount'])
            'apply state span' = @([double]$delta['gNdsTask103OwnStateSpanTicks'], [double]$delta['gNdsTask103OwnStateSpanCount'])
            'prepare run' = @([double]$delta['gNdsTask103OwnPrepareRunTicks'], [double]$delta['gNdsTask103OwnPrepareRunCount'])
        }
        $ownSum = 0.0
        foreach ($v in $ownSteps.Values) { $ownSum += $v[0] }
        Write-Host ("inside ndsRendererPrepareNativeStageOwner")
        Write-Host ("step                       ticks/frame   calls/frame    per call     share")
        foreach ($k in $ownSteps.Keys) {
            $t = $ownSteps[$k][0]; $c = $ownSteps[$k][1]
            Write-Host ("{0,-24} {1,12:N0} {2,13:N1} {3,11:N0} {4,9:P1}" -f `
                $k, ($t / $frames), ($c / $frames),
                ($(if ($c -gt 0) { $t / $c } else { 0 })), ($t / $own))
        }
        Write-Host ("{0,-24} {1,12:N0}  (span total {2:N0}, unattributed {3:N0})" -f `
            'SUM', ($ownSum / $frames), ($own / $frames), (($own - $ownSum) / $frames))
        Write-Host ("task36 reuse: {0:N1} hits, {1:N1} misses per frame" -f `
            ($delta['gNdsTask103OwnReuseCount'] / $frames),
            ($delta['gNdsTask103OwnReuseMissCount'] / $frames))
        Write-Host ""
    }
    $runHead = [double]$delta['gNdsTask103RunHeadTicks']
    $runDense = [double]$delta['gNdsTask103RunDenseTicks']
    $denseN = [double]$delta['gNdsTask103RunDenseCount']
    $nearN = [double]$delta['gNdsTask103RunNearCount']
    if (($runHead + $runDense) -gt 0) {
        Write-Host ("inside ndsRendererNativeStagePrepareRun")
        Write-Host ("head (policy/memset/texture) {0,10:N0} ticks/frame" -f ($runHead / $frames))
        Write-Host ("dense vertex loop            {0,10:N0} ticks/frame" -f ($runDense / $frames))
        Write-Host ("dense vertices               {0,10:N1} /frame  ({1:N0} ticks each)" -f `
            ($denseN / $frames), ($(if ($denseN -gt 0) { $runDense / $denseN } else { 0 })))
        Write-Host ("  of which near-transformed  {0,10:N1} /frame  ({1:P1} of dense)" -f `
            ($nearN / $frames), ($(if ($denseN -gt 0) { $nearN / $denseN } else { 0 })))
        Write-Host ("  colour+texcoord only       {0,10:N1} /frame  <- memo candidate" -f `
            (($denseN - $nearN) / $frames))
        Write-Host ""
    }
    Write-Host ("Instrument overhead: E3 adds no timer reads; E0-E2/E4-E6 add a few per call.")
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'Task 103 - stage replay run phase partition'
            target = $target
            rom = $rom
            romSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
            startFrame = [int]$samples['A'].frame
            endFrame = [int]$samples['B'].frame
            frames = $frames
            capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
            sampleA = $samples['A']
            sampleB = $samples['B']
            delta = $delta
            perFrame = [ordered]@{
                runs = $runs / $frames
                words = $words / $frames
                beginTicks = $begin / $frames
                pushTicks = $push / $frames
                tailTicks = $tail / $frames
            }
            perRun = [ordered]@{
                beginTicks = $begin / $runs
                pushTicks = $push / $runs
                tailTicks = $tail / $runs
            }
        }
        $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut }
                    else { Join-Path $root $JsonOut }
        $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
        Write-Host "Wrote $JsonOut"
    }
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
}
