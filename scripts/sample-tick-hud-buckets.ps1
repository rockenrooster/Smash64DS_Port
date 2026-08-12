[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4613,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-tick-hud-buckets',
    [switch]$NoBuild,
    # Requires a ROM built NDS_TASK68_FALLBACK_CENSUS=1. Off by default because
    # that flag adds BSS and this ROM's pacing is cache-placement sensitive, so a
    # census build is not comparable to an ordinary tick-HUD baseline.
    #
    # R2-03 E35: NDS_TASK75_LOAD_CENSUS=1 alone is NOT enough. It re-points the
    # shared per-frame ring at gNdsTask75AssetLoadCount, but this switch also
    # reads gNdsTickHudNativeOwnerFallbackByReason[], which only Task 68 defines,
    # so the run reaches its window and then dies in GDB with "No symbol ... in
    # current context". Build with BOTH flags: Task 75 wins the #if that selects
    # the ring source in nds_platform.c, and Task 68 supplies the symbol. Which
    # counter a given dump holds is a property of the build -- the column is
    # labelled "fallback" either way.
    [switch]$FallbackCensus,
    # Read the ROM's own 128-entry sample ring in a single stop instead of
    # stopping GDB once per presented frame. ndsPlatformTickHudSample already
    # records every bucket of every presented iteration into
    # sBattleTickHudRing -- the per-frame stop was collecting data the ROM was
    # keeping anyway. One stop cannot perturb pacing 128 times, and the ring is
    # indexed by iteration rather than by the presented-frame counter, so it is
    # not exposed to whatever makes that counter disagree with the marker.
    [switch]$RingDump,
    # Extra u32 globals to read once at the end of the run, reported and stored
    # beside the buckets. This exists for engagement proof: an optimization
    # behind a build flag that silently never fires is indistinguishable from an
    # optimization that fired and saved nothing, and this campaign has shipped
    # that mistake (Task 52 found the Task 36 replay structurally disabled after
    # it had already been measured). A candidate arm should carry its own
    # fired/skipped counters and read them from the same run that produced the
    # buckets, not from a second run that may not have taken the same path.
    [string[]]$ExtraGlobals = @(),
    # Extra `make` variables for the build, e.g.
    # -MakeFlags NDS_TASK75_LOAD_CENSUS=1,NDS_TASK68_FALLBACK_CENSUS=1.
    # -ExtraGlobals names that only exist behind a census flag REQUIRE the
    # matching flag here: without it this script rebuilds the ROM without the
    # symbol and then rejects it in its own guard below, which reads as "the
    # counter was deleted" rather than "the flag was never passed".
    [string[]]$MakeFlags = @(),
    # Continue when duplicate presented-frame ids are all REAL second iterations
    # of the 60 Hz loop rather than stale reads. Needed for slower builds (a
    # census ROM) whose pacing puts both iterations in separate samples. The
    # rows it then writes are for the per-frame SET only, never for P50/P95 --
    # the warning says so too. A stale read still fails hard and this switch
    # cannot suppress it.
    [switch]$AllowRepeatedFrames,
    # Globals sampled ONCE PER PRESENTED FRAME, alongside the buckets, and
    # written as extra -RowsCsv columns.
    #
    # This is not a variant of -ExtraGlobals, it is the thing -ExtraGlobals is
    # repeatedly mistaken for. -ExtraGlobals reads its names ONE TIME at the end
    # of the run, so it answers "how many did this run do"; it cannot answer
    # "which frames did them". R2-07 L2 needs the second question -- intersect
    # the asset-load frames with the over-gate frames -- and was blocked on
    # exactly that (board, 2026-07-31). Anything per-frame belongs here.
    [string[]]$PerFrameGlobals = @(),
    # Globals read ONCE PER RING STOP, stitched alongside the buckets and
    # labelled with the stop's frame range. This is the third granularity and
    # it exists because the other two cannot answer a whole-match question:
    # -ExtraGlobals reads once at the end ("how many did this run do"), and
    # -PerFrameGlobals is incompatible with -RingDump by construction (the ring
    # carries bucket words only), so a repeated-ring-dump run had no way to
    # carry any counter at all.
    #
    # Per stop is the right granularity for event counters anyway. The KO,
    # respawn and effect counters are CUMULATIVE, so differencing consecutive
    # stops gives the events inside each stop's window, which is what locates a
    # KO in a match without adding a single byte to the ROM. Use counters that
    # already exist and already have an in-ROM reader: a `volatile u32` whose
    # only consumer is a debugger gets collected by --gc-sections, so a NEW one
    # needs a marker-block reader added in the same change.
    [string[]]$PerStopGlobals = @(),
    # Per-frame rows as CSV, one line per presented sample. The percentile table
    # answers "how big is P95"; it cannot answer "which frames are the P95", and
    # every excursion investigation this campaign has run needed the second
    # question first -- R2-03 E35 had to profile frames 517-521 against a matched
    # control at 508-512, and identifying those five frames is what this writes
    # out. The JSON only ever carried the summary, so that identification was
    # being redone by hand each time.
    [string]$RowsCsv = '',
    # Presented frames between repeated ring stops. sBattleTickHudRing is 128
    # entries and sBattleTickHudRingCount saturates there (nds_platform.c:2137),
    # so ONE stop can never yield more than 128 samples however large -Samples
    # is; the guard further down already fails loudly on that rather than
    # silently returning the last 128. To cover a whole match, stop repeatedly
    # and concatenate instead of enlarging the ring: growing NDS_TICK_HUD_WINDOW
    # would cost ~24 KB of bss and change the instrument's own cache footprint,
    # which on this target has repeatedly moved the number being measured, and
    # it would make every prior measurement incomparable.
    #
    # The default is deliberately below 128. The ring advances once per
    # finalized iteration and a presented frame can carry TWO iterations (see
    # -AllowRepeatedFrames), so iterations >= presented frames; leaving headroom
    # is what lets the stitcher PROVE it did not alias a wrap rather than assume
    # it. A stride at 128 would make a full wrap indistinguishable from no
    # advance at all.
    [ValidateRange(8,120)][int]$RingStopStride = 96,
    [ValidateRange(1,4096)][int]$Samples = 32,
    [ValidateRange(1,1000000)][int]$StartFrame = 438,
    # 3600 used to be the cap, which a whole 3600-tick match cannot fit: the
    # match alone is ~60 s of guest time and this emulator runs it far slower
    # than real time under a GDB stub. A cap that binds turns "the run needed
    # longer" into a failure that looks exactly like a stall, which is the
    # confusion the timeout message below exists to end.
    [ValidateRange(30,14400)][int]$TimeoutSeconds = 900,
    # `name=value` pairs poked ONCE, at the first frame-complete marker, before
    # the sample window. This is what makes standing rule 7 expressible on the
    # gate instrument: prefer ONE binary with a runtime-settable route over two
    # separately-linked A/B ROMs, because this ROM's pacing is cache-placement
    # sensitive and split builds have already confused two comparisons. Without
    # it, every "dual-route" A/B silently degraded into the two-build form the
    # rule exists to forbid, and paid the +/-5,376 cross-build P95 floor for
    # nothing.
    #
    # The poke lands at the first marker rather than straight after
    # `target remote` on purpose: the marker is well past bss init, so a global
    # set there cannot be zeroed out from under the run.
    [string[]]$SetGlobals = @(),
    [string]$JsonOut = ''
)

# Reads the eleven NDS_TICK_HUD buckets
# (ALL/FTR/STG/BG/AUD/HUD/SRC/MISC/OTHR/WAIT/WORK) straight out of
# gNdsTickHudBuckets over GDB, one sample per presented battle iteration, so the
# HUD instrument can be recorded instead of photographed.
#
# Task 66 appended WAIT and WORK. ALL is VBlank-quantized wall time and so
# cannot show a saving smaller than one 560,190-tick period; WORK = ALL - WAIT
# is not quantized and is the series to search against. PROJECT_GOAL.md gates
# the milestone on WORK's P95 (<= 1.12M), which is why it is sampled as its own
# series rather than derived from the ALL and WAIT percentiles.
#
# The renderer benchmark path cannot do this: it asserts TICK_HUD=0 and
# SHIP_TELEMETRY=1 because the profile counters are a different instrument. The
# tick HUD is the only one that reports the whole loop partitioned into named
# buckets, and it is the instrument the retail device columns were read
# from, so a like-for-like emulator comparison has to come from here.
#
# Ticks are guest cpuGetTiming() deltas, so they do not depend on how fast the
# host runs. They DO depend on which melonDS is running: the repo fork models
# ARMv5 icache/dcache, stock melonDS does not. See emulators/README.md.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

# `pwsh -File script.ps1 -ExtraGlobals a,b,c` hands the whole list over as ONE
# literal string -- -File does no PowerShell parsing of argument values -- so
# [string[]] coerces it to a single element containing commas. The generated GDB
# printf then emits one %u for what GDB reads as three expressions and the run
# dies with "Wrong number of arguments for specified format-string" after the
# emulator has already reached the sample window. Re-split here so the direct
# call and the -File call behave the same; a symbol name can never contain a
# comma, so this is unambiguous.
$ExtraGlobals = @($ExtraGlobals |
    ForEach-Object { $_ -split ',' } |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -ne '' })
