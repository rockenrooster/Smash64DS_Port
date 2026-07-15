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
    'vertical/equal/native/nearest window profile; manual and automation isolated; ' +
    "local_config_audit=$([int]($AuditLocalConfigs -and -not $SkipLocalConfigs)).")
