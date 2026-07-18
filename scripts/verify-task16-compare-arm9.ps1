param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [string]$Objcopy = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objcopy.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [ValidateRange(0,127)][int]$RunnerSlot = 6,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $PSScriptRoot 'task16_compare_arm9_lab'
$build = Join-Path $root 'builds\build-task16-compare-arm9-lab'
$rom = Join-Path $build 'task16-compare-arm9-lab.nds'
$elf = Join-Path $build 'task16-compare-arm9-lab.elf'
$candidateObject = Join-Path $build 'build\nds_task16_float_compare.o'
$compareGolden = Join-Path $build 'build\libgcc_compare_golden.o'
$unorderedGolden = Join-Path $build 'build\libgcc_unordered_golden.o'
$expectedCandidateHash =
    'F822244564E6EFF11F3812B241A2E71ED7E013D34905687C4D9E2E8242C1185D'
$expectedCompareHash =
    '2B656E12FDE0F34CEB17395A5FF8FCC1EF0CEBBF94F8EDA3725BD26C0B3C2884'
$expectedUnorderedHash =
    '7B4D5CFBE032C0D80470495D888A2598C8D77BD434831B0F28488C0070A60407'
$sizes = [ordered]@{
    '__aeabi_fcmplt' = '00000034'
    '__aeabi_fcmple' = '00000034'
    '__aeabi_fcmpge' = '00000034'
    '__aeabi_fcmpgt' = '00000034'
    '__aeabi_fcmpun' = '0000001c'
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
foreach ($tool in @($Gdb, $Objcopy, $Objdump)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
        throw "Required tool was not found: $tool"
    }
}

