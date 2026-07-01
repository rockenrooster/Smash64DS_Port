$ErrorActionPreference = 'Stop'

$script:Smash64DSSnapshotRootGeneratedPatterns = @(
    'smash64ds*.elf',
    'smash64ds*.nds',
    'smash64ds*.ds.gba',
    'smash64ds*.map',
    'smash64ds*.sym',
    '_*.gdb',
    '_*.gdb.out',
    '_*.gdb.err',
    'melonds.*.log'
)

$script:Smash64DSSnapshotRootGeneratedExact = @(
    'melonDS.toml',
    'rtc.bin'
)

function Get-Smash64DSSevenZip {
    foreach ($candidate in @(
        "$env:ProgramFiles\7-Zip\7z.exe",
        "${env:ProgramFiles(x86)}\7-Zip\7z.exe"
    )) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return $candidate
        }
    }

    $cmd = Get-Command 7z -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    throw '7-Zip not found. Install it or add 7z.exe to PATH.'
}

function ConvertTo-Smash64DSRelativePath {
    param(
        [Parameter(Mandatory=$true)][string]$BasePath,
        [Parameter(Mandatory=$true)][string]$FullPath
    )

    $base = [System.IO.Path]::GetFullPath($BasePath).TrimEnd('\', '/')
    $full = [System.IO.Path]::GetFullPath($FullPath)
    $baseUri = [System.Uri]::new($base + [System.IO.Path]::DirectorySeparatorChar)
    $fullUri = [System.Uri]::new($full)
    $relative = [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($fullUri).ToString())
    return ($relative -replace '/', [System.IO.Path]::DirectorySeparatorChar)
}

function ConvertTo-Smash64DSSnapshotPath {
    param([Parameter(Mandatory=$true)][string]$Path)

    return (($Path -replace '\\', '/') -replace '^/+', '')
}

function Test-Smash64DSRootPathLike {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][string]$Pattern
    )

    $normalized = ConvertTo-Smash64DSSnapshotPath -Path $Path
    return (($normalized -notmatch '/') -and ($normalized -like $Pattern))
}

function Get-Smash64DSSnapshotPathCategory {
    param(
        [Parameter(Mandatory=$true)][string]$RelativePath,
        [Parameter(Mandatory=$true)][ValidateSet('Lean','CodeOnly','Full')][string]$Mode,
        [switch]$IncludeArtifacts,
        [switch]$IncludeDecompGenerated
    )

    $path = ConvertTo-Smash64DSSnapshotPath -Path $RelativePath
    $leaf = Split-Path -Leaf $path

    if ($path -eq 'SNAPSHOT_MANIFEST.txt') {
        return [PSCustomObject]@{ Include = $true; Category = 'included'; Reason = 'snapshot manifest' }
    }

    if ($path -match '(^|/)\.git($|/)') {
        return [PSCustomObject]@{ Include = $false; Category = 'git'; Reason = '.git repository metadata' }
    }

    if (($path -match '^build($|/)') -or ($path -match '^build-[^/]*($|/)')) {
        return [PSCustomObject]@{ Include = $false; Category = 'build_directory'; Reason = 'generated build directory' }
    }

    if ($Mode -eq 'Full') {
        return [PSCustomObject]@{ Include = $true; Category = 'included'; Reason = 'Full mode includes broad local payloads' }
    }

    foreach ($pattern in $script:Smash64DSSnapshotRootGeneratedPatterns) {
        if (Test-Smash64DSRootPathLike -Path $path -Pattern $pattern) {
            return [PSCustomObject]@{ Include = $false; Category = 'root_generated'; Reason = "root generated output $pattern" }
        }
    }

    if (($path -notmatch '/') -and ($script:Smash64DSSnapshotRootGeneratedExact -contains $leaf)) {
        return [PSCustomObject]@{ Include = $false; Category = 'root_generated'; Reason = "root generated/local file $leaf" }
    }

    if ((-not $IncludeArtifacts) -and ($path -match '^artifacts($|/)')) {
        return [PSCustomObject]@{ Include = $false; Category = 'artifacts'; Reason = 'artifacts are excluded by default' }
    }

    if ($path -match '^emulators($|/)') {
        if (($leaf -eq 'README.md') -or ($leaf -eq '.gitkeep')) {
            return [PSCustomObject]@{ Include = $true; Category = 'included'; Reason = 'lightweight emulator placeholder' }
        }

        return [PSCustomObject]@{ Include = $false; Category = 'emulator_payload'; Reason = 'local emulator payload/log/config' }
    }

    if (($Mode -eq 'CodeOnly') -and ($path -match '^decomp($|/)')) {
        return [PSCustomObject]@{ Include = $false; Category = 'decomp_reference'; Reason = 'CodeOnly excludes read-only decomp reference tree' }
    }

    if (($Mode -eq 'Lean') -and (-not $IncludeDecompGenerated) -and
        ($path -match '^decomp/'))
    {
        if ($path -match '^decomp/BattleShip-main/decomp/BattleShip_o2r($|/)') {
            return [PSCustomObject]@{ Include = $false; Category = 'decomp_duplicate_o2r'; Reason = 'duplicate nested BattleShip O2R tree; top-level BattleShip_o2r is build-critical' }
        }

        if ($path -match '^decomp/(?:.*/)?build/') {
            return [PSCustomObject]@{ Include = $false; Category = 'decomp_generated_build'; Reason = 'upstream decomp generated build output' }
        }

        if (($path -match '^decomp/(?:.*/)?target/') -or
            ($path -match '^decomp/(?:.*/)?\.gradle/') -or
            ($path -match '^decomp/(?:.*/)?cmake-build[^/]*/'))
        {
            return [PSCustomObject]@{ Include = $false; Category = 'decomp_tool_cache'; Reason = 'upstream decomp tool/build cache output' }
        }

        if (($leaf -like 'baserom*') -or
            ($leaf -like '*.z64') -or
            ($leaf -like '*.n64') -or
            ($leaf -like '*.nds') -or
            ($leaf -like '*.elf') -or
            ($leaf -like '*.ds.gba') -or
            ($leaf -like '*.o') -or
            ($leaf -like '*.a'))
        {
            return [PSCustomObject]@{ Include = $false; Category = 'decomp_binary_payload'; Reason = 'upstream decomp binary/generated payload' }
        }
    }

    return [PSCustomObject]@{ Include = $true; Category = 'included'; Reason = 'included by snapshot mode' }
}

function Get-Smash64DSArchiveEntries {
    param(
        [Parameter(Mandatory=$true)][string]$Archive,
        [string]$SevenZip = (Get-Smash64DSSevenZip)
    )

    if (-not (Test-Path -LiteralPath $Archive)) {
        throw "Archive not found: $Archive"
    }

    $archiveFull = [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Archive).Path)
    $entries = [System.Collections.Generic.List[string]]::new()
    $output = & $SevenZip l -slt $archiveFull
    if ($LASTEXITCODE -ne 0) {
        throw "7z list failed with exit code $LASTEXITCODE"
    }

    foreach ($line in $output) {
        if ($line -like 'Path = *') {
            $path = $line.Substring(7)
            if (-not $path) { continue }
            if ([System.String]::Equals($path, $archiveFull, [System.StringComparison]::OrdinalIgnoreCase)) { continue }
            $entries.Add((ConvertTo-Smash64DSSnapshotPath -Path $path)) | Out-Null
        }
    }

    return @($entries)
}

function Format-Smash64DSMegabytes {
    param([int64]$Bytes)

    return [math]::Round($Bytes / 1MB, 2)
}
