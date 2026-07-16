[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 300,
    [switch]$NoBuild,
    [string]$Screenshot = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Join-Path $root 'smash64ds-battle-playable-hwtri.nds'
$elf = Join-Path $root 'smash64ds-battle-playable-hwtri.elf'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$melonDsPath = $context.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir `
    -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-playable-fox-recovery.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-playable-fox-recovery.stderr.log'
$configState = $null
$emulator = $null

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context = '')
    if ($Condition) { return }
    if ($Context) { throw "$Message`n$Context" }
    throw $Message
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
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = $script:ElfSymbols |
        Where-Object { $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$" } |
        Select-Object -First 1
    if (-not $line) { throw "ELF symbol not found: $Name" }
    $match = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($match.Groups[1].Value, 16))
}

if (-not $NoBuild) {
    & make -C $root `
        TARGET=smash64ds-battle-playable-hwtri `
        BUILD=build-battle-playable-canonical-hwtri-harness `
        NDS_DEV_SCENE_HARNESS=battle_playable_realtime `
        NDS_DEV_LIVE_INPUT_PREVIEW=1 `
        NDS_HARNESS_FAST_LOGIC=0 `
        NDS_RENDERER_HW_TRIANGLES=1 `
        NDS_RENDERER_PROFILE_LEVEL=0 `
        NDS_DEBUG_HUD=0 `
        -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($path in @($rom, $elf, $melonDsPath, $Gdb, $nm)) {
    Assert-Condition (Test-Path -LiteralPath $path -PathType Leaf) `
        "Required Fox-recovery artifact is missing: $path"
}
$script:ElfSymbols = @(& $nm -a $elf)
Assert-Condition ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
$pads = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$connected = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$enabled = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
$frameMarker = Get-ElfSymbolAddress 'ndsBattlePlayableFrameCompleteMarker'
$timeUp = Get-ElfSymbolAddress 'ifCommonAnnounceTimeUpInitInterface'

