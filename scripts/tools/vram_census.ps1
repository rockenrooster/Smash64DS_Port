param([string]$Build = 'build-sweep-s67', [int[]]$Gkinds = @(43), [string]$Kinds = '16777472', [string]$Content = '336661769',
      [string]$Tag = 'vram', [int]$Slot = 11, [int]$Frames = 2400, [int]$Seconds = 240, [int]$Reserve = 0, [switch]$After,
      [switch]$NoCensus,
      [string[]]$Counters = @('gNdsParticleQuadResidentSheets', 'gNdsParticleAnimSheetFailCount',
                              'gNdsLabParticleReserveTaken', 'gNdsParticleQuadMissCount', 'gNdsRendererParticleAtlasFailCount',
                              'gNdsRendererBattleStaticTextureViolationCount', 'gNdsR2StagePrepareBuildCount',
                              'gNdsStageGCDrawAllLoopHardwareTextureRejectCount',
                              'gNdsFighterDLAllDrawHardwareTextureRejectCount'))
# Texture VRAM over a four-CPU match on a gate ROM built with
# -Extra NDS_DIAG_COUNTERS=1 (P8 V0): the renderer's census (gNdsVramCensus,
# enabled at boot) read once the match has run -Frames presented frames --
# free bytes and the largest free run inside the texture banks at GO, now and
# at their minimum, the peak texel and palette bytes, the free-run count --
# then each of -Counters (the 2026-08-02 failure modes: a quad miss, an atlas
# refusal, a static texture violation, stage prepare rebuilds, stage and
# fighter texture rejects). -Reserve N takes N more 8 KiB texture allocations
# right after the particle sheets (gNdsLabParticleReserveSheets): how many
# more sheets fit before those counters move; with -After they are taken when
# the countdown's atlases are released instead, evicting texture cache entries.
# -NoCensus leaves the walk off (the census fields read 0) and keeps only the
# counters: the walk every frame end slows the heavier stages several times. Defaults: the heavy roster (kinds 0,1,0,1,
# contents 9,13,17,20) on Hyrule Temple. One line a stage in
# builds/probes/vram-<Tag>.out.
. "$PSScriptRoot\common.ps1"
$out = Join-Path $S64Probes "vram-$Tag.out"
foreach ($g in $Gkinds) {
    $name = "$Tag-g$g"
    $fmt = 'X vram g=' + $g + ' reserve=' + $Reserve + $(if ($After) { 'a' } else { '' }) + ' frame=%u go_free=%u go_largest=%u live_free=%u live_largest=%u live_runs=%u usable=%u ' +
           'min_largest=%u at=%u after_largest=%u after_free=%u episode_largest=%u peak_tex=%u peak_pal=%u names=%u limit=%u' +
           (($Counters | ForEach-Object { " $_=%u" }) -join '') + '\n'
    # One string before the array: inside @( ) a line that ends in `+` starts
    # a new element instead of continuing this one.
    $printf = "printf `"$fmt`", gNdsFrameCounter, gNdsVramCensus.go.tex_free_usable, gNdsVramCensus.go.tex_largest_usable, " +
        'gNdsVramCensus.live.tex_free_usable, gNdsVramCensus.live.tex_largest_usable, gNdsVramCensus.live.tex_free_runs_usable, ' +
        'gNdsVramCensus.live.tex_usable, gNdsVramCensus.min_largest_free, gNdsVramCensus.min_largest_free_frame, ' +
        'gNdsVramCensus.after.tex_largest_usable, gNdsVramCensus.after.tex_free_usable, gNdsVramCensus.episode.tex_largest_usable, ' +
        'gNdsVramCensus.peak_tex_bytes, gNdsVramCensus.peak_pal_bytes, gNdsVramCensus.live.tex_names, gNdsVramCensus.name_limit' +
        (($Counters | ForEach-Object { ", $_" }) -join '')
    # One stop: the ROM calls ndsLabCensusFrameHook at gNdsLabCensusFrame (a
    # conditional breakpoint every frame killed the heavier stages' runs).
    $lines = @('break ndsLabCensusFrameHook', 'commands', 'silent', $printf,
               'delete', 'detach', 'quit', 'end')
    & "$PSScriptRoot\probe_session.ps1" -Build $Build -Rom 'smash64ds-p2-fourcpu-tickhud-hwtri' -Slot $Slot -Tag $name `
        -Seconds $Seconds -Pokes @("set var gNdsLabFourCpuGkind = $g", "set var gNdsLabFourCpuKinds = $Kinds",
                                   "set var gNdsLabP4Content = $Content",
                                   "set var gNdsVramCensusEnable = $(if ($NoCensus) { 0 } else { 1 })",
                                   "set var gNdsLabParticleReserveSheets = $Reserve",
                                   "set var gNdsLabParticleReserveAfterCountdown = $([int][bool]$After)",
                                   "set var gNdsLabCensusFrame = $Frames") `
        -Lines $lines -StopWithGdb | Out-Null
    $log = Join-Path $S64Probes "$name.log"
    $hit = Select-String -Path $log -Pattern '^X vram ' -ErrorAction SilentlyContinue | Select-Object -Last 1
    if ($hit) { $hit.Line | Add-Content $out } else { "g=$g NO RECORD" | Add-Content $out }
}
"VRAM DONE $Tag" | Add-Content $out
