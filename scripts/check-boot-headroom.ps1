[CmdletBinding()]
param(
    [string]$Build = 'build-tick-hud-buckets',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [string]$Nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe',
    [string]$Size = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-size.exe'
)

# Where a build sits against the ARM9 boot cliff, in ABSOLUTE addresses.
#
# The ROM runs out of main RAM before it reaches presented frame 1. The failure
# is silent by construction -- charter SS3.11: syMallocSet spins forever on
# exhaustion, so the signature is a total freeze, not an error return. A GDB
# harness reports that as a timeout, which reads exactly like a hung emulator or
# a bad invocation, and has now cost four builds across cycles 81 and 82.
#
# WHY ABSOLUTE ADDRESSES, AND NOT THE "+1,408 boots / +2,208 does not" BAND that
# the board carried for a week: that band was a DELTA over a datum build, and the
# datum moved every time the tree grew. Quoted against a later tree it reads as
# ~1.4 KB of headroom when the true remaining headroom is 96 bytes, because the
# tree had already spent 1,312 of the 1,408. Cycle 82 measured that directly.
# `fake_heap_start` is the end of .bss and therefore the heap base, so it is a
# faithful total-static-footprint meter (address delta == text delta + data delta
# + bss delta, verified to the byte across cycle-80/81/82 builds). Comparing
# absolute heap bases is valid across trees because what boots or does not is the
# heap SIZE that remains, and the top of the heap does not move.
#
# Every entry below was read with `nm` from an ELF still on disk and paired with
# a recorded boot outcome. Add a row only with both halves.
$ladder = @(
    @{ addr = 0x02294224; boots = $true;  build = 'build-c75-tickhud-publish'; note = 'published tick-HUD sibling' }
    @{ addr = 0x02294284; boots = $true;  build = 'build-r209-memo0';          note = 'frames 61-68' }
    @{ addr = 0x022947a4; boots = $true;  build = 'build-c80-gate-bothcpu';    note = 'whole-match 1600-sample gate arm; frames 60-67 cycle 82' }
    @{ addr = 0x02294804; boots = $true;  build = 'build-r209-memo1s';         note = 'frames 62-69 -- HIGHEST PROVEN BOOTING' }
    @{ addr = 0x02294b24; boots = $false; build = 'build-r209-memo2s';         note = 'LOWEST PROVEN FAILING' }
    @{ addr = 0x02294ba4; boots = $false; build = 'build-c82-src-nobracket';   note = 'SRC ring rows, no brackets -- 240 s, never reached ring stop 0' }
    @{ addr = 0x02294c04; boots = $false; build = 'build-c81-src-bothcpu';     note = 'SRC instrument with brackets' }
    @{ addr = 0x022950c4; boots = $false; build = 'build-r209-memo1';          note = 'Tex memo, 64 entries' }
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if (-not (Test-Path $elf)) {
    throw "No ELF at $elf -- build $Target into $Build first."
}
foreach ($tool in @($Nm, $Size)) {
    if (-not (Test-Path $tool)) { throw "Missing devkitARM tool: $tool" }
}

$line = (& $Nm $elf | Select-String -Pattern '\bfake_heap_start$').Line
if (-not $line) { throw "fake_heap_start not found in $elf" }
$heap = [Convert]::ToInt64((($line -split '\s+')[0]), 16)

$sz = (& $Size $elf | Select-Object -Last 1).Trim() -split '\s+'
$text = [int]$sz[0]; $data = [int]$sz[1]; $bss = [int]$sz[2]

$highestBoot = ($ladder | Where-Object { $_.boots } | Sort-Object { $_.addr } | Select-Object -Last 1)
$lowestFail  = ($ladder | Where-Object { -not $_.boots } | Sort-Object { $_.addr } | Select-Object -First 1)

$headroom = $highestBoot.addr - $heap
$band = $lowestFail.addr - $highestBoot.addr

Write-Host ("build            : {0}" -f $Build)
Write-Host ("fake_heap_start  : 0x{0:x8}" -f $heap)
Write-Host ("static footprint : text {0:N0}  data {1:N0}  bss {2:N0}  total {3:N0}" -f $text, $data, $bss, ($text + $data + $bss))
Write-Host ("highest booting  : 0x{0:x8}  ({1})" -f $highestBoot.addr, $highestBoot.build)
Write-Host ("lowest failing   : 0x{0:x8}  ({1})" -f $lowestFail.addr, $lowestFail.build)
Write-Host ("unproven band    : {0:N0} bytes wide" -f $band)

if ($heap -ge $lowestFail.addr) {
    $over = $heap - $highestBoot.addr
    Write-Host ""
    Write-Host ("OVER CLIFF: {0:N0} bytes above the highest address proven to boot, and at or above" -f $over) -ForegroundColor Red
    Write-Host ("            0x{0:x8}, which was measured NOT to boot. Do not spend a measuring" -f $lowestFail.addr) -ForegroundColor Red
    Write-Host  "            run on this build; it will time out looking like a hung harness."   -ForegroundColor Red
    exit 1
}
if ($heap -gt $highestBoot.addr) {
    Write-Host ""
    Write-Host ("UNPROVEN: inside the {0:N0}-byte band between the highest booting and lowest" -f $band) -ForegroundColor Yellow
    Write-Host  "          failing address. Take the 8-sample -StartFrame 60 boot probe before"   -ForegroundColor Yellow
    Write-Host  "          any measuring run, and add the outcome to the ladder in this script."  -ForegroundColor Yellow
    exit 0
}
Write-Host ""
Write-Host ("OK: {0:N0} bytes of PROVEN headroom (to 0x{1:x8})." -f $headroom, $highestBoot.addr) -ForegroundColor Green
Write-Host  "    Proven means another build linked at or below that address booted. Anything"
Write-Host  "    larger is unproven, not safe -- the cliff itself has never been bisected."
exit 0
