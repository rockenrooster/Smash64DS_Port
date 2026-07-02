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
$rom = Join-Path $root 'smash64ds-menu-chain-mariofox-init.nds'
$elf = Join-Path $root 'smash64ds-menu-chain-mariofox-init.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.menu-chain-mariofox-init-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.menu-chain-mariofox-init-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_menu_chain_mariofox_init_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-menu-chain-mariofox-init BUILD=build-menu-chain-mariofox-init-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_init -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Menu-chain Mario/Fox init harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_MODEL=%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxSetupMask, gNdsFighterModelRealGObjCount, gNdsFighterModelStubGObjCount, gNdsFighterModelProcessDeferredCount, gNdsFighterModelP0ModelDObjCount, gNdsFighterModelP1ModelDObjCount',
        'printf "FTR_STRUCT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxStructMask, gNdsFighterMarioFoxStructPoolUsedMask, gNdsFighterMarioFoxStructCount',
        'printf "FTR_INIT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxInitResult, gNdsFighterMarioFoxCollResult, gNdsFighterMarioFoxDeferResult, gNdsFighterMarioFoxInitMask, gNdsFighterMarioFoxInitDeferredMask, gNdsFighterMarioFoxInitCount',
        'printf "FTR_INIT_P0=%u,%u,%u,%u,%u,%u,%u,%#x\n", gNdsFighterInitP0FKind, gNdsFighterInitP0PercentDamage, gNdsFighterInitP0ShieldHealth, gNdsFighterInitP0GA, gNdsFighterInitP0JumpsUsed, gNdsFighterInitP0HitStatus, gNdsFighterInitP0DamageKind, gNdsFighterInitP0MotionAttackID',
        'printf "FTR_INIT_P1=%u,%u,%u,%u,%u,%u,%u,%#x\n", gNdsFighterInitP1FKind, gNdsFighterInitP1PercentDamage, gNdsFighterInitP1ShieldHealth, gNdsFighterInitP1GA, gNdsFighterInitP1JumpsUsed, gNdsFighterInitP1HitStatus, gNdsFighterInitP1DamageKind, gNdsFighterInitP1MotionAttackID',
        'printf "FTR_INIT_FLOOR=%u,%u,%u,%u,%#x,%#x\n", gNdsFighterInitP0FloorProjectAttempt, gNdsFighterInitP1FloorProjectAttempt, gNdsFighterInitP0FloorProjectResult, gNdsFighterInitP1FloorProjectResult, gNdsFighterInitP0FloorLineID, gNdsFighterInitP1FloorLineID',
        'printf "FTR_INIT_DAMAGECOLL=%#x,%#x,%#x,%#x,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterInitDamageCollMask, gNdsFighterInitDamageCollNormalMask, gNdsFighterInitDamageCollJointMask, gNdsFighterInitDamageCollHalfSizeMask, gNdsFighterInitP0DamageCollCount, gNdsFighterInitP1DamageCollCount, gNdsFighterInitP0DamageCollJoint0, gNdsFighterInitP1DamageCollJoint0, gNdsFighterInitP0DamageCollSizeXBits, gNdsFighterInitP1DamageCollSizeXBits, gNdsFighterInitP0DamageCollSizeYBits, gNdsFighterInitP1DamageCollSizeYBits, gNdsFighterInitP0DamageCollSizeZBits, gNdsFighterInitP1DamageCollSizeZBits',
        'printf "FTR_INIT_DAMAGEPARTS=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterInitDamageCollPartsMask, gNdsFighterInitDamageCollMatrixMask, gNdsFighterInitDamageCollScaleMask, gNdsFighterInitP0DamageCollWorldXBits, gNdsFighterInitP1DamageCollWorldXBits, gNdsFighterInitP0DamageCollWorldYBits, gNdsFighterInitP1DamageCollWorldYBits, gNdsFighterInitP0DamageCollScaleXBits, gNdsFighterInitP1DamageCollScaleXBits',
        'printf "FTR_INIT_CALLS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterInitPhysicsStopCount, gNdsFighterInitAttackClearCount, gNdsFighterInitHitStatusPartCount, gNdsFighterInitColAnimResetCount, gNdsFighterInitProcessAttachCount, gNdsFighterInitStatusSetCount, gNdsFighterInitDisplayProbeCount',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $chain = [regex]::Match($gdbStdout, 'CHAIN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $final = [regex]::Match($gdbStdout, 'CHAIN_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $model = [regex]::Match($gdbStdout, 'FTR_MODEL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $struct = [regex]::Match($gdbStdout, 'FTR_STRUCT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $init = [regex]::Match($gdbStdout, 'FTR_INIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $p0 = [regex]::Match($gdbStdout, 'FTR_INIT_P0=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $p1 = [regex]::Match($gdbStdout, 'FTR_INIT_P1=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $floor = [regex]::Match($gdbStdout, 'FTR_INIT_FLOOR=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $damageColl = [regex]::Match($gdbStdout, 'FTR_INIT_DAMAGECOLL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $damageParts = [regex]::Match($gdbStdout, 'FTR_INIT_DAMAGEPARTS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $calls = [regex]::Match($gdbStdout, 'FTR_INIT_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 16 -or [int]$harn.Groups[3].Value -ne 9 -or [int]$harn.Groups[4].Value -ne 1 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Menu-chain Mario/Fox init harness did not start at VSMode from Title.`n$gdbStdout"
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
    if (-not $model.Success -or (Convert-MarkerUInt32 $model.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $model.Groups[2].Value) -ne 0x4654474f -or ((Convert-MarkerUInt32 $model.Groups[3].Value) -band 0xfff) -ne 0xfff -or [int]$model.Groups[4].Value -ne 2 -or [int]$model.Groups[5].Value -ne 0) {
        throw "Mario/Fox model proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $struct.Success -or (Convert-MarkerUInt32 $struct.Groups[1].Value) -ne 0x46545348 -or (Convert-MarkerUInt32 $struct.Groups[2].Value) -ne 0x46544a54 -or (Convert-MarkerUInt32 $struct.Groups[3].Value) -ne 0x46545354 -or ((Convert-MarkerUInt32 $struct.Groups[4].Value) -band 0xfff) -ne 0xfff -or (Convert-MarkerUInt32 $struct.Groups[5].Value) -ne 0x3 -or [int]$struct.Groups[6].Value -ne 2) {
        throw "Persistent FTStruct proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $init.Success -or (Convert-MarkerUInt32 $init.Groups[1].Value) -ne 0x4654494e -or (Convert-MarkerUInt32 $init.Groups[2].Value) -ne 0x4654434c -or (Convert-MarkerUInt32 $init.Groups[3].Value) -ne 0x46544446 -or ((Convert-MarkerUInt32 $init.Groups[4].Value) -band 0x3fff) -ne 0x3fff -or [int]$init.Groups[6].Value -ne 2) {
        throw "Mario/Fox init-state proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $p0.Success -or [int]$p0.Groups[1].Value -ne 0 -or [int]$p0.Groups[2].Value -ne 0 -or [int]$p0.Groups[3].Value -ne 55 -or [int]$p0.Groups[4].Value -ne 0 -or [int]$p0.Groups[5].Value -ne 0 -or [int]$p0.Groups[6].Value -ne 0 -or [int]$p0.Groups[7].Value -ne 0 -or (Convert-MarkerUInt32 $p0.Groups[8].Value) -ne 0) {
        throw "Mario init-state fields failed after menu chain.`n$gdbStdout"
    }
    if (-not $p1.Success -or [int]$p1.Groups[1].Value -ne 1 -or [int]$p1.Groups[2].Value -ne 0 -or [int]$p1.Groups[3].Value -ne 55 -or [int]$p1.Groups[4].Value -ne 0 -or [int]$p1.Groups[5].Value -ne 0 -or [int]$p1.Groups[6].Value -ne 0 -or [int]$p1.Groups[7].Value -ne 0 -or (Convert-MarkerUInt32 $p1.Groups[8].Value) -ne 0) {
        throw "Fox init-state fields failed after menu chain.`n$gdbStdout"
    }
    if (-not $floor.Success -or [int]$floor.Groups[1].Value -ne 1 -or [int]$floor.Groups[2].Value -ne 1 -or [int]$floor.Groups[3].Value -ne 1 -or [int]$floor.Groups[4].Value -ne 1 -or (Convert-MarkerUInt32 $floor.Groups[5].Value) -ne 0 -or (Convert-MarkerUInt32 $floor.Groups[6].Value) -ne 0) {
        throw "Mario/Fox bounded floor projection proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $damageColl.Success -or ((Convert-MarkerUInt32 $damageColl.Groups[1].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageColl.Groups[2].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageColl.Groups[3].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageColl.Groups[4].Value) -band 0x3) -ne 0x3 -or [int]$damageColl.Groups[5].Value -ne 10 -or [int]$damageColl.Groups[6].Value -ne 11 -or [int]$damageColl.Groups[7].Value -ne 6 -or [int]$damageColl.Groups[8].Value -ne 5 -or (Convert-MarkerUInt32 $damageColl.Groups[9].Value) -ne 0x424e0000 -or (Convert-MarkerUInt32 $damageColl.Groups[10].Value) -ne 0x424c0000 -or (Convert-MarkerUInt32 $damageColl.Groups[11].Value) -ne 0x42600000 -or (Convert-MarkerUInt32 $damageColl.Groups[12].Value) -ne 0x41d00000 -or (Convert-MarkerUInt32 $damageColl.Groups[13].Value) -ne 0x423e0000 -or (Convert-MarkerUInt32 $damageColl.Groups[14].Value) -ne 0x41b40000) {
        throw "Mario/Fox bounded damage-collision shell proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $damageParts.Success -or ((Convert-MarkerUInt32 $damageParts.Groups[1].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageParts.Groups[2].Value) -band 0x3) -ne 0x3 -or ((Convert-MarkerUInt32 $damageParts.Groups[3].Value) -band 0x3) -ne 0x3 -or (Convert-MarkerUInt32 $damageParts.Groups[8].Value) -eq 0 -or (Convert-MarkerUInt32 $damageParts.Groups[9].Value) -eq 0) {
        throw "Mario/Fox bounded damage-collision FTParts proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $calls.Success -or [int]$calls.Groups[1].Value -lt 2 -or [int]$calls.Groups[2].Value -lt 2 -or [int]$calls.Groups[3].Value -lt 2 -or [int]$calls.Groups[4].Value -lt 2 -or [int]$calls.Groups[5].Value -ne 0 -or [int]$calls.Groups[6].Value -ne 0 -or [int]$calls.Groups[7].Value -ne 0) {
        throw "Fighter init compatibility/deferred call counters failed after menu chain.`n$gdbStdout"
    }
    Write-Output ("Menu-chain Mario/Fox init harness passed: chain masks={0}/{1}/{2}, model={3}, struct={4}, init={5}, damageColl={6}/{7}/{8}/{9}, parts={10}/{11}/{12}, final=22/21" -f $chain.Groups[2].Value, $chain.Groups[4].Value, $chain.Groups[6].Value, $model.Groups[3].Value, $struct.Groups[4].Value, $init.Groups[4].Value, $damageColl.Groups[1].Value, $damageColl.Groups[2].Value, $damageColl.Groups[3].Value, $damageColl.Groups[4].Value, $damageParts.Groups[1].Value, $damageParts.Groups[2].Value, $damageParts.Groups[3].Value)
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
