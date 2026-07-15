$ErrorActionPreference = 'Stop'

$script:MelonDSCanonicalWindowWidth = 488
$script:MelonDSCanonicalWindowHeight = 675
$script:MelonDSCanonicalGeometry =
    'AdnQywADAAAAAAeCAAACLwAACIEAAAPmAAAHggAAAk4AAAiBAAAD5gAAAAAAAAAACgAAAAeCAAACTgAACIEAAAPm'

function Get-ProjectRoot {
    param([string]$ScriptRoot)

    return (Resolve-Path (Join-Path $ScriptRoot '..')).Path
}

function Get-MelonDSActiveRunnerSlot {
    if ($env:SMASH64DS_RUNNER_SLOT -match '^-?[0-9]+$') {
        return [int]$env:SMASH64DS_RUNNER_SLOT
    }
    return -1
}

function Get-MelonDSActiveGdbPort {
    param([int]$DefaultPort = 3333)

    if ($env:SMASH64DS_GDB_PORT -match '^[0-9]+$') {
        return [int]$env:SMASH64DS_GDB_PORT
    }
    return $DefaultPort
}

function Get-MelonDSRunnerPort {
    param(
        [ValidateRange(0,127)][int]$RunnerSlot,
        [ValidateSet('ARM9','ARM7')][string]$Cpu = 'ARM9'
    )

    # Keep the user's manual 3333/3334 pair unconditionally free. Slot 2 uses
    # the explicitly documented high pair selected after the prior collision.
    $basePort = switch ($RunnerSlot) {
        0 { 4323 }
        2 { 4463 }
        default { 3333 + ($RunnerSlot * 10) }
    }
    if ($Cpu -eq 'ARM7') {
        return $basePort + 1
    }
    return $basePort
}

function Resolve-MelonDSRepoExecutablePath {
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][string]$MelonDS
    )

    $rootPath = [System.IO.Path]::GetFullPath($Root)
    $emulatorRoot = [System.IO.Path]::GetFullPath(
        (Join-Path $rootPath 'emulators'))
    $candidate = if ([System.IO.Path]::IsPathRooted($MelonDS)) {
        $MelonDS
    } else {
        Join-Path $rootPath $MelonDS
    }
    $candidatePath = [System.IO.Path]::GetFullPath($candidate)
    $emulatorPrefix = $emulatorRoot.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar

    if (-not $candidatePath.StartsWith(
            $emulatorPrefix,
            [System.StringComparison]::OrdinalIgnoreCase) -or
        [System.IO.Path]::GetFileName($candidatePath) -ine 'melonDS.exe') {
        throw "melonDS must be the repo-owned executable under '$emulatorRoot': $candidatePath"
    }
    return $candidatePath
}

