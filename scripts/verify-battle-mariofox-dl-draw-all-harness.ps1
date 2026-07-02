param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$HardwareTriangles
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = if ($HardwareTriangles) {
    'smash64ds-battle-mariofox-dl-draw-all-hwtri'
} else {
    'smash64ds-battle-mariofox-dl-draw-all'
}
$build = if ($HardwareTriangles) {
    'build-battle-mariofox-dl-draw-all-hwtri'
} else {
    'build-battle-mariofox-dl-draw-all-harness'
}
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Join-Path $root "$target.nds"
$elf = Join-Path $root "$target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-dl-draw-all-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-dl-draw-all-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_dl_draw_all_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$makeArgs = @(
    '-C', $root,
    "TARGET=$target",
    "BUILD=$build",
    'NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw_all',
    '-j16'
)
if ($HardwareTriangles) {
    $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
}
& make @makeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox all-DL draw harness build did not produce the expected ROM and ELF.'
}
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath -GdbPort $verifierContext.GdbPort -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $verifierContext.GdbPort | Out-Null
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, 1))
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind',
        'printf "FTR_DL_MULTI=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLMultiDrawResult, gNdsFighterMarioFoxDLMultiDrawSafeResult, gNdsFighterMarioFoxDLMultiDrawMask, gNdsFighterMarioFoxDLMultiDrawDeferredMask, gNdsFighterMarioFoxDLMultiDrawCount',
        'printf "FTR_DL_MULTI_COUNTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterDLMultiDrawP0CandidateCount, gNdsFighterDLMultiDrawP1CandidateCount, gNdsFighterDLMultiDrawP0SelectedCount, gNdsFighterDLMultiDrawP1SelectedCount, gNdsFighterDLMultiDrawP0AttemptCount, gNdsFighterDLMultiDrawP1AttemptCount, gNdsFighterDLMultiDrawP0CleanCount, gNdsFighterDLMultiDrawP1CleanCount, gNdsFighterDLMultiDrawP0DrawnDObjCount, gNdsFighterDLMultiDrawP1DrawnDObjCount, gNdsFighterDLMultiDrawP0FailedCount, gNdsFighterDLMultiDrawP1FailedCount, gNdsFighterDLMultiDrawP0SelectedIndexMask, gNdsFighterDLMultiDrawP1SelectedIndexMask',
        'printf "FTR_DL_MULTI_STATS=%u,%u,%#x,%#x,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterDLMultiDrawP0FirstBlocker, gNdsFighterDLMultiDrawP1FirstBlocker, gNdsFighterDLMultiDrawP0BlockerMask, gNdsFighterDLMultiDrawP1BlockerMask, gNdsFighterDLMultiDrawP0CommandCount, gNdsFighterDLMultiDrawP1CommandCount, gNdsFighterDLMultiDrawP0FirstOpcode, gNdsFighterDLMultiDrawP1FirstOpcode, gNdsFighterDLMultiDrawP0UnsupportedOpcode, gNdsFighterDLMultiDrawP1UnsupportedOpcode, gNdsFighterDLMultiDrawP0UnsupportedCommandCount, gNdsFighterDLMultiDrawP1UnsupportedCommandCount',
        'printf "FTR_DL_MULTI_GEOM=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLMultiDrawP0VertexDecodedCount, gNdsFighterDLMultiDrawP1VertexDecodedCount, gNdsFighterDLMultiDrawP0TriangleCount, gNdsFighterDLMultiDrawP1TriangleCount, gNdsFighterDLMultiDrawP0TriangleValidCount, gNdsFighterDLMultiDrawP1TriangleValidCount, gNdsFighterDLMultiDrawP0TriangleDrawnCount, gNdsFighterDLMultiDrawP1TriangleDrawnCount, gNdsFighterDLMultiDrawP0PixelCount, gNdsFighterDLMultiDrawP1PixelCount, gNdsFighterDLMultiDrawTotalPixelCount',
        'printf "FTR_DL_MULTI_RENDER=%u,%u,%u,%u\n", gNdsFighterDLMultiDrawP0RealTriangleDrawnCount, gNdsFighterDLMultiDrawP1RealTriangleDrawnCount, gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount, gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount',
        'printf "FTR_DL_ALL=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLAllDrawResult, gNdsFighterMarioFoxDLAllDrawSafeResult, gNdsFighterMarioFoxDLAllDrawMask, gNdsFighterMarioFoxDLAllDrawDeferredMask, gNdsFighterMarioFoxDLAllDrawCount',
        'printf "FTR_DL_ALL_CALLBACKS=%u,%u,%u\n", gNdsFighterDLAllDrawDisplayCallbackCount, gNdsFighterDLAllDrawP0DisplayCallbackCount, gNdsFighterDLAllDrawP1DisplayCallbackCount',
        'printf "FTR_DL_ALL_PREVIEW=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLAllDrawPreviewWidth, gNdsFighterDLAllDrawPreviewHeight, gNdsFighterDLAllDrawPreviewPitch, gNdsFighterDLAllDrawPreviewReady, gNdsFighterDLAllDrawPreviewCommitBefore, gNdsFighterDLAllDrawPreviewCommitAfter, gNdsFighterDLAllDrawPreviewCommitDelta',
        'printf "FTR_DL_ALL_COUNTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterDLAllDrawP0CandidateCount, gNdsFighterDLAllDrawP1CandidateCount, gNdsFighterDLAllDrawP0SelectedCount, gNdsFighterDLAllDrawP1SelectedCount, gNdsFighterDLAllDrawP0AttemptCount, gNdsFighterDLAllDrawP1AttemptCount, gNdsFighterDLAllDrawP0CleanCount, gNdsFighterDLAllDrawP1CleanCount, gNdsFighterDLAllDrawP0DrawnDObjCount, gNdsFighterDLAllDrawP1DrawnDObjCount, gNdsFighterDLAllDrawP0FailedCount, gNdsFighterDLAllDrawP1FailedCount, gNdsFighterDLAllDrawP0SelectedIndexMask, gNdsFighterDLAllDrawP1SelectedIndexMask',
        'printf "FTR_DL_ALL_STATS=%u,%u,%#x,%#x,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterDLAllDrawP0FirstBlocker, gNdsFighterDLAllDrawP1FirstBlocker, gNdsFighterDLAllDrawP0BlockerMask, gNdsFighterDLAllDrawP1BlockerMask, gNdsFighterDLAllDrawP0CommandCount, gNdsFighterDLAllDrawP1CommandCount, gNdsFighterDLAllDrawP0FirstOpcode, gNdsFighterDLAllDrawP1FirstOpcode, gNdsFighterDLAllDrawP0UnsupportedOpcode, gNdsFighterDLAllDrawP1UnsupportedOpcode, gNdsFighterDLAllDrawP0UnsupportedCommandCount, gNdsFighterDLAllDrawP1UnsupportedCommandCount',
        'printf "FTR_DL_ALL_GEOM=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLAllDrawP0VertexDecodedCount, gNdsFighterDLAllDrawP1VertexDecodedCount, gNdsFighterDLAllDrawP0TriangleCount, gNdsFighterDLAllDrawP1TriangleCount, gNdsFighterDLAllDrawP0TriangleValidCount, gNdsFighterDLAllDrawP1TriangleValidCount, gNdsFighterDLAllDrawP0TriangleDrawnCount, gNdsFighterDLAllDrawP1TriangleDrawnCount, gNdsFighterDLAllDrawP0PixelCount, gNdsFighterDLAllDrawP1PixelCount, gNdsFighterDLAllDrawTotalPixelCount',
        'printf "FTR_DL_ALL_RENDER=%u,%u,%u,%u\n", gNdsFighterDLAllDrawP0RealTriangleDrawnCount, gNdsFighterDLAllDrawP1RealTriangleDrawnCount, gNdsFighterDLAllDrawP0MarkerTriangleDrawnCount, gNdsFighterDLAllDrawP1MarkerTriangleDrawnCount',
        'printf "FTR_DL_ALL_HW=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLAllDrawP0HardwareTriangleCount, gNdsFighterDLAllDrawP1HardwareTriangleCount, gNdsFighterDLAllDrawP0HardwareOracleTriangleCount, gNdsFighterDLAllDrawP1HardwareOracleTriangleCount, gNdsFighterDLAllDrawP0HardwareOracleRejectCount, gNdsFighterDLAllDrawP1HardwareOracleRejectCount, gNdsFighterDLAllDrawP0HardwareMatrixSeedCount, gNdsFighterDLAllDrawP1HardwareMatrixSeedCount',
        'printf "FTR_DL_ALL_HWTEX=%u,%u,%u,%u,%#x,%u,%u\n", gNdsFighterDLAllDrawHardwareTextureBindCount, gNdsFighterDLAllDrawHardwareTextureUploadCount, gNdsFighterDLAllDrawHardwareTextureReadyCount, gNdsFighterDLAllDrawHardwareTextureRejectCount, gNdsFighterDLAllDrawHardwareTextureFormatMask, gNdsFighterDLAllDrawHardwareTextureMaxWidth, gNdsFighterDLAllDrawHardwareTextureMaxHeight',
        'printf "FTR_DL_ALL_AXIS=%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterDLAllDrawP0Axis, gNdsFighterDLAllDrawP1Axis, gNdsFighterDLAllDrawP0Area, gNdsFighterDLAllDrawP1Area, gNdsFighterDLAllDrawP0MinA, gNdsFighterDLAllDrawP0MaxA, gNdsFighterDLAllDrawP0MinB, gNdsFighterDLAllDrawP0MaxB, gNdsFighterDLAllDrawP1MinA, gNdsFighterDLAllDrawP1MaxA, gNdsFighterDLAllDrawP1MinB, gNdsFighterDLAllDrawP1MaxB',
        'printf "FTR_DL_ALL_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%#x,%#x\n", gNdsFighterDLAllDrawP0ScreenMinX, gNdsFighterDLAllDrawP0ScreenMaxX, gNdsFighterDLAllDrawP0ScreenMinY, gNdsFighterDLAllDrawP0ScreenMaxY, gNdsFighterDLAllDrawP1ScreenMinX, gNdsFighterDLAllDrawP1ScreenMaxX, gNdsFighterDLAllDrawP1ScreenMinY, gNdsFighterDLAllDrawP1ScreenMaxY, gNdsFighterDLAllDrawP0ColorChecksum, gNdsFighterDLAllDrawP1ColorChecksum',
        'printf "FTR_DL_ALL_STATE=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterDLAllDrawP0StatusAfter, gNdsFighterDLAllDrawP1StatusAfter, gNdsFighterDLAllDrawP0MotionAfter, gNdsFighterDLAllDrawP1MotionAfter, gNdsFighterDLAllDrawP0GAAfter, gNdsFighterDLAllDrawP1GAAfter, gNdsFighterDLAllDrawP0RootXBeforeBits, gNdsFighterDLAllDrawP0RootXAfterBits, gNdsFighterDLAllDrawP1RootXBeforeBits, gNdsFighterDLAllDrawP1RootXAfterBits',
        'printf "FTR_DL_ALL_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDLAllDrawGObjDelta, gNdsFighterDLAllDrawDrawCallCount, gNdsFighterDLAllDrawMatrixCallCount, gNdsFighterDLAllDrawGameplayUpdateCount, gNdsFighterDLAllDrawRangeRejectCount, gNdsFighterDLAllDrawVertexRangeRejectCount',
        'printf "FTR_DL_ALL_FAIL=%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%#x,%#x,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLAllDrawP0FirstFailedSelectedIndex, gNdsFighterDLAllDrawP1FirstFailedSelectedIndex, gNdsFighterDLAllDrawP0FirstFailedTreeIndex, gNdsFighterDLAllDrawP1FirstFailedTreeIndex, gNdsFighterDLAllDrawP0FirstFailedReason, gNdsFighterDLAllDrawP1FirstFailedReason, gNdsFighterDLAllDrawP0FirstFailedDObj, gNdsFighterDLAllDrawP1FirstFailedDObj, gNdsFighterDLAllDrawP0FirstFailedDL, gNdsFighterDLAllDrawP1FirstFailedDL, gNdsFighterDLAllDrawP0FirstFailedCommandCount, gNdsFighterDLAllDrawP1FirstFailedCommandCount, gNdsFighterDLAllDrawP0FirstFailedFirstOpcode, gNdsFighterDLAllDrawP1FirstFailedFirstOpcode, gNdsFighterDLAllDrawP0FirstFailedBlocker, gNdsFighterDLAllDrawP1FirstFailedBlocker, gNdsFighterDLAllDrawP0FirstFailedUnsupportedOpcode, gNdsFighterDLAllDrawP1FirstFailedUnsupportedOpcode, gNdsFighterDLAllDrawP0FirstFailedUnsupportedCommandCount, gNdsFighterDLAllDrawP1FirstFailedUnsupportedCommandCount, gNdsFighterDLAllDrawP0FirstFailedVertexRangeRejectCount, gNdsFighterDLAllDrawP1FirstFailedVertexRangeRejectCount, gNdsFighterDLAllDrawP0FirstFailedVertexDecodedCount, gNdsFighterDLAllDrawP1FirstFailedVertexDecodedCount, gNdsFighterDLAllDrawP0FirstFailedTriangleCount, gNdsFighterDLAllDrawP1FirstFailedTriangleCount, gNdsFighterDLAllDrawP0FirstFailedTriangleValidCount, gNdsFighterDLAllDrawP1FirstFailedTriangleValidCount',
        'printf "PLATFORM_DL_PREVIEW=%u,%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount, gNdsOriginalDLPreviewDrawCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $multi = [regex]::Match($gdbStdout, 'FTR_DL_MULTI=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $multiCounts = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $multiStats = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_STATS=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $multiGeom = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $multiRender = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $all = [regex]::Match($gdbStdout, 'FTR_DL_ALL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $callbacks = [regex]::Match($gdbStdout, 'FTR_DL_ALL_CALLBACKS=([0-9]+),([0-9]+),([0-9]+)')
    $preview = [regex]::Match($gdbStdout, 'FTR_DL_ALL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $counts = [regex]::Match($gdbStdout, 'FTR_DL_ALL_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stats = [regex]::Match($gdbStdout, 'FTR_DL_ALL_STATS=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $geom = [regex]::Match($gdbStdout, 'FTR_DL_ALL_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $render = [regex]::Match($gdbStdout, 'FTR_DL_ALL_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $hw = [regex]::Match($gdbStdout, 'FTR_DL_ALL_HW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $hwTexture = [regex]::Match($gdbStdout, 'FTR_DL_ALL_HWTEX=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $axis = [regex]::Match($gdbStdout, 'FTR_DL_ALL_AXIS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $screen = [regex]::Match($gdbStdout, 'FTR_DL_ALL_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $state = [regex]::Match($gdbStdout, 'FTR_DL_ALL_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $safe = [regex]::Match($gdbStdout, 'FTR_DL_ALL_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $fail = [regex]::Match($gdbStdout, 'FTR_DL_ALL_FAIL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 33 -and [int]$harn.Groups[3].Value -eq 22 -and [int]$harn.Groups[4].Value -eq 21 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Battle Mario/Fox all-DL draw harness did not select direct VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($multi.Success -and (Convert-MarkerUInt32 $multi.Groups[1].Value) -eq 0x46544d55 -and (Convert-MarkerUInt32 $multi.Groups[2].Value) -eq 0x46544d56 -and ((Convert-MarkerUInt32 $multi.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $multi.Groups[4].Value) -eq 0xff -and [int]$multi.Groups[5].Value -eq 2) 'Prerequisite multi-DL draw proof did not pass.' $gdbStdout
    Assert-Condition ($multiCounts.Success -and [int]$multiCounts.Groups[1].Value -eq 14 -and [int]$multiCounts.Groups[2].Value -eq 18 -and [int]$multiCounts.Groups[3].Value -eq 4 -and [int]$multiCounts.Groups[4].Value -eq 4 -and [int]$multiCounts.Groups[7].Value -eq 4 -and [int]$multiCounts.Groups[8].Value -eq 4 -and [int]$multiCounts.Groups[11].Value -eq 0 -and [int]$multiCounts.Groups[12].Value -eq 0) 'Prerequisite multi-DL candidate/selection counts changed.' $gdbStdout
    Assert-Condition ($all.Success -and (Convert-MarkerUInt32 $all.Groups[1].Value) -eq 0x4654414c -and (Convert-MarkerUInt32 $all.Groups[2].Value) -eq 0x46544153 -and ((Convert-MarkerUInt32 $all.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $all.Groups[4].Value) -eq 0xff -and [int]$all.Groups[5].Value -eq 2) 'Mario/Fox all-DL draw proof did not pass.' $gdbStdout
    Assert-Condition ($callbacks.Success -and [int]$callbacks.Groups[1].Value -eq 2 -and [int]$callbacks.Groups[2].Value -eq 1 -and [int]$callbacks.Groups[3].Value -eq 1) 'All-DL draw did not enter ftDisplayMainProcDisplay exactly once per fighter.' $gdbStdout
    Assert-Condition ($preview.Success -and [int]$preview.Groups[1].Value -eq 96 -and [int]$preview.Groups[2].Value -eq 72 -and [int]$preview.Groups[3].Value -ge 96 -and [int]$preview.Groups[4].Value -eq 1 -and [int]$preview.Groups[6].Value -gt [int]$preview.Groups[5].Value -and [int]$preview.Groups[7].Value -eq 1) 'All-DL preview was not committed as a 96x72 retained surface.' $gdbStdout
    Assert-Condition ($counts.Success -and [int]$counts.Groups[1].Value -eq 14 -and [int]$counts.Groups[2].Value -eq 18 -and [int]$counts.Groups[3].Value -eq 14 -and [int]$counts.Groups[4].Value -eq 18 -and [int]$counts.Groups[5].Value -eq 14 -and [int]$counts.Groups[6].Value -eq 18 -and [int]$counts.Groups[7].Value -eq 14 -and [int]$counts.Groups[8].Value -eq 18 -and [int]$counts.Groups[9].Value -eq 14 -and [int]$counts.Groups[10].Value -eq 18 -and [int]$counts.Groups[11].Value -eq 0 -and [int]$counts.Groups[12].Value -eq 0 -and (Convert-MarkerUInt32 $counts.Groups[13].Value) -ne 0 -and (Convert-MarkerUInt32 $counts.Groups[14].Value) -ne 0) 'All-DL candidate/selection counts were not expected.' $gdbStdout
    Assert-Condition ($stats.Success -and $multiStats.Success -and [int]$stats.Groups[1].Value -eq 0 -and [int]$stats.Groups[2].Value -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[3].Value) -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[4].Value) -eq 0 -and [int]$stats.Groups[5].Value -gt [int]$multiStats.Groups[5].Value -and [int]$stats.Groups[6].Value -gt [int]$multiStats.Groups[6].Value -and (Convert-MarkerUInt32 $stats.Groups[7].Value) -ne 0 -and (Convert-MarkerUInt32 $stats.Groups[8].Value) -ne 0 -and (Convert-MarkerUInt32 $stats.Groups[9].Value) -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[10].Value) -eq 0 -and [int]$stats.Groups[11].Value -eq 0 -and [int]$stats.Groups[12].Value -eq 0) 'All-DL renderer stats were not clean or did not exceed multi-DL command counts.' $gdbStdout
    Assert-Condition ($geom.Success -and $multiGeom.Success -and [int]$geom.Groups[1].Value -gt [int]$multiGeom.Groups[1].Value -and [int]$geom.Groups[2].Value -gt [int]$multiGeom.Groups[2].Value -and [int]$geom.Groups[3].Value -gt [int]$multiGeom.Groups[3].Value -and [int]$geom.Groups[4].Value -gt [int]$multiGeom.Groups[4].Value -and [int]$geom.Groups[5].Value -gt [int]$multiGeom.Groups[5].Value -and [int]$geom.Groups[6].Value -gt [int]$multiGeom.Groups[6].Value -and [int]$geom.Groups[7].Value -gt [int]$multiGeom.Groups[7].Value -and [int]$geom.Groups[8].Value -gt [int]$multiGeom.Groups[8].Value -and [int]$geom.Groups[9].Value -ge [int]$multiGeom.Groups[9].Value -and [int]$geom.Groups[10].Value -ge [int]$multiGeom.Groups[10].Value -and [int]$geom.Groups[11].Value -ge [int]$multiGeom.Groups[11].Value) 'All-DL geometry/pixels did not exceed the first-four-DL baseline.' $gdbStdout
    Assert-Condition ($render.Success -and $multiRender.Success -and [int]$render.Groups[1].Value -gt [int]$multiRender.Groups[1].Value -and [int]$render.Groups[2].Value -gt [int]$multiRender.Groups[2].Value) 'All-DL real non-degenerate triangle count did not exceed first-four-DL baseline.' $gdbStdout
    Assert-Condition ($hw.Success) 'All-DL hardware triangle diagnostics were not printed.' $gdbStdout
    if ($HardwareTriangles) {
        Assert-Condition ([int]$hw.Groups[1].Value -gt 0 -and [int]$hw.Groups[2].Value -gt 0 -and [int]$hw.Groups[3].Value -gt 0 -and [int]$hw.Groups[4].Value -gt 0 -and [int]$hw.Groups[7].Value -gt 0 -and [int]$hw.Groups[8].Value -gt 0) 'Hardware all-DL draw did not submit triangles from seeded matrices.' $gdbStdout
    }
    Assert-Condition ($hwTexture.Success) 'All-DL hardware texture diagnostics were not printed.' $gdbStdout
    if ($HardwareTriangles) {
        Assert-Condition ([int]$hwTexture.Groups[1].Value -gt 0 -and [int]$hwTexture.Groups[2].Value -gt 0 -and [int]$hwTexture.Groups[3].Value -gt 0 -and (Convert-MarkerUInt32 $hwTexture.Groups[5].Value) -ne 0 -and [int]$hwTexture.Groups[6].Value -gt 0 -and [int]$hwTexture.Groups[7].Value -gt 0) 'Hardware all-DL draw did not bind/upload a texture.' $gdbStdout
    }
    Assert-Condition ($axis.Success -and [int]$axis.Groups[1].Value -le 2 -and [int]$axis.Groups[2].Value -le 2 -and [int]$axis.Groups[3].Value -gt 0 -and [int]$axis.Groups[4].Value -gt 0 -and (([int]$axis.Groups[6].Value -ne [int]$axis.Groups[5].Value) -or ([int]$axis.Groups[8].Value -ne [int]$axis.Groups[7].Value)) -and (([int]$axis.Groups[10].Value -ne [int]$axis.Groups[9].Value) -or ([int]$axis.Groups[12].Value -ne [int]$axis.Groups[11].Value))) 'All-DL shared projection axes/bounds were not usable.' $gdbStdout
    Assert-Condition ($screen.Success -and [int]$screen.Groups[1].Value -ge 0 -and [int]$screen.Groups[2].Value -le 95 -and [int]$screen.Groups[3].Value -ge 0 -and [int]$screen.Groups[4].Value -le 71 -and [int]$screen.Groups[5].Value -ge 0 -and [int]$screen.Groups[6].Value -le 95 -and [int]$screen.Groups[7].Value -ge 0 -and [int]$screen.Groups[8].Value -le 71 -and (Convert-MarkerUInt32 $screen.Groups[9].Value) -ne 0 -and (Convert-MarkerUInt32 $screen.Groups[10].Value) -ne 0) 'All-DL screen bounds/checksums were not valid.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 10 -and [int]$state.Groups[2].Value -eq 10 -and [int]$state.Groups[3].Value -eq 4 -and [int]$state.Groups[4].Value -eq 4 -and [int]$state.Groups[5].Value -eq 0 -and [int]$state.Groups[6].Value -eq 0 -and (Convert-MarkerUInt32 $state.Groups[7].Value) -eq (Convert-MarkerUInt32 $state.Groups[8].Value) -and (Convert-MarkerUInt32 $state.Groups[9].Value) -eq (Convert-MarkerUInt32 $state.Groups[10].Value)) 'All-DL draw mutated fighter state or root position.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0 -and [int]$safe.Groups[5].Value -eq 0 -and [int]$safe.Groups[6].Value -eq 0) 'All-DL draw escaped bounded software-preview behavior.' $gdbStdout
    Assert-Condition ($fail.Success -and [uint64]$fail.Groups[1].Value -eq 4294967295 -and [uint64]$fail.Groups[2].Value -eq 4294967295 -and [uint64]$fail.Groups[3].Value -eq 4294967295 -and [uint64]$fail.Groups[4].Value -eq 4294967295 -and (Convert-MarkerUInt32 $fail.Groups[5].Value) -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[6].Value) -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[7].Value) -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[8].Value) -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[9].Value) -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[10].Value) -eq 0 -and [int]$fail.Groups[11].Value -eq 0 -and [int]$fail.Groups[12].Value -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[13].Value) -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[14].Value) -eq 0 -and [int]$fail.Groups[15].Value -eq 0 -and [int]$fail.Groups[16].Value -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[17].Value) -eq 0 -and (Convert-MarkerUInt32 $fail.Groups[18].Value) -eq 0 -and [int]$fail.Groups[19].Value -eq 0 -and [int]$fail.Groups[20].Value -eq 0 -and [int]$fail.Groups[21].Value -eq 0 -and [int]$fail.Groups[22].Value -eq 0 -and [int]$fail.Groups[23].Value -eq 0 -and [int]$fail.Groups[24].Value -eq 0 -and [int]$fail.Groups[25].Value -eq 0 -and [int]$fail.Groups[26].Value -eq 0 -and [int]$fail.Groups[27].Value -eq 0 -and [int]$fail.Groups[28].Value -eq 0) 'All-DL first-failure diagnostics were not fully clear after strict F3DEX2 decode.' $gdbStdout
    Assert-Condition ($platform.Success -and [int]$platform.Groups[1].Value -eq 1 -and [int]$platform.Groups[2].Value -eq 96 -and [int]$platform.Groups[3].Value -eq 72 -and [int]$platform.Groups[4].Value -ge [int]$preview.Groups[6].Value) 'Platform original-DL preview state is not ready after all-DL draw.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after all-DL draw.' $gdbStdout
    $summary = ("Battle Mario/Fox all-DL draw harness passed: scene=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels={0}/{1} tris={2}/{3} real={4}/{5} marker={6}/{7} clean=14/18 failed=0/0 preview=96x72 safe=1" -f $geom.Groups[9].Value, $geom.Groups[10].Value, $geom.Groups[3].Value, $geom.Groups[4].Value, $render.Groups[1].Value, $render.Groups[2].Value, $render.Groups[3].Value, $render.Groups[4].Value)
    if ($HardwareTriangles) {
        $summary += (" hw={0}/{1} oracle={2}/{3} reject={4}/{5} seed={6}/{7} hwtex=bind{8}/upload{9}/ready{10}/reject{11}/fmt{12}/max{13}x{14}" -f $hw.Groups[1].Value, $hw.Groups[2].Value, $hw.Groups[3].Value, $hw.Groups[4].Value, $hw.Groups[5].Value, $hw.Groups[6].Value, $hw.Groups[7].Value, $hw.Groups[8].Value, $hwTexture.Groups[1].Value, $hwTexture.Groups[2].Value, $hwTexture.Groups[3].Value, $hwTexture.Groups[4].Value, $hwTexture.Groups[5].Value, $hwTexture.Groups[6].Value, $hwTexture.Groups[7].Value)
    }
    Write-Output $summary
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
