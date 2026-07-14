param(
    [string]$Python = 'python'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$checker = Join-Path $PSScriptRoot 'check_pupupu_water_aot.py'

if (-not (Test-Path -LiteralPath $checker -PathType Leaf)) {
    throw "Pupupu water AOT checker not found: $checker"
}
if ($null -eq (Get-Command $Python -ErrorAction SilentlyContinue)) {
    throw "Python command not found: $Python"
}

& $Python -B $checker --repo-root $root
if ($LASTEXITCODE -ne 0) {
    throw 'The deterministic Pupupu water AOT corpus contract failed.'
}

Write-Output 'Pupupu water AOT corpus contract passed.'
