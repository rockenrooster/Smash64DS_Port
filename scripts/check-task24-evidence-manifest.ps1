param(
    [string]$Manifest = 'artifacts/performance/2026-07-18_task24-worktree-evidence-migration-manifest-v3.json'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$artifactsRoot = (Resolve-Path (Join-Path $root 'artifacts')).Path.TrimEnd('\')
$manifestPath = if ([IO.Path]::IsPathRooted($Manifest)) {
    [IO.Path]::GetFullPath($Manifest)
} else {
    [IO.Path]::GetFullPath((Join-Path $root $Manifest))
}
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Task 24 evidence manifest is missing: $manifestPath"
}

$manifestData = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$seen = [Collections.Generic.Dictionary[string,object]]::new(
    [StringComparer]::OrdinalIgnoreCase)
$mutableAliases = 0
foreach ($record in $manifestData.files) {
    if ($record.destinationPath) {
        $path = [IO.Path]::GetFullPath($record.destinationPath)
    } else {
        $path = [IO.Path]::GetFullPath((Join-Path (
            Join-Path $artifactsRoot $record.category) $record.relativePath))
        if (($record.category -eq 'visibility') -and
            ($record.relativePath -in @('latest.png', 'previous.png'))) {
            $mutableAliases++
            continue
        }
    }
    if (-not $path.StartsWith($artifactsRoot + '\',
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Task 24 evidence destination escaped artifacts: $path"
    }

    if ($seen.ContainsKey($path)) {
        $expected = $seen[$path]
        if (($expected.Bytes -ne [int64]$record.bytes) -or
            ($expected.Hash -ne $record.sha256)) {
            throw "Task 24 evidence records disagree for destination: $path"
        }
        continue
    }
    $seen[$path] = [pscustomobject]@{
        Bytes = [int64]$record.bytes
        Hash = [string]$record.sha256
    }
}

foreach ($entry in $seen.GetEnumerator()) {
    $path = $entry.Key
    $expected = $entry.Value
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Task 24 evidence destination is missing: $path"
    }
    $item = Get-Item -LiteralPath $path
    if ([int64]$item.Length -ne $expected.Bytes) {
        throw "Task 24 evidence size changed: $path"
    }
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
    if ($hash -ne $expected.Hash) {
        throw "Task 24 evidence hash changed: $path"
    }
}

Write-Output (
    'Task 24 evidence manifest passed: records={0} immutableDestinations={1} mutableAliases={2} failures=0' -f
    $manifestData.files.Count, $seen.Count, $mutableAliases)
