param(
    [ValidateSet('Full','Latest','LatestFast','BoundaryDirect','Boundary','Regression','RegressionFast','Smoke','SmokeFast','Fighter','Direct','MenuChain')]
    [string]$Profile = 'Boundary',
    [string[]]$Only,
    [string]$From,
    [switch]$Force,
    [string]$TimingPath
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$plan = @(Get-Smash64DSVerifyPlan -Profile $Profile -Only $Only -From $From)
if ($plan.Count -eq 0) {
    throw "No verifiers selected for profile '$Profile'."
}
$builtDefault = $false
$seen = @{}
$timings = @()
$totalStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
foreach ($record in $plan) {
    if (-not $record.Target -or -not $record.Build -or -not $record.Harness) {
        if (-not $builtDefault) {
            Write-Output 'Building default verification ROM.'
            $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
            & make -C $root TARGET=smash64ds BUILD=build NDS_DEV_SCENE_HARNESS=normal -B -j16
            $stopwatch.Stop()
            $timings += [PSCustomObject]@{
                name = 'default'
                target = 'smash64ds'
                build = 'build'
                harness = 'normal'
                durationSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 3)
                exitCode = $LASTEXITCODE
            }
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            $builtDefault = $true
        }
        continue
    }
    $key = "$($record.Target)|$($record.Build)|$($record.Harness)"
    if ($seen.ContainsKey($key)) {
        continue
    }
    $seen[$key] = $true
    Write-Output "Building verifier output: $($record.Name)"
    $makeArgs = @(
        '-C', $root,
        "TARGET=$($record.Target)",
        "BUILD=$($record.Build)",
        "NDS_DEV_SCENE_HARNESS=$($record.Harness)"
    )
    if ($Force) {
        $makeArgs += '-B'
    }
    $makeArgs += '-j16'
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    & make @makeArgs
    $stopwatch.Stop()
    $timings += [PSCustomObject]@{
        name = $record.Name
        target = $record.Target
        build = $record.Build
        harness = $record.Harness
        durationSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 3)
        exitCode = $LASTEXITCODE
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$totalStopwatch.Stop()
if ($TimingPath) {
    $resolvedTimingPath = if ([System.IO.Path]::IsPathRooted($TimingPath)) {
        $TimingPath
    } else {
        Join-Path $root $TimingPath
    }
    $timingDir = Split-Path -Parent $resolvedTimingPath
    if ($timingDir) {
        New-Item -ItemType Directory -Force -Path $timingDir | Out-Null
    }
    [PSCustomObject]@{
        generatedAt = (Get-Date).ToUniversalTime().ToString('o')
        profile = $Profile
        force = [bool]$Force
        totalSeconds = [Math]::Round($totalStopwatch.Elapsed.TotalSeconds, 3)
        count = $timings.Count
        results = $timings
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $resolvedTimingPath -Encoding UTF8
    Write-Output "Wrote build timing artifact: $resolvedTimingPath"
}
if ($timings.Count -gt 0) {
    $totalSeconds = [Math]::Round($totalStopwatch.Elapsed.TotalSeconds, 3)
    Write-Output ("Build profile timing: count={0} total={1:N2}s" -f $timings.Count, $totalSeconds)
    $timings | Sort-Object durationSeconds -Descending | Select-Object -First 10 name, durationSeconds | Format-Table -AutoSize
}
Write-Output "Built verifier profile '$Profile' outputs."
