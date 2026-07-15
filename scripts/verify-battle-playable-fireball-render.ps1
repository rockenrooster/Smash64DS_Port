[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 180,
    [string]$Screenshot = ''
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
$target = 'smash64ds-battle-playable-hwtri'
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
$captureScript = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
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

function Convert-UInt32BitsToSingle {
    param([int64]$Bits)
    return [BitConverter]::ToSingle(
        [BitConverter]::GetBytes([uint32]$Bits), 0)
}

function Test-FiniteSingle {
    param([single]$Value)
    return (-not [Single]::IsNaN($Value)) -and
        (-not [Single]::IsInfinity($Value))
}

function Convert-UInt32ToInt32 {
    param([int64]$Value)
    return [BitConverter]::ToInt32(
        [BitConverter]::GetBytes([uint32]$Value), 0)
}

function Resolve-VisibilityOutput {
    param([string]$Path)

    $visibilityDir = [System.IO.Path]::GetFullPath(
        (Join-Path $root 'artifacts\visibility'))
    $resolved = if ([string]::IsNullOrWhiteSpace($Path)) {
        $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff'
        Join-Path $visibilityDir "${stamp}_fireball-long-travel-p$PID.png"
    } elseif ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $root $Path))
    }
    $visibilityPrefix = $visibilityDir.TrimEnd('\', '/') +
        [System.IO.Path]::DirectorySeparatorChar
    Assert-Condition ($resolved.StartsWith(
            $visibilityPrefix,
            [System.StringComparison]::OrdinalIgnoreCase)) `
        "Fireball captures must stay under '$visibilityDir': $resolved"
    Assert-Condition ([System.IO.Path]::GetExtension($resolved) -ieq '.png') `
        "Fireball evidence capture must be a PNG: $resolved"
    return $resolved
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
if (-not (Test-Path $captureScript)) {
    throw "melonDS capture helper not found: $captureScript"
}
$playbackPadsAddress = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$playbackConnectedAddress = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$playbackEnabledAddress = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
$screenshotPath = Resolve-VisibilityOutput $Screenshot
New-Item -ItemType Directory -Path $logDir,
    (Split-Path -Parent $screenshotPath) -Force | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath `
        -GdbPort $verifierContext.GdbPort `
        -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr, $screenshotPath -Force `
        -ErrorAction SilentlyContinue
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
    $captureScriptGdb = $captureScript.Replace('\', '/')
    $screenshotGdb = $screenshotPath.Replace('\', '/')
    $captureCommand =
        'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" -EmulatorProcessId {1} -Output "{2}"' -f
        $captureScriptGdb, $emulator.Id, $screenshotGdb
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
        'set $fb_map_entries = 0',
        'set $fb_weapon = 0',
        'set $fb_wp = 0',
        'break ndsMarioFireballProcMapProbe',
        'commands',
        'silent',
        'set $fb_weapon = (GObj *)$r0',
        'set $fb_wp = (WPStruct *)$fb_weapon->user_data.p',
        'set $fb_map_entries = $fb_map_entries + 1',
        'set $fb_pre_mask_prev = $fb_wp->coll_data.mask_prev',
        'set $fb_pre_mask_curr = $fb_wp->coll_data.mask_curr',
        'set $fb_pre_vx_bits = *(unsigned int *)&$fb_wp->physics.vel_air.x',
        'set $fb_pre_vy_bits = *(unsigned int *)&$fb_wp->physics.vel_air.y',
        'continue',
        'end',
        'tbreak wpmariofireball.c:113 if (($fb_wp != 0) && (($fb_wp->coll_data.mask_curr & 0x800) != 0))',
        'continue',
        'set $fb_post_mask_curr = $fb_wp->coll_data.mask_curr',
        'set $fb_post_mask_stat = $fb_wp->coll_data.mask_stat',
        'set $fb_floor_flags = $fb_wp->coll_data.floor_flags',
        'set $fb_floor_line = $fb_wp->coll_data.floor_line_id',
        'set $fb_post_vx_bits = *(unsigned int *)&$fb_wp->physics.vel_air.x',
        'set $fb_post_vy_bits = *(unsigned int *)&$fb_wp->physics.vel_air.y',
        'set $fb_angle_x_bits = *(unsigned int *)&$fb_wp->coll_data.floor_angle.x',
        'set $fb_angle_y_bits = *(unsigned int *)&$fb_wp->coll_data.floor_angle.y',
        'set $fb_lifetime_rebound = $fb_wp->lifetime',
        'set $fb_destroy_rebound = gNdsFighterProjectileProofMapDestroyCount',
        'delete breakpoints',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'set $fb_scan = (GObj *)gGCCommonLinks[5]',
        'set $fb_scan_count = 0',
        'set $fb_live = 0',
        'while (($fb_scan != 0) && ($fb_scan_count < 32))',
        'if ($fb_scan == $fb_weapon)',
        'set $fb_live = 1',
        'end',
        'set $fb_scan = (GObj *)$fb_scan->link_next',
        'set $fb_scan_count = $fb_scan_count + 1',
        'end',
        'set $fb_scan_overflow = ($fb_scan != 0)',
        'set $fb_wp_match = 0',
        'set $fb_lifetime_frame = -1',
        'if ($fb_live != 0)',
        'set $fb_wp_match = ((WPStruct *)$fb_weapon->user_data.p == $fb_wp)',
        'set $fb_lifetime_frame = $fb_wp->lifetime',
        'end',
        'set $fb_destroy_frame = gNdsFighterProjectileProofMapDestroyCount',
        'printf "FIREBALL_REBOUND=%u,%#x,%#x,%#x,%#x,%#x,%d,%#x,%#x,%#x,%#x,%#x,%#x,%d,%u,%u,%u,%u,%u,%d\n", $fb_map_entries, $fb_pre_mask_prev, $fb_pre_mask_curr, $fb_post_mask_curr, $fb_post_mask_stat, $fb_floor_flags, $fb_floor_line, $fb_pre_vx_bits, $fb_pre_vy_bits, $fb_post_vx_bits, $fb_post_vy_bits, $fb_angle_x_bits, $fb_angle_y_bits, $fb_lifetime_rebound, $fb_destroy_rebound, $fb_destroy_frame, $fb_live, $fb_scan_overflow, $fb_wp_match, $fb_lifetime_frame',
        # Continue the same source-created object to its fortieth natural
        # hardware submit. The existing post-submit line replaces a production-
        # only marker while proving the custom 0x47 matrix at long travel.
        'tbreak reloc_backend_movement.c:12928 if gNdsWeaponRendererFireballSubmitCount == 40',
        'continue',
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted',
        'printf "RENDER_PROFILE_LEVEL=%u\n", gNdsRendererProfileLevel',
        'printf "FIREBALL_SOURCE=%u,%u,%u,%u,%u,%#x\n", gNdsFighterProjectileProofSpawnCallCount, gNdsFighterProjectileProofSpawnSuccessCount, gNdsFighterProjectileProofDamageMax, gNdsFighterProjectileProofLifetimeMax, gNdsFighterProjectileProofWeaponCountMax, gNdsFighterProjectileProofKindMask',
        'printf "WEAPON_RENDER=%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsWeaponRendererCaptureCount, gNdsWeaponRendererDObjDrawCount, gNdsWeaponRendererSubmitCount, gNdsWeaponRendererVisibleDrawCount, gNdsWeaponRendererTriangleCount, gNdsWeaponRendererTextureReadyCount, gNdsWeaponRendererTextureRejectCount, gNdsWeaponRendererKindMask, gNdsWeaponRendererCallbackKind, gNdsWeaponRendererNoZCount, gNdsWeaponRendererMovingDrawCount, gNdsWeaponRendererFireballSubmitCount, gNdsWeaponRendererFireballTriangleCount, gNdsWeaponRendererFireballVisibleDrawCount, gNdsWeaponRendererRejectedDrawCount',
        'printf "FIREBALL_TRANSFORM=%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsWeaponRendererFireballCustom47AppliedCount, gNdsWeaponRendererFireballCustom47MismatchCount, gNdsRendererAdapterCustom47DetectedCount, gNdsRendererAdapterCustom47AppliedCount, gNdsRendererAdapterCustom47RejectCount, gNdsRendererAdapterCustom47TranslationMismatchCount, gNdsRendererAdapterCustom47LastXObjsNum, gNdsRendererAdapterCustom47LastKinds, gNdsRendererAdapterCustom47LastRotateXBits, gNdsRendererAdapterCustom47LastRotateYBits, gNdsRendererAdapterCustom47LastTranslateX20p12, gNdsRendererAdapterCustom47LastTranslateY20p12, gNdsRendererAdapterCustom47LastTranslateZ20p12, gNdsWeaponRendererFireballFirstXBits, gNdsWeaponRendererFireballFirstYBits, gNdsWeaponRendererFireballLastXBits, gNdsWeaponRendererFireballLastYBits',
        'printf "MP_TOPOLOGY=%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x\n", gNdsCollisionRuntimeDiagnostics.topology_build_attempts, gNdsCollisionRuntimeDiagnostics.topology_build_successes, gNdsCollisionRuntimeDiagnostics.topology_rebuilds, gNdsCollisionRuntimeDiagnostics.topology_lines, gNdsCollisionRuntimeDiagnostics.topology_shared_directed, gNdsCollisionRuntimeDiagnostics.topology_orphan_endpoints, gNdsCollisionRuntimeDiagnostics.topology_reversed_lines, gNdsCollisionRuntimeDiagnostics.topology_ambiguous_endpoints, gNdsCollisionRuntimeDiagnostics.topology_invalid, gNdsCollisionRuntimeDiagnostics.topology_hash',
        'printf "MP_FLOOR=%u,%u,%u,%u,%u\n", gNdsCollisionRuntimeDiagnostics.floor_sweep_calls, gNdsCollisionRuntimeDiagnostics.floor_sweep_hits, gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_sweeps, gNdsCollisionRuntimeDiagnostics.floor_flat_ascending_accepts, gNdsCollisionRuntimeDiagnostics.floor_adj_ambiguous',
        ('printf "INPUT=%u,%#x,%u,%u,%#x\n", *(unsigned int*)0x{0:x8}, *(unsigned int*)0x{1:x8}, gNdsControllerPlaybackFrameCount, gNdsControllerPlaybackReadCount, *(unsigned short*)0x{2:x8}' -f $playbackEnabledAddress, $playbackConnectedAddress, $playbackPadsAddress),
        'printf "MEMARENA=%#x,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom',
        $captureCommand,
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript `
        -Gdb $Gdb `
        -Elf $elf `
        -Root $root `
        -Commands $gdbCommands `
        -ScriptName $scriptName `
        -TimeoutSeconds $TimeoutSeconds).Stdout

    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $profile = [regex]::Match($gdbStdout, 'RENDER_PROFILE_LEVEL=([0-9]+)')
    $source = [regex]::Match($gdbStdout, 'FIREBALL_SOURCE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $weapon = [regex]::Match($gdbStdout, 'WEAPON_RENDER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $transform = [regex]::Match($gdbStdout, 'FIREBALL_TRANSFORM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $rebound = [regex]::Match($gdbStdout, 'FIREBALL_REBOUND=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $topology = [regex]::Match($gdbStdout, 'MP_TOPOLOGY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $floor = [regex]::Match($gdbStdout, 'MP_FLOOR=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'INPUT=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $memory = [regex]::Match($gdbStdout, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $hv = Get-Ints $harn
    $sv = Get-Ints $scene
    $fv = Get-Ints $source
    $wv = Get-Ints $weapon
    $xv = Get-Ints $transform
    $rv = Get-Ints $rebound
    $tv = Get-Ints $topology
    $flv = Get-Ints $floor
    $iv = Get-Ints $input
    $mv = Get-Ints $memory

    Assert-Condition $rebound.Success 'Fireball never reached a genuine source floor rebound.' $gdbStdout
    Assert-Condition $transform.Success 'Fireball never reached the natural long-travel transform marker.' $gdbStdout
    $preVx = [double](Convert-UInt32BitsToSingle $rv[7])
    $preVy = [double](Convert-UInt32BitsToSingle $rv[8])
    $postVx = [double](Convert-UInt32BitsToSingle $rv[9])
    $postVy = [double](Convert-UInt32BitsToSingle $rv[10])
    $normalX = [double](Convert-UInt32BitsToSingle $rv[11])
    $normalY = [double](Convert-UInt32BitsToSingle $rv[12])
    $preSpeedSq = ($preVx * $preVx) + ($preVy * $preVy)
    $postSpeedSq = ($postVx * $postVx) + ($postVy * $postVy)
    $normalSq = ($normalX * $normalX) + ($normalY * $normalY)
    $dot = ($preVx * $normalX) + ($preVy * $normalY)
    $expectedPostVx = 0.85 * ($preVx - (2.0 * $dot * $normalX))
    $expectedPostVy = 0.85 * ($preVy - (2.0 * $dot * $normalY))
    $rotateX = [double](Convert-UInt32BitsToSingle $xv[8])
    $rotateY = [double](Convert-UInt32BitsToSingle $xv[9])
    $firstX = [double](Convert-UInt32BitsToSingle $xv[13])
    $firstY = [double](Convert-UInt32BitsToSingle $xv[14])
    $lastX = [double](Convert-UInt32BitsToSingle $xv[15])
    $lastY = [double](Convert-UInt32BitsToSingle $xv[16])
    $translateX = [double](Convert-UInt32ToInt32 $xv[10]) / 4096.0
    $translateY = [double](Convert-UInt32ToInt32 $xv[11]) / 4096.0
    $translateZ = [double](Convert-UInt32ToInt32 $xv[12]) / 4096.0
    $horizontalTravel = [Math]::Abs($lastX - $firstX)

    Assert-Condition (Test-Path -LiteralPath $screenshotPath -PathType Leaf) `
        "Fireball run did not produce its evidence screenshot: $screenshotPath" `
        $gdbStdout
    Assert-Condition ((Get-Item -LiteralPath $screenshotPath).Length -gt 1024) `
        "Fireball evidence screenshot is unexpectedly small: $screenshotPath" `
        $gdbStdout
    Add-Type -AssemblyName System.Drawing
    $screenshotBitmap = [System.Drawing.Bitmap]::FromFile($screenshotPath)
    try {
        Assert-Condition ($screenshotBitmap.Width -eq
                $script:MelonDSCanonicalWindowWidth -and
                $screenshotBitmap.Height -eq
                $script:MelonDSCanonicalWindowHeight) `
            ("Fireball evidence screenshot did not use the canonical {0}x{1} window: {2}" -f
                $script:MelonDSCanonicalWindowWidth,
                $script:MelonDSCanonicalWindowHeight,
                $screenshotPath) `
            $gdbStdout
    } finally {
        $screenshotBitmap.Dispose()
    }
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $screenshotPath `
        -MinDifferentFraction 0.01 `
        -MinDominantGreenFraction 0.03 `
        -MinNonWhiteNonGreenFraction 0.20 `
        -WindowScaledCapture
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Assert-Condition ($harn.Success -and $hv[0] -eq 0x4841524e -and $hv[1] -eq 163 -and $hv[2] -eq 22 -and $hv[3] -eq 21 -and $hv[4] -eq 0) 'Fireball verifier did not run the registered battle_playable realtime scene.' $gdbStdout
    Assert-Condition ($scene.Success -and $sv[0] -eq 22 -and $sv[1] -eq 21 -and $sv[2] -eq 6 -and $sv[3] -eq 1 -and $sv[4] -eq 1) 'Fireball input did not occur naturally after GO with the original timer running.' $gdbStdout
    Assert-Condition ($profile.Success -and [int64]$profile.Groups[1].Value -eq 0) 'Fireball verifier did not use the shipped renderer profile.' $gdbStdout
    Assert-Condition ($source.Success -and $fv[0] -eq 1 -and $fv[1] -eq 1 -and $fv[2] -eq 7 -and $fv[3] -eq 140 -and $fv[4] -eq 1 -and $fv[5] -eq 1) 'Original Mario Fireball source creation/attributes did not run exactly once from input.' $gdbStdout
    Assert-Condition ($rv[0] -ge 1 -and (($rv[1] -band 0x800) -eq 0) -and $rv[2] -eq 0 -and (($rv[3] -band 0x800) -ne 0) -and (($rv[4] -band 0x800) -ne 0) -and $rv[6] -ge 0) 'Fireball trace was not the first valid floor-mask rising edge.' $gdbStdout
    Assert-Condition ((Test-FiniteSingle ([single]$preVx)) -and (Test-FiniteSingle ([single]$preVy)) -and (Test-FiniteSingle ([single]$postVx)) -and (Test-FiniteSingle ([single]$postVy)) -and (Test-FiniteSingle ([single]$normalX)) -and (Test-FiniteSingle ([single]$normalY)) -and $preVy -lt 0.0 -and $postVy -gt 0.0) 'Fireball floor rebound velocities or normal were invalid.' $gdbStdout
    Assert-Condition ([Math]::Abs($normalSq - 1.0) -le 0.02 -and [Math]::Abs($postVx - $expectedPostVx) -le 0.05 -and [Math]::Abs($postVy - $expectedPostVy) -le 0.05) 'Fireball did not use the original reflected floor normal and 0.85 component scaling.' $gdbStdout
    Assert-Condition ($preSpeedSq -gt 0.0 -and $postSpeedSq -ge 900.0 -and [Math]::Abs(($postSpeedSq / $preSpeedSq) - 0.7225) -le 0.0025) 'Fireball rebound speed did not preserve 0.85 scaling or the source post-rebound minimum of 30.' $gdbStdout
    Assert-Condition ($rv[14] -eq $rv[15] -and $rv[16] -eq 1 -and $rv[17] -eq 0 -and $rv[18] -eq 1 -and $rv[19] -eq $rv[13]) 'The same Fireball object did not survive its source rebound callback.' $gdbStdout
    Assert-Condition ($topology.Success -and $tv[0] -eq 1 -and $tv[1] -eq 1 -and $tv[2] -eq 0 -and $tv[3] -eq 7 -and $tv[4] -eq 8 -and $tv[5] -eq 6 -and $tv[6] -eq 5 -and $tv[7] -eq 0 -and $tv[8] -eq 0 -and $tv[9] -eq 3903148810) 'Live Pupupu topology did not match BattleShip shared-vertex-ID construction.' $gdbStdout
    Assert-Condition ($floor.Success -and $flv[0] -gt 0 -and $flv[1] -gt 0 -and $flv[3] -eq 0 -and $flv[4] -eq 0) 'Live floor acquisition accepted an ascending/ambiguous collision or never hit.' $gdbStdout
    Assert-Condition ($weapon.Success -and $wv[2] -eq 40 -and $wv[0] -eq $wv[2] -and $wv[1] -eq $wv[2] -and $wv[3] -eq $wv[2] -and $wv[4] -eq (2 * $wv[2]) -and $wv[5] -eq $wv[2] -and $wv[6] -eq 0 -and $wv[7] -eq 1 -and $wv[8] -eq 0x444c4831 -and $wv[9] -eq $wv[2] -and ($wv[10] + 1) -eq $wv[2] -and $wv[11] -eq $wv[2] -and $wv[12] -eq (2 * $wv[11]) -and $wv[13] -eq $wv[11] -and $wv[14] -eq 0) 'The forty captured Mario Fireball callbacks were not textured, rejection-free source DLHEAD1 no-Z two-triangle hardware draws with natural motion.' $gdbStdout
    Assert-Condition ($xv[0] -eq 40 -and $xv[1] -eq 0 -and
        $xv[2] -ge 40 -and $xv[3] -eq $xv[2] -and $xv[4] -eq 0 -and
        $xv[5] -eq 0 -and $xv[6] -eq 2 -and $xv[7] -eq 0x4712) `
        'The source 0x47 Fireball MVP callback was not applied exactly once per Fireball draw without rejection or translation drift.' `
        $gdbStdout
    Assert-Condition ((Test-FiniteSingle ([single]$rotateX)) -and (Test-FiniteSingle ([single]$rotateY)) -and [Math]::Abs($rotateX) -gt 0.01 -and (Test-FiniteSingle ([single]$firstX)) -and (Test-FiniteSingle ([single]$firstY)) -and (Test-FiniteSingle ([single]$lastX)) -and (Test-FiniteSingle ([single]$lastY)) -and $horizontalTravel -gt 500.0) 'Fireball custom rotation or natural long-distance travel was invalid.' $gdbStdout
    Assert-Condition ([Math]::Abs($translateX) -lt 524288.0 -and [Math]::Abs($translateY) -lt 524288.0 -and [Math]::Abs($translateZ) -lt 524288.0) 'Fireball source MVP translation row overflowed DS 20.12 range.' $gdbStdout
    Assert-Condition ($input.Success -and $iv[0] -eq 1 -and (($iv[1] -band 3) -eq 3) -and $iv[3] -gt 0 -and $iv[4] -eq 0) 'Fireball verifier did not consume and release the real playback B input after status entry.' $gdbStdout
    Assert-Condition ($memory.Success -and $mv[0] -eq 0x4d4c4544 -and $mv[1] -eq 22 -and $mv[2] -ge 131072) 'Fireball renderer violated the P1 arena reserve floor.' $gdbStdout
    Write-Output ("battle_playable Fireball source-MVP render passed: source={0}/{1} maps={2} floor={3} speed={4:N2}->{5:N2} draw={6} travel={7:N1} reserve={8} capture={9}" -f $fv[0], $fv[1], $rv[0], $rv[6], [Math]::Sqrt($preSpeedSq), [Math]::Sqrt($postSpeedSq), $wv[2], $horizontalTravel, $mv[2], $screenshotPath)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            if (-not (Test-Path -LiteralPath $screenshotPath -PathType Leaf)) {
                try {
                    & $captureScript `
                        -EmulatorProcessId $emulator.Id `
                        -Output $screenshotPath | Out-Null
                } catch {
                    Write-Warning (
                        'Fallback Fireball evidence capture failed: ' +
                        $_.Exception.Message)
                }
            }
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
}
