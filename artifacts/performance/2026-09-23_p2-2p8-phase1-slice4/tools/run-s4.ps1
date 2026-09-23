param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Admit = 0,
    [int]$Route = 0,
    [int]$Census = 0,
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$Samples = 1972,
    [int]$StartFrame = 2,
    [int]$Diag = 0,
    [int]$Shadow = 0,
    [int]$IdCensus = 0,
    [int]$Slow = 0
)
# P2-2p8 Phase 1 slice 4 sampler wrapper: runner slot 9 / GDB 3423 only.
# The words are poked at the first frame-complete marker (whole u32 words).
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice4'
New-Item -ItemType Directory -Force -Path $art | Out-Null
$buckets = 43
$lab = @('runs', 'word', 'frame', 'regions', 'bg3_not_empty', 'lock_before', 'lock_after', 'records',
    'skipped_costume', 'skipped_hat', 'applied', 'fails', 'asset_fails', 'entries_admitted', 'bytes_admitted',
    'bytes_in_d', 'bytes_in_ab', 'locked', 'lock_frame', 'free_before', 'largest_before', 'free_after',
    'largest_after', 'exit_safety', 'bases', 'pruned', 'absent', 'libc_skipped', 'slot_skipped', 'libc_top_min', 'read_ticks', 'apply_ticks', 'reject_mask', 'reject_first[0]', 'reject_first[1]', 'reject_first[2]', 'reject_first[3]', 'reject_first[4]', 'reject_first[5]', 'ticks', 'exit_runs', 'exit_released', 'exit_orphans', 'done', 'outside_count')
$extra = @($lab | ForEach-Object { "gNdsFtrAdmitLab.$_" })
foreach ($f in 'per_fighter_applied', 'per_fighter_bytes', 'outside') {
    $extra += @(0..3 | ForEach-Object { "gNdsFtrAdmitLab.$f[$_]" })
}
$extra += @('gNdsFtrLeanAdmitFail') + @(0..3 | ForEach-Object { "gNdsFtrLeanAdmitFailFirst[$_]" })
$extra += @('gNdsFtrAdmitDynReadyHigh', 'gNdsFtrAdmitDynAdmittedHigh', 'gNdsFtrAdmitDynTouchedHigh', 'gNdsFtrAdmitDynFull',
    'gNdsVramBankDLent', 'gNdsVramBg3RefusedWrites', 'gNdsVramBankDTakes', 'gNdsVramBankDReturns', 'gNdsVramBankDMissedExits',
    'gNdsRendererNativeFailure.count', 'gNdsRendererNativeFailure.status', 'gNdsRendererNativeFailure.identity',
    'gNdsRendererNativeFailure.reason', 'gNdsRendererNativeDirectReject.count', 'gNdsRendererNativeDirectReject.site',
    'gNdsFtrLean.go_frame', 'gNdsFtrLean.fighter_uploads', 'gNdsFtrLean.fighter_uploads_after_go',
    'gNdsFtrLean.fighter_upload_bytes_after_go', 'gNdsFtrLean.reject_count', 'gNdsFtrLean.reject_mask',
    'gNdsFighterPacketHits', 'gNdsFighterPacketRecords', 'gNdsFighterPacketFaults', 'gNdsFighterPacketDeclines',
    'gNdsFtrLeanRoute', 'gNdsFtrLeanAdmit', 'gNdsVramCensusEnable', 'gNdsTaskmanGeneralHeapFreeMin',
    'gNdsRendererAdapterOwnerSelectedRootsHighWater', 'gNdsTaskmanLibcTopChunkMin', 'gNdsTaskmanLibcRuntimeHighWater')
