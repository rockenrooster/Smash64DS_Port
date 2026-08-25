[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Build,
    [Parameter(Mandatory = $true)][string]$Target,
    # How many battle frames to let run before the state dump. Two is enough to
    # prove the match started; raise it to watch the arena past setup. Only the
    # first and the last are printed, so a whole-match figure costs one line.
    [ValidateRange(1, 4096)][int]$BattleFrames = 2,
    # Name every scene-arena allocation of at least this many bytes, with its
    # caller. 0 disables. This is the census a scaling fix is sized from: the
    # per-kind file trees are only part of what a four-fighter battle spends,
    # and "where did the arena go" is not answerable from the per-kind table.
    [ValidateRange(0, 1048576)][int]$TraceMallocAtLeast = 0,
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [string]$Artifact = ''
)

# WHAT DOES A MATCH COST WHEN IT IS REACHED THROUGH THE SHIPPED MENUS?
#
# Board row P2-3f8 landed Captain Falcon and closed with an INFERENCE: the
# argmax four-kind roster costs 335,824 B against P2-3r13's measured 24,356 B
# margin, so it should exhaust the scene arena and reach
# ndsSyMallocOverflowHalt -- the permanent silent ARM9 halt that presented as
# "boots but never starts a match" in row P2-3r11. Nobody had ever measured a
# four-distinct-kind match driven FROM THE SHELL at any roster, and
# `NDS_P2_FOUR_CPU_ROSTER` (the lab stress arm) is a different entry path:
# nds_match_config.c's NDS_P2_FOUR_CPU_STRESS block writes the descriptor
# directly and, in its own words, "bypasses PlayersVS".
#
# This probe is the run that measures the shell instead of predicting it. It
# breaks on the four seams the answer lives at and lets the walk drive:
#
#   ndsMenuShellCssCommit        what the character select actually committed
#   ftManagerSetupFilesMainKind  every per-kind main-file tree, with the heap
#                                free BEFORE the request -- the currency P2-2's
#                                byte law is written in
#   ftManagerMakeFighter         so a roster that never reaches four is a count
#                                rather than silence
#   ndsSyMallocOverflowHalt      the halt itself, with everything it published
#
# Pair it with `NDS_P2_SHELL_ARGMAX_ROSTER=1`, which seeds the descriptor with
# the heaviest landed roster so the character select opens on it.
#
# A GDB POKE CANNOT SUBSTITUTE FOR THAT FLAG, and the first version of this
# probe was written believing it could (2026-08-25). It wrote the character
# select's own u8[4] state arrays at the commit breakpoint -- correctly, as
# aligned 32-bit stores, reading the new values straight back -- and the commit
# that ran one instruction later still published the OLD roster. melonDS's GDB
# stub reads and writes main RAM behind the ARM9 data cache, so a poke into a
# line the guest is actively using is invisible to the guest and is then
# overwritten by the writeback. This is the write-side of the known read-side
# trap ("gdb stack reads are stale on the cache fork", 2026-08-22). A poke only
# sticks in a location the CPU has not touched yet -- which is why
# `gNdsMenuShellWalkBudget` works and why per-frame screen state does not. Make
# the guest's own code write the value: that is a build flag, not a breakpoint.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_shell-four-kind.txt')
}

