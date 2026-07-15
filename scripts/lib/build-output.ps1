function Resolve-Smash64DSBuildPath {
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][string]$Build
    )
    if ([System.IO.Path]::IsPathRooted($Build)) {
        return [System.IO.Path]::GetFullPath($Build)
    }
    if (($Build -like 'build*') -and
        (-not $Build.Contains('\')) -and
        (-not $Build.Contains('/'))) {
        return [System.IO.Path]::GetFullPath(
            (Join-Path (Join-Path $Root 'builds') $Build))
    }
    return [System.IO.Path]::GetFullPath((Join-Path $Root $Build))
}

function Resolve-Smash64DSBuildOutput {
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][string]$Target,
        [Parameter(Mandatory=$true)][string]$Build,
        [Parameter(Mandatory=$true)][ValidateSet('.nds','.elf','.map','.sym','.ds.gba')][string]$Extension
    )
    $publishedTargets = @('smash64ds', 'smash64ds-battle-playable-hwtri')
    $outputRoot = if ($publishedTargets -contains $Target) {
        [System.IO.Path]::GetFullPath($Root)
    } else {
        Resolve-Smash64DSBuildPath -Root $Root -Build $Build
    }
    return Join-Path $outputRoot "$Target$Extension"
}