$extra += @(0..11 | ForEach-Object { "gNdsFtrLean.reject_first[$_]" })
# slice 4 (lab): carved residency and the identity's collision checks
foreach ($f in 'opens', 'carrier_fail', 'lazy', 'retires', 'retire_regions', 'region_returns', 'exit_released',
    'libc_before', 'libc_after', 'census_runs', 'census_records', 'census_keys', 'census_dups', 'census_skipped',
    'census_noprobe', 'census_collisions', 'census_overflow', 'census_ticks') {
    $extra += "gNdsFtrCarveLab.$f"
}
foreach ($f in 'regions', 'reserved', 'used', 'carved', 'fails', 'shrink_freed') {
    $extra += @(0..1 | ForEach-Object { "gNdsFtrCarveLab.$f[$_]" })
}
$extra += @(0..3 | ForEach-Object { "gNdsFtrCarveLab.census_first[$_]" })
# the exact-key shadow exists only in a NDS_TEX_IDENT_SHADOW=1 lab build
$elfPath = Join-Path $root "builds\$Build\$Target.elf"
$nmExe = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$hasShadow = [bool](& $nmExe $elfPath | Select-String -SimpleMatch ' gNdsTexIdentShadowOn' -Quiet)
if (($Shadow -ne 0) -and (-not $hasShadow)) { throw "-Shadow needs a NDS_TEX_IDENT_SHADOW=1 build; $elfPath has no shadow" }
if ($hasShadow) {
    foreach ($f in 'checks', 'collisions', 'shadow_overflow', 'journal_count', 'journal_overflow', 'journal_collisions') {
        $extra += "gNdsTexIdentLab.$f"
    }
    $extra += @(0..3 | ForEach-Object { "gNdsTexIdentLab.collision_first[$_]" })
    $extra += 'gNdsTexIdentShadowOn'
}
$extra += @('gNdsFtrAdmitIdentCensus', 'gNdsRendererTextureKeyPoolEntriesHighWater')
# slice 4 (lab): texture ids past a material's sprite list
# slice 4: the lean path counters (gNdsFtrLean), every scalar and array field
# the ELF has (read from the ELF's DWARF via gdb ptype at build time would be
# nicer; the list below is kept in step with include/nds/renderer_fighter_lean.h)
$leanScalars = @('draws', 'shadow_runs', 'attempts', 'entry_hits', 'entry_switches', 'materializations',
    'materialize_ticks', 'materialize_list_ticks', 'materialize_words', 'materialize_words_max', 'materialize_roots',
    'materialize_textures', 'materialize_tint_binds', 'materialize_tint_pending', 'event_ticks', 'region_takes',
    'kernel_joints', 'kernel_slow_joints', 'kernel_fail',
    'head_ticks', 'guard_ticks', 'kernel_ticks', 'patch_ticks', 'submit_ticks', 'dma_wait_ticks', 'dma_wait_spins',
    'oracle_runs', 'oracle_words', 'oracle_clip_roots', 'oracle_donor_memo', 'oracle_key_moved', 'oracle_unconsumed',
    'oracle_source_miss', 'tint_patch_binds', 'tint_patch_moved', 'tint_patch_miss', 'tint_patch_white', 'tint_patch_shape',
    'packet_dma_waits', 'packet_dma_wait_ticks', 'ge_busy_samples', 'ge_busy_hits',
    'flush_bytes', 'light_patches', 'texgen_patches', 'pinned_residency_miss', 'pre_checks', 'pre_skips', 'book_ticks',
    'ident_watch_miss', 'projection_patches', 'retuples', 'kernel_rebuilds', 'kernel_part_joints',
    'key_events', 'key_seen_before', 'key_distinct',
    'variants_learned', 'variant_switches', 'variant_evictions', 'variant_diff_words', 'variant_diff_max',
    'tint_repatches', 'rerecord_resets', 'verify_runs', 'verify_variant_runs')
