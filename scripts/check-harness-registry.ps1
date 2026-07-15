$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
function Fail-Check {
    param([string]$Message)
    throw "Harness registry check failed: $Message"
}
function Assert-ProfilePlan {
    param(
        [string]$Profile,
        [string[]]$Expected
    )
    $actual = @(Get-Smash64DSVerifyPlan -Profile $Profile | ForEach-Object { $_.Name })
    if ($actual.Count -ne $Expected.Count) {
        Fail-Check "profile '$Profile' count mismatch: actual=$($actual.Count), expected=$($Expected.Count), actual entries=$($actual -join ', ')"
    }
    for ($i = 0; $i -lt $Expected.Count; $i++) {
        if ($actual[$i] -ne $Expected[$i]) {
            Fail-Check "profile '$Profile' entry $i mismatch: actual=$($actual[$i]), expected=$($Expected[$i])"
        }
    }
}
$registry = @(Get-Smash64DSHarnessRegistry)
$harnessRecords = @($registry | Where-Object { -not [string]::IsNullOrWhiteSpace($_.Harness) })
$headerPath = Join-Path $root 'include\nds\nds_scene_harness.h'
$makefilePath = Join-Path $root 'Makefile'
$headerText = Get-Content -LiteralPath $headerPath -Raw
$makefileText = Get-Content -LiteralPath $makefilePath -Raw
$headerModes = @{}
[regex]::Matches($headerText, '#define\s+NDS_DEV_SCENE_HARNESS_([A-Z0-9_]+)\s+([0-9]+)u?') | ForEach-Object {
    $name = $_.Groups[1].Value.ToLowerInvariant()
    $headerModes[$name] = [int]$_.Groups[2].Value
}
$makefileModes = @{}
$makePatterns = @(
    'else ifeq \(\$\(NDS_DEV_SCENE_HARNESS\),([a-zA-Z0-9_]+)\)\s*CFLAGS \+= -DNDS_DEV_SCENE_HARNESS=([0-9]+)',
    'else ifeq \(\$\(NDS_DEV_SCENE_HARNESS\),([a-zA-Z0-9_]+)\)\s*NDS_DEV_SCENE_HARNESS_ID\s*:=\s*([0-9]+)'
)
foreach ($makePattern in $makePatterns) {
    [regex]::Matches($makefileText, $makePattern) | ForEach-Object {
        $makefileModes[$_.Groups[1].Value] = [int]$_.Groups[2].Value
    }
}
if (($makefileText -match 'ifeq \(\$\(NDS_DEV_SCENE_HARNESS\),normal\)\s*CFLAGS \+= -DNDS_DEV_SCENE_HARNESS=([0-9]+)') -or
    ($makefileText -match 'ifeq \(\$\(NDS_DEV_SCENE_HARNESS\),normal\)\s*NDS_DEV_SCENE_HARNESS_ID\s*:=\s*([0-9]+)')) {
    $makefileModes['normal'] = [int]$Matches[1]
}
foreach ($field in @('Name','Harness','Script')) {
    $values = @($registry | ForEach-Object { $_.$field } | Where-Object { $null -ne $_ -and "$_" -ne '' })
    $dupes = @($values | Group-Object | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name })
    if ($dupes.Count -gt 0) { Fail-Check "duplicate $field value(s): $($dupes -join ', ')" }
}
$modeDupes = @($registry |
    Where-Object { $null -ne $_.Mode } |
    Group-Object Mode |
    Where-Object { $_.Count -gt 1 })
