[CmdletBinding()]
param(
    # Sibling of the repository, derived so the checkout is not tied to one
    # machine's folder layout.
    [string]$Root = [IO.Path]::GetFullPath(
        (Join-Path $PSScriptRoot '..\..\..\smash64ds-publish'))
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Root = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
    throw "public tree is absent: $Root"
}

$files = @(Get-ChildItem -LiteralPath $Root -Recurse -File -Force |
    Where-Object { $_.FullName -notlike "$Root\.git\*" })
$relative = @{}
foreach ($file in $files) {
    $relative[$file.FullName] = $file.FullName.Substring($Root.Length + 1).Replace('\', '/')
}

$failures = [Collections.Generic.List[string]]::new()
$forbiddenNames = @($relative.Values | Where-Object {
    $_ -match '(?i)(^|/)(baserom[^/]*|[^/]+\.(z64|n64|v64|nds|o2r))$'
})
$forbiddenTrees = @($relative.Values | Where-Object {
    $_ -match '(?i)(^|/)(BattleShip_o2r|relocData)(/|$)' -or
    $_ -match '(?i)^assets/(audio|renderer)(/|$)'
})
$embeddedContent = @(@(
    'src/nds/nds_native_stage_owner.generated.inc',
    'src/nds/nds_native_fighter_owner.generated.inc'
) | Where-Object { Test-Path -LiteralPath (Join-Path $Root $_) })

if ($forbiddenNames) {
    $failures.Add("forbidden filenames: $($forbiddenNames -join ', ')")
}
if ($forbiddenTrees) {
    $failures.Add("forbidden content trees: $($forbiddenTrees -join ', ')")
}
if ($embeddedContent) {
    $failures.Add("embedded content includes: $($embeddedContent -join ', ')")
}

$largeBinaries = [Collections.Generic.List[string]]::new()
$magicHits = [Collections.Generic.List[string]]::new()
$stringHits = [Collections.Generic.List[string]]::new()
$strictUtf8 = [Text.UTF8Encoding]::new($false, $true)
$magicPatterns = @(
    @('80371240', 'N64 big-endian magic'),
    @('37804012', 'N64 byte-swapped magic'),
    @('40123780', 'N64 little-endian magic')
)

foreach ($file in $files) {
    $bytes = [IO.File]::ReadAllBytes($file.FullName)
    $isBinary = $bytes -contains 0
    if (-not $isBinary) {
        try {
            [void]$strictUtf8.GetString($bytes)
        } catch {
            $isBinary = $true
        }
    }
    if ($isBinary -and $file.Length -gt 256KB) {
        $largeBinaries.Add("$($relative[$file.FullName]) ($($file.Length) bytes)")
    }

    $hex = [Convert]::ToHexString($bytes)
    foreach ($pattern in $magicPatterns) {
        if ($hex.Contains($pattern[0], [StringComparison]::Ordinal)) {
            $magicHits.Add("$($relative[$file.FullName]): $($pattern[1])")
        }
    }

    $latin = [Text.Encoding]::Latin1.GetString($bytes)
    foreach ($needle in @('SMASH BROTHERS', 'D:\Stuff', $env:USERNAME)) {
        if ($latin.Contains($needle, [StringComparison]::Ordinal)) {
            $stringHits.Add("$($relative[$file.FullName]): $needle")
        }
    }
}

if ($largeBinaries.Count) {
    $failures.Add("binary files over 256 KiB: $($largeBinaries -join ', ')")
}
if ($magicHits.Count) {
    $failures.Add("N64 magic hits: $($magicHits -join ', ')")
}
if ($stringHits.Count) {
    $failures.Add("forbidden string hits: $($stringHits -join ', ')")
}

$bytesTotal = ($files | Measure-Object -Property Length -Sum).Sum
Write-Host "FILES=$($files.Count)"
Write-Host "BYTES=$bytesTotal"
Write-Host "MIB=$([Math]::Round($bytesTotal / 1MB, 3))"
Write-Host "FORBIDDEN_FILENAMES=$($forbiddenNames.Count)"
Write-Host "FORBIDDEN_TREES=$($forbiddenTrees.Count)"
Write-Host "EMBEDDED_CONTENT_FILES=$($embeddedContent.Count)"
Write-Host "LARGE_BINARIES=$($largeBinaries.Count)"
Write-Host "ROM_MAGIC_HITS=$($magicHits.Count)"
Write-Host "FORBIDDEN_STRING_HITS=$($stringHits.Count)"

if ($failures.Count) {
    throw "public leak audit failed: $($failures -join '; ')"
}
Write-Host 'LEAK_AUDIT=PASS'
