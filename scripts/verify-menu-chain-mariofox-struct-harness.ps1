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
$rom = Join-Path $root 'smash64ds-menu-chain-mariofox-struct.nds'
$elf = Join-Path $root 'smash64ds-menu-chain-mariofox-struct.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.menu-chain-mariofox-struct-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.menu-chain-mariofox-struct-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_menu_chain_mariofox_struct_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-menu-chain-mariofox-struct BUILD=build-menu-chain-mariofox-struct-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_struct -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Menu-chain Mario/Fox struct harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_RELOC=%#x,%#x,%#x,%u,%u\n", gNdsFighterMarioFoxRelocResult, gNdsFighterMarioFoxRelocAssetMask, gNdsFighterMarioFoxRelocDependencyMask, gNdsFighterMarioFoxExternalFixupCount, gNdsFighterMarioFoxExternalFixupFailCount',
        'printf "FTR_MODEL=%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxSetupMask, gNdsFighterModelRealGObjCount, gNdsFighterModelStubGObjCount, gNdsFighterModelProcessDeferredCount, gNdsFighterModelP0ModelDObjCount, gNdsFighterModelP1ModelDObjCount',
        'printf "FTR_STRUCT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxStructMask, gNdsFighterMarioFoxStructPoolUsedMask, gNdsFighterMarioFoxStructCount',
        'printf "FTR_STRUCT_PTRS=%u,%u,%u,%u,%u,%u\n", gNdsFighterStructP0PtrReady, gNdsFighterStructP1PtrReady, gNdsFighterStructP0UserDataPtrReady, gNdsFighterStructP1UserDataPtrReady, gNdsFighterStructP0FtGetStructReady, gNdsFighterStructP1FtGetStructReady',
        'printf "FTR_STRUCT_JOINTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterStructP0TopJointReady, gNdsFighterStructP1TopJointReady, gNdsFighterStructP0JointCount, gNdsFighterStructP1JointCount, gNdsFighterStructP0CommonJointCount, gNdsFighterStructP1CommonJointCount, gNdsFighterStructP0CollTranslateReady, gNdsFighterStructP1CollTranslateReady, gNdsFighterStructP0CollLRReady, gNdsFighterStructP1CollLRReady',
        'printf "FTR_STRUCT_IDS=%u,%u,%u,%d,%u,%u,%u,%d,%u,%u\n", gNdsFighterStructP0FKind, gNdsFighterStructP0PKind, gNdsFighterStructP0Player, gNdsFighterStructP0LR, gNdsFighterStructP0Stock, gNdsFighterStructP1FKind, gNdsFighterStructP1PKind, gNdsFighterStructP1Player, gNdsFighterStructP1LR, gNdsFighterStructP1Stock',
        'printf "FTR_STRUCT_INPUT=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterStructP0InputMaskA, gNdsFighterStructP0InputMaskB, gNdsFighterStructP0InputMaskZ, gNdsFighterStructP0InputMaskL, gNdsFighterStructP1InputMaskA, gNdsFighterStructP1InputMaskB, gNdsFighterStructP1InputMaskZ, gNdsFighterStructP1InputMaskL',
        'printf "FTR_STRUCT_DEFER=%u,%u,%u\n", gNdsFighterStructProcessAttachCount, gNdsFighterStructStatusSetCount, gNdsFighterStructDisplayProbeCount',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $chain = [regex]::Match($gdbStdout, 'CHAIN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $final = [regex]::Match($gdbStdout, 'CHAIN_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $reloc = [regex]::Match($gdbStdout, 'FTR_RELOC=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $model = [regex]::Match($gdbStdout, 'FTR_MODEL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $struct = [regex]::Match($gdbStdout, 'FTR_STRUCT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $ptrs = [regex]::Match($gdbStdout, 'FTR_STRUCT_PTRS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $joints = [regex]::Match($gdbStdout, 'FTR_STRUCT_JOINTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $ids = [regex]::Match($gdbStdout, 'FTR_STRUCT_IDS=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'FTR_STRUCT_INPUT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $defer = [regex]::Match($gdbStdout, 'FTR_STRUCT_DEFER=([0-9]+),([0-9]+),([0-9]+)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 14 -or [int]$harn.Groups[3].Value -ne 9 -or [int]$harn.Groups[4].Value -ne 1 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Menu-chain Mario/Fox struct harness did not start at VSMode from Title.`n$gdbStdout"
    }
    if (-not $chain.Success -or (Convert-MarkerUInt32 $chain.Groups[1].Value) -ne 0x56535452 -or ((Convert-MarkerUInt32 $chain.Groups[2].Value) -band 0xff) -ne 0xff -or (Convert-MarkerUInt32 $chain.Groups[3].Value) -ne 0x50565452 -or ((Convert-MarkerUInt32 $chain.Groups[4].Value) -band 0xff) -ne 0xff -or (Convert-MarkerUInt32 $chain.Groups[5].Value) -ne 0x4d53454c -or ((Convert-MarkerUInt32 $chain.Groups[6].Value) -band 0xff) -ne 0xff) {
        throw "Menu-chain transitions did not reach VSBattle.`n$gdbStdout"
    }
    if (-not $final.Success -or [int]$final.Groups[1].Value -ne 16 -or [int]$final.Groups[2].Value -ne 21 -or [int]$final.Groups[3].Value -ne 22 -or [int]$final.Groups[4].Value -ne 21 -or [int]$final.Groups[5].Value -ne 6) {
        throw "Menu-chain final scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not VSBattle after menu chain.`n$gdbStdout"
    }
    if (-not $reloc.Success -or (Convert-MarkerUInt32 $reloc.Groups[1].Value) -ne 0x4654524c -or ((Convert-MarkerUInt32 $reloc.Groups[2].Value) -band 0x7fff) -ne 0x7fff -or [int]$reloc.Groups[5].Value -ne 0) {
        throw "Mario/Fox fighter relocation proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $model.Success -or (Convert-MarkerUInt32 $model.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $model.Groups[2].Value) -ne 0x4654474f -or ((Convert-MarkerUInt32 $model.Groups[3].Value) -band 0xfff) -ne 0xfff -or [int]$model.Groups[4].Value -ne 2 -or [int]$model.Groups[5].Value -ne 0 -or [int]$model.Groups[6].Value -lt 2 -or [int]$model.Groups[7].Value -lt 1 -or [int]$model.Groups[8].Value -lt 1) {
        throw "Mario/Fox model proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $struct.Success -or (Convert-MarkerUInt32 $struct.Groups[1].Value) -ne 0x46545348 -or (Convert-MarkerUInt32 $struct.Groups[2].Value) -ne 0x46544a54 -or (Convert-MarkerUInt32 $struct.Groups[3].Value) -ne 0x46545354 -or ((Convert-MarkerUInt32 $struct.Groups[4].Value) -band 0xfff) -ne 0xfff -or (Convert-MarkerUInt32 $struct.Groups[5].Value) -ne 0x3 -or [int]$struct.Groups[6].Value -ne 2) {
        throw "Persistent FTStruct proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $ptrs.Success) { throw "Persistent FTStruct pointer diagnostics missing after menu chain.`n$gdbStdout" }
    for ($i = 1; $i -le 6; $i++) {
        if ([int]$ptrs.Groups[$i].Value -ne 1) { throw "Persistent FTStruct pointer proof failed after menu chain.`n$gdbStdout" }
    }
    if (-not $ids.Success) {
        throw "Mario/Fox FTStruct identity fields missing after menu chain.`n$gdbStdout"
    }
    $p1LR = [int64]$ids.Groups[9].Value
    if ([int]$ids.Groups[1].Value -ne 0 -or [int]$ids.Groups[2].Value -ne 0 -or [int]$ids.Groups[3].Value -ne 0 -or [int]$ids.Groups[4].Value -ne 1 -or [int]$ids.Groups[5].Value -ne 2 -or [int]$ids.Groups[6].Value -ne 1 -or [int]$ids.Groups[7].Value -ne 0 -or [int]$ids.Groups[8].Value -ne 1 -or (($p1LR -ne -1) -and ($p1LR -ne 4294967295)) -or [int]$ids.Groups[10].Value -ne 2) {
        throw "Mario/Fox FTStruct identity fields failed after menu chain.`n$gdbStdout"
    }
    if (-not $joints.Success -or [int]$joints.Groups[1].Value -ne 1 -or [int]$joints.Groups[2].Value -ne 1 -or [int]$joints.Groups[5].Value -lt 20 -or [int]$joints.Groups[6].Value -lt 20 -or [int]$joints.Groups[7].Value -ne 1 -or [int]$joints.Groups[8].Value -ne 1 -or [int]$joints.Groups[9].Value -ne 1 -or [int]$joints.Groups[10].Value -ne 1) {
        throw "FTStruct joint or collision pointer proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $input.Success -or (Convert-MarkerUInt32 $input.Groups[1].Value) -ne 0x8000 -or (Convert-MarkerUInt32 $input.Groups[2].Value) -ne 0x4000 -or (Convert-MarkerUInt32 $input.Groups[3].Value) -ne 0x2000 -or (Convert-MarkerUInt32 $input.Groups[4].Value) -ne 0x20 -or (Convert-MarkerUInt32 $input.Groups[5].Value) -ne 0x8000 -or (Convert-MarkerUInt32 $input.Groups[6].Value) -ne 0x4000 -or (Convert-MarkerUInt32 $input.Groups[7].Value) -ne 0x2000 -or (Convert-MarkerUInt32 $input.Groups[8].Value) -ne 0x20) {
        throw "FTStruct input mask proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $defer.Success -or [int]$defer.Groups[1].Value -ne 0 -or [int]$defer.Groups[2].Value -ne 0 -or [int]$defer.Groups[3].Value -ne 0) {
        throw "Fighter status/process/display code ran during menu-chain struct proof.`n$gdbStdout"
    }
    Write-Output ("Menu-chain Mario/Fox struct harness passed: chain masks={0}/{1}/{2}, model={3}, struct={4}, final=22/21" -f $chain.Groups[2].Value, $chain.Groups[4].Value, $chain.Groups[6].Value, $model.Groups[3].Value, $struct.Groups[4].Value)
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
