$ErrorActionPreference = 'Stop'

function Get-ProjectRoot {
    param([string]$ScriptRoot)

    return (Resolve-Path (Join-Path $ScriptRoot '..')).Path
}

function Resolve-MelonDSPath {
    param(
        [string]$Root,
        [string]$MelonDS
    )

    if ([System.IO.Path]::IsPathRooted($MelonDS)) {
        return $MelonDS
    }
    return Join-Path $Root $MelonDS
}

function Enable-MelonDSGdbConfig {
    param([string]$MelonDSPath)

    $melonDsDir = Split-Path -Parent $MelonDSPath
    $config = Join-Path $melonDsDir 'melonDS.toml'
    $originalConfig = $null

    if (Test-Path $config) {
        $originalConfig = Get-Content $config -Raw
    } else {
        return [PSCustomObject]@{
            Config = $config
            OriginalConfig = $null
            Created = $false
        }
    }

    $text = Get-Content $config -Raw
    $gdbSectionPattern = '(?s)\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?'
    $enabled = $text -replace `
        '(?s)(\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?\bEnabled\s*=\s*)false', `
        '${1}true'
    $enabled = $enabled -replace `
        '(?s)(\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?\bEnable\s*=\s*)false', `
        '${1}true'
    if ($enabled -notmatch "$gdbSectionPattern\bEnabled\s*=\s*true") {
        $enabled = $enabled -replace `
            '(?m)(^\[Instance0\.Gdb\]\s*\r?\n)', `
            "`$1Enabled = true`r`n"
    }
    if ($enabled -notmatch "$gdbSectionPattern\bEnable\s*=\s*true") {
        $enabled = $enabled -replace `
            '(?m)(^\[Instance0\.Gdb\]\s*\r?\n)', `
            "`$1Enable = true`r`n"
    }
    if ($enabled -notmatch "$gdbSectionPattern\bEnabled\s*=\s*true" -or
        $enabled -notmatch "$gdbSectionPattern\bEnable\s*=\s*true") {
        throw 'Could not enable the melonDS ARM9 GDB stub.'
    }
    Set-Content $config -Value $enabled -NoNewline

    return [PSCustomObject]@{
        Config = $config
        OriginalConfig = $originalConfig
        Created = $false
    }
}

function Restore-MelonDSGdbConfig {
    param([object]$State)

    if ($null -eq $State) {
        return
    }
    if ($null -ne $State.OriginalConfig) {
        Set-Content $State.Config -Value $State.OriginalConfig -NoNewline
    } elseif ($State.Config) {
        Remove-Item $State.Config -Force -ErrorAction SilentlyContinue
    }
}

function Wait-MelonDSGdbListener {
    param(
        [System.Diagnostics.Process]$Process,
        [int]$Port = 3333,
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
