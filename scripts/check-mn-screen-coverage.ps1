param([string]$Python = 'python')

# P2-1j. THE SCREEN ASSET-COVERAGE GATE.
#
# Three owner visual passes in a row found on-screen elements the shell simply
# never converted, and every one arrived through the owner's eye rather than
# through tooling. The owner named the root cause twice: the source code is all
# there, so verification must come from it, exhaustively.
#
# scripts/menus/audit_mn_screen_coverage.py is that comparison -- every sprite
# each `mn*` scene constructs, against every kit token our shell draws on the
# same screen -- and this wrapper is what makes it run on a kept checkpoint
# rather than when somebody remembers. It is a STATIC checker: no ROM, no
# emulator, ~1 s, so it sits beside check-gbi-decode-fixtures.ps1 and
# check-nds-particle-banks.ps1 in verify-all.ps1's unconditional block rather
# than inside a runtime verifier.
#
# It fails on an unexplained delta AND on a stale allowlist entry, which is the
# half that keeps the excuses honest: a delta that gets fixed takes its
# allowlist entry with it or the run goes red.

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$audit = Join-Path $PSScriptRoot 'menus/audit_mn_screen_coverage.py'
$allowlist = Join-Path $PSScriptRoot 'menus/mn_screen_coverage_allowlist.json'

if ($null -eq (Get-Command $Python -ErrorAction SilentlyContinue)) {
    throw "Python command not found: $Python"
}
foreach ($required in @($audit, $allowlist)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Screen coverage audit is missing a file it needs: $required"
    }
}

# The audit reads the BattleShip reference, which is fetched rather than
# tracked. A tree without it cannot run this gate, and saying so beats a
# confusing parse error out of the tool.
$decomp = Join-Path $root 'decomp/BattleShip-main/decomp/src/mn'
if (-not (Test-Path -LiteralPath $decomp)) {
    throw ("The BattleShip reference is not present ($decomp). Run " +
           'scripts/fetch-battleship-reference.ps1 before verifying.')
}

$out = Join-Path $root 'artifacts/verification/mn-screen-coverage.json'
& $Python $audit --json $out
if ($LASTEXITCODE -ne 0) {
    throw ("Screen asset coverage FAILED. Either the element is missing from " +
           'the shell and belongs on the screen, or the delta is explained ' +
           "and belongs in scripts/menus/mn_screen_coverage_allowlist.json " +
           'with the ruling that accepted it. A STALE entry fails the same ' +
           'way: delete it when its delta is fixed.')
}
Write-Output 'check-mn-screen-coverage: PASS'
