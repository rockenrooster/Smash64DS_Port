param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot '..'),
    [string]$Python = 'python',
    [string]$HostCC = '',
    [string]$ArmCC = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe'
)

$ErrorActionPreference = 'Stop'

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$Program,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE"
    }
}

$sourceRootPath = (Resolve-Path $SourceRoot).Path
$ownerGenerator = Join-Path $PSScriptRoot 'generate_nds_native_owners.py'
$corpusGenerator = Join-Path $PSScriptRoot 'generate_renderer_parity_corpus.py'
$microbench = Join-Path $PSScriptRoot 'renderer_parity_microbench.c'

if ([string]::IsNullOrWhiteSpace($HostCC)) {
    $hostCommand = Get-Command gcc.exe -ErrorAction SilentlyContinue
    if ($null -eq $hostCommand) {
        $hostCommand = Get-Command clang.exe -ErrorAction SilentlyContinue
    }
    if ($null -eq $hostCommand) {
        throw 'No host C compiler (gcc.exe or clang.exe) is available.'
    }
    $HostCC = $hostCommand.Source
}
if (-not (Test-Path -LiteralPath $ArmCC -PathType Leaf)) {
    throw "ARM9 compiler not found: $ArmCC"
}

Invoke-Checked -Program $Python -Arguments @(
    '-B',
    $ownerGenerator,
    '--check',
    '--source-root', $sourceRootPath
)
Invoke-Checked -Program $Python -Arguments @(
    '-B',
    $corpusGenerator,
    '--check',
    '--source-root', $sourceRootPath
)

$tempBase = [System.IO.Path]::GetFullPath(
    [System.IO.Path]::GetTempPath()
)
$tempDir = [System.IO.Path]::GetFullPath(
    (Join-Path $tempBase ('smash64ds-renderer-parity-' + [guid]::NewGuid()))
)
if (-not $tempDir.StartsWith($tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing temporary output outside $tempBase"
}
[void](New-Item -ItemType Directory -Path $tempDir)
$hostExe = Join-Path $tempDir 'renderer-parity-host.exe'
$armObject = Join-Path $tempDir 'renderer-parity-arm9.o'

try {
    Invoke-Checked -Program $HostCC -Arguments @(
        '-std=c11',
        '-O2',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-DRENDERER_PARITY_HOST_MAIN=1',
        $microbench,
        '-o', $hostExe
    )
    $hostOutput = & $hostExe
    if ($LASTEXITCODE -ne 0) {
        throw "Host parity microbenchmark failed with exit code $LASTEXITCODE"
    }
    $hostOutput | ForEach-Object { Write-Output $_ }

    Invoke-Checked -Program $ArmCC -Arguments @(
        '-std=c11',
        '-Os',
        '-mcpu=arm946e-s',
        '-marm',
        '-ffreestanding',
        '-fno-builtin',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-c', $microbench,
        '-o', $armObject
    )
    $armNm = Join-Path (Split-Path -Parent $ArmCC) 'arm-none-eabi-nm.exe'
    if (-not (Test-Path -LiteralPath $armNm -PathType Leaf)) {
        throw "ARM9 symbol tool not found: $armNm"
    }
    $symbols = & $armNm '-g' '--defined-only' $armObject
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to inspect ARM9 parity object symbols.'
    }
    if (-not ($symbols -match 'smash64dsRendererParityMicrobench')) {
        throw 'ARM9 parity object lost its device-callable no-GX entry point.'
    }
    $armBytes = (Get-Item -LiteralPath $armObject).Length
    if ($armBytes -gt 32768) {
        throw "ARM9 parity object is unexpectedly large: $armBytes bytes"
    }
    Write-Output ((
        "renderer parity device contract: ARM946E-S object={0} bytes, " +
        'no GX/runtime linkage, exported entry point present'
    ) -f $armBytes)
}
finally {
    if (Test-Path -LiteralPath $hostExe) {
        Remove-Item -LiteralPath $hostExe -Force
    }
    if (Test-Path -LiteralPath $armObject) {
        Remove-Item -LiteralPath $armObject -Force
    }
    if (Test-Path -LiteralPath $tempDir) {
        Remove-Item -LiteralPath $tempDir -Force
    }
}
