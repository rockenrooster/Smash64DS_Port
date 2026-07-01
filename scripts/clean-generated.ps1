param(
    [switch]$DryRun,
    [switch]$Force,
    [switch]$KeepArtifacts,
    [switch]$KeepNormalBuild,
    [switch]$KeepLatestBuilds
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$latestKeepPaths = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$latestKeepLabels = [System.Collections.Generic.List[string]]::new()
function Add-LatestKeepPath {
    param(
        [string]$Path,
        [string]$Label
    )
    if (-not $Path) { return }
    $full = [System.IO.Path]::GetFullPath($Path)
    if ($latestKeepPaths.Add($full)) {
        $latestKeepLabels.Add(("{0} -> {1}" -f $Label, $full)) | Out-Null
    }
}
if ($KeepLatestBuilds) {
    $registryPath = Join-Path $PSScriptRoot 'lib\harness-registry.ps1'
    if (-not (Test-Path -LiteralPath $registryPath)) {
        throw "Cannot resolve -KeepLatestBuilds because registry is missing: $registryPath"
    }
    . $registryPath
    $latestPlan = @(Get-Smash64DSVerifyPlan -Profile Boundary)
    if ($latestPlan.Count -eq 0) {
        throw 'Cannot resolve -KeepLatestBuilds because Boundary profile returned no entries.'
    }
    foreach ($entry in $latestPlan) {
        if ($entry.Build) {
            Add-LatestKeepPath -Path (Join-Path $root $entry.Build) -Label ("{0} build" -f $entry.Name)
        }
        if ($entry.Target) {
            foreach ($ext in @('.elf', '.nds', '.ds.gba', '.map', '.sym')) {
                Add-LatestKeepPath -Path (Join-Path $root ("{0}{1}" -f $entry.Target, $ext)) -Label ("{0} output" -f $entry.Name)
            }
        }
    }
}
function Test-InRoot {
    param([string]$Path)
    $full = [System.IO.Path]::GetFullPath($Path)
    return $full.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)
}
function Get-PathSize {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return 0L }
    $item = Get-Item -LiteralPath $Path -Force
    if (-not $item.PSIsContainer) { return [int64]$item.Length }
    $sum = 0L
    Get-ChildItem -LiteralPath $Path -Force -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
        if (-not $_.PSIsContainer) { $sum += [int64]$_.Length }
    }
    return $sum
}
function Add-Candidate {
    param(
        [System.Collections.Generic.List[object]]$Candidates,
        [string]$Path,
        [string]$Reason
    )
    if (-not (Test-Path -LiteralPath $Path)) { return }
    if (-not (Test-InRoot -Path $Path)) { throw "Refusing to clean path outside repo: $Path" }
    $full = [System.IO.Path]::GetFullPath($Path)
    if ($KeepLatestBuilds -and $latestKeepPaths.Contains($full)) {
        return
    }
    $name = Split-Path -Leaf $Path
    if ($KeepNormalBuild) {
        $normalPreserve = @('build', 'smash64ds.elf', 'smash64ds.nds', 'smash64ds.ds.gba')
        if ($normalPreserve -contains $name) { return }
    }
    if ($KeepArtifacts -and $Path.StartsWith((Join-Path $root 'artifacts'), [System.StringComparison]::OrdinalIgnoreCase)) {
        return
    }
    if ($Candidates | Where-Object { $_.Path -eq $full }) { return }
    $Candidates.Add([PSCustomObject]@{
        Path = $full
        Reason = $Reason
        Bytes = Get-PathSize -Path $full
    }) | Out-Null
}
$candidates = [System.Collections.Generic.List[object]]::new()
if ($KeepLatestBuilds) {
    Write-Output 'KeepLatestBuilds resolved from Boundary profile:'
    foreach ($label in $latestKeepLabels) {
        Write-Output ("  {0}" -f $label)
    }
}
Add-Candidate -Candidates $candidates -Path (Join-Path $root 'build') -Reason 'normal build directory'
Get-ChildItem -LiteralPath $root -Force -Directory -Filter 'build-*' | ForEach-Object {
    Add-Candidate -Candidates $candidates -Path $_.FullName -Reason 'harness build directory'
}
foreach ($pattern in @('smash64ds*.elf', 'smash64ds*.nds', 'smash64ds*.ds.gba', 'smash64ds*.map', 'smash64ds*.sym', '_*.gdb', '_*.gdb.out', '_*.gdb.err', 'melonds.*.log', 'rtc.bin')) {
    Get-ChildItem -LiteralPath $root -Force -File -Filter $pattern | ForEach-Object {
        Add-Candidate -Candidates $candidates -Path $_.FullName -Reason "root generated file $pattern"
    }
}
if (-not $KeepArtifacts) {
    $emulatorLogs = Join-Path $root 'artifacts\emulator-logs'
    if (Test-Path -LiteralPath $emulatorLogs) {
        Get-ChildItem -LiteralPath $emulatorLogs -Force -File -Filter '*.log' | ForEach-Object {
            Add-Candidate -Candidates $candidates -Path $_.FullName -Reason 'emulator log'
        }
    }
}
$totalBytes = ($candidates | Measure-Object -Property Bytes -Sum).Sum
if ($null -eq $totalBytes) { $totalBytes = 0L }
foreach ($candidate in $candidates | Sort-Object Path) {
    $mb = [math]::Round($candidate.Bytes / 1MB, 2)
    $verb = if ($DryRun) { 'Would delete' } else { 'Delete' }
    Write-Output ("{0}: {1} ({2} MB) [{3}]" -f $verb, $candidate.Path, $mb, $candidate.Reason)
}
if (-not $DryRun -and -not $Force -and $candidates.Count -gt 0) {
    $answer = Read-Host "Delete $($candidates.Count) generated path(s)? Type YES to continue"
    if ($answer -ne 'YES') {
        Write-Output 'Clean canceled.'
        exit 1
    }
}
if (-not $DryRun) {
    foreach ($candidate in $candidates) {
        if (-not (Test-InRoot -Path $candidate.Path)) { throw "Refusing to delete path outside repo: $($candidate.Path)" }
        Remove-Item -LiteralPath $candidate.Path -Force -Recurse -ErrorAction SilentlyContinue
    }
}
$totalMb = [math]::Round($totalBytes / 1MB, 2)
$action = if ($DryRun) { 'would remove' } elseif ($candidates.Count -eq 0) { 'removed' } else { 'removed' }
Write-Output ("Generated clean summary: {0} path(s) matched, {1} MB {2}, deleted={3}" -f $candidates.Count, $totalMb, $action, (-not $DryRun))