foreach ($i in 0..3) { $extra += "gNdsFtrLean.variant_reject[$i]" }
foreach ($i in 0..3) { $extra += "gNdsFtrLean.verify_mismatch[$i]" }
foreach ($i in 0..3) { $extra += "gNdsFtrLean.verify_first[$i]" }
foreach ($i in 0..3) { $extra += "gNdsFtrLean.materialize_why[$i]" }
$extra += @('gNdsTaskmanArenaChosenSize', 'gNdsTaskmanArenaRefineBytes', 'gNdsTaskmanArenaAllocFailCount')
foreach ($i in 0..3) { $extra += "gNdsFtrLean.kernel_part_ticks[$i]" }
foreach ($i in 0..7) { $extra += "gNdsFtrLean.materialize_part_ticks[$i]" }
foreach ($i in 0..5) { $extra += "gNdsFtrLean.materialize_key_miss[$i]" }
foreach ($f in $leanScalars) { $extra += "gNdsFtrLean.$f" }
foreach ($i in 0..19) { $extra += "gNdsFtrLean.decline[$i]" }
foreach ($i in 0..10) { $extra += "gNdsFtrLean.event[$i]" }
foreach ($i in 0..7) { $extra += "gNdsFtrLean.oracle_mismatch[$i]" }
foreach ($i in 0..1) { $extra += "gNdsFtrLean.oracle_clip_max[$i]" }
foreach ($i in 0..1) { $extra += "gNdsFtrLean.oracle_record_under_hit[$i]" }
foreach ($i in 0..7) { $extra += "gNdsFtrLean.oracle_record_diff[$i]" }
foreach ($i in 0..7) { $extra += "gNdsFtrLean.oracle_first[$i]" }
foreach ($i in 0..3) { $extra += "gNdsFtrLean.tint_rerecords[$i]" }
foreach ($i in 0..7) { $extra += "gNdsFtrLean.guard_part_ticks[$i]" }
foreach ($i in 0..5) { $extra += "gNdsFtrLean.patch_part_ticks[$i]" }
foreach ($i in 0..3) { $extra += "gNdsFtrLean.submit_part_ticks[$i]" }
# slice 4: per kind (0 Donkey, 1 Samus, 2 Link, 3 Kirby)
foreach ($f in 'k_attempts', 'k_draws', 'k_materializations', 'k_head_ticks', 'k_guard_ticks', 'k_kernel_ticks',
    'k_patch_ticks', 'k_submit_ticks', 'k_dma_wait_ticks', 'k_event_ticks', 'k_materialize_ticks', 'k_words_max',
    'k_kernel_joints', 'k_high_draws', 'k_oracle_runs', 'k_oracle_donor_memo',
    'k_key_events', 'k_key_seen_before', 'k_variants_learned', 'k_variant_switches') {
    $extra += @(0..3 | ForEach-Object { "gNdsFtrLean.$f[$_]" })
}
foreach ($k in 0..3) {
    foreach ($i in 0..19) { $extra += "gNdsFtrLean.k_decline[$k][$i]" }
    foreach ($i in 0..10) { $extra += "gNdsFtrLean.k_event[$k][$i]" }
    foreach ($i in 0..7) { $extra += "gNdsFtrLean.k_kernel_class[$k][$i]" }
    foreach ($i in 0..15) { $extra += "gNdsFtrLean.k_program_draws[$k][$i]" }
    foreach ($i in 0..7) { $extra += "gNdsFtrLean.k_oracle_mismatch[$k][$i]" }
    foreach ($i in 0..5) { $extra += "gNdsFtrLean.k_key_miss[$k][$i]" }
    foreach ($i in 0..3) { $extra += "gNdsFtrLean.k_why[$k][$i]" }
}
$extra += @('gNdsFtrLeanSlow', 'gNdsFtrPlanVerifyRuns', 'gNdsFtrPlanVerifyMismatch', 'gNdsFtrPlanHit', 'gNdsFtrPlanBuild')
$extra += @(
    'gNdsFighterMarioFoxDLAllDrawCount', 'gNdsR2GxComposeCaptures', 'gNdsR2GxComposeDeclines')
