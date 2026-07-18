param(
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [ValidateRange(0,1)][int]$Phase2Mode = 1,
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
    $stockBytes = 0
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
        $stockBytes += $expected
    }

    $phase2Bytes = 0
    if ($Phase2Mode -eq 1) {
        $relocatedCompare = Join-Path $resolvedBuild '_arm_cmpsf2.itcm.o'
        $compareSymbols = @(& $Objdump -t $relocatedCompare)
        if ($LASTEXITCODE -ne 0) {
            throw 'Could not inspect the relocated comparison golden object.'
        }
        if (@($compareSymbols | Where-Object {
                $_ -match '\s__aeabi_fcmpeq$'
            }).Count -ne 0 -or
            @($compareSymbols | Where-Object {
                $_ -match '\s__nds_task9_libgcc_fcmpeq_golden$'
            }).Count -ne 1) {
            throw 'Phase 2 did not rename exactly the stock fcmpeq wrapper as its golden.'
        }

        $phase2Object = Join-Path $resolvedBuild 'nds_task9_float_phase2.o'
        if (-not (Test-Path -LiteralPath $phase2Object -PathType Leaf)) {
            throw "Missing Task 9 Phase 2 object '$phase2Object'."
        }
        $phase2Bytes = Get-SectionBytes -Path $phase2Object `
            -Section '.itcm.task9_float_phase2'
        if ($phase2Bytes -ne 0x24) {
            throw "Task 9 Phase 2 fcmpeq leaf is $phase2Bytes bytes instead of 36."
        }
        $phase2Symbols = @(& $Objdump -t $phase2Object)
        if ($LASTEXITCODE -ne 0 -or
            @($phase2Symbols | Where-Object {
                $_ -match '\sF\s+\.itcm\.task9_float_phase2\s+[0-9a-fA-F]+\s+__aeabi_fcmpeq$'
            }).Count -ne 1 -or
            @($phase2Symbols | Where-Object { $_ -match '\*UND\*' }).Count -ne 0) {
            throw 'Task 9 Phase 2 object is not one self-contained fcmpeq ITCM definition.'
        }
        $phase2Disassembly = @(& $Objdump -dr $phase2Object)
        if ($LASTEXITCODE -ne 0) {
            throw 'Could not disassemble the Task 9 Phase 2 object.'
        }
        $phase2Instructions = @($phase2Disassembly | Where-Object {
            $_ -match '^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]{8}\s+'
        })
        if ($phase2Instructions.Count -eq 0 -or
            @($phase2Instructions | Where-Object {
                $_ -match '\bblx?\b|\bpush\b|\bpop\b|\bsp\b'
            }).Count -ne 0) {
            throw 'Task 9 Phase 2 fcmpeq is not a stackless ARM leaf.'
        }
        $phase2Code = Join-Path $temp 'task9-phase2.bin'
        & $Objcopy --dump-section ".itcm.task9_float_phase2=$phase2Code" `
            $phase2Object
        if ($LASTEXITCODE -ne 0) {
            throw 'Could not dump Task 9 Phase 2 machine code.'
        }
        $phase2CodeHash =
            (Get-FileHash -LiteralPath $phase2Code -Algorithm SHA256).Hash
        if ($phase2CodeHash -ne
            '07B3147B9CF599BDD408AF922A4B9F6891734B4C3AB7DE7C3A700DDE92B6FBE2') {
            throw "Task 9 Phase 2 machine code drifted: $phase2CodeHash."
        }
        $codeRows += "phase2-fcmpeq=$phase2Bytes/$phase2CodeHash"
    } else {
        $relocatedCompare = Join-Path $resolvedBuild '_arm_cmpsf2.itcm.o'
        $compareSymbols = @(& $Objdump -t $relocatedCompare)
        if ($LASTEXITCODE -ne 0 -or
            @($compareSymbols | Where-Object {
                $_ -match '\s__aeabi_fcmpeq$'
            }).Count -ne 1 -or
            @($compareSymbols | Where-Object {
                $_ -match '\s__nds_task9_libgcc_fcmpeq_golden$'
            }).Count -ne 0) {
            throw 'Phase 1 comparison object did not retain the stock fcmpeq symbol.'
        }
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
    if ($Phase2Mode -eq 1 -and
        @($symbols | Where-Object {
            $_ -match '\sF\s+\.itcm\s+[0-9a-fA-F]+.*\s__nds_task9_libgcc_fcmpeq_golden$'
        }).Count -ne 1) {
        throw 'Task 9 Phase 2 ELF omitted its selected-libgcc fcmpeq golden.'
    }

    $itcmBytes = Get-SectionBytes -Path $resolvedElf -Section '.itcm'
    if ($itcmBytes -gt 32768) {
        throw "Candidate ITCM use $itcmBytes exceeds 32,768 bytes."
    }
    if ($BaselineElf) {
        $baseBytes = Get-SectionBytes -Path `
            (Resolve-Path -LiteralPath $BaselineElf).Path -Section '.itcm'
        $payloadBytes = $stockBytes + $phase2Bytes
        if (($itcmBytes - $baseBytes) -ne $payloadBytes) {
            throw "Candidate ITCM delta $($itcmBytes - $baseBytes) is not the exact $payloadBytes-byte payload."
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
    Write-Output ("Task 9 float ITCM passed: phase2={0} stockBytes={1} phase2Bytes={2} itcm={3}/32768 free={4} libgcc={5} sha256={6} code=[{7}]" -f
        $Phase2Mode, $stockBytes, $phase2Bytes, $itcmBytes,
        (32768 - $itcmBytes), $libgcc,
        $archiveHash, ($codeRows -join ', '))
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
