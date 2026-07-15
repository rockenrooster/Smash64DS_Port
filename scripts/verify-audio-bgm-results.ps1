param(
    [int]$RunnerSlot = 4,
    [int]$GdbPort = 3373,
    [switch]$NoBuild,
    [int]$DelaySeconds = 3,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 300,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

if ($RunnerSlot -lt 0) {
    throw 'Results audio verification requires an automation-owned runner slot; user melonDS config is out of scope.'
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$selectedPort = if ($PSBoundParameters.ContainsKey('GdbPort')) {
    $GdbPort
} else {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
}
if ($selectedPort -in @(3333, 3334)) {
    throw 'Results audio verification may not use reserved user ports 3333/3334.'
}

$target = 'smash64ds-audio-results-natural'
$build = 'build-audio-results-natural'
$harness = 'battle_playable_match_lifecycle'
$rom = Join-Path $root "$target.nds"
$elf = Join-Path $root "$target.elf"
$scriptName = '_audio_bgm_results.gdb'
$emulator = $null
$configState = $null
$summary = $null

function Assert-Result {
    param([bool]$Condition, [string]$Message, [string]$Context = '')
    if (-not $Condition) { throw "$Message`n$Context" }
}

function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') {
            $values += [int64](Convert-MarkerUInt32 $text)
        } else {
            $values += [int64]$text
        }
    }
    return $values
}

