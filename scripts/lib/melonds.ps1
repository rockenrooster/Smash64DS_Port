$ErrorActionPreference = 'Stop'

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

    $basePort = 3333 + ($RunnerSlot * 10)
    if ($Cpu -eq 'ARM7') {
        return $basePort + 1
    }
    return $basePort
}

function Resolve-MelonDSPath {
    param(
        [string]$Root,
        [string]$MelonDS
    )

    $slot = Get-MelonDSActiveRunnerSlot
    if ($slot -ge 0) {
        return Join-Path $Root "emulators\melonds-runners\slot$slot\melonDS.exe"
    }

    if ([System.IO.Path]::IsPathRooted($MelonDS)) {
        return $MelonDS
    }
    return Join-Path $Root $MelonDS
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
        [int]$GdbPort = 3333
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
        [int]$GdbPort = 3333,
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

    $sectionPattern = '(?ms)^\[' + [regex]::Escape($Section) + '\]\s*.*?(?=^\[|\z)'
    $sectionMatch = [regex]::Match($Text, $sectionPattern)
    if (-not $sectionMatch.Success) {
        $prefix = $Text
        if ($prefix.Length -gt 0 -and -not $prefix.EndsWith("`n")) {
            $prefix += "`r`n"
        }
        return $prefix + "[$Section]`r`n$Key = $Value`r`n"
    }

    $block = $sectionMatch.Value
    $keyPattern = '(?m)^(' + [regex]::Escape($Key) + '\s*=\s*).*$'
    if ($block -match $keyPattern) {
        $block = [regex]::Replace($block, $keyPattern, "`${1}$Value")
    } else {
        $block = $block.TrimEnd() + "`r`n$Key = $Value`r`n"
    }

    return $Text.Substring(0, $sectionMatch.Index) +
        $block +
        $Text.Substring($sectionMatch.Index + $sectionMatch.Length)
}

function Set-MelonDSTomlRootValue {
    param(
        [string]$Text,
        [string]$Key,
        [string]$Value
    )

    $firstSection = [regex]::Match($Text, '(?m)^\[')
    $rootLength = if ($firstSection.Success) { $firstSection.Index } else { $Text.Length }
    $root = $Text.Substring(0, $rootLength)
    $keyPattern = '(?m)^(' + [regex]::Escape($Key) + '\s*=\s*).*$'
    if ($root -match $keyPattern) {
        $root = [regex]::Replace($root, $keyPattern, "`${1}$Value")
    } else {
        $root = $root.TrimEnd() + "`r`n$Key = $Value`r`n`r`n"
    }

    return $root + $Text.Substring($rootLength)
}

function Set-MelonDSGdbConfig {
    param(
        [string]$MelonDSPath,
        [int]$GdbPort = (Get-MelonDSActiveGdbPort),
        [Nullable[int]]$Arm7Port,
        [switch]$Persistent
    )

    if (-not $Arm7Port.HasValue) {
        $Arm7Port = $GdbPort + 1
    }

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

    $text = Set-MelonDSTomlValue -Text $text -Section 'Instance0.Gdb' -Key 'Enable' -Value 'true'
    $text = Set-MelonDSTomlValue -Text $text -Section 'Instance0.Gdb' -Key 'Enabled' -Value 'true'
    $text = Set-MelonDSTomlValue -Text $text -Section 'Instance0.Gdb.ARM9' -Key 'BreakOnStartup' -Value 'false'
    $text = Set-MelonDSTomlValue -Text $text -Section 'Instance0.Gdb.ARM9' -Key 'Port' -Value "$GdbPort"
    $text = Set-MelonDSTomlValue -Text $text -Section 'Instance0.Gdb.ARM7' -Key 'BreakOnStartup' -Value 'false'
    $text = Set-MelonDSTomlValue -Text $text -Section 'Instance0.Gdb.ARM7' -Key 'Port' -Value "$Arm7Port"
    # Verification logic is timer-relative and should not inherit a user's
    # wall-clock limiter or JIT setting. The interpreter remains the reference.
    $text = Set-MelonDSTomlRootValue -Text $text -Key 'LimitFPS' -Value 'false'
    $text = Set-MelonDSTomlValue -Text $text -Section 'JIT' -Key 'Enable' -Value 'false'
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
        [switch]$Persistent
    )

    return Set-MelonDSGdbConfig `
        -MelonDSPath $MelonDSPath `
        -GdbPort $GdbPort `
        -Arm7Port $Arm7Port `
        -Persistent:$Persistent
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
        [int]$GdbPort = 3333,
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
    $path = Join-Path $slotDir 'melonDS.exe'
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
