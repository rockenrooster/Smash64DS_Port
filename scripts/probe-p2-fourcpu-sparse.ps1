[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4613,
    [int]$RunnerSlot = -1,
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [switch]$NoBuild,
    [ValidateRange(32,2048)][int]$Frame = 32,
    [switch]$FirstProductionAfterFrame,
    [ValidateRange(30,900)][int]$TimeoutSeconds = 300,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (($Frame % 32) -ne 0) {
    throw '-Frame must be a multiple of 32 because the stress marker is sparse.'
}
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root "artifacts\verification\p2-2-fourcpu-sparse$Frame.txt"
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'p2-2-fourcpu-sparse.gdb'
$gdbOut = Join-Path $temp 'p2-2-fourcpu-sparse.gdb.out'
$gdbErr = Join-Path $temp 'p2-2-fourcpu-sparse.gdb.err'
$emulatorOut = Join-Path $temp 'p2-2-fourcpu-sparse.melonds.out'
$emulatorErr = Join-Path $temp 'p2-2-fourcpu-sparse.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$Target" "BUILD=$Build"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required probe input does not exist: $path"
        }
    }

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue

    # Sparse debugger stops only.  The stress target emits this marker every 32
    # presented frames, so reaching frame 128 costs four host stops rather than
    # 128 conditional per-frame stops.  This is a diagnosis probe, not a
    # percentile gate.
    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsBattlePlayableTickHudSparseMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $Frame",
        'continue',
        'end',
        'end',
        'continue',
        ('printf "SPARSE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsBattlePlayablePacingPresentedFrames, ' +
         'gNdsBattlePlayablePacingLogicFrames, ' +
         'gNdsTickHudBuckets[0], gNdsTickHudBuckets[1], ' +
         'gNdsTickHudBuckets[2], gNdsTickHudBuckets[3], ' +
         'gNdsTickHudBuckets[6], gNdsTickHudBuckets[10], ' +
         'gNdsFtrPlanBuild, gNdsFtrPlanHit, ' +
         'gNdsFighterDLAllDrawP0HardwareTriangleCount, ' +
         'gNdsFighterDLAllDrawP1HardwareTriangleCount'),
        ('printf "DETAIL=%u,%u,%u,%u,%u\n", ' +
         'gNdsRendererFastRunMode, gNdsFighterStructP0Detail, ' +
         'gNdsFighterStructP1Detail, ' +
         'sNdsNativeFighterDenseNormalsBuiltLow, ' +
         'sNdsNativeFighterDenseNormalsBuilt'),
        ('printf "TABLE=%p,%p,%p\n", ' +
         'sNdsNativeFighterActiveTables, ' +
         '&sNdsNativeFighterLowTables, &sNdsNativeFighterHighTables'),
        ('printf "MEM=%u,%u,%u\n", ' +
         'gNdsTaskmanGeneralHeapFreeMin, gNdsTaskmanArenaChosenSize, ' +
         'gNdsTaskmanArenaAllocFailCount'),
        # Scene residency is allocated before the first sparse checkpoint and
        # the memory ledger keeps a high-water. Capture it with the same
        # four-fighter binary so P2-2 can publish an arena/reloc budget without
        # dividing shared Mario/Fox files by instance count.
        ('printf "MEMLEDGER=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater, ' +
         'gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerRelocBytes, ' +
         'gNdsMemoryLedgerRelocStageBytes, gNdsMemoryLedgerRelocFighterBytes, ' +
         'gNdsMemoryLedgerRelocInterfaceBytes, gNdsMemoryLedgerRelocMenuBytes, ' +
         'gNdsMemoryLedgerRelocOpeningBytes, gNdsMemoryLedgerRelocOtherBytes, ' +
         'gNdsMemoryLedgerRelocStaleBytes'),
        ('printf "WALL=%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsSObjWallpaperCacheBuildCount, gNdsSObjWallpaperCacheHitCount, ' +
         'gNdsSObjWallpaperCacheFastDrawCount, ' +
         'gNdsSObjWallpaperCacheFallbackCount, gNdsSObjWallpaperCacheWidth, ' +
         'gNdsSObjWallpaperCacheHeight, gNdsSObjWallpaperCacheOpaquePixels'),
        ('printf "WALLFINAL=%u,%u,%u,%u\n", ' +
         'gNdsSObjWallpaperFinalDirectCount, gNdsSObjWallpaperFinalSkipCount, ' +
         'gNdsSObjWallpaperFinalKeyChangeCount, ' +
         'gNdsSObjWallpaperFinalPixelWriteCount')
    )
    if ($FirstProductionAfterFrame) {
        # Install this only after the sparse checkpoint. Breakpointing production
        # from boot would stop on menu/seed work and would not answer whether the
        # four-CPU battle's first real owner uses the source-required Low tree.
        $gdbLines += @(
            'delete',
            'break ndsRendererExecuteNativeFighterOwnerProduction',
            'continue',
            ('printf "PRODUCTION=%u,%u,%u,%u,%u,%u,%u\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, ' +
             'gNdsBattlePlayablePacingLogicFrames, slot, use_low_detail, ' +
             'input_count, gNdsFighterStructP0Detail, gNdsFighterStructP1Detail'),
            ('printf "PRODTABLE=%p,%p,%p\n", ' +
             'sNdsNativeFighterActiveTables, ' +
             '&sNdsNativeFighterLowTables, &sNdsNativeFighterHighTables')
        )
    }
    $gdbLines += @('detach', 'quit')
    [System.IO.File]::WriteAllLines($gdbScript, $gdbLines)

    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "P2-2 sparse frame-32 probe exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        throw "P2-2 sparse GDB probe failed: $(Get-Content $gdbErr -Raw)"
    }
    $output = Get-Content $gdbOut -Raw
    if ($output -notmatch ("SPARSE=$Frame,")) {
        throw "Sparse probe did not stop on presented frame ${Frame}:`n$output"
    }
    if ($FirstProductionAfterFrame -and ($output -notmatch 'PRODUCTION=')) {
        throw "Sparse probe never reached fighter production after frame ${Frame}:`n$output"
    }

    $artifactDir = Split-Path -Parent $Artifact
    if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
    Set-Content -LiteralPath $Artifact -Value $output
    Write-Output $output
    Write-Output "Wrote $Artifact"
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
}
