$ErrorActionPreference = 'Stop'

function Assert-Condition {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$makeCommand = Get-Command make -CommandType Application -ErrorAction Stop |
    Select-Object -First 1
$probeToken = [Guid]::NewGuid().ToString('N')
$buildRelative = "builds/build-toolchain-path-normalization-$probeToken"
$buildPath = Join-Path $root $buildRelative
$expected = @{
    TOOLCHAIN_DEVKITPRO = '/c/devkitPro'
    TOOLCHAIN_DEVKITARM = '/c/devkitPro/devkitARM'
    TOOLCHAIN_CALICO = '/c/devkitPro/calico'
    TOOLCHAIN_LIBNDS = '/c/devkitPro/libnds'
}
$cases = @(
    [PSCustomObject]@{
        Name = 'Windows backslashes'
        DevkitPro = 'C:\devkitPro'
        DevkitArm = 'C:\devkitPro\devkitARM'
    },
    [PSCustomObject]@{
        Name = 'Windows forward slashes'
        DevkitPro = 'C:/devkitPro'
        DevkitArm = 'C:/devkitPro/devkitARM'
    },
    [PSCustomObject]@{
        Name = 'Lowercase drive'
        DevkitPro = 'c:/devkitPro'
        DevkitArm = 'c:/devkitPro/devkitARM'
    },
    [PSCustomObject]@{
        Name = 'MSYS paths'
        DevkitPro = '/c/devkitPro'
        DevkitArm = '/c/devkitPro/devkitARM'
    },
    [PSCustomObject]@{
        Name = 'Forward trailing slashes'
        DevkitPro = 'C:/devkitPro/'
        DevkitArm = 'C:/devkitPro/devkitARM/'
    },
    [PSCustomObject]@{
        Name = 'Backslash trailing slashes'
        DevkitPro = 'C:\devkitPro\'
        DevkitArm = 'C:\devkitPro\devkitARM\'
    }
)

$hadDevkitPro = Test-Path Env:DEVKITPRO
$hadDevkitArm = Test-Path Env:DEVKITARM
$savedDevkitPro = $env:DEVKITPRO
$savedDevkitArm = $env:DEVKITARM
$locationPushed = $false

try {
    Assert-Condition (-not (Test-Path -LiteralPath $buildPath)) `
        "Toolchain path probe unexpectedly already exists: $buildPath"

    Push-Location $root
    $locationPushed = $true

    foreach ($case in $cases) {
        $env:DEVKITPRO = $case.DevkitPro
        $env:DEVKITARM = $case.DevkitArm

        $rawOutput = @(
            & $makeCommand.Source `
                '--no-print-directory' `
                '-s' `
                '-f' `
                'Makefile' `
                "BUILD=$buildRelative" `
                'print-toolchain-paths' 2>&1 |
                ForEach-Object { "$_" }
        )
        $makeExitCode = $LASTEXITCODE
        Assert-Condition ($makeExitCode -eq 0) `
            ("{0} make probe failed with exit code {1}:`n{2}" -f `
                $case.Name, $makeExitCode, ($rawOutput -join "`n"))

        $actual = @{}
        foreach ($line in $rawOutput) {
            if ($line -match '^(TOOLCHAIN_(?:DEVKITPRO|DEVKITARM|CALICO|LIBNDS))=(.*)$') {
                Assert-Condition (-not $actual.ContainsKey($Matches[1])) `
                    "$($case.Name) emitted duplicate $($Matches[1]) metadata."
                $actual[$Matches[1]] = $Matches[2]
            }
        }

        foreach ($key in $expected.Keys) {
            Assert-Condition ($actual.ContainsKey($key)) `
                "$($case.Name) did not emit $key metadata."
            Assert-Condition ($actual[$key] -ceq $expected[$key]) `
                ("{0} normalized {1} to <{2}>; expected <{3}>." -f `
                    $case.Name, $key, $actual[$key], $expected[$key])
        }

        $normalizedText = $actual.Values -join "`n"
        Assert-Condition (-not $normalizedText.Contains('\')) `
            "$($case.Name) retained a backslash in normalized toolchain paths."
        Assert-Condition (-not [regex]::IsMatch(
                $normalizedText,
                '(?i)[a-z]:devkitPro(?:[/\\]|$)')) `
            "$($case.Name) retained a drive-relative toolchain path."
        Assert-Condition (-not $normalizedText.Contains('//')) `
            "$($case.Name) introduced a double slash in normalized toolchain paths."
        Assert-Condition (-not (Test-Path -LiteralPath $buildPath)) `
            "$($case.Name) created the supposedly nonbuilding probe directory: $buildPath"
    }
} finally {
    if ($locationPushed) {
        Pop-Location
    }
    if ($hadDevkitPro) {
        $env:DEVKITPRO = $savedDevkitPro
    } else {
        Remove-Item Env:DEVKITPRO -ErrorAction SilentlyContinue
    }
    if ($hadDevkitArm) {
        $env:DEVKITARM = $savedDevkitArm
    } else {
        Remove-Item Env:DEVKITARM -ErrorAction SilentlyContinue
    }
}

Write-Output (
    'Toolchain path normalization passed for {0} path spellings; no build directory was created.' -f `
        $cases.Count
)
