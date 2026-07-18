[CmdletBinding()]
param(
    [ValidateSet('Mario', 'Fox')][string]$Actor = 'Mario',
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 300,
    [switch]$NoBuild,
    [switch]$ObserverFreeSnapshot,
    [switch]$FreezeDiagnostics,
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
if ($ObserverFreeSnapshot -and $FreezeDiagnostics) {
    throw 'Observer-free and freeze-diagnostic captures are mutually exclusive.'
}
$target = if ($FreezeDiagnostics) {
    'smash64ds-battle-playable-freeze-diagnostics-on-hwtri'
} else {
    'smash64ds-battle-playable-hwtri'
}
$build = if ($FreezeDiagnostics) {
    'build-freeze-diagnostics-on-hwtri'
} else {
    'build-battle-playable-canonical-hwtri-harness'
}
$artifactDir = if ($FreezeDiagnostics) {
    Join-Path $root (Join-Path 'builds' $build)
} else {
    $root
}
$rom = Join-Path $artifactDir "$target.nds"
$elf = Join-Path $artifactDir "$target.elf"
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$melonDsPath = $context.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir `
    -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$slug = $Actor.ToLowerInvariant()
$stdout = Join-Path $logDir "melonds.down-air-$slug.stdout.log"
$stderr = Join-Path $logDir "melonds.down-air-$slug.stderr.log"
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
        $value = $Match.Groups[$i].Value
        $values += if ($value -like '0x*') {
            [int64](Convert-MarkerUInt32 $value)
        } else {
            [int64]$value
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
foreach ($path in @($rom, $elf, $melonDsPath, $Gdb, $nm)) {
    Assert-Condition (Test-Path -LiteralPath $path -PathType Leaf) `
        "Required Down-Air artifact is missing: $path"
}
$script:ElfSymbols = @(& $nm -a $elf)
Assert-Condition ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
$pads = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$connected = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$enabled = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
$statusSet = Get-ElfSymbolAddress 'ftMainSetStatus'
$kneeBendUpdate = Get-ElfSymbolAddress 'ndsBaseFTCommonKneeBendProcUpdate'
$downAirUpdate = Get-ElfSymbolAddress 'ndsBaseFTCommonAttackAirLwProcUpdate'
$controllerRead = Get-ElfSymbolAddress 'osContGetReadData'
$timeUp = Get-ElfSymbolAddress 'ifCommonAnnounceTimeUpInitInterface'
$freezeStall = if ($FreezeDiagnostics) {
    Get-ElfSymbolAddress 'ndsFreezeDiagnosticsStallMarker'
} else {
    0
}
$freezeException = if ($FreezeDiagnostics) {
    Get-ElfSymbolAddress 'ndsFreezeDiagnosticsExceptionHandler'
} else {
    0
}
$downAirAssetSymbol = if ($Actor -eq 'Mario') {
    'llFTMarioAnimAttackAirDFileID'
} else {
    'llFTFoxAnimAttackAirDFileID'
}
$downAirAssetToken = Get-ElfSymbolAddress $downAirAssetSymbol
$expectedAssetId = if ($Actor -eq 'Mario') { 626 } else { 771 }

