param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [string]$Objcopy = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objcopy.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [ValidateRange(0,127)][int]$RunnerSlot = 10,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $PSScriptRoot 'task16_i2f_arm9_lab'
$build = Join-Path $root 'builds\build-task16-i2f-arm9-lab'
$rom = Join-Path $build 'task16-i2f-arm9-lab.nds'
$elf = Join-Path $build 'task16-i2f-arm9-lab.elf'
$candidateObject = Join-Path $build 'build\i2f_candidate.o'
$goldenObject = Join-Path $build 'build\libgcc_i2f_golden.o'
$expectedCandidateHash =
    'EE6FE8233D98D26A602988362BF014D08515AA477EF493DE55F31B26ED8D0573'
$expectedGoldenHash =
    'E1F79C55786F2323E18924D71A1268945789DD33C3C8C8861042F41A5CCB4A91'

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

if (-not $NoBuild) {
    New-Item -ItemType Directory -Force -Path (Join-Path $build 'source') |
        Out-Null
    Copy-Item -LiteralPath (Join-Path $source 'Makefile') `
        -Destination (Join-Path $build 'Makefile') -Force
    Copy-Item -LiteralPath (Join-Path $source 'source\main.c') `
        -Destination (Join-Path $build 'source\main.c') -Force
    Copy-Item -LiteralPath (Join-Path $root `
        'src\nds\nds_task16_float_i2f.s') `
        -Destination (Join-Path $build 'source\i2f_candidate.s') -Force
    & make -C $build -j4
    if ($LASTEXITCODE -ne 0) { throw 'ARM9 i2f lab build failed.' }
}

foreach ($required in @($rom, $elf, $candidateObject, $goldenObject)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "ARM9 i2f lab output was not found: $required"
    }
}

$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('smash64ds-task16-i2f-arm9-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    $candidateCode = Join-Path $temp 'candidate.bin'
    $goldenCode = Join-Path $temp 'golden.bin'
    & $Objcopy --dump-section ".itcm.task16_i2f=$candidateCode" `
        $candidateObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump candidate ARM code.' }
    & $Objcopy --dump-section ".itcm=$goldenCode" $goldenObject
    if ($LASTEXITCODE -ne 0) { throw 'Could not dump golden ARM code.' }
    if ((Get-FileHash $candidateCode -Algorithm SHA256).Hash -ne
        $expectedCandidateHash) {
        throw 'ARM9 lab candidate differs from the audited machine code.'
    }
    if ((Get-FileHash $goldenCode -Algorithm SHA256).Hash -ne
        $expectedGoldenHash) {
        throw 'ARM9 lab golden differs from the selected libgcc object.'
    }

    $symbols = @(& $Objdump -t $elf)
    if ($LASTEXITCODE -ne 0 -or
        @($symbols | Where-Object {
            $_ -match '\sF\s+\.itcm\s+0000005c.*\s__aeabi_i2f$'
        }).Count -ne 1 -or
        @($symbols | Where-Object {
            $_ -match '\sF\s+\.itcm\s+00000020.*\s__nds_task16_libgcc_i2f_golden$'
        }).Count -ne 1) {
        throw 'Candidate and literal libgcc golden are not both linked in ARM9 ITCM.'
    }

    if ([string]::IsNullOrWhiteSpace($MelonDS)) {
        $commonGit = (& git -C $root rev-parse --path-format=absolute `
            --git-common-dir).Trim()
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
    }
    $sourceConfig = Join-Path (Split-Path -Parent $MelonDS) 'melonDS.toml'
    if (Test-Path -LiteralPath $sourceConfig -PathType Leaf) {
        Copy-Item -LiteralPath $sourceConfig -Destination $runnerConfig -Force
    }

    $context = Initialize-MelonDSVerifierContext `
        -Root $root -MelonDS $runner -RunnerSlot $RunnerSlot
    $logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
    $stdout = Join-Path $logDir 'melonds.task16-i2f-arm9.stdout.log'
    $stderr = Join-Path $logDir 'melonds.task16-i2f-arm9.stderr.log'
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue

    $emulator = $null
    $scriptName = '_task16_i2f_arm9.gdb'
    try {
        $emulator = Start-Process -FilePath $context.MelonDSPath `
            -ArgumentList $rom `
            -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr `
            -WindowStyle Hidden -PassThru
        Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort |
            Out-Null

        $commands = @(
            'set pagination off',
            'set confirm off',
            'set remotetimeout 5',
            ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
            'if (gTask16I2fResult != 0x49324650) && (gTask16I2fResult != 0x49324646)',
            'break task16I2fArm9ProofHalt',
            'continue',
            'end',
            'printf "TASK16_I2F_ARM9=%#x,%u,%u,%#x,%#x,%#x,%u,%u,%u,%#x\n", gTask16I2fResult, gTask16I2fVectorCount, gTask16I2fMismatchCount, gTask16I2fFirstInput, gTask16I2fFirstCandidate, gTask16I2fFirstGolden, gTask16I2fCandidateTicks, gTask16I2fGoldenTicks, gTask16I2fTimingCount, gTask16I2fTimingSink',
            'detach',
            'quit'
        )
        $gdbOutput = (Invoke-GdbMarkerScript `
            -Gdb $Gdb -Elf $elf -Root $root -Commands $commands `
            -ScriptName $scriptName -TimeoutSeconds 60).Stdout
        $marker = [regex]::Match($gdbOutput,
            'TASK16_I2F_ARM9=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
        if (-not $marker.Success) {
            throw "ARM9 i2f proof marker was absent.`n$gdbOutput"
        }
        $result = Convert-MarkerUInt32 $marker.Groups[1].Value
        $vectors = [uint32]$marker.Groups[2].Value
        $mismatches = [uint32]$marker.Groups[3].Value
        $candidateTicks = [uint32]$marker.Groups[7].Value
        $goldenTicks = [uint32]$marker.Groups[8].Value
        $timingCount = [uint32]$marker.Groups[9].Value
        if (($result -ne 0x49324650) -or ($vectors -ne 393256u) -or
            ($mismatches -ne 0) -or ($timingCount -ne 65536u) -or
            ($candidateTicks -ge $goldenTicks)) {
            throw "Literal ARM9 i2f differential/timing failed.`n$gdbOutput"
        }
    } finally {
        if (($null -ne $emulator) -and (-not $emulator.HasExited)) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
        Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    }

    $romHash = (Get-FileHash $rom -Algorithm SHA256).Hash
    $elfHash = (Get-FileHash $elf -Algorithm SHA256).Hash
    Write-Output (("Task 16 i2f literal ARM9 proof passed: vectors={0} " +
        "mismatches=0 candidate/golden_ticks={1}/{2} saving={3} " +
        "ROM={4} ELF={5} rom_sha256={6} elf_sha256={7}") -f `
        $vectors, $candidateTicks, $goldenTicks,
        ($goldenTicks - $candidateTicks), $rom, $elf, $romHash, $elfHash)
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
