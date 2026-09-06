[CmdletBinding()]
param(
    [string]$Build = 'build-p2-campaign-walk',
    [string]$Target = 'smash64ds',
    [string]$Rom = '',
    [string]$Elf = '',
    [string]$BuildConfig = '',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(10, 120)][int]$TimeoutSeconds = 120,
    # Scene-entry stops, not frames: one stop is one ndsSceneManagerEnter.
    # The 1P route is Title -> ModeSelect -> 1PMode -> 1P CSS -> 1PIntro ->
    # first battle, so a handful of stops covers it; a guest the walk cannot
    # steer parks instead and the run ends by timeout (verdict BLOCKED).
    [ValidateRange(2, 64)][int]$Hits = 12,
    # Presents past the 1P-battle entry before the one screenshot. Entry
    # itself is pre-presentation; a short advance lands inside the match.
    [ValidateRange(1, 600)][int]$BattlePresents = 2,
    [string]$Artifact = '',
    [string]$Screenshot = ''
)

# CAMPAIGN-ENABLED ROM ACCEPTANCE PROBE. Drives a 1P build from cold boot
# over the REAL route -- native Title / ModeSelect -> source 1PMode menu ->
# source 1P CSS -> first battle -- and captures the actual scene ids, the
# battle's stage/fighter kinds and human stocks, the registry/allocator/
# asset error counters, and one presented battle frame.
#
# TWO GUEST-SIDE LEGS, both ordinary input. The native screens run on the
# scripted walk (NDS_P2_MENU_WALK / ndsMenuShellWalkTap) with the campaign
# route selected (gNdsMenuShellWalkRoute=1: Title START, then A on the opening
# 1P GAME cursor instead of the VS tour's DOWN+A). The two imported source
# menus (mn1pmode, mnplayers1pgame) read the source controller pipeline, so
# their leg is the walk's guest-side playback driver
# (ndsMenuShellWalkDrive1PSourceMenus in src/nds/nds_menu_shell_core.c, called
# from the source-menu pump in src/port/taskman_seam_harness.c): A past the
# 1PMode entry gate, cursor holds to Link's portrait + A on the 1P CSS, START
# past its 60-tic gate. Nothing here writes guest memory per frame, adds a
# guest harness mode, or completes the route by fiat: no scene_curr jumps, no
# victory poke, no seed restore. If the route cannot be steered, the run
# parks, the verdict reads BLOCKED, and the owning seam is named. A BLOCKED
# run is a missing-input report, not a pass.
#
# LAB BUILD CONTRACT, enforced below: NDS_P2_1P_GAME=1 (the campaign linked),
# NDS_P2_MENU_WALK!=0 (both walk legs compiled in), NDS_HARNESS_FAST_LOGIC=0
# (the walk's dwells and the driver's tic tables are realtime frames).
# Shipping/user ROMs keep NDS_P2_MENU_WALK=0 and human input; this probe
# refuses such a build rather than misreading it.
#
# STOP-POINT DRIVEN. Every wait is a breakpoint hit, never a wall-clock
# sleep for a menu or a match timer. (The one sleep below waits for the host
# OS to present the emulator window for the screenshot, not for the guest.)
#
# FAIL FAST. A nonzero allocator/asset/registry error counter, an ARM9 abort
# signature, or a dead emulator ends the run as FAILED, not as a verdict.

