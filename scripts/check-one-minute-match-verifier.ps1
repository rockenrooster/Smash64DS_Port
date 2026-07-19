$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$wrapperPath = Join-Path $PSScriptRoot 'verify-battle-playable-one-minute-match.ps1'
$lifecyclePath = Join-Path $PSScriptRoot 'verify-battle-playable-match-lifecycle-harness.ps1'
$battlePath = Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1'
$ownerPath = Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1'
$scenePath = Join-Path $root 'src\port\scene_harness.c'
$taskmanPath = Join-Path $root 'src\port\taskman_seam.c'
$melonPath = Join-Path $PSScriptRoot 'lib\melonds.ps1'
$gdbPath = Join-Path $PSScriptRoot 'lib\gdb-markers.ps1'
$makePath = Join-Path $root 'Makefile'

function Assert-Text {
    param(
        [Parameter(Mandatory=$true)][string]$Text,
        [Parameter(Mandatory=$true)][string]$Pattern,
        [Parameter(Mandatory=$true)][string]$Message
    )
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

foreach ($path in @($wrapperPath, $lifecyclePath, $battlePath, $ownerPath, $melonPath, $gdbPath)) {
    $tokens = $null
    $errors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile(
        $path, [ref]$tokens, [ref]$errors)
    if ($errors.Count -ne 0) {
        throw "PowerShell parser rejected $path`n$($errors | Out-String)"
    }
}

$wrapper = Get-Content -LiteralPath $wrapperPath -Raw
$lifecycle = Get-Content -LiteralPath $lifecyclePath -Raw
$battle = Get-Content -LiteralPath $battlePath -Raw
$owner = Get-Content -LiteralPath $ownerPath -Raw
$scene = Get-Content -LiteralPath $scenePath -Raw
$taskman = Get-Content -LiteralPath $taskmanPath -Raw
$melon = Get-Content -LiteralPath $melonPath -Raw
$gdb = Get-Content -LiteralPath $gdbPath -Raw
$make = Get-Content -LiteralPath $makePath -Raw

Assert-Text $taskman '#define NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT 2u' `
    'Realtime scheduler no longer declares exactly two source updates per present.'
Assert-Text $taskman 'updates_this_iteration =\s*NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;' `
    'Realtime scheduler no longer executes the exact fixed two-update batch.'
Assert-Text $taskman '(?s)for \(update_in_iteration = 0u;.*?update_in_iteration < updates_this_iteration;.*?ndsPlatformReadInput\(\);.*?ndsBattlePlayableAdvanceRealtimeLogicClock\(\);.*?ndsRunMarioFoxProofUpdate\(.*?\);.*?if \(terminal_update != 0u\).*?ndsBattlePlayablePresentRealtimeFrame\(\);' `
    'Realtime scheduler no longer performs input + one source tick + one source update twice before the sole draw/present.'
Assert-Text $taskman 'gNdsBattlePlayablePacingLogicFrames \+=\s*NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;' `
    'Pacing telemetry no longer commits exactly two updates at the presentation boundary.'
if ($taskman -match 'updates_owed|RealtimeUpdatesOwed|REALTIME_UPDATE_CAP|PacingDroppedUpdates|PacingUpdateCapHit') {
    throw 'Realtime scheduler regained forbidden vblank-debt or catch-up accounting.'
}

Assert-Text $wrapper '-CPUOpponentProof\s+`\s*\r?\n\s*-MatchLifecycleProof\s+`\s*\r?\n\s*-OneMinuteMatchProof' `
    'One-minute wrapper does not select the existing CPU/lifecycle mode-163 path.'
Assert-Text $wrapper '(?s)-OneMinuteMatchProof\s+`.*?-RendererFastRunMode 9\s+`.*?-NativeStageGeneratedSegment0Enable 1\s+`.*?-StaticTextureAotMode 1\s+`.*?-IFCommonHybridOamMode 0\s+`.*?-RequireZeroPostGoTextureFence' `
    'One-minute wrapper does not select the published-equivalent M3/M4 renderer and strict post-GO fence.'
Assert-Text $owner '(?s)if \(\$MatchLifecycleProof\) \{\s*\$CPUOpponentProof = \$true.*?\}\s*if \(\$CPUOpponentProof\) \{\s*\$FoxCpuMode = 1\s*\$foxCpuModeSelected = \$true\s*\}' `
    'CPU/lifecycle proof no longer forces the Fox CPU decision path on.'
Assert-Text $owner '(?s)\$preBattleSelectorSelected =\s*\$staticTextureAotSelected -or \$foxCpuModeSelected.*?if \(\$preBattleSelectorSelected\) \{.*?''tbreak scVSBattleStartBattle''.*?if \(\$foxCpuModeSelected\) \{\s*\$preBattleSetupCommands \+=\s*\(''set variable gNdsBattlePlayableFoxCpuEnabled = \{0\}'' -f\s*\$FoxCpuMode\).*?\}.*?\$gdbCommands = @\(\s*\$gdbCommands\[0\.\.3\]\s*\$preBattleSetupCommands' `
    'Common verifier no longer sets the selected Fox CPU mode before battle setup.'
Assert-Text $wrapper '(?s)-RealtimePresentation\s+`.*?-OneMinuteMatchProof\s+`.*?-RequireLocked30Pacing' `
    'One-minute wrapper no longer selects the realtime exact-two-update scheduler gate.'
if ($wrapper -match 'RequireRealtime60Fps|capture-melonds|Screenshot|FiveMinute|five-minute|18000|unthrottled state/memory') {
    throw 'One-minute wrapper retained an obsolete 60-FPS, screenshot, five-minute, or unthrottled assumption.'
}
Assert-Text $lifecycle 'Get-MelonDSRunnerPort -RunnerSlot \$RunnerSlot -Cpu ARM9' `
    'Match-lifecycle wrapper no longer derives an isolated GDB port from RunnerSlot.'
Assert-Text $lifecycle '-MelonDS \$MelonDS' `
    'Match-lifecycle wrapper no longer forwards the selected repo-local emulator.'
Assert-Text $lifecycle '-Gdb \$Gdb' `
    'Match-lifecycle wrapper no longer forwards the selected GDB executable.'
Assert-Text $lifecycle '-GdbPort \$selectedGdbPort' `
    'Match-lifecycle wrapper no longer forwards its selected isolated GDB port.'

Assert-Text $battle '\$target = ''smash64ds-battle-playable-one-minute-match-hwtri''' `
    'One-minute verifier target is not artifact-isolated from the canonical ROM.'
Assert-Text $battle '\$build = ''build-battle-playable-one-minute-match-hwtri-harness''' `
    'One-minute verifier build directory is not isolated.'
Assert-Text $battle '(?s)if \(\$OneMinuteMatchProof -and.*?\$RendererFastRunMode -ne 9.*?\$StaticTextureAotMode -ne 1.*?\$IFCommonHybridOamMode -ne 0.*?-not \$RequireZeroPostGoTextureFence' `
    'One-minute verifier no longer rejects a non-published-equivalent M3/M4 configuration.'
Assert-Text $battle "(?s)NDS_RENDERER_FAST_RUN_DEFAULT = '9'.*NDS_SCENE_MIP_CACHE_LAB = '0'.*NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT = '1'.*NDS_DEBUG_HUD = '0'" `
    'One-minute verifier no longer supplies the isolated target exact 9/0/1 release defaults.'
Assert-Text $battle '-HardwareTriangles:\$hardwareTriangles' `
    'One-minute verifier no longer forwards hardware rendering from its hwtri target.'
Assert-Text $battle '\$RendererProfileLevel = if \(\$OneMinuteMatchProof\) \{ 0 \} else \{ 2 \}' `
    'One-minute verifier no longer disables verifier-only renderer profiling.'
Assert-Text $battle '-OneMinuteMatchProof:\$OneMinuteMatchProof' `
    'One-minute selector is not forwarded to the natural-runtime owner.'
Assert-Text $battle '-StaticTextureAotMode \$StaticTextureAotMode' `
    'One-minute verifier no longer forwards static texture residency.'
Assert-Text $battle '-NativeStageGeneratedSegment0Enable \$NativeStageGeneratedSegment0Enable' `
    'One-minute verifier no longer forwards the generated stage-program selector.'
Assert-Text $battle '-IFCommonHybridOamMode \$IFCommonHybridOamMode' `
    'One-minute verifier no longer forwards published IFCommon OAM ownership.'
Assert-Text $battle '-RequireZeroPostGoTextureFence:\$RequireZeroPostGoTextureFence' `
    'One-minute verifier no longer forwards the strict M4 texture fence.'

Assert-Text $scene 'gSCManagerTransferBattleState\.game_rules = SCBATTLE_GAMERULE_TIME;\s*gSCManagerTransferBattleState\.time_limit = 1;' `
    'Canonical mode 163 is not configured for the one-minute timed rule.'
if (($scene -match 'NDS_DEV_RESULTS_VISUAL_SMOKE') -or ($make -match 'NDS_DEV_RESULTS_VISUAL_SMOKE')) {
    throw 'Obsolete verifier-only timer-rule switch still exists.'
}
Assert-Text $owner 'tbreak scVSBattleFuncUpdate if gSCManagerBattleState->time_limit == 1 && gSCManagerBattleState->time_remain == 3600 && gSCManagerBattleState->time_passed == 0' `
    'One-minute verifier lost the exact 1:00 start synchronization gate.'
Assert-Text $owner 'MATCH_START=' `
    'One-minute verifier lost the synchronized start-state marker.'
Assert-Text $owner '(?s)\$life\[4\] -eq 1.*\$life\[5\] -eq 0.*\$life\[6\] -eq 3600' `
    'One-minute verifier lost its exact timer-expiry assertion.'
Assert-Text $owner '(?s)\$life\[8\] -eq 22.*\$life\[9\] -eq 24' `
    'One-minute verifier lost the VSBattle-to-VS-Results transition assertion.'
Assert-Text $owner '\$expectedMemoryArenaScene = if \(\$OneMinuteMatchProof\) \{ 24 \} else \{ 22 \}' `
    'One-minute verifier no longer accepts the final memory ledger from VS Results while retaining VSBattle elsewhere.'
Assert-Text $owner 'MATCH_SAFETY=' `
    'One-minute verifier lost its safety-counter marker.'
Assert-Text $owner 'AUDIO_FGM_KO=' `
    'One-minute verifier lost its natural KO FGM marker.'
Assert-Text $owner "(?s)439,292,154.*154,439,292.*370,289,154.*154,370,289" `
    'One-minute verifier lost the exact Mario/Fox regular/up-star KO call orders.'
Assert-Text $owner '(?s)\$fgmKo\[10\] -eq 0.*\$fgmKo\[11\] -eq 0.*\$fgmKo\[12\] -eq 0.*\$fgmKo\[13\] -eq 0' `
    'One-minute verifier no longer requires clean included-KO playback.'
Assert-Text $owner '(?s)\$audioFgmKoPass = .*?if \(-not \$Task25RPacingTrace\) \{\s*Assert-Condition \$audioFgmKoPass.*?exactKoFgmTriplet = \$audioFgmKoPass' `
    'Task 25R no longer reports KO-audio failure while the normal verifier remains strict.'
Assert-Text $owner '\$safety\[0\.\.15\] \| Where-Object \{ \$_ -ne 0 \}' `
    'One-minute verifier no longer requires every true safety counter to remain zero.'
Assert-Text $owner '\$fdc\[3\] -eq \$fdc\[0\]' `
    'One-minute verifier no longer requires every selected fighter part to be submitted.'
Assert-Text $owner '\(\$fdc\[7\] \+ \$fdc\[8\]\) -gt 0' `
    'One-minute verifier no longer requires source visibility-bound decisions.'
Assert-Text $owner '\$safety\[16\] -eq \$fdc\[8\]' `
    'One-minute verifier no longer cross-checks natural visibility failures.'
Assert-Text $owner '(?s)for \(\$phase = 0; \$phase -lt 5; \$phase\+\+\).*?\$phaseRateX10 -ge 590.*?\$phaseRateX10 -le 610' `
    'One-minute lifecycle path no longer hard-gates held-30 phase update rates.'
Assert-Text $owner '(?s)\$reserveBytes = \[int64\]\$ma\[6\] - \$audioResidentBytes.*?\$reservePass = \$reserveBytes -ge 131072.*?if \(-not \$Task25RPacingTrace\) \{\s*Assert-Condition \$reservePass' `
    'One-minute verifier lost the conservative 128 KiB reserve gate.'
Assert-Text $owner '\$expectedM4TeardownCount = if \(\$OneMinuteMatchProof\) \{ 1 \} else \{ 0 \}' `
    'One-minute verifier no longer requires exactly one M4 teardown.'
Assert-Text $owner '(?s)\$m4FenceFinalValues\[4\] -eq 24.*?\$m4FenceFinalValues\[5\] -eq 136192.*?\$m4FenceFinalValues\[7\] -eq \$expectedM4TeardownCount.*?\$m4FenceFinalCountSum -eq 0' `
    'One-minute verifier lost the exact M4 residency and zero post-GO work assertions.'
Assert-Text $owner '\$bp\[2\] -eq \(2 \* \$bp\[3\]\)' `
    'One-minute verifier lost the hard exact-two-updates-per-present ratio gate.'
Assert-Text $owner 'gNdsBattlePlayablePacingRestartRequested = 1' `
    'One-minute verifier no longer excludes its synchronized MATCH_START pause from natural pacing evidence.'
Assert-Text $owner '-MuteAudio:\$OneMinuteMatchProof' `
    'One-minute match no longer requests host-emulator muting.'
Assert-Text $melon "Section 'Instance0\.Audio' -Key 'Volume' -Value '0'" `
    'melonDS verifier muting no longer sets Instance0.Audio.Volume to zero.'
Assert-Text $gdb 'ReadToEndAsync\(\)' `
    'GDB capture timeout can block before cleanup.'
Assert-Text $gdb 'Kill\(\$true\)' `
    'GDB timeout no longer kills its child process tree.'
Assert-Text $owner '(?s)finally \{.*Stop-Process -Id \$emulator\.Id -Force' `
    'melonDS is not guaranteed to stop on verifier exit.'
Assert-Text $owner 'phaseUpdateRate=' `
    'One-minute verifier no longer publishes per-phase source update rates.'
if (($owner -match 'FiveMinute|five-minute|5:00') -or ($battle -match 'FiveMinute|five-minute')) {
    throw 'One-minute verifier path retains a five-minute label or selector.'
}

Write-Output "One-minute mode-163 match verifier static check passed: $root"
