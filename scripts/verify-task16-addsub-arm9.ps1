param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateRange(0,127)][int]$RunnerSlot = 7,
    [ValidateRange(0, [long]::MaxValue)][long]$MinimumRandomPairs = 100000000,
    [ValidateRange(60,3600)][int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $PSScriptRoot 'task16_addsub_arm9_lab'
$rom = Join-Path $source 'task16-addsub-arm9-lab.nds'
$elf = Join-Path $source 'task16-addsub-arm9-lab.elf'

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

& make -C $source -j4
if ($LASTEXITCODE -ne 0) { throw 'ARM9 add/sub lab build failed.' }
foreach ($required in @($rom, $elf, $Gdb)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required add/sub proof file was not found: $required"
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

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir 'melonds.task16-addsub-arm9.stdout.log'
$stderr = Join-Path $logDir 'melonds.task16-addsub-arm9.stderr.log'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue

$emulator = $null
$scriptName = '_task16_addsub_arm9.gdb'
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
        'if (gTask16AddSubArm9Result != 0x41313650) && (gTask16AddSubArm9Result != 0x41313646)',
        'break task16AddSubArm9ProofHalt',
        'continue',
        'end',
        'printf "TASK16_ARM9=%#x,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gTask16AddSubArm9Result, gTask16AddSubArm9ValueCount, gTask16AddSubArm9PairCount, gTask16AddSubArm9DirectedOperationCount, gTask16AddSubArm9RandomTarget, gTask16AddSubArm9RandomCompleted, gTask16AddSubArm9MismatchCount, gTask16AddSubArm9FirstOperation, gTask16AddSubArm9FirstLeft, gTask16AddSubArm9FirstRight, gTask16AddSubArm9FirstCandidate, gTask16AddSubArm9FirstGolden',
        'detach',
        'quit'
    )
    $gdbOutput = (Invoke-GdbMarkerScript `
        -Gdb $Gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName $scriptName -TimeoutSeconds $TimeoutSeconds).Stdout
    $marker = [regex]::Match($gdbOutput,
        'TASK16_ARM9=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $marker.Success) {
        throw "ARM9 add/sub proof marker was absent.`n$gdbOutput"
    }
    $result = Convert-MarkerUInt32 $marker.Groups[1].Value
    $valueCount = [uint32]$marker.Groups[2].Value
    $operationCount = [uint32]$marker.Groups[3].Value
    $directedOperationCount = [uint32]$marker.Groups[4].Value
    $randomTarget = [uint32]$marker.Groups[5].Value
    $randomCompleted = [uint32]$marker.Groups[6].Value
    $mismatchCount = [uint32]$marker.Groups[7].Value
    $expectedDirected = 2u * $valueCount * $valueCount
    $expectedOperations = [uint64]$expectedDirected + 2u * $randomTarget
    if (($result -ne 0x41313650) -or ($valueCount -lt 50) -or
        ($directedOperationCount -ne $expectedDirected) -or
        ($randomTarget -lt $MinimumRandomPairs) -or
        ($randomCompleted -ne $randomTarget) -or
        ([uint64]$operationCount -ne $expectedOperations) -or
        ($mismatchCount -ne 0u)) {
        throw "Literal ARM9 add/sub differential failed.`n$gdbOutput"
    }
    Write-Output ("Task 16 ARM9 add/sub proof passed: directedValues={0} directedOperations={1} randomPairs={2} totalOperations={3} mismatches=0 ROM={4} ELF={5}" -f
        $valueCount, $directedOperationCount, $randomTarget,
        $operationCount, $rom, $elf)
} finally {
    if (($null -ne $emulator) -and (-not $emulator.HasExited)) {
        Stop-Process -Id $emulator.Id -Force
        $emulator.WaitForExit()
    }
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
}
