[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [ValidateRange(30,1800)][int]$TimeoutSeconds = 300,
    [ValidateRange(0,2000000)]
    [int]$MaxServiceLatenessMicroseconds = 250000,
    [ValidateRange(0,2000000)]
    [int]$MaxAckWaitMicroseconds = 20000,
    [string]$Screenshot = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-crowd-ack-hwtri'
$build = 'build-battle-playable-crowd-ack-hwtri-harness'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $target -Build $build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $target -Build $build -Extension '.elf'
$buildConfigPath = Join-Path $root "builds\$build\nds_build_config.h"
$metadataPath = Join-Path $root 'assets\audio\fgm_phase_pack_ima.json'
$audioSourcePath = Join-Path $root 'src\nds\nds_audio_fgm.c'
$audioHeaderPath = Join-Path $root 'include\nds\nds_audio_fgm.h'
$upstreamPath = Join-Path $root (
    'decomp\BattleShip-main\decomp\src\sc\sccommon\scvsbattle.c')
$busClock = 33513982UL
$publicExcitedID = 626
$expectedHarnessMode = 163
$expectedBattleScene = 22
$expectedFgmResult = 0x46474d31L
$expectedHarnessResult = 0x4841524eL
$expectedMemoryResult = 0x4d4c4544L
$expectedMatchTicks = 3600
$expectedDurationReleaseReason = 3
$configState = $null
$emulator = $null

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context = '')

    if ($Condition) { return }
    if ([string]::IsNullOrWhiteSpace($Context)) { throw $Message }
    throw "$Message`n$Context"
}

function Convert-MarkerField {
    param([string]$Value)

    if ($Value -match '^0x[0-9a-fA-F]+$') {
        return [int64][uint32]::Parse(
            $Value.Substring(2),
            [System.Globalization.NumberStyles]::HexNumber)
    }
    return [int64]$Value
}

function Get-MarkerRow {
    param(
        [string]$Text,
        [string]$Name,
        [int]$ExpectedFields
    )

    # GDB can print the stopped source line, this marker, and its next status
    # message on one physical line. Marker payloads are numeric CSV, so use an
    # identifier boundary plus that closed alphabet instead of line anchors.
    $matches = [regex]::Matches(
        $Text, ('(?<![A-Za-z0-9_])' + [regex]::Escape($Name) +
        '=([0-9A-Fa-fxX,-]+)'))
    Assert-Condition ($matches.Count -eq 1) `
        "Expected exactly one $Name marker, found $($matches.Count)." $Text
    $row = [int64[]]@($matches[0].Groups[1].Value.Split(',') |
        ForEach-Object { Convert-MarkerField $_ })
    Assert-Condition ($row.Count -eq $ExpectedFields) `
        "$Name marker had $($row.Count) fields; expected $ExpectedFields." `
        $Text
    return ,$row
}

function Get-UInt32Delta {
    param([uint32]$Start, [uint32]$End)

    return ([uint64]$End + 0x100000000UL - [uint64]$Start) %
        0x100000000UL
}

function Convert-CpuTicksToMicroseconds {
    param([uint64]$Ticks)

    return [uint64](($Ticks * 1000000UL) / $busClock)
}

function Get-MinCpuTicksForMicroseconds {
    param([uint64]$Microseconds)

    return [uint64](
        (($busClock * $Microseconds) + 999999UL) / 1000000UL)
}

function Assert-ElfSymbol {
    param([string]$Name, [string[]]$NmTable)

    $escapedName = [regex]::Escape($Name)
    $found = $NmTable | Where-Object {
        $_ -match "^[0-9a-fA-F]+\s+\S\s+$escapedName$"
    } | Select-Object -First 1
    Assert-Condition ($null -ne $found) (
        "Required timing telemetry symbol is missing from the canonical " +
        "ELF: $Name. This verifier will not infer timing from unrelated " +
        'state or claim an audio gate without direct evidence.')
}

function Resolve-VisibilityOutput {
    param([string]$Path)

    $visibilityDir = [System.IO.Path]::GetFullPath(
        (Join-Path $root 'artifacts\visibility'))
    $resolved = if ([string]::IsNullOrWhiteSpace($Path)) {
        $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff'
        Join-Path $visibilityDir (
            "${stamp}_crowd-envelope-post-release-p$PID.png")
    } elseif ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $root $Path))
    }
    $visibilityPrefix = $visibilityDir.TrimEnd('\', '/') +
        [System.IO.Path]::DirectorySeparatorChar
    Assert-Condition ($resolved.StartsWith(
            $visibilityPrefix,
            [System.StringComparison]::OrdinalIgnoreCase)) `
        "Crowd-envelope captures must stay under '$visibilityDir': $resolved"
    return $resolved
}

