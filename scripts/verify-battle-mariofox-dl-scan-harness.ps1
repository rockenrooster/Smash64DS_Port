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
$rom = Join-Path $root 'smash64ds-battle-mariofox-dl-scan.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-dl-scan.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-dl-scan-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-dl-scan-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_dl_scan_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-dl-scan BUILD=build-battle-mariofox-dl-scan-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_scan -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox DL scan harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_DISPLAY=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDisplayResult, gNdsFighterMarioFoxDisplaySafeResult, gNdsFighterMarioFoxDisplayMask, gNdsFighterMarioFoxDisplayDeferredMask, gNdsFighterMarioFoxDisplayCallbackCount',
        'printf "FTR_DISPLAY_COUNTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDisplayP0DObjCount, gNdsFighterDisplayP1DObjCount, gNdsFighterDisplayP0MObjCount, gNdsFighterDisplayP1MObjCount, gNdsFighterDisplayP0AObjCount, gNdsFighterDisplayP1AObjCount, gNdsFighterDisplayP0DLReadyCount, gNdsFighterDisplayP1DLReadyCount, gNdsFighterDisplayP0PartsPtrCount, gNdsFighterDisplayP1PartsPtrCount',
        'printf "FTR_DL_SCAN=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLScanResult, gNdsFighterMarioFoxDLScanSafeResult, gNdsFighterMarioFoxDLScanMask, gNdsFighterMarioFoxDLScanDeferredMask, gNdsFighterMarioFoxDLScanCount',
        'printf "FTR_DL_SCAN_PTRS=%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLScanP0FirstDL, gNdsFighterDLScanP1FirstDL, gNdsFighterDLScanP0AssetID, gNdsFighterDLScanP1AssetID, gNdsFighterDLScanP0Offset, gNdsFighterDLScanP1Offset, gNdsFighterDLScanP0DObjIndex, gNdsFighterDLScanP1DObjIndex',
        'printf "FTR_DL_SCAN_STATS=%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterDLScanP0Blocker, gNdsFighterDLScanP1Blocker, gNdsFighterDLScanP0CommandCount, gNdsFighterDLScanP1CommandCount, gNdsFighterDLScanP0FirstOpcode, gNdsFighterDLScanP1FirstOpcode, gNdsFighterDLScanP0UnsupportedOpcode, gNdsFighterDLScanP1UnsupportedOpcode',
        'printf "FTR_DL_SCAN_SHAPE=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterDLScanP0VertexCount, gNdsFighterDLScanP1VertexCount, gNdsFighterDLScanP0TriangleCount, gNdsFighterDLScanP1TriangleCount, gNdsFighterDLScanP0EndCommandCount, gNdsFighterDLScanP1EndCommandCount, gNdsFighterDLScanP0BranchCommandCount, gNdsFighterDLScanP1BranchCommandCount, gNdsFighterDLScanP0TextureMask, gNdsFighterDLScanP1TextureMask',
        'printf "FTR_DL_SCAN_FAMILIES=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLScanP0UnsupportedCommandCount, gNdsFighterDLScanP1UnsupportedCommandCount, gNdsFighterDLScanP0VertexCommandCount, gNdsFighterDLScanP1VertexCommandCount, gNdsFighterDLScanP0TriangleCommandCount, gNdsFighterDLScanP1TriangleCommandCount, gNdsFighterDLScanP0OtherModeCommandCount, gNdsFighterDLScanP1OtherModeCommandCount, gNdsFighterDLScanP0CullCommandCount, gNdsFighterDLScanP1CullCommandCount, gNdsFighterDLScanP0StateCommandCount, gNdsFighterDLScanP1StateCommandCount, gNdsFighterDLScanP0RenderCommandCount, gNdsFighterDLScanP1RenderCommandCount',
        'printf "FTR_DL_SCAN_STATE=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterDLScanP0StatusAfter, gNdsFighterDLScanP1StatusAfter, gNdsFighterDLScanP0MotionAfter, gNdsFighterDLScanP1MotionAfter, gNdsFighterDLScanP0GAAfter, gNdsFighterDLScanP1GAAfter, gNdsFighterDLScanP0RootXBeforeBits, gNdsFighterDLScanP0RootXAfterBits, gNdsFighterDLScanP1RootXBeforeBits, gNdsFighterDLScanP1RootXAfterBits',
        'printf "FTR_DL_SCAN_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDLScanGObjDelta, gNdsFighterDLScanDrawCallCount, gNdsFighterDLScanMatrixCallCount, gNdsFighterDLScanGameplayUpdateCount, gNdsFighterDLScanRangeRejectCount, gNdsFighterDLScanBranchResolveCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $display = [regex]::Match($gdbStdout, 'FTR_DISPLAY=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $counts = [regex]::Match($gdbStdout, 'FTR_DISPLAY_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $scan = [regex]::Match($gdbStdout, 'FTR_DL_SCAN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $ptrs = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_PTRS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stats = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_STATS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $shape = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_SHAPE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $families = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_FAMILIES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $state = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $safe = [regex]::Match($gdbStdout, 'FTR_DL_SCAN_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 25 -and [int]$harn.Groups[3].Value -eq 22 -and [int]$harn.Groups[4].Value -eq 21 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Battle Mario/Fox DL scan harness did not select direct VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($display.Success -and (Convert-MarkerUInt32 $display.Groups[1].Value) -eq 0x46544450 -and (Convert-MarkerUInt32 $display.Groups[2].Value) -eq 0x46544453 -and ((Convert-MarkerUInt32 $display.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $display.Groups[4].Value) -eq 0x3f -and [int]$display.Groups[5].Value -eq 2) 'Display metadata proof did not pass before DL scan.' $gdbStdout
    Assert-Condition ($counts.Success -and [int]$counts.Groups[1].Value -eq 25 -and [int]$counts.Groups[2].Value -eq 27 -and [int]$counts.Groups[3].Value -eq 0 -and [int]$counts.Groups[4].Value -eq 0 -and [int]$counts.Groups[5].Value -eq 0 -and [int]$counts.Groups[6].Value -eq 0 -and [int]$counts.Groups[7].Value -eq 14 -and [int]$counts.Groups[8].Value -eq 18) 'Display metadata counts changed before direct DL scan.' $gdbStdout
    Assert-Condition ($scan.Success -and (Convert-MarkerUInt32 $scan.Groups[1].Value) -eq 0x46544c50 -and (Convert-MarkerUInt32 $scan.Groups[2].Value) -eq 0x46544c53 -and ((Convert-MarkerUInt32 $scan.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $scan.Groups[4].Value) -eq 0xff -and [int]$scan.Groups[5].Value -eq 2) 'Mario/Fox DL scan proof did not pass.' $gdbStdout
    Assert-Condition ($ptrs.Success -and (Convert-MarkerUInt32 $ptrs.Groups[1].Value) -ne 0 -and (Convert-MarkerUInt32 $ptrs.Groups[2].Value) -ne 0 -and [uint32]$ptrs.Groups[3].Value -ne [uint32]::MaxValue -and [uint32]$ptrs.Groups[4].Value -ne [uint32]::MaxValue -and [uint32]$ptrs.Groups[5].Value -ne [uint32]::MaxValue -and [uint32]$ptrs.Groups[6].Value -ne [uint32]::MaxValue) 'DL scan did not find loaded-file-owned display lists.' $gdbStdout
    Assert-Condition ($stats.Success -and [int]$stats.Groups[1].Value -eq 0 -and [int]$stats.Groups[2].Value -eq 0 -and [int]$stats.Groups[3].Value -ge 59 -and [int]$stats.Groups[4].Value -ge 69 -and (Convert-MarkerUInt32 $stats.Groups[5].Value) -ne 0 -and (Convert-MarkerUInt32 $stats.Groups[6].Value) -ne 0 -and (Convert-MarkerUInt32 $stats.Groups[7].Value) -eq 0 -and (Convert-MarkerUInt32 $stats.Groups[8].Value) -eq 0) 'DL scan parser did not finish cleanly.' $gdbStdout
    Assert-Condition ($shape.Success -and [int]$shape.Groups[1].Value -gt 0 -and [int]$shape.Groups[2].Value -gt 0 -and [int]$shape.Groups[3].Value -gt 0 -and [int]$shape.Groups[4].Value -gt 0 -and [int]$shape.Groups[5].Value -gt 0 -and [int]$shape.Groups[6].Value -gt 0) 'DL scan did not record bounded geometry shape.' $gdbStdout
    Assert-Condition ($families.Success -and [int]$families.Groups[1].Value -eq 0 -and [int]$families.Groups[2].Value -eq 0 -and [int]$families.Groups[3].Value -gt 0 -and [int]$families.Groups[4].Value -gt 0 -and [int]$families.Groups[5].Value -gt 0 -and [int]$families.Groups[6].Value -gt 0 -and [int]$families.Groups[11].Value -gt 0 -and [int]$families.Groups[12].Value -gt 0 -and [int]$families.Groups[13].Value -gt 0 -and [int]$families.Groups[14].Value -gt 0) 'DL scan command-family counters were not populated cleanly.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 10 -and [int]$state.Groups[2].Value -eq 10 -and [int]$state.Groups[3].Value -eq 4 -and [int]$state.Groups[4].Value -eq 4 -and [int]$state.Groups[5].Value -eq 0 -and [int]$state.Groups[6].Value -eq 0 -and (Convert-MarkerUInt32 $state.Groups[7].Value) -eq (Convert-MarkerUInt32 $state.Groups[8].Value) -and (Convert-MarkerUInt32 $state.Groups[9].Value) -eq (Convert-MarkerUInt32 $state.Groups[10].Value)) 'DL scan changed fighter state or root position.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0) 'DL scan escaped into object creation, draw, matrix, or gameplay behavior.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary.' $gdbStdout
    Write-Output ("Battle Mario/Fox DL scan harness passed: scene=22/21 dl={0}/{1} asset={2}/{3} commands={4}/{5} blocker={6}/{7} safe=1" -f $ptrs.Groups[1].Value, $ptrs.Groups[2].Value, $ptrs.Groups[3].Value, $ptrs.Groups[4].Value, $stats.Groups[3].Value, $stats.Groups[4].Value, $stats.Groups[1].Value, $stats.Groups[2].Value)
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
