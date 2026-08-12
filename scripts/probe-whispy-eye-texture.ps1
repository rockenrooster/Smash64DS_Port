[CmdletBinding()]
param(
    [string]$Build = 'build-c128-foxgun',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600,
    [ValidateRange(1, 64)][int]$Changes = 12,
    [string]$Artifact = ''
)

# BUGS.md "Whispy's face looks like it plays at low FPS" -- which half is broken.
#
# The whole-match read already settled that the blink texture is NOT missing:
# lookup misses 0, conversion calls 0, conversion ticks 0, pinned static hits
# 70,072. Something resolves to a resident entry every time. Two very different
# causes produce exactly that, and they have opposite fixes:
#
#   (a) KEY UNDER-DISCRIMINATION -- the eye image changes but the renderer's
#       texture key does not carry the differing word, so all three states hash
#       to the open eye. Fix: widen the key, then add residency.
#   (b) THE IMAGE NEVER CHANGES -- `texture_id_curr` stays put, so there is no
#       differing word to carry and the renderer is behaving correctly. Fix is
#       upstream, in whatever should be animating it.
#
# Reading the port settles the shape of the question: the material builder DOES
# index `mobj->sub.sprites[texture_id_curr]` (reloc_backend_renderer_dl.c ~7997),
# so a moving id WOULD move the image pointer and therefore the key. That makes
# (b) the live hypothesis and (a) the one the contract assumed. Do not fix either
# on that reasoning -- read the id.
#
# NO REBUILD, and every expression is rooted at the global `gGRCommonStruct`.
# `whispy_eyes_texture` is the REQUEST (-1 when idle, else 0..2) and
# `map_gobj[3]`'s MObj chain carries what the renderer will actually draw.
# Printing only on CHANGE keeps a 60 Hz breakpoint from emitting a wall of
# identical rows; the run stops after $Changes distinct states.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_whispy-eye-texture-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.whispy-eye-texture-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.whispy-eye-texture-probe.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'grPupupuUpdateGObjAnims',
    'gGRCommonStruct'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Whispy eye probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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

    # $fst/$mo etc, never $fp/$sp/$pc/$lr: on ARM those ARE registers and the
    # assignment fails as "not an lvalue" against an unrelated line.
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $changes = 0',
        'set $lastid = -999',
        'set $lastreq = -999',
        'set $laststa = -999',
        'set $lastbw = -32768',
        'break grPupupuUpdateGObjAnims',
        'commands',
        'silent',
        'set $req = gGRCommonStruct.pupupu.whispy_eyes_texture',
        'set $sta = gGRCommonStruct.pupupu.whispy_eyes_status',
        # map_gobj[0] is Whispy'"'"'s EYES -- grPupupuInitAll (grpupupu.c:666) builds
        # it from llGRPupupuMapMapHead with llGRPupupuMapWhispyEyesTransformKinds-
        # MObjSub, and it is the object the BLINK attaches to (`:573`,
        # dGRPupupuWhispyEyesAnims). map_gobj[3] is FLOWERS FRONT, created with a
        # NULL MObjSub, and reading it for the blink is a dead end -- the first
        # cut of this probe did exactly that and read mobj=(nil) every time.
        'set $g0 = gGRCommonStruct.pupupu.map_gobj[0]',
        'set $id = -1',
        'set $nid = -1',
        'set $spr = (void **)0',
        'set $img = (void *)0',
        'set $mo = (MObj *)0',
        'set $depth = -1',
        # The MObj hangs off a DObj somewhere in the tree, not necessarily the
        # root; walk the child chain until one has a material.
        'if $g0 != 0',
        'set $dn = (DObj *)$g0->obj',
        'set $d = 0',
        'while ($dn != 0) && ($d < 8) && ($mo == 0)',
        'if $dn->mobj != 0',
        'set $mo = $dn->mobj',
        'set $depth = $d',
        'end',
        'set $dn = (DObj *)$dn->child',
        'set $d = $d + 1',
        'end',
        'if $mo != 0',
        'set $id = $mo->texture_id_curr',
        'set $nid = $mo->texture_id_next',
        'set $spr = $mo->sub.sprites',
        'if $spr != 0',
        'set $img = $spr[$id]',
        'end',
        'end',
        'end',
        # blinkwait is in the filter too. Without it the trace only samples the
        # frames a blink is REQUESTED, which cannot tell "counted 0 -> -10 and
        # reseeded" from "sat at 0 and re-fired" -- and those are the two
        # candidate mechanisms.
        'set $bw = gGRCommonStruct.pupupu.whispy_blink_wait',
        # The `$bw <= 12` gate spends the whole budget on the window that decides
        # this row. Without it a 48-row run is 48 rows of an uneventful countdown
        # (241 -> 179) and never reaches a blink at all.
        'if (($id != $lastid) || ($req != $lastreq) || ($sta != $laststa) || ($bw != $lastbw)) && ($bw <= 12)',
        'set $changes = $changes + 1',
        'set $lastid = $id',
        'set $lastreq = $req',
        'set $laststa = $sta',
        'set $lastbw = $bw',
        'printf "WEYE n=%d req=%d status=%d blinkwait=%d animframe=%f lr=%d depth=%d id=%d next=%d sprites=%p img=%p\n", $changes, $req, $sta, $bw, ($g0 != 0) ? $g0->anim_frame : -1, gGRCommonStruct.pupupu.lr_players, $depth, $id, $nid, $spr, $img',
        # The whole sprite table, once, so "the id never moves" can be told apart
        # from "the id moves but every entry is the same pointer".
        'if ($changes == 1) && ($spr != 0)',
        'printf "WEYETAB s0=%p s1=%p s2=%p s3=%p\n", $spr[0], $spr[1], $spr[2], $spr[3]',
        'end',
        'end',
        ('if $changes < ' + $Changes),
        'continue',
        'end',
        'end',

        'continue',
        'printf "WEYEDONE changes=%d\n", $changes',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'whispy_eye_texture_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the helper's capture file, not its return value: a run that times out
    # waiting for its last change holds the same evidence as one that completes,
    # and only the second throws.
    $captured = Join-Path $log_temp 'whispy_eye_texture_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^WEYE' } |
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
