[CmdletBinding()]
param(
    [string]$Rom,
    # 0 means "let the Makefile decide", which is the correct default: the
    # Makefile sets `MAKEFLAGS += -j$(NDS_JOBS)` from nproc, and an explicit -j on
    # the command line OVERRIDES that. Thirty-one scripts/*.ps1 harnesses were
    # fixed for this in 2026-07 and build.ps1 -- the canonical clean-checkout
    # build -- was missed. Pass a nonzero value only to deliberately override.
    [ValidateRange(0, 256)]
    [int]$Jobs = 0,
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
        # `-Jobs 0` means "let the build tool decide", which is why the make path
        # below guards with `if ($Jobs -gt 0)`. This call did not, so it passed
        # `cmake --build --parallel 0` -- which cmake rejects with a usage dump and
        # exit 1. Since 0 is the DEFAULT, `.\build.ps1 -Rom <rom>` could not
        # complete at all without also passing -Jobs; the failure looked like a
        # broken asset extractor rather than a bad argument. Omit the flag entirely
        # so cmake applies its own default, matching the make path's semantics.
        $extractArguments = @('--build', $cmakeBuild, '--config', 'Release',
                              '--target', 'ExtractAssets')
        if ($Jobs -gt 0) {
            $extractArguments += @('--parallel', [string]$Jobs)
        }
        Invoke-LoggedProcess 'build-BattleShip-ExtractAssets' $cmake `
            $extractArguments $battleRoot $hostToolEnvironment
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
    # 2026-09-05: relocData is a live NitroFS input now -- the Makefile stages
    # $(BATTLESHIP_RELOCDATA) files under nitrofs/relocdata/us -- so the old
    # "(inactive in the shipping target)" note was stale.
    Write-Host "    relocData PASS: $relocFiles files (staged into NitroFS relocdata/us by the Makefile)"

    Write-Step '5/7 Regenerating port-owned derived assets'
    # Every BGM track the runtime table names (2026-09-05: 47 of 47 gmMusicID
    # entries) is rendered from its S1_music_sbk sequence into the gitignored
    # assets\audio tree, once: a track whose .bin already exists is kept, since
    # the pins in include\nds\nds_audio_bgm.h are its render metadata verbatim
    # and a re-render is a deliberate re-pin, not a build step. The (sequence,
    # file) pairs come from the runtime sources themselves so this list cannot
    # drift from the table: every row of the track table in nds_audio_bgm.c
    # opens `{ nSYAudioBGM<Name>, NDS_AUDIO_BGM_PATH_<C>, ...`; the gmMusicID
    # member IS the sequence index (include\gm\gmsound.h pins every member
    # explicitly), and NDS_AUDIO_BGM_PATH_<C> "nitro:/audio/bgm_<stem>_ima.bin"
    # names the file. Where the header also carries NDS_AUDIO_BGM_TRACK_<C>
    # <n>u it must agree with the enum, or two tracked sources have drifted.
    # (2026-09-05, second pass: the morning's version keyed on the TRACK_
    # define alone and would have stopped on CASTLE -- the seven stage rows
    # behind NDS_P2_STAGE_<X> pin their render metadata but never had a TRACK_
    # index, and the row's enum member is the only index the runtime reads.
    # Before that the step named render-audio-bgm-pupupu.py and four
    # _pcm16.raw outputs; both were retired with the Yoshi's Island landing.)
    $bgmHeader = Get-Content -LiteralPath (Join-Path $RepoRoot 'include\nds\nds_audio_bgm.h') -Raw
    $bgmRuntime = Get-Content -LiteralPath (Join-Path $RepoRoot 'src\nds\nds_audio_bgm.c') -Raw
    $bgmEnum = Get-Content -LiteralPath (Join-Path $RepoRoot 'include\gm\gmsound.h') -Raw
    $bgmMusicId = @{}
    foreach ($m in [regex]::Matches($bgmEnum, '(nSYAudioBGM\w+)\s*=\s*(\d+)')) {
        $bgmMusicId[$m.Groups[1].Value] = [int]$m.Groups[2].Value
    }
    $bgmPinned = @{}
    foreach ($m in [regex]::Matches($bgmHeader, '#define NDS_AUDIO_BGM_TRACK_(\w+) (\d+)u')) {
        $bgmPinned[$m.Groups[1].Value] = [int]$m.Groups[2].Value
    }
    $bgmFile = @{}
    foreach ($m in [regex]::Matches($bgmRuntime, '#define NDS_AUDIO_BGM_PATH_(\w+) "nitro:/audio/(bgm_\w+_ima\.bin)"')) {
        $bgmFile[$m.Groups[1].Value] = $m.Groups[2].Value
    }
    $bgmRows = [regex]::Matches($bgmRuntime, '\{\s*(nSYAudioBGM\w+),\s*NDS_AUDIO_BGM_PATH_(\w+),')
    if ($bgmRows.Count -eq 0) {
        Stop-Build 'nds_audio_bgm.c has no track row of the shape { nSYAudioBGM<Name>, NDS_AUDIO_BGM_PATH_<C>, ...'
    }
    $bgmOutputs = @()
    foreach ($m in $bgmRows) {
        $musicId = $m.Groups[1].Value
        $trackName = $m.Groups[2].Value
        if (-not $bgmMusicId.ContainsKey($musicId)) {
            Stop-Build "nds_audio_bgm.c row $trackName names $musicId but gmMusicID in include\gm\gmsound.h has no such member"
        }
        if (-not $bgmFile.ContainsKey($trackName)) {
            Stop-Build "nds_audio_bgm.c row $trackName has no NDS_AUDIO_BGM_PATH_$trackName define"
        }
        $sequence = $bgmMusicId[$musicId]
        if ($bgmPinned.ContainsKey($trackName) -and $bgmPinned[$trackName] -ne $sequence) {
            Stop-Build "nds_audio_bgm.h pins NDS_AUDIO_BGM_TRACK_$trackName $($bgmPinned[$trackName])u but gmsound.h's $musicId is $sequence"
        }
        $bgmRelative = Join-Path 'assets\audio' $bgmFile[$trackName]
        $bgmOutputs += $bgmRelative
        $bgmOutputs += [IO.Path]::ChangeExtension($bgmRelative, '.json')
        if (Test-Path -LiteralPath (Join-Path $RepoRoot $bgmRelative) -PathType Leaf) {
            continue
        }
        Invoke-Python $python "render-bgm-$sequence" `
            @((Join-Path $RepoRoot 'scripts\sfx\bgm\render-audio-bgm.py'),
              '--repo', $RepoRoot, '--sequence-index', [string]$sequence,
              '--output', $bgmRelative) $RepoRoot
    }
    Write-Host "    BGM: $($bgmRows.Count) tracks in the runtime table"
    Invoke-Python $python 'render-fgm-phase-pack' `
        @((Join-Path $RepoRoot 'scripts\sfx\render-audio-fgm-phase-pack.py'),
          '--repo-root', $RepoRoot) $RepoRoot
    Invoke-Python $python 'generate-static-textures' `
        @((Join-Path $RepoRoot 'scripts\generate_battle_playable_static_textures.py'),
          '--repo-root', $RepoRoot) $RepoRoot
    Invoke-Python $python 'generate-native-stage' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot) $RepoRoot
    # P2-4n1 step 5. The second native stage packet. Its C symbols and macros
    # are namespaced, so it links beside Dream Land's in the same translation
    # unit; it is compiled only when NDS_P2_STAGE_YOSTER=1, but it is always
    # generated so the file cannot go stale behind the flag.
    Invoke-Python $python 'generate-native-stage-yoster' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'yoster') $RepoRoot
    Invoke-Python $python 'generate-native-stage-jungle' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'jungle') $RepoRoot
    Invoke-Python $python 'generate-native-stage-castle' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'castle') $RepoRoot
    Invoke-Python $python 'generate-native-stage-sector' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'sector') $RepoRoot
    Invoke-Python $python 'generate-native-stage-hyrule' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'hyrule') $RepoRoot
    Invoke-Python $python 'generate-native-stage-inishie' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'inishie') $RepoRoot
    Invoke-Python $python 'generate-native-stage-zebes' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'zebes') $RepoRoot
    Invoke-Python $python 'generate-native-stage-yamabuki' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'yamabuki') $RepoRoot
    Invoke-Python $python 'generate-native-stage-pupupusmall' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'pupupusmall') $RepoRoot
    Invoke-Python $python 'generate-native-stage-yostersmall' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'yostersmall') $RepoRoot
    Invoke-Python $python 'generate-native-stage-metal' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'metal') $RepoRoot
    Invoke-Python $python 'generate-native-stage-zako' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'zako') $RepoRoot
    Invoke-Python $python 'generate-native-stage-last' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'last') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus3' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus3') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_mario' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_mario') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_fox' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_fox') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_donkey' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_donkey') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_samus' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_samus') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_luigi' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_luigi') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_link' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_link') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_yoshi' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_yoshi') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_captain' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_captain') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_kirby' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_kirby') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_pikachu' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_pikachu') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_purin' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_purin') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus1_ness' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus1_ness') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_mario' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_mario') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_fox' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_fox') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_donkey' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_donkey') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_samus' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_samus') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_luigi' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_luigi') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_link' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_link') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_yoshi' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_yoshi') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_captain' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_captain') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_kirby' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_kirby') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_pikachu' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_pikachu') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_purin' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_purin') $RepoRoot
    Invoke-Python $python 'generate-native-stage-bonus2_ness' `
        @((Join-Path $RepoRoot 'scripts\stages\generate_nds_native_stage.py'),
          '--repo-root', $RepoRoot, '--stage', 'bonus2_ness') $RepoRoot
    Invoke-Python $python 'generate-native-fighters' `
        @((Join-Path $RepoRoot 'scripts\fighters\generate_nds_native_owners.py'),
          '--source-root', $RepoRoot) $RepoRoot
    # THE native fighter owner oracle. Task 56 shipped broken on 2026-08-10 and
    # the owner saw missing geometry on both fighters; P2-3r17 (2026-08-25) was
    # opened by a second missing-geometry report and found the checker of the
    # day could only build the frozen Mario/Fox HIGH context, so Luigi, Donkey
    # and every LOW program were outside what any gate could express. This one
    # runs six closures -- source, vertex, matrix routing, facing, winding and
    # primitive -- for every landed owner in both detail levels, and fails
    # closed. It runs here because it checks the file the step above just wrote,
    # and because nothing else would ever run it.
    Invoke-Python $python 'check-native-owner-geometry-closure' `
        @((Join-Path $RepoRoot 'scripts\fighters\check_native_owner_geometry_closure.py')) `
        $RepoRoot
    $generatedOutputs = @(
        'assets\audio\fgm_phase_pack_ima.bin', 'assets\audio\fgm_phase_pack_ima.json',
        'assets\renderer\battle_playable_static_textures.rgb5a1.bin',
        'src\nds\generated\battle_playable_static_textures.generated.inc',
        'src\nds\nds_native_stage_owner.generated.inc',
        'src\nds\nds_native_stage_yoster.generated.inc',
        'src\nds\nds_native_stage_jungle.generated.inc',
        'src\nds\nds_native_stage_castle.generated.inc',
        'src\nds\nds_native_stage_sector.generated.inc',
        'src\nds\nds_native_stage_hyrule.generated.inc',
        'src\nds\nds_native_stage_inishie.generated.inc',
        'src\nds\nds_native_stage_zebes.generated.inc',
        'src\nds\nds_native_stage_yamabuki.generated.inc',
        'src\nds\nds_native_stage_pupupusmall.generated.inc',
        'src\nds\nds_native_stage_yostersmall.generated.inc',
        'src\nds\nds_native_stage_metal.generated.inc',
        'src\nds\nds_native_stage_zako.generated.inc',
        'src\nds\nds_native_stage_last.generated.inc',
        'src\nds\nds_native_stage_bonus3.generated.inc',
        'src\nds\nds_native_stage_bonus1_mario.generated.inc',
        'src\nds\nds_native_stage_bonus1_fox.generated.inc',
        'src\nds\nds_native_stage_bonus1_donkey.generated.inc',
        'src\nds\nds_native_stage_bonus1_samus.generated.inc',
        'src\nds\nds_native_stage_bonus1_luigi.generated.inc',
        'src\nds\nds_native_stage_bonus1_link.generated.inc',
        'src\nds\nds_native_stage_bonus1_yoshi.generated.inc',
        'src\nds\nds_native_stage_bonus1_captain.generated.inc',
        'src\nds\nds_native_stage_bonus1_kirby.generated.inc',
        'src\nds\nds_native_stage_bonus1_pikachu.generated.inc',
        'src\nds\nds_native_stage_bonus1_purin.generated.inc',
        'src\nds\nds_native_stage_bonus1_ness.generated.inc',
        'src\nds\nds_native_stage_bonus2_mario.generated.inc',
        'src\nds\nds_native_stage_bonus2_fox.generated.inc',
        'src\nds\nds_native_stage_bonus2_donkey.generated.inc',
        'src\nds\nds_native_stage_bonus2_samus.generated.inc',
        'src\nds\nds_native_stage_bonus2_luigi.generated.inc',
        'src\nds\nds_native_stage_bonus2_link.generated.inc',
        'src\nds\nds_native_stage_bonus2_yoshi.generated.inc',
        'src\nds\nds_native_stage_bonus2_captain.generated.inc',
        'src\nds\nds_native_stage_bonus2_kirby.generated.inc',
        'src\nds\nds_native_stage_bonus2_pikachu.generated.inc',
        'src\nds\nds_native_stage_bonus2_purin.generated.inc',
        'src\nds\nds_native_stage_bonus2_ness.generated.inc',
        'src\nds\nds_native_fighter_owner.generated.inc',
        # 2026-09-05: the owner generator also writes the native actor packet
        # (the Jungle barrel, e2e5c8bef5b). The .inc is gitignored with the rest
        # of src\nds\generated, so a clean checkout must see it produced here.
        'src\nds\generated\nds_native_actor_tarucann.generated.inc',
        'include\nds\generated\nds_native_actor_tarucann.generated.h'
    )
    # 2026-09-05: the BGM half of the assertion is derived from the runtime
    # table walked above -- every track's .bin and its .json sidecar -- instead
    # of a hand list that named five of 47 tracks.
    $generatedOutputs = @($bgmOutputs) + $generatedOutputs
    foreach ($relative in $generatedOutputs) {
        if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot $relative) -PathType Leaf)) {
            Stop-Build "generator did not produce $relative"
        }
    }
    Write-Host '    port asset regeneration PASS'

    Write-Step '6/7 Building the Nintendo DS shipping target'
    # 2026-09-05: the P2 base ROM. Until today this step still named the P1
    # target, smash64ds-battle-playable-hwtri -- the FROZEN P1 artifact that
    # docs\VERIFYING.md says nothing routine rebuilds -- so a clean-checkout
    # publish would have overwritten it with a HEAD build that cannot match its
    # pin. `smash64ds` is the Makefile's published P2 target (P2-1M, owner
    # 2026-08-19: "smash64ds is the base now"); an explicit TARGET is the
    # spelling the Makefile's defaulted-TARGET note exempts. The Makefile's
    # own flag block defines the configuration and nothing is overridden here,
    # so this builds exactly what bare `make` builds -- NDS_P2_1P_GAME stays at
    # its `?= 0` default until the owner flips it.
    $publishTarget = 'smash64ds'
    $processEnvironment = @{ PYTHONDONTWRITEBYTECODE = '1' }
    if ($Clean) {
        Invoke-LoggedProcess 'clean-port-target' $make `
            @("TARGET=$publishTarget", 'clean') $RepoRoot $processEnvironment
    }
    $makeArguments = @("TARGET=$publishTarget")
    if ($Jobs -gt 0) {
        $makeArguments += "-j$Jobs"
    }
    Invoke-LoggedProcess 'build-port-target' $make `
        $makeArguments $RepoRoot $processEnvironment

    Write-Step '7/7 Reporting ROM identity'
    # 2026-09-05: report the ROM step 6 actually built. DECOMP_PIN.txt's
    # OUTPUT_* rows still describe the frozen P1 artifact
    # (smash64ds-battle-playable-hwtri.nds, 12,530,688 B), so they are compared
    # only when they name this target's ROM; until the owner re-pins them for
    # smash64ds.nds the identity is printed without a reference verdict rather
    # than read off a root file this build never wrote.
    $output = Join-Path $RepoRoot "$publishTarget.nds"
    if (-not (Test-Path -LiteralPath $output -PathType Leaf)) {
        Stop-Build "port build succeeded without producing $output"
    }
    $outputLength = (Get-Item -LiteralPath $output).Length
    $outputHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $output).Hash
    Write-Host "    Output:  $output"
    Write-Host "    Bytes:   $outputLength"
    Write-Host "    SHA-256: $outputHash"
    if ($pins.OUTPUT_NAME -ine "$publishTarget.nds") {
        Write-Warning (
            "DECOMP_PIN.txt pins $($pins.OUTPUT_NAME) ($($pins.OUTPUT_BYTES) bytes), not " +
            "$publishTarget.nds; no reference identity exists for this build. Re-pin " +
            'OUTPUT_NAME/OUTPUT_BYTES/OUTPUT_SHA256 when the P2 ROM is released.'
        )
    } elseif ($outputLength -eq [int64]$pins.OUTPUT_BYTES -and
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
