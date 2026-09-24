param(
    [Parameter(Mandatory = $true)][string]$LogName,
    [Parameter(Mandatory = $true)][string]$Build,
    # One source edit: the file (relative to the repo root), the exact text to
    # find (must occur once) and its replacement.
    [Parameter(Mandatory = $true)][string]$File,
    [Parameter(Mandatory = $true)][string]$Find,
    [Parameter(Mandatory = $true)][string]$Replace,
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$SeedFrom = '',
    [string[]]$MakeArgs = @()
)
# P2-2p8 Phase 1 slice 5 A/B variant build: holds the shared build lock for
# the whole swap, applies ONE source edit, builds into its own dir, then puts
# the file back and proves it byte-identical (sha256) before releasing the lock
# -- no other build can see the edited tree.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice5'
$log = Join-Path $art $LogName
$path = Join-Path $root $File
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'phase1-slice5-variant' -NoNewline
$orig = [System.IO.File]::ReadAllBytes($path)
$origHash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
try {
    $text = [System.Text.Encoding]::UTF8.GetString($orig)
    $count = ([regex]::Matches($text, [regex]::Escape($Find))).Count
    if ($count -ne 1) { throw "find text occurs $count times in $File" }
    $edited = $text.Replace($Find, $Replace)
    [System.IO.File]::WriteAllBytes($path, [System.Text.Encoding]::UTF8.GetBytes($edited))
    (Get-Item -LiteralPath $path).LastWriteTime = Get-Date
    $dir = Join-Path $root "builds\$Build"
    if ((-not (Test-Path -LiteralPath $dir)) -and ($SeedFrom -ne '')) {
        Copy-Item -LiteralPath (Join-Path $root "builds\$SeedFrom") -Destination $dir -Recurse
    }
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    Push-Location $root
    $start = Get-Date
    $all = @("TARGET=$Target", "BUILD=$Build") + $MakeArgs
    Set-Content -LiteralPath $log -Value ("make " + ($all -join ' ') + "   (variant: $File '$Find' -> '$Replace')")
    $ErrorActionPreference = 'Continue'
    & make @all *>> $log
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    Pop-Location
    $nds = Join-Path $dir "$Target.nds"
    $sha = if (Test-Path -LiteralPath $nds) { (Get-FileHash -LiteralPath $nds -Algorithm SHA256).Hash } else { 'none' }
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds + " sha256=$sha")
} finally {
    [System.IO.File]::WriteAllBytes($path, $orig)
    (Get-Item -LiteralPath $path).LastWriteTime = Get-Date
    $after = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    Add-Content -LiteralPath $log -Value ("RESTORED $File; identical: " + ($after -eq $origHash))
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
