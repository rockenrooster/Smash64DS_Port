param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [switch]$AuditLocalConfigs,
    [switch]$SkipLocalConfigs
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

function Assert-Policy {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$window = Set-MelonDSWindowProfile -Text ''
$windowNeedles = @(
    '[Instance0.Window0]',
    'Enabled = true',
    'ShowOSD = false',
    "Geometry = `"$script:MelonDSCanonicalGeometry`"",
    'ScreenLayout = 0',
    'ScreenRotation = 0',
    'ScreenGap = 0',
    'ScreenSwap = false',
    'ScreenSizing = 0',
    'IntegerScaling = false',
    'ScreenAspectTop = 0',
    'ScreenAspectBot = 0',
    'ScreenFilter = false',
    '[Instance0.Window1]',
    '[Instance0.Window2]',
    '[Instance0.Window3]'
)
foreach ($needle in $windowNeedles) {
    Assert-Policy $window.Contains($needle) `
        "Canonical melonDS window profile is missing: $needle"
}
Assert-Policy ((Set-MelonDSWindowProfile -Text $window) -ceq $window) `
    'Canonical melonDS window profile is not idempotent.'
Assert-Policy (
    (($script:MelonDSCanonicalWindowWidth - 16) * 3) -eq
    (($script:MelonDSCanonicalWindowHeight - 64) * 2)) `
    'Canonical melonDS viewport is not the exact 256x384 dual-screen aspect.'

$manual = Set-MelonDSManualProfile -Text ''
foreach ($needle in @(
    'LimitFPS = true', 'Volume = 256', 'Enabled = false',
    'BreakOnStartup = true', 'Port = 3333', 'Port = 3334',
    'Renderer = 1', 'ScaleFactor = 6', 'Enable = false'
)) {
    Assert-Policy $manual.Contains($needle) `
        "Manual melonDS profile is missing: $needle"
}

$automation = Set-MelonDSAutomationProfile -Text '' `
    -GdbPort 4463 -Arm7Port 4464 -MuteAudio
foreach ($needle in @(
    'LimitFPS = false', 'Volume = 0', 'Enabled = true',
    'BreakOnStartup = false', 'Port = 4463', 'Port = 4464',
    'Renderer = 0', 'ScaleFactor = 1', 'Enable = false'
)) {
    Assert-Policy $automation.Contains($needle) `
        "Automation melonDS profile is missing: $needle"
}

$expectedPorts = @{
    0 = @(4323, 4324)
    1 = @(3343, 3344)
    2 = @(4463, 4464)
    3 = @(3363, 3364)
    4 = @(3373, 3374)
    8 = @(3413, 3414)
}
foreach ($entry in $expectedPorts.GetEnumerator()) {
    Assert-Policy (
        (Get-MelonDSRunnerPort -RunnerSlot $entry.Key -Cpu ARM9) -eq
            $entry.Value[0] -and
        (Get-MelonDSRunnerPort -RunnerSlot $entry.Key -Cpu ARM7) -eq
            $entry.Value[1]) `
        "Runner slot $($entry.Key) port mapping drifted."
}

$acceptedPath = Join-Path $Root 'emulators\melonds\melonDS.exe'
Assert-Policy ((Resolve-MelonDSRepoExecutablePath `
    -Root $Root -MelonDS $acceptedPath) -eq
    [System.IO.Path]::GetFullPath($acceptedPath)) `
    'Repo-local melonDS path was rejected.'
$outsideRejected = $false
try {
    $null = Resolve-MelonDSRepoExecutablePath -Root $Root `
        -MelonDS (Join-Path (Split-Path -Parent $Root) 'melonDS.exe')
} catch {
    $outsideRejected = $true
}
Assert-Policy $outsideRejected `
    'An external melonDS path escaped the repo-local executable guard.'

# Path shape alone never proved the runner slots hold the SAME build as
# emulators\melonds\melonDS.exe. On 2026-07-22 all nine slots were found still
# carrying a stock 2025-11-18 melonDS while the source had been replaced with the
# owner's instrumented fork, so every sharded run silently used a different
# emulator than the manual one. Binaries are gitignored, so this stays advisory
# when the source is absent (clean clone) and hard-fails only on real drift.
$slotIdentity = 'skipped'
$sourceMelonDS = Join-Path $Root 'emulators\melonds\melonDS.exe'
$runnerRoot = Join-Path $Root 'emulators\melonds-runners'
if ((Test-Path -LiteralPath $sourceMelonDS -PathType Leaf) -and
    (Test-Path -LiteralPath $runnerRoot -PathType Container)) {
    $sourceLength = (Get-Item -LiteralPath $sourceMelonDS).Length
    $sourceHash = $null
    $checked = 0
    foreach ($slotDir in (Get-ChildItem -LiteralPath $runnerRoot -Directory |
            Where-Object { $_.Name -match '^slot[0-9]+$' } | Sort-Object Name)) {
        $slotMelonDS = Join-Path $slotDir.FullName 'melonDS.exe'
        if (-not (Test-Path -LiteralPath $slotMelonDS -PathType Leaf)) { continue }
        $checked++
        # Length first: it separates a wrong build for free and keeps the common
        # all-correct case to exactly one hash pass over each slot.
        $slotMatches = ((Get-Item -LiteralPath $slotMelonDS).Length -eq $sourceLength)
        if ($slotMatches) {
            if ($null -eq $sourceHash) {
                $sourceHash = (Get-FileHash -LiteralPath $sourceMelonDS `
                    -Algorithm SHA256).Hash
            }
            $slotMatches = ((Get-FileHash -LiteralPath $slotMelonDS `
                -Algorithm SHA256).Hash -eq $sourceHash)
        }
        Assert-Policy $slotMatches (
            "Runner $($slotDir.Name) melonDS.exe is not the repo-owned build in " +
            "emulators\melonds. Refresh every slot with " +
            ".\scripts\New-MelonDSRunnerSlots.ps1 -Count <N> -Force.")
    }
    $slotIdentity = "$checked/$checked"
}

$fastRawBenchmark = Get-Content -LiteralPath (
    Join-Path $Root 'scripts\benchmark-renderer-fast-raw.ps1') -Raw
Assert-Policy ($fastRawBenchmark -match
    '(?s)\$selectedGdbPort\s*=\s*if\s*\(\(\$RunnerSlot\s*-ge\s*0\).*?' +
    'Get-MelonDSRunnerPort\s+-RunnerSlot\s+\$RunnerSlot\s+-Cpu\s+ARM9.*?' +
    '-GdbPort\s+\$selectedGdbPort') `
    'Fast-raw benchmark no longer preserves runner-slot GDB port isolation.'

$launchScripts = @(Get-ChildItem -LiteralPath (Join-Path $Root 'scripts') `
    -Filter '*.ps1' -File | Where-Object {
        (Get-Content -LiteralPath $_.FullName -Raw).Contains('Start-Process') -and
        (Get-Content -LiteralPath $_.FullName -Raw).Contains('melonDsPath')
    })
foreach ($scriptFile in $launchScripts) {
    $text = Get-Content -LiteralPath $scriptFile.FullName -Raw
    Assert-Policy ($text.Contains('Initialize-MelonDSVerifierContext') -or
        $text.Contains('Resolve-MelonDSRepoExecutablePath')) `
        "melonDS launch bypasses the repo-local resolver: $($scriptFile.Name)"
}
$absoluteLiteral = [regex]'(?i)[A-Z]:\\[^\r\n''"]*melonDS\.exe'
foreach ($scriptFile in Get-ChildItem -LiteralPath (Join-Path $Root 'scripts') `
        -Filter '*.ps1' -File) {
    $text = Get-Content -LiteralPath $scriptFile.FullName -Raw
    Assert-Policy (-not $absoluteLiteral.IsMatch($text)) `
        "Hard-coded external melonDS executable found: $($scriptFile.Name)"
}

if ($AuditLocalConfigs -and -not $SkipLocalConfigs -and
    (Test-Path -LiteralPath (Join-Path $Root 'emulators') -PathType Container)) {
    & (Join-Path $PSScriptRoot 'Set-MelonDSWindowConfig.ps1') `
        -Root $Root -AllWorktrees -Check | Out-Null
}

Write-Output (
    'melonDS policy check passed: repo-local executable only; ' +
    "$($script:MelonDSCanonicalWindowWidth)x$($script:MelonDSCanonicalWindowHeight) " +
    'vertical/equal/native/nearest no-bar window profile; manual and automation isolated; ' +
    "runner_slots_match_source=$slotIdentity; " +
    "local_config_audit=$([int]($AuditLocalConfigs -and -not $SkipLocalConfigs)).")
