[CmdletBinding()]
param(
    [string]$Build = 'build-p2-shell',
    [string]$Target = 'smash64ds-p2-shell-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 120,
    [ValidateSet('ModeSelect', 'Css')][string]$Screen = 'ModeSelect',
    # Real wall-clock hold, not a frame count. Sized generously against the
    # 353-738 fps this shipping-configuration ROM has measured running FREE
    # under this emulator (capture-p2-shell.ps1's own calibration: three runs
    # of the character select at 353/717/738 fps against its 60 Hz guest
    # pacing) -- so even at the low end this clears each track's own natural
    # stream length (Mode Select ~64 guest-s / 3,852 frames, Battle Select
    # ~15 guest-s / 915 frames) with a wide margin for at least one full loop
    # back plus stability afterward. See the header comment below for why this
    # has to be wall-clock rather than a frame-counted breakpoint.
    [ValidateRange(3000, 300000)][int]$HoldMilliseconds = 30000,
    [string]$Artifact = ''
)

# P2-1k (g2) LOOP-PROOF PROBE. Menu BGM (tracks 44 ModeSelect, 10 BattleSelect)
# must loop indefinitely rather than falling silent once its rendered stream
# is exhausted. Nothing in the phase's existing instruments holds a screen
# open long enough to find out: probe-p2-shell.ps1 stops once per SCENE ENTRY
# (11 stops for a whole scripted pass), and the fastest of those screens
# (Battle Select, ~15 virtual seconds of BGM) is gone in well under a second
# of scripted dwell -- nowhere near its own stream length, let alone a loop.
#
# HOW THE HOLD WORKS, AND WHY NO NEW HARNESS MODE WAS NEEDED. The scripted
# walk (NDS_P2_MENU_WALK) already exposes exactly the control this needs:
# `ndsMenuShellWalkTap` (nds_menu_shell.c) refuses to inject any further input
# once `gNdsMenuShellWalkLoops >= gNdsMenuShellWalkBudget` -- "stop driving and
# leave the shell to the player. The screen parks rather than looping
# forever," in the function's own comment. `gNdsMenuShellWalkBudget` is
# `NDS_MENU_PUBLISHED volatile`, i.e. an existing runtime poke (the same one
# `verify-p2-shell-loop.ps1` seeds for its lap count), so parking a screen mid
# walk is one `set variable` at that screen's own entry breakpoint: poke the
# budget to the CURRENT lap count (0, first lap) the instant
# `ndsMenuShellRunModeSelect` / `ndsMenuShellRunCharSelect` fires, and no
# further tap is ever injected -- the screen's own
# `while (sMenuLeaving == FALSE)` loop just keeps running with nothing to
# end it.
#
# WHY THE HOLD IS TIMED IN WALL-CLOCK MILLISECONDS, NOT FRAME-COUNTED
# BREAKPOINT HITS -- THE FIRST TWO DRAFTS OF THIS PROBE AND WHY BOTH WERE
# WRONG. Draft 1 broke on `ndsPlatformEndFrame` (fires once per presented
# frame) and re-armed via the breakpoint's own `commands` block every hit,
# reading `gNdsMenuShellFrames[screen] >= target` to know when to stop. Over
# a planned ~15,500 hits it did not finish inside a 1200 s budget; CPU
# sampling mid-run showed melonDS at ~9% utilization, i.e. mostly idle
# waiting on the remote round trip -- the per-hit STOP COUNT was the cost,
# not the emulator, so draft 2 cut the target frame counts and the per-hit
# work to a couple of gdb-local compares. It still stalled, and the captured
# counters explained why: `gNdsAudioBgmElapsedFrames`, `RefillCount` and
# `StreamedBytes` all read exactly 0 after 2,000 held frames while
# `gNdsAudioBgmPlaying` read 1 the whole time. BGM's actual seam transitions
# (buffer swap, refill, the loop-back this row exists to prove) run off a
# REAL DS HARDWARE TIMER interrupt + a worker thread
# (`ndsAudioBgmArmTimer`/`ndsAudioBgmTimerCallback`/`ndsAudioBgmHandleSeam`,
# nds_audio_bgm.c) under the shipping `NDS_HARNESS_FAST_LOGIC=0` build this
# proof has to use -- and a gdb breakpoint that halts the CPU on literally
# every presented frame does not give that timer a chance to elapse between
# halts the way genuinely free execution does. The existing phase probes
# (`probe-p2-shell.ps1`) stop only ~11 times for a WHOLE scripted pass and
# have never shown this symptom; stopping thousands of times a second is a
# qualitatively different interaction with the emulator, not just a slower
# one. So the hold here uses the SAME free-running technique
# `verify-battle-playable-down-air-stall.ps1`'s `-ObserverFreeSnapshot`/
# `-FreezeDiagnostics` arms already use for exactly this reason: MI-async
# (`set mi-async on`), `-exec-continue` (non-blocking resume) followed, after
# a genuine wall-clock delay via `Invoke-GdbMarkerScript`'s
# `-InteractiveSteps`, by `-exec-interrupt`. Between those two the target runs
# with NO breakpoint armed on any per-frame hook at all -- as free as the
# proven-good scene-entry probes -- and a `hook-stop` defined once prints the
# BGM counters on the single stop that interrupt produces.
#
# ONE SCREEN PER RUN. Parking Mode Select and then un-parking to walk onward
# to Character Select (an earlier draft's plan, to cover both tracks in one
# launch) adds a second free-run phase whose own length has to be judged from
# the SAME single interrupt-driven stop, which is unnecessary risk for a
# probe that already runs in well under a minute; `-Screen Css` reaches the
# character select through the walk's own untouched Title -> Mode Select ->
# VS Mode timing (a few hundred presented frames, seconds at worst) and parks
# there instead. Run both.
#
# WHAT PROVES THE LOOP, NOT JUST THE FLAG. `gNdsAudioBgmLoopCount` increments
# inside `ndsAudioBgmReadPacket` only when the read cursor actually wraps
# (`sNdsAudioBgmNextPacket >= track->packet_count`, seek back to
# `loop_record`, resume at `loop_packet`) -- it cannot climb without a real
# `fseek` + real packet reads happening again, so a nonzero reading while
# `playing` stays 1 and every fail/stop counter (`NaturalStopCount`,
# `ErrorStopCount`, `SeamMissCount`, `OverrunCount`, `ReadFailCount`,
# `PacketFailCount`, `HeaderFailCount`) stays 0 is direct engagement
# evidence, not an inference from the static track table.
# `gNdsAudioBgmPlaybackPositionBytes` (live under NDS_SHIP_TELEMETRY=1, which
# this lab target sets) additionally shows the mapped playback position
# itself wrapping into `[loop_start_bytes, stream_bytes)` after the first
# pass, and `gNdsAudioBgmElapsedFrames`/`RefillCount` prove the per-frame
# update actually ran throughout rather than having stalled silently the way
# draft 1/2's per-frame breakpoint made it stall.
#
# Nothing here writes guest memory outside the one poke this row's own
# mechanism is designed for (`gNdsMenuShellWalkBudget`).

