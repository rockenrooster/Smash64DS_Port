[CmdletBinding()]
param(
    [string]$Build = 'build-p2-1c-ui-kit',
    [string]$Target = 'smash64ds-p2-1c-ui-kit-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 900,
    # Stops, not frames. The breakpoint is ndsUiKitSfx, which the demo reaches
    # about five times per 256 presented frames, so 30 stops is roughly 1,500
    # frames of window. The window is reported with every figure; a percentile
    # without its window is not a number.
    [ValidateRange(2, 200)][int]$Hits = 30,
    [string]$Artifact = ''
)

# P2-1c evidence. Reads the UI kit's published counters and the demo's own
# per-frame work-tick and VBlank-interval histograms out of a ROM built
# NDS_P2_UI_KIT_DEMO=1.
#
# WHY THIS BREAKPOINT. A per-frame stop would be one host round trip per
# presented frame, which this repository has already paid for once (memory:
# sample the ring in one stop, never 128 per-frame stops). ndsUiKitSfx is the
# lowest-frequency site the demo has -- the cursor cue fires on a 64-frame
# period and the confirm cue on a 256-frame one -- so the stops sit ~50 frames
# apart and the histogram between them is the ROM's own uninterrupted work.
# The counters are cumulative, so the LAST line is the whole window.
#
# WHAT THE NUMBERS MEAN. gNdsUiKitDemoWorkHist buckets ARM9 ticks between the
# end of one present and the end of the next frame's body -- work per presented
# frame, not the frame period -- at 35,012 ticks a bucket, one sixteenth of the
# 560,190-tick 60 Hz VBlank budget. Bucket b means [b*35012, (b+1)*35012).
# gNdsUiKitDemoVBlankHist[i] counts presented frames whose VBlank interval was
# i+1, so index 0 is a clean 60 Hz present and anything else is a slip.
#
# Nothing here writes guest memory.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-1c-ui-kit.txt')
}

$buildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "p2-1c probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$configText = Get-Content -LiteralPath $buildConfig -Raw
foreach ($flag in @('NDS_P2_UI_KIT', 'NDS_P2_UI_KIT_DEMO')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        # A run against a kit-less ROM would report every counter as "missing
        # symbol" and read as a broken kit rather than a wrong build.
        throw "p2-1c probe: $Build was built with $flag off; nothing to read."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $m.Groups[1].Value)
}

