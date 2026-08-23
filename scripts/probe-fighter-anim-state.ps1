[CmdletBinding()]
param(
    [string]$Build = 'build-p2-luigi-realtime',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600,
    # Calls of ftParamUpdateAnimKeys to let pass before the first sample. Two
    # fighters make two calls per logic tick, so 600 is ~300 ticks into the
    # battle (past the entry animations).
    [ValidateRange(0, 100000)][int]$SkipCalls = 600,
    # Samples taken, each one ftParamUpdateAnimKeys call apart, so consecutive
    # samples of the SAME fighter are Samples/fighters ticks apart.
    [ValidateRange(1, 64)][int]$Samples = 6,
    # Also walk each joint's AObj list (track/kind/length/base/target/rates), the
    # evidence that separates "parser never wrote a track" from "tracks carry
    # zero data" from "player never evaluated".
    [switch]$AObjs,
    [string]$Artifact = ''
)

# P2-3 fighter-admission probe: does fighter N animate at all?
#
# Breaks inside ftParamUpdateAnimKeys (the one site that parses and plays every
# fighter joint) and prints, for the fighter being updated, the FTStruct identity
# (kind/status/motion/joint counts/anim_desc), the figatree attach counters and
# every common joint's anim_wait / anim_frame / script pointer / rotation. A
# fighter with NO animation reads every joint at anim_wait == AOBJ_ANIM_NULL
# (-1.0e38-ish sentinel) or rotations that never move between samples; a fighter
# whose figatree failed to resolve shows
# gNdsFighterNaturalMotionFigatreeNullCount climbing with the attach count.
#
# Written for the owner's 2026-08-23 "last time I played Luigi, there were no
# animations" report; kept because every remaining fighter row needs the same
# question answered on admission.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_fighter-anim-state.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.fighter-anim-state.stdout.log'
$stderr = Join-Path $log_dir 'melonds.fighter-anim-state.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'ftParamUpdateAnimKeys',
    'gNdsFighterNaturalMotionFigatreeAttachCount',
    'gNdsFighterNaturalMotionFigatreeNullCount',
    'gNdsFighterNaturalMotionFigatreeAnimInvalidCount',
    'gNdsR2FtAnimParseCalls',
    'gNdsR2FtAnimNullSkips',
    'gNdsR2FtAnimParseEarlyOut',
    'gNdsR2FtAnimParseStepped'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Fighter anim-state probe symbols absent from {0}: {1}." -f
        $elf, ($missing -join ', '))
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
        'set $g = (GObj *)$r0',
        'set $fst = (FTStruct *)$g->user_data.p',
        'printf "ANIMSTATE sample=%d gobj=0x%x flags=0x%x kind=%d player=%d status=%d motion=%d frame=%f joints=%u common=%u desc=0x%x is_anim_joint=%d speed=%f\n", $s, $g, $g->flags, $fst->fkind, $fst->player, $fst->status_id, $fst->motion_id, $g->anim_frame, $fst->nds_joint_count, $fst->nds_common_joint_count, $fst->anim_desc.word, $fst->anim_desc.flags.is_anim_joint, $fst->anim_speed',
        'printf "ANIMSTATE counters attach=%u null=%u invalid=%u parse=%u nullskip=%u early=%u stepped=%u\n", gNdsFighterNaturalMotionFigatreeAttachCount, gNdsFighterNaturalMotionFigatreeNullCount, gNdsFighterNaturalMotionFigatreeAnimInvalidCount, gNdsR2FtAnimParseCalls, gNdsR2FtAnimNullSkips, gNdsR2FtAnimParseEarlyOut, gNdsR2FtAnimParseStepped',
        'set $i = 0',
        'while $i < 22',
        'set $j = $fst->joints[$i]',
        'if $j != 0',
        'printf "ANIMJOINT s=%d i=%d dobj=0x%x script=0x%x aobj=0x%x wait=%g speed=%g frame=%g rot=%g,%g,%g tra=%g,%g,%g\n", $s, $i, $j, $j->anim_joint.event16, $j->aobj, $j->anim_wait, $j->anim_speed, $j->anim_frame, $j->rotate.vec.f.x, $j->rotate.vec.f.y, $j->rotate.vec.f.z, $j->translate.vec.f.x, $j->translate.vec.f.y, $j->translate.vec.f.z',
        'if $aobjs',
        'set $a = $j->aobj',
        'while $a != 0',
        # Fighter AObjs carry Q fixed point bit-cast into the f32 slots
        # (nds_anim_fixed.h: kind >= 5), so the slots print as raw s32 words;
        # a float-kind AObj (kind < 5) reads the same words as IEEE bits.
        'printf "ANIMAOBJ s=%d i=%d aobj=0x%x track=%u kind=%u length=%d linv=%d base=%d target=%d rbase=%d rtarget=%d\n", $s, $i, $a, $a->track, $a->kind, *(int *)&$a->length, *(int *)&$a->length_invert, *(int *)&$a->value_base, *(int *)&$a->value_target, *(int *)&$a->rate_base, *(int *)&$a->rate_target',
        'set $a = $a->next',
        'end',
        'end',
        'else',
        'printf "ANIMJOINT s=%d i=%d dobj=0\n", $s, $i',
        'end',
        'set $i = $i + 1',
        'end'
    )
    $commands = [System.Collections.Generic.List[string]]::new()
    $commands.AddRange([string[]]@(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break ftParamUpdateAnimKeys',
        ("ignore 1 {0}" -f $SkipCalls),
        ('set $aobjs = {0}' -f $(if ($AObjs) { 1 } else { 0 })),
        'set $s = 0'
    ))
    for ($k = 0; $k -lt $Samples; $k++) {
        $commands.Add('continue')
        $commands.AddRange([string[]]$sample)
        $commands.Add('set $s = $s + 1')
    }
    $commands.AddRange([string[]]@('detach', 'quit'))

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
        -ScriptName 'fighter_anim_state_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    $captured = Join-Path $log_temp 'fighter_anim_state_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(ANIMSTATE|ANIMJOINT|ANIMAOBJ)' } |
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