$ErrorActionPreference = 'Stop'
$scripts = Split-Path -Parent $PSScriptRoot
$root = Split-Path -Parent $scripts
. (Join-Path $scripts 'lib\melonds.ps1')
. (Join-Path $scripts 'lib\gdb-markers.ps1')
. (Join-Path $scripts 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-1k-bgm-loop-' +
        $Screen.ToLowerInvariant() + '.txt')
}

$buildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "bgm-loop probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$configText = Get-Content -LiteralPath $buildConfig -Raw
foreach ($flag in @('NDS_P2_MENU_SHELL', 'NDS_HARNESS_FAST_LOGIC', 'NDS_BGM_FALSIFIER_OFF')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    $value = if ($m.Success) { $m.Groups[1].Value } else { 'absent' }
    Write-Output ("build config: {0}={1}" -f $flag, $value)
}
# This proof needs the REAL BGM path (open/read/loop), not the falsifier
# stub, and the real hardware-timer seam (fast-logic drains seams off a
# frame-tied clock instead), or a clean read here would prove nothing about
# the shipping mechanism.
$fastLogicM = [regex]::Match($configText, '(?m)^#define\s+NDS_HARNESS_FAST_LOGIC\s+(\d+)u?$')
$falsifierM = [regex]::Match($configText, '(?m)^#define\s+NDS_BGM_FALSIFIER_OFF\s+(\d+)u?$')
if ((-not $fastLogicM.Success) -or ($fastLogicM.Groups[1].Value -ne '0')) {
    throw 'bgm-loop probe: build is not NDS_HARNESS_FAST_LOGIC=0; the real hardware-timer BGM seam would not be exercised.'
}
if ($falsifierM.Success -and ($falsifierM.Groups[1].Value -ne '0')) {
    throw 'bgm-loop probe: build has NDS_BGM_FALSIFIER_OFF=1; BGM open/read is stubbed out and there is nothing to prove.'
}

