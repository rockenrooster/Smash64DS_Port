param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Route = 0,
    [int]$Census = 1,
    [ValidateSet('a', 'b', 'none')][string]$Set = 'a'
)
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a'
New-Item -ItemType Directory -Force -Path $art | Out-Null
$buckets = 43
$snapFields = @('frame','tex_names','tex_bytes','tex_size_field','tex_free','tex_free_blocks','tex_largest_free',
    'tex_alloc_blocks','tex_alloc_bytes','tex_start','tex_end','tex_usable','tex_free_usable','tex_largest_usable',
    'tex_free_runs_usable','pal_names','pal_bytes','pal_free','pal_free_blocks',
    'pal_largest_free','pal_alloc_blocks','pal_alloc_bytes','pal_start','pal_end','pal_usable','pal_free_usable',
    'pal_largest_usable','pal_free_runs_usable','dispcnt','vramcnt_abcd',
    'vramcnt_efg','bg2_opaque','bg3_opaque','last_request')
function SnapNames([string]$snap) {
    $n = @($snapFields | ForEach-Object { "gNdsVramCensus.$snap.$_" })
    foreach ($arr in 'bytes', 'count', 'pal') {
        $n += @(0..($buckets - 1) | ForEach-Object { "gNdsVramCensus.$snap.$arr[$_]" })
    }
    return $n
}
$common = @('gNdsFtrLeanRoute', 'gNdsVramCensusEnable', 'gNdsVramCensus.frames', 'gNdsVramCensus.captures',
    'gNdsVramCensus.walk_guard_hits', 'gNdsVramCensus.name_limit', 'gNdsVramCensusNameOverflow',
    'gNdsRendererNativeFailure.count', 'gNdsRendererNativeDirectReject.count', 'gNdsFtrLean.go_frame',
    'gNdsFtrLean.fighter_uploads', 'gNdsFtrLean.fighter_uploads_after_go', 'gNdsFtrLean.fighter_upload_bytes_after_go',
    'gNdsFtrLean.reject_count')
$extra = @($common)
if ($Set -eq 'a') {
    foreach ($s in 'go', 'burst', 'episode', 'after') { $extra += SnapNames $s }
    $extra += @(0..($buckets - 1) | ForEach-Object { "gNdsVramCensus.peak_bytes[$_]" })
    $extra += @(0..($buckets - 1) | ForEach-Object { "gNdsVramCensus.peak_count[$_]" })
    $extra += @('gNdsVramCensus.peak_tex_bytes', 'gNdsVramCensus.peak_pal_bytes', 'gNdsVramCensus.min_largest_free',
        'gNdsVramCensus.min_largest_free_frame', 'gNdsVramCensus.bg_frame[0]', 'gNdsVramCensus.bg_frame[1]',
        'gNdsVramCensus.bg2_opaque[0]', 'gNdsVramCensus.bg2_opaque[1]', 'gNdsVramCensus.bg3_opaque[0]',
        'gNdsVramCensus.bg3_opaque[1]', 'gNdsVramCensusOverlayMask', 'gNdsVramCensusSiteCount')
    $extra += @(0..29 | ForEach-Object { "gNdsVramCensusSitePc[$_]" })
    $extra += @('gNdsOriginalSpriteBg2ClearBytes', 'gNdsOriginalSpriteBg2CopyBytes', 'gNdsOriginalSpriteBg2FinalWriteBytes',
        'gNdsOriginalSpriteBg3ClearBytes', 'gNdsOriginalSpriteBg3CopyBytes', 'gNdsOriginalSpriteBg3FinalWriteBytes',
        'gNdsOriginalSpritePreviewCommitCount', 'gNdsEntryEffectStartupTextureReleaseCount',
        'gNdsEntryEffectStartupTextureReleaseBytes', 'gNdsRendererAdapterOwnerSelectedRootsHighWater',
        'gNdsRendererAdapterOwnerMaterialsPerRootHighWater', 'gNdsTaskmanGeneralHeapFreeMin')
} elseif ($Set -eq 'b') {
    foreach ($f in 'key_count', 'key_overflow', 'key_uploads', 'key_uploads_after_go', 'key_prov_fail') {
        $extra += @(0..3 | ForEach-Object { "gNdsVramCensus.$f[$_]" })
    }
    foreach ($slot in 0..3) { foreach ($k in 0..47) { foreach ($w in 0..2) { $extra += "gNdsVramCensus.key[$slot][$k][$w]" } } }
}
$perStop = @()
if (($Census -ne 0) -and ($Set -eq 'a')) {
    $perStop = @('gNdsVramCensus.live.frame', 'gNdsVramCensus.live.tex_bytes', 'gNdsVramCensus.live.tex_free',
        'gNdsVramCensus.live.tex_largest_free', 'gNdsVramCensus.live.tex_free_blocks', 'gNdsVramCensus.live.tex_free_usable',
        'gNdsVramCensus.live.tex_largest_usable', 'gNdsVramCensus.live.tex_free_runs_usable', 'gNdsVramCensus.live.pal_bytes',
        'gNdsVramCensus.live.pal_free', 'gNdsVramCensus.live.pal_largest_free',
        'gNdsOriginalSpriteBg2FinalWriteBytes', 'gNdsOriginalSpriteBg3FinalWriteBytes',
        'gNdsOriginalSpriteBg2CopyBytes', 'gNdsOriginalSpriteBg3CopyBytes',
        'gNdsOriginalSpriteBg2ClearBytes', 'gNdsOriginalSpriteBg3ClearBytes')
    $perStop += @(0..($buckets - 1) | ForEach-Object { "gNdsVramCensus.live.bytes[$_]" })
}
$sets = @("gNdsFtrLeanRoute=$Route")
if ($Census -ne 0) { $sets += "gNdsVramCensusEnable=$Census" }
$rows = Join-Path $art "$Arm-rows.csv"
$json = Join-Path $art "$Arm.json"
$log = Join-Path $art "$Arm-run.log"
$start = Get-Date
$argsList = @('-NoProfile', '-File', (Join-Path $root 'scripts\sample-tick-hud-buckets.ps1'), '-NoBuild',
    '-Target', 'smash64ds-p2-fourcpu-tickhud-hwtri', '-Build', 'build-p2-fourcpu-tickhud', '-RingDump',
    '-Samples', '1972', '-StartFrame', '2', '-SetGlobals', ($sets -join ','), '-ExtraGlobals', ($extra -join ','),
    '-RowsCsv', $rows, '-JsonOut', $json, '-RunnerSlot', '9', '-GdbPort', '3423', '-TimeoutSeconds', '3600')
if ($perStop.Count -ne 0) { $argsList += @('-PerStopGlobals', ($perStop -join ',')) }
& pwsh @argsList *> $log
$code = $LASTEXITCODE
Add-Content -LiteralPath $log -Value ("`nRUN_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds + " extras=" + $extra.Count + " perstop=" + $perStop.Count)
