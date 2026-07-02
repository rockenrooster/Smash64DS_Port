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
foreach ($field in @('Name','Harness','Mode','Script')) {
    $values = @($registry | ForEach-Object { $_.$field } | Where-Object { $null -ne $_ -and "$_" -ne '' })
    $dupes = @($values | Group-Object | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name })
    if ($dupes.Count -gt 0) { Fail-Check "duplicate $field value(s): $($dupes -join ', ')" }
}
foreach ($record in $registry) {
    $scriptPath = Join-Path $PSScriptRoot $record.Script
    if (-not (Test-Path -LiteralPath $scriptPath)) {
        Fail-Check "missing verifier script for '$($record.Name)': $($record.Script)"
    }
}
foreach ($wrapper in @('verify-current.ps1','verify-dev-fast.ps1','verify-boundary.ps1','verify-regression.ps1')) {
    $wrapperPath = Join-Path $PSScriptRoot $wrapper
    if (-not (Test-Path -LiteralPath $wrapperPath)) {
        Fail-Check "missing verifier wrapper script: $wrapper"
    }
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
$latestExpect = @{
    battle_mariofox_stage_mplivehit_status_loop = 161
    menu_chain_mariofox_stage_mplivehit_status_loop = 162
}
foreach ($name in $latestExpect.Keys) {
    $record = $registry | Where-Object { $_.Name -eq $name } | Select-Object -First 1
    if (-not $record) { Fail-Check "missing latest registry entry '$name'" }
    if ($record.Mode -ne $latestExpect[$name]) {
        Fail-Check "latest mode mismatch for '$name': registry=$($record.Mode), expected=$($latestExpect[$name])"
    }
}
Assert-ProfilePlan 'BoundaryDirect' @(
    'battle_mariofox_stage_mplivehit_status_loop'
)
Assert-ProfilePlan 'Boundary' @(
    'battle_mariofox_stage_mplivehit_status_loop',
    'menu_chain_mariofox_stage_mplivehit_status_loop'
)
Assert-ProfilePlan 'Latest' @(
    'runtime',
    'title',
    'battle_mariofox_stage_mplivehit_status_loop',
    'menu_chain_mariofox_stage_mplivehit_status_loop'
)
Assert-ProfilePlan 'Regression' @(
    'runtime',
    'stage_mplivehit_continuous_runtime',
    'title',
    'battle_mariofox_gcdrawall_loop',
    'menu_chain_mariofox_gcdrawall_loop',
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
    'battle_mariofox_stage_mplivehit_damage_loop',
    'menu_chain_mariofox_stage_mplivehit_damage_loop',
    'battle_mariofox_stage_mplivehit_status_loop',
    'menu_chain_mariofox_stage_mplivehit_status_loop'
)
foreach ($suffix in @('model','struct','init','wait','wait_tick','wait_ground','display_probe','dl_scan','dl_execute','dl_draw','dl_draw_multi','dl_draw_all','walk_input','walk_loop','dash_run','jump_loop','landing_loop','process_loop','scheduler_loop','controller_loop','preview_loop','gcrunall_loop','live_preview','gcdrawall_loop','stage_gcdrawall_loop','stage_collision_loop','stage_floor_follow_loop','stage_floor_edge_loop','stage_mpprocess_floor_loop','stage_mpupdate_floor_loop','stage_mpsweep_floor_loop','stage_mpcross_floor_loop','stage_mpadjust_floor_loop','stage_mpedge_floor_loop','stage_mpwall_floor_loop','stage_mpwallhit_floor_loop','stage_mpwallcopy_floor_loop','stage_mppass_floor_loop','stage_mpplatform_floor_loop','stage_mpplatform_active_floor_loop','stage_mpplatform_tick_floor_loop','stage_mppass_input_loop','stage_mpplatform_pos_floor_loop','stage_mpplatform_speed_floor_loop','stage_inishie_scale_loop','stage_mppassive_recover_loop','stage_mpdamage_recover_loop','stage_mplivehit_damage_loop','stage_mplivehit_status_loop','stage_mpstale_floor_loop','stage_mplivestale_floor_loop','stage_mpmotionstale_floor_loop','stage_mpcliffstatus_floor_loop','stage_mpclifftick_floor_loop','stage_mpfallmap_floor_loop','stage_mpfallland_floor_loop','stage_mpceil_floor_loop','stage_mpceilstatus_floor_loop','stage_mpcliffcatch_floor_loop','stage_mpcliffwait_floor_loop','stage_mpcliffattack_floor_loop','stage_mpcliffattack_action_loop','stage_mpcliffcommon2_loop','stage_mpcliffescape_action_loop','stage_mpcliffescape_common2_loop','stage_mpcliffclimb_floor_loop','stage_mpcliffclimb_action_loop','stage_mpcliffclimb_common2_loop','stage_mpcliffclimb_finish_loop','stage_mpcliffwait_damage_loop','stage_mppassive_loop','stage_mpdownwait_loop','stage_turn_loop','stage_mpdownrecover_loop','stage_mpcliffledge_loop','stage_mpclifflive_loop')) {
    $direct = "battle_mariofox_$suffix"
    $menu = "menu_chain_mariofox_$suffix"
    if (-not ($registry | Where-Object { $_.Name -eq $direct })) { Fail-Check "missing paired direct Mario/Fox proof '$direct'" }
    if (-not ($registry | Where-Object { $_.Name -eq $menu })) { Fail-Check "missing paired menu-chain Mario/Fox proof '$menu'" }
}
$harnessCount = $harnessRecords.Count
$scriptCount = (@($registry | Select-Object -ExpandProperty Script -Unique)).Count
Write-Output "Harness registry check passed: $harnessCount harness mappings, $scriptCount verifier scripts, 0 drift."
