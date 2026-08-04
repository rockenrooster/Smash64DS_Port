[CmdletBinding()]
param(
    [string]$Build = 'build-c52-flag1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [string]$Artifact = ''
)

# DOES THE SHIELD TREE REACH THE MATRIX BUILDER AND THE DL SUBMITTER AT ALL?
#
# The 0x4F fix changed the shield root's local matrix from identity to the
# fighter joint's world matrix (measured: translate 109.7/1842/-36, scale 9.33)
# and the rendered top screen did not move by one pixel. Either the shield tree
# never reaches ndsRendererAdapterBuildDObjLocalMatrix, or it never reaches
# ndsRendererAdapterSubmitStageDL, or what is on screen is not this effect.
#
# Both are static functions inlined away as callees, so the probe conditions on
# the DObj pointer instead: the shield's display proc hands over the root, and
# the two breakpoints below arm only after that.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_shield-submit-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.shield-submit-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.shield-submit-probe.stderr.log'
$config_state = $null
$emulator = $null

$required = @(
    'efManagerShieldProcDisplay',
    'ndsRendererAdapterBuildDObjLocalMatrix',
    'ndsRendererAdapterSubmitStageDL',
    'gSCManagerBattleState',
    'gNdsEffectRendererTriangleCount'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Shield submit probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $sroot = 0',
        'set $schild = 0',
        'set $locals = 0',
        'set $dls = 0',
        'set $execs = 0',
        'set $sdl = 0',

        'break *efManagerShieldProcDisplay',
        'break *ndsRendererAdapterBuildDObjLocalMatrix',
        'break *ndsRendererAdapterSubmitStageDL',
        'break *ndsRendererExecuteDisplayListWithVertexCache',
        'break *ndsRendererAdapterPrepareInitialMatrices',
        'condition 5 ($schild != 0) && ($r0 == $schild)',
        'disable 5',
        'condition 2 ($sroot != 0) && (($r0 == $sroot) || ($r0 == $schild))',
        'condition 3 ($sroot != 0) && (($r0 == $sroot) || ($r0 == $schild))',
        'condition 4 $sroot != 0',
        'disable 2',
        'disable 3',
        'disable 4',

        'commands 1',
        'silent',
        'set $sroot = (DObj *)((GObj *)$r0)->obj',
        'set $schild = (DObj *)$sroot->child',
        'printf "ARMED tr=%d root=%p child=%p\n", gSCManagerBattleState->time_remain, $sroot, $schild',
        'enable 2',
        'enable 3',
        'enable 5',
        'disable 1',
        'continue',
        'end',

        'commands 2',
        'silent',
        'set $locals = $locals + 1',
        'printf "LOCAL n=%d dobj=%p tr=%d\n", $locals, $r0, gSCManagerBattleState->time_remain',
        # The 0x4F helper''s own guards, read AT BUILD TIME rather than at the
        # display proc: attach, its parent_gobj, its FTParts, and the world
        # matrix row the helper copies. Whichever of these is 0 is the reason
        # the helper returned FALSE and the root fell back to identity.
        'if $r0 == $sroot',
        'set $a = (DObj *)((DObj *)$r0)->user_data.p',
        'printf "GUARD att=%p pgobj=%p parts=%p\n", $a, $a->parent_gobj, (FTParts *)$a->user_data.p',
        'printf "GUARDW k0=0x%x w=%f,%f,%f r0x=%f\n", ((DObj *)$r0)->xobjs[0]->kind, ((FTParts *)$a->user_data.p)->mtx_translate[3][0], ((FTParts *)$a->user_data.p)->mtx_translate[3][1], ((FTParts *)$a->user_data.p)->mtx_translate[3][2], ((FTParts *)$a->user_data.p)->mtx_translate[0][0]',
        'end',
        'if ($locals + $dls) < 12',
        'continue',
        'end',
        'end',

        'commands 3',
        'silent',
        'set $dls = $dls + 1',
        'set $sdl = (Gfx *)$r1',
        'printf "SUBMIT n=%d dobj=%p dl=%p tris=%u\n", $dls, $r0, $r1, gNdsEffectRendererTriangleCount',
        'enable 4',
        'if ($locals + $dls) < 12',
        'continue',
        'end',
        'end',

        'commands 4',
        'silent',
        'set $cfg = (NDSRendererConfig *)$r1',
        'set $mv = (NDSRendererMatrix20p12 *)$cfg->initial_modelview',
        'printf "EXEC dl=%p shielddl=%p mv=%p proj=%p depth=%u cmds=%u geom=%u user=%p\n", $r0, $sdl, $mv, $cfg->initial_projection, $cfg->max_depth, $cfg->max_commands, $cfg->initial_geometry_mode, $cfg->user',
        'x/8xw $cfg',
        'if $mv != 0',
        'printf "MV t=%d,%d,%d r0=%d,%d,%d\n", $mv->m[3][0], $mv->m[3][1], $mv->m[3][2], $mv->m[0][0], $mv->m[0][1], $mv->m[0][2]',
        'end',
        'set $execs = $execs + 1',
        'if $execs < 14',
        'continue',
        'end',
        'end',

        # r0-r2 only. The three stacked arguments (projection_ptr, modelview,
        # modelview_ptr) were read here from $sp/$sp+8 and came back 0x1 and a
        # rotating pair of addresses -- stack reads are not sound on this
        # remote, and the config read at breakpoint 4 answers the same question
        # from a pointer deref instead.
        'commands 5',
        'silent',
        'printf "PREP dobj=%p cobj=%p persist=%d\n", $r0, $r1, $r2',
        'continue',
        'end',

        'continue',
        'printf "SUBMITDONE locals=%d dls=%d execs=%d tris=%u dobjdraw=%u reject=%u\n", $locals, $dls, $execs, gNdsEffectRendererTriangleCount, gNdsEffectRendererDObjDrawCount, gNdsEffectRendererRejectedDrawCount',
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'shield_submit_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $capture.Stdout
    $capture.Stdout -split "`n" |
        Where-Object { $_ -match 'ARMED|LOCAL|SUBMIT' } |
        ForEach-Object { Write-Output $_ }
    Write-Output "probe capture: $Artifact"
}
finally {
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
