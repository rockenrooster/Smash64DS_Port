[CmdletBinding()]
param(
    [string[]]$RuntimeExport = @(),
    [switch]$ReconcileExisting,
    [switch]$ReconciliationJson
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$generator = Join-Path $PSScriptRoot 'generate_battle_playable_texture_census.py'

if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) {
    throw "Texture census generator is absent: $generator"
}

$python = Get-Command python -ErrorAction Stop
$generatorArgs = @($generator, '--repo-root', $repoRoot, '--check')
foreach ($path in $RuntimeExport) {
    $generatorArgs += @('--runtime-export', $path)
}
if ($ReconcileExisting) {
    $generatorArgs += '--reconcile-existing'
}
if ($ReconciliationJson) {
    $generatorArgs += '--reconciliation-json'
}
& $python.Source @generatorArgs
if ($LASTEXITCODE -ne 0) {
    throw "Battle Playable texture census failed with exit $LASTEXITCODE."
}
