[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4613,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    # Lab A/B only: point at a sizing build of the SAME target (for example one
    # compiled with NDS_R2_DRAW_SUPPRESS_MASK). The registry gate never passes
    # this, so the default remains the configuration-exact gate build.
    [string]$Build = 'build-p2-fourcpu-tickhud',
    # Calibrated from the first crash-free four-CPU source match: source
    # identity/clock are read exactly at presented frame 1, while the tick-HUD
    # ring's first populated timing sample is frame 2. Frames 2..1973 therefore
    # provide 1,972 timing samples and the same frame-1 -> frame-1973 guest
    # coverage proves clock 60 -> 1 (59/60 s). Do not manufacture a frame-1
    # bucket: the guest ring demonstrably has no such sample at that marker.
    [ValidateRange(128,16384)][int]$Samples = 1972,
    [ValidateRange(1,1000000)][int]$StartFrame = 2,
    [ValidateRange(30,14400)][int]$TimeoutSeconds = 3600,
    [string]$JsonOut = '',
    [string]$RowsCsv = '',
    [string]$CoverageJsonOut = '',
    [string]$MemoryJsonOut = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-p2-fourcpu-tickhud-hwtri'
$build = $Build
$coverageStartFrame = 1

if ([string]::IsNullOrWhiteSpace($JsonOut)) {
    $JsonOut = Join-Path $root 'artifacts\verification\p2-2-fourcpu-tickhud.json'
}
if ([string]::IsNullOrWhiteSpace($RowsCsv)) {
    $RowsCsv = Join-Path $root 'artifacts\verification\p2-2-fourcpu-tickhud.csv'
}
if ([string]::IsNullOrWhiteSpace($CoverageJsonOut)) {
    $CoverageJsonOut = Join-Path $root 'artifacts\verification\p2-2-fourcpu-coverage.json'
}
if ([string]::IsNullOrWhiteSpace($MemoryJsonOut)) {
    $MemoryJsonOut = Join-Path $root 'artifacts\verification\p2-2-fourcpu-memory.json'
}

# P2-2 items 4/5: read memory and bounded-pool evidence from the SAME run as
# the timing buckets. A second emulator run cannot prove that its low-water or
# saturation belongs to the frames whose cadence was measured. These are all
# existing live counters, so this adds no guest instrumentation or placement.
$memoryGlobals = @(
    'gNdsTaskmanGeneralHeapFreeMin',
    'gNdsTaskmanArenaChosenSize',
    'gNdsTaskmanArenaAllocFailCount',
    'gNdsGCDrawsActiveMax',
    'gNdsEffectPoolDepth',
    'gNdsEffectPoolFreeMin',
    'gNdsParticleStructsMax',
    'gNdsParticleGeneratorsMax',
    'gNdsParticleTransformsMax',
    'gNdsParticleRejectCount',
    'gNdsAObjEvent32NormalizedHighWater',
    'gNdsAObjEvent32NormalizeFailCount',
    'gNdsAObjEvent32HashOverflowCount',
    'gNdsSyMallocOverflowCount',
    'gNdsObjmanPanicCount',
    # P2-3r13. THE PER-CONTEXT GRAPHICS HEAP. The source sizes it 0xD000 and the
    # DS renderer replaced most of what it was for, but unlike the DL and RDP
    # buffers it still has live writers, so it is measured before it is trimmed.
    # HighWater is a per-FRAME peak sampled at the reset and at every point the
    # fighter draw rolls its own consumption back. Overflow is asserted: the
    # source only warns and keeps writing, so an undersized heap corrupts rather
    # than fails.
    'gNdsTaskmanGraphicsHeapHighWater',
    'gNdsTaskmanGraphicsHeapCapacity',
    'gNdsTaskmanGraphicsHeapOverflowCount',
    # P2-3f9 cut that reservation 0xD000 -> 0x2000 (90,112 B back to the scene
    # arena, which is what let four distinct kinds fit). The overflow counter
    # above cannot see the one writer that CHECKS before it writes -- the
    # adapter's material branch table refuses and returns, so the pointer never
    # passes `end`. This counts that refusal, and it is asserted at 0.
    'gNdsTaskmanGraphicsHeapNoRoomCount',
    # P2-3r13. The title/opening/Castle scene file store left ARM9 .bss for the
    # scene arena, which is what paid for four distinct fighter kinds in the
    # shipping configuration. A VSBattle must never allocate it: AllocCount 0 on
    # this arm is the proof the move actually returned the bytes to the battle,
    # and DeclineCount 0 says no scene was refused it.
    'gNdsRelocSceneFileBufferAllocCount',
    'gNdsRelocSceneFileBufferDeclineCount',
    # P2-3f9. THE BATTLEPACK CARVE IS A PER-MATCH DECISION NOW, so the build
    # flag no longer says whether the pack was resident. 287,936 B of the same
    # arena the fighter kinds need buys -34,304 ticks on two fighters and -64 on
    # four (P2-3r13), so a match with more than two distinct kinds declines it.
    # DeclineCount and MatchKinds are reported, never asserted: which way the
    # decision went is a property of the roster, not a fault.
    'gNdsBattlePackCarveDeclineCount',
    'gNdsBattlePackCarveMatchKinds',
    'gNdsBattlePackResidentBytes',
    'gNdsR2AnimCacheArenaReservedBytes',
    # P2-3r11. THE POSE POOL IS EXACTLY FULL ON THIS ARM AND NOWHERE ELSE:
    # NDS_FT_POSE_FIGHTERS is 4 and this is the only configuration that creates
    # four fighters, so a fifth bind has no spare slot. A BindFull is not a
    # saturation statistic like the effect pool's -- it drops that fighter to
    # the generic AObj path silently, changing how it animates, so it is
    # asserted below rather than merely reported.
    'gNdsFtPoseBinds',
    'gNdsFtPoseBindFull',
    # The acceptance instrument for whatever animation-arena budget this arm is
    # built with. `NDS_R2_ANIM_CACHE_ARENA_BYTES` is smaller on the four-distinct
    # -kind roster because the fighter kinds took the difference (P2-3r11), and a
    # cache that no longer fits its working set shows up HERE rather than in a
    # tick figure nobody can attribute. Rejects/Misses are DATA, not a failure:
    # every miss degrades to the on-demand load.
    'gNdsR2AnimCacheMisses',
    'gNdsR2AnimCacheRejects',
    'gNdsR2AnimCacheHits',
    'gNdsR2AnimCacheFills',
    'gNdsR2AnimCacheBytes',
    'gNdsR2AnimCacheArenaUsedBytes',
    'gNdsR2AnimCacheRawRecycles',
    'gNdsRelocAssetDirectDispatch',
    'gNdsRelocAssetDirectReadCount',
    'gNdsRelocAssetDirectFallbackCount',
    'gNdsRelocAssetFighterStreamDispatch',
    'gNdsRelocAssetFighterStreamReads',
    'gNdsRelocAssetFighterStreamMisses',
    'gNdsRelocAssetFighterStreamFailures',
    # These are part of the shipping tick-HUD target already. Do not enable
    # Task-68's fallback census here: that flag changes BSS/cache placement and
    # would make the gate measure a different binary. PlanBuild means the live
    # low-detail collection passed the detail-aware generated-owner validator;
    # PlanHit proves subsequent frames reused that validated identity. The
    # focused debugger proof accompanying this gate checks the production entry
    # itself with use_low_detail=1.
    'gNdsFtrPlanBuild',
    'gNdsFtrPlanHit',
    'gNdsFtrPlanVerifyMismatch',
    'gNdsFighterDLAllDrawP0HardwareTriangleCount',
    'gNdsFighterDLAllDrawP1HardwareTriangleCount',
    # P2-3r15. THE TWO COUNTERS THAT CAN EXPRESS A ROSTER WIDER THAN TWO NAMES.
    # Everything above stops at P0/P1 -- two triangle totals and two fkind
    # publications -- which is precisely how this arm ran Mario/Fox mirrors for a
    # whole phase while its artifact said "four fighters" and how a stranded
    # `#define` in Donkey Kong's cargo ladder survived to 2026-08-25. The kind
    # census is one byte per player slot holding `fkind + 1` (0 = empty slot, so
    # Mario is distinguishable from absent); the draw mask is one bit per slot
    # that emitted hardware triangles.
    'gNdsSCVSBattleOriginalFighterKinds',
    'gNdsFighterDLAllDrawSlotTriangleMask'
)

$coverageGlobals = @(
    'gNdsBattleTextHudTimeSeconds',
    'gNdsBattlePlayablePacingLogicFrames',
    'gSCManagerTransferBattleState.time_limit',
    'gNdsSCVSBattleOriginalPlayerCount',
    'gNdsSCVSBattleOriginalCpuCount',
    'gNdsSCVSBattleOriginalFighterGObjCount',
    'gNdsSCVSBattleOriginalActivePlayerMask'
)

# One collector owns all headline timing math for this project. Reusing it keeps
# P50/P95, WORK-H, the VBlank histogram, repeated-ring stitching and max cadence
# definition identical to the two-fighter Boundary instrument. This arm joined
# Boundary after the accepted 2026-08-21 whole-match run; the registry is now
# the authority for that membership, so this script must remain configuration-
# exact rather than growing a second, looser "stress" contract here.
$sampleArgs = @{
    MelonDS = $MelonDS
    Gdb = $Gdb
    GdbPort = $GdbPort
    RunnerSlot = $RunnerSlot
    Target = $target
    Build = $build
    RingDump = $true
    RingStartRead = $true
    RingStartReadFrame = $coverageStartFrame
    Samples = $Samples
    StartFrame = $StartFrame
    TimeoutSeconds = $TimeoutSeconds
    ExtraGlobals = $memoryGlobals
    PerStopGlobals = $coverageGlobals
    JsonOut = $JsonOut
    RowsCsv = $RowsCsv
}
if ($NoBuild) { $sampleArgs.NoBuild = $true }

& (Join-Path $PSScriptRoot 'sample-tick-hud-buckets.ps1') @sampleArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Coverage is part of the SAME measurement. The old wrapper launched an entire
# second emulated match after the bucket run just to read two clock values. That
# doubled the most expensive P2-2 verifier and, worse, allowed timing and memory
# to come from one battle while source identity came from another. The collector
# now takes one exact frame-1 read and carries these same globals at every sparse
# ring drain; the final drain is exact too.
$sample = Get-Content -LiteralPath $JsonOut -Raw | ConvertFrom-Json
if (($null -eq $sample.ringStartRead) -or ($sample.ringStopReads.Count -eq 0)) {
    throw 'Four-CPU stress artifact is missing same-run start/end coverage reads.'
}
$start = $sample.ringStartRead
$end = $sample.ringStopReads[-1]
$expectedEndFrame = $StartFrame + $Samples - 1
if (([uint64]$sample.startFrame -ne [uint64]$StartFrame) -or
    ([uint64]$sample.endFrame -ne [uint64]$expectedEndFrame) -or
    ([uint64]$start.frame -ne [uint64]$coverageStartFrame) -or
    ([uint64]$end.frame -ne [uint64]$expectedEndFrame)) {
    throw ("Four-CPU timing rows are not the requested inclusive window: " +
        "artifact=$($sample.startFrame)..$($sample.endFrame), " +
        "coverage=$($start.frame)..$($end.frame), " +
        "requested timing=$StartFrame..$expectedEndFrame, " +
        "coverage=$coverageStartFrame..$expectedEndFrame.")
}

$clockStart = [int]$start.'gNdsBattleTextHudTimeSeconds'
$clockEnd = [int]$end.'gNdsBattleTextHudTimeSeconds'
$seededMinutes = [int]$start.'gSCManagerTransferBattleState.time_limit'
$seededMinutesEnd = [int]$end.'gSCManagerTransferBattleState.time_limit'
if (($seededMinutes -ne 1) -or ($seededMinutesEnd -ne 1)) {
    throw ("Four-CPU stress is not the required one-minute Time match: " +
        "time_limit=$seededMinutes->$seededMinutesEnd.")
}
$identity = [ordered]@{
    playerCount = [int]$start.gNdsSCVSBattleOriginalPlayerCount
    cpuCount = [int]$start.gNdsSCVSBattleOriginalCpuCount
    fighterCount = [int]$start.gNdsSCVSBattleOriginalFighterGObjCount
    activePlayerMask = [int]$start.gNdsSCVSBattleOriginalActivePlayerMask
}
$identityEnd = [ordered]@{
    playerCount = [int]$end.gNdsSCVSBattleOriginalPlayerCount
    cpuCount = [int]$end.gNdsSCVSBattleOriginalCpuCount
    fighterCount = [int]$end.gNdsSCVSBattleOriginalFighterGObjCount
    activePlayerMask = [int]$end.gNdsSCVSBattleOriginalActivePlayerMask
}
foreach ($pair in @(
    @{ Name='player count'; Start=$identity.playerCount; End=$identityEnd.playerCount; Want=0 },
    @{ Name='CPU count'; Start=$identity.cpuCount; End=$identityEnd.cpuCount; Want=4 },
    @{ Name='fighter count'; Start=$identity.fighterCount; End=$identityEnd.fighterCount; Want=4 },
    @{ Name='active-player mask'; Start=$identity.activePlayerMask; End=$identityEnd.activePlayerMask; Want=15 }
)) {
    if (($pair.Start -ne $pair.Want) -or ($pair.End -ne $pair.Want)) {
        throw ("Four-CPU source identity mismatch for $($pair.Name): " +
            "$($pair.Start)->$($pair.End), expected $($pair.Want).")
    }
}
$elapsedSeconds = [math]::Abs($clockStart - $clockEnd)
$matchSeconds = $seededMinutes * 60
$coverage = [PSCustomObject]@{
    probe = 'same-run four CPU match window coverage'
    target = $target
    romSha256 = $sample.romSha256
    startFrame = $coverageStartFrame
    timingStartFrame = $StartFrame
    endFrame = $expectedEndFrame
    clockStart = $clockStart
    clockEnd = $clockEnd
    elapsedSeconds = $elapsedSeconds
    matchSeconds = $matchSeconds
    fractionOfMatch = $(if ($matchSeconds) { $elapsedSeconds / $matchSeconds } else { 0 })
    presentedDelta = [int64]$end.frame - [int64]$start.frame
    logicDelta = [int64]$end.gNdsBattlePlayablePacingLogicFrames -
        [int64]$start.gNdsBattlePlayablePacingLogicFrames
    sourceIdentity = $identity
    capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
}
$coverageDir = Split-Path -Parent $CoverageJsonOut
if ($coverageDir) { New-Item -ItemType Directory -Force -Path $coverageDir | Out-Null }
$coverage | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $CoverageJsonOut
# The clock publication is integer seconds, while the two stops are presented-
# frame edges.  A window that begins on the first battle present and ends on the
# last can therefore differ by one displayed second even though it spans the
# whole playable interval.  Refuse anything shorter than that quantization
# allowance; in particular, the historical 440..2040 window (~86.7%) cannot be
# called whole-match again.
$minimumWholeMatchFraction = 0.98
if ([double]$coverage.fractionOfMatch -lt $minimumWholeMatchFraction) {
    throw ("P2-2 four-CPU timing window is not whole-match: " +
        ("{0:P2} of the guest's configured match ({1}s of {2}s). " -f
            [double]$coverage.fractionOfMatch,
            [int]$coverage.elapsedSeconds,
            [int]$coverage.matchSeconds) +
        'Recalibrate -StartFrame/-Samples before accepting timing or memory data.')
}

# Coverage passed, so the end-of-window high/low-water values collected by the
# timing run cover the accepted whole-match span rather than an arbitrary
# prefix. Normalize them into the P2-2 budget artifact. Saturation is DATA, not
# a failure: BattleShip deliberately reserves its last four effects for forced
# returns and bounded pools may refuse allocations. Allocator overflow or an
# objman panic, however, is never a valid completed stress run.
$extra = @{}
foreach ($item in $sample.extras) {
    $extra[[string]$item.name] = [uint64]$item.value
}
foreach ($name in $memoryGlobals) {
    if (-not $extra.ContainsKey($name)) {
        throw "Stress timing artifact is missing required memory counter '$name'."
    }
}

$nativePlanBuild = $extra['gNdsFtrPlanBuild']
$nativePlanHit = $extra['gNdsFtrPlanHit']
$nativePlanMismatch = $extra['gNdsFtrPlanVerifyMismatch']
if (($nativePlanBuild -eq 0) -or ($nativePlanHit -eq 0) -or
    ($nativePlanMismatch -ne 0)) {
    throw ("Four-CPU low-detail native owner did not prove a stable validated plan: " +
        "build=$nativePlanBuild hit=$nativePlanHit verifyMismatch=$nativePlanMismatch.")
}
if (($extra['gNdsFighterDLAllDrawP0HardwareTriangleCount'] -eq 0) -or
    ($extra['gNdsFighterDLAllDrawP1HardwareTriangleCount'] -eq 0)) {
    # P2-3r15: these two are PLAYER SLOTS 0 and 1, not fighter kinds. They were
    # the same thing while the roster was Mario/Fox mirrors and are not any
    # more; slots 2/3 are covered by gNdsFighterDLAllDrawSlotTriangleMask below.
    throw ("Four-CPU stress created four source fighters but did not prove " +
        "player slots 0 and 1 drew: slot0Triangles=" +
        "$($extra['gNdsFighterDLAllDrawP0HardwareTriangleCount']) slot1Triangles=" +
        "$($extra['gNdsFighterDLAllDrawP1HardwareTriangleCount']).")
}

if ($extra['gNdsFtPoseBindFull'] -ne 0) {
    throw ("Four-fighter stress exhausted the fighter pose pool: " +
        "binds=$($extra['gNdsFtPoseBinds']) bindFull=$($extra['gNdsFtPoseBindFull']). " +
        'NDS_FT_POSE_FIGHTERS has no spare slot on this arm, so a refused bind ' +
        'silently drops that fighter to the generic AObj path and the run did ' +
        'not animate four fighters the way the shipped engine does.')
}

# THE FLAGS THE FIGURES WERE MEASURED UNDER, CARRIED WITH THE FIGURES.
# `docs/VERIFYING.md` step 3: the build directory's nds_build_config.h is the
# truth about what was measured. Two of its flags change what a tick figure from
# this arm MEANS -- the roster decides whether the four fighters are four
# distinct kinds or Mario/Fox mirrors, and NDS_R2_BATTLEPACK decides whether
# Fox's clip pack was resident. A pack-off figure is not comparable to a pack-on
# one, and this project has already banked nine artifacts whose flags differed
# from the ROM they were attributed to (2026-08-17). Stamp them rather than
# trusting a reader to go and look.
$buildConfigPath = Join-Path $root ("builds\{0}\nds_build_config.h" -f $build)
function Get-StressBuildFlag([string]$Name) {
    if (-not (Test-Path -LiteralPath $buildConfigPath)) { return $null }
    foreach ($line in (Get-Content -LiteralPath $buildConfigPath)) {
        if ($line -match ('^\s*#define\s+' + [regex]::Escape($Name) + '\s+(\d+)')) {
            return [int]$Matches[1]
        }
    }
    return $null
}
$rosterFlag = Get-StressBuildFlag 'NDS_P2_FOUR_CPU_ROSTER'
$battlePackFlag = Get-StressBuildFlag 'NDS_R2_BATTLEPACK'

# P2-3r15. WHAT THE FOUR FIGHTERS ACTUALLY WERE, decoded from the guest rather
# than inferred from a build flag. `fkind + 1` per byte, slot 0 in the low byte;
# 0 means the slot created no fighter. Names are BattleShip's nFTKind ordering
# (ft/fttypes.h): 0 Mario, 1 Fox, 2 Donkey, 3 Samus, 4 Luigi.
$kindNames = @('Mario', 'Fox', 'Donkey', 'Samus', 'Luigi', 'Link', 'Yoshi',
    'Captain', 'Kirby', 'Pikachu', 'Purin', 'Ness')
$kindWord = [uint32]$extra['gNdsSCVSBattleOriginalFighterKinds']
$observedRoster = @(0..3 | ForEach-Object {
    $b = [int](($kindWord -shr ($_ * 8)) -band 0xff)
    if ($b -eq 0) { 'empty' }
    elseif (($b - 1) -lt $kindNames.Count) { $kindNames[$b - 1] }
    else { "kind$($b - 1)" }
})
$expectedRoster = if ($rosterFlag -eq 1) {
    @('Samus', 'Fox', 'Captain', 'Donkey')
} else {
    @('Mario', 'Fox', 'Mario', 'Fox')
}
# ...AND WHETHER ALL FOUR OF THEM DREW. The P0/P1 triangle totals are slots 0
# and 1 only, so on the four-distinct-kind roster they are silent about Captain
# and Donkey Kong -- the exact shape of instrument that let a stranded DK setter
# survive. One bit per slot that emitted hardware triangles. Both are JUDGED
# after the ledger is written, below.
$slotDrawMask = [uint32]$extra['gNdsFighterDLAllDrawSlotTriangleMask']

$generalHeapFloor = [uint64]25600
$effectDepth = $extra['gNdsEffectPoolDepth']
$effectFreeMin = $extra['gNdsEffectPoolFreeMin']
$memory = [PSCustomObject]@{
    target = $target
    romSha256 = $sample.romSha256
    coverageArtifact = $CoverageJsonOut
    buildDirectory = $build
    fighterRoster = $(if ($rosterFlag -eq 1) {
        'four distinct kinds (Samus/Fox/Captain/Donkey)'
    } elseif ($null -eq $rosterFlag) { 'unknown' } else {
        'Mario/Fox mirrors'
    })
    # P2-3r15: what the GUEST said, not what the flag implies.
    fighterRosterObserved = ($observedRoster -join '/')
    fighterRosterExpected = ($expectedRoster -join '/')
    fighterKindWord = ('0x{0:X8}' -f $kindWord)
    fighterSlotDrawMask = ('0x{0:X}' -f $slotDrawMask)
    battlePackBuilt = $(if ($null -eq $battlePackFlag) { 'unknown' }
        else { [bool]($battlePackFlag -eq 1) })
    # P2-3f9: the RUNTIME answer, which is the one that matters since the carve
    # became roster-dependent. Resident bytes 0 with a nonzero decline count is
    # the expected reading on a match with three or more distinct kinds.
    battlePackResidentBytes = $extra['gNdsBattlePackResidentBytes']
    battlePackCarveDeclineCount = $extra['gNdsBattlePackCarveDeclineCount']
    battlePackCarveMatchKinds = $extra['gNdsBattlePackCarveMatchKinds']
    animCacheArenaReservedBytes = $extra['gNdsR2AnimCacheArenaReservedBytes']
    ftPoseBinds = $extra['gNdsFtPoseBinds']
    ftPoseBindFull = $extra['gNdsFtPoseBindFull']
    animCacheMisses = $extra['gNdsR2AnimCacheMisses']
    animCacheRejects = $extra['gNdsR2AnimCacheRejects']
    animCacheHits = $extra['gNdsR2AnimCacheHits']
    animCacheFills = $extra['gNdsR2AnimCacheFills']
    animCacheLiveBytes = $extra['gNdsR2AnimCacheBytes']
    animCacheArenaUsedBytes = $extra['gNdsR2AnimCacheArenaUsedBytes']
    animCacheRawRecycles = $extra['gNdsR2AnimCacheRawRecycles']
    animDirectDispatch = $extra['gNdsRelocAssetDirectDispatch']
    animDirectReads = $extra['gNdsRelocAssetDirectReadCount']
    animDirectFallbacks = $extra['gNdsRelocAssetDirectFallbackCount']
    animStreamDispatch = $extra['gNdsRelocAssetFighterStreamDispatch']
    animStreamReads = $extra['gNdsRelocAssetFighterStreamReads']
    animStreamMisses = $extra['gNdsRelocAssetFighterStreamMisses']
    animStreamFailures = $extra['gNdsRelocAssetFighterStreamFailures']
    arenaChosenBytes = $extra['gNdsTaskmanArenaChosenSize']
    arenaSearchAllocationFailures = $extra['gNdsTaskmanArenaAllocFailCount']
    graphicsHeapCapacityBytes = $extra['gNdsTaskmanGraphicsHeapCapacity']
    graphicsHeapPeakBytes = $extra['gNdsTaskmanGraphicsHeapHighWater']
    graphicsHeapOverflowCount = $extra['gNdsTaskmanGraphicsHeapOverflowCount']
    graphicsHeapNoRoomCount = $extra['gNdsTaskmanGraphicsHeapNoRoomCount']
    sceneFileBufferArenaAllocCount = $extra['gNdsRelocSceneFileBufferAllocCount']
    sceneFileBufferDeclineCount = $extra['gNdsRelocSceneFileBufferDeclineCount']
    generalHeapFreeMinBytes = $extra['gNdsTaskmanGeneralHeapFreeMin']
    generalHeapSafetyFloorBytes = $generalHeapFloor
    generalHeapMarginAboveFloorBytes =
        ([int64]$extra['gNdsTaskmanGeneralHeapFreeMin'] - [int64]$generalHeapFloor)
    dObjActiveMax = $extra['gNdsGCDrawsActiveMax']
    effectPoolCapacity = $effectDepth
    effectPoolFreeMin = $effectFreeMin
    effectPoolActiveMax = if ($effectDepth -ge $effectFreeMin) {
        $effectDepth - $effectFreeMin
    } else { 0 }
    effectEnteredSourceForcedReserve = ($effectFreeMin -lt 5)
    particleStructCapacity = 112
    particleStructMax = $extra['gNdsParticleStructsMax']
    particleGeneratorCapacity = 24
    particleGeneratorMax = $extra['gNdsParticleGeneratorsMax']
    particleTransformCapacity = 80
    particleTransformMax = $extra['gNdsParticleTransformsMax']
    particleStructSaturated = ($extra['gNdsParticleStructsMax'] -ge 112)
    particleGeneratorSaturated = ($extra['gNdsParticleGeneratorsMax'] -ge 24)
    particleTransformSaturated = ($extra['gNdsParticleTransformsMax'] -ge 80)
    particleStructRejects = $extra['gNdsParticleRejectCount']
    aObjEvent32NormalizedHighWater = $extra['gNdsAObjEvent32NormalizedHighWater']
    aObjEvent32NormalizeFailCount = $extra['gNdsAObjEvent32NormalizeFailCount']
    aObjEvent32HashOverflowCount = $extra['gNdsAObjEvent32HashOverflowCount']
    syMallocOverflowCount = $extra['gNdsSyMallocOverflowCount']
    objmanPanicCount = $extra['gNdsObjmanPanicCount']
    nativeOwnerPlanBuild = $nativePlanBuild
    nativeOwnerPlanHit = $nativePlanHit
    nativeOwnerPlanVerifyMismatch = $nativePlanMismatch
    # SLOT totals, not kind totals. Named by slot since P2-3f9: they happen to
    # be Samus and Fox on the landed-content roster, and a field name that says so would
    # quietly relabel a different fighter's triangles the next time slot 0
    # changes -- which P2-3f9 attempted and backed out.
    nativeOwnerSlot0HardwareTriangles = $extra['gNdsFighterDLAllDrawP0HardwareTriangleCount']
    nativeOwnerSlot1HardwareTriangles = $extra['gNdsFighterDLAllDrawP1HardwareTriangleCount']
    capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
}

# WRITE THE LEDGER BEFORE JUDGING IT. The assertions below used to run first, so
# a run that breached the floor threw with its own evidence unwritten and the
# next question -- "by how much, and which bucket moved" -- needed a whole second
# 25-minute run to answer (P2-3r13, 2026-08-25).
$memoryDir = Split-Path -Parent $MemoryJsonOut
if ($memoryDir) { New-Item -ItemType Directory -Force -Path $memoryDir | Out-Null }
$memory | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $MemoryJsonOut

# P2-3r15. THE ROSTER THE GATE ACTUALLY MEASURED, asserted rather than reported.
# Boundary's stress arm defaults to the four landed kinds since this row; the
# mirror roster stays reachable as `NDS_P2_FOUR_CPU_ROSTER=0` and is the A/B
# control. Either way the arm must run the roster its own build config declares.
if (@(Compare-Object $observedRoster $expectedRoster -SyncWindow 0).Count -ne 0) {
    throw ("Four-CPU stress ran the wrong roster: slots were " +
        "$($memory.fighterRosterObserved) but NDS_P2_FOUR_CPU_ROSTER=" +
        "$rosterFlag asks for $($memory.fighterRosterExpected) (kind word " +
        "$($memory.fighterKindWord)). A gate that measures a roster nobody " +
        'declared is not a regression guard.')
}
if ($slotDrawMask -ne 0xF) {
    throw ("Four-CPU stress did not draw all four fighter slots: hardware-" +
        "triangle slot mask $($memory.fighterSlotDrawMask) (want 0xF) on " +
        "roster $($memory.fighterRosterObserved). Slots 0/1 have their own " +
        'triangle totals; this mask is the only thing that speaks for slots ' +
        '2 and 3.')
}
if (($memory.syMallocOverflowCount -ne 0) -or
    ($memory.objmanPanicCount -ne 0) -or
    ($memory.aObjEvent32NormalizeFailCount -ne 0) -or
    ($memory.aObjEvent32HashOverflowCount -ne 0)) {
    throw ("Four-fighter stress hit a hard allocator/object-animation failure: " +
        "SyMallocOverflow=$($memory.syMallocOverflowCount), " +
        "ObjmanPanic=$($memory.objmanPanicCount), " +
        "AObj32NormalizeFail=$($memory.aObjEvent32NormalizeFailCount), " +
        "AObj32HashOverflow=$($memory.aObjEvent32HashOverflowCount).")
}
# The graphics heap does not fail when it runs out -- the source prints and keeps
# writing (decomp taskman.c:330) -- so an overrun is silent corruption and has to
# be asserted rather than reported.
if ([uint64]$memory.graphicsHeapOverflowCount -ne 0) {
    throw ("Four-fighter stress overran the per-context graphics heap " +
        "$($memory.graphicsHeapOverflowCount) time(s): peak " +
        "$($memory.graphicsHeapPeakBytes) B of " +
        "$($memory.graphicsHeapCapacityBytes) B. Raise " +
        "NDS_R2_VSBATTLE_GRAPHICS_ARENA_BYTES; a silent overrun scribbles past " +
        "the arena.")
}
# P2-3f9: the other half of the same question. This writer refuses instead of
# overrunning, so it costs a material branch rather than memory -- which is
# invisible to the overflow counter and easy to mistake for a DObj that simply
# had no materials.
if ([uint64]$memory.graphicsHeapNoRoomCount -ne 0) {
    throw ("Four-fighter stress refused $($memory.graphicsHeapNoRoomCount) " +
        "material branch table(s) for want of graphics-heap room: peak " +
        "$($memory.graphicsHeapPeakBytes) B of " +
        "$($memory.graphicsHeapCapacityBytes) B. Those DObjs drew without " +
        "their material branch. Raise NDS_R2_VSBATTLE_GRAPHICS_ARENA_BYTES.")
}
# P2-3r13: a VSBattle that allocates the scene file store has taken back the
# 185,696 B the four-distinct-kind roster is standing on.
if ([uint64]$memory.sceneFileBufferArenaAllocCount -ne 0) {
    throw ("Four-fighter stress allocated the scene file store inside a battle " +
        "($($memory.sceneFileBufferArenaAllocCount) time(s)). That store is the " +
        "arena the four-kind roster was paid for; see ndsRelocSceneFileBuffer.")
}
if ([uint64]$memory.generalHeapFreeMinBytes -lt $generalHeapFloor) {
    throw ("Four-fighter stress breached the general-heap safety floor: " +
        "$($memory.generalHeapFreeMinBytes) B < $generalHeapFloor B. " +
        "P2-2 may not trade source-correct four-fighter state for allocator risk.")
}
if (([uint64]$memory.animCacheHits -eq 0) -or
    ([uint64]$memory.animCacheFills -eq 0) -or
    ([uint64]$memory.animCacheRawRecycles -eq 0) -or
    ([uint64]$memory.animCacheLiveBytes -gt
        [uint64]$memory.animCacheArenaReservedBytes) -or
    ([uint64]$memory.animCacheArenaUsedBytes -gt
        [uint64]$memory.animCacheArenaReservedBytes)) {
    throw ("Four-fighter raw animation cache did not prove bounded circular " +
        "engagement: hits=$($memory.animCacheHits) " +
        "fills=$($memory.animCacheFills) wraps=$($memory.animCacheRawRecycles) " +
        "live=$($memory.animCacheLiveBytes) " +
        "cursor=$($memory.animCacheArenaUsedBytes) " +
        "reserved=$($memory.animCacheArenaReservedBytes).")
}
if (([uint64]$memory.animDirectDispatch -ne 1) -or
    ([uint64]$memory.animDirectReads -eq 0) -or
    ([uint64]$memory.animDirectFallbacks -ne 0)) {
    throw ("Four-fighter direct NitroROM animation acquisition did not engage " +
        "cleanly: dispatch=$($memory.animDirectDispatch) " +
        "reads=$($memory.animDirectReads) " +
        "fallbacks=$($memory.animDirectFallbacks).")
}
if (([uint64]$memory.animStreamDispatch -ne 1) -or
    ([uint64]$memory.animStreamReads -eq 0) -or
    ([uint64]$memory.animStreamFailures -ne 0)) {
    throw ("Four-fighter source-normalized animation stream did not engage " +
        "cleanly: dispatch=$($memory.animStreamDispatch) " +
        "reads=$($memory.animStreamReads) misses=$($memory.animStreamMisses) " +
        "failures=$($memory.animStreamFailures).")
}

Write-Host ''
Write-Host 'P2-2 four-fighter stress artifacts:'
Write-Host "  buckets:  $JsonOut"
Write-Host "  rows:     $RowsCsv"
Write-Host "  coverage: $CoverageJsonOut"
Write-Host "  memory:   $MemoryJsonOut"
Write-Host 'P2-2 four-CPU standing stress passed its correctness, cadence, native-owner, and memory gates.'
exit 0