$visibilityDir = Join-Path $root 'artifacts\visibility'
$screenshotPath = if ([string]::IsNullOrWhiteSpace($Screenshot)) {
    Join-Path $visibilityDir (
        (Get-Date -Format 'yyyy-MM-dd_HHmmss-fffffff') +
        "_down-air-$slug.png")
} elseif ([System.IO.Path]::IsPathRooted($Screenshot)) {
    [System.IO.Path]::GetFullPath($Screenshot)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $Screenshot))
}
$visibilityPrefix =
    [System.IO.Path]::GetFullPath($visibilityDir).TrimEnd('\') + '\'
Assert-Condition ($screenshotPath.StartsWith(
        $visibilityPrefix,
        [System.StringComparison]::OrdinalIgnoreCase)) `
    'Down-Air screenshots must stay under artifacts\visibility.'
New-Item -ItemType Directory -Force -Path $logDir, $visibilityDir | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath -GdbPort $context.GdbPort `
        -Persistent:([bool]$context.PersistentConfig)
    Remove-Item $stdout, $stderr, $screenshotPath -Force `
        -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList ('"{0}"' -f $rom) `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator -Port $context.GdbPort | Out-Null

    $capture = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
    $captureCommand = (
        'shell powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{0}" ' +
        '-EmulatorProcessId {1} -Output "{2}"') -f
        $capture.Replace('\', '/'), $emulator.Id,
        $screenshotPath.Replace('\', '/')
    $observerReady = Join-Path $logDir "down-air-$slug-observer.ready"
    $observerReadyGdb = $observerReady.Replace('\', '/')
    $actorKind = if ($Actor -eq 'Mario') { 0 } else { 1 }
    $actorPads = $pads + (6 * $actorKind)
    $connectedMask = if ($Actor -eq 'Mario') { 1 } else { 3 }
    $p2SetupCommands = if ($Actor -eq 'Fox') {
        @(
            'set $p1_head = *(unsigned int *)&gSCManagerTransferBattleState.players[1]',
            'set *(unsigned int *)&gSCManagerTransferBattleState.players[1] = $p1_head & 0xff00ffff',
            'set $battle_counts = *(unsigned int *)((unsigned char *)&gSCManagerTransferBattleState + 4)',
            'set *(unsigned int *)((unsigned char *)&gSCManagerTransferBattleState + 4) = ($battle_counts & 0xffff0000) | 2',
            'set gNdsBattlePlayableFoxCpuEnabled = 0'
        )
    } else {
        @()
    }
    $observerDefinitions = if ($ObserverFreeSnapshot) {
        @(
            'set $observer_armed = 0',
            'define hook-stop',
            'if $observer_armed != 0',
            'set $observer_armed = 0',
            'printf "DOWN_AIR_OBSERVER=%u,%u,%u,%u,%u,%u,%d,%d,%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u\n", $logic_start, gNdsBattlePlayablePacingLogicFrames, $present_start, gNdsBattlePlayablePacingPresentedFrames, $reads_start, gNdsControllerPlaybackReadCount, $actor_fp->status_id, $actor_fp->motion_id, $actor_fp->status_total_tics, *(unsigned int *)&$actor_gobj->anim_frame, *(unsigned int *)&$actor_fp->motion_frame, (unsigned int)$actor_fp->proc_update, (unsigned int)$actor_fp->proc_interrupt, (unsigned int)$actor_fp->proc_map, $pc, $lr, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed',
            ('set {{unsigned int}}0x{0:x8} = 0' -f $enabled),
            'detach',
            'quit',
            'end',
            'end'
        )
    } else {
        @()
    }
    $freezeDefinitions = if ($FreezeDiagnostics) {
        @(
            'set $freeze_fallback_armed = 0',
            'set $freeze_exception_armed = 0',
            'define hook-stop',
            'if $freeze_fallback_armed != 0',
            ('if $pc != 0x{0:x8}' -f $freezeException),
            'if gNdsFreezeDiagnosticsWatchdogTripCount == 0',
            'set $freeze_fallback_armed = 0',
            'printf "DOWN_AIR_FREEZE_LIVE=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%#x,%#x,%#x,%#x,%u,%u,%#x,%#x,%#x\n", gNdsFreezeDiagnosticsInterruptedPC, gNdsFreezeDiagnosticsInterruptedLR, gNdsFreezeDiagnosticsHeartbeat, gNdsFreezeDiagnosticsLastBreadcrumb, gNdsFreezeDiagnosticsBreadcrumbWriteCount, sNdsFreezeDiagnosticsInitialized, sNdsFreezeDiagnosticsWatchdogArmed, sNdsFreezeDiagnosticsStaleSamples, sNdsFreezeDiagnosticsLastHeartbeat, gNdsFreezeDiagnosticsWatchdogTripCount, gNdsFreezeDiagnosticsForceTrip, $actor_fp->status_id, $actor_fp->motion_id, $actor_fp->status_total_tics, *(unsigned int *)&$actor_gobj->anim_frame, *(unsigned int *)&$actor_fp->motion_frame, (unsigned int)$actor_fp->proc_update, (unsigned int)$actor_fp->proc_map, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, *(unsigned int *)0x04000208, *(unsigned int *)0x04000210, *(unsigned int *)0x04000214',
            'printf "DOWN_AIR_FREEZE_RING=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFreezeDiagnosticsBreadcrumbs[0], gNdsFreezeDiagnosticsBreadcrumbs[1], gNdsFreezeDiagnosticsBreadcrumbs[2], gNdsFreezeDiagnosticsBreadcrumbs[3], gNdsFreezeDiagnosticsBreadcrumbs[4], gNdsFreezeDiagnosticsBreadcrumbs[5], gNdsFreezeDiagnosticsBreadcrumbs[6], gNdsFreezeDiagnosticsBreadcrumbs[7]',
            'printf "DOWN_AIR_CPU=%#x,%#x,%#x,%#x,%#x\n", gNdsFreezeDiagnosticsReportKind, $cpsr, $pc, $lr, $sp',
            'printf "DOWN_AIR_SCHED=%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", *(unsigned int *)0x02ff0080, *(unsigned int *)0x02ff0084, *(unsigned int *)0x02ff0088, *(unsigned int *)0x02ff008c, *(unsigned int *)0x02ff0090, *(unsigned int *)0x02ff0094, *(unsigned int *)0x02ff3ff8',
            'set $sched_thread = *(unsigned int *)0x02ff0088',
            'set $sched_index = 0',
            'while ($sched_thread != 0) && ($sched_index < 12)',
            'printf "DOWN_AIR_THREAD=%u,%#x,%#x,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", $sched_index, $sched_thread, *(unsigned int *)($sched_thread + 80), *(unsigned char *)($sched_thread + 84), *(unsigned char *)($sched_thread + 85), *(unsigned char *)($sched_thread + 86), *(unsigned char *)($sched_thread + 87), *(unsigned int *)($sched_thread + 52), *(unsigned int *)($sched_thread + 56), *(unsigned int *)($sched_thread + 60), *(unsigned int *)($sched_thread + 64), *(unsigned int *)($sched_thread + 104), *(unsigned int *)($sched_thread + 108), *(unsigned int *)($sched_thread + 96)',
            'set $sched_thread = *(unsigned int *)($sched_thread + 80)',
            'set $sched_index = $sched_index + 1',
            'end',
            ('set {{unsigned int}}0x{0:x8} = 0' -f $enabled),
            'detach',
            'quit',
            'end',
            'end',
            'end',
            'end',
            ('break *0x{0:x8}' -f $freezeException),
            'set $freeze_exception_breakpoint = $bpnum',
            'commands $freeze_exception_breakpoint',
            'silent',
            'if $freeze_exception_armed != 0',
            'printf "DOWN_AIR_EXCEPTION=%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%d,%d,%u,%u,%u\n", $r1, $r0, *(unsigned int *)($r0 + 52), *(unsigned int *)($r0 + 56), *(unsigned int *)($r0 + 60), *(unsigned int *)($r0 + 64), *(unsigned int *)($r0 + 68), *(unsigned int *)($r0 + 72), *(unsigned int *)($r0 + 76), $actor_fp->status_id, $actor_fp->motion_id, $actor_fp->status_total_tics, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed',
            'printf "DOWN_AIR_EXCEPTION_REGS=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", *(unsigned int *)($r0 + 0), *(unsigned int *)($r0 + 4), *(unsigned int *)($r0 + 8), *(unsigned int *)($r0 + 12), *(unsigned int *)($r0 + 16), *(unsigned int *)($r0 + 20), *(unsigned int *)($r0 + 24), *(unsigned int *)($r0 + 28), *(unsigned int *)($r0 + 32), *(unsigned int *)($r0 + 36), *(unsigned int *)($r0 + 40), *(unsigned int *)($r0 + 44), *(unsigned int *)($r0 + 48)',
            'set $exception_sp = *(unsigned int *)($r0 + 52)',
            'printf "DOWN_AIR_EXCEPTION_STACK=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", *(unsigned int *)($exception_sp + 0), *(unsigned int *)($exception_sp + 4), *(unsigned int *)($exception_sp + 8), *(unsigned int *)($exception_sp + 12), *(unsigned int *)($exception_sp + 16), *(unsigned int *)($exception_sp + 20), *(unsigned int *)($exception_sp + 24), *(unsigned int *)($exception_sp + 28), *(unsigned int *)($exception_sp + 32), *(unsigned int *)($exception_sp + 36), *(unsigned int *)($exception_sp + 40), *(unsigned int *)($exception_sp + 44)',
            ('set {{unsigned int}}0x{0:x8} = 0' -f $enabled),
            'detach',
            'quit',
            'end',
            'continue',
            'end',
            ('break *0x{0:x8}' -f $freezeStall),
            'set $freeze_breakpoint = $bpnum',
            'commands $freeze_breakpoint',
            'silent',
            'printf "DOWN_AIR_FREEZE=%#x,%#x,%u,%#x,%u,%u,%u,%#x,%u,%d,%d,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFreezeDiagnosticsInterruptedPC, gNdsFreezeDiagnosticsInterruptedLR, gNdsFreezeDiagnosticsWatchdogTripCount, gNdsFreezeDiagnosticsReportKind, gNdsFreezeDiagnosticsReportLogicFrames, gNdsFreezeDiagnosticsReportPresentedFrames, gNdsFreezeDiagnosticsHeartbeat, gNdsFreezeDiagnosticsLastBreadcrumb, gNdsFreezeDiagnosticsBreadcrumbWriteCount, $actor_fp->status_id, $actor_fp->motion_id, $actor_fp->status_total_tics, *(unsigned int *)&$actor_gobj->anim_frame, *(unsigned int *)&$actor_fp->motion_frame, (unsigned int)$actor_fp->proc_update, (unsigned int)$actor_fp->proc_map, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed',
            'printf "DOWN_AIR_FREEZE_RING=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFreezeDiagnosticsBreadcrumbs[0], gNdsFreezeDiagnosticsBreadcrumbs[1], gNdsFreezeDiagnosticsBreadcrumbs[2], gNdsFreezeDiagnosticsBreadcrumbs[3], gNdsFreezeDiagnosticsBreadcrumbs[4], gNdsFreezeDiagnosticsBreadcrumbs[5], gNdsFreezeDiagnosticsBreadcrumbs[6], gNdsFreezeDiagnosticsBreadcrumbs[7]',
            'printf "DOWN_AIR_FREEZE_IO=%#x,%u,%u,%#x,%#x,%u,%u,%u,%u,%d\n", gNdsFreezeDiagnosticsReportGXStatus, gNdsFreezeDiagnosticsReportPolygonCount, gNdsFreezeDiagnosticsReportVertexCount, gNdsFreezeDiagnosticsReportIpcFifo, gNdsFreezeDiagnosticsReportIpcSync, gNdsFreezeDiagnosticsSwapPending, gNdsFreezeDiagnosticsFgmEnterCount, gNdsFreezeDiagnosticsFgmReturnCount, gNdsFreezeDiagnosticsLastFgmID, gNdsFreezeDiagnosticsLastFgmChannel',
            ('set {{unsigned int}}0x{0:x8} = 0' -f $enabled),
            'detach',
            'quit',
            'end'
        )
    } else {
        @()
    }
    $observerEntryCommands = if ($ObserverFreeSnapshot) {
        @(
            'disable $input_breakpoint',
            'disable $status_breakpoint',
            'disable $kneebend_breakpoint',
            'disable $downair_breakpoint',
            'disable $timeup_breakpoint',
            'set $outcome = 4',
            'set $observer_armed = 1',
            ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$observerReadyGdb' -Value ready`"")
        )
    } elseif ($FreezeDiagnostics) {
        @(
            'disable $input_breakpoint',
            'disable $status_breakpoint',
            'disable $kneebend_breakpoint',
            'disable $downair_breakpoint',
            'disable $timeup_breakpoint',
            'set $outcome = 4',
            'set $freeze_fallback_armed = 1',
            'set $freeze_exception_armed = 1',
            ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$observerReadyGdb' -Value ready`"")
        )
    } else {
        @()
    }
    $summaryCommands = if ($ObserverFreeSnapshot -or $FreezeDiagnostics) {
        @()
    } else {
        @(
            ('set {{unsigned int}}0x{0:x8} = 0' -f $actorPads),
            'printf "DOWN_AIR_RESULT=%u,%u,%u,%u\n", $actor_kind, $outcome, $downair_entries, $seen',
            'printf "DOWN_AIR_KNEE=%u,%#x,%#x,%#x\n", $knee_updates, $knee_frame_bits, $knee_speed_bits, $knee_length_bits',
            'printf "DOWN_AIR_ENTRY=%d,%d,%d,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%u\n", $entry_status, $entry_motion, $entry_ga, $payload_delta, $header_delta, $open_fail_delta, $format_fail_delta, $short_read_delta, $entry_update, $entry_anim_bits, $entry_motion_bits, $entry_fp_anim_bits, $entry_heap0, $entry_heap1, $status_transition',
            'printf "DOWN_AIR_ASSET=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u\n", $entry_asset_token, $entry_asset_id, $entry_asset_data, $entry_heap_ptr, $entry_figatree_ptr, $entry_asset_size, $entry_asset_owner, $entry_asset_fixup_fails, $entry_motion_count',
            'printf "DOWN_AIR_BASELINE=%u,%u,%u,%u,%u,%u,%u,%u\n", $logic_start, $cpu_start, $present_start, $reads_start, $status_tics_start, $open_fails_start, $format_fails_start, $short_reads_start',
            'printf "DOWN_AIR_TERMINAL=%u,%u,%u,%u,%u,%#x,%#x,%d,%d,%d,%u,%u,%u,%u\n", $terminal_logic, $terminal_cpu, $terminal_present, $terminal_reads, $terminal_status_tics, $terminal_anim_bits, $terminal_motion_bits, $terminal_status, $terminal_motion, $terminal_exit_status, $terminal_update, $terminal_open_fails, $terminal_format_fails, $terminal_short_reads',
            'printf "DOWN_AIR_AERIALS=%u,%u,%u,%u,%u\n", $air_n, $air_f, $air_b, $air_u, $air_d',
            'printf "DOWN_AIR_SCENE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSceneHarnessMode, gSCManagerSceneData.scene_curr, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, $mario->fkind, $fox->fkind',
            'printf "DOWN_AIR_ACTORS=%u,%u,%u,%u,%u,%u,%u,%u\n", $mario->pkind, $mario->player, $fox->pkind, $fox->player, $fox->level, gNdsBattlePlayableFoxCpuEnabled, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count',
            'printf "DOWN_AIR_MEMORY=%#x,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerArenaHeadroom',
            ('set {{unsigned int}}0x{0:x8} = 0' -f $enabled),
            'detach',
            'quit'
        )
    }

    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set breakpoint pending off',
        'set mi-async on',
        'set remotetimeout 10',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        # Install event breakpoints before first execution so melonDS JIT sees
        # them.  Fox qualification temporarily promotes the pre-spawn P1 slot
        # to human and then drives only its ordinary controller input.
        'set $armed = 0',
        ("set `$actor_kind = {0}" -f $actorKind),
        'set $mario_gobj = 0',
        'set $fox_gobj = 0',
        'set $mario = 0',
        'set $fox = 0',
        'set $actor_gobj = 0',
        'set $actor_fp = 0',
        'set $outcome = 0',
        'set $phase = 0',
        'set $downair_entries = 0',
        'set $input_reads = 0',
        'set $knee_updates = 0',
        'set $knee_frame_bits = 0',
        'set $knee_speed_bits = 0',
        'set $knee_length_bits = 0',
        'set $seen = 0',
        'set $sampled = 0',
        'set $air_n = 0',
        'set $air_f = 0',
        'set $air_b = 0',
        'set $air_u = 0',
        'set $air_d = 0',
        'set $entry_status = -1',
        'set $entry_motion = -1',
        'set $entry_ga = -1',
        'set $entry_anim_bits = 0',
        'set $entry_motion_bits = 0',
        'set $entry_fp_anim_bits = 0',
        'set $entry_heap0 = 0',
        'set $entry_heap1 = 0',
        'set $entry_heap_ptr = 0',
        'set $entry_figatree_ptr = 0',
        'set $entry_asset_token = 0',
        'set $entry_asset_id = 0xffffffff',
        'set $entry_asset_data = 0',
        'set $entry_asset_size = 0',
        'set $entry_asset_owner = 0xffffffff',
        'set $entry_asset_fixup_fails = 0xffffffff',
        'set $entry_motion_count = 0',
        'set $entry_update = 0',
        'set $payload_delta = 0',
        'set $header_delta = 0',
        'set $open_fail_delta = 0',
        'set $format_fail_delta = 0',
        'set $short_read_delta = 0',
        'set $status_transition = 0',
        'set $previous_payloads = 0',
        'set $previous_headers = 0',
        'set $previous_open_fails = 0',
        'set $previous_format_fails = 0',
        'set $previous_short_reads = 0',
        'set $open_fails_start = 0',
        'set $format_fails_start = 0',
        'set $short_reads_start = 0',
        'set $logic_start = 0',
        'set $cpu_start = 0',
        'set $present_start = 0',
        'set $reads_start = 0',
        'set $status_tics_start = 0',
        'set $terminal_logic = 0',
        'set $terminal_cpu = 0',
        'set $terminal_present = 0',
        'set $terminal_reads = 0',
        'set $terminal_status_tics = 0',
        'set $terminal_anim_bits = 0',
        'set $terminal_motion_bits = 0',
        'set $terminal_status = -1',
        'set $terminal_motion = -1',
        'set $terminal_exit_status = -1',
        'set $terminal_update = 0',
        'set $terminal_open_fails = 0',
        'set $terminal_format_fails = 0',
        'set $terminal_short_reads = 0'
    ) + $observerDefinitions + $freezeDefinitions + @(
        ('break *0x{0:x8}' -f $controllerRead),
        'set $input_breakpoint = $bpnum',
        'commands $input_breakpoint',
        'silent',
        'if ($armed != 0) && ($phase == 0)',
        'set $input_reads = $input_reads + 1',
        'if $input_reads == 4',
        ('set {{unsigned short}}0x{0:x8} = 0x0008' -f $actorPads),
        'end',
        'if $input_reads >= 20',
        'disable $input_breakpoint',
        'end',
        'end',
        'continue',
        'end',
        ('break *0x{0:x8}' -f $statusSet),
        'set $status_breakpoint = $bpnum',
        'commands $status_breakpoint',
        'silent',
        'if ($armed != 0) && ((GObj *)$r0 == $actor_gobj)',
        'if ($r1 == 20) && ($phase == 0)',
        'set $phase = 1',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $actorPads),
        'disable $input_breakpoint',
        'end',
        'if (($r1 == 22) || ($r1 == 23)) && ($phase == 1)',
        'set $phase = 2',
        ('set {{unsigned short}}0x{0:x8} = 0x8000' -f $actorPads),
        ('set {{signed char}}0x{0:x8} = -80' -f ($actorPads + 3)),
        'end',
        'if $r1 == 209',
        'set $air_n = $air_n + 1',
        'end',
        'if $r1 == 210',
        'set $air_f = $air_f + 1',
        'end',
        'if $r1 == 211',
        'set $air_b = $air_b + 1',
        'end',
        'if $r1 == 212',
        'set $air_u = $air_u + 1',
        'end',
        'if $r1 == 213',
        'set $air_d = $air_d + 1',
        'set $status_transition = 1',
        'set $previous_payloads = gNdsRelocAssetPayloadReadCount',
        'set $previous_headers = gNdsRelocAssetHeaderReadCount',
        'set $previous_open_fails = gNdsRelocAssetOpenFailCount',
        'set $previous_format_fails = gNdsRelocAssetFormatFailCount',
        'set $previous_short_reads = gNdsRelocAssetShortReadCount',
        'end',
        'if ($seen != 0) && ($sampled != 0) && ($r1 != 213)',
        'set $terminal_exit_status = $r1',
        'set $terminal_open_fails = gNdsRelocAssetOpenFailCount',
        'set $terminal_format_fails = gNdsRelocAssetFormatFailCount',
        'set $terminal_short_reads = gNdsRelocAssetShortReadCount',
        'set $outcome = 1',
        'disable $input_breakpoint',
        'disable $status_breakpoint',
        'disable $kneebend_breakpoint',
        'disable $downair_breakpoint',
        'disable $timeup_breakpoint',
        'end',
        'end',
        'if $outcome == 0',
        'continue',
        'end',
        'end',
        ('break *0x{0:x8}' -f $kneeBendUpdate),
        'set $kneebend_breakpoint = $bpnum',
        'commands $kneebend_breakpoint',
        'silent',
        'if ($armed != 0) && ((GObj *)$r0 == $actor_gobj)',
        'set $knee_updates = $knee_updates + 1',
        'set $knee_frame_bits = *(unsigned int *)&$actor_fp->status_vars.common.kneebend.anim_frame',
        'set $knee_speed_bits = *(unsigned int *)&((DObj *)$actor_gobj->obj)->anim_speed',
        'set $knee_length_bits = *(unsigned int *)&$actor_fp->attr->kneebend_anim_length',
        'if (($knee_speed_bits == 0) || ($knee_updates >= 8)) && ($actor_fp->status_id == 20)',
        'set $outcome = 3',
        'disable $input_breakpoint',
        'disable $status_breakpoint',
        'disable $kneebend_breakpoint',
        'disable $downair_breakpoint',
        'disable $timeup_breakpoint',
        'end',
        'end',
        'if $outcome == 0',
        'continue',
        'end',
        'end',
        ('break *0x{0:x8}' -f $downAirUpdate),
        'set $downair_breakpoint = $bpnum',
        'commands $downair_breakpoint',
        'silent',
        'if ($armed != 0) && ((GObj *)$r0 == $actor_gobj)',
        'set $downair_entries = $downair_entries + 1',
        'printf "DOWN_AIR_HEARTBEAT=%u,%u,%#x\n", $downair_entries, $actor_fp->status_total_tics, *(unsigned int *)&$actor_gobj->anim_frame',
        'if $seen == 0',
        'set $seen = 1',
        'set $phase = 3',
        # Record the exact first-use asset, then count nine natural callback
        # entries: the later eight each follow one completed Down-Air update.
        'disable $input_breakpoint',
        'disable $kneebend_breakpoint',
        'set $entry_status = $actor_fp->status_id',
        'set $entry_motion = $actor_fp->motion_id',
        'set $entry_ga = $actor_fp->ga',
        'set $entry_anim_bits = *(unsigned int *)&$actor_gobj->anim_frame',
        'set $entry_motion_bits = *(unsigned int *)&$actor_fp->motion_frame',
        'set $entry_fp_anim_bits = *(unsigned int *)&$actor_fp->anim_frame',
        'set $entry_heap_ptr = (unsigned int)$actor_fp->figatree_heap',
        'set $entry_figatree_ptr = (unsigned int)$actor_fp->figatree',
        'set $entry_motion_count = $actor_fp->data->mainmotion_array_count',
        'set $entry_asset_token = $actor_fp->data->mainmotion->motion_desc[$entry_motion].anim_file_id',
        'if $actor_fp->figatree_heap != 0',
        'set $entry_heap0 = *(unsigned int *)$actor_fp->figatree_heap',
        'set $entry_heap1 = *((unsigned int *)$actor_fp->figatree_heap + 1)',
        'end',
        # A first use of this exact AObj16 animation appends its force-loaded
        # payload after retiring the previous heap alias.  Read that one entry
        # directly; a remote word-by-word table scan stops emulation for minutes.
        'set $loaded_i = sNdsRelocLoadedFileCount',
        'if $loaded_i != 0',
        'set $loaded_i = $loaded_i - 1',
        'if ((unsigned int)sNdsRelocLoadedFiles[$loaded_i].data == $entry_heap_ptr)',
        'set $entry_asset_id = sNdsRelocLoadedFiles[$loaded_i].asset_id',
        'set $entry_asset_data = (unsigned int)sNdsRelocLoadedFiles[$loaded_i].data',
        'set $entry_asset_size = sNdsRelocLoadedFiles[$loaded_i].data_size',
        'set $entry_asset_owner = sNdsRelocLoadedFiles[$loaded_i].owner_scene',
        'set $entry_asset_fixup_fails = sNdsRelocLoadedFiles[$loaded_i].external_fixup_fail_count',
        'end',
        'end',
        'set $entry_update = gNdsRendererProfileUpdateTicks',
        'set $payload_delta = gNdsRelocAssetPayloadReadCount - $previous_payloads',
        'set $header_delta = gNdsRelocAssetHeaderReadCount - $previous_headers',
        'set $open_fail_delta = gNdsRelocAssetOpenFailCount - $previous_open_fails',
        'set $format_fail_delta = gNdsRelocAssetFormatFailCount - $previous_format_fails',
        'set $short_read_delta = gNdsRelocAssetShortReadCount - $previous_short_reads',
        'set $logic_start = gNdsBattlePlayablePacingLogicFrames',
        'set $cpu_start = gNdsFTComputerProcessCount',
        'set $present_start = gNdsBattlePlayablePacingPresentedFrames',
        'set $reads_start = gNdsControllerPlaybackReadCount',
        'set $status_tics_start = $actor_fp->status_total_tics',
        'set $open_fails_start = gNdsRelocAssetOpenFailCount',
        'set $format_fails_start = gNdsRelocAssetFormatFailCount',
        'set $short_reads_start = gNdsRelocAssetShortReadCount',
        'printf "DOWN_AIR_EVENT=entry,%d,%d,%u,%u\n", $entry_status, $entry_motion, $payload_delta, $header_delta',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $actorPads)
    ) + $observerEntryCommands + @(
        'end',
        'end',
        'if ($seen != 0) && ($downair_entries == 9)',
        'set $sampled = 1',
        'set $terminal_logic = gNdsBattlePlayablePacingLogicFrames',
        'set $terminal_cpu = gNdsFTComputerProcessCount',
        'set $terminal_present = gNdsBattlePlayablePacingPresentedFrames',
        'set $terminal_reads = gNdsControllerPlaybackReadCount',
        'set $terminal_status_tics = $actor_fp->status_total_tics',
        'set $terminal_anim_bits = *(unsigned int *)&$actor_gobj->anim_frame',
        'set $terminal_motion_bits = *(unsigned int *)&$actor_fp->motion_frame',
        'set $terminal_status = $actor_fp->status_id',
        'set $terminal_motion = $actor_fp->motion_id',
        'set $terminal_update = gNdsRendererProfileUpdateTicks',
        $captureCommand,
        'disable $input_breakpoint',
        'disable $kneebend_breakpoint',
        'disable $downair_breakpoint',
        'end',
        'if $outcome == 0',
        'continue',
        'end',
        'end',
        ('break *0x{0:x8}' -f $timeUp),
        'set $timeup_breakpoint = $bpnum',
        'commands $timeup_breakpoint',
        'silent',
        'if $armed != 0',
        'set $outcome = 2',
        'disable $input_breakpoint',
        'disable $status_breakpoint',
        'disable $kneebend_breakpoint',
        'disable $downair_breakpoint',
        'disable $timeup_breakpoint',
        'end',
        'if $outcome == 0',
        'continue',
        'end',
        'end',
        # For the Fox arm only, promote the pre-spawn P1 slot to a normal
        # second human.  No live fighter or status state is rewritten.
        'tbreak scVSBattleStartBattle',
        'continue'
    ) + $p2SetupCommands + @(
        # Wait for BattleShip's original timer, then arm the event observers.
        'tbreak ifcommon.c:3175',
        'continue',
        'set $mario_gobj = (GObj *)gSCManagerBattleState->players[0].fighter_gobj',
        'set $fox_gobj = (GObj *)gSCManagerBattleState->players[1].fighter_gobj',
        'set $mario = (FTStruct *)$mario_gobj->user_data.p',
        'set $fox = (FTStruct *)$fox_gobj->user_data.p',
        'set $actor_gobj = $mario_gobj',
        'set $actor_fp = $mario',
        'if $actor_kind != 0',
        'set $actor_gobj = $fox_gobj',
        'set $actor_fp = $fox',
        'end',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $actorPads),
        ('set {{unsigned int}}0x{0:x8} = {1}' -f $connected, $connectedMask),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $enabled),
        'set $armed = 1',
        'continue'
    ) + $summaryCommands

    $gdbArguments = @{
        Gdb = $Gdb
        Elf = $elf
        Root = $root
        Commands = $gdbCommands
        ScriptName = "_battle_playable_down_air_${slug}.gdb"
        TimeoutSeconds = $TimeoutSeconds
    }
    if ($ObserverFreeSnapshot -or $FreezeDiagnostics) {
        $gdbArguments.ReadyFile = $observerReady
        $gdbArguments.InteractiveSteps = @(
            [pscustomobject]@{
                DelayMilliseconds = 0
                Commands = @('1-exec-continue')
            },
            [pscustomobject]@{
                DelayMilliseconds = 12000
                Commands = @('2-exec-interrupt')
            }
        )
        $gdbArguments.MiInteractive = $true
    }
    $gdbStdout = (Invoke-GdbMarkerScript @gdbArguments).Stdout

    if ($ObserverFreeSnapshot) {
        $observer = [regex]::Match($gdbStdout,
            'DOWN_AIR_OBSERVER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
        Assert-Condition $observer.Success `
            "$Actor observer-free interrupt did not emit a target snapshot." `
            $gdbStdout
        $ov = Get-Ints $observer
        $logicDelta = $ov[1] - $ov[0]
        $presentDelta = $ov[3] - $ov[2]
        $readsDelta = $ov[5] - $ov[4]
        Assert-Condition ($logicDelta -ge 8 -and $presentDelta -ge 4 -and
            $readsDelta -gt 0 -and $ov[14] -ne 0 -and $ov[15] -ne 0 -and
            ($ov[16] + $ov[17]) -eq 3600) `
            "$Actor did not clear the observer-free Down-Air window." `
            $observer.Value
        Write-Output ((
            'Down-Air observer-free target snapshot passed: actor={0} ' +
            'logic={1} present={2} reads={3} status={4} motion={5} ' +
            'pc={6} lr={7}.') -f
            $Actor, $logicDelta, $presentDelta, $readsDelta, $ov[6], $ov[7],
            $observer.Groups[15].Value, $observer.Groups[16].Value)
        return
    }

    if ($FreezeDiagnostics) {
        $exception = [regex]::Match($gdbStdout,
            'DOWN_AIR_EXCEPTION=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
        $exceptionRegs = [regex]::Match($gdbStdout,
            'DOWN_AIR_EXCEPTION_REGS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
        $exceptionStack = [regex]::Match($gdbStdout,
            'DOWN_AIR_EXCEPTION_STACK=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
        $live = [regex]::Match($gdbStdout,
            'DOWN_AIR_FREEZE_LIVE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
        $freeze = [regex]::Match($gdbStdout,
            'DOWN_AIR_FREEZE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
        $ring = [regex]::Match($gdbStdout,
            'DOWN_AIR_FREEZE_RING=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
        $sched = [regex]::Match($gdbStdout,
            'DOWN_AIR_SCHED=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
        $cpu = [regex]::Match($gdbStdout,
            'DOWN_AIR_CPU=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
        $threads = [regex]::Matches($gdbStdout,
            'DOWN_AIR_THREAD=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
        $io = [regex]::Match($gdbStdout,
            'DOWN_AIR_FREEZE_IO=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
        if ($exception.Success) {
            $exceptionv = Get-Ints $exception
            Assert-Condition ($exceptionRegs.Success -and
                $exceptionStack.Success -and
                $exceptionv[1] -ne 0 -and $exceptionv[4] -ne 0 -and
                $exceptionv[9] -eq 213 -and $exceptionv[10] -eq 188 -and
                ($exceptionv[12] + $exceptionv[13]) -eq 3600) `
                "$Actor exception context was incomplete or left Down-Air." `
                $gdbStdout
            Write-Output ((
                'Down-Air exception captured: actor={0} flags={1} ctx={2} ' +
                'pc={3} lr={4} sp={5} cpsr={6} spsr={7} ' +
                'cp15/excpt={8}/{9} status={10} motion={11} tic={12} ' +
                'regs=[{13}] stack=[{14}].') -f
                $Actor, $exceptionv[0], $exception.Groups[2].Value,
                $exception.Groups[5].Value, $exception.Groups[4].Value,
                $exception.Groups[3].Value, $exception.Groups[6].Value,
                $exception.Groups[7].Value, $exception.Groups[8].Value,
                $exception.Groups[9].Value, $exceptionv[9],
                $exceptionv[10], $exceptionv[11],
                ((1..13 | ForEach-Object {
                    $exceptionRegs.Groups[$_].Value
                }) -join ','),
                ((1..12 | ForEach-Object {
                    $exceptionStack.Groups[$_].Value
                }) -join ','))
            return
        }
        if ($live.Success) {
            Assert-Condition ($ring.Success -and $cpu.Success -and
                $sched.Success -and
                $threads.Count -ge 2) `
                "$Actor live freeze snapshot omitted target scheduler state." `
                $gdbStdout
            $livev = Get-Ints $live
            $ringv = Get-Ints $ring
            Assert-Condition ($livev[0] -ne 0 -and $livev[1] -ne 0 -and
                $livev[2] -gt 0 -and $livev[3] -ne 0 -and
                $livev[4] -ge 8 -and $livev[5] -eq 1 -and
                $livev[11] -eq 213 -and $livev[12] -eq 188 -and
                ($livev[18] + $livev[19]) -eq 3600 -and
                ($ringv | Where-Object { $_ -ne 0 }).Count -gt 0) `
                "$Actor live freeze snapshot was incomplete or left Down-Air." `
                $live.Value
            Write-Output ((
                'Down-Air live freeze captured: actor={0} pc={1} lr={2} ' +
                'heartbeat={3} last={4} write={5} watchdog={6}/{7}/{8} ' +
                'trip/force={9}/{10} status={11} motion={12} tic={13} ' +
                'irq={14}/{15}/{16} ring=[{17}] sched={18}/{19}/{20}/{21} ' +
                'cpu={22}/{23}/{24}/{25}/{26} threads=[{27}].') -f
                $Actor, $live.Groups[1].Value, $live.Groups[2].Value,
                $livev[2], $live.Groups[4].Value, $livev[4],
                $livev[6], $livev[7], $livev[8], $livev[9], $livev[10],
                $livev[11], $livev[12], $livev[13],
                $live.Groups[21].Value, $live.Groups[22].Value,
                $live.Groups[23].Value,
                ((1..8 | ForEach-Object { $ring.Groups[$_].Value }) -join ','),
                $sched.Groups[1].Value, $sched.Groups[2].Value,
                $sched.Groups[3].Value, $sched.Groups[4].Value,
                $cpu.Groups[1].Value, $cpu.Groups[2].Value,
                $cpu.Groups[3].Value, $cpu.Groups[4].Value,
                $cpu.Groups[5].Value,
                (($threads | ForEach-Object {
                    '{0}:{1}/{2}/{3}/{4}/{5}/{6}' -f
                        $_.Groups[2].Value, $_.Groups[4].Value,
                        $_.Groups[5].Value, $_.Groups[10].Value,
                        $_.Groups[11].Value, $_.Groups[12].Value,
                        $_.Groups[13].Value
                }) -join ','))
            return
        }
        Assert-Condition ($freeze.Success -and $ring.Success -and $io.Success) `
            "$Actor freeze watchdog did not emit a complete target report." `
            $gdbStdout
        $fv = Get-Ints $freeze
        $ringv = Get-Ints $ring
        $iov = Get-Ints $io
        Assert-Condition ($fv[0] -ne 0 -and $fv[1] -ne 0 -and
            $fv[2] -ge 1 -and $fv[3] -eq 0x5354414c -and
            $fv[4] -gt 0 -and $fv[5] -gt 0 -and
            $fv[6] -gt 0 -and $fv[7] -ne 0 -and $fv[8] -ge 8 -and
            $fv[9] -eq 213 -and $fv[10] -eq 188 -and
            ($fv[16] + $fv[17]) -eq 3600 -and
            ($ringv | Where-Object { $_ -ne 0 }).Count -gt 0) `
            "$Actor freeze watchdog report was incomplete or left Down-Air." `
            $freeze.Value
        Write-Output ((
            'Down-Air freeze watchdog captured: actor={0} pc={1} lr={2} ' +
            'logic={3} present={4} heartbeat={5} last={6} write={7} ' +
            'status={8} motion={9} tic={10} gx={11} poly/vertex={12}/{13} ' +
            'fgm={14}/{15} ring=[{16}].') -f
            $Actor, $freeze.Groups[1].Value, $freeze.Groups[2].Value,
            $fv[4], $fv[5], $fv[6], $freeze.Groups[8].Value, $fv[8],
            $fv[9], $fv[10], $fv[11], $io.Groups[1].Value, $iov[1],
            $iov[2], $iov[6], $iov[7],
            ((1..8 | ForEach-Object { $ring.Groups[$_].Value }) -join ','))
        return
    }

    $result = [regex]::Match($gdbStdout,
        'DOWN_AIR_RESULT=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $entry = [regex]::Match($gdbStdout,
        'DOWN_AIR_ENTRY=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $asset = [regex]::Match($gdbStdout,
        'DOWN_AIR_ASSET=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $knee = [regex]::Match($gdbStdout,
        'DOWN_AIR_KNEE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $baseline = [regex]::Match($gdbStdout,
        'DOWN_AIR_BASELINE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $terminal = [regex]::Match($gdbStdout,
        'DOWN_AIR_TERMINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $aerials = [regex]::Match($gdbStdout,
        'DOWN_AIR_AERIALS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $scene = [regex]::Match($gdbStdout,
        'DOWN_AIR_SCENE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $actors = [regex]::Match($gdbStdout,
        'DOWN_AIR_ACTORS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memory = [regex]::Match($gdbStdout,
        'DOWN_AIR_MEMORY=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $rv = Get-Ints $result
    $kv = Get-Ints $knee
    $ev = Get-Ints $entry
    $assetv = Get-Ints $asset
    $bv = Get-Ints $baseline
    $tv = Get-Ints $terminal
    $airv = Get-Ints $aerials
    $sv = Get-Ints $scene
    $av = Get-Ints $actors
    $mv = Get-Ints $memory

    Assert-Condition ($result.Success -and $rv[0] -eq $actorKind) `
        'Down-Air verifier did not emit its actor result.' $gdbStdout
    if ($rv[1] -eq 3) {
        throw "$Actor stalled in KneeBend before Down-Air (updates=$($kv[0]) frame=$($knee.Groups[2].Value) speed=$($knee.Groups[3].Value) length=$($knee.Groups[4].Value)).`n$gdbStdout"
    }
    if ($rv[1] -eq 2) {
        throw "$Actor did not enter Down-Air before match time-up.`n$gdbStdout"
    }
    Assert-Condition ($rv[1] -eq 1 -and $rv[2] -eq 9 -and $rv[3] -eq 1) `
        "$Actor did not naturally complete Down-Air." $gdbStdout
    Assert-Condition ($knee.Success -and $kv[0] -gt 0 -and
        $kv[2] -ne 0 -and $kv[3] -ne 0) `
        "$Actor did not traverse a live KneeBend animation before Down-Air." `
        $gdbStdout
    Assert-Condition ($entry.Success -and $ev[0] -eq 213 -and
        $ev[1] -eq 188 -and $ev[2] -eq 1 -and
        $ev[3] -ge 1 -and $ev[4] -ge 1 -and
        $ev[5] -eq 0 -and $ev[6] -eq 0 -and $ev[7] -eq 0 -and
        $ev[12] -ne 0 -and $ev[14] -eq 1) `
        "$Actor Down-Air did not load a valid animation payload cleanly." `
        $gdbStdout
    Assert-Condition ($asset.Success -and
        $assetv[0] -eq [int64]$downAirAssetToken -and
        $assetv[1] -eq $expectedAssetId -and
        $assetv[2] -ne 0 -and $assetv[2] -eq $assetv[3] -and
        $assetv[3] -eq $assetv[4] -and $assetv[5] -gt 0 -and
        $assetv[6] -eq 22 -and $assetv[7] -eq 0 -and
        $assetv[8] -gt 188) `
        "$Actor Down-Air did not resolve its exact source asset token and payload." `
        $gdbStdout
    Assert-Condition ($baseline.Success -and $aerials.Success -and
        $airv[0] -eq 0 -and $airv[1] -eq 0 -and $airv[2] -eq 0 -and
        $airv[3] -eq 0 -and $airv[4] -ge 1) `
        "$Actor did not select only the original Down-Air aerial status." `
        $gdbStdout
    $logicDelta = $tv[0] - $bv[0]
    $cpuDelta = $tv[1] - $bv[1]
    $presentDelta = $tv[2] - $bv[2]
    $readsDelta = $tv[3] - $bv[3]
    $actorUpdatesDelta = $tv[4] - $bv[4]
    $animChanged = [int]($tv[5] -ne $ev[9])
    $minimumCpuUpdates = if ($Actor -eq 'Mario') { 8 } else { 0 }
    Assert-Condition ($terminal.Success -and $logicDelta -ge 8 -and
        $cpuDelta -ge $minimumCpuUpdates -and
        $presentDelta -ge 4 -and $readsDelta -gt 0 -and
        $actorUpdatesDelta -ge 8 -and $animChanged -eq 1 -and
        $tv[7] -eq 213 -and $tv[8] -eq 188 -and
        $tv[9] -ne 213 -and $tv[10] -gt 0 -and
        $tv[11] -eq $bv[5] -and $tv[12] -eq $bv[6] -and
        $tv[13] -eq $bv[7]) `
        "$Actor Down-Air stopped logic, animation time, or controller reads." `
        $gdbStdout
    Assert-Condition ($scene.Success -and $sv[0] -eq 163 -and
        $sv[1] -eq 22 -and $sv[2] -eq 1 -and $sv[3] -eq 1 -and
        $sv[4] -gt 0 -and ($sv[4] + $sv[5]) -eq 3600 -and
        $sv[6] -eq 0 -and $sv[7] -eq 1) `
        'Down-Air qualification left the canonical one-minute match.' `
        $gdbStdout
    $expectedFoxKind = if ($Actor -eq 'Mario') { 1 } else { 0 }
    $expectedFoxCpuEnabled = if ($Actor -eq 'Mario') { 1 } else { 0 }
    $expectedPlayerCount = if ($Actor -eq 'Mario') { 1 } else { 2 }
    $expectedCpuCount = if ($Actor -eq 'Mario') { 1 } else { 0 }
    Assert-Condition ($actors.Success -and $av[0] -eq 0 -and
        $av[1] -eq 0 -and $av[2] -eq $expectedFoxKind -and
        $av[3] -eq 1 -and $av[4] -eq 3 -and
        $av[5] -eq $expectedFoxCpuEnabled -and
        $av[6] -eq $expectedPlayerCount -and
        $av[7] -eq $expectedCpuCount) `
        'Down-Air qualification used an unexpected fighter control configuration.' `
        $gdbStdout
    Assert-Condition ($memory.Success -and $mv[0] -eq 0x4d4c4544 -and
        $mv[1] -eq 22 -and $mv[2] -ge 131072) `
        'Down-Air qualification violated the P1 arena reserve.' $gdbStdout
    Assert-Condition (Test-Path -LiteralPath $screenshotPath -PathType Leaf) `
        'Down-Air qualification did not produce a screenshot.' $gdbStdout
    Assert-Condition ((Get-Item $screenshotPath).Length -gt 1024) `
        'Down-Air screenshot is unexpectedly small.' $gdbStdout

    Write-Output ((
        '{0} Down-Air passed: knee={1} actorUpdates={2} completedPresents={3}/4 ' +
        'asset=0x{4:x} token=0x{5:x} exit={6} reloc={7}/{8} ' +
        'logic/cpu/reads={9}/{10}/{11} updateTicks={12} reserve={13} screenshot={14}') -f
        $Actor, $kv[0], $actorUpdatesDelta, $presentDelta,
        $assetv[1], $assetv[0], $tv[9], $ev[3], $ev[4],
        $logicDelta, $cpuDelta, $readsDelta, $tv[10], $mv[2],
        $screenshotPath)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
    }
    Restore-MelonDSGdbConfig -State $configState
}