foreach ($i in 0..7) { $extra += "gNdsFighterPacketMissWord[$i]" }
$extra += @('gNdsNativeFailureLabCount')
foreach ($i in 0..3) { foreach ($w in 0..4) { $extra += "gNdsNativeFailureLab[$i][$w]" } }
foreach ($f in 'key_count', 'key_overflow', 'key_uploads', 'key_uploads_after_go', 'key_prov_fail') {
    $extra += @(0..3 | ForEach-Object { "gNdsVramCensus.$f[$_]" })
}
if ($Diag -ne 0) {
    $extra += @(0..3 | ForEach-Object { "gNdsFtrAdmitLab.diag_state[$_]" })
    $extra += @(0..3 | ForEach-Object { "gNdsFtrAdmitLab.diag_same_image[$_]" })
    foreach ($slot in 0..3) {
        foreach ($k in 0..58) {
            $extra += "gNdsFtrAdmitLab.diag_outside_key[$slot][$k]"
            $extra += "gNdsFtrAdmitLab.diag_admit_key[$slot][$k]"
        }
    }
}
$perStop = @()
if ($Census -ne 0) {
    foreach ($s in 'go', 'burst', 'episode', 'after') {
        foreach ($f in 'frame', 'tex_bytes', 'tex_bytes_d', 'tex_usable', 'tex_free_usable', 'tex_largest_usable',
            'tex_free_runs_usable', 'pal_bytes', 'pal_free_usable', 'pal_largest_usable', 'vramcnt_abcd', 'dispcnt',
            'bg3_opaque') {
            $extra += "gNdsVramCensus.$s.$f"
        }
        foreach ($arr in 'bytes', 'bytes_d', 'count') {
            $extra += @(0..($buckets - 1) | ForEach-Object { "gNdsVramCensus.$s.$arr[$_]" })
        }
    }
    $extra += @('gNdsVramCensus.frames', 'gNdsVramCensus.captures', 'gNdsVramCensus.walk_guard_hits',
        'gNdsVramCensus.peak_tex_bytes', 'gNdsVramCensus.min_largest_free', 'gNdsVramCensus.min_largest_free_frame',
        'gNdsVramCensusSiteCount')
    $extra += @(0..29 | ForEach-Object { "gNdsVramCensusSitePc[$_]" })
    $perStop = @('gNdsVramCensus.live.frame', 'gNdsVramCensus.live.tex_bytes', 'gNdsVramCensus.live.tex_bytes_d',
        'gNdsVramCensus.live.tex_usable', 'gNdsVramCensus.live.tex_free_usable', 'gNdsVramCensus.live.tex_largest_usable',
        'gNdsVramCensus.live.tex_free_runs_usable', 'gNdsVramCensus.live.pal_bytes',
        'gNdsFtrAdmitLab.outside[0]', 'gNdsFtrAdmitLab.outside[1]', 'gNdsFtrAdmitLab.outside[2]', 'gNdsFtrAdmitLab.outside[3]',
        'gNdsRendererNativeFailure.count', 'gNdsFtrAdmitDynFull')
    $perStop += @(0..($buckets - 1) | ForEach-Object { "gNdsVramCensus.live.bytes_d[$_]" })
}
$sets = @("gNdsFtrLeanRoute=$Route", "gNdsFtrLeanAdmit=$Admit", "gNdsFtrAdmitIdentCensus=$IdCensus", "gNdsFtrLeanSlow=$Slow")
if ($hasShadow) { $sets += "gNdsTexIdentShadowOn=$Shadow" }
if ($Census -ne 0) { $sets += "gNdsVramCensusEnable=$Census" }
$rows = Join-Path $art "$Arm-rows.csv"
$json = Join-Path $art "$Arm.json"
$log = Join-Path $art "$Arm-run.log"
$start = Get-Date
$argsList = @('-NoProfile', '-File', (Join-Path $root 'scripts\sample-tick-hud-buckets.ps1'), '-NoBuild',
    '-Target', $Target, '-Build', $Build, '-RingDump',
    '-Samples', "$Samples", '-StartFrame', "$StartFrame", '-SetGlobals', ($sets -join ','), '-ExtraGlobals', ($extra -join ','),
    '-RowsCsv', $rows, '-JsonOut', $json, '-RunnerSlot', '9', '-GdbPort', '3423', '-TimeoutSeconds', '3600')
if ($perStop.Count -ne 0) { $argsList += @('-PerStopGlobals', ($perStop -join ',')) }
& pwsh @argsList *> $log
$code = $LASTEXITCODE
Add-Content -LiteralPath $log -Value ("`nRUN_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds + " extras=" + $extra.Count + " perstop=" + $perStop.Count)
