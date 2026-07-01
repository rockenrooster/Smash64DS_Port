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
$rom = Join-Path $root 'smash64ds-menu-chain-mariofox-dl-draw.nds'
$elf = Join-Path $root 'smash64ds-menu-chain-mariofox-dl-draw.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.menu-chain-mariofox-dl-draw-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.menu-chain-mariofox-dl-draw-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_menu_chain_mariofox_dl_draw_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-menu-chain-mariofox-dl-draw BUILD=build-menu-chain-mariofox-dl-draw-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Menu-chain Mario/Fox DL draw harness build did not produce the expected ROM and ELF.'
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
        'printf "CHAIN=%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask, gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask, gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask',
        'printf "CHAIN_FINAL=%u,%u,%u,%u,%u\n", gNdsVSModeStartTransitionSceneCurrFinal, gNdsPlayersVSReadyTransitionSceneCurrFinal, gNdsMapsSelectTransitionSceneCurrFinal, gNdsMapsSelectTransitionScenePrevFinal, gNdsMapsSelectTransitionSelectedGKind',
        'printf "FTR_DL_SCAN=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLScanResult, gNdsFighterMarioFoxDLScanSafeResult, gNdsFighterMarioFoxDLScanMask, gNdsFighterMarioFoxDLScanDeferredMask, gNdsFighterMarioFoxDLScanCount',
        'printf "FTR_DL_SCAN_STATS=%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterDLScanP0Blocker, gNdsFighterDLScanP1Blocker, gNdsFighterDLScanP0CommandCount, gNdsFighterDLScanP1CommandCount, gNdsFighterDLScanP0FirstOpcode, gNdsFighterDLScanP1FirstOpcode, gNdsFighterDLScanP0UnsupportedOpcode, gNdsFighterDLScanP1UnsupportedOpcode',
        'printf "FTR_DL_EXEC=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLExecResult, gNdsFighterMarioFoxDLExecSafeResult, gNdsFighterMarioFoxDLExecMask, gNdsFighterMarioFoxDLExecDeferredMask, gNdsFighterMarioFoxDLExecCount',
        'printf "FTR_DL_EXEC_STATS=%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterDLExecP0Blocker, gNdsFighterDLExecP1Blocker, gNdsFighterDLExecP0CommandCount, gNdsFighterDLExecP1CommandCount, gNdsFighterDLExecP0FirstOpcode, gNdsFighterDLExecP1FirstOpcode, gNdsFighterDLExecP0UnsupportedOpcode, gNdsFighterDLExecP1UnsupportedOpcode, gNdsFighterDLExecP0UnsupportedCommandCount, gNdsFighterDLExecP1UnsupportedCommandCount',
        'printf "FTR_DL_EXEC_GEOM=%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u\n", gNdsFighterDLExecP0VertexDecodedCount, gNdsFighterDLExecP1VertexDecodedCount, gNdsFighterDLExecP0VertexCommandCount, gNdsFighterDLExecP1VertexCommandCount, gNdsFighterDLExecP0VertexValidMask, gNdsFighterDLExecP1VertexValidMask, gNdsFighterDLExecP0TriangleCount, gNdsFighterDLExecP1TriangleCount, gNdsFighterDLExecP0TriangleValidCount, gNdsFighterDLExecP1TriangleValidCount',
        'printf "FTR_DL_DRAW=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLDrawResult, gNdsFighterMarioFoxDLDrawSafeResult, gNdsFighterMarioFoxDLDrawMask, gNdsFighterMarioFoxDLDrawDeferredMask, gNdsFighterMarioFoxDLDrawCount',
        'printf "FTR_DL_DRAW_PREVIEW=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLDrawPreviewWidth, gNdsFighterDLDrawPreviewHeight, gNdsFighterDLDrawPreviewPitch, gNdsFighterDLDrawPreviewReady, gNdsFighterDLDrawPreviewCommitBefore, gNdsFighterDLDrawPreviewCommitAfter, gNdsFighterDLDrawPreviewCommitDelta',
        'printf "FTR_DL_DRAW_STATS=%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterDLDrawP0Blocker, gNdsFighterDLDrawP1Blocker, gNdsFighterDLDrawP0CommandCount, gNdsFighterDLDrawP1CommandCount, gNdsFighterDLDrawP0FirstOpcode, gNdsFighterDLDrawP1FirstOpcode, gNdsFighterDLDrawP0UnsupportedOpcode, gNdsFighterDLDrawP1UnsupportedOpcode, gNdsFighterDLDrawP0UnsupportedCommandCount, gNdsFighterDLDrawP1UnsupportedCommandCount',
        'printf "FTR_DL_DRAW_GEOM=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLDrawP0VertexDecodedCount, gNdsFighterDLDrawP1VertexDecodedCount, gNdsFighterDLDrawP0TriangleCount, gNdsFighterDLDrawP1TriangleCount, gNdsFighterDLDrawP0TriangleValidCount, gNdsFighterDLDrawP1TriangleValidCount, gNdsFighterDLDrawP0TriangleDrawnCount, gNdsFighterDLDrawP1TriangleDrawnCount, gNdsFighterDLDrawP0PixelCount, gNdsFighterDLDrawP1PixelCount, gNdsFighterDLDrawTotalPixelCount',
        'printf "FTR_DL_DRAW_RENDER=%u,%u,%u,%u\n", gNdsFighterDLDrawP0RealTriangleDrawnCount, gNdsFighterDLDrawP1RealTriangleDrawnCount, gNdsFighterDLDrawP0MarkerTriangleDrawnCount, gNdsFighterDLDrawP1MarkerTriangleDrawnCount',
        'printf "FTR_DL_DRAW_AXIS=%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterDLDrawP0Axis, gNdsFighterDLDrawP1Axis, gNdsFighterDLDrawP0Area, gNdsFighterDLDrawP1Area, gNdsFighterDLDrawP0MinA, gNdsFighterDLDrawP0MaxA, gNdsFighterDLDrawP0MinB, gNdsFighterDLDrawP0MaxB, gNdsFighterDLDrawP1MinA, gNdsFighterDLDrawP1MaxA, gNdsFighterDLDrawP1MinB, gNdsFighterDLDrawP1MaxB',
        'printf "FTR_DL_DRAW_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%#x,%#x\n", gNdsFighterDLDrawP0ScreenMinX, gNdsFighterDLDrawP0ScreenMaxX, gNdsFighterDLDrawP0ScreenMinY, gNdsFighterDLDrawP0ScreenMaxY, gNdsFighterDLDrawP1ScreenMinX, gNdsFighterDLDrawP1ScreenMaxX, gNdsFighterDLDrawP1ScreenMinY, gNdsFighterDLDrawP1ScreenMaxY, gNdsFighterDLDrawP0ColorChecksum, gNdsFighterDLDrawP1ColorChecksum',
        'printf "FTR_DL_DRAW_STATE=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterDLDrawP0StatusAfter, gNdsFighterDLDrawP1StatusAfter, gNdsFighterDLDrawP0MotionAfter, gNdsFighterDLDrawP1MotionAfter, gNdsFighterDLDrawP0GAAfter, gNdsFighterDLDrawP1GAAfter, gNdsFighterDLDrawP0RootXBeforeBits, gNdsFighterDLDrawP0RootXAfterBits, gNdsFighterDLDrawP1RootXBeforeBits, gNdsFighterDLDrawP1RootXAfterBits',
        'printf "FTR_DL_DRAW_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDLDrawGObjDelta, gNdsFighterDLDrawDrawCallCount, gNdsFighterDLDrawMatrixCallCount, gNdsFighterDLDrawGameplayUpdateCount, gNdsFighterDLDrawRangeRejectCount, gNdsFighterDLDrawVertexRangeRejectCount',
        'printf "PLATFORM_DL_PREVIEW=%u,%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount, gNdsOriginalDLPreviewDrawCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $chain = [regex]::Match($gdbStdout, 'CHAIN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $final = [regex]::Match($gdbStdout, 'CHAIN_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $scan = [regex]::Match($gdbStdout, 'FTR_DL_SCAN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $scanStats = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_STATS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $exec = [regex]::Match($gdbStdout, 'FTR_DL_EXEC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $execStats = [regex]::Match($gdbStdout, 'FTR_DL_EXEC_STATS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $execGeom = [regex]::Match($gdbStdout, 'FTR_DL_EXEC_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $draw = [regex]::Match($gdbStdout, 'FTR_DL_DRAW=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $preview = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $drawStats = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_STATS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $drawGeom = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $drawRender = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $axis = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_AXIS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $screen = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $state = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $safe = [regex]::Match($gdbStdout, 'FTR_DL_DRAW_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 30 -and [int]$harn.Groups[3].Value -eq 9 -and [int]$harn.Groups[4].Value -eq 1 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Menu-chain Mario/Fox DL draw harness did not start at VSMode from Title.' $gdbStdout
    Assert-Condition ($chain.Success -and (Convert-MarkerUInt32 $chain.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $chain.Groups[2].Value) -band 0xff) -eq 0xff -and (Convert-MarkerUInt32 $chain.Groups[3].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $chain.Groups[4].Value) -band 0xff) -eq 0xff -and (Convert-MarkerUInt32 $chain.Groups[5].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $chain.Groups[6].Value) -band 0xff) -eq 0xff) 'Menu-chain transitions did not reach VSBattle.' $gdbStdout
    Assert-Condition ($final.Success -and [int]$final.Groups[1].Value -eq 16 -and [int]$final.Groups[2].Value -eq 21 -and [int]$final.Groups[3].Value -eq 22 -and [int]$final.Groups[4].Value -eq 21 -and [int]$final.Groups[5].Value -eq 6) 'Menu-chain final scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not VSBattle after menu chain.' $gdbStdout
    Assert-Condition ($scan.Success -and (Convert-MarkerUInt32 $scan.Groups[1].Value) -eq 0x46544c50 -and (Convert-MarkerUInt32 $scan.Groups[2].Value) -eq 0x46544c53 -and ((Convert-MarkerUInt32 $scan.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and [int]$scan.Groups[5].Value -eq 2) 'Prerequisite DL scan proof did not pass before menu-chain draw.' $gdbStdout
    Assert-Condition ($scanStats.Success -and [int]$scanStats.Groups[1].Value -eq 0 -and [int]$scanStats.Groups[2].Value -eq 0 -and (Convert-MarkerUInt32 $scanStats.Groups[7].Value) -eq 0 -and (Convert-MarkerUInt32 $scanStats.Groups[8].Value) -eq 0) 'Prerequisite menu-chain DL scan was not blocker-free.' $gdbStdout
    Assert-Condition ($exec.Success -and (Convert-MarkerUInt32 $exec.Groups[1].Value) -eq 0x46544c45 -and (Convert-MarkerUInt32 $exec.Groups[2].Value) -eq 0x46544c58 -and ((Convert-MarkerUInt32 $exec.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $exec.Groups[4].Value) -eq 0xff -and [int]$exec.Groups[5].Value -eq 2) 'Menu-chain DL execute proof did not pass before draw.' $gdbStdout
    Assert-Condition ($execStats.Success -and [int]$execStats.Groups[1].Value -eq 0 -and [int]$execStats.Groups[2].Value -eq 0 -and [int]$execStats.Groups[3].Value -ge 59 -and [int]$execStats.Groups[4].Value -ge 69 -and (Convert-MarkerUInt32 $execStats.Groups[7].Value) -eq 0 -and (Convert-MarkerUInt32 $execStats.Groups[8].Value) -eq 0 -and [int]$execStats.Groups[9].Value -eq 0 -and [int]$execStats.Groups[10].Value -eq 0) 'Menu-chain DL execute stats were not clean before draw.' $gdbStdout
    Assert-Condition ($execGeom.Success -and [int]$execGeom.Groups[1].Value -gt 0 -and [int]$execGeom.Groups[2].Value -gt 0 -and [int]$execGeom.Groups[7].Value -gt 0 -and [int]$execGeom.Groups[8].Value -gt 0 -and [int]$execGeom.Groups[9].Value -gt 0 -and [int]$execGeom.Groups[10].Value -gt 0) 'Menu-chain DL execute did not decode vertices and triangles before draw.' $gdbStdout
    Assert-Condition ($draw.Success -and (Convert-MarkerUInt32 $draw.Groups[1].Value) -eq 0x46544c44 -and (Convert-MarkerUInt32 $draw.Groups[2].Value) -eq 0x46544c57 -and ((Convert-MarkerUInt32 $draw.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $draw.Groups[4].Value) -eq 0xff -and [int]$draw.Groups[5].Value -eq 2) 'Menu-chain Mario/Fox DL draw proof did not pass.' $gdbStdout
    Assert-Condition ($preview.Success -and [int]$preview.Groups[1].Value -eq 96 -and [int]$preview.Groups[2].Value -eq 72 -and [int]$preview.Groups[3].Value -ge 96 -and [int]$preview.Groups[4].Value -eq 1 -and [int]$preview.Groups[6].Value -gt [int]$preview.Groups[5].Value -and [int]$preview.Groups[7].Value -eq 1) 'Menu-chain DL draw preview was not committed as a 96x72 retained surface.' $gdbStdout
    Assert-Condition ($platform.Success -and [int]$platform.Groups[1].Value -eq 1 -and [int]$platform.Groups[2].Value -eq 96 -and [int]$platform.Groups[3].Value -eq 72 -and [int]$platform.Groups[4].Value -ge [int]$preview.Groups[6].Value) 'Menu-chain platform original-DL preview state is not ready after draw.' $gdbStdout
    Assert-Condition ($drawStats.Success -and [int]$drawStats.Groups[1].Value -eq 0 -and [int]$drawStats.Groups[2].Value -eq 0 -and [int]$drawStats.Groups[3].Value -ge 59 -and [int]$drawStats.Groups[4].Value -ge 69 -and (Convert-MarkerUInt32 $drawStats.Groups[5].Value) -ne 0 -and (Convert-MarkerUInt32 $drawStats.Groups[6].Value) -ne 0 -and (Convert-MarkerUInt32 $drawStats.Groups[7].Value) -eq 0 -and (Convert-MarkerUInt32 $drawStats.Groups[8].Value) -eq 0 -and [int]$drawStats.Groups[9].Value -eq 0 -and [int]$drawStats.Groups[10].Value -eq 0) 'Menu-chain DL draw stats were not clean.' $gdbStdout
    Assert-Condition ($drawGeom.Success -and [int]$drawGeom.Groups[1].Value -gt 0 -and [int]$drawGeom.Groups[2].Value -gt 0 -and [int]$drawGeom.Groups[3].Value -gt 0 -and [int]$drawGeom.Groups[4].Value -gt 0 -and [int]$drawGeom.Groups[5].Value -gt 0 -and [int]$drawGeom.Groups[6].Value -gt 0 -and [int]$drawGeom.Groups[7].Value -gt 0 -and [int]$drawGeom.Groups[8].Value -gt 0 -and [int]$drawGeom.Groups[9].Value -gt 0 -and [int]$drawGeom.Groups[10].Value -gt 0 -and [int]$drawGeom.Groups[11].Value -gt 0) 'Menu-chain DL draw did not produce visible software pixels for both fighters.' $gdbStdout
    Assert-Condition ($drawRender.Success -and [int]$drawRender.Groups[1].Value -gt 0 -and [int]$drawRender.Groups[2].Value -gt 0) 'Menu-chain DL draw did not produce real non-degenerate triangles for both fighters.' $gdbStdout
    Assert-Condition ($axis.Success -and [int]$axis.Groups[1].Value -le 2 -and [int]$axis.Groups[2].Value -le 2 -and [int]$axis.Groups[3].Value -gt 0 -and [int]$axis.Groups[4].Value -gt 0 -and (([int]$axis.Groups[6].Value -ne [int]$axis.Groups[5].Value) -or ([int]$axis.Groups[8].Value -ne [int]$axis.Groups[7].Value)) -and (([int]$axis.Groups[10].Value -ne [int]$axis.Groups[9].Value) -or ([int]$axis.Groups[12].Value -ne [int]$axis.Groups[11].Value))) 'Menu-chain DL draw projection axes/bounds were not usable.' $gdbStdout
    Assert-Condition ($screen.Success -and [int]$screen.Groups[1].Value -ge 0 -and [int]$screen.Groups[2].Value -le 95 -and [int]$screen.Groups[3].Value -ge 0 -and [int]$screen.Groups[4].Value -le 71 -and [int]$screen.Groups[5].Value -ge 0 -and [int]$screen.Groups[6].Value -le 95 -and [int]$screen.Groups[7].Value -ge 0 -and [int]$screen.Groups[8].Value -le 71 -and [int]$screen.Groups[2].Value -ge [int]$screen.Groups[1].Value -and [int]$screen.Groups[6].Value -ge [int]$screen.Groups[5].Value -and (Convert-MarkerUInt32 $screen.Groups[9].Value) -ne 0 -and (Convert-MarkerUInt32 $screen.Groups[10].Value) -ne 0) 'Menu-chain DL draw screen bounds/checksums were not valid.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 10 -and [int]$state.Groups[2].Value -eq 10 -and [int]$state.Groups[3].Value -eq 4 -and [int]$state.Groups[4].Value -eq 4 -and [int]$state.Groups[5].Value -eq 0 -and [int]$state.Groups[6].Value -eq 0 -and (Convert-MarkerUInt32 $state.Groups[7].Value) -eq (Convert-MarkerUInt32 $state.Groups[8].Value) -and (Convert-MarkerUInt32 $state.Groups[9].Value) -eq (Convert-MarkerUInt32 $state.Groups[10].Value)) 'Menu-chain DL draw mutated fighter state or root position.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0 -and [int]$safe.Groups[5].Value -eq 0 -and [int]$safe.Groups[6].Value -eq 0) 'Menu-chain DL draw escaped bounded software-preview behavior.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after menu-chain draw.' $gdbStdout
    Write-Output ("Menu-chain Mario/Fox DL draw harness passed: chain final=22/21 pixels={0}/{1} tris={2}/{3} real={4}/{5} marker={6}/{7} preview=96x72 commit=1 safe=1" -f $drawGeom.Groups[9].Value, $drawGeom.Groups[10].Value, $drawGeom.Groups[3].Value, $drawGeom.Groups[4].Value, $drawRender.Groups[1].Value, $drawRender.Groups[2].Value, $drawRender.Groups[3].Value, $drawRender.Groups[4].Value)
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