# Same normalisation for -MakeFlags, and for the same reason as the comment
# above: `-MakeFlags A=1,B=1` arrives as ONE string, which make would take as a
# single malformed variable rather than two.
$MakeFlags = @($MakeFlags |
    ForEach-Object { $_ -split ',' } |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -ne '' })
$PerFrameGlobals = @($PerFrameGlobals |
    ForEach-Object { $_ -split ',' } |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -ne '' })
# Same normalisation, same reason as the two lists above.
$SetGlobals = @($SetGlobals |
    ForEach-Object { $_ -split ',' } |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -ne '' })
foreach ($pair in $SetGlobals) {
    # GDB accepts aggregate field paths in `set var`, and a focused stage lab
    # needs to shorten a source countdown without adding a permanent mirror
    # global solely for the harness. Keep array/pointer expressions excluded;
    # dotted C field names are the only added grammar.
    if ($pair -notmatch '^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*)*\s*=\s*-?[0-9]+$') {
        throw "-SetGlobals expects name=value pairs; got '$pair'."
    }
}
# Poked at the first frame-complete marker, then the breakpoint is discarded so
# it cannot interact with the sampling breakpoints installed below.
$setGlobalLines = @(if ($SetGlobals.Count -gt 0) {
    'break ndsBattlePlayableFrameCompleteMarker'
    'continue'
    foreach ($pair in $SetGlobals) { "set var $pair" }
    foreach ($pair in $SetGlobals) {
        $n = ($pair -split '=')[0].Trim()
        "printf `"SETGLOBAL=$n,%u\n`", $n"
    }
    'delete'
})
# -PerStopGlobals needs it for the SAME reason, and did not have it: the three
# lists above were normalised when they were added and this one was added later
# (R2-07) without noticing the pattern. A single comma-joined string then
# reaches the symbol guard as ONE element, so every requested name is reported
# missing at once -- which reads as "the counters were never linked" and sends
# the next hour into the build instead of the harness. The tell is in the error
# text: the guard joins with ', ' and the failure printed bare commas.
$PerStopGlobals = @($PerStopGlobals |
    ForEach-Object { $_ -split ',' } |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -ne '' })
# -RingDump reads ONE ring of bucket words out of the ROM at a single stop
# ($ringBytes = bucketNames * 128 * 4). Per-frame globals are not in that ring
# -- they ride the per-frame printf, which -RingDump does not execute -- so the
# combination writes a rows CSV whose extra columns are all EMPTY, and exits 0.
# That happened on 2026-08-01 and cost a full 128-frame run plus the rebuild
# that preceded it; the artifact looked completely normal until the columns were
# read. Refuse it instead of producing a plausible empty result.
if ($RingDump -and ($PerFrameGlobals.Count -ne 0)) {
    throw ('-PerFrameGlobals cannot be combined with -RingDump: the ring holds ' +
        'bucket words only, so every requested column would be written empty. ' +
        'Drop -RingDump to take one stop per frame (raise -TimeoutSeconds; 128 ' +
        'stops does not fit the 900 s default).')
}

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'
# MUST match enum NDSTickHudBucket in include/nds/nds_startup.h, in order and in
# count -- this list sizes the ring read ($ringBytes = count * 128 * 4), so a
# mismatch does not fail, it returns ADJACENT MEMORY as plausible columns. Cycle
# 82 read an 11-bucket ROM with 14 names and got `named` at 220.4% of ALL and a
# 842,496-tick figure for a bucket the ROM did not have. Move this list in the
# same commit as the enum, never after it.
#
# SHDT/SWRM are SRC sub-buckets (cycle 85), appended after WORK so every existing
# index is unchanged. They are nested inside SRC and must stay out of every
# "named" total below, or the share double-counts them.
$bucketNames = @('ALL', 'FTR', 'STG', 'BG', 'AUD', 'HUD', 'SRC', 'MISC', 'OTHR',
                 'WAIT', 'WORK', 'SHDT', 'SWRM',
                 'GCRA', 'SCPU', 'SCAT', 'SPRM',
                 'SINT', 'SPHD', 'SPHC')
# GCRA/SCPU/SCAT/SPRM are the cycle-86 SBAS split, appended after the cycle-85
# pair on the same terms: sub-spans of SRC, so out of every "named" total.
# GCRA is gcRunAll -- the whole simulation; SCPU/SCAT/SPRM are three of the six
# per-fighter procs. The analyzer derives SOUT = SBAS - GCRA and
# SGCO = GCRA - SCPU - SCAT - SHDT - SPRM at zero byte cost.
#
# SINT/SPHD/SPHC are the cycle-92 SGCO split, appended again on the same terms.
# They are the OTHER three per-fighter procs (ftmanager.c:858-860): the interrupt
# proc (with SCPU nested inside it) and the two mutually exclusive physics/map
# arms. The analyzer derives SITR = SINT - SCPU and the non-fighter remainder
# SOBJ = GCRA - SINT - SPHD - SPHC - SCAT - SHDT - SPRM, so SGCO stops being a
# residual and stays derivable as SITR + SPHD + SPHC + SOBJ for regression.
$srcSubBuckets = @('SHDT', 'SWRM', 'GCRA', 'SCPU', 'SCAT', 'SPRM',
                   'SINT', 'SPHD', 'SPHC')
# Must match enum NDSTickHudNativeOwnerFallbackReason in include/nds/nds_startup.h.
$fallbackReasons = if ($FallbackCensus) {
    @('calls', 'eligible', 'animLock', 'selected', 'displayList',
      'materialCount', 'validate', 'matrices', 'materialPrep',
      'inputs', 'contract', 'postGx', 'begin',
      'animLoad', 'animResident') } else { @() }

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'tick-hud-buckets.gdb'
$gdbOut = Join-Path $temp 'tick-hud-buckets.gdb.out'
$gdbErr = Join-Path $temp 'tick-hud-buckets.gdb.err'
$emulatorOut = Join-Path $temp 'tick-hud-buckets.melonds.out'
$emulatorErr = Join-Path $temp 'tick-hud-buckets.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        # Census flags must reach `make`. This script's own header documents
        # that -ExtraGlobals reads such as gNdsTask75AssetLoadCount need
        # NDS_TASK75_LOAD_CENSUS=1 (and NDS_TASK68_FALLBACK_CENSUS=1), but the
        # build line never passed them -- so a run that asked for those globals
        # rebuilt the ROM WITHOUT them and then failed its own symbol guard.
        # Third harness in this campaign with the same defect (2026-07-31);
        # capture-sudden-death-entry.ps1 and soak-freeze-watch.ps1 were the
        # other two. Generic pass-through here so the next flag needs no edit.
        $makeArgs = @("TARGET=$target", "BUILD=$Build") + $MakeFlags
        make -C $root @makeArgs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required tick-HUD sample file is missing: $path"
        }
    }

    # Validate -ExtraGlobals against the ELF symbol table BEFORE launching
    # anything. A misspelled name is only caught by GDB, which happens after
    # melonDS has booted and run to the sample window -- so a typo costs a full
    # measurement run to discover. It cost two in one session before this check
    # existed. nm reads the same ELF GDB will, so agreement is guaranteed.
    # -PerFrameGlobals gets the identical guard, for the identical reason.
    # ALWAYS-READ SYMBOLS GET THE SAME GUARD, because the caller-supplied lists
    # are not the only way a name can be wrong. This block used to run ONLY when
    # -ExtraGlobals or -PerFrameGlobals were passed, so a plain
    # `-RingDump` invocation -- which reads four symbols unconditionally --
    # validated nothing at all and the guard was inert exactly when it was the
    # only check available. Found 2026-08-03 while diagnosing a run that
    # exhausted its budget; these four were in fact present, but proving that
    # took a separate nm pass the harness should have done itself.
    $alwaysRead = @('ndsBattlePlayableFrameCompleteMarker',
                    'gNdsBattlePlayablePacingPresentedFrames',
                    'sBattleTickHudRing', 'sBattleTickHudRingHead',
                    'sBattleTickHudRingCount')
    if (($ExtraGlobals.Count + $PerFrameGlobals.Count + $PerStopGlobals.Count +
         $alwaysRead.Count) -ne 0) {
        $nm = Join-Path (Split-Path -Parent $Gdb) 'arm-none-eabi-nm.exe'
        if (Test-Path -LiteralPath $nm -PathType Leaf) {
            $symbols = [System.Collections.Generic.HashSet[string]]::new(
                [string[]](& $nm --defined-only $elf |
                    ForEach-Object { ($_ -split '\s+')[-1] }))
            foreach ($pair in @(
                @{ n = '-ExtraGlobals';    v = $ExtraGlobals },
                @{ n = '-PerFrameGlobals'; v = $PerFrameGlobals },
                @{ n = '-PerStopGlobals';  v = $PerStopGlobals },
                @{ n = 'always-read';      v = $alwaysRead })) {
                # Validate the BASE identifier so an array element is readable:
                # `gNdsTickHudNativeOwnerFallbackByReason[13]` is a legal GDB
                # expression but never a symbol name, and R2-07 L5 needs exactly
                # that (animLoad is index 13 of the fallback reason array).
                # A typo in the base is still caught, which is what the guard is
                # for; a bad subscript is a GDB error, not a silent zero.
                #
                # STRUCT MEMBERS TOO, for the identical reason and because the
                # subscript case alone was not enough: the collision diagnostics
                # are one struct (`gNdsCollisionRuntimeDiagnostics`) with ~30
                # fields, so every per-frame collision read is `base.field`,
                # which is a legal GDB expression and never a symbol name. On
                # 2026-08-01 that rejected a whole 128-frame attribution run
                # after its ROM had already been built. Strip both suffixes.
                $missing = @($pair.v | Where-Object {
                    -not $symbols.Contains(($_ -replace '[\[.].*$', '')) })
                if ($missing.Count -ne 0) {
                    throw ("$($pair.n) names not defined in $([System.IO.Path]::GetFileName($elf)): " +
                        "$($missing -join ', '). A name that exists but is never written reads 0, " +
                        "which is a real measurement; a name that does not exist is a typo. " +
                        'If the name lives behind a census flag, pass it with -MakeFlags.')
                }
            }
        }
    }

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    # One stop per presented iteration from boot, gated host-side until
    # StartFrame; this matches how the renderer benchmark reaches its own start
    # frame, and keeps every sample on a real settled combat frame.
    # Both loop bounds and the printf arity come off $bucketNames so adding a
    # bucket is a one-line change. Task 66 added two and the second hardcoded
    # bound was missed, which silently dropped WAIT and WORK from the table
    # while every other check still passed.
    # Task 67: the native-owner fallback counters ride along on the same stop.
    # They are cumulative, so the per-frame count is a difference between
    # consecutive samples -- which is the point, because it lines the fallback
    # up against the very frame whose WORK spiked instead of leaving the two to
    # be correlated across separate runs.
    #
    # Only the total rides per frame. Reading the four per-reason counters here
    # too meant five extra GDB round-trips on every stop, which stretched the
    # stop far enough that the game's own frame pacing skipped and repeated
    # presented frames -- the sampler's uniqueness check caught it. The reason
    # breakdown is a run-level question, so it is read once at the end instead.
    $fallbackFields = if ($FallbackCensus -and (-not $RingDump)) {
        @('gNdsTickHudNativeOwnerFallbackCount') } else { @() }
    # Per-frame globals ride the SAME printf as the buckets, which is the whole
    # point: one read per presented frame, at the frame's own sample point, so
    # the value is attributable to that frame.
    $sampleFields = ((0..($bucketNames.Count - 1) |
        ForEach-Object { "gNdsTickHudBuckets[$_]" }) + $fallbackFields +
        $PerFrameGlobals) -join ', '
    $extraLine = if ($ExtraGlobals.Count -ne 0) {
        "printf `"TICKEXTRA=$((, '%u' * $ExtraGlobals.Count) -join ',')\n`", " +
            ($ExtraGlobals -join ', ')
    } else { $null }
    $sampleColumnCount = $bucketNames.Count + $fallbackFields.Count +
        $PerFrameGlobals.Count
    $tickHudFormat = (, '%u' * ($sampleColumnCount + 1)) -join ','
    $ringPath = Join-Path $temp 'tick-hud-ring.bin'
    $fbRingPath = Join-Path $temp 'tick-hud-fallback-ring.bin'
    $ringWindow = 128
    $ringBytes = $bucketNames.Count * $ringWindow * 4
    $ringEndFrame = $StartFrame + $Samples
    # REPEATED RING STOPS. One stop yields at most $ringWindow samples, so a
    # whole-match window needs several, each taken before the ring can wrap.
    # The last target is always $ringEndFrame so the requested window still ends
    # exactly where a single-stop run would have ended.
    $ringStopTargets = @()
    if ($RingDump) {
        if ($Samples -le $ringWindow) {
            $ringStopTargets = @($ringEndFrame)
        } else {
            $t = $StartFrame + $RingStopStride
            while ($t -lt $ringEndFrame) {
                $ringStopTargets += $t
                $t += $RingStopStride
            }
            $ringStopTargets += $ringEndFrame
        }
        if ($FallbackCensus -and ($ringStopTargets.Count -gt 1)) {
            # The fallback ring shares the bucket ring's index, so stitching it
            # would need the same per-stop treatment. Rather than half-support
            # it and hand back a column that is quietly wrong for every stop
            # after the first, refuse.
            throw ('-FallbackCensus is single-stop only, but this run needs ' +
                "$($ringStopTargets.Count) ring stops for $Samples samples. " +
                "Lower -Samples to $ringWindow or drop -FallbackCensus.")
        }
    }
    $ringStopPaths = @(for ($i = 0; $i -lt $ringStopTargets.Count; $i++) {
        Join-Path $temp "tick-hud-ring-$i.bin"
    })
    # One unrolled stop per target rather than a GDB-side loop: the breakpoint
    # condition changes per stop, and `delete` + a fresh `break` is the same
    # shape the -FallbackCensus baseline stop already uses and is known to work
    # in batch mode.
    $ringStopLines = @(for ($i = 0; $i -lt $ringStopTargets.Count; $i++) {
        'break ndsBattlePlayableFrameCompleteMarker'
        'commands'
        'silent'
        "if gNdsBattlePlayablePacingPresentedFrames < $($ringStopTargets[$i])"
        'continue'
        'end'
        'end'
        'continue'
        ('printf "TICKRING' + $i + '=%u,%u,%u\n", ' +
            'sBattleTickHudRingHead, sBattleTickHudRingCount, ' +
            'gNdsBattlePlayablePacingPresentedFrames')
        $(if ($PerStopGlobals.Count -ne 0) {
            "printf `"TICKSTOP$i=$((, '%u' * $PerStopGlobals.Count) -join ',')\n`", " +
                ($PerStopGlobals -join ', ')
        })
        # dump binary memory splits its arguments on whitespace, so the bounds
        # have to be single tokens -- a cast like "(char *)&ring" parses as two
        # arguments and fails. Index one past the last bucket row for the end
        # bound, which is the same form the Task 34 stage-stream dump uses.
        ("dump binary memory $($ringStopPaths[$i]) " +
            "&sBattleTickHudRing[0][0] " +
            "&sBattleTickHudRing[$($bucketNames.Count)][0]")
        'delete'
    })
    $gdbLines = if ($RingDump) {
        @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        $setGlobalLines,
        # The fallback counters run from boot, so a single read at the end would
        # charge the census window with every fallback taken during boot, the
        # menu and the seeding presents. Stop once at the start frame to take a
        # baseline and difference it. Two stops, not 128.
        $(if ($FallbackCensus) { 'break ndsBattlePlayableFrameCompleteMarker' }),
        $(if ($FallbackCensus) { 'commands' }),
        $(if ($FallbackCensus) { 'silent' }),
        $(if ($FallbackCensus) {
            "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame" }),
        $(if ($FallbackCensus) { 'continue' }),
        $(if ($FallbackCensus) { 'end' }),
        $(if ($FallbackCensus) { 'end' }),
        $(if ($FallbackCensus) { 'continue' }),
        $(if ($FallbackCensus) {
            "printf `"TICKFB0=$((, '%u' * $fallbackReasons.Count) -join ',')\n`", " +
                ((0..($fallbackReasons.Count - 1) | ForEach-Object {
                    "gNdsTickHudNativeOwnerFallbackByReason[$_]" }) -join ', ')
        }),
        $(if ($FallbackCensus) { 'delete' })
        ) + $ringStopLines + @(
        $(if ($FallbackCensus) {
            "dump binary memory $fbRingPath &sBattleTickHudFallbackRing[0] " +
                "&sBattleTickHudFallbackRing[$ringWindow]" }),
        ('printf "TICKVBI=%u,%u,%u,%u,%u\n", ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[2], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[3], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[4], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[5], ' +
            'gNdsBattlePlayablePacingPresentIntervalMax'),
        'printf "TICKSLIP=%u\n", gNdsBattlePlayablePacingCadenceViolationCount',
        $(if ($FallbackCensus) {
            "printf `"TICKFB=$((, '%u' * $fallbackReasons.Count) -join ',')\n`", " +
                ((0..($fallbackReasons.Count - 1) | ForEach-Object {
                    "gNdsTickHudNativeOwnerFallbackByReason[$_]" }) -join ', ')
        }),
        $extraLine,
        'detach')
    } else {
        @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        $setGlobalLines,
        'set $tick_samples = 0',
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        ("printf `"TICKHUD=$tickHudFormat\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, $sampleFields"),
        'set $tick_samples = $tick_samples + 1',
        ('if $tick_samples < {0}' -f $Samples),
        'continue',
        'end',
        'end',
        'continue',
        ('printf "TICKVBI=%u,%u,%u,%u,%u\n", ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[2], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[3], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[4], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[5], ' +
            'gNdsBattlePlayablePacingPresentIntervalMax'),
        'printf "TICKSLIP=%u\n", gNdsBattlePlayablePacingCadenceViolationCount',
        $(if ($FallbackCensus) {
            "printf `"TICKFB=$((, '%u' * $fallbackReasons.Count) -join ',')\n`", " +
                ((0..($fallbackReasons.Count - 1) | ForEach-Object {
                    "gNdsTickHudNativeOwnerFallbackByReason[$_]" }) -join ', ')
        }),
        $extraLine,
        'detach')
    }
    # Drop the nulls the conditional lines leave behind. A null writes a blank
    # line, and a blank line in a GDB script re-executes the previous command --
    # which next to a 'continue' would silently run the emulator on past the
    # frame the dump was supposed to be taken at.
    # Flatten one extra level before filtering. -SetGlobals contributes a
    # NESTED array, and a nested array piped straight into Where-Object is
    # emitted as a single object that stringifies to one space-joined line --
    # "break ... continue set var ... delete" -- which gdb rejects as a
    # malformed breakpoint location. In -batch that error is printed and
    # execution carries on, so the run still reaches its window and still
    # prints a full, plausible bucket table with the poke silently unapplied.
    # It was caught only because the arm carried its own engagement counters
    # and they read 0 where the census said they must read 10,336.
    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | ForEach-Object { $_ } |
            Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        # A timeout here is almost never "too slow" -- it is the battle present
        # loop having stopped advancing, which this harness reported as a
        # generic timeout for eleven cycles while the guest was demonstrably
        # alive (2026-08-03: counter frozen at 195 while gNdsFrameCounter kept
        # climbing). The breakpoint script is silent until the target frame, so
        # the stdout carries no frame number; read the counter once from the
        # still-running emulator and name it. Confined to the timeout branch:
        # the measurement path is untouched whether or not this read works.
        # TWO reads, seconds apart, not one. A single counter value cannot tell
        # "stalled" from "slow", and this harness reported both as the same
        # generic timeout: the Task 56 strip arm blew 900 s twice and then
        # 2400 s, looked like a slow run all three times across three days, and
        # was in fact never advancing at all. The second read is what makes the
        # difference a measurement instead of a guess.
        $readPresented = {
            $value = $null
            try {
                $probeScript = Join-Path $temp 'tickhud-frozen-probe.gdb'
                Set-Content -LiteralPath $probeScript -Encoding ASCII -Value @(
                    'set pagination off', 'set confirm off',
                    'set remotetimeout 10',
                    "target remote 127.0.0.1:$($context.GdbPort)",
                    ('printf "FROZENAT=%u\n", ' +
                        'gNdsBattlePlayablePacingPresentedFrames'),
                    'detach', 'quit')
                $probeOut = Join-Path $temp 'tickhud-frozen-probe.out'
                $probe = Start-Process -FilePath $Gdb `
                    -ArgumentList @('-q', '-batch', '-x', $probeScript, $elf) `
                    -WorkingDirectory $root -RedirectStandardOutput $probeOut `
                    -RedirectStandardError `
                        (Join-Path $temp 'tickhud-frozen-probe.err') `
                    -WindowStyle Hidden -PassThru
                if ($probe.WaitForExit(30000)) {
                    $m = [regex]::Match((Get-Content $probeOut -Raw),
                        'FROZENAT=(\d+)')
                    if ($m.Success) { $value = [uint64]$m.Groups[1].Value }
                } else { Stop-Process -Id $probe.Id -Force }
            } catch { }
            return $value
        }
        $liveA = & $readPresented
        Start-Sleep -Seconds 6
        $liveB = & $readPresented
        # How far the run actually got. Every completed ring stop already
        # printed its own frame number, so the stdout carries the progress the
        # breakpoint script is otherwise silent about.
        $partial = [string](Get-Content $gdbOut -Raw -ErrorAction SilentlyContinue)
        $reached = @([regex]::Matches($partial, 'TICKRING(\d+)=\d+,\d+,(\d+)'))
        $progress = if ($reached.Count -ne 0) {
            $last = $reached[$reached.Count - 1]
            ("reached ring stop $($last.Groups[1].Value) of " +
                "$($ringStopTargets.Count) at presented frame " +
                "$($last.Groups[2].Value)")
        } else {
            "never reached ring stop 0 (target frame $($ringStopTargets[0]))"
        }
        if (($null -ne $liveA) -and ($null -ne $liveB)) {
            if ($liveB -eq $liveA) {
                throw ("Tick-HUD run STALLED after ${TimeoutSeconds}s: " +
                    'gNdsBattlePlayablePacingPresentedFrames read ' +
                    "$liveA twice, 6 s apart, so the battle present loop is " +
                    "NOT advancing. $progress; this run needs " +
                    "$ringEndFrame. This is a stall to diagnose (check GXSTAT " +
                    'for a geometry-engine hang), NOT a slow run to re-time ' +
                    'with a longer -TimeoutSeconds.')
            }
            $rate = ($liveB - $liveA) / 6.0
            $remaining = if ($ringEndFrame -gt $liveB) {
                $ringEndFrame - $liveB } else { 0 }
            $eta = if ($rate -gt 0) {
                '{0:N0}' -f ($remaining / $rate) } else { 'unknown' }
            throw ("Tick-HUD run TOO SLOW after ${TimeoutSeconds}s: the guest " +
                "IS advancing ($liveA -> $liveB in 6 s, " +
                ('{0:N1}' -f $rate) + " presented frames/s) but is still " +
                "$remaining frames short of $ringEndFrame. $progress. " +
                "Re-run with -TimeoutSeconds about $eta s longer; this is not " +
                'a stall.')
        }
        throw ("Tick-HUD GDB run exceeded ${TimeoutSeconds}s before $Samples " +
            "samples, and the liveness probe could not read " +
            "gNdsBattlePlayablePacingPresentedFrames, so stalled-vs-slow is " +
            "UNRESOLVED. $progress.")
    }
    if ($gdbProcess.ExitCode -ne 0) {
        throw "Tick-HUD GDB run failed: $(Get-Content $gdbErr -Raw)"
    }

    $output = Get-Content $gdbOut -Raw
    $rows = if ($RingDump) {
        # Task 70: the per-frame fallback deltas share the ring index, so they
        # append as one more column and every frame carries its own count.
        # Single-stop only -- the guard where $ringStopTargets is built refuses
        # the combination rather than emitting a quietly wrong column.
        $fbRaw = if ($FallbackCensus) {
            if (-not (Test-Path -LiteralPath $fbRingPath -PathType Leaf)) {
                throw ("Fallback ring dump wrote no file at $fbRingPath. The " +
                    'ROM must be built NDS_TASK68_FALLBACK_CENSUS=1.')
            }
            $bytes = [System.IO.File]::ReadAllBytes($fbRingPath)
            if ($bytes.Length -ne ($ringWindow * 4)) {
                throw ("Fallback ring dump is $($bytes.Length) bytes, expected " +
                    "$($ringWindow * 4).")
            }
            $bytes
        } else { $null }
        $stitched = New-Object 'System.Collections.Generic.List[uint64[]]'
        $ringStopSkews = New-Object 'System.Collections.Generic.List[object]'
        # Where each stop's rows begin in $stitched, and the skew that stop was
        # read with. Together these let the duplicate-frame guard below tell a
        # LABELLING collision at a stop seam from a bad read, which it could not
        # do before and which cost two whole-match runs on 2026-08-11.
        $stopRowStart = New-Object 'System.Collections.Generic.List[int]'
        $stopSkew = New-Object 'System.Collections.Generic.List[int]'
        $ringStopReads = New-Object 'System.Collections.Generic.List[object]'
        $prevStopValues = @(0..([Math]::Max($PerStopGlobals.Count - 1, 0)) |
            ForEach-Object { [uint64]0 })
        $prevHead = -1
        $prevFrame = [uint64]0
        for ($k = 0; $k -lt $ringStopTargets.Count; $k++) {
            $m = [regex]::Match($output, "TICKRING$k=([0-9]+),([0-9]+),([0-9]+)")
            if (-not $m.Success) {
                throw ("Tick-HUD ring dump produced no TICKRING$k line " +
                    "(stop $($k + 1) of $($ringStopTargets.Count), target " +
                    "frame $($ringStopTargets[$k])). GDB output:`n$output")
            }
            if ($PerStopGlobals.Count -ne 0) {
                $sm = [regex]::Match($output,
                    "TICKSTOP$k=([0-9]+(?:,[0-9]+){$($PerStopGlobals.Count - 1)})")
                if (-not $sm.Success) {
                    throw ("Tick-HUD stop $k printed no TICKSTOP$k line for the " +
                        "$($PerStopGlobals.Count) requested -PerStopGlobals. " +
                        "GDB output:`n$output")
                }
                $vals = @($sm.Groups[1].Value -split ',' |
                    ForEach-Object { [uint64]$_ })
                $rec = [ordered]@{
                    stop = $k
                    frame = [uint64]$m.Groups[3].Value
                    fromFrame = if ($k -eq 0) { [uint64]$StartFrame }
                                else { $prevFrame }
                }
                for ($g = 0; $g -lt $PerStopGlobals.Count; $g++) {
                    # Cumulative value AND the delta since the previous stop.
                    # The counters this is aimed at (dead frames, rebirth
                    # phases, hit-spark spawns) only count upward, so the delta
                    # is the events inside this stop's window and is the column
                    # a KO-versus-tail question actually needs.
                    $rec[$PerStopGlobals[$g]] = $vals[$g]
                    $rec[($PerStopGlobals[$g] + 'Delta')] = if ($k -eq 0) {
                        [uint64]0
                    } elseif ($vals[$g] -ge $prevStopValues[$g]) {
                        $vals[$g] - $prevStopValues[$g]
                    } else { [uint64]0 }
                }
                $ringStopReads.Add([pscustomobject]$rec)
                $prevStopValues = $vals
            }
            $head = [int]$m.Groups[1].Value
            $count = [int]$m.Groups[2].Value
            $frame = [uint64]$m.Groups[3].Value
            if (-not (Test-Path -LiteralPath $ringStopPaths[$k] -PathType Leaf)) {
                throw "Tick-HUD ring dump wrote no file at $($ringStopPaths[$k])."
            }
            $raw = [System.IO.File]::ReadAllBytes($ringStopPaths[$k])
            if ($raw.Length -ne $ringBytes) {
                throw ("Tick-HUD ring dump $k is $($raw.Length) bytes, expected " +
                    "$ringBytes ($($bucketNames.Count) buckets x $ringWindow).")
            }
            # How many ring slots this stop contributes, and whether that number
            # can be trusted. The ring index is the sample identity; the
            # presented-frame counter is only a label and can advance more
            # slowly than the ring (one presented frame can carry two
            # iterations), so iterations >= presented frames ALWAYS. That
            # inequality is what turns a silent wrap into a hard failure.
            $newCount = if ($k -eq 0) {
                [Math]::Min($count, $ringWindow)
            } else {
                $delta = ($head - $prevHead) % $ringWindow
                if ($delta -lt 0) { $delta += $ringWindow }
                $presentedDelta = [int]($frame - $prevFrame)
                if ($presentedDelta -gt $ringWindow) {
                    throw ("Tick-HUD ring stop $k advanced $presentedDelta " +
                        "presented frames (frames $prevFrame..$frame), more " +
                        "than the $ringWindow-entry ring can hold, so at least " +
                        "$($presentedDelta - $ringWindow) frames were " +
                        'overwritten before they were read. Lower ' +
                        '-RingStopStride.')
                }
                if (($delta -eq 0) -and ($presentedDelta -gt 0)) {
                    throw ("Tick-HUD ring stop $k read an unchanged ring head " +
                        "($head) after $presentedDelta presented frames " +
                        "(frames $prevFrame..$frame). That is either a stalled " +
                        'ring or an exact 128-slot wrap, and the two are ' +
                        'indistinguishable from the head alone. Lower ' +
                        '-RingStopStride.')
                }
                # RING SLOTS vs PRESENTED FRAMES ARE NOT THE SAME COUNT, and
                # this records the difference instead of asserting one.
                #
                # Measured 2026-08-04 on the baseline ROM: a 64-presented-frame
                # span (536..600) advanced the ring 63 slots. The stop is taken
                # at ndsBattlePlayableFrameCompleteMarker, which is not the same
                # point in the iteration as the ndsPlatformTickHudSample call
                # that advances the ring, so the two counters are read at a
                # fixed skew that does not have to be zero -- and a presented
                # frame can carry two iterations, which pushes the other way.
                #
                # Failing on a small mismatch would be asserting an invariant
                # this harness has not established. Failing on NO mismatch would
                # hide a wrap. So: hard-fail only where loss is certain
                # (presentedDelta > ringWindow, checked above, and delta == 0),
                # and carry the per-stop skew out in the JSON so a series that
                # drifted is visible in the evidence rather than argued about.
                $skew = $presentedDelta - $delta
                $ringStopSkews.Add([pscustomobject]@{
                    stop = $k
                    targetFrame = $ringStopTargets[$k]
                    frame = [uint64]$frame
                    head = $head
                    ringSlots = $delta
                    presentedFrames = $presentedDelta
                    skew = $skew
                })
                $delta
            }
            # Read the skew back out of the record rather than off $skew: that
            # variable is not assigned on the k == 0 pass, and PowerShell's `if`
            # does not open a scope, so it would silently carry the PREVIOUS
            # stop's value into the first stop.
            $stopRowStart.Add($stitched.Count)
            $stopSkew.Add($(if ($k -eq 0) { 0 } else {
                [int]$ringStopSkews[$ringStopSkews.Count - 1].skew }))
            # Oldest-first walk of just this stop's new slots, ending at $head.
            $start = (($head - $newCount) % $ringWindow)
            if ($start -lt 0) { $start += $ringWindow }
            for ($j = 0; $j -lt $newCount; $j++) {
                $slot = ($start + $j) % $ringWindow
                $row = New-Object 'System.Collections.Generic.List[uint64]'
                $row.Add([uint64]($frame - ($newCount - 1 - $j)))
                for ($b = 0; $b -lt $bucketNames.Count; $b++) {
                    $row.Add([BitConverter]::ToUInt32($raw,
                        ((($b * $ringWindow) + $slot) * 4)))
                }
                if ($null -ne $fbRaw) {
                    $row.Add([BitConverter]::ToUInt32($fbRaw, ($slot * 4)))
                }
                $stitched.Add([uint64[]]$row.ToArray())
            }
            $prevHead = $head
            $prevFrame = $frame
        }
        if ($stitched.Count -lt $Samples) {
            throw ("Tick-HUD ring stitched $($stitched.Count) iterations from " +
                "$($ringStopTargets.Count) stop(s), fewer than the $Samples " +
                'requested. Raise -StartFrame or lower -Samples.')
        }
        # Explicit index walk, not `Select-Object -Last`: the pipeline wraps
        # each uint64[] row in a PSObject and the downstream percentile code
        # indexes the rows directly, which then fails with "Argument types do
        # not match" AFTER the CSV has already been written -- i.e. it looks
        # like a stats bug, not a plumbing one. The comma operator keeps each
        # row from being unrolled into the result array.
        $firstRow = $stitched.Count - $Samples
        @(for ($r = $firstRow; $r -lt $stitched.Count; $r++) { , $stitched[$r] })
    } else {
        @([regex]::Matches($output,
            "TICKHUD=([0-9]+(?:,[0-9]+){$sampleColumnCount})") | ForEach-Object {
                , [uint64[]]($_.Groups[1].Value -split ',')
            })
    }
    if ($rows.Count -ne $Samples) {
        throw ("Tick-HUD run produced $($rows.Count) of $Samples samples. " +
            "GDB output:`n$output")
    }
    $frames = @($rows | ForEach-Object { $_[0] })
    if (($frames | Select-Object -Unique).Count -ne $rows.Count) {
        # The guest cannot produce this. ndsBattlePlayablePresentFrame increments
        # the counter and ndsBattlePlayableFinalizePresentedIteration calls the
        # marker immediately after, with no branch between them in the settled
        # loop -- so two marker hits at one counter value is not something the
        # game did, it is something the read saw. Report enough to tell the two
        # candidate causes apart instead of just failing:
        #   identical payload -> a stale read (GDB reads the emulated memory
        #     array directly, so a dirty ARM9 dcache line still holding the new
        #     value reads back as the old one)
        #   differing payload -> the loop really did iterate twice, and the
        #     frame counter is not the per-iteration identity we assumed.
        $dupIndex = @(1..($rows.Count - 1) | Where-Object {
            $rows[$_][0] -eq $rows[$_ - 1][0] })
        $detail = @($dupIndex | Select-Object -First 4 | ForEach-Object {
            $prev = $rows[$_ - 1] -join ','
            $cur = $rows[$_] -join ','
            "  [{0}] {1}`n  [{2}] {3}  payload {4}" -f
                ($_ - 1), $prev, $_, $cur,
                $(if ($prev -eq $cur) { 'IDENTICAL (stale read)' }
                  else { 'DIFFERS (real second iteration)' })
        })
        # A STALE READ IS ALWAYS FATAL; a real second iteration need not be.
        # The two are distinguished right above by payload equality, and they
        # mean opposite things: identical payload is the instrument misreading
        # the same frame twice (a defect in the measurement, never acceptable),
        # while a differing payload is the 60 Hz loop genuinely iterating twice
        # inside one presented frame. A slower build -- a census ROM, say --
        # shifts pacing enough for both iterations to land in separate samples,
        # which is why the board says a census build is "not pacing-comparable"
        # and is to be read for the load-frame SET only. Refusing to write the
        # rows in that case throws away the very data the run was for.
        $anyIdentical = @($dupIndex | Where-Object {
            ($rows[$_] -join ',') -eq ($rows[$_ - 1] -join ',') }).Count -gt 0
        # WHICH DUPLICATES ARE ARITHMETIC RATHER THAN MEASUREMENT.
        #
        # The stitcher labels each row by counting BACKWARD from the
        # presented-frame counter read at its stop, so the first row of stop k
        # is labelled $frame - $delta + 1 where $delta is that stop's RING
        # slots. The stop separately records skew = presentedFrames - ringSlots
        # and this harness deliberately refuses to assert skew == 0 (see the
        # comment where it is computed). Put those together: skew == -1 makes
        # `$frame - $delta + 1` equal the previous stop's last label exactly,
        # and in general a stop with skew < 0 collides on its first -skew rows.
        # The collision is produced by the labelling arithmetic; the ring slots
        # underneath it are distinct real iterations.
        #
        # This does NOT soften the guard. An identical payload still fails
        # unconditionally, and so does any duplicate away from a seam -- which
        # is what caught the cycle-118 vertex-memo defect, whose four duplicates
        # included one that was not at a stop boundary. It only stops the
        # harness from reporting its own labelling as a suspected match change,
        # which on 2026-08-11 cost two whole-match runs before the pattern (all
        # five duplicates at exact multiples of -RingStopStride) was noticed.
        $seamIndex = @(if ($RingDump) {
            for ($k = 1; $k -lt $stopRowStart.Count; $k++) {
                for ($n = 0; $n -lt (-$stopSkew[$k]); $n++) {
                    $stopRowStart[$k] + $n - $firstRow
                }
            }
        })
        $unexplained = @($dupIndex | Where-Object { $seamIndex -notcontains $_ })
        if ((-not $anyIdentical) -and ($unexplained.Count -eq 0)) {
            Write-Warning ("Tick-HUD samples repeated a presented frame " +
                "($($dupIndex.Count) of $($rows.Count)). EVERY duplicate is a " +
                'ring-stop seam whose recorded skew explains the label ' +
                'collision, and no payload repeats, so these are distinct real ' +
                'iterations that share a frame LABEL. Percentiles are over the ' +
                'iterations and stand; the frame ids at those seams do not. ' +
                "Seam rows: $($dupIndex -join ',').")
        } elseif ($AllowRepeatedFrames -and (-not $anyIdentical)) {
            Write-Warning ("Tick-HUD samples repeated a presented frame " +
                "($($dupIndex.Count) of $($rows.Count)), every duplicate a REAL " +
                'second iteration. Continuing because -AllowRepeatedFrames was ' +
                'given. These rows are NOT pacing-comparable: use them for the ' +
                'per-frame SET (which frames did what), never for P50/P95.')
        } else {
            throw ("Tick-HUD samples repeated a presented frame " +
                "($($dupIndex.Count) of $($rows.Count)):`n$($frames -join ',')`n" +
                "columns: frame,$($bucketNames -join ',')" +
                "$(if ($fallbackFields) { ',fbTotal' })`n" +
                ($detail -join "`n") +
                "$(if ($unexplained.Count -ne 0) { "`n$($unexplained.Count) of " +
                    "$($dupIndex.Count) duplicates are NOT at a ring-stop seam " +
                    "(rows $($unexplained -join ',')); a seam collision is " +
                    'explained by that stop`s recorded negative skew and these ' +
                    'are not, so the change altered the match rather than the ' +
                    'labelling. Diff this set against a re-run before reaching ' +
                    'for -AllowRepeatedFrames.' })" +
                "$(if ($anyIdentical) { "`nAt least one duplicate is IDENTICAL " +
                    '(stale read) -- -AllowRepeatedFrames will NOT suppress that.' })")
        }
    }
    if ([uint64]$frames[0] -lt [uint64]$StartFrame) {
        throw "Tick-HUD sampling began at frame $($frames[0]), before $StartFrame."
    }

    if (-not [string]::IsNullOrWhiteSpace($RowsCsv)) {
        $workCsvIndex = [array]::IndexOf($bucketNames, 'WORK') + 1
        $hudCsvIndex = [array]::IndexOf($bucketNames, 'HUD') + 1
        # Per-frame globals become trailing columns, named after the symbol so a
        # reader never has to guess which is which. They sit after WORK-H so the
        # existing column order -- which other tooling and every prior rows CSV
        # depend on -- is unchanged.
        # -FallbackCensus was ALREADY sampled per frame (it rides $sampleFields
        # with the buckets) and was then dropped at write time -- the same shape
        # as -ExtraGlobals, data collected and discarded. R2-07 L4 needs exactly
        # this column: which frames fell off the native fighter owner.
        $csv = @(,('frame,' + ($bucketNames -join ',') + ',WORK-H' +
            $(if ($fallbackFields.Count) { ',fbTotal' }) +
            $(if ($PerFrameGlobals.Count) { ',' + ($PerFrameGlobals -join ',') })))
        $fallbackBase = $bucketNames.Count + 1
        $perFrameBase = $bucketNames.Count + $fallbackFields.Count + 1
        foreach ($row in $rows) {
            $cells = @($row[0])
            for ($b = 0; $b -lt $bucketNames.Count; $b++) {
                $cells += $row[$b + 1]
            }
            $cells += ([uint64]$row[$workCsvIndex] - [uint64]$row[$hudCsvIndex])
            for ($fb = 0; $fb -lt $fallbackFields.Count; $fb++) {
                $cells += $row[$fallbackBase + $fb]
            }
            for ($g = 0; $g -lt $PerFrameGlobals.Count; $g++) {
                $cells += $row[$perFrameBase + $g]
            }
            $csv += ($cells -join ',')
        }
        # Same two lines -JsonOut has carried since it was added, and they
        # belong here for a stronger reason: this write happens BEFORE the
        # percentiles are computed at all, so a missing parent directory does
        # not lose a file -- it throws away the whole run, after the emulator
        # has already spent its ~7 minutes, and prints a path error rather than
        # a measurement. -JsonOut's own guard runs 370 lines later and cannot
        # save it.
        $rowsDir = Split-Path -Parent $RowsCsv
        if ($rowsDir) { New-Item -ItemType Directory -Force -Path $rowsDir | Out-Null }
        Set-Content -LiteralPath $RowsCsv -Value $csv -Encoding ascii
        Write-Host "Wrote $RowsCsv"
    }

    $stats = @(0..($bucketNames.Count - 1) | ForEach-Object {
        $bucket = $_
        $values = @($rows | ForEach-Object { $_[$bucket + 1] })
        $sorted = @($values | Sort-Object)
        [PSCustomObject]@{
            bucket = $bucketNames[$bucket]
            mean = [uint64][Math]::Round((
                $values | Measure-Object -Average).Average, 0)
            p50 = [uint64]$sorted[[int][Math]::Floor(($sorted.Count - 1) * 0.50)]
            p95 = [uint64]$sorted[[int][Math]::Floor(($sorted.Count - 1) * 0.95)]
            min = [uint64]($values | Measure-Object -Minimum).Minimum
            max = [uint64]($values | Measure-Object -Maximum).Maximum
        }
    })

    # WORK-H: work with the tick HUD's own console render taken back out.
    #
    # This ROM exists to be measured, and the measuring costs something: the
    # text HUD redraws about twice a second and each redraw is worth a few
    # hundred thousand ticks, which is why HUD's spread is over 300x. That
    # barely moves a mean but it lands squarely on a P95 -- and P95 is the
    # statistic PROJECT_GOAL.md gates the milestone on. Subtracting per sample
    # (not subtracting one percentile from another) gives the P95 the published
    # profile-0 ROM would have, since it carries no tick HUD at all.
    $workIndex = [array]::IndexOf($bucketNames, 'WORK') + 1
    $hudIndex = [array]::IndexOf($bucketNames, 'HUD') + 1
    # HUD is normally a part of WORK, so the difference is normally positive.
    # It is NOT positive on a torn row: when a presented frame runs a second
    # logic iteration, a sample can catch WORK already reset for the new
    # iteration while HUD still carries the previous redraw, and HUD's redraws
    # are worth a few hundred thousand ticks each. `[uint64]a - [uint64]b` then
    # throws "Arithmetic operation resulted in an overflow" from inside
    # Measure-Object, roughly twenty minutes into a run, naming a number
    # ("-1047808") that appears nowhere in the instrument -- so the failure
    # reads as a corrupt ROM rather than as the duplicate-sample warning printed
    # further up. It cost a full 1600-sample run on a ROM whose only sin was
    # being a one-CPU build, where cheap logic makes second iterations common
    # (237 of 1600 rather than the usual 2 to 4).
    #
    # Drop those rows and say how many. They are not pacing-comparable anyway,
    # which is exactly what the -AllowRepeatedFrames warning already says.
    # A plain foreach, not ForEach-Object: the pipeline form runs its block in a
    # child scope, so the torn counter would have to be $script:-qualified and
    # would silently read 0 if this block ever moves inside a function.
    $workNoHudTorn = 0
    $workNoHudList = New-Object 'System.Collections.Generic.List[uint64]'
    foreach ($r in $rows) {
        $w = [uint64]$r[$workIndex]
        $h = [uint64]$r[$hudIndex]
        if ($h -gt $w) { $workNoHudTorn++ } else { $workNoHudList.Add($w - $h) }
    }
    $workNoHud = @($workNoHudList)
    if ($workNoHud.Count -eq 0) {
        throw ("Every one of $($rows.Count) sampled rows had HUD > WORK, so " +
            'WORK-H has no valid sample. That is a torn-read symptom, not a ' +
            'measurement: re-run with a build whose pacing gives one logic ' +
            'iteration per presented frame (the both-CPU arm).')
    }
    if ($workNoHudTorn -ne 0) {
        Write-Warning ("WORK-H dropped $workNoHudTorn of $($rows.Count) rows " +
            'where HUD exceeded WORK (a second logic iteration sampled ' +
            'mid-update). The remaining ' + $workNoHud.Count + ' rows are the ' +
            'WORK-H series below.')
    }
    $sortedWorkNoHud = @($workNoHud | Sort-Object)
    $stats += [PSCustomObject]@{
        bucket = 'WORK-H'
        mean = [uint64][Math]::Round((
            $workNoHud | Measure-Object -Average).Average, 0)
        p50 = [uint64]$sortedWorkNoHud[
            [int][Math]::Floor(($sortedWorkNoHud.Count - 1) * 0.50)]
        p95 = [uint64]$sortedWorkNoHud[
            [int][Math]::Floor(($sortedWorkNoHud.Count - 1) * 0.95)]
        min = [uint64]($workNoHud | Measure-Object -Minimum).Minimum
        max = [uint64]($workNoHud | Measure-Object -Maximum).Maximum
    }

    # Task 67: fallbacks per frame, and how the frames that took one compare to
    # the frames that did not. If the P95 really is the renderer dropping out of
    # its native owner, the two medians separate here and nowhere else.
    $fbBase = $bucketNames.Count + 1
    $workCol = [array]::IndexOf($bucketNames, 'WORK') + 1
    $hudCol = [array]::IndexOf($bucketNames, 'HUD') + 1
    # Ring mode rings the delta itself, so the column is already per-frame and
    # every sample counts. The per-stop column is cumulative and has to be
    # differenced, which costs the first sample.
    $fbPerFrame = @(if ($FallbackCensus -and $RingDump) {
        for ($i = 0; $i -lt $rows.Count; $i++) {
            [PSCustomObject]@{
                frame = $rows[$i][0]
                total = [uint64]$rows[$i][$fbBase]
                workH = [uint64]$rows[$i][$workCol] - [uint64]$rows[$i][$hudCol]
            }
        }
    } elseif ($FallbackCensus) {
        for ($i = 1; $i -lt $rows.Count; $i++) {
            [PSCustomObject]@{
                frame = $rows[$i][0]
                total = [uint64]$rows[$i][$fbBase] - [uint64]$rows[$i - 1][$fbBase]
                workH = [uint64]$rows[$i][$workCol] - [uint64]$rows[$i][$hudCol]
            }
        }
    })
    $fbFrames = @($fbPerFrame | Where-Object { $_.total -gt 0 })
    $cleanFrames = @($fbPerFrame | Where-Object { $_.total -eq 0 })

    # ALL is measured wall ticks for the whole iteration, not a sum of the
    # others; OTHR is defined as the ALL remainder, and WAIT/WORK are two more
    # views of ALL rather than additional named work. All four are therefore
    # excluded from the named share, which exists so a future run can tell "the
    # loop got slower" from "attribution drifted".
    $meanAll = ($stats | Where-Object { $_.bucket -eq 'ALL' }).mean
    # SHDT/SWRM join the four views-of-ALL in this exclusion because they are
    # sub-spans of SRC, which is already in the sum. Counting them would inflate
    # `named` past ALL and read exactly like attribution drift.
    $meanNamed = (($stats | Where-Object {
        $_.bucket -notin (@('ALL', 'OTHR', 'WAIT', 'WORK', 'WORK-H') +
                          $srcSubBuckets) } |
        Measure-Object -Property mean -Sum).Sum)

    $vbiMatch = [regex]::Match($output, 'TICKVBI=([0-9]+(?:,[0-9]+){4})')
    $slipMatch = [regex]::Match($output, 'TICKSLIP=([0-9]+)')
    $vbi = if ($vbiMatch.Success) {
        [uint64[]]($vbiMatch.Groups[1].Value -split ',')
    } else { @(0, 0, 0, 0, 0) }
    $vbiTotal = [uint64]($vbi[0] + $vbi[1] + $vbi[2] + $vbi[3])

    $extraValues = if ($ExtraGlobals.Count -ne 0) {
        $extraMatch = [regex]::Match($output,
            "TICKEXTRA=([0-9]+(?:,[0-9]+){$($ExtraGlobals.Count - 1)})")
        if (-not $extraMatch.Success) {
            # Do not fall back to zeros. A missing read and a counter that never
            # incremented look the same as zeros, and telling them apart is the
            # entire reason -ExtraGlobals exists.
            throw ("Requested $($ExtraGlobals.Count) extra globals but the run " +
                "produced no TICKEXTRA line. GDB output:`n$output")
        }
        [uint64[]]($extraMatch.Groups[1].Value -split ',')
    } else { @() }
    $extras = @(if ($ExtraGlobals.Count -ne 0) {
        0..($ExtraGlobals.Count - 1) | ForEach-Object {
            [PSCustomObject]@{ name = $ExtraGlobals[$_]; value = $extraValues[$_] }
        }
    })

    # Standing rule 8 -- a routed arm must PROVE the route took, and until now
    # that proof existed only on the console. The pokes above already print a
    # readback, but nothing parsed it, so a poke that did not stick produced an
    # artifact that is byte-for-byte as plausible as one that did. Cycle 99 lost
    # a whole cycle to exactly that: three whole-match JSONs, every one of them
    # complete, every one of them route 0, and the only evidence otherwise was
    # in console scrollback that no longer exists. Parse the readback into the
    # artifact, and refuse the run when it disagrees with what was requested --
    # a percentile table from an arm whose route never engaged is not a slower
    # candidate, it is a mislabelled control.
    $setGlobalReadbacks = @(foreach ($pair in $SetGlobals) {
        $n = ($pair -split '=')[0].Trim()
        # printf reads it back with %u, so compare in the same 32-bit unsigned
        # domain the guest stores it in; -SetGlobals accepts a leading '-'.
        $wantU = [uint32]([int64](($pair -split '=')[1].Trim()) -band 0xFFFFFFFF)
        $m = [regex]::Match($output, "SETGLOBAL=$([regex]::Escape($n)),([0-9]+)")
        if (-not $m.Success) {
            throw ("-SetGlobals $n produced no SETGLOBAL readback line, so the " +
                "poke cannot be shown to have landed. GDB output:`n$output")
        }
        [PSCustomObject]@{
            name = $n
            requested = $wantU
            readback = [uint32]$m.Groups[1].Value
            stuck = ([uint32]$m.Groups[1].Value -eq $wantU)
        }
    })
    $setGlobalFailed = @($setGlobalReadbacks | Where-Object { -not $_.stuck })
    if ($setGlobalFailed.Count -ne 0) {
        throw ('-SetGlobals did not stick: ' + (($setGlobalFailed | ForEach-Object {
            '{0} requested {1} but read back {2}' -f $_.name, $_.requested, $_.readback
        }) -join '; ') + '. The readback is taken in the same stop as the poke, ' +
            'so this is a failed write, not the guest overwriting it later. ' +
            'Refusing to emit a percentile table for an arm whose route did ' +
            'not engage.')
    }

    $melonVersion = (Get-Item -LiteralPath $context.MelonDSPath).VersionInfo.FileVersion
    $dldiEnabled = Get-MelonDSDldiEnabled -ConfigPath $configState.Config
    $result = [PSCustomObject]@{
        target = $target
        rom = $rom
        romSha256 = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
        melonDS = $context.MelonDSPath
        melonDSSha256 = (Get-FileHash -LiteralPath $context.MelonDSPath `
            -Algorithm SHA256).Hash
        melonDSVersion = $melonVersion
        gitShort = (git -C $root rev-parse --short HEAD)
        # gitShort stamps HEAD, which is NOT what was measured when the tree is
        # dirty. R2-03 E69's artifact reads 0b39c1a -- the commit BEFORE the change
        # it measured -- because the sources were still uncommitted, and a later
        # session spent a cycle concluding "the baseline does not reproduce" from
        # it. Record the tree state so a future reader can tell.
        gitDirtyPaths = @(git -C $root status --porcelain).Count
        # THE EMULATOR CONFIGURATION IS PART OF THE MEASUREMENT. DLDI on costs
        # roughly 29,696 ticks of WORK-H P95 -- E69's own commit reads 1,096,768
        # with it off and 1,126,464 with it on, identical source -- because DLDI
        # resolves nitro:/ through the SD-card driver instead of the card ROM
        # interface and so prices real I/O into the frame. It is also REQUIRED for
        # retail parity, so the DLDI-on figure is the honest one. Nothing recorded
        # it until 2026-07-29, which made every pre-3eb9ecdb baseline in the board
        # silently optimistic and unusable for a cross-commit delta.
        dldiEnabled = $dldiEnabled
        ringDump = [bool]$RingDump
        samples = $rows.Count
        startFrame = [uint64]$frames[0]
        endFrame = [uint64]$frames[-1]
        buckets = $stats
        # The per-frame series, not just its order statistics. A P95 says how
        # bad the bad frames are; only the series says whether they are periodic
        # (a scheduled refill or a cart read) or event-driven (a hit, a KO).
        # Those want opposite fixes, and the summary cannot tell them apart.
        bucketNames = $bucketNames
        rows = $rows
        meanAll = $meanAll
        meanNamed = $meanNamed
        vbi2 = $vbi[0]
        vbi3 = $vbi[1]
        vbi4 = $vbi[2]
        vbi5plus = $vbi[3]
        vbiMax = $vbi[4]
        vbiTotal = $vbiTotal
        cadenceViolations = if ($slipMatch.Success) {
            [uint64]$slipMatch.Groups[1].Value } else { 0 }
        extras = $extras
        # What was poked and what it read back in the same stop. Empty for an
        # unrouted run; the run throws above rather than reaching here with a
        # readback that disagrees, so every value present here is `stuck`.
        setGlobals = $setGlobalReadbacks
        # Per-stop gap accounting for a repeated-ring-dump run. Empty for a
        # single-stop run. A reader deciding whether a whole-match series is
        # trustworthy needs the skew per stop, not a pass/fail: a gap that
        # lands on a KO is exactly the sample that must not vanish quietly.
        ringStops = if ($RingDump) { $ringStopTargets.Count } else { 0 }
        # .ToArray(), NOT @(...). Wrapping a List[object] in @() and then
        # putting it in the hashtable a [pscustomobject] is built from throws
        # "Argument types do not match" -- and it throws AFTER -RowsCsv has
        # already been written, so it reads like a percentile bug rather than a
        # plumbing one. Cost one 16-minute run to find; the raw list and
        # .ToArray() both work.
        ringStopSkews = if ($RingDump -and ($null -ne $ringStopSkews)) {
            $ringStopSkews.ToArray() } else { @() }
        ringStopReads = if ($RingDump -and ($null -ne $ringStopReads)) {
            $ringStopReads.ToArray() } else { @() }
        capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
    }

    # DLDI and tree-dirtiness ride on the headline because both silently invalidate
    # a cross-commit comparison, and both did: DLDI is worth ~29,696 WORK-H P95, and
    # E69's artifact stamps the commit before the change it measured.
    Write-Output ("Tick-HUD buckets: target=$target samples=$($rows.Count) " +
        "frames=$($frames[0])..$($frames[-1]) melonDS=$melonVersion " +
        "sha=$($result.melonDSSha256.Substring(0,16)) git=$($result.gitShort)" +
        "$(if ($result.gitDirtyPaths -gt 0) { "+dirty($($result.gitDirtyPaths))" }) " +
        "dldi=$(if ($null -eq $dldiEnabled) { 'unknown' } elseif ($dldiEnabled) { 'ON' } else { 'off' })")
    # P50/P95 lead because they are the decision basis in docs/VERIFYING.md and
    # they survive the spikes (a text redraw or a respawn) that make a mean of a
    # few hundred frames unrepresentative. spread = p95/p50 names exactly which
    # buckets those are: anything above ~2 should not be compared by mean.
    # %ALL is p50-relative and deliberately does not sum to 100 - percentiles do
    # not add. The additive identity is the mean line below.
    $p50All = ($stats | Where-Object { $_.bucket -eq 'ALL' }).p50
    if ($Samples -lt 40) {
        # With n samples the p95 index resolves to 1/n, so a short run reports
        # something closer to p(100-100/n) and can miss the burst frames
        # entirely - which is exactly what makes AUD/HUD look tame.
        Write-Warning ("p95 resolution is 1/$Samples here; use -Samples 40 or " +
            'more before treating p95 or spread as a decision input.')
    }
    $stats | Format-Table `
        @{n='bucket';e={$_.bucket};w=6}, `
        @{n='p50';e={'{0,10:N0}' -f $_.p50}}, `
        @{n='p95';e={'{0,10:N0}' -f $_.p95}}, `
        @{n='spread';e={'{0,6:N2}' -f ($_.p95 / [double][Math]::Max(1, $_.p50))}}, `
        @{n='mean';e={'{0,10:N0}' -f $_.mean}}, `
        @{n='min';e={'{0,10:N0}' -f $_.min}}, `
        @{n='max';e={'{0,10:N0}' -f $_.max}}, `
        @{n='%ALLp50';e={'{0,7:N1}' -f (100.0 * $_.p50 / $p50All)}} -AutoSize
    Write-Output (("named={0:N0} ({1:N1}% of ALL)  VBI 2:{2} 3:{3} 4:{4} 5+:{5} " +
        "max:{6} total:{7}  slips={8}") -f
        $meanNamed, (100.0 * $meanNamed / $meanAll),
        $vbi[0], $vbi[1], $vbi[2], $vbi[3], $vbi[4], $vbiTotal,
        $result.cadenceViolations)
    if ($extras.Count -ne 0) {
        Write-Output ('extras: ' + (($extras | ForEach-Object {
            '{0}={1:N0}' -f $_.name, $_.value }) -join '  '))
    }

    if ($FallbackCensus) {
    $fbMatch = [regex]::Match($output,
        "TICKFB=([0-9]+(?:,[0-9]+){$($fallbackReasons.Count - 1)})")
    $fbTotals = if ($fbMatch.Success) {
        [uint64[]]($fbMatch.Groups[1].Value -split ',')
    } else { @(0) * $fallbackReasons.Count }
    # Ring mode brackets the window with a baseline read, so the reported
    # numbers are fallbacks taken during the sampled frames rather than since
    # reset -- boot, the menu and the seeding presents no longer count.
    $fb0Match = [regex]::Match($output,
        "TICKFB0=([0-9]+(?:,[0-9]+){$($fallbackReasons.Count - 1)})")
    $fbBaseline = if ($fb0Match.Success) {
        [uint64[]]($fb0Match.Groups[1].Value -split ',')
    } else { @([uint64]0) * $fallbackReasons.Count }
    $fbWindow = @(0..($fallbackReasons.Count - 1) | ForEach-Object {
        [uint64]$fbTotals[$_] - [uint64]$fbBaseline[$_] })
    $fbByReason = @(0..($fallbackReasons.Count - 1) | ForEach-Object {
        '{0}:{1}' -f $fallbackReasons[$_], $fbWindow[$_]
    })
    if ($RingDump) {
        # calls and eligible lead the enum as denominators, so they are excluded
        # from the fallback total and reported as the rate's base instead --
        # summing them in would report 522 fallbacks out of 256 draws.
        #
        # R2-03 E54: the enum's LAST TWO entries are not fallback reasons either.
        # Task 73's AnimForceLoad/AnimForceResident ride along on this counter
        # bank because it already had plumbing, and summing them in reported
        # "23 fell back" for a window whose real answer was 5 -- with animLoad's
        # 18 dominating the breakdown and pointing the next reader at animation
        # residency instead of at the hitlag shuffle that was actually firing.
        # Report them, separately, as what they are.
        $fbDenominators = 2
        $fbRideAlongs = 2
        $fbReasonCount = $fallbackReasons.Count - $fbDenominators - $fbRideAlongs
        $fbCalls = [double]$fbWindow[0]
        $fbReasonSum = (($fbWindow | Select-Object -Skip $fbDenominators `
            -First $fbReasonCount | Measure-Object -Sum).Sum)
        Write-Output (("native-owner: {0} draws over {1} frames, {2} eligible, " +
            "{3} fell back ({4:N1}%)  [{5}]") -f
            $fbWindow[0], $rows.Count, $fbWindow[1], $fbReasonSum,
            (100.0 * $fbReasonSum / [Math]::Max(1.0, $fbCalls)),
            (($fbByReason | Select-Object -Skip $fbDenominators `
                -First $fbReasonCount) -join ' '))
        Write-Output ("  Task 73 anim residency (NOT fallbacks): {0}" -f
            (($fbByReason | Select-Object -Last $fbRideAlongs) -join ' '))
        Write-Output (("  frames with a fallback: {0} of {1} " +
            "(ring counts every reason including the ride-alongs)") -f
            $fbFrames.Count, $fbPerFrame.Count)
    } else {
        Write-Output (("native-owner fallback: {0} of {1} frames took one  [{2}]") -f
            $fbFrames.Count, $fbPerFrame.Count, ($fbByReason -join ' '))
    }
    if (($fbFrames.Count -gt 0) -and ($cleanFrames.Count -gt 0)) {
        $fbMed = ((@($fbFrames.workH | Sort-Object))[
            [int][Math]::Floor(($fbFrames.Count - 1) * 0.5)])
        $clMed = ((@($cleanFrames.workH | Sort-Object))[
            [int][Math]::Floor(($cleanFrames.Count - 1) * 0.5)])
        Write-Output (("  WORK-H median: fallback {0:N0} vs clean {1:N0} " +
            "({2:N2}x)") -f $fbMed, $clMed, ($fbMed / [double]$clMed))
        # A median over a handful of frames is a weak basis for "the fallback
        # frames are the expensive frames". What settles it is whether the two
        # populations separate: how many clean frames reach the cheapest
        # fallback frame. Zero overlap is the claim; anything else is not.
        $fbMin = ($fbFrames.workH | Measure-Object -Minimum).Minimum
        $cleanAbove = @($cleanFrames | Where-Object { $_.workH -ge $fbMin })
        $cleanSorted = @($cleanFrames.workH | Sort-Object)
        Write-Output (("  separation: cheapest fallback frame {0:N0}; " +
            "{1} of {2} clean frames reach it; clean P95 {3:N0}") -f
            $fbMin, $cleanAbove.Count, $cleanFrames.Count,
            $cleanSorted[[int][Math]::Floor(($cleanSorted.Count - 1) * 0.95)])
        Write-Output ("  fallback frames: " + (($fbFrames |
            ForEach-Object { '{0}({1}):{2:N0}' -f
                $_.frame, $_.total, $_.workH }) -join ' '))
    }
    }

    if ($JsonOut) {
        $jsonDir = Split-Path -Parent $JsonOut
        if ($jsonDir) { New-Item -ItemType Directory -Force -Path $jsonDir | Out-Null }
        $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $JsonOut
        Write-Output "Wrote $JsonOut"
    }
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr, `
        (Join-Path $temp 'tick-hud-ring.bin'), `
        (Join-Path $temp 'tick-hud-fallback-ring.bin') `
        -Force -ErrorAction SilentlyContinue
}
