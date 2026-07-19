param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 1,
    [int]$GdbPort = 3343,
    [switch]$NoBuild,
    [int]$DelaySeconds = 30
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-audio-fgm-hwtri'
$buildName = 'build-battle-playable-audio-fgm-hwtri-harness'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $target -Build $buildName -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $target -Build $buildName -Extension '.elf'
$object = Join-Path $root "builds\$buildName\nds_audio_fgm.o"
$buildConfig = Join-Path $root "builds\$buildName\nds_build_config.h"
$metadataPath = Join-Path $root 'assets\audio\fgm_phase_pack_ima.json'
$runnerExe = Join-Path $root "emulators\melonds-runners\slot$RunnerSlot\melonDS.exe"

& (Join-Path $PSScriptRoot 'check-audio-fgm-phase-pack.ps1')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path $PSScriptRoot 'check-audio-runtime-fixtures.ps1')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$peakMin = [int](($metadata.entries | Measure-Object decoded_peak -Minimum).Minimum)
$rmsMin = [double](($metadata.entries | Measure-Object decoded_rms -Minimum).Minimum)
if (($peakMin -le 0) -or ($rmsMin -le 0.0)) {
    throw 'Offline FGM waveform evidence is silent.'
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
if (-not $NoBuild) {
    & make -C $root `
        TARGET=$target `
        BUILD=$buildName `
        NDS_DEV_SCENE_HARNESS=battle_playable_realtime `
        NDS_DEV_LIVE_INPUT_PREVIEW=1 `
        NDS_HARNESS_FAST_LOGIC=0 `
        NDS_RENDERER_HW_TRIANGLES=1 `
        NDS_DEBUG_HUD=0 `
        NDS_RENDERER_PROFILE_LEVEL=0 `
        NDS_SCENE_MIP_CACHE_LAB=0 `
        NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS=0 `
        -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($required in @($rom, $elf, $object, $buildConfig)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Audio FGM build output not found: $required"
    }
}
$buildConfigText = Get-Content -LiteralPath $buildConfig -Raw
if ($buildConfigText -notmatch
    '(?m)^#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS 0\r?$') {
    throw 'Production FGM verification must compile with ARM7 ACK diagnostics disabled.'
}

$sizeTool = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-size.exe'
if (-not (Test-Path -LiteralPath $sizeTool -PathType Leaf)) {
    throw "ARM size tool not found: $sizeTool"
}
$nmTool = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-nm.exe'
if (-not (Test-Path -LiteralPath $nmTool -PathType Leaf)) {
    throw "ARM symbol tool not found: $nmTool"
}
$objectSymbols = @(& $nmTool $object)
if ($LASTEXITCODE -ne 0) {
    throw 'Could not inspect the production FGM backend symbols.'
}
foreach ($forbiddenSymbol in @(
        'gNdsAudioFgmArm7AckTrace',
        'sNdsAudioFgmArm7AckSequence',
        'ndsAudioFgmArm7AckTraceRecord',
        'soundGetActiveChannels',
        'soundSetAutoUpdate')) {
    if ($objectSymbols | Select-String -SimpleMatch $forbiddenSymbol) {
        throw "Production FGM backend retained diagnostic symbol: $forbiddenSymbol"
    }
}
$sectionTotals = @{
    text = 0L
    rodata = 0L
    data = 0L
    bss = 0L
    itcm = 0L
}
& $sizeTool -A $object | ForEach-Object {
    if ($_ -match '^\.(text|rodata|data|bss|itcm)[^\s]*\s+([0-9]+)\s+') {
        $sectionTotals[$Matches[1]] += [int64]$Matches[2]
    }
}
if (($sectionTotals.text -gt 3584) -or
    ($sectionTotals.rodata -gt 512) -or
    ($sectionTotals.data -gt 16) -or
    ($sectionTotals.bss -gt ([int64]$metadata.resident_bytes + 1792L)) -or
    ($sectionTotals.itcm -ne 0)) {
    throw (('FGM backend binary budget failed: text={0} rodata={1} data={2} ' +
        'bss={3} itcm={4}.') -f
        $sectionTotals.text, $sectionTotals.rodata, $sectionTotals.data,
        $sectionTotals.bss, $sectionTotals.itcm)
}

if (-not (Test-Path -LiteralPath $runnerExe -PathType Leaf)) {
    & (Join-Path $PSScriptRoot 'New-MelonDSRunnerSlots.ps1') `
        -Count ($RunnerSlot + 1) `
        -MelonDS $MelonDS
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit
$expectedRunnerPath = (Resolve-Path -LiteralPath $runnerExe).Path
if (-not $verifierContext.MelonDSPath.Equals(
        $expectedRunnerPath, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Audio FGM verification must use isolated runner slot $RunnerSlot, not $($verifierContext.MelonDSPath)."
}
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir 'melonds.audio-fgm-phase.stdout.log'
$stderr = Join-Path $logDir 'melonds.audio-fgm-phase.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_audio_fgm_phase.gdb'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

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
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, 30))

    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f $verifierContext.GdbPort),
        'printf "HARN=%#x,%u,%u,%u\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gSCManagerSceneData.scene_curr, gNdsTaskmanBoundedUpdateCount',
        'printf "FGM_LOAD=%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsAudioFgmResult, gNdsAudioFgmMask, gNdsAudioFgmLoaded, gNdsAudioFgmResidentBytes, gNdsAudioFgmSupportedCount, gNdsAudioFgmOpenFailCount, gNdsAudioFgmReadFailCount, gNdsAudioFgmFormatFailCount',
        'printf "FGM_PLAY=%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsAudioFgmPlayCalls, gNdsAudioFgmSupportedPlayCount, gNdsAudioFgmUnsupportedCallCount, gNdsAudioFgmIncludedLookupFailCount, gNdsAudioFgmPlayFailCount, gNdsAudioFgmPhasePlayMask, gNdsAudioFgmLoopPlayCount, gNdsAudioFgmEnvelopeStepCount',
        'printf "FGM_PHASE=%u,%u,%u,%u,%u\n", gNdsAudioFgmPhasePlayCounts[0], gNdsAudioFgmPhasePlayCounts[1], gNdsAudioFgmPhasePlayCounts[2], gNdsAudioFgmPhasePlayCounts[3], gNdsAudioFgmPhasePlayCounts[4]',
        'printf "FGM_KO=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioFgmKoPlayMask, gNdsAudioFgmKoPlayCounts[0], gNdsAudioFgmKoPlayCounts[1], gNdsAudioFgmKoPlayCounts[2], gNdsAudioFgmKoPlayCounts[3], gNdsAudioFgmKoPlayCounts[4], gNdsAudioFgmKoTraceCount, gNdsAudioFgmKoTrace[0], gNdsAudioFgmKoTrace[1], gNdsAudioFgmKoTrace[2]',
        'printf "FGM_POOL=%u,%u,%u,%u,%u,%u,%u,%#x,%u,%#x\n", gNdsAudioFgmHandleAcquireCount, gNdsAudioFgmHandleCapacity, gNdsAudioFgmHandleReleaseCount, gNdsAudioFgmHandleRecycleCount, gNdsAudioFgmPoolExhaustCount, gNdsAudioFgmActiveHandles, gNdsAudioFgmMaxActiveHandles, gNdsAudioFgmChannelMask, gNdsAudioFgmLastChannel, gNdsAudioFgmFidelityDebtMask',
        'printf "FGM_LIFE=%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioFgmStopCalls, gNdsAudioFgmStopAllCalls, gNdsAudioFgmDurationStopCount, gNdsAudioFgmStaleStopCount, gNdsAudioFgmGenerationMismatchCount, gNdsAudioFgmLastInstanceToken, gNdsAudioFgmInstanceTokenWrapCount',
        'printf "BGM=%#x,%u,%u,%u,%d\n", gNdsAudioBgmResult, gNdsAudioBgmPlaying, gNdsAudioBgmChunkPlayCount, gNdsAudioBgmReadBytes, sNdsAudioBgmSoundID',
        'printf "MEM=%u,%u,%u\n", gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript `
        -Gdb $Gdb `
        -Elf $elf `
        -Root $root `
        -Commands $gdbCommands `
        -ScriptName $scriptName).Stdout

    $harn = [regex]::Match($gdbStdout,
        'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $load = [regex]::Match($gdbStdout,
        'FGM_LOAD=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $play = [regex]::Match($gdbStdout,
        'FGM_PLAY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $phase = [regex]::Match($gdbStdout,
        'FGM_PHASE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $ko = [regex]::Match($gdbStdout,
        'FGM_KO=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $pool = [regex]::Match($gdbStdout,
        'FGM_POOL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $life = [regex]::Match($gdbStdout,
        'FGM_LIFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $bgm = [regex]::Match($gdbStdout,
        'BGM=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $memory = [regex]::Match($gdbStdout,
        'MEM=([0-9]+),([0-9]+),([0-9]+)')

    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 163 -or
        [int]$harn.Groups[3].Value -ne 22 -or
        [int]$harn.Groups[4].Value -lt 300) {
        throw "Natural realtime battle did not reach the countdown window.`n$gdbStdout"
    }
    if (-not $load.Success -or
        (Convert-MarkerUInt32 $load.Groups[1].Value) -ne 0x46474d31 -or
        [int]$load.Groups[3].Value -ne 1 -or
        [int]$load.Groups[4].Value -ne [int]$metadata.resident_bytes -or
        [int]$load.Groups[5].Value -ne [int]$metadata.entry_count -or
        [int]$load.Groups[6].Value -ne 0 -or
        [int]$load.Groups[7].Value -ne 0 -or
        [int]$load.Groups[8].Value -ne 0) {
        throw "FGM phase pack did not load cleanly.`n$gdbStdout"
    }
    $supportedPlays = if ($play.Success) {
        [int]$play.Groups[2].Value
    } else { 0 }
    if (-not $play.Success -or
        $supportedPlays -lt 5 -or
        [int]$play.Groups[4].Value -ne 0 -or
        [int]$play.Groups[5].Value -ne 0 -or
        (Convert-MarkerUInt32 $play.Groups[6].Value) -ne 0x1f -or
        [int]$play.Groups[7].Value -ne 0 -or
        ([int]$play.Groups[1].Value -ne
         ([int]$play.Groups[2].Value + [int]$play.Groups[3].Value))) {
        throw "Natural FGM play accounting failed.`n$gdbStdout"
    }
    if (-not $phase.Success -or
        (@(1..5 | ForEach-Object { [int]$phase.Groups[$_].Value }) -ne 1)) {
        throw "One or more natural phase IDs did not play exactly once.`n$gdbStdout"
    }
    if (-not $ko.Success) {
        throw "Natural regular-KO diagnostics were absent.`n$gdbStdout"
    }
    $koValues = @(1..10 | ForEach-Object {
            [int64](Convert-MarkerUInt32 $ko.Groups[$_].Value)
        })
    $koIsIdle = (($koValues -join ',') -eq '0,0,0,0,0,0,0,0,0,0')
    $koIsExactMarioTrio = (($koValues -join ',') -eq
        '19,1,1,0,0,1,3,439,292,154')
    $koIsExactFoxTrio = (($koValues -join ',') -eq
        '28,0,0,1,1,1,3,370,289,154')
    if (-not $koIsIdle -and -not $koIsExactMarioTrio -and
        -not $koIsExactFoxTrio) {
        throw ('Natural regular-KO state was neither idle nor an exact Mario/' +
            "Fox voice/slam/explosion order.`n$gdbStdout")
    }
    $fgmChannelMask = if ($pool.Success) {
        Convert-MarkerUInt32 $pool.Groups[8].Value
    } else { [uint32]0 }
    if (-not $pool.Success -or
        [int]$pool.Groups[1].Value -ne $supportedPlays -or
        [int]$pool.Groups[2].Value -ne 8 -or
        [int]$pool.Groups[3].Value -gt [int]$pool.Groups[1].Value -or
        [int]$pool.Groups[4].Value -lt 1 -or
        [int]$pool.Groups[5].Value -ne 0 -or
        [int]$pool.Groups[6].Value -ne
            ([int]$pool.Groups[1].Value - [int]$pool.Groups[3].Value) -or
        [int]$pool.Groups[7].Value -lt 1 -or
        $fgmChannelMask -eq 0 -or
        [int]$pool.Groups[9].Value -ge 16 -or
        (Convert-MarkerUInt32 $pool.Groups[10].Value) -ne 28) {
        throw "FGM recycle/channel/fidelity diagnostics failed.`n$gdbStdout"
    }
    if (-not $life.Success -or
        [int]$life.Groups[5].Value -ne 0 -or
        [int]$life.Groups[6].Value -eq 0 -or
        [int]$life.Groups[7].Value -ne 0) {
        throw "FGM token/channel generation ownership failed.`n$gdbStdout"
    }
    $bgmChannel = if ($bgm.Success) { [int]$bgm.Groups[5].Value } else { -1 }
    if (-not $bgm.Success -or
        (Convert-MarkerUInt32 $bgm.Groups[1].Value) -ne 0x42474d31 -or
        [int]$bgm.Groups[2].Value -ne 1 -or
        [int]$bgm.Groups[3].Value -lt 1 -or
        [int]$bgm.Groups[4].Value -lt 65536 -or
        $bgmChannel -lt 0 -or $bgmChannel -ge 16 -or
        (($fgmChannelMask -band ([uint32]1 -shl $bgmChannel)) -ne 0)) {
        throw "FGM playback did not coexist on channels distinct from BGM.`n$gdbStdout"
    }
    if (-not $memory.Success -or [uint64]$memory.Groups[1].Value -lt 131072) {
        throw "FGM runtime left less than 128 KiB arena headroom.`n$gdbStdout"
    }

    Write-Output (
        ('Audio FGM natural phase/regular-KO pack passed: pack={0} bytes ' +
         'peak_min={1} ' +
         'rms_min={2:N3} plays={3}+{4}unsupported phase=0x1f ' +
         'channels={5} bgm_channel={6} max_live={7} envelope_steps={8} ' +
         'headroom={9} text/rodata/data/bss/itcm={10}/{11}/{12}/{13}/{14}.') -f
        $metadata.resident_bytes, $peakMin, $rmsMin,
        $play.Groups[2].Value, $play.Groups[3].Value,
        $pool.Groups[8].Value, $bgmChannel, $pool.Groups[7].Value,
        $play.Groups[8].Value, $memory.Groups[1].Value,
        $sectionTotals.text, $sectionTotals.rodata, $sectionTotals.data,
        $sectionTotals.bss, $sectionTotals.itcm)
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
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
}
