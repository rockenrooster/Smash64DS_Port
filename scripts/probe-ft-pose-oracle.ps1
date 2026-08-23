[CmdletBinding()]
param(
    [string]$Build = 'build-p2-pose-oracle',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    # ftParamUpdateAnimKeys calls between samples: two fighters make two calls
    # per 60 Hz tick, so 2000 is ~1000 ticks (~17 s of match) per sample.
    [ValidateRange(1, 1000000)][int]$CallsPerSample = 2000,
    [ValidateRange(1, 64)][int]$Samples = 4,
    [string]$Artifact = ''
)

# P2-2p6 fighter pose engine oracle readout.
#
# Boots a ROM built with NDS_FT_POSE=1 NDS_FT_POSE_ORACLE=1, in which the engine
# animates shadow copies of every fighter joint beside the generic parser/player
# and compares them bit for bit after every tick, and prints the engine's
# counters at each sample: binds, updates, joint ticks/evals/holds, track
# evaluations, and the oracle's compare/mismatch counts with the first
# mismatch's joint/field/want/got/frame. A whole match with Mismatches 0 over a
# non-trivial Compares count is the equivalence proof the engine ships on;
# any mismatch names the joint and field (0-2 rot, 3-5 tra, 6-8 sca, 9
# anim_wait, 10 anim_frame, 11 flags, 12 gobj anim_frame).

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_ft-pose-oracle.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.ft-pose-oracle.stdout.log'
$stderr = Join-Path $log_dir 'melonds.ft-pose-oracle.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'ftParamUpdateAnimKeys', 'gNdsFtPoseBinds', 'gNdsFtPoseBindFull',
    'gNdsFtPoseUpdates', 'gNdsFtPoseJointTicks', 'gNdsFtPoseJointEvals',
    'gNdsFtPoseJointHolds', 'gNdsFtPoseTrackEvals', 'gNdsFtPoseStepped',
    'gNdsFtPoseUnbinds', 'gNdsFtPoseOracleCompares',
    'gNdsFtPoseOracleMismatches', 'gNdsFtPoseOracleFirstJoint',
    'gNdsFtPoseOracleFirstField', 'gNdsFtPoseOracleFirstWant',
    'gNdsFtPoseOracleFirstGot', 'gNdsFtPoseOracleFirstFrame',
    'gNdsObjAnimRunawayCount', 'gNdsR2CubicSaturations',
    'gNdsFtPoseTrackOverflow', 'gNdsFtPoseAObjLiveMax'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Pose oracle symbols absent from {0}: {1}. Build with NDS_FT_POSE=1 " -f
        $elf, ($missing -join ', ')) + 'NDS_FT_POSE_ORACLE=1.'
}

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $sample = @(
        'printf "POSEORACLE sample=%d binds=%u full=%u unbinds=%u updates=%u jointticks=%u jointevals=%u jointholds=%u trackevals=%u stepped=%u runaway=%u saturations=%u overflow=%u aobjlivemax=%u\n", $s, gNdsFtPoseBinds, gNdsFtPoseBindFull, gNdsFtPoseUnbinds, gNdsFtPoseUpdates, gNdsFtPoseJointTicks, gNdsFtPoseJointEvals, gNdsFtPoseJointHolds, gNdsFtPoseTrackEvals, gNdsFtPoseStepped, gNdsObjAnimRunawayCount, gNdsR2CubicSaturations, gNdsFtPoseTrackOverflow, gNdsFtPoseAObjLiveMax',
        'printf "POSEORACLE compares=%u mismatches=%u first: joint=%u field=%u want=0x%08x got=0x%08x frame=0x%08x\n", gNdsFtPoseOracleCompares, gNdsFtPoseOracleMismatches, gNdsFtPoseOracleFirstJoint, gNdsFtPoseOracleFirstField, gNdsFtPoseOracleFirstWant, gNdsFtPoseOracleFirstGot, gNdsFtPoseOracleFirstFrame'
    )
    $commands = [System.Collections.Generic.List[string]]::new()
    $commands.AddRange([string[]]@(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break ftParamUpdateAnimKeys',
        'set $s = 0'
    ))
    for ($k = 0; $k -lt $Samples; $k++) {
        $commands.Add(("ignore 1 {0}" -f $CallsPerSample))
        $commands.Add('continue')
        $commands.AddRange([string[]]$sample)
        $commands.Add('set $s = $s + 1')
    }
    $commands.AddRange([string[]]@('detach', 'quit'))

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
        -ScriptName 'ft_pose_oracle_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    $captured = Join-Path $log_temp 'ft_pose_oracle_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^POSEORACLE' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
