param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Admit = 2,
    [int]$Route = 0,
    [string]$Build = 'build-p2p8-s6',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$Samples = 1972,
    [int]$StartFrame = 2,
    [int]$Slow = 0,
    # extra comma-free global names to read at the end (appended)
    [string[]]$Also = @()
)
# P2-2p8 Phase 1 slice 6 sampler wrapper (from slice 5's run-s5.ps1): runner
# slot 9 / GDB 3423 only. The words are poked at the first frame-complete
# marker (whole u32 words). Every counter name is checked against the ELF's
# symbols first (the lean counters are fields of gNdsFtrLean; the slice 5 lab
# attribution block gNdsFtrLeanAttr exists only in an NDS_FTR_LEAN_KTIME=1
# build), so a stale list fails before the run, not inside GDB.
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice6'
New-Item -ItemType Directory -Force -Path $art | Out-Null
$elfPath = Join-Path $root "builds\$Build\$Target.elf"
$nmExe = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$syms = & $nmExe $elfPath | ForEach-Object { ($_ -split '\s+')[-1] }
$has = @{}
foreach ($s in $syms) { $has[$s] = $true }
$extra = @()
foreach ($n in 'gNdsFtrLeanAdmitFail', 'gNdsRendererNativeFailure.count', 'gNdsRendererNativeFailure.status',
    'gNdsRendererNativeFailure.identity', 'gNdsRendererNativeFailure.reason',
    'gNdsRendererNativeDirectReject.count', 'gNdsFighterPacketHits', 'gNdsFighterPacketRecords',
    'gNdsFighterPacketFaults', 'gNdsFighterPacketDeclines', 'gNdsFtrLeanRoute', 'gNdsFtrLeanAdmit',
    'gNdsTaskmanGeneralHeapFreeMin', 'gNdsTaskmanLibcTopChunkMin', 'gNdsTaskmanLibcRuntimeHighWater',
    'gNdsTaskmanArenaChosenSize', 'gNdsTaskmanArenaRefineBytes', 'gNdsTaskmanArenaAllocFailCount',
    'gNdsFtrLeanSlow', 'gNdsFtrPlanVerifyRuns', 'gNdsFtrPlanVerifyMismatch', 'gNdsFtrPlanHit', 'gNdsFtrPlanBuild',
    'gNdsFighterMarioFoxDLAllDrawCount', 'gNdsFtrDrawMemoHits', 'gNdsFtrDrawMemoFills',
    'gNdsFtrDrawMemoBypass', 'gNdsFtrDrawMemoInvalidations', 'gNdsFtrDrawMemoBoundary',
    'gNdsVramBankDTakes', 'gNdsVramBankDReturns', 'gNdsVramBankDMissedExits',
    'gNdsRendererFoxGunDrawCount', 'gNdsRendererFoxGunTriangleCount') {
    $base = ($n -split '\.')[0]
    if ($has.ContainsKey($base)) { $extra += $n }
}
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
    'tint_repatches', 'rerecord_resets', 'verify_runs', 'verify_variant_runs',
    'plan_reuses', 'plan_reuse_restarts', 'verify_plan_runs', 'tint_rerecords_mirrored', 'fence_rerecords_mirrored',
    'fighter_uploads', 'fighter_uploads_after_go', 'go_frame')
$lean = @()
foreach ($f in $leanScalars) { $lean += "gNdsFtrLean.$f" }
foreach ($i in 0..3) { $lean += "gNdsFtrLean.tint_rerecords[$i]"; $lean += "gNdsFtrLean.variant_reject[$i]"; $lean += "gNdsFtrLean.verify_mismatch[$i]";
    $lean += "gNdsFtrLean.verify_first[$i]"; $lean += "gNdsFtrLean.materialize_why[$i]";
    $lean += "gNdsFtrLean.kernel_part_ticks[$i]"; $lean += "gNdsFtrLean.submit_part_ticks[$i]" }
foreach ($i in 0..7) { $lean += "gNdsFtrLean.materialize_part_ticks[$i]"; $lean += "gNdsFtrLean.oracle_mismatch[$i]";
    $lean += "gNdsFtrLean.oracle_record_diff[$i]"; $lean += "gNdsFtrLean.oracle_first[$i]";
    $lean += "gNdsFtrLean.guard_part_ticks[$i]" }
