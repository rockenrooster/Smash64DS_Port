[CmdletBinding()]
param(
    [string]$Build = 'build-row6-v1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(60, 600)][int]$TimeoutSeconds = 300,
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
    [Parameter(Mandatory = $true)][string]$EvidenceLabel
)

# BUGS ROW 6, THE POSITIVE CONTROL AND THE CAPABILITY TEST IN ONE RUN.
#
# ndsRendererAdapterPrepareNativeStageOwner rejects through a label that calls
# ndsRendererHardwareAbortBattleStaticTextures. That discards the entire
# hardware texture cache and clears sNdsRendererBattleStaticTexturePrepared --
# and ndsRendererHardwareArmBattleStaticTextures refuses to re-arm without that
# flag, so nothing re-prepares the 24 pinned statics for the rest of the scene.
# Read from source, that is scene-wide and permanent, which is verbatim what the
# owner reported ("all of the above", and a flat "no" to recovery).
#
# Two forced deaths on 2026-08-04 did not reproduce it, and the owner reproduces
# it in ordinary play that is expensive to replicate. So this probe stops hunting
# the trigger and forces the MECHANISM instead. It answers two questions that do
# not need the natural trigger at all:
#
#   1. POSITIVE CONTROL. gNdsRendererStageOwnerAbortCount and
#      gNdsRendererStaticTexturePreparedNow have never been observed non-zero
#      and non-one respectively, so a zero from them is not yet evidence of
#      anything. After this run it is.
#   2. CAPABILITY. If forcing the abort on a healthy textured frame makes the
#      whole scene lose its textures and never get them back, this mechanism is
#      PROVEN CAPABLE of producing the owner's symptom -- which is not the same
#      claim as "this is the cause", and must not be written up as if it were.
#      If it does NOT, then this seam is not the owner's bug however well it
#      reads, and row 6 reopens at the top.
#
# The lever is a gdb `call` of the abort itself, taken at
# ndsBattlePlayableFrameCompleteMarker so no GX batch is mid-flight. Note the
# risk honestly: ndsRendererHardwareDiscardTextureCache deletes texture names
# through libnds's VRAM block allocator (glDeleteTextures), and this repo's
# standing rule is to keep gdb away from guest allocators because they hang the
# target. The frame boundary is the safest point available, and a hang here is
# itself a reportable result rather than a lost run -- the artifact is written
# from the catch.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'

$artifact = Join-Path $root ("artifacts\verification\" + $EvidenceLabel +
    "_row6-force-abort.txt")
$capture_names = @('before', 'after-1f', 'after-1s', 'after-5s') |
    ForEach-Object { $EvidenceLabel + '_row6-abort-' + $_ + '.png' }
$existing = @($capture_names | ForEach-Object {
        Join-Path $root ('artifacts\visibility\' + $_)
    } | Where-Object { Test-Path -LiteralPath $_ })
if (($existing.Count -ne 0) -or (Test-Path -LiteralPath $artifact)) {
    throw ("EvidenceLabel '$EvidenceLabel' would overwrite: " +
        (@($existing + @($artifact | Where-Object { Test-Path -LiteralPath $_ })) -join ', '))
}

$required = @(
    'ndsBattlePlayableFrameCompleteMarker',
    'ndsRendererHardwareAbortBattleStaticTextures',
    'gNdsRendererStageOwnerRejectCount',
    'gNdsRendererStageOwnerFirstRejectReason',
    'gNdsRendererStageOwnerLastRejectReason',
    'gNdsRendererStageOwnerAbortCount',
    'gNdsRendererStaticTexturePreparedNow',
    'gNdsRendererBattleStaticTextureViolationCount',
    'gNdsRendererBattleStaticTextureTeardownCount',
    'gNdsRendererBattleStaticTexturePinnedHitCount',
    'gNdsRendererBattleStaticTexturePreparedCount',
    'gNdsRendererBattleStaticTextureArmCount',
    'gNdsRendererSceneTextureVramResetCount',
    'gNdsEffectRendererTextureReadyCount',
    'gNdsEffectRendererTextureRejectCount',
    'gNdsFrameCounter', 'gNdsBattleTextHudTimeSeconds'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw "probe symbols absent from $elf : $($missing -join ', ')"
}

# nm, not gdb: the NDS_WEAK gc-sectioned twin gives `Breakpoint at 0x0` with two
# locations and then captures the wrong frame in silence. See probe-ko-vfx.ps1.
function Get-TextAddress([string]$Name) {
    $found = @($nm_lines | Where-Object {
            $_ -match ('^([0-9a-fA-F]{8})\s+[TtWw]\s+' +
                [regex]::Escape($Name) + '$')
        })
    if ($found.Count -ne 1) {
        throw "$Name has $($found.Count) text symbols in $elf (need exactly 1)."
    }
    return '0x' + ($found[0] -split '\s+')[0]
}
$frame_marker = Get-TextAddress 'ndsBattlePlayableFrameCompleteMarker'

$capture_helper = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.row6-force-abort.stdout.log'
$stderr = Join-Path $log_dir 'melonds.row6-force-abort.stderr.log'
$config_state = $null
$emulator = $null

function New-CaptureCommand([string]$Name, [int]$EmulatorProcessId) {
    return ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
        $capture_helper + '" -EmulatorProcessId ' + $EmulatorProcessId +
        ' -Output "artifacts/visibility/' + $EvidenceLabel +
        '_row6-abort-' + $Name + '.png"')
}

