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
$expectedCandidateTextHash =
    '07B3147B9CF599BDD408AF922A4B9F6891734B4C3AB7DE7C3A700DDE92B6FBE2'
$asm = Join-Path $PSScriptRoot 'task9_phase2_fcmpeq_candidate.S'
$hostSource = Join-Path $PSScriptRoot 'task9_phase2_fcmpeq_proof.c'

foreach ($tool in @($HostCc, $ArmGcc, $ArmAr, $ArmNm, $ArmObjcopy, $ArmObjdump)) {
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
    ('smash64ds-task9-fcmpeq-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $privateArchive = Join-Path $temp 'libgcc.a'
    $stockObject = Join-Path $temp '_arm_cmpsf2.o'
    $stockText = Join-Path $temp '_arm_cmpsf2.text.bin'
    $candidateObject = Join-Path $temp 'task9_phase2_fcmpeq_candidate.o'
    $candidateText = Join-Path $temp 'task9_phase2_fcmpeq_candidate.bin'
    $hostExe = Join-Path $temp 'task9_phase2_fcmpeq_proof.exe'

    Copy-Item -LiteralPath $libgcc -Destination $privateArchive
    Push-Location $temp
    try {
        & $ArmAr x $privateArchive '_arm_cmpsf2.o'
        if ($LASTEXITCODE -ne 0) { throw 'Fresh libgcc extraction failed.' }
    } finally {
        Pop-Location
    }
    & $ArmObjcopy --dump-section ".text=$stockText" $stockObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump stock compare text.' }
    $stockHash = (Get-FileHash -LiteralPath $stockText -Algorithm SHA256).Hash
    if ($stockHash -ne $expectedCompareTextHash) {
        throw "Selected _arm_cmpsf2.o text drifted: $stockHash"
    }

    & $ArmGcc -c -march=armv5te -mtune=arm946e-s -marm `
        -o $candidateObject $asm
    if ($LASTEXITCODE -ne 0) { throw 'Candidate assembly failed.' }
    $symbol = @(& $ArmNm -S --size-sort $candidateObject | Where-Object {
        $_ -match '\stask9Phase2FcmpeqCandidate$'
    })
    if ($symbol.Count -ne 1 -or $symbol[0] -notmatch '^\S+\s+00000024\s+T\s+') {
        throw "Candidate is not the expected 36-byte ARM leaf: $symbol"
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
        ".itcm.task9_fcmpeq=$candidateText" $candidateObject
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
    Write-Output (("Task 9 Phase 2 fcmpeq proof passed: state={0} " +
        "candidate=36B/0-stack/0-call candidate_sha256={1} " +
        "libgcc={2} libgcc_sha256={3} stock_text_sha256={4}") -f `
        $proofState, $candidateHash, $libgcc, $archiveHash, $stockHash)
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