foreach ($dupe in $modeDupes) {
    $names = @($dupe.Group | ForEach-Object { $_.Name } | Sort-Object)
    if (($dupe.Name -ne '163') -or (($names -join ',') -ne 'battle_playable,battle_playable_match_lifecycle,battle_playable_realtime')) {
        Fail-Check "duplicate Mode value '$($dupe.Name)' for unexpected aliases: $($names -join ', ')"
    }
}
foreach ($record in $registry) {
    $scriptPath = Join-Path $PSScriptRoot $record.Script
    if (-not (Test-Path -LiteralPath $scriptPath)) {
        Fail-Check "missing verifier script for '$($record.Name)': $($record.Script)"
    }
}
foreach ($wrapper in @('verify-current.ps1','verify-dev-fast.ps1','verify-boundary.ps1','verify-p1-gate.ps1','verify-regression.ps1')) {
    $wrapperPath = Join-Path $PSScriptRoot $wrapper
    if (-not (Test-Path -LiteralPath $wrapperPath)) {
        Fail-Check "missing verifier wrapper script: $wrapper"
    }
}
$devFastText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'verify-dev-fast.ps1') -Raw
$openingSkipText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'verify-opening-skip.ps1') -Raw
$realtimeText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'verify-battle-playable-realtime-harness.ps1') -Raw
$p1GateText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'verify-p1-gate.ps1') -Raw
$verifyAllText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'verify-all.ps1') -Raw
$battleLoopText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') -Raw
$captureText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'capture-melonds.ps1') -Raw
$debugMelonText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'debug-melonds.ps1') -Raw
$melonLibText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'lib\melonds.ps1') -Raw
$publishedRomCheckPath = Join-Path $PSScriptRoot 'check-published-roms.ps1'
if (-not (Test-Path -LiteralPath $publishedRomCheckPath -PathType Leaf)) {
    Fail-Check 'missing two-ROM publication contract checker'
}
$forcedBuildTokenPattern = '(?<![A-Za-z0-9_-])[''"]?-B[''"]?(?![A-Za-z0-9_-])'
if (($devFastText -notmatch "'-FastIteration'") -or
    ($devFastText -match "Profile\s*=\s*'BoundaryDirect'") -or
    ($devFastText -match $forcedBuildTokenPattern)) {
    Fail-Check 'verify-dev-fast is not the incremental canonical fast-iteration path'
}
if (($p1GateText -notmatch 'P1Gate execution is retired') -or
    ($p1GateText -notmatch 'two-ROM runtime model') -or
    ($p1GateText -match '&\s*make')) {
    Fail-Check 'verify-p1-gate can still publish compile-distinct ROMs'
}
if (($realtimeText -notmatch 'check-published-roms\.ps1') -or
    ($realtimeText -notmatch '\[switch\]\$FastIteration')) {
    Fail-Check 'published realtime verifier is missing fast iteration or the ROM contract'
}
if (($realtimeText -notmatch 'Resolve-MelonDSRunnerSlot') -or
    ($realtimeText -notmatch '-MelonDS\s+\$captureMelonDS') -or
    ($realtimeText -notmatch 'System\.Threading\.Mutex') -or
    ($realtimeText -notmatch 'System\.IO\.File\]::Replace')) {
    Fail-Check 'canonical realtime capture is missing runner-slot isolation or atomic stable publication'
}
if (($melonLibText -notmatch 'function\s+Set-MelonDSDualScreenLayout') -or
    ($melonLibText -notmatch "@\('ScreenSizing',\s*'0'\)") -or
    ($captureText -notmatch 'Set-MelonDSDualScreenLayout') -or
    ($debugMelonText -notmatch 'Set-MelonDSDualScreenLayout')) {
    Fail-Check 'visible melonDS launch/capture no longer guarantees both DS screens'
}
if (($realtimeText -match 'MinFighterRegionFraction|MinRegionFighterFraction|MinRequiredRegionFighterFraction') -or
    ($battleLoopText -notmatch 'FTR_DISPLAY_CONTRACT=') -or
    ($battleLoopText -notmatch '(?s)Assert-Condition\s*\(\$stageHardwareFighter\.Success.*?\$shwf\[0\]\s*-eq\s*\(2\s*\*\s*\$hw\[0\]\).*?\$shwf\[1\]\s*-eq\s*\(626\s*\*\s*\$hw\[0\]\)') -or
    ($battleLoopText -notmatch '(?s)Assert-Condition\s*\(\$fighterDisplayContract\.Success.*?\$fdc\[0\]\s*-gt\s*0.*?\$fdc\[3\]\s*-gt\s*0.*?\$fdc\[7\]\s*-gt\s*0.*?\$fdc\[8\]\s*-eq\s*0')) {
    Fail-Check 'canonical realtime verifier must use selected/submitted/in-bounds GDB fighter contracts without fixed fighter crops'
}
if ($verifyAllText -notmatch '(?s)\(\$record\.Name -eq ''battle_playable_realtime''\).*?\$arguments \+= ''-FastIteration''') {
    Fail-Check 'verify-all does not select the one-capture canonical fast path for realtime records'
}
$compactExpectedMatch = [regex]::Match(
    $openingSkipText,
    '(?s)\$expected\s*=\s*if\s*\(\$Compact\)\s*\{\s*@\{(?<Compact>.*?)\}\s*\}\s*else\s*\{\s*@\{(?<Default>.*?)\}\s*\}'
)
if (($openingSkipText -notmatch '\[switch\]\$Compact') -or
    (-not $compactExpectedMatch.Success) -or
    ($compactExpectedMatch.Groups['Compact'].Value -match 'ROOM_RELOC_SYMBOL_COUNT') -or
    ($compactExpectedMatch.Groups['Compact'].Value -notmatch "TASKMAN_RETURNS\s*=\s*'2'") -or
    ($compactExpectedMatch.Groups['Compact'].Value -notmatch "ROOM_RELOC_MASK\s*=\s*'0xff'") -or
    ($compactExpectedMatch.Groups['Compact'].Value -notmatch "ROOM_RELOC_DATA\s*=\s*'1'") -or
    ($compactExpectedMatch.Groups['Compact'].Value -notmatch "ROOM_RELOC_WORD_SWAP_FAILS\s*=\s*'0'") -or
    ($compactExpectedMatch.Groups['Compact'].Value -notmatch "ROOM_RELOC_POINTER_FAILS\s*=\s*'0'") -or
    ($compactExpectedMatch.Groups['Compact'].Value -notmatch "ROOM_RELOC_SYMBOL_FAILS\s*=\s*'0'") -or
    ($compactExpectedMatch.Groups['Default'].Value -notmatch "ROOM_RELOC_SYMBOL_COUNT\s*=\s*'43'")) {
    Fail-Check 'opening skip compact contract does not preserve reloc failures while leaving exact historical counters in the default verifier'
}
if ($verifyAllText -notmatch '(?s)\(\$Profile -eq ''P1Gate''\).*?\(\$record\.Name -eq ''opening_skip''\).*?\$arguments \+= ''-Compact''') {
    Fail-Check 'P1Gate does not select the compact opening skip contract'
}
if ($battleLoopText -notmatch '\[int64\]\$aobj32\.Groups\[4\]\.Value -eq 0') {
    Fail-Check 'battle lifecycle verifier does not parse unsigned AObj32 failure count safely'
}
if (($makefileText -notmatch 'ifeq \(\$\(TARGET\),smash64ds-battle-playable-hwtri\)') -or
    ($makefileText -notmatch '(?s)ifeq \(\$\(TARGET\),smash64ds-battle-playable-hwtri\).*?override NDS_DEV_SCENE_HARNESS := battle_playable_realtime.*?override NDS_DEV_LIVE_INPUT_PREVIEW := 1.*?override NDS_HARNESS_FAST_LOGIC := 0.*?override NDS_RENDERER_HW_TRIANGLES := 1')) {
    Fail-Check 'published battle target is not intrinsically realtime, interactive, and hardware-rendered'
}
if (($makefileText -notmatch 'NDS_PUBLISHED_TARGETS := smash64ds smash64ds-battle-playable-hwtri') -or
    ($makefileText -notmatch 'override NDS_PUBLISH_USER_ROM :=') -or
    ($makefileText -notmatch 'Non-published target .* may not write into the project root') -or
    ($makefileText -notmatch 'export OUTPUT := \$\(NDS_OUTPUT_ROOT\)/\$\(TARGET\)')) {
    Fail-Check 'Makefile no longer confines non-published ROMs to their build directories'
}
foreach ($record in $harnessRecords) {
    $expectedKey = $record.Harness.ToLowerInvariant()
    if (-not $headerModes.ContainsKey($expectedKey)) {
        Fail-Check "header missing mode for harness '$($record.Harness)'"
    }
    if ($headerModes[$expectedKey] -ne $record.Mode) {
        Fail-Check "header mode mismatch for '$($record.Harness)': registry=$($record.Mode), header=$($headerModes[$expectedKey])"
    }
    if (-not $makefileModes.ContainsKey($record.Harness)) {
        Fail-Check "Makefile missing NDS_DEV_SCENE_HARNESS mapping for '$($record.Harness)'"
    }
    if ($makefileModes[$record.Harness] -ne $record.Mode) {
        Fail-Check "Makefile mode mismatch for '$($record.Harness)': registry=$($record.Mode), Makefile=$($makefileModes[$record.Harness])"
    }
}
$sceneConfigMacroPattern = '\b(?:NDS_DEV_SCENE_HARNESS|NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP)\b'
$sceneConfigIncludePattern = '#\s*include\s+["<]nds_scene_harness_config\.h[">]'
$sceneConfigViolations = @()
Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File |
    Where-Object { $_.Extension -in @('.c', '.h') } |
    ForEach-Object {
        $text = Get-Content -LiteralPath $_.FullName -Raw
        if (($text -match $sceneConfigMacroPattern) -and
            ($text -notmatch $sceneConfigIncludePattern)) {
            $sceneConfigViolations += $_.FullName.Substring($root.Length).TrimStart([char[]]@('\','/'))
        }
    }
if ($sceneConfigViolations.Count -gt 0) {
    Fail-Check "scene harness macro reference(s) missing nds_scene_harness_config.h include: $($sceneConfigViolations -join ', ')"
}
$latestExpect = @{
    battle_playable_realtime = 163
}
foreach ($name in $latestExpect.Keys) {
    $record = $registry | Where-Object { $_.Name -eq $name } | Select-Object -First 1
    if (-not $record) { Fail-Check "missing latest registry entry '$name'" }
    if ($record.Mode -ne $latestExpect[$name]) {
        Fail-Check "latest mode mismatch for '$name': registry=$($record.Mode), expected=$($latestExpect[$name])"
    }
}
$latestRecords = @($registry | Where-Object { $_.Tags -contains 'latest' })
$latestMode163 = @($latestRecords | Where-Object { $_.Mode -eq 163 })
if (($latestMode163.Count -ne 1) -or ($latestMode163[0].Name -ne 'battle_playable_realtime')) {
    Fail-Check "mode 163 latest metadata must name only battle_playable_realtime: $(@($latestMode163.Name) -join ', ')"
}
foreach ($diagnosticName in @('battle_mariofox_stage_mplivehit_status_loop', 'menu_chain_mariofox_stage_mplivehit_status_loop')) {
    $record = $registry | Where-Object { $_.Name -eq $diagnosticName } | Select-Object -First 1
    if ((-not $record) -or ($record.Tags -notcontains 'diagnostic') -or ($record.Tags -contains 'latest')) {
        Fail-Check "legacy bounded record '$diagnosticName' must be diagnostic and not latest"
    }
}
Assert-ProfilePlan 'BoundaryDirect' @(
    'battle_playable_realtime'
)
Assert-ProfilePlan 'Boundary' @(
    'battle_playable_realtime'
)
Assert-ProfilePlan 'P1Gate' @(
    'opening_skip',
    'battle_playable_realtime',
    'battle_playable',
    'battle_playable_match_lifecycle'
)
Assert-ProfilePlan 'Latest' @(
    'runtime',
    'battle_playable_realtime'
)
Assert-ProfilePlan 'LatestFast' @(
    'battle_playable_realtime'
)
Assert-ProfilePlan 'Regression' @(
    'runtime',
    'stage_mplivehit_continuous_runtime',
    'title',
    'battle_mariofox_stage_gcdrawall_loop',
    'menu_chain_mariofox_stage_gcdrawall_loop',
    'battle_mariofox_stage_collision_loop',
    'menu_chain_mariofox_stage_collision_loop',
    'battle_mariofox_stage_floor_follow_loop',
    'menu_chain_mariofox_stage_floor_follow_loop',
    'battle_mariofox_stage_floor_edge_loop',
    'menu_chain_mariofox_stage_floor_edge_loop',
    'battle_mariofox_stage_mpprocess_floor_loop',
    'menu_chain_mariofox_stage_mpprocess_floor_loop',
    'battle_mariofox_stage_mpupdate_floor_loop',
    'menu_chain_mariofox_stage_mpupdate_floor_loop',
    'battle_mariofox_stage_mpsweep_floor_loop',
    'menu_chain_mariofox_stage_mpsweep_floor_loop',
    'battle_mariofox_stage_mpcross_floor_loop',
    'menu_chain_mariofox_stage_mpcross_floor_loop',
    'battle_mariofox_stage_mpadjust_floor_loop',
    'menu_chain_mariofox_stage_mpadjust_floor_loop',
    'battle_mariofox_stage_mpedge_floor_loop',
    'menu_chain_mariofox_stage_mpedge_floor_loop',
    'battle_mariofox_stage_mpwall_floor_loop',
    'menu_chain_mariofox_stage_mpwall_floor_loop',
    'battle_mariofox_stage_mpstale_floor_loop',
    'menu_chain_mariofox_stage_mpstale_floor_loop',
    'battle_mariofox_stage_mplivestale_floor_loop',
    'menu_chain_mariofox_stage_mplivestale_floor_loop',
    'battle_mariofox_stage_mpmotionstale_floor_loop',
    'menu_chain_mariofox_stage_mpmotionstale_floor_loop',
    'battle_mariofox_stage_mpcliffstatus_floor_loop',
    'menu_chain_mariofox_stage_mpcliffstatus_floor_loop',
    'battle_mariofox_stage_mpclifftick_floor_loop',
    'menu_chain_mariofox_stage_mpclifftick_floor_loop',
    'battle_mariofox_stage_mpfallmap_floor_loop',
    'menu_chain_mariofox_stage_mpfallmap_floor_loop',
    'battle_mariofox_stage_mpfallland_floor_loop',
    'menu_chain_mariofox_stage_mpfallland_floor_loop',
    'battle_mariofox_stage_mpceil_floor_loop',
    'menu_chain_mariofox_stage_mpceil_floor_loop',
    'battle_mariofox_stage_mpceilstatus_floor_loop',
    'menu_chain_mariofox_stage_mpceilstatus_floor_loop',
    'battle_mariofox_stage_mpcliffcatch_floor_loop',
    'menu_chain_mariofox_stage_mpcliffcatch_floor_loop',
    'battle_mariofox_stage_mpcliffwait_floor_loop',
    'menu_chain_mariofox_stage_mpcliffwait_floor_loop',
    'battle_mariofox_stage_mpcliffattack_floor_loop',
    'menu_chain_mariofox_stage_mpcliffattack_floor_loop',
    'battle_mariofox_stage_mpcliffattack_action_loop',
    'menu_chain_mariofox_stage_mpcliffattack_action_loop',
    'battle_mariofox_stage_mpcliffcommon2_loop',
    'menu_chain_mariofox_stage_mpcliffcommon2_loop',
    'battle_mariofox_stage_mpcliffescape_action_loop',
    'menu_chain_mariofox_stage_mpcliffescape_action_loop',
    'battle_mariofox_stage_mpcliffescape_common2_loop',
    'menu_chain_mariofox_stage_mpcliffescape_common2_loop',
    'battle_mariofox_stage_mpcliffclimb_floor_loop',
    'menu_chain_mariofox_stage_mpcliffclimb_floor_loop',
    'battle_mariofox_stage_mpcliffclimb_action_loop',
    'menu_chain_mariofox_stage_mpcliffclimb_action_loop',
    'battle_mariofox_stage_mpcliffclimb_common2_loop',
    'menu_chain_mariofox_stage_mpcliffclimb_common2_loop',
    'battle_mariofox_stage_mpcliffclimb_finish_loop',
    'menu_chain_mariofox_stage_mpcliffclimb_finish_loop',
    'battle_mariofox_stage_mpcliffwait_damage_loop',
    'menu_chain_mariofox_stage_mpcliffwait_damage_loop',
    'battle_mariofox_stage_mppassive_loop',
    'menu_chain_mariofox_stage_mppassive_loop',
    'battle_mariofox_stage_mpdownwait_loop',
    'menu_chain_mariofox_stage_mpdownwait_loop',
    'battle_mariofox_stage_turn_loop',
    'menu_chain_mariofox_stage_turn_loop',
    'battle_mariofox_stage_mpdownrecover_loop',
    'menu_chain_mariofox_stage_mpdownrecover_loop',
    'battle_mariofox_stage_mpcliffledge_loop',
    'menu_chain_mariofox_stage_mpcliffledge_loop',
    'battle_mariofox_stage_mpclifflive_loop',
    'menu_chain_mariofox_stage_mpclifflive_loop',
    'battle_mariofox_stage_mpwallhit_floor_loop',
    'menu_chain_mariofox_stage_mpwallhit_floor_loop',
    'battle_mariofox_stage_mpwallcopy_floor_loop',
    'menu_chain_mariofox_stage_mpwallcopy_floor_loop',
    'battle_mariofox_stage_mppass_floor_loop',
    'menu_chain_mariofox_stage_mppass_floor_loop',
    'battle_mariofox_stage_mpplatform_floor_loop',
    'menu_chain_mariofox_stage_mpplatform_floor_loop',
    'battle_mariofox_stage_mpplatform_active_floor_loop',
    'menu_chain_mariofox_stage_mpplatform_active_floor_loop',
    'battle_mariofox_stage_mpplatform_tick_floor_loop',
    'menu_chain_mariofox_stage_mpplatform_tick_floor_loop',
    'battle_mariofox_stage_mppass_input_loop',
    'menu_chain_mariofox_stage_mppass_input_loop',
    'battle_mariofox_stage_mpplatform_pos_floor_loop',
    'menu_chain_mariofox_stage_mpplatform_pos_floor_loop',
    'battle_mariofox_stage_mpplatform_speed_floor_loop',
    'menu_chain_mariofox_stage_mpplatform_speed_floor_loop',
    'battle_mariofox_stage_inishie_scale_loop',
    'menu_chain_mariofox_stage_inishie_scale_loop',
    'battle_mariofox_stage_mppassive_recover_loop',
    'menu_chain_mariofox_stage_mppassive_recover_loop',
    'battle_mariofox_stage_mpdamage_recover_loop',
    'menu_chain_mariofox_stage_mpdamage_recover_loop',
    'battle_playable_realtime',
    'battle_playable',
    'battle_playable_match_lifecycle'
)
Assert-ProfilePlan 'RegressionCore' @(
    'runtime',
    'title',
    'battle_playable_realtime',
    'battle_playable_match_lifecycle',
    'battle_mariofox_stage_mpcliffstatus_floor_loop',
    'battle_mariofox_stage_mpupdate_floor_loop',
    'menu_chain_mariofox_stage_mpupdate_floor_loop'
)
foreach ($suffix in @('model','struct','init','wait','wait_tick','wait_ground','display_probe','dl_scan','dl_execute','dl_draw','dl_draw_multi','dl_draw_all','walk_input','walk_loop','dash_run','jump_loop','landing_loop','process_loop','scheduler_loop','controller_loop','preview_loop','gcrunall_loop','live_preview','stage_gcdrawall_loop','stage_collision_loop','stage_floor_follow_loop','stage_floor_edge_loop','stage_mpprocess_floor_loop','stage_mpupdate_floor_loop','stage_mpsweep_floor_loop','stage_mpcross_floor_loop','stage_mpadjust_floor_loop','stage_mpedge_floor_loop','stage_mpwall_floor_loop','stage_mpwallhit_floor_loop','stage_mpwallcopy_floor_loop','stage_mppass_floor_loop','stage_mpplatform_floor_loop','stage_mpplatform_active_floor_loop','stage_mpplatform_tick_floor_loop','stage_mppass_input_loop','stage_mpplatform_pos_floor_loop','stage_mpplatform_speed_floor_loop','stage_inishie_scale_loop','stage_mppassive_recover_loop','stage_mpdamage_recover_loop','stage_mplivehit_status_loop','stage_mpstale_floor_loop','stage_mplivestale_floor_loop','stage_mpmotionstale_floor_loop','stage_mpcliffstatus_floor_loop','stage_mpclifftick_floor_loop','stage_mpfallmap_floor_loop','stage_mpfallland_floor_loop','stage_mpceil_floor_loop','stage_mpceilstatus_floor_loop','stage_mpcliffcatch_floor_loop','stage_mpcliffwait_floor_loop','stage_mpcliffattack_floor_loop','stage_mpcliffattack_action_loop','stage_mpcliffcommon2_loop','stage_mpcliffescape_action_loop','stage_mpcliffescape_common2_loop','stage_mpcliffclimb_floor_loop','stage_mpcliffclimb_action_loop','stage_mpcliffclimb_common2_loop','stage_mpcliffclimb_finish_loop','stage_mpcliffwait_damage_loop','stage_mppassive_loop','stage_mpdownwait_loop','stage_turn_loop','stage_mpdownrecover_loop','stage_mpcliffledge_loop','stage_mpclifflive_loop')) {
    $direct = "battle_mariofox_$suffix"
    $menu = "menu_chain_mariofox_$suffix"
    if (-not ($registry | Where-Object { $_.Name -eq $direct })) { Fail-Check "missing paired direct Mario/Fox proof '$direct'" }
    if (-not ($registry | Where-Object { $_.Name -eq $menu })) { Fail-Check "missing paired menu-chain Mario/Fox proof '$menu'" }
}
$harnessCount = $harnessRecords.Count
$scriptCount = (@($registry | Select-Object -ExpandProperty Script -Unique)).Count
Write-Output "Harness registry check passed: $harnessCount harness mappings, $scriptCount verifier scripts, 0 drift."
