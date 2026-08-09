[CmdletBinding()]
param(
    [string]$Build = 'build-whispy-native-textures-lab',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 300,
    [ValidateRange(20, 1200)][int]$CaptureFrame = 100,
    [ValidateRange(0, 7)][int]$Route = 7,
    [ValidateRange(0, 1)][int]$ExpectedFoxCpu = 0,
    [ValidateRange(0, 1200)][int]$TraceFrames = 0,
    [string]$Label = 'native-textures',
    [switch]$Native,
    [switch]$AOT,
    [switch]$NaturalWindWait,
    [switch]$NoScreenshot
)

# Focused Whispy capture. By default the normal non-CPU boundary's first
# wind wait is shortened to four frames. -NaturalWindWait leaves that source
# field untouched for a measurement aligned to an unmodified gameplay capture.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.elf'
$captureHelper = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$shot = Join-Path $root (
    'artifacts\visibility\2026-08-08_whispy-' + $Label + '.png')
$artifact = Join-Path $root (
    'artifacts\verification\whispy-' + $Label + '.txt')
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir "melonds.whispy-$Label.stdout.log"
$stderr = Join-Path $logDir "melonds.whispy-$Label.stderr.log"
$configState = $null
$emulator = $null

$required = @(
    'ndsBattlePlayableFrameCompleteMarker',
    'gGRCommonStruct',
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsParticleQuadEmitCount',
    'gNdsParticleQuadMissCount'
)
if ($TraceFrames -gt 0) {
    $required += @(
        'gNdsTickHudBuckets',
        'gNdsWhispyAOTGeneratorTicks',
        'gNdsWhispyAOTStructTicks',
        'gNdsMiscParticleDrawTicks'
    )
}
if ($Native) {
    $required += @(
        'gNdsWhispyNativeTextureDrawCount',
        'gNdsWhispyNativeTextureMissCount',
        'gNdsWhispyNativeTextureMask',
        'gNdsWhispyNativeSourceFrameMask',
        'gNdsRendererWhispyNativePrepareCount',
        'gNdsRendererWhispyNativeFailCount',
        'gNdsRendererWhispyNativeBytes',
        'gNdsParticleStructsLive',
        'gNdsParticleGeneratorsLive',
        'gNdsParticleTransformsLive',
        'gNdsParticleStructsMax',
        'gNdsParticleGeneratorsMax',
        'gNdsParticleTransformsMax',
        'gNdsObjmanPanicCount',
        'gNdsObjAnimRunawayCount',
        'gNdsAudioFgmPlayFailCount',
        'gNdsAudioFgmPrematureRetireCount',
        'gNdsAudioFgmGenerationMismatchCount',
        'gNdsAudioFgmStaleStopCount',
        'gNdsBattlePlayableFoxCpuEnabled'
    )
}
if ($AOT) {
    $required += @(
        'gNdsWhispyAOTGeneratorFastRuns',
        'gNdsWhispyAOTGeneratorFallbackRuns',
        'gNdsWhispyAOTGeneratorVisits',
        'gNdsWhispyAOTGeneratorEmits',
        'gNdsWhispyAOTTableTrigPairs',
        'gNdsWhispyAOTStructVisits',
        'gNdsWhispyAOTStructFastUpdates',
        'gNdsWhispyAOTStructSourceUpdates',
        'gNdsWhispyAOTDividesAvoided',
        'gNdsWhispyAOTRigidDraws',
        'gNdsWhispyAOTRigidDrawFallbacks',
        'gNdsWhispyAOTRoute',
        'gNdsWhispyAOTTier2GeneratorMatches',
        'gNdsWhispyAOTTier2DirectUpdates',
        'gNdsWhispyAOTTier2FixedTransforms',
        'gNdsWhispyAOTTier2FixedSubmits',
        'gNdsWhispyAOTTier2FixedFallbacks',
        'gNdsWhispyAOTTier3FastBinds',
        'gNdsWhispyAOTTier3BindFallbacks',
        'gNdsWhispyAOTTier4PacketQuads',
        'gNdsWhispyAOTTier4PacketStateGroups',
        'gNdsWhispyAOTTier4PacketFlushes',
        'gNdsWhispyAOTTier4PacketWords',
        'gNdsWhispyAOTTier4PacketFallbacks',
        'sNdsRendererWhispyNativeBinding',
        'gNdsParticleQuadAlphaBreaks',
        'gNdsParticleQuadSheetBreaks',
        'gNdsParticleScaleEscalations',
        'gNdsParticleWorldClampCount'
    )
}
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Whispy probe symbols absent from {0}: {1}" -f
        $elf, ($missing -join ', '))
}

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force `
        -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort)
    )
    $commands += @(
        'set $frames = 0',
        'set $shots = 0',
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        'set $frames = $frames + 1',
        'if $frames == 1'
    )
    if (-not $NaturalWindWait) {
        $commands += 'set var gGRCommonStruct.pupupu.whispy_wind_wait = 4'
    }
    if ($AOT) {
        # Set the writable route selector at the first completed battle frame
        # so every tier can be measured from one ROM; the lab's initialized
        # default remains the most optimized route 7.
        $commands += ("set var gNdsWhispyAOTRoute = {0}" -f $Route)
    }
    $commands += @(
        'end',
        ("if (`$frames == $CaptureFrame) && (`$shots == 0)"),
        'set $shots = 1'
    )
    $commands += @('end')
    if ($AOT -and ($TraceFrames -gt 0)) {
        $traceStart = $CaptureFrame - $TraceFrames + 1
        $commands += @(
            ("if (`$frames >= $traceStart) && (`$frames <= $CaptureFrame)"),
            ('printf "WHISPY_TRACE frame=%u misc=%u update=%u generator=%u work_h=%u misc_bucket=%u all=%u ftr=%u stg=%u bg=%u aud=%u hud=%u src=%u other=%u wait=%u work=%u draws=%u direct=%u ' +
                'fixed=%u fast_bind=%u packet_quads=%u packet_flush=%u ' +
                'packet_words=%u alpha=%u sheet=%u\n", $frames, ' +
                'gNdsMiscParticleDrawTicks, ' +
                'gNdsWhispyAOTStructTicks, ' +
                'gNdsWhispyAOTGeneratorTicks, ' +
                '(gNdsTickHudBuckets[10] - gNdsTickHudBuckets[5]), ' +
                'gNdsTickHudBuckets[7], ' +
                'gNdsTickHudBuckets[0], ' +
                'gNdsTickHudBuckets[1], ' +
                'gNdsTickHudBuckets[2], ' +
                'gNdsTickHudBuckets[3], ' +
                'gNdsTickHudBuckets[4], ' +
                'gNdsTickHudBuckets[5], ' +
                'gNdsTickHudBuckets[6], ' +
                'gNdsTickHudBuckets[8], ' +
                'gNdsTickHudBuckets[9], ' +
                'gNdsTickHudBuckets[10], ' +
                'gNdsWhispyNativeTextureDrawCount, ' +
                'gNdsWhispyAOTTier2DirectUpdates, ' +
                'gNdsWhispyAOTTier2FixedSubmits, ' +
                'gNdsWhispyAOTTier3FastBinds, ' +
                'gNdsWhispyAOTTier4PacketQuads, ' +
                'gNdsWhispyAOTTier4PacketFlushes, ' +
                'gNdsWhispyAOTTier4PacketWords, ' +
                'gNdsParticleQuadAlphaBreaks, ' +
                'gNdsParticleQuadSheetBreaks'),
            'end'
        )
    }
    $commands += @(
        'set $done = 0',
        ("if `$frames > $($CaptureFrame + 2)"),
        'set $done = 1',
        'end',
        'if $done == 0',
        'continue',
        'end',
        'end',
        'continue'
    )
    if ($Native) {
        $commands += @(
            ('printf "WHISPY_NATIVE frames=%u shots=%u draws=%u miss=%u ' +
                'textures=%#x source_frames=%#x prepare=%u fail=%u bytes=%u ' +
                'quad_emit=%u quad_miss=%u live=%u/%u/%u max=%u/%u/%u ' +
                'panic=%u runaway=%u audio_fail=%u audio_early=%u ' +
                'audio_gen=%u audio_stale=%u cpu=%u\n", ' +
                '$frames, $shots, gNdsWhispyNativeTextureDrawCount, ' +
                'gNdsWhispyNativeTextureMissCount, ' +
                'gNdsWhispyNativeTextureMask, ' +
                'gNdsWhispyNativeSourceFrameMask, ' +
                'gNdsRendererWhispyNativePrepareCount, ' +
                'gNdsRendererWhispyNativeFailCount, ' +
                'gNdsRendererWhispyNativeBytes, gNdsParticleQuadEmitCount, ' +
                'gNdsParticleQuadMissCount, gNdsParticleStructsLive, ' +
                'gNdsParticleGeneratorsLive, gNdsParticleTransformsLive, ' +
                'gNdsParticleStructsMax, gNdsParticleGeneratorsMax, ' +
                'gNdsParticleTransformsMax, gNdsObjmanPanicCount, ' +
                'gNdsObjAnimRunawayCount, gNdsAudioFgmPlayFailCount, ' +
                'gNdsAudioFgmPrematureRetireCount, ' +
                'gNdsAudioFgmGenerationMismatchCount, ' +
                'gNdsAudioFgmStaleStopCount, ' +
                'gNdsBattlePlayableFoxCpuEnabled')
        )
    } else {
        $commands += @(
            ('printf "WHISPY_CONTROL frames=%u shots=%u quad_emit=%u ' +
                'quad_miss=%u\n", $frames, $shots, ' +
                'gNdsParticleQuadEmitCount, gNdsParticleQuadMissCount')
        )
    }
    if ($AOT) {
        $commands += @(
            ('printf "WHISPY_AOT gen_fast=%u gen_fallback=%u gen_visits=%u ' +
                'emits=%u trig_pairs=%u struct_visits=%u struct_fast=%u ' +
                'struct_source=%u divides_avoided=%u rigid=%u ' +
                'rigid_fallback=%u route=%u tier2_matches=%u ' +
                'tier2_direct=%u tier2_xform=%u tier2_submit=%u ' +
                'tier2_fallback=%u fast_bind=%u bind_fallback=%u ' +
                'packet_quads=%u packet_state=%u packet_flush=%u ' +
                'packet_words=%u packet_fallback=%u\n", ' +
                'gNdsWhispyAOTGeneratorFastRuns, ' +
                'gNdsWhispyAOTGeneratorFallbackRuns, ' +
                'gNdsWhispyAOTGeneratorVisits, ' +
                'gNdsWhispyAOTGeneratorEmits, ' +
                'gNdsWhispyAOTTableTrigPairs, ' +
                'gNdsWhispyAOTStructVisits, ' +
                'gNdsWhispyAOTStructFastUpdates, ' +
                'gNdsWhispyAOTStructSourceUpdates, ' +
                'gNdsWhispyAOTDividesAvoided, ' +
                'gNdsWhispyAOTRigidDraws, ' +
                'gNdsWhispyAOTRigidDrawFallbacks, ' +
                'gNdsWhispyAOTRoute, ' +
                'gNdsWhispyAOTTier2GeneratorMatches, ' +
                'gNdsWhispyAOTTier2DirectUpdates, ' +
                'gNdsWhispyAOTTier2FixedTransforms, ' +
                'gNdsWhispyAOTTier2FixedSubmits, ' +
                'gNdsWhispyAOTTier2FixedFallbacks, ' +
                'gNdsWhispyAOTTier3FastBinds, ' +
                'gNdsWhispyAOTTier3BindFallbacks, ' +
                'gNdsWhispyAOTTier4PacketQuads, ' +
                'gNdsWhispyAOTTier4PacketStateGroups, ' +
                'gNdsWhispyAOTTier4PacketFlushes, ' +
                'gNdsWhispyAOTTier4PacketWords, ' +
                'gNdsWhispyAOTTier4PacketFallbacks'),
            ('printf "WHISPY_BIND palettes=%#x/%#x/%#x valid=%u/%u/%u\n", ' +
                'sNdsRendererWhispyNativeBinding[0].palette_format, ' +
                'sNdsRendererWhispyNativeBinding[1].palette_format, ' +
                'sNdsRendererWhispyNativeBinding[2].palette_format, ' +
                'sNdsRendererWhispyNativeBinding[0].valid, ' +
                'sNdsRendererWhispyNativeBinding[1].valid, ' +
                'sNdsRendererWhispyNativeBinding[2].valid')
        )
        if ($TraceFrames -gt 0) {
            $commands += @(
            ('printf "WHISPY_DRAW misc_ticks=%u alpha_breaks=%u ' +
                'sheet_breaks=%u scale_escalations=%u clamps=%u\n", ' +
                'gNdsMiscParticleDrawTicks, gNdsParticleQuadAlphaBreaks, ' +
                'gNdsParticleQuadSheetBreaks, ' +
                'gNdsParticleScaleEscalations, ' +
                'gNdsParticleWorldClampCount'),
            ('printf "WHISPY_UPDATE ticks=%u\n", ' +
                'gNdsWhispyAOTStructTicks'),
            ('printf "WHISPY_GENERATOR ticks=%u\n", ' +
                'gNdsWhispyAOTGeneratorTicks')
            )
        }
    }
    $commands += @('detach', 'quit')

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName "whispy_$Label.gdb" `
        -TimeoutSeconds $TimeoutSeconds
    if (-not $NoScreenshot) {
        # Capture after GDB detaches and the GUI thread is live. Calling the
        # Win32 window capture as a `shell` command while melonDS was stopped
        # at the frame breakpoint deadlocked the GUI and consumed the entire
        # five-minute probe timeout. The particles live for 64-100 frames, so
        # the few resumed frames here remain inside the forced wind event.
        & $captureHelper -EmulatorProcessId $emulator.Id -Output $shot
    }
    if ((-not $NoScreenshot) -and
        ((-not (Test-Path -LiteralPath $shot -PathType Leaf)) -or
         ((Get-Item -LiteralPath $shot).Length -le 1024))) {
        throw "Whispy screenshot capture failed: $shot"
    }
    $nativeContract =
        'WHISPY_NATIVE .*draws=[1-9][0-9]* miss=0 .*textures=0x7 ' +
        'source_frames=0xf .*prepare=1 fail=0 bytes=1520 .*quad_miss=0 ' +
        '.*panic=0 runaway=0 audio_fail=0 .*audio_gen=0 audio_stale=0 ' +
        ("cpu={0}" -f $ExpectedFoxCpu)
    if ($Native -and ($capture.Stdout -notmatch $nativeContract)) {
        throw "Whispy native contract failed:`n$($capture.Stdout)"
    }
    if ($AOT) {
        $aotPattern = if ($Route -ge 5) {
            'WHISPY_AOT gen_fast=[1-9][0-9]* .*emits=[1-9][0-9]* ' +
            'trig_pairs=[1-9][0-9]* .*struct_fast=[1-9][0-9]* ' +
            'struct_source=0 divides_avoided=[1-9][0-9]* .*' +
            ("rigid_fallback=0 route={0} " -f $Route) +
            'tier2_matches=[1-9][0-9]* tier2_direct=[1-9][0-9]* ' +
            'tier2_xform=[1-9][0-9]* tier2_submit=[1-9][0-9]* ' +
            'tier2_fallback=0 fast_bind=[1-9][0-9]* bind_fallback=0 ' +
            'packet_quads=[1-9][0-9]* packet_state=[1-9][0-9]* ' +
            'packet_flush=[1-9][0-9]* packet_words=[1-9][0-9]* ' +
            'packet_fallback=0'
        } elseif ($Route -eq 4) {
            'WHISPY_AOT gen_fast=[1-9][0-9]* .*emits=[1-9][0-9]* ' +
            'trig_pairs=[1-9][0-9]* .*struct_fast=[1-9][0-9]* .*' +
            'divides_avoided=[1-9][0-9]* .*rigid_fallback=0 route=4 ' +
            'tier2_matches=[1-9][0-9]* tier2_direct=[1-9][0-9]* ' +
            'tier2_xform=[1-9][0-9]* tier2_submit=[1-9][0-9]* ' +
            'tier2_fallback=0 fast_bind=[1-9][0-9]* bind_fallback=0 ' +
            'packet_quads=[1-9][0-9]* packet_state=[1-9][0-9]* ' +
            'packet_flush=[1-9][0-9]* packet_words=[1-9][0-9]* ' +
            'packet_fallback=0'
        } elseif ($Route -eq 3) {
            'WHISPY_AOT gen_fast=[1-9][0-9]* .*emits=[1-9][0-9]* ' +
            'trig_pairs=[1-9][0-9]* .*struct_fast=[1-9][0-9]* .*' +
            'divides_avoided=[1-9][0-9]* .*rigid_fallback=0 route=3 ' +
            'tier2_matches=[1-9][0-9]* tier2_direct=[1-9][0-9]* ' +
            'tier2_xform=[1-9][0-9]* tier2_submit=[1-9][0-9]* ' +
            'tier2_fallback=0 fast_bind=[1-9][0-9]* bind_fallback=0 ' +
            'packet_quads=0 packet_state=0 packet_flush=0 packet_words=0 ' +
            'packet_fallback=0'
        } elseif ($Route -eq 2) {
            'WHISPY_AOT gen_fast=[1-9][0-9]* .*emits=[1-9][0-9]* ' +
            'trig_pairs=[1-9][0-9]* .*struct_fast=[1-9][0-9]* .*' +
            'divides_avoided=[1-9][0-9]* .*rigid_fallback=0 route=2 ' +
            'tier2_matches=[1-9][0-9]* tier2_direct=[1-9][0-9]* ' +
            'tier2_xform=[1-9][0-9]* tier2_submit=[1-9][0-9]* ' +
            'tier2_fallback=0 fast_bind=0 bind_fallback=0 ' +
            'packet_quads=0 packet_state=0 packet_flush=0 packet_words=0 ' +
            'packet_fallback=0'
        } elseif ($Route -eq 1) {
            'WHISPY_AOT gen_fast=[1-9][0-9]* .*emits=[1-9][0-9]* ' +
            'trig_pairs=[1-9][0-9]* .*struct_fast=[1-9][0-9]* .*' +
            'divides_avoided=[1-9][0-9]* rigid=[1-9][0-9]* ' +
            'rigid_fallback=0 route=1 tier2_matches=0 tier2_direct=0 ' +
            'tier2_xform=0 tier2_submit=0 tier2_fallback=0 fast_bind=0 ' +
            'bind_fallback=0 ' +
            'packet_quads=0 packet_state=0 packet_flush=0 ' +
            'packet_words=0 packet_fallback=0'
        } else {
            'WHISPY_AOT gen_fast=0 .*struct_fast=0 .*rigid=0 ' +
            'rigid_fallback=0 route=0 tier2_matches=0 tier2_direct=0 ' +
            'tier2_xform=0 tier2_submit=0 tier2_fallback=0 fast_bind=0 ' +
            'bind_fallback=0 ' +
            'packet_quads=0 packet_state=0 packet_flush=0 ' +
            'packet_words=0 packet_fallback=0'
        }
        if ($capture.Stdout -notmatch $aotPattern) {
            throw "Whispy AOT engagement contract failed:`n$($capture.Stdout)"
        }
        if ($Route -ge 5) {
            $fullAot = [regex]::Match(
                $capture.Stdout,
                'struct_visits=([0-9]+) struct_fast=([0-9]+) ' +
                'struct_source=([0-9]+).*tier2_direct=([0-9]+)')
            if ((-not $fullAot.Success) -or
                ([uint64]$fullAot.Groups[1].Value -eq 0) -or
                ([uint64]$fullAot.Groups[1].Value -ne
                 [uint64]$fullAot.Groups[2].Value) -or
                ([uint64]$fullAot.Groups[3].Value -ne 0) -or
                ([uint64]$fullAot.Groups[1].Value -ne
                 [uint64]$fullAot.Groups[4].Value)) {
                throw ("Whispy full-AOT route did not close every post-construction " +
                    "update:`n$($capture.Stdout)")
            }
        }
        # All three source textures own different palettes. Equal cached base
        # words mean the optimized binder would silently recolor one texture
        # with another texture's palette even though every draw counter passes.
        $bindingMatch = [regex]::Match(
            $capture.Stdout,
            'WHISPY_BIND palettes=(0x[0-9a-fA-F]+)/' +
            '(0x[0-9a-fA-F]+)/(0x[0-9a-fA-F]+) valid=1/1/1')
        if (-not $bindingMatch.Success) {
            throw "Whispy native palette binding trace failed:`n$($capture.Stdout)"
        }
        $palette0 = [Convert]::ToUInt32(
            $bindingMatch.Groups[1].Value.Substring(2), 16)
        $palette1 = [Convert]::ToUInt32(
            $bindingMatch.Groups[2].Value.Substring(2), 16)
        $palette2 = [Convert]::ToUInt32(
            $bindingMatch.Groups[3].Value.Substring(2), 16)
        if (($palette0 -eq $palette1) -or
            ($palette0 -eq $palette2) -or
            ($palette1 -eq $palette2)) {
            throw ("Whispy native palettes alias: {0}/{1}/{2}" -f
                $bindingMatch.Groups[1].Value,
                $bindingMatch.Groups[2].Value,
                $bindingMatch.Groups[3].Value)
        }
    }
    New-Item -ItemType Directory -Force `
        -Path (Split-Path -Parent $artifact) | Out-Null
    Set-Content -LiteralPath $artifact -Value $capture.Stdout
    $capture.Stdout | Select-String `
        -Pattern 'WHISPY_(?:NATIVE|CONTROL|AOT|DRAW|UPDATE|GENERATOR|BIND|TRACE)'
    Write-Output "probe capture: $artifact"
    if (-not $NoScreenshot) {
        Write-Output "screenshot:    $shot"
    }
}
finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
            $emulator.WaitForExit()
        }
    }
    if ($null -ne $configState) {
        Restore-MelonDSGdbConfig -State $configState
    }
}