$visibilityDir = Join-Path $root 'artifacts\visibility'
$screenshotPath = if ([string]::IsNullOrWhiteSpace($Screenshot)) {
    Join-Path $visibilityDir (
        (Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff') +
        '_fox-recovery.png')
} elseif ([System.IO.Path]::IsPathRooted($Screenshot)) {
    [System.IO.Path]::GetFullPath($Screenshot)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $Screenshot))
}
$visibilityPrefix = [System.IO.Path]::GetFullPath($visibilityDir).TrimEnd('\') + '\'
Assert-Condition ($screenshotPath.StartsWith(
        $visibilityPrefix,
        [System.StringComparison]::OrdinalIgnoreCase)) `
    'Fox-recovery screenshots must stay under artifacts\visibility.'
New-Item -ItemType Directory -Force -Path $logDir, $visibilityDir | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath -GdbPort $context.GdbPort `
        -Persistent:([bool]$context.PersistentConfig)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList ('"{0}"' -f $rom) -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru

    $capture = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
    $captureCommand = (
        'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" ' +
        '-EmulatorProcessId {1} -Output "{2}"') -f
        $capture.Replace('\', '/'), $emulator.Id,
        $screenshotPath.Replace('\', '/')

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set breakpoint pending off',
        'set remotetimeout 10',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        # BattleShip starts the source timer here. The driver writes only the
        # external player-0 pad after GO; it never writes fighter or map state.
        'tbreak ifcommon.c:3175',
        'continue',
        'set $mario = (GObj *)gSCManagerBattleState->players[0].fighter_gobj',
        'set $fox = (GObj *)gSCManagerBattleState->players[1].fighter_gobj',
        'set $mfp = (FTStruct *)$mario->user_data.p',
        'set $ffp = (FTStruct *)$fox->user_data.p',
        'set $mroot = (DObj *)$mario->obj',
        'set $froot = (DObj *)$fox->obj',
        'set $frame = 0',
        'set $outcome = 0',
        'set $recover_seen = 0',
        'set $recover_frames = 0',
        'set $recover_start_x = 0',
        'set $recover_start_y = 0',
        'set $recover_start_ga = -1',
        'set $recover_start_line = -2',
        'set $recover_start_status = -1',
        'set $recover_start_input = -1',
        'set $return_x = 0',
        'set $return_y = 0',
        'set $return_line = -2',
        'set $return_status = -1',
        'set $start_percent = $ffp->percent_damage',
        'set $max_percent = $ffp->percent_damage',
        'set $start_falls = gSCManagerBattleState->players[1].falls',
        'set $recover_counter_base = gNdsFTComputerRecoveryFrames',
        'set $objective_mask_base = gNdsFTComputerObjectiveMask',
        'set $input_reads_base = gNdsControllerPlaybackReadCount',
        'set $cpu_process_base = gNdsFTComputerProcessCount',
        'set $fgm_calls_base = gNdsAudioFgmPlayCalls',
        'set $fgm_supported_base = gNdsAudioFgmSupportedPlayCount',
        'set $fgm_unsupported_base = gNdsAudioFgmUnsupportedCallCount',
        'set $fgm_lookup_fail_base = gNdsAudioFgmIncludedLookupFailCount',
        'set $fgm_play_fail_base = gNdsAudioFgmPlayFailCount',
        'set $fgm_pool_base = gNdsAudioFgmPoolExhaustCount',
        'set $fgm_generation_base = gNdsAudioFgmGenerationMismatchCount',
        'set $fgm_stale_base = gNdsAudioFgmStaleStopCount',
        'set $fgm_acquire_base = gNdsAudioFgmHandleAcquireCount',
        'set $fgm_release_base = gNdsAudioFgmHandleReleaseCount',
        'set $fgm_active_base = gNdsAudioFgmActiveHandles',
        'set $attack_frames = 0',
        'set $same_actor = 1',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 3)),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $connected),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $enabled),

        'break ndsAudioFgmPlay',
        'commands',
        'silent',
        'printf "FOX_RECOVERY_FGM=%u\n", (unsigned int)$r0',
        'continue',
        'end',

        ('break *0x{0:x8}' -f $frameMarker),
        'set $frame_breakpoint = $bpnum',
        'commands $frame_breakpoint',
        'silent',
        'set $frame = $frame + 1',
        'if gSCManagerBattleState->players[1].fighter_gobj != $fox',
        'set $same_actor = 0',
        'set $outcome = 3',
        'end',
        'if $ffp->percent_damage > $max_percent',
        'set $max_percent = $ffp->percent_damage',
        'end',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 3)),
        'if ($recover_seen == 0) && ($outcome == 0)',
        # Follow Fox with ordinary run/dash input, but keep Mario inside the
        # main-floor edges. One-frame A pulses produce natural attacks.
        'if $mroot->translate.vec.f.x < -1850.0',
        ('set {{signed char}}0x{0:x8} = 80' -f ($pads + 2)),
        'else',
        'if $mroot->translate.vec.f.x > 1850.0',
        ('set {{signed char}}0x{0:x8} = -80' -f ($pads + 2)),
        'else',
        'if $froot->translate.vec.f.x >= $mroot->translate.vec.f.x',
        ('set {{signed char}}0x{0:x8} = 80' -f ($pads + 2)),
        'else',
        ('set {{signed char}}0x{0:x8} = -80' -f ($pads + 2)),
        'end',
        'end',
        'end',
        'set $dx = $froot->translate.vec.f.x - $mroot->translate.vec.f.x',
        'if ($dx > -700.0) && ($dx < 700.0) && (($frame % 12) == 0)',
        ('set {{unsigned short}}0x{0:x8} = 0x8000' -f $pads),
        'set $attack_frames = $attack_frames + 1',
        'end',
        'else',
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 2)),
        'end',
        'if $ffp->computer.objective == 4',
        'if $recover_seen == 0',
        'set $recover_seen = 1',
        'set $recover_start_x = (int)($froot->translate.vec.f.x * 1000.0)',
        'set $recover_start_y = (int)($froot->translate.vec.f.y * 1000.0)',
        'set $recover_start_ga = $ffp->ga',
        'set $recover_start_line = $ffp->coll_data.floor_line_id',
        'set $recover_start_status = $ffp->status_id',
        'set $recover_start_input = $ffp->computer.input_kind',
        'end',
        'set $recover_frames = $recover_frames + 1',
        'end',
        'if ($recover_seen != 0) && ($ffp->ga == 0) && ($ffp->coll_data.floor_line_id >= 0) && ($ffp->computer.objective != 4)',
        'set $return_x = (int)($froot->translate.vec.f.x * 1000.0)',
        'set $return_y = (int)($froot->translate.vec.f.y * 1000.0)',
        'set $return_line = $ffp->coll_data.floor_line_id',
        'set $return_status = $ffp->status_id',
        'set $outcome = 1',
        'end',
        'if $outcome != 0',
        'disable $frame_breakpoint',
        'disable $timeup_breakpoint',
        'end',
        'if $outcome == 0',
        'continue',
        'end',
        'end',

        ('break *0x{0:x8}' -f $timeUp),
        'set $timeup_breakpoint = $bpnum',
        'commands $timeup_breakpoint',
        'silent',
        'set $outcome = 2',
        'disable $frame_breakpoint',
        'disable $timeup_breakpoint',
        'end',
        'continue',

        ('set {{unsigned short}}0x{0:x8} = 0' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 3)),
        'printf "FOX_RECOVERY_RESULT=%u,%u,%u,%u,%u,%u\n", $outcome, $frame, $recover_seen, $recover_frames, $attack_frames, $same_actor',
        'printf "FOX_RECOVERY_START=%d,%d,%d,%d,%d,%d\n", $recover_start_x, $recover_start_y, $recover_start_ga, $recover_start_line, $recover_start_status, $recover_start_input',
        'printf "FOX_RECOVERY_RETURN=%d,%d,%d,%d,%d,%d\n", $return_x, $return_y, $ffp->ga, $return_line, $return_status, $ffp->computer.objective',
        'printf "FOX_RECOVERY_CPU=%u,%#x,%u,%u,%u,%u\n", gNdsFTComputerProcessCount - $cpu_process_base, gNdsFTComputerObjectiveMask, gNdsFTComputerRecoveryFrames - $recover_counter_base, gNdsFTComputerInputChangeCount, gNdsFTComputerButtonBFrames, gNdsFTComputerStatusChangeCount',
        'printf "FOX_RECOVERY_DAMAGE=%u,%u,%d,%d\n", $start_percent, $max_percent, $start_falls, gSCManagerBattleState->players[1].falls',
        ('printf "FOX_RECOVERY_INPUT=%u,%#x,%u,%#x,%d,%d\n", *(unsigned int *)0x{0:x8}, *(unsigned int *)0x{1:x8}, gNdsControllerPlaybackReadCount - $input_reads_base, *(unsigned short *)0x{2:x8}, *(signed char *)0x{3:x8}, *(signed char *)0x{4:x8}' -f $enabled, $connected, $pads, ($pads + 2), ($pads + 3)),
        'printf "FOX_RECOVERY_SCENE=%u,%u,%u,%u,%u,%u,%#x\n", gNdsSceneHarnessMode, gSCManagerSceneData.scene_curr, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, gNdsSceneHarnessReservedMask',
        'printf "FOX_RECOVERY_MEMORY=%#x,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom',
        'printf "FOX_RECOVERY_AUDIO=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioFgmPlayCalls - $fgm_calls_base, gNdsAudioFgmSupportedPlayCount - $fgm_supported_base, gNdsAudioFgmUnsupportedCallCount - $fgm_unsupported_base, gNdsAudioFgmIncludedLookupFailCount - $fgm_lookup_fail_base, gNdsAudioFgmPlayFailCount - $fgm_play_fail_base, gNdsAudioFgmPoolExhaustCount - $fgm_pool_base, gNdsAudioFgmGenerationMismatchCount - $fgm_generation_base, gNdsAudioFgmStaleStopCount - $fgm_stale_base, gNdsAudioFgmHandleAcquireCount - $fgm_acquire_base, gNdsAudioFgmHandleReleaseCount - $fgm_release_base, $fgm_active_base, gNdsAudioFgmActiveHandles, gNdsAudioFgmMaxActiveHandles',
        'printf "FOX_RECOVERY_AUDIO_STATE=%#x,%u,%u,%u,%u,%u,%#x\n", gNdsAudioFgmResult, gNdsAudioFgmLoaded, gNdsAudioFgmResidentBytes, gNdsAudioFgmOpenFailCount, gNdsAudioFgmReadFailCount, gNdsAudioFgmFormatFailCount, gNdsAudioFgmChannelMask',
        'printf "FOX_RECOVERY_BGM=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%#x,%u,%u,%u\n", gNdsAudioBgmResult, gNdsAudioBgmPlaying, gNdsAudioBgmTrackID, gNdsAudioBgmPupupuPlayCount, gNdsAudioBgmSoundActive, gNdsAudioBgmPlayFailCount, gNdsAudioBgmOpenFailCount, gNdsAudioBgmReadFailCount, gNdsAudioBgmUnsafeWriteCount, gNdsAudioBgmOverrunCount, gNdsAudioBgmStreamBytesPerSecond, gNdsAudioBgmExpectedBytesPerSecond, sNdsAudioBgmSoundID, s_soundAutoUpdate, *(unsigned short *)0x02ffff8c, gNdsAudioBgmResidentBytes, gNdsAudioBgmPlaybackBytes, gNdsAudioBgmRefillCount',
        $captureCommand,
        ('set {{unsigned int}}0x{0:x8} = 0' -f $enabled),
        'detach',
        'quit'
    )

    $gdbStdout = (Invoke-GdbMarkerScript `
        -Gdb $Gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName '_battle_playable_fox_recovery.gdb' `
        -TimeoutSeconds $TimeoutSeconds).Stdout

    $result = [regex]::Match($gdbStdout, 'FOX_RECOVERY_RESULT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $start = [regex]::Match($gdbStdout, 'FOX_RECOVERY_START=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $return = [regex]::Match($gdbStdout, 'FOX_RECOVERY_RETURN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $cpu = [regex]::Match($gdbStdout, 'FOX_RECOVERY_CPU=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $damage = [regex]::Match($gdbStdout, 'FOX_RECOVERY_DAMAGE=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $input = [regex]::Match($gdbStdout, 'FOX_RECOVERY_INPUT=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $scene = [regex]::Match($gdbStdout, 'FOX_RECOVERY_SCENE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $memory = [regex]::Match($gdbStdout, 'FOX_RECOVERY_MEMORY=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $audio = [regex]::Match($gdbStdout, 'FOX_RECOVERY_AUDIO=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $audioState = [regex]::Match($gdbStdout, 'FOX_RECOVERY_AUDIO_STATE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $bgm = [regex]::Match($gdbStdout, 'FOX_RECOVERY_BGM=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $fgmEvents = @([regex]::Matches(
        $gdbStdout, 'FOX_RECOVERY_FGM=([0-9]+)') |
        ForEach-Object { [int]$_.Groups[1].Value })
    $fgmIDs = @($fgmEvents | Sort-Object -Unique)
    $includedIDs = @(626, 470, 469, 467, 490, 372, 430,
        439, 292, 370, 289, 154)
    $voiceIDs = @(372, 430)
    $supportedEvents = @($fgmEvents | Where-Object { $_ -in $includedIDs })
    $unsupportedEvents = @($fgmEvents | Where-Object { $_ -notin $includedIDs })
    $unsupportedIDs = @($unsupportedEvents | Sort-Object -Unique)
    $rv = Get-Ints $result; $sv = Get-Ints $start; $tv = Get-Ints $return
    $cv = Get-Ints $cpu; $dv = Get-Ints $damage; $iv = Get-Ints $input
    $qv = Get-Ints $scene; $mv = Get-Ints $memory; $av = Get-Ints $audio
    $asv = Get-Ints $audioState; $bv = Get-Ints $bgm

    Assert-Condition (Test-Path -LiteralPath $screenshotPath -PathType Leaf) `
        'Fox recovery run did not produce a screenshot.' $gdbStdout
    Assert-Condition ((Get-Item $screenshotPath).Length -gt 1024) `
        'Fox recovery screenshot is unexpectedly small.' $gdbStdout
    Assert-Condition ($result.Success -and $rv[0] -eq 1 -and
        $rv[1] -gt 0 -and $rv[1] -lt 3600 -and $rv[2] -eq 1 -and
        $rv[3] -gt 0 -and $rv[4] -gt 0 -and $rv[5] -eq 1) `
        'Fox did not naturally select Recover and return before Time Up.' $gdbStdout
    Assert-Condition ($start.Success -and $sv[2] -eq 1 -and $sv[3] -lt 0) `
        'Recover did not begin from Fox Air/offstage state.' $gdbStdout
    Assert-Condition ($return.Success -and $tv[2] -eq 0 -and
        $tv[3] -ge 0 -and $tv[5] -ne 4) `
        'Fox did not naturally leave Recover on a valid floor.' $gdbStdout
    Assert-Condition ($cpu.Success -and $cv[0] -gt 0 -and
        (($cv[1] -band 0x10) -eq 0x10) -and $cv[2] -gt 0 -and
        $cv[3] -gt 0 -and $cv[5] -gt 0) `
        'Imported level-3 Fox CPU did not drive the Recover objective.' $gdbStdout
    Assert-Condition ($damage.Success -and $dv[1] -gt $dv[0] -and
        $dv[2] -eq $dv[3]) `
        'The route lacked natural Mario damage or crossed a Fox KO/rebirth.' $gdbStdout
    Assert-Condition ($input.Success -and $iv[0] -eq 1 -and
        (($iv[1] -band 1) -eq 1) -and $iv[2] -gt 0 -and
        $iv[3] -eq 0 -and $iv[4] -eq 0 -and $iv[5] -eq 0) `
        'External player-0 input was not consumed and released cleanly.' $gdbStdout
    Assert-Condition ($scene.Success -and $qv[0] -eq 163 -and
        $qv[1] -eq 22 -and $qv[2] -eq 1 -and $qv[3] -eq 1 -and
        $qv[4] -gt 0 -and ($qv[4] + $qv[5]) -eq 3600 -and
        $qv[6] -eq 0) `
        'Recovery did not stay inside the natural one-minute mode-163 match.' `
        $gdbStdout
    Assert-Condition ($memory.Success -and $mv[0] -eq 0x4d4c4544 -and
        $mv[1] -eq 22 -and $mv[2] -ge 131072) `
        'Fox recovery violated the P1 arena reserve.' $gdbStdout
    Assert-Condition (($voiceIDs | Where-Object { $_ -notin $fgmIDs }).Count -eq 0) `
        'Natural recovery did not trigger both selected fighter voices.' $gdbStdout
    Assert-Condition ($audio.Success -and
        $av[0] -eq $fgmEvents.Count -and
        $av[1] -eq $supportedEvents.Count -and
        $av[2] -eq $unsupportedEvents.Count -and
        $av[3] -eq 0 -and $av[4] -eq 0 -and $av[5] -eq 0 -and
        $av[6] -eq 0 -and $av[7] -eq 0 -and
        $av[8] -eq $supportedEvents.Count -and
        $av[9] -le ($av[10] + $av[8]) -and
        $av[11] -eq ($av[10] + $av[8] - $av[9]) -and
        $av[12] -gt 0) `
        'Selected fighter voices did not acquire clean DS handles/channels.' `
        $gdbStdout
    Assert-Condition ($audioState.Success -and
        $asv[0] -eq 0x46474d31 -and $asv[1] -eq 1 -and
        $asv[2] -eq 102196 -and $asv[3] -eq 0 -and $asv[4] -eq 0 -and
        $asv[5] -eq 0 -and $asv[6] -ne 0) `
        'Fighter-voice pack load/channel state is invalid.' $gdbStdout
    Assert-Condition ($bgm.Success -and
        $bv[0] -eq 0x42474d31 -and $bv[1] -eq 1 -and $bv[2] -eq 0 -and
        $bv[3] -eq 1 -and $bv[4] -eq 1 -and
        $bv[5] -eq 0 -and $bv[6] -eq 0 -and $bv[7] -eq 0 -and
        $bv[8] -eq 0 -and $bv[9] -eq 0 -and
        $bv[10] -ge 42100 -and $bv[10] -le 46100 -and
        $bv[11] -eq 44100 -and $bv[12] -ge 0 -and
        $bv[13] -in 0,1 -and
        (($bv[14] -band ([int64]1 -shl $bv[12])) -ne 0) -and
        $bv[15] -eq 65536 -and $bv[16] -gt 0 -and $bv[17] -gt 0) `
        'Natural Pupupu BGM lacked an active DS channel or clean stream state.' `
        $gdbStdout

    Write-Output (('Fox recovery natural FGM IDs: all={0} supported={1} ' +
        'unsupported={2}') -f ($fgmIDs -join ','),
        (($supportedEvents | Sort-Object -Unique) -join ','),
        ($unsupportedIDs -join ','))
    Write-Output (('Pupupu BGM natural: channel={0} active=0x{1:x} ' +
        'auto={2} stream={3}/{4} refills={5}') -f
        $bv[12], $bv[14], $bv[13], $bv[10], $bv[11], $bv[17])
    Write-Output ((
        'battle_playable Fox recovery passed: frames={0} recover={1} ' +
        'start=({2},{3}) return=({4},{5}) line={6} damage={7}->{8} ' +
        'reserve={9} screenshot={10}') -f
        $rv[1], $rv[3], $sv[0], $sv[1], $tv[0], $tv[1], $tv[3],
        $dv[0], $dv[1], $mv[2], $screenshotPath)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
    }
    Restore-MelonDSGdbConfig -State $configState
}
