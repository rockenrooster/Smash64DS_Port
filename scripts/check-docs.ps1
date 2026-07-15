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
    'docs/goal-objective.md',
    'docs/HANDOFF.md',
    'docs/HARNESSES.md',
    'docs/KNOWN_ISSUES.md',
    'docs/NEXT_BOUNDARY_QUEUE.md',
    'docs/OPTIMIZATION_ROADMAP.md',
    'docs/optimization/NATIVE_RENDERER_PLAN.md',
    'docs/P1_EXECUTION_BOARD.md',
    'docs/PERF_LEDGER.md',
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
$indexedDocMatches = [regex]::Matches(
    $docsIndex, '\|\s*`([^`]+\.md)`\s*\|')
foreach ($match in $indexedDocMatches) {
    $indexedName = $match.Groups[1].Value
    if (-not (Test-Path -LiteralPath (Join-Path $root "docs\$indexedName"))) {
        Fail-Docs "docs/README.md indexes missing document $indexedName"
    }
}
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
$p1Board = Read-RepoText 'docs/P1_EXECUTION_BOARD.md'
$rootReadme = Read-RepoText 'README.md'
$roadmap = Read-RepoText 'docs/ROADMAP.md'
$nextBoundary = Read-RepoText 'docs/NEXT_BOUNDARY_QUEUE.md'
$optimizationRoadmap = Read-RepoText 'docs/OPTIMIZATION_ROADMAP.md'
$nativeRendererPlan = Read-RepoText 'docs/optimization/NATIVE_RENDERER_PLAN.md'
$porting = Read-RepoText 'docs/PORTING.md'
$diagnosticReference = Read-RepoText 'docs/DIAGNOSTIC_REFERENCE.md'
$knownIssues = Read-RepoText 'docs/KNOWN_ISSUES.md'
foreach ($pair in @(@('docs/STATUS.md', $status), @('docs/HANDOFF.md', $handoff))) {
    $lineCount = @($pair[1] -split "`r?`n").Count
    if ($lineCount -gt 150) {
        Fail-Docs "$($pair[0]) is too long: $lineCount lines, budget=150"
    }
}
foreach ($requiredBoardToken in @(
    'battle_playable_realtime',
    '2026-07-19 23:59 Central',
    'Laboratory profile-1 ROMs are evidence only',
    'July 16',
    'one-minute',
    'Integration/release',
    'Renderer implementation',
    'Gameplay + QA',
    'Performance research',
    '## Phase Evidence',
    'Artifact class',
    'Active median / P95',
    'Countdown / GO',
    'Early combat',
    'Late combat',
    'KO / rebirth',
    'Time Up / Results',
    'Integration decision',
    'canonical-profile-0'
)) {
    if (-not $p1Board.Contains($requiredBoardToken)) {
        Fail-Docs "docs/P1_EXECUTION_BOARD.md is missing '$requiredBoardToken'"
    }
}
$boardStampMatch = [regex]::Match(
    $p1Board,
    '(?m)^Updated:\s*(\d{4}-\d{2}-\d{2})\s+(\d{2}:\d{2})\s+Central\s*$')
if (-not $boardStampMatch.Success) {
    Fail-Docs 'docs/P1_EXECUTION_BOARD.md is missing a parseable Central update timestamp.'
}
$boardStamp = [datetime]::ParseExact(
    "$($boardStampMatch.Groups[1].Value) $($boardStampMatch.Groups[2].Value)",
    'yyyy-MM-dd HH:mm',
    [System.Globalization.CultureInfo]::InvariantCulture)
