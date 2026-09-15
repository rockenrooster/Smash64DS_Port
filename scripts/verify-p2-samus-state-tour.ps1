param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateSet(7,8)][int]$RunnerSlot = 7,
    [int]$DelaySeconds = 0,
    [string]$Build = 'build-p2-samus-tour',
    [switch]$NoBuild,
    [ValidateRange(240,900)][int]$TimeoutSeconds = 600
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-SamusTour {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if (-not $Condition) {
        if ($Evidence) { throw "$Message`n$Evidence" }
        throw $Message
    }
}

$target = 'smash64ds-battle-playable-fast-hwtri'
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir "$target.nds"
$elf = Join-Path $buildDir "$target.elf"
$config = Join-Path $buildDir 'nds_build_config.h'
$sceneConfig = Join-Path $buildDir 'nds_scene_harness_config.h'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_SAMUS=1' 'NDS_P2_PROOF_FIGHTER0=3' `
        'NDS_P2_SAMUS_STATE_TOUR=1' `
        'NDS_HARNESS_FAST_PRESENT_ON_REQUEST=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig, $nm)) {
    Assert-SamusTour (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus state-tour proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_P2_SAMUS_STATE_TOUR 1',
    '#define NDS_HARNESS_FAST_PRESENT_ON_REQUEST 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-SamusTour $configText.Contains($definition) `
        "Samus state-tour build is missing required definition: $definition" $config
}
Assert-SamusTour $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Samus state-tour proof must run the mode-163 battle_playable harness.' $sceneConfig

$elfSymbols = @(& $nm -a $elf)
Assert-SamusTour ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = $elfSymbols | Where-Object {
        $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$"
    } | Select-Object -First 1
    Assert-SamusTour ($null -ne $line) "ELF symbol not found: $Name"
    $m = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($m.Groups[1].Value, 16))
}
$battleStart = Get-ElfSymbolAddress 'scVSBattleStartBattle'
$osStopThread = Get-ElfSymbolAddress 'osStopThread'

