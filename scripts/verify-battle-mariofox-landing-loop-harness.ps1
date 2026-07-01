param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [string]$Harness = 'battle_mariofox_landing_loop',
    [string]$Target = 'smash64ds-battle-mariofox-landing-loop',
    [string]$Build = 'build-battle-mariofox-landing-loop-harness',
    [int]$ExpectedMode = 43,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox Landing-loop',
    [string]$HarnessSelectMessage = 'Direct Landing-loop harness did not select VSBattle from Maps.'
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
$target = $Target
$build = $Build
$rom = Join-Path $root "$target.nds"
$elf = Join-Path $root "$target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stdout.log"
$stderr = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stderr.log"
$configState = $null
$emulator = $null
$scriptName = "_$($Harness)_harness.gdb"
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') { $values += [int64](Convert-MarkerUInt32 $text) }
        else { $values += [int64]$text }
    }
    return $values
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=$target BUILD=$build NDS_DEV_SCENE_HARNESS=$Harness -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw "$Label harness build did not produce the expected ROM and ELF."
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
        'printf "JUMP_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxJumpLoopResult, gNdsFighterMarioFoxJumpLoopSafeResult, gNdsFighterMarioFoxJumpLoopMask, gNdsFighterMarioFoxJumpLoopDeferredMask, gNdsFighterMarioFoxJumpLoopCount',
        'printf "LAND_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxLandingLoopResult, gNdsFighterMarioFoxLandingLoopSafeResult, gNdsFighterMarioFoxLandingLoopMask, gNdsFighterMarioFoxLandingLoopDeferredMask, gNdsFighterMarioFoxLandingLoopCount',
        'printf "LAND_STATUS=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLandingP0StatusStart, gNdsFighterLandingP1StatusStart, gNdsFighterLandingP0StatusFall, gNdsFighterLandingP1StatusFall, gNdsFighterLandingP0StatusLanding, gNdsFighterLandingP1StatusLanding, gNdsFighterLandingP0StatusWait, gNdsFighterLandingP1StatusWait',
        'printf "LAND_MOTION=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLandingP0MotionStart, gNdsFighterLandingP1MotionStart, gNdsFighterLandingP0MotionFall, gNdsFighterLandingP1MotionFall, gNdsFighterLandingP0MotionLanding, gNdsFighterLandingP1MotionLanding, gNdsFighterLandingP0MotionWait, gNdsFighterLandingP1MotionWait',
        'printf "LAND_GA=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLandingP0GAStart, gNdsFighterLandingP1GAStart, gNdsFighterLandingP0GAFall, gNdsFighterLandingP1GAFall, gNdsFighterLandingP0GALanding, gNdsFighterLandingP1GALanding, gNdsFighterLandingP0GAWait, gNdsFighterLandingP1GAWait',
        'printf "LAND_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLandingJumpAnimEndCallCount, gNdsFighterLandingFallSetStatusCallCount, gNdsFighterLandingFtMainFallStatusCount, gNdsFighterLandingSetGroundCallCount, gNdsFighterLandingSetStatusCallCount, gNdsFighterLandingFtMainLandingLightStatusCount, gNdsFighterLandingFtMainLandingHeavyStatusCount, gNdsFighterLandingEndCallCount, gNdsFighterLandingWaitSetStatusCount, gNdsFighterLandingWaitSetStatusSuccessCount',
        'printf "LAND_FRAMES=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLandingFallFrameMax, gNdsFighterLandingLandingFrameTarget, gNdsFighterLandingP0FallFrameCount, gNdsFighterLandingP1FallFrameCount, gNdsFighterLandingP0FallInterruptCount, gNdsFighterLandingP1FallInterruptCount, gNdsFighterLandingP0FallPhysicsCount, gNdsFighterLandingP1FallPhysicsCount, gNdsFighterLandingP0FallMapCount, gNdsFighterLandingP1FallMapCount, gNdsFighterLandingP0LandingFrameCount, gNdsFighterLandingP1LandingFrameCount',
        'printf "LAND_MAP=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLandingAirNoCollisionCount, gNdsFighterLandingFloorDetectCount, gNdsFighterLandingFloorClampCount, gNdsFighterLandingFastFallCount, gNdsFighterLandingHeavyDeniedCount, gNdsFighterLandingFallAerialDeniedCount, gNdsFighterLandingJumpAerialDeniedCount, gNdsFighterLandingCliffDeniedCount, gNdsFighterLandingCeilingDeniedCount, gNdsFighterLandingGADriftCount',
        'printf "LAND_MOVE=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterLandingP0FloorYMilli, gNdsFighterLandingP1FloorYMilli, gNdsFighterLandingP0RootYFallStartMilli, gNdsFighterLandingP1RootYFallStartMilli, gNdsFighterLandingP0RootYFinalMilli, gNdsFighterLandingP1RootYFinalMilli, gNdsFighterLandingP0RootDeltaXMilli, gNdsFighterLandingP1RootDeltaXMilli, gNdsFighterLandingP0RootDirectionOK, gNdsFighterLandingP1RootDirectionOK, gNdsFighterLandingP0RootFloorOK, gNdsFighterLandingP1RootFloorOK',
        'printf "LAND_VEL=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u\n", gNdsFighterLandingP0VelYFallStartMilli, gNdsFighterLandingP1VelYFallStartMilli, gNdsFighterLandingP0VelYBeforeLandingMilli, gNdsFighterLandingP1VelYBeforeLandingMilli, gNdsFighterLandingP0GroundVelAfterLandingMilli, gNdsFighterLandingP1GroundVelAfterLandingMilli, gNdsFighterLandingP0GroundVelAfterWaitMilli, gNdsFighterLandingP1GroundVelAfterWaitMilli, gNdsFighterLandingGravityCallCount, gNdsFighterLandingAirDriftCallCount, gNdsFighterLandingAirFrictionCallCount, gNdsFighterLandingGroundFrictionCallCount, gNdsFighterLandingWaitFrictionCallCount',
        'printf "LAND_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLandingDeferredInterruptCheckCount, gNdsFighterLandingFastFallCheckCount, gNdsFighterLandingGObjDelta, gNdsFighterLandingUnexpectedStatusCount, gNdsFighterLandingDeniedStatusCount, gNdsFighterLandingProcessAttachCount, gNdsFighterLandingDisplayProbeCount, gNdsFighterLandingGameplayUpdateCount, gNdsFighterLandingDrawCallCount, gNdsFighterLandingMatrixCallCount, gNdsFighterLandingRootYDriftCount, gNdsFighterLandingGADriftCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $jump = [regex]::Match($gdbStdout, 'JUMP_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'LAND_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'LAND_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $motion = [regex]::Match($gdbStdout, 'LAND_MOTION=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $ga = [regex]::Match($gdbStdout, 'LAND_GA=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'LAND_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $frames = [regex]::Match($gdbStdout, 'LAND_FRAMES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $map = [regex]::Match($gdbStdout, 'LAND_MAP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $move = [regex]::Match($gdbStdout, 'LAND_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vel = [regex]::Match($gdbStdout, 'LAND_VEL=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'LAND_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($jump.Success -and (Convert-MarkerUInt32 $jump.Groups[1].Value) -eq 0x464a4d50 -and (Convert-MarkerUInt32 $jump.Groups[2].Value) -eq 0x464a4d53 -and ((Convert-MarkerUInt32 $jump.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and [int]$jump.Groups[5].Value -eq 2) 'Prerequisite Jump-loop proof did not pass.' $gdbStdout
    Assert-Condition ($loop.Success -and (Convert-MarkerUInt32 $loop.Groups[1].Value) -eq 0x464c4e44 -and (Convert-MarkerUInt32 $loop.Groups[2].Value) -eq 0x464c4e53 -and ((Convert-MarkerUInt32 $loop.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $loop.Groups[4].Value) -eq 0xff -and [int]$loop.Groups[5].Value -eq 2) 'Landing-loop proof did not pass.' $gdbStdout
    $s = Get-Ints $status; $m = Get-Ints $motion; $g = Get-Ints $ga
    Assert-Condition (($s -join ',') -eq '22,22,26,26,31,31,10,10') 'Landing status path was not expected.' $gdbStdout
    Assert-Condition (($m -join ',') -eq '16,16,20,20,25,25,4,4') 'Landing motion path was not expected.' $gdbStdout
    Assert-Condition (($g -join ',') -eq '1,1,1,1,0,0,0,0') 'Landing GA path was not expected.' $gdbStdout
    $c = Get-Ints $calls
    Assert-Condition (($c[0] -eq 2) -and ($c[1] -eq 2) -and ($c[2] -eq 2) -and ($c[3] -eq 2) -and ($c[4] -eq 2) -and ($c[5] -eq 2) -and ($c[6] -eq 0) -and ($c[7] -eq 2) -and ($c[8] -eq 2) -and ($c[9] -eq 2)) 'Landing original call counters were not expected.' $gdbStdout
    $f = Get-Ints $frames
    Assert-Condition (($f[0] -ge 64) -and ($f[1] -ge 1) -and ($f[2] -gt 0) -and ($f[3] -gt 0) -and ($f[2] -le $f[0]) -and ($f[3] -le $f[0]) -and ($f[4] -eq $f[2]) -and ($f[5] -eq $f[3]) -and ($f[6] -eq $f[2]) -and ($f[7] -eq $f[3]) -and ($f[8] -eq $f[2]) -and ($f[9] -eq $f[3]) -and ($f[10] -eq $f[1]) -and ($f[11] -eq $f[1])) 'Landing frame counters were not expected.' $gdbStdout
    $mp = Get-Ints $map
    Assert-Condition (($mp[0] -gt 0) -and ($mp[1] -eq 2) -and ($mp[2] -eq 2) -and (@($mp[3..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Landing map/safety counters were not expected.' $gdbStdout
    $mv = Get-Ints $move
    Assert-Condition (($mv[2] -gt $mv[0]) -and ($mv[3] -gt $mv[1]) -and ($mv[4] -eq $mv[0]) -and ($mv[5] -eq $mv[1]) -and ($mv[8] -eq 1) -and ($mv[9] -eq 1) -and ($mv[10] -eq 1) -and ($mv[11] -eq 1)) 'Landing movement/floor markers failed.' $gdbStdout
    $v = Get-Ints $vel
    Assert-Condition (($v[2] -le 0) -and ($v[3] -le 0) -and ($v[8] -ge ($f[2] + $f[3])) -and ($v[11] -ge 2) -and ($v[12] -ge 2) -and ([Math]::Abs($v[6]) -le [Math]::Abs($v[4])) -and ([Math]::Abs($v[7]) -le [Math]::Abs($v[5]))) 'Landing velocity/friction markers failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition (($sf[0] -gt 0) -and ($sf[1] -ge ($f[2] + $f[3])) -and (@($sf[2..11] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Landing safe escape counters were not zero.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after Landing-loop proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 fall={0}/{1} landing={2}/{3} wait={4}/{5} frames={6}/{7} floor={8}/{9} safe=1" -f $s[2], $s[3], $s[4], $s[5], $s[6], $s[7], $f[2], $f[3], $mv[0], $mv[1])
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
