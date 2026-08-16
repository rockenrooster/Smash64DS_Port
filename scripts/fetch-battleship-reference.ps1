param(
    [switch]$Force,
    [switch]$VerifyOnly
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

# decomp/BattleShip-main is a build input, not only a reference: the Makefile
# compiles decomp/src/sys in place, puts decomp/src on the include path, and
# copies NitroFS payloads out of BattleShip_o2r/ and decomp/assets/us/relocData.
# It is gitignored (/decomp/) because it is third-party source plus ROM-derived
# data, so this script reconstructs it from upstream at the pinned commits the
# port was written against.
#
# The two trees are pinned independently and deliberately. The on-disk snapshot
# is a GitHub zip of BattleShip main plus a SEPARATE checkout of the upstream
# decompilation - it is NOT the decomp submodule BattleShip itself pins, and
# `git submodule update` would silently swap the behavioral reference.
$battleShipUrl = 'https://github.com/JRickey/BattleShip.git'
$battleShipPin = '62b513e9895ac5a4e833102e098afdfc05c9a48c'  # main, 2026-06-10
$decompUrl = 'https://github.com/VetriTheRetri/ssb-decomp-re.git'
$decompPin = 'e6f3eee68dbe19fbac87914b613ff4ea6f29e251'      # main, 2026-06-12

$destination = Join-Path $root 'decomp/BattleShip-main'
$decompDestination = Join-Path $destination 'decomp'

# `decomp/` is the immutable source-of-truth checkout.  The hashes below are
# the pinned upstream bytes for the files that historically received DS-local
# edits.  Keeping this small explicit list makes a regression loud without
# requiring ROM-derived assets to exist.  Build-time DS overlays are generated
# outside decomp/ by generate-battleship-import-overlay.ps1.
$sourceHashes = [ordered]@{
    'src/ft/ftanim.c'                  = 'ce1117762b59daa30005b877d76300f9ab7d605f31261d8bc1eef21f67e5a370'
    'src/ft/ftmain.c'                  = '16b94385756eba833da25f09c35a5566d9196c4026b482698f862f06422757b8'
    'src/mn/mncommon/mnstartup.c'      = '269dc61008aa367030a020d6226d2e9115625a9801087bd97abe2b74114d91c5'
    'src/mn/mncommon/mntitle.c'        = '858fdde72bd07af1a18cf0879ad20162de39d20f1b55c76ab2995a1ce9055133'
    'src/mv/mvopening/mvopeningroom.c' = 'efe584d094f97615dd3addcb3d7d9eb06a63541bc6c077a842c98ca93931832e'
    'src/sc/scmanager.c'               = 'c0bec9e72124b02090de7e76fe4fe1e2116599a80c97eaf769d217414acdd17b'
    'src/sys/objanim.c'                = 'eddedabd7aaffb4090e01fe0edcfac77f4262f42b91a3fe8faeddae2e3356dde'
    'src/sys/objhelper.c'              = '3db4a43da04541229579ba5c5ee87e03193223cab0545c9d12d89d53d9c5d36b'
    'src/sys/objman.c'                 = '65827df0a5de50300e9874cb1f008bebe009732315dc1c779f53f5f3ea07faf6'
    'src/sys/taskman.c'                = '1676abe622c02d91c1172622176461bd042489c1cd520576dab6e1836ddc44e1'
}
$sourceTreeHash = '1f77d1bc5705504fe7e4c36e1588ea540c59d5446d412c6bbd4f79d7c2892efa'

function Invoke-Git {
    param([string[]]$Arguments)
    $output = & git @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw ('git {0} failed: {1}' -f ($Arguments -join ' '), ($output -join "`n"))
    }
    return $output
}

