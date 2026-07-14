param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild
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
$target = 'smash64ds-battle-playable-canonical-hwtri'
$build = 'build-battle-playable-canonical-hwtri-harness'
$rom = Join-Path $root "$target.nds"
$elf = Join-Path $root "$target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir `
    -Root $root `
    -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-playable-fireball-render.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-playable-fireball-render.stderr.log'
$scriptName = '_battle_playable_fireball_render.gdb'
$configState = $null
$emulator = $null

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}

function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') {
            $values += [int64](Convert-MarkerUInt32 $text)
        } else {
            $values += [int64]$text
        }
    }
    return $values
}

function Get-ElfSymbolAddress {
    param([string]$Name)
    $escapedName = [regex]::Escape($Name)
    $line = @(& $nm -a $elf) |
        Where-Object { $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escapedName$" } |
        Select-Object -First 1
    if (-not $line) { throw "ELF symbol not found: $Name" }
    $match = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($match.Groups[1].Value, 16))
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$nm = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-nm.exe'
if (-not $NoBuild) {
    & make -C $root `
        "TARGET=$target" `
        "BUILD=$build" `
        NDS_DEV_SCENE_HARNESS=battle_playable_realtime `
        NDS_DEV_LIVE_INPUT_PREVIEW=1 `
        NDS_HARNESS_FAST_LOGIC=0 `
        NDS_RENDERER_HW_TRIANGLES=1 `
        NDS_RENDERER_PROFILE_LEVEL=0 `
        NDS_DEBUG_HUD=0 `
        -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Canonical battle_playable build did not produce the expected ROM and ELF.'
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
if (-not (Test-Path $nm)) { throw "ELF symbol tool not found: $nm" }
$playbackPadsAddress = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$playbackConnectedAddress = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$playbackEnabledAddress = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath `
        -GdbPort $verifierContext.GdbPort `
        -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator `
        -Port $verifierContext.GdbPort | Out-Null
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        # ifcommon.c:3167-3179 is the original BattleShip transition that
        # starts the match timer. Breaking on the assignment avoids a slow
        # debugger-side condition at every countdown frame.
        'tbreak ifcommon.c:3175',
        'continue',
        # melonDS cannot safely execute inferior function calls across this
        # ARM/Thumb boundary. Resolve the private playback storage from this
        # exact ELF and write it directly; osContGetReadData still consumes it
        # through the normal controller path as real B input.
        ('set {{unsigned short}}0x{0:x8} = 0x4000' -f $playbackPadsAddress),
        ('set {{unsigned int}}0x{0:x8} = 3' -f $playbackConnectedAddress),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $playbackEnabledAddress),
        'tbreak ftmariospecialn.c:ftMarioSpecialNSetStatus',
        'continue',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $playbackPadsAddress),
        'tbreak battleship_mario_fireball.c:wpMarioFireballMakeWeapon',
        'continue',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted',
        'printf "RENDER_PROFILE_LEVEL=%u\n", gNdsRendererProfileLevel',
        'printf "FIREBALL_SOURCE=%u,%u,%u,%u,%u,%#x\n", gNdsFighterProjectileProofSpawnCallCount, gNdsFighterProjectileProofSpawnSuccessCount, gNdsFighterProjectileProofDamageMax, gNdsFighterProjectileProofLifetimeMax, gNdsFighterProjectileProofWeaponCountMax, gNdsFighterProjectileProofKindMask',
        'printf "WEAPON_RENDER=%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsWeaponRendererCaptureCount, gNdsWeaponRendererDObjDrawCount, gNdsWeaponRendererSubmitCount, gNdsWeaponRendererVisibleDrawCount, gNdsWeaponRendererTriangleCount, gNdsWeaponRendererTextureReadyCount, gNdsWeaponRendererTextureRejectCount, gNdsWeaponRendererKindMask, gNdsWeaponRendererCallbackKind, gNdsWeaponRendererNoZCount, gNdsWeaponRendererMovingDrawCount, gNdsWeaponRendererFireballSubmitCount, gNdsWeaponRendererFireballTriangleCount, gNdsWeaponRendererFireballVisibleDrawCount, gNdsWeaponRendererRejectedDrawCount',
        ('printf "INPUT=%u,%#x,%u,%u,%#x\n", *(unsigned int*)0x{0:x8}, *(unsigned int*)0x{1:x8}, gNdsControllerPlaybackFrameCount, gNdsControllerPlaybackReadCount, *(unsigned short*)0x{2:x8}' -f $playbackEnabledAddress, $playbackConnectedAddress, $playbackPadsAddress),
        'printf "MEMARENA=%#x,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript `
        -Gdb $Gdb `
        -Elf $elf `
        -Root $root `
        -Commands $gdbCommands `
        -ScriptName $scriptName).Stdout

    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $profile = [regex]::Match($gdbStdout, 'RENDER_PROFILE_LEVEL=([0-9]+)')
    $source = [regex]::Match($gdbStdout, 'FIREBALL_SOURCE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $weapon = [regex]::Match($gdbStdout, 'WEAPON_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'INPUT=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $memory = [regex]::Match($gdbStdout, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $hv = Get-Ints $harn
    $sv = Get-Ints $scene
    $fv = Get-Ints $source
    $wv = Get-Ints $weapon
    $iv = Get-Ints $input
    $mv = Get-Ints $memory

    Assert-Condition ($harn.Success -and $hv[0] -eq 0x4841524e -and $hv[1] -eq 163 -and $hv[2] -eq 22 -and $hv[3] -eq 21 -and $hv[4] -eq 0) 'Fireball verifier did not run the registered battle_playable realtime scene.' $gdbStdout
    Assert-Condition ($scene.Success -and $sv[0] -eq 22 -and $sv[1] -eq 21 -and $sv[2] -eq 6 -and $sv[3] -eq 1 -and $sv[4] -eq 1) 'Fireball input did not occur naturally after GO with the original timer running.' $gdbStdout
    Assert-Condition ($profile.Success -and [int64]$profile.Groups[1].Value -eq 0) 'Fireball verifier did not use the shipped renderer profile.' $gdbStdout
    Assert-Condition ($source.Success -and $fv[0] -eq 1 -and $fv[1] -eq 1 -and $fv[2] -eq 7 -and $fv[3] -eq 140 -and $fv[4] -eq 1 -and $fv[5] -eq 1) 'Original Mario Fireball source creation/attributes did not run exactly once from input.' $gdbStdout
    Assert-Condition ($weapon.Success -and $wv[2] -ge 4 -and $wv[0] -eq $wv[2] -and $wv[1] -eq $wv[2] -and $wv[3] -eq $wv[2] -and $wv[4] -eq (2 * $wv[2]) -and $wv[5] -eq $wv[2] -and $wv[6] -eq 0 -and $wv[7] -eq 1 -and $wv[8] -eq 0x444c4831 -and $wv[9] -eq $wv[2] -and ($wv[10] + 1) -eq $wv[2] -and $wv[11] -eq $wv[2] -and $wv[12] -eq (2 * $wv[11]) -and $wv[13] -eq $wv[11] -and $wv[14] -eq 0) 'Every captured Mario Fireball callback was not a textured, rejection-free source DLHEAD1 no-Z two-triangle hardware draw with natural motion.' $gdbStdout
    Assert-Condition ($input.Success -and $iv[0] -eq 1 -and (($iv[1] -band 3) -eq 3) -and $iv[3] -gt 0 -and $iv[4] -eq 0) 'Fireball verifier did not consume and release the real playback B input after status entry.' $gdbStdout
    Assert-Condition ($memory.Success -and $mv[0] -eq 0x4d4c4544 -and $mv[1] -eq 22 -and $mv[2] -ge 131072) 'Fireball renderer violated the P1 arena reserve floor.' $gdbStdout
    Write-Output ("battle_playable Fireball render passed: source={0}/{1} damage={2} life={3} draw={4} tri={5} texture={6} move={7} reserve={8}" -f $fv[0], $fv[1], $fv[2], $fv[3], $wv[2], $wv[4], $wv[5], $wv[10], $mv[2])
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
