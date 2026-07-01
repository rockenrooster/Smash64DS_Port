param(
    [switch]$Build,
    [switch]$NoBuild,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateSet('Full','Latest','LatestFast','BoundaryDirect','Boundary','Regression','RegressionFast','Smoke','SmokeFast','Fighter','Direct','MenuChain')]
    [string]$Profile = 'Full',
    [string[]]$Only,
    [string]$From,
    [switch]$List,
    [switch]$SkipRegistryCheck,
    [ValidateRange(0,3600)][int]$DelaySeconds = 5,
    [ValidateRange(1,128)][int]$ShardCount = 1,
    [ValidateRange(0,127)][int]$ShardIndex = 0,
    [ValidateRange(-1,127)][int]$RunnerSlot = -1,
    [ValidateRange(1,65535)][int]$GdbPort = 3333
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
function Select-VerifyPlanShard {
    param(
        [object[]]$Plan,
        [int]$Count,
        [int]$Index
    )
    if ($Index -ge $Count) {
        throw "ShardIndex $Index must be less than ShardCount $Count."
    }
    $selected = @()
    for ($i = 0; $i -lt $Plan.Count; $i++) {
        if (($i % $Count) -eq $Index) {
            $selected += $Plan[$i]
        }
    }
    return @($selected)
}
function Invoke-VerifyScript {
    param(
        [string]$Script,
        [string[]]$Arguments
    )
    $argList = @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', $Script
    ) + $Arguments
    $process = Start-Process -FilePath $powerShellExe `
        -ArgumentList $argList `
        -WorkingDirectory $root `
        -NoNewWindow `
        -Wait `
        -PassThru
    if ($process.ExitCode -ne 0) {
        exit $process.ExitCode
    }
}
if ($Build -and $NoBuild) {
    throw 'Use either -Build or -NoBuild, not both.'
}
$selectedGdbPort = if (($RunnerSlot -ge 0) -and -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}
if ($ShardCount -gt 1 -and $RunnerSlot -lt 0) {
    Write-Warning 'Parallel shards require isolated runner slots. Use -RunnerSlot N or separate checkouts.'
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
    if ($RunnerSlot -ge 0 -and -not $List) {
        Resolve-MelonDSRunnerSlot `
            -Root $root `
            -RunnerSlot $RunnerSlot `
            -MelonDS $MelonDS `
            -GdbPort $selectedGdbPort `
            -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') | Out-Null
    }
    if ($Build) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        & make -C $root TARGET=smash64ds BUILD=build NDS_DEV_SCENE_HARNESS=normal -B -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    $plan = @(Get-Smash64DSVerifyPlan -Profile $Profile -Only $Only -From $From)
    $plan = @(Select-VerifyPlanShard -Plan $plan -Count $ShardCount -Index $ShardIndex)
    if ($List) {
        $plan | Select-Object Name, Mode, Harness, Script, Target, Build, @{Name='Tags';Expression={$_.Tags -join ','}} | Format-Table -AutoSize
        exit 0
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
                throw "NoBuild requested, but verifier output is missing for '$($record.Name)'. Run .\scripts\build-verify-profile.ps1 -Profile $Profile first."
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
        Invoke-VerifyScript -Script $scriptPath -Arguments $arguments
    }
    if ($Profile -eq 'Full' -and -not $Only -and -not $From -and $ShardCount -eq 1) {
        Write-Output 'Full verification passed.'
    } elseif ($Profile -eq 'Latest' -and -not $Only -and -not $From -and $ShardCount -eq 1) {
        Write-Output 'Latest verification profile passed.'
    } elseif ($Profile -eq 'BoundaryDirect' -and -not $Only -and -not $From -and $ShardCount -eq 1) {
        Write-Output 'BoundaryDirect verification profile passed.'
    } elseif ($Profile -eq 'Boundary' -and -not $Only -and -not $From -and $ShardCount -eq 1) {
        Write-Output 'Boundary verification profile passed.'
    } elseif ($Profile -eq 'Regression' -and -not $Only -and -not $From -and $ShardCount -eq 1) {
        Write-Output 'Regression verification profile passed.'
    } else {
        Write-Output "Verification profile '$Profile' shard $ShardIndex/$ShardCount passed."
    }
} finally {
    foreach ($key in $previousEnv.Keys) {
        if ($null -eq $previousEnv[$key]) {
            Remove-Item "Env:\$key" -ErrorAction SilentlyContinue
        } else {
            Set-Item "Env:\$key" $previousEnv[$key]
        }
    }
}
