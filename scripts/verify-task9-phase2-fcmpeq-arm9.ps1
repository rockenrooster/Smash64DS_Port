param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [string]$Objcopy = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objcopy.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [ValidateRange(0,127)][int]$RunnerSlot = 7,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $PSScriptRoot 'task9_phase2_arm9_lab'
$build = Join-Path $root 'builds\build-task9-phase2-fcmpeq-arm9-lab'
$rom = Join-Path $build 'task9-phase2-fcmpeq-arm9-lab.nds'
$elf = Join-Path $build 'task9-phase2-fcmpeq-arm9-lab.elf'
$candidateObject = Join-Path $build 'build\fcmpeq_candidate.o'
$goldenObject = Join-Path $build 'build\libgcc_fcmpeq_golden.o'
$expectedCandidateHash =
    '07B3147B9CF599BDD408AF922A4B9F6891734B4C3AB7DE7C3A700DDE92B6FBE2'
$expectedGoldenHash =
    '2B656E12FDE0F34CEB17395A5FF8FCC1EF0CEBBF94F8EDA3725BD26C0B3C2884'

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
    Copy-Item -LiteralPath (Join-Path $source 'source\fcmpeq_candidate.s') `
        -Destination (Join-Path $build 'source\fcmpeq_candidate.s') -Force
    foreach ($dependency in @('main.d', 'fcmpeq_candidate.d')) {
        Remove-Item -LiteralPath (Join-Path $build "build\$dependency") `
            -Force -ErrorAction SilentlyContinue
    }
    & make -C $build -j4
    if ($LASTEXITCODE -ne 0) { throw 'ARM9 fcmpeq lab build failed.' }
}

foreach ($required in @($rom, $elf, $candidateObject, $goldenObject)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "ARM9 fcmpeq lab output was not found: $required"
    }
}

$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('smash64ds-task9-arm9-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $candidateCode = Join-Path $temp 'candidate.bin'
    $goldenCode = Join-Path $temp 'golden.bin'
    & $Objcopy --dump-section ".itcm.task9_float_phase2=$candidateCode" `
        $candidateObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump candidate ARM code.' }
    & $Objcopy --dump-section ".itcm=$goldenCode" $goldenObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump golden ARM code.' }
    if ((Get-FileHash -LiteralPath $candidateCode -Algorithm SHA256).Hash -ne
        $expectedCandidateHash) {
        throw 'ARM9 lab candidate differs from the production machine code.'
    }
    if ((Get-FileHash -LiteralPath $goldenCode -Algorithm SHA256).Hash -ne
        $expectedGoldenHash) {
        throw 'ARM9 lab golden differs from the selected libgcc machine code.'
    }

    $symbols = @(& $Objdump -t $elf)
    if ($LASTEXITCODE -ne 0 -or
        @($symbols | Where-Object {
            $_ -match '\sF\s+\.itcm\s+00000024.*\s__aeabi_fcmpeq$'
        }).Count -ne 1 -or
        @($symbols | Where-Object {
            $_ -match '\sF\s+\.itcm\s+00000018.*\s__nds_task9_libgcc_fcmpeq_golden$'
        }).Count -ne 1) {
        throw 'Candidate and literal libgcc golden are not both linked in ARM9 ITCM.'
    }
    $disassembly = @(& $Objdump -d $elf)
    if ($LASTEXITCODE -ne 0 -or
        @($disassembly | Where-Object {
            $_ -match '\bblx?\b.*<__aeabi_fcmpeq>$'
        }).Count -ne 1 -or
        @($disassembly | Where-Object {
            $_ -match '\bblx?\b.*<__nds_task9_libgcc_fcmpeq_golden>$'
        }).Count -ne 1) {
        throw 'ARM9 lab does not call each literal comparison implementation exactly once per pair.'
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
            New-Item -ItemType HardLink -Path $runner -Target $MelonDS |
                Out-Null
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
    $stdout = Join-Path $logDir 'melonds.task9-fcmpeq-arm9.stdout.log'
    $stderr = Join-Path $logDir 'melonds.task9-fcmpeq-arm9.stderr.log'
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue

    $emulator = $null
    $scriptName = '_task9_fcmpeq_arm9.gdb'
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
            'if (gTask9FcmpeqArm9Result != 0x46395150) && (gTask9FcmpeqArm9Result != 0x46395146)',
            'break task9Phase2Arm9ProofHalt',
            'continue',
            'end',
            'printf "TASK9_ARM9=%#x,%u,%u,%u,%#x,%#x,%u,%u\n", gTask9FcmpeqArm9Result, gTask9FcmpeqArm9ValueCount, gTask9FcmpeqArm9PairCount, gTask9FcmpeqArm9MismatchCount, gTask9FcmpeqArm9FirstLeft, gTask9FcmpeqArm9FirstRight, gTask9FcmpeqArm9FirstCandidate, gTask9FcmpeqArm9FirstGolden',
            'detach',
            'quit'
        )
        $gdbOutput = (Invoke-GdbMarkerScript `
            -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands `
            -ScriptName $scriptName -TimeoutSeconds 30).Stdout
        $marker = [regex]::Match($gdbOutput,
            'TASK9_ARM9=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
        if (-not $marker.Success) {
            throw "ARM9 fcmpeq proof marker was absent.`n$gdbOutput"
        }
        $result = Convert-MarkerUInt32 $marker.Groups[1].Value
        $valueCount = [uint32]$marker.Groups[2].Value
        $pairCount = [uint32]$marker.Groups[3].Value
        $mismatchCount = [uint32]$marker.Groups[4].Value
        if (($result -ne 0x46395150) -or ($valueCount -lt 40) -or
            ($pairCount -ne ($valueCount * $valueCount)) -or
            ($mismatchCount -ne 0)) {
            throw "Literal ARM9 fcmpeq differential failed.`n$gdbOutput"
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
    Write-Output (("Task 9 Phase 2 literal ARM9 proof passed: values={0} " +
        "pairs={1} mismatches=0 ROM={2} ELF={3} rom_sha256={4} " +
        "elf_sha256={5}") -f $valueCount, $pairCount, $rom, $elf,
        $romHash, $elfHash)
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
