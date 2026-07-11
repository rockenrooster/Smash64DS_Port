param(
    [ValidateSet('Full','Latest','LatestFast','BoundaryDirect','Boundary','Regression','RegressionCore','RegressionFast','Smoke','SmokeFast','Fighter','Direct','MenuChain')]
    [string]$Profile = 'Boundary',
    [string[]]$Only,
    [string]$From,
    [switch]$Force,
    [switch]$NoSharedBuild,
    [int]$ParallelBuilds = 0,
    [int]$ParallelBuildJobs = 0,
    [string]$TimingPath,
    [switch]$VerifyStamp,
    [switch]$Detach
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$profileKey = $Profile.ToLowerInvariant()
$costDir = Join-Path $root 'artifacts\verifier-cost'
$stampPath = Join-Path $costDir 'prebuild-stamp.json'

function Get-GitHead {
    $rev = (& git -C $root rev-parse HEAD 2>$null)
    if ($LASTEXITCODE -ne 0) { return 'unknown' }
    return ([string]$rev).Trim()
}

function Convert-ToHexString {
    param([byte[]]$Bytes)
    return (($Bytes | ForEach-Object { $_.ToString('x2') }) -join '')
}

function Get-BuildConfigHash {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $configs = @()
        foreach ($base in @($root, (Join-Path $root 'builds'))) {
            if (-not (Test-Path -LiteralPath $base)) { continue }
            Get-ChildItem -LiteralPath $base -Directory -Filter 'build*' -ErrorAction SilentlyContinue | ForEach-Object {
                $candidate = Join-Path $_.FullName 'nds_build_config.h'
                if (Test-Path -LiteralPath $candidate) {
                    $configs += (Get-Item -LiteralPath $candidate)
                }
            }
        }
        foreach ($config in @($configs | Sort-Object FullName)) {
            $relative = $config.FullName.Substring($root.Length).TrimStart([char[]]@('\','/'))
            $prefix = [System.Text.Encoding]::UTF8.GetBytes("$relative`n")
            [void]$sha.TransformBlock($prefix, 0, $prefix.Length, $null, 0)
            $bytes = [System.IO.File]::ReadAllBytes($config.FullName)
            [void]$sha.TransformBlock($bytes, 0, $bytes.Length, $null, 0)
        }
        [void]$sha.TransformFinalBlock([byte[]]::new(0), 0, 0)
        return Convert-ToHexString $sha.Hash
    } finally {
        $sha.Dispose()
    }
}

function Get-TargetArtifactRecord {
    param([string]$Target)
    $items = @()
    foreach ($extension in @('.nds', '.elf')) {
        $path = Join-Path $root "$Target$extension"
        if (Test-Path -LiteralPath $path) {
            $item = Get-Item -LiteralPath $path
            $items += [PSCustomObject]@{
                name = $item.Name
                path = $item.FullName.Substring($root.Length).TrimStart([char[]]@('\','/'))
                mtimeUtc = $item.LastWriteTimeUtc.ToString('o')
                size = $item.Length
            }
        }
    }
    return [PSCustomObject]@{
        target = $Target
        artifacts = $items
    }
}

function Write-PrebuildStamp {
    param(
        [object[]]$Timings,
        [double]$TotalSeconds
    )
    New-Item -ItemType Directory -Force -Path $costDir | Out-Null
    $targets = @($Timings | ForEach-Object { $_.target } | Where-Object { $_ } | Sort-Object -Unique)
    [PSCustomObject]@{
        generatedAt = (Get-Date).ToUniversalTime().ToString('o')
        gitRev = Get-GitHead
        profile = $Profile
        force = [bool]$Force
        noSharedBuild = [bool]$NoSharedBuild
        configHash = Get-BuildConfigHash
        totalSeconds = [Math]::Round($TotalSeconds, 3)
        count = $targets.Count
        timings = @($Timings | ForEach-Object {
            [PSCustomObject]@{
                name = $_.name
                target = $_.target
                build = $_.build
                registryBuild = $_.registryBuild
                harness = $_.harness
                sharedBuild = $_.sharedBuild
                worker = $_.worker
                durationSeconds = $_.durationSeconds
                exitCode = $_.exitCode
                logPath = $_.logPath
            }
        })
        targets = @($targets | ForEach-Object { Get-TargetArtifactRecord $_ })
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $stampPath -Encoding UTF8
    Write-Output "Wrote prebuild stamp: $stampPath"
}

function Test-PrebuildStamp {
    if (-not (Test-Path -LiteralPath $stampPath)) {
        Write-Error "Missing prebuild stamp: $stampPath"
        return $false
    }
    $stamp = Get-Content -LiteralPath $stampPath -Raw | ConvertFrom-Json
    $currentRev = Get-GitHead
    if ($stamp.gitRev -ne $currentRev) {
        Write-Error "Prebuild stamp git rev mismatch: stamp=$($stamp.gitRev) current=$currentRev"
        return $false
    }
    $currentHash = Get-BuildConfigHash
    if ($stamp.configHash -ne $currentHash) {
        Write-Error "Prebuild stamp config hash mismatch: stamp=$($stamp.configHash) current=$currentHash"
        return $false
    }
    foreach ($target in @($stamp.targets)) {
        foreach ($artifact in @($target.artifacts)) {
            $path = Join-Path $root $artifact.path
            if (-not (Test-Path -LiteralPath $path)) {
                Write-Error "Missing stamped artifact: $($artifact.path)"
                return $false
            }
            $item = Get-Item -LiteralPath $path
            $stampedMtimeUtc = if ($artifact.mtimeUtc -is [datetime]) {
                $artifact.mtimeUtc.ToUniversalTime().ToString('o')
            } else {
                [string]$artifact.mtimeUtc
            }
            if (($item.Length -ne [int64]$artifact.size) -or ($item.LastWriteTimeUtc.ToString('o') -ne $stampedMtimeUtc)) {
                Write-Error "Stamped artifact changed: $($artifact.path)"
                return $false
            }
        }
    }
    Write-Host ("Prebuild stamp valid: profile={0} targets={1} generatedAt={2}" -f $stamp.profile, $stamp.count, $stamp.generatedAt)
    return $true
}

function Quote-DetachedProcessArg {
    param([string]$Value)
    if ($Value -notmatch '[\s"]') {
        return $Value
    }
    return '"' + $Value.Replace('"', '\"') + '"'
}

function Add-DetachedProcessArg {
    param(
        [System.Collections.Generic.List[string]]$Words,
        [string]$Name,
        [object]$Value
    )
    if ($null -eq $Value) { return }
    if ($Value -is [array]) {
        if ($Value.Count -eq 0) { return }
        $Words.Add($Name)
        foreach ($entry in $Value) { $Words.Add((Quote-DetachedProcessArg ([string]$entry))) }
        return
    }
    if ([string]$Value -ne '') {
        $Words.Add($Name)
        $Words.Add((Quote-DetachedProcessArg ([string]$Value)))
    }
}

if ($VerifyStamp) {
    if ($Detach) { throw '-VerifyStamp and -Detach cannot be combined.' }
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $ok = Test-PrebuildStamp
    $stopwatch.Stop()
    Write-Output ("Prebuild stamp check took {0:N2}s." -f $stopwatch.Elapsed.TotalSeconds)
    if ($ok) { exit 0 }
    exit 1
}

if ($Detach) {
    New-Item -ItemType Directory -Force -Path (Join-Path $costDir 'build-logs') | Out-Null
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $stdoutLog = Join-Path $costDir "build-logs\prebuild-$profileKey-$stamp.out.log"
    $stderrLog = Join-Path $costDir "build-logs\prebuild-$profileKey-$stamp.err.log"
    $powerShellExe = (Get-Process -Id $PID).Path
    $words = [System.Collections.Generic.List[string]]::new()
    $words.Add('-NoProfile')
    $words.Add('-ExecutionPolicy')
    $words.Add('Bypass')
    $words.Add('-File')
    $words.Add((Quote-DetachedProcessArg $PSCommandPath))
    Add-DetachedProcessArg $words '-Profile' $Profile
    Add-DetachedProcessArg $words '-Only' $Only
    Add-DetachedProcessArg $words '-From' $From
    if ($Force) { $words.Add('-Force') }
    if ($NoSharedBuild) { $words.Add('-NoSharedBuild') }
    Add-DetachedProcessArg $words '-ParallelBuilds' $ParallelBuilds
    Add-DetachedProcessArg $words '-ParallelBuildJobs' $ParallelBuildJobs
    Add-DetachedProcessArg $words '-TimingPath' $TimingPath
    $argumentLine = $words -join ' '
    $process = Start-Process -FilePath $powerShellExe -ArgumentList $argumentLine -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog -WindowStyle Hidden -PassThru
    Write-Output "Detached prebuild started: pid=$($process.Id)"
    Write-Output "stdout: $stdoutLog"
    Write-Output "stderr: $stderrLog"
    Write-Output "stamp on success: $stampPath"
    exit 0
}

$plan = @(Get-Smash64DSVerifyPlan -Profile $Profile -Only $Only -From $From)
if ($plan.Count -eq 0) {
    throw "No verifiers selected for profile '$Profile'."
}
$sharedBuild = "build-verify-$profileKey-shared"
$optSizeSharedBuild = "build-verify-$profileKey-optsize-shared"
$optSizeHarnessModes = @(161, 162, 163)
function Test-OptSizeHarnessMode {
    param($Mode)

    foreach ($modeValue in @($Mode)) {
        [int]$parsedMode = 0
        if ([int]::TryParse([string]$modeValue, [ref]$parsedMode) -and
            ($optSizeHarnessModes -contains $parsedMode)) {
            return $true
        }
    }
    return $false
}
if ($ParallelBuilds -le 0) {
    $ParallelBuilds = if ($Profile -in @('Full','Regression','RegressionCore')) { 4 } else { 1 }
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
    $defaultBuildJobs = [Math]::Max($ParallelBuildJobs, 16)
    $makeArgs = @(
        '-C', $root,
        'TARGET=smash64ds',
        'BUILD=build',
        'NDS_DEV_SCENE_HARNESS=normal',
        'NDS_HARNESS_FAST_LOGIC=1'
    )
    if ($Force) {
        $makeArgs += '-B'
    }
    $makeArgs += "-j$defaultBuildJobs"
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
        $fastLogicValue = if (@($record.Tags) -contains 'realtime') { '0' } else { '1' }
        $logicSuffix = if ($fastLogicValue -eq '0') { '-realtime' } else { '' }
        if (-not $NoSharedBuild) {
            $rendererSuffix = if ($record.Target -like '*-hwtri') { '-hwtri' } else { '' }
            if (Test-OptSizeHarnessMode $record.Mode) {
                $build = "$optSizeSharedBuild$logicSuffix$rendererSuffix"
            } else {
                $build = "$sharedBuild$logicSuffix$rendererSuffix"
            }
            $usesSharedBuild = $true
        }
        Write-Output "Building verifier output: $($record.Name)"
        $makeArgs = @(
            '-C', $root,
            "TARGET=$($record.Target)",
            "BUILD=$build",
            "NDS_DEV_SCENE_HARNESS=$($record.Harness)",
            "NDS_HARNESS_FAST_LOGIC=$fastLogicValue"
        )
        if ($record.Target -like '*-hwtri') {
            $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
        }
        if (@($record.Tags) -contains 'live_input') {
            $makeArgs += 'NDS_DEV_LIVE_INPUT_PREVIEW=1'
        }
        if ($record.Harness -eq 'battle_playable_match_lifecycle') {
            $makeArgs += 'NDS_DEV_RESULTS_VISUAL_SMOKE=1'
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
            $records = ConvertFrom-Json -InputObject $sliceJson
            if ($null -eq $records) {
                $records = @()
            }
            $optSizeHarnessModes = @(161, 162, 163)
            function Test-OptSizeHarnessMode {
                param($Mode)

                foreach ($modeValue in @($Mode)) {
                    [int]$parsedMode = 0
                    if ([int]::TryParse([string]$modeValue, [ref]$parsedMode) -and
                        ($optSizeHarnessModes -contains $parsedMode)) {
                        return $true
                    }
                }
                return $false
            }
            $forcedSharedBuilds = @{}
            foreach ($record in $records) {
                $build = $record.Build
                $usesSharedBuild = $false
                $fastLogicValue = if (@($record.Tags) -contains 'realtime') { '0' } else { '1' }
                $logicSuffix = if ($fastLogicValue -eq '0') { '-realtime' } else { '' }
                if (-not $noSharedBuild) {
                    $rendererSuffix = if ($record.Target -like '*-hwtri') { '-hwtri' } else { '' }
                    if (Test-OptSizeHarnessMode $record.Mode) {
                        $build = "build-verify-$profileKey-optsize-shared-$worker$logicSuffix$rendererSuffix"
                    } else {
                        $build = "build-verify-$profileKey-shared-$worker$logicSuffix$rendererSuffix"
                    }
                    $usesSharedBuild = $true
                }
                $makeArgs = @(
                    '-C', $root,
                    "TARGET=$($record.Target)",
                    "BUILD=$build",
                    "NDS_DEV_SCENE_HARNESS=$($record.Harness)",
                    "NDS_HARNESS_FAST_LOGIC=$fastLogicValue"
                )
                if ($record.Target -like '*-hwtri') {
                    $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
                }
                if (@($record.Tags) -contains 'live_input') {
                    $makeArgs += 'NDS_DEV_LIVE_INPUT_PREVIEW=1'
                }
                if ($record.Harness -eq 'battle_playable_match_lifecycle') {
                    $makeArgs += 'NDS_DEV_RESULTS_VISUAL_SMOKE=1'
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
                $previousErrorActionPreference = $ErrorActionPreference
                $ErrorActionPreference = 'Continue'
                & make @makeArgs *> $logPath
                $exitCode = $LASTEXITCODE
                $ErrorActionPreference = $previousErrorActionPreference
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
Write-PrebuildStamp -Timings $timings -TotalSeconds $totalStopwatch.Elapsed.TotalSeconds
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
