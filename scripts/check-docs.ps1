param(
    [int]$AgentsMaxLines = 150,
    [int]$AgentsMaxSectionLines = 45
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
function Fail-Docs {
    param([string]$Message)
    throw "Docs check failed: $Message"
}
function Read-RepoText {
    param([string]$RelativePath)
    return Get-Content -LiteralPath (Join-Path $root $RelativePath) -Raw
}
$requiredDocs = @(
    'docs/README.md',
    'docs/ARCHITECTURE.md',
    'docs/DECOMP_MAP.md',
    'docs/DIAGNOSTIC_REFERENCE.md',
    'docs/EMULATOR_STRATEGY.md',
    'docs/GOAL_DEBUGGING.md',
    'docs/HANDOFF.md',
    'docs/HARNESSES.md',
    'docs/KNOWN_ISSUES.md',
    'docs/NEXT_BOUNDARY_QUEUE.md',
    'docs/PORTING.md',
    'docs/ROADMAP.md',
    'docs/STATUS.md',
    'docs/VERIFYING.md'
)
foreach ($doc in $requiredDocs) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $doc))) {
        Fail-Docs "missing required doc: $doc"
    }
}
$agentsPath = Join-Path $root 'AGENTS.md'
$agentsLines = @(Get-Content -LiteralPath $agentsPath)
if ($agentsLines.Count -gt $AgentsMaxLines) {
    Fail-Docs "AGENTS.md is too long: $($agentsLines.Count) lines, budget=$AgentsMaxLines"
}
$sectionName = 'preamble'
$sectionStart = 0
for ($i = 0; $i -le $agentsLines.Count; $i++) {
    $isHeading = ($i -lt $agentsLines.Count) -and ($agentsLines[$i] -match '^##\s+(.+)$')
    if ($isHeading -or ($i -eq $agentsLines.Count)) {
        $sectionLength = $i - $sectionStart
        if ($sectionLength -gt $AgentsMaxSectionLines) {
            Fail-Docs "AGENTS.md section '$sectionName' is too long: $sectionLength lines, budget=$AgentsMaxSectionLines"
        }
        if ($isHeading) {
            $sectionName = $Matches[1]
            $sectionStart = $i
        }
    }
}
$docsIndex = Read-RepoText 'docs/README.md'
$docFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'docs') -Filter '*.md' -File | ForEach-Object { $_.Name } | Sort-Object)
foreach ($docFile in $docFiles) {
    if ($docFile -eq 'README.md') { continue }
    if ($docsIndex -notmatch [regex]::Escape($docFile)) {
        Fail-Docs "docs/README.md does not index $docFile"
    }
}
$harnessDoc = Read-RepoText 'docs/HARNESSES.md'
if ($harnessDoc -notmatch 'HARNESS_INDEX_SOURCE:\s*scripts/lib/harness-registry\.ps1') {
    Fail-Docs 'docs/HARNESSES.md is missing the harness index source marker.'
}
if ($harnessDoc -notmatch 'verify-all\.ps1 -Profile Full -List') {
    Fail-Docs 'docs/HARNESSES.md must document the generated harness-index command.'
}
$status = Read-RepoText 'docs/STATUS.md'
$handoff = Read-RepoText 'docs/HANDOFF.md'
$boundary = @(Get-Smash64DSVerifyPlan -Profile Boundary)
foreach ($record in $boundary) {
    foreach ($pair in @(@('docs/STATUS.md', $status), @('docs/HANDOFF.md', $handoff))) {
        $docName = $pair[0]
        $text = $pair[1]
        $tokens = @($record.Name, $record.Harness, $record.Script) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        $found = $false
        foreach ($token in $tokens) {
            if ($text.Contains($token)) { $found = $true; break }
        }
        if (-not $found) {
            Fail-Docs "$docName does not reference current boundary entry '$($record.Name)'."
        }
    }
}
$sourceFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -Include *.c,*.h -File) +
    @(Get-ChildItem -LiteralPath (Join-Path $root 'include') -Recurse -Include *.h -File)
$knownIssues = Read-RepoText 'docs/KNOWN_ISSUES.md'
$knownStatus = $status + "`n" + $knownIssues
$markerPattern = 'NDS_ARCH_(?:STUB|DEFERRED):\s*([A-Za-z0-9_.:/-]+)'
foreach ($file in $sourceFiles) {
    $matches = [regex]::Matches((Get-Content -LiteralPath $file.FullName -Raw), $markerPattern)
    foreach ($match in $matches) {
        $token = $match.Groups[1].Value
        if (-not $knownStatus.Contains($token)) {
            $relative = [System.IO.Path]::GetRelativePath($root, $file.FullName)
            Fail-Docs "source marker '$token' in $relative is not documented in STATUS.md or KNOWN_ISSUES.md"
        }
    }
}
$registryCount = @(Get-Smash64DSHarnessRegistry).Count
Write-Output "Docs check passed: docs=$($docFiles.Count), registryEntries=$registryCount, AGENTS.md=$($agentsLines.Count) lines."
