param([string]$Build = 'build-prof-s67', [string]$Tag = 's67p', [int[]]$Slots = @(1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
      [string]$Set = 'all', [int]$StartFrame = 200, [int]$Frames = 384, [switch]$NoSplit)
# P0's profile census (docs/P8/P8_PLAN.md section 5) on a profiling ROM built
# with the census window (build_gate_rom.ps1 -Extra NDS_TASK37_PROFILE=1,
# NDS_BATTLE_PROFILE=1, NDS_TASK37_PROFILE_START=200, NDS_TASK37_PROFILE_FRAMES=384,
# and NDS_TASK37_PROFILE_PER_FRAME_REGION=1 unless -NoSplit; the census refuses
# another window): the cast and the heavy roster on all sixteen VS stages, the
# four mixes on Hyrule Temple and Saffron City. Each run is one
# run-task37-profile-census.ps1 on its own runner slot (starts 30 s apart),
# split over the gate (per-function totals only with -NoSplit: the per-frame
# regions' DTCM words do not fit beside the ship layout's), written to
# artifacts/performance/<date>_<Tag>-<run>-g<gkind>-profile/. -Set picks
# 'cast', 'heavy', 'mixes', 'all' or 'everyone' (the twelve original fighters
# four a roster with items on, on Hyrule Temple, Saffron City and Congo
# Jungle). One line a run in
# builds/probes/profile-<Tag>.out.
. "$PSScriptRoot\common.ps1"
$stages = @(0, 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 16, 41, 42, 43, 44)
function Word([int[]]$v) { $w = 0; for ($i = 0; $i -lt 4; $i++) { $w = $w -bor ($v[$i] -shl (8 * $i)) }; [string]([uint32]$w) }
$runs = @()
if ($Set -in 'all', 'cast') { foreach ($g in $stages) { $runs += @{ Name = "cast-g$g"; G = $g; Kinds = '4294967295'; Content = '0' } } }
if ($Set -in 'all', 'heavy') { foreach ($g in $stages) { $runs += @{ Name = "heavy-g$g"; G = $g; Kinds = (Word @(0, 1, 0, 1)); Content = (Word @(9, 13, 17, 20)) } } }
if ($Set -eq 'everyone') {
    # The twelve original fighters, four a roster, items on at the very-high
    # rate: R1's cold set must not take a fighter's or an item's draw code
    # for cold just because the cast and heavy rosters lack them.
    foreach ($roster in @(@(0, 1, 2, 3), @(4, 5, 6, 7), @(8, 9, 10, 11))) { foreach ($g in 43, 7, 2) {
        $runs += @{ Name = "f$($roster[0])-g$g"; G = $g; Kinds = (Word $roster); Content = '0'; Items = $true } } }
}
if ($Set -in 'all', 'mixes') {
    $mixes = @{ A = @(@(6, 0, 7, 7), @(4, 9, 19, 14)); B = @(@(1, 0, 10, 1), @(13, 10, 16, 3));
                C = @(@(7, 1, 7, 8), @(2, 8, 18, 0)); D = @(@(7, 7, 7, 0), @(5, 6, 11, 17)) }
    foreach ($m in 'A', 'B', 'C', 'D') { foreach ($g in 43, 7) {
        $runs += @{ Name = "mix$m-g$g"; G = $g; Kinds = (Word $mixes[$m][0]); Content = (Word $mixes[$m][1]) } } }
}
$out = Join-Path $S64Probes "profile-$Tag.out"
$date = Get-Date -Format 'yyyy-MM-dd'
$queue = [System.Collections.Queue]::new($runs)
$jobs = @{}
while ($queue.Count -gt 0 -or $jobs.Count -gt 0) {
    foreach ($slot in $Slots) {
        if ($jobs.ContainsKey($slot)) {
            if ($jobs[$slot].State -in 'Completed', 'Failed', 'Stopped') {
                Receive-Job $jobs[$slot] 2>&1 | ForEach-Object { "$_" } | Add-Content $out
                Remove-Job $jobs[$slot]
                $jobs.Remove($slot)
            } else { continue }
        }
        if ($queue.Count -eq 0) { continue }
        $r = $queue.Dequeue()
        $dir = Join-Path $S64Repo "artifacts\performance\$($date)_$Tag-$($r.Name)-profile"
        $jobs[$slot] = Start-Job -ArgumentList $S64Repo, $Build, $r, $dir, $slot, $StartFrame, $Frames, ([bool]$NoSplit) -ScriptBlock {
            param($repo, $build, $r, $dir, $slot, $start, $frames, $nosplit)
            New-Item -ItemType Directory -Force $dir | Out-Null
            $log = Join-Path $dir 'run.log'
            $split = if ($nosplit) { @{ PerFrameRegion = $false } } else { @{ SplitOverGate = $true } }
            try {
                & (Join-Path $repo 'scripts\run-task37-profile-census.ps1') -Build $build -NoBuild `
                    -Target smash64ds-p2-fourcpu-tickhud-hwtri -StartFrame $start -Frames $frames -Top 60 `
                    -OutDir $dir -RunnerSlot $slot @split `
                    -BootSetGlobals (@("gNdsLabFourCpuGkind=$($r.G)", "gNdsLabFourCpuKinds=$($r.Kinds)", "gNdsLabP4Content=$($r.Content)") +
                                     $(if ($r.Items) { @('gNdsLabItemRate=5') } else { @() })) *> $log
                "$($r.Name) slot=$slot exit=$LASTEXITCODE census=$(Test-Path (Join-Path $dir 'census.json'))"
            } catch {
                "FAILED $($_.Exception.Message)" | Add-Content $log
                "$($r.Name) slot=$slot FAILED: $($_.Exception.Message)"
            }
        }
        Start-Sleep -Seconds 30
    }
    Start-Sleep -Seconds 10
}
"PROFILE SET DONE $Tag" | Add-Content $out