& (Join-Path $PSScriptRoot 'check-audio-bgm-derived-assets.ps1') -Root $root

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
if (-not $NoBuild) {
    & make -C $root `
        "TARGET=$target" `
        "BUILD=$build" `
        "NDS_DEV_SCENE_HARNESS=$harness" `
        'NDS_DEV_LIVE_INPUT_PREVIEW=1' `
        'NDS_HARNESS_FAST_LOGIC=1' `
        'NDS_DEV_RESULTS_VISUAL_SMOKE=1' `
        'NDS_RENDERER_PROFILE_LEVEL=0' `
        -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
Assert-Result ((Test-Path -LiteralPath $rom) -and
    (Test-Path -LiteralPath $elf)) 'Results audio build did not produce its isolated ROM and ELF.'
Assert-Result (Test-Path -LiteralPath $Gdb) "GDB not found: $Gdb"

$context = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $selectedPort `
    -GdbPortExplicit `
    -NoBuild:$NoBuild
$melonDsPath = $context.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir 'melonds.audio-bgm-results.stdout.log'
$stderr = Join-Path $logDir 'melonds.audio-bgm-results.stderr.log'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $melonDsPath `
        -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) `
        -MuteAudio
    $configText = Get-Content -LiteralPath $configState.Config -Raw
    Assert-Result ($configText -match '(?m)^Volume\s*=\s*0\s*$') `
        'Automation runner host audio is not muted.'
    Assert-Result ($configText -match "(?m)^Port\s*=\s*$($context.GdbPort)\s*$") `
        'Automation runner ARM9 port does not match the requested isolated port.'
    Assert-Result ($configText -match "(?m)^Port\s*=\s*$($context.Arm7Port)\s*$") `
        'Automation runner ARM7 port does not match the requested isolated port.'

    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    $arm9Listener = Wait-MelonDSGdbListener `
        -Process $emulator -Port $context.GdbPort
    $arm7Listener = $null
    for ($i = 0; $i -lt 40 -and $null -eq $arm7Listener; $i++) {
        $arm7Listener = Get-NetTCPConnection `
            -LocalPort $context.Arm7Port -State Listen `
            -ErrorAction SilentlyContinue |
            Where-Object { $_.OwningProcess -eq $emulator.Id } |
            Select-Object -First 1
        if ($null -eq $arm7Listener) { Start-Sleep -Milliseconds 250 }
    }
    Assert-Result ($arm9Listener.OwningProcess -eq $emulator.Id -and
        $null -ne $arm7Listener) 'Runner listeners are not owned by the isolated melonDS process.'

    Start-Sleep -Seconds ([Math]::Max(1, $DelaySeconds))
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break syAudioUpdateBGMState if gNdsAudioBgmResultsPlayCount == 1',
        'continue',
        'delete breakpoints',
        'printf "AUDIO_CORE=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioBgmResult, gNdsAudioBgmMask, gNdsAudioBgmPlaying, gNdsAudioBgmTrackID, gNdsAudioBgmPlayCalls, gNdsAudioBgmStopCalls, gNdsAudioBgmCheckCalls, gNdsAudioBgmOpenFailCount, gNdsAudioBgmReadFailCount, gNdsAudioBgmUnsupportedTrackCount, gNdsAudioBgmReadBytes, gNdsAudioBgmResidentBytes, gNdsAudioBgmChunkBytes, gNdsAudioBgmChunkPlayCount, gNdsAudioBgmElapsedFrames, gNdsAudioBgmRefillCount, gNdsAudioBgmUnsafeWriteCount, gNdsAudioBgmOverrunCount, gNdsAudioBgmStreamBytesPerSecond, gNdsAudioBgmExpectedBytesPerSecond, gNdsAudioBgmLoopCount',
        'printf "AUDIO_RESULTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioBgmStreamBytes, gNdsAudioBgmLoopStartBytes, gNdsAudioBgmIsLooping, gNdsAudioBgmPupupuPlayCount, gNdsAudioBgmWinMarioPlayCount, gNdsAudioBgmWinFoxPlayCount, gNdsAudioBgmResultsPlayCount, gNdsAudioBgmNaturalStopCount, gNdsAudioBgmLastNaturalStopTrackID, gNdsAudioBgmPostNaturalTransitionCount, gNdsAudioBgmPostNaturalTransitionFromTrackID, gNdsAudioBgmPostNaturalTransitionToTrackID, gNdsAudioBgmTrackSwitchCount, gNdsAudioBgmFinitePaddingBytes',
        'printf "AUDIO_LIFE=%u,%u,%u,%u,%u,%d,%#x,%d\n", gNdsAudioBgmFileOpen, gNdsAudioBgmSoundActive, gNdsAudioBgmPlayFailCount, gNdsAudioBgmErrorStopCount, gNdsAudioBgmErrorCleanupFailCount, gSYAudioCSPlayers[0]->state, sNdsAudioBgmFile, sNdsAudioBgmSoundID',
        'printf "MEMARENA=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerGeneration, gNdsMemoryLedgerArenaCapacity, gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater, gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerDLBytes, gNdsMemoryLedgerGraphicsBytes, gNdsMemoryLedgerRdpBytes, gNdsMemoryLedgerFigatreeHeapSize',
        'printf "VSB_END=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleLifecycleResult, gNdsSCVSBattleLifecycleArenaAdapterCount, gNdsSCVSBattleLifecycleTaskmanExitCount, gNdsSCVSBattleLifecycleTaskmanStatus, gNdsSCVSBattleLifecycleTimeLimit, gNdsSCVSBattleLifecycleTimeRemain, gNdsSCVSBattleLifecycleTimePassed, gNdsSCVSBattleLifecycleGameStatus, gNdsSCVSBattleLifecycleScenePrev, gNdsSCVSBattleLifecycleSceneCurr, gNdsSCVSBattleLifecycleIsSuddenDeath',
        'printf "VS_RESULTS=%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsVSResultsResult, gNdsVSResultsMask, gNdsVSResultsStartCount, gNdsVSResultsTickCount, gNdsVSResultsLoadedFileCount, gNdsVSResultsFighterCount, gNdsVSResultsGObjCount, gNdsVSResultsSObjCount, gNdsVSResultsKind',
        'detach',
        'quit'
    )
    $gdbOutput = (Invoke-GdbMarkerScript `
        -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName $scriptName `
        -TimeoutSeconds $TimeoutSeconds).Stdout

    $coreMatch = [regex]::Match($gdbOutput, 'AUDIO_CORE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $resultsMatch = [regex]::Match($gdbOutput, 'AUDIO_RESULTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $lifeMatch = [regex]::Match($gdbOutput, 'AUDIO_LIFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+)')
    $memoryMatch = [regex]::Match($gdbOutput, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battleMatch = [regex]::Match($gdbOutput, 'VSB_END=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vsResultsMatch = [regex]::Match($gdbOutput, 'VS_RESULTS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    Assert-Result ($coreMatch.Success -and $resultsMatch.Success -and
        $lifeMatch.Success -and $memoryMatch.Success -and
        $battleMatch.Success -and $vsResultsMatch.Success) `
        'Results audio GDB marker set was incomplete.' $gdbOutput

    $core = Get-Ints $coreMatch
    $results = Get-Ints $resultsMatch
    $audioLife = Get-Ints $lifeMatch
    $memory = Get-Ints $memoryMatch
    $battle = Get-Ints $battleMatch
    $vsResults = Get-Ints $vsResultsMatch
    $winnerCount = $results[4] + $results[5]
    $winnerTrack = if ($results[4] -eq 1) { 12 } else { 16 }
    $winnerName = if ($winnerTrack -eq 12) { 'Mario' } else { 'Fox' }

    Assert-Result ($battle[0] -eq 0x5642454e -and $battle[4] -eq 1 -and
        $battle[5] -eq 0 -and $battle[6] -ge 3600 -and
        $battle[8] -eq 22 -and $battle[9] -eq 24) `
        'The natural one-minute source match did not reach VS Results.' $gdbOutput
    Assert-Result ($vsResults[0] -eq 0x56535231 -and
        (($vsResults[1] -band 0x1f) -eq 0x1f) -and
        $vsResults[3] -ge 120 -and $vsResults[4] -eq 8 -and
        $vsResults[5] -ge 2) 'The imported VS Results scene was not live.' $gdbOutput
    Assert-Result ($core[0] -eq 0x42474d31 -and $core[2] -eq 1 -and
        $core[3] -eq 22 -and $core[4] -eq 3 -and $core[5] -ge 1 -and
        $core[7] -eq 0 -and $core[8] -eq 0 -and $core[9] -eq 0 -and
        $core[11] -eq 65536 -and $core[12] -eq 65536 -and
        $core[16] -eq 0 -and $core[17] -eq 0 -and
        $core[18] -ge 42100 -and $core[18] -le 46100 -and
        $core[19] -eq 44100) `
        'BGM streamer failed its exact I/O, residency, rate, or safety contract.' $gdbOutput
    Assert-Result ($results[0] -eq 1624750 -and $results[1] -eq 34912 -and
        $results[2] -eq 1 -and $results[3] -eq 1 -and
        $winnerCount -eq 1 -and $results[6] -eq 1 -and
        $results[7] -eq 1 -and $results[8] -eq $winnerTrack -and
        $results[9] -eq 1 -and $results[10] -eq $winnerTrack -and
        $results[11] -eq 22 -and $results[12] -eq 0 -and
        $results[13] -gt 0) `
        'Winner BGM did not naturally stop and release the source Results thread into track 22.' $gdbOutput
    Assert-Result ($audioLife[0] -eq 1 -and $audioLife[1] -eq 1 -and
        $audioLife[2] -eq 0 -and $audioLife[3] -eq 0 -and
        $audioLife[4] -eq 0 -and $audioLife[5] -eq 1 -and
        $audioLife[6] -ne 0 -and $audioLife[7] -ge 0) `
        'BGM file/channel lifecycle or error-cleanup invariant failed.' $gdbOutput
    $reserveAfterAudio = $memory[6] - $core[11]
    Assert-Result ($memory[0] -eq 0x4d4c4544 -and $memory[1] -eq 22 -and
        $memory[3] -ge 0x130000 -and $reserveAfterAudio -ge 131072) `
        'Battle arena reserve fell below the audio-adjusted floor.' $gdbOutput

    $romItem = Get-Item -LiteralPath $rom
    $romSha = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant()
    $summaryFormat = 'Audio Results natural PASS: winner={0}({1}) transition={1}->22 ' +
        'plays={2}/pupupu{3}/mario{4}/fox{5}/results{6} natural={7} ' +
        'errors={8}/{9}/{10} unsafe={11} overrun={12} cleanup={13}/{14}/{15} ' +
        'read={16} refills={17} padding={18} resident={19} headroom={20} reserve_after_audio={21} ' +
        'results_tick={22} runner=slot{23}@{24}/{25} pid={26} rom={27}/{28}'
    $summary = $summaryFormat -f (
        $winnerName, $winnerTrack, $core[4], $results[3], $results[4],
        $results[5], $results[6], $results[7], $core[7], $core[8],
        $core[9], $core[16], $core[17], $audioLife[2], $audioLife[3],
        $audioLife[4], $core[10], $core[15], $results[13], $core[11],
        $memory[6], $reserveAfterAudio, $vsResults[3], $RunnerSlot,
        $context.GdbPort, $context.Arm7Port, $emulator.Id, $romItem.Length,
        $romSha)
}
finally {
    if ($null -ne $emulator) {
        try {
            if (-not $emulator.HasExited) { Stop-Process -Id $emulator.Id -Force }
            $emulator.WaitForExit(5000) | Out-Null
        } catch {
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
}

$remainingProcesses = @(Get-Process melonDS -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -eq $melonDsPath })
$remainingListeners = @(Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue |
    Where-Object { $_.LocalPort -in @($context.GdbPort, $context.Arm7Port) })
Assert-Result ($remainingProcesses.Count -eq 0 -and
    $remainingListeners.Count -eq 0) 'Isolated Results audio runner did not clean up.'
Write-Output $summary
Write-Output 'Audio Results runner cleanup PASS: process=0 listeners=0 host_volume=0.'
