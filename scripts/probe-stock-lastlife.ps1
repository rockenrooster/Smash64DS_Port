[CmdletBinding()]
param(
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [Parameter(Mandatory=$true)][string]$Build,
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    # The four-CPU arm hits the frame hook once per source frame, and a gdb stop
    # that walks four fighters costs roughly a third of a second. The run only
    # needs to reach -StopFrame, so size the ceiling for that, not for the match.
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    # Which player slot is driven out of the world. 3 is deliberate on a
    # four-fighter arm: eliminating the LAST slot leaves three teams alive, so
    # sIFCommonBattlePlace does not reach 0 and the match keeps running -- which
    # is the only shape in which "what happens to an eliminated fighter" is
    # observable at all.
    [ValidateRange(0, 3)][int]$Victim = 3,
    [ValidateRange(10, 3000)][int]$ArmFrame = 30,
    [ValidateRange(30, 3000)][int]$KillFrame = 600,
    [ValidateRange(60, 3600)][int]$StopFrame = 1800,
    [string]$Artifact = ''
)

# P2-3r14: DOES VS STOCK'S LAST-STOCK PATH REACH THE WEAK NO-OP, AND WHAT DOES
# THE FIGHTER DO WHEN IT DOES?
#
# The row was opened from a source-plus-link reading. This probe is the runtime
# half it owes. `ftCommonDeadCheckRebirth` (decomp ftcommondead.c:98-104) calls
# `ftCommonSleepSetStatus` and RETURNS when the stock rule is set and the
# fighter's stock_count has reached -1; the port's only definition of that
# function is a two-byte `NDS_WEAK` stub. A stub nothing reaches is not a
# defect, so the question has to be asked of a running match.
#
# TWO LEVERS, both of them values the game itself writes, applied from the
# once-per-battle-frame seam `ifCommonBattleUpdateInterfaceAll`:
#
#   1. `gSCManagerBattleState->game_rules = SCBATTLE_GAMERULE_STOCK` and every
#      live stock_count set to 0. That is exactly the state the shell's VS rules
#      screen commits (`ndsMenuShellVsSaveRules`, nds_menu_shell.c:1442-1445)
#      with the stock value at its 1-stock floor: `cfg->stocks` is the menu's
#      displayed count minus one, and `ftManagerMakeFighter` publishes it to
#      both the fighter and the battle state (ftmanager.c:698-703). The next
#      death takes stock_count to -1 through `ftCommonDeadUpdateScore`.
#   2. The victim's TopN joint is written below `map_bound_bottom`. That is not
#      a KO injection: it is the position the game's OWN blast-zone test reads
#      (`ftCommonDeadCheckInterruptCommon`, ftcommondead.c:471), so the whole
#      dead ladder -- DeadDown, the score/stock update, the dead wait, and
#      finally `ftCommonDeadCheckRebirth` -- runs source code end to end.
#
# Nothing else is written, no guest function is called, and every read is a
# global or a pointer derived from one (`gGCCommonLinks[3]` walked by
# `link_next`, and the breakpoint's own `$r0`), because stack locals are not
# trustworthy through melonDS's gdb stub (CLAUDE.OPUS.md).
#
# THE DISCRIMINATOR IS A PAIR, not a single count. `ftCommonDeadCheckRebirth`
# has exactly two endings: `ftCommonRebirthDownSetStatus` (respawn) or
# `ftCommonSleepSetStatus` (out). Breaking on both makes "the Sleep arm was
# taken" readable even if the two-byte weak stub resolves badly -- and the
# breakpoint on the stub is kept anyway, with gdb's `Breakpoint N at ...` line
# carried into the artifact, so a 0x0 resolution is visible rather than silent.
#
# The census prints every fighter's status/stock on a fixed cadence so a null
# result stays diagnosable: "the hook never ran", "the KO never landed" and
# "the fighter is stuck" are three different pictures, not one zero.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

# BattleShip constants, restated so the probe does not depend on gdb resolving
# an enum out of the right compilation unit.
$GAMERULE_STOCK = 2                 # sc/scdef.h SCBATTLE_GAMERULE_STOCK
$LINK_ID_FIGHTER = 3                # sys/objdef.h nGCCommonLinkIDFighter

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_stock-lastlife.txt')
}

