$ErrorActionPreference = 'Stop'

function Convert-MarkerUInt32 {
    param([string]$Value)

    if ($Value.StartsWith('0x')) {
        return [Convert]::ToUInt32($Value.Substring(2), 16)
    }
    return [Convert]::ToUInt32($Value, 10)
}

function Invoke-GdbMarkerScript {
    param(
        [string]$Gdb,
        [string]$Elf,
        [string]$Root,
        [string[]]$Commands,
        [string]$ScriptName = '_verify_markers.gdb'
    )

    $tempDir = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
        $env:SMASH64DS_VERIFY_TEMP_DIR
    } else {
        Join-Path $Root 'artifacts\verifier-temp\default'
    }
    New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

    $gdbPort = if ($env:SMASH64DS_GDB_PORT -match '^[0-9]+$') {
        [int]$env:SMASH64DS_GDB_PORT
    } else {
        3333
    }
    $patchedCommands = @(
        $Commands | ForEach-Object {
            $_ -replace 'target remote 127\.0\.0\.1:[0-9]+', "target remote 127.0.0.1:$gdbPort"
        }
    )

    $gdbScriptPath = Join-Path $tempDir $ScriptName
    $gdbStdoutPath = Join-Path $tempDir ($ScriptName + '.out')
    $gdbStderrPath = Join-Path $tempDir ($ScriptName + '.err')
    Set-Content $gdbScriptPath -Value ($patchedCommands -join "`n")

    $gdbInfo = New-Object System.Diagnostics.ProcessStartInfo
    $gdbInfo.FileName = $Gdb
    $gdbInfo.Arguments = '-batch -ex "set confirm off" "{0}" -x "{1}"' -f $Elf, $gdbScriptPath
    $gdbInfo.RedirectStandardOutput = $true
    $gdbInfo.RedirectStandardError = $true
    $gdbInfo.UseShellExecute = $false
    $gdbInfo.CreateNoWindow = $true

    $gdbProcess = [System.Diagnostics.Process]::Start($gdbInfo)
    $stdout = $gdbProcess.StandardOutput.ReadToEnd()
    $stderr = $gdbProcess.StandardError.ReadToEnd()
    if ($gdbProcess.WaitForExit(30000) -eq $false) {
        try {
            $gdbProcess.Kill()
        } catch {
        }
        throw "GDB marker capture timed out.`n$stdout`n$stderr"
    }
    Set-Content $gdbStdoutPath -Value $stdout
    Set-Content $gdbStderrPath -Value $stderr

    if ($gdbProcess.ExitCode -ne 0) {
        throw "GDB marker capture failed with exit $($gdbProcess.ExitCode).`n$stdout`n$stderr"
    }

    return [PSCustomObject]@{
        Stdout = $stdout
        Stderr = $stderr
        ScriptPath = $gdbScriptPath
        StdoutPath = $gdbStdoutPath
        StderrPath = $gdbStderrPath
    }
}

function Remove-GdbMarkerTemps {
    param(
        [string]$Root,
        [string]$ScriptName = '_verify_markers.gdb'
    )

    $tempDir = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
        $env:SMASH64DS_VERIFY_TEMP_DIR
    } else {
        Join-Path $Root 'artifacts\verifier-temp\default'
    }

    Remove-Item (Join-Path $tempDir $ScriptName) -Force -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $tempDir ($ScriptName + '.out')) -Force -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $tempDir ($ScriptName + '.err')) -Force -ErrorAction SilentlyContinue
}
