param(
    # The sweep tag whose CSVs are the replay digest reference: the heavy
    # roster on runner slots 1-3 = Hyrule Temple, Congo Jungle, Yoshi's Island
    # (artifacts/performance/p8-sweep/<Reference>-<slot>-r9-13-17-20-g<gkind>.csv,
    # as a four-stage gate sweep writes them). Without it the digest step is
    # skipped and the summary says so.
    [string]$Reference = '',
    # Steps to leave out: any of host, configs, digests.
    [string[]]$Skip = @(),
    [int]$Workers = 6,
    # The persistent build directories are builds/<Prefix>-<config>.
    [string]$Prefix = 'qc'
)
# P8 H-2: the checks a change passes before it is committed, in one command.
#  1. host: every tracked scripts/**/test_*.py (run_host_tests.py).
#  2. configs: the five configurations built incrementally into
#     builds/<Prefix>-<config> -- the play ROM (the ship link), the walk ROM,
#     the gate ROM, the profiling gate ROM and the wireless lab ROM -- each
#     seeded once from the newest build of its kind.
#  3. digests: the heavy roster on three stages on the gate ROM against the
#     reference sweep (scripts/compare-replay-digest.py): IDENTICAL or fail.
# Exit 0 only when every step that ran passed; the summary is
# builds/probes/quick-check.txt. Builds serialise on the tools' mutex, so this
# waits behind a running build; never edit sources while it runs.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
Set-Location $S64Repo
$summary = Join-Path $S64Probes 'quick-check.txt'
$lines = @("quick_check $(Get-Date -Format 's') HEAD $(git rev-parse --short HEAD)")
$failed = $false

function Step([string]$name, [bool]$ok, [string]$detail) {
    $script:lines += ('{0,-8} {1}  {2}' -f $name, $(if ($ok) { 'PASS' } else { 'FAIL' }), $detail)
    if (-not $ok) { $script:failed = $true }
}

function Newest([string]$filter) {
    Get-ChildItem builds -Directory -Filter $filter | Sort-Object LastWriteTime -Descending |
        Select-Object -First 1 | ForEach-Object { $_.Name }
}

function Seed([string]$build, [string]$filter) {
    if (Test-Path "builds/$build") { return '' }
    $seed = Newest $filter
    if ($null -eq $seed) { return '' }
    return $seed
}

if ($Skip -notcontains 'host') {
    $out = Join-Path $S64Probes 'quick-check-host.txt'
    $total = python scripts/tools/run_host_tests.py $out --workers $Workers
    Step 'host' ($LASTEXITCODE -eq 0) "$total ($out)"
}

$gate = "$Prefix-gate"
if ($Skip -notcontains 'configs') {
    $profile = @('NDS_TASK37_PROFILE=1', 'NDS_TASK37_PROFILE_START=800', 'NDS_TASK37_PROFILE_FRAMES=800',
                 'NDS_TASK37_PROFILE_PER_FRAME_REGION=1')
    $configs = @(
        @{ Name = 'play'; Script = 'build_play_rom.ps1'; Filter = 'play-p*'; Extra = @() },
        @{ Name = 'walk'; Script = 'build_walk_rom.ps1'; Filter = 'walk-u*'; Extra = @() },
        @{ Name = 'gate'; Script = 'build_gate_rom.ps1'; Filter = 'build-sweep-s*'; Extra = @() },
        @{ Name = 'prof'; Script = 'build_gate_rom.ps1'; Filter = 'build-prof-s*'; Extra = $profile },
        @{ Name = 'net'; Script = 'build_play_rom.ps1'; Filter = 'net-*';
           Extra = @('NDS_NET_LAB_LOBBY=1', 'NDS_NET_LAB_SWEEP=1') })
    foreach ($c in $configs) {
        $build = "$Prefix-$($c.Name)"
        $seed = Seed $build $c.Filter
        $t0 = Get-Date
        try {
            & "$PSScriptRoot\$($c.Script)" -Build $build -Seed $seed -Extra $c.Extra | Out-Null
            $ok = $true
            $detail = "builds/$build"
        } catch {
            $ok = $false
            $detail = "builds/$build: $($_.Exception.Message)"
        }
        $secs = [int]((Get-Date) - $t0).TotalSeconds
        Step $c.Name $ok "$detail ${secs}s$(if ($seed) { " (seeded from $seed)" })"
    }
}

if ($Skip -notcontains 'digests') {
    if ($Reference -eq '') {
        $lines += 'digests  SKIP  no -Reference sweep tag'
    } elseif (-not (Test-Path "builds/$gate")) {
        Step 'digests' $false "no gate ROM in builds/$gate"
    } else {
        $tag = "$Prefix-$(Get-Date -Format 'MMddHHmm')"
        $stages = @(43, 2, 5)
        $jobs = @()
        for ($i = 0; $i -lt $stages.Count; $i++) {
            $jobs += Start-Job -ArgumentList $S64Repo, $gate, $tag, $stages[$i], (1 + $i), (35 * $i) -ScriptBlock {
                param($repo, $build, $tag, $g, $slot, $delay)
                Start-Sleep -Seconds $delay
                & "$repo\scripts\tools\worst_case_sweep.ps1" -Build $build -Rosters '0,1,0,1/9,13,17,20' `
                    -Gkinds @($g) -Tag "$tag-$slot" -Slot $slot *> $null
            }
        }
        $jobs | Wait-Job | Remove-Job
        for ($i = 0; $i -lt $stages.Count; $i++) {
            $slot = 1 + $i
            $name = "r9-13-17-20-g$($stages[$i]).csv"
            $want = Join-Path $S64Repo "artifacts\performance\p8-sweep\$Reference-$slot-$name"
            $got = Join-Path $S64Repo "artifacts\performance\p8-sweep\$tag-$slot-$name"
            if (-not ((Test-Path $want) -and (Test-Path $got))) {
                Step "digest$slot" $false "missing $(if (-not (Test-Path $want)) { $want } else { $got })"
                continue
            }
            $verdict = python scripts/compare-replay-digest.py $want $got | Select-Object -Last 1
            Step "digest$slot" ($verdict -like 'IDENTICAL*') "g$($stages[$i]): $verdict"
        }
    }
}

$lines += $(if ($failed) { 'QUICK CHECK FAILED' } else { 'QUICK CHECK PASSED' })
$lines | Set-Content -LiteralPath $summary
$lines | ForEach-Object { $_ }
if ($failed) { exit 1 }
