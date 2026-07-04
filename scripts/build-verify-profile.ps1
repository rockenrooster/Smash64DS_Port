param(
    [ValidateSet('Full','Latest','LatestFast','BoundaryDirect','Boundary','Regression','RegressionFast','Smoke','SmokeFast','Fighter','Direct','MenuChain')]
    [string]$Profile = 'Boundary',
    [string[]]$Only,
    [string]$From,
    [switch]$Force,
    [switch]$NoSharedBuild,
    [int]$ParallelBuilds = 0,
    [int]$ParallelBuildJobs = 0,
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
$profileKey = $Profile.ToLowerInvariant()
$sharedBuild = "build-verify-$profileKey-shared"
$optSizeSharedBuild = "build-verify-$profileKey-optsize-shared"
$optSizeHarnessModes = @(161, 162, 163)
if ($ParallelBuilds -le 0) {
    $ParallelBuilds = if ($Profile -in @('Full','Regression')) { 4 } else { 1 }
}
if ($ParallelBuildJobs -le 0) {
    $ParallelBuildJobs = if ($ParallelBuilds -gt 1) { 4 } else { 16 }
}
$seen = @{}
$needsDefault = $false
$buildRecords = @()
foreach ($record in $plan) {
    if (-not $record.Target -or -not $record.Build -or -not $record.Harness) {
        $needsDefault = $true
        continue
    }
    $key = "$($record.Target)|$($record.Build)|$($record.Harness)"
    if ($seen.ContainsKey($key)) {
        continue
    }
    $seen[$key] = $true
    $buildRecords += $record
}
$timings = @()
$totalStopwatch = [System.Diagnostics.Stopwatch]::StartNew()

if ($needsDefault) {
    Write-Output 'Building default verification ROM.'
    $makeArgs = @(
        '-C', $root,
        'TARGET=smash64ds',
        'BUILD=build',
        'NDS_DEV_SCENE_HARNESS=normal'
    )
    if ($Force) {
        $makeArgs += '-B'
    }
    $makeArgs += "-j$ParallelBuildJobs"
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    & make @makeArgs
    $stopwatch.Stop()
    $timings += [PSCustomObject]@{
        name = 'default'
        target = 'smash64ds'
        build = 'build'
        registryBuild = 'build'
        harness = 'normal'
        sharedBuild = $false
        worker = $null
        durationSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 3)
        exitCode = $LASTEXITCODE
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (($ParallelBuilds -le 1) -or ($buildRecords.Count -le 1)) {
    $forcedSharedBuilds = @{}
    foreach ($record in $buildRecords) {
        $build = $record.Build
        $usesSharedBuild = $false
        if (-not $NoSharedBuild) {
            $rendererSuffix = if ($record.Target -like '*-hwtri') { '-hwtri' } else { '' }
            if ($optSizeHarnessModes -contains [int]$record.Mode) {
                $build = "$optSizeSharedBuild$rendererSuffix"
            } else {
                $build = "$sharedBuild$rendererSuffix"
            }
            $usesSharedBuild = $true
        }
        Write-Output "Building verifier output: $($record.Name)"
        $makeArgs = @(
            '-C', $root,
            "TARGET=$($record.Target)",
            "BUILD=$build",
            "NDS_DEV_SCENE_HARNESS=$($record.Harness)"
        )
        if ($record.Target -like '*-hwtri') {
            $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
        }
        $forceThisBuild = [bool]$Force
        if ($usesSharedBuild -and $Force) {
            if ($forcedSharedBuilds.ContainsKey($build)) {
                $forceThisBuild = $false
            } else {
                $forcedSharedBuilds[$build] = $true
            }
        }
        if ($forceThisBuild) {
            $makeArgs += '-B'
        }
        $makeArgs += "-j$ParallelBuildJobs"
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        & make @makeArgs
        $stopwatch.Stop()
        $timings += [PSCustomObject]@{
            name = $record.Name
            target = $record.Target
            build = $build
            registryBuild = $record.Build
            harness = $record.Harness
            sharedBuild = $usesSharedBuild
            worker = $null
            durationSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 3)
            exitCode = $LASTEXITCODE
        }
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
} else {
    Write-Output "Building $($buildRecords.Count) verifier outputs with $ParallelBuilds parallel workers (-j$ParallelBuildJobs each)."
    $logDir = Join-Path $root 'artifacts\verifier-cost\build-logs'
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    $jobs = @()
    for ($worker = 0; $worker -lt $ParallelBuilds; $worker++) {
        $slice = @()
        for ($i = $worker; $i -lt $buildRecords.Count; $i += $ParallelBuilds) {
            $slice += $buildRecords[$i]
        }
        if ($slice.Count -eq 0) {
            continue
        }
        $sliceJson = $slice | ConvertTo-Json -Depth 5 -Compress
        $jobs += Start-Job -ArgumentList @(
            $root,
            $sliceJson,
            $profileKey,
            [bool]$Force,
            [bool]$NoSharedBuild,
            $worker,
            $ParallelBuildJobs,
            $logDir,
            $env:DEVKITPRO,
            $env:DEVKITARM
        ) -ScriptBlock {
            param(
                [string]$root,
                [string]$sliceJson,
                [string]$profileKey,
                [bool]$force,
                [bool]$noSharedBuild,
                [int]$worker,
                [int]$parallelBuildJobs,
                [string]$logDir,
                [string]$devkitPro,
                [string]$devkitArm
            )
            $ErrorActionPreference = 'Stop'
            if ($devkitPro) { $env:DEVKITPRO = $devkitPro }
            if ($devkitArm) { $env:DEVKITARM = $devkitArm }
            $records = @(ConvertFrom-Json -InputObject $sliceJson)
            $optSizeHarnessModes = @(161, 162, 163)
            $forcedSharedBuilds = @{}
            foreach ($record in $records) {
                $build = $record.Build
                $usesSharedBuild = $false
                if (-not $noSharedBuild) {
                    $rendererSuffix = if ($record.Target -like '*-hwtri') { '-hwtri' } else { '' }
                    if ($optSizeHarnessModes -contains [int]$record.Mode) {
                        $build = "build-verify-$profileKey-optsize-shared-$worker$rendererSuffix"
                    } else {
                        $build = "build-verify-$profileKey-shared-$worker$rendererSuffix"
                    }
                    $usesSharedBuild = $true
                }
                $makeArgs = @(
                    '-C', $root,
                    "TARGET=$($record.Target)",
                    "BUILD=$build",
                    "NDS_DEV_SCENE_HARNESS=$($record.Harness)"
                )
                if ($record.Target -like '*-hwtri') {
                    $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
                }
                $forceThisBuild = [bool]$force
                if ($usesSharedBuild -and $force) {
                    if ($forcedSharedBuilds.ContainsKey($build)) {
                        $forceThisBuild = $false
                    } else {
                        $forcedSharedBuilds[$build] = $true
                    }
                }
                if ($forceThisBuild) {
                    $makeArgs += '-B'
                }
                $makeArgs += "-j$parallelBuildJobs"
                $logPath = Join-Path $logDir ("worker{0}-{1}.log" -f $worker, $record.Name)
                Write-Output ("[worker {0}] Building verifier output: {1}" -f $worker, $record.Name)
                $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
                & make @makeArgs *> $logPath
                $exitCode = $LASTEXITCODE
                $stopwatch.Stop()
                [PSCustomObject]@{
                    kind = 'timing'
                    name = $record.Name
                    target = $record.Target
                    build = $build
                    registryBuild = $record.Build
                    harness = $record.Harness
                    sharedBuild = $usesSharedBuild
                    worker = $worker
                    durationSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 3)
                    exitCode = $exitCode
                    logPath = $logPath
                }
                if ($exitCode -ne 0) {
                    Write-Output ("[worker {0}] Build failed: {1}" -f $worker, $record.Name)
                    if (Test-Path -LiteralPath $logPath) {
                        Get-Content -LiteralPath $logPath -Tail 200
                    }
                    break
                }
            }
        }
    }
    $jobOutput = @($jobs | Receive-Job -Wait)
    $jobs | Remove-Job
    $jobOutput | Where-Object { -not (($_ -is [pscustomobject]) -and ($_.kind -eq 'timing')) } | Write-Output
    $jobTimings = @($jobOutput | Where-Object { ($_ -is [pscustomobject]) -and ($_.kind -eq 'timing') })
    foreach ($timing in $jobTimings) {
        $timings += [PSCustomObject]@{
            name = $timing.name
            target = $timing.target
            build = $timing.build
            registryBuild = $timing.registryBuild
            harness = $timing.harness
            sharedBuild = $timing.sharedBuild
            worker = $timing.worker
            durationSeconds = $timing.durationSeconds
            exitCode = $timing.exitCode
            logPath = $timing.logPath
        }
    }
    $failed = @($timings | Where-Object { $_.exitCode -ne 0 })
    if ($failed.Count -gt 0) {
        exit $failed[0].exitCode
    }
    if ($jobTimings.Count -ne $buildRecords.Count) {
        throw "Parallel build stopped early: built $($jobTimings.Count) of $($buildRecords.Count) verifier outputs."
    }
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
