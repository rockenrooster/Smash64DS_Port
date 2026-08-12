[CmdletBinding()]
param(
    [string]$Build = 'build-c131-position',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 1500,
    [ValidateRange(1, 32)][int]$FlameHits = 6,
    [string]$Artifact = ''
)

# The batched diagnostic for both 2026-08-12 attachment-position rows
# (artifacts/bugs/2026-08-12_r2-07-position/CONTRACT.md). ONE build, ONE run,
# both bugs, and the ROM is behaviourally identical to the one the owner played
# because NDS_R2_POSITION_PROBE only records.
#
# PREDICTIONS, written before the run so a surprise is legible.
#
# FOX -- the A-vs-B invariant.
#   A = gmCollisionGetFighterPartsWorldPosition(joints[17], {60,0,0}), which is
#       simultaneously the beam origin and the muzzle-flash position, because
#       wpFoxBlasterMakeWeapon hands the same vector to
#       efManagerFoxBlasterGlowMakeEffect.
#   B = func_ovl2_800EDBA4(joints[17]) then that same local {60,0,0} through
#       parts->mtx_translate -- the arm the visible gun overlay uses.
#   * If the shared fighter-parts cache is sound, DX/DY/DZ are ~0 and the
#     remaining error is in the gun mesh's own muzzle vertex, not in gameplay.
#   * If DY is materially non-zero, the two source-equivalent routes disagree
#     and the defect is the cache seam, NOT Fox. Under that outcome Fox's file
#     must not be touched and no pos.y += anything may be added.
#   * gNdsFoxSpawnAnimLocks selects which arm A took. With it 0 AND
#     gNdsFoxSpawnTrans5 0, A walked the parent chain applying each joint's
#     LOCAL matrix and never rebuilt a world matrix -- the case where a
#     descendant left at transform_update_mode 1 by
#     ftParamsUpdateFighterPartsTransformAll (reset_mode FALSE on descendants,
#     unlike source ftparam.c:2283) reuses a local matrix built on an earlier
#     frame. gNdsFoxSpawnUpdateMode is that field, read at the spawn.
#
# FIRE -- the joint rotation the port never performs.
#   The probe SHADOWS ftParamGetEffectJointPosition on its own counter, so the
#   fighter's effect_joint_array_id is untouched and flames still spawn where
#   the played ROM put them.
#   * SelIndex must advance and wrap at 5; JointID must take up to five DISTINCT
#     source joint ids from attr->effect_joint_ids.
#   * Joint X/Y/Z must MOVE between emissions -- that is the burn walking the
#     body, which is what the owner is not seeing.
#   * Generic X/Y/Z is what the shipped code actually uses. Expect it roughly
#     constant across the burst; that constancy IS the bug.
#   * If the joint positions are themselves displaced by the same delta Fox
#     reports, fix the shared helper FIRST -- restoring the rotation over a
#     broken helper scatters flames across five wrong places and measures like
#     progress.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_attachment-position.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.attachment-position.stdout.log'
$stderr = Join-Path $log_dir 'melonds.attachment-position.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'battleship_wpFoxBlasterMakeWeapon',
    'gNdsFoxSpawnAX', 'gNdsFoxSpawnBX', 'gNdsFoxSpawnDX',
    'gNdsFoxSpawnAnimLocks', 'gNdsFoxSpawnUpdateMode', 'gNdsFoxSpawnTrans5',
    'gNdsFoxSpawnProbeCount',
    'gNdsFlameProbeCount', 'gNdsFlameProbeJointID', 'gNdsFlameProbeJointX',
    'gNdsFlameProbeSelIndex', 'gNdsFlameProbeGenericX',
    'efManagerFlameLRMakeEffect', 'efManagerFlameRandomMakeEffect',
    'gSCManagerBattleState'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Position probe symbols absent from {0}: {1}. Build with " -f
        $elf, ($missing -join ', ')) +
        'NDS_R2_POSITION_PROBE=1; a missing probe global is usually --gc-sections.'
}

