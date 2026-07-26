param(
    [switch]$RequireBoth
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$allowed = @(
    'smash64ds-battle-playable-hwtri.nds',
    'smash64ds.nds'
)
$roms = @(Get-ChildItem -LiteralPath $root -File -Filter 'smash64ds*.nds')
$unexpected = @($roms | Where-Object { $allowed -notcontains $_.Name })
if ($unexpected.Count -gt 0) {
    throw ('Unexpected published ROM output(s): {0}. Keep only {1}.' -f
        (($unexpected.Name | Sort-Object) -join ', '), ($allowed -join ', '))
}
if ($RequireBoth) {
    $missing = @($allowed | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $root $_) -PathType Leaf)
    })
    if ($missing.Count -gt 0) {
        throw ('Required published ROM output(s) missing: {0}.' -f
            ($missing -join ', '))
    }
}
$present = @($roms.Name | Sort-Object)

$battleRomPath = Join-Path $root 'smash64ds-battle-playable-hwtri.nds'
if (Test-Path -LiteralPath $battleRomPath -PathType Leaf) {
    $task62Signature = [Text.Encoding]::GetEncoding(28591).GetString(
        [byte[]](1, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 1,
                 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2))
    $battleRomText = [Text.Encoding]::GetEncoding(28591).GetString(
        [IO.File]::ReadAllBytes($battleRomPath))
    if ($battleRomText.IndexOf(
            $task62Signature, [StringComparison]::Ordinal) -ge 0) {
        throw 'Published battle ROM contains the rejected Task 62 Dream Land DS-mesh payload. Rebuild it with NDS_DREAMLAND_DS_MESH=0.'
    }
}

Write-Output ('Published ROM contract passed: {0}' -f
    $(if ($present.Count -gt 0) { $present -join ', ' } else { '(none yet)' }))
exit 0
