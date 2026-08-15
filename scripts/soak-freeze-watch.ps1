[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4619,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-r2-bothcpu',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [switch]$NoBuild,
    # NDS_R2_BOTH_CPU, passed to make explicitly. Defaults ON because the default
    # -Build IS the both-CPU stress ROM, and because the flag defaults to 0 in the
    # Makefile: until 2026-07-30 this script built `build-r2-bothcpu` WITHOUT
    # passing it, so the directory said both-CPU and the generated
    # nds_build_config.h said `NDS_R2_BOTH_CPU 0`. A soak run that way has a human
    # player on P1, never produces the CPU-vs-CPU tie, and therefore never enters
    # Sudden Death at all -- which is the only thing this ROM exists to reproduce
    # (docs/BUGS.md, Sudden Death row). Set -BothCpu:$false for a single-CPU soak.
    [bool]$BothCpu = $true,
    [ValidateRange(2, 120)][int]$PollSeconds = 10,
    # Owner, 2026-07-29: *"for a soak 5 mins tops, because I don't think the
    # results screen ever ends"*, then *"5 mins is too long for the stress ROM,
    # should be like 2.5 min"*. Both hold: 5 is the ceiling, 2.5 is the default,
    # and the default Build below IS the stress ROM. A longer run buys no coverage
    # -- the ROM reaches Results once and stays there, because mnVSResultsCheckExit
    # (decomp mnvsresults.c:266) exits on a START_BUTTON tap with no timeout. A
    # measured both-CPU match completes well inside 2.5 minutes: one full match
    # reported gNdsVSResultsStartCount=1 with 2,043 presented frames. Fractional
    # on purpose -- this was [int] and could not express the owner's number.
    # The 5.0 ceiling was raised to 7.0 on 2026-07-31 for one measured reason, and
    # the default stays 2.5. A rematch run has to observe TWO full matches plus the
    # second GAME SET hand-off: match one ends around t+170 s, the START tap lands
    # at t+167 s, and one game minute is ~136 s of wall clock, so match two's
    # match-end transition falls at roughly t+300 s -- exactly where the old cap
    # terminated the emulator. That produced two "NO-FREEZE" verdicts whose final
    # frame was the GAME SET zoom with the tick HUD already blanked, i.e. the run
    # ended AT the moment under investigation, which is not evidence either way
    # (the owner reported a freeze there both times). Keep runs short by default;
    # spend the extra two minutes only when the question is past match two.
    # The 7.0 ceiling was raised to 12.0 on 2026-08-13 for the R2-07 stress-gate
    # acceptance battery, which asks for THREE completed successive matches in
    # one emulator session ("pressing start at results screen restarts the P1
    # match, up to infinite successive matches", SwitchPlan 7). Match N ends
    # around t+170+140*(N-1) s, so three matches need ~460 s and four ~600 s,
    # and 7.0 cut the run off inside match three. The owner's "5 mins tops" is
    # about FREEZE soaks and still governs those; the default stays 2.5. Do not
    # spend more than the question needs -- this is the most expensive run in
    # the project (docs/VERIFYING.md, Run Economics).
    [ValidateRange(0.5, 12.0)][double]$MinutesToRun = 2.5,
    # THE MATCH TIMER, in game minutes, forwarded as NDS_R2_SOAK_MATCH_MINUTES.
    # -1 = auto (the default and the right answer almost always), 0 = leave the
    # harness seeding alone (the canonical one-minute Time match), 1..7 = force.
    #
    # This used to be seeded at 7 inside scene_harness.c's NDS_R2_BOTH_CPU
    # branch, so every both-CPU build got a 420-second match whether it was
    # soaking or being measured. It cost the campaign every both-CPU tick
    # figure: the gate arm sampled frames 440-2040 of a 7-minute match, which is
    # 12.6% coverage -- the opening minute -- while the identical window on
    # Boundary covers 86.7% and ends at the buzzer. Owner's ruling 2026-08-05:
    # "the soak was only meant to catch freezes, boundary and both cpu gates
    # should be the 60 sec match". The match length is now the soak's own knob
    # and no gate build sets it.
    #
    # AUTO IS DERIVED FROM -MinutesToRun RATHER THAN A CONSTANT, which is what
    # makes the original trap inexpressible: a match can no longer be shorter
    # than the run watching it. Two cases, and they want opposite things:
    #
    #   -PressStartSeconds 0 (a passive freeze soak) wants the match NEVER to
    #   end, because every second after it is a Results screen and proves
    #   nothing about gameplay. Ceiling(MinutesToRun) game minutes always
    #   outlasts the run: one game minute costs at least 60 s of wall clock and
    #   currently costs ~136 s, so this over-provisions and can never
    #   under-provision however much faster the ROM gets.
    #
    #   -PressStartSeconds > 0 (a rematch/Results/Sudden-Death soak) wants the
    #   match to END, or Results is never reached and START has nothing to
    #   dismiss. That case gets 0, the canonical one-minute match. The old
    #   hardcoded 7 silently broke this: at ~136 s per game minute a 7-minute
    #   match needs ~16 minutes of wall clock and the ceiling here is 7, so no
    #   both-CPU run could reach Results at all.
    [ValidateRange(-1, 7)][int]$MatchMinutes = -1,
    # Consecutive identical frames needed to call it frozen. Two screens of a DS
    # game in motion never render byte-identically, but legitimately static
    # moments exist -- and the longest is much longer than it sounds. This port's
    # scene transitions reload both fighters' asset sets by string path through
    # NitroFS, and the board measured that hand-off as roughly THIRTY SECONDS of
    # dead air with the last frame still on screen. At 4 samples x 10s the trip
    # threshold was 40s, barely past it, and a Sudden Death scene load duly
    # tripped as a freeze while the PC sat on a working `cmp` in the renderer.
    # 8 puts the threshold at 80s, safely clear of the measured dead air.
    [ValidateRange(2, 20)][int]$IdenticalFramesToTrip = 8,
    # Seconds after launch to tap START once. 0 disables. Results exits only on
    # START (mnVSResultsCheckExit, decomp mnvsresults.c:266) and only after
    # sMNVSResultsAllowExitWait -- 410 Results tics for a normal result, which at
    # the measured ~10.1 VBlanks/tic lands around 139 s from launch. Pick a value
    # past that or the tap is swallowed and the run proves nothing.
    #
    # KEEP REMATCH/SUDDEN-DEATH RUNS SHORT. Owner, 2026-07-30: these loops "go on
    # far longer than they need to". Measured timings for the canonical config:
    # Results is reachable around t+170 s, and the rematch fires on the first
    # press that wins the foreground race. Everything after roughly 30 s of match
    # two is repetition -- the corruption is visible immediately and the counters
    # are already latched. So the useful shape is
    #   -MinutesToRun 3.5 -PressStartSeconds 165 -PressStartCount 2
    # which is the floor: ~170 s is the match itself (one game minute runs ~136 s
    # of wall clock at the measured rate) and cannot be cut without changing the
    # match timer. Do not default to 5 minutes for these; 5 is the ceiling for a
    # freeze soak, not the setting for an input experiment.
    [ValidateRange(0, 300)][int]$PressStartSeconds = 0,
    # How many times to repeat that press, one per poll. See the comment at the
    # press site: a single synthetic press wins the foreground race only about
    # half the time.
    [ValidateRange(1, 20)][int]$PressStartCount = 6,
    # Re-arm the press burst every N seconds so one run drives SUCCESSIVE
    # rematches (owner's P1 standard: infinite rematches, no freeze). 0 keeps the
    # single-window behaviour. ~150 for the one-minute config -- see the press
    # site for why this is a cadence rather than a Results detection.
    [ValidateRange(0, 600)][int]$PressStartEverySeconds = 0,
    # PRESS ONLY WHEN THE PICTURE SAYS "Results", instead of on a wall clock.
    # Replaces -PressStartCount/-PressStartEverySeconds when set.
    #
    # WHY. The cadence form cost the R2-07 stress chain half its run on
    # 2026-08-13: every press that lands during a match PAUSES it (the port
    # implements START as pause), so a 3-press burst leaves the match paused on
    # odd parity, and the next burst is a match-length away. Measured: presses
    # at t+174/180 and t+328/334 left the game paused for 132 s each time --
    # 264 of 600 seconds -- and the run reached 2 matches instead of 4.
    #
    # THE DETECTOR is the bottom screen going STILL. On the tick-HUD ROM the HUD
    # redraws its digits every presented battle frame, so two consecutive polls
    # that hash the bottom half identically mean the battle scene is not up;
    # combined with a cooldown longer than one poll, a press can then only ever
    # land on Results. Measured signature on the same run: bottom-half
    # inter-poll change was 1.0-4.2% during battle and EXACTLY 0.0% on both
    # Results screens (`artifacts/performance/2026-08-13_c-stress/`).
    #
    # This is tick-HUD-specific by construction. On a ROM whose bottom screen is
    # not the HUD, use the cadence form.
    [switch]$PressStartOnResults,
    # Build with NDS_R2_SECOND_ENTRY_DIAG=1 so the per-caller allocation ledger
    # and the MObj chain probe exist. Required before the reported-globals list
    # below can name any gNdsAllocLedger* or gNdsR2ChainProbe* symbol: without
    # it they are absent, and an absent symbol reads as a deleted counter rather
    # than a flag that was never passed.
    [switch]$SecondEntryDiag,
    # Any other `NAME=VALUE` make flags this soak's build needs, e.g.
    # `-MakeFlags NDS_R2_PARTICLE_RUNTIME=1,NDS_R2_PARTICLE_DRAW=1`.
    #
    # This exists because -BothCpu and -SecondEntryDiag were the only two flags
    # this script forwarded, and a flag it does not forward is a flag the rebuild
    # SILENTLY CLEARS: every NDS_* knob is `?=` in the Makefile, so an omitted
    # flag reverts to 0 and the ROM the soak measures is not the ROM the
    # directory name promises. Each entry is verified against the generated
    # nds_build_config.h below, so asking for a flag the build did not take is an
    # error rather than a null result.
    [string[]]$MakeFlags = @(),
    # Keep every polled frame as a PNG in this directory, named by elapsed
    # seconds. Off by default (a 10-minute run at -PollSeconds 5 is ~120 files).
    #
    # WHY THIS EXISTS: the watch hashed the window and threw the pixels away, so
    # a FROZEN-PICTURE verdict could name neither the frame it froze on nor the
    # transition it froze at -- and a NO-FREEZE verdict could not show what the
    # rematch chain actually looked like. The R2-07 stress gate has to compare
    # match two against match one at GAME SET, Results and first battle frames,
    # and the capture is already being taken on every poll: this only writes it
    # down. The bitmap comes from PrintWindow (window, never desktop), so an
    # occluder cannot be photographed into the evidence.
    [string]$SaveFramesTo = '',
    [string]$JsonOut = ''
)

