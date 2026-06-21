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

    $gdbScriptPath = Join-Path $Root $ScriptName
    $gdbStdoutPath = Join-Path $Root ($ScriptName + '.out')
    $gdbStderrPath = Join-Path $Root ($ScriptName + '.err')
    Set-Content $gdbScriptPath -Value ($Commands -join "`n")

    $gdbInfo = New-Object System.Diagnostics.ProcessStartInfo
    $gdbInfo.FileName = $Gdb
    $gdbInfo.ArgumentList.Add($Elf)
    $gdbInfo.ArgumentList.Add('-x')
    $gdbInfo.ArgumentList.Add($gdbScriptPath)
    $gdbInfo.RedirectStandardOutput = $true
    $gdbInfo.RedirectStandardError = $true
    $gdbInfo.UseShellExecute = $false
    $gdbInfo.CreateNoWindow = $true

    $gdbProcess = [System.Diagnostics.Process]::Start($gdbInfo)
    $stdout = $gdbProcess.StandardOutput.ReadToEnd()
    $stderr = $gdbProcess.StandardError.ReadToEnd()
    $gdbProcess.WaitForExit()
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

    Remove-Item (Join-Path $Root $ScriptName) -Force -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $Root ($ScriptName + '.out')) -Force -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $Root ($ScriptName + '.err')) -Force -ErrorAction SilentlyContinue
}