function New-StateCommand([string]$Name) {
    return ('printf "ROW6 tag=' + $Name + ' frame=%u clock=%u ' +
        'abort=%u preparednow=%u ownerreject=%u ownerfirst=%u ownerlast=%u ' +
        'viol=%u teardown=%u pinned=%u prepared=%u arm=%u vramreset=%u ' +
        'eready=%u ereject=%u\n", ' +
        'gNdsFrameCounter, gNdsBattleTextHudTimeSeconds, ' +
        'gNdsRendererStageOwnerAbortCount, ' +
        'gNdsRendererStaticTexturePreparedNow, ' +
        'gNdsRendererStageOwnerRejectCount, ' +
        'gNdsRendererStageOwnerFirstRejectReason, ' +
        'gNdsRendererStageOwnerLastRejectReason, ' +
        'gNdsRendererBattleStaticTextureViolationCount, ' +
        'gNdsRendererBattleStaticTextureTeardownCount, ' +
        'gNdsRendererBattleStaticTexturePinnedHitCount, ' +
        'gNdsRendererBattleStaticTexturePreparedCount, ' +
        'gNdsRendererBattleStaticTextureArmCount, ' +
        'gNdsRendererSceneTextureVramResetCount, ' +
        'gNdsEffectRendererTextureReadyCount, ' +
        'gNdsEffectRendererTextureRejectCount')
}

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
        'set remotetimeout 30',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        # A healthy, fully textured mid-match frame. 300 frames is five seconds
        # in, well past scene setup and the opening spawn.
        ('tbreak *' + $frame_marker),
        'ignore $bpnum 300',
        'continue',
        (New-StateCommand 'before'),
        (New-CaptureCommand 'before' $emulator.Id),

        # THE LEVER. Stopped at the frame-complete marker, so no GX batch is in
        # flight when the cache is torn down.
        'call ndsRendererHardwareAbortBattleStaticTextures()',
        (New-StateCommand 'immediately-after-call'),

        # One frame later: is the scene already untextured?
        ('tbreak *' + $frame_marker),
        'ignore $bpnum 1',
        'continue',
        (New-StateCommand 'after-1f'),
        (New-CaptureCommand 'after-1f' $emulator.Id),

        # One second later: has anything re-armed?
        ('tbreak *' + $frame_marker),
        'ignore $bpnum 60',
        'continue',
        (New-StateCommand 'after-1s'),
        (New-CaptureCommand 'after-1s' $emulator.Id),

        # Five seconds later. If the textures are still gone here, "permanent"
        # is measured rather than argued from the source.
        ('tbreak *' + $frame_marker),
        'ignore $bpnum 300',
        'continue',
        (New-StateCommand 'after-5s'),
        (New-CaptureCommand 'after-5s' $emulator.Id),

        'detach',
        'quit'
    )

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifact) | Out-Null
    # The artifact is written from the catch as well: the lever calls into
    # libnds's VRAM allocator, so a hang is a plausible outcome and its
    # transcript is the evidence for it.
    try {
        $capture = Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
            -ScriptName 'row6_force_abort.gdb' `
            -TimeoutSeconds $TimeoutSeconds
        Set-Content -LiteralPath $artifact -Value $capture
        $capture | Select-String -Pattern 'ROW6'
        Write-Output "probe capture: $artifact"
    }
    catch {
        Set-Content -LiteralPath $artifact -Value ($_ | Out-String)
        Write-Output "probe TIMED OUT or failed; partial transcript: $artifact"
        throw
    }
}
finally {
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
