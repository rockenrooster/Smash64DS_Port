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
    return [PSCustomObject]@{
        ExitCode = $process.ExitCode
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
        return
    }
    if ($RetryTransport -and (Test-TransportVerifierFailure -Text $result.Output)) {
        Write-Warning "Transport-class verifier failure for '$Label'; retrying once."
        $retry = Invoke-VerifyScriptOnce -Script $Script -Arguments $Arguments
        if ($retry.ExitCode -eq 0) {
            Write-Output "Transport retry passed: $Label"
            return
        }
        exit $retry.ExitCode
    }
    exit $result.ExitCode
}
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
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        & make -C $root TARGET=smash64ds BUILD=build NDS_DEV_SCENE_HARNESS=normal NDS_HARNESS_FAST_LOGIC=0 -B -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    Invoke-VerifyScript `
        -Script (Join-Path $PSScriptRoot 'check-gbi-decode-fixtures.ps1') `
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