$driverSymbol = 'ifCommonBattleUpdateInterfaceAll'
# Order matters only for readability; each gets its own hit counter.
$ladderSymbols = @(
    'ftCommonDeadDownSetStatus',
    'ftCommonDeadUpdateScore',
    'ifCommonBattleUpdateScoreStocks',
    'ifCommonAnnounceEndMessage',
    'ftCommonDeadCheckRebirth',
    'ftCommonRebirthDownSetStatus',
    'ftCommonSleepSetStatus'
)
$symbolTable = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
foreach ($name in (@($driverSymbol) + $ladderSymbols)) {
    if ($symbolTable -notcontains $name) {
        throw "probe-stock-lastlife: symbol '$name' absent from $elf."
    }
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.stock-lastlife.stdout.log'
$stderr = Join-Path $log_dir 'melonds.stock-lastlife.stderr.log'
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

    $commands = [System.Collections.Generic.List[string]]::new()
    $commands.AddRange([string[]]@(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort)
    ))

    # 1. The driver: one hit per battle frame.
    $commands.AddRange([string[]]@(
        ('break {0}' -f $driverSymbol),
        'set $frames = 0',
        'set $blasts = 0',
        'commands 1',
        'silent',
        'set $frames = $frames + 1',
        # The battle-entry heartbeat. Without it a run that dies during asset
        # load is indistinguishable from one whose lever never fired.
        'if ($frames == 1)',
        '  printf "TRACE FIRSTFRAME rules=%d status=%d\n", gSCManagerBattleState->game_rules, gSCManagerBattleState->game_status',
        'end',
        ('if ($frames == {0})' -f $ArmFrame),
        # EVERY BYTE-WIDE GUEST WRITE BELOW IS A 32-BIT READ-MODIFY-WRITE, and
        # that is not style. MEASURED 2026-08-25 on this fork, four runs:
        # a one-byte `set var` to a 4-BYTE-ALIGNED guest address kills melonDS
        # outright (host exit 0xC000001D, ILLEGAL INSTRUCTION), while the same
        # one-byte write to an UNALIGNED address succeeds. `$fst->level`
        # (FTStruct+19) wrote fine in the same loop iteration in which
        # `$fst->stock_count` (FTStruct+20) took the emulator down; gdb reports
        # only "Remote communication error. Target disconnected", which reads
        # exactly like a slow probe. The RMW form is aligned by construction, so
        # the wrong form is not expressible here. See scripts/lib/gdb-markers.ps1.
        '  printf "TRACE ARM0 frame=%d rules=%d\n", $frames, gSCManagerBattleState->game_rules',
        '  set $wp = (unsigned int *)(((unsigned int)&gSCManagerBattleState->game_rules) & ~3)',
        '  set $sh = ((((unsigned int)&gSCManagerBattleState->game_rules) & 3) * 8)',
        ('  set var *$wp = ((*$wp) & ~(255 << $sh)) | ({0} << $sh)' -f $GAMERULE_STOCK),
        '  printf "TRACE ARM1 rules=%d\n", gSCManagerBattleState->game_rules',
        '  set $i = 0',
        '  while ($i < 4)',
        '    set $wp = (unsigned int *)(((unsigned int)&gSCManagerBattleState->players[$i].stock_count) & ~3)',
        '    set $sh = ((((unsigned int)&gSCManagerBattleState->players[$i].stock_count) & 3) * 8)',
        '    set var *$wp = ((*$wp) & ~(255 << $sh))',
        '    set $i = $i + 1',
        '  end',
        ('  set $g = gGCCommonLinks[{0}]' -f $LINK_ID_FIGHTER),
        '  set $n = 0',
        '  while ($g != 0 && $n < 8)',
        # NOT `$fp`: gdb-markers.ps1 rejects convenience variables that shadow an
        # ARM register name, and $fp is one.
        '    set $fst = (FTStruct *)$g->user_data.p',
        '    if ($fst != 0)',
        '      set $wp = (unsigned int *)(((unsigned int)&$fst->stock_count) & ~3)',
        '      set $sh = ((((unsigned int)&$fst->stock_count) & 3) * 8)',
        '      set var *$wp = ((*$wp) & ~(255 << $sh))',
        '      printf "TRACE ARMFT n=%d p=%d kind=%d stock=%d\n", $n, $fst->player, $fst->fkind, $fst->stock_count',
        '    end',
        '    set $g = $g->link_next',
        '    set $n = $n + 1',
        '  end',
        '  printf "TRACE STOCKARM frame=%d rules=%d p0=%d p1=%d p2=%d p3=%d\n", $frames, gSCManagerBattleState->game_rules, gSCManagerBattleState->players[0].stock_count, gSCManagerBattleState->players[1].stock_count, gSCManagerBattleState->players[2].stock_count, gSCManagerBattleState->players[3].stock_count',
        'end',
        # Repeated until it takes, not for a fixed window: the interface update
        # and the fighter interrupt proc are different GObjs, so one write can
        # land on the wrong side of the blast-zone test within a frame -- and,
        # measured 2026-08-25, a fixed 9-frame window at frame 240 fired ZERO
        # times because the fighters were still `is_ghost` in the source's own
        # entry/countdown. The ghost guard is what makes the repeat safe: the
        # instant the KO takes, the fighter is a ghost and the writes stop.
        ('if ($frames >= {0} && $blasts < 6)' -f $KillFrame),
        ('  set $g = gGCCommonLinks[{0}]' -f $LINK_ID_FIGHTER),
        '  set $n = 0',
        '  while ($g != 0 && $n < 8)',
        '    set $fst = (FTStruct *)$g->user_data.p',
        ('    if ($fst != 0 && $fst->player == {0} && $fst->is_ghost == 0)' -f $Victim),
        '      set var $fst->joints[0]->translate.vec.f.y = gMPCollisionGroundData->map_bound_bottom - 3000',
        '      set $blasts = $blasts + 1',
        '      printf "TRACE FORCEBLAST frame=%d n=%d p=%d status=%d stock=%d\n", $frames, $blasts, $fst->player, $fst->status_id, $fst->stock_count',
        '    end',
        '    set $g = $g->link_next',
        '    set $n = $n + 1',
        '  end',
        'end',
        'if ($frames % 150 == 0)',
        ('  set $g = gGCCommonLinks[{0}]' -f $LINK_ID_FIGHTER),
        '  set $n = 0',
        '  while ($g != 0 && $n < 8)',
        '    set $fst = (FTStruct *)$g->user_data.p',
        '    if ($fst != 0)',
        '      printf "TRACE CENSUS frame=%d p=%d kind=%d status=%d stock=%d pstock=%d ghost=%d invis=%d camera=%d\n", $frames, $fst->player, $fst->fkind, $fst->status_id, $fst->stock_count, gSCManagerBattleState->players[$fst->player].stock_count, $fst->is_ghost, $fst->is_invisible, $fst->camera_mode',
        '    end',
        '    set $g = $g->link_next',
        '    set $n = $n + 1',
        '  end',
        '  printf "TRACE CLOCK frame=%d status=%d remain=%d\n", $frames, gSCManagerBattleState->game_status, gSCManagerBattleState->time_remain',
        'end',
        ('if ($frames < {0})' -f $StopFrame),
        '  continue',
        'else',
        '  printf "TRACE STOPFRAME frame=%d\n", $frames',
        'end',
        'end'
    ))

    # 2. The ladder. `ftCommonDeadCheckRebirth` prints the two values its own
    #    branch reads, off the entry register rather than through a stack frame.
    $index = 2
    foreach ($name in $ladderSymbols) {
        $body = [System.Collections.Generic.List[string]]::new()
        $body.AddRange([string[]]@(
            ('break {0}' -f $name),
            ('set $h{0} = 0' -f $index),
            ('commands {0}' -f $index),
            'silent',
            ('set $h{0} = $h{0} + 1' -f $index)
        ))
        if ($name -eq 'ftCommonDeadCheckRebirth') {
            $body.AddRange([string[]]@(
                'set $cr = (FTStruct *)((GObj *)$r0)->user_data.p',
                ('printf "TRACE {0} hit=%d frame=%d p=%d stock=%d rules=%d status=%d\n", $h{1}, $frames, $cr->player, $cr->stock_count, gSCManagerBattleState->game_rules, $cr->status_id' -f $name, $index)
            ))
        } elseif ($name -eq 'ftCommonSleepSetStatus' -or
                  $name -eq 'ftCommonRebirthDownSetStatus') {
            $body.AddRange([string[]]@(
                'set $cr = (FTStruct *)((GObj *)$r0)->user_data.p',
                ('printf "TRACE {0} hit=%d frame=%d p=%d stock=%d\n", $h{1}, $frames, $cr->player, $cr->stock_count' -f $name, $index)
            ))
        } else {
            $body.Add(('printf "TRACE {0} hit=%d frame=%d\n", $h{1}, $frames' -f $name, $index))
        }
        $body.AddRange([string[]]@('continue', 'end'))
        $commands.AddRange([string[]]$body.ToArray())
        $index++
    }

    $commands.Add('continue')
    $commands.Add('printf "TRACE STOP pc=0x%x lr=0x%x frames=%d\n", $pc, $lr, $frames')
    $commands.AddRange([string[]]@('detach', 'quit'))

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
            -ScriptName 'stock_lastlife_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    } catch {
        Write-Output ("stock-lastlife probe ended: " + $_.Exception.Message.Split("`n")[0])
    }
}
finally {
    $captured = Join-Path $log_temp 'stock_lastlife_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        # Breakpoint resolution is printed with the markers on purpose: a
        # `Breakpoint N at 0x0 ... (M locations)` line means the body was
        # inlined into its callers and a zero hit count on it proves nothing.
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(TRACE|Breakpoint )' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        # A gdb "Target disconnected" is ambiguous between a guest crash, a
        # stub fault and an emulator exit. Say which one happened rather than
        # leaving the next run to rediscover it (CLAUDE.OPUS.md: a timeout was
        # a crash).
        $emulator.Refresh()
        if ($emulator.HasExited) {
            Write-Output ("TRACE EMULATOR exited=1 code=" + $emulator.ExitCode)
        } else {
            Write-Output 'TRACE EMULATOR exited=0'
        }
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
