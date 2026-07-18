param(
    [ValidateSet(1,2,4,8,16,32)][int]$Workers = 16,
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
$expectedStockTextHash =
    'E1F79C55786F2323E18924D71A1268945789DD33C3C8C8861042F41A5CCB4A91'
$expectedCandidateHash =
    'EE6FE8233D98D26A602988362BF014D08515AA477EF493DE55F31B26ED8D0573'
$candidateSource = Join-Path $PSScriptRoot `
    '..\src\nds\nds_task16_float_i2f.s'
$hostSource = Join-Path $PSScriptRoot 'task16_i2f_proof.c'

foreach ($tool in @(
    $HostCc, $ArmGcc, $ArmAr, $ArmNm, $ArmObjcopy, $ArmObjdump
)) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Required tool was not found: $tool"
    }
}

$libgcc = (& $ArmGcc -march=armv5te -mtune=arm946e-s -mthumb `
    -print-libgcc-file-name).Trim()
if ((Get-FileHash $libgcc -Algorithm SHA256).Hash -ne $expectedArchiveHash) {
    throw 'Selected Thumb libgcc archive drifted.'
}

$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('smash64ds-task16-i2f-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $archive = Join-Path $temp 'libgcc.a'
    $stockObject = Join-Path $temp '_arm_addsubsf3.o'
    $stockText = Join-Path $temp 'stock.bin'
    $candidateObject = Join-Path $temp 'candidate.o'
    $candidateText = Join-Path $temp 'candidate.bin'
    $hostExe = Join-Path $temp 'proof.exe'

    Copy-Item -LiteralPath $libgcc -Destination $archive
    Push-Location $temp
    try {
        & $ArmAr x $archive '_arm_addsubsf3.o'
        if ($LASTEXITCODE -ne 0) { throw 'Fresh libgcc extraction failed.' }
    } finally {
        Pop-Location
    }
    & $ArmObjcopy --dump-section ".text=$stockText" $stockObject
    if ($LASTEXITCODE -ne 0 -or
        (Get-FileHash $stockText -Algorithm SHA256).Hash -ne
            $expectedStockTextHash) {
        throw 'Selected _arm_addsubsf3.o text drifted.'
    }
    $stockSymbols = @(& $ArmNm -S --size-sort $stockObject)
    if (@($stockSymbols | Where-Object {
        $_ -match '^\S+\s+00000020\s+T\s+__aeabi_i2f$'
    }).Count -ne 1) {
        throw 'Stock __aeabi_i2f is not the audited 32-byte entry stub.'
    }

    & $ArmGcc -c -march=armv5te -mtune=arm946e-s -marm `
        -o $candidateObject $candidateSource
    if ($LASTEXITCODE -ne 0) { throw 'Candidate assembly failed.' }
    $candidateSymbols = @(& $ArmNm -S --size-sort $candidateObject)
    if (@($candidateSymbols | Where-Object {
        $_ -match '^\S+\s+0000005c\s+T\s+__aeabi_i2f$'
    }).Count -ne 1) {
        throw 'Candidate is not the expected 92-byte ARM leaf.'
    }
    $disassembly = @(& $ArmObjdump -dr $candidateObject)
    $instructions = @($disassembly | Where-Object {
        $_ -match '^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]{8}\s+'
    })
    if ($instructions.Count -ne 23 -or
        @($instructions | Where-Object {
            $_ -match '\b(bl|blx|push|pop|stm|ldm)\b|\bsp\b'
        }).Count -ne 0 -or
        @($instructions | Where-Object { $_ -match '\bclz\b' }).Count -ne 1) {
        throw 'Candidate is not the audited 23-instruction stackless/call-free CLZ leaf.'
    }
    & $ArmObjcopy --dump-section `
        ".itcm.task16_i2f=$candidateText" $candidateObject
    if ($LASTEXITCODE -ne 0 -or
        (Get-FileHash $candidateText -Algorithm SHA256).Hash -ne
            $expectedCandidateHash) {
        throw 'Candidate machine code drifted.'
    }

    & $HostCc -std=c11 -O3 -fno-fast-math -ffp-contract=off `
        -Wall -Wextra -Werror -o $hostExe $hostSource
    if ($LASTEXITCODE -ne 0) { throw 'Host proof build failed.' }

    $slice = [uint64]0x100000000 / $Workers
    $processes = @()
    for ($index = 0; $index -lt $Workers; ++$index) {
        $start = [uint64]$index * $slice
        $stdout = Join-Path $temp ("proof-{0:D2}.out" -f $index)
        $stderr = Join-Path $temp ("proof-{0:D2}.err" -f $index)
        $processes += Start-Process -FilePath $hostExe `
            -ArgumentList @($start, $slice) `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr `
            -WindowStyle Hidden -PassThru
    }
    $processes | Wait-Process
    for ($index = 0; $index -lt $Workers; ++$index) {
        $processes[$index].Refresh()
        $stdout = Join-Path $temp ("proof-{0:D2}.out" -f $index)
        $stderr = Join-Path $temp ("proof-{0:D2}.err" -f $index)
        $expected = '^PASS start={0} count={1} checksum=[0-9a-f]{{16}}$' -f `
            ([uint64]$index * $slice), $slice
        if (($processes[$index].ExitCode -ne 0) -or
            ((Get-Content $stdout -Raw).Trim() -notmatch $expected)) {
            throw "Exhaustive host worker $index failed: $(Get-Content $stderr -Raw)"
        }
    }

    Write-Output (("Task 16 i2f exhaustive proof passed: inputs={0} " +
        "workers={1} candidate=92B/23-insn/0-stack/0-call " +
        "candidate_sha256={2} stock_text_sha256={3} libgcc_sha256={4}") -f `
        ([uint64]$slice * $Workers), $Workers, $expectedCandidateHash,
        $expectedStockTextHash, $expectedArchiveHash)
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