$required = @(
    'ndsUiKitSfx',
    'gNdsUiKitEnterCount', 'gNdsUiKitEnterRejectCount',
    'gNdsUiKitEngine', 'gNdsUiKitPackBytesLoaded', 'gNdsUiKitPackHash',
    'gNdsUiKitPackHashMismatchCount', 'gNdsUiKitPackReadFailCount',
    'gNdsUiKitTextComposeCount', 'gNdsUiKitTextComposeSkipCount',
    'gNdsUiKitTextOverflowCount', 'gNdsUiKitCommitCount',
    'gNdsUiKitCommitIdleCount', 'gNdsUiKitVisibleObjectCount',
    'gNdsUiKitSfxRequestCount', 'gNdsUiKitSfxLastId',
    'gNdsUiKitDemoEntered', 'gNdsUiKitDemoSceneKind', 'gNdsUiKitDemoFrames',
    'gNdsUiKitDemoEnterFrameTicks', 'gNdsUiKitDemoWorkTicksLast',
    'gNdsUiKitDemoWorkTicksMax', 'gNdsUiKitDemoWorkHist',
    'gNdsUiKitDemoVBlankHist', 'gNdsUiKitDemoVBlankMax'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-1c probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# ndsUiKitExit and its counter are NOT in the required list, and that is a
# fact about the demo rather than an oversight: the demo never leaves the
# scene it entered, so --gc-sections drops the function and the .bss section
# its counter lives in. `__attribute__((used))` binds the compiler, not the
# linker. P2-1d, which does exit scenes, will find both present.
$hasExit = $symbols -contains 'gNdsUiKitExitCount'
if (-not $hasExit) {
    Write-Output ('note: gNdsUiKitExitCount is absent from this build (no ' +
        'caller of ndsUiKitExit); the UKSTATE exits field reads `absent`.')
}
# The FGM miss ring is the SFX seam's negative half: the kit asks for the
# source's own menu cue ids, and this ring says whether the pack carries them.
# Optional so the probe still reads a build without the audio runtime, and
# reported absent rather than as zero.
$hasMissRing = ($symbols -contains 'gNdsAudioFgmMissRingIDs') -and
    ($symbols -contains 'gNdsAudioFgmMissRingCounts') -and
    ($symbols -contains 'gNdsAudioFgmMissRingCount')
if (-not $hasMissRing) {
    Write-Output ('note: gNdsAudioFgmMissRing* are absent from this build; ' +
        'the UKMISS line will read `absent`, not 0.')
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.p2-1c-ui-kit.stdout.log'
$stderr = Join-Path $log_dir 'melonds.p2-1c-ui-kit.stderr.log'
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
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set print elements 64',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        'break ndsUiKitSfx',
        'commands',
        'silent',
        'set $n = $n + 1',
        $(if ($hasExit) {
            'printf "UKSTATE %d entered=%u scene=%u engine=%u enters=%u rej=%u exits=%u\n", $n, gNdsUiKitDemoEntered, gNdsUiKitDemoSceneKind, gNdsUiKitEngine, gNdsUiKitEnterCount, gNdsUiKitEnterRejectCount, gNdsUiKitExitCount'
        } else {
            'printf "UKSTATE %d entered=%u scene=%u engine=%u enters=%u rej=%u exits=absent\n", $n, gNdsUiKitDemoEntered, gNdsUiKitDemoSceneKind, gNdsUiKitEngine, gNdsUiKitEnterCount, gNdsUiKitEnterRejectCount'
        }),
        'printf "UKPACK %d bytes=%u hash=%08x mismatch=%u readfail=%u\n", $n, gNdsUiKitPackBytesLoaded, gNdsUiKitPackHash, gNdsUiKitPackHashMismatchCount, gNdsUiKitPackReadFailCount',
        'printf "UKDRAW %d compose=%u skip=%u overflow=%u commit=%u idle=%u visible=%u\n", $n, gNdsUiKitTextComposeCount, gNdsUiKitTextComposeSkipCount, gNdsUiKitTextOverflowCount, gNdsUiKitCommitCount, gNdsUiKitCommitIdleCount, gNdsUiKitVisibleObjectCount',
        'printf "UKSFX %d move=%u confirm=%u back=%u lastid=%u\n", $n, gNdsUiKitSfxRequestCount[0], gNdsUiKitSfxRequestCount[1], gNdsUiKitSfxRequestCount[2], gNdsUiKitSfxLastId',
        'printf "UKFRAME %d frames=%u enterticks=%u last=%u max=%u vbmax=%u\n", $n, gNdsUiKitDemoFrames, gNdsUiKitDemoEnterFrameTicks, gNdsUiKitDemoWorkTicksLast, gNdsUiKitDemoWorkTicksMax, gNdsUiKitDemoVBlankMax',
        'printf "UKWORK %d %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", $n, gNdsUiKitDemoWorkHist[0], gNdsUiKitDemoWorkHist[1], gNdsUiKitDemoWorkHist[2], gNdsUiKitDemoWorkHist[3], gNdsUiKitDemoWorkHist[4], gNdsUiKitDemoWorkHist[5], gNdsUiKitDemoWorkHist[6], gNdsUiKitDemoWorkHist[7], gNdsUiKitDemoWorkHist[8], gNdsUiKitDemoWorkHist[9], gNdsUiKitDemoWorkHist[10], gNdsUiKitDemoWorkHist[11], gNdsUiKitDemoWorkHist[12], gNdsUiKitDemoWorkHist[13], gNdsUiKitDemoWorkHist[14], gNdsUiKitDemoWorkHist[15]',
        'printf "UKVB %d %u %u %u %u\n", $n, gNdsUiKitDemoVBlankHist[0], gNdsUiKitDemoVBlankHist[1], gNdsUiKitDemoVBlankHist[2], gNdsUiKitDemoVBlankHist[3]',
        $(if ($hasMissRing) {
            'printf "UKMISS %d ring=%u id0=%u c0=%u id1=%u c1=%u\n", $n, gNdsAudioFgmMissRingCount, gNdsAudioFgmMissRingIDs[0], gNdsAudioFgmMissRingCounts[0], gNdsAudioFgmMissRingIDs[1], gNdsAudioFgmMissRingCounts[1]'
        } else {
            'printf "UKMISS %d ring=absent\n", $n'
        }),
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        'continue',
        'printf "UKSTOP n=%d pc=%08x cpsr=%08x\n", $n, $pc, $cpsr',
        'info symbol $pc',
        'print gNdsUiKitDemoWorkHist',
        'print gNdsUiKitDemoVBlankHist',
        'print gNdsUiKitSfxRequestCount',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'p2_1c_ui_kit_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the capture file, never the helper's return value: this probe's last
    # command is an unbounded `continue`, so it exits by timeout by design and
    # the capture still holds every line taken before that.
    $captured = Join-Path $log_temp 'p2_1c_ui_kit_probe.gdb.out'
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