function Assert-MelonDSRepoExecutablePath {
    param([Parameter(Mandatory=$true)][string]$MelonDSPath)

    $candidatePath = [System.IO.Path]::GetFullPath($MelonDSPath)
    $cursor = [System.IO.DirectoryInfo](Split-Path -Parent $candidatePath)
    while (($null -ne $cursor) -and ($cursor.Name -ine 'emulators')) {
        $cursor = $cursor.Parent
    }
    if (($null -eq $cursor) -or ($null -eq $cursor.Parent) -or
        [System.IO.Path]::GetFileName($candidatePath) -ine 'melonDS.exe') {
        throw "melonDS must be a repo-owned .\emulators executable: $candidatePath"
    }
    return Resolve-MelonDSRepoExecutablePath `
        -Root $cursor.Parent.FullName -MelonDS $candidatePath
}

function Resolve-MelonDSPath {
    param(
        [string]$Root,
        [string]$MelonDS
    )

    $slot = Get-MelonDSActiveRunnerSlot
    if ($slot -ge 0) {
        return Resolve-MelonDSRepoExecutablePath -Root $Root -MelonDS (
            Join-Path $Root "emulators\melonds-runners\slot$slot\melonDS.exe")
    }
    return Resolve-MelonDSRepoExecutablePath -Root $Root -MelonDS $MelonDS
}

function Get-MelonDSVerifierLogDir {
    param(
        [string]$Root,
        [int]$RunnerSlot = -1
    )

    if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_LOG_DIR)) {
        return $env:SMASH64DS_VERIFY_LOG_DIR
    }
    if ($RunnerSlot -ge 0) {
        return Join-Path $Root "artifacts\emulator-logs\slot$RunnerSlot"
    }
    return Join-Path $Root 'artifacts\emulator-logs'
}

function Get-MelonDSVerifierTempDir {
    param(
        [string]$Root,
        [int]$RunnerSlot = -1
    )

    if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
        return $env:SMASH64DS_VERIFY_TEMP_DIR
    }
    if ($RunnerSlot -ge 0) {
        return Join-Path $Root "artifacts\verifier-temp\slot$RunnerSlot"
    }
    return Join-Path $Root 'artifacts\verifier-temp\default'
}

function Set-MelonDSVerifierRunContext {
    param(
        [string]$Root,
        [int]$RunnerSlot = -1,
        [int]$GdbPort = 4333
    )

    $env:SMASH64DS_RUNNER_SLOT = "$RunnerSlot"
    $env:SMASH64DS_GDB_PORT = "$GdbPort"
    $env:SMASH64DS_VERIFY_LOG_DIR = Get-MelonDSVerifierLogDir -Root $Root -RunnerSlot $RunnerSlot
    $env:SMASH64DS_VERIFY_TEMP_DIR = Get-MelonDSVerifierTempDir -Root $Root -RunnerSlot $RunnerSlot

    New-Item -ItemType Directory -Force -Path $env:SMASH64DS_VERIFY_LOG_DIR | Out-Null
    New-Item -ItemType Directory -Force -Path $env:SMASH64DS_VERIFY_TEMP_DIR | Out-Null
}

function Initialize-MelonDSVerifierContext {
    param(
        [string]$Root,
        [string]$MelonDS,
        [int]$RunnerSlot = -1,
        [int]$GdbPort = 4333,
        [switch]$GdbPortExplicit,
        [switch]$NoBuild
    )

    $selectedPort = if (($RunnerSlot -ge 0) -and -not $GdbPortExplicit) {
        Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
    } else {
        $GdbPort
    }

    Set-MelonDSVerifierRunContext -Root $Root -RunnerSlot $RunnerSlot -GdbPort $selectedPort

    if ($NoBuild) {
        $env:SMASH64DS_VERIFY_NO_BUILD = '1'
    } else {
        Remove-Item Env:\SMASH64DS_VERIFY_NO_BUILD -ErrorAction SilentlyContinue
    }

    if ($RunnerSlot -ge 0) {
        return Resolve-MelonDSRunnerSlot `
            -Root $Root `
            -RunnerSlot $RunnerSlot `
            -MelonDS $MelonDS `
            -GdbPort $selectedPort `
            -GdbPortExplicit:$GdbPortExplicit
    }

    $path = Resolve-MelonDSPath -Root $Root -MelonDS $MelonDS
    return [PSCustomObject]@{
        RunnerSlot = -1
        MelonDSPath = $path
        GdbPort = $selectedPort
        Arm7Port = $selectedPort + 1
        ConfigPath = Join-Path (Split-Path -Parent $path) 'melonDS.toml'
        PersistentConfig = $false
    }
}

