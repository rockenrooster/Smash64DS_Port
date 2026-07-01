param(
    [Parameter(Mandatory=$true)]
    [string]$Archive,
    [Parameter(Mandatory=$true)]
    [ValidateSet('Lean','CodeOnly')]
    [string]$Mode
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\snapshot-hygiene.ps1')
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
$sevenZip = Get-Smash64DSSevenZip
$entries = @(Get-Smash64DSArchiveEntries -Archive $archivePath -SevenZip $sevenZip)
$forbidden = @()
foreach ($entry in $entries) {
    $category = Get-Smash64DSSnapshotPathCategory -RelativePath $entry -Mode $Mode
    if (-not $category.Include) {
        $forbidden += [PSCustomObject]@{
            Path = $entry
            Category = $category.Category
            Reason = $category.Reason
        }
    }
}
if ($forbidden.Count -gt 0) {
    Write-Error ("Snapshot hygiene failed: {0} forbidden entrie(s) found in {1}" -f $forbidden.Count, $archivePath)
    $forbidden | Select-Object -First 50 | ForEach-Object {
        Write-Output ("  {0} [{1}] {2}" -f $_.Path, $_.Category, $_.Reason)
    }
    exit 1
}
$manifest = @($entries | Where-Object { $_ -eq 'SNAPSHOT_MANIFEST.txt' })
$archiveItem = Get-Item -LiteralPath $archivePath
Write-Output ("Snapshot hygiene passed: archive={0}, mode={1}, entries={2}, sizeMB={3}, manifest={4}" -f
    $archivePath,
    $Mode,
    $entries.Count, `
    (Format-Smash64DSMegabytes -Bytes $archiveItem.Length), `
    ($manifest.Count -gt 0))
