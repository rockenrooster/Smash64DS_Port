param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4624,
    [int]$RunnerSlot = -1,
    [Parameter(Mandatory=$true)][string]$Build,
    [string]$Target = 'smash64ds-results-lab-hwtri',
    [switch]$NoBuild,
    # Two SOURCE tics, both after the scene has settled. The window between them
    # is the measurement; see the header.
    [ValidateRange(1, 100000)][int]$TicA = 130,
    [ValidateRange(1, 100000)][int]$TicB = 210,
    # The scene clock to key the two stops on. Results has its own
    # (sMNVSResultsTotalTimeTics); pass gNdsRendererProfileFrameCount with the
    # battle tickhud ROM to ask the same "how many presents per logical frame"
    # question there, which is how the surplus present was confirmed
    # Results-only rather than a shared defect.
    [string]$TicSymbol = 'sMNVSResultsTotalTimeTics',
    [string]$BreakAt = 'ndsPlatformEndFrame',
    # -1 leaves the ROM's configured default alone. 0/8/9 are useful for the
    # BUGS.md matched-source Results native-vs-generic comparison without
    # building a semantically different ROM for each arm.
    [ValidateRange(-1, 9)][int]$RendererFastRunMode = -1,
    # Same-ROM diagnostic control for Results VFX cost. This does not alter the
    # source particle update/interpreter state or free VRAM; immediately after
    # sample A it only makes the renderer treat the already-resident atlas as
    # unavailable, so B measures the same scene/tics without particle GX work.
    [switch]$DisableParticleAtlasAfterA,
    # Requires a ROM built with NDS_TASK68_FALLBACK_CENSUS=1. Keep optional so
    # ordinary Results census ROMs do not fail GDB expression parsing on the
    # counters that are intentionally compiled out of shipping builds.
    [switch]$NativeFallbackCensus,
    [string]$Output,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600
)

# Per-frame cost census for the VS Results screen.
#
# WHY A DELTA AND NOT A READ. The tick-HUD buckets (gNdsTickHudFighterTicks,
# gNdsTickHudStageTicks, gNdsTickHudVBlankWaitTicks, ...) are `+=` accumulators
# that are zeroed in exactly two places, `taskman_seam.c:5077` and `:7769`, both
# inside the BATTLE presentation loop. On the battle path they are therefore
# per-frame buckets. The Results loop never reaches either site, so on Results
# they are free-running totals that nothing ever resets.
#
# Reading one at a stop and dividing by a frame count is wrong twice over: it
# divides a running total by the wrong denominator, and it silently folds in
# whatever the counter accumulated during the pre-Results boot. Measured
# 2026-07-30: doing exactly that produced a "WAIT = 1,018,354/frame" figure --
# about 1.8 VBlanks -- for a code path whose only wait is a single
# swiWaitForVBlank that cannot exceed one VBlank period. The impossible result
# was the tell that the denominator, not the emulator, was wrong.
#
# So: stop twice, at two known source tics, and difference. (bucket_B -
# bucket_A) / (TicB - TicA) is a true per-frame rate no matter what the counter
# held at TicA, and (vblank_B - vblank_A) / (TicB - TicA) is the frame's cost in
# VBlanks measured the same way at the same instants -- which makes the two
# cross-checkable rather than two separate stories.
#
# Pair with `smash64ds-results-lab-hwtri` (harness mode `results_playable`),
# which boots straight into Results, so a full census costs seconds.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

if ($TicB -le $TicA) { throw "TicB ($TicB) must be greater than TicA ($TicA)." }

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'

if (-not $NoBuild) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    make -C $root "TARGET=$Target" "BUILD=$Build"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($path in @($rom, $elf)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required census input is missing: $path"
    }
}