$required = @(
    'ndsMenuShellCssCommit', 'ftManagerSetupFilesMainKind',
    'ftManagerMakeFighter', 'ndsSyMallocOverflowHalt',
    'ifCommonBattleUpdateInterfaceAll',
    'gNdsSyMallocOverflowCount', 'gNdsSyMallocOverflowArenaID',
    'gNdsSyMallocOverflowRequest', 'gNdsSyMallocOverflowAlignment',
    'gNdsSyMallocOverflowHeadroom', 'gNdsSyMallocOverflowCallerLR',
    'gNdsTaskmanArenaChosenSize', 'gNdsTaskmanArenaAllocFailCount',
    'gNdsTaskmanHeapGeneration', 'gSYTaskmanGeneralHeap',
    'gNdsMenuShellCssCommitSlot', 'gSCManagerSceneData',
    'gNdsTaskmanGeneralHeapFreeMin', 'gNdsTaskmanGraphicsHeapHighWater',
    'gNdsTaskmanGraphicsHeapCapacity', 'gNdsTaskmanGraphicsHeapOverflowCount',
    'gNdsTaskmanGraphicsHeapNoRoomCount'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw "shell-four-kind probe symbols absent from ${elf}: $($missing -join ', ')"
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.shell-four-kind.stdout.log'
$stderr = Join-Path $log_dir 'melonds.shell-four-kind.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$free = '(unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr'

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

    $commands = [System.Collections.Generic.List[string]]::new()
    $commands.AddRange([string[]]@(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $c_commit = 0',
        'set $c_setup = 0',
        'set $c_make = 0',
        'set $c_batt = 0'
    ))

    # 1 -- the character select's commit, read on the way OUT so the roster in
    #      the artifact is the one the battle was handed.
    $commands.AddRange([string[]]@(
        'break ndsMenuShellCssCommit',
        'commands 1',
        'silent',
        'set $c_commit = $c_commit + 1',
        ('printf "FOURKIND COMMIT hit=%d s0=0x%06x s1=0x%06x s2=0x%06x s3=0x%06x free=%u\n", $c_commit, gNdsMenuShellCssCommitSlot[0], gNdsMenuShellCssCommitSlot[1], gNdsMenuShellCssCommitSlot[2], gNdsMenuShellCssCommitSlot[3], {0}' -f $free),
        'continue',
        'end'
    ))

    # 2 -- every per-kind main-file setup. `kind` is the ftKind ordinal in r0
    #      (ftmanager.c:281 `void ftManagerSetupFilesMainKind(s32 fkind)`), and
    #      `scene` separates the character select's preview residency from the
    #      battle's.
    $commands.AddRange([string[]]@(
        'break ftManagerSetupFilesMainKind',
        'commands 2',
        'silent',
        'set $c_setup = $c_setup + 1',
        ('printf "FOURKIND SETUP hit=%d kind=%d scene=%d free=%u ptr=0x%08x lr=0x%08x\n", $c_setup, $r0, (int)gSCManagerSceneData.scene_curr, {0}, (unsigned)gSYTaskmanGeneralHeap.ptr, $lr' -f $free),
        'continue',
        'end'
    ))

    $commands.AddRange([string[]]@(
        'break ftManagerMakeFighter',
        'commands 3',
        'silent',
        'set $c_make = $c_make + 1',
        ('printf "FOURKIND MAKE hit=%d scene=%d free=%u\n", $c_make, (int)gSCManagerSceneData.scene_curr, {0}' -f $free),
        'continue',
        'end'
    ))

    # 4 -- THE HALT. No continue: this is where the run ends if it ends here.
    $commands.AddRange([string[]]@(
        'break ndsSyMallocOverflowHalt',
        'commands 4',
        ('printf "FOURKIND HALT count=%u arena_id=%u request=%u align=%u headroom=%u caller_lr=0x%08x\n", gNdsSyMallocOverflowCount, gNdsSyMallocOverflowArenaID, gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowAlignment, gNdsSyMallocOverflowHeadroom, gNdsSyMallocOverflowCallerLR'),
        'info symbol gNdsSyMallocOverflowCallerLR',
        'bt 16',
        'end'
    ))

    # 5 -- the battle's own per-frame interface update. Reaching it is the proof
    #      the match STARTED, which is the other half of the answer.
    $commands.AddRange([string[]]@(
        'break ifCommonBattleUpdateInterfaceAll',
        'commands 5',
        'silent',
        'set $c_batt = $c_batt + 1',
        ('if ($c_batt == 1) || ($c_batt >= {0})' -f $BattleFrames),
        ('printf "FOURKIND BATTLE hit=%d make=%d setup=%d free=%u\n", $c_batt, $c_make, $c_setup, {0}' -f $free),
        'end',
        ('if $c_batt < {0}' -f $BattleFrames),
        'continue',
        'end',
        'end'
    ))

    if ($TraceMallocAtLeast -gt 0) {
        # `syTaskmanMalloc` is the obvious seam and it is the WRONG one: gdb
        # resolves it to 20 locations because it is inlined into most callers,
        # and at an inlined location the parameter `size` is not in scope -- the
        # whole capture dies on the first hit with a half-written printf.
        # `syMallocSet` is the port's own out-of-line wrapper
        # (battleship_sys_malloc.c) and takes (bp, size, alignment) by name, so
        # it has exactly one location and every argument is readable. The
        # backtrace is what names the buyer.
        if ($symbols -notcontains 'syMallocSet') {
            throw "syMallocSet absent from ${elf}: -TraceMallocAtLeast needs it."
        }
        $commands.AddRange([string[]]@(
            ('break syMallocSet if size >= {0}' -f $TraceMallocAtLeast),
            'commands 6',
            'silent',
            ('printf "FOURKIND BIGALLOC size=%u scene=%d free=%u\n", (unsigned)size, (int)gSCManagerSceneData.scene_curr, {0}' -f $free),
            'bt 5',
            'continue',
            'end'
        ))
    }

    $commands.Add('continue')
    $commands.AddRange([string[]]@(
        'printf "FOURKIND STOP pc=0x%x lr=0x%x commit=%d setup=%d make=%d batt=%d\n", $pc, $lr, $c_commit, $c_setup, $c_make, $c_batt',
        ('printf "FOURKIND ARENA chosen=%u allocfail=%u heapgen=%u start=0x%08x ptr=0x%08x end=0x%08x free=%u overflow=%u\n", gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount, gNdsTaskmanHeapGeneration, (unsigned)gSYTaskmanGeneralHeap.start, (unsigned)gSYTaskmanGeneralHeap.ptr, (unsigned)gSYTaskmanGeneralHeap.end, {0}, gNdsSyMallocOverflowCount' -f $free),
        'printf "FOURKIND ROSTER s0=0x%06x s1=0x%06x s2=0x%06x s3=0x%06x\n", gNdsMenuShellCssCommitSlot[0], gNdsMenuShellCssCommitSlot[1], gNdsMenuShellCssCommitSlot[2], gNdsMenuShellCssCommitSlot[3]',
        'printf "FOURKIND LOWWATER generalfreemin=%u gfxpeak=%u gfxcap=%u gfxoverflow=%u gfxnoroom=%u\n", gNdsTaskmanGeneralHeapFreeMin, gNdsTaskmanGraphicsHeapHighWater, gNdsTaskmanGraphicsHeapCapacity, gNdsTaskmanGraphicsHeapOverflowCount, gNdsTaskmanGraphicsHeapNoRoomCount',
        'bt 8',
        'detach',
        'quit'
    ))

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
            -ScriptName 'shell_four_kind_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    } catch {
        Write-Output ('shell-four-kind capture ended: ' +
            $_.Exception.Message.Split("`n")[0])
    }
}
finally {
    $captured = Join-Path $log_temp 'shell_four_kind_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force `
            -Path (Split-Path -Parent $Artifact) | Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        # Read the `Breakpoint N at ...` resolutions too: a symbol that gdb
        # resolves to several locations (inlined body) makes a zero hit count
        # meaningless, and filtering them away hides that (P2-3r10).
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(FOURKIND|#\d|Breakpoint |\S+ in section)' }
        Write-Output ("probe capture: " + $Artifact)
    }
    if ($null -ne $emulator) {
        # Ask first, kill second: a forced kill skips melonDS's SD flush and can
        # corrupt emulators/melonds/dldi.bin, which then reads as a TIMEOUT.
        try { $emulator.CloseMainWindow() | Out-Null } catch { }
        if (-not $emulator.WaitForExit(4000)) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state | Out-Null
    }
}
