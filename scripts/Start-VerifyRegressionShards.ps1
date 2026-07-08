param(
    [ValidateRange(1,32)][int]$ShardCount = 4,
    [ValidateSet('Regression','RegressionFast','Boundary')]
    [string]$Profile = 'Regression',
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$List
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path
$logRoot = Join-Path $root ("artifacts\verify-shards\{0:yyyyMMdd_HHmmss}" -f (Get-Date))
$commands = @()
for ($i = 0; $i -lt $ShardCount; $i++) {
    $slotExe = Join-Path $root "emulators\melonds-runners\slot$i\melonDS.exe"
    if (-not (Test-Path -LiteralPath $slotExe)) {
        throw "Missing melonDS runner slot $i. Run .\scripts\New-MelonDSRunnerSlots.ps1 -Count $ShardCount first."
    }
    $args = @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', (Join-Path $PSScriptRoot 'verify-all.ps1'),
        '-Profile', $Profile,
        '-ShardCount', "$ShardCount",
        '-ShardIndex', "$i",
        '-RunnerSlot', "$i",
        '-DelaySeconds', "$DelaySeconds"
    )
    if ($NoBuild) { $args += '-NoBuild' }
    $commands += [PSCustomObject]@{
        Shard = $i
        Command = "$powerShellExe " + (($args | ForEach-Object { '"' + ($_ -replace '"','\"') + '"' }) -join ' ')
        Args = $args
    }
}
if ($List) {
    $commands | Select-Object Shard, Command | Format-List
    exit 0
}
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
$processes = @()
foreach ($command in $commands) {
    $stdout = Join-Path $logRoot "shard$($command.Shard).log"
    $stderr = Join-Path $logRoot "shard$($command.Shard).err.log"
    Write-Output "Starting shard $($command.Shard); log=$stdout"
    $processes += [PSCustomObject]@{
        Shard = $command.Shard
        Process = Start-Process -FilePath $powerShellExe `
            -ArgumentList $command.Args `
            -WorkingDirectory $root `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr `
            -WindowStyle Hidden `
            -PassThru
        Stdout = $stdout
        Stderr = $stderr
    }
}
$failed = @()
foreach ($entry in $processes) {
    $entry.Process.WaitForExit()
    if ($entry.Process.ExitCode -ne 0) {
        $failed += $entry
    }
}
if ($failed.Count -gt 0) {
    $summary = $failed | ForEach-Object { "shard $($_.Shard) exit $($_.Process.ExitCode): $($_.Stdout)" }
    throw "One or more verifier shards failed.`n$($summary -join "`n")"
}
Write-Output "All $ShardCount verifier shard(s) passed. Logs: $logRoot"