# Unattended freeze watch. R2-06's "soak clean" clause has had no instrument --
# the board says so in as many words -- and the owner reports "lots of freeze
# bugs that seem random", so this is the missing instrument rather than a
# one-off probe.
#
# HOW IT DETECTS, and why it is not GDB-based: melonDS's GDB stub serves exactly
# ONE session per emulation run. Measured 2026-07-29 -- after a clean `detach`
# the TCP listener is still bound and still accepts a connection, but the second
# session produces no output and the client hangs until its timeout. So a polled
# GDB watchdog cannot be built at all, and the breakpoint-driven alternative
# stops the core on every presented frame and runs the ROM roughly twelve times
# slower than real time, which changes the timing of anything race-shaped -- the
# worst possible property in a hunt for a random freeze.
#
# It therefore watches the screen, which is also how the owner sees a freeze:
# SHA-256 over the window pixels every PollSeconds. Frames in motion never hash
# equal; IdenticalFramesToTrip consecutive equal hashes is a frozen picture.
#
# The single GDB session is then spent where it is worth the most -- on the
# freeze itself, once. It captures the PC, a backtrace, the register file,
# REG_IME/IE/IF, GXSTAT and the IPC registers in one attach, because those are
# what separate a game-code loop from an interrupts-disabled VBlank wait from an
# audio/IPC handshake from a geometry-engine deadlock, and a random freeze that
# must be reproduced twice to be diagnosed will not be diagnosed.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')
. (Join-Path $PSScriptRoot 'lib\melonds-screenshot.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'

# Resolve -MatchMinutes before anything uses it: the build line, the config-header
# check and the end-of-run coverage verdict all read the SAME number, so a soak
# cannot build one match length and judge itself against another. See the
# parameter's own comment for why auto splits on the press schedule.
$resolvedMatchMinutes = $MatchMinutes
if ($resolvedMatchMinutes -lt 0) {
    $resolvedMatchMinutes = if ($PressStartSeconds -gt 0) {
        0
    } else {
        [math]::Min(7, [int][math]::Ceiling($MinutesToRun))
    }
}
Write-Host ("soak match timer: {0}" -f $(if ($resolvedMatchMinutes -gt 0) {
    "$resolvedMatchMinutes game minute(s) (NDS_R2_SOAK_MATCH_MINUTES)" }
    else { 'canonical 1-minute match (harness default, no override)' }))

# This script exposed a -NoBuild switch and never built anything: the switch only
# sets SMASH64DS_VERIFY_NO_BUILD, which OTHER verifiers read. So a soak silently
# ran whatever ROM happened to be on disk. Measured 2026-07-30: a source fix went
# in, `make` succeeded for the default target, the soak was started without
# -NoBuild, and it soaked a tickhud ROM from the previous evening -- a stale
# result that reads exactly like a real one, because a stale ROM boots and its
# picture moves. Only the missing counter symbols gave it away. Build here, like
# every census harness does, so the switch means what its name says.
if (-not $NoBuild) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    # Every flag the run depends on goes on the make line. A flag left off is a
    # flag the rebuild silently clears: capture-sudden-death-entry.ps1 had this
    # exact bug on 2026-07-31 and overwrote a hand-built diag ELF, which cost two
    # runs that printed nothing and read like an emulator hang.
    $makeArgs = @("TARGET=$Target", "BUILD=$Build",
                  "NDS_R2_BOTH_CPU=$([int][bool]$BothCpu)",
                  "NDS_R2_SOAK_MATCH_MINUTES=$resolvedMatchMinutes")
    if ($SecondEntryDiag) { $makeArgs += 'NDS_R2_SECOND_ENTRY_DIAG=1' }
    $makeArgs += $MakeFlags
    make -C $root @makeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
# Prove the ROM is the configuration that was asked for, rather than trusting the
# build directory's name. The generated header is the only thing that knows.
#
# RESOLVE THE BUILD DIRECTORY THE SAME WAY THE ROM PATH ABOVE DOES (2026-08-13).
# This was `Join-Path $root "$Build\nds_build_config.h"`, i.e. <root>\build-x,
# while every build directory in this repo lives under <root>\builds\ -- so the
# path never existed, `Test-Path` was always false, and EVERY guard in this block
# (both-CPU, match timer, second-entry diag, -MakeFlags) was skipped in silence
# on every soak ever run. That is the exact failure the block was written to
# prevent, one level up: a check that cannot fire reads identically to a check
# that passed.
$configHeader = Join-Path `
    (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
if (Test-Path -LiteralPath $configHeader -PathType Leaf) {
    $wantBothCpu = [int][bool]$BothCpu
    $seen = [regex]::Match(
        (Get-Content -LiteralPath $configHeader -Raw),
        '(?m)^#define\s+NDS_R2_BOTH_CPU\s+(\d+)')
    if ($seen.Success -and ([int]$seen.Groups[1].Value -ne $wantBothCpu)) {
        throw ("Soak ROM is NDS_R2_BOTH_CPU=$($seen.Groups[1].Value) but " +
               "-BothCpu asked for $wantBothCpu. A both-CPU soak that is not " +
               'both-CPU never creates the tie and never reaches Sudden Death, ' +
               'and it looks exactly like a clean run. Rebuild without -NoBuild.')
    }
    # Same rule for the match timer, and it is the one this check exists for
    # now: a soak whose ROM kept the canonical one-minute match spends its tail
    # on a Results screen and still reports NO-FREEZE, which is the false
    # negative that wasted two runs on 2026-08-02. -NoBuild runs are checked
    # too -- the header describes the ROM actually on disk, and -NoBuild is
    # exactly when it is easiest to soak a ROM built for something else. The
    # in-guest read at the end of the run is the real proof; this only catches
    # it before the minutes are spent.
    $soakSeen = [regex]::Match(
        (Get-Content -LiteralPath $configHeader -Raw),
        '(?m)^#define\s+NDS_R2_SOAK_MATCH_MINUTES\s+(\d+)')
    if ($soakSeen.Success -and
        ([int]$soakSeen.Groups[1].Value -ne $resolvedMatchMinutes)) {
        throw ("Soak ROM is NDS_R2_SOAK_MATCH_MINUTES=$($soakSeen.Groups[1].Value) " +
               "but this run resolved $resolvedMatchMinutes. The match timer and " +
               'the run length would disagree, so the run would watch a Results ' +
               'screen and call it NO-FREEZE. Rebuild without -NoBuild.')
    }
    # Same rule for the second-entry instruments. Asking for them and silently
    # getting a build without them turns every ledger/chain global into a
    # missing symbol, which reads as "the counter is gone" rather than "the flag
    # was never passed".
    $diagSeen = [regex]::Match(
        (Get-Content -LiteralPath $configHeader -Raw),
        '(?m)^#define\s+NDS_R2_SECOND_ENTRY_DIAG\s+(\d+)')
    if ($SecondEntryDiag -and
        (-not ($diagSeen.Success -and ([int]$diagSeen.Groups[1].Value -ne 0)))) {
        throw ('-SecondEntryDiag was requested but ' + $Build + ' is built ' +
               'with NDS_R2_SECOND_ENTRY_DIAG=0, so the allocation ledger and ' +
               'chain probe do not exist. Rebuild without -NoBuild.')
    }
    # And the same rule, generalised, for every -MakeFlags entry. -NoBuild runs
    # are checked too: the header describes whatever ROM is actually on disk, so
    # this is exactly the case where soaking the wrong build is easiest.
    $configText = Get-Content -LiteralPath $configHeader -Raw
    foreach ($flag in $MakeFlags) {
        $pair = [regex]::Match($flag, '^\s*([A-Za-z_][A-Za-z0-9_]*)=(.*)$')
        if (-not $pair.Success) {
            throw "-MakeFlags entry '$flag' is not NAME=VALUE."
        }
        $name = $pair.Groups[1].Value
        $want = $pair.Groups[2].Value.Trim()
        $got = [regex]::Match(
            $configText, ('(?m)^#define\s+' + [regex]::Escape($name) + '\s+(.+?)\s*$'))
        if (-not $got.Success) {
            throw ("-MakeFlags asked for $flag but $Build\nds_build_config.h " +
                   "does not define $name at all. Either the flag name is wrong " +
                   'or it is not one the config header records.')
        }
        if ($got.Groups[1].Value.Trim() -ne $want) {
            throw ("-MakeFlags asked for $flag but $Build is built with " +
                   "$name $($got.Groups[1].Value.Trim()). Soaking it would " +
                   'measure a different ROM than the one requested.')
        }
    }
}
foreach ($path in @($rom, $elf)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required soak file is missing: $path"
    }
}
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$logDir = Join-Path $root 'artifacts\verification\freeze-soak'
if ($SaveFramesTo) {
    [void](New-Item -ItemType Directory -Force -Path $SaveFramesTo)
    $SaveFramesTo = (Resolve-Path -LiteralPath $SaveFramesTo).Path
}

# The one GDB attach this run gets, wherever it is spent. Both the freeze capture
# and the clean-run counter read go through here.
#
# This function was CALLED before it existed. The clean-run read at the bottom of
# the script referenced Invoke-SoakGdb from the day it was written and nothing
# defined it, so every NO-FREEZE verdict this instrument has ever produced was
# pixels-only -- no match count, no arena size, no overflow latch -- and the
# failure was invisible because it lands after the verdict is already printed.
# Third defect of that exact shape in one day: a verification step was added and
# never confirmed to run. Returns the captured text, or $null if nothing landed;
# a timeout still returns whatever GDB flushed, because this attach cannot be
# retried (melonDS serves one stub session per emulation run).
function Invoke-SoakGdb {
    param(
        [Parameter(Mandatory=$true)][string]$Tag,
        [Parameter(Mandatory=$true)][string[]]$Commands,
        [int]$TimeoutSeconds = 120
    )

    $script = Join-Path $temp "soak-$Tag.gdb"
    $stdout = Join-Path $temp "soak-$Tag.out"
    $stderr = Join-Path $temp "soak-$Tag.err"

    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)") + $Commands + @('detach'))
    $process = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $process.Id -Force
        Write-Host "$Tag attach TIMED OUT -- the stub could not halt the core."
    }
    if (Test-Path -LiteralPath $stdout) {
        return Get-Content -LiteralPath $stdout -Raw
    }
    return $null
}

# DROP THE FIELDS THIS ELF DOES NOT DEFINE, by asking `nm`. The clean-run read
# puts every field into ONE printf, and gdb fails the whole command on the first
# unknown symbol -- so a single flag-scoped counter (the particle block, the
# ledger) silently costs all ~80 readings, which is exactly how a run once
# printed nothing and read as an emulator hang. A per-flag `if` covers only the
# flags somebody remembered; this covers all of them, and says what it dropped
# so an absent counter cannot be mistaken for a zero one.
#
# Shared with the freeze capture since 2026-07-31, which is when the particle
# counters were added there too. It was inline in the clean-run path before
# that, which is the reason the freeze path had none.
$script:SoakDefinedSyms = $null
function Select-SoakSymbols {
    param(
        [Parameter(Mandatory=$true, Position=0)][AllowEmptyCollection()][string[]]$Fields,
        [switch]$Announce
    )

    if ($null -eq $script:SoakDefinedSyms) {
        $script:SoakDefinedSyms = @{}
        $nmTool = Join-Path (Split-Path -Parent $Gdb) 'arm-none-eabi-nm.exe'
        if (Test-Path -LiteralPath $nmTool -PathType Leaf) {
            foreach ($entry in (& $nmTool --defined-only $elf)) {
                $parts = ($entry -split '\s+')
                if ($parts.Count -ge 3) { $script:SoakDefinedSyms[$parts[2]] = $true }
            }
        }
    }
    if ($script:SoakDefinedSyms.Count -eq 0) {
        return $Fields
    }
    $dropped = @($Fields | Where-Object {
        $base = ($_ -split '[\.\[]')[0]
        ($base -match '^[gs]Nds') -and (-not $script:SoakDefinedSyms.ContainsKey($base))
    })
    if (($dropped.Count -gt 0) -and $Announce) {
        Write-Host ('  note: this ROM does not define ' + $dropped.Count +
                    ' counter(s); dropped ' + ($dropped -join ', '))
    }
    return @($Fields | Where-Object { $dropped -notcontains $_ })
}

$configState = $null
$emulator = $null
$samples = @()
$verdict = 'RUNNING'
$diagnosis = $null
$capture = $null
try {
    # The GDB stub is enabled but deliberately unused unless a freeze trips, so
    # the one available session is still free at the moment it is needed.
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    # WindowStyle: visible-by-design -- the window IS the instrument here. A
    # hidden melonDS has no MainWindowHandle and no desktop pixels, so the frame
    # hash would be constant and every run would report a freeze.
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'soak.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'soak.melonds.err') `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $emulator.Refresh()
    $window = $emulator.MainWindowHandle
    if ($window -eq [IntPtr]::Zero) {
        throw 'melonDS opened no main window, so the screen cannot be watched.'
    }
    Write-Host ("soak: {0} [{1}] slot {2}, hashing the window every {3}s for {4}m; trip at {5} identical frames" -f `
        [System.IO.Path]::GetFileName($rom), $Build, $RunnerSlot, $PollSeconds,
        $MinutesToRun, $IdenticalFramesToTrip)

    $started = Get-Date
    $deadline = $started.AddMinutes($MinutesToRun)
    $previousHash = $null
    $identical = 0
    $distinct = 0
    # Repeat the press. SetForegroundWindow is refused whenever another process
    # owns the foreground, so a single synthetic press is unreliable: measured
    # 2026-07-30, two identical runs gave gNdsVSResultsPadMask 0x1000 and then 0.
    # Repeating on the poll cadence makes at least one land without needing the
    # focus race to be won on the first try. A deterministic alternative exists
    # (the controller playback path in `src/port/controller_backend.c`, already
    # driven by verify-battle-mariofox-gcrunall-loop-harness.ps1) and is the
    # right instrument if this ever needs to be exact rather than eventual.
    $startPresses = 0
    $pressWindow = 1
    $lastPressAt = $started
    $previousBottomHash = $null
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds $PollSeconds
        # The Results detector, when -PressStartOnResults is set. Hashed FIRST so
        # the press below decides on this poll's picture rather than on a clock.
        $bottomHash = if ($PressStartOnResults) {
            Get-MelonDSWindowFrameHash -WindowHandle $window -Half Bottom
        } else { $null }
        $atResults = $PressStartOnResults -and ($null -ne $previousBottomHash) -and
                     ($bottomHash -eq $previousBottomHash)
        $previousBottomHash = $bottomHash
        # R2-07: the Results screen exits on a START tap and nothing else, so a
        # passive soak can never reach match two -- which is exactly the state the
        # rematch redirect has to be tested in. One timed press turns this into a
        # two-match soak. ENTER is melonDS's START; the window must be foregrounded
        # first or SendKeys goes to whatever else has focus.
        # A second (third, Nth) press window. The owner's P1 standard is *infinite*
        # rematches without a freeze, so one press proving one rematch is not the
        # test -- and the initial burst cannot reach Results #2, which lands about
        # a match length later. -PressStartEverySeconds re-arms the burst on that
        # cadence, so one run walks several match entries. It is a CADENCE, not a
        # detection: a press that lands mid-match is either swallowed or pauses,
        # so pick the interval from the measured match length (~150 s for the
        # one-minute config, match end to match end) rather than something small.
        if ((-not $PressStartOnResults) -and ($PressStartEverySeconds -gt 0) -and
            ($startPresses -ge $PressStartCount) -and
            ((Get-Date) -ge $lastPressAt.AddSeconds($PressStartEverySeconds))) {
            $startPresses = 0
            $pressWindow++
        }
        # DETECTED, NOT TIMED. One press per detected Results screen, with a
        # cooldown longer than the poll so the second press of a burst cannot
        # land in the match that press just started -- which is the whole defect
        # this replaces. See -PressStartOnResults for the measurement.
        $pressNow = if ($PressStartOnResults) {
            $atResults -and ((Get-Date) -ge $lastPressAt.AddSeconds(
                [Math]::Max(20, $PollSeconds * 3))) -and
                ((Get-Date) -ge $started.AddSeconds($PressStartSeconds))
        } else {
            ($PressStartSeconds -gt 0) -and ($startPresses -lt $PressStartCount) -and
                ((Get-Date) -ge $started.AddSeconds($PressStartSeconds))
        }
        if ($pressNow) {
            # HOLD the key, do not tap it. SendKeys presses and releases within
            # milliseconds; the Results screen renders at roughly 6 FPS, so a tap
            # that short almost never falls inside a guest input sample and
            # `button_tap` never sees the edge. Measured: a SendKeys ENTER at
            # t+151s left gNdsVSResultsRematchCount at 0 with Results still
            # ticking. keybd_event with a real down/up pair spans several guest
            # frames, which is what a player actually does.
            [void][Smash64DSWindowCapture]::SetForegroundWindow($window)
            Start-Sleep -Milliseconds 400
            [Smash64DSWindowCapture]::keybd_event(0x0D, 0, 0, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 500
            [Smash64DSWindowCapture]::keybd_event(0x0D, 0, 2, [UIntPtr]::Zero)
            $startPresses++
            $lastPressAt = Get-Date
            if ($PressStartOnResults) {
                # Parenthesise the WHOLE concatenation before -f; without the
                # outer parens the operator binds to the last literal only and
                # the braces reach the log verbatim. Fourth recurrence of that
                # precedence bug in this file.
                Write-Host ((("  t+{0,5}s  held START (ENTER) 500 ms  [press " +
                    "{1}] on a detected Results screen") -f
                    [int]((Get-Date) - $started).TotalSeconds, $startPresses))
            } else {
                Write-Host ("  t+{0,5}s  held START (ENTER) 500 ms  [{1}/{2}] window {3}" -f
                    [int]((Get-Date) - $started).TotalSeconds, $startPresses,
                    $PressStartCount, $pressWindow)
            }
        }
        $emulator.Refresh()
        if ($emulator.HasExited) {
            $verdict = 'EMULATOR-EXITED'
            $diagnosis = "melonDS exited with code $($emulator.ExitCode)"
            break
        }
        # TOP HALF, i.e. the GAME picture. The whole-client hash cannot see a
        # stopped game on the measuring ROM: its bottom screen is the tick HUD
        # and those digits change every presented frame. Measured 2026-08-13 --
        # 264 s of a 600 s chain run were spent PAUSED, top screen pixel-frozen
        # for 132 s twice, and every poll still hashed "distinct". See the
        # -Half comment in lib\melonds-screenshot.ps1.
        $hash = Get-MelonDSWindowFrameHash -WindowHandle $window -Half Top
        $elapsed = [int]((Get-Date) - $started).TotalSeconds
        if ($SaveFramesTo) {
            # Named by elapsed seconds AND by the hash prefix the sample row
            # carries, so a row in the JSON and a file on disk are the same
            # frame without anyone counting entries.
            $framePath = Join-Path $SaveFramesTo (
                'f{0:d5}s-{1}.png' -f $elapsed, $hash.Substring(0, 8))
            [void](Save-MelonDSWindowCapture -WindowHandle $window `
                -Path $framePath -PreferPrintWindow)
        }
        if ($hash -eq $previousHash) {
            $identical++
        } else {
            if ($identical -gt 0) {
                Write-Host ("  t+{0,5}s  moving again after {1} identical frame(s)" -f $elapsed, $identical)
            }
            $identical = 0
            $distinct++
        }
        $samples += [pscustomobject]@{
            elapsedSeconds = $elapsed
            frameHash = $hash.Substring(0, 16)
            identicalRun = $identical
        }
        $previousHash = $hash
        if ($identical -ge ($IdenticalFramesToTrip - 1)) {
            # Never having seen the picture MOVE is either a broken capture or a
            # ROM that died before it animated anything -- and since the hash went
            # chrome-free the second is the common case, so decide it from pixels
            # instead of assuming. A uniform grab is the instrument's fault; a
            # detailed one that never changes is the ROM's.
            if ($distinct -le 1) {
                $colors = Measure-MelonDSWindowDistinctColors -WindowHandle $window
                if ($colors -le 2) {
                    $verdict = 'CAPTURE-STATIC'
                    $diagnosis = ("the guest area holds only $colors distinct " +
                        'colour(s), so the capture is suspect rather than the ROM')
                } else {
                    $verdict = 'FROZEN-FROM-START'
                    $diagnosis = ("the picture never changed once across " +
                        "$($identical + 1) samples yet holds $colors distinct " +
                        'colours, so the ROM drew a frame and then stopped')
                }
            } else {
                $verdict = 'FROZEN-PICTURE'
                # Parenthesise the WHOLE concatenation before -f. Without the
                # outer parens the operator binds to the last string literal
                # only, so {0} and {1} reach the artifact as literal braces --
                # which is exactly what 2026-07-31_012201 recorded. Third
                # recurrence of this precedence bug in this campaign.
                $diagnosis = (("{0} consecutive identical window hashes over " +
                    "{1}s, after {2} distinct frames") -f ($identical + 1),
                    (($identical + 1) * $PollSeconds), $distinct)
            }
            break
        }
        if (($distinct % 6) -eq 0) {
            Write-Host ("  t+{0,5}s  alive, {1} distinct frames so far" -f $elapsed, $distinct)
        }
    }
    if ($verdict -eq 'RUNNING') { $verdict = 'NO-FREEZE' }
    Write-Host ''
    Write-Host "verdict: $verdict$(if ($diagnosis) { " -- $diagnosis" })"

    # A clean run leaves the one GDB session unspent, so spend it here. Without
    # this, "N minutes with a changing picture" is all the soak can claim -- and
    # that is much weaker than it sounds, because a ROM parked on the animating
    # results screen also shows changing frames. gNdsVSResultsStartCount ticks
    # once per results-scene start, so it is the match count, and it is the number
    # that decides whether a soak exercised match teardown and rematch at all or
    # simply watched one match's aftermath for twenty-five minutes.
    if ($verdict -eq 'NO-FREEZE') {
        $cleanFields = @(
            'sVBlankCount',
            'gNdsBattlePlayablePacingPresentedFrames',
            'dSYTaskmanUpdateCount',
            'gNdsVSResultsStartCount',
            'gNdsVSResultsTickCount',
            'gNdsSyMallocOverflowCount',
            # A clean run still has to prove it did not fall off the arena-search
            # cliff: below 0x130000 = 1245184 the build has quietly lost ~237 KB
            # of game heap AND the Task 36 replay path, and a soak that only
            # watched pixels would call that healthy.
            'gNdsTaskmanArenaChosenSize',
            'gNdsTaskmanArenaAllocFailCount',
            # Minutes, seeded to 1 at scene_harness.c:182. Read so the soak can
            # say how much of its own runtime was actually gameplay.
            'gSCManagerTransferBattleState.time_limit',
            'gNdsR2AnimCacheArenaUsedBytes',
            'gNdsR2AnimCacheArenaOverflows',
            'gNdsR2AnimCacheFills',
            'gNdsR2AnimCacheHits',
            'gNdsR2AnimCacheRejects',
            # Slice 1 phase 5. State must read 1 (READY) -- 2/3/4 mean the linked
            # blob failed its magic/version/extent check, which is a build
            # mismatch, not a runtime fault. Hits are the acquisitions the
            # resident pack served; Misses are the fighter-animation acquisitions
            # that still fell through to the generic loader, and that is the
            # NEGATIVE CONTROL: while only one fighter is resident, Misses MUST
            # stay non-zero, and when both are resident it is the K0 after-GO
            # zero-I/O assertion's own counter.
            'gNdsBattlePackState',
            'gNdsBattlePackClips',
            'gNdsBattlePackBytes',
            'gNdsBattlePackHits',
            'gNdsBattlePackMisses',
            # Residency, which Hits alone cannot separate from "nobody asked".
            # LoadSteps counts the streamed 16 KB chunks; ResidentBytes is the
            # adopted extent; LoadFails is non-zero only when the blob never
            # arrived; Drops counts the scene rewinds that reclaimed it.
            'gNdsBattlePackLoadSteps',
            'gNdsBattlePackResidentBytes',
            'gNdsBattlePackLoadFails',
            'gNdsBattlePackDrops',
            # SLICE 1 PHASE 7 -- the K0 after-GO zero-I/O assertion, per fighter.
            # Index [0] is Mario, [1] is Fox, and only one of the two is resident
            # in the pack, so THE CONTROL IS IN THE SAME RUN: the unpacked
            # fighter must read NON-ZERO on every row where the packed one reads
            # zero. Both zero means the assertion measured nothing. Each counter
            # is incremented at its own site (see include/nds/nds_reloc_assets.h)
            # rather than inferred from BattlePackHits, because a zero one level
            # downstream of a rejected request reads exactly like a deletion.
            # Whole-run totals read at the end-of-run stop, which is the shape
            # the 2026-08-15 D-cache staleness finding explicitly excluded.
            'gNdsK0AfterGoAcquisitions[0]', 'gNdsK0AfterGoAcquisitions[1]',
            'gNdsK0AfterGoPackHits[0]', 'gNdsK0AfterGoPackHits[1]',
            'gNdsK0AfterGoFatReads[0]', 'gNdsK0AfterGoFatReads[1]',
            'gNdsK0AfterGoSeeks[0]', 'gNdsK0AfterGoSeeks[1]',
            'gNdsK0AfterGoByteSwaps[0]', 'gNdsK0AfterGoByteSwaps[1]',
            'gNdsK0AfterGoRelocs[0]', 'gNdsK0AfterGoRelocs[1]',
            'gNdsK0AfterGoNormalizes[0]', 'gNdsK0AfterGoNormalizes[1]',
            'gNdsK0AfterGoCacheCopies[0]', 'gNdsK0AfterGoCacheCopies[1]',
            'gNdsK0AfterGoTokenResolves[0]', 'gNdsK0AfterGoTokenResolves[1]',
            'gNdsK0AfterGoPathLookups[0]', 'gNdsK0AfterGoPathLookups[1]',
            # Overflows alone CANNOT say why the cache stopped absorbing, and on
            # 2026-08-02 that cost a wrong reading: Overflows 126 beside
            # UsedBytes 3728 was read as "the 92,160-byte arena filled", which
            # 3,728 bytes obviously did not. The arena has three distinct ways to
            # stop working -- never reserved, reserved then dropped by a scene
            # rewind, or genuinely full -- and the reject count is identical in
            # all three. These six separate them, they were already compiled into
            # the ROM, and only this list was missing them. Reserve/ReserveFail is
            # the "did it ever engage" pair; GenerationMismatches is the ORDINARY
            # second-entry drop; RangeFaults is not ordinary and means the block
            # left the live region without a rewind. ReservedBytes is 0 whenever
            # the cache is running unbacked, which is the state that sends every
            # animation load to the shared heap and refills the freeze class.
            'gNdsR2AnimCacheArenaReserveCount',
            'gNdsR2AnimCacheArenaReserveFailCount',
            'gNdsR2AnimCacheArenaReservedBytes',
            'gNdsR2AnimCacheArenaGenerationMismatches',
            'gNdsR2AnimCacheArenaRangeFaults',
            'gNdsR2AnimCacheArenaInvalidations',
            # The arithmetic behind the last refusal. Overflows beside
            # ReservedBytes cannot say whether the arena was full or the request
            # was absurd; these two can.
            'gNdsR2AnimCacheArenaOverflowLastSize',
            'gNdsR2AnimCacheArenaOverflowLastUsed',
            # The VS battle scene arena re-budget (battleship_scvsbattle.c).
            # 0 here means the graphics heap is back at the N64's 0xD000 and the
            # shield freeze is armed again; the byte figure is the engagement
            # proof that the setup struct the scene was actually built from
            # carried the new value.
            'gNdsSCVSBattleRebudgetCount',
            'gNdsSCVSBattleRebudgetGraphicsBytes',
            # Per-particle alpha engagement. Non-zero means particles are
            # fading; 0 with particles drawing means every live particle shared
            # one alpha, which is what the dropped-alpha bug looked like.
            'gNdsParticleQuadAlphaBreaks',
            # R2-07 R1. The Battle -> Results hand-off is the visible half of the
            # owner's complaint -- ~30 s of dead air with the last battle frame
            # still on screen -- and this soak already reaches Results exactly
            # once, so it is the cheapest place to price it. FuncStart is the whole
            # task-start; SetupFiles is the fighter-asset half inside it, so the
            # difference is the scene's own file list plus construction.
            # ...and the enclosing span, because FuncStart turned out to be only
            # 21,851,904 ticks (0.65 s) of a hand-off recorded at ~30 s. Battle
            # taskman exit to the first Results tick, so it is the dead air itself.
            'gNdsVSResultsTransitionTicks',
            'gNdsVSResultsFuncStartTicks',
            'gNdsVSResultsSetupFilesTicks',
            'gNdsVSResultsSetupFilesCalls',
            # The reveal, which is the dead air as the player experiences it. The
            # source holds the wallpaper to Results tic 80 and the panels to tic
            # 120, so these are per-frame cost x 80 and x 120 -- divide by
            # 33,514,000 for seconds.
            'gNdsVSResultsToWallpaperTicks',
            'gNdsVSResultsToResultsTicks',
            # R2-07: START on Results restarts the match. Non-zero proves the
            # redirect fired, which is the only way a soak reaches match two --
            # a passive one still cannot, because nothing presses START.
            'gNdsVSResultsRematchCount',
            # Sticky input evidence. SeenMask 0 after a press means the key never
            # reached the guest; SeenMask non-zero with TapMask 0 means the edge
            # is wrong. START_BUTTON is 0x1000.
            'gNdsVSResultsInputPollCount',
            'gNdsVSResultsPadMask',
            'gNdsVSResultsInputSeenMask',
            'gNdsVSResultsInputTapMask',
            # The publish interlock. Suppressed counts the second
            # syControllerUpdateGlobalData of a single read -- each of those used
            # to overwrite a live button_tap with zero. EdgeSeenMask non-zero
            # with PublishedTapMask zero would mean the edge is computed and then
            # lost after the publish, which is a different defect again.
            # All seven are reset on the first Results tick, so they are Results
            # only and divide by gNdsVSResultsTickCount. Run-global versions of
            # these lied once already: a soak presses START on a wall-clock
            # schedule, so early presses land during the battle.
            'gNdsControllerReadCount',
            'gNdsControllerReadEdgeCount',
            'gNdsControllerPublishCount',
            'gNdsControllerPublishSuppressedCount',
            'gNdsControllerPublishTapNonzeroCount',
            'gNdsControllerEdgeSeenMask',
            'gNdsControllerPublishedTapMask',
            # Second-entry preparation. A rematch and a Sudden Death are both
            # re-entries into the battle scene, and both were drawing against
            # resources the scene load had torn down. PrepareCount must rise once
            # per battle entry; a non-zero ViolationCount means the texture cache
            # was discarded while still marked prepared.
            'gNdsRendererBattleStaticTexturePrepareCount',
            'gNdsRendererBattleStaticTextureViolationCount',
            'gNdsSCVSBattleSuddenDeathPrepareCount',
            # The scene-owned texture-VRAM reset, which is the second-entry
            # corruption's fix and therefore the cheapest regression guard the
            # rematch lane has: it must read exactly one per battle-scene entry
            # (2 after one rematch, 3 after two). A frozen count means a new
            # entry path reached the scene without the reset and the stage is
            # about to be drawn against the previous match's allocator state.
            'gNdsRendererSceneTextureVramResetCount',
            # The GAME SET announcement, end to end, with no new code needed.
            # PlacementInitCount must read one per battle entry: it calls the
            # source's own ifCommonBattleInitPlacement, which sets
            # sIFCommonBattlePlace = teams - 1. Nothing called it before
            # 2026-07-31, so the counter sat at its .bss zero and the match-end
            # test `--sIFCommonBattlePlace == 0` (ifcommon.c:2735-2740) could
            # never fire -- which is why no VS match of any length has ever
            # announced GAME SET. After a decisive match sIFCommonBattlePlace
            # should read 0: that IS the announcement having been triggered.
            'gNdsSCVSBattlePlacementInitCount',
            'sIFCommonBattlePlace',
            # Did the rematch actually re-enter the battle scene through the
            # scene manager's dispatch loop? AdapterCount rises once per
            # scManagerFuncUpdate, so 2 means scVSBattleStartScene ran twice and
            # syTaskmanStartTask rewound the general heap for match two. GObjCount
            # and MallocCount separate "rewound and refilled" from "piled on top".
            # Non-zero proves a same-kind scene re-entry was caught as stale.
            'gNdsRelocSceneReentryEvictCount',
            # Non-zero proves the native OAM path's retained texture names were
            # dropped with the VRAM behind them. Must rise once per scene change.
            'gNdsIFCommonNativeOamTextureDiscardCount',
            # STAGE FAST-PATH ENGAGEMENT, and the reason it is read on a rematch
            # soak specifically. `STG` is 2.21x on the second entry, and one
            # explanation is that the stage's direct path stops engaging there
            # and the generic renderer takes over -- which would raise the cost
            # AND change what is drawn, unlike a clean double-draw. These are the
            # counters R2-02 E1a added for exactly this question; their own
            # comment records that Task 52 once found the Task 36 replay
            # structurally disabled and silently indistinguishable from a null
            # result. Reuse should dominate Build; Build climbing across the
            # second entry is the fast path dropping out.
            'gNdsR2StagePrepareReuseCount',
            'gNdsR2StagePrepareBuildCount',
            'gNdsR2StagePreflightElideCount',
            # THE CLIFF, on every run rather than only on the one that fell off
            # it. ifCommonSetMaxNumGObj caps the GObj pool the moment the
            # general heap drops under 25,600 bytes free, and past that cap the
            # GO countdown dereferences a NULL. Measured 2026-08-01: the
            # particle runtime alone leaves 26,776 free -- 1,176 bytes of
            # margin -- and adding the quad draw's 3,008 bytes of .text took it
            # to 23,032 and killed the battle. COMMONSMAX is -1 while the cap
            # has never fired; any other value means it HAS, and the run after
            # it is living on borrowed GObjs. Read the margin, not the verdict.
            'sGCCommonsMaxNum',
            'sGCCommonsActiveNum',
            # R2-07 clause 2, present only with NDS_R2_PARTICLE_RUNTIME=1 (the
            # nm filter below drops them otherwise). Load result and reject count
            # say the bank is sound; Live/Max are the pool high-water, which is
            # the no-gameplay-allocation contract (SwitchPlan 3.11) made visible;
            # DrawSeamCount is "the interpreter reached the draw seam this frame"
            # and stays non-zero even while the DS quad path is unbuilt, so it is
            # the difference between "no effects ran" and "effects ran, nothing
            # drew".
            'gNdsParticleBankLoadResult',
            'gNdsParticleBankScriptsUnpacked',
            'gNdsParticleBankScriptsRejected',
            'gNdsParticleScriptStartCount',
            'gNdsParticleGeneratorStartCount',
            'gNdsParticleRejectCount',
            'gNdsParticleDrawSeamCount',
            'gNdsParticleStructsLive',
            'gNdsParticleStructsMax',
            'gNdsParticleGeneratorsLive',
            'gNdsParticleGeneratorsMax',
            'gNdsParticleTransformsMax',
            'gNdsParticleRootSpawnCount',
            # WHICH textures a real match draws, and how many quads a frame has
            # to carry. Both are VRAM/budget inputs the static reachability
            # cannot answer: the generator admits 31 textures and 137,152 bytes
            # against 119,872 free after the battle's pinned static set, so the
            # upload list has to come off a measurement rather than off the
            # over-approximation. The mask is one bit per SOURCE texture id.
            'gNdsParticleTextureUseMask[0]',
            'gNdsParticleTextureUseMask[1]',
            'gNdsParticleDrawVisibleCount',
            'gNdsParticleDrawVisibleMax',
            # The draw. EmitCount against VisibleCount is the fail-closed
            # margin, and MissCount names the gap: a particle whose texture is
            # not in the atlas draws NOTHING, so "no effects" and "wrong
            # effects" stay distinguishable. AtlasBytes proves the upload ran.
            'gNdsParticleQuadEmitCount',
            'gNdsParticleQuadEmitMax',
            'gNdsParticleQuadMissCount',
            'gNdsParticleQuadMissMask[0]',
            'gNdsParticleQuadMissMask[1]',
            'gNdsParticleQuadMissFrameMask',
            'gNdsParticleQuadStrideCount',
            'gNdsParticleBankEFCommonID',
            'gNdsParticleInitAllCount',
            'gNdsParticleBankRegisterCount',
            'gNdsRendererParticleAtlasPrepareCount',
            'gNdsRendererParticleAtlasFailCount',
            'gNdsRendererParticleAtlasBytes',
            # WHICH script was refused. Reason 4 means the pack marked the id
            # unreachable and failed closed -- a hole in the reachability
            # derivation, not a runtime bug. 1/2/3 mean the bank itself is
            # wrong. The bare count cannot tell those apart.
            'gNdsParticleRejectRingCount',
            'gNdsParticleRejectRingScripts[0]', 'gNdsParticleRejectRingBanks[0]',
            'gNdsParticleRejectRingReasons[0]', 'gNdsParticleRejectRingCounts[0]',
            'gNdsParticleRejectRingScripts[1]', 'gNdsParticleRejectRingBanks[1]',
            'gNdsParticleRejectRingReasons[1]', 'gNdsParticleRejectRingCounts[1]',
            'gNdsParticleRejectRingScripts[2]', 'gNdsParticleRejectRingBanks[2]',
            'gNdsParticleRejectRingReasons[2]', 'gNdsParticleRejectRingCounts[2]',
            'gNdsParticleRejectRingScripts[3]', 'gNdsParticleRejectRingBanks[3]',
            'gNdsParticleRejectRingReasons[3]', 'gNdsParticleRejectRingCounts[3]',
            'gNdsParticleRejectRingScripts[4]', 'gNdsParticleRejectRingBanks[4]',
            'gNdsParticleRejectRingReasons[4]', 'gNdsParticleRejectRingCounts[4]',
            'gNdsParticleRejectRingScripts[5]', 'gNdsParticleRejectRingBanks[5]',
            'gNdsParticleRejectRingReasons[5]', 'gNdsParticleRejectRingCounts[5]',
            # Dream Land's own bank. PupupuID 255 means it never registered;
            # ScriptsPacked 0 means it registered and normalized nothing, which
            # puts Whispy's leaves and dust straight back on the reject ring at
            # reason 2. Both were the state of the world until 2026-08-01.
            'gNdsParticleBankPupupuID',
            'gNdsParticlePupupuScriptsPacked',
            'gNdsRendererTask36ReplayArenaStaleCount',
            # WHY the native stage owner refused. Present at
            # NDS_R2_STAGE_ROUTE_PROBE=1 (or profile level 1), which is the only
            # way to tell a topology change from a texture-upload failure from a
            # binding rejection -- seven sites set these. Needed because
            # NDS_R2_PARTICLE_DRAW=1 turns StagePrepareBuildCount 2 -> 197 and
            # puts 196 of 566 frames at five or more VBlanks, one per rejection,
            # while its own tick cost is only ~10,000 in MISC.
            # These two are LATCHES, reset at the top of every prepare, so an
            # end-of-run read describes the last frame only -- both read 0 on a
            # run whose battle rebuilt 197 times. Kept because they name the
            # site when a stop lands inside the window; the counters below are
            # what answer "why 197".
            'gNdsRendererTask36RendererRejectReason',
            'gNdsRendererTask36PrepareRunRejectReason',
            'gNdsR2StageKeyMissInvalid',
            'gNdsR2StageKeyMissGeneration',
            'gNdsR2StageKeyMissStamp',
            'gNdsR2StageKeyMissConfig',
            'gNdsR2StageKeyMissAssets',
            # 1 policy mismatch, 2 stage source texture unresolved, 3 visit
            # range, 4 dense vertex index, 5 degenerate clip w, 6 alpha unset.
            'gNdsR2StageRejectCounts[1]',
            'gNdsR2StageRejectCounts[2]',
            'gNdsR2StageRejectCounts[3]',
            'gNdsR2StageRejectCounts[4]',
            'gNdsR2StageRejectCounts[5]',
            'gNdsR2StageRejectCounts[6]',
            # The LAST link in that chain, and it already existed: R2-07 E2
            # ungated the reason mask and a first-rejection cache census on
            # NDS_R2_STAGE_ROUTE_PROBE for exactly this question. TEXIMAGE in
            # the mask means the eviction retry ran out of things to evict,
            # which is two different failures with two different fixes --
            # texture VRAM full (bytes) or every slot pinned/touched-this-frame
            # (slots) -- and the census separates them.
            'gNdsRendererProfileTextureRejectReasonMask',
            'gNdsR2TexRejectCensusValid',
            'gNdsR2TexRejectCensusFree',
            'gNdsR2TexRejectCensusLive',
            'gNdsR2TexRejectCensusPinned',
            'gNdsR2TexRejectCensusThisFrame',
            'gNdsR2TexRejectCensusEvictable',
            # BUGS.md: "sometimes Mario's fireballs don't spawn". These two make
            # the report falsifiable without a new probe -- the import already
            # counts both sides of the call (battleship_mario_fireball.c).
            # SpawnCall == SpawnSuccess means every request produced a weapon and
            # the bug is upstream in the special-N state machine or the input;
            # SpawnCall > SpawnSuccess means the source's own make refused, which
            # is a weapon-pool or per-owner-limit question instead.
            'gNdsFighterProjectileProofSpawnCallCount',
            'gNdsFighterProjectileProofSpawnSuccessCount',
            'gNdsFighterProjectileProofWeaponCountMax',
            # WHICH NULL wpManagerMakeWeapon returned. GObj != 0 means
            # gcMakeGObjSPAfter refused; Pool != 0 means the 32-entry WPStruct
            # free list was empty. SpawnCall - SpawnSuccess is their sum.
            'gNdsFighterProjectileProofSpawnFailGObjCount',
            'gNdsFighterProjectileProofSpawnFailPoolCount',
            'gNdsFighterProjectileProofSpawnFailGObjMax',
            'gNdsFighterProjectileProofSpawnFailGObjActive',
            'gNdsFighterProjectileProofSpawnFailHeapFree',
            'gNdsWeaponStructBytes',
            'gNdsWeaponPoolEntries',
            # Under 25,600 means the GObj cap latched during THIS match. The
            # end-of-run GENERALFREE below reads the Results scene and cannot
            # see it.
            'gNdsTaskmanGeneralHeapFreeMin',
            'gNdsGCDrawsActiveMax',
            # NDS_R2_EFFECT_POOL, installed depth and battle low-water. Pinned
            # at 4 means cosmetic effects are being refused (the source's own
            # five-free cut) and the depth is too small; 0 means the forced
            # reserve the KO burst relies on is being consumed too.
            'gNdsEffectPoolDepth',
            'gNdsEffectPoolFreeMin',
            # The KO burst. Attempt == Complete means the DObj tree the source
            # walks unguarded came back whole every time; a non-zero drop mask
            # names the missing link (NDS_KO_BURST_DROP_* in nds_effects.h) and
            # is the difference between a skipped cosmetic and the null
            # dereference the owner reported as a freeze on 2026-08-01.
            # Non-zero proves the EFDesc offset resolver ran. Zero with source
            # effects live means descs are still holding symbol ADDRESSES, and
            # the next DObj-tree effect will walk garbage until the heap dies.
            'gNdsEFDescResolveCount',
            'gNdsEFDescDisabledCount',
            # Descs whose file the span table does not recognise, so nothing
            # bounds-checked their offsets. Silent before 2026-08-03, and it
            # covered the shield and Fox's reflector the whole time. Must be 0.
            'gNdsEFDescUnknownFileCount',
            'gNdsEFDescEffectsSpan[0]',
            'gNdsEFDescEffectsSpan[1]',
            'gNdsEFDescEffectsSpan[2]',
            'gNdsKOBurstAttemptCount',
            'gNdsKOBurstCompleteCount',
            'gNdsKOBurstDropMask',
            # BUGS.md "Star KO twinkle not playing in correct spot". Where the
            # sparkle was actually asked for, in whole units. The source spawns
            # it at the fighter's TopN joint, so these should track the fighter.
            'gNdsStarKOSparkleCount',
            'gNdsStarKOSparkleLastX',
            'gNdsStarKOSparkleLastY',
            'gNdsStarKOSparkleLastZ',
            # BUGS.md "Some Crowd noise audio cues get cut off". The only
            # remaining mechanism once the handle pool and the sample cache are
            # both shown to fail closed: a hardware channel reused while the cue
            # was still audible. Zero here clears channel contention.
            'gNdsAudioFgmPrematureRetireCount',
            'gNdsAudioFgmPrematureRetireLastID',
            'gNdsAudioFgmPoolExhaustCount',
            'gNdsAudioFgmGenerationMismatchCount',
            # LIVE DObjs beside the high-water. gcGetDObjSetNextAlloc grows the
            # pool out of the general heap and never shrinks it, so peak is what
            # costs -- but peak only means "simultaneous" if ejected DObjs go
            # back on sGCDrawHead. If live tracks max at END of run, where
            # almost no effect is on screen, they are LEAKING and the fix is one
            # missing free rather than a bounded-pool rewrite.
            'sGCDrawsActiveNum',
            # BUGS.md crowd row, present only at
            # NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1. "The crowd is silent" has three
            # distinct causes and the FGM backend's UnsupportedCallCount
            # separates only the last: the actor never ran (ActorMakeCount 0),
            # it ran and never decided (ProcUpdate > 0 with PlayCommon 0), or it
            # decided and the pack refused (PlayCommon > 0 with the id on the
            # miss ring). LastCommonFGM/LastCallFGM name which cue it picked.
            'gNdsFtPublicActorMakeCount',
            'gNdsFtPublicProcUpdateCount',
            'gNdsFtPublicCommonCheckCount',
            'gNdsFtPublicPlayCommonCount',
            'gNdsFtPublicLastCommonFGM',
            'gNdsFtPublicCallStartCount',
            'gNdsFtPublicLastCallFGM',
            # R2-07 L7 step one, present only at NDS_R2_COLLISION_L7_ORACLE=1.
            # MaxDevQ12[0..2] are the answer, in 1/4096 world units at probe
            # offsets of 1/4/16 units; the gate is 82 (0.0200 world units).
            # ScaleMin/Max settle which falsifier domain SSB64 actually visits
            # -- inside 0.90-1.10 (3686..4506) the kernel is already proven,
            # out at 0.25-2.00 (1024..8192) it is not.
            'gNdsR2CollisionOracleSamples',
            'gNdsR2CollisionOracleSingular',
            'gNdsR2CollisionOracleMaxDevQ12[0]',
            'gNdsR2CollisionOracleMaxDevQ12[1]',
            'gNdsR2CollisionOracleMaxDevQ12[2]',
            'gNdsR2CollisionOracleOverBoundCount',
            'gNdsR2CollisionOracleScaleMinQ12',
            'gNdsR2CollisionOracleScaleMaxQ12',
            # THE FGM ALLOWLIST, made visible on any natural run. A cue the game
            # asks for and the pack does not carry fails closed and is silent,
            # which is how five announcer lines went missing without a single
            # error: UnsupportedCallCount rises and nothing else does. Supported
            # vs Unsupported is the whole "is this cue packed yet" question, and
            # PlayFail separates "packed but the mixer refused" from it.
            'gNdsAudioFgmLoaded',
            'gNdsAudioFgmResult',
            'gNdsAudioFgmOpenFailCount',
            'gNdsAudioFgmReadFailCount',
            'gNdsAudioFgmFormatFailCount',
            'gNdsAudioFgmResidentBytes',
            'gNdsAudioFgmIncludedLookupFailCount',
            'gNdsAudioFgmSupportedCount',
            'gNdsAudioFgmPlayCalls',
            'gNdsAudioFgmSupportedPlayCount',
            'gNdsAudioFgmUnsupportedCallCount',
            'gNdsAudioFgmPlayFailCount',
            # WHY a cue stopped, which is the only thing that separates BUGS.md
            # "Some Crowd noise audio cues get cut off" from a cue that simply
            # ended. DurationStop is a normal end; GenerationMismatch is the
            # handle finding its channel reassigned under it, i.e. a steal, and
            # a crowd cue is the likeliest victim because PublicWin and
            # PublicExcited are the two longest in the pack at 950 and 1,200
            # ticks. PoolExhaust and MaxActiveHandles bound the eight-handle
            # pool that does the stealing. All unconditional in nds_audio_fgm.c,
            # so they exist in every build -- the rule below applies.
            'gNdsAudioFgmMaxActiveHandles',
            'gNdsAudioFgmPoolExhaustCount',
            'gNdsAudioFgmDurationStopCount',
            # BUGS.md crowd cut-off. DurationStop above counts every channel
            # retire; PrematureRetire counts only those taken while the previous
            # owner's own end_tick was still ahead -- a cue ended mid-sample.
            # This is the row's acceptance, and it needs a BOTH-CPU soak: a
            # scripted probe run reported both as 0, which cleared nothing
            # because DurationStop 0 means the retire path never ran at all.
            'gNdsAudioFgmPrematureRetireCount',
            'gNdsAudioFgmPrematureRetireLastID',
            # And the freeze guards: non-zero means the general heap ran out
            # during play and the ROM DECLINED instead of spinning in
            # syMallocSet. Before 2026-08-02 that was the shield freeze.
            'gNdsRelocHeapDeclineCount',
            # The other two freeze guards, and they were BOTH unreadable from
            # here until 2026-08-03. objman's nineteen converted spins landed
            # with a witness that this list never named, so a soak could not
            # say whether an object pool had run short; and the animation-script
            # parsers were still unbounded, which is where the three
            # 2026-08-02/03 captures actually stopped. Both counts must read 0.
            # ObjmanPanicMask names the pool; ObjAnimRunawayMask names the
            # parser (0/1 DObj, 2/3 MObj, 4/5 CObj, 6/7 figatree) and
            # Script/Opcode name the stream that faulted.
            'gNdsObjmanPanicCount',
            'gNdsObjmanPanicMask',
            'gNdsObjAnimRunawayCount',
            'gNdsObjAnimRunawayMask',
            'gNdsObjAnimRunawayScript',
            'gNdsObjAnimRunawayOpcode',
            # ndsRelocResolvePointerFromFileBase. Misalign > 0 is a stored word
            # refused BEFORE it could become an unwalkable animation script --
            # the engagement proof for that fix. Offset is its positive control:
            # 0 there means the resolver's fallback never ran at all, so a
            # Misalign of 0 says nothing either way.
            'gNdsRelocResolveOffsetCount',
            'gNdsRelocResolveMisalignCount',
            'gNdsRelocResolveMisalignValue',
            'gNdsAudioFgmReleaseRampCount',
            'gNdsAudioFgmGenerationMismatchCount',
            'gNdsAudioFgmStaleStopCount',
            'gNdsAudioFgmStopAllCalls',
            'gNdsAudioFgmMissRingCount',
            # WHICH cue was refused, not just how many. The count alone cannot
            # tell "the announcer is still missing" from "an unrelated menu cue
            # was asked for once", and that is the difference between a row
            # being closed and looking closed. Ring capacity is 16; the first
            # eight cover every count observed (max 6).
            'gNdsAudioFgmMissRingIDs[0]', 'gNdsAudioFgmMissRingCounts[0]',
            'gNdsAudioFgmMissRingIDs[1]', 'gNdsAudioFgmMissRingCounts[1]',
            'gNdsAudioFgmMissRingIDs[2]', 'gNdsAudioFgmMissRingCounts[2]',
            'gNdsAudioFgmMissRingIDs[3]', 'gNdsAudioFgmMissRingCounts[3]',
            'gNdsAudioFgmMissRingIDs[4]', 'gNdsAudioFgmMissRingCounts[4]',
            'gNdsAudioFgmMissRingIDs[5]', 'gNdsAudioFgmMissRingCounts[5]',
            'gNdsAudioFgmMissRingIDs[6]', 'gNdsAudioFgmMissRingCounts[6]',
            'gNdsAudioFgmMissRingIDs[7]', 'gNdsAudioFgmMissRingCounts[7]',
            # R2-07 R4b. Skip+Redraw = foreground layers built; a settled
            # Results scene should be nearly all Skip. Any Overflow means the
            # defer buffer is undersized and the memo silently disabled itself.
            'gNdsSObjLayerMemoSkipCount',
            'gNdsSObjLayerMemoRedrawCount',
            'gNdsSObjLayerMemoOverflowCount',
            # R2-07 Results cadence. The owner's acceptance metric for this
            # scene is P95 CPU TICKS (<= 1.12M), the same figure the battle is
            # gated on. Ticks are recovered from the interval distribution:
            # one VBlank is 560,190 ticks, so the P95 interval times that is
            # the P95 frame cost.
            #
            # Indices are enumerated rather than printing the array by name --
            # GDB prints a bare array symbol as a single scalar, which read as
            # a meaningless 36,158,572 on the first run and would have been
            # taken for data. Bins run to 15 (15+); 2..10 covers everything
            # observed so far (max 9) and the 15+ bin catches a blow-up.
            'gNdsVSResultsPresentIntervalBucket[2]',
            'gNdsVSResultsPresentIntervalBucket[3]',
            'gNdsVSResultsPresentIntervalBucket[4]',
            'gNdsVSResultsPresentIntervalBucket[5]',
            'gNdsVSResultsPresentIntervalBucket[6]',
            'gNdsVSResultsPresentIntervalBucket[7]',
            'gNdsVSResultsPresentIntervalBucket[8]',
            'gNdsVSResultsPresentIntervalBucket[9]',
            'gNdsVSResultsPresentIntervalBucket[10]',
            'gNdsVSResultsPresentIntervalBucket[15]',
            'gNdsVSResultsPresentIntervalMax',
            'gNdsVSResultsPresentIntervalSamples',
            'gNdsSCVSBattleLifecycleArenaAdapterCount',
            'gNdsSCVSBattleOriginalGObjCount',
            'gNdsTaskmanMallocCount',
            # Tick-HUD brackets, read while match two is running. The rematch
            # slowdown is sustained (~3x) rather than a rare excursion, so one
            # sample names the owning bracket against match one's known figures:
            # FTR 392,896, SRC 471,232, STG 177,088. Attribute before cutting --
            # this bug has already produced three plausible-but-wrong causes.
            'gNdsTickHudFighterTicks',
            'gNdsTickHudStageTicks',
            'gNdsTickHudSourceTicks',
            'gNdsTickHudForegroundTicks',
            'gNdsTickHudBackgroundTicks',
            'gNdsTickHudAudioTicks',
            'gNdsTickHudFlushTicks',
            'gNdsTickHudVBlankWaitTicks',
            # The source controller THREAD is live (syMainThread5 osStartThreads
            # syControllerThreadMain), and its read/publish use the raw decomp
            # symbols, so the port wrapper cannot see them. PollCount counts every
            # osContGetReadData including the thread's, so PollCount well above
            # ReadCount is the second pipeline. WaitUpdate non-NULL means the
            # thread's unguarded publish branch (controller.c:484) is reachable.
            'gNdsControllerPollCount',
            'sSYControllerWaitUpdate',
            # The gcRunAll bracket. gcRunAll IS the Results task_update, so it is
            # the span between the publish (leaves button_tap holding START) and
            # mnVSResultsCheckExit inside it (reads zero). EntryTapMask zero means
            # the tap is already gone before task_update begins and the publish
            # value never reached this side; Alive>0 with Lost==Alive means it is
            # destroyed inside.
            'gNdsGcRunAllEntryTapMask',
            'gNdsGcRunAllExitTapMask',
            'gNdsGcRunAllTapAliveCount',
            'gNdsGcRunAllTapLostCount',
            # The exit gate itself, read from the scene's own statics rather than
            # inferred from the taskman tick count -- those are two different
            # clocks and only this one gates mnVSResultsCheckExit. TotalTimeTics
            # below AllowExitWait means START is being ignored for a reason that
            # has nothing to do with the input pipeline.
            'sMNVSResultsTotalTimeTics',
            'sMNVSResultsAllowExitWait',
            # The only port writers to gSYControllerDevices[].button_tap outside
            # the publish. Nonzero on Results would make one of them the third
            # writer; zero rules the whole fighter-script family out.
            'gNdsFighterProcessLoopControllerBridgeCount',
            'gNdsFighterProcessLoopP0InputApplyCount',
            'gNdsFighterSchedulerLoopP0InputApplyCount',
            # SLICE 50's texture-binding certificate, and the ONLY instrument
            # that can answer its recorded residual risk: Boundary proves ONE
            # match, and the per-frame re-proof it replaced was self-healing
            # against invalidation paths nobody enumerated. SweepFail must be 0
            # in every match -- non-zero after a restart means a certificate
            # read current against a released texture slot, i.e. stale native
            # geometry. EpochBump must be NON-zero here (it is 0 *within* a
            # match by design, and the discard at every battle entry releases
            # every entry), so a 0 across a rematch chain is the failure, not
            # the success. Fast/Sweep are the positive control: Sweep staying
            # tiny while Fast tracks ~10/frame is the win still being taken.
            'gNdsR2TexProofFastCount',
            'gNdsR2TexProofSweepCount',
            'gNdsR2TexProofSweepFailCount',
            'gNdsR2TextureEpochBumpCount',
            # HANDOFF's latent cliff, made readable across a MULTI-match run.
            # sNdsAObjEvent32NormalizedCount reached 973 of its 1,024 cap after
            # one minute, and the overflow branch does not fail loudly: it
            # rejects the script (reason 12) and SILENTLY SKIPS the animation
            # attach. The table is cleared only by ndsRelocResetLoadedFiles
            # (src/port/reloc_backend_assets.c:2093), so whether a rematch
            # resets it or piles on top of it decides whether match three
            # animates. FailCount rising beside a Count near 1,024 is the
            # cliff; a Count that returns to its one-match value is the reset.
            'sNdsAObjEvent32NormalizedCount',
            'gNdsAObjEvent32NormalizeScriptCount',
            'gNdsAObjEvent32NormalizeReuseCount',
            'gNdsAObjEvent32NormalizeFailCount',
            # The battle's own presented-cadence histogram, which AGENTS.md
            # requires every device A/B report to carry (2/3/4/5+ plus the max
            # interval). These are reset per battle ENTRY (taskman_seam.c:4552
            # seeds VBlankStart in the same block), so an end-of-run read
            # describes the LAST match of a rematch chain -- the only per-match
            # cadence figure available for match two onwards, because the tick
            # sampler cannot press START and so can never leave match one.
            'gNdsBattlePlayablePacingPresentIntervalBucket[2]',
            'gNdsBattlePlayablePacingPresentIntervalBucket[3]',
            'gNdsBattlePlayablePacingPresentIntervalBucket[4]',
            'gNdsBattlePlayablePacingPresentIntervalBucket[5]',
            'gNdsBattlePlayablePacingPresentIntervalMax',
            'gNdsBattlePlayablePacingCadenceViolationCount',
            'gNdsBattlePlayablePacingVBlanks')
        # Second-entry allocation ledger, only when the build defines it. This is
        # the rematch arm of the item 3 question the Sudden Death lane already
        # answered: that lane measured 925,816 B for match one and 906,568 B for
        # the Sudden Death setup from an identical rewound baseline of 319,968.
        # A rematch total read here is the third figure. Appending is safe
        # because the reads are keyed by NAME, not position -- see below.
        if ($SecondEntryDiag) {
            $cleanFields += @(
                'gNdsAllocLedgerTotalBytes',
                'gNdsAllocLedgerUsed',
                'gNdsAllocLedgerOverflow')
        }
        $cleanFields = Select-SoakSymbols $cleanFields -Announce
        $format = (, '%u' * $cleanFields.Count) -join ','
        # The ROM's own per-iteration sample ring, read in the SAME single stop.
        # A point read of gNdsTickHud*Ticks is ONE frame -- those globals are
        # zeroed at the top of every presentation-loop iteration
        # (taskman_seam.c:5077, :7769) -- and reading a distribution from one
        # sample has now cost this campaign three withdrawn conclusions. The ring
        # holds the last NDS_TICK_HUD_WINDOW presented iterations, so after a
        # rematch its contents are match-TWO frames, which is exactly the
        # population in question. Bucket 1 is Fighters, 2 is Stage.
        $progress = Invoke-SoakGdb -Tag 'clean' -TimeoutSeconds 90 -Commands @(
            "printf `"CLEAN=$format\n`", $($cleanFields -join ', ')",
            # The GObj-latch margin, as a number rather than as a verdict. Under
            # 25,600 the pool is capped and the GO countdown dies; see the
            # sGCCommonsMaxNum comment in $cleanFields.
            ('printf "GENERALFREE=%d\n", ' +
             '(char *)gSYTaskmanGeneralHeap.end - ' +
             '(char *)gSYTaskmanGeneralHeap.ptr'),
            'printf "RINGHEAD=%u,%u\n", sBattleTickHudRingHead, sBattleTickHudRingCount',
            'echo RINGFIGHTERS=\n',
            'output sBattleTickHudRing[1]',
            'echo \n')
        if ($null -eq $progress) {
            Write-Host 'end-of-run attach failed; match count unknown for this run.'
        } else {
            $match = [regex]::Match($progress, 'CLEAN=([0-9,]+)')
            if (-not $match.Success) {
                # GDB's printf is all-or-nothing: one unresolvable name aborts the
                # whole statement, so every counter is lost together. That used to
                # be SILENT -- this branch did not exist -- and a soak whose ROM
                # predated a new counter reported nothing but the pixel verdict,
                # which is indistinguishable from a soak that simply had nothing
                # to say. Echo what GDB actually replied; the "No symbol ... in
                # current context" line names the offending counter directly.
                Write-Host 'end-of-run counter read produced no CLEAN= line. GDB said:'
                # (ring output, if any, is echoed below with the rest of the reply)
                foreach ($line in ($progress -split "`r?`n" |
                    Where-Object { $_.Trim() -ne '' } | Select-Object -Last 12)) {
                    Write-Host "    $line"
                }
            } else {
                $values = $match.Groups[1].Value -split ','
                # Keyed by NAME, not position. Adding a counter to $cleanFields
                # used to silently renumber every read below it; the arena pair
                # was inserted after both indices in use purely by luck.
                $counter = @{}
                Write-Host 'progress over the run:'
                for ($i = 0; $i -lt $cleanFields.Count; $i++) {
                    Write-Host ("    {0,-40} {1}" -f $cleanFields[$i], $values[$i])
                    $counter[$cleanFields[$i]] = [uint32]$values[$i]
                    $samples += [pscustomobject]@{
                        counter = $cleanFields[$i]; value = [uint32]$values[$i] }
                }
                $presented = $counter['gNdsBattlePlayablePacingPresentedFrames']
                $matches_run = $counter['gNdsVSResultsStartCount']
                # Owner, 2026-07-29: *"if you want to run a longer soak for any
                # reason, then you also need to change the match timer to match
                # the soak time"*. Read from the ROM rather than assumed, so the
                # two cannot drift: gSCManagerTransferBattleState.time_limit is in
                # MINUTES (scene_harness.c:182 seeds 1). Past that the match is
                # over and every remaining second watches a Results screen that
                # never exits, so the extra time proves nothing about gameplay.
                $matchMinutes = $counter['gSCManagerTransferBattleState.time_limit']
                # THE MATCH TIMER, READ OUT OF THE GUEST. This is the only proof
                # that the soak got the match it asked for: NDS_R2_SOAK_MATCH_MINUTES
                # is a build flag, and reading a flag's default proves nothing about
                # what the ROM seeded. The expected value is what THIS run resolved,
                # so a build/soak mismatch cannot pass as a clean run.
                $expectedMatch = if ($resolvedMatchMinutes -gt 0) { $resolvedMatchMinutes } else { 1 }
                if ($matchMinutes -ne [uint32]$expectedMatch) {
                    Write-Host (('  WARNING: the guest seeded a {0}-minute match but ' +
                        'this run resolved {1}. The ROM is not the configuration ' +
                        'that was asked for.') -f $matchMinutes, $expectedMatch)
                } else {
                    Write-Host ("  match timer confirmed in-guest: {0} minute(s)" -f $matchMinutes)
                }
                # The GObj-latch margin. CAVEAT, measured the first time this
                # printed: the clean read happens at END of run, which is the
                # RESULTS scene, and the heap is per-scene -- a battle that
                # aborted at 23,032 free reports 318,924 here. So this number is
                # informative, not the guard. The GUARD is sGCCommonsMaxNum
                # above: it is -1 until ifCommonSetMaxNumGObj caps the pool and
                # sticky afterwards, so any other value means the cap fired
                # during THIS run whatever the heap says now. Read them
                # together; the freeze capture prints the battle-time free bytes
                # for the run that actually fell over.
                $freeMatch = [regex]::Match($progress, 'GENERALFREE=(-?\d+)')
                if ($freeMatch.Success) {
                    $free = [int]$freeMatch.Groups[1].Value
                    Write-Host ("    {0,-40} {1} (end-of-run scene, not battle)" -f
                        'general heap free bytes', $free)
                    $samples += [pscustomobject]@{
                        counter = 'generalHeapFreeBytes'; value = $free }
                }
                # ONE-WAY ONLY, and this comment used to claim the opposite.
                # A value other than -1 does mean ifCommonSetMaxNumGObj capped
                # the GObj pool, and past that cap gcMakeGObj returns NULL while
                # the GO countdown dereferences it. But -1 proves NOTHING: the
                # cap is NOT sticky across a scene change, and this read happens
                # at end of run, which is the Results scene, so a battle that
                # latched at 47 still reports -1 here (docs/HANDOFF.md says so in
                # as many words). Calling this "the actual guard" is how a
                # 2026-08-02 run got reported as "the cap never fired" when its
                # own gNdsTaskmanGeneralHeapFreeMin was 24,404 -- already under
                # the 25,600 threshold. READ THE HEAP LOW-WATER AND THE SPAWN-FAIL
                # COUNTERS; this line can only ever confirm, never clear.
                $commonsMax = $counter['sGCCommonsMaxNum']
                if (($null -ne $commonsMax) -and ($commonsMax -ne 4294967295u)) {
                    Write-Host (('  WARNING: the GObj pool cap FIRED at {0}. ' +
                        'ifCommonSetMaxNumGObj saw the general heap under 25,600 ' +
                        'bytes free; past that cap gcMakeGObj returns NULL and the ' +
                        'GO countdown dereferences one. Free arena bytes -- .text ' +
                        'costs arena one for one here.') -f $commonsMax)
                }
                $arena = $counter['gNdsTaskmanArenaChosenSize']
                if ($arena -lt 1245184u) {
                    Write-Host ((
                        "  WARNING: taskman arena {0} is BELOW the 0x130000 floor " +
                        "after {1} failed steps -- this build lost ~237 KB of game " +
                        'heap and the Task 36 replay path. Check static BSS growth.'
                        ) -f $arena, $counter['gNdsTaskmanArenaAllocFailCount'])
                }
                Write-Host ("  => {0} results-scene start(s), i.e. {0} completed match(es)" -f $matches_run)
                # A run that never presented a battle frame has not been soaked;
                # it has failed to boot, and calling that NO-FREEZE is exactly the
                # false negative that cost this instrument two withdrawn verdicts.
                #
                # ...but PresentedFrames is PER BATTLE ENTRY, not per run:
                # ndsBattlePlayablePacingStart (taskman_seam.c:4517) zeroes it every
                # time the battle scene starts, and Sudden Death is a second entry.
                # Measured 2026-08-02: a run whose match tied at TIME 00:00 ended
                # inside the Sudden Death scene load -- the documented ~30 s of dead
                # air -- and reported presented 0 with PlacementInitCount 2 and a
                # screenshot of a fought-out match at 128%/81%. That is the OPPOSITE
                # of never having started, and it read as a boot failure caused by
                # the change under test. PlacementInitCount is the run-global
                # witness: it rises once per battle entry and nothing resets it, so
                # a non-zero value with presented 0 means the LAST entry has not
                # drawn yet, which is a short run rather than a dead ROM.
                $entries = $counter['gNdsSCVSBattlePlacementInitCount']
                if (($presented -eq 0u) -and ($null -ne $entries) -and ($entries -gt 1u)) {
                    $verdict = 'ENTRY-IN-PROGRESS'
                    $diagnosis = (('battle entry {0} presented no frame yet -- ' +
                        'PresentedFrames is per entry and Sudden Death/rematch ' +
                        'resets it, so this run ended inside a scene load. ' +
                        'Entries 1..{1} DID run. Re-soak longer to judge the last ' +
                        'one.') -f $entries, ($entries - 1u))
                    Write-Host "verdict CORRECTED to $verdict -- $diagnosis"
                } elseif ($presented -eq 0u) {
                    $verdict = 'NEVER-STARTED'
                    $diagnosis = ('the ROM presented zero battle frames, so nothing ' +
                        'was soaked -- the picture moved, but not because of gameplay')
                    Write-Host "verdict CORRECTED to $verdict -- $diagnosis"
                } elseif ($matches_run -ge 1u) {
                    # Reaching Results is the natural END of a soak, not a bonus:
                    # mnVSResultsCheckExit (decomp mnvsresults.c:266) returns TRUE
                    # only on a START_BUTTON tap and has no timeout, and the DS
                    # results loop (taskman_seam.c:6968) is bounded by updates only
                    # when NDS_HARNESS_FAST_LOGIC != 0 -- which every shipped target
                    # pins to 0. So an unattended ROM sits in Results forever, by
                    # original design. No passive soak can ever see match two.
                    Write-Host ("  battle completed and Results is up. A passive soak " +
                        "CANNOT reach match {0}: Results exits on START only." -f `
                        ($matches_run + 1u))
                } else {
                    Write-Host '  WARNING: no match completed. This run says nothing' `
                        'about match teardown, rematch, or cross-match drift.'
                }
                # A SOAK THAT PASSES WHILE TESTING NOTHING IS WORSE THAN ONE THAT
                # FAILS, so this is a verdict and not the NOTE it used to be. A
                # passive freeze soak exists to keep gameplay running; once the
                # match ends, every remaining second watches a Results screen that
                # never exits (mnVSResultsCheckExit needs START), and NO-FREEZE
                # earned there is about the results loop, not about play. Two runs
                # on 2026-08-02 were spent that way and read clean.
                #
                # Only fires when a press schedule was NOT configured: a
                # rematch/Sudden-Death soak deliberately outlives its match,
                # because reaching Results is the whole point of that run. Placed
                # after the chain above so a more severe correction wins, and
                # gated on NO-FREEZE so it can only ever downgrade a pass.
                if (($verdict -eq 'NO-FREEZE') -and ($PressStartSeconds -le 0) -and
                    ($matches_run -ge 1u)) {
                    $verdict = 'SOAK-UNDERCOVERED'
                    $diagnosis = (('the {0}-minute match ended during a {1}-minute ' +
                        'passive soak, so the tail watched Results rather than ' +
                        'gameplay. Re-run with -MatchMinutes {2} or higher; the ' +
                        'default resolves this automatically, so this run was ' +
                        'given an explicit value that was too small.') -f
                        $matchMinutes, $MinutesToRun,
                        [int][math]::Ceiling($MinutesToRun))
                    Write-Host "verdict CORRECTED to $verdict -- $diagnosis"
                }
            }
        }
    }

    # Capture on the way out REGARDLESS of verdict. NO-FREEZE proves the picture
    # MOVES; it says nothing about whether the picture is CORRECT, and the
    # second-entry defects this soak is aimed at -- stale textures, a corrupt
    # stage, duplicated fighters -- all animate happily. A rematch soak that
    # ends clean and leaves no image cannot answer the question that was asked
    # of it, which is exactly what happened on the 2026-07-31 rematch run.
    if ($verdict -notin @('FROZEN-PICTURE', 'FROZEN-FROM-START', 'CAPTURE-STATIC')) {
        [void](New-Item -ItemType Directory -Force -Path $logDir)
        $cleanStamp = Get-Date -Format 'yyyy-MM-dd_HHmmss'
        $cleanShot = Join-Path $logDir "$cleanStamp-$verdict.png"
        # -PreferPrintWindow on BOTH capture sites: this is an unattended soak on
        # a machine the owner is using, so a foreground raise is not available
        # and CopyFromScreen would photograph whatever is stacked over melonDS.
        [void](Save-MelonDSWindowCapture -WindowHandle $window -Path $cleanShot `
            -PreferPrintWindow)
        Write-Host "final frame saved to $cleanShot"
    }
    if ($verdict -in @('FROZEN-PICTURE', 'FROZEN-FROM-START', 'CAPTURE-STATIC')) {
        [void](New-Item -ItemType Directory -Force -Path $logDir)
        $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss'
        $shot = Join-Path $logDir "$stamp-frozen.png"
        [void](Save-MelonDSWindowCapture -WindowHandle $window -Path $shot `
            -PreferPrintWindow)
        Write-Host "frozen frame saved to $shot"

        $capture = Invoke-SoakGdb -Tag 'freeze' -TimeoutSeconds 120 -Commands (@(
            'printf "FREEZE-PC=%p\n", $pc',
            # The whole freeze class is decomp malloc.c:30's `while (TRUE);`, which
            # compiles to a single self-branch. One instruction at the PC therefore
            # names the mechanism outright -- `b.n <self>` is a spin, anything else
            # is not -- and it needs no second sample, which matters because the
            # stub allows exactly one session and stops freeze the emulator, so
            # elapsed guest time cannot be sampled twice from inside one attach.
            'x/1i $pc',
            'backtrace 40',
            # WHAT THE GAME LOOP WAS RUNNING, which the backtrace often cannot
            # say. When the ARM9's game thread dies or blocks, calico schedules
            # the idle thread, so the capture comes back as a bare armWaitForIrq
            # with every register zeroed and no frame from the guest at all --
            # three captures in a row on 2026-08-01. gcRunAll (objman.c:2202)
            # publishes gGCCurrentCommon and gGCCurrentProcess as it walks, and
            # they are NOT cleared on the way out, so they name the last GObj
            # and the last process callback the loop entered. gdb symbolises the
            # function pointers, which is the whole point. Separate commands so
            # a NULL deref on one cannot take the others with it.
            # NOTE, 2026-08-01: an erroring -ex ABORTS THE REST OF THE BATCH.
            # A block of gNdsTask20CoroutineCensus* probes was added here
            # unconditionally, and against any ROM built without
            # NDS_TASK20_STACK_PROFILE=1 the very next capture stopped dead
            # after the backtrace -- losing COUNTERS, MALLOCOVF, KOBURST and
            # every heap number on the run that was meant to settle the
            # question. Nothing in the output said so; the file was just short.
            # Only add a command here if the symbol exists in EVERY build this
            # script can be pointed at. (The probes themselves are gone: they
            # refuted the stack-overflow theory -- ovf=0, high-water 268 of
            # 16,384 -- and are not worth this hazard to keep.)
            'p gGCCurrentCommon',
            'p *gGCCurrentCommon',
            'p gGCCurrentProcess',
            'p *gGCCurrentProcess',
            'info registers',
            'printf "REG_IME=%08x\n", *(unsigned int *)0x04000208',
            'printf "REG_IE=%08x\n", *(unsigned int *)0x04000210',
            'printf "REG_IF=%08x\n", *(unsigned int *)0x04000214',
            'printf "GXSTAT=%08x\n", *(unsigned int *)0x04000600',
            'printf "IPCSYNC=%08x\n", *(unsigned int *)0x04000180',
            'printf "IPCFIFOCNT=%08x\n", *(unsigned int *)0x04000184',
            ('printf "COUNTERS=%u,%u,%u,%u\n", sVBlankCount, ' +
             'gNdsBattlePlayablePacingPresentedFrames, dSYTaskmanUpdateCount, ' +
             'gNdsVSResultsTickCount'),
            # Separate printfs on purpose: one missing symbol fails its whole
            # command, and COUNTERS must not be lost to an absent arena counter.
            ('printf "ANIMARENA=%u,%u\n", gNdsR2AnimCacheArenaUsedBytes, ' +
             'gNdsR2AnimCacheArenaOverflows'),
            # The taskman arena is chosen by a downward calloc search at boot, so
            # ANY growth in static BSS can push it over the 0x130000 = 1245184
            # cliff and cost ~237 KB in one step. A heap-exhaustion freeze is
            # therefore not fully diagnosed without the size that was actually
            # secured -- print it next to the request that failed.
            ('printf "TASKARENA=%u,%u\n", gNdsTaskmanArenaChosenSize, ' +
             'gNdsTaskmanArenaAllocFailCount'),
            ('printf "MALLOCOVF=%u,id=%u,req=%u,head=%u,lr=%08x\n", ' +
             'gNdsSyMallocOverflowCount, gNdsSyMallocOverflowArenaID, ' +
             'gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, ' +
             'gNdsSyMallocOverflowCallerLR'),
            # The KO burst, which is a repeat freeze site (owner, 2026-08-01) and
            # so belongs in the FREEZE capture rather than only in the
            # end-of-run globals -- a frozen run never reaches those, which cost
            # two builds' worth of blind iteration. STAGE is the breadcrumb: it
            # is the last checkpoint the burst reached, so it names the step that
            # faulted without a usable backtrace. See NDS_KO_BURST_STAGE_* in
            # include/nds/nds_effects.h.
            ('printf "KOBURST=att=%u,ok=%u,drop=%03x,stage=%u\n", ' +
             'gNdsKOBurstAttemptCount, gNdsKOBurstCompleteCount, ' +
             'gNdsKOBurstDropMask, gNdsKOBurstStage'),
            # EFDesc offset resolution. RESOLVE>0 proves the resolver ran;
            # DISABLED counts descs neutralised for want of a backing asset; the
            # three spans are EFCommonEffects1/2/3, where a span of sizeof(Sprite)
            # means the asset is absent entirely.
            ('printf "EFDESC=resolved=%u,disabled=%u,span=%u/%u/%u\n", ' +
             'gNdsEFDescResolveCount, gNdsEFDescDisabledCount, ' +
             'gNdsEFDescEffectsSpan[0], gNdsEFDescEffectsSpan[1], ' +
             'gNdsEFDescEffectsSpan[2]'),
            # THE OTHER GIVE-UP SPIN, and the reason the allocator counters alone
            # are not a diagnosis. `syTaskmanCheckBufferLengths` (decomp
            # sys/taskman.c:329) has two `while (TRUE);` branches of its own: line
            # 338 for a display-list buffer past its end, line 344 for the
            # graphics heap. Reproduced 2026-07-31 on a rematch with MALLOCOVF=0,
            # so a run that stops there and reads only the malloc counters looks
            # like "a spin with no cause". Print the four DL buffers and the
            # graphics heap: head - start is the volume emitted, length is the
            # capacity, and whichever kind has head > start+length is the one
            # that gave up. One printf per kind so an absent symbol cannot take
            # the rest with it.
            ('printf "DLBUF0=start=%p,len=%u,head=%p,used=%d\n", ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][0].start, ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][0].length, ' +
             'gSYTaskmanDLHeads[0], ' +
             '(char *)gSYTaskmanDLHeads[0] - ' +
             '(char *)sSYTaskmanDLBuffers[gSYTaskmanTaskID][0].start'),
            ('printf "DLBUF1=start=%p,len=%u,head=%p,used=%d\n", ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][1].start, ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][1].length, ' +
             'gSYTaskmanDLHeads[1], ' +
             '(char *)gSYTaskmanDLHeads[1] - ' +
             '(char *)sSYTaskmanDLBuffers[gSYTaskmanTaskID][1].start'),
            ('printf "DLBUF2=start=%p,len=%u,head=%p,used=%d\n", ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][2].start, ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][2].length, ' +
             'gSYTaskmanDLHeads[2], ' +
             '(char *)gSYTaskmanDLHeads[2] - ' +
             '(char *)sSYTaskmanDLBuffers[gSYTaskmanTaskID][2].start'),
            ('printf "DLBUF3=start=%p,len=%u,head=%p,used=%d\n", ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][3].start, ' +
             'sSYTaskmanDLBuffers[gSYTaskmanTaskID][3].length, ' +
             'gSYTaskmanDLHeads[3], ' +
             '(char *)gSYTaskmanDLHeads[3] - ' +
             '(char *)sSYTaskmanDLBuffers[gSYTaskmanTaskID][3].start'),
            'printf "DLTASK=%d\n", gSYTaskmanTaskID',
            # THE GOBJ LATCH, and it is a different failure from every spin
            # above. ifCommonSetMaxNumGObj (decomp ifcommon.c:3156) caps the
            # GObj pool at whatever count is active the moment the general heap
            # drops below 25 KiB free. Past that cap gcMakeGObj returns NULL and
            # ifCommonCountdownMakeInterface dereferences it -- a DATA ABORT at
            # the GO countdown with a perfectly healthy allocator, which is
            # exactly what MALLOCOVF=0 plus a park in __excpt_entry looks like.
            # It has bitten this port twice (the source-sized particle pools,
            # then NDS_R2_PARTICLE_DRAW's .text). GENERALFREE is the number that
            # decides whether a candidate fix has to find bytes or change the
            # threshold; COMMONSMAX is -1 while the cap has never fired.
            ('printf "GENERALHEAP=start=%p,ptr=%p,end=%p,free=%d\n", ' +
             'gSYTaskmanGeneralHeap.start, gSYTaskmanGeneralHeap.ptr, ' +
             'gSYTaskmanGeneralHeap.end, (char *)gSYTaskmanGeneralHeap.end - ' +
             '(char *)gSYTaskmanGeneralHeap.ptr'),
            'printf "COMMONSMAX=%d\n", sGCCommonsMaxNum',
            'printf "COMMONSACTIVE=%u\n", sGCCommonsActiveNum',
            'printf "SPRITESACTIVE=%u\n", sGCSpritesActiveNum',
            ('printf "GFXHEAP=start=%p,ptr=%p,end=%p,used=%d\n", ' +
             'gSYTaskmanGraphicsHeap.start, gSYTaskmanGraphicsHeap.ptr, ' +
             'gSYTaskmanGraphicsHeap.end, (char *)gSYTaskmanGraphicsHeap.ptr - ' +
             '(char *)gSYTaskmanGraphicsHeap.start')
            # The particle counters belong on the FREEZE path too, not only on
            # the clean-run path they were added to. 2026-07-31: the arena fix
            # let NDS_R2_PARTICLE_RUNTIME=1 reach the battle and then abort near
            # ifCommonTrafficMakeSObj, and the one question that capture could
            # not answer was whether the bank had loaded at all -- because these
            # are read only when the run does NOT freeze, which is exactly the
            # run that does not need them. Guarded by the same nm filter, so a
            # ROM without the runtime prints nothing extra.
            ) + @(Select-SoakSymbols @(
                # THE TWO CONVERTED FREEZE CLASSES, first, because a frozen run
                # never reaches the clean-run globals and these are the ones
                # that say the ROM survived something instead of spinning in it.
                # objman's pools, then the animation-script parsers -- the three
                # 2026-08-02/03 captures stopped in gcParseDObjAnimJoint's event
                # loop, which no counter could see until it was bounded.
                'gNdsObjmanPanicCount',
                'gNdsObjmanPanicMask',
                'gNdsObjAnimRunawayCount',
                'gNdsObjAnimRunawayMask',
                'gNdsObjAnimRunawayScript',
                'gNdsObjAnimRunawayOpcode',
                'gNdsRelocResolveOffsetCount',
                'gNdsRelocResolveMisalignCount',
                'gNdsRelocResolveMisalignValue',
                'gNdsParticleBankLoadResult',
                'gNdsParticleBankScriptsUnpacked',
                'gNdsParticleBankScriptsRejected',
                'gNdsParticleScriptStartCount',
                'gNdsParticleRejectCount',
                'gNdsParticleStructsLive',
                'gNdsParticleStructsMax',
                'gNdsParticleDrawSeamCount',
                # THE TEXTURE SIDE, and why it is on the freeze path. The
                # particle atlas is 32,768 bytes of the 119,872 free after the
                # battle's pinned static set, and the interface's own OAM
                # atlases are prepared from the same pool. A 2026-08-01 abort at
                # the GO countdown -- ifCommonTrafficMakeSObj, MALLOCOVF=0, so
                # not the heap -- appeared only with NDS_R2_PARTICLE_DRAW=1, and
                # the capture could not say whether the atlas had taken VRAM the
                # interface then could not get, because none of these were read.
                'gNdsRendererParticleAtlasPrepareCount',
                'gNdsRendererParticleAtlasFailCount',
                'gNdsRendererParticleAtlasBytes',
                'gNdsRendererBattleStaticTexturePrepareCount',
                'gNdsRendererBattleStaticTextureViolationCount',
                'gNdsRendererSceneTextureVramResetCount',
                'gNdsIFCommonNativeOamTextureDiscardCount') | ForEach-Object {
                    "printf `"$_=%u\n`", $_" }))
        if ($capture) {
            Write-Host '--- freeze capture'
            Write-Host $capture
            # A static picture is not proof of a hang, and the capture already
            # holds the discriminator: `x/1i $pc` on a spin disassembles to a
            # branch to its OWN address, which is what decomp malloc.c:30's
            # `while (TRUE);` and ndsSyMallocOverflowHalt both compile to. Any
            # other instruction means the ARM9 was executing real work when it was
            # halted, and a long scene load looks exactly like a freeze from
            # outside. Say which one was observed rather than assuming the worse.
            $pcMatch = [regex]::Match($capture, 'FREEZE-PC=(0x[0-9a-fA-F]+)')
            $spinning = $false
            if ($pcMatch.Success) {
                $pcText = $pcMatch.Groups[1].Value
                $spinning = $capture -match
                    ('=>\s*' + [regex]::Escape($pcText) + '[^\r\n]*\bb(?:\.n|\.w)?\s+' +
                     [regex]::Escape($pcText) + '\b')
            }
            # SECOND WITNESS, independent of the pixels: the guest's own
            # presented-frame counter. A picture that never changed means the
            # ARM9 stopped presenting, so a large presented count over the same
            # window is a flat contradiction and the CAPTURE is the suspect, not
            # the ROM. Added 2026-08-01 after a soak hashed the owner's browser
            # for eighty seconds and reported FROZEN-FROM-START against a ROM
            # that had presented 1,721 frames -- the number was right there in
            # the same capture, unread. The pixel path was fixed at its root
            # (Get-MelonDSWindowFrameHash now reads the window, not the screen);
            # this stays as the check that does not depend on that being true.
            # VBlanks per presented frame, NOT presented-frames-per-second. The
            # rate form was tried first and immediately produced a false alarm:
            # a ROM that ran healthily for 35s and then died had 418 frames over
            # 110s, which is 3.8/s and trips any "still drawing" threshold, even
            # though it had genuinely stopped. The ratio does not have that
            # blind spot, because sVBlankCount keeps climbing after the ARM9
            # stops presenting. Healthy is 2-6 VBlanks per presented frame --
            # the project's own pacing histogram; the browser-occluder run read
            # 5035/1721 = 2.9 while a dead ROM read 81510/418 = 195.
            $countersMatch = [regex]::Match($capture, 'COUNTERS=(\d+),(\d+),')
            if ($countersMatch.Success) {
                $vblanks = [int]$countersMatch.Groups[1].Value
                $presented = [int]$countersMatch.Groups[2].Value
                $frozenSeconds = ($identical + 1) * $PollSeconds
                if (($verdict -like 'FROZEN*') -and ($presented -gt 0)) {
                    $perFrame = [math]::Round($vblanks / $presented, 1)
                    if ($perFrame -le 8) {
                        $verdict = "$verdict-PRESENTING"
                        Write-Host ''
                        Write-Host ("CONTRADICTION -- verdict marked $verdict. " +
                            "The picture was called frozen for ${frozenSeconds}s, " +
                            "but the guest presented $presented frames across " +
                            "$vblanks VBlanks -- $perFrame VBlanks per frame, " +
                            'which is a normally paced ROM, not a stopped one. ' +
                            'Suspect the CAPTURE before the ROM: check the saved ' +
                            'PNG actually shows melonDS and not a window stacked ' +
                            'over it, and note that an attached GDB halts a ' +
                            'RUNNING core at an arbitrary PC -- that backtrace is ' +
                            'not evidence of a hang.')
                    }
                }
            }
            if (-not $spinning) {
                $verdict = "$verdict-UNCONFIRMED"
                Write-Host ''
                Write-Host ("verdict DOWNGRADED to $verdict -- the PC is not a " +
                    'self-branch, so the core was executing when halted. This is a ' +
                    'stall or a slow scene load, NOT the allocator spin class. ' +
                    'Compare against the ~30s NitroFS scene-load dead air before ' +
                    'treating it as a hang.')
            } elseif ($capture -match '__excpt_entry') {
                # A self-branch inside calico's exception handler is NOT the
                # game giving up -- it is the CPU having already aborted, and
                # the handler parking. Reported as the allocator class once
                # (2026-08-01, a countdown-time data abort with MALLOCOVF=0),
                # which sent the first ten minutes of the diagnosis at the heap.
                # The registers say which: cpsr mode 0x17/0x97 is ABT, lr_usr is
                # the faulting caller, and r0/r3 usually still hold the pointer
                # that was dereferenced.
                Write-Host ''
                $verdict = "$verdict-ABORT"
                Write-Host ('CPU EXCEPTION, not a give-up loop: the PC is parked ' +
                    'in __excpt_entry, so the ARM9 took a data/prefetch abort and ' +
                    'calico halted there. MALLOCOVF/DLBUF/GFXHEAP describe a ' +
                    'HEALTHY allocator in this case and must not be read as the ' +
                    'cause. Read `lr_usr` for the faulting function and r0-r3 for ' +
                    'the bad pointer; cpsr low bits 0x17 = data abort.')
            } else {
                Write-Host ''
                Write-Host ('spin CONFIRMED: the PC branches to itself, which is ' +
                    'the `while (TRUE);` give-up signature. WHICH give-up is a ' +
                    'separate question -- BattleShip has eleven, across sys/malloc.c ' +
                    'AND sys/taskman.c, and they have different fixes. Read the ' +
                    'source line gdb printed above the PC: malloc.c:30 is heap ' +
                    'exhaustion (MALLOCOVF names it), taskman.c:338 is a ' +
                    'display-list buffer past its end (DLBUF0..3 name it), ' +
                    'taskman.c:344 is the graphics heap (GFXHEAP). Do not report ' +
                    'this as "the allocator" without the matching counter.')
            }
        } else {
            Write-Host 'freeze capture produced nothing; the attach failed outright.'
        }
        Set-Content -LiteralPath (Join-Path $logDir "$stamp-$verdict.txt") -Value (
            @("rom=$rom", "build=$Build", "verdict=$verdict",
              "diagnosis=$diagnosis", "screenshot=$shot", '') +
            ($samples | Format-Table -AutoSize | Out-String) + $capture)
        Write-Host "capture written to artifacts\verification\freeze-soak\$stamp-$verdict.txt"
    }

    if ($JsonOut) {
        [void](New-Item -ItemType Directory -Force `
            -Path (Split-Path -Parent $JsonOut))
        @{
            rom = $rom
            build = $Build
            runnerSlot = $RunnerSlot
            verdict = $verdict
            diagnosis = $diagnosis
            pollSeconds = $PollSeconds
            minutesToRun = $MinutesToRun
            identicalFramesToTrip = $IdenticalFramesToTrip
            samples = $samples
        } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $JsonOut
    }
    if ($verdict -ne 'NO-FREEZE') { exit 2 }
} finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
