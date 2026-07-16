[CmdletBinding()]
param(
    [switch]$NoBuild,
    [ValidateRange(1, 64)][int]$Jobs = 16,
    [string]$Nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Assert-Condition {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw "freeze diagnostics build failed: $Message"
    }
}

function Get-GlobalSymbols {
    param([string]$Elf)

    $lines = @(& $Nm -g --defined-only $Elf 2>&1 | ForEach-Object { "$_" })
    Assert-Condition ($LASTEXITCODE -eq 0) "nm rejected $Elf`: $($lines -join ' | ')"
    return @($lines | ForEach-Object {
        if ($_ -match '^[0-9a-fA-F]+\s+\S\s+(\S+)$') { $Matches[1] }
    })
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
Assert-Condition (Test-Path -LiteralPath $Nm -PathType Leaf) "nm not found: $Nm"
$make = (Get-Command make -CommandType Application -ErrorAction Stop |
    Select-Object -First 1).Source

$builds = @(
    [pscustomobject]@{
        Mode = 'on'
        Value = 1
        Target = 'smash64ds-battle-playable-freeze-diagnostics-on-hwtri'
        Build = 'build-freeze-diagnostics-on-hwtri'
    },
    [pscustomobject]@{
        Mode = 'off'
        Value = 0
        Target = 'smash64ds-battle-playable-freeze-diagnostics-off-hwtri'
        Build = 'build-freeze-diagnostics-off-hwtri'
    }
)

foreach ($record in $builds) {
    if (-not $NoBuild) {
        $logDir = Join-Path $root 'artifacts\verification\freeze-diagnostics'
        [void](New-Item -ItemType Directory -Force -Path $logDir)
        $stdoutLog = Join-Path $logDir "$($record.Mode)-build.stdout.log"
        $stderrLog = Join-Path $logDir "$($record.Mode)-build.stderr.log"
        $process = Start-Process -FilePath $make -ArgumentList @(
            '--no-print-directory', '-s', '-C', $root,
            "TARGET=$($record.Target)", "BUILD=$($record.Build)", "-j$Jobs"
        ) -WorkingDirectory $root -RedirectStandardOutput $stdoutLog `
            -RedirectStandardError $stderrLog -NoNewWindow -PassThru -Wait
        if ($process.ExitCode -ne 0) {
            $tail = @(
                Get-Content -LiteralPath $stderrLog -Tail 60
                Get-Content -LiteralPath $stdoutLog -Tail 20
            )
            Assert-Condition $false (
                "$($record.Mode) build exited with $($process.ExitCode)`: " +
                ($tail -join ' | '))
        }
    }

    $record | Add-Member NoteProperty BuildPath (
        Join-Path $root (Join-Path 'builds' $record.Build))
    $record | Add-Member NoteProperty Rom (
        Join-Path $record.BuildPath "$($record.Target).nds")
    $record | Add-Member NoteProperty Elf (
        Join-Path $record.BuildPath "$($record.Target).elf")
    $record | Add-Member NoteProperty Config (
        Join-Path $record.BuildPath 'nds_build_config.h')

    Assert-Condition (Test-Path -LiteralPath $record.Rom -PathType Leaf) `
        "$($record.Mode) ROM is missing: $($record.Rom)"
    Assert-Condition (Test-Path -LiteralPath $record.Elf -PathType Leaf) `
        "$($record.Mode) ELF is missing: $($record.Elf)"
    Assert-Condition (Test-Path -LiteralPath $record.Config -PathType Leaf) `
        "$($record.Mode) config is missing: $($record.Config)"

    $configText = Get-Content -LiteralPath $record.Config -Raw
    Assert-Condition ($configText -match (
            '(?m)^#define NDS_FREEZE_DIAGNOSTICS {0}$' -f $record.Value)) `
        "$($record.Mode) config did not select NDS_FREEZE_DIAGNOSTICS=$($record.Value)"
    $record | Add-Member NoteProperty Symbols (Get-GlobalSymbols $record.Elf)
}

$requiredOnSymbols = @(
    'ndsFreezeDiagnosticsInit',
    'ndsFreezeDiagnosticsIrqVector',
    'ndsFreezeDiagnosticsStallMarker',
    'gNdsFreezeDiagnosticsHeartbeat',
    'gNdsFreezeDiagnosticsWatchdogTripCount'
)
foreach ($symbol in $requiredOnSymbols) {
    Assert-Condition ($builds[0].Symbols -ccontains $symbol) `
        "on ELF is missing $symbol"
}
$offDiagnostics = @($builds[1].Symbols | Where-Object {
    $_ -match '^(?:gNdsFreezeDiagnostics|ndsFreezeDiagnostics)'
})
Assert-Condition ($offDiagnostics.Count -eq 0) `
    "off ELF retained diagnostic symbols: $($offDiagnostics -join ', ')"

$onConfig = @(Get-Content -LiteralPath $builds[0].Config | Where-Object {
    $_ -notmatch '^#define NDS_FREEZE_DIAGNOSTICS '
})
$offConfig = @(Get-Content -LiteralPath $builds[1].Config | Where-Object {
    $_ -notmatch '^#define NDS_FREEZE_DIAGNOSTICS '
})
$configDelta = @(Compare-Object -CaseSensitive $onConfig $offConfig)
Assert-Condition ($configDelta.Count -eq 0) `
    "on/off generated configs differ beyond NDS_FREEZE_DIAGNOSTICS: $($configDelta -join ' | ')"

$onHash = (Get-FileHash -LiteralPath $builds[0].Rom -Algorithm SHA256).Hash
$offHash = (Get-FileHash -LiteralPath $builds[1].Rom -Algorithm SHA256).Hash
Assert-Condition ($onHash -cne $offHash) 'on/off ROM hashes are unexpectedly identical'

Write-Output (
    'Freeze diagnostics A/B passed: on={0} sha256={1} off={2} sha256={3}; off_symbols=0' -f `
        $builds[0].Rom, $onHash, $builds[1].Rom, $offHash)