$required = @(
    'ndsMenuShellRunModeSelect', 'ndsMenuShellRunCharSelect',
    'gNdsMenuShellWalkBudget', 'gNdsMenuShellWalkLoops', 'gNdsMenuShellScreen',
    'gNdsAudioBgmLoopCount', 'gNdsAudioBgmPlaying', 'gNdsAudioBgmTrackID',
    'gNdsAudioBgmRefillCount', 'gNdsAudioBgmElapsedFrames',
    'gNdsAudioBgmStreamedBytes', 'gNdsAudioBgmPlaybackPositionBytes',
    'gNdsAudioBgmNaturalStopCount', 'gNdsAudioBgmErrorStopCount',
    'gNdsAudioBgmSeamMissCount', 'gNdsAudioBgmOverrunCount',
    'gNdsAudioBgmReadFailCount', 'gNdsAudioBgmPacketFailCount',
    'gNdsAudioBgmHeaderFailCount'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("bgm-loop probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir ('melonds.p2-1k-bgm-loop-' + $Screen.ToLowerInvariant() + '.stdout.log')
$stderr = Join-Path $log_dir ('melonds.p2-1k-bgm-loop-' + $Screen.ToLowerInvariant() + '.stderr.log')
$readyFile = Join-Path $log_dir ('p2-1k-bgm-loop-' + $Screen.ToLowerInvariant() + '.ready')
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr, $readyFile -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $breakFunc = if ($Screen -eq 'ModeSelect') { 'ndsMenuShellRunModeSelect' } else { 'ndsMenuShellRunCharSelect' }
    $label = if ($Screen -eq 'ModeSelect') { 'modesel' } else { 'css' }
    $readyFileGdb = $readyFile.Replace('\', '/')

    $bgmFmt = 'scr=%u track=%u loop=%u playing=%u refills=%u elapsed=%u streamed=%u posbytes=%u'
    $bgmFields = 'gNdsMenuShellScreen, gNdsAudioBgmTrackID, gNdsAudioBgmLoopCount, gNdsAudioBgmPlaying, gNdsAudioBgmRefillCount, gNdsAudioBgmElapsedFrames, gNdsAudioBgmStreamedBytes, gNdsAudioBgmPlaybackPositionBytes'
    $failFmt = 'natstop=%u errstop=%u seammiss=%u overrun=%u readfail=%u pktfail=%u hdrfail=%u'
    $failFields = 'gNdsAudioBgmNaturalStopCount, gNdsAudioBgmErrorStopCount, gNdsAudioBgmSeamMissCount, gNdsAudioBgmOverrunCount, gNdsAudioBgmReadFailCount, gNdsAudioBgmPacketFailCount, gNdsAudioBgmHeaderFailCount'

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set print elements 128',
        'set breakpoint pending off',
        'set mi-async on',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $armed = 0',

        # hook-stop fires on EVERY stop, including the park breakpoint below
        # (before $armed is set, so it is a no-op there) and the async
        # -exec-interrupt that ends the free-running hold (after $armed is
        # set, so THAT stop is the one this prints).
        'define hook-stop',
        'if $armed != 0',
        'set $armed = 0',
        ('printf "HOLD-COMPLETE screen=' + $label + ' ' + $bgmFmt + ' ' + $failFmt + '\n", ' + $bgmFields + ', ' + $failFields),
        'end',
        'end',

        ('break ' + $breakFunc),
        'commands',
        'silent',
        'set gNdsMenuShellWalkBudget = 0',
        ('printf "PARK ' + $label + ' budget=%u loops=%u\n", gNdsMenuShellWalkBudget, gNdsMenuShellWalkLoops'),
        'set $armed = 1',
        ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '" + $readyFileGdb + "' -Value ready`""),
        'end',

        # This bare `continue` runs the target through the walk's own boot ->
        # title -> (Mode Select ->) VS Mode timing until the park breakpoint
        # above fires; its commands leave the target halted there (no further
        # `continue`), which is what lets this statement's own blocking
        # `continue` complete and script-file processing fall through to
        # stdin, where -InteractiveSteps drives the timed free-running hold.
        'continue'
    )

    $gdbArguments = @{
        Gdb = $gdb
        Elf = $elf
        Root = $root
        Commands = $commands
        ScriptName = ('p2_1k_bgm_loop_' + $label + '.gdb')
        TimeoutSeconds = $TimeoutSeconds
        ReadyFile = $readyFile
        InteractiveSteps = @(
            [pscustomobject]@{ DelayMilliseconds = 0; Commands = @('1-exec-continue') },
            [pscustomobject]@{ DelayMilliseconds = $HoldMilliseconds; Commands = @('2-exec-interrupt') },
            # A fixed settle window between the interrupt and detach: the
            # interrupt itself is async (non-blocking), so detaching
            # immediately risks racing the stop notification that triggers
            # hook-stop and losing the HOLD-COMPLETE line entirely.
            [pscustomobject]@{ DelayMilliseconds = 3000; Commands = @('detach', 'quit') }
        )
        MiInteractive = $true
    }
    Invoke-GdbMarkerScript @gdbArguments | Out-Null
}
finally {
    $captured = Join-Path $log_temp ('p2_1k_bgm_loop_' + $label + '.gdb.out')
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
