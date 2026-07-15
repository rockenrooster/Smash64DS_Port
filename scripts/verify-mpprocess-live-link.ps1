[CmdletBinding()]
param(
    [ValidateRange(1, 64)][int]$Jobs = 16,
    [string]$MakeCommand = 'make',
    [switch]$NoBuild,
    [string]$BuildDir = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

function Fail-Verification {
    param([string]$Message)
    throw "mpprocess live-link verification failed: $Message"
}

function Assert-Verification {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { Fail-Verification $Message }
}

function Resolve-RequiredLeaf {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Fail-Verification "$Label is missing: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-IsolatedBuildPath {
    param([string]$Requested, [switch]$MustExist)
    $buildsRoot = [IO.Path]::GetFullPath((Join-Path $root 'builds'))
    $candidate = if ([string]::IsNullOrWhiteSpace($Requested)) {
        $runToken = ('{0}-{1}' -f $PID,
            ([guid]::NewGuid().ToString('N').Substring(0, 12)))
        Join-Path $buildsRoot "build-mpp-live-link-$runToken"
    } elseif ([IO.Path]::IsPathRooted($Requested)) {
        $Requested
    } elseif (($Requested -notmatch '[\\/]') -and
        ($Requested -match '^build')) {
        Join-Path $buildsRoot $Requested
    } else {
        Join-Path $root $Requested
    }
    $candidate = [IO.Path]::GetFullPath($candidate)
    if (Test-Path -LiteralPath $candidate -PathType Container) {
        $candidate = (Resolve-Path -LiteralPath $candidate).Path
    } elseif ($MustExist) {
        Fail-Verification "-NoBuild build directory is missing: $candidate"
    }
    $buildsPrefix = $buildsRoot.TrimEnd('\', '/') +
        [IO.Path]::DirectorySeparatorChar
    Assert-Verification ($candidate.StartsWith(
            $buildsPrefix,
            [StringComparison]::OrdinalIgnoreCase)) `
        "live-link build directory must stay under '$buildsRoot': $candidate"
    return $candidate
}

function ConvertTo-MsysPath {
    param([string]$Path)
    $normalized = [IO.Path]::GetFullPath($Path).Replace('\', '/')
    if ($normalized -match '^([A-Za-z]):/(.*)$') {
        return '/{0}/{1}' -f $Matches[1].ToLowerInvariant(), $Matches[2]
    }
    Assert-Verification ($normalized.StartsWith('/')) `
        "cannot convert output path to an absolute MSYS path: $Path"
    return $normalized
}

function Get-BuildInputFiles {
    $paths = [System.Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    $explicitFiles = @(
        (Join-Path $root 'Makefile'),
        (Join-Path $root 'scripts\check-mpprocess-live-link.ps1'),
        (Join-Path $root 'scripts\verify-mpprocess-live-link.ps1'),
        (Join-Path $root 'scripts\lib\harness-registry.ps1')
    )
    $inputTrees = @(
        (Join-Path $root 'src'),
        (Join-Path $root 'include'),
        (Join-Path $root 'assets'),
        (Join-Path $root 'decomp\BattleShip-main\decomp\src'),
        (Join-Path $root 'decomp\BattleShip-main\BattleShip_o2r'),
        (Join-Path $root `
            'decomp\BattleShip-main\decomp\assets\us\relocData')
    )
    foreach ($path in $explicitFiles) {
        [void]$paths.Add((Resolve-RequiredLeaf $path 'build input'))
    }
    foreach ($tree in $inputTrees) {
        Assert-Verification (Test-Path -LiteralPath $tree -PathType Container) `
            "build-input tree is missing: $tree"
        foreach ($item in @(Get-ChildItem -LiteralPath $tree `
            -Recurse -File -Force)) {
            [void]$paths.Add($item.FullName)
        }
    }
    return @($paths | Sort-Object)
}

function Get-BuildInputFingerprint {
    $pathsBefore = @(Get-BuildInputFiles)
    $records = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in $pathsBefore) {
        $before = Get-Item -LiteralPath $path
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        $after = Get-Item -LiteralPath $path
        Assert-Verification (($before.Length -eq $after.Length) -and
            ($before.LastWriteTimeUtc.Ticks -eq
             $after.LastWriteTimeUtc.Ticks)) `
            "build input changed while hashing: $path"
        $relative = [IO.Path]::GetRelativePath($root, $path).Replace('\', '/')
        [void]$records.Add("$relative`0$($before.Length)`0$hash")
    }
    $pathsAfter = @(Get-BuildInputFiles)
    Assert-Verification (($pathsBefore -join "`n") -ceq
        ($pathsAfter -join "`n")) `
        'build-input file set changed while hashing'
    $bytes = [Text.Encoding]::UTF8.GetBytes(($records -join "`n"))
    $hasher = [Security.Cryptography.SHA256]::Create()
    try {
        return (($hasher.ComputeHash($bytes) | ForEach-Object {
            '{0:x2}' -f $_
        }) -join '').ToUpperInvariant()
    } finally {
        $hasher.Dispose()
    }
}

function Assert-BuildInputsStable {
    param([string]$Expected, [string]$Phase)
    $actual = Get-BuildInputFingerprint
    Assert-Verification ($actual -ceq $Expected) `
        "build inputs changed $Phase`: $Expected != $actual"
}

function Get-ArtifactIdentity {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return 'missing' }
    Assert-Verification (Test-Path -LiteralPath $Path -PathType Leaf) `
        "published artifact path is not a file: $Path"
    $item = Get-Item -LiteralPath $Path
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    return '{0}|{1}|{2}' -f $item.Length,
        $item.LastWriteTimeUtc.Ticks, $hash
}

function Get-PublishedArtifactState {
    param([string]$Target)
    $state = [ordered]@{}
    foreach ($extension in @('elf', 'nds', 'ds.gba', 'lst')) {
        $path = Join-Path $root "$Target.$extension"
        $state[$path] = Get-ArtifactIdentity $path
    }
    return $state
}

function Assert-PublishedArtifactsUnchanged {
    param([object]$Expected, [string]$Phase)
    foreach ($path in $Expected.Keys) {
        $actual = Get-ArtifactIdentity $path
        Assert-Verification ($actual -ceq $Expected[$path]) `
            "published root artifact changed $Phase`: $path"
    }
}

function Assert-LiveConfig {
    param([string]$ConfigPath)
    $configText = Get-Content -LiteralPath $ConfigPath -Raw
    $expected = [ordered]@{
        NDS_BUILD_HARNESS_VARIANT = '"battle_playable_realtime"'
        NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE = '1'
        NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE = '0'
    }
    foreach ($name in $expected.Keys) {
        $matches = @([regex]::Matches(
            $configText,
            ('(?m)^[ \t]*#define[ \t]+' + [regex]::Escape($name) +
             '[ \t]+([^\s]+)[ \t]*\r?$')
        ))
        Assert-Verification (($matches.Count -eq 1) -and
            ($matches[0].Groups[1].Value -ceq $expected[$name])) `
            "build config does not record exact $name=$($expected[$name])"
    }
}

$records = @(Get-Smash64DSHarnessRegistry | Where-Object {
    $_.Name -ceq 'battle_playable_realtime'
})
Assert-Verification ($records.Count -eq 1) `
    'registry must contain exactly one battle_playable_realtime record'
$record = $records[0]
Assert-Verification ($record.Target -ceq
    'smash64ds-battle-playable-hwtri') `
    "battle_playable_realtime target is not the published P1 target: $($record.Target)"
Assert-Verification ($record.Harness -ceq 'battle_playable_realtime') `
    "battle_playable_realtime registry harness drifted: $($record.Harness)"

if ($NoBuild -and [string]::IsNullOrWhiteSpace($BuildDir)) {
    Fail-Verification '-NoBuild requires an explicit -BuildDir.'
}
$buildPath = Get-IsolatedBuildPath $BuildDir -MustExist:$NoBuild
$buildRelative = [IO.Path]::GetRelativePath($root, $buildPath).Replace('\', '/')
Assert-Verification (-not $buildRelative.StartsWith('../')) `
    "isolated build path escaped the repository: $buildPath"
$makeOutputPath = ConvertTo-MsysPath $buildPath
$checker = Resolve-RequiredLeaf `
    (Join-Path $PSScriptRoot 'check-mpprocess-live-link.ps1') `
    'mpprocess live-link checker'
$publishedBefore = Get-PublishedArtifactState $record.Target
$inputFingerprint = Get-BuildInputFingerprint
$mode = if ($NoBuild) { 'existing-isolated-build' } else { 'fresh-isolated-build' }

if (-not $NoBuild) {
    Write-Host ("mpprocess live-link build: live=1 private=0 target={0} build={1} output={2}" -f
        $record.Target, $buildRelative, $makeOutputPath)
    Push-Location $root
    try {
        & $MakeCommand @(
            "TARGET=$($record.Target)",
            "BUILD=$buildRelative",
            "NDS_OUTPUT_ROOT=$makeOutputPath",
            "NDS_DEV_SCENE_HARNESS=$($record.Harness)",
            'NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE=1',
            'NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE=0',
            '-B',
            "-j$Jobs"
        ) 2>&1 | ForEach-Object { Write-Host "$_" }
        $makeExit = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($makeExit -ne 0) {
        Fail-Verification "isolated live-link build failed with exit code $makeExit"
    }
    Assert-PublishedArtifactsUnchanged $publishedBefore 'during the build'
    Assert-BuildInputsStable $inputFingerprint 'during the build'
}

Assert-Verification (Test-Path -LiteralPath $buildPath -PathType Container) `
    "isolated live-link build directory is missing: $buildPath"
$configPath = Resolve-RequiredLeaf `
    (Join-Path $buildPath 'nds_build_config.h') 'live-link build config'
$mapPath = Resolve-RequiredLeaf (Join-Path $buildPath '.map') `
    'live-link map'
$elfPath = Resolve-RequiredLeaf `
    (Join-Path $buildPath "$($record.Target).elf") 'live-link ELF'
$romPath = Resolve-RequiredLeaf `
    (Join-Path $buildPath "$($record.Target).nds") 'isolated battle_playable ROM'
Assert-LiveConfig $configPath

Write-Host ("mpprocess live-link check: checker={0} build={1} config={2} elf={3}" -f
    $checker, $buildPath, $configPath, $elfPath)
$checkerOutput = @(& $checker -BuildDir $buildPath -Elf $elfPath |
    ForEach-Object { "$_" })
$checkerExit = $LASTEXITCODE
foreach ($line in $checkerOutput) { Write-Host $line }
if ($checkerExit -ne 0) {
    Fail-Verification "live-link checker failed with exit code $checkerExit"
}
$passLines = @($checkerOutput | Where-Object {
    $_ -cmatch '^mpprocess-live-link=PASS\b'
})
Assert-Verification ($passLines.Count -eq 1) `
    'live-link checker did not emit exactly one PASS record'
Assert-PublishedArtifactsUnchanged $publishedBefore 'while verification ran'
Assert-BuildInputsStable $inputFingerprint 'while verification ran'

Write-Output ((
    'mpprocess live-link verification passed: mode={0} live=1 private=0 ' +
    'target={1} build={2} config={3} map={4} elf={5} rom={6} ' +
    'checker={7} inputs={8}'
) -f $mode, $record.Target, $buildPath, $configPath, $mapPath,
    $elfPath, $romPath, $checker, $inputFingerprint)
exit 0
