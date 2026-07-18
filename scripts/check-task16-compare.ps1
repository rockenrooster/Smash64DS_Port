param(
    [ValidateRange(0, [long]::MaxValue)]
    [long]$RandomVectors = 100000000,
    [string]$HostCc = 'gcc.exe',
    [string]$ArmGcc = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe',
    [string]$ArmAr = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-ar.exe',
    [string]$ArmNm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe',
    [string]$ArmObjcopy = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objcopy.exe',
    [string]$ArmObjdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
)

$ErrorActionPreference = 'Stop'
$expectedArchiveHash =
    'C755ADC33ECA252260360327904591B8462CCE5C25E48B0E881AC0B295953F48'
$expectedCompareTextHash =
    '2B656E12FDE0F34CEB17395A5FF8FCC1EF0CEBBF94F8EDA3725BD26C0B3C2884'
$expectedUnorderedTextHash =
    '7B4D5CFBE032C0D80470495D888A2598C8D77BD434831B0F28488C0070A60407'
$expectedCandidateTextHash =
    'F822244564E6EFF11F3812B241A2E71ED7E013D34905687C4D9E2E8242C1185D'
$asm = Join-Path $PSScriptRoot '..\src\nds\nds_task16_float_compare.s'
$hostSource = Join-Path $PSScriptRoot 'task16_compare_proof.c'
$expectedSymbols = [ordered]@{
    '__aeabi_fcmplt' = 0x34
    '__aeabi_fcmple' = 0x34
    '__aeabi_fcmpge' = 0x34
    '__aeabi_fcmpgt' = 0x34
    '__aeabi_fcmpun' = 0x1c
}

foreach ($tool in @($HostCc, $ArmGcc, $ArmAr, $ArmNm, $ArmObjcopy,
                     $ArmObjdump)) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Required tool was not found: $tool"
    }
}

$libgcc = (& $ArmGcc -march=armv5te -mtune=arm946e-s -mthumb `
    -print-libgcc-file-name).Trim()
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $libgcc)) {
    throw 'Could not resolve the selected Thumb libgcc archive.'
}
$archiveHash = (Get-FileHash -LiteralPath $libgcc -Algorithm SHA256).Hash
if ($archiveHash -ne $expectedArchiveHash) {
    throw "Selected libgcc archive drifted: $archiveHash"
}

$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('smash64ds-task16-compare-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $privateArchive = Join-Path $temp 'libgcc.a'
    $compareObject = Join-Path $temp '_arm_cmpsf2.o'
    $unorderedObject = Join-Path $temp '_arm_unordsf2.o'
    $compareText = Join-Path $temp '_arm_cmpsf2.text.bin'
    $unorderedText = Join-Path $temp '_arm_unordsf2.text.bin'
    $candidateObject = Join-Path $temp 'task16_compare.o'
    $candidateText = Join-Path $temp 'task16_compare.bin'
    $hostExe = Join-Path $temp 'task16_compare_proof.exe'

    Copy-Item -LiteralPath $libgcc -Destination $privateArchive
    Push-Location $temp
    try {
        & $ArmAr x $privateArchive '_arm_cmpsf2.o' '_arm_unordsf2.o'
        if ($LASTEXITCODE -ne 0) { throw 'Fresh libgcc extraction failed.' }
    } finally {
        Pop-Location
    }
    & $ArmObjcopy --dump-section ".text=$compareText" $compareObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump stock compare text.' }
    & $ArmObjcopy --dump-section ".text=$unorderedText" $unorderedObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump stock unordered text.' }
    if ((Get-FileHash -LiteralPath $compareText -Algorithm SHA256).Hash -ne
        $expectedCompareTextHash) {
        throw 'Selected _arm_cmpsf2.o text drifted.'
    }
    if ((Get-FileHash -LiteralPath $unorderedText -Algorithm SHA256).Hash -ne
        $expectedUnorderedTextHash) {
        throw 'Selected _arm_unordsf2.o text drifted.'
    }

    & $ArmGcc -c -march=armv5te -mtune=arm946e-s -marm `
        -o $candidateObject $asm
    if ($LASTEXITCODE -ne 0) { throw 'Candidate assembly failed.' }
    $symbols = @(& $ArmNm -S --size-sort $candidateObject)
    foreach ($name in $expectedSymbols.Keys) {
        $size = '{0:x8}' -f $expectedSymbols[$name]
        $matches = @($symbols | Where-Object {
            $_ -match ("^\S+\s+$size\s+T\s+" + [regex]::Escape($name) + '$')
        })
        if ($matches.Count -ne 1) {
            throw "$name is not the expected $($expectedSymbols[$name])-byte ARM leaf."
        }
    }
    $disassembly = @(& $ArmObjdump -dr $candidateObject)
    if ($LASTEXITCODE -ne 0) { throw 'Candidate disassembly failed.' }
    if ($disassembly -match '\b(bl|blx|push|pop|stm|ldm)\b') {
        throw 'Candidate unexpectedly calls or touches a stack frame.'
    }
    if (-not ($disassembly -match '^\s*0:\s+e1a02080\s+')) {
        throw 'Candidate did not assemble as ARM-state code.'
    }
    & $ArmObjcopy --dump-section `
        ".itcm.task16_float_compare=$candidateText" $candidateObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump candidate text.' }
    $candidateHash =
        (Get-FileHash -LiteralPath $candidateText -Algorithm SHA256).Hash
    if ($candidateHash -ne $expectedCandidateTextHash) {
        throw "Candidate machine code drifted: $candidateHash"
    }

    & $HostCc -std=c11 -O3 -fno-fast-math -ffp-contract=off `
        -Wall -Wextra -Werror -o $hostExe $hostSource
    if ($LASTEXITCODE -ne 0) { throw 'Host proof build failed.' }
    & $hostExe $RandomVectors
    if ($LASTEXITCODE -ne 0) { throw 'Host differential proof failed.' }

    $proofState = if ($RandomVectors -ge 100000000) { 'complete' } else { 'smoke' }
    Write-Output (("Task 16 compare proof passed: state={0} " +
        "symbols=fcmplt/fcmple/fcmpge/fcmpgt/fcmpun bytes=52/52/52/52/28 " +
        "total=236 stack=0 calls=0 candidate_sha256={1} " +
        "libgcc_sha256={2}") -f $proofState, $candidateHash, $archiveHash)
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
