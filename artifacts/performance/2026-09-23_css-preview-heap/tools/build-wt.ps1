param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    # One string, space- or comma-separated: `pwsh -File` does not bind arrays.
    [string]$MakeArgs = '',
    [string]$Worktree = 'D:\Stuff\DevFolder\Smash64DS_Port_worktrees\cssfix',
    [int]$Jobs = 8
)
# Build one target into one build dir of the CSS-fix worktree (a detached
# HEAD checkout, so the main tree's in-flight agent edits stay out of it).
$ErrorActionPreference = 'Continue'
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
$env:PATH = 'C:\Program Files\Git\cmd;' + $env:PATH
$log = Join-Path $Worktree "builds\$Build.log"
New-Item -ItemType Directory -Force -Path (Join-Path $Worktree 'builds') | Out-Null
Push-Location $Worktree
$start = Get-Date
$all = @("-j$Jobs", "TARGET=$Target", "BUILD=$Build") +
    @($MakeArgs -split '[ ,]+' | Where-Object { $_ -ne '' })
Set-Content -LiteralPath $log -Value ("make " + ($all -join ' '))
& make @all *>> $log
$code = $LASTEXITCODE
Pop-Location
Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds)
"MAKE_EXIT=$code elapsed=$([int]((Get-Date) - $start).TotalSeconds)s log=$log"
