$ErrorActionPreference = 'Stop'

function Convert-MarkerUInt32 {
    param([string]$Value)

    if ($Value.StartsWith('0x')) {
        return [Convert]::ToUInt32($Value.Substring(2), 16)
    }
    return [Convert]::ToUInt32($Value, 10)
}

# Build MI commands for an interactive step. Probes must not hand-roll these.
#
# Two traps live here, each of which has already cost a cycle:
#  1. A format-string `printf` wrapped in -interpreter-exec console "..." is
#     unparseable (the inner quotes close the outer string). GDB answers
#     ^error for that one command while every other command in the run
#     succeeds, so the transcript looks healthy and the value is simply
#     missing. Cost the frame counter twice on 2026-08-03.
#  2. -data-evaluate-expression needs the expression QUOTED whenever it
#     contains a space -- "(unsigned long*)x" fails with a bare "Usage:"
#     reply for the same silent-looking reason.
# Read values with New-GdbMiValueRead and neither form is expressible.
function New-GdbMiValueRead {
    param([Parameter(Mandatory)][string[]]$Expression)

    return @($Expression | ForEach-Object {
        '-data-evaluate-expression "' + ($_ -replace '"', '\"') + '"'
    })
}

# Console commands that genuinely have no MI equivalent (info symbol, x, bt).
# Never pass a printf with a format string through this -- use
# New-GdbMiValueRead, which is why this rejects one outright rather than
# letting it fail silently at runtime.
function New-GdbMiConsole {
    param([Parameter(Mandatory)][string[]]$Command)

    return @($Command | ForEach-Object {
        if ($_ -match '^\s*printf\s') {
            throw ("New-GdbMiConsole cannot carry a printf ('$_'): the inner " +
                   'quotes are unparseable inside -interpreter-exec console ' +
                   'and GDB fails that command silently. Use ' +
                   'New-GdbMiValueRead for value reads.')
        }
        '-interpreter-exec console "' + ($_ -replace '"', '\"') + '"'
    })
}

