param(
    # 150 -> 160 (owner, 2026-08-01), for the owner's own addition to the
    # efficiency guidance -- prefer larger slices of work, do not soak 120 s for
    # a 60 s match, do not spend a ROM build on a change that does not need one.
    # 160 -> 170 (cycle 79, 2026-08-05): the file sat at 163 authored lines from
    # the owner's own subagent-switch and efficiency additions, and this checker
    # is hand-run, so the overage went unnoticed. Raised rather than trimmed:
    # the cap exists to stop AGENTS.md accumulating derived detail, not to
    # bound what the owner puts in it.
    # 170 -> 200 (P2-1M, 2026-08-19), same rationale a third time: the file sits
    # at 184 authored lines of owner-authored rules -- the DS Visual Fidelity
    # section and the subagent-switch modes are both the owner's own text, and
    # this row has no licence to trim them.
    [int]$AgentsMaxLines = 200,
    # 45 -> 55 (cycle 79, 2026-08-05): Hard Rules sat at 49 lines of owner
    # rules; same rationale as the file cap above.
    [int]$AgentsMaxSectionLines = 55,
    # Raised from 150 to 200 by the owner, 2026-07-31.
    [int]$HandoffMaxLines = 200
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
    'PROJECT_GOAL.md',
    'docs/HANDOFF.md',
    'docs/HARNESSES.md',
    'docs/KNOWN_ISSUES.md',
    # P2-1M (2026-08-19). This list named the two P1-era planning surfaces --
    # `Smash64DS_Runtime2_SwitchPlan.md` and `P1_EXECUTION_BOARD.md` -- long
    # after the P2 restructure archived both, so the checker threw on its very
    # first loop and every assertion below it had been dead since. It now
    # requires the surfaces P2 actually runs on.
    'docs/P2_EXECUTION_BOARD.md',
    'docs/P2_PLAN.md',
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
# The CODEGRAPH_START/END block is tool-injected, not authored guidance, and the
# same text reaches every agent through the global instructions anyway. The line
# cap exists to keep the OWNER's rules lean, so a managed block should not eat the
# owner's budget -- on 2026-07-29 it was the difference between 167 and 157 and
# would have forced real rules to be deleted to make room for boilerplate. Section
# limits below still count it, so it cannot grow without bound.
$authoredLines = @()
$inManagedBlock = $false
foreach ($line in $agentsLines) {
    if ($line -match '<!--\s*CODEGRAPH_START\s*-->') { $inManagedBlock = $true }
    if (-not $inManagedBlock) { $authoredLines += $line }
    if ($line -match '<!--\s*CODEGRAPH_END\s*-->') { $inManagedBlock = $false }
}
if ($authoredLines.Count -gt $AgentsMaxLines) {
    Fail-Docs ("AGENTS.md is too long: $($authoredLines.Count) authored lines " +
        "($($agentsLines.Count) with the managed CodeGraph block)")
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

# AGENTS.md has said "HANDOFF.md should be 150 lines max" for many cycles with
# nothing enforcing it, and on 2026-07-29 it was found at 177 -- discovered only
# because `Get-Content | Measure-Object -Line` had reported a confident, wrong 148
# and a manual recount disagreed. A cap that is documented but unmeasured is not a
# cap. Count with .Count; Measure-Object -Line is not a line counter for an array
# that is already split into lines.
$handoffLines = @(Get-Content -LiteralPath (Join-Path $root 'docs/HANDOFF.md')).Count
if ($handoffLines -gt $HandoffMaxLines) {
    Fail-Docs ("docs/HANDOFF.md is too long: $handoffLines lines against a " +
        "$HandoffMaxLines-line cap. It is the restart surface only -- move durable " +
        'detail to its owning doc (the board owns queue and results, PERF_LEDGER ' +
        'measurements, KNOWN_ISSUES durable gaps, VERIFYING.md how a task is run).')
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

$board = Read-RepoText 'docs/P2_EXECUTION_BOARD.md'
$handoff = Read-RepoText 'docs/HANDOFF.md'
$harnesses = Read-RepoText 'docs/HARNESSES.md'
$known = Read-RepoText 'docs/KNOWN_ISSUES.md'
$porting = Read-RepoText 'docs/PORTING.md'
$agents = $agentsLines -join "`n"

# The P2 board's own load-bearing sections. `## Standing rules` carries the
# measurement law, `## Queue` the only dynamic queue, and the two match/publish
# laws are the ones a row is most likely to quietly soften.
foreach ($token in @(
    '## Standing rules', '## Queue', 'one-minute', 'SHA-256', 'Publish law'
)) {
    if (-not $board.Contains($token)) {
        Fail-Docs "P2 board is missing '$token'"
    }
}
if ($board -notmatch '(?m)^Updated:\s*\d{4}-\d{2}-\d{2}') {
    Fail-Docs 'P2 board update stamp is not parseable (want: Updated: YYYY-MM-DD)'
}
# The canonical published-ROM hash, in the one line format both this checker and
# a reader can find. P2-1M republished `smash64ds.nds` as the base ROM, so this
# pin now guards the P2 baseline rather than P1's.
if ($board -notmatch '(?m)^SHA-256\s+[0-9A-F]{64}\s*$') {
    Fail-Docs 'P2 board lacks the canonical published-ROM SHA-256'
}
if ($harnesses -notmatch 'HARNESS_INDEX_SOURCE:\s*scripts/lib/harness-registry\.ps1' -or
    $harnesses -notmatch 'verify-all\.ps1 -Profile Boundary -List' -or
    $harnesses -notmatch 'verify-all\.ps1 -Profile Latest -List') {
    Fail-Docs 'Harness registry authority is missing'
}
# P2-1M (2026-08-19): the first token was
# 'cosmetic exactness to one measured experiment' and AGENTS.md has not said
# that since the owner rewrote DS Visual Fidelity -- it now reads "Timebox
# exactness-polish to one measured experiment", because "cosmetic" was exactly
# the framing the round-1 visual-pass failure came from. Pin the sentence that
# is actually load-bearing, not the retired adjective.
# Tokens must not span a line wrap: `$agents` is the file joined with "`n", so
# 'exactness-polish to one measured experiment' cannot match text that wraps
# after "measured". Keep every token inside one source line.
foreach ($token in @(
    'Timebox exactness-polish to one measured',
    'artifacts/visibility', 'mechanically equivalent',
    'third A', 'ticks, FPS'
)) {
    if (-not $agents.Contains($token)) {
        Fail-Docs "AGENTS.md is missing '$token'"
    }
}
# The likeness figure is a fidelity-contract quantity owned by ARCHITECTURE.md
# (Fidelity Boundary). AGENTS.md carried a duplicate until 0204c54329 removed
# it; assert the fact at its owner rather than restoring the duplication.
if (-not (Read-RepoText 'docs/ARCHITECTURE.md').Contains(
        'Presentation targets roughly 90% overall likeness')) {
    Fail-Docs 'ARCHITECTURE.md lost the presentation likeness target'
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