$ErrorActionPreference = 'Stop'
$scripts = Split-Path -Parent $PSScriptRoot
$root = Split-Path -Parent $scripts
. (Join-Path $scripts 'lib\melonds.ps1')
. (Join-Path $scripts 'lib\gdb-markers.ps1')
. (Join-Path $scripts 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
if ([string]::IsNullOrWhiteSpace($Rom)) {
    $Rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
}
if ([string]::IsNullOrWhiteSpace($Elf)) {
    $Elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
}
if ([string]::IsNullOrWhiteSpace($BuildConfig)) {
    $BuildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
}
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-campaign.txt')
}
if ([string]::IsNullOrWhiteSpace($Screenshot)) {
    $Screenshot = Join-Path $root ('artifacts\visibility\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-campaign-battle.png')
}
if (-not (Test-Path -LiteralPath $Rom -PathType Leaf)) {
    throw "p2-campaign probe: ROM not found: $Rom (pass -Rom explicitly)."
}
if (-not (Test-Path -LiteralPath $Elf -PathType Leaf)) {
    throw "p2-campaign probe: ELF not found: $Elf (pass -Elf explicitly)."
}
if (-not (Test-Path -LiteralPath $BuildConfig -PathType Leaf)) {
    throw "p2-campaign probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$configText = Get-Content -LiteralPath $BuildConfig -Raw
# REJECT FLAG 0. A run against a ROM without the campaign linked would walk
# the VS route (or park) and read as a broken campaign rather than a wrong
# build, exactly the trap probe-p2-shell.ps1 guards for the menu shell.
foreach ($flag in @('NDS_P2_1P_GAME')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        throw "p2-campaign probe: $Build was built with $flag off; nothing to read."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $m.Groups[1].Value)
}
# LAB INPUT CONTRACT. The campaign route is steered guest-side: the shell walk
# for the native screens plus the playback driver for the source menus. A ROM
# without the walk (human-input shipping config) would park on ModeSelect and
# read as a broken campaign rather than a wrong build, so refuse it up front.
# Fast logic would shrink the walk's dwells and the driver's tic tables below
# the source gates they are written against, so refuse that too.
foreach ($flag in @('NDS_P2_MENU_WALK')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        throw "p2-campaign probe: $Build was built with $flag off; the campaign walk is not linked (rebuild the lab 1P config, never the shipping ROM)."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $m.Groups[1].Value)
}
foreach ($flag in @('NDS_HARNESS_FAST_LOGIC')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    $value = if ($m.Success) { $m.Groups[1].Value } else { 'absent' }
    if ($value -ne '0') {
        throw "p2-campaign probe: $Build has $flag=$value; the walk needs realtime frames (need 0)."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $value)
}
foreach ($flag in @('NDS_P2_MENU_SHELL',
                    'NDS_R2_SCENE_LOOP_WALK', 'NDS_DEV_SCENE_HARNESS')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    $value = if ($m.Success) { $m.Groups[1].Value } else { 'absent' }
    Write-Output ("build config: {0}={1}" -f $flag, $value)
}

