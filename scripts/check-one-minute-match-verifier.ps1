$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$wrapperPath = Join-Path $PSScriptRoot 'verify-battle-playable-one-minute-match.ps1'
$lifecyclePath = Join-Path $PSScriptRoot 'verify-battle-playable-match-lifecycle-harness.ps1'
$battlePath = Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1'
$ownerPath = Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1'
$scenePath = Join-Path $root 'src\port\scene_harness.c'
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
$melon = Get-Content -LiteralPath $melonPath -Raw
$gdb = Get-Content -LiteralPath $gdbPath -Raw
$make = Get-Content -LiteralPath $makePath -Raw

Assert-Text $wrapper '-CPUOpponentProof\s+`\s*\r?\n\s*-MatchLifecycleProof\s+`\s*\r?\n\s*-OneMinuteMatchProof' `
    'One-minute wrapper does not select the existing CPU/lifecycle mode-163 path.'
Assert-Text $owner '(?s)if \(\$MatchLifecycleProof\) \{\s*\$CPUOpponentProof = \$true.*?\}\s*if \(\$CPUOpponentProof\) \{\s*\$FoxCpuMode = 1\s*\$foxCpuModeSelected = \$true\s*\}' `
    'CPU/lifecycle proof no longer forces the Fox CPU decision path on.'
Assert-Text $owner '(?s)\$preBattleSelectorSelected =\s*\$staticTextureAotSelected -or \$foxCpuModeSelected.*?if \(\$preBattleSelectorSelected\) \{.*?''tbreak scVSBattleStartBattle''.*?if \(\$foxCpuModeSelected\) \{\s*\$preBattleSetupCommands \+=\s*\(''set variable gNdsBattlePlayableFoxCpuEnabled = \{0\}'' -f\s*\$FoxCpuMode\).*?\}.*?\$gdbCommands = @\(\s*\$gdbCommands\[0\.\.3\]\s*\$preBattleSetupCommands' `
    'Common verifier no longer sets the selected Fox CPU mode before battle setup.'
Assert-Text $wrapper 'unthrottled state/memory only; realtime performance is not measured' `
    'One-minute wrapper lost its explicit no-realtime-claim notice.'
if ($wrapper -match 'RequireRealtime60Fps|capture-melonds|Screenshot|FiveMinute|five-minute|18000') {
    throw 'One-minute wrapper must not claim realtime, capture a desktop screenshot, or retain a five-minute assumption.'
}
Assert-Text $lifecycle 'Get-MelonDSRunnerPort -RunnerSlot \$RunnerSlot -Cpu ARM9' `
    'Match-lifecycle wrapper no longer derives an isolated GDB port from RunnerSlot.'
Assert-Text $lifecycle '-MelonDS \$MelonDS' `
    'Match-lifecycle wrapper no longer forwards the selected repo-local emulator.'
Assert-Text $lifecycle '-Gdb \$Gdb' `
    'Match-lifecycle wrapper no longer forwards the selected GDB executable.'
Assert-Text $lifecycle '-GdbPort \$selectedGdbPort' `
    'Match-lifecycle wrapper no longer forwards its selected isolated GDB port.'

Assert-Text $battle 'smash64ds-battle-playable-one-minute-match' `
    'One-minute verifier target is not artifact-isolated from the canonical ROM.'
Assert-Text $battle 'build-battle-playable-one-minute-match-harness' `
    'One-minute verifier build directory is not isolated.'
Assert-Text $battle '\$RendererProfileLevel = if \(\$OneMinuteMatchProof\) \{ 0 \} else \{ 2 \}' `
    'One-minute verifier no longer disables verifier-only renderer profiling.'
Assert-Text $battle '-OneMinuteMatchProof:\$OneMinuteMatchProof' `
    'One-minute selector is not forwarded to the natural-runtime owner.'

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
Assert-Text $owner 'MATCH_SAFETY=' `
    'One-minute verifier lost its safety-counter marker.'
Assert-Text $owner 'AUDIO_FGM_KO=' `
    'One-minute verifier lost its natural KO FGM marker.'
Assert-Text $owner "(?s)439,292,154.*154,439,292.*370,289,154.*154,370,289" `
    'One-minute verifier lost the exact Mario/Fox regular/up-star KO call orders.'
Assert-Text $owner '(?s)\$fgmKo\[10\] -eq 0.*\$fgmKo\[11\] -eq 0.*\$fgmKo\[12\] -eq 0.*\$fgmKo\[13\] -eq 0' `
    'One-minute verifier no longer requires clean included-KO playback.'
Assert-Text $owner 'Where-Object \{ \$_ -ne 0 \}' `
    'One-minute verifier no longer requires every safety counter to remain zero.'
Assert-Text $owner '\(\$ma\[6\] - \$audioResidentBytes\) -ge 131072' `
    'One-minute verifier lost the conservative 128 KiB reserve gate.'
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
Assert-Text $owner 'unthrottled state/memory only; realtime not measured' `
    'One-minute verifier output can be mistaken for realtime evidence.'
if (($owner -match 'FiveMinute|five-minute|5:00') -or ($battle -match 'FiveMinute|five-minute')) {
    throw 'One-minute verifier path retains a five-minute label or selector.'
}

Write-Output "One-minute mode-163 match verifier static check passed: $root"
