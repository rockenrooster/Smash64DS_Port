param([Parameter(Mandatory)][string]$Build, [string]$Seed = '', [string[]]$Extra = @())
# The owner's play ROM (builds/<Build>/smash64ds.nds): free play
# (TARGET=smash64ds) with every content and every P5 map, written into its
# own build directory (NDS_OUTPUT_ROOT) and never to the root ROM. Names so
# far: play-pNN, play-p7x..z.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
Set-Location $S64Repo
Use-S64DevkitPro
$rootStamp = (Get-Item smash64ds.nds -ErrorAction SilentlyContinue).LastWriteTimeUtc
Copy-S64BuildSeed -Build $Build -Seed $Seed -DropRoms @('smash64ds.nds')
$out = (Join-Path $S64Repo "builds\$Build") -replace '\\', '/'
Invoke-S64Serialized {
    make TARGET=smash64ds "BUILD=builds/$Build" "NDS_OUTPUT_ROOT=$out" @S64PlayFlags @Extra *> "builds\$Build.log"
    "make exit $LASTEXITCODE"
}
$after = (Get-Item smash64ds.nds -ErrorAction SilentlyContinue).LastWriteTimeUtc
if ($after -ne $rootStamp) { 'WARNING: root smash64ds.nds changed' }
$rom = Join-Path $S64Repo "builds\$Build\smash64ds.nds"
if (-not (Test-Path $rom)) { 'no ROM'; Get-Content "builds\$Build.log" -Tail 30; exit 1 }
"$Build " + (Get-Item $rom).Length
$cfg = Join-Path $S64Repo "builds\$Build\nds_build_config.h"
'P4 contents on: ' + (Select-String -Path $cfg -Pattern '#define NDS_P4_[A-Z]+ 1$').Count
(Select-String -Path $cfg -Pattern '#define NDS_P5 ' | Select-Object -First 1).Line
