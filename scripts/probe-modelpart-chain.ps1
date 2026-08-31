[CmdletBinding()]
param(
    [string]$Build = 'build-c127-fire',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600,
    [ValidateRange(1, 64)][int]$Hits = 8,
    [string]$Artifact = ''
)

# BUGS.md "Fox's pistol model is missing", DRAW half.
#
# The state half already ships: ftParamSetModelPartID records
# modelpart_status[joint - nFTPartsJointCommonStart].modelpart_id_curr and
# gNdsFighterModelPartOnCount rises 5x on the gate arm. Nothing reads it back,
# so no geometry is ever submitted.
#
# Before writing a submitter, walk the chain the submitter would have to walk
# and prove each link EXISTS -- created -> container -> desc -> part -> dl ->
# owning reloc file. Four Whispy theories died to cheap counters for want of
# exactly this, and every link here is a pointer that may simply be NULL in the
# port because its cross-file fixup never ran.
#
# NO REBUILD. Every expression is rooted at an ARM argument register captured
# at the function's FIRST instruction, where the ABI guarantees r0/r1/r2 are
# the arguments. `ftGetStruct` is exactly `(FTStruct *)gobj->user_data.p` in
# BattleShip fighter.h, so use that stable public object ABI rather than the old
# private `sNdsFighterStructPool` symbol (which current shipping links discard).
# Reading the C local `fp` is what this deliberately avoids: on this remote,
# optimized stack locals have already produced a false root cause.
#
# The convenience variable is $fst, NOT $fp: on ARM, `$fp` IS the frame-pointer
# register, so `set $fp = ...` assigns r11 and fails with "Left operand of
# assignment is not an lvalue" the moment no frame is selected -- reported
# against the top-level `continue`, naming neither the line nor the register.
# Avoid $fp/$sp/$pc/$lr/$rN as convenience-variable names in any gdb probe.
#
# The last link -- "which loaded reloc file owns that dl" -- is what decides the
# implementation. If the dl lives in MiscData315 rather than FoxModel, then
# assigning joint->dl the way source does would push the WHOLE fighter off the
# native draw path (ndsFighterDrawPlanResolve rejects the collection when any
# selected dl resolves to a file whose asset_id != the expected one), which is
# the real reason the DS port must overlay instead of assign.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_modelpart-chain-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.modelpart-chain-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.modelpart-chain-probe.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

