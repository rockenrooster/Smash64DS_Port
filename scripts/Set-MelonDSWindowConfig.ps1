[CmdletBinding()]
param(
    [string[]]$Root = @((Resolve-Path (Join-Path $PSScriptRoot '..')).Path),
    [switch]$AllWorktrees,
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

$rootInputs = @($Root)
if ($AllWorktrees) {
    $discoveryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
    $worktreeLines = @(& git -C $discoveryRoot worktree list --porcelain)
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not enumerate registered Git worktrees.'
    }
    $rootInputs += @($worktreeLines | Where-Object {
        $_.StartsWith('worktree ')
    } | ForEach-Object {
        $path = $_.Substring('worktree '.Length)
        if ($path -match '^/([A-Za-z])/(.*)$') {
            $path = "$($Matches[1]):/$($Matches[2])"
        }
        $path
    })
}
$rootInputs = @($rootInputs | ForEach-Object {
    (Resolve-Path -LiteralPath $_).Path
} | Sort-Object -Unique)

$rows = @()
$processedRoots = 0
foreach ($rootInput in $rootInputs) {
    $rootPath = (Resolve-Path -LiteralPath $rootInput).Path
    $emulatorRoot = Join-Path $rootPath 'emulators'
    if (-not (Test-Path -LiteralPath $emulatorRoot -PathType Container)) {
        if ($AllWorktrees) { continue }
        throw "Repo emulator directory not found: $emulatorRoot"
    }
    $prefix = $emulatorRoot.TrimEnd('\', '/') + '\'
    $allTomls = @(Get-ChildItem -LiteralPath $emulatorRoot -Recurse `
        -Filter '*.toml' -File | Sort-Object FullName)
    $configs = @(Get-ChildItem -LiteralPath $emulatorRoot -Recurse `
        -Filter 'melonDS.toml' -File | Sort-Object FullName)
    if ($configs.Count -eq 0) {
        if ($AllWorktrees -and $allTomls.Count -eq 0) { continue }
        throw "No melonDS.toml files found under: $emulatorRoot"
    }
    if ($allTomls.Count -ne $configs.Count) {
        $unexpected = @($allTomls | Where-Object Name -INe 'melonDS.toml' |
            Select-Object -ExpandProperty FullName)
        throw "Unclassified emulator TOML(s): $($unexpected -join ', ')"
    }
    $processedRoots++

    foreach ($config in $configs) {
        $directory = Split-Path -Parent $config.FullName
        $executable = Join-Path $directory 'melonDS.exe'
        $null = Resolve-MelonDSRepoExecutablePath `
            -Root $rootPath -MelonDS $executable
        if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
            throw "Config has no repo-local companion executable: $($config.FullName)"
        }

        $relative = $config.FullName.Substring($prefix.Length)
        $before = Get-Content -LiteralPath $config.FullName -Raw
        $kind = ''
        $arm9Port = 0
        $arm7Port = 0
        if ($relative -ieq 'melonds\melonDS.toml') {
            $kind = 'manual'
            $arm9Port = 3333
            $arm7Port = 3334
            $after = Set-MelonDSManualProfile -Text $before
        } elseif ($relative -match '^melonds-runners\\slot([0-9]+)\\melonDS\.toml$') {
            $slot = [int]$Matches[1]
            $kind = "runner-slot$slot"
            $arm9Port = Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM9
            $arm7Port = Get-MelonDSRunnerPort -RunnerSlot $slot -Cpu ARM7
            $after = Set-MelonDSAutomationProfile -Text $before `
                -GdbPort $arm9Port -Arm7Port $arm7Port -MuteAudio
        } else {
            throw "Unclassified melonDS.toml under repo emulators: $relative"
        }

        $changed = $after -cne $before
        if ($Check -and $changed) {
            throw "melonDS config is outside the canonical profile: $($config.FullName)"
        }
        if ($changed) {
            Set-Content -LiteralPath $config.FullName -Value $after -NoNewline
        }
        $finalText = if ($changed -and -not $Check) { $after } else { $before }
        if ((Set-MelonDSWindowProfile -Text $finalText) -cne $finalText) {
            throw "melonDS window profile was not idempotent: $($config.FullName)"
        }
        $rows += [PSCustomObject]@{
            Root = $rootPath
            Config = $relative
            Kind = $kind
            Window = "$($script:MelonDSCanonicalWindowWidth)x$($script:MelonDSCanonicalWindowHeight)"
            ARM9 = $arm9Port
            ARM7 = $arm7Port
            Changed = $changed
        }
    }
}

$rows | Format-Table Root, Config, Kind, Window, ARM9, ARM7, Changed -AutoSize
Write-Output "melonDS config profile PASS: roots=$processedRoots configs=$($rows.Count) repo_local=1."
