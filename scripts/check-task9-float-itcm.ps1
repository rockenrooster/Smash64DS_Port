param(
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [ValidateRange(0,1)][int]$Phase2Mode = 1,
    [ValidateRange(0,1)][int]$Task16CompareMode = 0,
    [ValidateRange(0,1)][int]$Task16I2fMode = 0,
    [ValidateRange(0,1)][int]$Task16AddSubMode = 0,
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

function Get-SectionLoadDelta {
    param([string]$Path, [string]$Section)
    $lines = @(& $Objdump -h $Path)
    if ($LASTEXITCODE -ne 0) { throw "objdump failed for '$Path'." }
    $pattern = ('^\s*\d+\s+' + [regex]::Escape($Section) +
        '\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+')
    $rows = @($lines | Where-Object { $_ -match $pattern })
    if ($rows.Count -ne 1 -or $rows[0] -notmatch $pattern) {
        throw "Expected exactly one $Section section in '$Path'."
    }
    return ([Convert]::ToInt64($Matches[2], 16) -
        [Convert]::ToInt64($Matches[1], 16))
}

function Get-ItcmFillBytes {
    param([string]$MapPath)
    $lines = @(Get-Content -LiteralPath $MapPath)
    $inside = $false
    $total = 0
    foreach ($line in $lines) {
        if (-not $inside) {
            if ($line -match '^\.itcm\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+') {
                $inside = $true
            }
            continue
        }
        if ($line -match '^\.\S+\s+0x[0-9a-fA-F]+') { break }
        if ($line -match '^\s+\*fill\*\s+0x[0-9a-fA-F]+\s+0x([0-9a-fA-F]+)') {
            $total += [Convert]::ToInt32($Matches[1], 16)
        }
    }
    if (-not $inside) { throw "Map '$MapPath' omitted the .itcm output section." }
    return $total
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
        $relocatedSource = Join-Path $resolvedBuild "$stem.itcm.o"
        foreach ($path in @($fresh, $relocatedSource)) {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                throw "Missing Task 9 generated object '$path'."
            }
        }
        # Work on a private copy. Multiple same-ROM verifier runs may inspect
        # one build concurrently, and Windows objcopy is not reliable when all
        # processes dump sections from the same input object at once.
        $relocated = Join-Path $temp "$stem.itcm.o"
        Copy-Item -LiteralPath $relocatedSource -Destination $relocated
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

    $relocatedAddSub = Join-Path $temp '_arm_addsubsf3.itcm.o'
    $addSubSymbols = @(& $Objdump -t $relocatedAddSub)
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not inspect the relocated add/sub object.'
    }
    $task16Bytes = 0
    $task16AddSubBytes = 0
    if ($Task16AddSubMode -eq 1) {
        foreach ($routine in @('fadd', 'fsub')) {
            if (@($addSubSymbols | Where-Object {
                    $_ -match ("\s__aeabi_{0}$" -f $routine)
                }).Count -ne 0 -or
                @($addSubSymbols | Where-Object {
                    $_ -match ("\s__nds_task16_libgcc_{0}_golden$" -f $routine)
                }).Count -ne 1) {
                throw "Task 16 did not rename exactly the stock $routine wrapper as its golden."
            }
        }
        $task16ObjectSource = Join-Path $resolvedBuild 'nds_task16_float_addsub.o'
        if (-not (Test-Path -LiteralPath $task16ObjectSource -PathType Leaf)) {
            throw "Missing Task 16 add/sub object '$task16ObjectSource'."
        }
        $task16Object = Join-Path $temp 'nds_task16_float_addsub.o'
        Copy-Item -LiteralPath $task16ObjectSource -Destination $task16Object
        $task16AddSubBytes = Get-SectionBytes -Path $task16Object `
            -Section '.itcm.task16_float_addsub'
        if ($task16AddSubBytes -ne 0x194) {
            throw "Task 16 add/sub leaf is $task16AddSubBytes bytes instead of 404."
        }
        $task16Symbols = @(& $Objdump -t $task16Object)
        if ($LASTEXITCODE -ne 0 -or
            @($task16Symbols | Where-Object {
                $_ -match '\sF\s+\.itcm\.task16_float_addsub\s+00000004\s+__aeabi_fsub$'
            }).Count -ne 1 -or
            @($task16Symbols | Where-Object {
                $_ -match '\sF\s+\.itcm\.task16_float_addsub\s+00000190\s+__aeabi_fadd$'
            }).Count -ne 1) {
            throw 'Task 16 object does not contain the expected shared add/sub leaf.'
        }
        $task16Disassembly = @(& $Objdump -dr $task16Object)
        $task16Instructions = @($task16Disassembly | Where-Object {
            $_ -match '^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]{8}\s+'
        })
        if ($LASTEXITCODE -ne 0 -or $task16Instructions.Count -ne 101 -or
            @($task16Instructions | Where-Object { $_ -match '\bclz\b' }).Count -ne 1 -or
            @($task16Instructions | Where-Object {
                $_ -match '\bblx?\b|\bpush\b|\bpop\b|\bsp\b'
            }).Count -ne 0) {
            throw 'Task 16 add/sub is not the expected stackless, call-free ARM CLZ leaf.'
        }
        $task16Code = Join-Path $temp 'task16-addsub.bin'
        & $Objcopy --dump-section ".itcm.task16_float_addsub=$task16Code" `
            $task16Object
        if ($LASTEXITCODE -ne 0) {
            throw 'Could not dump Task 16 add/sub machine code.'
        }
        $task16CodeHash =
            (Get-FileHash -LiteralPath $task16Code -Algorithm SHA256).Hash
        if ($task16CodeHash -ne
            '9A74410744210A544CC57EA1323C3C9A896D430E2295718870DA4B827E4139FE') {
            throw "Task 16 add/sub machine code drifted: $task16CodeHash."
        }
        $task16Bytes += $task16AddSubBytes
        $codeRows += "task16-addsub=$task16AddSubBytes/$task16CodeHash"
    } else {
        foreach ($routine in @('fadd', 'fsub')) {
            if (@($addSubSymbols | Where-Object {
                    $_ -match ("\s__aeabi_{0}$" -f $routine)
                }).Count -ne 1 -or
                @($addSubSymbols | Where-Object {
                    $_ -match ("\s__nds_task16_libgcc_{0}_golden$" -f $routine)
                }).Count -ne 0) {
                throw "Task 16 off-mode did not retain the stock $routine wrapper."
            }
        }
    }

    $task16I2fBytes = 0
    if ($Task16I2fMode -eq 1) {
        if (@($addSubSymbols | Where-Object {
                $_ -match '\s__aeabi_i2f$'
            }).Count -ne 0 -or
            @($addSubSymbols | Where-Object {
                $_ -match '\s__nds_task16_libgcc_i2f_golden$'
            }).Count -ne 1) {
            throw 'Task 16 did not rename exactly the stock i2f wrapper as its golden.'
        }
        $task16I2fObjectSource = Join-Path $resolvedBuild 'nds_task16_float_i2f.o'
        if (-not (Test-Path -LiteralPath $task16I2fObjectSource -PathType Leaf)) {
            throw "Missing Task 16 i2f object '$task16I2fObjectSource'."
        }
        $task16I2fObject = Join-Path $temp 'nds_task16_float_i2f.o'
        Copy-Item -LiteralPath $task16I2fObjectSource -Destination $task16I2fObject
        $task16I2fBytes = Get-SectionBytes -Path $task16I2fObject `
            -Section '.itcm.task16_i2f'
        if ($task16I2fBytes -ne 0x5c) {
            throw "Task 16 i2f leaf is $task16I2fBytes bytes instead of 92."
        }
        $task16I2fSymbols = @(& $Objdump -t $task16I2fObject)
        if ($LASTEXITCODE -ne 0 -or
            @($task16I2fSymbols | Where-Object {
                $_ -match '\sF\s+\.itcm\.task16_i2f\s+0000005c.*\s__aeabi_i2f$'
            }).Count -ne 1 -or
            @($task16I2fSymbols | Where-Object { $_ -match '\*UND\*' }).Count -ne 0) {
            throw 'Task 16 i2f object is not the exact self-contained 92-byte leaf.'
        }
        $task16I2fDisassembly = @(& $Objdump -dr $task16I2fObject)
        $task16I2fInstructions = @($task16I2fDisassembly | Where-Object {
            $_ -match '^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]{8}\s+'
        })
        if ($LASTEXITCODE -ne 0 -or $task16I2fInstructions.Count -ne 23 -or
            @($task16I2fInstructions | Where-Object { $_ -match '\bclz\b' }).Count -ne 1 -or
            @($task16I2fInstructions | Where-Object {
                $_ -match '\bblx?\b|\bpush\b|\bpop\b|\bsp\b'
            }).Count -ne 0) {
            throw 'Task 16 i2f is not the expected stackless, call-free ARM CLZ leaf.'
        }
        $task16I2fCode = Join-Path $temp 'task16-i2f.bin'
        & $Objcopy --dump-section ".itcm.task16_i2f=$task16I2fCode" `
            $task16I2fObject
        if ($LASTEXITCODE -ne 0) { throw 'Could not dump Task 16 i2f code.' }
        $task16I2fCodeHash =
            (Get-FileHash -LiteralPath $task16I2fCode -Algorithm SHA256).Hash
        if ($task16I2fCodeHash -ne
            'EE6FE8233D98D26A602988362BF014D08515AA477EF493DE55F31B26ED8D0573') {
            throw "Task 16 i2f machine code drifted: $task16I2fCodeHash."
        }
        $task16Bytes += $task16I2fBytes
        $codeRows += "task16-i2f=$task16I2fBytes/$task16I2fCodeHash"
    } elseif (@($addSubSymbols | Where-Object {
            $_ -match '\s__aeabi_i2f$'
        }).Count -ne 1 -or
        @($addSubSymbols | Where-Object {
            $_ -match '\s__nds_task16_libgcc_i2f_golden$'
        }).Count -ne 0) {
        throw 'Task 16 i2f off-mode did not retain the stock wrapper.'
    }

    $phase2Bytes = 0
    if ($Phase2Mode -eq 1) {
        $relocatedCompare = Join-Path $temp '_arm_cmpsf2.itcm.o'
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

        $phase2ObjectSource = Join-Path $resolvedBuild 'nds_task9_float_phase2.o'
        if (-not (Test-Path -LiteralPath $phase2ObjectSource -PathType Leaf)) {
            throw "Missing Task 9 Phase 2 object '$phase2ObjectSource'."
        }
        $phase2Object = Join-Path $temp 'nds_task9_float_phase2.o'
        Copy-Item -LiteralPath $phase2ObjectSource -Destination $phase2Object
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
        $relocatedCompare = Join-Path $temp '_arm_cmpsf2.itcm.o'
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

    $compareRoutines = @('fcmplt', 'fcmple', 'fcmpge', 'fcmpgt')
    $unordObject = Join-Path $temp '_arm_unordsf2.itcm.o'
    $unordSymbols = @(& $Objdump -t $unordObject)
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not inspect the relocated unordered-comparison object.'
    }
    $task16CompareBytes = 0
    if ($Task16CompareMode -eq 1) {
        foreach ($routine in $compareRoutines) {
            if (@($compareSymbols | Where-Object {
                    $_ -match ("\s__aeabi_{0}$" -f $routine)
                }).Count -ne 0 -or
                @($compareSymbols | Where-Object {
                    $_ -match ("\s__nds_task16_libgcc_{0}_golden$" -f $routine)
                }).Count -ne 1) {
                throw "Task 16 did not rename exactly the stock $routine wrapper."
            }
        }
        if (@($unordSymbols | Where-Object {
                $_ -match '\s__aeabi_fcmpun$'
            }).Count -ne 0 -or
            @($unordSymbols | Where-Object {
                $_ -match '\s__nds_task16_libgcc_fcmpun_golden$'
            }).Count -ne 1) {
            throw 'Task 16 did not rename exactly the stock fcmpun wrapper.'
        }
        $task16CompareObjectSource = Join-Path $resolvedBuild `
            'nds_task16_float_compare.o'
        if (-not (Test-Path -LiteralPath $task16CompareObjectSource -PathType Leaf)) {
            throw "Missing Task 16 compare object '$task16CompareObjectSource'."
        }
        $task16CompareObject = Join-Path $temp 'nds_task16_float_compare.o'
        Copy-Item -LiteralPath $task16CompareObjectSource `
            -Destination $task16CompareObject
        $task16CompareBytes = Get-SectionBytes -Path $task16CompareObject `
            -Section '.itcm.task16_float_compare'
        if ($task16CompareBytes -ne 0xec) {
            throw "Task 16 compare leaf is $task16CompareBytes bytes instead of 236."
        }
        $task16CompareSymbols = @(& $Objdump -t $task16CompareObject)
        $candidateCompareSizes = [ordered]@{
            '__aeabi_fcmplt' = 0x34
            '__aeabi_fcmple' = 0x34
            '__aeabi_fcmpge' = 0x34
            '__aeabi_fcmpgt' = 0x34
            '__aeabi_fcmpun' = 0x1c
        }
        foreach ($entry in $candidateCompareSizes.GetEnumerator()) {
            $size = '{0:x8}' -f $entry.Value
            if (@($task16CompareSymbols | Where-Object {
                    $_ -match ("\sF\s+\.itcm\.task16_float_compare\s+$size\s+" +
                        [regex]::Escape($entry.Key) + '$')
                }).Count -ne 1) {
                throw "Task 16 compare object omitted $($entry.Key)/$size."
            }
        }
        if (@($task16CompareSymbols | Where-Object {
                $_ -match '\*UND\*'
            }).Count -ne 0) {
            throw 'Task 16 compare object contains an unresolved symbol.'
        }
        $task16CompareDisassembly = @(& $Objdump -dr $task16CompareObject)
        $task16CompareInstructions = @($task16CompareDisassembly | Where-Object {
            $_ -match '^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]{8}\s+'
        })
        if ($LASTEXITCODE -ne 0 -or $task16CompareInstructions.Count -ne 59 -or
            @($task16CompareInstructions | Where-Object {
                $_ -match '\b(bl|blx|push|pop|stm|ldm)\b|\bsp\b'
            }).Count -ne 0) {
            throw 'Task 16 compare is not the expected 59-instruction stackless ARM payload.'
        }
        $task16CompareCode = Join-Path $temp 'task16-compare.bin'
        & $Objcopy --dump-section `
            ".itcm.task16_float_compare=$task16CompareCode" `
            $task16CompareObject
        if ($LASTEXITCODE -ne 0) { throw 'Could not dump Task 16 compare code.' }
        $task16CompareCodeHash =
            (Get-FileHash -LiteralPath $task16CompareCode -Algorithm SHA256).Hash
        if ($task16CompareCodeHash -ne
            'F822244564E6EFF11F3812B241A2E71ED7E013D34905687C4D9E2E8242C1185D') {
            throw "Task 16 compare machine code drifted: $task16CompareCodeHash."
        }
        $task16Bytes += $task16CompareBytes
        $codeRows += "task16-compare=$task16CompareBytes/$task16CompareCodeHash"
    } else {
        foreach ($routine in $compareRoutines) {
            if (@($compareSymbols | Where-Object {
                    $_ -match ("\s__aeabi_{0}$" -f $routine)
                }).Count -ne 1 -or
                @($compareSymbols | Where-Object {
                    $_ -match ("\s__nds_task16_libgcc_{0}_golden$" -f $routine)
                }).Count -ne 0) {
                throw "Task 16 compare off-mode did not retain stock $routine."
            }
        }
        if (@($unordSymbols | Where-Object {
                $_ -match '\s__aeabi_fcmpun$'
            }).Count -ne 1 -or
            @($unordSymbols | Where-Object {
                $_ -match '\s__nds_task16_libgcc_fcmpun_golden$'
            }).Count -ne 0) {
            throw 'Task 16 compare off-mode did not retain stock fcmpun.'
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
    $finalSizes = [ordered]@{
        '__aeabi_fadd' = if ($Task16AddSubMode -eq 1) { 0x190 } else { 0x1bc }
        '__aeabi_fsub' = if ($Task16AddSubMode -eq 1) { 0x4 } else { 0x1c0 }
        '__aeabi_i2f' = if ($Task16I2fMode -eq 1) { 0x5c } else { 0x20 }
        '__aeabi_fcmplt' = if ($Task16CompareMode -eq 1) { 0x34 } else { 0x18 }
        '__aeabi_fcmple' = if ($Task16CompareMode -eq 1) { 0x34 } else { 0x18 }
        '__aeabi_fcmpge' = if ($Task16CompareMode -eq 1) { 0x34 } else { 0x18 }
        '__aeabi_fcmpgt' = if ($Task16CompareMode -eq 1) { 0x34 } else { 0x18 }
        '__aeabi_fcmpun' = if ($Task16CompareMode -eq 1) { 0x1c } else { 0x38 }
    }
    foreach ($entry in $finalSizes.GetEnumerator()) {
        $size = '{0:x8}' -f $entry.Value
        if (@($symbols | Where-Object {
                $_ -match ("\sF\s+\.itcm\s+$size.*\s" +
                    [regex]::Escape($entry.Key) + '$')
            }).Count -ne 1) {
            throw "ELF helper $($entry.Key) does not match selected size $size."
        }
    }
    if ($Phase2Mode -eq 1 -and
        @($symbols | Where-Object {
            $_ -match '\sF\s+\.itcm\s+[0-9a-fA-F]+.*\s__nds_task9_libgcc_fcmpeq_golden$'
        }).Count -ne 1) {
        throw 'Task 9 Phase 2 ELF omitted its selected-libgcc fcmpeq golden.'
    }
    if ($Phase2Mode -eq 0 -and
        @($symbols | Where-Object {
            $_ -match '\s__nds_task9_libgcc_fcmpeq_golden$'
        }).Count -ne 0) {
        throw 'Task 9 Phase 1 ELF unexpectedly contains the Phase 2 golden symbol.'
    }
    foreach ($routine in @('fadd', 'fsub')) {
        $goldenCount = @($symbols | Where-Object {
            $_ -match ("\sF\s+\.itcm\s+[0-9a-fA-F]+.*\s__nds_task16_libgcc_{0}_golden$" -f $routine)
        }).Count
        if (($Task16AddSubMode -eq 1 -and $goldenCount -ne 1) -or
            ($Task16AddSubMode -eq 0 -and $goldenCount -ne 0)) {
            throw "Task 16 ELF add/sub golden state does not match mode $Task16AddSubMode."
        }
    }
    $i2fGoldenCount = @($symbols | Where-Object {
        $_ -match '\sF\s+\.itcm\s+[0-9a-fA-F]+.*\s__nds_task16_libgcc_i2f_golden$'
    }).Count
    if (($Task16I2fMode -eq 1 -and $i2fGoldenCount -ne 1) -or
        ($Task16I2fMode -eq 0 -and $i2fGoldenCount -ne 0)) {
        throw "Task 16 ELF i2f golden state does not match mode $Task16I2fMode."
    }
    foreach ($routine in @($compareRoutines + 'fcmpun')) {
        $goldenCount = @($symbols | Where-Object {
            $_ -match ("\sF\s+\.itcm\s+[0-9a-fA-F]+.*\s__nds_task16_libgcc_{0}_golden$" -f $routine)
        }).Count
        if (($Task16CompareMode -eq 1 -and $goldenCount -ne 1) -or
            ($Task16CompareMode -eq 0 -and $goldenCount -ne 0)) {
            throw "Task 16 ELF $routine golden state does not match mode $Task16CompareMode."
        }
    }

    $itcmBytes = Get-SectionBytes -Path $resolvedElf -Section '.itcm'
    if ($itcmBytes -gt 32768) {
        throw "Candidate ITCM use $itcmBytes exceeds 32,768 bytes."
    }
    $hotBytes = Get-SectionBytes -Path $resolvedElf -Section '.text.hot'
    if ($hotBytes -ne 0) {
        $hotLoadDelta = Get-SectionLoadDelta -Path $resolvedElf -Section '.text.hot'
        $mainLoadDelta = Get-SectionLoadDelta -Path $resolvedElf -Section '.main'
        if ($mainLoadDelta -ne $hotLoadDelta) {
            throw ".main VMA/LMA delta $mainLoadDelta differs from .text.hot delta $hotLoadDelta."
        }
    }

    $map = Join-Path $resolvedBuild '.map'
    if (-not (Test-Path -LiteralPath $map -PathType Leaf)) {
        throw "Candidate link map was not found: $map"
    }
    $mapText = Get-Content -LiteralPath $map -Raw
    foreach ($member in $members.Keys) {
        if ($mapText -match
            ("libgcc\.a\(" + [regex]::Escape($member) + "\)")) {
            throw "$member was linked again from libgcc instead of being preempted."
        }
    }
    $candidateMapSections = [ordered]@{
        '.itcm.task16_float_compare' = @($Task16CompareMode, '0xec',
            'nds_task16_float_compare.o')
        '.itcm.task16_i2f' = @($Task16I2fMode, '0x5c',
            'nds_task16_float_i2f.o')
        '.itcm.task16_float_addsub' = @($Task16AddSubMode, '0x194',
            'nds_task16_float_addsub.o')
    }
    foreach ($entry in $candidateMapSections.GetEnumerator()) {
        $mode = [int]$entry.Value[0]
        $pattern = ('(?s)' + [regex]::Escape($entry.Key) +
            '\s+0x[0-9a-fA-F]+\s+' + $entry.Value[1] + '\s+' +
            [regex]::Escape($entry.Value[2]))
        $present = $mapText -match $pattern
        if (($mode -eq 1 -and -not $present) -or
            ($mode -eq 0 -and $present)) {
            throw "Map state for $($entry.Key) does not match mode $mode."
        }
    }
    $candidateFillBytes = Get-ItcmFillBytes -MapPath $map
    if ($BaselineElf) {
        $resolvedBaselineElf = (Resolve-Path -LiteralPath $BaselineElf).Path
        $baseBytes = Get-SectionBytes -Path $resolvedBaselineElf -Section '.itcm'
        $baselineMap = Join-Path (Split-Path -Parent $resolvedBaselineElf) '.map'
        if (-not (Test-Path -LiteralPath $baselineMap -PathType Leaf)) {
            throw "Baseline link map was not found: $baselineMap"
        }
        $baselineFillBytes = Get-ItcmFillBytes -MapPath $baselineMap
        $baselineSymbols = @(& $Objdump -t $resolvedBaselineElf)
        if ($LASTEXITCODE -ne 0) { throw 'Could not read baseline ELF symbols.' }
        if ($task16Bytes -gt 0) {
            if (@($baselineSymbols | Where-Object {
                    $_ -match '\s__nds_task16_libgcc_\S+_golden$'
                }).Count -ne 0) {
                throw 'Task 16 baseline already contains a Task 16 golden symbol.'
            }
            $rawPayloadBytes = $task16Bytes
        } else {
            $rawPayloadBytes = $stockBytes + $phase2Bytes
        }
        $expectedDelta = $rawPayloadBytes +
            $candidateFillBytes - $baselineFillBytes
        $actualDelta = $itcmBytes - $baseBytes
        if ($actualDelta -ne $expectedDelta) {
            throw "Candidate ITCM delta $actualDelta is not raw $rawPayloadBytes plus fill delta $($candidateFillBytes - $baselineFillBytes)."
        }
    }

    if ((Get-FileHash -LiteralPath $libgcc -Algorithm SHA256).Hash -ne
        $expectedArchiveHash) {
        throw 'Installed Thumb libgcc archive changed during verification.'
    }
    Write-Output ("Task 9 float ITCM passed: phase2={0} task16Compare/I2f/AddSub={1}/{2}/{3} stockBytes={4} phase2Bytes={5} task16RawBytes={6} fillBytes={7} itcm={8}/32768 free={9} libgcc={10} sha256={11} code=[{12}]" -f
        $Phase2Mode, $Task16CompareMode, $Task16I2fMode,
        $Task16AddSubMode, $stockBytes, $phase2Bytes, $task16Bytes,
        $candidateFillBytes, $itcmBytes, (32768 - $itcmBytes), $libgcc,
        $archiveHash, ($codeRows -join ', '))
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