# gdb abandons a command file after its first error, so an absent symbol costs
# the whole run silently.
$required = @(
    'ftParamSetModelPartID',
    'ndsFighterRendererInvalidateDObjStateCaches',
    'gNdsFighterMarioFoxRelocDependencyMask',
    'gNdsFighterModelPartOnCount',
    'sNdsRelocLoadedFiles',
    'sNdsRelocLoadedFileCount'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Model-part chain probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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
        'set $donehits = 0',
        'set $pending = 0',
        'set $stop = 0',
        'set $expected_dl = (void *)0',
        'set $expected_flags = 0',
        'break *ftParamSetModelPartID',
        'commands',
        'silent',
        'set $hits = $hits + 1',
        'set $g = (GObj *)$r0',
        'set $joint = (int)$r1',
        'set $mpid = (int)$r2',
        'set $fst = (FTStruct *)$g->user_data.p',
        'set $slot = $joint - 4',
        'set $current_mpid = -127',
        'if ($fst != 0) && ($slot >= 0) && ($slot < 26)',
        'set $current_mpid = $fst->modelpart_status[$slot].modelpart_id_curr',
        'end',
        'if $current_mpid != $mpid',
        'set $pending = 1',
        'set $pending_g = $g',
        'set $pending_joint = $joint',
        'set $pending_mpid = $mpid',
        'end',
        'printf "MPCHAIN hit=%d joint=%d id=%d gobj=%p fp=%p depmask=0x%x on=%u\n", $hits, $joint, $mpid, $g, $fst, gNdsFighterMarioFoxRelocDependencyMask, gNdsFighterModelPartOnCount',
        'if $fst != 0',
        'printf "MPWHO fkind=%d player=%d costume=%d detail_curr=%d detail_base=%d attr=%p\n", $fst->fkind, $fst->player, $fst->costume, $fst->detail_curr, $fst->detail_base, $fst->attr',
        'if $fst->attr != 0',
        'set $mc = $fst->attr->modelparts_container',
        'set $ap = $fst->attr->accesspart',
        'printf "MPCONT container=%p accesspart=%p\n", $mc, $ap',
        'if $ap != 0',
        'printf "MPACC acc_joint=%d acc_dl=%p\n", $ap->joint_id, $ap->dl',
        'end',
        'if $mc != 0',
        'set $desc = $mc->modelparts_desc[$slot]',
        'printf "MPDESC slot=%d desc=%p\n", $slot, $desc',
        'if ($desc != 0) && ($mpid >= 0)',
        'set $part = &$desc->modelparts[$mpid][$fst->detail_curr - 1]',
        'set $expected_dl = $part->dl',
        'set $expected_flags = $part->flags',
        'printf "MPPART part=%p dl=%p mobjsubs=%p flags=0x%x\n", $part, $part->dl, $part->mobjsubs, $part->flags',
        'if $part->dl != 0',
        # Which loaded reloc file owns the dl. ndsRelocFindLoadedFileContaining
        # is static and calling it would need a call frame on the remote, so
        # walk the same table it walks -- pure memory reads.
        'set $i = 0',
        'set $found = 0',
        'while ($i < sNdsRelocLoadedFileCount) && ($found == 0)',
        'set $lf = &sNdsRelocLoadedFiles[$i]',
        'if ($lf->data != 0) && ((char *)$part->dl >= (char *)$lf->data) && ((char *)$part->dl < ((char *)$lf->data + $lf->data_size))',
        'set $found = 1',
        'printf "MPOWNER asset=0x%x data=%p size=%u off=%u\n", $lf->asset_id, $lf->data, $lf->data_size, (unsigned int)((char *)$part->dl - (char *)$lf->data)',
        'end',
        'set $i = $i + 1',
        'end',
        'if $found == 0',
        'printf "MPOWNER none files=%u\n", sNdsRelocLoadedFileCount',
        'end',
        'printf "MPWORD w0=0x%x w1=0x%x\n", *(unsigned int *)$part->dl, *((unsigned int *)$part->dl + 1)',
        'end',
        'end',
        'end',
        'end',
        'end',
        # The joint the geometry would hang off. Its FTParts carries the world
        # matrix an overlay submit needs; a NULL joint means there is nothing to
        # attach to and the row is a different bug.
        'if $fst != 0',
        'set $jd = $fst->joints[$joint]',
        'printf "MPJOINT joint_dobj=%p\n", $jd',
        'if $jd != 0',
        'set $jp = (FTParts *)$jd->user_data.p',
        'printf "MPJPARTS parts=%p dl=%p flags=0x%x\n", $jp, $jd->dl, $jd->flags',
        'if $jp != 0',
        'printf "MPJW w=%f,%f,%f\n", $jp->mtx_translate[3][0], $jp->mtx_translate[3][1], $jp->mtx_translate[3][2]',
        'end',
        'end',
        'end',
        'continue',
        'end',

        # ftParamSetModelPartID calls this only after the live DObj/MObj mutation
        # in the fixed path. Associate the immediately preceding writer call and
        # prove the selected joint now carries the source part DL/flags before
        # the renderer can rebuild its display-contract/draw-plan cache.
        'break *ndsFighterRendererInvalidateDObjStateCaches',
        'commands',
        'silent',
        'if $pending != 0',
        'set $applied_fp = (FTStruct *)$pending_g->user_data.p',
        'set $applied_joint = $applied_fp->joints[$pending_joint]',
        'set $applied_parts = (FTParts *)$applied_joint->user_data.p',
        'set $donehits = $donehits + 1',
        'printf "MPAPPLIED hit=%d fkind=%d joint=%d id=%d dl=%p expect=%p flags=0x%x expectflags=0x%x statusid=%d\n", $donehits, $applied_fp->fkind, $pending_joint, $pending_mpid, $applied_joint->dl, $expected_dl, $applied_parts->flags, $expected_flags, $applied_fp->status_id',
        'set $pending = 0',
        ('if $donehits >= ' + $Hits),
        'set $stop = 1',
        'end',
        'end',
        'if $stop == 0',
        'continue',
        'end',
        'end',

        'continue',
        'printf "MPCHAINDONE entries=%d applied=%d\n", $hits, $donehits',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'modelpart_chain_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # Persist and report from the helper's own capture file, NOT from its return
    # value. A run that reaches its hit target and one that times out waiting for
    # the last hit produce the SAME evidence; only the second throws, and the
    # throw discards the return value, so reading it would have thrown away a
    # complete chain because hit 8 never came. The file is written either way.
    $captured = Join-Path $log_temp 'modelpart_chain_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^MP' } |
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
