[CmdletBinding()]
param(
    [string]$Build = 'build-c128-foxgun',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600,
    [ValidateRange(1, 32)][int]$Events = 6,
    [string]$Artifact = ''
)

# BUGS.md row 1, second question. The first probe established WHAT: the eyes
# GObj animation reaches anim_frame 1.0 for one sample and is back to 0 the
# next, so the blink is a one-frame animation. This one asks WHY, and it is a
# read of the script itself rather than another theory.
#
# grpupupu.c:565 hands gcAddAnimAll a joint table at
#   dGRPupupuWhispyEyesAnims[lr][status][0] + map_head
# and a material table at [1] + map_head, which for BLINK is 0 -> NULL. The
# port intercepts that call (battleship_sys_objanim.c:1567): it normalizes
# every joint script first, and if ANY of them is rejected it skips
# ndsBaseGcAddAnimAll ENTIRELY -- no gobj->anim_frame, no dobj->anim_joint.
# So a one-frame playout has two very different explanations:
#
#   (a) THE ADD NEVER HAPPENED. sNdsAObjEvent32NormalizedCount sits at 973 of
#       1,024 after a minute, and reason 12 is exactly "the table would
#       overflow". Then anim_frame never gets set and the 1.0 is somebody
#       else's leftover. Tell: gNdsAObjEvent32NormalizeFailCount moves across
#       the blink frame, and the script words are still in SOURCE layout.
#   (b) THE SCRIPT REALLY IS ONE FRAME. objdef.h:283 packs source words as
#       opcode[31:25] flags[24:15] payload[14:0]; the port repacks them to
#       opcode[6:0] flags[16:7] payload[31:17]. Frame length is the sum of the
#       payloads of the blocking commands, plus one
#       (reloc_backend_diagnostic_recorders.c:244). Wait is opcode 2, End is 0.
#       If every payload decodes to 0 the data says one frame and the fix is
#       upstream of the runtime.
#
# The MOUTH is the control and it costs nothing: grpupupu.c:362 sets the mouth
# to Stretch on the SAME frame the blink fires, through the same gcAddAnimAll,
# but with a non-NULL material table. If the mouth plays out over many frames
# and the eyes do not, the NULL material argument or the eye data is the
# difference; if both end in one frame the runtime is broken for this GObj.
#
# NO REBUILD. Everything is rooted at gGRCommonStruct, dGRPupupuWhispyEyesAnims
# and the port's normalize counters, all of which are globals.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_whispy-blink-script.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.whispy-blink-script.stdout.log'
$stderr = Join-Path $log_dir 'melonds.whispy-blink-script.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'grPupupuUpdateGObjAnims',
    'gGRCommonStruct',
    'dGRPupupuWhispyEyesAnims',
    'gNdsAObjEvent32NormalizeFailCount',
    'gNdsAObjEvent32NormalizeScriptCount',
    'gNdsAObjEvent32NormalizeReuseCount',
    'gNdsAObjEvent32NormalizeLastFailReason'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Whispy blink script probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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

    # Never $fp/$sp/$pc/$lr/$rN: on ARM those ARE registers and the assignment
    # fails as "not an lvalue" against an unrelated line -- which is why the
    # obvious name for the facing side here is $side. gdb-markers.ps1 refuses
    # the script rather than letting it fail mid-run.
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $n = 0',
        'set $pending = 0',
        'set $fail0 = 0',
        'set $scr0 = 0',
        'set $reu0 = 0',
        'break grPupupuUpdateGObjAnims',
        'commands',
        'silent',
        'set $sta = gGRCommonStruct.pupupu.whispy_eyes_status',
        'set $side = gGRCommonStruct.pupupu.lr_players',
        'set $g0 = gGRCommonStruct.pupupu.map_gobj[0]',
        'set $g1 = gGRCommonStruct.pupupu.map_gobj[1]',
        'set $d0 = ($g0 != 0) ? (DObj *)$g0->obj : (DObj *)0',
        # The frame AFTER an add is the only place the result is visible: the
        # breakpoint sits at function entry, so this hit reads what the
        # PREVIOUS hit built. Counter deltas here are the whole of test (a).
        'if $pending != 0',
        'set $pending = 0',
        'printf "WPOST n=%d dfail=%u dscript=%u dreuse=%u lastreason=%u gframe=%f gspeed=%f dwait=%d dframe=%f djoint=%p droot=%d mouthframe=%f\n", $n, gNdsAObjEvent32NormalizeFailCount - $fail0, gNdsAObjEvent32NormalizeScriptCount - $scr0, gNdsAObjEvent32NormalizeReuseCount - $reu0, gNdsAObjEvent32NormalizeLastFailReason, ($g0 != 0) ? $g0->anim_frame : -1, ($g0 != 0) ? $g0->anim_speed : -1, ($d0 != 0) ? $d0->anim_wait : -999, ($d0 != 0) ? $d0->anim_frame : -1, ($d0 != 0) ? $d0->anim_joint.event32 : (void *)0, ($d0 != 0) ? $d0->is_anim_root : -1, ($g1 != 0) ? $g1->anim_frame : -1',
        'end',
        ('if ($sta != -1) && ($n < ' + $Events + ')'),
        'set $n = $n + 1',
        'set $pending = 1',
        'set $fail0 = gNdsAObjEvent32NormalizeFailCount',
        'set $scr0 = gNdsAObjEvent32NormalizeScriptCount',
        'set $reu0 = gNdsAObjEvent32NormalizeReuseCount',
        'set $head = (unsigned int)gGRCommonStruct.pupupu.map_head',
        'set $joff = (unsigned int)dGRPupupuWhispyEyesAnims[$side][$sta][0]',
        'set $moff = (unsigned int)dGRPupupuWhispyEyesAnims[$side][$sta][1]',
        'set $tab = (unsigned int **)($joff + $head)',
        'printf "WPRE n=%d status=%d lr=%d head=%08x jointoff=%08x matoff=%08x table=%p gframe=%f mouthstatus=%d\n", $n, $sta, $side, $head, $joff, $moff, $tab, ($g0 != 0) ? $g0->anim_frame : -1, gGRCommonStruct.pupupu.whispy_mouth_status',
        # Four entries is past the root and its first children, and the table
        # is a plain array indexed by DObj-tree order, so a short read is safe
        # even though the real length is the tree size.
        'set $j = 0',
        'while $j < 4',
        'set $s = $tab[$j]',
        'if $s != 0',
        'printf "  WJ%d %p : %08x %08x %08x %08x %08x %08x\n", $j, $s, $s[0], $s[1], $s[2], $s[3], $s[4], $s[5]',
        'else',
        'printf "  WJ%d (nil)\n", $j',
        'end',
        'set $j = $j + 1',
        'end',
        'end',
        ('if ($n < ' + $Events + ') || ($pending != 0)'),
        'continue',
        'end',
        'end',

        'continue',
        'printf "WDONE n=%d\n", $n',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'whispy_blink_script_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the helper capture file, not its return value: a run that times out
    # waiting for its last event holds the same evidence as one that completes,
    # and only the second throws.
    $captured = Join-Path $log_temp 'whispy_blink_script_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^\s*W' } |
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
