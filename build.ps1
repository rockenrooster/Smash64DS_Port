[CmdletBinding()]
param(
    [string]$Rom,
    [ValidateRange(1, 256)]
    [int]$Jobs = [Math]::Max(1, [Environment]::ProcessorCount),
    [switch]$Clean,
    [string]$DecompPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$RepoRoot = $PSScriptRoot
$LogRoot = Join-Path $RepoRoot 'builds\publish-build-logs'

function Write-Step {
    param([string]$Message)
    Write-Host "`n==> $Message" -ForegroundColor Cyan
}

function Stop-Build {
    param([string]$Message)
    throw [InvalidOperationException]::new($Message)
}

function Read-PinFile {
    $path = Join-Path $RepoRoot 'DECOMP_PIN.txt'
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Stop-Build "pin file is absent: $path"
    }
    $pins = @{}
    foreach ($line in Get-Content -LiteralPath $path) {
        $trimmed = $line.Trim()
        if (-not $trimmed -or $trimmed.StartsWith('#')) {
            continue
        }
        $parts = $trimmed.Split('=', 2)
        if ($parts.Count -ne 2 -or -not $parts[0]) {
            Stop-Build "invalid pin line in ${path}: $line"
        }
        $pins[$parts[0]] = $parts[1]
    }
    $required = @(
        'BATTLESHIP_URL', 'BATTLESHIP_COMMIT', 'DECOMP_URL', 'DECOMP_COMMIT',
        'DECOMP_PATCH', 'DECOMP_PATCH_SHA256', 'LIBULTRASHIP_COMMIT',
        'TORCH_COMMIT', 'BASEROM_US_BYTES', 'BASEROM_US_SHA1',
        'OUTPUT_NAME', 'OUTPUT_BYTES', 'OUTPUT_SHA256'
    )
    foreach ($key in $required) {
        if (-not $pins.ContainsKey($key)) {
            Stop-Build "required pin $key is absent from $path"
        }
    }
    return $pins
}

function Get-Application {
    param([string]$Name, [string]$InstallHint)
    $command = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $command) {
        Stop-Build "missing prerequisite '$Name'. $InstallHint"
    }
    return $command.Source
}

function Get-Python3 {
    foreach ($name in @('python3', 'python', 'py')) {
        $commands = @(Get-Command $name -CommandType Application -All -ErrorAction SilentlyContinue)
        foreach ($command in $commands) {
            $prefix = if ($name -eq 'py') { @('-3') } else { @() }
            $output = @(& $command.Source @prefix --version 2>&1)
            if ($LASTEXITCODE -eq 0 -and ($output -join ' ') -match '^Python 3\.') {
                return [pscustomobject]@{
                    Path = $command.Source
                    Prefix = [string[]]$prefix
                    Display = (($output | Select-Object -First 1).ToString()).Trim()
                    MakeValue = if ($name -eq 'py') { 'py -3' } else { $name }
                }
            }
        }
    }
    Stop-Build (
        'missing Python 3. Install Python 3 and ensure python3, python, or the ' +
        'Windows py launcher is available on PATH.'
    )
}

function Get-VersionLine {
    param([string]$Path, [string[]]$Arguments)
    $output = @(& $Path @Arguments 2>&1)
    if ($LASTEXITCODE -ne 0) {
        Stop-Build "failed to run prerequisite $Path $($Arguments -join ' ')"
    }
    return (($output | Select-Object -First 1).ToString()).Trim()
}

function Quote-ProcessArgument {
    param([string]$Value)
    if ($Value.Contains('"')) {
        Stop-Build "unsupported quote in process argument: $Value"
    }
    if ($Value -match '\s') {
        return '"' + $Value + '"'
    }
    return $Value
}

