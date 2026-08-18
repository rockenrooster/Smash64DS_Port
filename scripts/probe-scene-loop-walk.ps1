[CmdletBinding()]
param(
    [string]$Build = 'build-p2-1b-walk',
    [string]$Target = 'smash64ds-battle-playable-fast-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 600,
    # 3 loops = 1 startup entry + 4 x VSBattle + 4 x VSResults + 3 x VSMode,
    # plus one more per Sudden Death the bounded battle scores into.
    [ValidateRange(2, 32)][int]$Hits = 12,
    # Read the ring out of a ROM built WITHOUT the walk -- the Boundary proof
    # build, for instance, whose own scene sequence is
    # VSBattle -> [Sudden Death] -> VSResults. Fewer entries, on the shipping
    # configuration. Must be asked for explicitly: a run that silently measured
    # a parking ROM would report "no leak" having exercised one entry.
    [switch]$AllowNoWalk,
    [string]$Artifact = ''
)

# P2-1b leak evidence. Reads the scene manager's per-entry arena ring out of a
# ROM built with NDS_R2_SCENE_LOOP_WALK=<loops>, which drives
# menu -> battle -> results -> menu automatically instead of parking.
#
# WHAT THE NUMBERS MEAN. `syTaskmanStartTask` rewinds the taskman general arena
# on every scene entry (decomp sys/taskman.c:1227 -> :258). If that rewind is
# the whole story, then entry k and entry k+3 into the SAME scene kind must
# reach the SAME arena high-water: same scene, same content, same allocations,
# from the same base. A high-water that climbs loop over loop is a leak, and
# the ring names the scene it leaked in. This is the row's discriminating read;
# a screenshot cannot see it and a single end-of-run heap number cannot either,
# because the arena has already been rewound by the time a run ends.
#
# The breakpoint is ndsSceneManagerEnter, which runs BEFORE the arena is
# re-initialised, so the ring it prints is complete for every entry that has
# already exited. Nothing here writes guest memory.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_scene-loop-walk.txt')
}

$buildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "scene-loop-walk probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$walkMatch = [regex]::Match((Get-Content -LiteralPath $buildConfig -Raw),
    '(?m)^#define\s+NDS_R2_SCENE_LOOP_WALK\s+(\d+)u?$')
$walkValue = if ($walkMatch.Success) { $walkMatch.Groups[1].Value } else { 'absent' }
$walkRequested = ($walkValue -ne '0') -and ($walkValue -ne 'absent')
if (-not $walkRequested) {
    if (-not $AllowNoWalk) {
        # Asking for the walk and silently measuring a ROM that parks after one
        # scene would read as "no leak" having exercised one entry.
        throw ("scene-loop-walk probe: $Build was built with " +
            "NDS_R2_SCENE_LOOP_WALK=$walkValue, so the ROM does not close the " +
            'loop on its own. Pass -AllowNoWalk to read the ring anyway.')
    }
    Write-Output 'build config: NDS_R2_SCENE_LOOP_WALK=0 (-AllowNoWalk): reading the ROM own scene sequence, not a loop.'
} else {
    Write-Output ("build config: NDS_R2_SCENE_LOOP_WALK={0}" -f $walkValue)
}