foreach ($i in 0..15) { $lean += "gNdsFtrLean.oracle_shade_first[$i]" }
foreach ($i in 0..5) { $lean += "gNdsFtrLean.materialize_key_miss[$i]"; $lean += "gNdsFtrLean.patch_part_ticks[$i]" }
foreach ($i in 0..19) { $lean += "gNdsFtrLean.decline[$i]" }
foreach ($i in 0..2) { $lean += "gNdsFtrLean.verify_plan_mismatch[$i]" }
foreach ($i in 0..10) { $lean += "gNdsFtrLean.event[$i]" }
foreach ($i in 0..1) { $lean += "gNdsFtrLean.oracle_clip_max[$i]"; $lean += "gNdsFtrLean.oracle_record_under_hit[$i]" }
foreach ($f in 'k_owner', 'k_attempts', 'k_draws', 'k_materializations', 'k_head_ticks', 'k_guard_ticks', 'k_kernel_ticks',
    'k_patch_ticks', 'k_submit_ticks', 'k_dma_wait_ticks', 'k_event_ticks', 'k_materialize_ticks', 'k_words_max',
    'k_kernel_joints', 'k_high_draws', 'k_oracle_runs', 'k_oracle_donor_memo',
    'k_key_events', 'k_key_seen_before', 'k_variants_learned', 'k_variant_switches') {
    $lean += @(0..3 | ForEach-Object { "gNdsFtrLean.$f[$_]" })
}
foreach ($k in 0..3) {
    foreach ($i in 0..19) { $lean += "gNdsFtrLean.k_decline[$k][$i]" }
    foreach ($i in 0..10) { $lean += "gNdsFtrLean.k_event[$k][$i]" }
    foreach ($i in 0..7) { $lean += "gNdsFtrLean.k_kernel_class[$k][$i]"; $lean += "gNdsFtrLean.k_oracle_mismatch[$k][$i]" }
    foreach ($i in 0..15) { $lean += "gNdsFtrLean.k_program_draws[$k][$i]" }
    foreach ($i in 0..5) { $lean += "gNdsFtrLean.k_key_miss[$k][$i]" }
    foreach ($i in 0..3) { $lean += "gNdsFtrLean.k_why[$k][$i]" }
}
if ($has.ContainsKey('gNdsFtrLean')) { $extra += $lean }
# slice 5: the lab attribution block (KTIME ROM only); every field name below
# is also in kinds5.py.
if ($has.ContainsKey('gNdsFtrLeanAttr')) {
    foreach ($f in 'kernel_warm_ticks', 'kernel_dcold_ticks', 'kernel_cold_ticks', 'kernel_attr_draws',
        'kernel_attr_joints', 'quiet_wait_ticks', 'quiet_draws', 'book_ticks', 'kernel_part_joints') {
        $extra += @(0..3 | ForEach-Object { "gNdsFtrLeanAttr.$f[$_]" })
    }
    foreach ($k in 0..3) {
        foreach ($i in 0..3) { $extra += "gNdsFtrLeanAttr.head_part_ticks[$k][$i]" }
        foreach ($i in 0..3) { $extra += "gNdsFtrLeanAttr.kernel_part_ticks[$k][$i]" }
        foreach ($i in 0..3) { $extra += "gNdsFtrLeanAttr.submit_part_ticks[$k][$i]" }
        foreach ($i in 0..7) { $extra += "gNdsFtrLeanAttr.guard_part_ticks[$k][$i]" }
        foreach ($i in 0..7) { $extra += "gNdsFtrLeanAttr.patch_part_ticks[$k][$i]" }
    }
    foreach ($i in 0..7) { $extra += "gNdsFtrLeanAttr.event_part_ticks[$i]"; $extra += "gNdsFtrLeanAttr.event_part_count[$i]" }
}
foreach ($n in $Also) { $extra += $n }
# drop names whose base symbol is missing (a pre-slice-5 ROM has no k_guard_part_ticks):
# gdb would fail the whole read otherwise. Field presence is checked by a
# dry gdb ptype below.
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$ptype = & $gdb -batch -ex "ptype gNdsFtrLean" $elfPath 2>$null
$ptypeAttr = if ($has.ContainsKey('gNdsFtrLeanAttr')) { & $gdb -batch -ex "ptype gNdsFtrLeanAttr" $elfPath 2>$null } else { @() }
$fieldsLean = @{}
foreach ($l in $ptype) { if ($l -match '\s(\w+)(\[|;)') { $fieldsLean[$Matches[1]] = $true } }
$fieldsAttr = @{}
foreach ($l in $ptypeAttr) { if ($l -match '\s(\w+)(\[|;)') { $fieldsAttr[$Matches[1]] = $true } }
$kept = @()
$dropped = 0
foreach ($n in $extra) {
    if ($n -match '^gNdsFtrLean\.(\w+)') { if (-not $fieldsLean.ContainsKey($Matches[1])) { $dropped++; continue } }
    if ($n -match '^gNdsFtrLeanAttr\.(\w+)') { if (-not $fieldsAttr.ContainsKey($Matches[1])) { $dropped++; continue } }
    $kept += $n
}
$sets = @("gNdsFtrLeanRoute=$Route", "gNdsFtrLeanAdmit=$Admit", "gNdsFtrLeanSlow=$Slow")
$rows = Join-Path $art "$Arm-rows.csv"
$json = Join-Path $art "$Arm.json"
$log = Join-Path $art "$Arm-run.log"
$start = Get-Date
$argsList = @('-NoProfile', '-File', (Join-Path $root 'scripts\sample-tick-hud-buckets.ps1'), '-NoBuild',
    '-Target', $Target, '-Build', $Build, '-RingDump',
    '-Samples', "$Samples", '-StartFrame', "$StartFrame", '-SetGlobals', ($sets -join ','), '-ExtraGlobals', ($kept -join ','),
    '-RowsCsv', $rows, '-JsonOut', $json, '-RunnerSlot', '9', '-GdbPort', '3423', '-TimeoutSeconds', '3600')
& pwsh @argsList *> $log
$code = $LASTEXITCODE
$romSha = (Get-FileHash -LiteralPath (Join-Path $root "builds\$Build\$Target.nds") -Algorithm SHA256).Hash
Add-Content -LiteralPath $log -Value ("`nRUN_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds + " extras=" + $kept.Count + " dropped=$dropped rom=$romSha")
