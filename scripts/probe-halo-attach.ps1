[CmdletBinding()]
param(
    [string]$Build = 'build-c59-flag1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [string]$Artifact = ''
)

# WHERE DOES THE REBIRTH HALO GO, AND IS IT EVEN BUILT?
#
# Cycle 59 measured that nothing halo-shaped is on screen in EITHER arm at its
# own spawn tic, with a 0.16% whole-viewport cross-build delta against a 42.9%
# same-build floor -- so the row is an EXISTENCE question, not an appearance one.
# This walks created -> alive -> attached -> submitted on the existing ROM.
#
# The source oracle says where to look. dEFManagerRebirthHaloEffectDesc's root
# carries main matrix kind 0x50 (efmanager.c:1655), NOT the shield's 0x4F, and
# sGCMatrixFuncList is an array of {proc_diff, proc_same} PAIRS
# (objdisplay.h:20), so kind - 66 selects pair 14 of dLBCommonFuncMatrixList =
# func_ovl0_800C99CC, not pair 13's func_ovl0_800C994C. Different callback,
# different maths: 0x50 is a PURE TRANSLATION to the attached joint's world
# position and writes no gGCScaleX at all.
#
# Prediction this probe exists to confirm or kill: k0 reads 0x50, the port has
# no case for it, the root falls to ndsRendererAdapterBuildDObjFallbackMtx, and
# the halo's world translate is therefore the ORIGIN rather than the respawning
# fighter's Top joint.
#
# THE HALO IS FOUND BY ITS MATRIX KIND, NOT BY ITS MAKER, and that is deliberate
# on two counts.
#
# `break *efManagerRebirthHaloMakeEffect` DOES NOT WORK on this ELF. The name has
# two definitions -- the routed strong one in battleship_efmanager.c and an
# NDS_WEAK substitute stub in battle_playable_compat_stubs.c:141 -- and gdb
# resolves the name through DEBUG INFO, which still describes the weak one even
# though --gc-sections discarded its code. The result is "Breakpoint 1 at 0x0:
# file battle_playable_compat_stubs.c, line 326", a breakpoint that can never
# fire, and the only symptom is the whole run timing out. nm reports the live
# address (T 0x02095a24) and disagrees; nm wins. Same family as addr2line naming
# deleted functions.
#
# Conditioning on `xobjs[0]->kind == 0x50` also tests the hypothesis directly:
# it asks whether ANY DObj carrying the halo's matrix kind reaches the local
# matrix builder and the DL submitter, without plumbing a pointer through a
# `finish` (which is not reliable inside a breakpoint command list). The guard
# checks xobjs_num and the pointer first, because most stage DObjs have neither.
# Everything printed is a register or a file-static -- no stack object is read
# (BUGS.md, cycle 55).

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
        (Get-Date -Format 'yyyy-MM-dd') + '_halo-attach-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.halo-attach-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.halo-attach-probe.stderr.log'
$config_state = $null
$emulator = $null

# A gdb batch aborts every command after the first absent symbol, so the whole
# read set is checked, not only the breakpoint targets.
$required = @(
    'efManagerRebirthHaloMakeEffect',
    'ndsRendererAdapterBuildDObjLocalMatrix',
    'ndsRendererAdapterSubmitStageDL',
    'gSCManagerBattleState',
    'gNdsEffectRendererDObjDrawCount',
    'gNdsEffectRendererRejectedDrawCount',
    'gNdsEffectRendererTriangleCount',
    'gNdsEffectRendererTextureReadyCount',
    'sNdsRendererAdapterStageWorldCache',
    'sNdsRendererAdapterStageWorldCacheCount',
    'sNdsRendererAdapterDObjWorldCache'
)
$nmLines = & $nm $elf
$symbols = $nmLines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Halo attach probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# nm, not gdb's name lookup -- see the note above on the weak twin at 0x0. Take
# the text symbol specifically, so a same-named data symbol cannot be picked up.
$makerLine = $nmLines |
    Where-Object { $_ -match '^([0-9a-fA-F]{8})\s+[Tt]\s+efManagerRebirthHaloMakeEffect$' } |
    Select-Object -First 1
