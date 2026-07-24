[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4614,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task52-e0-baseline',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [switch]$NoBuild,
    [int]$StartFrame = 438,
    [int]$EndFrame = 445,
    [switch]$ArenaGate,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 600
)

# E0 proof (Task 52): confirms profile-0 Dream Land steady state draws through the
# Task 36 replay path (ndsRendererTask36ReplayRun), not the compose/fallback path.
#
# IMPORTANT: the gNdsRendererTask36Replay* counter globals are compile-gated to
# NDS_RENDERER_PROFILE_LEVEL==1, and the profile-1 builds use a different taskman
# arena layout that REJECTS the replay buffer (state=DISABLED, arena_reject=1) --
# so a profile-1 build is NOT representative of the shipping profile-0 replay
# behavior. Instead this reads the always-present internal owner struct
# sNdsRendererTask36ReplayOwner (gated only on NDS_TASK36_HW_COMPOSE==2, which the
# tick-HUD target sets) directly from the profile-0 tick-HUD ROM by field name,
# which GDB resolves from the ELF debug info regardless of the counter gating.
# That is the ground truth for "is replay active in the shipping program."
#
# This mirrors scripts/sample-tick-hud-buckets.ps1's GDB/emulator plumbing.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = $Target

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'task52-replay-active.gdb'
$gdbOut = Join-Path $temp 'task52-replay-active.gdb.out'
$gdbErr = Join-Path $temp 'task52-replay-active.gdb.err'
$emulatorOut = Join-Path $temp 'task52-replay-active.melonds.out'
$emulatorErr = Join-Path $temp 'task52-replay-active.melonds.err'

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required file is missing: $path"
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

    # REPLAY_PROBE columns (read from the always-present internal owner struct,
    # NOT the profile-1-only gNds* counters -- see header comment):
    # frame, owner.state, owner.word_count, owner.frame_capture, owner.frame_replay,
    # owner.capture_fault, owner.captured_segment_mask, owner.topology_generation,
    # owner.current_run, owner.command_slot
    # state enum: 0=UNSEEDED 1=CAPTURING 2=READY 3=DISABLED
    # frame_replay==1 means THIS frame drew through the replay path.
    if ($ArenaGate) {
        # Diagnose WHICH gate disabled replay: arena size vs rigid-binding mask.
        # ARENA_GATE columns: frame, arena_chosen_size, arena_alloc_fail_count,
        # owner.state, frame.rigid_binding_mask (lo), RIGID_BINDING_MASK constant (lo)
        # The replay admission guard (nds_renderer.c:4184/4195) requires
        # arena_chosen_size==0x150000 (and zero alloc fails) AND the frame's
        # rigid_binding_mask to equal NDS_RENDERER_TASK36_RIGID_BINDING_MASK.
        $probePrintf = ('printf "ARENA_GATE=%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
            'gNdsBattlePlayablePacingPresentedFrames, ' +
            '(unsigned int)gNdsTaskmanArenaChosenSize, ' +
            '(unsigned int)gNdsTaskmanArenaAllocFailCount, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.state, ' +
            '(unsigned int)(sNdsRendererAdapterNativeStageWorkspace.frame.rigid_binding_mask & 0xFFFFFFFFu), ' +
            '0xc00fffffu, ' +
            '(unsigned int)((sNdsRendererAdapterNativeStageWorkspace.frame.rigid_binding_mask >> 32) & 0xFFFFFFFFu), ' +
            '0x00000381u')
    } else {
        $probePrintf = ('printf "REPLAY_PROBE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
            'gNdsBattlePlayablePacingPresentedFrames, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.state, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.word_count, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.frame_capture, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.frame_replay, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.capture_fault, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.captured_segment_mask, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.topology_generation, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.current_run, ' +
            '(unsigned int)sNdsRendererTask36ReplayOwner.command_slot')
    }
    [System.IO.File]::WriteAllLines($gdbScript, @(
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
        "if gNdsBattlePlayablePacingPresentedFrames > $EndFrame",
        'detach',
        'end',
        $probePrintf,
        'continue',
        'end',
        'continue'))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "GDB run exceeded ${TimeoutSeconds}s before frame $EndFrame."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        throw "GDB run failed: $(Get-Content $gdbErr -Raw)"
    }

    $output = Get-Content $gdbOut -Raw
    Write-Host "Task 52 E0 replay-active probe: target=$target build=$Build frames=$StartFrame..$EndFrame"
    Write-Host $output
}
finally {
    if ($null -ne $emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $configState) {
        Restore-MelonDSGdbConfig -State $configState
    }
}
