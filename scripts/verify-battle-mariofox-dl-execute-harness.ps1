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
$rom = Join-Path $root 'smash64ds-battle-mariofox-dl-execute.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-dl-execute.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-dl-execute-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-dl-execute-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_dl_execute_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-dl-execute BUILD=build-battle-mariofox-dl-execute-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_execute -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox DL execute harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_DL_SCAN=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLScanResult, gNdsFighterMarioFoxDLScanSafeResult, gNdsFighterMarioFoxDLScanMask, gNdsFighterMarioFoxDLScanDeferredMask, gNdsFighterMarioFoxDLScanCount',
        'printf "FTR_DL_SCAN_STATS=%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterDLScanP0Blocker, gNdsFighterDLScanP1Blocker, gNdsFighterDLScanP0CommandCount, gNdsFighterDLScanP1CommandCount, gNdsFighterDLScanP0FirstOpcode, gNdsFighterDLScanP1FirstOpcode, gNdsFighterDLScanP0UnsupportedOpcode, gNdsFighterDLScanP1UnsupportedOpcode',
        'printf "FTR_DL_EXEC=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLExecResult, gNdsFighterMarioFoxDLExecSafeResult, gNdsFighterMarioFoxDLExecMask, gNdsFighterMarioFoxDLExecDeferredMask, gNdsFighterMarioFoxDLExecCount',
        'printf "FTR_DL_EXEC_STATS=%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterDLExecP0Blocker, gNdsFighterDLExecP1Blocker, gNdsFighterDLExecP0CommandCount, gNdsFighterDLExecP1CommandCount, gNdsFighterDLExecP0FirstOpcode, gNdsFighterDLExecP1FirstOpcode, gNdsFighterDLExecP0UnsupportedOpcode, gNdsFighterDLExecP1UnsupportedOpcode, gNdsFighterDLExecP0UnsupportedCommandCount, gNdsFighterDLExecP1UnsupportedCommandCount',
        'printf "FTR_DL_EXEC_GEOM=%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u\n", gNdsFighterDLExecP0VertexDecodedCount, gNdsFighterDLExecP1VertexDecodedCount, gNdsFighterDLExecP0VertexCommandCount, gNdsFighterDLExecP1VertexCommandCount, gNdsFighterDLExecP0VertexValidMask, gNdsFighterDLExecP1VertexValidMask, gNdsFighterDLExecP0TriangleCount, gNdsFighterDLExecP1TriangleCount, gNdsFighterDLExecP0TriangleValidCount, gNdsFighterDLExecP1TriangleValidCount',
        'printf "FTR_DL_EXEC_BOUNDS=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%#x,%#x\n", gNdsFighterDLExecP0MinX, gNdsFighterDLExecP0MaxX, gNdsFighterDLExecP0MinY, gNdsFighterDLExecP0MaxY, gNdsFighterDLExecP0MinZ, gNdsFighterDLExecP0MaxZ, gNdsFighterDLExecP1MinX, gNdsFighterDLExecP1MaxX, gNdsFighterDLExecP1MinY, gNdsFighterDLExecP1MaxY, gNdsFighterDLExecP1MinZ, gNdsFighterDLExecP1MaxZ, gNdsFighterDLExecP0ColorChecksum, gNdsFighterDLExecP1ColorChecksum',
        'printf "FTR_DL_EXEC_FAMILIES=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterDLExecP0OtherModeCommandCount, gNdsFighterDLExecP1OtherModeCommandCount, gNdsFighterDLExecP0CullCommandCount, gNdsFighterDLExecP1CullCommandCount, gNdsFighterDLExecP0StateCommandCount, gNdsFighterDLExecP1StateCommandCount, gNdsFighterDLExecP0RenderCommandCount, gNdsFighterDLExecP1RenderCommandCount, gNdsFighterDLExecP0BranchCommandCount, gNdsFighterDLExecP1BranchCommandCount, gNdsFighterDLExecP0TextureMask, gNdsFighterDLExecP1TextureMask',
        'printf "FTR_DL_EXEC_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDLExecGObjDelta, gNdsFighterDLExecDrawCallCount, gNdsFighterDLExecMatrixCallCount, gNdsFighterDLExecGameplayUpdateCount, gNdsFighterDLExecRangeRejectCount, gNdsFighterDLExecVertexRangeRejectCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $scan = [regex]::Match($gdbStdout, 'FTR_DL_SCAN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $scanStats = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_STATS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $exec = [regex]::Match($gdbStdout, 'FTR_DL_EXEC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stats = [regex]::Match($gdbStdout, 'FTR_DL_EXEC_STATS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $geom = [regex]::Match($gdbStdout, 'FTR_DL_EXEC_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $bounds = [regex]::Match($gdbStdout, 'FTR_DL_EXEC_BOUNDS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $families = [regex]::Match($gdbStdout, 'FTR_DL_EXEC_FAMILIES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $safe = [regex]::Match($gdbStdout, 'FTR_DL_EXEC_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 27 -and [int]$harn.Groups[3].Value -eq 22 -and [int]$harn.Groups[4].Value -eq 21 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Battle Mario/Fox DL execute harness did not select direct VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scan.Success -and (Convert-MarkerUInt32 $scan.Groups[1].Value) -eq 0x46544c50 -and (Convert-MarkerUInt32 $scan.Groups[2].Value) -eq 0x46544c53 -and ((Convert-MarkerUInt32 $scan.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and [int]$scan.Groups[5].Value -eq 2) 'Prerequisite DL scan proof did not pass before execute.' $gdbStdout
    Assert-Condition ($scanStats.Success -and [int]$scanStats.Groups[1].Value -eq 0 -and [int]$scanStats.Groups[2].Value -eq 0 -and (Convert-MarkerUInt32 $scanStats.Groups[7].Value) -eq 0 -and (Convert-MarkerUInt32 $scanStats.Groups[8].Value) -eq 0) 'Prerequisite DL scan was not blocker-free.' $gdbStdout
    Assert-Condition ($exec.Success -and (Convert-MarkerUInt32 $exec.Groups[1].Value) -eq 0x46544c45 -and (Convert-MarkerUInt32 $exec.Groups[2].Value) -eq 0x46544c58 -and ((Convert-MarkerUInt32 $exec.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $exec.Groups[4].Value) -eq 0xff -and [int]$exec.Groups[5].Value -eq 2) 'Mario/Fox DL execute proof did not pass.' $gdbStdout
    Assert-Condition ($stats.Success -and [int]$stats.Groups[1].Value -eq 0 -and [int]$stats.Groups[2].Value -eq 0 -and [int]$stats.Groups[3].Value -ge 59 -and [int]$stats.Groups[4].Value -ge 69 -and (Convert-MarkerUInt32 $stats.Groups[7].Value) -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[8].Value) -eq 0 -and [int]$stats.Groups[9].Value -eq 0 -and [int]$stats.Groups[10].Value -eq 0) 'DL execute stats were not clean.' $gdbStdout
    Assert-Condition ($geom.Success -and [int]$geom.Groups[1].Value -gt 0 -and [int]$geom.Groups[2].Value -gt 0 -and [int]$geom.Groups[3].Value -gt 0 -and [int]$geom.Groups[4].Value -gt 0 -and (Convert-MarkerUInt32 $geom.Groups[5].Value) -ne 0 -and (Convert-MarkerUInt32 $geom.Groups[6].Value) -ne 0 -and [int]$geom.Groups[7].Value -gt 0 -and [int]$geom.Groups[8].Value -gt 0 -and [int]$geom.Groups[9].Value -gt 0 -and [int]$geom.Groups[10].Value -gt 0) 'DL execute did not decode vertices and triangles.' $gdbStdout
    Assert-Condition ($bounds.Success -and ([int]$bounds.Groups[1].Value -ne [int]$bounds.Groups[2].Value -or [int]$bounds.Groups[3].Value -ne [int]$bounds.Groups[4].Value -or [int]$bounds.Groups[5].Value -ne [int]$bounds.Groups[6].Value) -and ([int]$bounds.Groups[7].Value -ne [int]$bounds.Groups[8].Value -or [int]$bounds.Groups[9].Value -ne [int]$bounds.Groups[10].Value -or [int]$bounds.Groups[11].Value -ne [int]$bounds.Groups[12].Value) -and (Convert-MarkerUInt32 $bounds.Groups[13].Value) -ne 0 -and (Convert-MarkerUInt32 $bounds.Groups[14].Value) -ne 0) 'DL execute bounds/checksum were not populated.' $gdbStdout
    Assert-Condition ($families.Success -and [int]$families.Groups[5].Value -gt 0 -and [int]$families.Groups[6].Value -gt 0 -and [int]$families.Groups[7].Value -gt 0 -and [int]$families.Groups[8].Value -gt 0 -and (Convert-MarkerUInt32 $families.Groups[11].Value) -ne 0 -and (Convert-MarkerUInt32 $families.Groups[12].Value) -ne 0) 'DL execute command-family counters were not populated.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0 -and [int]$safe.Groups[5].Value -eq 0 -and [int]$safe.Groups[6].Value -eq 0) 'DL execute escaped bounded decode-only behavior.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary.' $gdbStdout
    Write-Output ("Battle Mario/Fox DL execute harness passed: commands={0}/{1} verts={2}/{3} tris={4}/{5} safe=1" -f $stats.Groups[3].Value, $stats.Groups[4].Value, $geom.Groups[1].Value, $geom.Groups[2].Value, $geom.Groups[7].Value, $geom.Groups[8].Value)
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
