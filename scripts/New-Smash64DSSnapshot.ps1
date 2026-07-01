param(
    [ValidateSet('Lean','CodeOnly','Full')]
    [string]$Mode = 'Lean',
    [string]$Source = (Join-Path $PSScriptRoot '..'),
    [string]$DestDir,
    [switch]$DryRun,
    [switch]$IncludeArtifacts,
    [switch]$IncludeDecompGenerated
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\snapshot-hygiene.ps1')
function Get-PathSize {
    param([System.IO.FileInfo]$File)
    return [int64]$File.Length
}
function Add-Stat {
    param(
        [hashtable]$Stats,
        [string]$Category,
        [int64]$Bytes
    )
    if (-not $Stats.ContainsKey($Category)) {
        $Stats[$Category] = [PSCustomObject]@{
            Category = $Category
            Count = 0
            Bytes = 0L
        }
    }
    $Stats[$Category].Count++
    $Stats[$Category].Bytes += $Bytes
}
function Get-DefaultSnapshotDestDir {
    $desktop = [Environment]::GetFolderPath('Desktop')
    if ($desktop) {
        $desktopSnapshots = Join-Path $desktop 'Snapshots'
        if (Test-Path -LiteralPath $desktopSnapshots) {
            return $desktopSnapshots
        }
    }
    return (Join-Path (Resolve-Path (Join-Path $PSScriptRoot '..')).Path 'artifacts\snapshots')
}
function Get-LatestVerifierManifestLines {
    $registryPath = Join-Path $PSScriptRoot 'lib\harness-registry.ps1'
    if (-not (Test-Path -LiteralPath $registryPath)) {
        return @('Latest verifiers: unavailable (scripts/lib/harness-registry.ps1 missing)')
    }
    try {
        . $registryPath
        $latest = @(Get-Smash64DSVerifyPlan -Profile Latest)
        $lines = @('Latest verifiers from harness registry:')
        foreach ($entry in $latest) {
            $script = if ($entry.Script) { $entry.Script } else { '(no script)' }
            $lines += ("- {0}: {1}" -f $entry.Name, $script)
        }
        return $lines
    } catch {
        return @("Latest verifiers: unavailable ($($_.Exception.Message))")
    }
}
function Write-SnapshotStats {
    param(
        [hashtable]$Stats,
        [string]$Label
    )
    Write-Host $Label -ForegroundColor Cyan
    foreach ($key in @($Stats.Keys | Sort-Object)) {
        $stat = $Stats[$key]
        $mb = Format-Smash64DSMegabytes -Bytes $stat.Bytes
        Write-Host ("  {0}: {1} file(s), {2} MB" -f $stat.Category, $stat.Count, $mb)
    }
}
function Write-TopSnapshotFiles {
    param(
        [object[]]$Files,
        [string]$Label
    )
    Write-Host $Label -ForegroundColor Cyan
    $top = @($Files | Sort-Object Bytes -Descending | Select-Object -First 20)
    if ($top.Count -eq 0) {
        Write-Host '  (none)'
        return
    }
    foreach ($file in $top) {
        Write-Host ("  {0} MB  [{1}] {2}" -f
            (Format-Smash64DSMegabytes -Bytes $file.Bytes),
            $file.Category,
            $file.Path)
    }
}
function Write-LeanDecompProbe {
    param(
        [object[]]$IncludedFiles,
        [object[]]$ExcludedFiles,
        [string]$ProbePath
    )
    $probe = ConvertTo-Smash64DSSnapshotPath -Path $ProbePath
    $included = @($IncludedFiles | Where-Object { $_.Path.StartsWith($probe, [System.StringComparison]::OrdinalIgnoreCase) })
    $excluded = @($ExcludedFiles | Where-Object { $_.Path.StartsWith($probe, [System.StringComparison]::OrdinalIgnoreCase) })
    if ($included.Count -gt 0) {
        Write-Host ("  included: {0} ({1} file(s), {2} MB)" -f
            $probe,
            $included.Count, `
            (Format-Smash64DSMegabytes -Bytes (($included | Measure-Object -Property Bytes -Sum).Sum)))
    } elseif ($excluded.Count -gt 0) {
        $categories = @($excluded | Select-Object -ExpandProperty Category -Unique | Sort-Object)
        Write-Host ("  excluded: {0} ({1} file(s), {2} MB, categories={3})" -f
            $probe,
            $excluded.Count, `
            (Format-Smash64DSMegabytes -Bytes (($excluded | Measure-Object -Property Bytes -Sum).Sum)), `
            ($categories -join ','))
    } else {
        Write-Host ("  missing: {0}" -f $probe)
    }
}
function Test-CreatedSnapshot {
    param(
        [string]$Archive,
        [string]$Mode,
        [switch]$IncludeArtifacts,
        [switch]$IncludeDecompGenerated,
        [string]$SevenZip
    )
    if ($Mode -eq 'Full') { return }
    $entries = @(Get-Smash64DSArchiveEntries -Archive $Archive -SevenZip $SevenZip)
    $forbidden = @()
    foreach ($entry in $entries) {
        $category = Get-Smash64DSSnapshotPathCategory `
            -RelativePath $entry `
            -Mode $Mode `
            -IncludeArtifacts:$IncludeArtifacts `
            -IncludeDecompGenerated:$IncludeDecompGenerated
        if (-not $category.Include) {
            $forbidden += [PSCustomObject]@{
                Path = $entry
                Category = $category.Category
                Reason = $category.Reason
            }
        }
    }
    if ($forbidden.Count -gt 0) {
        $details = ($forbidden | Select-Object -First 20 | ForEach-Object {
            "  $($_.Path) [$($_.Category)]"
        }) -join [Environment]::NewLine
        throw "Snapshot hygiene self-check failed for $Archive with $($forbidden.Count) forbidden entrie(s):`n$details"
    }
    Write-Host ("Snapshot hygiene self-check passed: {0} entrie(s), mode={1}" -f $entries.Count, $Mode) -ForegroundColor Green
}
$sourceRoot = (Resolve-Path -LiteralPath $Source).Path
if (-not (Test-Path -LiteralPath $sourceRoot -PathType Container)) {
    throw "Source folder not found: $sourceRoot"
}
if (-not $DestDir) {
    $DestDir = Get-DefaultSnapshotDestDir
}
$destRoot = [System.IO.Path]::GetFullPath($DestDir)
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$sourceName = Split-Path -Leaf $sourceRoot
$archiveName = "{0}_{1}_{2}.zip" -f $sourceName, $Mode, $timestamp
$archive = Join-Path $destRoot $archiveName
$sevenZip = Get-Smash64DSSevenZip
if ($Mode -eq 'Full') {
    Write-Warning 'Full snapshot mode may include root generated outputs, artifacts, local emulator payloads, and other local data. Use only for explicit debugging/repro needs. The legacy broad exporter is retained as scripts/New-Smash64DSSnapshot.Legacy.ps1.'
}
if (($Mode -eq 'Lean') -and $IncludeDecompGenerated) {
    Write-Warning 'Lean -IncludeDecompGenerated may include upstream decomp build outputs, baseroms, generated binaries, and duplicate O2R payloads. Use only for explicit debug snapshots.'
} elseif (($Mode -ne 'Lean') -and $IncludeDecompGenerated) {
    Write-Warning '-IncludeDecompGenerated only affects Lean mode and will be ignored for this snapshot mode.'
}
$stats = @{}
$includedFiles = [System.Collections.Generic.List[string]]::new()
$includedDetails = [System.Collections.Generic.List[object]]::new()
$excludedDetails = [System.Collections.Generic.List[object]]::new()
$includedBytes = 0L
Get-ChildItem -LiteralPath $sourceRoot -Force -Recurse -File | ForEach-Object {
    $relative = ConvertTo-Smash64DSRelativePath -BasePath $sourceRoot -FullPath $_.FullName
    $category = Get-Smash64DSSnapshotPathCategory `
        -RelativePath $relative `
        -Mode $Mode `
        -IncludeArtifacts:$IncludeArtifacts `
        -IncludeDecompGenerated:$IncludeDecompGenerated
    $bytes = Get-PathSize -File $_
    Add-Stat -Stats $stats -Category $category.Category -Bytes $bytes
    if ($category.Include) {
        $includedFiles.Add($relative) | Out-Null
        $includedDetails.Add([PSCustomObject]@{
            Path = ConvertTo-Smash64DSSnapshotPath -Path $relative
            Bytes = $bytes
            Category = $category.Category
        }) | Out-Null
        $includedBytes += $bytes
    } else {
        $excludedDetails.Add([PSCustomObject]@{
            Path = ConvertTo-Smash64DSSnapshotPath -Path $relative
            Bytes = $bytes
            Category = $category.Category
        }) | Out-Null
    }
}
$latestVerifierLines = @(Get-LatestVerifierManifestLines)
if ($DryRun) {
    Write-Host ("Snapshot dry run: mode={0}, source={1}" -f $Mode, $sourceRoot) -ForegroundColor Cyan
    Write-Host ("Archive would be: {0}" -f $archive)
    Write-Host ("IncludeArtifacts: {0}" -f ([bool]$IncludeArtifacts))
    Write-Host ("IncludeDecompGenerated: {0}" -f ([bool]$IncludeDecompGenerated))
    Write-Host ("Estimated included size: {0} MB ({1} file(s))" -f
        (Format-Smash64DSMegabytes -Bytes $includedBytes),
        $includedFiles.Count)
    if ($Mode -eq 'Lean') {
        Write-Host 'Lean includes decomp source/reference/build-critical O2R but excludes upstream generated decomp payloads by default.'
        Write-Host 'Use -IncludeDecompGenerated only for explicit debug snapshots.'
        Write-Host 'CodeOnly excludes all decomp/.'
    } elseif ($Mode -eq 'CodeOnly') {
        Write-Host 'CodeOnly excludes all decomp/.'
    }
    Write-SnapshotStats -Stats $stats -Label 'Snapshot file totals by category:'
    if ($Mode -eq 'Lean') {
        Write-Host 'Lean decomp inclusion/exclusion probes:' -ForegroundColor Cyan
        Write-LeanDecompProbe -IncludedFiles $includedDetails -ExcludedFiles $excludedDetails -ProbePath 'decomp/sm64-nds/build/'
        Write-LeanDecompProbe -IncludedFiles $includedDetails -ExcludedFiles $excludedDetails -ProbePath 'decomp/BattleShip-main/decomp/tools/vpk0_build/target/'
        Write-LeanDecompProbe -IncludedFiles $includedDetails -ExcludedFiles $excludedDetails -ProbePath 'decomp/BattleShip-main/decomp/BattleShip_o2r/'
        Write-LeanDecompProbe -IncludedFiles $includedDetails -ExcludedFiles $excludedDetails -ProbePath 'decomp/BattleShip-main/BattleShip_o2r/'
    }
    Write-TopSnapshotFiles -Files $includedDetails -Label 'Top 20 included files by size:'
    Write-TopSnapshotFiles -Files $excludedDetails -Label 'Top 20 excluded files by size:'
    Write-Host 'Latest verifier names for manifest:'
    foreach ($line in $latestVerifierLines) {
        Write-Host "  $line"
    }
    return
}
if (-not (Test-Path -LiteralPath $destRoot)) {
    New-Item -ItemType Directory -Path $destRoot -Force | Out-Null
}
if (Test-Path -LiteralPath $archive) {
    throw "Archive already exists: $archive"
}
$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("smash64ds-snapshot-{0}" -f ([Guid]::NewGuid().ToString('N')))
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
try {
    $fileList = Join-Path $tempDir 'snapshot-files.txt'
    $manifest = Join-Path $tempDir 'SNAPSHOT_MANIFEST.txt'
    $includedFiles | Set-Content -LiteralPath $fileList -Encoding UTF8
    $excludedCategories = @($stats.Keys | Where-Object { $_ -ne 'included' } | Sort-Object)
    $manifestLines = @(
        'Smash64DS Snapshot Manifest',
        "Timestamp: $timestamp",
        "Mode: $Mode",
        "Source: $sourceRoot",
        "IncludeArtifacts: $([bool]$IncludeArtifacts)",
        "IncludeDecompGenerated: $([bool]$IncludeDecompGenerated)",
        "IncludedFiles: $($includedFiles.Count)",
        "IncludedMB: $(Format-Smash64DSMegabytes -Bytes $includedBytes)",
        "ExcludedCategories: $($excludedCategories -join ', ')",
        '',
        'Mode summary:',
        'Lean: source/docs/scripts/include/src/assets plus decomp source/reference/build-critical O2R, excluding generated/local/upstream build payloads.',
        'CodeOnly: Lean without decomp/.',
        'Full: broad debug/repro escape hatch; not for normal handoff.',
        '',
        'Excluded category totals:'
    )
    foreach ($category in $excludedCategories) {
        $stat = $stats[$category]
        $manifestLines += ("- {0}: {1} file(s), {2} MB" -f $category, $stat.Count, (Format-Smash64DSMegabytes -Bytes $stat.Bytes))
    }
    $manifestLines += ''
    $manifestLines += $latestVerifierLines
    $manifestLines | Set-Content -LiteralPath $manifest -Encoding UTF8
    Write-SnapshotStats -Stats $stats -Label 'Snapshot file totals by category:'
    Write-Host ("Creating snapshot: {0}" -f $archive) -ForegroundColor Cyan
    Push-Location $sourceRoot
    try {
        & $sevenZip a -tzip -mx1 $archive "@$fileList" -scsUTF-8
        if ($LASTEXITCODE -ne 0) {
            throw "7z archive creation failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }
    Push-Location $tempDir
    try {
        & $sevenZip a -tzip -mx1 $archive 'SNAPSHOT_MANIFEST.txt'
        if ($LASTEXITCODE -ne 0) {
            throw "7z manifest add failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }
    $archiveItem = Get-Item -LiteralPath $archive
    Test-CreatedSnapshot `
        -Archive $archiveItem.FullName `
        -Mode $Mode `
        -IncludeArtifacts:$IncludeArtifacts `
        -IncludeDecompGenerated:$IncludeDecompGenerated `
        -SevenZip $sevenZip
    Write-Host ("Done: {0} ({1} MB)" -f $archiveItem.FullName, (Format-Smash64DSMegabytes -Bytes $archiveItem.Length)) -ForegroundColor Green
} finally {
    Remove-Item -LiteralPath $tempDir -Force -Recurse -ErrorAction SilentlyContinue
}
