[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_-]+$')][string]$Name,
    [Parameter(Mandatory)][ValidateRange(0,31)][int]$RunnerSlot,
    [Parameter(Mandatory)][ValidateRange(0,8)][int]$StageKind,
    # 255 keeps the walk's own Mario/Fox pair, so a stage case is unchanged.
    # Any other value is an FTKind poked into the character-select walk gate;
    # the guest refuses a fighter this build does not carry, so a case naming
    # an absent fighter fails its own commit assertion rather than quietly
    # measuring Mario.
    [ValidateScript({ $_ -eq 255 -or ($_ -ge 0 -and $_ -le 11) })][int]$Fighter1Kind = 255,
    [ValidateScript({ $_ -eq 255 -or ($_ -ge 0 -and $_ -le 11) })][int]$Fighter2Kind = 255,
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
    fighter1=$Fighter1Kind; fighter2=$Fighter2Kind;
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
        'sNdsRendererAdapterNativeStageWorkspace',
        'gNdsInishiePakkunCandidateStep',
        'gNdsNativeFighterValidateRejectCode',
        'gNdsMenuShellCssWalkTargetKind','gNdsMenuShellCssWalkTargetKind2',
        'gNdsRendererFastOwnerTriangleCount')) {
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
        ('set variable gNdsMenuShellCssWalkTargetKind = ' + $Fighter1Kind),
        ('set variable gNdsMenuShellCssWalkTargetKind2 = ' + $Fighter2Kind),
        'continue','end','tbreak scVSBattleStartBattle','continue','delete',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        ('ignore $bpnum ' + ($Presents - 1)), 'continue',
        'printf "DIAG_STATE=%u,%u,%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerBattleState->gkind, gSCManagerBattleState->time_passed, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count',
        'printf "DIAG_NATIVE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.scene, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, gNdsRendererNativeFailure.material, gNdsRendererNativeFailure.reason',
        'printf "DIAG_STAGE_OWNER=%u,%u,%u\n", gNdsRendererStageOwnerFirstRejectReason, gNdsRendererStageOwnerRejectCount, sNdsRendererAdapterNativeStageWorkspace.dobj_count',
        # Which fighters actually committed, and whether they drew. A fighter
        # case that never commits its kind is measuring Mario; one that commits
        # and emits no owner triangles is a successful empty draw, which the
        # native-only contract forbids just as much as a recorded failure.
        'printf "DIAG_FIGHTER=%u,%u,%d,%d\n", gSCManagerBattleState->players[0].fkind, gSCManagerBattleState->players[1].fkind, gSCManagerBattleState->players[0].total_damage_all, gSCManagerBattleState->players[1].total_damage_given',
        'echo DIAG_OWNERTRI=', 'output gNdsRendererFastOwnerTriangleCount', 'echo \n',
        # A fighter-domain REJECTED_PROGRAM means a native owner was declined at
        # validate. These name which check, which slot and what it expected
        # against what it observed, which is the difference between guessing at
        # a missing model-part variant and knowing the pair.
        'printf "DIAG_FTREJECT=%u,%u,%#x,%#x,%#x,%#x\n", gNdsNativeFighterValidateRejectCode, gNdsNativeFighterValidateRejectSlot, gNdsNativeFighterValidateRejectLow, gNdsNativeFighterValidateRejectRoot, gNdsNativeFighterValidateRejectObserved, gNdsNativeFighterValidateRejectExpected',
        # Stage-owner reject reason 6 means ndsRendererPrepareNativeStageOwner
        # returned FALSE for the whole stage; these are the steps inside it, so
        # the same run that reports the reject also says which step declined.
        'printf "DIAG_STAGE_PREP=%u,%u,%#x,%u,%u,%u,%u\n", gNdsNativeStageOwnerPrepareFailStep, gNdsNativeStageOwnerPrepareFailSegment, gNdsNativeStageOwnerPrepareGuardMask, gNdsNativeStagePrepareRunFailStep, gNdsNativeStagePrepareRunFailRun, gNdsNativeStageValidateFullFailStep, gNdsNativeStageValidateFullFailIndex',
        # Step 2 is the texture resolve. Its operands and the static corpus's
        # own counters say whether the pin set was prepared at all, whether it
        # was violated, and which image the run could not resolve.
        'printf "DIAG_PAKKUN=%u,%#x,%#x,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsInishiePakkunCandidateStep, gNdsInishiePakkunMaterialFlags, gNdsInishiePakkunEffects, gNdsInishiePakkunDrawCount, gNdsInishiePakkunSubmitFailCount, gNdsInishiePakkunSubmitStep, gNdsInishiePakkunImageW0, gNdsInishiePakkunImage, gNdsInishiePakkunProjection, gNdsInishiePakkunModelview',
        'printf "DIAG_STAGE_TEX=%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsNativeStagePrepareRunTexture[0], gNdsNativeStagePrepareRunTexture[1], gNdsNativeStagePrepareRunTexture[2], gNdsRendererBattleStaticTexturePreparedCount, gNdsRendererBattleStaticTexturePrepareFailCount, gNdsRendererBattleStaticTextureViolationCount, gNdsRendererBattleStaticTexturePinnedHitCount, gNdsRendererBattleStaticTextureFailStep')
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
    if ($Fighter1Kind -ne 255) {
        $fighter = [regex]::Match($text,'(?m)^DIAG_FIGHTER=(-?[0-9,\-]+)\r?$')
        $ownertri = [regex]::Match($text,'(?m)^DIAG_OWNERTRI=\{([0-9, ]+)\}\r?$')
        if (-not $fighter.Success -or -not $ownertri.Success) { throw 'Missing fighter diagnostic markers.' }
        $result.fighter_state = @($fighter.Groups[1].Value.Split(',') | ForEach-Object { [int]$_ })
        $result.owner_triangles = @($ownertri.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_.Trim() })
        # A case that does not commit its own fighter has measured Mario and
        # would report his verdict under another fighter's name.
        if ($result.fighter_state[0] -ne $Fighter1Kind) { throw 'Probe committed the wrong player-1 fighter.' }
        if ($Fighter2Kind -ne 255 -and $result.fighter_state[1] -ne $Fighter2Kind) { throw 'Probe committed the wrong player-2 fighter.' }
        # Zero recorded failures AND zero drawn triangles is a successful empty
        # draw, which is exactly what the native-only contract forbids.
        if (($result.owner_triangles | Measure-Object -Sum).Sum -eq 0) { throw 'No native owner emitted triangles on the sampled frame.' }
    }
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
