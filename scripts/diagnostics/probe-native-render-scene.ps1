[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_-]+$')][string]$Name,
    [Parameter(Mandatory)][ValidateRange(0,31)][int]$RunnerSlot,
    [Parameter(Mandatory)][ValidateRange(0,8)][int]$StageKind,
    [ValidateRange(1,1200)][int]$Presents = 16,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 240,
    [Parameter(Mandatory)][string]$Rom,
    [Parameter(Mandatory)][string]$Elf,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$NoCapture
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
. (Join-Path $root 'scripts/lib/melonds.ps1')
. (Join-Path $root 'scripts/lib/gdb-markers.ps1')
$slotDir = Join-Path $root "emulators/melonds-runners/slot$RunnerSlot"
$exe = Join-Path $slotDir 'melonDS.exe'
$config = Join-Path $slotDir 'melonDS.toml'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$watch = [Diagnostics.Stopwatch]::StartNew()
$lease = [Threading.Mutex]::new($false, "Smash64DS_DiagnosticRunner_$RunnerSlot")
$locked = $false
$emulator = $null
$originalConfig = $null
$canRestoreConfig = $true
$previousStorage = $env:SMASH64DS_VERIFY_STORAGE_DIR
$result = [ordered]@{ name=$Name; slot=$RunnerSlot; stage=$StageKind; presents=$Presents;
    transport='failed'; native='unobserved'; error=$null; elapsed_seconds=0 }
