param(
    [int]$AgentsMaxLines = 150,
    [int]$AgentsMaxSectionLines = 45
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')

function Fail-Docs([string]$Message) { throw "Docs check failed: $Message" }
function Read-RepoText([string]$Path) {
    Get-Content -LiteralPath (Join-Path $root $Path) -Raw
}

$required = @(
    'docs/README.md',
    'docs/ARCHITECTURE.md',
    'docs/DECOMP_MAP.md',
    'docs/DIAGNOSTIC_REFERENCE.md',
    'docs/EMULATOR_STRATEGY.md',
    'docs/goal-objective.md',
    'docs/HANDOFF.md',
    'docs/HARNESSES.md',
    'docs/KNOWN_ISSUES.md',
    'docs/optimization/NATIVE_RENDERER_PLAN.md',
    'docs/P1_EXECUTION_BOARD.md',
    'docs/PERF_LEDGER.md',
    'docs/PORTING.md',
    'docs/VERIFYING.md'
)
foreach ($path in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $path))) {
        Fail-Docs "missing required doc: $path"
    }
}

$agentsPath = Join-Path $root 'AGENTS.md'
$agentsLines = @(Get-Content -LiteralPath $agentsPath)
if ($agentsLines.Count -gt $AgentsMaxLines) {
    Fail-Docs "AGENTS.md is too long: $($agentsLines.Count)"
}
$heading = 'preamble'
$start = 0
for ($i = 0; $i -le $agentsLines.Count; $i++) {
    $next = ($i -lt $agentsLines.Count) -and ($agentsLines[$i] -match '^##\s+(.+)$')
    if ($next -or $i -eq $agentsLines.Count) {
        if (($i - $start) -gt $AgentsMaxSectionLines) {
            Fail-Docs "AGENTS.md section '$heading' is too long"
        }
        if ($next) { $heading = $Matches[1]; $start = $i }
    }
}

$index = Read-RepoText 'docs/README.md'
$indexed = [regex]::Matches($index, '\|\s*`([^`]+\.md)`\s*\|')
foreach ($match in $indexed) {
    $path = Join-Path $root "docs\$($match.Groups[1].Value)"
    if (-not (Test-Path -LiteralPath $path)) {
        Fail-Docs "docs/README.md indexes missing document $path"
    }
}
foreach ($file in Get-ChildItem (Join-Path $root 'docs') -File -Filter '*.md') {
    if ($file.Name -ne 'README.md' -and
        $index -notmatch [regex]::Escape($file.Name)) {
        Fail-Docs "docs/README.md does not index $($file.Name)"
    }
}

$board = Read-RepoText 'docs/P1_EXECUTION_BOARD.md'
$handoff = Read-RepoText 'docs/HANDOFF.md'
$harnesses = Read-RepoText 'docs/HARNESSES.md'
$native = Read-RepoText 'docs/optimization/NATIVE_RENDERER_PLAN.md'
$known = Read-RepoText 'docs/KNOWN_ISSUES.md'
$porting = Read-RepoText 'docs/PORTING.md'
$agents = $agentsLines -join "`n"

if (@($handoff -split "`r?`n").Count -gt 100) {
    Fail-Docs 'docs/HANDOFF.md exceeds 100 lines'
}
foreach ($token in @(
    'battle_playable_realtime', '2026-07-19 23:59 Central',
    '## Phase Evidence', '## Acceptance Matrix', 'M2 ', 'M3 ', 'M4 ',
    'one-minute', 'SHA-256'
)) {
    if (-not $board.Contains($token)) {
        Fail-Docs "P1 board is missing '$token'"
    }
}
if ($board -notmatch '(?m)^Updated:\s*\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}\s+Central\s*$') {
    Fail-Docs 'P1 board update timestamp is not parseable'
}
if ($board -notmatch '(?m)^SHA-256\s+[0-9A-F]{64}\s*$') {
    Fail-Docs 'P1 board lacks the canonical SHA-256'
}
if (-not $native.Contains('implementation contract') -or
    -not $native.Contains('P1_EXECUTION_BOARD.md')) {
    Fail-Docs 'Native renderer plan lost its ownership contract'
}
if ($harnesses -notmatch 'HARNESS_INDEX_SOURCE:\s*scripts/lib/harness-registry\.ps1' -or
    $harnesses -notmatch 'verify-all\.ps1 -Profile Boundary -List' -or
    $harnesses -notmatch 'verify-all\.ps1 -Profile Latest -List') {
    Fail-Docs 'Harness registry authority is missing'
}
foreach ($token in @(
    'Presentation targets roughly 90% overall likeness',
    'cosmetic exactness to one measured experiment',
    'artifacts/visibility', 'Never approximate gameplay semantics',
    'third A', 'ticks, FPS'
)) {
    if (-not $agents.Contains($token)) {
        Fail-Docs "AGENTS.md is missing '$token'"
    }
}

$boundary = @(Get-Smash64DSVerifyPlan -Profile Boundary)
foreach ($record in $boundary) {
    foreach ($text in @($board, $handoff)) {
        if (-not ($text.Contains($record.Name) -or
                  $text.Contains($record.Harness) -or
                  $text.Contains($record.Script))) {
            Fail-Docs "active docs omit Boundary entry '$($record.Name)'"
        }
    }
}

$today = [System.TimeZoneInfo]::ConvertTimeBySystemTimeZoneId(
    [datetimeoffset]::UtcNow, 'Central Standard Time').Date
foreach ($match in [regex]::Matches($porting, '(?m)^##\s+(\d{4}-\d{2}-\d{2})\s+-\s+(.+)$')) {
    $date = [datetime]::ParseExact($match.Groups[1].Value, 'yyyy-MM-dd',
        [System.Globalization.CultureInfo]::InvariantCulture)
    if ($date.Date -gt $today -and $match.Groups[2].Value -notmatch '(?i)\[planned\]' -and
        -not $porting.Contains("PORTING_DATE_ERRATUM: $($match.Groups[1].Value) | $($match.Groups[2].Value) |")) {
        Fail-Docs "unexplained future PORTING heading: $($match.Value)"
    }
}

$markerPattern = 'NDS_ARCH_(?:STUB|DEFERRED):\s*([A-Za-z0-9_.:/-]+)'
foreach ($file in @(Get-ChildItem (Join-Path $root 'src') -Recurse -Include *.c,*.h -File) +
                  @(Get-ChildItem (Join-Path $root 'include') -Recurse -Include *.h -File)) {
    foreach ($match in [regex]::Matches((Get-Content $file.FullName -Raw), $markerPattern)) {
        if (-not $known.Contains($match.Groups[1].Value)) {
            Fail-Docs "undocumented source marker '$($match.Groups[1].Value)'"
        }
    }
}

$count = @(Get-Smash64DSHarnessRegistry).Count
Write-Output "Docs check passed: docs=$($indexed.Count), registryEntries=$count, AGENTS.md=$($agentsLines.Count) lines."
