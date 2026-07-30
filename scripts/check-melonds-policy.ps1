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

# DLDI must be ON in both profiles and must point at the one repo-owned image.
# It was pinned by nothing until 2026-07-29: the manual emulator had it true and
# all nine runner slots had it false, so no scripted verifier ran the I/O
# configuration the owner plays in -- which is a plausible reason the freeze
# reports never reproduced under automation. Asserted per-section, because a bare
# 'Enable = true' needle would be satisfied by the unrelated [Instance0.Gdb] key.
$dldiImage = ([System.IO.Path]::GetFullPath((Join-Path $Root `
    'emulators\melonds\dldi.bin'))) -replace '\\', '/'
foreach ($profile in @(
    @{ Name = 'manual'; Text = $manual; ReadOnly = 'false' },
    @{ Name = 'automation'
       Text = (Set-MelonDSAutomationProfile -Text '' -GdbPort 4463 `
                   -Arm7Port 4464 -MuteAudio)
       ReadOnly = 'true' }
)) {
    $dldi = [regex]::Match($profile.Text,
        '(?ms)^\[DLDI\]\r?\n(.*?)(?=^\[|\z)')
    Assert-Policy $dldi.Success `
        "melonDS $($profile.Name) profile has no [DLDI] section."
    foreach ($needle in @('Enable = true', "ImagePath = `"$dldiImage`"",
        'FolderSync = false', "ReadOnly = $($profile.ReadOnly)")) {
        Assert-Policy $dldi.Groups[1].Value.Contains($needle) `
            "melonDS $($profile.Name) [DLDI] is missing: $needle"
    }
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

# Every scripted launch must stay off the owner's desktop. Without
# -WindowStyle Hidden each Start-Process throws a console or emulator window
# into the foreground and steals focus, and a measurement session launches
# dozens. CLAUDE.md has carried this rule since the gdb launches were caught
# doing it, and on 2026-07-29 an AST sweep still found 19 unhidden sites --
# every melonDS launch had been fixed by hand and every gdb launch had not.
# Enforce it here so a new harness cannot reintroduce one.
#
# -NoNewWindow is the accepted alternative: it reuses the current console
# (and is mutually exclusive with -WindowStyle, so requiring both is not
# possible). A splatted invocation is accepted when its script sets
# WindowStyle somewhere, which is how the -Visible switch is implemented.
#
# Some launches must stay VISIBLE, and the exemption is per-call-site rather
# than per-file: a `# WindowStyle: visible-by-design` comment within the eight
# lines above the call. Blanket-hiding every site is what made this necessary --
# hiding the melonDS launch in a screenshot harness leaves MainWindowHandle at
# IntPtr.Zero, so the emulator runs fine and the capture silently dies. The
# marker sits at the call so the next person to sweep this rule reads the reason
# before overriding it, instead of rediscovering it from a black PNG.
$visibleMarker = 'WindowStyle: visible-by-design'
$unhidden = @()
foreach ($scriptFile in Get-ChildItem -LiteralPath (Join-Path $Root 'scripts') `
        -Filter '*.ps1' -File -Recurse) {
    $tokens = $null
    $parseErrors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile(
        $scriptFile.FullName, [ref]$tokens, [ref]$parseErrors)
    Assert-Policy (($null -eq $parseErrors) -or ($parseErrors.Count -eq 0)) `
        ("Harness script does not parse: $($scriptFile.Name): " +
         "$(if ($parseErrors) { $parseErrors[0].Message })")
    $scriptText = $ast.Extent.Text
    $scriptLines = $scriptText -split "\r?\n"
    foreach ($command in $ast.FindAll({ param($node)
            $node -is [System.Management.Automation.Language.CommandAst] }, $true)) {
        if ($command.GetCommandName() -ne 'Start-Process') { continue }
        $commandText = $command.Extent.Text
        if ($commandText -match '-WindowStyle\s+Hidden') { continue }
        if ($commandText -match '-NoNewWindow') { continue }
        if (($commandText -match '^Start-Process\s+@') -and
            ($scriptText -match 'WindowStyle')) { continue }
        $first = [Math]::Max(0, $command.Extent.StartLineNumber - 9)
        $last = [Math]::Max(0, $command.Extent.StartLineNumber - 2)
        $preceding = if ($last -ge $first) {
            ($scriptLines[$first..$last] -join "`n")
        } else { '' }
        if ($preceding.Contains($visibleMarker)) { continue }
        $unhidden += "$($scriptFile.Name):$($command.Extent.StartLineNumber)"
    }
}
Assert-Policy ($unhidden.Count -eq 0) (
    'Start-Process without -WindowStyle Hidden steals the owner''s foreground: ' +
    ($unhidden -join ', '))

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