$exitCode = 1
try {
    try { $locked = $lease.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $locked = $true }
    if (-not $locked) { throw "Runner slot $RunnerSlot is already leased." }
    $exe = Resolve-MelonDSRepoExecutablePath -Root $root -MelonDS $exe
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Missing runner slot $RunnerSlot." }
    $live = @(Get-Process melonDS -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe })
    if ($live.Count) { throw "Runner slot $RunnerSlot already has a live emulator." }
    $Rom = (Resolve-Path -LiteralPath $Rom).Path
    $Elf = (Resolve-Path -LiteralPath $Elf).Path
    $result.rom_sha256 = (Get-FileHash -LiteralPath $Rom).Hash
    $result.elf_sha256 = (Get-FileHash -LiteralPath $Elf).Hash
    # Reject an incompatible ELF before paying the scene startup cost.
    $gdb = Join-Path 'C:/devkitPro/devkitARM/bin' 'arm-none-eabi-gdb.exe'
    $symbolArguments = @('-nx','--batch',$Elf)
    foreach ($symbol in @('ndsSceneManagerEnter','gNdsMenuShellWalkBudget',
        'gNdsMenuShellSssWalkTargetGkind','scVSBattleStartBattle',
        'ndsBattlePlayableFrameCompleteMarker','gSCManagerSceneData',
        'gSCManagerBattleState','gNdsRendererNativeFailure',
        'gNdsRendererStageOwnerFirstRejectReason','gNdsRendererStageOwnerRejectCount',
        'sNdsRendererAdapterNativeStageWorkspace')) {
        $symbolArguments += @('-ex',"info address $symbol")
    }
    $symbolOutput = & $gdb @symbolArguments 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Diagnostic ELF symbol preflight failed: $symbolOutput" }
    $result.symbol_preflight = 'pass'
    if (Test-Path -LiteralPath $config) { $originalConfig = [IO.File]::ReadAllBytes($config) }
    $env:SMASH64DS_VERIFY_STORAGE_DIR = Join-Path $slotDir "diagnostics/$Name"
    $result.storage_directory = $env:SMASH64DS_VERIFY_STORAGE_DIR
    # Each child has its own process environment, GDB files, config and disk.
    Remove-Item Env:SMASH64DS_VERIFY_LOG_DIR,Env:SMASH64DS_VERIFY_TEMP_DIR -ErrorAction SilentlyContinue
    $context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $RunnerSlot -NoBuild
    $result.arm9_port = $context.GdbPort
    $result.arm7_port = $context.Arm7Port
    Enable-MelonDSGdbConfig -MelonDSPath $exe -GdbPort $context.GdbPort `
        -Persistent -BreakOnStartup -MuteAudio | Out-Null
    $emulator = Start-Process -FilePath $exe -ArgumentList @($Rom) `
        -WorkingDirectory $slotDir -WindowStyle Hidden -PassThru
    $result.emulator_pid = $emulator.Id
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $commands = @('set pagination off','set confirm off','set remotetimeout 30',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break ndsSceneManagerEnter','commands','silent',
        'set variable gNdsMenuShellWalkBudget = 1',
        ('set variable gNdsMenuShellSssWalkTargetGkind = ' + $StageKind),
        'continue','end','tbreak scVSBattleStartBattle','continue','delete',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        ('ignore $bpnum ' + ($Presents - 1)), 'continue',
        'printf "DIAG_STATE=%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerBattleState->gkind, gSCManagerBattleState->time_passed, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count',
        'printf "DIAG_NATIVE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.scene, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, gNdsRendererNativeFailure.material, gNdsRendererNativeFailure.reason',
        'printf "DIAG_STAGE_OWNER=%u,%u,%u\n", gNdsRendererStageOwnerFirstRejectReason, gNdsRendererStageOwnerRejectCount, sNdsRendererAdapterNativeStageWorkspace.dobj_count')
    if (-not $NoCapture) {
        $capture = Join-Path $root "artifacts/visibility/$Name.png"
        $helper = Join-Path $root 'scripts/capture-running-melonds-window.ps1'
        $commands += ('shell pwsh -NoProfile -File "' + $helper + '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $capture + '"')
        $result.capture = $capture
    }
    $commands += @('detach','quit')
    $scriptName = "diagnostic_$Name.gdb"
    Invoke-GdbMarkerScript -Gdb $gdb -Elf $Elf -Root $root -Commands $commands `
        -ScriptName $scriptName -TimeoutSeconds $TimeoutSeconds | Out-Null
    $transcript = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR "$scriptName.out"
    # The shared helper writes .out/.err beside the generated GDB command file.
    if (-not (Test-Path -LiteralPath $transcript)) { $transcript += '.txt' }
    $text = Get-Content -LiteralPath $transcript -Raw
    Copy-Item -LiteralPath $transcript -Destination (Join-Path $output "$Name.gdb.txt")
    $state = [regex]::Match($text,'(?m)^DIAG_STATE=([0-9,]+)\r?$')
    $native = [regex]::Match($text,'(?m)^DIAG_NATIVE=([0-9,]+)\r?$')
    if (-not $state.Success -or -not $native.Success) { throw 'Missing diagnostic result markers.' }
    $result.state = @($state.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    $result.native_failure = @($native.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    if ($result.state[0] -ne 22 -or $result.state[1] -ne $StageKind) { throw 'Probe reached the wrong scene/stage.' }
    if (-not $NoCapture -and -not (Test-Path -LiteralPath $result.capture)) { throw 'Screenshot was not produced.' }
    if ((Get-FileHash -LiteralPath $Rom).Hash -ne $result.rom_sha256 -or
        (Get-FileHash -LiteralPath $Elf).Hash -ne $result.elf_sha256) { throw 'ROM or ELF changed during diagnosis.' }
    $result.transport = 'ok'
    $result.native = if ($result.native_failure[0] -eq 0) { 'pass' } else { 'fail' }
    $exitCode = if ($result.native -eq 'pass') { 0 } else { 2 }
} catch {
    $result.error = $_.Exception.Message
} finally {
    if ($emulator) {
        try {
            $emulator.Refresh()
            $result.peak_working_set_bytes = $emulator.PeakWorkingSet64
            $result.cpu_seconds = $emulator.TotalProcessorTime.TotalSeconds
        } catch {
            $result.metrics_error = $_.Exception.Message
        } finally {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
            try {
                if (-not $emulator.WaitForExit(10000)) { throw 'The owned emulator did not stop.' }
            } catch {
                $result.cleanup_error = $_.Exception.Message
                $result.transport = 'failed'
                $canRestoreConfig = $false
                $exitCode = 1
            }
        }
    }
    if ($canRestoreConfig -and $null -ne $originalConfig) { [IO.File]::WriteAllBytes($config,$originalConfig) }
    $env:SMASH64DS_VERIFY_STORAGE_DIR = $previousStorage
    $result.elapsed_seconds = [Math]::Round($watch.Elapsed.TotalSeconds,3)
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output "$Name.json") -Encoding utf8
    if ($locked) { $lease.ReleaseMutex() }
    $lease.Dispose()
}
exit $exitCode