if ($null -eq $makerLine) {
    throw "efManagerRebirthHaloMakeEffect has no text symbol in $elf."
}
$makerAddress = '0x' + ($makerLine -split '\s+')[0]

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

        'set $locals = 0',
        'set $dls = 0',
        'set $spawns = 0',

        ("break *{0}" -f $makerAddress),
        'break *ndsRendererAdapterBuildDObjLocalMatrix',
        'break *ndsRendererAdapterSubmitStageDL',
        'set $hroot = 0',
        'set $hchild = 0',
        'condition 2 (((DObj *)$r0)->xobjs_num > 0) && (((DObj *)$r0)->xobjs[0] != 0) && (((DObj *)$r0)->xobjs[0]->kind == 0x50)',
        # The GEOMETRY is on the child, whose kind is nGCMatrixKindTraRotRpyRSca
        # like a thousand other DObjs, so the submit cannot be found by kind. It
        # is found by the pointer the root breakpoint just captured.
        'condition 3 ($hroot != 0) && (($r0 == $hroot) || ($r0 == $hchild))',
        # Prove the breakpoints resolved. A breakpoint at 0x0 is what a
        # gc-sectioned weak twin looks like, and without this line its only
        # symptom is the whole run timing out with no explanation.
        'info breakpoints',

        'commands 1',
        'silent',
        'set $spawns = $spawns + 1',
        'printf "HALOSPAWN n=%d tr=%d fighter=%p draw=%u reject=%u tris=%u texready=%u\n", $spawns, gSCManagerBattleState->time_remain, $r0, gNdsEffectRendererDObjDrawCount, gNdsEffectRendererRejectedDrawCount, gNdsEffectRendererTriangleCount, gNdsEffectRendererTextureReadyCount',
        'continue',
        'end',

        'commands 2',
        'silent',
        'set $locals = $locals + 1',
        'set $hroot = (DObj *)$r0',
        'set $hchild = (DObj *)((DObj *)$r0)->child',
        # k0 == 0x50 by construction. udp is the Top joint the maker stored, and
        # its FTParts world translation is WHERE THE HALO SHOULD BE -- the value
        # the 0x50 case now feeds to syMatrixTra. Printing it here makes "the
        # matrix is right but the platform is off camera" and "the matrix is
        # still the origin" different-looking results instead of the same blank
        # screenshot.
        'printf "HALOLOCAL n=%d dobj=%p child=%p tr=%d udp=%p flags=0x%x hidden=%d\n", $locals, $r0, $hchild, gSCManagerBattleState->time_remain, ((DObj *)$r0)->user_data.p, ((DObj *)$r0)->flags, (int)(((DObj *)$r0)->flags & 1)',
        'set $aj = (DObj *)((DObj *)$r0)->user_data.p',
        'if $aj != 0',
        # pgobj is the guard the 0x50 builder tests before calling
        # gmCollisionGetFighterPartsWorldPosition; a NULL one silently routes the
        # root back to the fallback identity, which looks exactly like "the case
        # is missing". mtx_translate is printed only as a STALENESS control --
        # 0x50 reads unk_dobjtrans_0x10 through gmCollision*, not this cache, so
        # a value that never changes across tics is the cache being stale, not
        # the fighter standing still.
        'printf "HALOJOINT pgobj=%p parts=%p mtxt=%f,%f,%f\n", $aj->parent_gobj, (FTParts *)$aj->user_data.p, ((FTParts *)$aj->user_data.p)->mtx_translate[3][0], ((FTParts *)$aj->user_data.p)->mtx_translate[3][1], ((FTParts *)$aj->user_data.p)->mtx_translate[3][2]',
        'end',
        'if $hchild != 0',
        'printf "HALOCHILD dl=%p dllink=%p flags=0x%x sca=%f xnum=%d\n", $hchild->dl, $hchild->dl_link, $hchild->flags, $hchild->scale.vec.f.x, $hchild->xobjs_num',
        'end',
        'if ($locals + $dls) < 24',
        'continue',
        'end',
        'end',

        'commands 3',
        'silent',
        'set $dls = $dls + 1',
        'printf "HALOSUBMIT n=%d dobj=%p dl=%p tr=%d tris=%u texready=%u\n", $dls, $r0, $r1, gSCManagerBattleState->time_remain, gNdsEffectRendererTriangleCount, gNdsEffectRendererTextureReadyCount',
        # The world matrix the prep actually built, read out of the file-static
        # stage-world cache rather than any argument register. A translation at
        # or near 0,0,0 is the fallback identity; the respawning fighter's Top
        # joint is not at the origin.
        'set $wi = 0',
        'while $wi < sNdsRendererAdapterStageWorldCacheCount',
        'if (sNdsRendererAdapterStageWorldCache[$wi].dobj == $hroot) || (sNdsRendererAdapterStageWorldCache[$wi].dobj == $hchild)',
        'set $ws = sNdsRendererAdapterStageWorldCache[$wi].world_slot',
        'printf "HALOWORLD i=%d dobj=%p slot=%d t=%d,%d,%d r0=%d,%d,%d\n", $wi, sNdsRendererAdapterStageWorldCache[$wi].dobj, $ws, sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[3][2], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][0], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][1], sNdsRendererAdapterDObjWorldCache[$ws].world.m[0][2]',
        'end',
        'set $wi = $wi + 1',
        'end',
        'if ($locals + $dls) < 24',
        'continue',
        'end',
        'end',

        'continue',
        'printf "HALODONE spawns=%d locals=%d dls=%d draw=%u reject=%u tris=%u texready=%u\n", $spawns, $locals, $dls, gNdsEffectRendererDObjDrawCount, gNdsEffectRendererRejectedDrawCount, gNdsEffectRendererTriangleCount, gNdsEffectRendererTextureReadyCount',
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'halo_attach_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $capture.Stdout
    $capture.Stdout -split "`n" |
        Where-Object { $_ -match 'HALO' } |
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