function Write-ExactCaptureHelper {
    param([Parameter(Mandatory=$true)][string]$Path)

    $helper = @'
param(
    [Parameter(Mandatory=$true)][int]$EmulatorProcessId,
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if ($null -eq ('Smash64DSCrowdEnvelopeCapture' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Smash64DSCrowdEnvelopeCapture
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr window, IntPtr insertAfter,
        int x, int y, int width, int height, uint flags);
}
"@
}

$process = Get-Process -Id $EmulatorProcessId -ErrorAction Stop
$deadline = (Get-Date).AddSeconds(10)
do {
    $process.Refresh()
    if ($process.HasExited) { throw 'melonDS exited before evidence capture.' }
    if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 50
} while ((Get-Date) -lt $deadline)
$handle = $process.MainWindowHandle
if ($handle -eq [IntPtr]::Zero) {
    throw 'melonDS did not expose a window for evidence capture.'
}

# Match scripts/lib/melonds.ps1's canonical natural stacked-screen window.
[void][Smash64DSCrowdEnvelopeCapture]::ShowWindow($handle, 9)
[void][Smash64DSCrowdEnvelopeCapture]::SetWindowPos(
    $handle, [IntPtr](-1),
    $script:MelonDSCanonicalWindowX,
    $script:MelonDSCanonicalWindowY,
    $script:MelonDSCanonicalWindowWidth,
    $script:MelonDSCanonicalWindowHeight, 0x40)
[void][Smash64DSCrowdEnvelopeCapture]::SetForegroundWindow($handle)
Start-Sleep -Milliseconds 250

$rect = New-Object Smash64DSCrowdEnvelopeCapture+Rect
if (-not [Smash64DSCrowdEnvelopeCapture]::GetWindowRect(
        $handle, [ref]$rect)) {
    throw 'Could not read the melonDS evidence window bounds.'
}
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if (($width -le 0) -or ($height -le 0)) {
    throw "Invalid melonDS evidence window bounds ${width}x${height}."
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Output) |
    Out-Null
$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
try {
    $graphics.CopyFromScreen(
        $rect.Left, $rect.Top, 0, 0, $bitmap.Size)
    $bitmap.Save($Output, [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
    [void][Smash64DSCrowdEnvelopeCapture]::SetWindowPos(
        $handle, [IntPtr](-2), 0, 0, 0, 0, 0x43)
}
'@
    Set-Content -LiteralPath $Path -Value $helper
}

# Lock this gate to the normal BattleShip battle-start call rather than the
# separate sudden-death call for the same crowd voice.
$battleStart = @(Select-String -LiteralPath $upstreamPath `
    -Pattern '^void scVSBattleStartBattle\(void\)$')
$nextFunction = @(Select-String -LiteralPath $upstreamPath `
    -Pattern '^sb32 scVSBattleSetScoreCheckSuddenDeath\(void\)$')
Assert-Condition (($battleStart.Count -eq 1) -and
    ($nextFunction.Count -eq 1) -and
    ($nextFunction[0].LineNumber -gt $battleStart[0].LineNumber)) `
    'Could not isolate the upstream BattleShip normal battle-start function.'
$sourceCalls = @(Select-String -LiteralPath $upstreamPath `
    -SimpleMatch 'func_800269C0_275C0(nSYAudioVoicePublicExcited);' |
    Where-Object {
        ($_.LineNumber -gt $battleStart[0].LineNumber) -and
        ($_.LineNumber -lt $nextFunction[0].LineNumber)
    })
Assert-Condition ($sourceCalls.Count -eq 1) `
    'Expected one PublicExcited source call in normal BattleShip battle start.'
$upstreamCallLine = $sourceCalls[0].LineNumber
$upstreamLines = Get-Content -LiteralPath $upstreamPath
Assert-Condition (
    $upstreamLines[$upstreamCallLine].Trim() -eq
        'ifCommonTimerMakeInterface(ifCommonAnnounceTimeUpInitInterface);') `
    'BattleShip no longer starts the timer immediately after PublicExcited.'

# Restrict the duration line to ndsAudioFgmUpdate so the explicit stop path
# cannot satisfy this natural-release gate.
$audioLines = Get-Content -LiteralPath $audioSourcePath
$updateStart = @(Select-String -LiteralPath $audioSourcePath `
    -Pattern '^void ndsAudioFgmUpdate\(void\)$')
$stopAllStart = @(Select-String -LiteralPath $audioSourcePath `
    -Pattern '^void ndsAudioFgmStopAll\(void\)$')
$durationRelease = @(Select-String -LiteralPath $audioSourcePath `
    -SimpleMatch (
        'NDS_AUDIO_FGM_RELEASE_REASON_DURATION, now));') |
    Where-Object {
        ($_.LineNumber -gt $updateStart[0].LineNumber) -and
        ($_.LineNumber -lt $stopAllStart[0].LineNumber)
    })
Assert-Condition (($updateStart.Count -eq 1) -and
    ($stopAllStart.Count -eq 1) -and
    ($durationRelease.Count -eq 1)) `
    'Could not isolate the semantic FGM duration-release line.'
$durationReleaseLine = $durationRelease[0].LineNumber
Assert-Condition (
    $audioLines[$durationReleaseLine].Trim() -eq
        'gNdsAudioFgmDurationStopCount++;') `
    'Duration-stop accounting no longer immediately follows duration release.'

$manualModeCalls = @(Select-String -LiteralPath $audioSourcePath `
    -SimpleMatch 'soundSetAutoUpdate(false);')
$arm7AckCalls = @(Select-String -LiteralPath $audioSourcePath `
    -SimpleMatch 'active_channels = (u32)soundGetActiveChannels();')
$autoUpdateEnableCalls = @(Select-String -LiteralPath $audioSourcePath `
    -Pattern 'soundSetAutoUpdate\s*\(\s*(true|TRUE|1)\s*\)')
$audioHeaderText = Get-Content -LiteralPath $audioHeaderPath -Raw
Assert-Condition (($manualModeCalls.Count -eq 1) -and
    ($arm7AckCalls.Count -eq 2) -and
    ($autoUpdateEnableCalls.Count -eq 0)) `
    ('FGM ARM7 telemetry must keep Calico in manual mode and issue exactly ' +
    'two target-gated PLAY/STOP channel acknowledgments.')
Assert-Condition (($audioHeaderText -match
        '#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS 0') -and
    ($audioHeaderText -match
        '#if NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS') -and
    ($audioHeaderText -match
        '#define NDS_AUDIO_FGM_ARM7_ACK_EVENT_CAPACITY 2u') -and
    ($audioHeaderText -match
        '#define NDS_AUDIO_FGM_ARM7_ACK_KIND_PLAY 1u') -and
    ($audioHeaderText -notmatch
        'NDS_AUDIO_FGM_ARM7_ACK_KIND_ENVELOPE') -and
    ($audioHeaderText -match
        '#define NDS_AUDIO_FGM_ARM7_ACK_KIND_STOP 3u') -and
    ($audioHeaderText -match
        '#define NDS_AUDIO_FGM_RELEASE_REASON_DURATION 3u') -and
    ($audioHeaderText -match
        'extern volatile NDSAudioFgmArm7AckTrace gNdsAudioFgmArm7AckTrace;')) `
    'FGM ARM7 trace layout/constants are missing from the public header.'

$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$publicEntries = @($metadata.entries | Where-Object {
    [int]$_.id -eq $publicExcitedID
})
Assert-Condition ($publicEntries.Count -eq 1) `
    'FGM metadata must contain exactly one PublicExcited ID 626 entry.'
$publicEntry = $publicEntries[0]
$sourceEnvelope = @($publicEntry.source_volume_envelope)
Assert-Condition (($sourceEnvelope.Count -eq 29) -and
    ([uint64]$sourceEnvelope[0].tick -eq 0UL) -and
    ([int]$publicEntry.packed_envelope_count -eq 0) -and
    ([int]$publicEntry.ds_volume -eq 92)) `
    ('PublicExcited must keep 29 source-derived points in metadata while ' +
    'packing no runtime envelope commands and one constant hardware gain.')
$finalEnvelopePoint = $sourceEnvelope[$sourceEnvelope.Count - 1]
$sourceTickMicroseconds = [uint64]$metadata.source_fgm_timer_microseconds
$sourceZeroMicroseconds =
    [uint64]$finalEnvelopePoint.tick * $sourceTickMicroseconds
$silentTailStartSample =
    [uint64]$publicEntry.acoustic_oracle.silent_tail_start_sample
$silentTailStartMicroseconds = [uint64][math]::Ceiling(
    ([double]$silentTailStartSample * 1000000.0) /
    [double]$publicEntry.ds_frequency_hz)
$sourceStopMicroseconds = [uint64]$publicEntry.source_duration_microseconds
$sourceDurationTicks = [uint64]$publicEntry.source_duration_ticks
$packedDataOffset = [uint64]$publicEntry.pack_data_offset
$packedDataBytes = [uint64]$publicEntry.ima_adpcm_bytes
$packedSampleCount = [uint64]$publicEntry.ds_sample_count
$packedFrequencyHz = [uint64]$publicEntry.ds_frequency_hz
Assert-Condition (([int]$publicEntry.entry_index -eq 0) -and
    ([string]$publicEntry.name -eq 'nSYAudioVoicePublicExcited') -and
    ([int]$publicEntry.phase_index -eq 0) -and
    ([bool]$publicEntry.source_loop_infinite) -and
    ([int]$publicEntry.ds_loop_flag -eq 0) -and
    ([int]$publicEntry.ds_loop_point_words -eq 0) -and
    ([int]$publicEntry.ds_sample_count -eq 104204) -and
    ([int]$publicEntry.ds_frequency_hz -eq 15102) -and
    ($sourceTickMicroseconds -eq 5750UL) -and
    ($sourceDurationTicks -eq 1200UL) -and
    ($sourceStopMicroseconds -eq 6900000UL) -and
    ([uint64]$finalEnvelopePoint.tick -eq 1075UL) -and
    ([int]$finalEnvelopePoint.source_quadratic_target -eq 0) -and
    ($sourceZeroMicroseconds -eq 6181250UL) -and
    ($silentTailStartSample -eq 93437UL) -and
    ($silentTailStartMicroseconds -gt $sourceZeroMicroseconds) -and
    ($silentTailStartMicroseconds -lt $sourceStopMicroseconds) -and
    (($packedSampleCount * 1000000UL) -ge
        ($sourceStopMicroseconds * $packedFrequencyHz))) `
    'PublicExcited source timing metadata no longer matches BattleShip ID 626.'

$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$activeRunnerSlot = Get-MelonDSActiveRunnerSlot
$logDir = Get-MelonDSVerifierLogDir `
    -Root $root -RunnerSlot $activeRunnerSlot
$tempDir = Get-MelonDSVerifierTempDir `
    -Root $root -RunnerSlot $activeRunnerSlot
$stdout = Join-Path $logDir (
    'melonds.battle-playable-crowd-envelope-timing.stdout.log')
$stderr = Join-Path $logDir (
    'melonds.battle-playable-crowd-envelope-timing.stderr.log')
$scriptName = '_battle_playable_crowd_envelope_timing.gdb'
$captureHelper = Join-Path $root 'scripts\capture-running-melonds-window.ps1'
$screenshotPath = Resolve-VisibilityOutput $Screenshot

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
if (-not $NoBuild) {
    & make -C $root `
        TARGET=$target `
        BUILD=$build `
        NDS_DEV_SCENE_HARNESS=battle_playable_realtime `
        NDS_DEV_LIVE_INPUT_PREVIEW=1 `
        NDS_HARNESS_FAST_LOGIC=0 `
        NDS_RENDERER_HW_TRIANGLES=1 `
        NDS_RENDERER_PROFILE_LEVEL=0 `
        NDS_DEBUG_HUD=0 `
        NDS_SCENE_MIP_CACHE_LAB=0 `
        NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS=1 `
        -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($required in @($rom, $elf, $metadataPath, $audioSourcePath,
        $audioHeaderPath, $upstreamPath, $captureHelper, $buildConfigPath,
        $Gdb)) {
    Assert-Condition (Test-Path -LiteralPath $required -PathType Leaf) `
        "Required crowd-envelope timing input not found: $required"
}
$buildConfigText = Get-Content -LiteralPath $buildConfigPath -Raw
Assert-Condition ($buildConfigText -match
    '(?m)^#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS 1\r?$') `
    ('Crowd ACK verification requires its dedicated diagnostic build; ' +
    'the selected build config does not enable the trace.')

$nm = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-nm.exe'
Assert-Condition (Test-Path -LiteralPath $nm -PathType Leaf) `
    "ARM symbol tool not found: $nm"
$nmTable = @(& $nm -a $elf)
Assert-Condition ($LASTEXITCODE -eq 0) `
    'Could not inspect the crowd ACK diagnostic ELF timing symbols.'
foreach ($symbol in @(
        'ndsAudioFgmPlay',
        'ndsAudioFgmUpdate',
        'ndsAudioFgmArm7AckTraceRecord',
        'ndsAudioFgmReleaseHandle',
        'ndsBattlePlayableFrameCompleteMarker',
        'sNdsAudioFgmPack',
        'sNdsAudioFgmEntries',
        'sNdsAudioFgmHandles',
        'sNdsAudioFgmChannelOwners',
        'sNdsAudioFgmChannelGenerations',
        'gNdsAudioFgmResult',
        'gNdsAudioFgmLoaded',
        'gNdsAudioFgmPlayCalls',
        'gNdsAudioFgmSupportedPlayCount',
        'gNdsAudioFgmPlayFailCount',
        'gNdsAudioFgmPhasePlayMask',
        'gNdsAudioFgmPhasePlayCounts',
        'gNdsAudioFgmLoopPlayCount',
        'gNdsAudioFgmDurationStopCount',
        'gNdsAudioFgmStaleStopCount',
        'gNdsAudioFgmGenerationMismatchCount',
        'gNdsAudioFgmOpenFailCount',
        'gNdsAudioFgmReadFailCount',
        'gNdsAudioFgmFormatFailCount',
        'gNdsAudioFgmIncludedLookupFailCount',
        'gNdsAudioFgmLastID',
        'gNdsAudioFgmLastGeneration',
        'gNdsAudioFgmLastInstanceToken',
        'gNdsAudioFgmInstanceTokenWrapCount',
        'gNdsAudioFgmPoolExhaustCount',
        'gNdsAudioFgmHandleReleaseCount',
        'gNdsAudioFgmHandleCapacity',
        'gNdsAudioFgmEnvelopeStepCount',
        'gNdsAudioFgmArm7AckTrace',
        'gNdsSceneHarnessMode',
        'gNdsSceneHarnessResult',
        'gNdsRendererProfileLevel',
        'gNdsMemoryLedgerResult',
        'gNdsMemoryLedgerScene',
        'gNdsMemoryLedgerArenaHeadroom',
        'gSCManagerSceneData',
        'gSCManagerBattleState',
        'sIFCommonTimerIsStarted')) {
    Assert-ElfSymbol -Name $symbol -NmTable $nmTable
}

# GDB must resolve the semantic source locations used to rendezvous with the
# dedicated runtime trace. Do not substitute frame counts or infer command
# application timing from unrelated state.
$preflightOutput = @(& $Gdb -batch `
    -ex 'set pagination off' `
    -ex 'set confirm off' `
    -ex 'delete breakpoints' `
    -ex ("break scvsbattle.c:{0}" -f $upstreamCallLine) `
    -ex 'break ndsAudioFgmArm7AckTraceRecord' `
    -ex 'condition 2 $r1 == 3' `
    -ex 'break ndsAudioFgmReleaseHandle' `
    -ex 'condition 3 $r2 == 3' `
    -ex 'break ndsBattlePlayableFrameCompleteMarker' `
    -ex 'info breakpoints' `
    $elf 2>&1)
$preflightText = $preflightOutput -join "`n"
if (($LASTEXITCODE -ne 0) -or
    ($preflightText -match '(?i)no symbol|no source file|no line|not defined')) {
    throw (
        'Crowd ACK diagnostic ELF lacks the source-local rendezvous required ' +
        "for this gate.`n$preflightText`n" +
        'Retain the dedicated two-event ID 626 ARM7 acknowledgment trace; ' +
        'do not replace it with frame-count inference.')
}

New-Item -ItemType Directory -Path $logDir, $tempDir,
    (Split-Path -Parent $screenshotPath) -Force | Out-Null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath `
        -GdbPort $verifierContext.GdbPort `
        -Persistent:([bool]$verifierContext.PersistentConfig) `
        -MuteAudio
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator `
        -Port $verifierContext.GdbPort | Out-Null

    $captureHelperGdb = $captureHelper.Replace('\', '/')
    $screenshotGdb = $screenshotPath.Replace('\', '/')
    $captureCommand =
        'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" -EmulatorProcessId {1} -Output "{2}"' -f
        $captureHelperGdb, $emulator.Id, $screenshotGdb

    # These are semantic stops only: the upstream source call, the actual ID
    # 626 play, its natural duration release, and the immediately following
    # completed frame. The live handle at release proves ownership persisted
    # beyond the baked-silence threshold without a mid-tail volume command.
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 10',
        ("target remote 127.0.0.1:{0}" -f $verifierContext.GdbPort),
        ("tbreak scvsbattle.c:{0}" -f $upstreamCallLine),
        'continue',
        ('printf "CROWD_SOURCE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
            'gNdsSceneHarnessMode, gSCManagerSceneData.scene_curr, ' +
            'gNdsAudioFgmPhasePlayCounts[0], ' +
            'gNdsAudioFgmSupportedPlayCount, gNdsAudioFgmPlayCalls, ' +
            'gSCManagerBattleState->game_status, ' +
            'sIFCommonTimerIsStarted, gSCManagerBattleState->time_limit, ' +
            'gSCManagerBattleState->time_remain, ' +
            'gSCManagerBattleState->time_passed'),
        'tbreak ndsAudioFgmPlay if $r0 == 626',
        'continue',
        'finish',
        'set $public_handle = (NDSAudioFgmHandle *)$r0',
        'set $public_channel = (int)$public_handle->channel',
        'set $public_generation = (unsigned)$public_handle->generation',
        'set $public_start = (unsigned)$public_handle->start_tick',
        'set $public_end = (unsigned)$public_handle->end_tick',
        'set $public_entry = &sNdsAudioFgmEntries[0]',
        ('printf "CROWD_PLAY_HANDLE=%#x,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u\n", ' +
            '(unsigned)$public_handle, $public_handle->fgm_id, ' +
            '$public_handle->generation, $public_handle->start_tick, ' +
            '$public_handle->end_tick, $public_handle->envelope_offset, ' +
            '$public_handle->envelope_count, ' +
            '$public_handle->envelope_index, $public_handle->channel, ' +
            '$public_handle->allocated, $public_handle->live, ' +
            '$public_handle->effect.sfx_id'),
        ('printf "CROWD_PLAY_STATE=%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x\n", ' +
            'gNdsAudioFgmPhasePlayCounts[0], gNdsAudioFgmPhasePlayMask, ' +
            'gNdsAudioFgmSupportedPlayCount, gNdsAudioFgmLoopPlayCount, ' +
            'gNdsAudioFgmLastID, gNdsAudioFgmLastGeneration, ' +
            'gNdsAudioFgmLastInstanceToken, gNdsAudioFgmPlayCalls, ' +
            'gNdsAudioFgmPlayFailCount, gNdsAudioFgmPoolExhaustCount, ' +
            'gNdsAudioFgmGenerationMismatchCount, gNdsAudioFgmLoaded, ' +
            'gNdsAudioFgmResult'),
        ('printf "CROWD_PLAY_OWNER=%#x,%u,%u\n", ' +
            '(unsigned)sNdsAudioFgmChannelOwners[$public_channel], ' +
            'sNdsAudioFgmChannelGenerations[$public_channel], ' +
            '$public_handle->generation'),
        ('printf "CROWD_ENTRY=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
            '$public_entry->id, $public_entry->flags, ' +
            '$public_entry->data_offset, $public_entry->data_bytes, ' +
            '$public_entry->sample_count, $public_entry->frequency, ' +
            '$public_entry->duration_ticks, $public_entry->envelope_offset, ' +
            '$public_entry->envelope_count, $public_entry->volume, ' +
            '$public_entry->pan, $public_entry->loop_point_words'),
        'set $public_scan_i = 0',
        'set $public_allocated_count = 0',
        'set $public_live_count = 0',
        'while $public_scan_i < gNdsAudioFgmHandleCapacity',
        'if sNdsAudioFgmHandles[$public_scan_i].fgm_id == 626',
        'if sNdsAudioFgmHandles[$public_scan_i].allocated != 0',
        'set $public_allocated_count = $public_allocated_count + 1',
        'end',
        'if sNdsAudioFgmHandles[$public_scan_i].live != 0',
        'set $public_live_count = $public_live_count + 1',
        'end',
        'end',
        'set $public_scan_i = $public_scan_i + 1',
        'end',
        ('printf "CROWD_PLAY_DUP=%u,%u,%u\n", ' +
            'gNdsAudioFgmPhasePlayCounts[0], $public_allocated_count, ' +
            '$public_live_count'),
        ('tbreak ndsAudioFgmReleaseHandle if ' +
            '$r0 == $public_handle && $r2 == 3'),
        'continue',
        'set $public_stop_now = (unsigned)$r3',
        'set $public_duration_stops_before = gNdsAudioFgmDurationStopCount',
        'set $public_releases_before = gNdsAudioFgmHandleReleaseCount',
        ('printf "CROWD_STOP=%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u\n", ' +
            '$public_stop_now, $public_handle->start_tick, ' +
            '$public_handle->end_tick, ' +
            '(unsigned)($public_stop_now - $public_handle->start_tick), ' +
            'gNdsAudioFgmArm7AckTrace.duration_ticks, ' +
            '$public_handle->envelope_index, ' +
            '$public_handle->envelope_count, $public_handle->fgm_id, ' +
            '$public_handle->generation, $public_handle->channel, ' +
            'gNdsAudioFgmDurationStopCount, $public_handle->live, ' +
            '$public_handle->allocated, $public_handle->effect.sfx_id'),
        ('printf "CROWD_STOP_STATE=%u,%u,%u,%u,%#x,%u,%u,%u\n", ' +
            'gNdsAudioFgmPhasePlayCounts[0], gNdsAudioFgmPlayFailCount, ' +
            'gNdsAudioFgmPoolExhaustCount, ' +
            'gNdsAudioFgmGenerationMismatchCount, ' +
            '(unsigned)sNdsAudioFgmChannelOwners[$public_channel], ' +
            'sNdsAudioFgmChannelGenerations[$public_channel], ' +
            'gNdsAudioFgmEnvelopeStepCount, gNdsAudioFgmLoopPlayCount'),
        'set $public_scan_i = 0',
        'set $public_allocated_count = 0',
        'set $public_live_count = 0',
        'while $public_scan_i < gNdsAudioFgmHandleCapacity',
        'if sNdsAudioFgmHandles[$public_scan_i].fgm_id == 626',
        'if sNdsAudioFgmHandles[$public_scan_i].allocated != 0',
        'set $public_allocated_count = $public_allocated_count + 1',
        'end',
        'if sNdsAudioFgmHandles[$public_scan_i].live != 0',
        'set $public_live_count = $public_live_count + 1',
        'end',
        'end',
        'set $public_scan_i = $public_scan_i + 1',
        'end',
        ('printf "CROWD_STOP_DUP=%u,%u,%u\n", ' +
            'gNdsAudioFgmPhasePlayCounts[0], $public_allocated_count, ' +
            '$public_live_count'),
        ('printf "CROWD_STOP_SCENE=%u,%u,%u,%u,%u,%u\n", ' +
            'gSCManagerSceneData.scene_curr, ' +
            'gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, ' +
            'gSCManagerBattleState->time_limit, ' +
            'gSCManagerBattleState->time_remain, ' +
            'gSCManagerBattleState->time_passed'),
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'set $public_scan_i = 0',
        'set $public_allocated_count = 0',
        'set $public_live_count = 0',
        'while $public_scan_i < gNdsAudioFgmHandleCapacity',
        'if sNdsAudioFgmHandles[$public_scan_i].fgm_id == 626',
        'if sNdsAudioFgmHandles[$public_scan_i].allocated != 0',
        'set $public_allocated_count = $public_allocated_count + 1',
        'end',
        'if sNdsAudioFgmHandles[$public_scan_i].live != 0',
        'set $public_live_count = $public_live_count + 1',
        'end',
        'end',
        'set $public_scan_i = $public_scan_i + 1',
        'end',
        ('printf "CROWD_RELEASE=%u,%u,%u,%d,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u\n", ' +
            '$public_handle->allocated, $public_handle->live, ' +
            '$public_handle->fgm_id, $public_handle->channel, ' +
            '$public_handle->effect.sfx_id, $public_handle->generation, ' +
            '$public_handle->envelope_index, ' +
            '$public_handle->envelope_count, ' +
            '(unsigned)sNdsAudioFgmChannelOwners[$public_channel], ' +
            'sNdsAudioFgmChannelGenerations[$public_channel], ' +
            '$public_releases_before, gNdsAudioFgmHandleReleaseCount, ' +
            '$public_duration_stops_before, gNdsAudioFgmDurationStopCount'),
        ('printf "CROWD_RELEASE_DUP=%u,%u,%u\n", ' +
            'gNdsAudioFgmPhasePlayCounts[0], $public_allocated_count, ' +
            '$public_live_count'),
        ('printf "CROWD_RELEASE_SCENE=%u,%u,%u,%u,%u,%u\n", ' +
            'gSCManagerSceneData.scene_curr, ' +
            'gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, ' +
            'gSCManagerBattleState->time_limit, ' +
            'gSCManagerBattleState->time_remain, ' +
            'gSCManagerBattleState->time_passed'),
        ('printf "CROWD_ACK_TRACE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
            'gNdsAudioFgmArm7AckTrace.sequence, ' +
            'gNdsAudioFgmArm7AckTrace.event_count, ' +
            'gNdsAudioFgmArm7AckTrace.overflow_count, ' +
            'gNdsAudioFgmArm7AckTrace.mismatch_count, ' +
            'gNdsAudioFgmArm7AckTrace.fgm_id, ' +
            'gNdsAudioFgmArm7AckTrace.generation, ' +
            'gNdsAudioFgmArm7AckTrace.channel, ' +
            'gNdsAudioFgmArm7AckTrace.instance_token, ' +
            'gNdsAudioFgmArm7AckTrace.handle_start_tick, ' +
            'gNdsAudioFgmArm7AckTrace.handle_end_tick, ' +
            'gNdsAudioFgmArm7AckTrace.duration_ticks, ' +
            'gNdsAudioFgmArm7AckTrace.envelope_count'),
        ('printf "CROWD_ACK_PLAY=%u,%u,%u,%u,%u,%u,%u,%#x\n", ' +
            'gNdsAudioFgmArm7AckTrace.events[0].kind, ' +
            'gNdsAudioFgmArm7AckTrace.events[0].source_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[0].value, ' +
            'gNdsAudioFgmArm7AckTrace.events[0].service_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[0].command_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[0].command_return_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[0].acknowledge_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[0].active_channels'),
        ('printf "CROWD_ACK_STOP=%u,%u,%u,%u,%u,%u,%u,%#x\n", ' +
            'gNdsAudioFgmArm7AckTrace.events[1].kind, ' +
            'gNdsAudioFgmArm7AckTrace.events[1].source_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[1].value, ' +
            'gNdsAudioFgmArm7AckTrace.events[1].service_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[1].command_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[1].command_return_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[1].acknowledge_tick, ' +
            'gNdsAudioFgmArm7AckTrace.events[1].active_channels'),
        ('printf "CROWD_ERRORS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
            'gNdsAudioFgmOpenFailCount, gNdsAudioFgmReadFailCount, ' +
            'gNdsAudioFgmFormatFailCount, ' +
            'gNdsAudioFgmIncludedLookupFailCount, gNdsAudioFgmPlayFailCount, ' +
            'gNdsAudioFgmPoolExhaustCount, ' +
            'gNdsAudioFgmGenerationMismatchCount, ' +
            'gNdsAudioFgmStaleStopCount, ' +
            'gNdsAudioFgmInstanceTokenWrapCount'),
        ('printf "CROWD_MEM=%#x,%u,%u\n", gNdsMemoryLedgerResult, ' +
            'gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom'),
        ('printf "CROWD_HARN=%#x,%u,%u,%u\n", ' +
            'gNdsSceneHarnessResult, gNdsSceneHarnessMode, ' +
            'gSCManagerSceneData.scene_curr, gNdsRendererProfileLevel'),
        # Capture the exact post-release completed frame. This is visibility
        # evidence only; it does not prove mixed output or acoustic fidelity.
        $captureCommand,
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript `
        -Gdb $Gdb `
        -Elf $elf `
        -Root $root `
        -Commands $gdbCommands `
        -ScriptName $scriptName `
        -TimeoutSeconds $TimeoutSeconds).Stdout

    $source = Get-MarkerRow $gdbStdout 'CROWD_SOURCE' 10
    $playHandle = Get-MarkerRow $gdbStdout 'CROWD_PLAY_HANDLE' 12
    $playState = Get-MarkerRow $gdbStdout 'CROWD_PLAY_STATE' 13
    $playOwner = Get-MarkerRow $gdbStdout 'CROWD_PLAY_OWNER' 3
    $entry = Get-MarkerRow $gdbStdout 'CROWD_ENTRY' 12
    $playDuplicate = Get-MarkerRow $gdbStdout 'CROWD_PLAY_DUP' 3
    $stop = Get-MarkerRow $gdbStdout 'CROWD_STOP' 14
    $stopState = Get-MarkerRow $gdbStdout 'CROWD_STOP_STATE' 8
    $stopDuplicate = Get-MarkerRow $gdbStdout 'CROWD_STOP_DUP' 3
    $stopScene = Get-MarkerRow $gdbStdout 'CROWD_STOP_SCENE' 6
    $release = Get-MarkerRow $gdbStdout 'CROWD_RELEASE' 14
    $releaseDuplicate = Get-MarkerRow $gdbStdout 'CROWD_RELEASE_DUP' 3
    $releaseScene = Get-MarkerRow $gdbStdout 'CROWD_RELEASE_SCENE' 6
    $ackTrace = Get-MarkerRow $gdbStdout 'CROWD_ACK_TRACE' 12
    $ackPlay = Get-MarkerRow $gdbStdout 'CROWD_ACK_PLAY' 8
    $ackStop = Get-MarkerRow $gdbStdout 'CROWD_ACK_STOP' 8
    $errors = Get-MarkerRow $gdbStdout 'CROWD_ERRORS' 9
    $memory = Get-MarkerRow $gdbStdout 'CROWD_MEM' 3
    $harn = Get-MarkerRow $gdbStdout 'CROWD_HARN' 4

    Assert-Condition (($source[0] -eq $expectedHarnessMode) -and
        ($source[1] -eq $expectedBattleScene) -and
        ($source[2] -eq 0) -and ($source[3] -eq 0) -and
        ($source[4] -eq 0) -and ($source[5] -eq 0) -and
        ($source[6] -eq 0) -and ($source[7] -eq 1)) `
        ('PublicExcited was not reached as the first normal battle-start FGM ' +
        'inside the source one-minute Wait state.') `
        $gdbStdout

    $publicHandleAddress = [uint32]$playHandle[0]
    $publicGeneration = [uint32]$playHandle[2]
    $publicStart = [uint32]$playHandle[3]
    $publicEnd = [uint32]$playHandle[4]
    $publicChannel = [int]$playHandle[8]
    Assert-Condition (($publicHandleAddress -ne 0) -and
        ($playHandle[1] -eq $publicExcitedID) -and
        ($publicGeneration -ne 0) -and
        ($playHandle[5] -eq $entry[7]) -and
        ($playHandle[6] -eq 0) -and
        ($playHandle[7] -eq 0) -and
        ($publicChannel -ge 0) -and ($publicChannel -lt 16) -and
        ($playHandle[9] -eq 1) -and ($playHandle[10] -eq 1) -and
        ($playHandle[11] -ne 0)) `
        'PublicExcited did not return one live generation-backed handle.' `
        $gdbStdout
    Assert-Condition (($playState[0] -eq 1) -and
        ($playState[1] -eq 1) -and ($playState[2] -eq 1) -and
        ($playState[3] -eq 0) -and
        ($playState[4] -eq $publicExcitedID) -and
        ($playState[5] -eq $publicGeneration) -and
        ($playState[6] -eq $playHandle[11]) -and
        ($playState[7] -eq 1) -and
        ($playState[8] -eq 0) -and ($playState[9] -eq 0) -and
        ($playState[10] -eq 0) -and ($playState[11] -eq 1) -and
        ($playState[12] -eq $expectedFgmResult)) `
        'PublicExcited play accounting or pack state was not source-clean.' `
        $gdbStdout
    Assert-Condition (($playOwner[0] -eq $publicHandleAddress) -and
        ($playOwner[1] -eq $publicGeneration) -and
        ($playOwner[2] -eq $publicGeneration)) `
        'PublicExcited channel ownership did not match its generation.' `
        $gdbStdout
    Assert-Condition (($entry[0] -eq $publicExcitedID) -and
        ($entry[1] -eq 0) -and
        ($entry[2] -eq $packedDataOffset) -and
        ($entry[3] -eq $packedDataBytes) -and
        ($entry[4] -eq $packedSampleCount) -and
        ($entry[5] -eq $packedFrequencyHz) -and
        ($entry[6] -eq $sourceDurationTicks) -and
        ($entry[7] -eq 0) -and ($entry[8] -eq 0) -and
        ($entry[9] -eq [int]$publicEntry.ds_volume) -and
        ($entry[10] -eq 64) -and ($entry[11] -eq 0)) `
        'Resident PublicExcited entry did not match its source metadata.' `
        $gdbStdout
    Assert-Condition (($playDuplicate[0] -eq 1) -and
        ($playDuplicate[1] -eq 1) -and ($playDuplicate[2] -eq 1)) `
        'PublicExcited acquired more than one allocated/live ID 626 handle.' `
        $gdbStdout

    $stopNow = [uint32]$stop[0]
    $stopElapsed = Get-UInt32Delta -Start $publicStart -End $stopNow
    $storedDurationTicks = Get-UInt32Delta `
        -Start $publicStart -End $publicEnd
    # Match the backend's u64 integer division. PowerShell's `/` produces a
    # floating value and a direct [uint64] cast rounds the .8 remainder up.
    $expectedDurationCpuTicks = [uint64][math]::Floor(
        ([double]$busClock * [double]$sourceStopMicroseconds) / 1000000.0)
    Assert-Condition ($storedDurationTicks -eq $expectedDurationCpuTicks) `
        'PublicExcited handle duration no longer matches 6,900,000us.' `
        $gdbStdout
    Assert-Condition (($stop[1] -eq $publicStart) -and
        ($stop[2] -eq $publicEnd) -and
        ([uint64]$stop[3] -eq $stopElapsed) -and
        ($stop[4] -ge ($sourceDurationTicks - 1)) -and
        ($stop[5] -eq 0) -and ($stop[6] -eq 0) -and
        ($stop[7] -eq $publicExcitedID) -and
        ($stop[8] -eq $publicGeneration) -and
        ($stop[9] -eq $publicChannel) -and
        ($stop[11] -eq 1) -and ($stop[12] -eq 1) -and
        ($stop[13] -eq $playHandle[11])) `
        'PublicExcited did not reach its natural duration-release branch.' `
        $gdbStdout
    Assert-Condition (($stopState[0] -eq 1) -and
        ($stopState[1] -eq 0) -and ($stopState[2] -eq 0) -and
        ($stopState[3] -eq 0) -and
        ($stopState[4] -eq $publicHandleAddress) -and
        ($stopState[5] -eq $publicGeneration) -and
        ($stopState[6] -eq 0) -and ($stopState[7] -eq 0)) `
        'PublicExcited generation/ownership was not intact at duration release.' `
        $gdbStdout
    Assert-Condition (($stopDuplicate[0] -eq 1) -and
        ($stopDuplicate[1] -eq 1) -and ($stopDuplicate[2] -eq 1)) `
        'PublicExcited duplicated before its duration release.' $gdbStdout
    Assert-Condition (($stopScene[0] -eq $expectedBattleScene) -and
        ($stopScene[3] -eq 1) -and
        (($stopScene[4] + $stopScene[5]) -eq $expectedMatchTicks) -and
        ((($stopScene[1] -eq 0) -and ($stopScene[2] -eq 0) -and
          ($stopScene[4] -eq $expectedMatchTicks) -and
          ($stopScene[5] -eq 0)) -or
         (($stopScene[1] -eq 1) -and ($stopScene[2] -eq 1) -and
          ($stopScene[4] -gt 0) -and ($stopScene[5] -gt 0)))) `
        ('PublicExcited duration release left the coherent BattleShip ' +
        'Wait/Go one-minute timer state.') $gdbStdout
    Assert-Condition ($stopElapsed -ge $storedDurationTicks) `
        'PublicExcited duration release occurred before its source due point.' `
        $gdbStdout
    $silentTailDueTicks =
        Get-MinCpuTicksForMicroseconds $silentTailStartMicroseconds
    Assert-Condition (($stopElapsed -ge $silentTailDueTicks) -and
        (($packedSampleCount * 1000000UL) -ge
            ($sourceStopMicroseconds * $packedFrequencyHz))) `
        ('PublicExcited was not still generation-owned after its baked-silence ' +
        'threshold with packed samples covering the natural source stop.') `
        $gdbStdout
    $stopLatenessTicks = $stopElapsed - $storedDurationTicks
    $stopLatenessMicroseconds =
        Convert-CpuTicksToMicroseconds $stopLatenessTicks

    $targetChannelBit = [uint32](1 -shl $publicChannel)
    Assert-Condition (($ackTrace[0] -eq 1) -and
        ($ackTrace[1] -eq 2) -and ($ackTrace[2] -eq 0) -and
        ($ackTrace[3] -eq 0) -and
        ($ackTrace[4] -eq $publicExcitedID) -and
        ($ackTrace[5] -eq $publicGeneration) -and
        ($ackTrace[6] -eq $publicChannel) -and
        ($ackTrace[7] -eq $playHandle[11]) -and
        ($ackTrace[8] -eq $publicStart) -and
        ($ackTrace[9] -eq $publicEnd) -and
        ($ackTrace[10] -eq $sourceDurationTicks) -and
        ($ackTrace[11] -eq 0)) `
        ('PublicExcited ARM7 ACK trace identity, capacity, or source ' +
        'schedule metadata was not coherent.') $gdbStdout
    Assert-Condition (($ackPlay[0] -eq 1) -and
        ($ackPlay[1] -eq 0) -and
        ($ackPlay[2] -eq [int]$publicEntry.ds_volume) -and
        ($ackPlay[3] -eq $publicStart) -and
        (([uint32]$ackPlay[7] -band $targetChannelBit) -ne 0)) `
        ('Calico did not acknowledge the PublicExcited start command with ' +
        'its assigned hardware channel active.') $gdbStdout
    Assert-Condition (($ackStop[0] -eq 3) -and
        ($ackStop[1] -eq $sourceDurationTicks) -and
        ($ackStop[2] -eq $expectedDurationReleaseReason) -and
        ($ackStop[3] -eq $stopNow) -and
        (([uint32]$ackStop[7] -band $targetChannelBit) -eq 0)) `
        ('Calico did not acknowledge the natural PublicExcited duration ' +
        'stop with its hardware channel inactive.') $gdbStdout

    $playCommandTicks = Get-UInt32Delta `
        -Start ([uint32]$ackPlay[4]) -End ([uint32]$ackPlay[5])
    $playPublishTicks = Get-UInt32Delta `
        -Start ([uint32]$ackPlay[5]) -End ([uint32]$ackPlay[3])
    $playAckTicks = Get-UInt32Delta `
        -Start ([uint32]$ackPlay[5]) -End ([uint32]$ackPlay[6])
    $stopServiceTicks = Get-UInt32Delta `
        -Start ([uint32]$ackStop[3]) -End ([uint32]$ackStop[4])
    $stopCommandTicks = Get-UInt32Delta `
        -Start ([uint32]$ackStop[4]) -End ([uint32]$ackStop[5])
    $stopAckTicks = Get-UInt32Delta `
        -Start ([uint32]$ackStop[5]) -End ([uint32]$ackStop[6])
    foreach ($orderedDelta in @(
            $playCommandTicks, $playPublishTicks, $playAckTicks,
            $stopServiceTicks, $stopCommandTicks, $stopAckTicks)) {
        Assert-Condition ($orderedDelta -lt 0x80000000UL) `
            'ARM7 ACK trace CPU ticks were not in modular event order.' `
            $gdbStdout
    }

    $playAckWaitMicroseconds = Convert-CpuTicksToMicroseconds $playAckTicks
    $stopAckWaitMicroseconds = Convert-CpuTicksToMicroseconds $stopAckTicks
    foreach ($ackWaitMicroseconds in @(
            $playAckWaitMicroseconds, $stopAckWaitMicroseconds)) {
        Assert-Condition (
            $ackWaitMicroseconds -le $MaxAckWaitMicroseconds) `
            ('Calico ARM7 command acknowledgment exceeded the ' +
            "$MaxAckWaitMicroseconds us one-shot wait limit: " +
            "$ackWaitMicroseconds us.") $gdbStdout
    }

    $stopAckElapsed = Get-UInt32Delta `
        -Start $publicStart -End ([uint32]$ackStop[6])
    Assert-Condition ($stopAckElapsed -ge $storedDurationTicks) `
        'PublicExcited stop was acknowledged before its source duration.' `
        $gdbStdout
    $stopAckLatenessMicroseconds = Convert-CpuTicksToMicroseconds `
        ($stopAckElapsed - $storedDurationTicks)

    Assert-Condition (($release[0] -eq 0) -and ($release[1] -eq 0) -and
        ($release[2] -eq 0) -and ($release[3] -eq -1) -and
        ($release[4] -eq 0) -and ($release[5] -eq $publicGeneration) -and
        ($release[6] -eq 0) -and ($release[7] -eq 0) -and
        ($release[8] -eq 0) -and ($release[9] -eq 0) -and
        ($release[11] -ge ($release[10] + 1)) -and
        ($release[12] -eq $stop[10]) -and
        ($release[13] -ge ($release[12] + 1))) `
        ('PublicExcited did not finish release with its handle, token, and ' +
        'channel ownership cleared and release counters advanced.') $gdbStdout
    Assert-Condition (($releaseDuplicate[0] -eq 1) -and
        ($releaseDuplicate[1] -eq 0) -and
        ($releaseDuplicate[2] -eq 0)) `
        'PublicExcited replayed or retained an ID 626 handle after release.' `
        $gdbStdout
    Assert-Condition (($releaseScene[0] -eq $expectedBattleScene) -and
        ($releaseScene[3] -eq 1) -and
        (($releaseScene[4] + $releaseScene[5]) -eq $expectedMatchTicks) -and
        ((($releaseScene[1] -eq 0) -and ($releaseScene[2] -eq 0) -and
          ($releaseScene[4] -eq $expectedMatchTicks) -and
          ($releaseScene[5] -eq 0)) -or
         (($releaseScene[1] -eq 1) -and ($releaseScene[2] -eq 1) -and
          ($releaseScene[4] -gt 0) -and ($releaseScene[5] -gt 0)))) `
        ('PublicExcited post-release frame left the coherent BattleShip ' +
        'Wait/Go one-minute timer state.') $gdbStdout
    Assert-Condition ((@($errors | Where-Object { $_ -ne 0 }).Count) -eq 0) `
        'Crowd timing encountered an FGM load, lookup, play, or handle error.' `
        $gdbStdout
    Assert-Condition (($memory[0] -eq $expectedMemoryResult) -and
        ($memory[1] -eq $expectedBattleScene) -and
        ($memory[2] -ge 131072)) `
        'Crowd timing violated the P1 arena reserve floor.' $gdbStdout

    Assert-Condition (($harn[0] -eq $expectedHarnessResult) -and
        ($harn[1] -eq $expectedHarnessMode) -and
        ($harn[2] -eq $expectedBattleScene) -and ($harn[3] -eq 0)) `
        'Crowd timing did not run in canonical mode 163/profile 0 battle.' `
        $gdbStdout
    Assert-Condition (Test-Path -LiteralPath $screenshotPath -PathType Leaf) `
        "Crowd timing did not produce its visibility screenshot: $screenshotPath" `
        $gdbStdout
    Assert-Condition ((Get-Item -LiteralPath $screenshotPath).Length -gt 1024) `
        "Crowd timing screenshot is unexpectedly small: $screenshotPath" `
        $gdbStdout
    Assert-Condition (
        $stopLatenessMicroseconds -le $MaxServiceLatenessMicroseconds) `
        ('PublicExcited duration-release ARM9 gameplay-service dispatch was ' +
        "$stopLatenessMicroseconds us late (limit " +
        "$MaxServiceLatenessMicroseconds us).") $gdbStdout
    Assert-Condition (
        $stopAckLatenessMicroseconds -le $MaxServiceLatenessMicroseconds) `
        ('PublicExcited duration-stop ARM7 command acknowledgment was ' +
        "$stopAckLatenessMicroseconds us late (limit " +
        "$MaxServiceLatenessMicroseconds us).") $gdbStdout

    Write-Output (
        ('Crowd AOT Calico PLAY/STOP timing passed: id={0} generation={1} ' +
        'source_points={2} packed_points=0 source_line={3} ' +
        'silent_tail_start={4}us stop_due={5}us ' +
        'stop_service/ack_late={6}/{7}us ack_wait_play/stop={8}/{9}us ' +
        'reserve={10} screenshot={11}. Host audio was muted; command ACKs do not ' +
        'prove the final mixed output or acoustic fidelity.') -f
        $publicExcitedID, $publicGeneration, $sourceEnvelope.Count,
        $upstreamCallLine, $silentTailStartMicroseconds,
        $sourceStopMicroseconds, $stopLatenessMicroseconds,
        $stopAckLatenessMicroseconds, $playAckWaitMicroseconds,
        $stopAckWaitMicroseconds, $memory[2], $screenshotPath)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    Remove-Item $stdout, $stderr -Force `
        -ErrorAction SilentlyContinue
}