function Assert-PristineSource {
    foreach ($relativePath in $sourceHashes.Keys) {
        $file = Join-Path $decompDestination $relativePath
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
            throw ('Missing pinned BattleShip source file: {0}' -f $relativePath)
        }
        $actual = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
        $expected = $sourceHashes[$relativePath]
        if ($actual -ne $expected) {
            throw ('BattleShip source-of-truth drift: {0} SHA256 {1} != pinned {2}. ' +
                'Do not edit decomp/; put DS behavior in src/import, src/port, or the build overlay.' -f
                $relativePath, $actual, $expected)
        }
    }

    # The named hashes above give a useful error for every file the DS port has
    # historically touched. This aggregate closes the rest of the rule: ANY
    # byte change, addition, deletion, or rename anywhere under decomp/src is a
    # source-of-truth violation even if it has no SSB64_TARGET_NDS marker.
    $sourceRoot = Join-Path $decompDestination 'src'
    $hasher = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256)
    try {
        $relativePaths = [System.Collections.Generic.List[string]]::new()
        Get-ChildItem -LiteralPath $sourceRoot -Recurse -File | ForEach-Object {
            $relativePaths.Add(
                [IO.Path]::GetRelativePath($sourceRoot, $_.FullName).Replace('\', '/'))
        }
        $relativePathsArray = $relativePaths.ToArray()
        [Array]::Sort($relativePathsArray, [StringComparer]::Ordinal)
        foreach ($relative in $relativePathsArray) {
            $entry = Join-Path $sourceRoot $relative.Replace('/', [IO.Path]::DirectorySeparatorChar)
            $hasher.AppendData([Text.Encoding]::UTF8.GetBytes($relative))
            $hasher.AppendData([byte[]](0))
            $hasher.AppendData([IO.File]::ReadAllBytes($entry))
            $hasher.AppendData([byte[]](0))
        }
        $actualTreeHash = [Convert]::ToHexString($hasher.GetHashAndReset()).ToLowerInvariant()
    } finally {
        $hasher.Dispose()
    }
    if ($actualTreeHash -ne $sourceTreeHash) {
        throw (('BattleShip decomp/src tree drift: SHA256 {0} != pinned {1}. ' +
            'The entire decomp/src tree is immutable; put DS behavior in src/import, src/port, ' +
            'or the build overlay.') -f $actualTreeHash, $sourceTreeHash)
    }
}

if ($VerifyOnly) {
    if (-not (Test-Path -LiteralPath $decompDestination -PathType Container)) {
        throw ('Missing {0}. Run this script without -VerifyOnly to fetch it.' -f $decompDestination)
    }
    Assert-PristineSource
    $missingRom = @(
        'assets/us/relocData'
    ) | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $decompDestination $_) -PathType Container)
    }
    $missingO2r = -not (Test-Path -LiteralPath (Join-Path $destination 'BattleShip_o2r') -PathType Container)
    if ($missingRom.Count -gt 0 -or $missingO2r) {
        Write-Output 'BattleShip source present; ROM-derived payloads absent (see the note below).'
    } else {
        Write-Output 'BattleShip reference verified: pinned pristine source and ROM-derived payloads all present.'
    }
    exit 0
}

if (Test-Path -LiteralPath $destination) {
    if (-not $Force) {
        throw ('{0} already exists. Re-run with -Force to replace it, or use -VerifyOnly to check the existing tree.' -f $destination)
    }
    Write-Output ('Removing existing {0} ...' -f $destination)
    Remove-Item -LiteralPath $destination -Recurse -Force
}

# core.autocrlf=false keeps the working tree byte-identical to the pinned blobs,
# which is what makes the source-of-truth hash check meaningful on Windows.
Write-Output ('Cloning BattleShip @ {0} ...' -f $battleShipPin.Substring(0, 9))
Invoke-Git @('-c', 'core.autocrlf=false', 'clone', '--no-checkout',
    '--no-recurse-submodules', $battleShipUrl, $destination) | Out-Null
Invoke-Git @('-C', $destination, '-c', 'core.autocrlf=false', 'checkout',
    '--detach', $battleShipPin) | Out-Null

Write-Output ('Cloning ssb-decomp-re @ {0} ...' -f $decompPin.Substring(0, 9))
if (Test-Path -LiteralPath $decompDestination) {
    Remove-Item -LiteralPath $decompDestination -Recurse -Force
}
Invoke-Git @('-c', 'core.autocrlf=false', 'clone', '--no-checkout',
    $decompUrl, $decompDestination) | Out-Null
Invoke-Git @('-C', $decompDestination, '-c', 'core.autocrlf=false', 'checkout',
    '--detach', $decompPin) | Out-Null
Assert-PristineSource

Write-Output ''
Write-Output 'BattleShip source reference restored byte-for-byte from the pinned upstream commits.'
Write-Output ''
Write-Output 'NOT restored, because it is ROM-derived and cannot be redistributed:'
Write-Output '  decomp/BattleShip-main/decomp/assets/     (make extract, needs your own baserom.us.z64)'
Write-Output '  decomp/BattleShip-main/BattleShip_o2r/    (BattleShip asset export)'
Write-Output 'The NitroFS payload rules in the Makefile read both paths, so a build'
Write-Output 'needs them. See decomp/BattleShip-main/decomp/README.md for extraction.'
exit 0
