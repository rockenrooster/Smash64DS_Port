[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet('Latest','LatestFast','Boundary','Regression','RegressionFast')]
    [string]$Profile = 'Latest',
    [string[]]$Only,
    [switch]$List,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
function Resolve-OnlyVerifierNames {
    param(
        [object[]]$Registry,
        [string[]]$Requested
    )
    if (-not $Requested -or $Requested.Count -eq 0) {
        return @()
    }
    $resolved = @()
    foreach ($entry in $Requested) {
        $needle = "$entry".Trim()
        if ([string]::IsNullOrWhiteSpace($needle)) { continue }
        $scriptName = $needle
        if (-not $scriptName.EndsWith('.ps1')) { $scriptName = "$scriptName.ps1" }
        $matches = @(
            $Registry | Where-Object {
                $_.Name -eq $needle -or
                $_.Harness -eq $needle -or
                $_.Script -eq $needle -or
                $_.Script -eq $scriptName
            }
        )
        if ($matches.Count -eq 0) {
            throw "Unknown verifier '$needle'. Use a registry name, harness name, or verifier script name."
        }
        if ($matches.Count -gt 1) {
            throw "Verifier '$needle' matched more than one registry entry: $(@($matches | ForEach-Object { $_.Name }) -join ', ')"
        }
        $resolved += $matches[0].Name
    }
    return @($resolved | Select-Object -Unique)
}
function Format-DurationSeconds {
    param([Nullable[double]]$Seconds)
    if ($null -eq $Seconds) { return '-' }
    return ('{0:N2}s' -f $Seconds)
}
function New-ResultRow {
    param(
        [object]$Record,
        [string]$ProfileName,
        [Nullable[double]]$DurationSeconds,
        [string]$Status,
        [Nullable[int]]$ExitCode
    )
    [PSCustomObject]@{
        name = $Record.Name
        profile = $ProfileName
        duration = (Format-DurationSeconds $DurationSeconds)
        status = $Status
        script = $Record.Script
        durationSeconds = $DurationSeconds
        exitCode = $ExitCode
    }
}
function Invoke-VerifierForTiming {
    param(
        [object]$Record
    )
    $scriptPath = Join-Path $PSScriptRoot $Record.Script
    if (-not (Test-Path -LiteralPath $scriptPath)) {
        throw "Verifier script does not exist: $($Record.Script)"
    }
    $arguments = @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', $scriptPath,
        '-MelonDS', $MelonDS,
        '-Gdb', $Gdb
    )
    $process = Start-Process `
        -FilePath $powerShellExe `
        -ArgumentList $arguments `
        -WorkingDirectory $root `
        -NoNewWindow `
        -Wait `
        -PassThru
    return $process.ExitCode
}
$registry = @(Get-Smash64DSHarnessRegistry)
$resolvedOnly = @(Resolve-OnlyVerifierNames -Registry $registry -Requested $Only)
$plan = @(Get-Smash64DSVerifyPlan -Profile $Profile -Only $resolvedOnly)
if ($plan.Count -eq 0) {
    throw "No verifiers selected."
}
$isDryRun = $List -or $WhatIfPreference
if ($isDryRun) {
    Write-Output "Verifier cost plan for profile '$Profile' ($($plan.Count) verifier(s)); no emulator will be run."
    $plan | ForEach-Object {
        New-ResultRow -Record $_ -ProfileName $Profile -DurationSeconds $null -Status 'planned' -ExitCode $null
    } | Format-Table name, profile, duration, status, script -AutoSize
    exit 0
}
$results = @()
$anyFailed = $false
foreach ($record in $plan) {
    Write-Output "Running verifier: $($record.Name) [$($record.Script)]"
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $exitCode = $null
    $status = 'fail'
    try {
        $exitCode = Invoke-VerifierForTiming -Record $record
        if ($exitCode -eq 0) {
            $status = 'pass'
        } else {
            $anyFailed = $true
        }
    } catch {
        $anyFailed = $true
        Write-Warning $_.Exception.Message
    } finally {
        $stopwatch.Stop()
    }
    $results += New-ResultRow `
        -Record $record `
        -ProfileName $Profile `
        -DurationSeconds $stopwatch.Elapsed.TotalSeconds `
        -Status $status `
        -ExitCode $exitCode
}
$artifactDir = Join-Path $root 'artifacts\verifier-cost'
New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
$artifactPath = Join-Path $artifactDir "verifier-cost-$($Profile.ToLowerInvariant()).json"
$payload = [PSCustomObject]@{
    generatedAt = (Get-Date).ToUniversalTime().ToString('o')
    profile = $Profile
    only = $resolvedOnly
    count = $results.Count
    results = $results
}
$payload | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $artifactPath -Encoding UTF8
$results | Format-Table name, profile, duration, status, script -AutoSize
Write-Output "Wrote verifier cost artifact: $artifactPath"
if ($anyFailed) {
    exit 1
}