$required = @(
    'ndsSceneManagerEnter',
    'ndsPlatformEndFrame',
    'gSCManagerSceneData',
    'gSCManagerBattleState',
    'gNdsSceneManagerEnterCount',
    'gNdsSceneManagerRejectCount',
    'gNdsSceneManagerUnregisteredEnterCount',
    'gNdsSceneManagerArenaMismatchCount',
    'gNdsSceneManagerArenaBase',
    'gNdsSceneManagerArenaSize',
    'gNdsTaskmanArenaAllocFailCount',
    'gNdsRelocAssetOpenFailCount',
    'gNdsRelocAssetFormatFailCount',
    'gNdsRelocExternalFixupFailCount',
    'gNdsRelocAssetHeaderReadCount',
    'gNdsRelocAssetPayloadReadCount',
    'gNdsRendererProfileFrameCount',
    # Campaign-walk legs: the route select the probe arms, and the controller
    # pipeline telemetry proving the driver's input reached the source menus.
    'gNdsMenuShellWalkRoute',
    'gNdsControllerPlaybackEnabled',
    'gNdsControllerPlaybackConnectedMask',
    'gNdsControllerPublishedTapMask'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-campaign probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# The 1P-owned battle state is read when the 1P build links it; its absence
# is a link-stage report, not a probe defect, so it stays optional.
$has1PState = ($symbols -contains 'gSCManager1PGameBattleState')
if (-not $has1PState) {
    Write-Output 'note: gSCManager1PGameBattleState absent; CP1P will read `absent`.'
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.p2-campaign.stdout.log'
$stderr = Join-Path $log_dir 'melonds.p2-campaign.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$capture = Join-Path $scripts 'capture-running-melonds-window.ps1'
$config_state = $null
$emulator = $null
$timedOut = $false

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -BreakOnStartup -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    # Visible by design: the run photographs the emulator window once the
    # 1P battle presents, and a hidden launch leaves MainWindowHandle at
    # IntPtr.Zero, so the shot would come out black (capture-p2-shell.ps1).
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -PassThru
    $deadline = (Get-Date).AddSeconds(30)
    do {
        Start-Sleep -Milliseconds 250
        $emulator.Refresh()
    } while (($emulator.MainWindowHandle -eq [IntPtr]::Zero) -and
             (-not $emulator.HasExited) -and ((Get-Date) -lt $deadline))
    if ($emulator.HasExited -or ($emulator.MainWindowHandle -eq [IntPtr]::Zero)) {
        throw 'melonDS did not present a window to capture.'
    }
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    # Scene ids are the SCKind enum (include/sc/scene.h): Title 1,
    # ModeSelect 7, 1PMode 8, 1PGamePlayers 17, 1PGame 52. Fighter kinds are
    # FTKind (include/ft/fighter.h: Mario 0 .. Link 5); grounds are GRKind
    # (scene.h: Castle 0 .. Hyrule 4). Printed raw so the artifact, not this
    # script, is the record main validates.
    $battleLines = @(
        'printf "CPBATTLE %d gametype=%d gkind=%02x time=%u pl=%u cp=%u s0=%u/%u/%u s1=%u/%u/%u s2=%u/%u/%u s3=%u/%u/%u\n", $n, gSCManagerBattleState->game_type, gSCManagerBattleState->gkind, gSCManagerBattleState->time_limit, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count, gSCManagerBattleState->players[0].fkind, gSCManagerBattleState->players[0].pkind, gSCManagerBattleState->players[0].stock_count, gSCManagerBattleState->players[1].fkind, gSCManagerBattleState->players[1].pkind, gSCManagerBattleState->players[1].stock_count, gSCManagerBattleState->players[2].fkind, gSCManagerBattleState->players[2].pkind, gSCManagerBattleState->players[2].stock_count, gSCManagerBattleState->players[3].fkind, gSCManagerBattleState->players[3].pkind, gSCManagerBattleState->players[3].stock_count'
    )
    $stopLines = @(
        'printf "CPLINE %d curr=%u prev=%u enters=%u rej=%u unreg=%u mism=%u arenabase=%08x arenasize=%u\n", $n, gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsSceneManagerEnterCount, gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, gNdsSceneManagerArenaMismatchCount, gNdsSceneManagerArenaBase, gNdsSceneManagerArenaSize',
        'printf "CPERR %d allocfail=%u openfail=%u formatfail=%u fixupfail=%u hdr=%u payload=%u\n", $n, gNdsTaskmanArenaAllocFailCount, gNdsRelocAssetOpenFailCount, gNdsRelocAssetFormatFailCount, gNdsRelocExternalFixupFailCount, gNdsRelocAssetHeaderReadCount, gNdsRelocAssetPayloadReadCount',
        'printf "CPGFX %d peak=%u capacity=%u overflow=%u noroom=%u dl_overflow=%u\n", $n, gNdsTaskmanGraphicsHeapHighWater, gNdsTaskmanGraphicsHeapCapacity, gNdsTaskmanGraphicsHeapOverflowCount, gNdsTaskmanGraphicsHeapNoRoomCount, gNdsTaskmanDLOverflowCount',
        # Controller-pipeline proof: the driver's A/START taps must publish
        # through the source edge accumulator (bit 15 set == A delivered).
        'printf "CPCTL %d en=%u mask=%x published=%x\n", $n, gNdsControllerPlaybackEnabled, gNdsControllerPlaybackConnectedMask, gNdsControllerPublishedTapMask',
        'if gSCManagerBattleState != 0'
    ) + $battleLines + @(
        'else',
        'printf "CPBATTLE %d none\n", $n',
        'end',
        $(if ($has1PState) {
            'printf "CP1P %d player=%u fkind=%u diff=%u stocks=%u\n", $n, gSCManagerSceneData.player, gSCManagerSceneData.fkind, gSCManagerBackupData.spgame_difficulty, gSCManagerBackupData.spgame_stock_count'
        } else {
            'printf "CP1P %d absent\n", $n'
        })
    )
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set print elements 128',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        'set $inbattle = 0',
        'set $battleframes = 0',
        'break ndsSceneManagerEnter',
        'commands',
        'silent',
        # Set after runtime/BSS initialization and before the native menu
        # processes input. A boot-time write could be cleared by startup.
        'set variable gNdsMenuShellWalkRoute = 1',
        'set $n = $n + 1'
    ) + $stopLines + @(
        # nSCKind1PGame (52) is the first campaign battle. Its own entry stop
        # is pre-presentation, so step presents, photograph the halted window
        # (a halted melonDS keeps its last frame up), and leave.
        'if gSCManagerSceneData.scene_curr == 52',
        'printf "CPBATTLE-HIT %d renderframe=%u\n", $n, gNdsRendererProfileFrameCount',
        'set $inbattle = 1',
        'end',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        # GDB discards the rest of a breakpoint command list on continue.
        # Count frame hits in their own list; never put a capture after a
        # continue expecting the previous list to resume.
        'break ndsPlatformEndFrame',
        'commands',
        'silent',
        'if $inbattle',
        'set $battleframes = $battleframes + 1',
        ('if $battleframes >= ' + $BattlePresents)
    ) + $stopLines + @(
        'printf "CPFRAME renderframe=%u\n", gNdsRendererProfileFrameCount',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
         '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' +
         $Screenshot + '"'),
        'printf "CPDONE n=%d\n", $n',
        'detach',
        'quit',
        'end',
        'end',
        'continue',
        'end',
        $(if ($symbols -contains 'ndsSyMallocOverflowHalt') {
            'break ndsSyMallocOverflowHalt'
        }),
        $(if (@($symbols -match '^ndsPreviewPackLoadHalt(?:\.|$)').Count -ne 0) {
            'break ndsPreviewPackLoadHalt'
        }),
        $(if ($symbols -contains '__excpt_entry') { 'break __excpt_entry' }),
        # Starts the run; returns at the battle screenshot or the Hits-th
        # stop. A guest the walk cannot steer never reaches either and the
        # run ends by timeout -- verdict BLOCKED, not FAILED.
        'continue',
        'printf "CPSTOP n=%d pc=%08x cpsr=%08x\n", $n, $pc, $cpsr',
        'info symbol $pc',
        'printf "CPREGISTERS r0=%08x r1=%08x lr=%08x cpsr=%08x\n", $r0, $r1, $lr, $cpsr',
        'printf "CPOOM request=%u free=%u align=%u caller=%08x\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, gNdsSyMallocOverflowAlignment, gNdsSyMallocOverflowCallerLR',
        $(if ($symbols -contains 'gNdsPreviewPackLoadCount') {
            'printf "CPPACK loaded=%u bytes=%u failure=%u kind=%u\n", gNdsPreviewPackLoadCount, gNdsPreviewPackDataBytes, gNdsPreviewPackFailure, gNdsPreviewPackFailureKind'
        }),
        'bt 10',
        'detach',
        'quit'
    )

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
            -ScriptName 'p2_campaign_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    }
    catch {
        # Timeout here means the guest parked: no new scene entries, no
        # battle, nothing crashed (a crash trips the abort verdict below on
        # the partial capture). Keep the capture and report BLOCKED.
        if ("$_" -match 'timed out after') { $timedOut = $true }
        else { throw }
    }
}
finally {
    $captured = Join-Path $log_temp 'p2_campaign_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}

# --- Verdict (reads the capture file, never the helper's return) -----------
if (-not (Test-Path -LiteralPath $Artifact -PathType Leaf)) {
    Write-Output 'VERDICT: FAILED no-capture (no GDB transcript; see runner logs)'
    exit 1
}
$text = Get-Content -LiteralPath $Artifact -Raw
$failReasons = @()

# BreakOnStartup normally stops at 0xfffffffc before symbolized ARM9 startup.
# Other unsymbolized stops or an abort/undefined CPU mode are failures.
$terminal = [regex]::Match($text, 'CPSTOP n=\d+ pc=[0-9a-fA-F]+ cpsr=([0-9a-fA-F]+)')
$terminalMode = if ($terminal.Success) {
    [Convert]::ToUInt32($terminal.Groups[1].Value, 16) -band 0x1f
} else { 0 }
if (($text -match '(?m)^0x(?!fffffffc)[0-9a-fA-F]+ in \?\? \(\)') -or
    ($terminalMode -in @(0x17, 0x1b))) {
    $failReasons += 'cpu-abort-signature'
}
if ($text -match '(?m)^ndsSyMallocOverflowHalt in section') {
    $failReasons += 'general-heap-overflow'
}
if ($text -match '(?m)^ndsPreviewPackLoadHalt(?:\S*) in section') {
    $failReasons += 'preview-pack-refusal'
}
foreach ($match in [regex]::Matches($text, '(?m)^CPGFX \d+ peak=\d+ capacity=\d+ overflow=(\d+) noroom=(\d+) dl_overflow=(\d+)')) {
    if ([uint32]$match.Groups[1].Value -ne 0 -or
        [uint32]$match.Groups[2].Value -ne 0 -or
        [uint32]$match.Groups[3].Value -ne 0) {
        $failReasons += 'graphics-buffer-refusal'
    }
}
# ArenaAllocFailCount counts unsuccessful sizing probes before a successful
# arena allocation (diagnostics_taskman_heap.c), not fatal task allocations.
# Preserve the value in CPERR; the overflow trap detects real exhaustion.
foreach ($line in @([regex]::Matches($text, '(?m)^CPERR \d+ allocfail=(\d+) openfail=(\d+) formatfail=(\d+) fixupfail=(\d+).*$'))) {
    if (([uint32]$line.Groups[2].Value -ne 0) -or
        ([uint32]$line.Groups[3].Value -ne 0) -or ([uint32]$line.Groups[4].Value -ne 0)) {
        # Fatal OOM/asset failure: fail fast, do not route-judge a sick guest.
        $failReasons += ('error-counters: ' + $line.Value.Trim())
        break
    }
}
# Startup intentionally uses its source overlay arena outside this registry;
# allow its one entry only when the capture actually saw Startup (kind 27).
$startupEntries = [regex]::Matches($text, '(?m)^CPLINE \d+ curr=27 ').Count
$mismatches = @([regex]::Matches($text, '(?m)^CPLINE \d+ curr=\d+ prev=\d+ enters=\d+ rej=(\d+) unreg=(\d+) mism=(\d+).*$') |
    Where-Object { ([uint32]$_.Groups[1].Value -ne 0) -or ([uint32]$_.Groups[2].Value -gt $startupEntries) -or ([uint32]$_.Groups[3].Value -ne 0) } |
    Select-Object -First 1)
if ($mismatches.Count -gt 0) {
    $failReasons += ('registry: ' + $mismatches.Value.Trim())
}
if ($failReasons.Count -gt 0) {
    Write-Output ('VERDICT: FAILED ' + ($failReasons -join ' | '))
    exit 1
}

$scenes = @([regex]::Matches($text, '(?m)^CPLINE \d+ curr=(\d+) prev=(\d+).*$') |
    ForEach-Object { [int]$_.Groups[1].Value })
Write-Output ('route scenes: ' + ($scenes -join ' -> '))
$saw1PMode = ($scenes -contains 8)
$saw1PCss = ($scenes -contains 17)
$sawBattle = ($text -match '(?m)^CPBATTLE-HIT')
$shot = ($text -match '(?m)^CPFRAME')
# Last controller-pipeline reading: the whole 2x2 in one line -- playback
# enabled with pad 0 connected, and the sticky published-tap mask carrying
# the driver's A (bit 15). Zero means the source menus never saw input.
$ctl = [regex]::Match($text, '(?m)^CPCTL \d+ en=(\d+) mask=([0-9a-fA-F]+) published=([0-9a-fA-F]+).*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$ctlOk = ($ctl.Success -and ([uint32]$ctl.Groups[1].Value -ne 0) -and
    (([uint32]('0x' + $ctl.Groups[3].Value)) -band 0x8000) -ne 0)
if ($ctl.Success) {
    Write-Output ('controller pipeline: en={0} mask=0x{1} published=0x{2} A-delivered={3}' -f
        $ctl.Groups[1].Value, $ctl.Groups[2].Value, $ctl.Groups[3].Value, $ctlOk)
}
# The 1P-owned select state names the fighter the route committed.
$css1p = [regex]::Match($text, '(?m)^CP1P \d+ player=(\d+) fkind=(\d+) diff=(\d+) stocks=(\d+).*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$linkCommitted = ($css1p.Success -and ($css1p.Groups[2].Value -eq '5'))
if ($css1p.Success) {
    Write-Output ('1P select: player={0} fkind={1} difficulty={2} stocks={3}' -f
        $css1p.Groups[1].Value, $css1p.Groups[2].Value,
        $css1p.Groups[3].Value, $css1p.Groups[4].Value)
}
if ($sawBattle -and $shot -and $saw1PMode -and $saw1PCss -and $ctlOk -and $linkCommitted -and
    (Test-Path -LiteralPath $Screenshot -PathType Leaf)) {
    $hit = [regex]::Match($text, '(?m)^CPBATTLE \d+ gametype=(\d+) gkind=([0-9a-fA-F]+) time=(\d+) pl=(\d+) cp=(\d+) s0=(\d+)/(\d+)/(\d+) s1=(\d+)/(\d+)/(\d+) s2=(\d+)/(\d+)/(\d+) s3=(\d+)/(\d+)/(\d+).*$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    if ($hit.Success) {
        Write-Output ('battle: gametype={0} gkind=0x{1} time={2} pl={3} cp={4}' -f
            $hit.Groups[1].Value, $hit.Groups[2].Value, $hit.Groups[3].Value,
            $hit.Groups[4].Value, $hit.Groups[5].Value)
        # Link is fkind 5. The VS transfer block does not describe a 1P
        # fight, so this decode is reported for main's validation; the route
        # verdict above (8 -> 17 -> 52 with fkind 5 committed) already stands.
        $kinds = @($hit.Groups[6].Value, $hit.Groups[9].Value,
                   $hit.Groups[12].Value, $hit.Groups[15].Value)
        $stocks = @($hit.Groups[8].Value, $hit.Groups[11].Value,
                    $hit.Groups[14].Value, $hit.Groups[17].Value)
        $content = if (($kinds -contains '5') -and ($hit.Groups[2].Value -eq '04') -and
                       (($stocks | Where-Object { [int]$_ -gt 0 }).Count -ge 1)) {
            'MATCH Link/Hyrule/stocks'
        } else { 'MISMATCH (main validates; 1P fights report through CP1P, not the VS block)' }
        Write-Output ('content: kinds={0} stocks={1} {2}' -f
            ($kinds -join ','), ($stocks -join ','), $content)
    }
    Write-Output ('VERDICT: PASS 1P-Link-battle-presented route=1-7-8-17-52 frame=' + $Screenshot)
    exit 0
}

# No invented success: name the owning seam and stop.
Write-Output 'VERDICT: BLOCKED route did not reach a presented 1P Link battle.'
if (-not $saw1PMode) {
    Write-Output ('seam: ModeSelect exit is owned by the shell walk ' +
        '(src/nds/nds_menu_shell_core.c kNdsMenuWalkMode1P steers A on the ' +
        'opening 1P GAME cursor under gNdsMenuShellWalkRoute=1). No 1PMode ' +
        'entry means that tap never committed: check the input-ring lines ' +
        'and ndsSceneManagerFind(nSCKind1PMode) registration in this build.')
} elseif (-not $saw1PCss) {
    Write-Output ('seam: 1PMode reached but no 1P CSS entry. The A tap at ' +
        'driver tic 12 is owned by ndsMenuShellWalkDrive1PSourceMenus ' +
        '(src/nds/nds_menu_shell_core.c) through the playback pads; ' +
        'CPCTL published=0 means the edge never published, otherwise the ' +
        'scene_prev/InitVars option state refused it.')
} elseif ($scenes -contains 14) {
    Write-Output 'seam: CSS committed and the 1P intro entered. Inspect intro construction, updates and its scheduler-clock exit; a cleared later tap mask is not evidence of missing CSS input.'
} elseif (-not $ctlOk) {
    Write-Output ('seam: 1P CSS reached but the controller pipeline shows no ' +
        'published A tap. Owner is the playback path ' +
        '(src/port/controller_backend.c osContGetReadData + the retrace ' +
        'thread publish) or the driver tic table.')
} elseif (-not $linkCommitted) {
    Write-Output ('seam: 1P CSS ran but CP1P fkind != 5 (Link). Owner is the ' +
        'cursor drive (UP 28 / RIGHT 24 from (60,170) in ' +
        'ndsMenuShellWalkDrive1PSourceMenus) or a preselected scene fkind; ' +
        'read the captured CP1P/CPCTL lines for which.')
} else {
    Write-Output ('seam: Link committed but no 1P battle presented within ' +
        $Hits + ' stops; battle-link owner (sc1PGame bridge) to identify ' +
        'from the captured CPLINE/CPERR lines above.')
}
if ($timedOut) {
    Write-Output 'note: no later scene entry was captured before timeout; this does not establish whether the guest was updating or hung.'
}
exit 2
