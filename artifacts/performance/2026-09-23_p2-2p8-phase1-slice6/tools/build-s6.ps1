param(
    [string]$LogName = 'build.log',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$Build = 'build-p2p8-s6',
    [string[]]$MakeArgs = @(),
    # Seed a missing build dir from this one (objects + generated outputs), so
    # only what changed recompiles.
    [string]$SeedFrom = '',
    # A shell-configuration build elsewhere rewrites the shared particle-bank
    # outputs (include/nds/generated/nds_particle_banks.generated.h). The next
    # make of a tick-HUD dir can then compile the objects that include that
    # header before it regenerates it back (the hazard slices 2c-4 documented).
    # With -ForceParticleDeps every object whose dependency file names the
    # header is deleted first, so it recompiles after the regeneration.
    [switch]$ForceParticleDeps
)
# P2-2p8 Phase 1 slice 6 build wrapper (from slice 5's build-s5.ps1): waits for
# and holds the shared build lock, builds ONE target into ONE build dir,
# logs to the artifact dir.
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice6'
New-Item -ItemType Directory -Force -Path $art | Out-Null
$log = Join-Path $art $LogName
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'phase1-slice6' -NoNewline
try {
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    Push-Location $root
    $start = Get-Date
    $dir = Join-Path $root "builds\$Build"
    if ((-not (Test-Path -LiteralPath $dir)) -and ($SeedFrom -ne '')) {
        Copy-Item -LiteralPath (Join-Path $root "builds\$SeedFrom") -Destination $dir -Recurse
    }
    $all = @("TARGET=$Target", "BUILD=$Build") + $MakeArgs
    Set-Content -LiteralPath $log -Value ("make " + ($all -join ' '))
    if ($ForceParticleDeps) {
        $forced = @()
        Get-ChildItem -LiteralPath $dir -Filter '*.d' -ErrorAction SilentlyContinue | ForEach-Object {
            if (Select-String -LiteralPath $_.FullName -SimpleMatch 'nds_particle_banks.generated.h' -Quiet) {
                $obj = [System.IO.Path]::ChangeExtension($_.FullName, '.o')
                if (Test-Path -LiteralPath $obj) {
                    Remove-Item -LiteralPath $obj -Force
                    $forced += (Split-Path -Leaf $obj)
                }
            }
        }
        Add-Content -LiteralPath $log -Value ("forced particle-bank dependents: " + ($forced -join ' '))
    }
    & make @all *>> $log
    $code = $LASTEXITCODE
    Pop-Location
    $nds = Join-Path $dir "$Target.nds"
    $sha = if (Test-Path -LiteralPath $nds) { (Get-FileHash -LiteralPath $nds -Algorithm SHA256).Hash } else { 'none' }
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds + " sha256=$sha")
} finally {
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