# The acceptance claim is specifically source-selected states. Keep every
# state-tour helper free of direct status/motion injection: morph actions are
# controller playback, and ledge staging only establishes collision geometry.
$movementPath = Join-Path $root 'src\port\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$tourStart = $movementText.IndexOf('static void ndsSamusStateTourRecord')
$tourEnd = $movementText.IndexOf('#endif', $tourStart)
Assert-SamusTour (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the Samus state-tour implementation for injection guard checks.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
Assert-SamusTour ($tourText -notmatch 'ftMainSetStatus\s*\(') `
    'Samus state-tour guest setup may not call ftMainSetStatus.'
Assert-SamusTour ($tourText -notmatch 'samus->status_id\s*=(?!=)') `
    'Samus state-tour guest setup may not assign status_id.'
Assert-SamusTour ($tourText -notmatch 'samus->motion_id\s*=(?!=)') `
    'Samus state-tour guest setup may not assign motion_id.'

$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path $ctx.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null
    # The fast mode-163 ROM is intentionally unthrottled.  Install the one-shot
    # battle-start breakpoint as soon as the GDB listener exists; a wall-clock
    # delay here can let scVSBattleStartBattle execute before GDB attaches and
    # turn a healthy proof into a timeout waiting for a call that already ran.
    if ($DelaySeconds -gt 0) {
        throw 'DelaySeconds must remain 0 for the fast Samus state-tour proof.'
    }

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        # P2 production proofs must enter the battle before the scene-boundary
        # stop is armed; startup/menu teardowns also publish a boundary result.
        ('tbreak *0x{0:x8}' -f $battleStart),
        'continue',
        'set $sm_select2 = 0',
        'set $sm_select3 = 0',
        'set $sm_tri2 = 0',
        'set $sm_tri3 = 0',
        'set $sm_restored = 0',
        'set $sm_rejects = 0',
        'set $sm_program2_count = 0',
        'set $sm_program3_count = 0',
        'set $sm_pending = 0',
        'set $sm_pending_bit = 0',
        'set $sm_pending_status = 0',
        'set $sm_tri_base = 0',
        'set $sm_last = 0',
        'set $sm_restore_candidate = 0',
        'set $nf_base_seen = 0',
        'set $nf_count0 = 0',
        'set $nf_domain0 = 0',
        'set $nf_scene0 = 0',
        'set $nf_identity0 = 0',
        'set $nf_status0 = 0',
        'set $nf_root0 = 0',
        'set $nf_material0 = 0',
        'set $nf_reason0 = 0',
        # The first call is the handoff from the prerequisite common-moveset
        # proof into this tour. Snapshot the sticky native-failure tuple here;
        # any renderer failure during a morph state must change it.
        'tbreak ndsSamusStateTourAdvance',
        'commands', 'silent',
        'if gSCManagerBattleState == 0 || gSCManagerBattleState->players[0].fighter_gobj == 0',
        'printf "SAMUSMORPH_ROSTER_INVALID=missing_slot0\\n"',
        'detach', 'quit 2', 'end',
        'set $sm_fp = (FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p',
        'if $sm_fp->fkind != 3 || $sm_fp->nds_slot != 0',
        'printf "SAMUSMORPH_ROSTER_INVALID=kind:%u,slot:%u\\n", $sm_fp->fkind, $sm_fp->nds_slot',
        'detach', 'quit 2', 'end',
        'set $nf_count0 = gNdsRendererNativeFailure.count',
        'set $nf_domain0 = gNdsRendererNativeFailure.domain',
        'set $nf_scene0 = gNdsRendererNativeFailure.scene',
        'set $nf_identity0 = gNdsRendererNativeFailure.identity',
        'set $nf_status0 = gNdsRendererNativeFailure.status',
        'set $nf_root0 = gNdsRendererNativeFailure.root',
        'set $nf_material0 = gNdsRendererNativeFailure.material',
        'set $nf_reason0 = gNdsRendererNativeFailure.reason',
        'set $nf_base_seen = 1',
        'continue', 'end',
        # Programs 2/3 are selected from the live root vector. Restrict this
        # witness to this tour so the prerequisite GuardRoll cannot qualify it.
        'break ndsRendererNativeFighterSetRootProgram if slot == 5 && sNdsSamusStateTourActive != 0 && sNdsSamusStateTourDone == 0 && (program == 2 || program == 3)',
        'commands', 'silent',
        'set $sm_fp = (FTStruct*)gSCManagerBattleState->players[0].fighter_gobj->user_data.p',
        'set $sm_status = $sm_fp->status_id',
        'set $sm_bit = 0',
        'if $sm_status == 0x9c', 'set $sm_bit = 1', 'end',
        'if $sm_status == 0x9d', 'set $sm_bit = 2', 'end',
        'if $sm_status == 0x61', 'set $sm_bit = 4', 'end',
        'if $sm_status == 0x63', 'set $sm_bit = 8', 'end',
        'if $sm_status == 0xe5', 'set $sm_bit = 16', 'end',
        'if $sm_status == 0xe6', 'set $sm_bit = 32', 'end',
        'if $sm_bit != 0',
        'if program == 2',
        'set $sm_program2_count = $sm_program2_count + 1',
        'set $sm_seen = $sm_select2',
        'else',
        'set $sm_program3_count = $sm_program3_count + 1',
        'set $sm_seen = $sm_select3',
        'end',
        'if ($sm_seen & $sm_bit) == 0',
        'if $sm_pending != 0',
        'printf "SAMUSMORPH_PENDING_COLLISION=frame:%u,status:0x%x,program:%u,pending:%u\\n", gNdsBattlePlayablePacingPresentedFrames, $sm_status, program, $sm_pending',
        'detach', 'quit 2', 'end',
        'if program == 2',
        'set $sm_select2 = $sm_select2 | $sm_bit',
        'else',
        'set $sm_select3 = $sm_select3 | $sm_bit',
        'end',
        'set $sm_pending = program',
        'set $sm_pending_bit = $sm_bit',
        'set $sm_pending_status = $sm_status',
        'set $sm_tri_base = gNdsFighterDLAllDrawP0HardwareTriangleCount',
        'set $sm_last = gNdsBattlePlayablePacingPresentedFrames',
        'set $sm_restore_candidate = $sm_bit',
        'printf "SAMUSMORPHSELECT=frame:%u,status:0x%x,bit:0x%x,program:%u,p0tri:%u\\n", $sm_last, $sm_status, $sm_bit, program, $sm_tri_base',
        'end',
        'end',
        'continue', 'end',
        # A native rejection is the requested terminal finding. Dump the live
        # root vector and both generated morph program vectors before exiting.
        'break ndsFighterRejectNativeRender if sNdsSamusStateTourActive != 0 && fp->fkind == 3 && (fp->status_id == 0x9c || fp->status_id == 0x9d || fp->status_id == 0x61 || fp->status_id == 0x63 || fp->status_id == 0xe5 || fp->status_id == 0xe6)',
        'commands', 'silent',
        'set $sm_rejects = $sm_rejects + 1',
        'printf "SAMUSMORPHREJECT=frame:%u,status:0x%x,program:%u,decline:%u,selected:%u,detail:0x%x,reason:%u\\n", gNdsBattlePlayablePacingPresentedFrames, fp->status_id, sNdsNativeFighterRootPrograms[5], gNdsFtrDeclineStage, gNdsFtrDeclineSelected, gNdsFtrDeclineDetail, reason',
        'printf "SAMUSMORPHFAIL=%u,%u,%u,%u,%u,%u,%u,%u\\n", gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.scene, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, gNdsRendererNativeFailure.material, gNdsRendererNativeFailure.reason',
        'set $sr = 0',
        'while $sr < gNdsFtrDeclineSelected',
        'printf "SAMUSMORPHOBSROOT=%u,0x%x\\n", $sr, sNdsRendererAdapterNativeOwnerWorkspace.root_offsets[$sr]',
        'set $sr = $sr + 1', 'end',
        'printf "SAMUSMORPHPROGROOT=2,0,0x8158\\n"',
        'printf "SAMUSMORPHPROGROOT=3,0,0x8708\\n"',
        'bt 8', 'detach', 'quit 3', 'end',
        # The sparse morph probe's key invariant: a newly selected single-root
        # owner must add P0 hardware triangles at the next completed frame.
        'break ndsBattlePlayableFrameCompleteMarker if $sm_pending != 0 || ($sm_restore_candidate != 0 && ($sm_tri2 & $sm_restore_candidate) != 0 && ($sm_tri3 & $sm_restore_candidate) != 0 && sNdsNativeFighterRootPrograms[5] == 0 && gNdsBattlePlayablePacingPresentedFrames > $sm_last)',
        'commands', 'silent',
        'if $sm_pending != 0',
        'if sNdsNativeFighterRootPrograms[5] == $sm_pending && gNdsBattlePlayablePacingPresentedFrames == $sm_last + 1 && gNdsFighterDLAllDrawP0HardwareTriangleCount > $sm_tri_base',
        'set $sm_triangles = gNdsFighterDLAllDrawP0HardwareTriangleCount - $sm_tri_base',
        'if $sm_pending == 2',
        'set $sm_tri2 = $sm_tri2 | $sm_pending_bit',
        'else',
        'set $sm_tri3 = $sm_tri3 | $sm_pending_bit',
        'end',
        'printf "SAMUSMORPHSUBMIT=frame:%u,status:0x%x,bit:0x%x,program:%u,triangles:%u,tex:0x%x\\n", gNdsBattlePlayablePacingPresentedFrames, $sm_pending_status, $sm_pending_bit, $sm_pending, $sm_triangles, gNdsRendererProfileTextureRejectReasonMask',
        'else',
        'printf "SAMUSMORPH_SUBMIT_FAIL=frame:%u,last:%u,status:0x%x,program:%u,selected:%u,base:%u,now:%u\\n", gNdsBattlePlayablePacingPresentedFrames, $sm_last, $sm_pending_status, $sm_pending, sNdsNativeFighterRootPrograms[5], $sm_tri_base, gNdsFighterDLAllDrawP0HardwareTriangleCount',
        'detach', 'quit 2',
        'end',
        'set $sm_pending = 0',
        'end',
        'if $sm_restore_candidate != 0 && ($sm_tri2 & $sm_restore_candidate) != 0 && ($sm_tri3 & $sm_restore_candidate) != 0 && sNdsNativeFighterRootPrograms[5] == 0 && gNdsBattlePlayablePacingPresentedFrames > $sm_last',
        'set $sm_restored = $sm_restored | $sm_restore_candidate',
        'printf "SAMUSMORPHRESTORED=frame:%u,bit:0x%x\\n", gNdsBattlePlayablePacingPresentedFrames, $sm_restore_candidate',
        'set $sm_restore_candidate = 0',
        'end',
        'continue', 'end',
        ('tbreak *0x{0:x8} if gNdsSceneBoundaryResult != 0' -f $osStopThread),
        'continue',
        'printf "SAMUS_STATE_TOUR=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%d,%#x,%u,%u,%u\\n",gNdsSamusStateTourPhase,gNdsSamusStateTourPhaseFrames,sNdsSamusStateTourScenario,sNdsSamusStateTourStep,sNdsSamusStateTourFrames,sNdsSamusStateTourActive,sNdsSamusStateTourDone,gNdsSamusStateTourStageCount,gNdsSamusStateTourMask,gNdsSamusStateTourStatus,gNdsSamusStateTourMotion,gNdsSamusStateTourCliffID,gNdsFighterNaturalMovesetMask,gNdsFighterNaturalCombatStallCount,gNdsFighterNaturalCombatRollFrames,gNdsFighterNaturalCombatRollStatus',
        'printf "SAMUS_MORPH_FINAL=select2:%#x,select3:%#x,tri2:%#x,tri3:%#x,restored:%#x,rejects:%u,tex:%#x,count2:%u,count3:%u,finalProgram:%u,baseline:%u\\n", $sm_select2, $sm_select3, $sm_tri2, $sm_tri3, $sm_restored, $sm_rejects, gNdsRendererProfileTextureRejectReasonMask, $sm_program2_count, $sm_program3_count, sNdsNativeFighterRootPrograms[5], $nf_base_seen',
        'printf "SAMUS_MORPH_FAIL_BASE=%u,%u,%u,%u,%u,%u,%u,%u\\n", $nf_count0, $nf_domain0, $nf_scene0, $nf_identity0, $nf_status0, $nf_root0, $nf_material0, $nf_reason0',
        'printf "SAMUS_MORPH_FAIL_FINAL=%u,%u,%u,%u,%u,%u,%u,%u\\n", gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.domain, gNdsRendererNativeFailure.scene, gNdsRendererNativeFailure.identity, gNdsRendererNativeFailure.status, gNdsRendererNativeFailure.root, gNdsRendererNativeFailure.material, gNdsRendererNativeFailure.reason',
        'detach',
        'quit'
    )
    # Accurate-cache mode can spend more than 120 s walking CSS/entry on a cold
    # host before mode 163 reaches the battle.  This is a ceiling, not a sleep;
    # successful runs still return as soon as the guest publishes the bounded
    # battle teardown marker.
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-samus-state-tour.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-samus-state-tour.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout,
        'SAMUS_STATE_TOUR=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+),(-?\d+),(0x[0-9a-fA-F]+),(\d+),(\d+),(\d+)')
    $morph = [regex]::Match($stdout,
        'SAMUS_MORPH_FINAL=select2:(0x[0-9a-fA-F]+),select3:(0x[0-9a-fA-F]+),tri2:(0x[0-9a-fA-F]+),tri3:(0x[0-9a-fA-F]+),restored:(0x[0-9a-fA-F]+),rejects:(\d+),tex:(0x[0-9a-fA-F]+),count2:(\d+),count3:(\d+),finalProgram:(\d+),baseline:(\d+)')
    $failBase = [regex]::Match($stdout,
        'SAMUS_MORPH_FAIL_BASE=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)')
    $failFinal = [regex]::Match($stdout,
        'SAMUS_MORPH_FAIL_FINAL=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)')
    $submits = @([regex]::Matches($stdout,
        'SAMUSMORPHSUBMIT=frame:(\d+),status:0x([0-9a-fA-F]+),bit:0x([0-9a-fA-F]+),program:(\d+),triangles:(\d+),tex:0x([0-9a-fA-F]+)'))
    Assert-SamusTour $match.Success 'Samus state-tour marker is missing.' $stdout
    Assert-SamusTour $morph.Success 'Samus morph native-owner marker is missing.' $stdout
    Assert-SamusTour ($failBase.Success -and $failFinal.Success) `
        'Samus morph native-failure tuple markers are missing.' $stdout
    $v = @($match.Groups[1..16] | ForEach-Object { $_.Value })
    $mask = [Convert]::ToUInt32($v[8].Substring(2), 16)
    $naturalMask = [Convert]::ToUInt32($v[12].Substring(2), 16)
    Assert-SamusTour ([int]$v[0] -eq 10) 'Samus state tour did not reach Done phase.' $stdout
    Assert-SamusTour ([int]$v[2] -eq 10) 'Samus state tour did not complete all ten source scenarios.' $stdout
    Assert-SamusTour ([int]$v[3] -eq 7) 'Samus state tour did not finish through the Recover step.' $stdout
    Assert-SamusTour ([int]$v[5] -eq 1 -and [int]$v[6] -eq 1) `
        'Samus state tour never armed or never published Done.' $stdout
    Assert-SamusTour ([int]$v[7] -eq 12) `
        'Samus ledge scenarios must perform exactly two guest precondition stages each.' $stdout
    Assert-SamusTour ($mask -eq 0x1fffff) `
        ('Samus state tour did not visit the complete morph + Fall/Cliff mask: got 0x{0:x}' -f $mask) $stdout
    Assert-SamusTour ($naturalMask -eq 0x7ff) `
        ('The prerequisite controller-driven common moveset regressed: got 0x{0:x}' -f $naturalMask) $stdout
    Assert-SamusTour ([int]$v[13] -eq 0) 'Samus state tour accumulated a natural-combat stall.' $stdout
    Assert-SamusTour ([int]$v[14] -gt 0 -and [int]$v[15] -in @(156, 157)) `
        'Samus prerequisite combat did not enter BattleShip EscapeF/EscapeB from guarded fresh-stick input.' $stdout

    $morphValues = @($morph.Groups[1..11] | ForEach-Object { $_.Value })
    $select2 = [Convert]::ToUInt32($morphValues[0].Substring(2), 16)
    $select3 = [Convert]::ToUInt32($morphValues[1].Substring(2), 16)
    $tri2 = [Convert]::ToUInt32($morphValues[2].Substring(2), 16)
    $tri3 = [Convert]::ToUInt32($morphValues[3].Substring(2), 16)
    $restored = [Convert]::ToUInt32($morphValues[4].Substring(2), 16)
    $texReject = [Convert]::ToUInt32($morphValues[6].Substring(2), 16)
    foreach ($namedMask in @(
        @{ Name = 'program-2 selection'; Value = $select2 },
        @{ Name = 'program-3 selection'; Value = $select3 },
        @{ Name = 'program-2 next-frame triangles'; Value = $tri2 },
        @{ Name = 'program-3 next-frame triangles'; Value = $tri3 },
        @{ Name = 'program-0 restoration'; Value = $restored }
    )) {
        Assert-SamusTour ($namedMask.Value -eq 0x3f) `
            ("Samus morph $($namedMask.Name) mask is incomplete: got 0x{0:x}." -f $namedMask.Value) $stdout
    }
    Assert-SamusTour ([int]$morphValues[5] -eq 0) `
        'Samus morph tour hit ndsFighterRejectNativeRender.' $stdout
    Assert-SamusTour ($texReject -eq 0) `
        ('Samus morph tour accumulated a texture reject: mask=0x{0:x}.' -f $texReject) $stdout
    Assert-SamusTour ([int]$morphValues[7] -gt 0 -and [int]$morphValues[8] -gt 0) `
        'Samus morph programs 2/3 were not selected.' $stdout
    Assert-SamusTour ([int]$morphValues[9] -eq 0) `
        'Samus native root program was not restored to canonical program 0.' $stdout
    Assert-SamusTour ([int]$morphValues[10] -eq 1) `
        'Samus native-failure baseline was not sampled at state-tour entry.' $stdout

    $baseTuple = @($failBase.Groups[1..8] | ForEach-Object { $_.Value })
    $finalTuple = @($failFinal.Groups[1..8] | ForEach-Object { $_.Value })
    Assert-SamusTour (($baseTuple -join ',') -eq ($finalTuple -join ',')) `
        ("Samus state tour changed the sticky native-failure tuple: base=$($baseTuple -join ',') final=$($finalTuple -join ',').") $stdout

    $stateSpecs = @(
        [pscustomobject]@{ Name = 'rollf'; Bit = 0x01; Status = 0x9c },
        [pscustomobject]@{ Name = 'rollb'; Bit = 0x02; Status = 0x9d },
        [pscustomobject]@{ Name = 'ground-bomb'; Bit = 0x10; Status = 0xe5 },
        [pscustomobject]@{ Name = 'air-bomb'; Bit = 0x20; Status = 0xe6 },
        [pscustomobject]@{ Name = 'cliff-escape-quick'; Bit = 0x04; Status = 0x61 },
        [pscustomobject]@{ Name = 'cliff-escape-slow'; Bit = 0x08; Status = 0x63 }
    )
    $stateRows = @()
    foreach ($spec in $stateSpecs) {
        $matchesForState = @($submits | Where-Object {
            [Convert]::ToUInt32($_.Groups[3].Value, 16) -eq $spec.Bit
        })
        $p2 = @($matchesForState | Where-Object { [int]$_.Groups[4].Value -eq 2 }) | Select-Object -First 1
        $p3 = @($matchesForState | Where-Object { [int]$_.Groups[4].Value -eq 3 }) | Select-Object -First 1
        Assert-SamusTour ($null -ne $p2 -and $null -ne $p3) `
            "Samus $($spec.Name) did not submit both morph programs." $stdout
        foreach ($submit in @($p2, $p3)) {
            Assert-SamusTour ([Convert]::ToUInt32($submit.Groups[2].Value, 16) -eq $spec.Status) `
                "Samus $($spec.Name) morph submission carried the wrong source status." $stdout
            Assert-SamusTour ([uint32]$submit.Groups[5].Value -gt 0) `
                "Samus $($spec.Name) morph owner did not add P0 hardware triangles on the next frame." $stdout
            Assert-SamusTour ([Convert]::ToUInt32($submit.Groups[6].Value, 16) -eq 0) `
                "Samus $($spec.Name) morph submission observed a texture reject." $stdout
        }
        $stateRows += [pscustomobject]@{
            Name = $spec.Name
            Status = $spec.Status
            Program2Triangles = [uint32]$p2.Groups[5].Value
            Program3Triangles = [uint32]$p3.Groups[5].Value
        }
    }

    Write-Output ('P2-3 Samus natural morph+ledge tour passed: scenarios=10/10 stages=12 ' +
        ('mask=0x{0:x} morph=0x3f NAT_MOVESET=0x{1:x} roll={2}/{3} stalls=0.' -f
            $mask, $naturalMask, [int]$v[14], [int]$v[15]))
    foreach ($row in $stateRows) {
        Write-Output ("SAMUS_MORPH_STATE {0} status=0x{1:x} p2Triangles={2} p3Triangles={3} restored=1" -f
            $row.Name, $row.Status, $row.Program2Triangles, $row.Program3Triangles)
    }
    Write-Output ($match.Value)
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
