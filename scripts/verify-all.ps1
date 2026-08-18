param(
    [switch]$Build,
    [switch]$NoBuild,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateSet('Latest','Boundary')]
    [string]$Profile = 'Boundary',
    [string[]]$Only,
    [string]$From,
    [switch]$List,
    [switch]$SkipRegistryCheck,
    [ValidateRange(0,3600)][int]$DelaySeconds = 5,
    [ValidateRange(-1,127)][int]$RunnerSlot = -1,
    [ValidateRange(1,65535)][int]$GdbPort = 4333
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
function Test-ScriptParameter {
    param(
        [string]$ScriptPath,
        [string]$Name
    )
    if (-not (Test-Path -LiteralPath $ScriptPath)) {
        return $false
    }
    $tokens = $null
    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile(
        $ScriptPath,
        [ref]$tokens,
        [ref]$errors
    )
    if ($errors -and $errors.Count -gt 0 -or $null -eq $ast.ParamBlock) {
        return $false
    }
    return @($ast.ParamBlock.Parameters | ForEach-Object {
        $_.Name.VariablePath.UserPath
    }) -contains $Name
}
function Test-TransportVerifierFailure {
    param([string]$Text)
    if (-not $Text) { return $false }
    $patterns = @(
        'gdb.*timed out',
        'timed out.*gdb',
        'connect.*timeout',
        'connection timed out',
        'unable to connect',
        'remote replied unexpectedly',
        'ignoring packet error',
        'target disconnected',
        'no connection could be made',
        'zero markers',
        'HARN=0,0,0,0,0',
        'SCENE=0,0,0'
    )
    foreach ($pattern in $patterns) {
        if ($Text -match $pattern) { return $true }
    }
    return $false
}
function Assert-Smash64DSToolchainUsable {
    param([string]$Root)
    # PRESENCE IS NOT USABILITY, AND THIS GUARD USED TO TEST PRESENCE.
    #
    # Until 2026-08-16 the only toolchain guard in this driver was
    # `if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }`, and it sat
    # inside `if ($Build -and $needsNormalBuild)` -- so on a Boundary run, whose
    # plan has no `smash64ds` target, it never executed at all. A shell whose
    # DEVKITPRO was already set did not trip it either. The run then died deep
    # inside a grandchild at
    #   make: *** [Makefile:3312: builds/build-...-harness] Error 127
    # and the driver reported nothing useful.
    #
    # Makefile:3312 is `@$(MAKE) --no-print-directory -C $(BUILD) ...`, and on
    # this host `$(MAKE)` measures as **/opt/devkitpro/msys2/usr/bin/make** --
    # devkitPro's msys2 reports its own argv[0] in the MSYS namespace, so that
    # path only resolves when the recipe shell (`SHELL = /usr/bin/env bash`) is
    # that same msys2. When it is not, the recursion is a literal nonexistent
    # path and the sub-make exits 127. No amount of DEVKITPRO spelling fixes
    # that, and no static inspection can see it: the only thing that answers the
    # question is running one recursive make.
    #
    # So this runs one, before any verifier starts, and throws by name if it
    # fails. It also normalizes both variables in the process environment every
    # child inherits, which is the half the old guard was trying to do.
    foreach ($name in @('DEVKITPRO', 'DEVKITARM')) {
        $value = [Environment]::GetEnvironmentVariable($name, 'Process')
        if ([string]::IsNullOrWhiteSpace($value)) {
            $value = if ($name -eq 'DEVKITPRO') {
                'C:/devkitPro'
            } else {
                'C:/devkitPro/devkitARM'
            }
        }
        $value = (($value -replace '\\', '/') -replace '/+$', '')
        [Environment]::SetEnvironmentVariable($name, $value, 'Process')
    }
    foreach ($relative in @('ds_rules', 'bin/arm-none-eabi-gcc.exe')) {
        $required = $env:DEVKITARM + '/' + $relative
        if (-not (Test-Path -LiteralPath $required)) {
            throw ("Toolchain is present but unusable: DEVKITARM='" +
                $env:DEVKITARM + "' does not contain '" + $relative + "'.")
        }
    }
    $makeCommand = Get-Command 'make.exe' -CommandType Application `
        -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $makeCommand) {
        $makeCommand = Get-Command 'make' -CommandType Application `
            -ErrorAction SilentlyContinue | Select-Object -First 1
    }
    if ($null -eq $makeCommand) {
        throw 'Toolchain is unusable: no make executable is on PATH.'
    }
    # A unique BUILD, for the same reason check-toolchain-path-normalization.ps1
    # uses one: the probe target has no prerequisites and must not be able to
    # touch, or collide with, a real build directory.
    $probeBuild = "builds/build-verify-all-toolchain-probe-{0}" -f `
        ([Guid]::NewGuid().ToString('N'))
    $probeEval = '--eval=nds-recursive-make-probe: ;' +
        '@$(MAKE) --version >/dev/null 2>&1 && ' +
        'echo NDS_RECURSIVE_MAKE=OK || echo NDS_RECURSIVE_MAKE=FAIL:$$?'
    $probeOutput = @(& $makeCommand.Source '--no-print-directory' '-s' `
        '-C' $Root "BUILD=$probeBuild" $probeEval 'nds-recursive-make-probe' 2>&1 |
        ForEach-Object { "$_" })
    $probeExit = $LASTEXITCODE
    $probeText = ($probeOutput -join "`n")
    if (($probeExit -ne 0) -or ($probeText -notmatch 'NDS_RECURSIVE_MAKE=OK')) {
        throw ("Recursive make is unusable, so every harness build in this " +
            "profile would die at Makefile:3312 with Error 127. " +
            "make='" + $makeCommand.Source + "' DEVKITPRO='" + $env:DEVKITPRO +
            "' DEVKITARM='" + $env:DEVKITARM + "' probe exit=" + $probeExit +
            "`n" + $probeText)
    }
    if (Test-Path -LiteralPath (Join-Path $Root $probeBuild)) {
        throw ("The toolchain probe created a build directory: " + $probeBuild)
    }
    Write-Output ("Toolchain usable: recursive make OK, DEVKITPRO=" +
        $env:DEVKITPRO + " DEVKITARM=" + $env:DEVKITARM)
}
function Invoke-VerifyScriptOnce {
    param(
        [string]$Script,
        [string[]]$Arguments
    )
    $argList = @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', $Script
    ) + $Arguments
    $tempBase = Join-Path ([System.IO.Path]::GetTempPath()) ("smash64ds-verify-{0}" -f ([System.Guid]::NewGuid().ToString('N')))
    $stdoutPath = "$tempBase.out"
    $stderrPath = "$tempBase.err"
    $process = Start-Process -FilePath $powerShellExe `
        -ArgumentList $argList `
        -WorkingDirectory $root `
        -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath `
        -WindowStyle Hidden `
        -Wait `
        -PassThru
    $stdout = if (Test-Path -LiteralPath $stdoutPath) { Get-Content -LiteralPath $stdoutPath -Raw } else { '' }
    $stderr = if (Test-Path -LiteralPath $stderrPath) { Get-Content -LiteralPath $stderrPath -Raw } else { '' }
    if ($stdout) { [Console]::Out.Write($stdout) }
    if ($stderr) { [Console]::Error.Write($stderr) }
    Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
    # AN UNKNOWN EXIT CODE IS A FAILURE, NOT A PASS. `$null -eq 0` is $false in
    # PowerShell, so a null ExitCode reaches the failure branch below and
    # `exit $null` exits **0** -- a driver reporting green for a run it never
    # got an answer about. Give it a definite code here instead.
    $exitCode = if ($null -eq $process) { 70 } else { $process.ExitCode }
    if ($null -eq $exitCode) { $exitCode = 70 }
    return [PSCustomObject]@{
        ExitCode = $exitCode
        Output = "$stdout`n$stderr"
    }
}
function Invoke-VerifyScript {
    param(
        [string]$Script,
        [string[]]$Arguments,
        [string]$Label,
        [switch]$RetryTransport
    )
    $result = Invoke-VerifyScriptOnce -Script $Script -Arguments $Arguments
    if ($result.ExitCode -eq 0) {
        $script:verifiersPassed++
        return
    }
    if ($RetryTransport -and (Test-TransportVerifierFailure -Text $result.Output)) {
        Write-Warning "Transport-class verifier failure for '$Label'; retrying once."
        $retry = Invoke-VerifyScriptOnce -Script $Script -Arguments $Arguments
        if ($retry.ExitCode -eq 0) {
            Write-Output "Transport retry passed: $Label"
            $script:verifiersPassed++
            return
        }
        exit (Get-Smash64DSFailureExitCode -Code $retry.ExitCode)
    }
    exit (Get-Smash64DSFailureExitCode -Code $result.ExitCode)
}
function Get-Smash64DSFailureExitCode {
    param($Code)
    # Reaching a failure branch with a success code is a contradiction, and it
    # is the exact shape of a false green. Refuse to exit 0 from here.
    if (($null -eq $Code) -or ($Code -eq 0)) { return 70 }
    return $Code
}
$script:verifiersPassed = 0
if ($Build -and $NoBuild) {
    throw 'Use either -Build or -NoBuild, not both.'
}
$selectedGdbPort = if (($RunnerSlot -ge 0) -and -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}
$previousEnv = @{
    SMASH64DS_RUNNER_SLOT = $env:SMASH64DS_RUNNER_SLOT
    SMASH64DS_GDB_PORT = $env:SMASH64DS_GDB_PORT
    SMASH64DS_VERIFY_LOG_DIR = $env:SMASH64DS_VERIFY_LOG_DIR
    SMASH64DS_VERIFY_TEMP_DIR = $env:SMASH64DS_VERIFY_TEMP_DIR
    SMASH64DS_VERIFY_NO_BUILD = $env:SMASH64DS_VERIFY_NO_BUILD
}
try {
    Set-MelonDSVerifierRunContext -Root $root -RunnerSlot $RunnerSlot -GdbPort $selectedGdbPort
    if ($NoBuild) {
        $env:SMASH64DS_VERIFY_NO_BUILD = '1'
    } else {
        Remove-Item Env:\SMASH64DS_VERIFY_NO_BUILD -ErrorAction SilentlyContinue
    }
    $plan = @(Get-Smash64DSVerifyPlan -Profile $Profile -Only $Only -From $From)
    if ($List) {
        $plan | Select-Object Name, Mode, Harness, Script, Target, Build, @{Name='Tags';Expression={$_.Tags -join ','}} | Format-Table -AutoSize
        exit 0
    }
    if ($plan.Count -lt 1) {
        throw "Verification profile '$Profile' selected no verifiers to run."
    }
    # Unconditional, and before anything else runs: see the function's own
    # comment. The old guard ran only on `-Build` with a normal-build plan,
    # i.e. never on Boundary, which is the profile that reported green after a
    # sub-build died.
    Assert-Smash64DSToolchainUsable -Root $root
    if ($RunnerSlot -ge 0) {
        Resolve-MelonDSRunnerSlot `
            -Root $root `
            -RunnerSlot $RunnerSlot `
            -MelonDS $MelonDS `
            -GdbPort $selectedGdbPort `
            -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') | Out-Null
    }
    $needsNormalBuild = @($plan | Where-Object {
        [string]::IsNullOrWhiteSpace($_.Target) -or $_.Target -eq 'smash64ds'
    }).Count -gt 0
    if ($Build -and $needsNormalBuild) {
        & make -C $root TARGET=smash64ds BUILD=build NDS_DEV_SCENE_HARNESS=normal NDS_HARNESS_FAST_LOGIC=0 -B
        if ($LASTEXITCODE -ne 0) { exit (Get-Smash64DSFailureExitCode -Code $LASTEXITCODE) }
    }
    $expectedVerifiers = 3 + $plan.Count + $(if ($SkipRegistryCheck) { 0 } else { 1 })
    Invoke-VerifyScript `
        -Script (Join-Path $PSScriptRoot 'check-gbi-decode-fixtures.ps1') `
        -Arguments @()
    # 5.8 s, and it is here because a hand-run checker is a checker nobody runs:
    # the A5I3 atlas conversion shipped in cffe9ff with every pinned number in
    # check-nds-particle-banks.ps1 left stale, and the next person to run it by
    # hand -- a day later, chasing an unrelated bug -- got seven consecutive
    # failures that were all just arrears. Every pin in it is an argued number,
    # so making a kept checkpoint prove them is the whole point of having them.
    Invoke-VerifyScript `
        -Script (Join-Path $PSScriptRoot 'check-nds-particle-banks.ps1') `
        -Arguments @()
    # P2-1j, and it is here for the same reason the two above are: the owner
    # found four missing menu elements by LOOKING at three separate builds, and
    # nothing in this tree compared a screen's source sprite list against the
    # one we ship. ~1 s, static, no ROM.
    Invoke-VerifyScript `
        -Script (Join-Path $PSScriptRoot 'check-mn-screen-coverage.ps1') `
        -Arguments @()
    if (-not $SkipRegistryCheck) {
        Invoke-VerifyScript `
            -Script (Join-Path $PSScriptRoot 'check-harness-registry.ps1') `
            -Arguments @()
    }
    foreach ($record in $plan) {
        Write-Output "Running verifier: $($record.Name) [$($record.Script)]"
        if ($NoBuild) {
            $targetName = if ($record.Target) { $record.Target } else { 'smash64ds' }
            $rom = Join-Path $root "$targetName.nds"
            $elf = Join-Path $root "$targetName.elf"
            if (-not (Test-Path -LiteralPath $rom) -or -not (Test-Path -LiteralPath $elf)) {
                throw "NoBuild requested, but verifier output is missing for '$($record.Name)'. Build the retained ROM first."
            }
        }
        $scriptPath = Join-Path $PSScriptRoot $record.Script
        $arguments = @('-MelonDS', $MelonDS, '-Gdb', $Gdb)
        if (Test-ScriptParameter -ScriptPath $scriptPath -Name 'DelaySeconds') {
            $arguments += @('-DelaySeconds', "$DelaySeconds")
        }
        if (Test-ScriptParameter -ScriptPath $scriptPath -Name 'GdbPort') {
            $arguments += @('-GdbPort', "$selectedGdbPort")
        }
        if (Test-ScriptParameter -ScriptPath $scriptPath -Name 'RunnerSlot') {
            $arguments += @('-RunnerSlot', "$RunnerSlot")
        }
        if ($NoBuild -and (Test-ScriptParameter -ScriptPath $scriptPath -Name 'NoBuild')) {
            $arguments += '-NoBuild'
        }
        if (($record.Name -eq 'battle_playable_realtime') -and
            (Test-ScriptParameter -ScriptPath $scriptPath -Name 'FastIteration')) {
            $arguments += '-FastIteration'
            # Every profile compares the same completed Cut G frame pair. This
            # removes host-delay/camera drift from retained profile decisions
            # while both frames retain independent content/detail gates.
            if (Test-ScriptParameter -ScriptPath $scriptPath -Name 'MaxScreenshotChangedFraction') {
                $arguments += @('-MaxScreenshotChangedFraction', '0.50')
            }
            if (Test-ScriptParameter -ScriptPath $scriptPath -Name 'MaxScreenshotMeanChannelDelta') {
                $arguments += @('-MaxScreenshotMeanChannelDelta', '45')
            }
        }
        Invoke-VerifyScript -Script $scriptPath -Arguments $arguments -Label $record.Name -RetryTransport
    }
    # THE PASS LINE IS THE ONLY RELIABLE FAILURE SIGNAL THIS DRIVER HAS
    # (docs/VERIFYING.md says so), so it must not be printable without the work.
    # It is now gated on a count that every passing verifier increments.
    if ($script:verifiersPassed -ne $expectedVerifiers) {
        throw ("Verifier accounting mismatch: {0} passed, {1} expected. " +
            "Refusing to report '{2} verification profile passed.'" -f `
            $script:verifiersPassed, $expectedVerifiers, $Profile)
    }
    Write-Output "$Profile verification profile passed."
} finally {
    foreach ($key in $previousEnv.Keys) {
        if ($null -eq $previousEnv[$key]) {
            Remove-Item "Env:\$key" -ErrorAction SilentlyContinue
        } else {
            Set-Item "Env:\$key" $previousEnv[$key]
        }
    }
}
