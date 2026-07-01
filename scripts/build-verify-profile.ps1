param(
    [ValidateSet('Full','Latest','LatestFast','BoundaryDirect','Boundary','Regression','RegressionFast','Smoke','SmokeFast','Fighter','Direct','MenuChain')]
    [string]$Profile = 'Boundary',
    [string[]]$Only,
    [string]$From
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$plan = @(Get-Smash64DSVerifyPlan -Profile $Profile -Only $Only -From $From)
if ($plan.Count -eq 0) {
    throw "No verifiers selected for profile '$Profile'."
}
$builtDefault = $false
$seen = @{}
foreach ($record in $plan) {
    if (-not $record.Target -or -not $record.Build -or -not $record.Harness) {
        if (-not $builtDefault) {
            Write-Output 'Building default verification ROM.'
            & make -C $root TARGET=smash64ds BUILD=build NDS_DEV_SCENE_HARNESS=normal -B -j16
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            $builtDefault = $true
        }
        continue
    }
    $key = "$($record.Target)|$($record.Build)|$($record.Harness)"
    if ($seen.ContainsKey($key)) {
        continue
    }
    $seen[$key] = $true
    Write-Output "Building verifier output: $($record.Name)"
    & make -C $root TARGET=$($record.Target) BUILD=$($record.Build) NDS_DEV_SCENE_HARNESS=$($record.Harness) -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
Write-Output "Built verifier profile '$Profile' outputs."
