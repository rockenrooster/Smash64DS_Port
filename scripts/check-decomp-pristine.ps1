param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$decompSource = Join-Path $root 'decomp/BattleShip-main/decomp/src'
$fetcher = Join-Path $PSScriptRoot 'fetch-battleship-reference.ps1'

# Keep one authoritative hash list: fetch-battleship-reference.ps1 pins the
# exact upstream bytes of every source file that has historically been modified
# by the DS port.
& pwsh -NoProfile -ExecutionPolicy Bypass -File $fetcher -VerifyOnly
if ($LASTEXITCODE -ne 0) {
    throw "Pinned BattleShip source verification failed with exit code $LASTEXITCODE."
}

# A new DS adaptation in the source-of-truth tree is categorically wrong even
# before its file joins the historical hash list.
$markers = @(Get-ChildItem -LiteralPath $decompSource -Recurse -File |
    Select-String -SimpleMatch 'SSB64_TARGET_NDS')
if ($markers.Count -ne 0) {
    $where = ($markers | Select-Object -First 8 | ForEach-Object {
        '{0}:{1}' -f $_.Path, $_.LineNumber
    }) -join ', '
    throw "decomp/ contains DS-local SSB64_TARGET_NDS edits: $where"
}

$forbiddenPatchDir = Join-Path $PSScriptRoot 'decomp-patches/battleship'
if ((Test-Path -LiteralPath $forbiddenPatchDir) -and
    @(Get-ChildItem -LiteralPath $forbiddenPatchDir -File -Filter '*.patch').Count -ne 0) {
    throw 'DS patches may not target decomp/. Move build-only adaptations to scripts/import-overlays/battleship.'
}

Write-Output 'DECOMP_PRISTINE=PASS pinned_historical_files=10 ds_markers=0 decomp_patch_pipeline=absent'
