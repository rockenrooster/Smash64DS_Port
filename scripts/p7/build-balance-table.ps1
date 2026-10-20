<#
.SYNOPSIS
Builds the fighter balance sheet: every fighter's attack values as the game runs them.

.DESCRIPTION
Runs scripts/p7/balance_table.py over the current sources -- BattleShip's motion
scripts for the original twelve, the P4 export and the staging ROM for the Remix
and EXTRA fighters, scripts/p6 for Ike and Palutena -- and writes one
self-contained page, artifacts/balance/balance.html (double-click it to open in a
browser; no server needed), plus balance.json with the same values.

The values are the game's own data, so the output stays under artifacts/ and is
never committed.

.PARAMETER Out
Output folder (default: artifacts/balance under the repository).

.PARAMETER Open
Open the page in the default browser when it is written.

.EXAMPLE
pwsh -File scripts/p7/build-balance-table.ps1 -Open
#>
param(
    [string]$Out = '',
    [switch]$Open
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if ($Out -eq '') { $Out = Join-Path $root 'artifacts\balance' }
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$html = Join-Path $Out 'balance.html'
$json = Join-Path $Out 'balance.json'

Push-Location $root
try {
    python (Join-Path $PSScriptRoot 'balance_table.py') --html $html --json $json
    if ($LASTEXITCODE -ne 0) { throw "balance_table.py failed (exit $LASTEXITCODE)" }
} finally {
    Pop-Location
}
Write-Host "Balance sheet written: $html"
if ($Open) { Start-Process $html }
