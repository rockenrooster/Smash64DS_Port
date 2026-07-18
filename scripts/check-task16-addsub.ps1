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
$expectedStockHash =
    'E1F79C55786F2323E18924D71A1268945789DD33C3C8C8861042F41A5CCB4A91'
$expectedCandidateHash =
    '9A74410744210A544CC57EA1323C3C9A896D430E2295718870DA4B827E4139FE'
$asm = Join-Path $PSScriptRoot '..\src\nds\nds_task16_float_addsub.s'
$hostSource = Join-Path $PSScriptRoot 'task16_addsub_proof.c'

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
    ('smash64ds-task16-addsub-' + [guid]::NewGuid().ToString('N'))
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
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump stock add/sub text.' }
    $stockHash = (Get-FileHash -LiteralPath $stockText -Algorithm SHA256).Hash
    if ($stockHash -ne $expectedStockHash) {
        throw "Selected _arm_addsubsf3.o text drifted: $stockHash"
    }

    & $ArmGcc -c -march=armv5te -mtune=arm946e-s -marm `
        -o $candidateObject $asm
    if ($LASTEXITCODE -ne 0) { throw 'Candidate assembly failed.' }
    $symbols = @(& $ArmNm -S --size-sort $candidateObject)
    if (@($symbols | Where-Object {
            $_ -match '^\S+\s+00000004\s+T\s+__aeabi_fsub$'
        }).Count -ne 1 -or
        @($symbols | Where-Object {
            $_ -match '^\S+\s+00000190\s+T\s+__aeabi_fadd$'
        }).Count -ne 1) {
        throw "Candidate symbols are not the expected 404-byte shared leaf.`n$($symbols -join "`n")"
    }
    $disassembly = @(& $ArmObjdump -dr $candidateObject)
    $instructions = @($disassembly | Where-Object {
        $_ -match '^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]{8}\s+'
    })
    if ($instructions.Count -ne 101 -or
        @($instructions | Where-Object { $_ -match '\bclz\b' }).Count -ne 1 -or
        @($instructions | Where-Object {
            $_ -match '\bblx?\b|\bpush\b|\bpop\b|\bsp\b'
        }).Count -ne 0) {
        throw 'Candidate is not the expected stackless, call-free ARM CLZ leaf.'
    }
    & $ArmObjcopy --dump-section `
        ".itcm.task16_float_addsub=$candidateText" $candidateObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump candidate text.' }
    $candidateHash =
        (Get-FileHash -LiteralPath $candidateText -Algorithm SHA256).Hash
    if ($candidateHash -ne $expectedCandidateHash) {
        throw "Candidate machine code drifted: $candidateHash"
    }

    & $HostCc -std=c11 -O3 -Wall -Wextra -Werror `
        -o $hostExe $hostSource
    if ($LASTEXITCODE -ne 0) { throw 'Host add/sub proof build failed.' }
    & $hostExe $RandomVectors
    if ($LASTEXITCODE -ne 0) { throw 'Host add/sub proof failed.' }

    $state = if ($RandomVectors -ge 100000000) { 'complete' } else { 'smoke' }
    Write-Output (("Task 16 add/sub host normalization-model proof passed: state={0} " +
        "candidate=404B/101-insn/0-stack/0-call/1-clz sha256={1} " +
        "libgcc_sha256={2} stock_text_sha256={3}") -f
        $state, $candidateHash, $archiveHash, $stockHash)
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
