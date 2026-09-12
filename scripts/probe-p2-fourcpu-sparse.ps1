[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4613,
    [int]$RunnerSlot = -1,
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [switch]$NoBuild,
    [ValidateRange(32,2048)][int]$Frame = 32,
    [switch]$FirstProductionAfterFrame,
    [switch]$PacketFaultCensus,
    [switch]$FirstPacketFault,
    [switch]$FirstActualPacketFault,
    [switch]$FirstTextureReject,
    [switch]$FirstDirectReject,
    [switch]$FighterTextureReject,
    [switch]$PhysicalSpanFault,
    [switch]$FirstPoseBindFull,
    [switch]$StageRouteProbe,
    [switch]$AnimCacheProbe,
    [switch]$WeaponPoolCensus,
    [switch]$EntryPointerProbe,
    [switch]$LoadedFileProbe,
    [switch]$CaptainEntryTrace,
    [switch]$PackFailureProbe,
    [switch]$FirstKirbyReject,
    [switch]$FirstDonkeyReject,
    [switch]$FirstSamusReject,
    [switch]$FirstCutterReject,
    [switch]$FirstSwordReject,
    [ValidateRange(30,900)][int]$TimeoutSeconds = 300,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ((-not $FirstKirbyReject) -and (-not $FirstDonkeyReject) -and
    (-not $FirstSamusReject) -and
    (-not $FirstCutterReject) -and
    (-not $FirstSwordReject) -and
    (($Frame % 32) -ne 0)) {
    throw '-Frame must be a multiple of 32 because the stress marker is sparse.'
}
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root "artifacts\verification\p2-2-fourcpu-sparse$Frame.txt"
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'p2-2-fourcpu-sparse.gdb'
$gdbOut = Join-Path $temp 'p2-2-fourcpu-sparse.gdb.out'
$gdbErr = Join-Path $temp 'p2-2-fourcpu-sparse.gdb.err'
$emulatorOut = Join-Path $temp 'p2-2-fourcpu-sparse.melonds.out'
$emulatorErr = Join-Path $temp 'p2-2-fourcpu-sparse.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$Target" "BUILD=$Build"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required probe input does not exist: $path"
        }
    }

    # GDB on the Windows host can fail to convert long C symbol names from
    # CP1252 while parsing a breakpoint. Resolve the sparse marker from the ELF
    # once and use its numeric address, exactly like the focused fighter proofs.
    $nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
    if (-not (Test-Path -LiteralPath $nm -PathType Leaf)) {
        throw "Required nm input does not exist: $nm"
    }
    $markerLine = & $nm -n $elf | Where-Object {
        $_ -match '^([0-9a-fA-F]+)\s+\S\s+ndsBattlePlayableTickHudSparseMarker$'
    } | Select-Object -First 1
    if ($null -eq $markerLine) {
        throw 'ELF symbol not found: ndsBattlePlayableTickHudSparseMarker'
    }
    $markerMatch = [regex]::Match($markerLine, '^([0-9a-fA-F]+)')
    $sparseMarkerAddress = [uint32]([Convert]::ToUInt32(
        $markerMatch.Groups[1].Value, 16))

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue

    # Sparse debugger stops only.  The stress target emits this marker every 32
    # presented frames, so reaching frame 128 costs four host stops rather than
    # 128 conditional per-frame stops.  This is a diagnosis probe, not a
    # percentile gate.
    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)"
    )
    if ($PacketFaultCensus) {
        $gdbLines += @(
            'set $pf_split = 0',
            'set $pf_raw_textured = 0',
            'set $pf_raw_untextured = 0',
            'set $pf_site = 0',
            'set $pf_local = 0',
            'set $pf_matrix = 0',
            'set $pf_capacity = 0',
            'break src/nds/nds_renderer_native_fighter_production.c:180',
            'commands', 'silent', 'set $pf_split = $pf_split + 1', 'continue', 'end',
            'break src/nds/nds_renderer_native_common.c:7869',
            'commands', 'silent', 'set $pf_raw_textured = $pf_raw_textured + 1', 'continue', 'end',
            'break src/nds/nds_renderer_native_common.c:7875',
            'commands', 'silent', 'set $pf_raw_untextured = $pf_raw_untextured + 1', 'continue', 'end',
            'break src/nds/nds_renderer_preamble.c:3648',
            'commands', 'silent', 'set $pf_site = $pf_site + 1', 'continue', 'end',
            'break src/nds/nds_renderer_preamble.c:3675',
            'commands', 'silent', 'set $pf_local = $pf_local + 1', 'continue', 'end',
            'break src/nds/nds_renderer_textures_effects.c:11198',
            'commands', 'silent', 'set $pf_matrix = $pf_matrix + 1', 'continue', 'end',
            'break src/nds/nds_renderer_preamble.c:3504',
            'commands', 'silent', 'set $pf_capacity = $pf_capacity + 1', 'continue', 'end',
            'break src/nds/nds_renderer_preamble.c:3511',
            'commands', 'silent', 'set $pf_capacity = $pf_capacity + 1', 'continue', 'end',
            'break src/nds/nds_renderer_preamble.c:3522',
            'commands', 'silent', 'set $pf_capacity = $pf_capacity + 1', 'continue', 'end',
            'break src/nds/nds_renderer_native_common.c:7264',
            'commands', 'silent', 'set $pf_capacity = $pf_capacity + 1', 'continue', 'end'
        )
    }
    if ($EntryPointerProbe) {
        $gdbLines += @(
            'set $entryptr_count = 0',
            'break ndsRendererAdapterTryNativeEntryEffect',
            'commands', 'silent',
            'printf "ENTRYDL=%u,%p,cap=%p/c%08x,sam=%p/s%08x,dk=%p/d%08x,fox=%p/f%08x,dobj=%p\\n", gNdsBattlePlayablePacingPresentedFrames, dl, gFTDataCaptainSpecial2, (unsigned int)dl-(unsigned int)gFTDataCaptainSpecial2, gFTDataSamusSpecial2, (unsigned int)dl-(unsigned int)gFTDataSamusSpecial2, gFTDataDonkeySpecial2, (unsigned int)dl-(unsigned int)gFTDataDonkeySpecial2, gFTDataFoxSpecial3, (unsigned int)dl-(unsigned int)gFTDataFoxSpecial3, dobj',
            'set $entryptr_count = $entryptr_count + 1',
            'if $entryptr_count >= 80',
            'detach', 'quit',
            'end',
            'continue',
            'end'
        )
    }
    if ($CaptainEntryTrace) {
        $gdbLines += @(
            'set $cap_entry_gobj = 0',
            'set $cap_entry_updates = 0',
            'set $cap_entry_lists = 0',
            'set $appear_calls = 0',
            'break ftCommonAppearSetStatus',
            'commands', 'silent',
            'printf "APPEAR=%u,gobj=%p,fkind=%d,slot=%u,linknext=%p\\n", gNdsBattlePlayablePacingPresentedFrames, fighter_gobj, ((FTStruct*)fighter_gobj->user_data.p)->fkind, ((FTStruct*)fighter_gobj->user_data.p)->nds_slot, fighter_gobj->link_next',
            'set $appear_calls = $appear_calls + 1',
            'continue',
            'end',
            'break efManagerCaptainEntryCarMakeEffect',
            'commands', 'silent',
            'printf "CAPMAKE=%u,proc=%p,file=%p,pos=%p,lr=%d,recover=%u,free=%d,depth=%u,min=%u,gmax=%d,gactive=%d,heap=%u\\n", gNdsBattlePlayablePacingPresentedFrames, dEFManagerCaptainEntryCarEffectDesc.proc_display, gFTDataCaptainSpecial2, pos, lr, gNdsEFDescDeferRecoverCount, sEFManagerStructsFreeNum, gNdsEffectPoolDepth, gNdsEffectPoolFreeMin, sGCCommonsMaxNum, sGCCommonsActiveNum, (unsigned int)gSYTaskmanGeneralHeap.end-(unsigned int)gSYTaskmanGeneralHeap.ptr',
            'continue',
            'end',
            'break *0x020ba150',
            'commands', 'silent',
            'if $r7 != 0',
            'printf "CAPALLOC=%u,gobj=%p,obj=%p,fun=%p,ep=%p,update=%p,display=%p\\n", gNdsBattlePlayablePacingPresentedFrames, $r7, ((GObj*)$r7)->obj, ((GObj*)$r7)->func_run, ((GObj*)$r7)->user_data.p, ((EFStruct*)((GObj*)$r7)->user_data.p)->proc_update, ((GObj*)$r7)->proc_display',
            'else',
            'printf "CAPALLOC=%u,NULL\\n", gNdsBattlePlayablePacingPresentedFrames',
            'end',
            'continue',
            'end',
            'break ftManagerSetupFilesAllKind',
            'commands', 'silent',
            'if fkind == 7',
            'printf "CAPLOAD_ENTER=%u,file=%p,proc=%p,recover=%u\\n", gNdsBattlePlayablePacingPresentedFrames, gFTDataCaptainSpecial2, dEFManagerCaptainEntryCarEffectDesc.proc_display, gNdsEFDescDeferRecoverCount',
            'end',
            'continue',
            'end',
            'break efManagerCaptainEntryCarProcUpdate',
            'commands', 'silent',
            'set $cap_entry_gobj = effect_gobj',
            'set $cap_entry_updates = $cap_entry_updates + 1',
            'if $cap_entry_updates <= 4',
            'printf "CAPUPDATE=%u,%p,obj=%p,proc=%p\\n", gNdsBattlePlayablePacingPresentedFrames, effect_gobj, effect_gobj->obj, effect_gobj->proc_display',
            'end',
            'continue',
            'end',
            'break ndsRendererAdapterTryNativeEntryEffect',
            'commands', 'silent',
            'if $cap_entry_gobj != 0 && dobj->parent_gobj == $cap_entry_gobj',
            'printf "CAPROOT=%u,%p,base=%p,off=0x%x,dobj=%p\\n", gNdsBattlePlayablePacingPresentedFrames, dl, gFTDataCaptainSpecial2, (unsigned int)dl-(unsigned int)gFTDataCaptainSpecial2, dobj',
            'set $cap_entry_lists = $cap_entry_lists + 1',
            'if $cap_entry_lists >= 24',
            'detach', 'quit',
            'end',
            'end',
            'continue',
            'end'
        )
    }
    if ($PackFailureProbe) {
        # The compact fighter loader fails closed by parking forever in
        # ndsPreviewPackLoadHalt().  Trap it explicitly so a bad generated pack
        # or missing dependency takes one debugger stop instead of consuming the
        # probe's full timeout with no attribution.
        $gdbLines += @(
            'break ndsPreviewPackLoadHalt',
            'commands', 'silent',
            'printf "PACKFAIL=reason:%u,kind:%u,loads:%u,bytes:%u,externPatch:%u,externLoad:%u,externFailure:%u\n", reason, kind, gNdsPreviewPackLoadCount, gNdsPreviewPackDataBytes, gNdsBattleCoreExternPatchCount, gNdsBattleCoreExternLoadCount, gNdsBattleCoreExternFailure',
            'bt 12',
            'detach', 'quit',
            'end'
        )
    }
    if ($FirstKirbyReject) {
        # Stop on the first native-only Kirby program rejection, before the
        # sticky global failure record loses the live root-vector context.  The
        # owner workspace is populated by the admission pass and survives until
        # this fallback call, so this prints the exact vector the generated
        # root-program selector was asked to accept.
        $gdbLines += @(
            'break ndsFighterRejectNativeRender if fp->fkind == 8 && reason == 2',
            'commands', 'silent',
            ('printf "KIRBYREJECT=%u,status:0x%x,battle_slot:%u,dl:%p,' +
             'head:%u,program:%u,decline:%u,selected:%u,tried:%u\\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, fp->status_id, fp->nds_slot, dl, ' +
             'sNdsKirbyTrioHeadMp, sNdsNativeFighterRootPrograms[11], ' +
             'gNdsFtrDeclineStage, gNdsFtrDeclineSelected, gNdsFtrRootProgramsTried'),
            ('printf "KIRBYVALIDATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\\n", ' +
             'gNdsNativeFighterValidateRejectCode, ' +
             'gNdsNativeFighterValidateRejectSlot, ' +
             'gNdsNativeFighterValidateRejectLow, ' +
             'gNdsNativeFighterValidateRejectRoot, ' +
             'gNdsNativeFighterValidateRejectObserved, ' +
             'gNdsNativeFighterValidateRejectExpected, ' +
             'gNdsNativeFighterValidateRejectCount, ' +
             'gNdsNativeFighterValidateRejectAbsentBinding, ' +
             'gNdsNativeFighterValidateRejectForeignIndex, ' +
             'gNdsNativeFighterValidateRejectForeignOffset'),
            'set $kr = 0',
            'while $kr < gNdsFtrDeclineSelected',
            ('printf "KIRBYROOT=%u,asset:%u,0x%x,materials:%u,dobj:%p,parent:%p\\n", $kr, ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.loaded[$kr]->asset_id, ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.root_offsets[$kr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.material_counts[$kr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$kr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$kr]->parent'),
            'set $kj = 0',
            'while $kj < 32',
            'if fp->joints[$kj] == sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$kr]',
            'printf "KIRBYROOTJOINT=%u,%u\\n", $kr, $kj',
            'end',
            'set $kj = $kj + 1',
            'end',
            'set $kr = $kr + 1',
            'end',
            'printf "KIRBYPART=head:%d,body_dobj:%p\\n", fp->modelpart_status[2].modelpart_id_curr, fp->joints[7]',
            'bt 8',
            'detach', 'quit', 'end',
            "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames >= $Frame",
            'commands', 'silent',
            'printf "KIRBYREJECT_NONE_THROUGH=%u\\n", gNdsBattlePlayablePacingPresentedFrames',
            ('printf "KIRBYFINAL=%u,%u,%u,%u;%u,%u,%u,%u;%u,%u,%u,%u;program:%u,tried:%u\\n", ' +
             'gNdsFtrRejectCountBySlot[0], gNdsFtrRejectCountBySlot[1], ' +
             'gNdsFtrRejectCountBySlot[2], gNdsFtrRejectCountBySlot[3], ' +
             'gNdsFtrRejectStatusBySlot[0], gNdsFtrRejectStatusBySlot[1], ' +
             'gNdsFtrRejectStatusBySlot[2], gNdsFtrRejectStatusBySlot[3], ' +
             'gNdsFtrRejectReasonBySlot[0], gNdsFtrRejectReasonBySlot[1], ' +
             'gNdsFtrRejectReasonBySlot[2], gNdsFtrRejectReasonBySlot[3], ' +
             'sNdsNativeFighterRootPrograms[11], gNdsFtrRootProgramsTried'),
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstDonkeyReject) {
        # Stop on Donkey's first native-program rejection while the resolver's
        # exact live root vector and validator witnesses are still resident.
        # This diagnoses alternate source model-part programs without changing
        # ROM behavior or accepting a generic fallback.
        $gdbLines += @(
            'break ndsFighterRejectNativeRender if fp->fkind == 2 && reason == 2',
            'commands', 'silent',
            ('printf "DONKEYREJECT=%u,status:0x%x,battle_slot:%u,dl:%p,program:%u,decline:%u,owner:%u,selected:%u,index:%u,asset:%u,detail:0x%x,tried:%u\\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, fp->status_id, fp->nds_slot, dl, ' +
             'sNdsNativeFighterRootPrograms[3], gNdsFtrDeclineStage, gNdsFtrDeclineOwner, ' +
             'gNdsFtrDeclineSelected, gNdsFtrDeclineIndex, gNdsFtrDeclineAssetId, ' +
             'gNdsFtrDeclineDetail, gNdsFtrRootProgramsTried'),
            ('printf "DONKEYVALIDATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\\n", ' +
             'gNdsNativeFighterValidateRejectCode, gNdsNativeFighterValidateRejectSlot, ' +
             'gNdsNativeFighterValidateRejectLow, gNdsNativeFighterValidateRejectRoot, ' +
             'gNdsNativeFighterValidateRejectObserved, gNdsNativeFighterValidateRejectExpected, ' +
             'gNdsNativeFighterValidateRejectCount, gNdsNativeFighterValidateRejectAbsentBinding, ' +
             'gNdsNativeFighterValidateRejectForeignIndex, gNdsNativeFighterValidateRejectForeignOffset'),
            'set $dr = 0',
            'while $dr < gNdsFtrDeclineSelected',
            ('printf "DONKEYROOT=%u,asset:%u,0x%x,materials:%u,dobj:%p,parent:%p\\n", $dr, ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.loaded[$dr]->asset_id, ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.root_offsets[$dr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.material_counts[$dr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$dr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$dr]->parent'),
            'set $dj = 0',
            'while $dj < 32',
            'if fp->joints[$dj] == sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$dr]',
            'printf "DONKEYROOTJOINT=%u,%u\\n", $dr, $dj',
            'end',
            'set $dj = $dj + 1',
            'end',
            'set $dr = $dr + 1',
            'end',
            'set $dp = 0',
            'while $dp < 32',
            'if fp->modelpart_status[$dp].modelpart_id_curr != 0',
            'printf "DONKEYPART=%u,%d\\n", $dp, fp->modelpart_status[$dp].modelpart_id_curr',
            'end',
            'set $dp = $dp + 1',
            'end',
            'bt 8',
            'detach', 'quit', 'end',
            "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames >= $Frame",
            'commands', 'silent',
            'printf "DONKEYREJECT_NONE_THROUGH=%u\\n", gNdsBattlePlayablePacingPresentedFrames',
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstSamusReject) {
        # Stop on Samus's first native-program rejection while the resolver's
        # exact root vector/model-part state and validator witnesses are live.
        # Catch also owns SamusSpecial2's grapple-beam effect; treat that
        # 349:0x2E0 native-only rejection as the same observable capability.
        $gdbLines += @(
            'break ndsFighterRejectNativeRender if fp->fkind == 3 && reason == 2',
            'commands', 'silent',
            ('printf "SAMUSREJECT=%u,status:0x%x,battle_slot:%u,dl:%p,program:%u,decline:%u,owner:%u,selected:%u,index:%u,asset:%u,detail:0x%x,tried:%u\\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, fp->status_id, fp->nds_slot, dl, ' +
             'sNdsNativeFighterRootPrograms[5], gNdsFtrDeclineStage, gNdsFtrDeclineOwner, ' +
             'gNdsFtrDeclineSelected, gNdsFtrDeclineIndex, gNdsFtrDeclineAssetId, ' +
             'gNdsFtrDeclineDetail, gNdsFtrRootProgramsTried'),
            ('printf "SAMUSVALIDATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\\n", ' +
             'gNdsNativeFighterValidateRejectCode, gNdsNativeFighterValidateRejectSlot, ' +
             'gNdsNativeFighterValidateRejectLow, gNdsNativeFighterValidateRejectRoot, ' +
             'gNdsNativeFighterValidateRejectObserved, gNdsNativeFighterValidateRejectExpected, ' +
             'gNdsNativeFighterValidateRejectCount, gNdsNativeFighterValidateRejectAbsentBinding, ' +
             'gNdsNativeFighterValidateRejectForeignIndex, gNdsNativeFighterValidateRejectForeignOffset'),
            'set $sr = 0',
            'while $sr < gNdsFtrDeclineSelected',
            ('printf "SAMUSROOT=%u,asset:%u,0x%x,materials:%u,dobj:%p,parent:%p\\n", $sr, ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.loaded[$sr]->asset_id, ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.root_offsets[$sr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.material_counts[$sr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$sr], ' +
             'sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$sr]->parent'),
            'set $sj = 0',
            'while $sj < 32',
            'if fp->joints[$sj] == sNdsRendererAdapterNativeOwnerWorkspace.matrix_bindings[$sr]',
            'printf "SAMUSROOTJOINT=%u,%u\\n", $sr, $sj',
            'end',
            'set $sj = $sj + 1',
            'end',
            'set $sr = $sr + 1',
            'end',
            'set $sp = 0',
            'while $sp < 32',
            'if fp->modelpart_status[$sp].modelpart_id_curr != 0',
            'printf "SAMUSPART=%u,%d\\n", $sp, fp->modelpart_status[$sp].modelpart_id_curr',
            'end',
            'set $sp = $sp + 1',
            'end',
            'bt 8',
            'detach', 'quit', 'end',
            'break ndsRendererRecordNativeFailure if domain == 2 && (identity & 65535) == 349 && root == 0x2e0',
            'commands', 'silent',
            ('printf "SAMUSGRAPPLEREJECT=%u,reason:%u,scene:%u,identity:0x%x,status:0x%x,root:0x%x,material:0x%x\\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, reason, scene, identity, status, root, material'),
            'bt 10',
            'detach', 'quit', 'end',
            "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames >= $Frame",
            'commands', 'silent',
            'printf "SAMUSREJECT_NONE_THROUGH=%u\\n", gNdsBattlePlayablePacingPresentedFrames',
            ('printf "SAMUSFINAL=grapple:%u,entryDraw:%u,fallback:%u,texReject:0x%x,releaseCount:%u,releaseBytes:%u\\n", ' +
             'gNdsEntryEffectNativeRootDraws[59], gNdsEntryEffectNativeDrawCount, ' +
             'gNdsEntryEffectNativeFallbackCount, gNdsRendererProfileTextureRejectReasonMask, ' +
             'gNdsEntryEffectStartupTextureReleaseCount, gNdsEntryEffectStartupTextureReleaseBytes'),
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstCutterReject) {
        # Kirby Final Cutter owns generated effect roots 47..56 and travelling
        # weapon roots 57..58 in this candidate. Stop only if one of those exact
        # source roots is rejected; otherwise report every per-root draw count
        # at the requested natural stress frame. This is a probe-only
        # discriminator, not ROM behavior or a substitute for the wide gate.
        $gdbLines += @(
            ('break ndsRendererRecordNativeFailure if domain == 2 && (' +
             '((identity & 65535) == 348 && (root == 0x27a0 || root == 0x0c70 || root == 0x0ce0 || root == 0x11b0 || root == 0x1218 || root == 0x1280 || root == 0x2210 || root == 0x2270 || root == 0x22d0 || root == 0x2330)) || ' +
             '((identity & 65535) == 328 && (root == 0x1d238 || root == 0x1d308)))'),
            'commands', 'silent',
            ('printf "CUTTERREJECT=%u,reason:%u,scene:%u,identity:0x%x,status:0x%x,root:0x%x,material:0x%x\\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, reason, scene, identity, status, root, material'),
            'up',
            ('printf "CUTTERLIVE=kmodel:%p,dobj:%p,parent:%p,parentid:%u,mobj:%p,wp:%p,wpkind:%u,cutterkind:%u\\n", ' +
             'gFTDataKirbyModel, dobj, dobj->parent_gobj, dobj->parent_gobj->id, dobj->mobj, ' +
             'dobj->parent_gobj->user_data.p, ((WPStruct*)dobj->parent_gobj->user_data.p)->kind, nWPKindCutter'),
            'down',
            'bt 10',
            'detach', 'quit', 'end',
            "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames >= $Frame",
            'commands', 'silent',
            'printf "CUTTERREJECT_NONE_THROUGH=%u\\n", gNdsBattlePlayablePacingPresentedFrames',
            ('printf "CUTTERFINAL=roots:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u;entryDraw:%u,fallback:%u,texReject:0x%x,releaseCount:%u,releaseBytes:%u\\n", ' +
             'gNdsEntryEffectNativeRootDraws[47], gNdsEntryEffectNativeRootDraws[48], ' +
             'gNdsEntryEffectNativeRootDraws[49], gNdsEntryEffectNativeRootDraws[50], ' +
             'gNdsEntryEffectNativeRootDraws[51], gNdsEntryEffectNativeRootDraws[52], ' +
             'gNdsEntryEffectNativeRootDraws[53], gNdsEntryEffectNativeRootDraws[54], ' +
             'gNdsEntryEffectNativeRootDraws[55], gNdsEntryEffectNativeRootDraws[56], ' +
             'gNdsEntryEffectNativeDrawCount, ' +
             'gNdsEntryEffectNativeFallbackCount, gNdsRendererProfileTextureRejectReasonMask, ' +
             'gNdsEntryEffectStartupTextureReleaseCount, gNdsEntryEffectStartupTextureReleaseBytes'),
            ('printf "CUTTERWEAPON=roots:%u,%u\\n", ' +
             'gNdsEntryEffectNativeRootDraws[57], gNdsEntryEffectNativeRootDraws[58]'),
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstSwordReject) {
        # Stop on the exact common-item native-only failure currently leading
        # the wide stress latch.  Dump every Sword admission discriminator so
        # the fix targets the first false guard instead of guessing from the
        # sticky asset/root identity alone.
        $gdbLines += @(
            'break src/port/renderer_adapter_stage.c:5128',
            'commands', 'silent',
            'if loaded == 0 || loaded->asset_id != 86 || ((unsigned int)dl - (unsigned int)loaded->data) != 0x17d8',
            'continue',
            'end',
            ('printf "SWORDREJECT=%u,reason:%u,scene:%u,gkind:%u,asset:%u,root:0x%x,bytes:0x%x,mobj:%p,parentid:%u\\\\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, reason, gSCManagerSceneData.scene_curr, ' +
             'gSCManagerBattleState->gkind, loaded->asset_id, (unsigned int)dl - (unsigned int)loaded->data, ' +
             'loaded->data_size, dobj->mobj, dobj->parent_gobj->id'),
            ('printf "SWORDOWNER=active:%u,head:%u,candidate:%u,draw:%u,submitFail:%u,submitStep:%u,kind:%u,foreign:%u,latchedRoot:0x%x\\\\n", ' +
             'sNdsRendererAdapterItemSubmitActive, sNdsRendererAdapterItemSubmitHead, ' +
             'gNdsItemSwordCandidateStep, gNdsItemSwordDrawCount, gNdsItemSwordSubmitFailCount, ' +
             'gNdsItemSwordSubmitStep, gNdsItemSwordKind, gNdsItemSwordForeignKindCount, gNdsItemSwordRoot'),
            ('printf "SWORDIP=ip:%p,kind:%u,attr:%p,flags:0x%x,child:%p,dv:%p,dl7:%08x/%08x,expected:%p\\\\n", ' +
             'dobj->parent_gobj->user_data.p, ((ITStruct*)dobj->parent_gobj->user_data.p)->kind, ' +
             '((ITStruct*)dobj->parent_gobj->user_data.p)->attr, dobj->flags, dobj->child, dobj->dv, ' +
             'dl[7].words.w0, dl[7].words.w1, (char*)loaded->data + 0x16e8'),
            'bt 10',
            'detach', 'quit', 'end',
            "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames >= $Frame",
            'commands', 'silent',
            'printf "SWORDREJECT_NONE_THROUGH=%u\\\\n", gNdsBattlePlayablePacingPresentedFrames',
            ('printf "SWORDFINAL=candidate:%u,draw:%u,submitFail:%u,submitStep:%u,kind:%u,foreign:%u,root:0x%x\\\\n", ' +
             'gNdsItemSwordCandidateStep, gNdsItemSwordDrawCount, gNdsItemSwordSubmitFailCount, ' +
             'gNdsItemSwordSubmitStep, gNdsItemSwordKind, gNdsItemSwordForeignKindCount, gNdsItemSwordRoot'),
            ('printf "SWORDTEX=rejectMask:0x%x,useOff:%u,noCombine:%u,noTexel:%u,texReject:%u,primPrep:%u,primBind:%u\\\\n", ' +
             'gNdsRendererProfileTextureRejectReasonMask, gNdsRendererProfileUseTextureRejectStateOffCount, ' +
             'gNdsRendererProfileUseTextureRejectNoCombineCount, gNdsRendererProfileUseTextureRejectNoTexel0Count, ' +
             'gNdsStageGCDrawAllLoopHardwareTextureRejectCount, gNdsRendererPrimRgbTexel0AlphaPrepareCount, ' +
             'gNdsRendererPrimRgbTexel0AlphaBindCount'),
            ('printf "SWORDVRAM=staticCount:%u,staticBytes:%u,span:%u,skipped:%u,first:0x%x,end:0x%x,banks:0x%x,evict:%u\\\\n", ' +
             'gNdsRendererBattleStaticTexturePreparedCount, gNdsRendererBattleStaticTexturePreparedBytes, ' +
             'gNdsRendererBattleStaticTextureAllocationSpanBytes, gNdsRendererBattleStaticTextureSkippedCount, ' +
             'gNdsRendererBattleStaticTextureFirstAddress, gNdsRendererBattleStaticTextureEndAddress, ' +
             'gNdsRendererBattleStaticTextureBankMask, gNdsRendererProfileTextureCacheEvictCount'),
            ('printf "SWORDENTRYLIFE=releaseCount:%u,releaseBytes:%u\\\\n", ' +
             'gNdsEntryEffectStartupTextureReleaseCount, gNdsEntryEffectStartupTextureReleaseBytes'),
            ('printf "SWORDFENCE=firstClass:%u,firstFrame:%u,c0:%u,c1:%u,c2:%u,c3:%u,c4:%u,c5:%u,c6:%u,c7:%u,c8:%u,c9:%u\\\\n", ' +
             'gNdsRendererBattleTextureFenceFirstClassPlus1, gNdsRendererBattleTextureFenceFirstFrame, ' +
             'gNdsRendererBattleTextureFenceCounts[0], gNdsRendererBattleTextureFenceCounts[1], ' +
             'gNdsRendererBattleTextureFenceCounts[2], gNdsRendererBattleTextureFenceCounts[3], ' +
             'gNdsRendererBattleTextureFenceCounts[4], gNdsRendererBattleTextureFenceCounts[5], ' +
             'gNdsRendererBattleTextureFenceCounts[6], gNdsRendererBattleTextureFenceCounts[7], ' +
             'gNdsRendererBattleTextureFenceCounts[8], gNdsRendererBattleTextureFenceCounts[9]'),
            ('printf "SWORDBG=bg2clear:%u,bg2copy:%u,bg2final:%u,bg3clear:%u,bg3copy:%u,bg3final:%u\\\\n", ' +
             'gNdsOriginalSpriteBg2ClearBytes, gNdsOriginalSpriteBg2CopyBytes, gNdsOriginalSpriteBg2FinalWriteBytes, ' +
             'gNdsOriginalSpriteBg3ClearBytes, gNdsOriginalSpriteBg3CopyBytes, gNdsOriginalSpriteBg3FinalWriteBytes'),
            'set $sw_live = 0',
            'set $sw_current = 0',
            'set $sw_stagewarm = 0',
            'set $sw_evictable = 0',
            'set $sw_i = 45',
            'while $sw_i < (sizeof(sNdsRendererHardwareTextureCache) / sizeof(sNdsRendererHardwareTextureCache[0]))',
            'if sNdsRendererHardwareTextureCache[$sw_i].name != 0',
            'set $sw_live = $sw_live + 1',
            'set $sw_name = sNdsRendererHardwareTextureCache[$sw_i].name',
            'set $sw_tex = (gl_texture_data*)glGlobalData.texturePtrs.data[$sw_name]',
            ('printf "SWORDTEXENTRY=%u,name:%u,w:%u,h:%u,last:%u,stagewarm:%u,vram:%p,texSize:%u,format:0x%x\\\\n", ' +
             '$sw_i, $sw_name, sNdsRendererHardwareTextureCache[$sw_i].profile_width, ' +
             'sNdsRendererHardwareTextureCache[$sw_i].profile_height, ' +
             'sNdsRendererHardwareTextureCache[$sw_i].last_used_frame, ' +
             'sNdsRendererHardwareTextureCache[$sw_i].stage_warm, ' +
             '$sw_tex->vramAddr, $sw_tex->texSize, $sw_tex->texFormat'),
            'if sNdsRendererHardwareTextureCache[$sw_i].stage_warm != 0',
            'set $sw_stagewarm = $sw_stagewarm + 1',
            'else',
            'if sNdsRendererHardwareTextureCache[$sw_i].last_used_frame == (sNdsRendererHardwareFrameSerial + 1)',
            'set $sw_current = $sw_current + 1',
            'else',
            'set $sw_evictable = $sw_evictable + 1',
            'end',
            'end',
            'end',
            'set $sw_i = $sw_i + 1',
            'end',
            ('printf "SWORDCACHE=live:%u,current:%u,stagewarm:%u,evictable:%u,dynamicSlots:%u,serial:%u\\\\n", ' +
             '$sw_live, $sw_current, $sw_stagewarm, $sw_evictable, ' +
             '(unsigned int)(sizeof(sNdsRendererHardwareTextureCache) / sizeof(sNdsRendererHardwareTextureCache[0])) - 45, ' +
             'sNdsRendererHardwareFrameSerial'),
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstPacketFault) {
        $gdbLines += @(
            'break ndsFighterPacketTryReplay if gNdsBattlePlayablePacingPresentedFrames >= 65',
            'commands', 'silent',
            'printf "PKTRY frame=%u slot=%u owner=%u roots=%u rec=%u fault=%u\n", gNdsBattlePlayablePacingPresentedFrames, (texture_memo_owner_key >> 9) & 3, owner_slot, input_count, sNdsFighterPacketRecording, sNdsFighterPacketRecorder.fault',
            'continue', 'end',
            'break ndsFighterPacketFinishRecord if gNdsBattlePlayablePacingPresentedFrames >= 65',
            'commands', 'silent',
            'printf "PKFIN frame=%u slot=%d count=%u fault=%u\n", gNdsBattlePlayablePacingPresentedFrames, sNdsFighterPacketRecorder.packet - sNdsFighterPackets, sNdsFighterPacketRecorder.count, sNdsFighterPacketRecorder.fault',
            'continue', 'end',
            'break ndsRendererLoadHardwareSplitMatrices if sNdsFighterPacketRecording != 0',
            'commands', 'silent',
            'printf "FIRSTFAULT=split frame=%u battle_slot=%d root=%u\n", gNdsBattlePlayablePacingPresentedFrames, sNdsFighterPacketRecorder.packet - sNdsFighterPackets, sNdsFighterPacketRecorder.current_root',
            'bt 20',
            'detach', 'quit', 'end',
            'break ndsRendererNativeEmitProductionRawTexturedRun if sNdsFighterPacketRecording != 0',
            'commands', 'silent',
            'printf "FIRSTFAULT=raw-textured frame=%u battle_slot=%d root=%u run=%u\n", gNdsBattlePlayablePacingPresentedFrames, sNdsFighterPacketRecorder.packet - sNdsFighterPackets, sNdsFighterPacketRecorder.current_root, run_index',
            'detach', 'quit', 'end',
            'break ndsRendererNativeEmitProductionRawUntexturedRun if sNdsFighterPacketRecording != 0',
            'commands', 'silent',
            'printf "FIRSTFAULT=raw-untextured frame=%u battle_slot=%d root=%u run=%u\n", gNdsBattlePlayablePacingPresentedFrames, sNdsFighterPacketRecorder.packet - sNdsFighterPackets, sNdsFighterPacketRecorder.current_root, run_index',
            'detach', 'quit', 'end',
            'break ndsFighterPacketLoadGxComposedRecord if input->projection_matrix == 0 || input->gx_seed == 0 || (input->gx_local_count != 0 && input->gx_locals == 0)',
            'commands', 'silent',
            'printf "FIRSTFAULT=matrix-input frame=%u battle_slot=%d root=%u\n", gNdsBattlePlayablePacingPresentedFrames, sNdsFighterPacketRecorder.packet - sNdsFighterPackets, root_index',
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstActualPacketFault) {
        $gdbLines += @(
            'break ndsFighterPacketAbortRecord',
            'commands', 'silent',
            'printf "ACTUALFAULT=abort frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'up',
            'info args',
            'info locals',
            'p/x stats->geometry_mode',
            'p/x stats->othermode_h',
            'p/x stats->othermode_l',
            'p/x stats->texture_combine_w0',
            'p/x stats->texture_combine_w1',
            'p/x stats->env_color',
            'p stats->blocker',
            'p state->matrix_valid',
            'p state->matrix_generation',
            'p state->modelview_valid',
            'p state->texture_prepare_valid',
            'p state->texture_prepare_enabled',
            'p/x input->root_offset',
            'p input->gx_valid',
            'p input->gx_modelview_mirror_valid',
            'p sNdsNativeFighterActiveTables->epoch_direct_policy[epoch_index]',
            'bt 12', 'detach', 'quit', 'end',
            # Exact stores from this ELF's disassembly. Source-line breakpoints
            # land on shared optimized compares and produce false positives.
            'break *0x01ffc5b0',
            'commands', 'silent',
            'printf "ACTUALFAULT=split-record frame=%u root=%u\n", gNdsBattlePlayablePacingPresentedFrames, root_index',
            'bt 12', 'detach', 'quit', 'end',
            'break *0x0200395c',
            'commands', 'silent',
            'printf "ACTUALFAULT=packet-capacity frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'bt 12', 'detach', 'quit', 'end',
            'break *0x02003a8c',
            'commands', 'silent',
            'printf "ACTUALFAULT=shade-sites frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'bt 12', 'detach', 'quit', 'end',
            'break *0x02003b24',
            'commands', 'silent',
            'printf "ACTUALFAULT=root-locals frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'bt 12', 'detach', 'quit', 'end',
            'break *0x02003b7c',
            'commands', 'silent',
            'printf "ACTUALFAULT=root-null-or-index frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'bt 12', 'detach', 'quit', 'end',
            'break *0x02004978',
            'commands', 'silent',
            'printf "ACTUALFAULT=gx-input frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'bt 12', 'detach', 'quit', 'end',
            'break *0x020042dc',
            'commands', 'silent',
            'printf "ACTUALFAULT=finish-invalid frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'bt 12', 'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstTextureReject) {
        $gdbLines += @(
            'break ndsRendererHardwareRejectTexture',
            'commands', 'silent',
            'printf "TEXTUREREJECT=1 frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'info args',
            'info locals',
            'bt 14',
            'up',
            'info args',
            'info locals',
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FirstDirectReject) {
        $gdbLines += @(
            'break ndsRendererNativeDirectReject',
            'commands', 'silent',
            'printf "DIRECTREJECT=1 frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'info args',
            'bt 12',
            'up',
            'info args',
            'info locals',
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($FighterTextureReject) {
        $gdbLines += @(
            'break ndsRendererHardwareRejectTexture',
            'commands', 'silent',
            'if stats != (NDSRendererStats *)0x2259c34',
            'continue',
            'end',
            'printf "FIGHTERTEXREJECT=1 frame=%u\n", gNdsBattlePlayablePacingPresentedFrames',
            'info args',
            'bt 14',
            'up',
            'info args',
            'info locals',
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($PhysicalSpanFault) {
        $gdbLines += @(
            'break *0x02021ce0',
            'commands', 'silent',
            'printf "SPANFAULT=1 frame=%u logical=%#x rounded=%#x sp=%p\n", gNdsBattlePlayablePacingPresentedFrames, $r2, $r3, $sp',
            'info locals',
            'bt 12',
            'detach', 'quit', 'end',
            'continue'
        )
    }
    if ($WeaponPoolCensus) {
        # GDB-only ownership census. The free-list helpers inline in the shipped
        # build, so watch their one owner pointer and derive exact live depth on
        # every mutation instead of adding shipping counters to the hot path.
        $gdbLines += @(
            'set $wp_live = 0',
            'set $wp_live_max = 0',
            'set $wp_allocs = 0',
            'set $wp_frees = 0',
            'watch sWPManagerStructsAllocFree',
            'commands', 'silent',
            'if gNdsWeaponPoolEntries != 0',
            'set $wp_node = sWPManagerStructsAllocFree',
            'set $wp_free = 0',
            'while $wp_node != 0 && $wp_free <= gNdsWeaponPoolEntries',
            'set $wp_free = $wp_free + 1',
            'set $wp_node = $wp_node->next',
            'end',
            'if $wp_free <= gNdsWeaponPoolEntries',
            'set $wp_next_live = gNdsWeaponPoolEntries - $wp_free',
            'if $wp_next_live > $wp_live',
            'set $wp_allocs = $wp_allocs + ($wp_next_live - $wp_live)',
            'else',
            'set $wp_frees = $wp_frees + ($wp_live - $wp_next_live)',
            'end',
            'set $wp_live = $wp_next_live',
            'if $wp_live > $wp_live_max',
            'set $wp_live_max = $wp_live',
            'end',
            'end',
            'end',
            'continue', 'end'
        )
    }
    if ($FirstPoseBindFull) {
        # Exact store site from this target's current ELF.  Stop immediately
        # before gNdsFtPoseBindFull increments so r5 is the requesting GObj and
        # r8 is the requested joint count.  Dump the four fixed owner slots by
        # address to distinguish a fifth live owner from same-owner capacity
        # growth without relying on Windows GDB's fragile long-symbol decoding.
        $gdbLines += @(
            'break *0x02095a58',
            'continue',
            'printf "POSEFULL request=%p count=%u live=%u max=%u claims=%u releases=%u\n", $r5, $r8, gNdsFtPoseSlotLive, gNdsFtPoseSlotLiveMax, gNdsFtPoseSlotClaims, gNdsFtPoseSlotReleases',
            'printf "POSE0=%p,%u,%u,%u\n", *(unsigned int*)0x0223d928, *(unsigned int*)0x0223d930, *(unsigned int*)0x0223d934, *(unsigned int*)0x0223d93c',
            'printf "POSE1=%p,%u,%u,%u\n", *(unsigned int*)0x0223d96c, *(unsigned int*)0x0223d974, *(unsigned int*)0x0223d978, *(unsigned int*)0x0223d980',
            'printf "POSE2=%p,%u,%u,%u\n", *(unsigned int*)0x0223d9b0, *(unsigned int*)0x0223d9b8, *(unsigned int*)0x0223d9bc, *(unsigned int*)0x0223d9c4',
            'printf "POSE3=%p,%u,%u,%u\n", *(unsigned int*)0x0223d9f4, *(unsigned int*)0x0223d9fc, *(unsigned int*)0x0223da00, *(unsigned int*)0x0223da08',
            'bt 8',
            'detach', 'quit'
        )
    }
    elseif (-not $FirstPacketFault -and -not $FirstActualPacketFault -and -not $FirstTextureReject -and -not $FirstDirectReject -and -not $FighterTextureReject -and -not $FirstKirbyReject -and -not $FirstDonkeyReject -and -not $FirstSamusReject -and -not $FirstCutterReject -and -not $FirstSwordReject) {
    $gdbLines += @(
        ('break *0x{0:x8}' -f $sparseMarkerAddress),
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $Frame",
        'continue',
        'end',
        'end',
        'continue',
        ('printf "SPARSE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsBattlePlayablePacingPresentedFrames, ' +
         'gNdsBattlePlayablePacingLogicFrames, ' +
         'gNdsTickHudBuckets[0], gNdsTickHudBuckets[1], ' +
         'gNdsTickHudBuckets[2], gNdsTickHudBuckets[3], ' +
         'gNdsTickHudBuckets[6], gNdsTickHudBuckets[10], ' +
         'gNdsFtrPlanBuild, gNdsFtrPlanHit, ' +
         'gNdsFighterDLAllDrawP0HardwareTriangleCount, ' +
         'gNdsFighterDLAllDrawP1HardwareTriangleCount'),
        ('printf "ROSTER=%#x,%#x\n", ' +
         'gNdsSCVSBattleOriginalFighterKinds, ' +
         'gNdsFighterDLAllDrawSlotTriangleMask'),
        ('printf "FTRREJECT=%u,%u,%u,%u;%u,%u,%u,%u;%u,%u,%u,%u\n", ' +
         'gNdsFtrRejectCountBySlot[0], gNdsFtrRejectCountBySlot[1], ' +
         'gNdsFtrRejectCountBySlot[2], gNdsFtrRejectCountBySlot[3], ' +
         'gNdsFtrRejectStatusBySlot[0], gNdsFtrRejectStatusBySlot[1], ' +
         'gNdsFtrRejectStatusBySlot[2], gNdsFtrRejectStatusBySlot[3], ' +
         'gNdsFtrRejectReasonBySlot[0], gNdsFtrRejectReasonBySlot[1], ' +
         'gNdsFtrRejectReasonBySlot[2], gNdsFtrRejectReasonBySlot[3]'),
        ('printf "FTRDECLINE=%u,%u,%u,%u,%u,%u,%u;%u,%u,%u\n", ' +
         'gNdsFtrDeclineStage, gNdsFtrDeclineSelected, gNdsFtrDeclineIndex, ' +
         'gNdsFtrDeclineAssetId, gNdsFtrDeclineDetail, gNdsFtrRootProgramsTried, ' +
         'gNdsFtrDeclineOwner, gNdsFtrPreValidateBuild, ' +
         'gNdsFtrPreValidateReuse, gNdsFtrPreValidateReject'),
        ('printf "FTRVALIDATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsNativeFighterValidateRejectCode, ' +
         'gNdsNativeFighterValidateRejectSlot, ' +
         'gNdsNativeFighterValidateRejectLow, ' +
         'gNdsNativeFighterValidateRejectRoot, ' +
         'gNdsNativeFighterValidateRejectObserved, ' +
         'gNdsNativeFighterValidateRejectExpected, ' +
         'gNdsNativeFighterValidateRejectCount, ' +
         'gNdsNativeFighterValidateRejectAbsentBinding, ' +
         'gNdsNativeFighterValidateRejectForeignIndex, ' +
         'gNdsNativeFighterValidateRejectForeignOffset'),
        ('printf "NATIVEFAIL=%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, ' +
         'gNdsRendererNativeFailure.scene, gNdsRendererNativeFailure.identity, ' +
         'gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, ' +
         'gNdsRendererNativeFailure.material, gNdsRendererNativeFailure.reason'),
        ('printf "CHARGESHOT=%u,%u,%u,%u,%u,%u,%u,0x%x\n", ' +
         'gNdsChargeShotCandidateStep, gNdsChargeShotDrawCount, ' +
         'gNdsChargeShotSubmitFailCount, gNdsChargeShotSubmitStep, ' +
         'gNdsChargeShotProjection, gNdsChargeShotModelview, ' +
         'gNdsChargeShotAlpha, gNdsRendererProfileTextureRejectReasonMask'),
        ('printf "MEM=%u,%u,%u\n", ' +
         'gNdsTaskmanGeneralHeapFreeMin, gNdsTaskmanArenaChosenSize, ' +
         'gNdsTaskmanArenaAllocFailCount'),
        ('printf "BATTLECORE=%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsPreviewPackLoadCount, gNdsPreviewPackDataBytes, ' +
         'gNdsPreviewPackFailure, gNdsPreviewPackFailureKind, ' +
         'gNdsBattleCoreExternPatchCount, gNdsBattleCoreExternLoadCount, ' +
         'gNdsBattleCoreExternFailure'),
        ('printf "SHIELDPOSE=%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsShieldPoseLoadCount, gNdsShieldPoseResidentBytes, ' +
         'gNdsShieldPoseNativeFixupCount, gNdsShieldPoseLoadFailCount, ' +
         'gNdsShieldPoseNativeFixupRejectCount, gNdsShieldPoseDecodeFailCount'),
        ('printf "GOBJ=%d,%u,%u,%#x,%u\n", ' +
         'sGCCommonsMaxNum, sGCCommonsActiveNum, gNdsObjmanPanicCount, ' +
         'gNdsObjmanPanicMask, gNdsSyMallocOverflowCount'),
        ('printf "GFXHEAP=%u,%u,%u,%u\n", ' +
         'gNdsTaskmanGraphicsHeapCapacity, gNdsTaskmanGraphicsHeapHighWater, ' +
         'gNdsTaskmanGraphicsHeapOverflowCount, gNdsTaskmanGraphicsHeapNoRoomCount'),
        ('printf "PACKET=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsFighterPacketHits, gNdsFighterPacketRecords, ' +
         'gNdsFighterPacketFaults, gNdsFighterPacketDeclines, ' +
         'gNdsFighterPacketMissWord[0], gNdsFighterPacketMissWord[1], ' +
         'gNdsFighterPacketMissWord[2], gNdsFighterPacketMissWord[3], ' +
         'gNdsFighterPacketMissWord[4], gNdsFighterPacketMissWord[5], ' +
         'gNdsFighterPacketMissWord[6], gNdsFighterPacketMissWord[7], ' +
         'gNdsFighterPacketWordsMax'),
        ('printf "PKSLOT0=%u,%u,%u,%u,%u,%u,%u\n", ' +
         'sNdsFighterPackets[0].valid, sNdsFighterPackets[0].word_count, ' +
         'sNdsFighterPackets[0].word_capacity, sNdsFighterPackets[0].root_count, ' +
         'sNdsFighterPackets[0].site_count, sNdsFighterPackets[0].texture_count, ' +
         'sNdsFighterPackets[0].needs_fence'),
        ('printf "PKSLOT1=%u,%u,%u,%u,%u,%u,%u\n", ' +
         'sNdsFighterPackets[1].valid, sNdsFighterPackets[1].word_count, ' +
         'sNdsFighterPackets[1].word_capacity, sNdsFighterPackets[1].root_count, ' +
         'sNdsFighterPackets[1].site_count, sNdsFighterPackets[1].texture_count, ' +
         'sNdsFighterPackets[1].needs_fence'),
        ('printf "PKSLOT2=%u,%u,%u,%u,%u,%u,%u\n", ' +
         'sNdsFighterPackets[2].valid, sNdsFighterPackets[2].word_count, ' +
         'sNdsFighterPackets[2].word_capacity, sNdsFighterPackets[2].root_count, ' +
         'sNdsFighterPackets[2].site_count, sNdsFighterPackets[2].texture_count, ' +
         'sNdsFighterPackets[2].needs_fence'),
        ('printf "PKSLOT3=%u,%u,%u,%u,%u,%u,%u\n", ' +
         'sNdsFighterPackets[3].valid, sNdsFighterPackets[3].word_count, ' +
         'sNdsFighterPackets[3].word_capacity, sNdsFighterPackets[3].root_count, ' +
         'sNdsFighterPackets[3].site_count, sNdsFighterPackets[3].texture_count, ' +
         'sNdsFighterPackets[3].needs_fence'),
        ('printf "TEXTURE=%u,%u,%u\n", ' +
         'gNdsFighterDLAllDrawHardwareTextureReadyCount, ' +
         'gNdsFighterDLAllDrawHardwareTextureUploadCount, ' +
         'gNdsFighterDLAllDrawHardwareTextureRejectCount'),
        ('printf "PARTICLE=%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsParticleRejectCount, gNdsParticleRejectRingCount, ' +
         'gNdsParticleRejectRingBanks[0], gNdsParticleRejectRingScripts[0], ' +
         'gNdsParticleRejectRingReasons[0], gNdsParticleRejectRingCounts[0]'),
        ('printf "ANIMCACHE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsR2AnimCacheArenaReservedBytes, gNdsR2AnimCacheArenaUsedBytes, ' +
         'gNdsR2AnimCacheMisses, gNdsR2AnimCacheRejects, ' +
         'gNdsR2AnimCacheRejectedUniqueCount, gNdsR2AnimCacheRejectedUniqueBytes, ' +
         'gNdsR2AnimCacheArenaOverflows, gNdsR2AnimCacheArenaReserveFailCount, ' +
         'gNdsRelocAssetPayloadReadCount, gNdsR2AnimCacheRawRecycles'),
        ('printf "ANIMDIRECT=%u,%u\n", ' +
         'gNdsRelocAssetDirectReadCount, ' +
         'gNdsRelocAssetDirectFallbackCount'),
        ('printf "ANIMSTREAM=%u,%u,%u,%u\n", ' +
         'gNdsRelocAssetFighterStreamDispatch, ' +
         'gNdsRelocAssetFighterStreamReads, ' +
         'gNdsRelocAssetFighterStreamMisses, ' +
         'gNdsRelocAssetFighterStreamFailures'),
        ('printf "ENTRY_NATIVE=%u,%u,%u,%u,%u\n", ' +
         'gNdsEntryEffectNativeDrawCount, gNdsEntryEffectNativeFallbackCount, ' +
         'gNdsEntryEffectNativeTexturePrepareCount, ' +
         'gNdsEntryEffectNativeTextureBindCount, ' +
         'gNdsEntryEffectNativeRootDraws[10]'),
        ('printf "ENTRY_NATIVE_SAMUS=%u,%u\n", ' +
         'gNdsEntryEffectNativeRootDraws[11], ' +
         'gNdsEntryEffectNativeRootDraws[12]'),
        ('printf "ENTRY_NATIVE_CAPTAIN=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsEntryEffectNativeRootDraws[13], gNdsEntryEffectNativeRootDraws[14], ' +
         'gNdsEntryEffectNativeRootDraws[15], gNdsEntryEffectNativeRootDraws[16], ' +
         'gNdsEntryEffectNativeRootDraws[17], gNdsEntryEffectNativeRootDraws[18], ' +
         'gNdsEntryEffectNativeRootDraws[19], gNdsEntryEffectNativeRootDraws[20], ' +
         'gNdsEntryEffectNativeRootDraws[21], gNdsEntryEffectNativeRootDraws[22]'),
        ('printf "EFDESC=%u,%u,%u,%u,%u\n", ' +
         'gNdsEFDescResolveCount, gNdsEFDescDisabledCount, ' +
         'gNdsEFDescUnknownFileCount, gNdsEFDescDeferRecoverCount, ' +
         'gNdsEFDescDeferOverflowCount'),
        ('printf "EFDEFER=%u,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p\n", ' +
         'sNdsEFDeferredCount, sNdsEFDeferredDescs[0], sNdsEFDeferredDescs[1], ' +
         'sNdsEFDeferredDescs[2], sNdsEFDeferredDescs[3], sNdsEFDeferredDescs[4], ' +
         'sNdsEFDeferredDescs[5], sNdsEFDeferredDescs[6], sNdsEFDeferredDescs[7], ' +
         'sNdsEFDeferredDescs[8], sNdsEFDeferredDescs[9], sNdsEFDeferredDescs[10], ' +
         '&dEFManagerCaptainEntryCarEffectDesc'),
        ('printf "CAPENTRY_DESC=%p,%p,%p\n", ' +
         'dEFManagerCaptainEntryCarEffectDesc.proc_display, ' +
         'dEFManagerCaptainEntryCarEffectDesc.file_head, gFTDataCaptainSpecial2'),
        ('printf "CAPENTRY_OFFSETS=0x%x,0x%x,0x%x,0x%x\n", ' +
         'dEFManagerCaptainEntryCarEffectDesc.o_dobjsetup, ' +
         'dEFManagerCaptainEntryCarEffectDesc.o_mobjsub, ' +
         'dEFManagerCaptainEntryCarEffectDesc.o_anim_joint, ' +
         'dEFManagerCaptainEntryCarEffectDesc.o_matanim_joint'),
        ('printf "EFFECT_RENDER=%u,%u,%u,%u,%u,%u,%u,%u,0x%x,0x%x\n", ' +
         'gNdsEffectRendererCaptureCount, gNdsEffectRendererDObjDrawCount, ' +
         'gNdsEffectRendererSourceModelAdmitCount, gNdsEffectRendererSubmitCount, ' +
         'gNdsEffectRendererRejectedDrawCount, gNdsEffectRendererTriangleCount, ' +
         'gNdsEffectRendererTextureReadyCount, gNdsEffectRendererTextureRejectCount, ' +
         'gNdsEffectRendererCallbackKindMask, gNdsEffectRendererRejectedKindMask'),
        ('printf "POSE=%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsFtPoseBinds, gNdsFtPoseBindFull, gNdsFtPoseSlotLive, ' +
         'gNdsFtPoseSlotLiveMax, gNdsFtPoseSlotClaims, gNdsFtPoseSlotReleases'),
        # Scene residency is allocated before the first sparse checkpoint and
        # the memory ledger keeps a high-water. Capture it with the same
        # four-fighter binary so P2-2 can publish an arena/reloc budget without
        # dividing shared Mario/Fox files by instance count.
        ('printf "MEMLEDGER=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater, ' +
         'gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerRelocBytes, ' +
         'gNdsMemoryLedgerRelocStageBytes, gNdsMemoryLedgerRelocFighterBytes, ' +
         'gNdsMemoryLedgerRelocInterfaceBytes, gNdsMemoryLedgerRelocMenuBytes, ' +
         'gNdsMemoryLedgerRelocOpeningBytes, gNdsMemoryLedgerRelocOtherBytes, ' +
         'gNdsMemoryLedgerRelocStaleBytes'),
        ('printf "WALL=%u,%u,%u,%u,%u,%u,%u\n", ' +
         'gNdsSObjWallpaperCacheBuildCount, gNdsSObjWallpaperCacheHitCount, ' +
         'gNdsSObjWallpaperCacheFastDrawCount, ' +
         'gNdsSObjWallpaperCacheFallbackCount, gNdsSObjWallpaperCacheWidth, ' +
         'gNdsSObjWallpaperCacheHeight, gNdsSObjWallpaperCacheOpaquePixels'),
        ('printf "WALLFINAL=%u,%u,%u,%u\n", ' +
         'gNdsSObjWallpaperFinalDirectCount, gNdsSObjWallpaperFinalSkipCount, ' +
         'gNdsSObjWallpaperFinalKeyChangeCount, ' +
         'gNdsSObjWallpaperFinalPixelWriteCount')
    )
    if ($StageRouteProbe) {
        # NDS_R2_STAGE_ROUTE_PROBE keeps the native-stage reject reasons alive
        # at profile level 0.  The latch values alone describe only the last
        # prepare, so publish the counting array and reuse-key misses too.
        $gdbLines += @(
            'set $tc_live = 0',
            'set $tc_this = 0',
            'set $tc_next = 0',
            'set $tc_i = 24',
            'while $tc_i < (sizeof(sNdsRendererHardwareTextureCache) / sizeof(sNdsRendererHardwareTextureCache[0]))',
            'if sNdsRendererHardwareTextureCache[$tc_i].name != 0',
            'set $tc_live = $tc_live + 1',
            'if sNdsRendererHardwareTextureCache[$tc_i].last_used_frame == sNdsRendererHardwareFrameSerial',
            'set $tc_this = $tc_this + 1',
            'end',
            'if sNdsRendererHardwareTextureCache[$tc_i].last_used_frame == (sNdsRendererHardwareFrameSerial + 1)',
            'set $tc_next = $tc_next + 1',
            'end',
            'end',
            'set $tc_i = $tc_i + 1',
            'end',
            ('printf "TEXLIVE=%u,%u,%u,%u,%u\n", $tc_live, $tc_this, $tc_next, ' +
             '(unsigned int)(sizeof(sNdsRendererHardwareTextureCache) / sizeof(sNdsRendererHardwareTextureCache[0])), ' +
             'sNdsRendererHardwareFrameSerial'),
            ('printf "TEXHIGH=%u,%u\n", ' +
             'gNdsR2TextureLiveHighWater, gNdsR2TextureTouchedHighWater'),
            'set $static_i = 0',
            ('while $static_i < (sizeof(sNdsRendererHardwareStaticKeyPointers) / ' +
             'sizeof(sNdsRendererHardwareStaticKeyPointers[0]))'),
            ('printf "STATICUSE=%u,%u,%u,%u,%u\n", $static_i, ' +
             'sNdsRendererHardwareTextureCache[$static_i].static_record_plus1, ' +
             'sNdsRendererHardwareTextureCache[$static_i].last_used_frame, ' +
             'sNdsRendererHardwareTextureCache[$static_i].ready, ' +
             'sNdsRendererHardwareTextureCache[$static_i].pinned'),
            'set $static_i = $static_i + 1',
            'end',
            ('printf "STAGEROUTE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
             'gNdsR2StageKeyMissInvalid, gNdsR2StageKeyMissGeneration, ' +
             'gNdsR2StageKeyMissStamp, gNdsR2StageKeyMissConfig, ' +
             'gNdsR2StageKeyMissAssets, ' +
             'gNdsR2StageRejectCounts[1], gNdsR2StageRejectCounts[2], ' +
             'gNdsR2StageRejectCounts[3], gNdsR2StageRejectCounts[4], ' +
             'gNdsR2StageRejectCounts[5], gNdsR2StageRejectCounts[6], ' +
             'gNdsR2StagePrepareBuildCount, gNdsR2StagePrepareReuseCount, ' +
             'gNdsRendererProfileTextureRejectReasonMask'),
            ('printf "STAGELATCH=%u,%u,%u,%u,%u,%u\n", ' +
             'gNdsRendererStageOwnerRejectCount, ' +
             'gNdsRendererStageOwnerFirstRejectReason, ' +
             'gNdsRendererStageOwnerLastRejectReason, ' +
             'gNdsRendererTask36RendererRejectReason, ' +
             'gNdsRendererTask36PrepareRunRejectReason, ' +
             'gNdsR2TexRejectCensusValid'),
            ('printf "TEXCENSUS=%u,%u,%u,%u,%u,%u\n", ' +
             'gNdsR2TexRejectCensusFree, gNdsR2TexRejectCensusLive, ' +
             'gNdsR2TexRejectCensusPinned, gNdsR2TexRejectCensusThisFrame, ' +
             'gNdsR2TexRejectCensusEvictable, gNdsRendererProfileTextureRejectReasonMask'),
            ('printf "STAGETEXMISS=%u,%u,%08x,%u,%u\n", ' +
             'gNdsR2StageTextureMissCount, gNdsR2StageTextureMissRun, ' +
             'gNdsR2StageTextureMissHash, gNdsR2StageTextureMissArmed, ' +
              'gNdsR2StageTextureMissSourceFrameTried'),
            ('printf "STAGETEXPROV=%u,%08x,%u,%08x\n", ' +
             'gNdsR2StageTextureMissImageAsset, ' +
             'gNdsR2StageTextureMissImageOffset, ' +
             'gNdsR2StageTextureMissTlutAsset, ' +
             'gNdsR2StageTextureMissTlutOffset'),
            ('printf "STAGETEXKEY0=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n", ' +
             'gNdsR2StageTextureMissKeyWords[0],gNdsR2StageTextureMissKeyWords[1],gNdsR2StageTextureMissKeyWords[2],gNdsR2StageTextureMissKeyWords[3],gNdsR2StageTextureMissKeyWords[4],gNdsR2StageTextureMissKeyWords[5],gNdsR2StageTextureMissKeyWords[6],gNdsR2StageTextureMissKeyWords[7],gNdsR2StageTextureMissKeyWords[8],gNdsR2StageTextureMissKeyWords[9],gNdsR2StageTextureMissKeyWords[10],gNdsR2StageTextureMissKeyWords[11],gNdsR2StageTextureMissKeyWords[12],gNdsR2StageTextureMissKeyWords[13],gNdsR2StageTextureMissKeyWords[14],gNdsR2StageTextureMissKeyWords[15],gNdsR2StageTextureMissKeyWords[16],gNdsR2StageTextureMissKeyWords[17],gNdsR2StageTextureMissKeyWords[18],gNdsR2StageTextureMissKeyWords[19],gNdsR2StageTextureMissKeyWords[20],gNdsR2StageTextureMissKeyWords[21],gNdsR2StageTextureMissKeyWords[22],gNdsR2StageTextureMissKeyWords[23],gNdsR2StageTextureMissKeyWords[24],gNdsR2StageTextureMissKeyWords[25],gNdsR2StageTextureMissKeyWords[26],gNdsR2StageTextureMissKeyWords[27],gNdsR2StageTextureMissKeyWords[28],gNdsR2StageTextureMissKeyWords[29],gNdsR2StageTextureMissKeyWords[30],gNdsR2StageTextureMissKeyWords[31],gNdsR2StageTextureMissKeyWords[32]'),
            ('printf "STAGETEXKEY1=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n", ' +
             'gNdsR2StageTextureMissKeyWords[33],gNdsR2StageTextureMissKeyWords[34],gNdsR2StageTextureMissKeyWords[35],gNdsR2StageTextureMissKeyWords[36],gNdsR2StageTextureMissKeyWords[37],gNdsR2StageTextureMissKeyWords[38],gNdsR2StageTextureMissKeyWords[39],gNdsR2StageTextureMissKeyWords[40],gNdsR2StageTextureMissKeyWords[41],gNdsR2StageTextureMissKeyWords[42],gNdsR2StageTextureMissKeyWords[43],gNdsR2StageTextureMissKeyWords[44],gNdsR2StageTextureMissKeyWords[45],gNdsR2StageTextureMissKeyWords[46],gNdsR2StageTextureMissKeyWords[47],gNdsR2StageTextureMissKeyWords[48],gNdsR2StageTextureMissKeyWords[49],gNdsR2StageTextureMissKeyWords[50],gNdsR2StageTextureMissKeyWords[51],gNdsR2StageTextureMissKeyWords[52],gNdsR2StageTextureMissKeyWords[53],gNdsR2StageTextureMissKeyWords[54],gNdsR2StageTextureMissKeyWords[55],gNdsR2StageTextureMissKeyWords[56],gNdsR2StageTextureMissKeyWords[57],gNdsR2StageTextureMissKeyWords[58]'),
            ('printf "MISCACC=%u,%u,%u\n", ' +
             'gNdsMiscWeaponDrawTicks,gNdsMiscEffectDrawTicks,gNdsMiscParticleDrawTicks'),
            ('printf "EFPHASE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
             'gNdsEffectPhaseColorTicks,gNdsEffectPhaseTreeTicks,gNdsEffectPhaseDLTicks,' +
             'gNdsEffectPhaseFindTicks,gNdsEffectPhaseMaterialTicks,gNdsEffectPhaseMatrixTicks,' +
             'gNdsEffectPhaseExecTicks,gNdsEffectPhaseTexTicks,gNdsEffectPhaseVtxTicks,' +
             'gNdsEffectPhaseTriTicks,gNdsEffectPhaseTexInExecTicks')
        )
    }
    if ($AnimCacheProbe) {
        $gdbLines += @(
            ('printf "ANIMRING=%u,%u,%u,%u,%u,%u\n", ' +
             'sNdsR2AnimCacheCount, gNdsR2AnimCacheBytes, ' +
             'gNdsR2AnimCacheArenaUsedBytes, gNdsR2AnimCacheArenaReservedBytes, ' +
             'gNdsR2AnimCacheHits, gNdsR2AnimCacheMisses'),
            'set $anim_i = 0',
            'while $anim_i < sNdsR2AnimCacheCount',
            ('printf "ANIMENTRY=%u,%u,%u,%u\n", $anim_i, ' +
             'sNdsR2AnimCache[$anim_i].asset_id, ' +
             'sNdsR2AnimCache[$anim_i].size, ' +
             '(unsigned int)sNdsR2AnimCache[$anim_i].payload - ' +
             '(unsigned int)sNdsR2AnimCacheArena'),
            'set $anim_i = $anim_i + 1',
            'end'
        )
    }
    if ($LoadedFileProbe) {
        $gdbLines += @(
            'set $lf = 0',
            'printf "LOADED_COUNT=%u\\n", sNdsRelocLoadedFileCount',
            'while $lf < sNdsRelocLoadedFileCount',
            'printf "LOADED=%u,%u,%p,0x%x,%u\\n", $lf, sNdsRelocLoadedFiles[$lf].asset_id, sNdsRelocLoadedFiles[$lf].data, sNdsRelocLoadedFiles[$lf].data_size, sNdsRelocLoadedFiles[$lf].owner_generation',
            'set $lf = $lf + 1',
            'end'
        )
    }
    if ($PacketFaultCensus) {
        $gdbLines += @(
            'printf "FAULTCAUSE=%u,%u,%u,%u,%u,%u,%u\n", $pf_split, $pf_raw_textured, $pf_raw_untextured, $pf_site, $pf_local, $pf_matrix, $pf_capacity'
        )
    }
    if ($WeaponPoolCensus) {
        $gdbLines += @(
            'printf "WEAPONPOOL=%u,%u,%u,%u,%u\n", $wp_live_max, $wp_live, $wp_allocs, $wp_frees, gNdsWeaponPoolEntries'
        )
    }
    }
    if ($FirstProductionAfterFrame) {
        # Install this only after the sparse checkpoint. Breakpointing production
        # from boot would stop on menu/seed work and would not answer whether the
        # four-CPU battle's first real owner uses the source-required Low tree.
        $gdbLines += @(
            'delete',
            'break ndsRendererExecuteNativeFighterOwnerProduction',
            'continue',
            ('printf "PRODUCTION=%u,%u,%u,%u,%u,%u,%u\n", ' +
             'gNdsBattlePlayablePacingPresentedFrames, ' +
             'gNdsBattlePlayablePacingLogicFrames, slot, use_low_detail, ' +
             'input_count, gNdsFighterStructP0Detail, gNdsFighterStructP1Detail'),
            ('printf "PRODTABLE=%p,%p,%p\n", ' +
             'sNdsNativeFighterActiveTables, ' +
             '&sNdsNativeFighterLowTables, &sNdsNativeFighterHighTables')
        )
    }
    $gdbLines += @('detach', 'quit')
    [System.IO.File]::WriteAllLines($gdbScript, $gdbLines)
    if ($WeaponPoolCensus) {
        Copy-Item -LiteralPath $gdbScript -Destination ($Artifact + '.gdb') -Force
    }

    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        $partial = Get-Content $gdbOut -Raw -ErrorAction SilentlyContinue
        if (-not [string]::IsNullOrWhiteSpace($partial)) {
            $artifactDir = Split-Path -Parent $Artifact
            if ($artifactDir) {
                New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
            }
            Set-Content -LiteralPath $Artifact -Value $partial
        }
        throw "P2-2 sparse frame-32 probe exceeded ${TimeoutSeconds}s:`n$partial"
    }
    if ($gdbProcess.ExitCode -ne 0) {
        throw "P2-2 sparse GDB probe failed: $(Get-Content $gdbErr -Raw)"
    }
    $output = Get-Content $gdbOut -Raw
    if ($FirstPacketFault -or $FirstActualPacketFault -or $FirstTextureReject -or $FirstDirectReject -or $FighterTextureReject -or $PhysicalSpanFault -or $FirstPoseBindFull -or $FirstKirbyReject -or $FirstDonkeyReject -or $FirstSamusReject -or $FirstCutterReject -or $FirstSwordReject) {
        if ($FirstKirbyReject -and
            ($output -notmatch 'KIRBYREJECT=') -and
            ($output -notmatch 'KIRBYREJECT_NONE_THROUGH=')) {
            throw "Kirby reject lifetime probe reached neither terminal site:`n$output"
        }
        if ($FirstKirbyReject) {
            $artifactDir = Split-Path -Parent $Artifact
            if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
            Set-Content -LiteralPath $Artifact -Value $output
            Write-Output $output
            Write-Output "Wrote $Artifact"
            return
        }
        if ($FirstDonkeyReject -and
            ($output -notmatch 'DONKEYREJECT=') -and
            ($output -notmatch 'DONKEYREJECT_NONE_THROUGH=')) {
            throw "Donkey reject lifetime probe reached neither terminal site:`n$output"
        }
        if ($FirstDonkeyReject) {
            $artifactDir = Split-Path -Parent $Artifact
            if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
            Set-Content -LiteralPath $Artifact -Value $output
            Write-Output $output
            Write-Output "Wrote $Artifact"
            return
        }
        if ($FirstSamusReject -and
            ($output -notmatch 'SAMUSREJECT=') -and
            ($output -notmatch 'SAMUSGRAPPLEREJECT=') -and
            ($output -notmatch 'SAMUSREJECT_NONE_THROUGH=')) {
            throw "Samus reject lifetime probe reached neither terminal site:`n$output"
        }
        if ($FirstSamusReject) {
            $artifactDir = Split-Path -Parent $Artifact
            if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
            Set-Content -LiteralPath $Artifact -Value $output
            Write-Output $output
            Write-Output "Wrote $Artifact"
            return
        }
        if ($FirstCutterReject -and
            ($output -notmatch 'CUTTERREJECT=') -and
            ($output -notmatch 'CUTTERREJECT_NONE_THROUGH=')) {
            throw "Cutter reject probe did not reach a terminal witness:`n$output`nGDBERR:`n$errors"
        }
        if ($FirstCutterReject) {
            $artifactDir = Split-Path -Parent $Artifact
            New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
            Set-Content -LiteralPath $Artifact -Value $output
            Write-Output $output
            Write-Output "Wrote $Artifact"
            return
        }
        if ($FirstSwordReject -and
            ($output -notmatch 'SWORDREJECT=') -and
            ($output -notmatch 'SWORDREJECT_NONE_THROUGH=')) {
            throw "Sword reject lifetime probe reached neither terminal site:`n$output"
        }
        if ($FirstSwordReject) {
            $artifactDir = Split-Path -Parent $Artifact
            if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
            Set-Content -LiteralPath $Artifact -Value $output
            Write-Output $output
            Write-Output "Wrote $Artifact"
            return
        }
        if ($FirstPoseBindFull -and (-not (($output -join "`n") -match 'POSEFULL='))) {
            throw "Pose-bind-full probe never reached a refused bind:`n$output"
        }
        if ($PhysicalSpanFault -and ($output -notmatch 'SPANFAULT=')) {
            throw "Physical span fault probe never reached overflow:`n$output"
        }
        if ($FighterTextureReject -and ($output -notmatch 'FIGHTERTEXREJECT=')) {
            throw "Fighter texture reject probe never reached a reject site:`n$output"
        }
        if ($FirstDirectReject -and ($output -notmatch 'DIRECTREJECT=')) {
            throw "First direct reject probe never reached a reject site:`n$output"
        }
        if ($FirstTextureReject -and ($output -notmatch 'TEXTUREREJECT=')) {
            throw "First texture reject probe never reached a reject site:`n$output"
        }
        if ($output -notmatch 'FIRSTFAULT=') {
            if ($FirstActualPacketFault -and ($output -notmatch 'ACTUALFAULT=')) {
                throw "First actual packet fault probe never reached a fault site:`n$output"
            }
            elseif (-not $FirstActualPacketFault) {
                throw "First packet fault probe never reached a known fault site:`n$output"
            }
        }
        $artifactDir = Split-Path -Parent $Artifact
        if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
        Set-Content -LiteralPath $Artifact -Value $output
        Write-Output $output
        Write-Output "Wrote $Artifact"
        return
    }
    if ($output -notmatch ("SPARSE=$Frame,")) {
        if ($PackFailureProbe -and ($output -match 'PACKFAIL=')) {
            $artifactDir = Split-Path -Parent $Artifact
            if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
            Set-Content -LiteralPath $Artifact -Value $output
            throw "Compact fighter pack failed before sparse frame ${Frame}:`n$output"
        }
        $errors = Get-Content $gdbErr -Raw -ErrorAction SilentlyContinue
        throw "Sparse probe did not stop on presented frame ${Frame}:`n$output`nGDBERR:`n$errors"
    }
    if ($FirstProductionAfterFrame -and ($output -notmatch 'PRODUCTION=')) {
        throw "Sparse probe never reached fighter production after frame ${Frame}:`n$output"
    }

    $artifactDir = Split-Path -Parent $Artifact
    if ($artifactDir) { New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null }
    Set-Content -LiteralPath $Artifact -Value $output
    Write-Output $output
    Write-Output "Wrote $Artifact"
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
}
