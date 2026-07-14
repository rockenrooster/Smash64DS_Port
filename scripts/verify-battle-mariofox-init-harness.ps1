param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [switch]$ImportBattleShipFTManager,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ImportBattleShipFTManager = $true
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$target = 'smash64ds-battle-mariofox-init'
$build = 'build-battle-mariofox-init-harness'
$rom = Join-Path $root ("{0}.nds" -f $target)
$elf = Join-Path $root ("{0}.elf" -f $target)
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-init-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-init-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_init_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$makeArgs = @('-C', $root, "TARGET=$target", "BUILD=$build", 'NDS_DEV_SCENE_HARNESS=battle_mariofox_init', '-j16')
if ($ImportBattleShipFTManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTMANAGER=1'
}
& make @makeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox init harness build did not produce the expected ROM and ELF.'
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
        'printf "PUPUPU=%#x,%#x,%#x,%#x,%u,%u\n", gNdsSCVSBattleStageResult, gNdsSCVSBattleStageMask, gNdsPupupuGroundSetupResult, gNdsPupupuGroundSetupMask, gNdsPupupuGroundLayerGObjCount, gNdsPupupuGroundMapGObjCount',
        'printf "MAPOBJ=%u,%#x,%#x,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", gNdsStagePupupuMapObjSourceCount, gNdsStagePupupuMapObjDecodedMask, gNdsStagePupupuMapObjDuplicateMask, gNdsStagePupupuMapObjXs[0], gNdsStagePupupuMapObjYs[0], gNdsStagePupupuMapObjXs[1], gNdsStagePupupuMapObjYs[1], gNdsStagePupupuMapObjXs[2], gNdsStagePupupuMapObjYs[2], gNdsStagePupupuMapObjXs[3], gNdsStagePupupuMapObjYs[3], gNdsStagePupupuMapObjUnalignedReadCount',
        'printf "FTR_MANAGER=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterManagerResult, gNdsFighterManagerMask, gNdsFighterManagerExternMask, gNdsFighterManagerStatusBufferMask, gNdsFighterManagerFighterMask, gNdsFighterManagerDataMask, gNdsFighterManagerWaitMask, gNdsFighterManagerEntryMask, gNdsFighterManagerStatusBufferHitCount, gNdsFighterManagerFighterCount, gNdsFighterManagerFigatreeHeapSize',
        'printf "FTR_MODEL=%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxSetupMask, gNdsFighterModelRealGObjCount, gNdsFighterModelStubGObjCount, gNdsFighterModelProcessDeferredCount, gNdsFighterModelP0ModelDObjCount, gNdsFighterModelP1ModelDObjCount',
        'printf "FTR_STRUCT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxStructMask, gNdsFighterMarioFoxStructPoolUsedMask, gNdsFighterMarioFoxStructCount',
        'printf "FTR_INIT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxInitResult, gNdsFighterMarioFoxCollResult, gNdsFighterMarioFoxDeferResult, gNdsFighterMarioFoxInitMask, gNdsFighterMarioFoxInitDeferredMask, gNdsFighterMarioFoxInitCount',
        'printf "FTR_INIT_P0=%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterInitP0FKind, gNdsFighterInitP0PercentDamage, gNdsFighterInitP0ShieldHealth, gNdsFighterInitP0GA, gNdsFighterInitP0JumpsUsed, gNdsFighterInitP0HitStatus, gNdsFighterInitP0DamageKind, gNdsFighterInitP0MotionAttackID, gNdsFighterInitP0PassiveMarioTornado',
        'printf "FTR_INIT_P1=%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterInitP1FKind, gNdsFighterInitP1PercentDamage, gNdsFighterInitP1ShieldHealth, gNdsFighterInitP1GA, gNdsFighterInitP1JumpsUsed, gNdsFighterInitP1HitStatus, gNdsFighterInitP1DamageKind, gNdsFighterInitP1MotionAttackID, gNdsFighterInitP1PassiveMarioTornado',
        'printf "FTR_INIT_FLOOR=%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterInitP0FloorProjectAttempt, gNdsFighterInitP1FloorProjectAttempt, gNdsFighterInitP0FloorProjectResult, gNdsFighterInitP1FloorProjectResult, gNdsFighterInitP0FloorLineID, gNdsFighterInitP1FloorLineID, gNdsFighterInitP0FloorDistBits, gNdsFighterInitP1FloorDistBits',
        'printf "FTR_SOURCE=%#x,%#x,%#x,%#x,%u,%u,%#x,%#x\n", gNdsFighterInitP0RootTranslateXBits, gNdsFighterInitP1RootTranslateXBits, gNdsFighterInitP0RootTranslateYBits, gNdsFighterInitP1RootTranslateYBits, gNdsFighterInitP0GA, gNdsFighterInitP1GA, gNdsFighterInitP0FloorLineID, gNdsFighterInitP1FloorLineID',
        'printf "FTR_INIT_DAMAGECOLL=%#x,%#x,%#x,%#x,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterInitDamageCollMask, gNdsFighterInitDamageCollNormalMask, gNdsFighterInitDamageCollJointMask, gNdsFighterInitDamageCollHalfSizeMask, gNdsFighterInitP0DamageCollCount, gNdsFighterInitP1DamageCollCount, gNdsFighterInitP0DamageCollJoint0, gNdsFighterInitP1DamageCollJoint0, gNdsFighterInitP0DamageCollSizeXBits, gNdsFighterInitP1DamageCollSizeXBits, gNdsFighterInitP0DamageCollSizeYBits, gNdsFighterInitP1DamageCollSizeYBits, gNdsFighterInitP0DamageCollSizeZBits, gNdsFighterInitP1DamageCollSizeZBits',
        'printf "FTR_INIT_DAMAGEPARTS=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterInitDamageCollPartsMask, gNdsFighterInitDamageCollMatrixMask, gNdsFighterInitDamageCollScaleMask, gNdsFighterInitP0DamageCollWorldXBits, gNdsFighterInitP1DamageCollWorldXBits, gNdsFighterInitP0DamageCollWorldYBits, gNdsFighterInitP1DamageCollWorldYBits, gNdsFighterInitP0DamageCollScaleXBits, gNdsFighterInitP1DamageCollScaleXBits',
        'printf "FTR_INIT_CALLS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterInitPhysicsStopCount, gNdsFighterInitAttackClearCount, gNdsFighterInitHitStatusPartCount, gNdsFighterInitColAnimResetCount, gNdsFighterInitProcessAttachCount, gNdsFighterInitStatusSetCount, gNdsFighterInitDisplayProbeCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vsb = [regex]::Match($gdbStdout, 'VSB=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $pupupu = [regex]::Match($gdbStdout, 'PUPUPU=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $mapObj = [regex]::Match($gdbStdout, 'MAPOBJ=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $manager = [regex]::Match($gdbStdout, 'FTR_MANAGER=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $model = [regex]::Match($gdbStdout, 'FTR_MODEL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $struct = [regex]::Match($gdbStdout, 'FTR_STRUCT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $init = [regex]::Match($gdbStdout, 'FTR_INIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $p0 = [regex]::Match($gdbStdout, 'FTR_INIT_P0=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $p1 = [regex]::Match($gdbStdout, 'FTR_INIT_P1=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $floor = [regex]::Match($gdbStdout, 'FTR_INIT_FLOOR=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $sourceStart = [regex]::Match($gdbStdout, 'FTR_SOURCE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $damageColl = [regex]::Match($gdbStdout, 'FTR_INIT_DAMAGECOLL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $damageParts = [regex]::Match($gdbStdout, 'FTR_INIT_DAMAGEPARTS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $calls = [regex]::Match($gdbStdout, 'FTR_INIT_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 15 -or [int]$harn.Groups[3].Value -ne 22 -or [int]$harn.Groups[4].Value -ne 21 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle Mario/Fox init harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $vsb.Success -or (Convert-MarkerUInt32 $vsb.Groups[1].Value) -ne 0x56425355 -or ((Convert-MarkerUInt32 $vsb.Groups[2].Value) -band 0x6f) -ne 0x6f -or (Convert-MarkerUInt32 $vsb.Groups[3].Value) -ne 0x56425550 -or [int]$vsb.Groups[4].Value -ne 2 -or [int]$vsb.Groups[5].Value -ne 2 -or [int]$vsb.Groups[6].Value -ne 6) {
        throw "Imported VSBattle setup did not reach the Mario/Fox init boundary.`n$gdbStdout"
    }
    if (-not $pupupu.Success -or (Convert-MarkerUInt32 $pupupu.Groups[1].Value) -ne 0x50555042 -or ((Convert-MarkerUInt32 $pupupu.Groups[2].Value) -band 0xff) -ne 0xff -or (Convert-MarkerUInt32 $pupupu.Groups[3].Value) -ne 0x50554753 -or ((Convert-MarkerUInt32 $pupupu.Groups[4].Value) -band 0x3ff) -ne 0x3ff -or [int]$pupupu.Groups[5].Value -ne 4 -or [int]$pupupu.Groups[6].Value -ne 4) {
        throw "Pupupu stage/ground proof failed before fighter init.`n$gdbStdout"
    }
    if (-not $mapObj.Success -or [int]$mapObj.Groups[1].Value -lt 4 -or (Convert-MarkerUInt32 $mapObj.Groups[2].Value) -ne 0xf -or (Convert-MarkerUInt32 $mapObj.Groups[3].Value) -ne 0 -or [int]$mapObj.Groups[4].Value -ne 0 -or [int]$mapObj.Groups[5].Value -ne 6 -or [int]$mapObj.Groups[6].Value -ne -1397 -or [int]$mapObj.Groups[7].Value -ne 906 -or [int]$mapObj.Groups[8].Value -ne 1 -or [int]$mapObj.Groups[9].Value -ne 1545 -or [int]$mapObj.Groups[10].Value -ne 1421 -or [int]$mapObj.Groups[11].Value -ne 909 -or [int]$mapObj.Groups[12].Value -ne 0) {
        throw "Pupupu source map-object records did not decode from the aligned O2R array base.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
        $managerStatusMask = 0
        if ($manager.Success) {
            $managerStatusMask = (Convert-MarkerUInt32 $manager.Groups[7].Value) -bor (Convert-MarkerUInt32 $manager.Groups[8].Value)
        }
        if (-not $manager.Success -or (Convert-MarkerUInt32 $manager.Groups[1].Value) -ne 0x46544d47 -or ((Convert-MarkerUInt32 $manager.Groups[2].Value) -band 0xff) -ne 0xff -or ((Convert-MarkerUInt32 $manager.Groups[3].Value) -band 0xf) -ne 0xf -or ((Convert-MarkerUInt32 $manager.Groups[4].Value) -band 0x1fff) -ne 0x1fff -or ((Convert-MarkerUInt32 $manager.Groups[5].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $manager.Groups[6].Value) -band 0x3) -ne 0x3 -or ($managerStatusMask -band 0x3) -ne 0x3 -or [int]$manager.Groups[9].Value -lt 13 -or [int]$manager.Groups[10].Value -ne 2 -or [int]$manager.Groups[11].Value -eq 0) {
            throw "BattleShip ftmanager fenced proof failed.`n$gdbStdout"
        }
        if (-not $sourceStart.Success -or (Convert-MarkerUInt32 $sourceStart.Groups[1].Value) -ne 0 -or (Convert-MarkerUInt32 $sourceStart.Groups[2].Value) -ne 3299778560 -or (Convert-MarkerUInt32 $sourceStart.Groups[3].Value) -ne 0 -or (Convert-MarkerUInt32 $sourceStart.Groups[4].Value) -ne 0x44620000 -or [int]$sourceStart.Groups[5].Value -ne 0 -or [int]$sourceStart.Groups[6].Value -ne 0 -or (Convert-MarkerUInt32 $sourceStart.Groups[7].Value) -eq 4294967295 -or (Convert-MarkerUInt32 $sourceStart.Groups[8].Value) -eq 4294967295) {
            throw "Original ftmanager did not adopt both decoded Pupupu starts onto valid floors.`n$gdbStdout"
        }
    } elseif (-not $model.Success -or (Convert-MarkerUInt32 $model.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $model.Groups[2].Value) -ne 0x4654474f -or ((Convert-MarkerUInt32 $model.Groups[3].Value) -band 0xfff) -ne 0xfff -or [int]$model.Groups[4].Value -ne 2 -or [int]$model.Groups[5].Value -ne 0) {
        throw "Mario/Fox model proof failed before init setup.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $struct.Success -or (Convert-MarkerUInt32 $struct.Groups[1].Value) -ne 0x46545348 -or (Convert-MarkerUInt32 $struct.Groups[2].Value) -ne 0x46544a54 -or (Convert-MarkerUInt32 $struct.Groups[3].Value) -ne 0x46545354 -or ((Convert-MarkerUInt32 $struct.Groups[4].Value) -band 0xfff) -ne 0xfff -or (Convert-MarkerUInt32 $struct.Groups[5].Value) -ne 0x3 -or [int]$struct.Groups[6].Value -ne 2) {
        throw "Persistent FTStruct proof failed before init setup.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $init.Success -or (Convert-MarkerUInt32 $init.Groups[1].Value) -ne 0x4654494e -or (Convert-MarkerUInt32 $init.Groups[2].Value) -ne 0x4654434c -or (Convert-MarkerUInt32 $init.Groups[3].Value) -ne 0x46544446 -or ((Convert-MarkerUInt32 $init.Groups[4].Value) -band 0x3fff) -ne 0x3fff -or [int]$init.Groups[6].Value -ne 2) {
        throw "Mario/Fox init-state proof failed.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $p0.Success -or [int]$p0.Groups[1].Value -ne 0 -or [int]$p0.Groups[2].Value -ne 0 -or [int]$p0.Groups[3].Value -ne 55 -or [int]$p0.Groups[4].Value -ne 0 -or [int]$p0.Groups[5].Value -ne 0 -or [int]$p0.Groups[6].Value -ne 1 -or [int]$p0.Groups[7].Value -ne 0 -or (Convert-MarkerUInt32 $p0.Groups[8].Value) -ne 0) {
        throw "Mario init-state fields are not initialized as expected.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $p1.Success -or [int]$p1.Groups[1].Value -ne 1 -or [int]$p1.Groups[2].Value -ne 0 -or [int]$p1.Groups[3].Value -ne 55 -or [int]$p1.Groups[4].Value -ne 0 -or [int]$p1.Groups[5].Value -ne 0 -or [int]$p1.Groups[6].Value -ne 1 -or [int]$p1.Groups[7].Value -ne 0 -or (Convert-MarkerUInt32 $p1.Groups[8].Value) -ne 0) {
        throw "Fox init-state fields are not initialized as expected.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $floor.Success -or [int]$floor.Groups[1].Value -ne 1 -or [int]$floor.Groups[2].Value -ne 1 -or [int]$floor.Groups[3].Value -ne 1 -or [int]$floor.Groups[4].Value -ne 1 -or (Convert-MarkerUInt32 $floor.Groups[5].Value) -ne 0 -or (Convert-MarkerUInt32 $floor.Groups[6].Value) -ne 0) {
        throw "Mario/Fox bounded floor projection proof failed.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $damageColl.Success -or ((Convert-MarkerUInt32 $damageColl.Groups[1].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageColl.Groups[2].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageColl.Groups[3].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageColl.Groups[4].Value) -band 0x3) -ne 0x3 -or [int]$damageColl.Groups[5].Value -ne 10 -or [int]$damageColl.Groups[6].Value -ne 11 -or [int]$damageColl.Groups[7].Value -ne 6 -or [int]$damageColl.Groups[8].Value -ne 5 -or (Convert-MarkerUInt32 $damageColl.Groups[9].Value) -ne 0x424e0000 -or (Convert-MarkerUInt32 $damageColl.Groups[10].Value) -ne 0x424c0000 -or (Convert-MarkerUInt32 $damageColl.Groups[11].Value) -ne 0x42600000 -or (Convert-MarkerUInt32 $damageColl.Groups[12].Value) -ne 0x41d00000 -or (Convert-MarkerUInt32 $damageColl.Groups[13].Value) -ne 0x423e0000 -or (Convert-MarkerUInt32 $damageColl.Groups[14].Value) -ne 0x41b40000) {
        throw "Mario/Fox bounded damage-collision shell proof failed.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $damageParts.Success -or ((Convert-MarkerUInt32 $damageParts.Groups[1].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageParts.Groups[2].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageParts.Groups[3].Value) -band 0x3) -ne 0x3 -or (Convert-MarkerUInt32 $damageParts.Groups[8].Value) -eq 0 -or (Convert-MarkerUInt32 $damageParts.Groups[9].Value) -eq 0) {
        throw "Mario/Fox bounded damage-collision FTParts proof failed.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
    } elseif (-not $calls.Success -or [int]$calls.Groups[1].Value -lt 2 -or [int]$calls.Groups[2].Value -lt 2 -or [int]$calls.Groups[3].Value -lt 2 -or [int]$calls.Groups[4].Value -lt 2 -or [int]$calls.Groups[5].Value -ne 0 -or [int]$calls.Groups[6].Value -ne 0 -or [int]$calls.Groups[7].Value -ne 0) {
        throw "Fighter init compatibility/deferred call counters failed.`n$gdbStdout"
    }
    if (-not $boundary.Success -or (Convert-MarkerUInt32 $boundary.Groups[1].Value) -ne 0x53434e45 -or [int]$boundary.Groups[2].Value -ne 22) {
        throw "VSBattle did not park at the bounded scene boundary.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
        Write-Output ("Battle Mario/Fox init harness passed: scene=22/21 ftmanager={0} mask={1} status={2} fighters={3} entry={4} wait={5} figatree={6}" -f $manager.Groups[1].Value, $manager.Groups[2].Value, $manager.Groups[4].Value, $manager.Groups[5].Value, $manager.Groups[8].Value, $manager.Groups[7].Value, $manager.Groups[11].Value)
    } else {
        Write-Output ("Battle Mario/Fox init harness passed: scene=22/21 init={0} damageColl={1}/{2}/{3}/{4} parts={5}/{6}/{7} p0GA={8} p1GA={9} floor=1/1 calls={10}/{11}/{12}/{13}" -f $init.Groups[4].Value, $damageColl.Groups[1].Value, $damageColl.Groups[2].Value, $damageColl.Groups[3].Value, $damageColl.Groups[4].Value, $damageParts.Groups[1].Value, $damageParts.Groups[2].Value, $damageParts.Groups[3].Value, $p0.Groups[4].Value, $p1.Groups[4].Value, $calls.Groups[1].Value, $calls.Groups[2].Value, $calls.Groups[3].Value, $calls.Groups[4].Value)
    }
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