if (-not $NoBuild) {
    New-Item -ItemType Directory -Force -Path (Join-Path $build 'source') |
        Out-Null
    Copy-Item -LiteralPath (Join-Path $source 'Makefile') `
        -Destination (Join-Path $build 'Makefile') -Force
    Copy-Item -LiteralPath (Join-Path $source 'source\main.c') `
        -Destination (Join-Path $build 'source\main.c') -Force
    Copy-Item -LiteralPath (Join-Path $root 'src\nds\nds_task16_float_compare.s') `
        -Destination (Join-Path $build 'source\nds_task16_float_compare.s') -Force
    & make -C $build -j4
    if ($LASTEXITCODE -ne 0) { throw 'ARM9 Task 16 compare lab build failed.' }
}

foreach ($required in @($rom, $elf, $candidateObject, $compareGolden,
                         $unorderedGolden)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "ARM9 Task 16 compare lab output was not found: $required"
    }
}

$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('smash64ds-task16-arm9-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $candidateCode = Join-Path $temp 'candidate.bin'
    $compareCode = Join-Path $temp 'compare-golden.bin'
    $unorderedCode = Join-Path $temp 'unordered-golden.bin'
    & $Objcopy --dump-section ".itcm.task16_float_compare=$candidateCode" `
        $candidateObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump candidate ARM code.' }
    & $Objcopy --dump-section ".itcm=$compareCode" $compareGolden
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump compare golden ARM code.' }
    & $Objcopy --dump-section ".itcm=$unorderedCode" $unorderedGolden
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump unordered golden ARM code.' }
    if ((Get-FileHash -LiteralPath $candidateCode -Algorithm SHA256).Hash -ne
        $expectedCandidateHash) {
        throw 'ARM9 lab candidate differs from production machine code.'
    }
    if ((Get-FileHash -LiteralPath $compareCode -Algorithm SHA256).Hash -ne
        $expectedCompareHash) {
        throw 'ARM9 compare golden differs from selected libgcc machine code.'
    }
    if ((Get-FileHash -LiteralPath $unorderedCode -Algorithm SHA256).Hash -ne
        $expectedUnorderedHash) {
        throw 'ARM9 unordered golden differs from selected libgcc machine code.'
    }

    $symbols = @(& $Objdump -t $elf)
    if ($LASTEXITCODE -ne 0) { throw 'Could not read ARM9 lab symbols.' }
    foreach ($name in $sizes.Keys) {
        if (@($symbols | Where-Object {
            $_ -match ("\sF\s+\.itcm\s+" + $sizes[$name] +
                ".*\s" + [regex]::Escape($name) + '$')
        }).Count -ne 1) {
            throw "ARM9 lab omitted the exact $name candidate."
        }
        $golden = "__nds_task16_libgcc_$($name.Substring(8))_golden"
        if (@($symbols | Where-Object {
            $_ -match ("\sF\s+\.itcm\s+[0-9a-fA-F]+.*\s" +
                [regex]::Escape($golden) + '$')
        }).Count -ne 1) {
            throw "ARM9 lab omitted $golden."
        }
    }

    if ([string]::IsNullOrWhiteSpace($MelonDS)) {
        $commonGit = (& git -C $root rev-parse --path-format=absolute `
            --git-common-dir).Trim()
        if ($LASTEXITCODE -ne 0) { throw 'Could not resolve the main worktree.' }
        if ($commonGit -match '^/([A-Za-z])/(.*)$') {
            $commonGit = '{0}:/{1}' -f $Matches[1], $Matches[2]
        }
        $MelonDS = Join-Path (Split-Path -Parent $commonGit) `
            'emulators\melonds\melonDS.exe'
    }
    if (-not (Test-Path -LiteralPath $MelonDS -PathType Leaf)) {
        throw "Repo-local melonDS was not found: $MelonDS"
    }

    $slotDir = Join-Path $root "emulators\melonds-runners\slot$RunnerSlot"
    $runner = Join-Path $slotDir 'melonDS.exe'
    $runnerConfig = Join-Path $slotDir 'melonDS.toml'
    if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) {
        New-Item -ItemType Directory -Force -Path $slotDir | Out-Null
        try {
            New-Item -ItemType HardLink -Path $runner -Target $MelonDS | Out-Null
        } catch {
            Copy-Item -LiteralPath $MelonDS -Destination $runner
        }
        $sourceConfig = Join-Path (Split-Path -Parent $MelonDS) 'melonDS.toml'
        if (Test-Path -LiteralPath $sourceConfig -PathType Leaf) {
            Copy-Item -LiteralPath $sourceConfig -Destination $runnerConfig
        }
    }

    $context = Initialize-MelonDSVerifierContext `
        -Root $root -MelonDS $runner -RunnerSlot $RunnerSlot
    $logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
    $stdout = Join-Path $logDir 'melonds.task16-compare-arm9.stdout.log'
    $stderr = Join-Path $logDir 'melonds.task16-compare-arm9.stderr.log'
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue

    $emulator = $null
    $scriptName = '_task16_compare_arm9.gdb'
    try {
        $emulator = Start-Process -FilePath $context.MelonDSPath `
            -ArgumentList $rom `
            -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr `
            -WindowStyle Hidden -PassThru
        Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort |
            Out-Null

        $gdbCommands = @(
            'set pagination off',
            'set confirm off',
            'set remotetimeout 5',
            ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
            'if (gTask16CompareArm9Result != 0x54313650) && (gTask16CompareArm9Result != 0x54313646)',
            'break task16CompareArm9ProofHalt',
            'continue',
            'end',
            'printf "TASK16_ARM9=%#x,%u,%u,%u,%u,%u,%#x,%#x,%u,%u\n", gTask16CompareArm9Result, gTask16CompareArm9ValueCount, gTask16CompareArm9PairCount, gTask16CompareArm9CallCount, gTask16CompareArm9MismatchCount, gTask16CompareArm9FirstPredicate, gTask16CompareArm9FirstLeft, gTask16CompareArm9FirstRight, gTask16CompareArm9FirstCandidate, gTask16CompareArm9FirstGolden',
            'detach',
            'quit'
        )
        $gdbOutput = (Invoke-GdbMarkerScript `
            -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands `
            -ScriptName $scriptName -TimeoutSeconds 30).Stdout
        $marker = [regex]::Match($gdbOutput,
            'TASK16_ARM9=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
        if (-not $marker.Success) {
            throw "ARM9 Task 16 compare proof marker was absent.`n$gdbOutput"
        }
        $result = Convert-MarkerUInt32 $marker.Groups[1].Value
        $valueCount = [uint32]$marker.Groups[2].Value
        $pairCount = [uint32]$marker.Groups[3].Value
        $callCount = [uint32]$marker.Groups[4].Value
        $mismatchCount = [uint32]$marker.Groups[5].Value
        if (($result -ne 0x54313650) -or ($valueCount -lt 40) -or
            ($pairCount -ne ($valueCount * $valueCount)) -or
            ($callCount -ne ($pairCount * 5)) -or ($mismatchCount -ne 0)) {
            throw "Literal ARM9 Task 16 compare differential failed.`n$gdbOutput"
        }
    } finally {
        if (($null -ne $emulator) -and (-not $emulator.HasExited)) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
        Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    }

    $romHash = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
    $elfHash = (Get-FileHash -LiteralPath $elf -Algorithm SHA256).Hash
    Write-Output (("Task 16 literal ARM9 compare proof passed: values={0} " +
        "pairs={1} predicate_calls={2} mismatches=0 ROM={3} ELF={4} " +
        "rom_sha256={5} elf_sha256={6}") -f $valueCount, $pairCount,
        $callCount, $rom, $elf, $romHash, $elfHash)
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
