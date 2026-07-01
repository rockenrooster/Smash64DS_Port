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
$rom = Join-Path $root 'smash64ds-battle-mariofox-model.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-model.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-model-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-model-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_model_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-model BUILD=build-battle-mariofox-model-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_model -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox model harness build did not produce the expected ROM and ELF.'
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
        'printf "VSB=%#x,%#x,%#x,%u,%u,%u\n", gNdsSCVSBattleOriginalSetupResult, gNdsSCVSBattleOriginalSetupMask, gNdsSCVSBattleOriginalUpdateResult, gNdsSCVSBattleOriginalPlayerCount, gNdsSCVSBattleOriginalFighterCreateCount, gNdsSCVSBattleOriginalGKind',
        'printf "FTR_RELOC=%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxRelocResult, gNdsFighterMarioFoxRelocAssetMask, gNdsFighterMarioFoxRelocDependencyMask, gNdsFighterMarioFoxLoadedFileCount, gNdsFighterMarioFoxExternalFixupCount, gNdsFighterMarioFoxExternalFixupFailCount',
        'printf "FTR_PTRS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterMarioMainMotionPtrReady, gNdsFighterMarioMainPtrReady, gNdsFighterMarioModelPtrReady, gNdsFighterMarioAttrPtrReady, gNdsFighterMarioCommonPartsReady, gNdsFighterFoxMainMotionPtrReady, gNdsFighterFoxMainPtrReady, gNdsFighterFoxModelPtrReady, gNdsFighterFoxAttrPtrReady, gNdsFighterFoxCommonPartsReady',
        'printf "FTR_MODEL=%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxSetupMask, gNdsFighterModelRealGObjCount, gNdsFighterModelStubGObjCount, gNdsFighterModelProcessDeferredCount',
        'printf "FTR_P0=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterModelP0FKind, gNdsFighterModelP0GObjID, gNdsFighterModelP0TopDObjReady, gNdsFighterModelP0ModelDObjCount, gNdsFighterModelP0MObjCount, gNdsFighterModelP0AObjCount, gNdsFighterModelP0DisplayAttached',
        'printf "FTR_P1=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterModelP1FKind, gNdsFighterModelP1GObjID, gNdsFighterModelP1TopDObjReady, gNdsFighterModelP1ModelDObjCount, gNdsFighterModelP1MObjCount, gNdsFighterModelP1AObjCount, gNdsFighterModelP1DisplayAttached',
        'printf "PUPUPU_UPDATE_ZERO=%#x,%u\n", gNdsPupupuUpdateResult, gNdsPupupuUpdateTickCount',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vsb = [regex]::Match($gdbStdout, 'VSB=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $reloc = [regex]::Match($gdbStdout, 'FTR_RELOC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $ptrs = [regex]::Match($gdbStdout, 'FTR_PTRS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $model = [regex]::Match($gdbStdout, 'FTR_MODEL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $p0 = [regex]::Match($gdbStdout, 'FTR_P0=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $p1 = [regex]::Match($gdbStdout, 'FTR_P1=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $updateZero = [regex]::Match($gdbStdout, 'PUPUPU_UPDATE_ZERO=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 11 -or [int]$harn.Groups[3].Value -ne 22 -or [int]$harn.Groups[4].Value -ne 21 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle Mario/Fox model harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $vsb.Success -or (Convert-MarkerUInt32 $vsb.Groups[1].Value) -ne 0x56425355 -or ((Convert-MarkerUInt32 $vsb.Groups[2].Value) -band 0x7f) -ne 0x7f -or (Convert-MarkerUInt32 $vsb.Groups[3].Value) -ne 0x56425550 -or [int]$vsb.Groups[4].Value -ne 2 -or [int]$vsb.Groups[5].Value -ne 2 -or [int]$vsb.Groups[6].Value -ne 6) {
        throw "Imported VSBattle setup did not reach the Mario/Fox model proof boundary.`n$gdbStdout"
    }
    if (-not $reloc.Success -or (Convert-MarkerUInt32 $reloc.Groups[1].Value) -ne 0x4654524c -or ((Convert-MarkerUInt32 $reloc.Groups[2].Value) -band 0x7fff) -ne 0x7fff -or [int]$reloc.Groups[6].Value -ne 0) {
        throw "Mario/Fox fighter relocation proof failed.`n$gdbStdout"
    }
    if (-not $ptrs.Success) { throw "Mario/Fox pointer diagnostics missing.`n$gdbStdout" }
    for ($i = 1; $i -le 10; $i++) {
        if ([int]$ptrs.Groups[$i].Value -ne 1) {
            throw "Mario/Fox asset pointer readiness failed.`n$gdbStdout"
        }
    }
    if (-not $model.Success -or (Convert-MarkerUInt32 $model.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $model.Groups[2].Value) -ne 0x4654474f -or ((Convert-MarkerUInt32 $model.Groups[3].Value) -band 0xfff) -ne 0xfff -or [int]$model.Groups[4].Value -ne 2 -or [int]$model.Groups[5].Value -ne 0 -or [int]$model.Groups[6].Value -lt 2) {
        throw "Mario/Fox model GObj proof failed or fell back to stubs.`n$gdbStdout"
    }
    if (-not $p0.Success -or [int]$p0.Groups[1].Value -ne 0 -or [int]$p0.Groups[2].Value -eq 0 -or [int]$p0.Groups[3].Value -ne 1 -or [int]$p0.Groups[4].Value -lt 1 -or [int]$p0.Groups[7].Value -ne 1) {
        throw "Mario model object proof failed.`n$gdbStdout"
    }
    if (-not $p1.Success -or [int]$p1.Groups[1].Value -ne 1 -or [int]$p1.Groups[2].Value -eq 0 -or [int]$p1.Groups[3].Value -ne 1 -or [int]$p1.Groups[4].Value -lt 1 -or [int]$p1.Groups[7].Value -ne 1) {
        throw "Fox model object proof failed.`n$gdbStdout"
    }
    if (-not $updateZero.Success -or (Convert-MarkerUInt32 $updateZero.Groups[1].Value) -ne 0 -or [int]$updateZero.Groups[2].Value -ne 0) {
        throw "Mario/Fox model harness unexpectedly ran the Pupupu update probe.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "Battle Mario/Fox model harness replayed opening/title work.`n$gdbStdout"
    }
    Write-Output ("Battle Mario/Fox model harness passed: assets={0}, setup={1}, realGObjs={2}, p0DObjs={3}, p1DObjs={4}" -f $reloc.Groups[2].Value, $model.Groups[3].Value, $model.Groups[4].Value, $p0.Groups[4].Value, $p1.Groups[4].Value)
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
