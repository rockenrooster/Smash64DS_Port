[CmdletBinding()]
param(
    [string]$Build = 'build-fox-blaster-native-proof-lab',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 180,
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
    [string]$EvidenceLabel = '2026-08-08_fox-blaster-native-lab',
    [switch]$Control
)

# Event-driven Fox blaster proof. The canonical level-3 Fox CPU stays enabled;
# GDB runs freely until the source weapon maker fires, then captures after two
# completed beam draws. This avoids the severe observer cost of a per-frame
# breakpoint while retaining a source-owned spawn and an in-flight screenshot.

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
    'artifacts\visibility\' + $EvidenceLabel + '.png')
$artifact = Join-Path $root (
    'artifacts\verification\' + $EvidenceLabel + '.txt')
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir "melonds.$EvidenceLabel.stdout.log"
$stderr = Join-Path $logDir "melonds.$EvidenceLabel.stderr.log"
$configState = $null
$emulator = $null

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}

function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') {
            $values += [int64](Convert-MarkerUInt32 $text)
        } else {
            $values += [int64]$text
        }
    }
    return $values
}

$required = @(
    'scVSBattleStartBattle',
    'battleship_wpFoxBlasterMakeWeapon',
    'wpDisplayMain',
    'gSCManagerBattleState',
    'gNdsBattlePlayableFoxCpuEnabled',
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsFighterProjectileProofSpawnCallCount',
    'gNdsFighterProjectileProofSpawnSuccessCount',
    'gNdsFighterProjectileProofWeaponFrames',
    'gNdsFighterProjectileProofKindMask',
    'gNdsWeaponRendererBlasterSubmitCount',
    'gNdsWeaponRendererBlasterTriangleCount',
    'gNdsWeaponRendererBlasterVisibleDrawCount',
    'gNdsWeaponRendererTextureRejectCount',
    'gNdsObjmanPanicCount',
    'gNdsObjAnimRunawayCount',
    'gNdsAudioFgmPlayFailCount',
    'gNdsAudioFgmGenerationMismatchCount',
    'gNdsAudioFgmStaleStopCount'
)
if (-not $Control) {
    $required += @(
        'ndsRendererSubmitFoxBlasterQuad',
        'gNdsFoxBlasterQuadDrawCount',
        'gNdsFoxBlasterQuadFallbackCount',
        'gNdsFoxBlasterGlowAOTSpawnCount',
        'gNdsFoxBlasterGlowAOTDrawCount',
        'gNdsFoxBlasterGlowAOTFallbackCount',
        'gNdsFoxBlasterGlowAOTMissCount',
        'gNdsRendererFoxBlasterGlowPrepareCount',
        'gNdsRendererFoxBlasterGlowFailCount',
        'gNdsRendererFoxBlasterGlowBytes'
    )
} else {
    $required += @(
        'ndsRendererHardwareBeginTriangleBatch',
        'ndsRendererHardwarePackedVertexColor'
    )
}
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Fox blaster probe symbols absent from {0}: {1}" -f
        $elf, ($missing -join ', '))
}

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr, $shot, $artifact -Force `
        -ErrorAction SilentlyContinue
    # The reason below was already written; it just did not carry the token
    # `check-melonds-policy.ps1` looks for, so that checker was RED and
    # therefore protecting every other harness from nothing.
    # WindowStyle: visible-by-design
    # The capture helper needs a real top-level window handle. Hidden, melonDS
    # still runs the proof but MainWindowHandle stays IntPtr.Zero and the
    # screenshot silently dies -- a black PNG, not an error.
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Normal `
        -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator -Port $context.GdbPort | Out-Null

    $captureCommand =
        'shell pwsh.exe -NoProfile -File "{0}" -EmulatorProcessId {1} -Output "{2}" 2>&1' -f
        $captureHelper.Replace('\', '/'), $emulator.Id, $shot.Replace('\', '/')
    $engagementCounter = if ($Control) {
        'gNdsWeaponRendererBlasterSubmitCount'
    } else {
        'gNdsFoxBlasterQuadDrawCount'
    }
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'tbreak scVSBattleStartBattle',
        'continue',
        'set variable gNdsBattlePlayableFoxCpuEnabled = 1',
        'tbreak battleship_wpFoxBlasterMakeWeapon',
        'continue',
        'set $blaster_spawn_frame = gNdsBattlePlayablePacingPresentedFrames',
        'finish',
        'set $blaster_weapon = (GObj *)$r0',
        'if $blaster_weapon == 0',
        'printf "FOX_BLASTER_NULL=1\n"',
        'quit 86',
        'end',
        $(if ($Control) {
            ('break wpDisplayMain if (' + $engagementCounter + ' >= 2)')
        } else {
            ('break ndsRendererSubmitFoxBlasterQuad if (' +
                $engagementCounter + ' >= 2)')
        }),
        'continue'
    )
    if ($Control) {
        # The first triangle batch reached from this exact weapon display is
        # the blaster quad. Record its DS polygon format before the screenshot
        # so visual deltas can be assigned to GX state rather than geometry.
        $commands += @(
            'tbreak ndsRendererHardwareBeginTriangleBatch',
            'continue',
            ('printf "FOX_BLASTER_CONTROL_POLY=%#x,%u\n", ' +
                '(unsigned int)$r3, ((unsigned int)$r3 >> 16) & 31'),
            'finish',
            'tbreak ndsRendererHardwarePackedVertexColor',
            'continue',
            'finish',
            ('printf "FOX_BLASTER_CONTROL_COLOR=%#x\n", ' +
                '(unsigned int)$r0 & 0xffff')
        )
    }
    $commands += @(
        'printf "FOX_BLASTER_SPAWN_FRAME=%u\n", $blaster_spawn_frame',
        ('printf "CPU_CONFIG=%u,%u,%u,%u,%u,%u\n", ' +
            'gSCManagerBattleState->players[0].pkind, ' +
            'gSCManagerBattleState->players[1].pkind, ' +
            'gSCManagerBattleState->players[1].level, ' +
            'gSCManagerBattleState->pl_count, ' +
            'gSCManagerBattleState->cp_count, ' +
            'gNdsBattlePlayableFoxCpuEnabled'),
        ('printf "FOX_BLASTER_SOURCE=%u,%u,%u,%#x\n", ' +
            'gNdsFighterProjectileProofSpawnCallCount, ' +
            'gNdsFighterProjectileProofSpawnSuccessCount, ' +
            'gNdsFighterProjectileProofWeaponFrames, ' +
            'gNdsFighterProjectileProofKindMask'),
        ('printf "FOX_BLASTER_GENERIC=%u,%u,%u,%u\n", ' +
            'gNdsWeaponRendererBlasterSubmitCount, ' +
            'gNdsWeaponRendererBlasterTriangleCount, ' +
            'gNdsWeaponRendererBlasterVisibleDrawCount, ' +
            'gNdsWeaponRendererTextureRejectCount')
    )
    if (-not $Control) {
        $commands += @(
            ('printf "FOX_BLASTER_NATIVE=%u,%u\n", ' +
                'gNdsFoxBlasterQuadDrawCount, ' +
                'gNdsFoxBlasterQuadFallbackCount'),
            ('printf "FOX_BLASTER_GLOW=%u,%u,%u,%u,%u,%u,%u\n", ' +
                'gNdsFoxBlasterGlowAOTSpawnCount, ' +
                'gNdsFoxBlasterGlowAOTDrawCount, ' +
                'gNdsFoxBlasterGlowAOTFallbackCount, ' +
                'gNdsFoxBlasterGlowAOTMissCount, ' +
                'gNdsRendererFoxBlasterGlowPrepareCount, ' +
                'gNdsRendererFoxBlasterGlowFailCount, ' +
                'gNdsRendererFoxBlasterGlowBytes')
        )
    }
    $commands += @(
        ('printf "FOX_BLASTER_CAPTURE_FRAME=%u\n", ' +
            'gNdsBattlePlayablePacingPresentedFrames'),
        ('printf "FOX_BLASTER_SAFETY=%u,%u,%u,%u,%u\n", ' +
            'gNdsObjmanPanicCount, gNdsObjAnimRunawayCount, ' +
            'gNdsAudioFgmPlayFailCount, ' +
            'gNdsAudioFgmGenerationMismatchCount, ' +
            'gNdsAudioFgmStaleStopCount'),
        $captureCommand,
        'detach',
        'quit'
    )
    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName "fox_blaster_$EvidenceLabel.gdb" `
        -TimeoutSeconds $TimeoutSeconds
    $gdbStdout = $capture.Stdout
    [System.IO.Directory]::CreateDirectory(
        (Split-Path -Parent $artifact)) | Out-Null
    [System.IO.File]::WriteAllText(
        $artifact, $gdbStdout, [System.Text.Encoding]::UTF8)

    $cpuMatch = [regex]::Match(
        $gdbStdout, 'CPU_CONFIG=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $sourceMatch = [regex]::Match(
        $gdbStdout, 'FOX_BLASTER_SOURCE=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $genericMatch = [regex]::Match(
        $gdbStdout, 'FOX_BLASTER_GENERIC=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $captureFrameMatch = [regex]::Match(
        $gdbStdout, 'FOX_BLASTER_CAPTURE_FRAME=([0-9]+)')
    $safetyMatch = [regex]::Match(
        $gdbStdout, 'FOX_BLASTER_SAFETY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $cpu = Get-Ints $cpuMatch
    $source = Get-Ints $sourceMatch
    $generic = Get-Ints $genericMatch
    $captureFrame = Get-Ints $captureFrameMatch
    $safety = Get-Ints $safetyMatch
    Assert-Condition ($cpuMatch.Success -and $cpu[1] -eq 1 -and
        $cpu[2] -eq 3 -and $cpu[3] -eq 1 -and $cpu[4] -eq 1 -and
        $cpu[5] -eq 1) 'Fox was not the enabled level-3 CPU.' $gdbStdout
    # The maker breakpoint plus matching spawn call/success counters is the
    # ownership proof. WeaponFrames/kindMask belong to the bounded scripted
    # projectile phase and intentionally remain zero under the free CPU path.
    Assert-Condition ($sourceMatch.Success -and $source[0] -gt 0 -and
        $source[1] -gt 0) `
        'The enabled Fox CPU did not create a source blaster.' $gdbStdout
    Assert-Condition ($genericMatch.Success -and $generic[3] -eq 0) `
        'Fox blaster rendering rejected a texture or display contract.' `
        $gdbStdout
    Assert-Condition ($captureFrameMatch.Success) `
        'Fox blaster capture frame was not recorded.' $gdbStdout
    Assert-Condition ($safetyMatch.Success -and
        (($safety | Measure-Object -Sum).Sum -eq 0)) `
        'Fox blaster proof observed object, animation, or audio corruption.' `
        $gdbStdout
    if (-not $Control) {
        $nativeMatch = [regex]::Match(
            $gdbStdout, 'FOX_BLASTER_NATIVE=([0-9]+),([0-9]+)')
        $glowMatch = [regex]::Match(
            $gdbStdout, 'FOX_BLASTER_GLOW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
        $native = Get-Ints $nativeMatch
        $glow = Get-Ints $glowMatch
        Assert-Condition ($nativeMatch.Success -and $native[0] -ge 2 -and
            $native[1] -eq 0 -and $generic[0] -eq 0 -and
            $generic[1] -eq 0 -and $generic[2] -eq 0) `
            'The natural Fox shot missed the direct quad or used fallback.' `
            $gdbStdout
        Assert-Condition ($glowMatch.Success -and $glow[0] -ge 1 -and
            $glow[1] -ge 1 -and $glow[2] -eq 0 -and $glow[3] -eq 0 -and
            $glow[4] -eq 1 -and $glow[5] -eq 0 -and $glow[6] -eq 92) `
            'The Fox glow missed its exact PAL16 hardware-mirror path.' `
            $gdbStdout
    } else {
        Assert-Condition ($generic[0] -ge 2 -and
            $generic[1] -eq (2 * $generic[0]) -and
            $generic[2] -eq $generic[0]) `
            'The source-display control did not submit visible blaster quads.' `
            $gdbStdout
    }
    Assert-Condition ((Test-Path -LiteralPath $shot -PathType Leaf) -and
        ((Get-Item -LiteralPath $shot).Length -gt 1024)) `
        "Fox blaster screenshot capture failed: $shot" $gdbStdout
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $shot -MinDifferentFraction 0.01 `
        -MinDominantGreenFraction 0.03 `
        -MinNonWhiteNonGreenFraction 0.20 `
        -WindowScaledCapture
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Output (("Fox blaster {0} proof passed: CPU=level{1} " +
        "source={2}/{3} frames={4} render={5}/{6}/{7} captureFrame={8} " +
        "capture={9} evidence={10}") -f
        $(if ($Control) { 'control' } else { 'native' }), $cpu[2],
        $source[0], $source[1], $source[2], $generic[0], $generic[1],
        $generic[2], $captureFrame[0], $shot, $artifact)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item -LiteralPath $stdout, $stderr -Force `
        -ErrorAction SilentlyContinue
}
