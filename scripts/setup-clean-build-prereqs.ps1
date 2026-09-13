param(
    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,

    [string]$TargetRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'

function Resolve-Directory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw ("{0} does not exist or is not a directory: {1}" -f $Label, $Path)
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

$SourceRoot = Resolve-Directory -Path $SourceRoot -Label 'SourceRoot'
$TargetRoot = Resolve-Directory -Path $TargetRoot -Label 'TargetRoot'

$targetPrefix = @(& git -C $TargetRoot rev-parse --show-prefix 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw ('TargetRoot is not a Git worktree: {0}' -f ($targetPrefix -join "`n"))
}
if (($targetPrefix -join '') -ne '') {
    throw ('TargetRoot must be the Git worktree root; Git reports prefix: {0}' -f
        ($targetPrefix -join ''))
}
if ($SourceRoot.Equals($TargetRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'SourceRoot and TargetRoot must be different worktrees.'
}

# This is intentionally a fixed allowlist. Every target is an ignored child of
# the checkout, and none may contain a tracked path. Do not generalize this to
# decomp/, artifacts/, or any caller-supplied relative path.
$prerequisites = @(
    'decomp/BattleShip-main/BattleShip_o2r',
    'decomp/BattleShip-main/decomp/build',
    'decomp/BattleShip-main/decomp/assets',
    'assets'
)

$validated = @()
foreach ($relative in $prerequisites) {
    $source = Join-Path $SourceRoot $relative
    $target = Join-Path $TargetRoot $relative
    $parent = Split-Path -Parent $target

    if (-not (Test-Path -LiteralPath $source -PathType Container)) {
        throw ('Provisioned source is missing required derived tree: {0}' -f $source)
    }
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        throw ('Tracked target parent is missing; refusing to create it: {0}' -f $parent)
    }
    if ($null -ne (Get-Item -LiteralPath $target -Force -ErrorAction SilentlyContinue)) {
        throw ('Target already exists; this script never deletes or overlays paths: {0}' -f $target)
    }

    $tracked = @(& git -C $TargetRoot ls-files -- $relative 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw ('git ls-files failed for {0}: {1}' -f $relative, ($tracked -join "`n"))
    }
    if ($tracked.Count -ne 0) {
        throw ('Refusing to link {0}: Git tracks {1} path(s) below it.' -f $relative, $tracked.Count)
    }

    $ignoreProbe = ($relative.TrimEnd('/') + '/.clean-build-prereq-probe')
    & git -C $TargetRoot check-ignore --quiet --no-index -- $ignoreProbe
    if ($LASTEXITCODE -ne 0) {
        throw ('Refusing to link {0}: the target is not covered by an ignore rule.' -f $relative)
    }

    $validated += [PSCustomObject]@{
        Relative = $relative
        Source = $source
        Target = $target
    }
}

$trackedDrift = @(& git -C $TargetRoot status --short --untracked-files=no 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw ('git status failed before setup: {0}' -f ($trackedDrift -join "`n"))
}
if ($trackedDrift.Count -ne 0) {
    throw ('Target worktree has tracked changes before setup; refusing to provision: {0}' -f
        ($trackedDrift -join '; '))
}

foreach ($item in $validated) {
    New-Item -ItemType Junction -Path $item.Target -Target $item.Source -ErrorAction Stop | Out-Null
    Write-Output ('linked {0} -> {1}' -f $item.Relative, $item.Source)
}

$trackedDrift = @(& git -C $TargetRoot status --short --untracked-files=no 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw ('git status failed after setup: {0}' -f ($trackedDrift -join "`n"))
}
if ($trackedDrift.Count -ne 0) {
    throw ('Tracked-file drift detected after setup: {0}' -f ($trackedDrift -join '; '))
}

Write-Output ('CLEAN_BUILD_PREREQS_OK target={0} linked={1}' -f $TargetRoot, $validated.Count)