function Invoke-LoggedProcess {
    param(
        [string]$Label,
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$WorkingDirectory,
        [hashtable]$Environment = @{}
    )
    New-Item -ItemType Directory -Force -Path $LogRoot | Out-Null
    $safeLabel = $Label -replace '[^A-Za-z0-9_.-]', '-'
    $base = Join-Path $LogRoot (
        '{0}-{1}-{2}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), $safeLabel,
        ([guid]::NewGuid().ToString('N').Substring(0, 8))
    )
    $stdout = "$base.out.log"
    $stderr = "$base.err.log"
    $stamp = "$base.exit.txt"
    $argumentLine = (($Arguments | ForEach-Object {
        Quote-ProcessArgument ([string]$_)
    }) -join ' ')
    Write-Host "    $Label (logs: $stdout)"
    $start = @{
        FilePath = $FilePath
        ArgumentList = $argumentLine
        WorkingDirectory = $WorkingDirectory
        RedirectStandardOutput = $stdout
        RedirectStandardError = $stderr
        PassThru = $true
        WindowStyle = 'Hidden'
    }
    if ($Environment.Count) {
        $start.Environment = $Environment
    }
    $process = Start-Process @start
    while (-not $process.HasExited) {
        Start-Sleep -Milliseconds 500
        $process.Refresh()
    }
    $process.WaitForExit()
    [IO.File]::WriteAllText($stamp, "$($process.ExitCode)`n")
    if ($process.ExitCode -ne 0) {
        Write-Host "---- $Label stderr tail ----" -ForegroundColor Yellow
        if (Test-Path -LiteralPath $stderr) {
            Get-Content -LiteralPath $stderr -Tail 40 | Write-Host
        }
        Write-Host "---- $Label stdout tail ----" -ForegroundColor Yellow
        if (Test-Path -LiteralPath $stdout) {
            Get-Content -LiteralPath $stdout -Tail 40 | Write-Host
        }
        Stop-Build "$Label failed with exit code $($process.ExitCode); inspect $stdout and $stderr"
    }
}

function Invoke-Python {
    param(
        [pscustomobject]$Python,
        [string]$Label,
        [string[]]$Arguments,
        [string]$WorkingDirectory = $RepoRoot
    )
    $allArguments = @($Python.Prefix) + $Arguments
    Invoke-LoggedProcess -Label $Label -FilePath $Python.Path `
        -Arguments $allArguments -WorkingDirectory $WorkingDirectory `
        -Environment @{ PYTHONDONTWRITEBYTECODE = '1' }
}

