param(
    [Parameter(Mandatory=$true)][string]$Elf,
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [string]$BaselineElf = '',
    [string]$Objcopy = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objcopy.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
)

$ErrorActionPreference = 'Stop'
$expectedHash =
    'F822244564E6EFF11F3812B241A2E71ED7E013D34905687C4D9E2E8242C1185D'
$candidateSizes = [ordered]@{
    '__aeabi_fcmplt' = 0x34
    '__aeabi_fcmple' = 0x34
    '__aeabi_fcmpge' = 0x34
    '__aeabi_fcmpgt' = 0x34
    '__aeabi_fcmpun' = 0x1c
}
$goldenSizes = [ordered]@{
    '__nds_task16_libgcc_fcmplt_golden' = 0x18
    '__nds_task16_libgcc_fcmple_golden' = 0x18
    '__nds_task16_libgcc_fcmpge_golden' = 0x18
    '__nds_task16_libgcc_fcmpgt_golden' = 0x18
    '__nds_task16_libgcc_fcmpun_golden' = 0x38
}

function Get-SectionBytes {
    param([string]$Path, [string]$Section)

    $headers = @(& $Objdump -h $Path)
    if ($LASTEXITCODE -ne 0) { throw "Could not read sections from $Path." }
    $row = @($headers | Where-Object {
        $_ -match ('^\s*\d+\s+' + [regex]::Escape($Section) +
            '\s+([0-9a-fA-F]+)\s+')
    })
    if ($row.Count -ne 1 -or $row[0] -notmatch
        ('^\s*\d+\s+' + [regex]::Escape($Section) +
         '\s+([0-9a-fA-F]+)\s+')) {
        throw "Expected exactly one $Section section in $Path."
    }
    return [Convert]::ToInt32($Matches[1], 16)
}

$resolvedElf = (Resolve-Path -LiteralPath $Elf).Path
$resolvedBuild = (Resolve-Path -LiteralPath $BuildDirectory).Path
$candidateObject = Join-Path $resolvedBuild 'nds_task16_float_compare.o'
if (-not (Test-Path -LiteralPath $candidateObject -PathType Leaf)) {
    throw "Task 16 compare object was not found: $candidateObject"
}
if ((Get-SectionBytes $candidateObject '.itcm.task16_float_compare') -ne 236) {
    throw 'Task 16 compare object is not the exact 236-byte payload.'
}

$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('smash64ds-task16-itcm-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $code = Join-Path $temp 'candidate.bin'
    & $Objcopy --dump-section ".itcm.task16_float_compare=$code" `
        $candidateObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump Task 16 compare code.' }
    $hash = (Get-FileHash -LiteralPath $code -Algorithm SHA256).Hash
    if ($hash -ne $expectedHash) {
        throw "Task 16 compare machine code drifted: $hash"
    }

    $objectDisassembly = @(& $Objdump -dr $candidateObject)
    if ($LASTEXITCODE -ne 0 -or
        $objectDisassembly -match '\b(bl|blx|push|pop|stm|ldm)\b') {
        throw 'Task 16 compare object calls or touches a stack frame.'
    }

    $symbols = @(& $Objdump -t $resolvedElf)
    if ($LASTEXITCODE -ne 0) { throw 'Could not read candidate ELF symbols.' }
    foreach ($table in @($candidateSizes, $goldenSizes)) {
        foreach ($name in $table.Keys) {
            $size = '{0:x8}' -f $table[$name]
            if (@($symbols | Where-Object {
                $_ -match ("\sF\s+\.itcm\s+$size.*\s" +
                    [regex]::Escape($name) + '$')
            }).Count -ne 1) {
                throw "Candidate ELF omitted exact ITCM symbol $name/$size."
            }
        }
    }

    $itcmBytes = Get-SectionBytes $resolvedElf '.itcm'
    if ($itcmBytes -gt 32768) {
        throw "Candidate ITCM use $itcmBytes exceeds 32,768 bytes."
    }
    if ($BaselineElf) {
        $baselineBytes = Get-SectionBytes `
            (Resolve-Path -LiteralPath $BaselineElf).Path '.itcm'
        if (($itcmBytes - $baselineBytes) -ne 236) {
            throw "Task 16 ITCM delta $($itcmBytes - $baselineBytes) is not 236."
        }
    }

    $map = Join-Path $resolvedBuild '.map'
    if (Test-Path -LiteralPath $map) {
        $mapText = Get-Content -LiteralPath $map -Raw
        if ($mapText -notmatch
            '(?s)\.itcm\.task16_float_compare\s+0x[0-9a-fA-F]+\s+0xec\s+nds_task16_float_compare\.o') {
            throw 'Task 16 compare payload is absent from the final map.'
        }
    }
    Write-Output ("Task 16 compare ITCM passed: payload=236 " +
        "itcm=$itcmBytes/32768 free=$(32768 - $itcmBytes) " +
        "symbols=fcmplt/fcmple/fcmpge/fcmpgt/fcmpun hash=$hash")
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
