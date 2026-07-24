[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4615,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task56-parent',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [switch]$NoBuild,
    [int]$StartFrame = 438,
    [int]$EndFrame = 445,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 600
)

# Task 56 E0 path proof: confirms profile-0 steady-state Mario/Fox fighters draw
# through the NATIVE-FIGHTER PRODUCTION path (ndsRendererExecuteNativeFighterOwnerProduction,
# selected by gNdsRendererFastRunMode == 9 / NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE),
# not the generic Gfx display-list fallback.
#
# The native-fighter production emit code is gated #if NDS_RENDERER_HW_TRIANGLES &&
# (NDS_RENDERER_PROFILE_LEVEL < 2). The published/tick-HUD targets set HW_TRIANGLES=1
# and PROFILE_LEVEL=0, and NDS_RENDERER_FAST_RUN_DEFAULT := 9, so the production path
# IS compiled and IS the selected mode -- UNLESS is_use_animlocks / shuffle_tics force
# the generic fallback (reloc_backend_renderer_dl.c:11579-11585).
#
# This probe reads, at ndsBattlePlayableFrameCompleteMarker:
#   gNdsRendererFastRunMode                  (must be 9 in shipping)
#   sNdsRendererRuntimeFrameSummary fields   (populated at profile < 2; nds_renderer.c:2634)
#     hardware_triangles (+60)               (native emit -> nonzero; generic fallback also
#                                             emits, but cross-referenced with batch_begin)
#     hardware_vertices  (+56)               (== triangles * 3 for GL_TRIANGLES emit)
#     hardware_batch_begin_count (+64)       (one glBegin per state-group via the reuse path)
#     matrix_load_count  (+52)               (one per fighter root)
#     texture_prepare_count (+76)            (texture binds per textured epoch)
#     raw_cross_matrix_count (+104)          (cross-matrix fighter triangles)
#
# A frame with hardware_triangles >> 0 and hardware_vertices ~= 3 * hardware_triangles
# under mode 9 proves the native-fighter production path is the shipping fighter draw.
# If hardware_triangles were ~0 while FTR bucket is 579K, fighters would be going through
# a non-GX path (impossible for visible fighters) -- so nonzero triangles + mode 9 is
# conclusive.
#
# Mirrors scripts/probe-task52-replay-active.ps1's GDB/emulator plumbing.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'task56-fighter-path.gdb'
$gdbOut = Join-Path $temp 'task56-fighter-path.gdb.out'
$gdbErr = Join-Path $temp 'task56-fighter-path.gdb.err'
$emulatorOut = Join-Path $temp 'task56-fighter-path.melonds.out'
$emulatorErr = Join-Path $temp 'task56-fighter-path.melonds.err'
$emulator = $null
$configState = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$Target" "BUILD=$Build" -j16
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

    # FIGHTER_PATH columns: frame, fast_run_mode, hardware_triangles, hardware_vertices,
    # batch_begin, matrix_load, texture_prepare, raw_cross_matrix.
    # fast_run_mode==9 == NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE.
    # hardware_vertices == 3 * hardware_triangles confirms GL_TRIANGLES (not strip) emit.
    $probePrintf = ('printf "FIGHTER_PATH=%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
        'gNdsBattlePlayablePacingPresentedFrames, ' +
        '(unsigned int)gNdsRendererFastRunMode, ' +
        '(unsigned int)sNdsRendererRuntimeFrameSummary.hardware_triangles, ' +
        '(unsigned int)sNdsRendererRuntimeFrameSummary.hardware_vertices, ' +
        '(unsigned int)sNdsRendererRuntimeFrameSummary.hardware_batch_begin_count, ' +
        '(unsigned int)sNdsRendererRuntimeFrameSummary.matrix_load_count, ' +
        '(unsigned int)sNdsRendererRuntimeFrameSummary.texture_prepare_count, ' +
        '(unsigned int)sNdsRendererRuntimeFrameSummary.raw_cross_matrix_count')
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
    Write-Host "Task 56 E0 fighter-path probe: target=$Target build=$Build frames=$StartFrame..$EndFrame"
    Write-Host "columns: frame, fast_run_mode(9=NATIVE_COMPLETE_STAGE), hardware_triangles, hardware_vertices, batch_begin, matrix_load, texture_prepare, raw_cross_matrix"
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
