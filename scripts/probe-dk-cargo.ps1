[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Build,
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    # Wall-clock ceiling. MEASURED 2026-08-25: the forced pickup landed at
    # source frame 1483 of a 3600-frame match, and the whole run took a little
    # under twenty minutes -- a per-frame gdb hook that walks four fighters is
    # roughly twenty remote memory packets a frame. 900 s is not enough; 1200 s
    # covered the match with the heartbeat reaching frame 3600.
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 1200,
    [string]$Artifact = ''
)

# P2-3r10: DRIVE DONKEY KONG'S CARGO LADDER, do not wait for a CPU to choose it.
#
# The cargo ladder is only entered when DK's grab ends in a FORWARD throw
# (ftcommonthrow.c:35-48). A CPU in CatchWait smashes the stick toward the stage
# centre (ftcomputer.c:6026-6035), so whether that resolves to ThrowF or ThrowB
# depends on where the two fighters happen to be standing. Two whole-match
# traces on 2026-08-25 produced nine grabs between them and not one DK cargo
# pickup -- which is why this probe exists rather than a longer run.
#
# Two levers, both of them values the game itself sets, applied from a
# once-per-battle-frame seam (`ifCommonBattleUpdateInterfaceAll`):
#
#   1. `fp->level = 9` on every fighter. A level-3 CPU grabs a handful of times
#      a minute and connects far less often than that; level 9 is what
#      ftcomputer.c reads out of the same field, so this buys grab ATTEMPTS
#      without touching any grab code.
#   2. `status_vars.common.catchwait.throw_wait = 0` on a Donkey Kong who is
#      already in CatchWait. `ftCommonThrowCheckInterruptCatchWait`
#      (ftcommonthrow.c:110-128) takes `is_throwf = TRUE` when that counter has
#      run out, i.e. when the grab has been held past its window, so the very
#      next release is the forward throw -- through the game's own branch.
#
# Nothing else is written, and every read is a global or a pointer derived from
# one -- `gGCCommonLinks[nGCCommonLinkIDFighter]` walked by `link_next` --
# because stack locals and stack objects are not trustworthy through melonDS's
# gdb stub (CLAUDE.OPUS.md). The probe never calls a guest function.
#
# The frame hook prints a heartbeat with Donkey's live status, so "no cargo"
# can be told apart from "the hook never ran": a counter that cannot express
# the case under test is worse than no counter (P2-3r11).

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

# BattleShip constants, restated so the probe does not depend on gdb resolving
# an enum name out of the right compilation unit.
$FTKIND_DONKEY = 2                  # ftdef.h nFTKindDonkey
$STATUS_CATCHWAIT = 168             # ftcommonstatus.h status 168 (0xA8)
$LINK_ID_FIGHTER = 3                # sys/objdef.h nGCCommonLinkIDFighter

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_dk-cargo-driven.txt')
}

# The driver seam, and then the ladder this probe exists to observe. Every name
# is nm-verified: a missing one is a configuration error (NDS_P2_DONKEY=0 or a
# non-DK descriptor), not something to discover as a silent zero later.
$driverSymbol = 'ifCommonBattleUpdateInterfaceAll'
$ladderSymbols = @(
    'ndsBaseFTCommonCatchSetStatus',
    'ndsBaseFTCommonCatchWaitSetStatus',
    'ndsBaseFTCommonThrowSetStatus',
    'ftCommonCaptureShoulderedSetStatus',
    'ftDonkeyThrowFWaitSetStatus',
    'ftDonkeyThrowFWalkProcInterrupt',
    'ftDonkeyThrowFTurnProcUpdate',
    'ftDonkeyThrowFKneeBendProcUpdate',
    'ftDonkeyThrowFFallSetStatus',
    'ftDonkeyThrowFLandingProcUpdate',
    'ftDonkeyThrowFDamageSetStatus',
    'ftDonkeyThrowFFSetStatus',
    'ftDonkeyThrowFFProcUpdate',
    'ndsBaseFTCommonCaptureApplyCatchKnockback'
)
$symbolTable = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
foreach ($name in (@($driverSymbol) + $ladderSymbols)) {
    if ($symbolTable -notcontains $name) {
        throw "probe-dk-cargo: symbol '$name' absent from $elf (is NDS_P2_DONKEY=1?)."
    }
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.dk-cargo.stdout.log'
$stderr = Join-Path $log_dir 'melonds.dk-cargo.stderr.log'
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
        'set $forced = 0',
        'set $frames = 0',
        'set $dkstatus = -1',
        'commands 1',
        'silent',
        'set $frames = $frames + 1',
        ('set $g = gGCCommonLinks[{0}]' -f $LINK_ID_FIGHTER),
        'set $n = 0',
        'while ($g != 0 && $n < 8)',
        # NOT `$fp`: gdb-markers.ps1 rejects convenience variables that shadow an
        # ARM register name, and $fp is one.
        '  set $fst = (FTStruct *)$g->user_data.p',
        '  if ($fst != 0)',
        '    set var $fst->level = 9',
        ('    if ($fst->fkind == {0})' -f $FTKIND_DONKEY),
        '      set $dkstatus = $fst->status_id',
        ('      if ($fst->status_id == {0})' -f $STATUS_CATCHWAIT),
        '        set var $fst->status_vars.common.catchwait.throw_wait = 0',
        '        set $forced = $forced + 1',
        '        printf "TRACE CARGOFORCE n=%d frame=%d\n", $forced, $frames',
        '      end',
        '    end',
        '  end',
        '  set $g = $g->link_next',
        '  set $n = $n + 1',
        'end',
        # The heartbeat exists so a null result is diagnosable: it distinguishes
        # "the hook never ran" from "DK never reached CatchWait".
        'if ($frames % 600 == 0)',
        '  printf "TRACE HEARTBEAT frame=%d dkstatus=%d forced=%d\n", $frames, $dkstatus, $forced',
        'end',
        'continue',
        'end'
    ))

    # 2. The ladder, one marker per transition.
    $index = 2
    foreach ($name in $ladderSymbols) {
        $commands.AddRange([string[]]@(
            ('break {0}' -f $name),
            ('set $h{0} = 0' -f $index),
            ('commands {0}' -f $index),
            'silent',
            ('set $h{0} = $h{0} + 1' -f $index),
            ('printf "TRACE {0} hit=%d\n", $h{1}' -f $name, $index),
            'continue',
            'end'
        ))
        $index++
    }

    $commands.Add('continue')
    $commands.Add('printf "TRACE STOP pc=0x%x lr=0x%x\n", $pc, $lr')
    $commands.AddRange([string[]]@('detach', 'quit'))

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
            -ScriptName 'dk_cargo_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    } catch {
        Write-Output ("dk-cargo probe ended: " + $_.Exception.Message.Split("`n")[0])
    }
}
finally {
    $captured = Join-Path $log_temp 'dk_cargo_probe.gdb.out'
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
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