function Set-MelonDSTomlValue {
    param(
        [string]$Text,
        [string]$Section,
        [string]$Key,
        [string]$Value
    )

    # Normalize once at every mutation boundary so repeated profile application
    # is byte-idempotent even when Qt or Git supplied CRLF input.
    $Text = ($Text -replace "`r`n", "`n") -replace "`r", "`n"

    $sectionPattern = '(?ms)^\[' + [regex]::Escape($Section) + '\]\s*.*?(?=^\[|\z)'
    $sectionMatch = [regex]::Match($Text, $sectionPattern)
    if (-not $sectionMatch.Success) {
        $prefix = $Text
        if ($prefix.Length -gt 0 -and -not $prefix.EndsWith("`n")) {
            $prefix += "`n"
        }
        return $prefix + "[$Section]`n$Key = $Value`n"
    }

    $block = $sectionMatch.Value
    $keyPattern = '(?m)^(' + [regex]::Escape($Key) + '\s*=\s*).*$'
    if ($block -match $keyPattern) {
        $block = [regex]::Replace($block, $keyPattern, "`${1}$Value")
    } else {
        $block = $block.TrimEnd() + "`n$Key = $Value`n"
    }

    return $Text.Substring(0, $sectionMatch.Index) +
        $block +
        $Text.Substring($sectionMatch.Index + $sectionMatch.Length)
}

