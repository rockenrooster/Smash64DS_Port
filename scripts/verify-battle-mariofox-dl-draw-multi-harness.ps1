param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Join-Path $root 'smash64ds-battle-mariofox-dl-draw-multi.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-dl-draw-multi.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-dl-draw-multi-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-dl-draw-multi-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_dl_draw_multi_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-dl-draw-multi BUILD=build-battle-mariofox-dl-draw-multi-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw_multi -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox multi-DL draw harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_DL_DRAW=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLDrawResult, gNdsFighterMarioFoxDLDrawSafeResult, gNdsFighterMarioFoxDLDrawMask, gNdsFighterMarioFoxDLDrawDeferredMask, gNdsFighterMarioFoxDLDrawCount',
        'printf "FTR_DL_DRAW_GEOM=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLDrawP0VertexDecodedCount, gNdsFighterDLDrawP1VertexDecodedCount, gNdsFighterDLDrawP0TriangleCount, gNdsFighterDLDrawP1TriangleCount, gNdsFighterDLDrawP0TriangleValidCount, gNdsFighterDLDrawP1TriangleValidCount, gNdsFighterDLDrawP0TriangleDrawnCount, gNdsFighterDLDrawP1TriangleDrawnCount, gNdsFighterDLDrawP0PixelCount, gNdsFighterDLDrawP1PixelCount, gNdsFighterDLDrawTotalPixelCount',
        'printf "FTR_DL_DRAW_RENDER=%u,%u,%u,%u\n", gNdsFighterDLDrawP0RealTriangleDrawnCount, gNdsFighterDLDrawP1RealTriangleDrawnCount, gNdsFighterDLDrawP0MarkerTriangleDrawnCount, gNdsFighterDLDrawP1MarkerTriangleDrawnCount',
        'printf "FTR_DL_DRAW_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDLDrawGObjDelta, gNdsFighterDLDrawDrawCallCount, gNdsFighterDLDrawMatrixCallCount, gNdsFighterDLDrawGameplayUpdateCount, gNdsFighterDLDrawRangeRejectCount, gNdsFighterDLDrawVertexRangeRejectCount',
        'printf "FTR_DL_MULTI=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLMultiDrawResult, gNdsFighterMarioFoxDLMultiDrawSafeResult, gNdsFighterMarioFoxDLMultiDrawMask, gNdsFighterMarioFoxDLMultiDrawDeferredMask, gNdsFighterMarioFoxDLMultiDrawCount',
        'printf "FTR_DL_MULTI_PREVIEW=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLMultiDrawPreviewWidth, gNdsFighterDLMultiDrawPreviewHeight, gNdsFighterDLMultiDrawPreviewPitch, gNdsFighterDLMultiDrawPreviewReady, gNdsFighterDLMultiDrawPreviewCommitBefore, gNdsFighterDLMultiDrawPreviewCommitAfter, gNdsFighterDLMultiDrawPreviewCommitDelta',
        'printf "FTR_DL_MULTI_COUNTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterDLMultiDrawP0CandidateCount, gNdsFighterDLMultiDrawP1CandidateCount, gNdsFighterDLMultiDrawP0SelectedCount, gNdsFighterDLMultiDrawP1SelectedCount, gNdsFighterDLMultiDrawP0AttemptCount, gNdsFighterDLMultiDrawP1AttemptCount, gNdsFighterDLMultiDrawP0CleanCount, gNdsFighterDLMultiDrawP1CleanCount, gNdsFighterDLMultiDrawP0DrawnDObjCount, gNdsFighterDLMultiDrawP1DrawnDObjCount, gNdsFighterDLMultiDrawP0FailedCount, gNdsFighterDLMultiDrawP1FailedCount, gNdsFighterDLMultiDrawP0SelectedIndexMask, gNdsFighterDLMultiDrawP1SelectedIndexMask',
        'printf "FTR_DL_MULTI_STATS=%u,%u,%#x,%#x,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterDLMultiDrawP0FirstBlocker, gNdsFighterDLMultiDrawP1FirstBlocker, gNdsFighterDLMultiDrawP0BlockerMask, gNdsFighterDLMultiDrawP1BlockerMask, gNdsFighterDLMultiDrawP0CommandCount, gNdsFighterDLMultiDrawP1CommandCount, gNdsFighterDLMultiDrawP0FirstOpcode, gNdsFighterDLMultiDrawP1FirstOpcode, gNdsFighterDLMultiDrawP0UnsupportedOpcode, gNdsFighterDLMultiDrawP1UnsupportedOpcode, gNdsFighterDLMultiDrawP0UnsupportedCommandCount, gNdsFighterDLMultiDrawP1UnsupportedCommandCount',
        'printf "FTR_DL_MULTI_GEOM=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLMultiDrawP0VertexDecodedCount, gNdsFighterDLMultiDrawP1VertexDecodedCount, gNdsFighterDLMultiDrawP0TriangleCount, gNdsFighterDLMultiDrawP1TriangleCount, gNdsFighterDLMultiDrawP0TriangleValidCount, gNdsFighterDLMultiDrawP1TriangleValidCount, gNdsFighterDLMultiDrawP0TriangleDrawnCount, gNdsFighterDLMultiDrawP1TriangleDrawnCount, gNdsFighterDLMultiDrawP0PixelCount, gNdsFighterDLMultiDrawP1PixelCount, gNdsFighterDLMultiDrawTotalPixelCount',
        'printf "FTR_DL_MULTI_RENDER=%u,%u,%u,%u\n", gNdsFighterDLMultiDrawP0RealTriangleDrawnCount, gNdsFighterDLMultiDrawP1RealTriangleDrawnCount, gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount, gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount',
        'printf "FTR_DL_MULTI_AXIS=%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterDLMultiDrawP0Axis, gNdsFighterDLMultiDrawP1Axis, gNdsFighterDLMultiDrawP0Area, gNdsFighterDLMultiDrawP1Area, gNdsFighterDLMultiDrawP0MinA, gNdsFighterDLMultiDrawP0MaxA, gNdsFighterDLMultiDrawP0MinB, gNdsFighterDLMultiDrawP0MaxB, gNdsFighterDLMultiDrawP1MinA, gNdsFighterDLMultiDrawP1MaxA, gNdsFighterDLMultiDrawP1MinB, gNdsFighterDLMultiDrawP1MaxB',
        'printf "FTR_DL_MULTI_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%#x,%#x\n", gNdsFighterDLMultiDrawP0ScreenMinX, gNdsFighterDLMultiDrawP0ScreenMaxX, gNdsFighterDLMultiDrawP0ScreenMinY, gNdsFighterDLMultiDrawP0ScreenMaxY, gNdsFighterDLMultiDrawP1ScreenMinX, gNdsFighterDLMultiDrawP1ScreenMaxX, gNdsFighterDLMultiDrawP1ScreenMinY, gNdsFighterDLMultiDrawP1ScreenMaxY, gNdsFighterDLMultiDrawP0ColorChecksum, gNdsFighterDLMultiDrawP1ColorChecksum',
        'printf "FTR_DL_MULTI_STATE=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterDLMultiDrawP0StatusAfter, gNdsFighterDLMultiDrawP1StatusAfter, gNdsFighterDLMultiDrawP0MotionAfter, gNdsFighterDLMultiDrawP1MotionAfter, gNdsFighterDLMultiDrawP0GAAfter, gNdsFighterDLMultiDrawP1GAAfter, gNdsFighterDLMultiDrawP0RootXBeforeBits, gNdsFighterDLMultiDrawP0RootXAfterBits, gNdsFighterDLMultiDrawP1RootXBeforeBits, gNdsFighterDLMultiDrawP1RootXAfterBits',
        'printf "FTR_DL_MULTI_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDLMultiDrawGObjDelta, gNdsFighterDLMultiDrawDrawCallCount, gNdsFighterDLMultiDrawMatrixCallCount, gNdsFighterDLMultiDrawGameplayUpdateCount, gNdsFighterDLMultiDrawRangeRejectCount, gNdsFighterDLMultiDrawVertexRangeRejectCount',
        'printf "PLATFORM_DL_PREVIEW=%u,%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount, gNdsOriginalDLPreviewDrawCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $draw = [regex]::Match($gdbStdout, 'FTR_DL_DRAW=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $drawGeom = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $drawRender = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $drawSafe = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $multi = [regex]::Match($gdbStdout, 'FTR_DL_MULTI=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $preview = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $counts = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stats = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_STATS=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $geom = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $render = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $axis = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_AXIS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $screen = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $state = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $safe = [regex]::Match($gdbStdout, 'FTR_DL_MULTI_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 31 -and [int]$harn.Groups[3].Value -eq 22 -and [int]$harn.Groups[4].Value -eq 21 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Battle Mario/Fox multi-DL draw harness did not select direct VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($draw.Success -and (Convert-MarkerUInt32 $draw.Groups[1].Value) -eq 0x46544c44 -and (Convert-MarkerUInt32 $draw.Groups[2].Value) -eq 0x46544c57 -and ((Convert-MarkerUInt32 $draw.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $draw.Groups[4].Value) -eq 0xff -and [int]$draw.Groups[5].Value -eq 2) 'Prerequisite first-DL draw proof did not pass.' $gdbStdout
    Assert-Condition ($drawGeom.Success -and [int]$drawGeom.Groups[9].Value -gt 0 -and [int]$drawGeom.Groups[10].Value -gt 0 -and [int]$drawGeom.Groups[3].Value -eq 37 -and [int]$drawGeom.Groups[4].Value -eq 20) 'Prerequisite first-DL draw baseline changed.' $gdbStdout
    Assert-Condition ($drawRender.Success -and [int]$drawRender.Groups[1].Value -gt 0 -and [int]$drawRender.Groups[2].Value -gt 0) 'Prerequisite first-DL draw did not prove real triangles.' $gdbStdout
    Assert-Condition ($drawSafe.Success -and [int]$drawSafe.Groups[1].Value -eq 0 -and [int]$drawSafe.Groups[2].Value -eq 0 -and [int]$drawSafe.Groups[3].Value -eq 0 -and [int]$drawSafe.Groups[4].Value -eq 0 -and [int]$drawSafe.Groups[5].Value -eq 0 -and [int]$drawSafe.Groups[6].Value -eq 0) 'Prerequisite first-DL draw escaped bounded behavior.' $gdbStdout
    Assert-Condition ($multi.Success -and (Convert-MarkerUInt32 $multi.Groups[1].Value) -eq 0x46544d55 -and (Convert-MarkerUInt32 $multi.Groups[2].Value) -eq 0x46544d56 -and ((Convert-MarkerUInt32 $multi.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $multi.Groups[4].Value) -eq 0xff -and [int]$multi.Groups[5].Value -eq 2) 'Mario/Fox multi-DL draw proof did not pass.' $gdbStdout
    Assert-Condition ($preview.Success -and [int]$preview.Groups[1].Value -eq 96 -and [int]$preview.Groups[2].Value -eq 72 -and [int]$preview.Groups[3].Value -ge 96 -and [int]$preview.Groups[4].Value -eq 1 -and [int]$preview.Groups[6].Value -gt [int]$preview.Groups[5].Value -and [int]$preview.Groups[7].Value -eq 1) 'Multi-DL preview was not committed as a 96x72 retained surface.' $gdbStdout
    Assert-Condition ($counts.Success -and [int]$counts.Groups[1].Value -eq 14 -and [int]$counts.Groups[2].Value -eq 18 -and [int]$counts.Groups[3].Value -eq 4 -and [int]$counts.Groups[4].Value -eq 4 -and [int]$counts.Groups[5].Value -eq 4 -and [int]$counts.Groups[6].Value -eq 4 -and [int]$counts.Groups[7].Value -eq 4 -and [int]$counts.Groups[8].Value -eq 4 -and [int]$counts.Groups[9].Value -eq 4 -and [int]$counts.Groups[10].Value -eq 4 -and [int]$counts.Groups[11].Value -eq 0 -and [int]$counts.Groups[12].Value -eq 0 -and (Convert-MarkerUInt32 $counts.Groups[13].Value) -ne 0 -and (Convert-MarkerUInt32 $counts.Groups[14].Value) -ne 0) 'Multi-DL candidate/selection counts were not expected.' $gdbStdout
    Assert-Condition ($stats.Success -and [int]$stats.Groups[1].Value -eq 0 -and [int]$stats.Groups[2].Value -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[3].Value) -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[4].Value) -eq 0 -and [int]$stats.Groups[5].Value -gt 59 -and [int]$stats.Groups[6].Value -gt 69 -and (Convert-MarkerUInt32 $stats.Groups[7].Value) -ne 0 -and (Convert-MarkerUInt32 $stats.Groups[8].Value) -ne 0 -and (Convert-MarkerUInt32 $stats.Groups[9].Value) -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[10].Value) -eq 0 -and [int]$stats.Groups[11].Value -eq 0 -and [int]$stats.Groups[12].Value -eq 0) 'Multi-DL renderer stats were not clean.' $gdbStdout
    Assert-Condition ($geom.Success -and [int]$geom.Groups[1].Value -gt [int]$drawGeom.Groups[1].Value -and [int]$geom.Groups[2].Value -gt [int]$drawGeom.Groups[2].Value -and [int]$geom.Groups[3].Value -gt [int]$drawGeom.Groups[3].Value -and [int]$geom.Groups[4].Value -gt [int]$drawGeom.Groups[4].Value -and [int]$geom.Groups[5].Value -gt [int]$drawGeom.Groups[5].Value -and [int]$geom.Groups[6].Value -gt [int]$drawGeom.Groups[6].Value -and [int]$geom.Groups[7].Value -gt [int]$drawGeom.Groups[7].Value -and [int]$geom.Groups[8].Value -gt [int]$drawGeom.Groups[8].Value -and [int]$geom.Groups[9].Value -ge [int]$drawGeom.Groups[9].Value -and [int]$geom.Groups[10].Value -ge [int]$drawGeom.Groups[10].Value -and [int]$geom.Groups[11].Value -ge [int]$drawGeom.Groups[11].Value) 'Multi-DL geometry/pixels did not exceed the first-DL baseline.' $gdbStdout
    Assert-Condition ($render.Success -and [int]$render.Groups[1].Value -gt [int]$drawRender.Groups[1].Value -and [int]$render.Groups[2].Value -gt [int]$drawRender.Groups[2].Value) 'Multi-DL real non-degenerate triangle count did not exceed first-DL baseline.' $gdbStdout
    Assert-Condition ($axis.Success -and [int]$axis.Groups[1].Value -le 2 -and [int]$axis.Groups[2].Value -le 2 -and [int]$axis.Groups[3].Value -gt 0 -and [int]$axis.Groups[4].Value -gt 0 -and (([int]$axis.Groups[6].Value -ne [int]$axis.Groups[5].Value) -or ([int]$axis.Groups[8].Value -ne [int]$axis.Groups[7].Value)) -and (([int]$axis.Groups[10].Value -ne [int]$axis.Groups[9].Value) -or ([int]$axis.Groups[12].Value -ne [int]$axis.Groups[11].Value))) 'Multi-DL shared projection axes/bounds were not usable.' $gdbStdout
    Assert-Condition ($screen.Success -and [int]$screen.Groups[1].Value -ge 0 -and [int]$screen.Groups[2].Value -le 95 -and [int]$screen.Groups[3].Value -ge 0 -and [int]$screen.Groups[4].Value -le 71 -and [int]$screen.Groups[5].Value -ge 0 -and [int]$screen.Groups[6].Value -le 95 -and [int]$screen.Groups[7].Value -ge 0 -and [int]$screen.Groups[8].Value -le 71 -and (Convert-MarkerUInt32 $screen.Groups[9].Value) -ne 0 -and (Convert-MarkerUInt32 $screen.Groups[10].Value) -ne 0) 'Multi-DL screen bounds/checksums were not valid.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 10 -and [int]$state.Groups[2].Value -eq 10 -and [int]$state.Groups[3].Value -eq 4 -and [int]$state.Groups[4].Value -eq 4 -and [int]$state.Groups[5].Value -eq 0 -and [int]$state.Groups[6].Value -eq 0 -and (Convert-MarkerUInt32 $state.Groups[7].Value) -eq (Convert-MarkerUInt32 $state.Groups[8].Value) -and (Convert-MarkerUInt32 $state.Groups[9].Value) -eq (Convert-MarkerUInt32 $state.Groups[10].Value)) 'Multi-DL draw mutated fighter state or root position.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0 -and [int]$safe.Groups[5].Value -eq 0 -and [int]$safe.Groups[6].Value -eq 0) 'Multi-DL draw escaped bounded software-preview behavior.' $gdbStdout
    Assert-Condition ($platform.Success -and [int]$platform.Groups[1].Value -eq 1 -and [int]$platform.Groups[2].Value -eq 96 -and [int]$platform.Groups[3].Value -eq 72 -and [int]$platform.Groups[4].Value -ge [int]$preview.Groups[6].Value) 'Platform original-DL preview state is not ready after multi-DL draw.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after multi-DL draw.' $gdbStdout
    Write-Output ("Battle Mario/Fox multi-DL draw harness passed: scene=22/21 candidates=14/18 selected=4/4 pixels={0}/{1} tris={2}/{3} real={4}/{5} marker={6}/{7} clean=4/4 preview=96x72 safe=1" -f $geom.Groups[9].Value, $geom.Groups[10].Value, $geom.Groups[3].Value, $geom.Groups[4].Value, $render.Groups[1].Value, $render.Groups[2].Value, $render.Groups[3].Value, $render.Groups[4].Value)
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
