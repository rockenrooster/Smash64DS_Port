param(
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [string]$BaselineElf = '',
    [string]$Gcc = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe',
    [string]$Ar = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-ar.exe',
    [string]$Objcopy = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objcopy.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
)

$ErrorActionPreference = 'Stop'
$expectedArchiveHash =
    'C755ADC33ECA252260360327904591B8462CCE5C25E48B0E881AC0B295953F48'
$members = [ordered]@{
    '_arm_addsubsf3.o' = 0x2ac
    '_arm_muldivsf3.o' = 0x2f8
    '_arm_cmpsf2.o' = 0x114
    '_arm_unordsf2.o' = 0x38
    '_arm_fixsfsi.o' = 0x5c
    '_arm_fixunssfsi.o' = 0x54
}
$helpers = @(
    '__aeabi_fadd', '__aeabi_fsub', '__aeabi_fmul', '__aeabi_fdiv',
    '__aeabi_fcmpeq', '__aeabi_fcmplt', '__aeabi_fcmple',
    '__aeabi_fcmpge', '__aeabi_fcmpgt', '__aeabi_fcmpun',
    '__aeabi_f2iz', '__aeabi_f2uiz', '__aeabi_i2f', '__aeabi_ui2f'
)

function Get-SectionBytes {
    param([string]$Path, [string]$Section)
    $lines = @(& $Objdump -h $Path)
    if ($LASTEXITCODE -ne 0) { throw "objdump failed for '$Path'." }
    $line = @($lines | Where-Object {
        $_ -match ("^\s*\d+\s+" + [regex]::Escape($Section) +
            "\s+([0-9a-fA-F]+)\s+")
    })
    if ($line.Count -ne 1) {
        throw "Expected exactly one $Section section in '$Path'."
    }
    $null = $line[0] -match ("^\s*\d+\s+" + [regex]::Escape($Section) +
        "\s+([0-9a-fA-F]+)\s+")
    return [Convert]::ToInt32($Matches[1], 16)
}

foreach ($tool in @($Gcc, $Ar, $Objcopy, $Objdump)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
        throw "Required tool was not found at '$tool'."
    }
}
$resolvedElf = (Resolve-Path -LiteralPath $Elf).Path
$resolvedBuild = (Resolve-Path -LiteralPath $BuildDirectory).Path
$libgcc = (& $Gcc -march=armv5te -mtune=arm946e-s -mthumb `
    -print-libgcc-file-name).Trim()
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $libgcc)) {
    throw 'Could not resolve the selected Thumb multilib libgcc archive.'
}
$archiveHash = (Get-FileHash -LiteralPath $libgcc -Algorithm SHA256).Hash
if ($archiveHash -ne $expectedArchiveHash) {
    throw "Selected Thumb libgcc archive drifted or is corrupt: $archiveHash."
}

$temp = Join-Path ([System.IO.Path]::GetTempPath()) `
    ("smash64ds-task9-{0}" -f [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $tempArchive = Join-Path $temp 'libgcc.a'
    Copy-Item -LiteralPath $libgcc -Destination $tempArchive
    Push-Location $temp
    try {
        & $Ar x $tempArchive @($members.Keys)
        if ($LASTEXITCODE -ne 0) { throw 'Fresh libgcc member extraction failed.' }
    } finally {
        Pop-Location
    }

    $codeRows = @()
    $expectedBytes = 0
    foreach ($entry in $members.GetEnumerator()) {
        $member = $entry.Key
        $expected = [int]$entry.Value
        $stem = [System.IO.Path]::GetFileNameWithoutExtension($member)
        $fresh = Join-Path $temp $member
        $relocated = Join-Path $resolvedBuild "$stem.itcm.o"
        foreach ($path in @($fresh, $relocated)) {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                throw "Missing Task 9 generated object '$path'."
            }
        }
        if ((Get-SectionBytes -Path $fresh -Section '.text') -ne $expected -or
            (Get-SectionBytes -Path $relocated -Section '.itcm') -ne $expected) {
            throw "$member did not preserve its expected $expected code bytes."
        }

        $rawCode = Join-Path $temp "$stem.text.bin"
        $itcmCode = Join-Path $temp "$stem.itcm.bin"
        & $Objcopy --dump-section ".text=$rawCode" $fresh
        if ($LASTEXITCODE -ne 0) { throw "Could not dump $member .text." }
        & $Objcopy --dump-section ".itcm=$itcmCode" $relocated
        if ($LASTEXITCODE -ne 0) { throw "Could not dump $stem.itcm.o .itcm." }
        $codeHash = (Get-FileHash -LiteralPath $rawCode -Algorithm SHA256).Hash
        if ($codeHash -ne
            (Get-FileHash -LiteralPath $itcmCode -Algorithm SHA256).Hash) {
            throw "$member machine-code bytes changed during section relocation."
        }
        $codeRows += "$member=$expected/$codeHash"
        $expectedBytes += $expected
    }

    $symbols = @(& $Objdump -t $resolvedElf)
    if ($LASTEXITCODE -ne 0) { throw 'Could not read candidate ELF symbols.' }
    foreach ($helper in $helpers) {
        $matches = @($symbols | Where-Object {
            $_ -match ("\sF\s+\.itcm\s+[0-9a-fA-F]+.*\s" +
                [regex]::Escape($helper) + '$')
        })
        if ($matches.Count -ne 1) {
            throw "Expected exactly one $helper definition in ELF .itcm."
        }
    }

    $itcmBytes = Get-SectionBytes -Path $resolvedElf -Section '.itcm'
    if ($itcmBytes -gt 32768) {
        throw "Candidate ITCM use $itcmBytes exceeds 32,768 bytes."
    }
    if ($BaselineElf) {
        $baseBytes = Get-SectionBytes -Path `
            (Resolve-Path -LiteralPath $BaselineElf).Path -Section '.itcm'
        if (($itcmBytes - $baseBytes) -ne $expectedBytes) {
            throw "Candidate ITCM delta $($itcmBytes - $baseBytes) is not the exact $expectedBytes-byte payload."
        }
    }

    $map = Join-Path $resolvedBuild '.map'
    if (Test-Path -LiteralPath $map) {
        $mapText = Get-Content -LiteralPath $map -Raw
        foreach ($member in $members.Keys) {
            if ($mapText -match
                ("libgcc\.a\(" + [regex]::Escape($member) + "\)")) {
                throw "$member was linked again from libgcc instead of being preempted."
            }
        }
    }

    if ((Get-FileHash -LiteralPath $libgcc -Algorithm SHA256).Hash -ne
        $expectedArchiveHash) {
        throw 'Installed Thumb libgcc archive changed during verification.'
    }
    Write-Output ("Task 9 float ITCM passed: bytes={0} itcm={1}/32768 free={2} libgcc={3} sha256={4} code=[{5}]" -f
        $expectedBytes, $itcmBytes, (32768 - $itcmBytes), $libgcc,
        $archiveHash, ($codeRows -join ', '))
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
