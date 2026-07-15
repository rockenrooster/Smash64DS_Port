param(
    [string]$HostCC = ''
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

$oracleSource = Join-Path $PSScriptRoot 'native_light_sidecar_oracle.c'
if (-not (Test-Path -LiteralPath $oracleSource -PathType Leaf)) {
    throw "Native light-sidecar oracle source not found: $oracleSource"
}

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
elseif (-not (Test-Path -LiteralPath $HostCC -PathType Leaf)) {
    $hostCommand = Get-Command $HostCC -ErrorAction SilentlyContinue
    if ($null -eq $hostCommand) {
        throw "Host C compiler not found: $HostCC"
    }
    $HostCC = $hostCommand.Source
}

$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempPrefix = $tempBase.TrimEnd(
    [System.IO.Path]::DirectorySeparatorChar,
    [System.IO.Path]::AltDirectorySeparatorChar
) + [System.IO.Path]::DirectorySeparatorChar
$tempDir = [System.IO.Path]::GetFullPath(
    (Join-Path $tempBase ('smash64ds-light-sidecar-' + [guid]::NewGuid()))
)
if (-not $tempDir.StartsWith(
        $tempPrefix,
        [System.StringComparison]::OrdinalIgnoreCase
    )) {
    throw "Refusing temporary output outside $tempBase"
}
[void](New-Item -ItemType Directory -Path $tempDir)
$hostExe = Join-Path $tempDir 'native-light-sidecar-oracle.exe'

try {
    Invoke-Checked -Program $HostCC -Arguments @(
        '-std=c11',
        '-O2',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-ffp-contract=off',
        $oracleSource,
        '-o', $hostExe,
        '-lm'
    )

    $hostOutput = @(& $hostExe)
    $hostExitCode = $LASTEXITCODE
    $joinedOutput = $hostOutput -join [Environment]::NewLine
    if ($hostExitCode -ne 0) {
        if (-not [string]::IsNullOrWhiteSpace($joinedOutput)) {
            Write-Output $joinedOutput
        }
        throw "Native light-sidecar oracle failed with exit code $hostExitCode"
    }

    $requiredPatterns = @(
        'LIGHT_SIDECAR_ORACLE=PASS',
        'matrixMismatch=0',
        'directionMismatch=0',
        'shadeMismatch=0',
        'siblingNegative=[1-9][0-9]*',
        'ownerAlias=[1-9][0-9]*',
        'bindingAlias=[1-9][0-9]*',
        'eligible=1'
    )
    foreach ($pattern in $requiredPatterns) {
        if ($joinedOutput -notmatch $pattern) {
            throw "Native light-sidecar oracle output is missing: $pattern"
        }
    }
    Write-Output $joinedOutput
}
finally {
    if (Test-Path -LiteralPath $tempDir) {
        $resolvedTempDir = [System.IO.Path]::GetFullPath($tempDir)
        if (-not $resolvedTempDir.StartsWith(
                $tempPrefix,
                [System.StringComparison]::OrdinalIgnoreCase
            )) {
            throw "Refusing cleanup outside $tempBase"
        }
        Remove-Item -LiteralPath $resolvedTempDir -Recurse -Force
    }
}
