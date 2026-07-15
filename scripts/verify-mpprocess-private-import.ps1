param(
    [ValidateRange(1,64)][int]$Jobs = 16,
    [string]$MakeCommand = 'make'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

function Fail-Verification {
    param([string]$Message)
    throw "mpprocess private A/B verification failed: $Message"
}

function Resolve-RequiredLeaf {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Fail-Verification "$Label is missing: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-BuildInputFiles {
    $paths = [System.Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    $explicitFiles = @(
        (Join-Path $root 'Makefile'),
        (Join-Path $root 'scripts\check-mpprocess-private-import.ps1'),
        (Join-Path $root 'scripts\verify-mpprocess-private-import.ps1'),
        (Join-Path $root 'scripts\lib\harness-registry.ps1')
    )
    $inputTrees = @(
        (Join-Path $root 'src'),
        (Join-Path $root 'include'),
        (Join-Path $root 'assets'),
        (Join-Path $root 'decomp\BattleShip-main\decomp\src'),
        (Join-Path $root 'decomp\BattleShip-main\BattleShip_o2r'),
        (Join-Path $root 'decomp\BattleShip-main\decomp\assets\us\relocData')
    )

    foreach ($path in $explicitFiles) {
        [void]$paths.Add((Resolve-RequiredLeaf $path 'build input'))
    }
    foreach ($tree in $inputTrees) {
        if (-not (Test-Path -LiteralPath $tree -PathType Container)) {
            Fail-Verification "build-input tree is missing: $tree"
        }
        foreach ($item in @(Get-ChildItem -LiteralPath $tree -Recurse -File -Force)) {
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
        if (($before.Length -ne $after.Length) -or
            ($before.LastWriteTimeUtc.Ticks -ne $after.LastWriteTimeUtc.Ticks)) {
            Fail-Verification "build input changed while hashing: $path"
        }
        $relative = [IO.Path]::GetRelativePath($root, $path).Replace('\', '/')
        [void]$records.Add("$relative`0$($before.Length)`0$hash")
    }
    $pathsAfter = @(Get-BuildInputFiles)
    if (($pathsBefore -join "`n") -cne ($pathsAfter -join "`n")) {
        Fail-Verification 'build-input file set changed while hashing'
    }

    $bytes = [Text.Encoding]::UTF8.GetBytes(($records -join "`n"))
    $hasher = [Security.Cryptography.SHA256]::Create()
    try {
        return (($hasher.ComputeHash($bytes) |
            ForEach-Object { '{0:x2}' -f $_ }) -join '').ToUpperInvariant()
    } finally {
        $hasher.Dispose()
    }
}

function Assert-BuildInputsStable {
    param([string]$Expected, [string]$Phase)
    $actual = Get-BuildInputFingerprint
    if ($actual -cne $Expected) {
        Fail-Verification "build inputs changed $Phase`: $Expected != $actual"
    }
}

function Invoke-PrivateBuild {
    param(
        [int]$Gate,
        [string]$BuildName,
        [object]$HarnessRecord
    )
    $buildPath = Join-Path $root (Join-Path 'builds' $BuildName)
    # GNU make forwards this value through a POSIX shell.  A drive-qualified
    # target is normalized back to Windows separators by make, then those
    # backslashes are consumed as escapes at the final link step.  Use the
    # MSYS /drive/path spelling that the project Makefile uses for PROJECT_ROOT.
    $makeBuildPath = $buildPath.Replace('\', '/')
    if ($makeBuildPath -match '^([A-Za-z]):/(.*)$') {
        $makeBuildPath = '/{0}/{1}' -f $Matches[1].ToLowerInvariant(), $Matches[2]
    }
    Write-Host ("mpprocess private A/B build: live=0 private={0} build={1}" -f
        $Gate, $BuildName)
    Push-Location $root
    try {
        & $MakeCommand @(
            "TARGET=$($HarnessRecord.Target)",
            "BUILD=$BuildName",
            "NDS_OUTPUT_ROOT=$makeBuildPath",
            "NDS_DEV_SCENE_HARNESS=$($HarnessRecord.Harness)",
            'NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE=0',
            "NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE=$Gate",
            '-B',
            "-j$Jobs"
        ) 2>&1 | ForEach-Object { Write-Host "$_" }
        $makeExit = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    if ($makeExit -ne 0) {
        Fail-Verification "gate-$Gate build failed with exit code $makeExit"
    }

    $configPath = Resolve-RequiredLeaf (Join-Path $buildPath 'nds_build_config.h') `
        "gate-$Gate build config"
    $configText = Get-Content -LiteralPath $configPath -Raw
    $expectedGates = [ordered]@{
        NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE = '0'
        NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE = "$Gate"
    }
    foreach ($name in $expectedGates.Keys) {
        $gateMatches = @([regex]::Matches($configText,
            ('(?m)^[ \t]*#define[ \t]+' + [regex]::Escape($name) +
             '[ \t]+([^\s]+)[ \t]*\r?$')))
        if (($gateMatches.Count -ne 1) -or
            ($gateMatches[0].Groups[1].Value -cne $expectedGates[$name])) {
            Fail-Verification ("gate-{0} build config does not record {1}={2}" -f
                $Gate, $name, $expectedGates[$name])
        }
    }

    return [pscustomobject]@{
        BuildPath = (Resolve-Path -LiteralPath $buildPath).Path
        Elf = Resolve-RequiredLeaf (Join-Path $buildPath "$($HarnessRecord.Target).elf") `
            "gate-$Gate ELF"
        Rom = Resolve-RequiredLeaf (Join-Path $buildPath "$($HarnessRecord.Target).nds") `
            "gate-$Gate ROM"
    }
}

function Copy-PrivateArtifacts {
    param(
        [object]$BuildResult,
        [string]$StageName,
        [string]$Target,
        [string]$RunStageRoot
    )
    $stagePath = Join-Path $RunStageRoot $StageName
    [void](New-Item -ItemType Directory -Path $stagePath -Force)
    $stagedElf = Join-Path $stagePath "$Target.elf"
    $stagedRom = Join-Path $stagePath "$Target.nds"
    Copy-Item -LiteralPath $BuildResult.Elf -Destination $stagedElf -Force
    Copy-Item -LiteralPath $BuildResult.Rom -Destination $stagedRom -Force
    return [pscustomobject]@{
        Elf = Resolve-RequiredLeaf $stagedElf "$StageName staged ELF"
        Rom = Resolve-RequiredLeaf $stagedRom "$StageName staged ROM"
    }
}

$records = @(Get-Smash64DSHarnessRegistry |
    Where-Object { $_.Name -ceq 'battle_playable_realtime' })
if ($records.Count -ne 1) {
    Fail-Verification "registry must contain exactly one battle_playable_realtime record"
}
$record = $records[0]
if ([string]::IsNullOrWhiteSpace($record.Target) -or
    [string]::IsNullOrWhiteSpace($record.Harness)) {
    Fail-Verification 'battle_playable_realtime registry record lacks target or harness'
}

$runToken = ('{0}-{1}' -f $PID,
    ([guid]::NewGuid().ToString('N').Substring(0, 12)))
$offBuildName = "build-mpp-ab-$runToken-off"
$onBuildName = "build-mpp-ab-$runToken-on"
$runStageRoot = Join-Path $root `
    (Join-Path 'artifacts\verifier-temp\mpprocess-private' $runToken)
$inputFingerprint = Get-BuildInputFingerprint
Write-Host "mpprocess private A/B inputs: sha256=$inputFingerprint run=$runToken"

$offBuild = Invoke-PrivateBuild 0 $offBuildName $record
$offStage = Copy-PrivateArtifacts $offBuild 'off' $record.Target $runStageRoot
Assert-BuildInputsStable $inputFingerprint 'during the OFF build'

# ON is deliberately last for deterministic comparison. Both A/B outputs stay
# private to their build directories and never overwrite the published ROM.
$onBuild = Invoke-PrivateBuild 1 $onBuildName $record
$onStage = Copy-PrivateArtifacts $onBuild 'on' $record.Target $runStageRoot
Assert-BuildInputsStable $inputFingerprint 'during the ON build'

$checker = Resolve-RequiredLeaf (Join-Path $PSScriptRoot 'check-mpprocess-private-import.ps1') `
    'mpprocess private checker'
$shippedRom = Resolve-RequiredLeaf (Join-Path $root 'smash64ds-battle-playable-hwtri.nds') `
    'shipped battle-playable ROM'
& $checker `
    -BuildDir $onBuild.BuildPath `
    -Elf $onStage.Elf `
    -CanonicalRom $onStage.Rom `
    -ShippedRom $shippedRom `
    -LiveElf $onBuild.Elf `
    -LiveCanonicalRom $onBuild.Rom `
    -BaselineBuildDir $offBuild.BuildPath `
    -BaselineElf $offStage.Elf `
    -BaselineRom $offStage.Rom `
    -RequireAB
if ($LASTEXITCODE -ne 0) {
    Fail-Verification "checker failed with exit code $LASTEXITCODE"
}
Assert-BuildInputsStable $inputFingerprint 'while the checker ran'

Write-Output ("mpprocess private deterministic A/B passed: live=0 private-off=0 private-on=1 target={0} harness={1} run={2} inputs={3} off={4} on={5}" -f
    $record.Target, $record.Harness, $runToken, $inputFingerprint,
    $offBuildName, $onBuildName)
exit 0
