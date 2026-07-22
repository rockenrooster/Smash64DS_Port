# Legacy broad snapshot exporter retained for fallback/debug reference.
# Normal handoff snapshots should use scripts/New-Smash64DSSnapshot.ps1.
# Create a fastest-compression 7z snapshot of the Smash64DS_Port dev folder.
# Output: <user profile>\Desktop\Snapshots\Smash64DS_Port_<yyyyMMdd_HHmmss>.7z
$ErrorActionPreference = 'Stop'
$source  = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$destDir = Join-Path $HOME 'Desktop\Snapshots'
if (-not (Test-Path -LiteralPath $source)) {
    throw "Source folder not found: $source"
}
# Locate 7z.exe
$sevenZip = $null
foreach ($candidate in @(
    "$env:ProgramFiles\7-Zip\7z.exe",
    "${env:ProgramFiles(x86)}\7-Zip\7z.exe"
)) {
    if (Test-Path -LiteralPath $candidate) { $sevenZip = $candidate; break }
}
if (-not $sevenZip) {
    $cmd = Get-Command 7z -ErrorAction SilentlyContinue
    if ($cmd) { $sevenZip = $cmd.Source }
}
if (-not $sevenZip) {
    throw '7-Zip not found. Install it or add 7z.exe to your PATH.'
}
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$archive   = Join-Path $destDir "Smash64DS_Port_$timestamp.zip"
Write-Host "Compressing '$source' -> '$archive'" -ForegroundColor Cyan
# -mx1 = Fastest compression (use -mx0 for Store/no-compression, even faster)
# -xr!<name> = recursively exclude any folder/file matching <name>
#   .git          -> hidden git repo
#   build         -> top-level build output
#   build-*       -> anything starting with "build-" (build-debug, build-release, ...)
# (quoted so PowerShell passes each as a single token — unquoted, the '!' mangles it)
& $sevenZip a -tzip -mx1 '-xr!.git' '-xr!build' '-xr!build-*' $archive $source
if ($LASTEXITCODE -eq 0) {
    $sizeMb = [math]::Round((Get-Item -LiteralPath $archive).Length / 1MB, 2)
    Write-Host "Done: $archive ($sizeMb MB)" -ForegroundColor Green
} else {
    throw "7z exited with code $LASTEXITCODE"
}