# The arm is part of the trigger, same rule and same reason as
# probe-flame-quad-miss.ps1: this probe needs Fox to fire Neutral-B AND Mario to
# land a fire hit, and at NDS_R2_BOTH_CPU=0 Mario is a human who never moves.
$configHeader = Join-Path $root "builds\$Build\nds_build_config.h"
if (Test-Path -LiteralPath $configHeader -PathType Leaf) {
    $seen = [regex]::Match(
        (Get-Content -LiteralPath $configHeader -Raw),
        '(?m)^#define\s+NDS_R2_BOTH_CPU\s+(\d+)')
    if ($seen.Success -and ([int]$seen.Groups[1].Value -eq 0)) {
        throw ('This ROM is NDS_R2_BOTH_CPU=0. Nobody attacks, so neither the ' +
               'blaster spawn nor the fire burn happens. Rebuild with ' +
               'NDS_R2_BOTH_CPU=1.')
    }
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

    # battleship_wpFoxBlasterMakeWeapon, not the port wrapper: the probe runs in
    # the wrapper BEFORE this call, so every Fox global is already written when
    # this breakpoint fires, and reading them at the wrapper's entry would read
    # the previous shot.
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'break battleship_wpFoxBlasterMakeWeapon',
        'continue',
        'printf "FOXPOS n=%u tr=%d animlocks=%u mode=%d trans5=%u\n", gNdsFoxSpawnProbeCount, gSCManagerBattleState->time_remain, gNdsFoxSpawnAnimLocks, gNdsFoxSpawnUpdateMode, gNdsFoxSpawnTrans5',
        'printf "FOXPOS A %f %f %f\n", gNdsFoxSpawnAX, gNdsFoxSpawnAY, gNdsFoxSpawnAZ',
        'printf "FOXPOS B %f %f %f\n", gNdsFoxSpawnBX, gNdsFoxSpawnBY, gNdsFoxSpawnBZ',
        'printf "FOXPOS D %f %f %f\n", gNdsFoxSpawnDX, gNdsFoxSpawnDY, gNdsFoxSpawnDZ',
        'continue',
        'printf "FOXPOS n=%u tr=%d animlocks=%u mode=%d trans5=%u\n", gNdsFoxSpawnProbeCount, gSCManagerBattleState->time_remain, gNdsFoxSpawnAnimLocks, gNdsFoxSpawnUpdateMode, gNdsFoxSpawnTrans5',
        'printf "FOXPOS A %f %f %f\n", gNdsFoxSpawnAX, gNdsFoxSpawnAY, gNdsFoxSpawnAZ',
        'printf "FOXPOS B %f %f %f\n", gNdsFoxSpawnBX, gNdsFoxSpawnBY, gNdsFoxSpawnBZ',
        'printf "FOXPOS D %f %f %f\n", gNdsFoxSpawnDX, gNdsFoxSpawnDY, gNdsFoxSpawnDZ',

        # Both Flame makers, because which arrives first depends on the burn's
        # strength -- the FlameLR-only version of this waited out a whole run.
        'delete breakpoints',
        'set $f = 0',
        # `break *symbol`, NOT `break symbol`. Both Flame makers are thin
        # wrappers and GDB's line table folds a plain symbol breakpoint into
        # efManagerGetNextStructAlloc at efmanager.c:1766 -- a shared allocator
        # that has no `pos` parameter at all. Printing pos->x there reads
        # whatever unrelated symbol named `pos` is in scope, which is exactly how
        # this probe published a bogus "X and Z are swapped and negated"
        # conclusion that had to be retracted. The entry address cannot be
        # folded.
        'break *efManagerFlameLRMakeEffect',
        'break *efManagerFlameRandomMakeEffect',
        'commands 1-2',
        'silent',
        'set $f = $f + 1',
        # THE ARM-DISTINGUISHING READING, and the reason it exists: the ring
        # columns below CANNOT tell the two arms apart. `generic` is the position
        # the override replaced and `joint` is the source-selected joint, and
        # both are identical with the fix in or out -- the first run of this
        # probe printed byte-identical tables for build-c131-position (no
        # override) and build-c132-flamejoint (override), which is "one run
        # relabelled", not agreement. What the MAKER receives is the only value
        # that moves: feet (Y == 0) without the fix, a body joint (Y != 0) with
        # it.
        'printf "FLAMEARG f=%d pos %f %f %f\n", $f, pos->x, pos->y, pos->z',
        ('if $f < ' + $FlameHits),
        'continue',
        'end',
        'end',
        'continue',
        'printf "FLAMEPOS count=%u\n", gNdsFlameProbeCount',
        'printf "FLAMEPOS ring kind selidx jointid  jointXYZ  genericXYZ\n"',
        'set $i = 0',
        'while $i < 8',
        'printf "FLAMEPOS %d kind=%u sel=%u joint=%d  %f %f %f  %f %f %f\n", $i, gNdsFlameProbeKind[$i], gNdsFlameProbeSelIndex[$i], gNdsFlameProbeJointID[$i], gNdsFlameProbeJointX[$i], gNdsFlameProbeJointY[$i], gNdsFlameProbeJointZ[$i], gNdsFlameProbeGenericX[$i], gNdsFlameProbeGenericY[$i], gNdsFlameProbeGenericZ[$i]',
        'set $i = $i + 1',
        'end',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'attachment_position_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    $captured = Join-Path $log_temp 'attachment_position_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(FOXPOS|FLAMEPOS|FLAMEARG)' } |
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
