param(
    [ValidateSet('Lean')]
    [string]$Mode = 'Lean',
    [string]$Source,
    [string]$Archive
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\snapshot-hygiene.ps1')
if (($Source -and $Archive) -or ((-not $Source) -and (-not $Archive))) {
    throw 'Specify exactly one of -Source <repo> or -Archive <zip>.'
}
$repoRoot = if ($Source) {
    (Resolve-Path -LiteralPath $Source).Path
} else {
    (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
}
function New-PathSet {
    $set = New-Object 'System.Collections.Generic.HashSet[string]'
        ([System.StringComparer]::OrdinalIgnoreCase)
    return ,$set
}
function Add-PathSet {
    param(
        [System.Collections.Generic.HashSet[string]]$Set,
        [string]$Path
    )
    if ($Path) {
        $Set.Add((ConvertTo-Smash64DSSnapshotPath -Path $Path)) | Out-Null
    }
}
function Test-SnapshotSelectionIncludes {
    param([string]$RelativePath)
    $category = Get-Smash64DSSnapshotPathCategory `
        -RelativePath $RelativePath `
        -Mode $Mode
    return [bool]$category.Include
}
function Get-ImportDecompTargets {
    param([string]$Root)
    $targets = New-PathSet
    $importRoot = Join-Path $Root 'src\import'
    if (-not (Test-Path -LiteralPath $importRoot)) {
        throw "Import folder not found: $importRoot"
    }
    Get-ChildItem -LiteralPath $importRoot -Filter '*.c' -File | ForEach-Object {
        $wrapper = $_
        $text = Get-Content -LiteralPath $wrapper.FullName -Raw
        $matches = [regex]::Matches(
            $text,
            '^\s*#\s*include\s+"(?<path>[^"]*\.\./\.\./decomp/[^"]+)"',
            [System.Text.RegularExpressions.RegexOptions]::Multiline)
        foreach ($match in $matches) {
            $include = $match.Groups['path'].Value
            $full = [System.IO.Path]::GetFullPath(
                (Join-Path $wrapper.DirectoryName $include))
            if (-not $full.StartsWith(
                    $Root,
                    [System.StringComparison]::OrdinalIgnoreCase))
            {
                throw "Import include resolves outside repo: $include in $($wrapper.FullName)"
            }
            $relative = ConvertTo-Smash64DSRelativePath -BasePath $Root -FullPath $full
            Add-PathSet -Set $targets -Path $relative
        }
    }
    return $targets
}
function Get-MakefileRelocAssets {
    param([string]$Root)
    $makefile = Join-Path $Root 'Makefile'
    if (-not (Test-Path -LiteralPath $makefile)) {
        throw "Makefile not found: $makefile"
    }
    $assets = New-PathSet
    $inRelocList = $false
    function Add-RelocTokensFromLine {
        param([string]$Line)
        $withoutComment = ($Line -replace '#.*$', '')
        $payload = ($withoutComment -replace '\\\s*$', '').Trim()
        if (-not $payload) { return }
        foreach ($token in ($payload -split '\s+')) {
            $clean = $token.Trim()
            if ($clean -match '^reloc_[A-Za-z0-9_./-]+$') {
                Add-PathSet -Set $assets -Path $clean
            }
        }
    }
    foreach ($line in Get-Content -LiteralPath $makefile) {
        if ($line -match '^\s*NDS_[A-Za-z0-9_]+_RELOC_FILES\s*:=\s*(?<rest>.*)$') {
            $inRelocList = $true
            Add-RelocTokensFromLine -Line $Matches['rest']
            if ($line -notmatch '\\\s*$') {
                $inRelocList = $false
            }
            continue
        }
        if ($inRelocList) {
            Add-RelocTokensFromLine -Line $line
            if ($line -notmatch '\\\s*$') {
                $inRelocList = $false
            }
        }
    }
    return $assets
}
$archiveSet = $null
if ($Archive) {
    $archivePath = (Resolve-Path -LiteralPath $Archive).Path
    $entries = @(Get-Smash64DSArchiveEntries -Archive $archivePath)
    $archiveSet = New-PathSet
    foreach ($entry in $entries) {
        Add-PathSet -Set $archiveSet -Path $entry
    }
}
function Assert-IncludedPath {
    param(
        [string]$RelativePath,
        [string]$Reason
    )
    $relative = ConvertTo-Smash64DSSnapshotPath -Path $RelativePath
    if ($Archive) {
        if (-not $archiveSet.Contains($relative)) {
            throw "Archive is missing required $Reason path: $relative"
        }
        return
    }
    $full = Join-Path $repoRoot ($relative -replace '/', [System.IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        throw "Required $Reason file not found in source tree: $relative"
    }
    if (-not (Test-SnapshotSelectionIncludes -RelativePath $relative)) {
        throw "Lean snapshot rules exclude required $Reason path: $relative"
    }
}
$importTargets = Get-ImportDecompTargets -Root $repoRoot
if ($importTargets.Count -eq 0) {
    throw 'No src/import decomp include targets were found.'
}
$sysImportCount = 0
foreach ($target in $importTargets) {
    Assert-IncludedPath -RelativePath $target -Reason 'src/import decomp include'
    if ((ConvertTo-Smash64DSSnapshotPath -Path $target) -match '^decomp/BattleShip-main/decomp/src/sys/') {
        $sysImportCount++
    }
}
if ($sysImportCount -eq 0) {
    throw 'No imported BattleShip src/sys targets were found; Makefile sys wrapper coverage cannot be confirmed.'
}
$relocAssets = Get-MakefileRelocAssets -Root $repoRoot
if ($relocAssets.Count -eq 0) {
    throw 'No Makefile NDS_*_RELOC_FILES assets were found.'
}
foreach ($asset in $relocAssets) {
    $relative = ConvertTo-Smash64DSSnapshotPath -Path (
        Join-Path 'decomp/BattleShip-main/BattleShip_o2r' $asset)
    Assert-IncludedPath -RelativePath $relative -Reason 'Makefile O2R reloc asset'
}
$modeLabel = if ($Archive) { "archive=$archivePath" } else { "source=$repoRoot" }
Write-Output ("Snapshot build context passed: mode={0}, {1}, importTargets={2}, sysImportTargets={3}, o2rAssets={4}" -f
    $Mode,
    $modeLabel,
    $importTargets.Count,
    $sysImportCount,
    $relocAssets.Count)