$centralZone = [System.TimeZoneInfo]::FindSystemTimeZoneById('Central Standard Time')
$boardOffset = $centralZone.GetUtcOffset($boardStamp)
$boardStampCentral = [datetimeoffset]::new($boardStamp, $boardOffset)
$nowCentral = [System.TimeZoneInfo]::ConvertTime([datetimeoffset]::UtcNow, $centralZone)
$boardAge = $nowCentral - $boardStampCentral
$today = $nowCentral.Date
if (($boardAge.TotalMinutes -lt -5) -or ($boardAge.TotalHours -gt 36)) {
    Fail-Docs "docs/P1_EXECUTION_BOARD.md timestamp is stale or future-dated: $($boardStamp.ToString('yyyy-MM-dd HH:mm')) Central"
}
foreach ($pair in @(
    @('README.md', $rootReadme),
    @('docs/ROADMAP.md', $roadmap)
)) {
    foreach ($requiredToken in @('battle_playable_realtime', '2026-07-19 23:59', 'America/Chicago', 'P1_EXECUTION_BOARD.md')) {
        if (-not $pair[1].Contains($requiredToken)) {
            Fail-Docs "$($pair[0]) is missing current P1 token '$requiredToken'."
        }
    }
}
foreach ($pair in @(
    @('README.md', $rootReadme),
    @('docs/ROADMAP.md', $roadmap),
    @('docs/STATUS.md', $status),
    @('docs/HANDOFF.md', $handoff),
    @('docs/P1_EXECUTION_BOARD.md', $p1Board),
    @('docs/OPTIMIZATION_ROADMAP.md', $optimizationRoadmap),
    @('docs/DIAGNOSTIC_REFERENCE.md', $diagnosticReference)
)) {
    if (($pair[1] -match '(?i)current\s+active\s+(?:proof\s+)?boundary\s+(?:remains|is)\s+.*161/162') -or
        ($pair[1] -match '(?i)current\s+Boundary/Latest\s+pair\s+is.{0,160}161/162')) {
        Fail-Docs "$($pair[0]) still identifies modes 161/162 as the active boundary."
    }
}
if ($roadmap -match '(?i)`?battle_playable`?.{0,80}\bDeferred\b') {
    Fail-Docs 'docs/ROADMAP.md still presents battle_playable as deferred.'
}
$nextBoundaryLines = @($nextBoundary -split "`r?`n").Count
if (($nextBoundaryLines -gt 24) -or
    ($nextBoundary -match '(?i)compact current queue|current truth')) {
    Fail-Docs 'docs/NEXT_BOUNDARY_QUEUE.md must remain a short historical redirect.'
}
if (($optimizationRoadmap -match '(?i)this is the active optimization plan') -or
    ($optimizationRoadmap -match '(?i)this file owns priority')) {
    Fail-Docs 'docs/OPTIMIZATION_ROADMAP.md must not claim active-queue authority.'
}
if ((-not $nativeRendererPlan.Contains('P1_EXECUTION_BOARD.md')) -or
    (-not $nativeRendererPlan.Contains('implementation contract'))) {
    Fail-Docs 'docs/optimization/NATIVE_RENDERER_PLAN.md must remain the renderer technical contract under the P1 board.'
}
if ((-not $rootReadme.Contains('Profile-1')) -or
    (-not $status.Contains('profile-1')) -or
    (-not $p1Board.Contains('canonical/shipped pair')) -or
    (-not $p1Board.Contains('Profile-1 M2 samples and profile-2 forensic samples'))) {
    Fail-Docs 'active docs must distinguish canonical artifacts from profile-1 laboratory evidence.'
}
$canonicalHashMatch = [regex]::Match(
    $p1Board,
    '(?m)^SHA-256\s+([0-9A-F]{64})\s*$')
if (-not $canonicalHashMatch.Success) {
    Fail-Docs 'docs/P1_EXECUTION_BOARD.md lacks a full canonical SHA-256.'
}
$canonicalHash = $canonicalHashMatch.Groups[1].Value
foreach ($pair in @(
    @('README.md', $rootReadme),
    @('docs/STATUS.md', $status),
    @('docs/HANDOFF.md', $handoff)
)) {
    if (-not $pair[1].Contains($canonicalHash)) {
        Fail-Docs "$($pair[0]) canonical SHA does not match the P1 board."
    }
}
if (($docsIndex -notmatch [regex]::Escape('optimization/NATIVE_RENDERER_PLAN.md')) -or
    ($docsIndex -notmatch '\|\s*`goal-objective\.md`\s*\|\s*Objective contract\s*\|')) {
    Fail-Docs 'docs/README.md must index the Native Renderer Plan and label goal-objective as the objective contract.'
}
$futurePortingHeadings = [regex]::Matches(
    $porting,
    '(?m)^##\s+(\d{4}-\d{2}-\d{2})\s+-\s+(.+)$')
foreach ($match in $futurePortingHeadings) {
    $headingDate = [datetime]::ParseExact(
        $match.Groups[1].Value,
        'yyyy-MM-dd',
        [System.Globalization.CultureInfo]::InvariantCulture)
    if (($headingDate.Date -gt $today) -and ($match.Groups[2].Value -notmatch '(?i)\[planned\]')) {
        $erratumPrefix = "PORTING_DATE_ERRATUM: $($match.Groups[1].Value) | $($match.Groups[2].Value) |"
        if (-not $porting.Contains($erratumPrefix)) {
            Fail-Docs "docs/PORTING.md has unexplained future-dated heading: $($match.Value)"
        }
    }
}
foreach ($requiredIssueToken in @(
    '## P1 Release Blockers',
    'M2–M4',
    'Mario Fireball',
    'FGM/voice',
    'one-minute',
    'two-ROM',
    'dated captures',
    'manual user retest'
)) {
    if (-not $knownIssues.Contains($requiredIssueToken)) {
        Fail-Docs "docs/KNOWN_ISSUES.md P1 blocker table is missing '$requiredIssueToken'."
    }
}
foreach ($staleIssuePattern in @(
    'Percent/stock still use no-op',
    'otherwise-black canonical lower screen',
    'Fireball has no visible projectile and intermittent freezes remain unlocalized'
)) {
    if ($knownIssues.Contains($staleIssuePattern)) {
        Fail-Docs "docs/KNOWN_ISSUES.md retains stale current claim '$staleIssuePattern'."
    }
}
foreach ($requiredAgentToken in @(
    'highest-impact unowned red P1 row',
    'Agents share the live tree',
    'runner slot',
    'Only integration/release edits',
    'keep/revert threshold',
    'Do not begin P2'
)) {
    if ((Get-Content -LiteralPath $agentsPath -Raw) -notmatch [regex]::Escape($requiredAgentToken)) {
        Fail-Docs "AGENTS.md is missing concurrent-lane policy '$requiredAgentToken'."
    }
}
$boundary = @(Get-Smash64DSVerifyPlan -Profile Boundary)
foreach ($record in $boundary) {
    foreach ($pair in @(
        @('docs/STATUS.md', $status),
        @('docs/HANDOFF.md', $handoff),
        @('docs/P1_EXECUTION_BOARD.md', $p1Board)
    )) {
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
