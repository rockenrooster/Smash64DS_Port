param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Admit = 0,
    [int]$Route = 0,
    [int]$Census = 0,
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$Samples = 1972,
    [int]$Diag = 0,
    [int]$Shadow = 0,
    [int]$IdCensus = 0
)
# P2-2p8 Phase 1 slice 2c sampler wrapper: runner slot 9 / GDB 3423 only.
# The words are poked at the first frame-complete marker (whole u32 words).
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
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
foreach ($i in 0..7) { foreach ($w in 0..2) { $extra += "gNdsFtrAdmitLab.outside_first[$i][$w]" } }
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
# slice 2c (lab): carved residency and the identity's collision checks
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
foreach ($i in 0..7) { foreach ($w in 0..3) { $extra += "gNdsFtrOutsideLr[$i][$w]" } }
# slice 2c (lab): texture ids past a material's sprite list
$extra += @('gNdsNativeFailureLabCount')
foreach ($i in 0..15) { foreach ($w in 0..4) { $extra += "gNdsNativeFailureLab[$i][$w]" } }
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
$sets = @("gNdsFtrLeanRoute=$Route", "gNdsFtrLeanAdmit=$Admit", "gNdsFtrAdmitIdentCensus=$IdCensus")
if ($hasShadow) { $sets += "gNdsTexIdentShadowOn=$Shadow" }
if ($Census -ne 0) { $sets += "gNdsVramCensusEnable=$Census" }
$rows = Join-Path $art "$Arm-rows.csv"
$json = Join-Path $art "$Arm.json"
$log = Join-Path $art "$Arm-run.log"
$start = Get-Date
$argsList = @('-NoProfile', '-File', (Join-Path $root 'scripts\sample-tick-hud-buckets.ps1'), '-NoBuild',
    '-Target', $Target, '-Build', $Build, '-RingDump',
    '-Samples', "$Samples", '-StartFrame', '2', '-SetGlobals', ($sets -join ','), '-ExtraGlobals', ($extra -join ','),
    '-RowsCsv', $rows, '-JsonOut', $json, '-RunnerSlot', '9', '-GdbPort', '3423', '-TimeoutSeconds', '3600')
if ($perStop.Count -ne 0) { $argsList += @('-PerStopGlobals', ($perStop -join ',')) }
& pwsh @argsList *> $log
$code = $LASTEXITCODE
Add-Content -LiteralPath $log -Value ("`nRUN_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds + " extras=" + $extra.Count + " perstop=" + $perStop.Count)
