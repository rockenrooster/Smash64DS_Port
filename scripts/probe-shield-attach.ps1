[CmdletBinding()]
param(
    [string]$Build = 'build-c50-flag1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [ValidateRange(1, 64)][int]$Hits = 6,
    [string]$Artifact = ''
)

# WHAT THE SHIELD'S ROOT DObj ACTUALLY RESOLVES TO, AGAINST ITS BOUND JOINT.
#
# dEFManagerShieldEffectDesc's first DObj transform is main matrix kind 0x4F
# (efmanager.c:462).  0x4F is not an XObjTransformKind enumerator -- it is
# >= 66, so source routes it through sGCMatrixFuncList[0x4F - 66] =
# dLBCommonFuncMatrixList pair 13 = func_ovl0_800C994C (lbcommon.c:1445), which
# ignores the DObj's own translate/rotate/scale entirely and loads the matrix
# from the ATTACHED joint's world matrix:
#
#     attach_dobj = dobj->user_data.p;              // fp->joints[YRotN]
#     parts       = attach_dobj->user_data.p;
#     func_ovl2_800EDBA4(attach_dobj);              // refresh the chain
#     gmCollisionCopyMatrix(f, parts->mtx_translate);
#     syMatrixF2LFixedW(&f, mtx);
#
# efManagerShieldMakeEffect (efmanager.c:4139) stores that joint in
# DObjGetStruct(effect_gobj)->user_data.p, which is the root DObj this probe
# reads.  The port's ndsRendererAdapterBuildDObjXObjMatrix has no 0x4F case, so
# 0x4F lands in `default:` -> ndsRendererAdapterBuildDObjFallbackMtx, i.e.
# TraRotRpyRSca of the root DObj's OWN vectors.
#
# So the confirmation is one comparison, printed per hit:
#   ROOTXF  translate/rotate/scale the port's fallback would use
#   JOINT   parts->mtx_translate row 3, the world position source would use
# If those disagree, the shield is drawn in the wrong space and the seam is the
# missing 0x4F case, not the effect's update proc.
#
# Break is on efManagerShieldProcDisplay's FIRST INSTRUCTION (`break *sym`), not
# on the symbol, because only there does the ABI guarantee r0 == effect_gobj.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_shield-attach-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.shield-attach-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.shield-attach-probe.stderr.log'
$config_state = $null
$emulator = $null

# gdb abandons a command file after its first error, so an absent symbol costs
# the whole run silently.
$required = @(
    'efManagerShieldProcDisplay',
    'gSCManagerBattleState',
    'gNdsEffectRendererLink15DrawCount'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Shield attach probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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

        'set $hits = 0',
        'break *efManagerShieldProcDisplay',
        'commands',
        'silent',
        'set $hits = $hits + 1',
        'set $g = (GObj *)$r0',
        'set $root = (DObj *)$g->obj',
        'set $att = (DObj *)$root->user_data.p',
        'printf "ATTACH hit=%d tr=%d root=%p xn=%d att=%p link15=%u\n", $hits, gSCManagerBattleState->time_remain, $root, $root->xobjs_num, $att, gNdsEffectRendererLink15DrawCount',
        'if $root->xobjs_num > 0',
        'printf "ATTACHK k0=0x%x\n", $root->xobjs[0]->kind',
        'end',
        'if $root->xobjs_num > 1',
        'printf "ATTACHK k1=0x%x\n", $root->xobjs[1]->kind',
        'end',
        'printf "ROOTXF t=%f,%f,%f r=%f,%f,%f s=%f,%f,%f\n", $root->translate.vec.f.x, $root->translate.vec.f.y, $root->translate.vec.f.z, $root->rotate.vec.f.x, $root->rotate.vec.f.y, $root->rotate.vec.f.z, $root->scale.vec.f.x, $root->scale.vec.f.y, $root->scale.vec.f.z',
        'if $att != 0',
        'set $parts = (FTParts *)$att->user_data.p',
        'printf "JOINTXF t=%f,%f,%f parts=%p mode=%d\n", $att->translate.vec.f.x, $att->translate.vec.f.y, $att->translate.vec.f.z, $parts, $parts->transform_update_mode',
        'printf "JOINTW  w=%f,%f,%f r0=%f,%f,%f\n", $parts->mtx_translate[3][0], $parts->mtx_translate[3][1], $parts->mtx_translate[3][2], $parts->mtx_translate[0][0], $parts->mtx_translate[0][1], $parts->mtx_translate[0][2]',
        'end',
        # THE TREE, because a root whose 0x4F is correct still moves nothing if
        # the geometry node does not reach it through ->parent. The world-matrix
        # builder walks child->parent and stops at NULL, so a child left with a
        # NULL parent silently drops every ancestor transform.
        'set $n = (DObj *)$root->child',
        'set $d = 0',
        'while ($n != 0) && ($d < 6)',
        'printf "NODE d=%d dobj=%p parent=%p sib=%p child=%p dv=%p flags=0x%x xn=%d\n", $d, $n, $n->parent, $n->sib_next, $n->child, $n->dv, $n->flags, $n->xobjs_num',
        'if $n->xobjs_num > 0',
        'printf "NODEK d=%d k0=0x%x\n", $d, $n->xobjs[0]->kind',
        'end',
        'set $n = (DObj *)$n->child',
        'set $d = $d + 1',
        'end',
        ('if $hits < ' + $Hits),
        'continue',
        'end',
        'end',

        'continue',
        'printf "ATTACHDONE hits=%d\n", $hits',
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'shield_attach_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $capture.Stdout
    $capture.Stdout -split "`n" |
        Where-Object { $_ -match 'ATTACH|ROOTXF|JOINT' } |
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
