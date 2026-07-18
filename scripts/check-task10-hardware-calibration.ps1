param(
    [string]$Elf = '',
    [string]$ProductionElf = '',
    [string]$ProductionRom = '',
    [string]$ExpectedProductionRomSha256 = '',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [string]$Readelf = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-readelf.exe'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$makefile = Get-Content -LiteralPath (Join-Path $root 'Makefile') -Raw
$main = Get-Content -LiteralPath (Join-Path $root 'src\nds\main.c') -Raw
$platform = Get-Content -LiteralPath (Join-Path $root 'src\nds\nds_platform.c') -Raw
$taskman = Get-Content -LiteralPath (Join-Path $root 'src\port\taskman_seam.c') -Raw
$lab = Get-Content -LiteralPath (
    Join-Path $root 'src\nds\nds_task10_hardware_calibration.c') -Raw
$registry = Get-Content -LiteralPath (
    Join-Path $root 'scripts\lib\harness-registry.ps1') -Raw

function Assert-Contains {
    param([string]$Text, [string]$Needle, [string]$Message)
    if (-not $Text.Contains($Needle)) { throw $Message }
}

Assert-Contains $makefile 'smash64ds-task10-hardware-calibration' `
    'Task 10 standalone target is missing.'
Assert-Contains $makefile 'ifeq ($(NDS_TASK10_HARDWARE_CALIBRATION),1)' `
    'Task 10 lab source is not conditionally linked.'
Assert-Contains $main '#if NDS_TASK10_HARDWARE_CALIBRATION' `
    'Task 10 lab entry is not compile-time isolated.'
Assert-Contains $platform '#define NDS_BATTLE_PHASE_HUD_ENABLED' `
    'Profile-1 phase HUD gate is missing.'
foreach ($label in @('UPD', 'DRW', 'ACT', 'LOOP', 'SLIP', 'GIT')) {
    Assert-Contains $platform ('"' + $label) `
        "Task 10 HUD label $label is missing."
}
if ($taskman.Contains('gNdsRendererProfilePresentActiveTicks = 0u;')) {
    throw 'ACT is cleared before the HUD can sample the completed frame.'
}
foreach ($needle in @(
    '#define NDS_TASK10_ALU_ITERATIONS 1000000u',
    '#define NDS_TASK10_MAIN_RAM_WORDS (256u * 1024u / sizeof(u32))',
    '#define NDS_TASK10_CACHE_WORDS (4u * 1024u / sizeof(u32))',
    '#define NDS_TASK10_TOTAL_LOADS (8u * 1024u * 1024u)',
    '#define NDS_TASK10_GX_TRIANGLES 10000u',
    '#define NDS_TASK10_GX_SWAP_TRIANGLES 2048u',
    'target("arm"), section(".itcm")',
    'GFX_VERTEX16 = xy0;',
    'glFlush(GL_TRANS_MANUALSORT);')) {
    Assert-Contains $lab $needle "Task 10 lab contract drifted: $needle"
}
if ($registry -match 'task10|hardware.calibration') {
    throw 'Task 10 lab leaked into the harness registry.'
}

if ($Elf) {
    $resolvedElf = (Resolve-Path -LiteralPath $Elf).Path
    $symbols = (& $Readelf --wide -s $resolvedElf) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'readelf failed for Task 10 ELF.' }
    $thumb = [regex]::Match(
        $symbols, '(?m)^\s*\d+:\s+([0-9a-fA-F]+)\s+\d+\s+FUNC.*ndsTask10MemoryThumb$')
    $arm = [regex]::Match(
        $symbols, '(?m)^\s*\d+:\s+([0-9a-fA-F]+)\s+\d+\s+FUNC.*ndsTask10MemoryArm$')
    if (-not $thumb.Success -or -not $arm.Success) {
        throw 'Task 10 memory bench symbols are missing.'
    }
    if (([Convert]::ToUInt32($thumb.Groups[1].Value, 16) -band 1u) -ne 1u) {
        throw 'MEM-THMB is not Thumb-state code.'
    }
    if (([Convert]::ToUInt32($arm.Groups[1].Value, 16) -band 1u) -ne 0u) {
        throw 'MEM-ARM is not ARM-state code.'
    }

    $table = (& $Objdump -t $resolvedElf) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'objdump failed for Task 10 ELF.' }
    if ($table -notmatch
        '(?m)^01ff[0-9a-fA-F]+\s+.*\.itcm\s+[0-9a-fA-F]+\s+ndsTask10DependentAddItcm$') {
        throw 'ALU-ITCM bench is not linked into ITCM.'
    }
    if ($table -notmatch
        '(?m)^02[0-9a-fA-F]+\s+.*\.bss\s+00040000\s+sNdsTask10MainRamBuffer$') {
        throw 'Task 10 256 KiB buffer is not a main-RAM BSS symbol.'
    }
}

if ($ProductionElf) {
    $resolvedProductionElf = (Resolve-Path -LiteralPath $ProductionElf).Path
    $productionTable = (& $Objdump -t $resolvedProductionElf) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'objdump failed for production ELF.' }
    if ($productionTable -match 'ndsTask10|NdsTask10') {
        throw 'Task 10 lab symbols leaked into the production ELF.'
    }
}

if ($ProductionRom -or $ExpectedProductionRomSha256) {
    if (-not $ProductionRom -or -not $ExpectedProductionRomSha256) {
        throw 'ProductionRom and ExpectedProductionRomSha256 must be supplied together.'
    }
    $actualProductionRomSha256 = (
        Get-FileHash -Algorithm SHA256 -LiteralPath $ProductionRom).Hash
    if ($actualProductionRomSha256 -ne $ExpectedProductionRomSha256) {
        throw "Profile-0 ROM drifted: expected $ExpectedProductionRomSha256, " +
            "got $actualProductionRomSha256."
    }
}

Write-Output 'Task 10 hardware calibration structure passed.'
