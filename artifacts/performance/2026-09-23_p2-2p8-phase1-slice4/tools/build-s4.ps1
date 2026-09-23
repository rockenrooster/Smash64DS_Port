param(
    [string]$LogName = 'build.log',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string[]]$MakeArgs = @(),
    # A shell-configuration build elsewhere (the coordinator's 1P check, or a
    # lab build of this slice) rewrites the shared particle-bank outputs
    # (include/nds/generated/nds_particle_banks.generated.h and friends). The
    # next make of this dir can then compile the objects that include that
    # header before it regenerates it back (the hazard slices 2c and 3
    # documented). With -ForceParticleDeps every object whose dependency file
    # names the header is deleted first, so it is recompiled after the
    # regeneration and the ROM hash can be trusted.
    [switch]$ForceParticleDeps
)
# P2-2p8 Phase 1 slice 4 build wrapper: waits for and holds the shared build
# lock, builds ONE target into ONE build dir (no -j), logs to the artifact dir.
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice4'
New-Item -ItemType Directory -Force -Path $art | Out-Null
$log = Join-Path $art $LogName
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'phase1' -NoNewline
try {
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    Push-Location $root
    $start = Get-Date
    $all = @("TARGET=$Target", "BUILD=$Build") + $MakeArgs
    Set-Content -LiteralPath $log -Value ("make " + ($all -join ' '))
    if ($ForceParticleDeps) {
        $dir = Join-Path $root "builds\$Build"
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
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds)
} finally {
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