function Assert-ExactGeneratedDirectory {
    param([string]$Path, [string]$Parent, [string]$Leaf)
    $fullPath = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $fullParent = [IO.Path]::GetFullPath($Parent).TrimEnd('\', '/')
    if ([IO.Path]::GetDirectoryName($fullPath) -ine $fullParent -or
        [IO.Path]::GetFileName($fullPath) -ine $Leaf) {
        Stop-Build "refusing unsafe generated-directory operation: $fullPath"
    }
}

function Remove-GeneratedChild {
    param([string]$Path, [string]$Root)
    $fullPath = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    $fullRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    if (-not $fullPath.StartsWith($fullRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        Stop-Build "refusing to remove path outside generated root: $fullPath"
    }
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

function Assert-GitHead {
    param([string]$Git, [string]$Path, [string]$Expected, [string]$Label)
    $output = @(& $Git -C $Path rev-parse HEAD 2>$null)
    $actual = if ($output.Count) { $output[0].ToString().Trim() } else { '<unavailable>' }
    if ($LASTEXITCODE -ne 0 -or $actual -ine $Expected) {
        Stop-Build "$Label pin mismatch: expected $Expected, got $actual"
    }
    Write-Host "    $Label commit $actual"
}

function Test-GitCheckout {
    param([string]$Git, [string]$Path)
    $output = @(& $Git -C $Path rev-parse --show-toplevel 2>$null)
    if ($LASTEXITCODE -ne 0 -or -not $output.Count) {
        return $false
    }
    return [IO.Path]::GetFullPath($output[0].ToString()).TrimEnd('\', '/') -ieq
        [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
}

function Get-PinnedFileRows {
    param([hashtable]$Pins, [string]$Prefix)
    foreach ($key in @($Pins.Keys | Where-Object {
        $_ -like "$Prefix*"
    } | Sort-Object)) {
        $parts = $Pins[$key].Split('|', 2)
        if ($parts.Count -ne 2) {
            Stop-Build "invalid pinned-file row $key"
        }
        [pscustomobject]@{ Relative = $parts[0]; Hash = $parts[1] }
    }
}

function Assert-PinnedFiles {
    param([hashtable]$Pins, [string]$Prefix, [string]$Root, [string]$Label)
    foreach ($row in @(Get-PinnedFileRows $Pins $Prefix)) {
        $path = Join-Path $Root $row.Relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            Stop-Build "required $Label file is absent: $path"
        }
        $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
        if ($actual -ine $row.Hash) {
            Stop-Build "$Label hash mismatch for $($row.Relative): expected $($row.Hash), got $actual"
        }
    }
}

function Stage-Rom {
    param([string]$Source, [string]$Destination)
    $sourceFull = [IO.Path]::GetFullPath($Source)
    $destinationFull = [IO.Path]::GetFullPath($Destination)
    if ($sourceFull -ieq $destinationFull) {
        return
    }
    if (Test-Path -LiteralPath $destinationFull -PathType Leaf) {
        $existing = (Get-FileHash -Algorithm SHA1 -LiteralPath $destinationFull).Hash
        $incoming = (Get-FileHash -Algorithm SHA1 -LiteralPath $sourceFull).Hash
        if ($existing -ieq $incoming) {
            return
        }
    }
    Copy-Item -LiteralPath $sourceFull -Destination $destinationFull -Force
}

function Main {
    $pins = Read-PinFile

    Write-Step '1/7 Checking prerequisites'
    if (-not $env:DEVKITPRO -or -not (Test-Path -LiteralPath $env:DEVKITPRO -PathType Container)) {
        Stop-Build (
            'DEVKITPRO is absent or invalid. Install devkitPro with the nds-dev group ' +
            '(pacman -S nds-dev), then set DEVKITPRO.'
        )
    }
    if (-not $env:DEVKITARM -or -not (Test-Path -LiteralPath $env:DEVKITARM -PathType Container)) {
        Stop-Build (
            'DEVKITARM is absent or invalid. Install devkitPro with the nds-dev group ' +
            '(pacman -S nds-dev), then set DEVKITARM.'
        )
    }
    $gcc = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-gcc.exe'
    if (-not (Test-Path -LiteralPath $gcc -PathType Leaf)) {
        Stop-Build "arm-none-eabi-gcc is absent at $gcc; reinstall devkitARM via devkitPro pacman."
    }
    $git = Get-Application 'git' 'Install Git for Windows or the devkitPro MSYS2 git package.'
    $make = Get-Application 'make' 'Install the devkitPro MSYS2 make package.'
    $python = Get-Python3
    $cmakeCommands = @(Get-Command cmake -CommandType Application -All -ErrorAction SilentlyContinue)
    if (-not $cmakeCommands) {
        Stop-Build 'missing prerequisite cmake. Install CMake from https://cmake.org/download/.'
    }
    $cmakeCommand = $cmakeCommands | Where-Object {
        $_.Source -notmatch '[\\/](msys2|mingw)[\\/]'
    } | Select-Object -First 1
    if (-not $cmakeCommand) {
        $cmakeCommand = $cmakeCommands | Select-Object -First 1
    }
    $cmake = $cmakeCommand.Source
    Write-Host "    DEVKITPRO=$env:DEVKITPRO"
    Write-Host "    DEVKITARM=$env:DEVKITARM"
    Write-Host "    $(Get-VersionLine $gcc @('--version'))"
    Write-Host "    $(Get-VersionLine $make @('--version'))"
    Write-Host "    $($python.Display) ($($python.Path))"
    Write-Host "    $(Get-VersionLine $git @('--version'))"
    Write-Host "    $(Get-VersionLine $cmake @('--version'))"

    $defaultBattleRoot = Join-Path $RepoRoot 'decomp\BattleShip-main'
    $battleCandidate = if ($DecompPath) {
        [IO.Path]::GetFullPath($DecompPath)
    } else {
        $defaultBattleRoot
    }
    $torchReady = $false
    $battleBuild = Join-Path $battleCandidate 'build\us'
    if (Test-Path -LiteralPath $battleBuild -PathType Container) {
        $torchReady = $null -ne (Get-ChildItem -LiteralPath $battleBuild -Filter torch.exe `
            -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1)
    }
    if (-not $torchReady) {
        $vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
        $visualStudio = $null
        if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
            $visualStudio = (& $vswhere -latest -products * `
                -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
                -property installationPath 2>$null | Select-Object -First 1)
        }
        if (-not $visualStudio) {
            Stop-Build (
                'Torch must be built, but MSVC C++ Build Tools are absent. Install Visual ' +
                'Studio Build Tools with Desktop development with C++ and a Windows SDK.'
            )
        }
        Write-Host "    MSVC=$visualStudio"
    } else {
        Write-Host '    cached torch build found; MSVC rebuild is not required'
    }

    Write-Step '2/7 Validating the NTSC-U v1.0 ROM'
    if (-not $Rom) {
        Stop-Build 'missing -Rom. Usage: .\build.ps1 -Rom C:\path\to\baserom.us.z64 [-Jobs N] [-Clean] [-DecompPath dir]'
    }
    if (-not (Test-Path -LiteralPath $Rom -PathType Leaf)) {
        Stop-Build "ROM is absent: $Rom"
    }
    $romPath = (Resolve-Path -LiteralPath $Rom).Path
    $romLength = (Get-Item -LiteralPath $romPath).Length
    if ($romLength -ne [int64]$pins.BASEROM_US_BYTES) {
        Stop-Build "wrong ROM size: expected $($pins.BASEROM_US_BYTES) bytes for NTSC-U v1.0 NALE, got $romLength"
    }
    $header = [IO.File]::ReadAllBytes($romPath)[0..3]
    $magic = ($header | ForEach-Object { $_.ToString('X2') }) -join ' '
    if ($magic -eq '37 80 40 12') {
        Stop-Build 'byteswapped .v64 ROM detected; provide the big-endian NTSC-U v1.0 baserom.us.z64 dump.'
    }
    if ($magic -eq '40 12 37 80') {
        Stop-Build 'little-endian .n64 ROM detected; provide the big-endian NTSC-U v1.0 baserom.us.z64 dump.'
    }
    $romSha1 = (Get-FileHash -Algorithm SHA1 -LiteralPath $romPath).Hash
    if ($romSha1 -ine $pins.BASEROM_US_SHA1) {
        Stop-Build "wrong ROM hash: expected NTSC-U v1.0 NALE SHA-1 $($pins.BASEROM_US_SHA1), got $romSha1"
    }
    Write-Host "    ROM PASS: $romLength bytes, SHA-1 $romSha1"

    Write-Step '3/7 Acquiring and validating pinned BattleShip source'
    $battleRoot = $battleCandidate
    if ($DecompPath) {
        if (-not (Test-Path -LiteralPath $battleRoot -PathType Container)) {
            Stop-Build "-DecompPath does not exist: $battleRoot"
        }
        Write-Host "    reusing $battleRoot"
    } elseif (-not (Test-Path -LiteralPath $battleRoot -PathType Container)) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $battleRoot) | Out-Null
        Invoke-LoggedProcess 'clone-BattleShip' $git `
            @('clone', '--no-recurse-submodules', $pins.BATTLESHIP_URL, $battleRoot) $RepoRoot
        Invoke-LoggedProcess 'checkout-BattleShip-pin' $git `
            @('-C', $battleRoot, 'checkout', '--detach', $pins.BATTLESHIP_COMMIT) $RepoRoot
        Invoke-LoggedProcess 'init-BattleShip-submodules' $git `
            @('-C', $battleRoot, 'submodule', 'update', '--init', '--recursive',
              'libultraship', 'torch') $RepoRoot
        $decompRoot = Join-Path $battleRoot 'decomp'
        if (Test-Path -LiteralPath $decompRoot) {
            Assert-ExactGeneratedDirectory $decompRoot $battleRoot 'decomp'
            if (@(Get-ChildItem -LiteralPath $decompRoot -Force).Count -ne 0) {
                Stop-Build "unexpected non-empty decomp path after BattleShip clone: $decompRoot"
            }
            Remove-Item -LiteralPath $decompRoot -Force
        }
        Invoke-LoggedProcess 'clone-decomp-base' $git `
            @('clone', '--no-recurse-submodules', $pins.DECOMP_URL, $decompRoot) $RepoRoot
        Invoke-LoggedProcess 'checkout-decomp-pin' $git `
            @('-C', $decompRoot, 'checkout', '--detach', $pins.DECOMP_COMMIT) $RepoRoot
        Invoke-LoggedProcess 'init-decomp-submodules' $git `
            @('-C', $decompRoot, 'submodule', 'update', '--init', '--recursive') $RepoRoot
        $patchPath = Join-Path $RepoRoot $pins.DECOMP_PATCH
        $patchHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $patchPath).Hash
        if ($patchHash -ine $pins.DECOMP_PATCH_SHA256) {
            Stop-Build "decomp patch hash mismatch: expected $($pins.DECOMP_PATCH_SHA256), got $patchHash"
        }
        Invoke-LoggedProcess 'check-decomp-patch' $git `
            @('-C', $decompRoot, 'apply', '--check', $patchPath) $RepoRoot
        Invoke-LoggedProcess 'apply-decomp-patch' $git `
            @('-C', $decompRoot, 'apply', $patchPath) $RepoRoot
    } else {
        Write-Host "    reusing existing $battleRoot"
    }
    foreach ($requiredPath in @('CMakeLists.txt', 'config.yml', 'decomp', 'libultraship', 'torch')) {
        if (-not (Test-Path -LiteralPath (Join-Path $battleRoot $requiredPath))) {
            Stop-Build "BattleShip checkout is incomplete: missing $requiredPath under $battleRoot"
        }
    }
    $decompRoot = Join-Path $battleRoot 'decomp'
    if (Test-GitCheckout $git $battleRoot) {
        Assert-GitHead $git $battleRoot $pins.BATTLESHIP_COMMIT 'BattleShip'
    }
    if (Test-GitCheckout $git $decompRoot) {
        Assert-GitHead $git $decompRoot $pins.DECOMP_COMMIT 'decomp base'
    }
    foreach ($row in @(
        @('libultraship', $pins.LIBULTRASHIP_COMMIT),
        @('torch', $pins.TORCH_COMMIT)
    )) {
        $submoduleRoot = Join-Path $battleRoot $row[0]
        if (Test-GitCheckout $git $submoduleRoot) {
            Assert-GitHead $git $submoduleRoot $row[1] $row[0]
        }
    }
    Assert-PinnedFiles $pins 'DECOMP_PATCHED_SHA256_' $decompRoot 'patched decomp'
    Assert-PinnedFiles $pins 'BATTLESHIP_PROTECTED_SHA256_' $battleRoot 'protected BattleShip source'
    Write-Host '    five reviewed DS source patches verified'
    Stage-Rom $romPath (Join-Path $battleRoot 'baserom.us.z64')
    Stage-Rom $romPath (Join-Path $decompRoot 'baserom.us.z64')

    $temporaryDecompJunction = $null
    $defaultBattleFull = [IO.Path]::GetFullPath($defaultBattleRoot).TrimEnd('\', '/')
    $battleFull = [IO.Path]::GetFullPath($battleRoot).TrimEnd('\', '/')
    if ($defaultBattleFull -ine $battleFull) {
        if (Test-Path -LiteralPath $defaultBattleFull) {
            Stop-Build (
                "cannot expose -DecompPath at $defaultBattleFull because that path already exists"
            )
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $defaultBattleFull) |
            Out-Null
        New-Item -ItemType Junction -Path $defaultBattleFull -Target $battleFull | Out-Null
        $temporaryDecompJunction = $defaultBattleFull
        Write-Host "    temporary decomp junction -> $battleFull"
    }

    try {
    $moduleCheck = 'import yaml,tqdm,intervaltree,colorama,spimdisasm,rabbitizer,pygfxd,n64img,crunch64'
    $moduleOutput = @(& $python.Path @($python.Prefix) -c $moduleCheck 2>&1)
    if ($LASTEXITCODE -ne 0) {
        Stop-Build (
            "decomp Python modules are missing. Run '$($python.Path) -m pip install -r " +
            "$decompRoot\tools\splat\requirements.txt', then retry. Details: " +
            ($moduleOutput -join ' ')
        )
    }

    Write-Step '4/7 Extracting O2R and relocData from the validated ROM'
    $cmakeBuild = Join-Path $battleRoot 'build\us'
    $hostToolEnvironment = @{
        PATH = (Split-Path -Parent $cmake) + [IO.Path]::PathSeparator + $env:PATH
        PYTHONDONTWRITEBYTECODE = '1'
        VCPKG_DISABLE_METRICS = '1'
    }
    $sourceBackup = Join-Path $LogRoot ('source-backup-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $sourceBackup | Out-Null
    $protectedRows = @(Get-PinnedFileRows $pins 'BATTLESHIP_PROTECTED_SHA256_')
    foreach ($row in $protectedRows) {
        $backupPath = Join-Path $sourceBackup $row.Relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $backupPath) | Out-Null
        Copy-Item -LiteralPath (Join-Path $battleRoot $row.Relative) -Destination $backupPath
    }
    try {
        Invoke-LoggedProcess 'configure-BattleShip-extractor' $cmake `
            @('-S', $battleRoot, '-B', $cmakeBuild, '-A', 'x64',
              '-DSSB64_VERSION=us', "-DPython3_EXECUTABLE=$($python.Path)") $battleRoot `
            $hostToolEnvironment
        Invoke-LoggedProcess 'build-BattleShip-ExtractAssets' $cmake `
            @('--build', $cmakeBuild, '--config', 'Release', '--target', 'ExtractAssets',
              '--parallel', [string]$Jobs) $battleRoot $hostToolEnvironment
    } finally {
        foreach ($row in $protectedRows) {
            Copy-Item -LiteralPath (Join-Path $sourceBackup $row.Relative) `
                -Destination (Join-Path $battleRoot $row.Relative) -Force
        }
    }
    Assert-PinnedFiles $pins 'BATTLESHIP_PROTECTED_SHA256_' $battleRoot 'restored BattleShip source'
    $archive = Join-Path $cmakeBuild 'extracted\BattleShip.o2r'
    if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
        Stop-Build "ExtractAssets did not produce $archive"
    }
    $o2rOutput = Join-Path $battleRoot 'BattleShip_o2r'
    $o2rTemp = Join-Path $battleRoot ('.smash64ds-o2r-' + [guid]::NewGuid().ToString('N'))
    [IO.Compression.ZipFile]::ExtractToDirectory($archive, $o2rTemp)
    $o2rFiles = @(Get-ChildItem -LiteralPath $o2rTemp -Recurse -File).Count
    if ($o2rFiles -ne 2159) {
        Stop-Build "BattleShip.o2r materialized $o2rFiles files; expected 2159 (left at $o2rTemp)"
    }
    Assert-ExactGeneratedDirectory $o2rOutput $battleRoot 'BattleShip_o2r'
    if (Test-Path -LiteralPath $o2rOutput) {
        Remove-Item -LiteralPath $o2rOutput -Recurse -Force
    }
    Move-Item -LiteralPath $o2rTemp -Destination $o2rOutput
    Write-Host "    O2R PASS: $o2rFiles files"

    foreach ($relative in @('asm\us', '.splat\us', 'assets\us')) {
        Remove-GeneratedChild (Join-Path $decompRoot $relative) $decompRoot
    }
    Invoke-Python $python 'split-decomp-assets' `
        @((Join-Path $decompRoot 'tools\splat\split.py'),
          (Join-Path $decompRoot 'smashbrothers.us.yaml')) $decompRoot
    Invoke-Python $python 'extract-decomp-relocData' `
        @((Join-Path $RepoRoot 'scripts\extract-battleship-relocdata.py'),
          '--decomp-root', $decompRoot, '--version', 'us') $RepoRoot
    $relocData = Join-Path $decompRoot 'assets\us\relocData'
    $relocFiles = @(Get-ChildItem -LiteralPath $relocData -Recurse -File).Count
    if ($relocFiles -ne 3130) {
        Stop-Build "relocData materialized $relocFiles files; expected 3130"
    }
    Write-Host "    relocData PASS: $relocFiles files (inactive in the shipping target)"

    Write-Step '5/7 Regenerating port-owned derived assets'
    foreach ($bgm in @(
        @(0, 'assets\audio\bgm_pupupu_pcm16.raw'),
        @(12, 'assets\audio\bgm_win_mario_pcm16.raw'),
        @(16, 'assets\audio\bgm_win_fox_pcm16.raw'),
        @(22, 'assets\audio\bgm_results_pcm16.raw')
    )) {
        Invoke-Python $python "render-bgm-$($bgm[0])" `
            @((Join-Path $RepoRoot 'scripts\render-audio-bgm-pupupu.py'),
              '--repo', $RepoRoot, '--sequence-index', [string]$bgm[0],
              '--output', $bgm[1]) $RepoRoot
    }
    Invoke-Python $python 'render-fgm-phase-pack' `
        @((Join-Path $RepoRoot 'scripts\render-audio-fgm-phase-pack.py'),
          '--repo-root', $RepoRoot) $RepoRoot
    Invoke-Python $python 'generate-static-textures' `
        @((Join-Path $RepoRoot 'scripts\generate_battle_playable_static_textures.py'),
          '--repo-root', $RepoRoot) $RepoRoot
    Invoke-Python $python 'generate-native-stage' `
        @((Join-Path $RepoRoot 'scripts\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot) $RepoRoot
    Invoke-Python $python 'generate-native-fighters' `
        @((Join-Path $RepoRoot 'scripts\generate_nds_native_owners.py'),
          '--source-root', $RepoRoot) $RepoRoot
    $generatedOutputs = @(
        'assets\audio\bgm_pupupu_pcm16.raw', 'assets\audio\bgm_pupupu_pcm16.json',
        'assets\audio\bgm_win_mario_pcm16.raw', 'assets\audio\bgm_win_mario_pcm16.json',
        'assets\audio\bgm_win_fox_pcm16.raw', 'assets\audio\bgm_win_fox_pcm16.json',
        'assets\audio\bgm_results_pcm16.raw', 'assets\audio\bgm_results_pcm16.json',
        'assets\audio\fgm_phase_pack_ima.bin', 'assets\audio\fgm_phase_pack_ima.json',
        'assets\renderer\battle_playable_static_textures.rgb5a1.bin',
        'src\nds\generated\battle_playable_static_textures.generated.inc',
        'src\nds\nds_native_stage_owner.generated.inc',
        'src\nds\nds_native_fighter_owner.generated.inc'
    )
    foreach ($relative in $generatedOutputs) {
        if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot $relative) -PathType Leaf)) {
            Stop-Build "generator did not produce $relative"
        }
    }
    Write-Host '    port asset regeneration PASS'

    Write-Step '6/7 Building the Nintendo DS shipping target'
    $processEnvironment = @{ PYTHONDONTWRITEBYTECODE = '1' }
    if ($Clean) {
        Invoke-LoggedProcess 'clean-port-target' $make `
            @('TARGET=smash64ds-battle-playable-hwtri', 'clean') $RepoRoot $processEnvironment
    }
    Invoke-LoggedProcess 'build-port-target' $make `
        @('TARGET=smash64ds-battle-playable-hwtri', "-j$Jobs") $RepoRoot $processEnvironment

    Write-Step '7/7 Reporting ROM identity'
    $output = Join-Path $RepoRoot $pins.OUTPUT_NAME
    if (-not (Test-Path -LiteralPath $output -PathType Leaf)) {
        Stop-Build "port build succeeded without producing $output"
    }
    $outputLength = (Get-Item -LiteralPath $output).Length
    $outputHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $output).Hash
    Write-Host "    Output:  $output"
    Write-Host "    Bytes:   $outputLength"
    Write-Host "    SHA-256: $outputHash"
    if ($outputLength -eq [int64]$pins.OUTPUT_BYTES -and
        $outputHash -ieq $pins.OUTPUT_SHA256) {
        Write-Host '    REFERENCE IDENTITY PASS' -ForegroundColor Green
    } else {
        Write-Warning (
            "build succeeded but differs from the audited reference ($($pins.OUTPUT_BYTES) bytes, " +
            "$($pins.OUTPUT_SHA256)). Compare the printed toolchain versions before treating " +
            'this as a source regression.'
        )
    }
    } finally {
        if ($temporaryDecompJunction -and
            (Test-Path -LiteralPath $temporaryDecompJunction)) {
            $junction = Get-Item -LiteralPath $temporaryDecompJunction -Force
            if ($junction.LinkType -ne 'Junction') {
                Stop-Build (
                    "refusing to remove non-junction path: $temporaryDecompJunction"
                )
            }
            Remove-Item -LiteralPath $temporaryDecompJunction -Force
        }
    }
}

try {
    Main
} catch {
    [Console]::Error.WriteLine("BUILD FAILED: $($_.Exception.Message)")
    [Console]::Error.WriteLine("Logs, when available: $LogRoot")
    exit 1
}