$required = @(
    'ndsSceneManagerEnter',
    'ndsSceneManagerExit',
    'gNdsSceneManagerEnterCount',
    'gNdsSceneManagerExitCount',
    'gNdsSceneManagerRequestCount',
    'gNdsSceneManagerRejectCount',
    'gNdsSceneManagerCurrKind',
    'gNdsSceneManagerCurrTransition',
    'gNdsSceneManagerArenaBase',
    'gNdsSceneManagerArenaSize',
    'gNdsSceneManagerArenaMismatchCount',
    'gNdsSceneManagerUnregisteredEnterCount',
    'gNdsSceneManagerRingKind',
    'gNdsSceneManagerRingArenaHigh',
    'gNdsSceneManagerRingArenaFree',
    'gNdsSceneManagerRingTransition'
)
# The walk counters are referenced only by the walk tails, so --gc-sections
# drops them from a NDS_R2_SCENE_LOOP_WALK=0 build even though they carry
# __attribute__((used)) -- `used` binds the compiler, not the linker. Required
# when the walk is compiled in, absent by construction when it is not.
if ($walkRequested) {
    $required += @('gNdsSceneWalkHopsRemaining', 'gNdsSceneWalkLoopsCompleted')
}
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("scene-loop-walk probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# The battle-time heap low-water is written only by the realtime present path
# (taskman_seam.c:4948), so --gc-sections legitimately drops it from a
# fast-logic walk build. Its absence is reported, never assumed: reading 0 for
# a symbol that is not there would look like an exhausted heap.
$hasHeapFreeMin = $symbols -contains 'gNdsTaskmanGeneralHeapFreeMin'
if (-not $hasHeapFreeMin) {
    Write-Output ('note: gNdsTaskmanGeneralHeapFreeMin is absent from this ' +
        'build (no realtime present path); per-scene arena free comes from ' +
        'the ring instead.')
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.scene-loop-walk.stdout.log'
$stderr = Join-Path $log_dir 'melonds.scene-loop-walk.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set print elements 64',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        'break ndsSceneManagerEnter',
        'commands',
        'silent',
        'set $n = $n + 1',
        $(if ($walkRequested) {
            'printf "SLW enter %d curr=%u prev=%u tr=%u enters=%u exits=%u req=%u rej=%u unreg=%u mism=%u hops=%u loops=%u\n", $n, gNdsSceneManagerCurrKind, gNdsSceneManagerPrevKind, gNdsSceneManagerCurrTransition, gNdsSceneManagerEnterCount, gNdsSceneManagerExitCount, gNdsSceneManagerRequestCount, gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, gNdsSceneManagerArenaMismatchCount, gNdsSceneWalkHopsRemaining, gNdsSceneWalkLoopsCompleted'
        } else {
            'printf "SLW enter %d curr=%u prev=%u tr=%u enters=%u exits=%u req=%u rej=%u unreg=%u mism=%u hops=n/a loops=n/a\n", $n, gNdsSceneManagerCurrKind, gNdsSceneManagerPrevKind, gNdsSceneManagerCurrTransition, gNdsSceneManagerEnterCount, gNdsSceneManagerExitCount, gNdsSceneManagerRequestCount, gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, gNdsSceneManagerArenaMismatchCount'
        }),
        # The ring is re-emitted on EVERY hit, not once at the end. A walk that
        # parks before the hit cap leaves gdb blocked in `continue` until the
        # timeout, and a ring printed only after the cap would be lost exactly
        # in the case worth reading.
        'printf "SLWRINGK %d %u %u %u %u %u %u %u %u %u %u %u %u\n", $n, gNdsSceneManagerRingKind[0], gNdsSceneManagerRingKind[1], gNdsSceneManagerRingKind[2], gNdsSceneManagerRingKind[3], gNdsSceneManagerRingKind[4], gNdsSceneManagerRingKind[5], gNdsSceneManagerRingKind[6], gNdsSceneManagerRingKind[7], gNdsSceneManagerRingKind[8], gNdsSceneManagerRingKind[9], gNdsSceneManagerRingKind[10], gNdsSceneManagerRingKind[11]',
        'printf "SLWRINGH %d %u %u %u %u %u %u %u %u %u %u %u %u\n", $n, gNdsSceneManagerRingArenaHigh[0], gNdsSceneManagerRingArenaHigh[1], gNdsSceneManagerRingArenaHigh[2], gNdsSceneManagerRingArenaHigh[3], gNdsSceneManagerRingArenaHigh[4], gNdsSceneManagerRingArenaHigh[5], gNdsSceneManagerRingArenaHigh[6], gNdsSceneManagerRingArenaHigh[7], gNdsSceneManagerRingArenaHigh[8], gNdsSceneManagerRingArenaHigh[9], gNdsSceneManagerRingArenaHigh[10], gNdsSceneManagerRingArenaHigh[11]',
        'printf "SLWRINGF %d %u %u %u %u %u %u %u %u %u %u %u %u\n", $n, gNdsSceneManagerRingArenaFree[0], gNdsSceneManagerRingArenaFree[1], gNdsSceneManagerRingArenaFree[2], gNdsSceneManagerRingArenaFree[3], gNdsSceneManagerRingArenaFree[4], gNdsSceneManagerRingArenaFree[5], gNdsSceneManagerRingArenaFree[6], gNdsSceneManagerRingArenaFree[7], gNdsSceneManagerRingArenaFree[8], gNdsSceneManagerRingArenaFree[9], gNdsSceneManagerRingArenaFree[10], gNdsSceneManagerRingArenaFree[11]',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        'continue',
        $(if ($hasHeapFreeMin) {
            'printf "SLWSTOP n=%d arena_base=%x arena_size=%u heap_free_min=%u\n", $n, gNdsSceneManagerArenaBase, gNdsSceneManagerArenaSize, gNdsTaskmanGeneralHeapFreeMin'
        } else {
            'printf "SLWSTOP n=%d arena_base=%x arena_size=%u heap_free_min=absent\n", $n, gNdsSceneManagerArenaBase, gNdsSceneManagerArenaSize'
        }),
        'print gNdsSceneManagerRingKind',
        'print gNdsSceneManagerRingArenaHigh',
        'print gNdsSceneManagerRingArenaFree',
        'print/x gNdsSceneManagerRingTransition',
        $(if ($walkRequested) {
            'printf "SLWDONE enters=%u exits=%u rej=%u unreg=%u mism=%u loops=%u\n", gNdsSceneManagerEnterCount, gNdsSceneManagerExitCount, gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, gNdsSceneManagerArenaMismatchCount, gNdsSceneWalkLoopsCompleted'
        } else {
            'printf "SLWDONE enters=%u exits=%u rej=%u unreg=%u mism=%u loops=n/a\n", gNdsSceneManagerEnterCount, gNdsSceneManagerExitCount, gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, gNdsSceneManagerArenaMismatchCount'
        }),
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'scene_loop_walk_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the capture file, not the helper's return value: a probe whose ROM
    # parks before the hit cap exits by timeout, and the capture still holds
    # every entry line taken before that -- which is exactly the evidence that
    # says where the walk stopped.
    $captured = Join-Path $log_temp 'scene_loop_walk_probe.gdb.out'
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
