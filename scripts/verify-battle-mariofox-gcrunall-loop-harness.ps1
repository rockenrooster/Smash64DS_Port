param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipFTManager,
    [switch]$ImportBattleShipFTComputer,
    [switch]$ImportBattleShipVSResults,
    [switch]$ImportBattleShipBattlePlayable,
    [switch]$ImportBattleShipIFCommon,
    [switch]$ImportBattleShipMarioFireball,
    [switch]$ImportBattleShipFoxBlaster,
    [switch]$ImportBattleShipEffectManager,
    [switch]$ImportBattleShipFoxReflector,
    [switch]$ImportBattleShipMarioSpecialHi,
    [switch]$ImportBattleShipMarioSpecialLw,
    [switch]$ImportBattleShipFoxSpecialHi,
    [switch]$ImportBattleShipAudioAssets,
    [switch]$ImportBattleShipAudioBGM,
    [switch]$ImportBattleShipNormalMoveset,
    [switch]$HardwareTriangles,
    [switch]$BattlePlayable,
    [switch]$RealtimePresentation,
    [switch]$LiveInputPreview,
    [switch]$CPUOpponentProof,
    [switch]$MatchLifecycleProof,
    [switch]$RequireRealtime60Fps,
    [switch]$RendererBenchmarkOnly,
    [ValidateRange(0,2)][int]$RendererProfileLevel = 2,
    [ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0,
    [string]$Harness = 'battle_mariofox_gcrunall_loop',
    [string]$Target = 'smash64ds-battle-mariofox-gcrunall-loop',
    [string]$Build = 'build-battle-mariofox-gcrunall-loop-harness',
    [int]$ExpectedMode = 53,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox gcRunAll-loop',
    [string]$HarnessSelectMessage = 'Direct gcRunAll-loop harness did not select VSBattle from Maps.'
)
$ErrorActionPreference = 'Stop'
if ($RendererBenchmarkOnly -and ($RendererBenchmarkSamples -eq 0)) {
    throw 'RendererBenchmarkOnly requires RendererBenchmarkSamples greater than zero.'
}
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ImportBattleShipFTManager = $true
if ($BattlePlayable) {
    $ImportBattleShipBattlePlayable = $true
}
if ($CPUOpponentProof -or $MatchLifecycleProof) {
    $ImportBattleShipFTComputer = $true
    $LiveInputPreview = $true
}
if ($MatchLifecycleProof) {
    $CPUOpponentProof = $true
    $ImportBattleShipVSResults = $true
}
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Join-Path $root "$Target.nds"
$elf = Join-Path $root "$Target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stdout.log"
$stderr = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stderr.log"
$scriptName = "_$($Harness)_harness.gdb"
$configState = $null
$emulator = $null
$benchmarkMakeIdentity = $null
$benchmarkRomIdentity = $null
$benchmarkElfIdentity = $null
$benchmarkMelonIdentity = $null
$benchmarkMelonConfigSha256 = $null
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') { $values += [int64](Convert-MarkerUInt32 $text) }
        else { $values += [int64]$text }
    }
    return $values
}
function Test-DamageDigits {
    param([int64]$Damage, [int64]$DigitCount, [int64]$DigitsPack)
    if ($Damage -le 0) { return $false }
    if ($Damage -gt 999) { $Damage = 999 }
    $damageText = [string]$Damage
    $expected = @(10)
    for ($i = $damageText.Length - 1; $i -ge 0; $i--) {
        $expected += [int]([string]$damageText[$i])
    }
    if ($DigitCount -ne $expected.Count) { return $false }
    for ($i = 0; $i -lt $expected.Count; $i++) {
        $digit = ($DigitsPack -shr (8 * $i)) -band 0xff
        if ($digit -ne $expected[$i]) { return $false }
    }
    return $true
}
function Get-Median {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    if ($ordered.Count -eq 0) { return 0 }
    $middle = [int]($ordered.Count / 2)
    if (($ordered.Count % 2) -eq 0) {
        return [int64](($ordered[$middle - 1] + $ordered[$middle]) / 2)
    }
    return [int64]$ordered[$middle]
}
function Get-Percentile95 {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    if ($ordered.Count -eq 0) { return 0 }
    $index = [Math]::Max(0, [Math]::Ceiling($ordered.Count * 0.95) - 1)
    return [int64]$ordered[$index]
}
function Get-AdjacentChurn {
    param([int64[]]$Values)
    if ($Values.Count -eq 0) { return '0/0' }
    $changes = 0
    for ($i = 1; $i -lt $Values.Count; $i++) {
        if ($Values[$i] -ne $Values[$i - 1]) { $changes++ }
    }
    $distinct = @($Values | Sort-Object -Unique).Count
    return "$changes/$distinct"
}
function Get-MedianP95 {
    param([int64[]]$Values)
    return "$(Get-Median $Values)/$(Get-Percentile95 $Values)"
}
function Get-UnsignedMarkerMatches {
    param([string]$Text, [string]$Name, [int]$FieldCount)
    $fieldPattern = ((1..$FieldCount | ForEach-Object { '([0-9]+)' }) -join ',')
    return [regex]::Matches($Text, ([regex]::Escape("$Name=") + $fieldPattern))
}
function Get-SampleFieldValues {
    param([System.Collections.IEnumerable]$Samples, [int]$FieldIndex)
    $values = @()
    foreach ($sample in $Samples) {
        $values += [int64]$sample[$FieldIndex]
    }
    return [int64[]]$values
}
function Get-RatioBasisPoints {
    param([int64]$Numerator, [int64]$Denominator)
    if ($Denominator -le 0) { return [int64]0 }
    return [int64](($Numerator * 10000) / $Denominator)
}
function Test-RendererUploadPair {
    param([int64]$Count, [int64]$Bytes)
    # Every renderer profile may sample either a resident frame or one of the
    # exact animated-water upload phases. Profile 2 uses an independent
    # bytewise decoder, but its cache phase is not synchronized to this probe.
    return (($Count -eq 0 -and $Bytes -eq 0) -or
            ($Count -eq 1 -and ($Bytes -eq 4096 -or $Bytes -eq 32768)) -or
            ($Count -eq 2 -and $Bytes -eq 36864))
}
function Get-RendererUploadSequenceHash {
    param([int64[]]$Counts, [int64[]]$Bytes)
    $pairs = for ($i = 0; $i -lt $Counts.Count; $i++) {
        "$($Counts[$i]):$($Bytes[$i])"
    }
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $digest = $sha.ComputeHash(
            [System.Text.Encoding]::UTF8.GetBytes(($pairs -join ';')))
        return ([BitConverter]::ToString($digest)).Replace('-', '')
    } finally {
        $sha.Dispose()
    }
}
function Get-RendererOwnerBenchmarkCommand {
    param([ValidateRange(0,2)][int]$OwnerIndex)
    $owner = "gNdsRendererProfileOwners[$OwnerIndex]"
    return ('printf "OWNER_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, {0}, {1}.exclusive_ticks, {1}.selected_count, {1}.source_command_count, {1}.vertex_command_count, {1}.source_vertex_count, {1}.triangle_command_count, {1}.triangle_count, {1}.submit_class_count[0], {1}.submit_class_count[1], {1}.submit_class_count[2], {1}.submit_class_count[3], {1}.submit_class_count[4], {1}.submit_class_count[5], {1}.submit_class_count[6], {1}.submit_class_count[7], {1}.material_operation_count, {1}.matrix_change_count, {1}.texture_change_count, {1}.run_count, {1}.entry_state_hash, {1}.exit_state_hash, {1}.entry_vertex_cache_hash, {1}.exit_vertex_cache_hash, {1}.entry_resolver_hash, {1}.exit_resolver_hash, {1}.entry_global_hash, {1}.exit_global_hash, {1}.topology_signature, {1}.selected_event_signature, {1}.camera_signature, {1}.dobj_matrix_signature, {1}.material_signature, {1}.light_signature, {1}.texture_signature, {1}.semantic_output_hash' -f $OwnerIndex, $owner)
}
function Get-RendererSemanticBenchmarkCommand {
    $format = ((1..38 | ForEach-Object { '%u' }) -join ',')
    $arguments = @(
        'gNdsRendererProfileFrameCount',
        'gNdsRendererSemanticOutputHash',
        'gNdsRendererSemanticOutputHash2',
        'gNdsRendererSemanticEventCount',
        'gNdsRendererSemanticOverflowCount'
    )
    foreach ($ownerIndex in 0..2) {
        $owner = "gNdsRendererProfileOwners[$ownerIndex]"
        $arguments += @(
            "$owner.semantic_output_hash",
            "$owner.semantic_output_hash2",
            "$owner.semantic_event_count",
            "$owner.semantic_overflow_count",
            "$owner.semantic_occurrence_count",
            "$owner.semantic_first_owner_occurrence",
            "$owner.semantic_first_list_ordinal",
            "$owner.semantic_first_branch_path",
            "$owner.semantic_first_command_index",
            "$owner.semantic_first_tri2_half",
            "$owner.semantic_first_outcome"
        )
    }
    return ('printf "RENDER_SEMANTIC={0}\n", {1}' -f $format, ($arguments -join ', '))
}
function Get-BenchmarkMakeIdentity {
    param([string[]]$BaseMakeArgs)

    $identityArgs = @($BaseMakeArgs) + 'print-benchmark-flags'
    # The verifier imports a `make` wrapper that deliberately honors
    # SMASH64DS_VERIFY_NO_BUILD.  Identity is a read-only Makefile query and
    # must still run for same-ROM -NoBuild samples, so invoke the application.
    $makeExe = (Get-Command make.exe -CommandType Application).Source
    $identityOutput = @(& $makeExe @identityArgs 2>&1 |
        ForEach-Object { "$_" })
    if ($LASTEXITCODE -ne 0) {
        throw "Makefile benchmark-flag query failed.`n$($identityOutput -join "`n")"
    }
    $values = @{}
    foreach ($line in $identityOutput) {
        $match = [regex]::Match($line, '^BENCH_MAKE_([A-Z0-9_]+)=(.*)$')
        if ($match.Success) {
            $values[$match.Groups[1].Value] = $match.Groups[2].Value
        }
    }
    $required = @(
        'TARGET', 'HARNESS', 'HARNESS_ID', 'PROFILE',
        'RENDERER_BENCHMARK_MODE',
        'CFLAGS_COMMON', 'CFLAGS_RENDERER', 'CFLAGS_SCENE'
    )
    foreach ($key in $required) {
        if (-not $values.ContainsKey($key)) {
            throw "Makefile benchmark-flag query omitted BENCH_MAKE_$key.`n$($identityOutput -join "`n")"
        }
    }
    return [PSCustomObject]@{
        Target = $values.TARGET
        Harness = $values.HARNESS
        HarnessId = [int]$values.HARNESS_ID
        Profile = [int]$values.PROFILE
        RendererBenchmarkMode = [int]$values.RENDERER_BENCHMARK_MODE
        CommonCFlags = $values.CFLAGS_COMMON
        RendererCFlags = $values.CFLAGS_RENDERER
        SceneCFlags = $values.CFLAGS_SCENE
    }
}
function Get-BenchmarkFileIdentity {
    param([Parameter(Mandatory=$true)][string]$Path)

    $item = Get-Item -LiteralPath $Path
    return [PSCustomObject]@{
        Sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
        Bytes = [int64]$item.Length
        LastWriteUtc = $item.LastWriteTimeUtc.ToString('o')
    }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$makeArgs = @('-C', $root, "TARGET=$Target", "BUILD=$Build", "NDS_DEV_SCENE_HARNESS=$Harness", '-j16')
$makeArgs += "NDS_RENDERER_PROFILE_LEVEL=$RendererProfileLevel"
if ($ImportBattleShipFTManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTMANAGER=1'
}
if ($ImportBattleShipFTComputer) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTCOMPUTER=1'
}
if ($ImportBattleShipVSResults) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_VS_RESULTS=1'
}
if ($ImportBattleShipBattlePlayable) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1'
}
if ($ImportBattleShipIFCommon) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_IFCOMMON=1'
}
if ($ImportBattleShipMarioFireball) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL=1'
}
if ($ImportBattleShipFoxBlaster) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_BLASTER=1'
}
if ($ImportBattleShipEffectManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER=1'
}
if ($ImportBattleShipFoxReflector) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR=1'
}
if ($ImportBattleShipMarioSpecialHi) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI=1'
}
if ($ImportBattleShipMarioSpecialLw) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW=1'
}
if ($ImportBattleShipFoxSpecialHi) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI=1'
}
if ($ImportBattleShipAudioAssets) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS=1'
}
if ($ImportBattleShipAudioBGM) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_AUDIO_BGM=1'
}
if ($ImportBattleShipNormalMoveset) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET=1'
}
if ($MatchLifecycleProof) {
    $makeArgs += 'NDS_DEV_RESULTS_VISUAL_SMOKE=1'
}
if ($HardwareTriangles) {
    $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
}
if ($LiveInputPreview) {
    $makeArgs += 'NDS_DEV_LIVE_INPUT_PREVIEW=1'
}
if ($RealtimePresentation) {
    $makeArgs += 'NDS_HARNESS_FAST_LOGIC=0'
} else {
    $makeArgs += 'NDS_HARNESS_FAST_LOGIC=1'
}
if (-not $NoBuild) {
    & make @makeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw "$Label harness build did not produce the expected ROM and ELF."
}
if ($Target -match '^smash64ds-battle-playable-(canonical|coarse(?:-[a-z-]+)?|forensic)-hwtri$') {
    & (Join-Path $PSScriptRoot 'check-renderer-itcm-placement.ps1') `
        -Elf $elf `
        -BenchmarkAblation:($Target -like '*triangle-noop*')
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
if ($RendererBenchmarkSamples -gt 0) {
    $benchmarkMakeIdentity = Get-BenchmarkMakeIdentity -BaseMakeArgs $makeArgs
    Assert-Condition ($benchmarkMakeIdentity.Target -eq $Target -and
        $benchmarkMakeIdentity.Harness -eq $Harness -and
        $benchmarkMakeIdentity.HarnessId -eq $ExpectedMode -and
        $benchmarkMakeIdentity.Profile -eq $RendererProfileLevel) `
        'Makefile benchmark identity does not match the requested verifier target/harness/profile.' `
        ($benchmarkMakeIdentity | Format-List | Out-String)
    $benchmarkRomIdentity = Get-BenchmarkFileIdentity -Path $rom
    $benchmarkElfIdentity = Get-BenchmarkFileIdentity -Path $elf
    $melonItem = Get-Item -LiteralPath $melonDsPath
    $melonVersion = $melonItem.VersionInfo.ProductVersion
    if ([string]::IsNullOrWhiteSpace($melonVersion)) {
        $melonVersion = $melonItem.VersionInfo.FileVersion
    }
    if ([string]::IsNullOrWhiteSpace($melonVersion)) {
        $melonVersion = 'unknown'
    }
    $benchmarkMelonIdentity = [PSCustomObject]@{
        Version = $melonVersion
        Sha256 = (Get-FileHash -LiteralPath $melonDsPath -Algorithm SHA256).Hash
        Path = $melonDsPath
    }
}
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath -GdbPort $verifierContext.GdbPort -Persistent:([bool]$verifierContext.PersistentConfig)
    if ($RendererBenchmarkSamples -gt 0) {
        $benchmarkMelonConfigSha256 =
            (Get-FileHash -LiteralPath $configState.Config -Algorithm SHA256).Hash
    }
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $verifierContext.GdbPort | Out-Null
    # The natural combat chain runs ~1000+ bounded updates; battle_playable
    # continues into an input-driven KO -> Rebirth -> Wait. The lifecycle ROM
    # uses a one-minute source timer and gets one GDB session after startup;
    # repeated attach/detach cycles are unreliable in the melonDS GDB stub.
    $minimumDelay = if ($MatchLifecycleProof) {
        85
    } elseif ($BattlePlayable -and $RealtimePresentation -and
              ($RendererBenchmarkSamples -gt 0)) {
        # The synchronized breakpoint itself captures the requested warm
        # sample span. Eight seconds lets both canonical texture uploads
        # settle while keeping 128-frame profiles inside one logic-clock
        # epoch at the current uncapped benchmark rate.
        8
    } elseif ($BattlePlayable -and $RealtimePresentation) {
        12
    } elseif ($BattlePlayable) {
        3
    } else {
        15
    }
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, $minimumDelay))
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        'printf "AOBJ32=%u,%u,%u,%u,%#x,%#x\n", gNdsAObjEvent32NormalizeScriptCount, gNdsAObjEvent32NormalizeCommandCount, gNdsAObjEvent32NormalizeReuseCount, gNdsAObjEvent32NormalizeFailCount, gNdsAObjEvent32NormalizeFirstSourceWord, gNdsAObjEvent32NormalizeFirstNativeWord',
        'printf "AOBJ32_FAIL=%u,%u,%#x,%#x,%u,%#x\n", gNdsAObjEvent32NormalizeLastFailReason, gNdsAObjEvent32NormalizeLastFailOwner, gNdsAObjEvent32NormalizeLastFailAddress, gNdsAObjEvent32NormalizeLastFailWord, gNdsAObjEvent32NormalizeLastFailOpcode, gNdsAObjEvent32NormalizeLastFailFlags',
        'printf "MOBJ_ATTACH=%u,%u,%u,%#x,%#x\n", gNdsMObjSubAttachNormalizeCount, gNdsMObjSubAttachNativeCount, gNdsMObjSubAttachFailCount, gNdsMObjSubAttachFirstSourceFlags, gNdsMObjSubAttachFirstNativeFlags',
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind',
        'printf "BUILD_MODE=%#x,%#x,%#x\n", gNdsBuildModeCanonicalWord, gNdsBuildModeShippedWord, gNdsBuildModeFastWord',
        'printf "VS_TRANS=%#x,%#x\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask',
        'printf "PV_TRANS=%#x,%#x\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask',
        'printf "MAPS_TRANS=%#x,%#x,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionSelectedGKind',
        'printf "PREV_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxPreviewLoopResult, gNdsFighterMarioFoxPreviewLoopSafeResult, gNdsFighterMarioFoxPreviewLoopMask, gNdsFighterMarioFoxPreviewLoopDeferredMask, gNdsFighterMarioFoxPreviewLoopCount',
        'printf "GCRUNALL_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxGCRunAllLoopResult, gNdsFighterMarioFoxGCRunAllLoopSafeResult, gNdsFighterMarioFoxGCRunAllLoopMask, gNdsFighterMarioFoxGCRunAllLoopDeferredMask, gNdsFighterMarioFoxGCRunAllLoopCount, gNdsFighterGCRunAllLoopFrameMax, gNdsFighterGCRunAllLoopUpdateMax',
        'printf "GCRUNALL_TASKMAN=%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopPrepared, gNdsFighterGCRunAllLoopTaskmanUpdateCount, gNdsFighterGCRunAllLoopVSBattleUpdateCount, gNdsFighterGCRunAllLoopBaseVSBattleUpdateCount, gNdsFighterGCRunAllLoopRunAllCount, gNdsTaskmanBoundedUpdateCount',
        'printf "NAT_MOTION=%#x,%#x,%#x,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsFighterNaturalMotionResult, gNdsFighterNaturalMotionSafeResult, gNdsFighterNaturalMotionMask, gNdsFighterNaturalMotionPrepared, gNdsFighterNaturalMotionUpdateCount, gNdsFighterNaturalMotionBaseVSBattleUpdateCount, gNdsFighterNaturalMotionRunAllCount, gNdsFighterNaturalMotionControllerReadCount, gNdsFighterNaturalMotionManagerMask, gNdsFighterNaturalMotionGObjDelta, gNdsFighterNaturalMotionUnsafeCount',
        'printf "NAT_FIG=%u,%u,%u,%u\n", gNdsFighterNaturalMotionFigatreeAttachCount, gNdsFighterNaturalMotionFigatreeNullCount, gNdsFighterNaturalMotionFigatreeTableInvalidCount, gNdsFighterNaturalMotionFigatreeAnimInvalidCount',
        'printf "NAT_WAIT=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterNaturalMotionP0WaitFrameCount, gNdsFighterNaturalMotionP1WaitFrameCount, gNdsFighterNaturalMotionP0AnimAdvanceCount, gNdsFighterNaturalMotionP1AnimAdvanceCount, gNdsFighterNaturalMotionP0ValidJointCount, gNdsFighterNaturalMotionP1ValidJointCount, gNdsFighterNaturalMotionP0AnimStartBits, gNdsFighterNaturalMotionP1AnimStartBits, gNdsFighterNaturalMotionP0AnimFinalBits, gNdsFighterNaturalMotionP1AnimFinalBits',
        'printf "NAT_WALK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalMotionWalkInputFrame, gNdsFighterNaturalMotionP0WalkFrameCount, gNdsFighterNaturalMotionP1WalkFrameCount, gNdsFighterNaturalMotionP0StatusStart, gNdsFighterNaturalMotionP1StatusStart, gNdsFighterNaturalMotionP0StatusFinal, gNdsFighterNaturalMotionP1StatusFinal, gNdsFighterNaturalMotionP0WalkStatus, gNdsFighterNaturalMotionP1WalkStatus, gNdsFighterNaturalMotionP0WalkMotion, gNdsFighterNaturalMotionP1WalkMotion',
        'printf "NAT_CHAIN=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalCombatPhase, gNdsFighterNaturalCombatPhaseFrames, gNdsFighterNaturalCombatStallCount, gNdsFighterNaturalCombatP0DashFrames, gNdsFighterNaturalCombatP1DashFrames, gNdsFighterNaturalCombatP0RunFrames, gNdsFighterNaturalCombatP1RunFrames, gNdsFighterNaturalCombatP0RunBrakeFrames, gNdsFighterNaturalCombatP1RunBrakeFrames, gNdsFighterNaturalCombatP0TurnFrames, gNdsFighterNaturalCombatP1TurnFrames, gNdsFighterNaturalCombatApproachDXMilli',
        'printf "NAT_ATTACK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalCombatAttackerSlot, gNdsFighterNaturalCombatVictimSlot, gNdsFighterNaturalCombatAttackStatusFrames, gNdsFighterNaturalCombatAttackMotionFinal, gNdsFighterNaturalCombatHitboxActiveFrames, gNdsFighterNaturalCombatAttackRetryCount, gNdsFighterNaturalCombatVictimDamageStatus, gNdsFighterNaturalCombatVictimDamageFrames, gNdsFighterNaturalCombatVictimStartPercent, gNdsFighterNaturalCombatVictimFinalPercent, gNdsFighterNaturalCombatVictimKnockbackMilli, gNdsFighterNaturalCombatVictimRecoverWaitFrames',
        'printf "NAT_MOVESET=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%u,%u\n", gNdsFighterNaturalMovesetMask, gNdsFighterNaturalMovesetPhase, gNdsFighterNaturalMovesetPhaseFrames, gNdsFighterNaturalMovesetTiltS3Frames, gNdsFighterNaturalMovesetTiltHi3Frames, gNdsFighterNaturalMovesetTiltLw3Frames, gNdsFighterNaturalMovesetTiltHitboxFrames, gNdsFighterNaturalMovesetSmashFrames, gNdsFighterNaturalMovesetSmashHitboxFrames, gNdsFighterNaturalMovesetAerialFrames, gNdsFighterNaturalMovesetAerialHitboxFrames, gNdsFighterNaturalMovesetLandingFrames, gNdsFighterNaturalMovesetCatchFrames, gNdsFighterNaturalMovesetCatchWaitFrames, gNdsFighterNaturalMovesetThrowFrames, gNdsFighterNaturalMovesetThrownFrames, gNdsFighterNaturalMovesetThrowRecoverFrames, gNdsFighterNaturalMovesetAttackerStatus, gNdsFighterNaturalMovesetAttackerMotion, gNdsFighterNaturalMovesetAttackerGA, gNdsFighterNaturalMovesetAttackerRootYMilli, gNdsFighterNaturalMovesetVictimStatus, gNdsFighterNaturalMovesetVictimMotion, gNdsFighterNaturalMovesetVictimGA, gNdsFighterNaturalMovesetVictimRootYMilli, gNdsFighterNaturalMovesetThrowDamageBefore, gNdsFighterNaturalMovesetThrowDamageAfter',
        'printf "NAT_HITBOX=%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%#x\n", gNdsFighterDashRunAttackEventLastPlayer, gNdsFighterDashRunAttackEventLastStatus, gNdsFighterDashRunAttackEventLastState, gNdsFighterDashRunAttackEventLastAttackID, gNdsFighterDashRunAttackEventLastGroupID, gNdsFighterDashRunAttackEventLastDamage, gNdsFighterDashRunAttackEventLastSize, gNdsFighterDashRunAttackEventLastOffsetX, gNdsFighterDashRunAttackEventLastOffsetY, gNdsFighterDashRunAttackEventLastOffsetZ, gNdsFighterDashRunAttackEventLastAngle, gNdsFighterDashRunAttackEventLastKBG, gNdsFighterDashRunAttackEventLastKBW, gNdsFighterDashRunAttackEventLastBKB, gNdsFighterDashRunAttackEventLastFlags',
        'printf "NAT_HITLAG=%u,%u\n", gNdsFighterNaturalCombatP0HitlagFrames, gNdsFighterNaturalCombatP1HitlagFrames',
        'printf "NAT_GUARD=%u,%u,%u\n", gNdsFighterNaturalCombatGuardOnFrames, gNdsFighterNaturalCombatGuardFrames, gNdsFighterNaturalCombatGuardOffFrames',
        'printf "BPLAY_GEOM=%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleStageGroundDataReady, (unsigned int)gMPCollisionGroundData, (unsigned int)gMPCollisionGeometry, (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->line_info : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_links : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_id : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_data : 0), gNdsPupupuGroundDeferredMask, gNdsStageCollisionLoopGeometryReady, gNdsStageCollisionLoopGroundDataReady, gNdsStageCollisionLoopFloorLineCount, gNdsStageCollisionLoopFloorLineMin, gNdsStageCollisionLoopFloorLineMaxExclusive, gNdsStageMPSweepFloorLoopLineSweepDiffCallCount, gNdsStageMPSweepFloorLoopLineSweepDiffHitCount, gNdsStageMPSweepFloorLoopLineSweepDiffMissCount, gNdsStageMPSweepFloorLoopLineSweepVisitCount, gNdsStageMPSweepFloorLoopLineSweepCandidateCount, gNdsStageMPSweepFloorLoopLineSweepSameHitCount',
        'printf "BPLAY_WALLPAPER=%u,%#x\n", gNdsStagePupupuWallpaperPtrReady, (unsigned int)((gMPCollisionGroundData != 0) ? gMPCollisionGroundData->wallpaper : 0)',
        'printf "GCRUNALL_RUN=%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopOldProcessPauseCount, gNdsFighterGCRunAllLoopNonTargetGObjVisitCount, gNdsFighterGCRunAllLoopNonTargetProcessPauseCount, gNdsFighterGCRunAllLoopTargetProcessPreserveCount, gNdsFighterGCRunAllLoopGObjCountBefore, gNdsFighterGCRunAllLoopGObjCountAfter',
        'printf "GCRUNALL_PROCESS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0ProcessAttachCount, gNdsFighterGCRunAllLoopP1ProcessAttachCount, gNdsFighterGCRunAllLoopP0GObjProcessRunCount, gNdsFighterGCRunAllLoopP1GObjProcessRunCount, gNdsFighterGCRunAllLoopP0ProcCallbackCount, gNdsFighterGCRunAllLoopP1ProcCallbackCount, gNdsFighterGCRunAllLoopProcessAttachEscapeCount',
        'printf "GCRUNALL_INPUT=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterGCRunAllLoopP0PlaybackApplyCount, gNdsFighterGCRunAllLoopP1PlaybackApplyCount, gNdsFighterGCRunAllLoopP0ControllerToFTInputCount, gNdsFighterGCRunAllLoopP1ControllerToFTInputCount, gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount, gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount, gNdsFighterGCRunAllLoopP0DashTapEligibleCount, gNdsFighterGCRunAllLoopP1DashTapEligibleCount, gNdsFighterGCRunAllLoopP0ButtonTapMask, gNdsFighterGCRunAllLoopP1ButtonTapMask, gNdsFighterGCRunAllLoopP0ButtonHoldMask, gNdsFighterGCRunAllLoopP1ButtonHoldMask, gNdsFighterGCRunAllLoopP0JumpButtonTapCount, gNdsFighterGCRunAllLoopP1JumpButtonTapCount',
        'printf "GCRUNALL_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterGCRunAllLoopP0StatusStart, gNdsFighterGCRunAllLoopP1StatusStart, gNdsFighterGCRunAllLoopP0MotionStart, gNdsFighterGCRunAllLoopP1MotionStart, gNdsFighterGCRunAllLoopP0StatusFinal, gNdsFighterGCRunAllLoopP1StatusFinal, gNdsFighterGCRunAllLoopP0MotionFinal, gNdsFighterGCRunAllLoopP1MotionFinal, gNdsFighterGCRunAllLoopP0GAFinal, gNdsFighterGCRunAllLoopP1GAFinal, gNdsFighterGCRunAllLoopP0StatusVisitMask, gNdsFighterGCRunAllLoopP1StatusVisitMask, gNdsFighterGCRunAllLoopP0TransitionMask, gNdsFighterGCRunAllLoopP1TransitionMask',
        'printf "GCRUNALL_VISITS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0WaitVisitCount, gNdsFighterGCRunAllLoopP1WaitVisitCount, gNdsFighterGCRunAllLoopP0WalkVisitCount, gNdsFighterGCRunAllLoopP1WalkVisitCount, gNdsFighterGCRunAllLoopP0DashVisitCount, gNdsFighterGCRunAllLoopP1DashVisitCount, gNdsFighterGCRunAllLoopP0RunVisitCount, gNdsFighterGCRunAllLoopP1RunVisitCount, gNdsFighterGCRunAllLoopP0RunBrakeVisitCount, gNdsFighterGCRunAllLoopP1RunBrakeVisitCount, gNdsFighterGCRunAllLoopP0KneeBendVisitCount, gNdsFighterGCRunAllLoopP1KneeBendVisitCount, gNdsFighterGCRunAllLoopP0JumpVisitCount, gNdsFighterGCRunAllLoopP1JumpVisitCount, gNdsFighterGCRunAllLoopP0FallVisitCount, gNdsFighterGCRunAllLoopP1FallVisitCount, gNdsFighterGCRunAllLoopP0LandingVisitCount, gNdsFighterGCRunAllLoopP1LandingVisitCount',
        'printf "GCRUNALL_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0FrameCount, gNdsFighterGCRunAllLoopP1FrameCount, gNdsFighterGCRunAllLoopP0Completed, gNdsFighterGCRunAllLoopP1Completed, gNdsFighterGCRunAllLoopP0InterruptCount, gNdsFighterGCRunAllLoopP1InterruptCount, gNdsFighterGCRunAllLoopP0PhysicsCount, gNdsFighterGCRunAllLoopP1PhysicsCount, gNdsFighterGCRunAllLoopP0MapCount, gNdsFighterGCRunAllLoopP1MapCount',
        'printf "GCRUNALL_DRAW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterGCRunAllLoopPreviewWidth, gNdsFighterGCRunAllLoopPreviewHeight, gNdsFighterGCRunAllLoopPreviewPitch, gNdsFighterGCRunAllLoopPreviewReady, gNdsFighterGCRunAllLoopPreviewCommitDelta, gNdsFighterGCRunAllLoopDrawFrameCount, gNdsFighterGCRunAllLoopDisplayCallbackCount, gNdsFighterGCRunAllLoopP0DisplayCallbackCount, gNdsFighterGCRunAllLoopP1DisplayCallbackCount, gNdsFighterGCRunAllLoopP0CandidateCount, gNdsFighterGCRunAllLoopP1CandidateCount, gNdsFighterGCRunAllLoopP0DrawnDObjCount, gNdsFighterGCRunAllLoopP1DrawnDObjCount, gNdsFighterGCRunAllLoopTotalPixelCount, gNdsFighterGCRunAllLoopP0ColorChecksum, gNdsFighterGCRunAllLoopP1ColorChecksum',
        'printf "GCRUNALL_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterGCRunAllLoopP0ScreenXStart, gNdsFighterGCRunAllLoopP1ScreenXStart, gNdsFighterGCRunAllLoopP0ScreenXFinal, gNdsFighterGCRunAllLoopP1ScreenXFinal, gNdsFighterGCRunAllLoopP0ScreenXDelta, gNdsFighterGCRunAllLoopP1ScreenXDelta, gNdsFighterGCRunAllLoopP0ScreenYFloor, gNdsFighterGCRunAllLoopP1ScreenYFloor, gNdsFighterGCRunAllLoopP0ScreenRise, gNdsFighterGCRunAllLoopP1ScreenRise',
        'printf "GCRUNALL_MOVE=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0FloorYMilli, gNdsFighterGCRunAllLoopP1FloorYMilli, gNdsFighterGCRunAllLoopP0RootDeltaXMilli, gNdsFighterGCRunAllLoopP1RootDeltaXMilli, gNdsFighterGCRunAllLoopP0RootRiseMilli, gNdsFighterGCRunAllLoopP1RootRiseMilli, gNdsFighterGCRunAllLoopP0RootYFinalMilli, gNdsFighterGCRunAllLoopP1RootYFinalMilli, gNdsFighterGCRunAllLoopP0RootDirectionOK, gNdsFighterGCRunAllLoopP1RootDirectionOK, gNdsFighterGCRunAllLoopP0FloorOK, gNdsFighterGCRunAllLoopP1FloorOK',
        'printf "GCRUNALL_TRANS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopFallDetectCount, gNdsFighterGCRunAllLoopLandingDetectCount, gNdsFighterGCRunAllLoopSetGroundCount, gNdsFighterGCRunAllLoopSetAirCount, gNdsFighterGCRunAllLoopWaitSetStatusCount, gNdsFighterGCRunAllLoopRunBrakeEndCount, gNdsFighterGCRunAllLoopJumpAnimEndCount, gNdsFighterGCRunAllLoopLandingEndCount, gNdsFighterGCRunAllLoopDeferredInterruptCheckCount',
        'printf "GCRUNALL_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopGObjDelta, gNdsFighterGCRunAllLoopUnexpectedStatusCount, gNdsFighterGCRunAllLoopDeniedStatusCount, gNdsFighterGCRunAllLoopProcessAttachEscapeCount, gNdsFighterGCRunAllLoopDisplayProbeCount, gNdsFighterGCRunAllLoopGameplayUpdateCount, gNdsFighterGCRunAllLoopDrawCallCount, gNdsFighterGCRunAllLoopMatrixCallCount, gNdsFighterGCRunAllLoopRootYDriftCount, gNdsFighterGCRunAllLoopGADriftCount',
        'printf "PLATFORM_DL_PREVIEW=%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'printf "LIVE_PAD=%u,%u,%#x,%#x,%#x,%d,%d,%#x,%d,%d,%u,%u,%#x,%#x,%d,%d,%#x,%#x,%d,%d,%d\n", gNdsControllerLiveReadCount, gNdsControllerLiveMapCount, gNdsControllerLiveConnectedMask, gNdsPlatformHeldKeys, gNdsControllerLivePad0Button, gNdsControllerLivePad0StickX, gNdsControllerLivePad0StickY, gNdsControllerLivePad1Button, gNdsControllerLivePad1StickX, gNdsControllerLivePad1StickY, gNdsControllerPlaybackEnabled, gNdsControllerPlaybackReadCount, gSYControllerDevices[0].button_hold, gSYControllerDevices[0].button_tap, gSYControllerDevices[0].stick_range.x, gSYControllerDevices[0].stick_range.y, gSYControllerDevices[1].button_hold, gSYControllerDevices[1].button_tap, gSYControllerDevices[1].stick_range.x, gSYControllerDevices[1].stick_range.y, gNdsFighterBattlePlayableFinalXMilli',
        'detach',
        'quit'
    )
    if ($BattlePlayable -and $RealtimePresentation -and $HardwareTriangles -and -not $MatchLifecycleProof) {
        if ($RendererBenchmarkSamples -gt 0) {
            $coarseBenchmarkCommands = @()
            if ($RendererProfileLevel -ge 1) {
                $coarseBenchmarkCommands += 'printf "COARSE_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileLoopWallTicks, gNdsRendererProfileInputTicks, gNdsRendererProfileUpdateTicks, gNdsRendererProfileSourceUpdateTicks, gNdsRendererProfileAudioUpdateTicks, gNdsRendererProfilePresentActiveTicks, gNdsRendererProfileVBlankWaitTicks, gNdsRendererProfileBeginFrameTicks, gNdsRendererProfileDrawTicks, gNdsRendererProfileWallpaperTicks, gNdsRendererProfileOwners[0].exclusive_ticks, gNdsRendererProfileOwners[1].exclusive_ticks, gNdsRendererProfileOwners[2].exclusive_ticks, gNdsRendererProfileForegroundTicks, gNdsRendererProfileHudTicks, gNdsRendererProfileFlushTicks, gNdsRendererProfilePostVBlankTicks, gNdsRendererProfileThreadTicks, gNdsRendererProfileDrawResidualTicks, gNdsRendererProfilePresentResidualTicks, gNdsRendererProfileLoopResidualTicks, gNdsRendererProfileConservationErrorTicks, gNdsRendererProfileLogicTick'
                $coarseBenchmarkCommands += 'printf "STAGE0_BENCH=%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileStageLayer0Ticks'
                $coarseBenchmarkCommands += 'printf "GX_BOUNDARY=%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileGXStatusBeforeFlush, gNdsRendererProfileGXControlBeforeFlush, gNdsRendererProfileGXStatusAfterFlush, gNdsRendererProfileGXStatusPostVBlank, gNdsRendererProfileGXControlPostVBlank, gNdsRendererProfileFlushTicks'
                if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                    $coarseBenchmarkCommands += 'printf "SINK_BENCH=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBenchmarkSinkCursor, gNdsRendererBenchmarkSinkWordCount, gNdsRendererBenchmarkSinkCalibrationWords, gNdsRendererBenchmarkSinkCalibrationTicks, gNdsRendererBenchmarkSinkOwnerWords[0], gNdsRendererBenchmarkSinkOwnerWords[1], gNdsRendererBenchmarkSinkOwnerWords[2]'
                }
                if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                    $coarseBenchmarkCommands += 'printf "WARM_BENCH=%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererBenchmarkSuppressedTextureUploads, gNdsRendererBenchmarkSuppressedTextureUploadBytes'
                }
                if ($RendererProfileLevel -ge 2) {
                    foreach ($ownerIndex in 0..2) {
                        $coarseBenchmarkCommands += Get-RendererOwnerBenchmarkCommand -OwnerIndex $ownerIndex
                    }
                    $coarseBenchmarkCommands += Get-RendererSemanticBenchmarkCommand
                }
            }
            $benchmarkTriangleSymbol = if ($RendererBenchmarkOnly) {
                'gNdsRendererBenchmarkTriangleCount'
            } else {
                'gNdsRendererProfileHardwareTriangles'
            }
            $rendererBenchmarkCommand =
                'printf "RENDER_BENCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileLevel, gNdsRendererProfileFrameCount, gNdsRendererProfilePresentTicks, gNdsRendererProfileDrawTicks, gNdsRendererProfileStageAdapterTicks, gNdsRendererProfileMaterialTicks, gNdsRendererProfileMatrixTicks, gNdsRendererProfileDLTicks, gNdsRendererProfileTextureTicks, gNdsRendererProfileTriangleSubmitTicks, gNdsRendererProfileVertexSubmitTicks, {0}, gNdsRendererProfileOracleSamples, gNdsRendererProfileTextureUploads, gNdsRendererProfileTextureUploadBytes' -f $benchmarkTriangleSymbol
            $gdbCommands = @(
                $gdbCommands[0..3]
                'set $renderer_benchmark_samples = 0'
                'break ndsBattlePlayableFrameCompleteMarker'
                'commands'
                'silent'
                'if gNdsBattlePlayablePacingResult != 0'
                $rendererBenchmarkCommand
                $coarseBenchmarkCommands
                'set $renderer_benchmark_samples = $renderer_benchmark_samples + 1'
                'end'
                ('if $renderer_benchmark_samples < {0}' -f $RendererBenchmarkSamples)
                'continue'
                'end'
                'end'
                'continue'
                $gdbCommands[4..($gdbCommands.Count - 1)]
            )
        } else {
            $gdbCommands = @(
                $gdbCommands[0..3]
                'tbreak ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingResult != 0'
                'continue'
                $gdbCommands[4..($gdbCommands.Count - 1)]
            )
        }
    }
    elseif ($BattlePlayable -and -not $RealtimePresentation -and -not $MatchLifecycleProof) {
        # Fast-logic mode can still be inside its expensive final draw after
        # the gameplay proof completes. Sample only after taskman teardown has
        # stopped BGM and published the scene boundary.
        $gdbCommands = @(
            $gdbCommands[0..3]
            'tbreak osStopThread if gNdsSceneBoundaryResult != 0'
            'continue'
            $gdbCommands[4..($gdbCommands.Count - 1)]
        )
    }
    if ($HardwareTriangles) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $hardwareCommands = @(
            'printf "RENDER_PROFILE_LEVEL=%u\n", gNdsRendererProfileLevel',
            'printf "PLATFORM_HW=%u,%u,%u,%u,%#x,%#x\n", gNdsHardwareRendererSubmittedFrameCount, gNdsHardwareRendererFlushCount, gNdsHardwareRendererPolyRamCount, gNdsHardwareRendererVertexRamCount, gNdsHardwareRendererStatus, gNdsHardwareRendererControl',
            'printf "STAGE_GCDRAWALL_HW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsStageGCDrawAllLoopHardwareSubmitCount, gNdsStageGCDrawAllLoopHardwareTriangleCount, gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount, gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount, gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount, gNdsStageGCDrawAllLoopHardwareTextureBindCount, gNdsStageGCDrawAllLoopHardwareTextureUploadCount, gNdsStageGCDrawAllLoopHardwareTextureReadyCount, gNdsStageGCDrawAllLoopHardwareTextureRejectCount, gNdsStageGCDrawAllLoopHardwareTextureFormatMask, gNdsStageGCDrawAllLoopHardwareTextureMaxWidth, gNdsStageGCDrawAllLoopHardwareTextureMaxHeight',
            'printf "RENDER_STAGE_CARRY=%u,%u,%u,%u,%u,%u,%u\n", (gNdsStageGCDrawAllLoopHardwareCarrySeedCount < gNdsStageGCDrawAllLoopHardwareCarryCaptureCount) ? gNdsStageGCDrawAllLoopHardwareCarrySeedCount : gNdsStageGCDrawAllLoopHardwareCarryCaptureCount, (gNdsStageGCDrawAllLoopHardwareCarrySeedCount < gNdsStageGCDrawAllLoopHardwareCarryCaptureCount) ? gNdsStageGCDrawAllLoopHardwareCarrySeedCount : gNdsStageGCDrawAllLoopHardwareCarryCaptureCount, gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount, gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount, gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount, gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount, gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount',
            'printf "STAGE_GCDRAWALL_HW_FTR=%u,%u\n", gNdsStageGCDrawAllLoopHardwareFighterSubmitCount, gNdsStageGCDrawAllLoopHardwareFighterTriangleCount',
        'printf "FTR_DISPLAY_CONTRACT=%u,%u,%u,%u,%#x,%u,%u,%u,%u,%#x,%#x,%u,%u,%#x,%#x\n", gNdsFighterDisplayContractSelectedCount, gNdsFighterDisplayContractHiddenCount, gNdsFighterDisplayContractNoTextureCount, gNdsFighterDisplayContractSubmittedCount, gNdsFighterDisplayContractGeometryMode, gNdsFighterDisplayContractLightCount, gNdsFighterDisplayContractLightDirectionCount, gNdsFighterDisplayContractBoundsPassCount, gNdsFighterDisplayContractBoundsFailCount, gNdsFighterDisplayContractBoundsXBits, gNdsFighterDisplayContractBoundsYBits, gNdsFighterDLAllDrawP0SelectedCount, gNdsFighterDLAllDrawP1SelectedCount, gNdsFighterDisplayContractCycleType, gNdsFighterDisplayContractRenderMode',
            'printf "FTR_LIGHT_SEED=%u,%#x,%#x\n", gNdsFighterDisplayContractMaterialLightSeedCount, gNdsFighterDisplayContractMaterialLight1, gNdsFighterDisplayContractMaterialLight2',
            'printf "DLALL_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%#x,%#x,%#x,%#x\n", gNdsFighterDLAllDrawP0ScreenMinX, gNdsFighterDLAllDrawP0ScreenMaxX, gNdsFighterDLAllDrawP0ScreenMinY, gNdsFighterDLAllDrawP0ScreenMaxY, gNdsFighterDLAllDrawP1ScreenMinX, gNdsFighterDLAllDrawP1ScreenMaxX, gNdsFighterDLAllDrawP1ScreenMinY, gNdsFighterDLAllDrawP1ScreenMaxY, gNdsFighterDLAllDrawP0RootXBeforeBits, gNdsFighterDLAllDrawP0RootXAfterBits, gNdsFighterDLAllDrawP1RootXBeforeBits, gNdsFighterDLAllDrawP1RootXAfterBits',
            'printf "RENDER_PROFILE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileUpdateTicks, gNdsRendererProfilePresentTicks, gNdsRendererProfileDrawTicks, gNdsRendererProfileHudTicks, gNdsRendererProfileStageAdapterTicks, gNdsRendererProfileMaterialTicks, gNdsRendererProfileMatrixTicks, gNdsRendererProfileDLTicks, gNdsRendererProfileTextureTicks, gNdsRendererProfileTextureConvertTicks, gNdsRendererProfileTextureUploadTicks, gNdsRendererProfileTextureUploads, gNdsRendererProfileTextureUploadBytes, gNdsRendererProfileTextureBinds, gNdsRendererProfileHardwareVertices, gNdsRendererProfileHardwareTriangles, gNdsRendererProfileHardwareOverLimit',
            'printf "RENDER_BATCH=%u,%u,%u,%u,%u\n", gNdsRendererProfileHardwareBatchBeginCount, gNdsRendererProfileHardwareBatchReuseCount, gNdsRendererProfileHardwareBatchEndCount, gNdsRendererProfileTexturePrepareCount, gNdsRendererProfileTexturePrepareReuseCount',
            'printf "RENDER_TOPOLOGY=%u,%u,%u,%u\n", gNdsRendererProfileImmutableListCount, gNdsRendererProfileTrustedCommandCount, gNdsRendererProfileValidatedCommandCount, gNdsRendererProfileTriangleRunReuseCount',
            'printf "RENDER_COST=%u,%u\n", gNdsRendererProfileTriangleSubmitTicks, gNdsRendererProfileVertexSubmitTicks',
            'printf "RENDER_CI4LUT=%u,%u,%u,%u\n", gNdsRendererProfileCi4LutBuildCount, gNdsRendererProfileCi4LutReuseCount, gNdsRendererProfileCi4IndexCacheBuildCount, gNdsRendererProfileCi4IndexCacheReuseCount',
            'printf "RENDER_CI4MAP=%u,%u\n", gNdsRendererProfileCi4RepresentativePixelCount, gNdsRendererProfileCi4ReusePixelCount',
            'printf "RENDER_TEXHASH=%u,%u,%u,%u,%u\n", gNdsRendererProfileTextureLookupCallCount, gNdsRendererProfileTextureLookupProbeCount, gNdsRendererProfileTextureLookupActiveHitCount, gNdsRendererProfileTextureLookupTableHitCount, gNdsRendererProfileTextureLookupMissCount',
            'printf "RENDER_ORACLE=%u,%u,%u\n", gNdsRendererProfileOracleSamples, gNdsRendererProfileOracleMismatches, gNdsRendererProfileOracleMaxDelta',
            'printf "RENDER_MATRIX=%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsRendererProfileMatrixLoadCount, gNdsRendererProfileMatrixScaleWorld, gNdsRendererProfileProjectionM00, gNdsRendererProfileProjectionM11, gNdsRendererProfileProjectionM22, gNdsRendererProfileProjectionM32, gNdsRendererProfileModelviewM00, gNdsRendererProfileModelviewM11, gNdsRendererProfileModelviewM22, gNdsRendererProfileModelviewM30, gNdsRendererProfileModelviewM31, gNdsRendererProfileModelviewM32',
            'printf "RENDER_ADAPTER_CACHE=%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileCameraMatrixCacheHitCount, gNdsRendererProfileCameraMatrixCacheMissCount, gNdsRendererProfileCameraMatrixCacheOverflowCount, gNdsRendererProfileDObjWorldCacheHitCount, gNdsRendererProfileDObjWorldCacheMissCount, gNdsRendererProfileDObjWorldCacheOverflowCount',
            'printf "RENDER_RAW_MATRIX=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileRawCurrentCandidateCount, gNdsRendererProfileRawCurrentRangeRejectCount, gNdsRendererProfileRawCrossMatrixCount, gNdsRendererProfileMatrixPosTestSamples, gNdsRendererProfileMatrixPosTestMismatches, gNdsRendererProfileMatrixPosTestMaxError, gNdsRendererProfileMatrixPosTestWSignMismatches, gNdsRendererProfileMatrixPosTestClipMismatches, gNdsRendererProfileMatrixPosTestMatrixWordSamples, gNdsRendererProfileMatrixPosTestDropped',
            'printf "RENDER_SUBMIT=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileSubmitRawCurrentCount, gNdsRendererProfileSubmitRawSnapshotCount, gNdsRendererProfileSubmitProjectedCrossCount, gNdsRendererProfileSubmitProjectedNoZCount, gNdsRendererProfileSubmitProjectedDecalCount, gNdsRendererProfileSubmitProjectedPrimDepthCount, gNdsRendererProfileSubmitProjectedRangeOrMatrixCount, gNdsRendererProfileSubmitRejectCount, (6 * gNdsRendererProfileSubmitProjectedNoZCount) + (9 * (gNdsRendererProfileSubmitProjectedCrossCount + gNdsRendererProfileSubmitProjectedDecalCount + gNdsRendererProfileSubmitProjectedPrimDepthCount + gNdsRendererProfileSubmitProjectedRangeOrMatrixCount))',
            'printf "RENDER_HWDIV=%u,%u,%u,%u,%u\n", gNdsRendererProfileHardwareDivideSummary & 0xfff, (gNdsRendererProfileHardwareDivideSummary >> 12) & 0xff, (gNdsRendererProfileHardwareDivideSummary >> 20) & 0xff, (gNdsRendererProfileHardwareDivideSummary >> 28) & 1, (gNdsRendererProfileHardwareDivideSummary >> 29) & 1',
            'printf "RENDER_LAZY=%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileSourceVertexLoadCount, gNdsRendererProfileCPUTransformCount, gNdsRendererProfileTransformCacheHitCount, gNdsRendererProfileMatrixSnapshotCreateCount, gNdsRendererProfileMatrixSnapshotReuseCount, gNdsRendererProfileMatrixSnapshotOverflowCount',
            'printf "RENDER_VERTEX=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", gNdsRendererProfileRawVertexMinX, gNdsRendererProfileRawVertexMaxX, gNdsRendererProfileRawVertexMinY, gNdsRendererProfileRawVertexMaxY, gNdsRendererProfileRawVertexMinZ, gNdsRendererProfileRawVertexMaxZ, gNdsRendererProfileHWVertexMinX, gNdsRendererProfileHWVertexMaxX, gNdsRendererProfileHWVertexMinY, gNdsRendererProfileHWVertexMaxY, gNdsRendererProfileHWVertexMinZ, gNdsRendererProfileHWVertexMaxZ, gNdsRendererProfileHWVertexSaturateCount',
            'printf "RENDER_DEPTH=%u,%d,%d,%d,%d,%u,%d,%d,%d,%d,%u,%d,%d,%d,%d\n", gNdsRendererDepthStageSamples, gNdsRendererDepthStageMin, gNdsRendererDepthStageMax, gNdsRendererDepthStageWMin, gNdsRendererDepthStageWMax, gNdsRendererDepthFighterP0Samples, gNdsRendererDepthFighterP0Min, gNdsRendererDepthFighterP0Max, gNdsRendererDepthFighterP0WMin, gNdsRendererDepthFighterP0WMax, gNdsRendererDepthFighterP1Samples, gNdsRendererDepthFighterP1Min, gNdsRendererDepthFighterP1Max, gNdsRendererDepthFighterP1WMin, gNdsRendererDepthFighterP1WMax',
            'printf "RENDER_CLIP=%u\n", gNdsRendererProfileHWVertexSaturateCount',
            'printf "RENDER_TEXTURE=%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d\n", gNdsRendererProfileTextureSourceTexels, gNdsRendererProfileTextureGreenTexels, gNdsRendererProfileTextureNonWhiteTexels, gNdsRendererProfileTexturedVertexCount, gNdsRendererProfileTextureSampleCount, gNdsRendererProfileTextureSampleGreenCount, gNdsRendererProfileTextureSampleNonWhiteCount, gNdsRendererProfileTextureCacheAliasAvoidCount, gNdsRendererProfileTextureCoordMinS, gNdsRendererProfileTextureCoordMaxS, gNdsRendererProfileTextureCoordMinT, gNdsRendererProfileTextureCoordMaxT',
            'printf "RENDER_TEXEL1=%u,%u,%u,%#x,%u,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsRendererProfileTexel1CompositeCount, gNdsRendererProfileTexel1LoadMatchCount, gNdsRendererProfileTexel1RejectCount, gNdsRendererProfileTexel1RejectReasonMask, gNdsRendererProfileTexel1LastFraction, gNdsRendererProfileTexel1LastImage0, gNdsRendererProfileTexel1LastImage1, gNdsRendererProfileTexel1LastTileState, gNdsRendererProfileTexel1LastPrimaryState, gNdsRendererProfileTexel1FractionRefreshCount, gNdsRendererProfileTextureCacheEvictCount, gNdsRendererProfileTextureCi4DirectPixels',
            'printf "RENDER_TEXUSE=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x\n", gNdsRendererProfileUseTextureRejectNoStatsCount, gNdsRendererProfileUseTextureRejectStateOffCount, gNdsRendererProfileUseTextureRejectNoCombineCount, gNdsRendererProfileUseTextureRejectPrimitiveDecalCount, gNdsRendererProfileUseTextureRejectNoTexel0Count, gNdsRendererProfileUseTextureImplicitOnCount, gNdsRendererProfileUseTextureRejectFirstReason, gNdsRendererProfileUseTextureRejectFirstFlags, gNdsRendererProfileUseTextureRejectFirstW0, gNdsRendererProfileUseTextureRejectFirstW1, gNdsRendererProfileUseTextureRejectFirstGeometry',
            'printf "RENDER_TEXFMT=%#x,%#x,%#x,%#x,%#x\n", gNdsRendererProfileTextureConvertFormatMask, gNdsRendererProfileTextureBindFormatMask, gNdsRendererProfileTexturePaletteFormatMask, gNdsRendererProfileTextureRejectFormatMask, gNdsRendererProfileTextureRejectReasonMask',
            'printf "RENDER_TEXLANE=%#x,%u,%u,%#x,%#x,%#x,%#x\n", gNdsRendererProfileTextureLaneLayoutMask, gNdsRendererProfileTextureLaneByteAccessCount, gNdsRendererProfileTextureLaneHalfwordAccessCount, gNdsRendererProfileTextureLaneByteFormatMask, gNdsRendererProfileTextureLaneHalfwordFormatMask, gNdsRendererProfileTextureLaneByteMap, gNdsRendererProfileTextureLaneHalfwordMap',
            'printf "RENDER_COMBINE=%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsRendererProfileCombineModeCount, gNdsRendererProfileCombineModeDistinctCount, gNdsRendererProfileLitShadeCombineCount, gNdsRendererProfileMaterialCombineCount, gNdsRendererProfileProjectedSubmitFallbackCount, gNdsRendererProfileCombineMode0W0, gNdsRendererProfileCombineMode0W1, gNdsRendererProfileCombineMode1W0, gNdsRendererProfileCombineMode1W1, gNdsRendererProfileCombineMode2W0, gNdsRendererProfileCombineMode2W1, gNdsRendererProfileCombineMode3W0, gNdsRendererProfileCombineMode3W1',
            'printf "RENDER_LIGHT=%u,%u,%u\n", gNdsRendererProfileLightColorCommands, gNdsRendererProfileLightDirectionCommands, gNdsRendererProfileLightFallbackCount'
        )
        if ($BattlePlayable -and $RealtimePresentation) {
            $hardwareCommands += 'printf "SOBJ_WALL_CACHE=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSObjWallpaperCacheBuildCount, gNdsSObjWallpaperCacheHitCount, gNdsSObjWallpaperCacheFastDrawCount, gNdsSObjWallpaperCacheFallbackCount, gNdsSObjWallpaperCacheWidth, gNdsSObjWallpaperCacheHeight, gNdsSObjWallpaperCacheOpaquePixels, gNdsSObjWallpaperCacheBuildTicks, gNdsSObjWallpaperCacheDrawTicks'
            $hardwareCommands += 'printf "SOBJ_WALL_FINAL=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSObjWallpaperFinalDirectCount, gNdsSObjWallpaperFinalSkipCount, gNdsSObjWallpaperFinalKeyChangeCount, gNdsSObjWallpaperFinalPixelWriteCount, gNdsSObjBackgroundStagingClearBytes, gNdsSObjForegroundStagingClearBytes, gNdsOriginalSpriteBg2ClearBytes, gNdsOriginalSpriteBg2CopyBytes, gNdsOriginalSpriteBg2FinalWriteBytes, gNdsOriginalSpriteBg3ClearBytes, gNdsOriginalSpriteBg3CopyBytes, gNdsOriginalSpriteBg3FinalWriteBytes'
        }
        $gdbCommands = @($beforeDetach + $hardwareCommands + $afterDetach)
    }
    if ($BattlePlayable) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $battlePlayableCommands = @(
            'printf "BPLAY_KO=%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterBattlePlayableResult, gNdsFighterBattlePlayableMask, gNdsFighterBattlePlayableVictimSlot, gNdsFighterBattlePlayableVictimStockStart, gNdsFighterBattlePlayableVictimStockFinal, gNdsFighterBattlePlayableBattleStockStart, gNdsFighterBattlePlayableBattleStockFinal, gNdsFighterBattlePlayableFallsStart, gNdsFighterBattlePlayableFallsFinal',
            'printf "BPLAY_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterBattlePlayableDeadFrames, gNdsFighterBattlePlayableRebirthDownFrames, gNdsFighterBattlePlayableRebirthStandFrames, gNdsFighterBattlePlayableRebirthWaitFrames, gNdsFighterBattlePlayableFallAfterRebirthFrames, gNdsFighterBattlePlayableWaitAfterRebirthFrames, gNdsFighterBattlePlayableFinalStatus, gNdsFighterBattlePlayableFinalGA, gNdsFighterBattlePlayableFinalIsRebirth, gNdsFighterBattlePlayableKOStickFrames',
            'printf "BPLAY_MAP=%u,%u,%u,%u,%u,%#x,%#x,%d,%d,%d,%d,%d,%u,%u\n", gNdsFighterBattlePlayableMapCallCount, gNdsFighterBattlePlayableMapHitCount, gNdsFighterBattlePlayableMapFloorHitCount, gNdsFighterBattlePlayableMapCliffHitCount, gNdsFighterBattlePlayableMapCeilHitCount, gNdsFighterBattlePlayableMapLastMaskStat, gNdsFighterBattlePlayableMapLastMaskCurr, gNdsFighterBattlePlayableFinalXMilli, gNdsFighterBattlePlayableFinalYMilli, gNdsFighterBattlePlayableFinalVelXMilli, gNdsFighterBattlePlayableFinalVelYMilli, gNdsFighterBattlePlayableFinalFloorDistMilli, gNdsFighterBattlePlayableFinalFloor, gNdsFighterBattlePlayableFinalIsGhost',
            'printf "BPLAY_PACE=%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsBattlePlayablePacingResult, gNdsBattlePlayablePacingMode, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingDrawCalls, gNdsBattlePlayablePacingTimerTicks, gNdsBattlePlayablePacingPresentFpsX10, gNdsBattlePlayablePacingLogicFpsX10',
            'printf "MEMARENA=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerGeneration, gNdsMemoryLedgerArenaCapacity, gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater, gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerDLBytes, gNdsMemoryLedgerGraphicsBytes, gNdsMemoryLedgerRdpBytes, gNdsMemoryLedgerFigatreeHeapSize',
            'printf "MEMRELOC=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerRelocFiles, gNdsMemoryLedgerRelocBytes, gNdsMemoryLedgerRelocStageBytes, gNdsMemoryLedgerRelocFighterBytes, gNdsMemoryLedgerRelocInterfaceBytes, gNdsMemoryLedgerRelocMenuBytes, gNdsMemoryLedgerRelocOpeningBytes, gNdsMemoryLedgerRelocOtherBytes, gNdsMemoryLedgerRelocStaleFiles, gNdsMemoryLedgerRelocStaleBytes',
            'printf "MEMEVICT=%u,%u\n", gNdsMemoryLedgerEvictedFiles, gNdsMemoryLedgerEvictedBytes'
        )
        $gdbCommands = @($beforeDetach + $battlePlayableCommands + $afterDetach)
    }
    if ($ImportBattleShipFTComputer) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $computerCommands = @(
            'printf "CPU_CONFIG=%u,%u,%u,%u,%u,%u,%#x,%u\n", gSCManagerBattleState->players[0].pkind, gSCManagerBattleState->players[1].pkind, gSCManagerBattleState->players[1].level, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count, gSCManagerBattleState->time_limit, gSCManagerBattleState->item_toggles, gSCManagerBattleState->item_appearance_rate',
            'printf "CPU_AI=%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d\n", gNdsFTComputerSetupCount, gNdsFTComputerDamageDetectCount, gNdsFTComputerProcessCount, gNdsFTComputerTargetFrames, gNdsFTComputerObjectiveMask, gNdsFTComputerBehaviorMask, gNdsFTComputerInputChangeCount, gNdsFTComputerStickFrames, gNdsFTComputerButtonAFrames, gNdsFTComputerButtonBFrames, gNdsFTComputerButtonZFrames, gNdsFTComputerAttackFrames, gNdsFTComputerHitboxFrames, gNdsFTComputerGuardFrames, gNdsFTComputerRecoveryFrames, gNdsFTComputerStatusChangeCount, gNdsFTComputerFinalStatus, gNdsFTComputerFinalGA, gNdsFTComputerFinalInputKind, gNdsFTComputerMarioDamageMax, gNdsFTComputerFloorLineCount, gNdsFTComputerStartXMilli, gNdsFTComputerMinXMilli, gNdsFTComputerMaxXMilli, gNdsFTComputerFinalXMilli'
        )
        $gdbCommands = @($beforeDetach + $computerCommands + $afterDetach)
    }
    if ($MatchLifecycleProof) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $lifecycleCommands = @(
            'printf "VSB_END=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleLifecycleResult, gNdsSCVSBattleLifecycleArenaAdapterCount, gNdsSCVSBattleLifecycleTaskmanExitCount, gNdsSCVSBattleLifecycleTaskmanStatus, gNdsSCVSBattleLifecycleTimeLimit, gNdsSCVSBattleLifecycleTimeRemain, gNdsSCVSBattleLifecycleTimePassed, gNdsSCVSBattleLifecycleGameStatus, gNdsSCVSBattleLifecycleScenePrev, gNdsSCVSBattleLifecycleSceneCurr, gNdsSCVSBattleLifecycleIsSuddenDeath',
            'printf "VS_RESULTS=%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsVSResultsResult, gNdsVSResultsMask, gNdsVSResultsStartCount, gNdsVSResultsTickCount, gNdsVSResultsLoadedFileCount, gNdsVSResultsFighterCount, gNdsVSResultsGObjCount, gNdsVSResultsSObjCount, gNdsVSResultsKind',
            'printf "VS_RESULTS_FIGHTERS=%u,%#x,%d,%u,%#x,%d\n", gNdsVSResultsFighterPlace[0], gNdsVSResultsFighterStatus[0], gNdsVSResultsFighterMotion[0], gNdsVSResultsFighterPlace[1], gNdsVSResultsFighterStatus[1], gNdsVSResultsFighterMotion[1]',
            'printf "VS_RESULTS_DISPLAY=%u,%u,%u,%u\n", gNdsOriginalSpritePreviewReady, gNdsOriginalSpritePreviewCommitCount, gNdsOriginalSpritePreviewDisplayWidth, gNdsOriginalSpritePreviewDisplayHeight'
        )
        $gdbCommands = @($beforeDetach + $lifecycleCommands + $afterDetach)
    }
    if ($ImportBattleShipIFCommon) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $hudCommands = @(
            'printf "IFHUD=%u,%#x,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsIFCommonHUDRecordCount, gNdsIFCommonHUDObjectMask, gNdsIFCommonHUDP0DamageCurrent, gNdsIFCommonHUDP1DamageCurrent, gNdsIFCommonHUDP0DamageMax, gNdsIFCommonHUDP1DamageMax, gNdsIFCommonHUDP0DigitCount, gNdsIFCommonHUDP1DigitCount, gNdsIFCommonHUDP0Digits, gNdsIFCommonHUDP1Digits, gNdsIFCommonHUDP0StockCurrent, gNdsIFCommonHUDP1StockCurrent, gNdsIFCommonHUDP0StockMin, gNdsIFCommonHUDP1StockMin, gNdsIFCommonHUDP0StockMax, gNdsIFCommonHUDP1StockMax'
        )
        $gdbCommands = @($beforeDetach + $hudCommands + $afterDetach)
    }
    if ($ImportBattleShipMarioFireball -or $ImportBattleShipFoxBlaster) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $projectileCommands = @(
            'printf "PROJECTILE=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%#x\n", gNdsFighterProjectileProofResult, gNdsFighterProjectileProofMask, gNdsFighterProjectileProofActorSlot, gNdsFighterProjectileProofActorKind, gNdsFighterProjectileProofBPressFrames, gNdsFighterProjectileProofSpecialStatusFrames, gNdsFighterProjectileProofSpecialMotion, gNdsFighterProjectileProofAccessoryFrames, gNdsFighterProjectileProofFlag0Frames, gNdsFighterProjectileProofSpawnCallCount, gNdsFighterProjectileProofSpawnSuccessCount, gNdsFighterProjectileProofUpdateDestroyCount, gNdsFighterProjectileProofMapDestroyCount, gNdsFighterProjectileProofHitDestroyCount, gNdsFighterProjectileProofWeaponFrames, gNdsFighterProjectileProofWeaponCountMax, gNdsFighterProjectileProofKindMask, gNdsFighterProjectileProofAttackStateMask, gNdsFighterProjectileProofDamageMax, gNdsFighterProjectileProofLifetimeMax, gNdsFighterProjectileProofMapMask'
        )
        $gdbCommands = @($beforeDetach + $projectileCommands + $afterDetach)
    }
    if ($ImportBattleShipFoxReflector) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $reflectorCommands = @(
            'printf "REFLECTOR=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%u\n", gNdsFighterReflectorProofResult, gNdsFighterReflectorProofMask, gNdsFighterReflectorProofFoxSlot, gNdsFighterReflectorProofProjectileSlot, gNdsFighterReflectorProofDownBPressFrames, gNdsFighterReflectorProofStartFrames, gNdsFighterReflectorProofLoopFrames, gNdsFighterReflectorProofHitFrames, gNdsFighterReflectorProofIsReflectFrames, gNdsFighterReflectorProofReflectLRBeforeHit, gNdsFighterReflectorProofReflectLRClearFrames, gNdsFighterReflectorProofHitSetCallCount, gNdsFighterReflectorProofFireballProcCount, gNdsFighterReflectorProofFireballVelXBefore, gNdsFighterReflectorProofFireballVelXAfter, gNdsFighterReflectorProofFireballOwnerKind, gNdsFighterReflectorProofFireballCanReflect, gNdsFighterReflectorProofFireballCanAbsorb, gNdsFighterReflectorProofFireballCanShield, gNdsFighterReflectorProofFireballAttackCount, gNdsFighterReflectorProofFireballDamage, gNdsFighterReflectorProofFireballSizeMilli, gNdsFighterReflectorProofFireballDXMilli, gNdsFighterReflectorProofFireballDYMilli, gNdsFighterReflectorProofSpecialSizeMilli, gNdsFighterReflectorProofSpecialResist'
        )
        $gdbCommands = @($beforeDetach + $reflectorCommands + $afterDetach)
    }
    if ($ImportBattleShipMarioSpecialHi -or $ImportBattleShipMarioSpecialLw -or $ImportBattleShipFoxSpecialHi) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $specialsCommands = @(
            'printf "SPECIALS=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d\n", gNdsFighterSpecialsProofMask, gNdsFighterSpecialsProofPhase, gNdsFighterSpecialsProofPhaseFrames, gNdsFighterSpecialsMarioSlot, gNdsFighterSpecialsFoxSlot, gNdsFighterSpecialsMarioHiPressFrames, gNdsFighterSpecialsMarioHiFrames, gNdsFighterSpecialsMarioAirHiFrames, gNdsFighterSpecialsMarioFallSpecialFrames, gNdsFighterSpecialsMarioLandingFallSpecialFrames, gNdsFighterSpecialsMarioHiWaitFrames, gNdsFighterSpecialsMarioHiRootYMilli, gNdsFighterSpecialsMarioLwPressFrames, gNdsFighterSpecialsMarioLwFrames, gNdsFighterSpecialsMarioAirLwFrames, gNdsFighterSpecialsMarioLwDustEffectCount, gNdsFighterSpecialsMarioLwWaitFrames, gNdsFighterSpecialsFoxHiPressFrames, gNdsFighterSpecialsFoxHiStartFrames, gNdsFighterSpecialsFoxHiHoldFrames, gNdsFighterSpecialsFoxHiTravelFrames, gNdsFighterSpecialsFoxHiEndFrames, gNdsFighterSpecialsFoxHiBoundFrames, gNdsFighterSpecialsFoxHiWaitFrames, gNdsFighterSpecialsFoxHiRootYMilli'
        )
        $gdbCommands = @($beforeDetach + $specialsCommands + $afterDetach)
    }
    if ($ImportBattleShipAudioAssets) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $audioCommands = @(
            'printf "AUDIO_ASSET=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioAssetResult, gNdsAudioAssetMask, gNdsAudioAssetOpenCount, gNdsAudioAssetOpenFailCount, gNdsAudioAssetFormatFailCount, gNdsAudioAssetShortReadCount, gNdsAudioAssetRawBytes, gNdsAudioAssetResidentBytes, gNdsAudioAssetScratchMaxBytes, gNdsAudioAssetSeqCount, gNdsAudioAssetSeqFirstOffset, gNdsAudioAssetSeqFirstLength, gNdsAudioAssetSeqMaxLength, gNdsAudioAssetBank1BankCount, gNdsAudioAssetBank1InstrumentCount, gNdsAudioAssetBank1WaveCount, gNdsAudioAssetBank1SampleRate, gNdsAudioAssetBank2BankCount, gNdsAudioAssetBank2InstrumentCount, gNdsAudioAssetBank2WaveCount, gNdsAudioAssetBank2SampleRate, gNdsAudioAssetFgmUnkCount, gNdsAudioAssetFgmTableCount, gNdsAudioAssetFgmUcodeCount'
        )
        $gdbCommands = @($beforeDetach + $audioCommands + $afterDetach)
    }
    if ($ImportBattleShipAudioBGM) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $audioBgmCommands = @(
            'printf "AUDIO_BGM=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioBgmResult, gNdsAudioBgmMask, gNdsAudioBgmPlaying, gNdsAudioBgmTrackID, gNdsAudioBgmVolume, gNdsAudioBgmPlayCalls, gNdsAudioBgmStopCalls, gNdsAudioBgmCheckCalls, gNdsAudioBgmSetVolumeCalls, gNdsAudioBgmOpenFailCount, gNdsAudioBgmReadFailCount, gNdsAudioBgmUnsupportedTrackCount, gNdsAudioBgmReadBytes, gNdsAudioBgmResidentBytes, gNdsAudioBgmChunkBytes, gNdsAudioBgmChunkPlayCount, gNdsAudioBgmStoppedOnTeardown, gNdsAudioBgmElapsedFrames, gNdsAudioBgmStreamedBytes, gNdsAudioBgmStreamBytesPerSecond, gNdsAudioBgmExpectedBytesPerSecond, gNdsAudioBgmLoopCount, gNdsAudioBgmRefillCount, gNdsAudioBgmPlaybackPositionBytes, gNdsAudioBgmWritePositionBytes, gNdsAudioBgmPlaybackHalf, gNdsAudioBgmWriteHalf, gNdsAudioBgmUnsafeWriteCount, gNdsAudioBgmTimerTicks, gNdsAudioBgmPlaybackBytes, gNdsAudioBgmPlaybackLoopCount, gNdsAudioBgmOverrunCount'
        )
        $gdbCommands = @($beforeDetach + $audioBgmCommands + $afterDetach)
    }
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $aobj32 = [regex]::Match($gdbStdout, 'AOBJ32=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $mobjAttach = [regex]::Match($gdbStdout, 'MOBJ_ATTACH=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $buildMode = [regex]::Match($gdbStdout, 'BUILD_MODE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $vs = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $pv = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $maps = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $prev = [regex]::Match($gdbStdout, 'PREV_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'GCRUNALL_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $taskman = [regex]::Match($gdbStdout, 'GCRUNALL_TASKMAN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $natural = [regex]::Match($gdbStdout, 'NAT_MOTION=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $naturalFig = [regex]::Match($gdbStdout, 'NAT_FIG=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalWait = [regex]::Match($gdbStdout, 'NAT_WAIT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $naturalWalk = [regex]::Match($gdbStdout, 'NAT_WALK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalChain = [regex]::Match($gdbStdout, 'NAT_CHAIN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalAttack = [regex]::Match($gdbStdout, 'NAT_ATTACK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalMoveset = [regex]::Match($gdbStdout, 'NAT_MOVESET=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $naturalHitlag = [regex]::Match($gdbStdout, 'NAT_HITLAG=([0-9]+),([0-9]+)')
    $naturalGuard = [regex]::Match($gdbStdout, 'NAT_GUARD=([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayableKO = [regex]::Match($gdbStdout, 'BPLAY_KO=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayableStatus = [regex]::Match($gdbStdout, 'BPLAY_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayablePacing = [regex]::Match($gdbStdout, 'BPLAY_PACE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryArena = [regex]::Match($gdbStdout, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryReloc = [regex]::Match($gdbStdout, 'MEMRELOC=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryEvict = [regex]::Match($gdbStdout, 'MEMEVICT=([0-9]+),([0-9]+)')
    $computerConfig = [regex]::Match($gdbStdout, 'CPU_CONFIG=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $computerAI = [regex]::Match($gdbStdout, 'CPU_AI=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $battleLifecycle = [regex]::Match($gdbStdout, 'VSB_END=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsResults = [regex]::Match($gdbStdout, 'VS_RESULTS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsResultsFighters = [regex]::Match($gdbStdout, 'VS_RESULTS_FIGHTERS=([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+)')
    $vsResultsDisplay = [regex]::Match($gdbStdout, 'VS_RESULTS_DISPLAY=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $run = [regex]::Match($gdbStdout, 'GCRUNALL_RUN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $process = [regex]::Match($gdbStdout, 'GCRUNALL_PROCESS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'GCRUNALL_INPUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'GCRUNALL_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $visits = [regex]::Match($gdbStdout, 'GCRUNALL_VISITS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'GCRUNALL_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $draw = [regex]::Match($gdbStdout, 'GCRUNALL_DRAW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $screen = [regex]::Match($gdbStdout, 'GCRUNALL_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $move = [regex]::Match($gdbStdout, 'GCRUNALL_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'GCRUNALL_TRANS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'GCRUNALL_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platformHw = [regex]::Match($gdbStdout, 'PLATFORM_HW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $rendererProfileMarker = [regex]::Match($gdbStdout, 'RENDER_PROFILE_LEVEL=([0-9]+)')
    $stageHardware = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_HW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $stageCarry = [regex]::Match($gdbStdout, 'RENDER_STAGE_CARRY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageHardwareFighter = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_HW_FTR=([0-9]+),([0-9]+)')
    $fighterDisplayContract = [regex]::Match($gdbStdout, 'FTR_DISPLAY_CONTRACT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderProfile = [regex]::Match($gdbStdout, 'RENDER_PROFILE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $rendererBenchmark = [regex]::Matches($gdbStdout, 'RENDER_BENCH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $coarseBenchmark = @()
    $ownerBenchmark = @()
    $gxBoundaryBenchmark = @()
    $stage0Benchmark = @()
    $sinkBenchmark = @()
    $warmBenchmark = @()
    $rendererSemanticBenchmark = @()
    if (($RendererProfileLevel -ge 1) -and ($RendererBenchmarkSamples -gt 0)) {
        $coarseBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'COARSE_BENCH' -FieldCount 24)
        $gxBoundaryBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'GX_BOUNDARY' -FieldCount 7)
        $stage0Benchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'STAGE0_BENCH' -FieldCount 2)
        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
            $sinkBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'SINK_BENCH' -FieldCount 8)
        }
        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
            $warmBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'WARM_BENCH' -FieldCount 3)
        }
        if ($RendererProfileLevel -ge 2) {
            $ownerBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'OWNER_BENCH' -FieldCount 37)
            $rendererSemanticBenchmark = @(Get-UnsignedMarkerMatches -Text $gdbStdout -Name 'RENDER_SEMANTIC' -FieldCount 38)
        }
    }
    $renderBatch = [regex]::Match($gdbStdout, 'RENDER_BATCH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderTopology = [regex]::Match($gdbStdout, 'RENDER_TOPOLOGY=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderCost = [regex]::Match($gdbStdout, 'RENDER_COST=([0-9]+),([0-9]+)')
    $renderCi4Lut = [regex]::Match($gdbStdout, 'RENDER_CI4LUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderCi4Map = [regex]::Match($gdbStdout, 'RENDER_CI4MAP=([0-9]+),([0-9]+)')
    $renderTexHash = [regex]::Match($gdbStdout, 'RENDER_TEXHASH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $wallpaperCache = [regex]::Match($gdbStdout, 'SOBJ_WALL_CACHE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $wallpaperFinal = [regex]::Match($gdbStdout, 'SOBJ_WALL_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderOracle = [regex]::Match($gdbStdout, 'RENDER_ORACLE=([0-9]+),([0-9]+),([0-9]+)')
    $renderMatrix = [regex]::Match($gdbStdout, 'RENDER_MATRIX=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $renderAdapterCache = [regex]::Match($gdbStdout, 'RENDER_ADAPTER_CACHE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderRawMatrix = [regex]::Match($gdbStdout, 'RENDER_RAW_MATRIX=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderSubmit = [regex]::Match($gdbStdout, 'RENDER_SUBMIT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderHardwareDivide = [regex]::Match($gdbStdout, 'RENDER_HWDIV=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderLazy = [regex]::Match($gdbStdout, 'RENDER_LAZY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderVertex = [regex]::Match($gdbStdout, 'RENDER_VERTEX=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $renderDepth = [regex]::Match($gdbStdout, 'RENDER_DEPTH=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $renderClip = [regex]::Match($gdbStdout, 'RENDER_CLIP=([0-9]+)')
    $renderTexture = [regex]::Match($gdbStdout, 'RENDER_TEXTURE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $renderTexel1 = [regex]::Match($gdbStdout, 'RENDER_TEXEL1=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $renderTexUse = [regex]::Match($gdbStdout, 'RENDER_TEXUSE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderTexFmt = [regex]::Match($gdbStdout, 'RENDER_TEXFMT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderTexLane = [regex]::Match($gdbStdout, 'RENDER_TEXLANE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderCombine = [regex]::Match($gdbStdout, 'RENDER_COMBINE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $renderLight = [regex]::Match($gdbStdout, 'RENDER_LIGHT=([0-9]+),([0-9]+),([0-9]+)')
    $fighterLightSeed = [regex]::Match($gdbStdout, 'FTR_LIGHT_SEED=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $ifHud = [regex]::Match($gdbStdout, 'IFHUD=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $projectile = [regex]::Match($gdbStdout, 'PROJECTILE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $reflector = [regex]::Match($gdbStdout, 'REFLECTOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $specials = [regex]::Match($gdbStdout, 'SPECIALS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $audioAsset = [regex]::Match($gdbStdout, 'AUDIO_ASSET=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $audioBgm = [regex]::Match($gdbStdout, 'AUDIO_BGM=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($aobj32.Success -and [int64]$aobj32.Groups[4].Value -eq 0) 'AObjEvent32 source-command normalization rejected a live script.' $gdbStdout
    $aobjSummary = " aobj32=$($aobj32.Groups[1].Value)/$($aobj32.Groups[2].Value)/reuse$($aobj32.Groups[3].Value)/fail0"
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    if ($MatchLifecycleProof) {
        # scvsbattle.c:559-560 changes VSBattle to VS Results after task return.
        Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 24 -and [int]$scene.Groups[2].Value -eq 22 -and [int]$scene.Groups[3].Value -eq 6) 'Completed Pupupu battle did not enter VS Results from VSBattle.' $gdbStdout
    } else {
        Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    }
    if ($RealtimePresentation -and $LiveInputPreview -and $HardwareTriangles) {
        $bm = Get-Ints $buildMode
        Assert-Condition ($buildMode.Success -and $bm[0] -eq 0x43414e4f -and $bm[1] -eq 0x53484950 -and $bm[2] -eq 0) 'Canonical shipped ROM build marker did not report CANO/SHIP realtime config.' $gdbStdout
    } elseif (-not $RealtimePresentation) {
        $bm = Get-Ints $buildMode
        Assert-Condition ($buildMode.Success -and $bm[2] -eq 0x46415354) 'Fast-logic verifier build marker did not report FAST config.' $gdbStdout
    }
    if ($ImportBattleShipFTComputer) {
        $cc = Get-Ints $computerConfig
        $expectedTimeLimit = if ($MatchLifecycleProof) { 1 } else { 5 }
        Assert-Condition ($computerConfig.Success -and $cc[0] -eq 0 -and $cc[1] -eq 1 -and $cc[2] -eq 3 -and $cc[3] -eq 1 -and $cc[4] -eq 1 -and $cc[5] -eq $expectedTimeLimit -and $cc[6] -eq 0 -and $cc[7] -eq 0) 'Mode 163 did not configure the expected items-off Mario human versus Fox level-3 CPU match.' $gdbStdout
    }
    if ($ExpectedMode -eq 54) {
        Assert-Condition ($vs.Success -and (Convert-MarkerUInt32 $vs.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain VS Mode -> PlayersVS transition did not pass.' $gdbStdout
        Assert-Condition ($pv.Success -and (Convert-MarkerUInt32 $pv.Groups[1].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $pv.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain PlayersVS -> Maps transition did not pass.' $gdbStdout
        Assert-Condition ($maps.Success -and (Convert-MarkerUInt32 $maps.Groups[1].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $maps.Groups[2].Value) -band 0xff) -eq 0xff -and [int]$maps.Groups[3].Value -eq 6) 'Menu-chain Maps -> VSBattle transition did not pass.' $gdbStdout
    }
    if ($ImportBattleShipFTManager) {
        $nat = Get-Ints $natural
        $nfig = Get-Ints $naturalFig
        $nw = Get-Ints $naturalWait
        $nwalk = Get-Ints $naturalWalk
        $nc = Get-Ints $naturalChain
        $na = Get-Ints $naturalAttack
        $nh = Get-Ints $naturalHitlag
        $ng = Get-Ints $naturalGuard
        if ($ImportBattleShipNormalMoveset) {
            $nm = Get-Ints $naturalMoveset
        }
        $hardwareSummary = ''
        if ($BattlePlayable -and $RealtimePresentation) {
            $bp = Get-Ints $battlePlayablePacing
            $minPresentedFrames = if ($RequireRealtime60Fps) { 60 } else { 45 }
            Assert-Condition ($battlePlayablePacing.Success -and $bp[0] -eq 0x42505443 -and $bp[1] -eq 0 -and (($bp[2] -eq $bp[3]) -or ($bp[2] -eq ($bp[3] + 1))) -and (($bp[4] -eq $bp[3]) -or ($bp[4] -eq ($bp[3] + 1))) -and $bp[3] -ge $minPresentedFrames -and $bp[5] -gt 0 -and $bp[6] -gt 0 -and $bp[7] -gt 0) 'battle_playable realtime pacing smoke did not present live frames or keep draw/update within one in-flight vblank.' $gdbStdout
            if ($RequireRealtime60Fps) {
                Assert-Condition ($bp[6] -ge 593 -and $bp[6] -le 603 -and $bp[7] -ge 593 -and $bp[7] -le 603) 'battle_playable realtime pacing failed 59.3..60.3 presented/logic fps.' $gdbStdout
            } elseif (($bp[6] -lt 593) -or ($bp[6] -gt 603) -or ($bp[7] -lt 593) -or ($bp[7] -gt 603)) {
                Write-Warning ("$Label realtime HW textured perf below 60fps target: fps=$($bp[6])/$($bp[7]) x0.1; renderer-cache follow-up still required.")
            }
            if ($HardwareTriangles) {
                $hw = Get-Ints $platformHw
                $shw = Get-Ints $stageHardware
                $scarry = Get-Ints $stageCarry
                $shwf = Get-Ints $stageHardwareFighter
                $fdc = Get-Ints $fighterDisplayContract
                $rp = Get-Ints $renderProfile
                $rb = Get-Ints $renderBatch
                $rtopo = Get-Ints $renderTopology
                $rcost = Get-Ints $renderCost
                $rci4lut = Get-Ints $renderCi4Lut
                $rci4map = Get-Ints $renderCi4Map
                $rth = Get-Ints $renderTexHash
                $wc = Get-Ints $wallpaperCache
                $wf = Get-Ints $wallpaperFinal
                $ro = Get-Ints $renderOracle
                $rm = Get-Ints $renderMatrix
                $rac = Get-Ints $renderAdapterCache
                $rrm = Get-Ints $renderRawMatrix
                $rs = Get-Ints $renderSubmit
                $rhdiv = Get-Ints $renderHardwareDivide
                $rlazy = Get-Ints $renderLazy
                $rv = Get-Ints $renderVertex
                $rd = Get-Ints $renderDepth
                $rclip = Get-Ints $renderClip
                $rt = Get-Ints $renderTexture
                $rt1 = Get-Ints $renderTexel1
                $ma = Get-Ints $mobjAttach
                $rtu = Get-Ints $renderTexUse
                $rtf = Get-Ints $renderTexFmt
                $rtl = Get-Ints $renderTexLane
                $rc = Get-Ints $renderCombine
                $rl = Get-Ints $renderLight
                $fls = Get-Ints $fighterLightSeed
                $benchmarkSummary = ''
                $benchmarkIdentitySummary = ''
                $benchmarkMetricSummary = ''
                $benchmarkChurnSummary = ''
                $coarseMetricSummary = ''
                $coarseResidualRatioSummary = ''
                $gxBoundarySummary = ''
                $stage0Summary = ''
                $sinkMetricSummary = ''
                $warmMetricSummary = ''
                $semanticMetricSummary = ''
                $semanticChurnSummary = ''
                $semanticProvenanceSummaries = @()
                $ownerCensusSummaries = @()
                $ownerChurnSummaries = @()
                if ($RendererBenchmarkSamples -gt 0) {
                    Assert-Condition ($rendererBenchmark.Count -eq $RendererBenchmarkSamples) "Renderer benchmark captured $($rendererBenchmark.Count) of $RendererBenchmarkSamples requested warm frames." $gdbStdout
                    $benchFrames = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[2].Value })
                    $benchPresent = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[3].Value })
                    $benchDraw = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[4].Value })
                    $benchStage = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[5].Value })
                    $benchMaterial = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[6].Value })
                    $benchMatrix = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[7].Value })
                    $benchDL = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[8].Value })
                    $benchTexture = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[9].Value })
                    $benchSubmit = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[10].Value })
                    $benchVertex = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[11].Value })
                    $benchUploadCount = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[14].Value })
                    $benchUploadBytes = @($rendererBenchmark | ForEach-Object { [int64]$_.Groups[15].Value })
                    $benchSetup = @()
                    $benchScan = @()
                    for ($sampleIndex = 0; $sampleIndex -lt $rendererBenchmark.Count; $sampleIndex++) {
                        $sample = $rendererBenchmark[$sampleIndex]
                        $sampleProfile = [int64]$sample.Groups[1].Value
                        $sampleFrame = [int64]$sample.Groups[2].Value
                        $sampleDL = [int64]$sample.Groups[8].Value
                        $sampleSubmit = [int64]$sample.Groups[10].Value
                        $sampleVertex = [int64]$sample.Groups[11].Value
                        $sampleTriangles = [int64]$sample.Groups[12].Value
                        $sampleOracle = [int64]$sample.Groups[13].Value
                        $sampleUploadCount = [int64]$sample.Groups[14].Value
                        $sampleUploadBytes = [int64]$sample.Groups[15].Value
                        Assert-Condition ($sampleProfile -eq $RendererProfileLevel -and $sampleTriangles -eq 828 -and (($RendererProfileLevel -ge 2 -and $sampleOracle -eq 2484) -or ($RendererProfileLevel -lt 2 -and $sampleOracle -eq 0))) 'Renderer benchmark sampled the wrong profile or drifted from exact 828-triangle/2,484-oracle accounting.' $gdbStdout
                        Assert-Condition (Test-RendererUploadPair $sampleUploadCount $sampleUploadBytes) "Renderer benchmark sampled an invalid texture upload pair $sampleUploadCount/$sampleUploadBytes at frame $sampleFrame." $gdbStdout
                        if ($sampleIndex -gt 0) {
                            $previousFrame = [int64]$rendererBenchmark[$sampleIndex - 1].Groups[2].Value
                            Assert-Condition ($sampleFrame -eq ($previousFrame + 1)) "Renderer benchmark frame window is not synchronized: frame $sampleFrame followed $previousFrame." $gdbStdout
                        }
                        Assert-Condition ($sampleDL -ge $sampleSubmit -and $sampleSubmit -ge $sampleVertex) 'Renderer benchmark sampled incoherent nested DL/triangle/vertex costs.' $gdbStdout
                        $benchSetup += $sampleSubmit - $sampleVertex
                        $benchScan += $sampleDL - $sampleSubmit
                    }
                    $uploadSequenceHash = Get-RendererUploadSequenceHash $benchUploadCount $benchUploadBytes
                    $benchmarkMetricSummary = "Renderer benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) median/p95 ticks present=$((Get-Median $benchPresent))/$((Get-Percentile95 $benchPresent)) draw=$((Get-Median $benchDraw))/$((Get-Percentile95 $benchDraw)) stage=$((Get-Median $benchStage))/$((Get-Percentile95 $benchStage)) material=$((Get-Median $benchMaterial))/$((Get-Percentile95 $benchMaterial)) matrix=$((Get-Median $benchMatrix))/$((Get-Percentile95 $benchMatrix)) dl=$((Get-Median $benchDL))/$((Get-Percentile95 $benchDL)) texture=$((Get-Median $benchTexture))/$((Get-Percentile95 $benchTexture)) submit=$((Get-Median $benchSubmit))/$((Get-Percentile95 $benchSubmit)) vertex=$((Get-Median $benchVertex))/$((Get-Percentile95 $benchVertex)) setup=$((Get-Median $benchSetup))/$((Get-Percentile95 $benchSetup)) scan=$((Get-Median $benchScan))/$((Get-Percentile95 $benchScan)) uploads=$(Get-MedianP95 $benchUploadCount)/$(Get-MedianP95 $benchUploadBytes) uploadSequenceSha256=$uploadSequenceHash"
                    $benchmarkChurnSummary = "Renderer benchmark churn (adjacent changes/distinct values): present=$((Get-AdjacentChurn $benchPresent)) draw=$((Get-AdjacentChurn $benchDraw)) stage=$((Get-AdjacentChurn $benchStage)) material=$((Get-AdjacentChurn $benchMaterial)) matrix=$((Get-AdjacentChurn $benchMatrix)) dl=$((Get-AdjacentChurn $benchDL)) texture=$((Get-AdjacentChurn $benchTexture)) submit=$((Get-AdjacentChurn $benchSubmit)) vertex=$((Get-AdjacentChurn $benchVertex)) setup=$((Get-AdjacentChurn $benchSetup)) scan=$((Get-AdjacentChurn $benchScan))"
                    if ($RendererProfileLevel -ge 1) {
                        Assert-Condition ($coarseBenchmark.Count -eq $RendererBenchmarkSamples) "Coarse renderer benchmark captured $($coarseBenchmark.Count) of $RendererBenchmarkSamples synchronized frames." $gdbStdout
                        Assert-Condition ($gxBoundaryBenchmark.Count -eq $RendererBenchmarkSamples) "GX boundary benchmark captured $($gxBoundaryBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        Assert-Condition ($stage0Benchmark.Count -eq $RendererBenchmarkSamples) "Stage layer-0 benchmark captured $($stage0Benchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        if ($RendererProfileLevel -ge 2) {
                            Assert-Condition ($ownerBenchmark.Count -eq (3 * $RendererBenchmarkSamples)) "Owner renderer benchmark captured $($ownerBenchmark.Count) of $(3 * $RendererBenchmarkSamples) synchronized owner records." $gdbStdout
                            Assert-Condition ($rendererSemanticBenchmark.Count -eq $RendererBenchmarkSamples) "Renderer semantic benchmark captured $($rendererSemanticBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        $coarseSamples = [System.Collections.Generic.List[object]]::new()
                        $gxBoundarySamples = [System.Collections.Generic.List[object]]::new()
                        $stage0Samples = [System.Collections.Generic.List[object]]::new()
                        $sinkSamples = [System.Collections.Generic.List[object]]::new()
                        $warmSamples = [System.Collections.Generic.List[object]]::new()
                        $semanticSamples = [System.Collections.Generic.List[object]]::new()
                        $logicTickResetCount = 0
                        $drawResidualRatios = @()
                        $presentResidualRatios = @()
                        $loopResidualRatios = @()
                        $ownerSamples = @(
                            [System.Collections.Generic.List[object]]::new(),
                            [System.Collections.Generic.List[object]]::new(),
                            [System.Collections.Generic.List[object]]::new()
                        )
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                            Assert-Condition ($sinkBenchmark.Count -eq $RendererBenchmarkSamples) "CPU_PREP_NO_GX sink benchmark captured $($sinkBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                            Assert-Condition ($warmBenchmark.Count -eq $RendererBenchmarkSamples) "WARM_NO_UPLOAD benchmark captured $($warmBenchmark.Count) of $RendererBenchmarkSamples synchronized records." $gdbStdout
                        }
                        for ($sampleIndex = 0; $sampleIndex -lt $RendererBenchmarkSamples; $sampleIndex++) {
                            $coarse = Get-Ints $coarseBenchmark[$sampleIndex]
                            $render = Get-Ints $rendererBenchmark[$sampleIndex]
                            $frame = $coarse[0]
                            $loopWall = $coarse[1]
                            $update = $coarse[3]
                            $sourceUpdate = $coarse[4]
                            $audioUpdate = $coarse[5]
                            $presentActive = $coarse[6]
                            $vblankWait = $coarse[7]
                            $drawTicks = $coarse[9]
                            $drawKnown = $coarse[10] + $coarse[11] + $coarse[12] + $coarse[13] + $coarse[14]
                            $presentKnown = $coarse[8] + $drawTicks + $coarse[15] + $coarse[16] + $coarse[17] + $coarse[18]
                            $loopKnown = $coarse[2] + $update + $presentActive + $vblankWait
                            $expectedPresentActive = if ($render[2] -ge $vblankWait) { $render[2] - $vblankWait } else { 0 }
                            $expectedDrawResidual = if ($drawTicks -ge $drawKnown) { $drawTicks - $drawKnown } else { 0 }
                            $expectedPresentResidual = if ($presentActive -ge $presentKnown) { $presentActive - $presentKnown } else { 0 }
                            $expectedLoopResidual = if ($loopWall -ge $loopKnown) { $loopWall - $loopKnown } else { 0 }
                            $expectedConservationError = [int64]0
                            if ($vblankWait -gt $render[2]) { $expectedConservationError += $vblankWait - $render[2] }
                            if ($drawKnown -gt $drawTicks) { $expectedConservationError += $drawKnown - $drawTicks }
                            if ($presentKnown -gt $presentActive) { $expectedConservationError += $presentKnown - $presentActive }
                            if ($loopKnown -gt $loopWall) { $expectedConservationError += $loopKnown - $loopWall }

                            Assert-Condition ($frame -eq $render[1] -and $drawTicks -eq $render[3]) "Coarse renderer benchmark frame $frame is not synchronized with RENDER_BENCH." $gdbStdout
                            if ($sampleIndex -gt 0) {
                                $previousCoarse = Get-Ints $coarseBenchmark[$sampleIndex - 1]
                                $logicTickContinues =
                                    $coarse[23] -eq ($previousCoarse[23] + 1)
                                # BattleShip ifcommon.c:3173-3178 starts the
                                # five-minute timer after the Ready/Go wait by
                                # resetting the scheduler tic counter once.
                                $logicTimerStartReset =
                                    ($coarse[23] -eq 1) -and
                                    ($previousCoarse[23] -ge 300) -and
                                    ($logicTickResetCount -eq 0)
                                if ($logicTimerStartReset) {
                                    $logicTickResetCount++
                                }
                                Assert-Condition ($frame -eq ($previousCoarse[0] + 1) -and ($logicTickContinues -or $logicTimerStartReset)) "Coarse renderer benchmark frame/logic window is not contiguous or the single source timer-start reset at frame $frame tick $($coarse[23])." $gdbStdout
                            }
                            Assert-Condition (($sourceUpdate + $audioUpdate) -le $update) "Coarse renderer benchmark update subphases exceed update wall time at frame $frame." $gdbStdout
                            Assert-Condition ($presentActive -eq $expectedPresentActive -and $coarse[19] -eq $expectedDrawResidual -and $coarse[20] -eq $expectedPresentResidual -and $coarse[21] -eq $expectedLoopResidual -and $coarse[22] -eq $expectedConservationError) "Coarse renderer benchmark residual equations failed at frame $frame." $gdbStdout
                            Assert-Condition ($loopWall -gt 0 -and ($coarse[22] * 100) -le ($loopWall * 2)) "Coarse renderer benchmark conservation error exceeded 2 percent at frame $frame." $gdbStdout
                            Assert-Condition (($presentActive -eq 0) -or (($coarse[20] * 100) -le ($presentActive * 2))) "Coarse renderer benchmark present residual exceeded 2 percent at frame $frame." $gdbStdout

                            $gxBoundary = Get-Ints $gxBoundaryBenchmark[$sampleIndex]
                            Assert-Condition ($gxBoundary[0] -eq $frame -and $gxBoundary[4] -ne 0 -and $gxBoundary[5] -ne 0 -and $gxBoundary[6] -eq $coarse[16]) "GX boundary benchmark is not synchronized or lacks a post-VBlank completion sample at coarse frame $frame." $gdbStdout
                            $gxBoundarySamples.Add($gxBoundary)
                            $stage0 = Get-Ints $stage0Benchmark[$sampleIndex]
                            Assert-Condition ($stage0[0] -eq $frame -and $stage0[1] -gt 0) "Stage layer-0 benchmark is not synchronized or measured at frame $frame." $gdbStdout
                            $stage0Samples.Add($stage0)
                            if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                                $sink = Get-Ints $sinkBenchmark[$sampleIndex]
                                $sinkOwnerWords = $sink[5] + $sink[6] + $sink[7]
                                Assert-Condition ($sink[0] -eq $frame -and $sink[1] -eq $sink[2] -and $sink[2] -gt 0 -and $sink[3] -eq 1024 -and $sink[4] -gt 0 -and $sinkOwnerWords -eq $sink[2]) "CPU_PREP_NO_GX sink record is not synchronized, owner-conserved, or calibrated at frame $frame." $gdbStdout
                                $sinkSamples.Add($sink)
                            }
                            if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                                $warm = Get-Ints $warmBenchmark[$sampleIndex]
                                Assert-Condition ($warm[0] -eq $frame -and (Test-RendererUploadPair $warm[1] $warm[2])) "WARM_NO_UPLOAD sampled an invalid suppressed refresh pair $($warm[1])/$($warm[2]) at frame $frame." $gdbStdout
                                $warmSamples.Add($warm)
                            }
                            $drawResidualRatios += Get-RatioBasisPoints $coarse[19] $drawTicks
                            $presentResidualRatios += Get-RatioBasisPoints $coarse[20] $presentActive
                            $loopResidualRatios += Get-RatioBasisPoints $coarse[21] $loopWall

                            if ($RendererProfileLevel -ge 2) {
                                $ownerTriangleTotal = [int64]0
                                $ownerRunTotal = [int64]0
                                $ownerClassTotals = [int64[]]::new(8)
                                $expectedOwnerTriangles = @(202, 320, 306)
                                for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                    $owner = Get-Ints $ownerBenchmark[(3 * $sampleIndex) + $ownerIndex]
                                    Assert-Condition ($owner[0] -eq $frame -and $owner[1] -eq $ownerIndex -and $owner[2] -eq $coarse[11 + $ownerIndex]) "Owner renderer benchmark record $ownerIndex is not synchronized at frame $frame." $gdbStdout
                                    $ownerClassTotal = [int64]0
                                    foreach ($classIndex in 0..7) {
                                        $ownerClassTotal += $owner[9 + $classIndex]
                                        $ownerClassTotals[$classIndex] +=
                                            $owner[9 + $classIndex]
                                    }
                                    $ownerTriangleTotal += $owner[8]
                                    $ownerRunTotal += $owner[20]
                                    $boundaryHashesPresent = $true
                                    foreach ($hashIndex in 21..28) {
                                        if ($owner[$hashIndex] -eq 0) {
                                            $boundaryHashesPresent = $false
                                        }
                                    }
                                    Assert-Condition ($owner[8] -eq $expectedOwnerTriangles[$ownerIndex] -and $ownerClassTotal -eq $owner[8] -and $boundaryHashesPresent) "Owner renderer benchmark record $ownerIndex did not preserve its exact triangle/class partition or nonzero entry/exit boundary hashes at frame $frame." $gdbStdout
                                    $ownerSamples[$ownerIndex].Add($owner)
                                }
                                Assert-Condition ($ownerTriangleTotal -eq 828 -and $ownerRunTotal -eq 121 -and $ownerClassTotals[0] -eq 648 -and $ownerClassTotals[1] -eq 0 -and $ownerClassTotals[2] -eq 44 -and $ownerClassTotals[3] -eq 126 -and $ownerClassTotals[4] -eq 0 -and $ownerClassTotals[5] -eq 0 -and $ownerClassTotals[6] -eq 10 -and $ownerClassTotals[7] -eq 0) "Owner renderer benchmark did not conserve exact 828 triangles, 648/44/126/10 classes, and 121 runs at frame $frame." $gdbStdout
                                $semantic = Get-Ints $rendererSemanticBenchmark[$sampleIndex]
                                $ownerEventCount = [int64]0
                                $ownerOverflowCount = [int64]0
                                Assert-Condition ($semantic[0] -eq $frame -and $semantic[1] -ne 0 -and $semantic[2] -ne 0 -and $semantic[3] -eq 828 -and $semantic[3] -eq $render[11] -and $semantic[4] -eq 0) "Renderer semantic frame hash/count/overflow contract failed at frame $frame." $gdbStdout
                                for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                    $semanticBase = 5 + (11 * $ownerIndex)
                                    $ownerProfile = Get-Ints $ownerBenchmark[(3 * $sampleIndex) + $ownerIndex]
                                    $ownerEventCount += $semantic[$semanticBase + 2]
                                    $ownerOverflowCount += $semantic[$semanticBase + 3]
                                    Assert-Condition ($semantic[$semanticBase] -ne 0 -and $semantic[$semanticBase + 1] -ne 0 -and $semantic[$semanticBase] -eq $ownerProfile[36] -and $semantic[$semanticBase + 2] -eq $ownerProfile[8] -and $semantic[$semanticBase + 3] -eq 0 -and $semantic[$semanticBase + 4] -gt 0) "Renderer semantic owner $ownerIndex hash/count/occurrence contract failed at frame $frame." $gdbStdout
                                    Assert-Condition ($semantic[$semanticBase + 5] -lt $semantic[$semanticBase + 4] -and $semantic[$semanticBase + 6] -lt $ownerProfile[3] -and $semantic[$semanticBase + 7] -ne 0 -and $semantic[$semanticBase + 8] -lt $ownerProfile[4] -and $semantic[$semanticBase + 9] -le 1 -and $semantic[$semanticBase + 10] -le 3) "Renderer semantic owner $ownerIndex first provenance/outcome was out of bounds at frame $frame." $gdbStdout
                                }
                                Assert-Condition ($ownerEventCount -eq $semantic[3] -and $ownerOverflowCount -eq $semantic[4]) "Renderer semantic owner totals did not conserve frame events/overflow at frame $frame." $gdbStdout
                                $semanticSamples.Add($semantic)
                            }
                            $coarseSamples.Add($coarse)
                        }

                        $coarseLoop = Get-SampleFieldValues $coarseSamples 1
                        $coarseInput = Get-SampleFieldValues $coarseSamples 2
                        $coarseUpdate = Get-SampleFieldValues $coarseSamples 3
                        $coarseSourceUpdate = Get-SampleFieldValues $coarseSamples 4
                        $coarseAudioUpdate = Get-SampleFieldValues $coarseSamples 5
                        $coarseActive = Get-SampleFieldValues $coarseSamples 6
                        $coarseWait = Get-SampleFieldValues $coarseSamples 7
                        $coarseBegin = Get-SampleFieldValues $coarseSamples 8
                        $coarseDraw = Get-SampleFieldValues $coarseSamples 9
                        $coarseWallpaper = Get-SampleFieldValues $coarseSamples 10
                        $coarseStage = Get-SampleFieldValues $coarseSamples 11
                        $coarseMario = Get-SampleFieldValues $coarseSamples 12
                        $coarseFox = Get-SampleFieldValues $coarseSamples 13
                        $coarseForeground = Get-SampleFieldValues $coarseSamples 14
                        $coarseHud = Get-SampleFieldValues $coarseSamples 15
                        $coarseFlush = Get-SampleFieldValues $coarseSamples 16
                        $coarsePostVBlank = Get-SampleFieldValues $coarseSamples 17
                        $coarseThreads = Get-SampleFieldValues $coarseSamples 18
                        $coarseDrawResidual = Get-SampleFieldValues $coarseSamples 19
                        $coarsePresentResidual = Get-SampleFieldValues $coarseSamples 20
                        $coarseLoopResidual = Get-SampleFieldValues $coarseSamples 21
                        $coarseConservation = Get-SampleFieldValues $coarseSamples 22
                        $coarseLogic = Get-SampleFieldValues $coarseSamples 23
                        $coarseMetricSummary = "Renderer coarse benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) logic=$($coarseLogic[0])..$($coarseLogic[-1]) median/p95 ticks loop=$(Get-MedianP95 $coarseLoop) input=$(Get-MedianP95 $coarseInput) update=$(Get-MedianP95 $coarseUpdate) sourceUpdate=$(Get-MedianP95 $coarseSourceUpdate) audioUpdate=$(Get-MedianP95 $coarseAudioUpdate) active=$(Get-MedianP95 $coarseActive) wait=$(Get-MedianP95 $coarseWait) begin=$(Get-MedianP95 $coarseBegin) draw=$(Get-MedianP95 $coarseDraw) wallpaper=$(Get-MedianP95 $coarseWallpaper) stage=$(Get-MedianP95 $coarseStage) Mario=$(Get-MedianP95 $coarseMario) Fox=$(Get-MedianP95 $coarseFox) foreground=$(Get-MedianP95 $coarseForeground) hud=$(Get-MedianP95 $coarseHud) flush=$(Get-MedianP95 $coarseFlush) postVBlank=$(Get-MedianP95 $coarsePostVBlank) threads=$(Get-MedianP95 $coarseThreads) drawResidual=$(Get-MedianP95 $coarseDrawResidual) presentResidual=$(Get-MedianP95 $coarsePresentResidual) loopResidual=$(Get-MedianP95 $coarseLoopResidual) conservationError=$(Get-MedianP95 $coarseConservation)"
                        $coarseResidualRatioSummary = "Renderer coarse residual ratios (median/p95 basis points): draw=$(Get-MedianP95 $drawResidualRatios) present=$(Get-MedianP95 $presentResidualRatios) loop=$(Get-MedianP95 $loopResidualRatios)"
                        $gxBoundarySummary = "Renderer GX boundary: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) adjacent changes/distinct values statusBefore=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 1)) controlBefore=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 2)) statusAfterFlush=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 3)) statusPostVBlank=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 4)) controlPostVBlank=$(Get-AdjacentChurn (Get-SampleFieldValues $gxBoundarySamples 5)) median/p95 ticks flush=$(Get-MedianP95 (Get-SampleFieldValues $gxBoundarySamples 6))"
                        $stage0Summary = "Renderer stage layer-0 benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) median/p95 ticks=$(Get-MedianP95 (Get-SampleFieldValues $stage0Samples 1)) adjacent changes/distinct values=$(Get-AdjacentChurn (Get-SampleFieldValues $stage0Samples 1))"
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                            $sinkMetricSummary = "Renderer CPU_PREP_NO_GX sink: samples=$RendererBenchmarkSamples words=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 2)) cursor=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 1)) stageWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 5)) MarioWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 6)) FoxWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 7)) calibrationWords=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 3)) calibrationTicks=$(Get-MedianP95 (Get-SampleFieldValues $sinkSamples 4))"
                        }
                        if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                            Assert-Condition (((Get-SampleFieldValues $warmSamples 1) | Measure-Object -Sum).Sum -gt 0) 'WARM_NO_UPLOAD window did not observe any animated texture refresh to suppress.' $gdbStdout
                            $warmMetricSummary = "Renderer WARM_NO_UPLOAD: samples=$RendererBenchmarkSamples suppressedUploads=$(Get-MedianP95 (Get-SampleFieldValues $warmSamples 1)) suppressedBytes=$(Get-MedianP95 (Get-SampleFieldValues $warmSamples 2))"
                        }

                        $ownerLabels = @('stage', 'Mario', 'Fox')
                        if ($RendererProfileLevel -ge 2) {
                            for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                $samples = $ownerSamples[$ownerIndex]
                                $submitClasses = @()
                                foreach ($classIndex in 0..7) {
                                    $submitClasses += "class$classIndex=$(Get-MedianP95 (Get-SampleFieldValues $samples (9 + $classIndex)))"
                                }
                                $ownerCensusSummaries += "Renderer owner census $($ownerLabels[$ownerIndex]): samples=$RendererBenchmarkSamples median/p95 counts selected=$(Get-MedianP95 (Get-SampleFieldValues $samples 3)) sourceCommands=$(Get-MedianP95 (Get-SampleFieldValues $samples 4)) vertexCommands=$(Get-MedianP95 (Get-SampleFieldValues $samples 5)) sourceVertices=$(Get-MedianP95 (Get-SampleFieldValues $samples 6)) triangleCommands=$(Get-MedianP95 (Get-SampleFieldValues $samples 7)) triangles=$(Get-MedianP95 (Get-SampleFieldValues $samples 8)) $($submitClasses -join ' ') materialOps=$(Get-MedianP95 (Get-SampleFieldValues $samples 17)) matrixChanges=$(Get-MedianP95 (Get-SampleFieldValues $samples 18)) textureChanges=$(Get-MedianP95 (Get-SampleFieldValues $samples 19)) runs=$(Get-MedianP95 (Get-SampleFieldValues $samples 20))"
                                $ownerChurnSummaries += "Renderer owner churn ($RendererBenchmarkSamples frames; adjacent changes/distinct values) $($ownerLabels[$ownerIndex]): stateEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 21)) stateExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 22)) cacheEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 23)) cacheExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 24)) resolverEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 25)) resolverExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 26)) globalEntry=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 27)) globalExit=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 28)) topology=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 29)) selected=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 30)) camera=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 31)) DObj=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 32)) material=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 33)) light=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 34)) texture=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 35)) semantic=$(Get-AdjacentChurn (Get-SampleFieldValues $samples 36))"
                            }
                            $semanticOwnerMetrics = @()
                            $semanticOwnerHashChurn = @()
                            for ($ownerIndex = 0; $ownerIndex -lt 3; $ownerIndex++) {
                                $semanticBase = 5 + (11 * $ownerIndex)
                                $semanticOwnerMetrics += "$($ownerLabels[$ownerIndex])Events=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples ($semanticBase + 2))) $($ownerLabels[$ownerIndex])Overflow=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples ($semanticBase + 3))) $($ownerLabels[$ownerIndex])Occurrences=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples ($semanticBase + 4)))"
                                $semanticOwnerHashChurn += "$($ownerLabels[$ownerIndex])Hash1=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples $semanticBase)) $($ownerLabels[$ownerIndex])Hash2=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 1)))"
                                $semanticProvenanceSummaries += "Renderer semantic first provenance ($RendererBenchmarkSamples frames; adjacent changes/distinct values) $($ownerLabels[$ownerIndex]): occurrence=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 5))) list=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 6))) branch=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 7))) command=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 8))) tri2Half=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 9))) outcome=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples ($semanticBase + 10)))"
                            }
                            $semanticMetricSummary = "Renderer semantic benchmark: samples=$RendererBenchmarkSamples frames=$($benchFrames[0])..$($benchFrames[-1]) median/p95 counts frameEvents=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples 3)) frameOverflow=$(Get-MedianP95 (Get-SampleFieldValues $semanticSamples 4)) $($semanticOwnerMetrics -join ' ')"
                            $semanticChurnSummary = "Renderer semantic hash churn (adjacent changes/distinct values): frameHash1=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples 1)) frameHash2=$(Get-AdjacentChurn (Get-SampleFieldValues $semanticSamples 2)) $($semanticOwnerHashChurn -join ' ')"
                        } else {
                            $ownerCensusSummaries += 'Renderer owner detailed census: unavailable (profile 1 coarse timing only)'
                            $semanticChurnSummary = 'Renderer semantic hash churn: unavailable (profile 1 coarse timing only)'
                        }
                    }
                    $logicTickStart = $null
                    $logicTickEnd = $null
                    if ($RendererProfileLevel -ge 1) {
                        $logicTickStart = [int64]$coarseLogic[0]
                        $logicTickEnd = [int64]$coarseLogic[-1]
                    }
                    $benchmarkIdentity = [ordered]@{
                        schema = 1
                        target = $benchmarkMakeIdentity.Target
                        build = $Build
                        harness = [ordered]@{
                            name = $benchmarkMakeIdentity.Harness
                            id = $benchmarkMakeIdentity.HarnessId
                        }
                        rendererProfile = $benchmarkMakeIdentity.Profile
                        rendererBenchmarkMode =
                            $benchmarkMakeIdentity.RendererBenchmarkMode
                        effectiveCFlags = [ordered]@{
                            common = $benchmarkMakeIdentity.CommonCFlags
                            renderer = $benchmarkMakeIdentity.RendererCFlags
                            scene = $benchmarkMakeIdentity.SceneCFlags
                            source = 'make print-benchmark-flags'
                        }
                        artifacts = [ordered]@{
                            rom = [ordered]@{
                                path = $rom
                                sha256 = $benchmarkRomIdentity.Sha256
                                bytes = $benchmarkRomIdentity.Bytes
                                lastWriteUtc = $benchmarkRomIdentity.LastWriteUtc
                            }
                            elf = [ordered]@{
                                path = $elf
                                sha256 = $benchmarkElfIdentity.Sha256
                                bytes = $benchmarkElfIdentity.Bytes
                                lastWriteUtc = $benchmarkElfIdentity.LastWriteUtc
                            }
                        }
                        melonDS = [ordered]@{
                            version = $benchmarkMelonIdentity.Version
                            executableSha256 = $benchmarkMelonIdentity.Sha256
                            path = $benchmarkMelonIdentity.Path
                            configPath = $configState.Config
                            configSha256 = $benchmarkMelonConfigSha256
                            settings = [ordered]@{
                                limitFPS = $false
                                jit = $false
                                gdbArm9Port = $configState.GdbPort
                                gdbArm7Port = $configState.Arm7Port
                            }
                        }
                        sampling = [ordered]@{
                            warmupFrames = [Math]::Max(0, ([int64]$benchFrames[0] - 1))
                            samples = $RendererBenchmarkSamples
                            frameStart = [int64]$benchFrames[0]
                            frameEnd = [int64]$benchFrames[-1]
                            logicTickStart = $logicTickStart
                            logicTickEnd = $logicTickEnd
                            logicTickTimerStartResets = $logicTickResetCount
                            delaySeconds = [Math]::Max($DelaySeconds, $minimumDelay)
                        }
                    }
                    $benchmarkIdentitySummary =
                        'BENCH_IDENTITY=' +
                        ($benchmarkIdentity | ConvertTo-Json -Compress -Depth 6)
                }
                if ($RendererBenchmarkOnly) {
                    Assert-Condition ($benchmarkMakeIdentity.RendererBenchmarkMode -gt 0) 'Benchmark-only verifier was not built with a renderer benchmark mode.' ($benchmarkMakeIdentity | Format-List | Out-String)
                    if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 2) {
                        $sinkProfile = Get-Ints $renderProfile
                        $sinkBatch = Get-Ints $renderBatch
                        $sinkSubmit = Get-Ints $renderSubmit
                        $sinkPlatform = Get-Ints $platformHw
                        Assert-Condition ($renderProfile.Success -and $sinkProfile[15] -eq 2484 -and $sinkProfile[16] -eq 828 -and $sinkProfile[17] -eq 0) 'CPU_PREP_NO_GX did not execute exact 2,484-vertex/828-triangle CPU preparation.' $gdbStdout
                        Assert-Condition ($renderBatch.Success -and $sinkBatch[0] -eq 121 -and $sinkBatch[1] -eq 707 -and $sinkBatch[2] -eq 121 -and $sinkBatch[3] -eq 98 -and $sinkBatch[4] -eq 730) 'CPU_PREP_NO_GX drifted from exact batch and texture-preparation policy.' $gdbStdout
                        Assert-Condition ($renderSubmit.Success -and $sinkSubmit[0] -eq 648 -and $sinkSubmit[1] -eq 0 -and $sinkSubmit[2] -eq 44 -and $sinkSubmit[3] -eq 126 -and $sinkSubmit[4] -eq 0 -and $sinkSubmit[5] -eq 0 -and $sinkSubmit[6] -eq 10 -and $sinkSubmit[7] -eq 0) 'CPU_PREP_NO_GX drifted from the exact 648/44/126/10 submit-class partition.' $gdbStdout
                        Assert-Condition ($platformHw.Success -and $sinkPlatform[0] -gt 0 -and $sinkPlatform[2] -eq 0 -and $sinkPlatform[3] -eq 0) 'CPU_PREP_NO_GX unexpectedly emitted GX geometry or failed to complete benchmark frames.' $gdbStdout
                    }
                    if ($benchmarkMakeIdentity.RendererBenchmarkMode -eq 4) {
                        $warmProfile = Get-Ints $renderProfile
                        $warmBatch = Get-Ints $renderBatch
                        $warmSubmit = Get-Ints $renderSubmit
                        $warmPlatform = Get-Ints $platformHw
                        Assert-Condition ($renderProfile.Success -and $warmProfile[12] -eq 0 -and $warmProfile[13] -eq 0 -and $warmProfile[15] -eq 2484 -and $warmProfile[16] -eq 828 -and $warmProfile[17] -eq 0) 'WARM_NO_UPLOAD did not preserve exact geometry or leaked a real texture upload into the warm window.' $gdbStdout
                        Assert-Condition ($renderBatch.Success -and $warmBatch[0] -eq 121 -and $warmBatch[1] -eq 707 -and $warmBatch[2] -eq 121 -and $warmBatch[3] -eq 98 -and $warmBatch[4] -eq 730) 'WARM_NO_UPLOAD drifted from exact batch and texture-preparation policy.' $gdbStdout
                        Assert-Condition ($renderSubmit.Success -and $warmSubmit[0] -eq 648 -and $warmSubmit[1] -eq 0 -and $warmSubmit[2] -eq 44 -and $warmSubmit[3] -eq 126 -and $warmSubmit[4] -eq 0 -and $warmSubmit[5] -eq 0 -and $warmSubmit[6] -eq 10 -and $warmSubmit[7] -eq 0) 'WARM_NO_UPLOAD drifted from the exact 648/44/126/10 submit-class partition.' $gdbStdout
                        # The terminal hardware counters are sampled at a later
                        # live animation/logic frame than the synchronized CPU
                        # window. Clipping can leave the final observed RAM one
                        # triangle below the canonical frozen 715/2167 value;
                        # exact 828 CPU triangles, submit classes, batches, and
                        # submitted/flush frame conservation are the benchmark
                        # geometry contract.
                        Assert-Condition ($platformHw.Success -and $warmPlatform[0] -gt 0 -and $warmPlatform[0] -eq $warmPlatform[1] -and $warmPlatform[2] -gt 0 -and $warmPlatform[3] -gt 0) 'WARM_NO_UPLOAD failed to submit and flush live GX geometry.' $gdbStdout
                    }
                    Write-Output $benchmarkIdentitySummary
                    Write-Output $benchmarkMetricSummary
                    Write-Output $benchmarkChurnSummary
                    Write-Output $coarseMetricSummary
                    Write-Output $coarseResidualRatioSummary
                    Write-Output $gxBoundarySummary
                    Write-Output $stage0Summary
                    if ($sinkMetricSummary) { Write-Output $sinkMetricSummary }
                    if ($warmMetricSummary) { Write-Output $warmMetricSummary }
                    $ownerCensusSummaries | ForEach-Object { Write-Output $_ }
                    Write-Output "$Label renderer benchmark-only sample passed."
                    return
                }
                Assert-Condition ($rendererProfileMarker.Success -and [int64]$rendererProfileMarker.Groups[1].Value -eq $RendererProfileLevel) "Canonical realtime HW build did not report renderer profile level $RendererProfileLevel." $gdbStdout
                Assert-Condition $renderTexHash.Success 'Canonical realtime HW build did not publish texture lookup accounting.' $gdbStdout
                if ($RendererProfileLevel -lt 2) {
                    # The resident texture cache survives frame boundaries, so a
                    # completed warm frame may legitimately have zero misses.
                    # Active and indexed hits, conservation, and the probe bound
                    # are the invariant performance contract.
                    Assert-Condition ($rth[0] -gt 0 -and $rth[2] -gt 0 -and $rth[3] -gt 0 -and ($rth[2] + $rth[3] + $rth[4]) -eq $rth[0] -and $rth[1] -ge ($rth[3] + $rth[4]) -and $rth[1] -lt (4 * $rth[0])) 'Performance/coarse texture hash lookup lacked active/table coverage, exact accounting, or bounded probes.' $gdbStdout
                } else {
                    Assert-Condition (($rth | Measure-Object -Sum).Sum -eq 0) 'Forensic renderer unexpectedly used the performance texture hash lookup.' $gdbStdout
                }
                Assert-Condition ($platformHw.Success -and $hw[0] -gt 0 -and $hw[0] -eq $hw[1]) 'Canonical realtime HW build did not flush submitted DS 3D frames.' $gdbStdout
                Assert-Condition ($hw[2] -gt 0 -and $hw[3] -gt 0) 'Canonical realtime HW build submitted CPU-side triangles but DS GX polygon/vertex RAM stayed empty.' $gdbStdout
                Assert-Condition ($stageHardware.Success -and $shw[0] -eq (42 * $hw[0]) -and $shw[1] -eq (202 * $hw[0]) -and $shw[1] -eq ($shw[2] + $shw[3]) -and $shw[5] -gt 0 -and $shw[6] -gt 0 -and $shw[7] -gt 0 -and $shw[8] -eq 0) 'Canonical realtime HW build drifted from the exact per-frame 42-list/202-triangle textured stage contract.' $gdbStdout
                Assert-Condition ($stageCarry.Success -and $scarry[0] -eq $scarry[1] -and $scarry[0] -gt 8 -and $scarry[2] -gt 0 -and $scarry[3] -gt 0 -and $scarry[4] -gt 0 -and $scarry[5] -gt 0) 'Canonical realtime HW build did not prove persistent stage DObj texture/tile carry.' $gdbStdout
                Assert-Condition ($stageHardwareFighter.Success -and $shwf[0] -eq (2 * $hw[0]) -and $shwf[1] -eq (626 * $hw[0])) 'Canonical realtime HW build drifted from the exact per-frame two-owner/626-triangle fighter contract.' $gdbStdout
                # ftdisplaymain.c:1164-1242 sets this preamble and traverses only
                # source-selected, visible, textured fighter part display lists.
                Assert-Condition ($fighterDisplayContract.Success -and $fdc[0] -gt 0 -and $fdc[3] -gt 0 -and $fdc[0] -ge $fdc[3] -and ($fdc[0] - $fdc[3]) -le 64 -and (($fdc[4] -band 0x222005) -eq 0x222005) -and $fdc[5] -gt 0 -and $fdc[6] -gt 0 -and $fdc[7] -gt 0 -and $fdc[8] -eq 0 -and $fdc[11] -gt 0 -and $fdc[12] -gt 0 -and $fdc[13] -eq 0x00100000 -and $fdc[14] -eq [Convert]::ToUInt32('c4112078', 16)) 'Canonical realtime HW build did not preserve the original fighter display selection, lighting, geometry, cycle, render-mode, and visibility contract.' $gdbStdout
                Assert-Condition ($renderProfile.Success -and $rp[15] -eq 2484 -and $rp[16] -eq 828 -and $rp[17] -eq 0) 'Canonical realtime HW build drifted from the exact 2,484-vertex/828-triangle renderer contract.' $gdbStdout
                Assert-Condition (Test-RendererUploadPair $rp[12] $rp[13]) 'Realtime HW build reported an invalid canonical texture upload count/byte pair.' $gdbStdout
                Assert-Condition ($renderBatch.Success -and $rb[0] -eq 121 -and $rb[1] -eq 707 -and $rb[2] -eq 121 -and $rb[3] -eq 98 -and $rb[4] -eq 730) 'Canonical realtime HW build drifted from exact 121/707/121 batch and 98/730 texture-prepare accounting.' $gdbStdout
                if ($RendererProfileLevel -lt 2) {
                    Assert-Condition ($renderCi4Lut.Success -and $rci4lut[2] -eq 2 -and $rci4lut[3] -gt $rci4lut[2]) 'Performance/coarse renderer did not build exactly two immutable CI4 source-index planes and reuse them across live water addressing.' $gdbStdout
                    Assert-Condition ($renderCi4Map.Success -and $rci4map[0] -gt 0 -and $rci4map[1] -ge $rci4map[0]) 'Performance/coarse renderer did not resolve live large-water pixels through exact representative address/phase classes.' $gdbStdout
                } else {
                    Assert-Condition ($renderCi4Lut.Success -and $rci4lut[2] -eq 0 -and $rci4lut[3] -eq 0) 'Forensic renderer unexpectedly bypassed its independent bytewise CI4 source decoder.' $gdbStdout
                    Assert-Condition ($renderCi4Map.Success -and $rci4map[0] -eq 0 -and $rci4map[1] -eq 0) 'Forensic renderer unexpectedly reused performance CI4 representative maps.' $gdbStdout
                }
                if ($RendererProfileLevel -ge 2) {
                    Assert-Condition ($renderTopology.Success -and $rtopo[0] -gt 0 -and $rtopo[1] -gt 0 -and $rtopo[2] -gt 0) 'Profiled renderer did not hoist immutable reloc-list validation while retaining dynamic-list fallback validation.' $gdbStdout
                    Assert-Condition ($renderCost.Success -and $rcost[0] -gt 0 -and $rcost[1] -gt 0 -and $rcost[0] -ge $rcost[1]) 'Profiled renderer did not report a coherent triangle/vertex submission cost split.' $gdbStdout
                    if ($rp[12] -gt 0) {
                        Assert-Condition ($renderCi4Lut.Success -and ($rci4lut[0] + $rci4lut[1]) -gt 0) 'Profiled renderer uploaded animated textures without building or reusing the exact CI4 palette-pair LUT.' $gdbStdout
                    }
                } elseif ($RendererProfileLevel -eq 1) {
                    Assert-Condition ($renderTopology.Success -and (($rtopo | Measure-Object -Sum).Sum -eq 0) -and $renderCost.Success -and (($rcost | Measure-Object -Sum).Sum -eq 0)) 'Low-frequency O2 coarse profile unexpectedly retained detailed command/triangle profiling.' $gdbStdout
                }
                Assert-Condition ($wallpaperCache.Success -and $wc[0] -eq 1 -and $wc[1] -ge 1 -and $wc[2] -eq ($wc[0] + $wc[1]) -and $wc[3] -eq 0 -and $wc[4] -eq 300 -and $wc[5] -eq 220 -and $wc[6] -eq 66000 -and $wc[7] -gt 0 -and $wc[8] -gt 0) 'Canonical realtime HW build did not construct once and reuse the exact opaque Dream Land wallpaper decode cache.' $gdbStdout
                Assert-Condition ($wallpaperFinal.Success -and $wf[0] -eq $wc[2] -and $wf[0] -eq ($wf[1] + $wf[2]) -and $wf[2] -gt 0 -and $wf[3] -eq (49152 * $wf[2]) -and $wf[4] -eq 0 -and $wf[5] -eq 0 -and $wf[6] -eq 0 -and $wf[7] -eq 0 -and $wf[8] -eq (2 * $wf[3]) -and $wf[9] -eq 0 -and $wf[11] -eq 0) 'Canonical realtime HW build did not retain exact final BG2/BG3 ownership or still performed eliminated full-screen clears/staging/copies.' $gdbStdout
                Assert-Condition ($renderClip.Success -and $rclip[0] -eq $rv[12]) 'Canonical realtime HW build did not report a consistent clipping/saturation marker.' $gdbStdout
                Assert-Condition ($renderTexel1.Success -and $rt1[0] -gt 0 -and $rt1[1] -eq $rt1[0] -and $rt1[2] -eq 0 -and $rt1[9] -gt 0 -and $rt1[10] -eq 0 -and $rt1[11] -gt 0) 'Canonical realtime HW build did not resolve, directly precompose, and refresh the source Dream Land TEXEL0/TEXEL1 water material without evicting resident textures.' $gdbStdout
                Assert-Condition ($mobjAttach.Success -and $ma[0] -ge 4 -and $ma[1] -eq 0 -and $ma[2] -eq 0 -and $ma[3] -eq 0x0200 -and $ma[4] -eq 0x006b) 'Canonical realtime HW build did not normalize the live water and Whispy mixed-width O2R MObjSub fields at the BattleShip attachment boundary.' $gdbStdout
                Assert-Condition ($renderRawMatrix.Success -and $rrm[0] -gt 0) 'Canonical realtime HW build found no current-matrix ordinary-Z raw candidates.' $gdbStdout
                $submitTotal = $rs[0] + $rs[1] + $rs[2] + $rs[3] + $rs[4] + $rs[5] + $rs[6]
                $expectedProjectedDivisions = (6 * $rs[3]) + (9 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
                $preCutoverProjectedDivisions = (9 * ($rs[0] + $rs[1] + $rs[2] + $rs[4] + $rs[5] + $rs[6])) + (6 * $rs[3])
                $expectedProjectedFallbackCount = $rs[2] + $rs[4] + $rs[5] + $rs[6]
                if ($RendererProfileLevel -ge 2) {
                    # Forensic command diagnostics intentionally accumulate across
                    # the whole capture, while submit-class counters describe the
                    # completed frame. Every captured hardware frame has the same
                    # stable canonical geometry contract.
                    $expectedProjectedFallbackCount *= $hw[0]
                }
                Assert-Condition ($renderSubmit.Success -and $rs[0] -eq 648 -and $rs[1] -eq 0 -and $rs[2] -eq 44 -and $rs[3] -eq 126 -and $rs[4] -eq 0 -and $rs[5] -eq 0 -and $rs[6] -eq 10 -and $rs[7] -eq 0 -and $rs[0] -eq $rrm[0] -and $rs[2] -eq $rrm[2] -and $rs[6] -eq $rrm[1] -and $submitTotal -eq $rp[16]) 'Canonical realtime HW build drifted from the exact 648/44/126/10 hybrid submit-class partition.' $gdbStdout
                Assert-Condition ($rs[8] -eq $expectedProjectedDivisions -and ($rs[8] * 4) -lt $preCutoverProjectedDivisions) 'Canonical realtime HW build did not sharply reduce and exactly account projected division demand.' $gdbStdout
                $hardwareDivideEvaluations = $rhdiv[0] + $rhdiv[1] + $rhdiv[2]
                Assert-Condition ($renderHardwareDivide.Success -and $rhdiv[3] -eq 0 -and $rhdiv[4] -eq 0) 'Canonical realtime projected hardware divider reported a zero denominator or exact-result mismatch.' $gdbStdout
                if ($RendererProfileLevel -lt 2) {
                    Assert-Condition ($hardwareDivideEvaluations -eq 0) 'Shipping-equivalent profile retained hot-loop hardware-divider telemetry.' $gdbStdout
                } else {
                    $expectedHardwareDivideEvaluations = $rs[8] + (3 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
                    Assert-Condition ($rhdiv[0] -gt 0 -and $hardwareDivideEvaluations -eq $expectedHardwareDivideEvaluations) 'Forensic renderer did not compare every live DS hardware quotient with the former exact C result.' $gdbStdout
                }
                Assert-Condition ($renderLazy.Success -and $rlazy[0] -gt 0 -and $rlazy[3] -gt 0 -and $rlazy[4] -gt 0 -and $rlazy[5] -eq 0) 'Canonical realtime HW matrix snapshot table lacked natural load/create/reuse coverage or overflowed.' $gdbStdout
                Assert-Condition ($renderCombine.Success -and $rc[4] -eq $expectedProjectedFallbackCount) 'Canonical realtime HW projected-fallback accounting does not match the exceptional source-Z classes.' $gdbStdout
                Assert-Condition ($fighterLightSeed.Success -and $fls[0] -gt 0 -and $fls[1] -eq [Convert]::ToUInt32('ffffff00', 16) -and $fls[2] -eq [Convert]::ToUInt32('4c4c4c00', 16)) 'Canonical realtime HW build did not seed the fighter RSP light state from the selected source MObj material.' $gdbStdout
                if ($RendererProfileLevel -ge 2) {
                    Assert-Condition ($rlazy[1] -eq $rlazy[0] -and $rlazy[2] -gt 0) 'Forensic renderer did not eagerly transform every source vertex before exercising transform-cache hits.' $gdbStdout
                    Assert-Condition ($renderOracle.Success -and $ro[0] -eq 2484 -and $ro[1] -eq 0 -and $ro[2] -eq 0) 'Forensic realtime HW drifted from the exact 2,484/0/0 CPU 20.12 oracle contract.' $gdbStdout
                    Assert-Condition ($renderMatrix.Success) 'Forensic realtime HW build did not report loaded GX matrix ranges.' $gdbStdout
                    Assert-Condition ($renderVertex.Success) 'Forensic realtime HW build did not report submitted vertex ranges.' $gdbStdout
                    Assert-Condition ($renderDepth.Success -and $rd[0] -gt 0 -and $rd[5] -gt 0 -and $rd[10] -gt 0) 'Forensic realtime HW build did not report source-depth samples for the stage, Mario, and Fox.' $gdbStdout
                    Assert-Condition ($rd[1] -ge -4096 -and $rd[2] -le 4095 -and $rd[1] -le $rd[2] -and $rd[3] -gt 0 -and $rd[3] -le $rd[4] -and $rd[6] -ge -4096 -and $rd[7] -le 4095 -and $rd[6] -le $rd[7] -and $rd[8] -gt 0 -and $rd[8] -le $rd[9] -and $rd[11] -ge -4096 -and $rd[12] -le 4095 -and $rd[11] -le $rd[12] -and $rd[13] -gt 0 -and $rd[13] -le $rd[14]) 'Forensic realtime HW source-depth samples left signed 20.12 NDC or reported invalid clip W.' $gdbStdout
                    Assert-Condition ($renderTexture.Success -and $rt[0] -gt 0 -and $rt[1] -gt 0 -and $rt[2] -gt 0 -and $rt[3] -gt 0 -and $rt[4] -gt 0 -and $rt[5] -gt 0 -and $rt[6] -gt 0 -and $rt[8] -lt $rt[9] -and $rt[10] -lt $rt[11]) 'Forensic realtime HW build did not prove Dream Land texture data and texcoords reached GX submission.' $gdbStdout
                    Assert-Condition ($rt1[3] -eq 0 -and $rt1[4] -le 0xff -and $rt1[5] -ne 0 -and $rt1[6] -ne 0 -and $rt1[5] -ne $rt1[6]) 'Forensic realtime HW build did not retain TEXEL0/TEXEL1 source-state diagnostics.' $gdbStdout
                    Assert-Condition ($renderTexUse.Success) 'Forensic realtime HW build did not report texture-use rejection classes.' $gdbStdout
                    Assert-Condition ($renderTexFmt.Success -and $rtf[0] -ne 0 -and $rtf[1] -ne 0 -and $rtf[4] -eq 0) 'Forensic realtime HW build had unexplained texture bind failures by format.' $gdbStdout
                    Assert-Condition ($renderTexLane.Success -and (($rtl[0] -band 0x2) -ne 0) -and $rtl[1] -gt 0 -and $rtl[2] -gt 0 -and (($rtl[3] -band 0x100) -ne 0) -and $rtl[5] -eq 0x00010203 -and $rtl[6] -eq 0x02030001) 'Forensic realtime HW build did not prove O2R texture byte-lane decoding on the active CI4 path.' $gdbStdout
                    Assert-Condition ($rc[0] -gt 0 -and $rc[1] -gt 0) 'Forensic realtime HW build did not report source combine modes.' $gdbStdout
                    Assert-Condition ($renderLight.Success -and $rl[2] -eq 0) 'Forensic realtime HW build reported a source-light fallback.' $gdbStdout
                    Assert-Condition ($renderAdapterCache.Success -and $rac[0] -gt $rac[1] -and $rac[1] -gt 0 -and $rac[2] -eq 0 -and $rac[3] -gt 0 -and $rac[4] -gt 0 -and $rac[5] -eq 0) 'Forensic realtime HW build did not reuse frame-local camera/DObj matrices without cache overflow.' $gdbStdout
                    Assert-Condition ($rrm[3] -gt 0 -and $rrm[4] -eq 0 -and $rrm[5] -le 16 -and $rrm[6] -eq 0 -and $rrm[7] -eq 0 -and $rrm[8] -gt 0 -and $rrm[9] -eq 0) 'Forensic corrected composed GX matrix PosTest failed homogeneous, W-sign, clip, matrix-word, or capacity checks.' $gdbStdout
                } else {
                    Assert-Condition ($rlazy[1] -gt 0 -and (2 * $rlazy[1]) -lt $rlazy[0] -and $rlazy[2] -gt 0) 'Performance/coarse renderer did not defer a majority of CPU vertex transforms or reuse projected exceptions.' $gdbStdout
                    Assert-Condition ($renderOracle.Success -and $ro[0] -eq 0 -and $ro[1] -eq 0 -and $ro[2] -eq 0) 'Performance/coarse realtime HW build still performed forensic oracle transforms.' $gdbStdout
                    Assert-Condition ($rv[12] -eq 0) 'Performance/coarse realtime HW build saturated a submitted DS vertex.' $gdbStdout
                    Assert-Condition ($rt[0] -eq 0 -and $rt[3] -eq 0 -and $rt[4] -eq 0 -and $rc[0] -eq 0 -and $rc[1] -eq 0) 'Performance/coarse realtime HW build still published per-vertex or per-command forensic diagnostics.' $gdbStdout
                    Assert-Condition ($rrm[3] -eq 0 -and $rrm[4] -eq 0 -and $rrm[5] -eq 0 -and $rrm[6] -eq 0 -and $rrm[7] -eq 0 -and $rrm[8] -eq 0 -and $rrm[9] -eq 0) 'Performance/coarse realtime HW build still ran the GX PosTest oracle.' $gdbStdout
                }
                $hardwareSummary = " rprof=$RendererProfileLevel$benchmarkSummary gxram=$($hw[2])/$($hw[3]) gxstat=0x{0:x}/ctrl=0x{1:x} ftrContract=$($fdc[0])/$($fdc[3])/geom0x{17:x}/cycle0x{18:x}/rm0x{19:x}/light$($fdc[5])/$($fdc[6])/bounds$($fdc[7])/$($fdc[8]) oracle=$($ro[0])/$($ro[1])/$($ro[2]) batch=$($rb[0])/$($rb[1])/$($rb[2])/texprep$($rb[3])/$($rb[4]) wallCache=$($wc[0])/$($wc[1])/$($wc[2])/fb$($wc[3])/src$($wc[4])x$($wc[5])/$($wc[6])/ticks$($wc[7])/$($wc[8]) wallFinal=direct$($wf[0])/skip$($wf[1])/change$($wf[2])/px$($wf[3])/stage$($wf[4])/$($wf[5])/bg2$($wf[6])/$($wf[7])/$($wf[8])/bg3$($wf[9])/$($wf[10])/$($wf[11]) mtx=load$($rm[0])/scale$($rm[1])/p$($rm[2]),$($rm[3]),$($rm[4]),$($rm[5])/mv$($rm[6]),$($rm[7]),$($rm[8]),$($rm[9]),$($rm[10]),$($rm[11]) submit=raw$($rs[0])/snap$($rs[1])/cross$($rs[2])/noz$($rs[3])/dec$($rs[4])/prim$($rs[5])/range$($rs[6])/rej$($rs[7])/div$($rs[8]) lazy=load$($rlazy[0])/xf$($rlazy[1])/hit$($rlazy[2])/new$($rlazy[3])/reuse$($rlazy[4])/ovf$($rlazy[5]) rawcand=$($rrm[0])/$($rrm[1])/$($rrm[2]) postest=$($rrm[3])/$($rrm[4])/e$($rrm[5])/w$($rrm[6])/c$($rrm[7])/mw$($rrm[8])/drop$($rrm[9]) vraw=$($rv[0])..$($rv[1])/$($rv[2])..$($rv[3])/$($rv[4])..$($rv[5]) vhw=$($rv[6])..$($rv[7])/$($rv[8])..$($rv[9])/$($rv[10])..$($rv[11]) depth=stage$($rd[0]):$($rd[1])..$($rd[2])/p0$($rd[5]):$($rd[6])..$($rd[7])/p1$($rd[10]):$($rd[11]) clip=$($rclip[0]) texProof=$($rt[1])/$($rt[2])/$($rt[3]) sample=$($rt[5])/$($rt[6])/$($rt[4]) alias=$($rt[7]) st=$($rt[8])..$($rt[9])/$($rt[10])..$($rt[11]) texUse=$($rtu[0])/$($rtu[1])/$($rtu[2])/$($rtu[3])/$($rtu[4])/impl$($rtu[5])/first0x{7:x}/flags0x{8:x}/w0x{9:x}/w1x{10:x}/geom0x{11:x} texFmt=conv0x{2:x}/bind0x{3:x}/pal0x{4:x}/rej0x{5:x}/why0x{6:x} texLane=layout0x{12:x}/byte$($rtl[1])/half$($rtl[2])/bFmt0x{13:x}/hFmt0x{14:x}/bMap0x{15:x}/hMap0x{16:x} stageCarry=$($scarry[0])/$($scarry[1])/tex$($scarry[2])/tile$($scarry[3])/short$($scarry[4])/$($scarry[5])/seg$($scarry[6]) combine=$($rc[0])/$($rc[1])/lit$($rc[2])/mat$($rc[3])/proj$($rc[4]) light=$($rl[0])/$($rl[1])/$($rl[2]) profile=present$($rp[2])/draw$($rp[3])/stage$($rp[5])/mat$($rp[6])/dl$($rp[8])/tex$($rp[9])/conv$($rp[10])/upload$($rp[11]) texUploads=$($rp[12])/$($rp[13]) binds=$($rp[14]) vtx=$($rp[15]) tri=$($rp[16])" -f $hw[4], $hw[5], $rtf[0], $rtf[1], $rtf[2], $rtf[3], $rtf[4], $rtu[6], $rtu[7], $rtu[8], $rtu[9], $rtu[10], $rtl[0], $rtl[3], $rtl[4], $rtl[5], $rtl[6], $fdc[4], $fdc[13], $fdc[14]
                $hardwareSummary += " texDirect=$($rt1[11])"
                if ($RendererProfileLevel -ge 2) {
                    $hardwareSummary += " texel1state=0x{0:x}/0x{1:x}" -f $rt1[7], $rt1[8]
                    $hardwareSummary += " adapterCache=$($rac[0])/$($rac[1])/$($rac[2])/$($rac[3])/$($rac[4])/$($rac[5])"
                }
                if ($RendererProfileLevel -ge 1) {
                    $hardwareSummary += " hdiv=$($rhdiv[0])/$($rhdiv[1])/$($rhdiv[2])/z$($rhdiv[3])/mis$($rhdiv[4])"
                    $hardwareSummary += " topology=$($rtopo[0])/$($rtopo[1])/$($rtopo[2])/$($rtopo[3])"
                    $hardwareSummary += " cost=$($rcost[0])/$($rcost[1])"
                    $hardwareSummary += " ci4lut=$($rci4lut[0])/$($rci4lut[1])/idx$($rci4lut[2])/$($rci4lut[3])"
                    $hardwareSummary += " ci4map=$($rci4map[0])/$($rci4map[1])"
                }
                $hardwareSummary += " texHash=$($rth[0])/$($rth[1])/$($rth[2])/$($rth[3])/$($rth[4])"
            }
            if ($LiveInputPreview) {
                $livePad = [regex]::Match($gdbStdout, 'LIVE_PAD=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
                $lpv = Get-Ints $livePad
                Assert-Condition ($livePad.Success -and $lpv[0] -gt 0 -and $lpv[1] -gt 0 -and (($lpv[2] -band 3) -eq 3) -and $lpv[7] -eq 0 -and $lpv[8] -eq 0 -and $lpv[9] -eq 0 -and $lpv[10] -eq 0 -and $lpv[11] -eq 0 -and $lpv[16] -eq 0 -and $lpv[17] -eq 0 -and $lpv[18] -eq 0 -and $lpv[19] -eq 0) 'Canonical realtime build did not use live DS input with connected-neutral pad1.' $gdbStdout
            }
            if ($ImportBattleShipFTComputer) {
                $cpu = Get-Ints $computerAI
                Assert-Condition ($computerAI.Success -and $cpu[0] -eq 1 -and $cpu[1] -ge 2 -and $cpu[2] -gt 0 -and $cpu[3] -gt 0 -and $cpu[7] -gt 0 -and $cpu[20] -gt 0) 'Canonical realtime build did not run the imported Fox CPU setup/process/target/movement path.' $gdbStdout
                $hardwareSummary += " cpu=setup$($cpu[0])/proc$($cpu[2])/target$($cpu[3])/stick$($cpu[7])/obj0x$('{0:x}' -f $cpu[4])"
            }
            if ($ImportBattleShipAudioBGM) {
                $ab = Get-Ints $audioBgm
                Assert-Condition ($audioBgm.Success -and $ab[0] -eq 0x42474d31 -and (($ab[1] -band 0x1) -eq 0x1) -and (($ab[2] -eq 1) -or ($ab[6] -ge 1)) -and $ab[3] -eq 0 -and $ab[4] -eq 0x7800 -and $ab[5] -ge 1 -and $ab[9] -eq 0 -and $ab[10] -eq 0 -and $ab[11] -eq 0 -and $ab[13] -eq 65536 -and $ab[14] -eq 32768 -and $ab[19] -ge 42100 -and $ab[19] -le 46100 -and $ab[20] -eq 44100 -and $ab[22] -ge 4 -and $ab[23] -lt 65536 -and (($ab[24] -eq 0) -or ($ab[24] -eq 32768)) -and (($ab[25] -eq 0) -or ($ab[25] -eq 1)) -and (($ab[26] -eq 0) -or ($ab[26] -eq 1)) -and $ab[25] -ne $ab[26] -and $ab[27] -eq 0 -and $ab[28] -gt 0 -and $ab[29] -gt 0) 'Minimal BGM backend realtime smoke failed hardware-timer byte-rate or safe half refill guard.' $gdbStdout
            }
            if ($RendererBenchmarkSamples -gt 0) {
                Write-Output $benchmarkIdentitySummary
                Write-Output $benchmarkMetricSummary
                Write-Output $benchmarkChurnSummary
                if ($RendererProfileLevel -ge 1) {
                    Write-Output $coarseMetricSummary
                    Write-Output $coarseResidualRatioSummary
                    Write-Output $gxBoundarySummary
                    Write-Output $stage0Summary
                    $ownerCensusSummaries | ForEach-Object { Write-Output $_ }
                    $ownerChurnSummaries | ForEach-Object { Write-Output $_ }
                    if ($RendererProfileLevel -ge 2) {
                        Write-Output $semanticMetricSummary
                        Write-Output $semanticChurnSummary
                        $semanticProvenanceSummaries | ForEach-Object { Write-Output $_ }
                    } else {
                        Write-Output $semanticChurnSummary
                    }
                }
            }
            Write-Output ("$Label realtime pacing smoke passed: frames=$($bp[3]) fps=$($bp[6])/$($bp[7]) ticks=$($bp[5])$hardwareSummary$aobjSummary")
            return
        }
        if ($CPUOpponentProof) {
            $cpu = Get-Ints $computerAI
            # ftcomputer.c:6326 selects Attack inside 350 units; :7591-7592
            # dispatches that objective, and :3440-3460 emits A/B/Z commands.
            # ftmain.c:198-327 owns the resulting live attack-collision state.
            Assert-Condition ($computerAI.Success -and $cpu[0] -eq 1 -and $cpu[1] -ge 2 -and $cpu[2] -ge 1000 -and $cpu[3] -gt 0 -and (($cpu[4] -band 0x4) -eq 0x4) -and $cpu[6] -gt 0 -and $cpu[7] -gt 0 -and $cpu[8] -gt 0 -and $cpu[9] -gt 0 -and $cpu[10] -gt 0 -and $cpu[11] -gt 0 -and $cpu[12] -gt 0 -and $cpu[13] -gt 0 -and $cpu[15] -gt 0 -and $cpu[19] -gt 0 -and $cpu[20] -gt 0 -and ($cpu[23] - $cpu[22]) -ge 50000) 'Imported Fox CPU did not naturally target, move, attack with live hitboxes, guard, and damage Mario on Dream Land.' $gdbStdout
            if ($MatchLifecycleProof) {
                $life = Get-Ints $battleLifecycle
                $results = Get-Ints $vsResults
                $resultsFighters = Get-Ints $vsResultsFighters
                $resultsDisplay = Get-Ints $vsResultsDisplay
                # ifcommon.c:2472-2529 decrements the source timer and dispatches
                # Time Up at zero; :3144-3152 requests LoadScene after the end
                # interface; scvsbattle.c:513-560 owns the VS Results transition.
                # ifcommon.c:2486-2537 converts the configured minute count to
                # source tics and expires through the unchanged timer path.
                Assert-Condition ($battleLifecycle.Success -and $life[0] -eq 0x5642454e -and $life[1] -ge 1 -and $life[2] -ge 1 -and $life[3] -eq 1 -and $life[4] -eq 1 -and $life[5] -eq 0 -and $life[6] -ge 3600 -and $life[7] -eq 7 -and $life[8] -eq 22 -and $life[9] -eq 24 -and ($life[10] -eq 0 -or $life[10] -eq 1)) 'Original one-minute timer/end flow did not return through LoadScene and transition VSBattle to VS Results.' $gdbStdout
                Assert-Condition ($vsResults.Success -and $results[0] -eq 0x56535231 -and (($results[1] -band 0x1f) -eq 0x1f) -and $results[2] -ge 1 -and $results[3] -ge 120 -and $results[4] -eq 8 -and $results[5] -ge 2 -and $results[6] -gt 0 -and $results[7] -gt 0) 'Original VS Results scene did not load all source assets and advance through wallpaper/results/fighter creation.' $gdbStdout
                # mnvsresults.c:869-928 selects Win1..Win3 for place 0 and
                # Lose for other places; scsubsysfighter.c:74-77 installs it.
                $fighter0StatusOk = if ($resultsFighters[0] -eq 0) { $resultsFighters[1] -ge 0x10001 -and $resultsFighters[1] -le 0x10003 -and $resultsFighters[2] -ge 1 -and $resultsFighters[2] -le 3 } else { $resultsFighters[1] -eq 0x10005 -and $resultsFighters[2] -eq 5 }
                $fighter1StatusOk = if ($resultsFighters[3] -eq 0) { $resultsFighters[4] -ge 0x10001 -and $resultsFighters[4] -le 0x10003 -and $resultsFighters[5] -ge 1 -and $resultsFighters[5] -le 3 } else { $resultsFighters[4] -eq 0x10005 -and $resultsFighters[5] -eq 5 }
                Assert-Condition ($vsResultsFighters.Success -and $resultsFighters[0] -ne $resultsFighters[3] -and $fighter0StatusOk -and $fighter1StatusOk) 'Original VS Results fighters did not enter the source win/lose status and submotion paths.' $gdbStdout
                Assert-Condition ($vsResultsDisplay.Success -and $resultsDisplay[1] -gt 0 -and $resultsDisplay[2] -eq 256 -and $resultsDisplay[3] -eq 192) 'Original VS Results SObj display did not commit a full DS frame.' $gdbStdout
                Write-Output ("$Label one-minute lifecycle passed: adapter=$($life[1]) taskman=$($life[2]) ticks=$($life[6]) sudden=$($life[10]) scene=$($life[8])->$($life[9]) results=$($results[3])/files$($results[4])/fighters$($results[5])/sobjs$($results[7])")
            }
            Write-Output ("$Label original CPU proof passed: setup=$($cpu[0]) process=$($cpu[2]) target=$($cpu[3]) objective=0x$('{0:x}' -f $cpu[4]) behavior=0x$('{0:x}' -f $cpu[5]) inputs=$($cpu[6]) stick=$($cpu[7]) buttons=$($cpu[8])/$($cpu[9])/$($cpu[10]) attack=$($cpu[11])/$($cpu[12]) guard=$($cpu[13]) recover=$($cpu[14]) status=$($cpu[15]) damage=$($cpu[19]) x=$($cpu[21])/$($cpu[22])..$($cpu[23])/$($cpu[24])")
            return
        }
        $movementOnly = (($ExpectedMode -eq 39) -or ($ExpectedMode -eq 40))
        $liveHitOnly = (($ExpectedMode -eq 53) -or ($ExpectedMode -eq 54) -or ($ExpectedMode -eq 161) -or ($ExpectedMode -eq 162))
        $naturalMaskOk = if ($BattlePlayable) { (($nat[2] -band 0x6ffff) -eq 0x6ffff) } elseif ($movementOnly) { (($nat[2] -band 0x7fff) -eq 0x7fff) } elseif ($liveHitOnly) { (($nat[2] -band 0x3fdff) -eq 0x3fdff) } else { (($nat[2] -band 0xfffff) -eq 0xfffff) }
        $naturalAttackDamageOk = if ($BattlePlayable) { ($na[6] -gt 0 -and $na[7] -gt 0) } else { ($na[6] -gt 0 -and $na[7] -gt 0 -and $na[9] -gt $na[8]) }
        Assert-Condition ($natural.Success -and $nat[0] -eq 0x464e4d50 -and $nat[1] -eq 0x464e4d53 -and $naturalMaskOk -and $nat[3] -eq 1 -and $nat[4] -gt 0 -and $nat[5] -gt 0 -and $nat[6] -gt 0 -and $nat[7] -gt 0 -and (($nat[8] -band 0x3) -eq 0x3) -and $nat[10] -eq 0) 'Natural-motion manager runtime proof failed.' $gdbStdout
        Assert-Condition ($naturalFig.Success -and $nfig[0] -gt 0 -and $nfig[2] -eq 0 -and $nfig[3] -eq 0) 'Natural-motion figatree attach proof failed.' $gdbStdout
        Assert-Condition ($naturalWait.Success -and $nw[0] -ge 300 -and $nw[1] -ge 300 -and $nw[2] -gt 0 -and $nw[3] -gt 0 -and $nw[4] -ge 300 -and $nw[5] -ge 300) 'Natural-motion Wait animation proof failed.' $gdbStdout
        Assert-Condition ($naturalWalk.Success -and $nwalk[0] -gt 0 -and $nwalk[1] -ge 8 -and $nwalk[2] -ge 8 -and $nwalk[7] -gt 0 -and $nwalk[8] -gt 0 -and $nwalk[9] -gt 0 -and $nwalk[10] -gt 0) 'Natural-motion Walk transition proof failed.' $gdbStdout
        # Phase 14 == Done; phase 19 == battle_playable Done after KO/Rebirth.
        $expectedNaturalPhase = if ($BattlePlayable) { 19 } else { 14 }
        $naturalPhaseOk = if ($liveHitOnly) { ($nc[0] -ge 11) } else { ($nc[0] -eq $expectedNaturalPhase) }
        Assert-Condition ($naturalChain.Success -and $naturalPhaseOk -and $nc[2] -eq 0 -and $nc[3] -ge 2 -and $nc[4] -ge 2 -and $nc[5] -ge 8 -and $nc[6] -ge 8 -and $nc[7] -ge 2 -and $nc[8] -ge 2 -and $nc[9] -ge 1 -and $nc[10] -ge 1) 'Natural combat movement chain (dash/run/brake/turn) proof failed.' $gdbStdout
        if (-not $movementOnly) {
            $naturalRecoveryOk = if ($liveHitOnly) { $true } else { ($na[11] -gt 0) }
            Assert-Condition ($naturalAttack.Success -and $na[2] -gt 0 -and $na[4] -gt 0 -and $naturalAttackDamageOk -and $na[10] -gt 0 -and $naturalRecoveryOk) 'Natural attack->hit->damage lifecycle proof failed.' $gdbStdout
            Assert-Condition ($naturalHitlag.Success -and $nh[0] -gt 0 -and $nh[1] -gt 0) 'Natural hitlag proof failed on attacker/victim.' $gdbStdout
            if (-not $liveHitOnly) {
                Assert-Condition ($naturalGuard.Success -and $ng[0] -gt 0 -and $ng[1] -ge 10 -and $ng[2] -gt 0) 'Natural guard on/hold/off proof failed.' $gdbStdout
            }
        }
        $projectileSummary = ''
        if ($ImportBattleShipMarioFireball -or $ImportBattleShipFoxBlaster) {
            $pj = Get-Ints $projectile
            $expectedKind = if ($ImportBattleShipFoxReflector) { 0 } elseif ($ImportBattleShipFoxBlaster) { 1 } else { 0 }
            $projectileObserved = ($pj[14] -ge 3) -or ($pj[13] -gt 0)
            Assert-Condition ($projectile.Success -and $pj[0] -eq 0x50524f4a -and (($pj[1] -band 0x3f) -eq 0x3f) -and $pj[4] -gt 0 -and $pj[5] -gt 0 -and $projectileObserved -and $pj[15] -gt 0 -and (($pj[16] -band (1 -shl $expectedKind)) -ne 0) -and $pj[17] -ne 0 -and $pj[18] -gt 0) 'Natural projectile special proof failed.' $gdbStdout
            $projectileSummary = " projectile=actor$($pj[2])/kind$($pj[3]) b=$($pj[4]) status=$($pj[5]) accessory=$($pj[7]) flag0=$($pj[8]) spawn=$($pj[9]) ok=$($pj[10]) destroy=$($pj[11])/$($pj[12])/$($pj[13]) weaponFrames=$($pj[14]) max=$($pj[15]) kindMask=0x$('{0:x}' -f $pj[16]) attackMask=0x$('{0:x}' -f $pj[17]) dmg=$($pj[18]) life=$($pj[19]) map=0x$('{0:x}' -f $pj[20])"
        }
        if ($ImportBattleShipFoxReflector) {
            $rf = Get-Ints $reflector
            Assert-Condition ($reflector.Success -and $rf[0] -eq 0x52464c43 -and (($rf[1] -band 0xff) -eq 0xff) -and $rf[4] -gt 0 -and $rf[5] -gt 0 -and $rf[6] -gt 0 -and $rf[7] -gt 0 -and $rf[8] -gt 0 -and $rf[9] -ne 0 -and $rf[10] -gt 0 -and $rf[11] -gt 0 -and $rf[12] -gt 0 -and (($rf[13] -lt 0 -and $rf[14] -gt 0) -or ($rf[13] -gt 0 -and $rf[14] -lt 0)) -and $rf[15] -eq 1 -and $rf[16] -eq 1 -and $rf[19] -gt 0 -and $rf[20] -gt 0 -and $rf[21] -gt 0 -and $rf[24] -gt 0) 'Natural Fox reflector projectile proof failed.' $gdbStdout
            $projectileSummary += " reflector=0x$('{0:x}' -f $rf[1]) fox$($rf[2]) proj$($rf[3]) shine=$($rf[5])/$($rf[6])/$($rf[7]) reflect=$($rf[8]) lr=$($rf[9]) clear=$($rf[10]) proc=$($rf[12]) vx=$($rf[13])->$($rf[14]) owner=$($rf[15]) attrs=ref$($rf[16])/abs$($rf[17])/shield$($rf[18])/count$($rf[19])/dmg$($rf[20])/size$($rf[21]) delta=$($rf[22])/$($rf[23]) special=$($rf[24])/$($rf[25])"
        }
        $specialsSummary = ''
        if ($ImportBattleShipMarioSpecialHi -or $ImportBattleShipMarioSpecialLw -or $ImportBattleShipFoxSpecialHi) {
            $sp = Get-Ints $specials
            $expectedSpecialMask = 0
            if ($ImportBattleShipMarioSpecialHi) { $expectedSpecialMask = $expectedSpecialMask -bor 0x000f }
            if ($ImportBattleShipMarioSpecialLw) { $expectedSpecialMask = $expectedSpecialMask -bor 0x0070 }
            if ($ImportBattleShipFoxSpecialHi) { $expectedSpecialMask = $expectedSpecialMask -bor 0x0f80 }
            Assert-Condition ($specials.Success -and (($sp[0] -band $expectedSpecialMask) -eq $expectedSpecialMask) -and $sp[1] -eq 7) 'Natural remaining-specials proof failed.' $gdbStdout
            if ($ImportBattleShipMarioSpecialHi) {
                Assert-Condition ($sp[5] -gt 0 -and $sp[6] -gt 0 -and $sp[10] -ge 10 -and $sp[11] -gt 1000) 'Natural Mario Super Jump Punch status/launch/fall-special proof failed.' $gdbStdout
            }
            if ($ImportBattleShipMarioSpecialLw) {
                Assert-Condition ($sp[12] -gt 0 -and (($sp[13] -gt 0) -or ($sp[14] -gt 0)) -and $sp[15] -gt 0 -and $sp[16] -ge 10) 'Natural Mario Tornado status/effect/settle proof failed.' $gdbStdout
            }
            if ($ImportBattleShipFoxSpecialHi) {
                Assert-Condition ($sp[17] -gt 0 -and $sp[18] -gt 0 -and $sp[19] -gt 0 -and $sp[20] -gt 0 -and (($sp[21] -gt 0) -or ($sp[22] -gt 0)) -and $sp[23] -ge 10) 'Natural Fox Fire Fox status ladder proof failed.' $gdbStdout
            }
            $specialsSummary = " specials=0x$('{0:x}' -f $sp[0]) phase=$($sp[1]) mhi=$($sp[5])/$($sp[6])/$($sp[8])/$($sp[9])/$($sp[10]) y=$($sp[11]) mlw=$($sp[12])/$($sp[13])/$($sp[14]) dust=$($sp[15]) wait=$($sp[16]) foxhi=$($sp[17])/$($sp[18])/$($sp[19])/$($sp[20])/$($sp[21])/$($sp[22])/$($sp[23]) y=$($sp[24])"
        }
        $audioSummary = ''
        $audioBgmSummary = ''
        $audioBgmResidentBytes = 0
        if ($ImportBattleShipAudioAssets) {
            $aa = Get-Ints $audioAsset
            Assert-Condition ($audioAsset.Success -and $aa[0] -eq 0x41554431 -and $aa[1] -eq 0xff -and $aa[2] -eq 8 -and $aa[3] -eq 0 -and $aa[4] -eq 0 -and $aa[5] -eq 0 -and $aa[6] -eq 4422960 -and $aa[7] -eq 0 -and $aa[8] -le 65536 -and $aa[9] -eq 47 -and $aa[10] -eq 380 -and $aa[11] -eq 7999 -and $aa[12] -gt 0 -and $aa[13] -eq 1 -and $aa[14] -eq 42 -and $aa[15] -eq 117 -and $aa[16] -eq 32000 -and $aa[17] -eq 1 -and $aa[18] -eq 1 -and $aa[19] -eq 322 -and $aa[20] -eq 44100 -and $aa[21] -eq 100 -and $aa[22] -eq 464 -and $aa[23] -eq 695) 'Original audio asset parse-only proof failed.' $gdbStdout
            $audioSummary = " audio=seq$($aa[9]) bank1=$($aa[13])/$($aa[14])/$($aa[15])@$($aa[16]) bank2=$($aa[17])/$($aa[18])/$($aa[19])@$($aa[20]) fgm=$($aa[21])/$($aa[22])/$($aa[23]) raw=$($aa[6]) resident=$($aa[7]) scratch=$($aa[8])"
        }
        if ($ImportBattleShipAudioBGM) {
            $ab = Get-Ints $audioBgm
            $audioBgmResidentBytes = $ab[13]
            Assert-Condition ($audioBgm.Success -and $ab[0] -eq 0x42474d31 -and (($ab[1] -band 0x3) -eq 0x3) -and $ab[2] -eq 0 -and $ab[3] -eq 0 -and $ab[4] -eq 0x7800 -and $ab[5] -ge 1 -and $ab[6] -ge 1 -and $ab[9] -eq 0 -and $ab[10] -eq 0 -and $ab[11] -eq 0 -and $ab[12] -gt 65536 -and $ab[13] -eq 65536 -and $ab[14] -eq 32768 -and $ab[15] -ge 1 -and $ab[16] -eq 1 -and $ab[17] -ge 3200 -and $ab[19] -ge 42100 -and $ab[19] -le 46100 -and $ab[20] -eq 44100 -and $ab[22] -ge 4 -and $ab[23] -lt 65536 -and (($ab[24] -eq 0) -or ($ab[24] -eq 32768)) -and (($ab[25] -eq 0) -or ($ab[25] -eq 1)) -and (($ab[26] -eq 0) -or ($ab[26] -eq 1)) -and $ab[25] -ne $ab[26] -and $ab[27] -eq 0 -and $ab[28] -gt 0 -and $ab[29] -gt 0 -and $ab[31] -eq 0) 'Minimal BGM backend proof failed natural start/stop or hardware-timer 44100 B/s stream-rate guard.' $gdbStdout
            $audioBgmSummary = " bgm=track$($ab[3]) play=$($ab[5]) stop=$($ab[6]) refills=$($ab[22]) read=$($ab[12]) rate=$($ab[19]) loop=$($ab[21]) hwloop=$($ab[30]) resident=$($ab[13])"
        }
        $movesetSummary = ''
        if ($ImportBattleShipNormalMoveset) {
            Assert-Condition ($naturalMoveset.Success -and (($nm[0] -band 0x7ff) -eq 0x7ff) -and $nm[1] -eq 15 -and $nm[3] -gt 0 -and $nm[4] -gt 0 -and $nm[5] -gt 0 -and $nm[6] -gt 0 -and $nm[7] -gt 0 -and $nm[8] -gt 0 -and $nm[9] -gt 0 -and $nm[11] -gt 0 -and $nm[12] -gt 0 -and $nm[13] -gt 0 -and $nm[14] -gt 0 -and $nm[15] -gt 0 -and $nm[16] -ge 10 -and $nm[26] -gt $nm[25]) 'Natural normal-moveset tilt/smash/aerial/grab/throw proof failed.' $gdbStdout
            $movesetSummary = " moveset=0x$('{0:x}' -f $nm[0]) phase=$($nm[1]) tilt=$($nm[3])/$($nm[4])/$($nm[5]) smash=$($nm[7]) aerial=$($nm[9]) landing=$($nm[11]) grab=$($nm[12])/$($nm[13]) throw=$($nm[14])/$($nm[15])/$($nm[16]) throwDmg=$($nm[25])->$($nm[26])"
        }
        $battlePlayableSummary = ''
        if ($BattlePlayable) {
            $bpk = Get-Ints $battlePlayableKO
            $bps = Get-Ints $battlePlayableStatus
            $ma = Get-Ints $memoryArena
            $mr = Get-Ints $memoryReloc
            $me = Get-Ints $memoryEvict
            $bp = Get-Ints $battlePlayablePacing
            $victimStockDelta = $bpk[3] - $bpk[4]
            $battleStockDelta = $bpk[5] - $bpk[6]
            $fallsDelta = $bpk[8] - $bpk[7]
            Assert-Condition ($battlePlayablePacing.Success -and $bp[0] -eq 0x42505443 -and $bp[1] -eq 1 -and $bp[2] -ge 3200 -and $bp[3] -eq 0 -and $bp[4] -eq 0 -and $bp[5] -gt 0) 'battle_playable fast-logic pacing marker did not prove explicit unthrottled verification mode.' $gdbStdout
            Assert-Condition ($battlePlayableKO.Success -and $bpk[0] -eq 0x42504c59 -and (($bpk[1] -band 0xff) -eq 0xff) -and $victimStockDelta -gt 0 -and $victimStockDelta -eq $battleStockDelta -and $victimStockDelta -eq $fallsDelta) 'battle_playable stock/fall KO proof failed.' $gdbStdout
            Assert-Condition ($battlePlayableStatus.Success -and $bps[0] -gt 0 -and $bps[1] -gt 0 -and $bps[2] -gt 0 -and $bps[3] -gt 0 -and $bps[5] -ge 8 -and $bps[6] -eq 10 -and $bps[7] -eq 0 -and $bps[8] -eq 0 -and $bps[9] -gt 0) 'battle_playable Dead/Rebirth/return-control proof failed.' $gdbStdout
            $interfaceBytesOk = if ($ImportBattleShipIFCommon) { $mr[4] -gt 0 } else { $true }
            Assert-Condition ($memoryArena.Success -and $ma[0] -eq 0x4d4c4544 -and $ma[1] -eq 22 -and $ma[3] -ge 0x130000 -and ($ma[6] - $audioBgmResidentBytes) -ge 131072 -and $ma[7] -eq 163840 -and $ma[8] -eq 106496 -and $ma[9] -eq 49152 -and $ma[10] -gt 0) 'battle_playable memory arena ledger failed reserve or VSBattle taskman buffer assertions.' $gdbStdout
            Assert-Condition ($memoryReloc.Success -and $mr[0] -gt 0 -and $mr[1] -gt 0 -and $mr[2] -gt 0 -and $mr[3] -gt 0 -and $interfaceBytesOk -and $mr[5] -eq 0 -and $mr[6] -eq 0 -and $mr[8] -eq 0 -and $mr[9] -eq 0) 'battle_playable reloc memory ledger found stale or missing resident groups.' $gdbStdout
            Assert-Condition ($memoryEvict.Success) 'battle_playable reloc eviction ledger was not printed.' $gdbStdout
            $battlePlayableSummary = " bplay=stock$($bpk[3])->$($bpk[4]) falls$($bpk[7])->$($bpk[8]) dead=$($bps[0]) rebirth=$($bps[1])/$($bps[2])/$($bps[3]) recover=$($bps[4])/$($bps[5])"
            $battlePlayableSummary += " pace=fast logic$($bp[2]) draw$($bp[4])"
            $battlePlayableSummary += " mem=head$($ma[6]) reloc$($mr[1]) stage$($mr[2]) fighter$($mr[3]) if$($mr[4]) stale$($mr[8])/$($mr[9]) evict$($me[0])/$($me[1])"
            $battlePlayableSummary += $movesetSummary
            $battlePlayableSummary += $specialsSummary
            $battlePlayableSummary += $audioSummary
            $battlePlayableSummary += $audioBgmSummary
            if ($ImportBattleShipIFCommon) {
                $ih = Get-Ints $ifHud
                $victimSlot = [int]$bpk[2]
                $damageMax = @($ih[4], $ih[5])
                $digitCounts = @($ih[6], $ih[7])
                $digitPacks = @($ih[8], $ih[9])
                $stockCurrent = @($ih[10], $ih[11])
                $stockMin = @($ih[12], $ih[13])
                $stockMax = @($ih[14], $ih[15])
                $stockDelta = $stockMax[$victimSlot] - $stockMin[$victimSlot]
                $stockCurrentMatchesFinal = ($stockCurrent[$victimSlot] -eq $bpk[4]) -or ($stockCurrent[$victimSlot] -eq ($bpk[4] + 1))
                Assert-Condition ($ifHud.Success -and $ih[0] -gt 0 -and (($ih[1] -band 0x33) -eq 0x33)) 'Imported IFCommon HUD objects were not observed for both players.' $gdbStdout
                Assert-Condition ((Test-DamageDigits -Damage $damageMax[$victimSlot] -DigitCount $digitCounts[$victimSlot] -DigitsPack $digitPacks[$victimSlot])) 'Imported IFCommon percent digit images did not match the victim damage value.' $gdbStdout
                Assert-Condition ($stockMax[$victimSlot] -ge $bpk[3] -and $stockDelta -ge $victimStockDelta -and $stockCurrentMatchesFinal) 'Imported IFCommon stock icon display did not decrement after KO.' $gdbStdout
                $battlePlayableSummary += " hud=dmg$($damageMax[$victimSlot])/digits0x$('{0:x}' -f $digitPacks[$victimSlot]) stock$($stockMax[$victimSlot])->$($stockMin[$victimSlot])"
            }
        }
        if ($HardwareTriangles) {
            $hw = Get-Ints $platformHw
            $ro = Get-Ints $renderOracle
            $rrm = Get-Ints $renderRawMatrix
            $rs = Get-Ints $renderSubmit
            $rhdiv = Get-Ints $renderHardwareDivide
            $rlazy = Get-Ints $renderLazy
            $rp = Get-Ints $renderProfile
            Assert-Condition ($platformHw.Success -and $hw[0] -gt 0 -and $hw[0] -eq $hw[1]) 'Boundary hardware draw did not flush submitted DS 3D frames.' $gdbStdout
            $shw = Get-Ints $stageHardware
            Assert-Condition ($stageHardware.Success -and $shw[0] -gt 8) 'Boundary hardware replay did not exceed the old bounded DObj submit slice.' $gdbStdout
            Assert-Condition ($shw[1] -gt 0) 'Boundary hardware replay did not submit hardware triangles.' $gdbStdout
            Assert-Condition ($shw[1] -eq ($shw[2] + $shw[3])) 'Boundary hardware depth accounting does not match submitted triangles.' $gdbStdout
            Assert-Condition ($shw[3] -gt 0) 'Boundary hardware replay did not preserve source no-z projected-depth submission.' $gdbStdout
            Assert-Condition ($shw[5] -gt 0 -and $shw[6] -gt 0 -and $shw[7] -gt 0 -and $shw[9] -ne 0 -and $shw[10] -gt 0 -and $shw[11] -gt 0) 'Boundary hardware replay did not bind/upload a ready texture.' $gdbStdout
            Assert-Condition ($shw[8] -eq 0) 'Boundary hardware replay still rejects source-loaded stage texture state.' $gdbStdout
            $shwf = Get-Ints $stageHardwareFighter
            Assert-Condition ($stageHardwareFighter.Success -and $shwf[0] -eq 2 -and $shwf[1] -gt 0) 'Boundary hardware replay did not submit selected fighter triangles.' $gdbStdout
            Assert-Condition ($renderOracle.Success -and $ro[0] -gt 0 -and $ro[1] -eq 0 -and $ro[2] -eq 0) 'Boundary hardware submitted vertex positions drifted from the CPU 20.12 oracle.' $gdbStdout
            Assert-Condition ($renderRawMatrix.Success -and $rrm[0] -gt 0 -and $rrm[3] -gt 0 -and $rrm[4] -eq 0 -and $rrm[5] -le 16 -and $rrm[6] -eq 0 -and $rrm[7] -eq 0 -and $rrm[8] -gt 0 -and $rrm[9] -eq 0) 'Boundary corrected composed GX matrix PosTest failed natural-frame coverage or correctness.' $gdbStdout
            $submitTotal = $rs[0] + $rs[1] + $rs[2] + $rs[3] + $rs[4] + $rs[5] + $rs[6]
            $expectedProjectedDivisions = (6 * $rs[3]) + (9 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
            Assert-Condition ($renderSubmit.Success -and $renderProfile.Success -and $rs[0] -eq $rrm[0] -and $rs[0] -gt 0 -and $rs[2] -eq $rrm[2] -and $rs[3] -gt 0 -and $rs[6] -eq $rrm[1] -and $rs[7] -eq 0 -and $submitTotal -eq $rp[16] -and $rs[8] -eq $expectedProjectedDivisions) 'Boundary hybrid raw/projected class or projected-division accounting drifted.' $gdbStdout
            $expectedHardwareDivideEvaluations = $rs[8] + (3 * ($rs[2] + $rs[4] + $rs[5] + $rs[6]))
            Assert-Condition ($renderHardwareDivide.Success -and $rhdiv[0] -gt 0 -and ($rhdiv[0] + $rhdiv[1] + $rhdiv[2]) -eq $expectedHardwareDivideEvaluations -and $rhdiv[3] -eq 0 -and $rhdiv[4] -eq 0) 'Boundary forensic hardware-divider coverage or exact-result oracle drifted.' $gdbStdout
            Assert-Condition ($renderLazy.Success -and $rlazy[0] -gt 0 -and $rlazy[1] -eq $rlazy[0] -and $rlazy[2] -gt 0 -and $rlazy[3] -gt 0 -and $rlazy[4] -gt 0 -and $rlazy[5] -eq 0) 'Boundary forensic snapshot/lazy-transform accounting drifted or overflowed.' $gdbStdout
            $hardwareSummary = " hwflush=$($hw[0])/$($hw[1]) hwsubmit=$($shw[0]) hwtri=$($shw[1]) hwdepth=z$($shw[2])/proj$($shw[3])/decal$($shw[4]) hwtex=bind$($shw[5])/upload$($shw[6])/ready$($shw[7])/reject$($shw[8])/fmt$($shw[9])/max$($shw[10])x$($shw[11]) hwftr=$($shwf[0])/$($shwf[1]) oracle=$($ro[0])/$($ro[1])/$($ro[2]) submit=raw$($rs[0])/snap$($rs[1])/cross$($rs[2])/noz$($rs[3])/range$($rs[6])/rej$($rs[7])/div$($rs[8]) hdiv=$($rhdiv[0])/$($rhdiv[1])/$($rhdiv[2])/z$($rhdiv[3])/mis$($rhdiv[4]) lazy=load$($rlazy[0])/xf$($rlazy[1])/hit$($rlazy[2])/new$($rlazy[3])/reuse$($rlazy[4])/ovf$($rlazy[5]) rawcand=$($rrm[0])/$($rrm[1])/$($rrm[2]) postest=$($rrm[3])/$($rrm[4])/e$($rrm[5])/mw$($rrm[8])"
        }
        Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after natural-motion proof.' $gdbStdout
        Write-Output ("$Label ftmanager natural-combat harness passed: wait={0}/{1} walk={2}/{3} dash={4}/{5} run={6}/{7} attack={8} hitbox={9} dmg={10}->{11} status={12} knock={13} guard={14}/{15}/{16} retries={17} updates={18} mask=0x{19:x}{20}{21}{22}{23}" -f $nw[0], $nw[1], $nwalk[1], $nwalk[2], $nc[3], $nc[4], $nc[5], $nc[6], $na[2], $na[4], $na[8], $na[9], $na[6], $na[10], $ng[0], $ng[1], $ng[2], $na[5], $nat[4], $nat[2], $projectileSummary, $battlePlayableSummary, $hardwareSummary, $aobjSummary)
        return
    }
    $prv = Get-Ints $prev
    Assert-Condition ($prev.Success -and $prv[0] -eq 0x46504c56 -and $prv[1] -eq 0x46505653 -and (($prv[2] -band 0x1fff) -eq 0x1fff) -and $prv[3] -eq 0xff -and $prv[4] -eq 2) 'Prerequisite preview-loop proof did not pass.' $gdbStdout
    $lp = Get-Ints $loop
    Assert-Condition ($loop.Success -and $lp[0] -eq 0x4647414c -and $lp[1] -eq 0x46474153 -and (($lp[2] -band 0x1fff) -eq 0x1fff) -and $lp[3] -eq 0xff -and $lp[4] -eq 2 -and $lp[5] -ge 160 -and $lp[6] -ge 220) 'gcRunAll-loop result/mask did not pass.' $gdbStdout
    $tm = Get-Ints $taskman
    Assert-Condition ($tm[0] -eq 1 -and $tm[1] -gt 0 -and $tm[2] -gt 0 -and $tm[3] -eq $tm[2] -and $tm[4] -gt 0 -and $tm[5] -ge $tm[1]) 'gcRunAll-loop taskman path failed.' $gdbStdout
    $rn = Get-Ints $run
    Assert-Condition ($rn[0] -gt 0 -and $rn[1] -gt 0 -and $rn[3] -ge 2 -and $rn[4] -eq $rn[5]) 'gcRunAll pause/preserve/GObj stability setup failed.' $gdbStdout
    $pc = Get-Ints $process
    Assert-Condition ($pc[0] -eq 1 -and $pc[1] -eq 1 -and $pc[2] -gt 0 -and $pc[3] -gt 0 -and $pc[4] -eq $pc[2] -and $pc[5] -eq $pc[3] -and $pc[6] -eq 0) 'gcRunAll process attach/callback path failed.' $gdbStdout
    $inp = Get-Ints $input
    Assert-Condition ($inp[0] -gt 0 -and $inp[1] -gt 0 -and $inp[2] -gt 0 -and $inp[3] -gt 0 -and $inp[4] -eq 0 -and $inp[5] -eq 0 -and $inp[6] -gt 0 -and $inp[7] -gt 0 -and $inp[8] -ne 0 -and $inp[9] -ne 0 -and $inp[10] -ne 0 -and $inp[11] -ne 0 -and $inp[12] -gt 0 -and $inp[13] -gt 0) 'gcRunAll input bridge failed or wrote FTStruct directly.' $gdbStdout
    $st = Get-Ints $status
    Assert-Condition (($st[0..9] -join ',') -eq '10,10,4,4,10,10,4,4,0,0') 'gcRunAll start/final state was not Wait/Ground.' $gdbStdout
    Assert-Condition ((($st[10] -band 0x3ff) -eq 0x3ff) -and (($st[11] -band 0x3ff) -eq 0x3ff) -and (($st[12] -band 0x7ff) -eq 0x7ff) -and (($st[13] -band 0x7ff) -eq 0x7ff)) 'gcRunAll status/transition masks were incomplete.' $gdbStdout
    $vi = Get-Ints $visits
    Assert-Condition ((@($vi | Where-Object { $_ -le 0 }).Count -eq 0)) 'gcRunAll state visit counters were incomplete.' $gdbStdout
    $ca = Get-Ints $calls
    Assert-Condition ($ca[2] -eq 1 -and $ca[3] -eq 1 -and $ca[0] -gt 0 -and $ca[1] -gt 0 -and $ca[0] -le 180 -and $ca[1] -le 180 -and (@($ca[4..9] | Where-Object { $_ -le 0 }).Count -eq 0)) 'gcRunAll frame callback counters failed.' $gdbStdout
    $dr = Get-Ints $draw
    Assert-Condition ($dr[0] -eq 96 -and $dr[1] -eq 72 -and $dr[2] -ge 96 -and $dr[3] -eq 1 -and $dr[4] -ge 7 -and $dr[5] -ge 7 -and $dr[6] -ge 14 -and $dr[7] -ge 7 -and $dr[8] -ge 7 -and $dr[9] -ge 14 -and $dr[10] -ge 18 -and $dr[11] -ge 14 -and $dr[12] -ge 18 -and $dr[13] -gt 0 -and $dr[14] -ne 0 -and $dr[15] -ne 0) 'gcRunAll draw/DObj/pixel markers failed.' $gdbStdout
    $sc = Get-Ints $screen
    Assert-Condition ($sc[4] -ne 0 -and $sc[5] -ne 0 -and $sc[8] -gt 0 -and $sc[9] -gt 0) 'gcRunAll screen movement markers failed.' $gdbStdout
    $mv = Get-Ints $move
    Assert-Condition ($mv[2] -ne 0 -and $mv[3] -ne 0 -and $mv[4] -gt 0 -and $mv[5] -gt 0 -and $mv[6] -eq $mv[0] -and $mv[7] -eq $mv[1] -and $mv[8] -eq 1 -and $mv[9] -eq 1 -and $mv[10] -eq 1 -and $mv[11] -eq 1) 'gcRunAll movement/floor markers failed.' $gdbStdout
    $tr = Get-Ints $trans
    Assert-Condition ($tr[0] -ge 2 -and $tr[1] -ge 2 -and $tr[2] -ge 2 -and $tr[3] -ge 2 -and $tr[4] -ge 4 -and $tr[5] -ge 2 -and $tr[6] -ge 2 -and $tr[7] -ge 2 -and $tr[8] -gt 0) 'gcRunAll transition counters failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition ((@($sf[0..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'gcRunAll safe escape counters were not zero.' $gdbStdout
    $pd = Get-Ints $platform
    Assert-Condition ($pd[0] -eq 1 -and $pd[1] -eq 96 -and $pd[2] -eq 72 -and $pd[3] -ge $dr[4]) 'Platform original-DL preview markers failed.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after gcRunAll proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 gcRunAll={0} callbacks={1}/{2} draws={3} pixels={4} visits=0x{5:x}/0x{6:x} transitions=0x{7:x}/0x{8:x} screen-dx={9}/{10} screen-rise={11}/{12} final=Wait/Ground safe=1{13}" -f $tm[4], $pc[4], $pc[5], $dr[5], $dr[13], $st[10], $st[11], $st[12], $st[13], $sc[4], $sc[5], $sc[8], $sc[9], $aobjSummary)
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
}