function Invoke-GdbMarkerScript {
    param(
        [string]$Gdb,
        [string]$Elf,
        [string]$Root,
        # object[], NOT string[] -- see the flatten below. This is load-bearing.
        [object[]]$Commands,
        [string]$ScriptName = '_verify_markers.gdb',
        [ValidateRange(1,3600)][int]$TimeoutSeconds = 30,
        [string]$ReadyFile = '',
        [object[]]$InteractiveSteps = @(),
        [switch]$MiInteractive
    )

    # A JAGGED COMMAND LIST USED TO FUSE ITSELF ONTO ONE LINE, SILENTLY.
    #
    # This parameter was [string[]]. A caller that builds its script with a
    # helper returning several commands -- `$commands = @('target remote ...',
    # (New-State 'tag'))` -- hands PowerShell a jagged array, and binding that
    # to [string[]] STRINGIFIES each inner array into ONE space-joined element.
    # gdb then receives several commands on one line, rejects or half-parses it,
    # prints an error nobody reads, and carries on: the run reaches its timeout
    # having executed a different script than the caller wrote.
    #
    # It cost a probe on 2026-08-14. The tell was a stray `game_status` file in
    # the repo root: the fused line was `shell cmd /c echo TIME=%TIME% printf
    # "... gSCManagerBattleState->game_status ..."`, and cmd read the `->` as a
    # redirect. The conclusion drawn from that probe had to be retracted
    # (`…/2026-08-14_runtime2-p95-closure/GATE_ARM_OWNERS.md` §1.3).
    #
    # The MI path has guarded against exactly this since 2026-08-03 (see the
    # fused-verb check below) -- but only for InteractiveSteps, and the batch
    # command list, which every verifier uses, had no guard at all. Flattening
    # here makes the wrong form inexpressible rather than merely detected.
    $Commands = @(
        $Commands | ForEach-Object {
            if (($null -ne $_) -and ($_ -isnot [string]) -and
                ($_ -is [System.Collections.IEnumerable])) {
                $_ | ForEach-Object { [string]$_ }
            } else {
                [string]$_
            }
        }
    )
    $multiline = @($Commands | Where-Object { $_ -match "[`r`n]" })
    if ($multiline.Count -gt 0) {
        throw ("GDB command contains an embedded newline, which would split " +
               "into lines gdb parses separately: '" +
               ($multiline[0] -replace "[`r`n]+", ' | ') + "'")
    }

    $interactive = -not [string]::IsNullOrWhiteSpace($ReadyFile)
    if (($InteractiveSteps.Count -ne 0) -and (-not $interactive)) {
        throw 'Interactive GDB steps require a ready-file path.'
    }
    if ($MiInteractive -and (-not $interactive)) {
        throw 'MI GDB mode requires interactive steps and a ready-file path.'
    }

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

    # MI interactive mode is useless without async: in the default all-stop
    # synchronous mode GDB stops reading stdin the moment -exec-continue starts,
    # so the -exec-interrupt this mode exists to deliver is never seen and the
    # caller times out with no error. Only one caller ever knew to set this by
    # hand, and a second capture attempt lost a cycle rediscovering it, so the
    # mode now guarantees it rather than documenting it.
    if ($MiInteractive -and
        (-not ($patchedCommands -match '^\s*set\s+mi-async\s'))) {
        $patchedCommands = @('set mi-async on') + $patchedCommands
    }

    # `set $fp = ...` does not create a convenience variable on ARM: $fp IS the
    # frame-pointer register, and assigning it fails with "Left operand of
    # assignment is not an lvalue" the moment no frame is selected. gdb reports
    # that against whatever top-level command was running -- typically a bare
    # `continue` -- naming neither the offending line nor the register, so the
    # probe reads as "the breakpoint never fired". Cost one probe run on
    # 2026-08-12. The same holds for $sp/$pc/$lr/$rN/$cpsr.
    $reservedAssign = @(
        $patchedCommands | Where-Object {
            $_ -match '^\s*set\s+\$(fp|sp|pc|lr|cpsr|r[0-9]+)\s*='
        }
    )
    if ($reservedAssign.Count -gt 0) {
        throw ("GDB script assigns an ARM register as if it were a convenience " +
               "variable; rename it (e.g. `$fp -> `$fst): " +
               ($reservedAssign -join '; '))
    }

    $gdbScriptPath = Join-Path $tempDir $ScriptName
    $gdbStdoutPath = Join-Path $tempDir ($ScriptName + '.out')
    $gdbStderrPath = Join-Path $tempDir ($ScriptName + '.err')
    Set-Content $gdbScriptPath -Value ($patchedCommands -join "`n")
    if ($interactive) {
        Remove-Item -LiteralPath $ReadyFile -Force -ErrorAction SilentlyContinue
    }

    $gdbInfo = New-Object System.Diagnostics.ProcessStartInfo
    $gdbInfo.FileName = $Gdb
    $gdbArguments = if ($interactive) {
        '-ex "set confirm off" "{0}" -x "{1}"' -f $Elf, $gdbScriptPath
    } else {
        '-batch -ex "set confirm off" "{0}" -x "{1}"' -f $Elf, $gdbScriptPath
    }
    $gdbInfo.Arguments = if ($MiInteractive) {
        '--interpreter=mi2 ' + $gdbArguments
    } else {
        $gdbArguments
    }
    $gdbInfo.RedirectStandardInput = $interactive
    $gdbInfo.RedirectStandardOutput = $true
    $gdbInfo.RedirectStandardError = $true
    $gdbInfo.UseShellExecute = $false
    $gdbInfo.CreateNoWindow = $true

    $gdbProcess = [System.Diagnostics.Process]::Start($gdbInfo)
    $timedOut = $false
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        # Read asynchronously so WaitForExit remains the timeout authority.
        # The former synchronous ReadToEnd calls could block forever before
        # the nominal timeout was reached.
        $stdoutTask = $gdbProcess.StandardOutput.ReadToEndAsync()
        $stderrTask = $gdbProcess.StandardError.ReadToEndAsync()
        if ($interactive) {
            while ((-not $gdbProcess.HasExited) -and
                   (-not (Test-Path -LiteralPath $ReadyFile -PathType Leaf)) -and
                   ($timer.ElapsedMilliseconds -lt ($TimeoutSeconds * 1000))) {
                Start-Sleep -Milliseconds 20
                $gdbProcess.Refresh()
            }
            if ((-not $gdbProcess.HasExited) -and
                (Test-Path -LiteralPath $ReadyFile -PathType Leaf)) {
                foreach ($step in $InteractiveSteps) {
                    $delayMilliseconds = [int]$step.DelayMilliseconds
                    if ($delayMilliseconds -lt 0) {
                        throw 'Interactive GDB delays cannot be negative.'
                    }
                    if ($delayMilliseconds -ne 0) {
                        Start-Sleep -Milliseconds $delayMilliseconds
                    }
                    $gdbProcess.Refresh()
                    if ($gdbProcess.HasExited) { break }
                    foreach ($command in @($step.Commands)) {
                        if ($command -isnot [string]) {
                            throw 'Interactive GDB commands must be a flat string array.'
                        }
                        # A one-element helper call returns a bare string, and
                        # "string" + @(...) is STRING concatenation in
                        # PowerShell, so a call site that forgets @() around
                        # the first operand fuses every command onto one line.
                        # GDB then rejects the whole line with a single parse
                        # error and the run looks like one bad command instead
                        # of a lost step. Cost a capture run on 2026-08-03.
                        $verbs = ([regex]::Matches($command,
                            '(?:^|\s)-(?:interpreter-exec|data-evaluate-expression|exec-|gdb-|stack-|var-)')).Count
                        if ($verbs -gt 1) {
                            throw ("Fused GDB command carries $verbs MI verbs on one " +
                                   "line: '$command'. Wrap each helper call in @() " +
                                   'before concatenating -- "string" + @(...) is ' +
                                   'string concatenation, not array concatenation.')
                        }
                        $gdbProcess.StandardInput.WriteLine([string]$command)
                    }
                    $gdbProcess.StandardInput.Flush()
                }
            }
        }
        $remainingMilliseconds = [Math]::Max(
            0,
            ($TimeoutSeconds * 1000) - [int]$timer.ElapsedMilliseconds)
        $timedOut = -not $gdbProcess.WaitForExit($remainingMilliseconds)
        if ($timedOut) {
            try {
                $gdbProcess.Kill($true)
            } catch {
                try { $gdbProcess.Kill() } catch {}
            }
        }
        $gdbProcess.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $exitCode = if ($timedOut) { -1 } else { $gdbProcess.ExitCode }
    } finally {
        if (-not $gdbProcess.HasExited) {
            try {
                $gdbProcess.Kill($true)
            } catch {
                try { $gdbProcess.Kill() } catch {}
            }
        }
        $gdbProcess.Dispose()
        $timer.Stop()
        if ($interactive) {
            Remove-Item -LiteralPath $ReadyFile -Force -ErrorAction SilentlyContinue
        }
    }
    Set-Content $gdbStdoutPath -Value $stdout
    Set-Content $gdbStderrPath -Value $stderr

    $elapsedSeconds = [Math]::Round($timer.Elapsed.TotalSeconds, 1)

    if ($timedOut) {
        throw ("GDB marker capture timed out after $TimeoutSeconds seconds " +
               "(elapsed ${elapsedSeconds}s).`n$stdout`n$stderr")
    }

    if ($exitCode -ne 0) {
        throw ("GDB marker capture failed with exit $exitCode after " +
               "${elapsedSeconds}s.`n$stdout`n$stderr")
    }

    # Board row R0, 2026-08-14: a capture that PASSES tells nobody how close it
    # came to its ceiling, so the drift that eventually turns Boundary red is
    # invisible until it is a red. Print the margin on every success. Write-Host
    # rather than Write-Output: this function returns an object and callers
    # assign it, so an extra pipeline record would corrupt every caller.
    Write-Host ("GDB marker capture: {0}s elapsed of {1}s ceiling ({2:P0} used)." -f
                $elapsedSeconds, $TimeoutSeconds,
                ($elapsedSeconds / [Math]::Max(1, $TimeoutSeconds)))

    return [PSCustomObject]@{
        Stdout = $stdout
        Stderr = $stderr
        ScriptPath = $gdbScriptPath
        StdoutPath = $gdbStdoutPath
        StderrPath = $gdbStderrPath
        ElapsedSeconds = $elapsedSeconds
        TimeoutSeconds = $TimeoutSeconds
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