function Set-MelonDSWindowProfile {
    param(
        [Parameter(Mandatory=$true)]
        [AllowEmptyString()]
        [string]$Text
    )

    # Every repo-owned instance uses the same natural, equally sized vertical
    # DS pair. Capture code establishes 488x675 before pausing emulation; this
    # canonical Qt geometry keeps all checked-in-workspace TOMLs at one baseline.
    foreach ($setting in @(
        @('Enabled', 'true'),
        @('ShowOSD', 'false'),
        @('Geometry', "`"$script:MelonDSCanonicalGeometry`""),
        @('ScreenLayout', '0'),
        @('ScreenRotation', '0'),
        @('ScreenGap', '0'),
        @('ScreenSwap', 'false'),
        @('ScreenSizing', '0'),
        @('IntegerScaling', 'false'),
        @('ScreenAspectTop', '0'),
        @('ScreenAspectBot', '0'),
        @('ScreenFilter', 'false')
    )) {
        $Text = Set-MelonDSTomlValue -Text $Text `
            -Section 'Instance0.Window0' `
            -Key $setting[0] -Value $setting[1]
    }

    foreach ($window in 1..3) {
        $Text = Set-MelonDSTomlValue -Text $Text `
            -Section "Instance0.Window$window" `
            -Key 'Enabled' -Value 'false'
    }

    return $Text
}

function Set-MelonDSDualScreenLayout {
    param(
        [Parameter(Mandatory=$true)]
        [AllowEmptyString()]
        [string]$Text
    )

    return Set-MelonDSWindowProfile -Text $Text
}

function Set-MelonDSTomlRootValue {
    param(
        [string]$Text,
        [string]$Key,
        [string]$Value
    )

    $Text = ($Text -replace "`r`n", "`n") -replace "`r", "`n"

    $firstSection = [regex]::Match($Text, '(?m)^\[')
    $rootLength = if ($firstSection.Success) { $firstSection.Index } else { $Text.Length }
    $root = $Text.Substring(0, $rootLength)
    $keyPattern = '(?m)^(' + [regex]::Escape($Key) + '\s*=\s*).*$'
    if ($root -match $keyPattern) {
        $root = [regex]::Replace($root, $keyPattern, "`${1}$Value")
    } else {
        $root = $root.TrimEnd() + "`n$Key = $Value`n`n"
    }

    return $root + $Text.Substring($rootLength)
}

function Set-MelonDSManualProfile {
    param(
        [Parameter(Mandatory=$true)]
        [AllowEmptyString()]
        [string]$Text
    )

    $Text = Set-MelonDSWindowProfile -Text $Text
    $Text = Set-MelonDSTomlRootValue -Text $Text -Key 'PauseLostFocus' -Value 'false'
    $Text = Set-MelonDSTomlRootValue -Text $Text -Key 'TargetFPS' -Value '60.0'
    $Text = Set-MelonDSTomlRootValue -Text $Text -Key 'LimitFPS' -Value 'true'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Audio' -Key 'Volume' -Value '256'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb' -Key 'Enable' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb' -Key 'Enabled' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM9' -Key 'BreakOnStartup' -Value 'true'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM9' -Key 'Port' -Value '3333'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM7' -Key 'BreakOnStartup' -Value 'true'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM7' -Key 'Port' -Value '3334'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'JIT' -Key 'Enable' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D' -Key 'Renderer' -Value '1'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D.GL' -Key 'ScaleFactor' -Value '6'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D.GL' -Key 'BetterPolygons' -Value 'true'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D.GL' -Key 'HiresCoordinates' -Value 'true'
    return $Text
}

function Set-MelonDSAutomationProfile {
    param(
        [Parameter(Mandatory=$true)]
        [AllowEmptyString()]
        [string]$Text,
        [Parameter(Mandatory=$true)][int]$GdbPort,
        [Parameter(Mandatory=$true)][int]$Arm7Port,
        [switch]$MuteAudio
    )

    $Text = Set-MelonDSWindowProfile -Text $Text
    $Text = Set-MelonDSTomlRootValue -Text $Text -Key 'PauseLostFocus' -Value 'false'
    $Text = Set-MelonDSTomlRootValue -Text $Text -Key 'TargetFPS' -Value '60.0'
    $Text = Set-MelonDSTomlRootValue -Text $Text -Key 'LimitFPS' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb' -Key 'Enable' -Value 'true'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb' -Key 'Enabled' -Value 'true'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM9' -Key 'BreakOnStartup' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM9' -Key 'Port' -Value "$GdbPort"
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM7' -Key 'BreakOnStartup' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Gdb.ARM7' -Key 'Port' -Value "$Arm7Port"
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'JIT' -Key 'Enable' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D' -Key 'Renderer' -Value '0'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D.GL' -Key 'ScaleFactor' -Value '1'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D.GL' -Key 'BetterPolygons' -Value 'false'
    $Text = Set-MelonDSTomlValue -Text $Text -Section '3D.GL' -Key 'HiresCoordinates' -Value 'false'
    # Automation is always host-muted; the switch remains accepted so existing
    # verifier call sites do not need a flag migration.
    $Text = Set-MelonDSTomlValue -Text $Text -Section 'Instance0.Audio' -Key 'Volume' -Value '0'
    return $Text
}

function Set-MelonDSGdbConfig {
    param(
        [string]$MelonDSPath,
        [int]$GdbPort = (Get-MelonDSActiveGdbPort),
        [Nullable[int]]$Arm7Port,
        [switch]$Persistent,
        [switch]$MuteAudio
    )

    if (-not $Arm7Port.HasValue) {
        $Arm7Port = $GdbPort + 1
    }

    $MelonDSPath = Assert-MelonDSRepoExecutablePath -MelonDSPath $MelonDSPath
    $melonDsDir = Split-Path -Parent $MelonDSPath
    $config = Join-Path $melonDsDir 'melonDS.toml'
    $originalConfig = $null
    $created = $false

    if (Test-Path $config) {
        $originalConfig = Get-Content $config -Raw
        $text = $originalConfig
    } else {
        New-Item -ItemType Directory -Force -Path $melonDsDir | Out-Null
        $text = ''
        $created = $true
    }

    # Every automated launch is unthrottled, interpreter-only, software-rendered,
    # host-muted, and window-normalized. ROM audio remains fully active.
    $text = Set-MelonDSAutomationProfile -Text $text `
        -GdbPort $GdbPort -Arm7Port $Arm7Port -MuteAudio
    Set-Content $config -Value $text -NoNewline

    return [PSCustomObject]@{
        Config = $config
        OriginalConfig = $originalConfig
        Created = $created
        Persistent = [bool]$Persistent
        GdbPort = $GdbPort
        Arm7Port = $Arm7Port
    }
}

function Enable-MelonDSGdbConfig {
    param(
        [string]$MelonDSPath,
        [int]$GdbPort = (Get-MelonDSActiveGdbPort),
        [Nullable[int]]$Arm7Port,
        [switch]$Persistent,
        [switch]$MuteAudio
    )

    return Set-MelonDSGdbConfig `
        -MelonDSPath $MelonDSPath `
        -GdbPort $GdbPort `
        -Arm7Port $Arm7Port `
        -Persistent:$Persistent `
        -MuteAudio:$MuteAudio
}

function Restore-MelonDSGdbConfig {
    param([object]$State)

    if ($null -eq $State -or $State.Persistent) {
        return
    }
    if ($null -ne $State.OriginalConfig) {
        Set-Content $State.Config -Value $State.OriginalConfig -NoNewline
    } elseif ($State.Config) {
        Remove-Item $State.Config -Force -ErrorAction SilentlyContinue
    }
}

function Resolve-MelonDSRunnerSlot {
    param(
        [string]$Root,
        [int]$RunnerSlot = -1,
        [string]$MelonDS = (Join-Path $Root 'emulators\melonds\melonDS.exe'),
        [int]$GdbPort = 4333,
        [switch]$GdbPortExplicit
    )

    if ($RunnerSlot -lt 0) {
        $path = Resolve-MelonDSPath -Root $Root -MelonDS $MelonDS
        return [PSCustomObject]@{
            RunnerSlot = -1
            MelonDSPath = $path
            GdbPort = $GdbPort
            Arm7Port = $GdbPort + 1
            ConfigPath = Join-Path (Split-Path -Parent $path) 'melonDS.toml'
            PersistentConfig = $false
        }
    }

    $slotDir = Join-Path $Root "emulators\melonds-runners\slot$RunnerSlot"
    $path = Resolve-MelonDSRepoExecutablePath -Root $Root -MelonDS (
        Join-Path $slotDir 'melonDS.exe')
    if (-not (Test-Path -LiteralPath $path)) {
        throw "melonDS runner slot $RunnerSlot does not exist. Run .\scripts\New-MelonDSRunnerSlots.ps1 -Count $($RunnerSlot + 1)."
    }

    $selectedPort = if ($GdbPortExplicit) { $GdbPort } else { Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9 }
    $arm7Port = if ($GdbPortExplicit) { $GdbPort + 1 } else { Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM7 }
    Set-MelonDSGdbConfig -MelonDSPath $path -GdbPort $selectedPort -Arm7Port $arm7Port -Persistent | Out-Null

    return [PSCustomObject]@{
        RunnerSlot = $RunnerSlot
        MelonDSPath = $path
        GdbPort = $selectedPort
        Arm7Port = $arm7Port
        ConfigPath = Join-Path $slotDir 'melonDS.toml'
        PersistentConfig = $true
    }
}

function Wait-MelonDSGdbListener {
    param(
        [System.Diagnostics.Process]$Process,
        [int]$Port = (Get-MelonDSActiveGdbPort),
        [int]$Attempts = 60
    )

    for ($i = 0; $i -lt $Attempts; $i++) {
        $Process.Refresh()
        if ($Process.HasExited) {
            throw "melonDS exited before the ARM9 GDB sample point (exit $($Process.ExitCode))."
        }
        $listener = Get-NetTCPConnection -LocalPort $Port -State Listen `
            -ErrorAction SilentlyContinue |
            Where-Object { $_.OwningProcess -eq $Process.Id } |
            Select-Object -First 1
        if ($null -ne $listener) {
            return $listener
        }
        Start-Sleep -Milliseconds 500
    }
    throw "melonDS did not open the ARM9 GDB listener on 127.0.0.1:$Port."
}

function make {
    if ($env:SMASH64DS_VERIFY_NO_BUILD -eq '1') {
        Write-Output 'Skipping make because SMASH64DS_VERIFY_NO_BUILD=1.'
        $global:LASTEXITCODE = 0
        return
    }

    $makeCommand = Get-Command 'make.exe' -ErrorAction SilentlyContinue
    if ($null -eq $makeCommand) {
        $makeCommand = Get-Command 'make' -CommandType Application -ErrorAction Stop
    }

    & $makeCommand.Source @args
    $global:LASTEXITCODE = $LASTEXITCODE
}
