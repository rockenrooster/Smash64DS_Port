param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Route = 0,
    [int]$Admit = 0
)
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice1'
$lean = @('draws','shadow_runs','attempts','adopts',
    'adopt_refuse[1]','adopt_refuse[2]','adopt_refuse[3]','adopt_refuse[4]','adopt_refuse[5]','adopt_refuse[6]','adopt_refuse[7]',
    'adopt_tinted_sites','adopt_needs_fence','adopt_words','adopt_roots','adopt_textures',
    'kernel_joints','kernel_slow_joints','kernel_fail',
    'head_ticks','guard_ticks','kernel_ticks','patch_ticks','submit_ticks','dma_wait_ticks','dma_wait_spins',
    'oracle_runs','oracle_words','oracle_mismatch[0]','oracle_mismatch[1]','oracle_mismatch[2]','oracle_mismatch[3]','oracle_mismatch[4]','oracle_mismatch[5]','oracle_mismatch[6]',
    'oracle_max_lsb[0]','oracle_max_lsb[1]','oracle_max_lsb[2]',
    'oracle_key_moved[0]','oracle_key_moved[1]','oracle_key_moved[2]','oracle_key_moved[3]','oracle_key_moved[4]','oracle_key_moved[5]','oracle_key_moved[6]','oracle_key_moved[7]',
    'oracle_record_under_hit[0]','oracle_record_under_hit[1]','oracle_unconsumed','oracle_source_miss',
    'oracle_record_diff[0]','oracle_record_diff[1]','oracle_record_diff[2]','oracle_record_diff[3]','oracle_record_diff[4]','oracle_record_diff[5]','oracle_record_diff[6]','oracle_record_diff[7]','oracle_fence_rekey',
    'oracle_shade_witness_count','oracle_shade_witness[0][0]','oracle_shade_witness[0][1]','oracle_shade_witness[0][2]','oracle_shade_witness[0][3]','oracle_shade_witness[0][4]','oracle_shade_witness[0][5]','oracle_shade_witness[0][6]','oracle_shade_witness[0][7]','oracle_shade_witness[0][8]','oracle_shade_witness[0][9]','oracle_shade_witness[0][10]','oracle_shade_witness[0][11]','oracle_shade_witness[1][0]','oracle_shade_witness[1][1]','oracle_shade_witness[1][2]','oracle_shade_witness[1][3]','oracle_shade_witness[1][4]','oracle_shade_witness[1][5]','oracle_shade_witness[1][6]','oracle_shade_witness[1][7]','oracle_shade_witness[1][8]','oracle_shade_witness[1][9]','oracle_shade_witness[1][10]','oracle_shade_witness[1][11]','oracle_shade_witness[2][0]','oracle_shade_witness[2][1]','oracle_shade_witness[2][2]','oracle_shade_witness[2][3]','oracle_shade_witness[2][4]','oracle_shade_witness[2][5]','oracle_shade_witness[2][6]','oracle_shade_witness[2][7]','oracle_shade_witness[2][8]','oracle_shade_witness[2][9]','oracle_shade_witness[2][10]','oracle_shade_witness[2][11]','oracle_shade_witness[3][0]','oracle_shade_witness[3][1]','oracle_shade_witness[3][2]','oracle_shade_witness[3][3]','oracle_shade_witness[3][4]','oracle_shade_witness[3][5]','oracle_shade_witness[3][6]','oracle_shade_witness[3][7]','oracle_shade_witness[3][8]','oracle_shade_witness[3][9]','oracle_shade_witness[3][10]','oracle_shade_witness[3][11]',
    'record_shade_checked','record_shade_inconsistent','record_shade_inconsistent_packets','record_shade_witness_count','record_shade_witness[0][0]','record_shade_witness[0][1]','record_shade_witness[0][2]','record_shade_witness[0][3]','record_shade_witness[0][4]','record_shade_witness[0][5]','record_shade_witness[0][6]','record_shade_witness[0][7]','record_shade_witness[0][8]','record_shade_witness[1][0]','record_shade_witness[1][1]','record_shade_witness[1][2]','record_shade_witness[1][3]','record_shade_witness[1][4]','record_shade_witness[1][5]','record_shade_witness[1][6]','record_shade_witness[1][7]','record_shade_witness[1][8]',
    'ge_busy_samples','ge_busy_hits','packet_dma_waits','packet_dma_wait_ticks',
    'tint_rerecords[0]','tint_rerecords[1]','tint_rerecords[2]','tint_rerecords[3]',
    'tint_patch_binds','tint_patch_moved','tint_patch_miss','tint_patch_white','tint_patch_shape','adopt_tint_binds','adopt_fence_other',
    'fighter_uploads','fighter_uploads_after_go','fighter_upload_bytes_after_go','go_frame',
    'admit_runs','admit_pinned','admit_pinned_bytes',
    'union_textures[0]','union_textures[1]','union_textures[2]','union_textures[3]',
    'union_bytes[0]','union_bytes[1]','union_bytes[2]','union_bytes[3]',
    'census_live','census_free','census_pinned','census_static','census_live_bytes','census_fighter_bytes',
    'reject_count','reject_mask',
    'reject_first[0]','reject_first[1]','reject_first[2]','reject_first[3]','reject_first[4]','reject_first[5]',
    'reject_first[6]','reject_first[7]','reject_first[8]','reject_first[9]','reject_first[10]','reject_first[11]')
$decl = 0..15 | ForEach-Object { "gNdsFtrLean.decline[$_]" }
$extra = @($lean | ForEach-Object { "gNdsFtrLean.$_" }) + $decl + @(
    'gNdsRendererNativeFailure.count','gNdsRendererNativeFailure.status','gNdsRendererNativeFailure.identity',
    'gNdsRendererNativeFailure.reason','gNdsRendererNativeDirectReject.count','gNdsRendererNativeDirectReject.site',
    'gNdsFighterPacketHits','gNdsFighterPacketRecords','gNdsFighterPacketFaults','gNdsFighterPacketDeclines',
    'gNdsFighterMarioFoxDLAllDrawCount','gNdsR2FighterTintBuilds','gNdsR2FighterTintSetGeneration','gNdsR2FighterTintHits','gNdsR2FighterTintMisses','gNdsR2FighterTintEvictions','gNdsFighterPacketTintRerecords',
    'gNdsFtrLeanRoute','gNdsFtrLeanAdmit','gNdsTaskmanGeneralHeapFreeMin')
$sets = @("gNdsFtrLeanRoute=$Route")
if ($Admit -ne 0) { $sets += "gNdsFtrLeanAdmit=$Admit" }
$rows = Join-Path $art "$Arm-rows.csv"
$json = Join-Path $art "$Arm.json"
$log = Join-Path $art "$Arm-run.log"
$start = Get-Date
& pwsh -NoProfile -File (Join-Path $root 'scripts\sample-tick-hud-buckets.ps1') -NoBuild `
    -Target smash64ds-p2-fourcpu-tickhud-hwtri -Build build-p2-fourcpu-tickhud -RingDump `
    -Samples 1972 -StartFrame 2 -SetGlobals ($sets -join ',') -ExtraGlobals ($extra -join ',') `
    -RowsCsv $rows -JsonOut $json -RunnerSlot 9 -GdbPort 3423 -TimeoutSeconds 3600 *> $log
$code = $LASTEXITCODE
Add-Content -LiteralPath $log -Value ("`nRUN_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds)
