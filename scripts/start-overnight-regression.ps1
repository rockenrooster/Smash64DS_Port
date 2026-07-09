param(
    [Alias('?')]
    [switch]$Help,
    [switch]$Worker,
    [int]$DelaySeconds = 3,
    [int]$PollSeconds = 60,
    [string]$LogDir
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path

if ($Help) {
    Write-Output 'Usage: .\scripts\start-overnight-regression.ps1 [-DelaySeconds 3] [-PollSeconds 60]'
    Write-Output 'Starts one hidden overnight Regression worker: detached -Force prebuild, stamp wait, then shards 0..3 -NoBuild.'
    exit 0
}

if (-not $Worker) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $logDir = Join-Path $root "logs\overnight-regression-$stamp"
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    $driverLog = Join-Path $logDir 'driver.log'
    $args = @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', $PSCommandPath,
        '-Worker',
        '-DelaySeconds', "$DelaySeconds",
        '-PollSeconds', "$PollSeconds",
        '-LogDir', $logDir
    )
    $process = Start-Process -FilePath $powerShellExe `
        -ArgumentList $args `
        -WorkingDirectory $root `
        -WindowStyle Hidden `
        -RedirectStandardOutput $driverLog `
        -RedirectStandardError (Join-Path $logDir 'driver.err.log') `
        -PassThru
    Write-Output "Started overnight Regression worker pid=$($process.Id)"
    Write-Output "Logs: $logDir"
    Write-Output "Driver log: $driverLog"
    exit 0
}

if ([string]::IsNullOrWhiteSpace($LogDir)) {
    throw 'Worker requires -LogDir.'
}
$logDir = if ([System.IO.Path]::IsPathRooted($LogDir)) {
    $LogDir
} else {
    Join-Path $root $LogDir
}
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

function Invoke-LoggedCommand {
    param(
        [Parameter(Mandatory=$true)]
        [string]$Log,
        [Parameter(Mandatory=$true)]
        [string[]]$Command
    )
    $started = Get-Date -Format o
    "START $started $($Command -join ' ')" | Out-File -FilePath $Log -Encoding utf8
    $exe = $Command[0]
    $args = @()
    if ($Command.Count -gt 1) {
        $args = $Command[1..($Command.Count - 1)]
    }
    & $exe @args *>> $Log
    $exit = $LASTEXITCODE
    $finished = Get-Date -Format o
    "END $finished exit=$exit" | Out-File -FilePath $Log -Append -Encoding utf8
    return $exit
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

$prebuildDetachLog = Join-Path $logDir 'prebuild-detach.log'
$prebuildStampLog = Join-Path $logDir 'prebuild-stamp.log'
$buildScript = Join-Path $PSScriptRoot 'build-verify-profile.ps1'
$verifyScript = Join-Path $PSScriptRoot 'verify-all.ps1'

$exitCode = Invoke-LoggedCommand -Log $prebuildDetachLog -Command @(
    $buildScript, '-Profile', 'Regression', '-Force', '-Detach'
)
if ($exitCode -ne 0) {
    throw "Detached Regression prebuild launch failed: $prebuildDetachLog"
}

do {
    Start-Sleep -Seconds ([Math]::Max($PollSeconds, 5))
    $stampExit = Invoke-LoggedCommand -Log $prebuildStampLog -Command @(
        $buildScript, '-Profile', 'Regression', '-VerifyStamp'
    )
} while ($stampExit -ne 0)

for ($shard = 0; $shard -lt 4; $shard++) {
    $shardLog = Join-Path $logDir "shard-$shard.log"
    $exitCode = Invoke-LoggedCommand -Log $shardLog -Command @(
        $verifyScript,
        '-Profile', 'Regression',
        '-ShardCount', '4',
        '-ShardIndex', "$shard",
        '-RunnerSlot', "$shard",
        '-NoBuild',
        '-DelaySeconds', "$DelaySeconds"
    )
    if ($exitCode -ne 0) {
        throw "Regression shard $shard failed: $shardLog"
    }
}

Write-Output "Overnight Regression completed successfully. Logs: $logDir"
