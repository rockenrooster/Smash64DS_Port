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
$rom = Join-Path $root 'smash64ds-battle-mariofox-struct.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-struct.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-struct-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-struct-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_struct_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-struct BUILD=build-battle-mariofox-struct-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_struct -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox struct harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_MODEL=%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxSetupMask, gNdsFighterModelRealGObjCount, gNdsFighterModelStubGObjCount, gNdsFighterModelProcessDeferredCount, gNdsFighterModelP0ModelDObjCount, gNdsFighterModelP1ModelDObjCount',
        'printf "FTR_STRUCT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxStructMask, gNdsFighterMarioFoxStructPoolUsedMask, gNdsFighterMarioFoxStructCount',
        'printf "FTR_STRUCT_PTRS=%u,%u,%u,%u,%u,%u\n", gNdsFighterStructP0PtrReady, gNdsFighterStructP1PtrReady, gNdsFighterStructP0UserDataPtrReady, gNdsFighterStructP1UserDataPtrReady, gNdsFighterStructP0FtGetStructReady, gNdsFighterStructP1FtGetStructReady',
        'printf "FTR_STRUCT_P0=%u,%u,%u,%d,%u,%u,%u,%u,%u\n", gNdsFighterStructP0FKind, gNdsFighterStructP0PKind, gNdsFighterStructP0Player, gNdsFighterStructP0LR, gNdsFighterStructP0Stock, gNdsFighterStructP0Detail, gNdsFighterStructP0Costume, gNdsFighterStructP0Shade, gNdsFighterStructP0StatusTotalTics',
        'printf "FTR_STRUCT_P1=%u,%u,%u,%d,%u,%u,%u,%u,%u\n", gNdsFighterStructP1FKind, gNdsFighterStructP1PKind, gNdsFighterStructP1Player, gNdsFighterStructP1LR, gNdsFighterStructP1Stock, gNdsFighterStructP1Detail, gNdsFighterStructP1Costume, gNdsFighterStructP1Shade, gNdsFighterStructP1StatusTotalTics',
        'printf "FTR_STRUCT_JOINTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterStructP0TopJointReady, gNdsFighterStructP1TopJointReady, gNdsFighterStructP0JointCount, gNdsFighterStructP1JointCount, gNdsFighterStructP0CommonJointCount, gNdsFighterStructP1CommonJointCount, gNdsFighterStructP0CollTranslateReady, gNdsFighterStructP1CollTranslateReady, gNdsFighterStructP0CollLRReady, gNdsFighterStructP1CollLRReady',
        'printf "FTR_STRUCT_INPUT=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterStructP0InputMaskA, gNdsFighterStructP0InputMaskB, gNdsFighterStructP0InputMaskZ, gNdsFighterStructP0InputMaskL, gNdsFighterStructP1InputMaskA, gNdsFighterStructP1InputMaskB, gNdsFighterStructP1InputMaskZ, gNdsFighterStructP1InputMaskL',
        'printf "FTR_STRUCT_DEFER=%u,%u,%u\n", gNdsFighterStructProcessAttachCount, gNdsFighterStructStatusSetCount, gNdsFighterStructDisplayProbeCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vsb = [regex]::Match($gdbStdout, 'VSB=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $reloc = [regex]::Match($gdbStdout, 'FTR_RELOC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $model = [regex]::Match($gdbStdout, 'FTR_MODEL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $struct = [regex]::Match($gdbStdout, 'FTR_STRUCT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $ptrs = [regex]::Match($gdbStdout, 'FTR_STRUCT_PTRS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $p0 = [regex]::Match($gdbStdout, 'FTR_STRUCT_P0=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $p1 = [regex]::Match($gdbStdout, 'FTR_STRUCT_P1=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $joints = [regex]::Match($gdbStdout, 'FTR_STRUCT_JOINTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'FTR_STRUCT_INPUT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $defer = [regex]::Match($gdbStdout, 'FTR_STRUCT_DEFER=([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 13 -or [int]$harn.Groups[3].Value -ne 22 -or [int]$harn.Groups[4].Value -ne 21 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle Mario/Fox struct harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $vsb.Success -or (Convert-MarkerUInt32 $vsb.Groups[1].Value) -ne 0x56425355 -or ((Convert-MarkerUInt32 $vsb.Groups[2].Value) -band 0x6f) -ne 0x6f -or (Convert-MarkerUInt32 $vsb.Groups[3].Value) -ne 0x56425550 -or [int]$vsb.Groups[4].Value -ne 2 -or [int]$vsb.Groups[5].Value -ne 2 -or [int]$vsb.Groups[6].Value -ne 6) {
        throw "Imported VSBattle setup did not reach the Mario/Fox struct boundary.`n$gdbStdout"
    }
    if (-not $reloc.Success -or (Convert-MarkerUInt32 $reloc.Groups[1].Value) -ne 0x4654524c -or ((Convert-MarkerUInt32 $reloc.Groups[2].Value) -band 0x7fff) -ne 0x7fff -or [int]$reloc.Groups[6].Value -ne 0) {
        throw "Mario/Fox fighter relocation proof failed.`n$gdbStdout"
    }
    if (-not $model.Success -or (Convert-MarkerUInt32 $model.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $model.Groups[2].Value) -ne 0x4654474f -or ((Convert-MarkerUInt32 $model.Groups[3].Value) -band 0xfff) -ne 0xfff -or [int]$model.Groups[4].Value -ne 2 -or [int]$model.Groups[5].Value -ne 0 -or [int]$model.Groups[7].Value -lt 1 -or [int]$model.Groups[8].Value -lt 1) {
        throw "Mario/Fox model proof failed before struct setup.`n$gdbStdout"
    }
    if (-not $struct.Success -or (Convert-MarkerUInt32 $struct.Groups[1].Value) -ne 0x46545348 -or (Convert-MarkerUInt32 $struct.Groups[2].Value) -ne 0x46544a54 -or (Convert-MarkerUInt32 $struct.Groups[3].Value) -ne 0x46545354 -or ((Convert-MarkerUInt32 $struct.Groups[4].Value) -band 0xfff) -ne 0xfff -or (Convert-MarkerUInt32 $struct.Groups[5].Value) -ne 0x3 -or [int]$struct.Groups[6].Value -ne 2) {
        throw "Persistent FTStruct proof failed.`n$gdbStdout"
    }
    if (-not $ptrs.Success) { throw "Persistent FTStruct pointer diagnostics missing.`n$gdbStdout" }
    for ($i = 1; $i -le 6; $i++) {
        if ([int]$ptrs.Groups[$i].Value -ne 1) { throw "Persistent FTStruct pointer proof failed.`n$gdbStdout" }
    }
    if (-not $p0.Success -or [int]$p0.Groups[1].Value -ne 0 -or [int]$p0.Groups[2].Value -ne 0 -or [int]$p0.Groups[3].Value -ne 0 -or [int]$p0.Groups[4].Value -ne 1 -or [int]$p0.Groups[5].Value -ne 2 -or [int]$p0.Groups[6].Value -ne 1 -or [int]$p0.Groups[9].Value -ne 0) {
        throw "Mario FTStruct state fields are not initialized as expected.`n$gdbStdout"
    }
    if (-not $p1.Success -or [int]$p1.Groups[1].Value -ne 1 -or [int]$p1.Groups[2].Value -ne 0 -or [int]$p1.Groups[3].Value -ne 1 -or [int]$p1.Groups[4].Value -ne -1 -or [int]$p1.Groups[5].Value -ne 2 -or [int]$p1.Groups[6].Value -ne 1 -or [int]$p1.Groups[9].Value -ne 0) {
        throw "Fox FTStruct state fields are not initialized as expected.`n$gdbStdout"
    }
    if (-not $joints.Success -or [int]$joints.Groups[1].Value -ne 1 -or [int]$joints.Groups[2].Value -ne 1 -or [int]$joints.Groups[5].Value -lt 20 -or [int]$joints.Groups[6].Value -lt 20 -or [int]$joints.Groups[7].Value -ne 1 -or [int]$joints.Groups[8].Value -ne 1 -or [int]$joints.Groups[9].Value -ne 1 -or [int]$joints.Groups[10].Value -ne 1) {
        throw "FTStruct joint or collision pointer proof failed.`n$gdbStdout"
    }
    if (-not $input.Success -or (Convert-MarkerUInt32 $input.Groups[1].Value) -ne 0x8000 -or (Convert-MarkerUInt32 $input.Groups[2].Value) -ne 0x4000 -or (Convert-MarkerUInt32 $input.Groups[3].Value) -ne 0x2000 -or (Convert-MarkerUInt32 $input.Groups[4].Value) -ne 0x20 -or (Convert-MarkerUInt32 $input.Groups[5].Value) -ne 0x8000 -or (Convert-MarkerUInt32 $input.Groups[6].Value) -ne 0x4000 -or (Convert-MarkerUInt32 $input.Groups[7].Value) -ne 0x2000 -or (Convert-MarkerUInt32 $input.Groups[8].Value) -ne 0x20) {
        throw "FTStruct input mask proof failed.`n$gdbStdout"
    }
    if (-not $defer.Success -or [int]$defer.Groups[1].Value -ne 0 -or [int]$defer.Groups[2].Value -ne 0 -or [int]$defer.Groups[3].Value -ne 0) {
        throw "Fighter status/process/display code ran during struct proof.`n$gdbStdout"
    }
    if (-not $boundary.Success -or (Convert-MarkerUInt32 $boundary.Groups[1].Value) -ne 0x53434e45 -or [int]$boundary.Groups[2].Value -ne 22) {
        throw "VSBattle did not park at the bounded scene boundary.`n$gdbStdout"
    }
    Write-Output ("Battle Mario/Fox struct harness passed: scene=22/21 struct={0} pool={1} p0Joints={2} p1Joints={3} model={4}" -f $struct.Groups[4].Value, $struct.Groups[5].Value, $joints.Groups[5].Value, $joints.Groups[6].Value, $model.Groups[3].Value)
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
