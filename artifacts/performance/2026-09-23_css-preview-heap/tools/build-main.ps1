param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    # One string, space- or comma-separated: `pwsh -File` does not bind arrays.
    [string]$MakeArgs = '',
    [int]$Jobs = 8
)
# Build one target into one build dir of the MAIN tree under the shared
# P2-2p8 build lock (builds/.p2p8-build.lock), logging next to the build dir.
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$log = Join-Path $root "builds\$Build.log"
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'coordinator' -NoNewline
try {
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    $env:PATH = 'C:\Program Files\Git\cmd;' + $env:PATH
    Push-Location $root
    $start = Get-Date
    $all = @("-j$Jobs", "TARGET=$Target", "BUILD=$Build") +
        @($MakeArgs -split '[ ,]+' | Where-Object { $_ -ne '' })
    Set-Content -LiteralPath $log -Value ("make " + ($all -join ' '))
    & make @all *>> $log
    $code = $LASTEXITCODE
    Pop-Location
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds)
    "MAKE_EXIT=$code elapsed=$([int]((Get-Date) - $start).TotalSeconds)s log=$log"
} finally {
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
