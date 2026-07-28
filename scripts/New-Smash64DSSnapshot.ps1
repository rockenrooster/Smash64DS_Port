param(
    [string]$Source = (Join-Path $PSScriptRoot '..'),
    [string]$DestDir
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\snapshot-hygiene.ps1')
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
$sourceRoot = (Resolve-Path -LiteralPath $Source).Path
if (-not (Test-Path -LiteralPath $sourceRoot -PathType Container)) {
    throw "Source folder not found: $sourceRoot"
}
if (-not $DestDir) {
    $DestDir = Get-DefaultSnapshotDestDir
}
$destRoot = [System.IO.Path]::GetFullPath($DestDir)
if (-not (Test-Path -LiteralPath $destRoot)) {
    New-Item -ItemType Directory -Path $destRoot -Force | Out-Null
}
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$sourceName = Split-Path -Leaf $sourceRoot
$archive = Join-Path $destRoot ("{0}_Lean_{1}.zip" -f $sourceName, $timestamp)
if (Test-Path -LiteralPath $archive) {
    throw "Archive already exists: $archive"
}
$sevenZip = Get-Smash64DSSevenZip
$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("smash64ds-snapshot-{0}" -f ([Guid]::NewGuid().ToString('N')))
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
try {
    $fileList = Join-Path $tempDir 'snapshot-files.txt'
    # Directory leaf names whose entire subtree is excluded by the Lean rules.
    # The walk prunes these without descending, so their files are never
    # enumerated. Safe to prune: verified to contain zero included files (every
    # one maps to a subtree-total category in scripts/lib/snapshot-hygiene.ps1).
    # Per-file filtering still runs on everything else via the category function.
    $pruneDirs = [System.Collections.Generic.HashSet[string]]::new(
        [string[]]@('.git', '.codegraph', 'build', 'builds', 'target', '.gradle', 'artifacts'),
        [System.StringComparer]::OrdinalIgnoreCase)
    $included = [System.Collections.Generic.List[string]]::new()
    $stack = [System.Collections.Generic.Stack[string]]::new()
    $stack.Push($sourceRoot)
    while ($stack.Count -gt 0) {
        $current = $stack.Pop()
        foreach ($file in Get-ChildItem -LiteralPath $current -File -Force) {
            $relative = ConvertTo-Smash64DSRelativePath -BasePath $sourceRoot -FullPath $file.FullName
            $category = Get-Smash64DSSnapshotPathCategory -RelativePath $relative -Mode 'Lean'
            if ($category.Include) {
                $included.Add($relative) | Out-Null
            }
        }
        foreach ($dir in Get-ChildItem -LiteralPath $current -Directory -Force) {
            if ($pruneDirs.Contains($dir.Name)) { continue }
            $stack.Push($dir.FullName)
        }
    }
    $included | Set-Content -LiteralPath $fileList -Encoding UTF8
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
    $archiveItem = Get-Item -LiteralPath $archive
    Write-Host ("Done: {0} ({1} MB)" -f $archiveItem.FullName, (Format-Smash64DSMegabytes -Bytes $archiveItem.Length)) -ForegroundColor Green
} finally {
    Remove-Item -LiteralPath $tempDir -Force -Recurse -ErrorAction SilentlyContinue
}