$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'census-results.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'census-results.melonds.err') `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    Write-Host ("census: {0} [{1}] tics {2}..{3}" -f `
        [System.IO.Path]::GetFileName($rom), $Build, $TicA, $TicB)

    # ONE printf PER COUNTER. A single bad expression aborts the whole printf
    # statement and takes every counter in it down with it; a 2026-07-30 dump
    # lost about sixty values to one typo'd symbol. Separate statements cost
    # nothing and fail independently.
    $emit = {
        param($tag)
        $lines = @(
            "printf `"$tag.tic=%u\n`", $TicSymbol",
            "printf `"$tag.vblank=%u\n`", sVBlankCount",
            # PRESENTED frames, not source tics. These are not the same number
            # and assuming they were is what made the first WAIT figure
            # impossible: sTicks increments once per ndsPlatformEndFrame, right
            # beside the only swiWaitForVBlank on the path.
            "printf `"$tag.present=%u\n`", sTicks",
            # Which loop is driving, and at what ratio. sMNVSResultsTotalTimeTics
            # advances in mnVSResultsFuncRun (a GObj run proc, once per source
            # frame originally); these separate "the proc runs at half rate" from
            # "a different loop is presenting twice".
            "printf `"$tag.update=%u\n`", dSYTaskmanUpdateCount",
            "printf `"$tag.scenedraw=%u\n`", dSYTaskmanFrameCount",
            "printf `"$tag.profframe=%u\n`", gNdsRendererProfileFrameCount",
            "printf `"$tag.flushcount=%u\n`", gNdsHardwareRendererFlushCount",
            # sTicks counts ndsPlatformEndFrame calls but not WHICH call site.
            # One counter per site, so a surplus present is attributed rather
            # than guessed at.
            "printf `"$tag.battledraw=%u\n`", gNdsBattlePlayablePacingDrawCalls",
            "printf `"$tag.moviepresent=%u\n`", gNdsOpeningMoviePresentFrameCount",
            "printf `"$tag.mainloop=%u\n`", gNdsFrameCounter",
            "printf `"$tag.submitted=%u\n`", gNdsHardwareRendererSubmittedFrameCount",
            "printf `"$tag.ftr=%u\n`", gNdsTickHudFighterTicks",
            "printf `"$tag.stg=%u\n`", gNdsTickHudStageTicks",
            "printf `"$tag.bgd=%u\n`", gNdsTickHudBackgroundTicks",
            "printf `"$tag.fgd=%u\n`", gNdsTickHudForegroundTicks",
            "printf `"$tag.aud=%u\n`", gNdsTickHudAudioTicks",
            "printf `"$tag.src=%u\n`", gNdsTickHudSourceTicks",
            "printf `"$tag.flush=%u\n`", gNdsTickHudFlushTicks",
            "printf `"$tag.wait=%u\n`", gNdsTickHudVBlankWaitTicks",
            "printf `"$tag.particleemit=%u\n`", gNdsParticleQuadEmitCount",
            "printf `"$tag.particlevisible=%u\n`", gNdsParticleDrawVisibleCount",
            "printf `"$tag.resultssubmit=%u\n`", gNdsVSResultsFighterSubmitCount"
        )
        if ($NativeFallbackCensus) {
            $lines += "printf `"$tag.fallbacktotal=%u\n`", gNdsTickHudNativeOwnerFallbackCount"
            for ($i = 0; $i -lt 15; $i++) {
                $lines += "printf `"$tag.fb$i=%u\n`", gNdsTickHudNativeOwnerFallbackByReason[$i]"
            }
            $lines += @(
                "printf `"$tag.planbuild=%u\n`", gNdsFtrPlanBuild",
                "printf `"$tag.planhit=%u\n`", gNdsFtrPlanHit"
            )
        }
        $lines
    }

    $script = Join-Path $temp 'census-results.gdb'
    $stdout = Join-Path $temp 'census-results.gdb.out'
    $stderr = Join-Path $temp 'census-results.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $modeCommands = if ($RendererFastRunMode -ge 0) {
        @(
            "set variable gNdsRendererFastRunMode = $RendererFastRunMode",
            'printf "RESULTS_FAST_MODE=%u\n", gNdsRendererFastRunMode'
        )
    } else { @() }
    $particleControl = if ($DisableParticleAtlasAfterA) {
        @(
            'set variable sNdsRendererParticleAtlasPrepared = 0',
            'printf "RESULTS_PARTICLE_ATLAS_AFTER_A=%u\n", sNdsRendererParticleAtlasPrepared'
        )
    } else { @() }
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)"
    ) + $modeCommands + @(
        "tbreak $BreakAt if $TicSymbol == $TicA",
        'continue'
    ) + (& $emit 'A') + $particleControl + @(
        "tbreak $BreakAt if $TicSymbol == $TicB",
        'continue'
    ) + (& $emit 'B') + @(
        'printf "CENSUS-DONE\n"'
    ))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $text = ''
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 500
        if (Test-Path -LiteralPath $stdout) {
            $text = Get-Content -LiteralPath $stdout -Raw
            if ($text -match 'CENSUS-DONE') { break }
        }
        if ($gdbProcess.HasExited) { break }
    }
    if ($text -notmatch 'CENSUS-DONE') {
        if ($text) {
            Write-Host 'GDB never completed the census. Last output:'
            ($text -split "`n" | Select-Object -Last 20) | ForEach-Object { "    $_" }
        }
        throw "Census did not reach tic $TicB within ${TimeoutSeconds}s."
    }
    if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }

    $vals = @{}
    foreach ($line in ($text -split "`r?`n")) {
        if ($line -match '^([AB])\.([a-z0-9]+)=(\d+)\s*$') {
            $vals["$($Matches[1]).$($Matches[2])"] = [uint32]$Matches[3]
        }
    }
    foreach ($need in @('A.tic', 'B.tic', 'A.vblank', 'B.vblank')) {
        if (-not $vals.ContainsKey($need)) { throw "Census output is missing $need." }
    }
    if ($vals['A.tic'] -ne $TicA) { throw "First stop was tic $($vals['A.tic']), not $TicA." }
    if ($vals['B.tic'] -ne $TicB) { throw "Second stop was tic $($vals['B.tic']), not $TicB." }

    $srcTics = [double]($TicB - $TicA)
    $vbl = [double]($vals['B.vblank'] - $vals['A.vblank'])
    $VBLANK_TICKS = 560190.0

    # Divide by PRESENTED frames. Every bucket here accumulates once per
    # ndsPlatformEndFrame, so presented frames is their true denominator; source
    # tics is a different clock and only coincides when the scene presents
    # exactly once per update.
    $present = if ($vals.ContainsKey('A.present') -and $vals.ContainsKey('B.present')) {
        [double]$vals['B.present'] - [double]$vals['A.present']
    } else { $srcTics }
    if ($present -le 0) { throw "Presented-frame count did not advance ($present)." }
    $tics = $present

    Write-Host ''
    Write-Host ("Results cost over {0} source tics / {1} presented frames  ({2:N2} presents per tic)" -f `
        $srcTics, $present, ($present / $srcTics))
    Write-Host ("  VBlanks/present {0,10:N2}   ({1,12:N0} ticks of wall clock)" -f `
        ($vbl / $present), ($vbl / $present * $VBLANK_TICKS))
    Write-Host ("  VBlanks/tic     {0,10:N2}   ({1,12:N0} ticks of wall clock)" -f `
        ($vbl / $srcTics), ($vbl / $srcTics * $VBLANK_TICKS))
    Write-Host ''
    foreach ($drv in @(
        @('task_update calls', 'update'), @('scene_draw calls', 'scenedraw'),
        @('profiled frames',   'profframe'), @('GX flushes',     'flushcount'),
        @('GX submits',        'submitted'),
        @('battle presents',   'battledraw'), @('movie presents', 'moviepresent'),
        @('main-loop frames',  'mainloop'))) {
        if (-not ($vals.ContainsKey("A.$($drv[1])") -and
                  $vals.ContainsKey("B.$($drv[1])"))) { continue }
        $d = [double]$vals["B.$($drv[1])"] - [double]$vals["A.$($drv[1])"]
        Write-Host ("  {0,-18} {1,8:N0} total   {2,6:N2} per source tic   {3,6:N2} per present" -f `
            $drv[0], $d, ($d / $srcTics), ($d / $present))
    }
    Write-Host ''
    $rows = @(
        @('FTR  fighter draw', 'ftr'), @('STG  stage draw', 'stg'),
        @('BGD  background',   'bgd'), @('FGD  foreground', 'fgd'),
        @('AUD  audio',        'aud'), @('SRC  source',     'src'),
        @('FLUSH commit',      'flush'), @('WAIT vblank wait', 'wait')
    )
    $workSum = 0.0
    foreach ($row in $rows) {
        $k = $row[1]
        if (-not ($vals.ContainsKey("A.$k") -and $vals.ContainsKey("B.$k"))) {
            Write-Host ("  {0,-18} {1,12}" -f $row[0], 'MISSING'); continue
        }
        # Unsigned wrap is real on a free-running accumulator; treat B < A as a
        # wrap rather than printing a nonsense negative rate.
        $d = [double]$vals["B.$k"] - [double]$vals["A.$k"]
        if ($d -lt 0) { $d += 4294967296.0 }
        $per = $d / $tics
        if ($k -ne 'wait') { $workSum += $per }
        Write-Host ("  {0,-18} {1,12:N0} /frame   {2,6:P1} of wall" -f `
            $row[0], $per, ($per / ($vbl / $tics * $VBLANK_TICKS)))
    }
    Write-Host ''
    Write-Host ("  bracketed work    {0,12:N0} /frame" -f $workSum)
    Write-Host ("  gate (1.12M)      {0,12:N0} /frame   -> {1:N2}x over" -f `
        1120380.0, (($vbl / $tics * $VBLANK_TICKS) / 1120380.0))

    if ($Output) {
        [void](New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Output))
        $json = [ordered]@{
            build = $Build; target = $Target; ticA = $TicA; ticB = $TicB
            renderer_fast_run_mode = $RendererFastRunMode
            disable_particle_atlas_after_a = [bool]$DisableParticleAtlasAfterA
            native_fallback_census = [bool]$NativeFallbackCensus
            source_tics = $srcTics; presented_frames = $present
            presents_per_tic = ($present / $srcTics)
            vblanks_per_present = ($vbl / $present)
            wall_ticks_per_present = ($vbl / $present * $VBLANK_TICKS)
            buckets = [ordered]@{}
            raw = $vals
        }
        foreach ($row in $rows) {
            $k = $row[1]
            if (-not ($vals.ContainsKey("A.$k") -and $vals.ContainsKey("B.$k"))) { continue }
            $d = [double]$vals["B.$k"] - [double]$vals["A.$k"]
            if ($d -lt 0) { $d += 4294967296.0 }
            $json.buckets[$k] = ($d / $tics)
        }
        $json | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $Output -Encoding UTF8
        Write-Host ''
        Write-Host "wrote $Output"
    }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
