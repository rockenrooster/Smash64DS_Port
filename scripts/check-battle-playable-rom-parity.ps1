param(
    [string]$CanonicalRom = (Join-Path $PSScriptRoot '..\smash64ds-battle-playable-canonical-hwtri.nds'),
    [string]$ShippedRom = (Join-Path $PSScriptRoot '..\smash64ds-battle-playable-hwtri.nds')
)
$ErrorActionPreference = 'Stop'

foreach ($rom in @($CanonicalRom, $ShippedRom)) {
    if (-not (Test-Path -LiteralPath $rom -PathType Leaf)) {
        throw "Battle-playable ROM parity input is missing: $rom"
    }
}

$canonicalFile = Get-Item -LiteralPath $CanonicalRom
$shippedFile = Get-Item -LiteralPath $ShippedRom
if ($canonicalFile.Length -ne $shippedFile.Length) {
    throw ("Battle-playable ROM length mismatch: canonical={0} shipped={1}." -f
        $canonicalFile.Length, $shippedFile.Length)
}

$canonicalHash = (Get-FileHash -LiteralPath $canonicalFile.FullName -Algorithm SHA256).Hash
$shippedHash = (Get-FileHash -LiteralPath $shippedFile.FullName -Algorithm SHA256).Hash
if ($canonicalHash -ne $shippedHash) {
    throw ("Battle-playable ROM SHA256 mismatch: canonical={0} shipped={1}." -f
        $canonicalHash, $shippedHash)
}

Write-Output ("Battle-playable ROM parity passed: bytes={0} sha256={1}" -f
    $canonicalFile.Length, $canonicalHash)
exit 0
