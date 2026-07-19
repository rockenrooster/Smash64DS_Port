[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$SourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$ManifestPath = Join-Path $SourceRoot 'docs\publish\publish_manifest.json'
$Destination = [IO.Path]::GetFullPath('D:\Stuff\DevFolder\smash64ds-publish').TrimEnd('\', '/')

function Stop-Export {
    param([string]$Message)
    throw [InvalidOperationException]::new($Message)
}

function Assert-ExportPath {
    param([string]$RelativePath)

    $normalized = $RelativePath.Replace('\', '/')
    if ([IO.Path]::IsPathRooted($RelativePath) -or
        $normalized -match '(^|/)\.\.(/|$)') {
        Stop-Export "unsafe manifest path: $RelativePath"
    }

    $parts = $normalized.Split('/', [StringSplitOptions]::RemoveEmptyEntries)
    $excludedParts = @(
        'logs', 'artifacts', 'docs', 'prompts', '.agents', '.zcode', '.github',
        'emulators', 'assets', 'decomp', 'BattleShip_o2r', 'relocData'
    )
    foreach ($part in $parts) {
        if ($excludedParts -icontains $part) {
            Stop-Export "excluded path entered the public allowlist: $RelativePath"
        }
    }

    if (@('AGENTS.md', 'architect.md', 'worktree_report.md') -icontains $normalized -or
        $normalized -match '(?i)(^|/)baserom' -or
        $normalized -match '(?i)\.(z64|n64|v64|nds|o2r)$') {
        Stop-Export "forbidden public path: $RelativePath"
    }
}

if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    Stop-Export "publish manifest is absent: $ManifestPath"
}

$manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json
if ($manifest.shipping_allowlist_state -ne 'P2_SAFE_INPUT_TO_P3') {
    Stop-Export (
        'publish manifest is not approved for export: ' +
        [string]$manifest.shipping_allowlist_state
    )
}

$rows = @($manifest.shipping_allowlist)
if (-not $rows.Count) {
    Stop-Export 'publish manifest has an empty shipping allowlist'
}

$seen = [Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase
)
foreach ($row in $rows) {
    if (@('PORT-CODE', 'GENERATED-METADATA') -notcontains $row.bucket) {
        Stop-Export "unapproved bucket for $($row.path): $($row.bucket)"
    }
    Assert-ExportPath $row.path
    if (-not $seen.Add($row.path)) {
        Stop-Export "duplicate public path in manifest: $($row.path)"
    }

    $source = Join-Path $SourceRoot $row.path
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        Stop-Export "allowlisted source file is absent: $($row.path)"
    }
    $item = Get-Item -LiteralPath $source
    if ($item.Length -ne [int64]$row.bytes) {
        Stop-Export (
            "source size drift for $($row.path): expected $($row.bytes), got $($item.Length)"
        )
    }
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
    if ($hash -ine $row.sha256) {
        Stop-Export (
            "source hash drift for $($row.path): expected $($row.sha256), got $hash"
        )
    }
}

$expectedParent = [IO.Path]::GetFullPath('D:\Stuff\DevFolder').TrimEnd('\', '/')
if ([IO.Path]::GetDirectoryName($Destination) -ine $expectedParent -or
    [IO.Path]::GetFileName($Destination) -ine 'smash64ds-publish') {
    Stop-Export "refusing unsafe export destination: $Destination"
}

if (Test-Path -LiteralPath $Destination) {
    $destinationItem = Get-Item -LiteralPath $Destination -Force
    if (-not $destinationItem.PSIsContainer -or $destinationItem.LinkType) {
        Stop-Export "export destination is not a plain directory: $Destination"
    }
    foreach ($child in Get-ChildItem -LiteralPath $Destination -Force) {
        if ($child.Name -ne '.git') {
            Remove-Item -LiteralPath $child.FullName -Recurse -Force
        }
    }
} else {
    New-Item -ItemType Directory -Path $Destination | Out-Null
}

foreach ($row in $rows) {
    $source = Join-Path $SourceRoot $row.path
    $target = Join-Path $Destination $row.path
    $targetParent = Split-Path -Parent $target
    New-Item -ItemType Directory -Force -Path $targetParent | Out-Null
    Copy-Item -LiteralPath $source -Destination $target

    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash
    if ($hash -ine $row.sha256) {
        Stop-Export "copied-file hash mismatch for $($row.path)"
    }
}

$templates = @(
    @{ Source = 'README.md'; Target = 'README.md' },
    @{ Source = 'NOTICE.md'; Target = 'NOTICE.md' },
    @{ Source = '.gitignore'; Target = '.gitignore' }
)
foreach ($template in $templates) {
    $source = Join-Path $PSScriptRoot (Join-Path 'templates' $template.Source)
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        Stop-Export "public template is absent: $($template.Source)"
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $Destination $template.Target)
}

$exported = @(Get-ChildItem -LiteralPath $Destination -Recurse -File -Force |
    Where-Object { $_.FullName -notlike "$Destination\.git\*" })
$bytes = ($exported | Measure-Object -Property Length -Sum).Sum
Write-Host "Public export complete: $Destination"
Write-Host "Files: $($exported.Count)"
Write-Host "Bytes: $bytes"
Write-Host "Manifest entries: $($rows.Count); public templates: $($templates.Count)"
