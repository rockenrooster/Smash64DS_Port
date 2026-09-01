[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 7,
    [string]$Build = 'build-bugs-falcon-jumpb',
    [switch]$NoBuild,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 180,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-FalconModelPart {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if ($Condition) { return }
    if ($Evidence) { throw "$Message`n$Evidence" }
    throw $Message
}

$target = 'smash64ds-battle-playable-tickhud-hwtri'
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir "$target.nds"
$elf = Join-Path $buildDir "$target.elf"
$config = Join-Path $buildDir 'nds_build_config.h'
$sceneConfig = Join-Path $buildDir 'nds_scene_harness_config.h'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_PROOF_FIGHTER0=7' `
        'NDS_TASK68_FALLBACK_CENSUS=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig, $nm)) {
    Assert-FalconModelPart (Test-Path -LiteralPath $path -PathType Leaf) `
        "Falcon model-part proof input is missing: $path"
}
$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_TICK_HUD 1',
    '#define NDS_P2_LUIGI 1',
    '#define NDS_P2_DONKEY 1',
    '#define NDS_P2_CAPTAIN 1',
    '#define NDS_P2_PROOF_FIGHTER0 7',
    '#define NDS_TASK68_FALLBACK_CENSUS 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-FalconModelPart $configText.Contains($definition) `
        "Falcon model-part build is missing required definition: $definition" $config
}
Assert-FalconModelPart $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Falcon model-part proof must run the mode-163 BattleShip battle harness.' $sceneConfig

$elfSymbols = @(& $nm -a $elf)
Assert-FalconModelPart ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = $elfSymbols | Where-Object {
        $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$"
    } | Select-Object -First 1
    Assert-FalconModelPart ($null -ne $line) "ELF symbol not found: $Name"
    $m = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($m.Groups[1].Value, 16))
}

$pads = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$connected = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$enabled = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
$jumpSetStatus = Get-ElfSymbolAddress 'ftCommonJumpSetStatus'
$modelPartSet = Get-ElfSymbolAddress 'ftParamSetModelPartID'
$frameComplete = Get-ElfSymbolAddress 'ndsBattlePlayableFrameCompleteMarker'
foreach ($symbol in @(
    'gNdsTickHudNativeOwnerFallbackCount',
    'gNdsTickHudNativeOwnerFallbackByReason',
    'gNdsNativeFighterValidateRejectCode',
    'gNdsNativeFighterValidateRejectSlot',
    'gNdsNativeFighterValidateRejectLow',
    'gNdsNativeFighterValidateRejectRoot',
    'gNdsNativeFighterValidateRejectObserved',
    'gNdsNativeFighterValidateRejectExpected',
    'gNdsFtrPreValidateReject',
    'gNdsFtrPlanBuild',
    'gNdsFtrPlanHit',
    'gSCManagerBattleState'
)) { [void](Get-ElfSymbolAddress $symbol) }

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\verification\2026-08-30_bug-falcon-jumpb-modelpart.txt'
} elseif (-not [System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

# Source contract, BattleShip relocData/235_CaptainMainMotion.c:
# JumpB emits SetModelPartID(10, 2), SetModelPartID(16, 2), waits 30 ticks, then
# restores both to id 0. ftcommonjump.c selects JumpB when stick_x * lr is not
# above the forward threshold. This proof supplies only the controller pad;
# BattleShip owns KneeBend, JumpB selection, motion-event decoding and the
# model-part mutation. GDB observes state and the renderer census only.
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

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        # Install neutral playback immediately after attaching, then let the
        # stable frame-complete marker prove that BattleShip has reached the
        # requested live fighter in source Wait. Source-line stops in ifcommon
        # are not a stable lifecycle boundary in the current optimized build.
        ('set {{unsigned int}}0x{0:x8} = 0' -f $pads),
        ('set {{unsigned int}}0x{0:x8} = 0' -f ($pads + 4)),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $connected),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $enabled),
        # Stop on the first completed neutral frame where P0 is the requested
        # Captain and source Wait is active. The direct proof descriptor makes
        # P0 the controller-owned fighter; no CPU command path is involved.
        ('tbreak *0x{0:x8} if (gSCManagerBattleState != 0) && (gSCManagerBattleState->players[0].fighter_gobj != 0) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->fkind == 7) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id == nFTCommonStatusWait) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_control_disable == 0)' -f $frameComplete),
        'continue',
        'printf "FALCON_READY status=%d fkind=%d lr=%d player=%d\n", ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->fkind, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->lr, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->player',
        # DS/N64 playback pad layout is u16 button, s8 stick_x, s8 stick_y.
        # U_CBUTTONS (0x0008) is the common jump button in the port. Hold it
        # with horizontal stick opposite the live facing through KneeBend.
        ('set {{unsigned short}}0x{0:x8} = 0x0008' -f $pads),
        ('set {{signed char}}0x{0:x8} = -80 * ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->lr' -f ($pads + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 3)),
        ('tbreak *0x{0:x8} if ((FTStruct *)((GObj *)$r0)->user_data.p)->fkind == 7' -f $jumpSetStatus),
        'continue',
        'printf "FALCON_JUMP_SELECT prestatus=%d lr=%d stickx=%d product=%d\n", ((FTStruct *)((GObj *)$r0)->user_data.p)->status_id, ((FTStruct *)((GObj *)$r0)->user_data.p)->lr, ((FTStruct *)((GObj *)$r0)->user_data.p)->input.pl.stick_range.x, ((FTStruct *)((GObj *)$r0)->user_data.p)->input.pl.stick_range.x * ((FTStruct *)((GObj *)$r0)->user_data.p)->lr',
        # The current FTInput is already latched; release external playback now
        # so the proof cannot accidentally request a second jump later.
        ('set {{unsigned int}}0x{0:x8} = 0' -f $pads),
        # JumpB's source script emits SetModelPartID(10,2) immediately while
        # ftCommonJumpSetStatus is still running, before its 30-frame wait.
        # Arm the nested writer now; waiting for JumpSetStatus to return would
        # miss the mutation by construction. ABI r0/r1/r2 are the writer's
        # fighter GObj, joint id and model-part id at function entry.
        ('tbreak *0x{0:x8} if (((FTStruct *)((GObj *)$r0)->user_data.p)->fkind == 7) && ((int)$r1 == 10) && ((int)$r2 == 2)' -f $modelPartSet),
        'continue',
        'printf "FALCON_NATIVE_BASE calls=%u eligible=%u fallback=%u displaylist=%u validate=%u\n", gNdsTickHudNativeOwnerFallbackByReason[0], gNdsTickHudNativeOwnerFallbackByReason[1], gNdsTickHudNativeOwnerFallbackCount, gNdsTickHudNativeOwnerFallbackByReason[4], gNdsTickHudNativeOwnerFallbackByReason[6]',
        'printf "FALCON_REASON_BASE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTickHudNativeOwnerFallbackByReason[2],gNdsTickHudNativeOwnerFallbackByReason[3],gNdsTickHudNativeOwnerFallbackByReason[4],gNdsTickHudNativeOwnerFallbackByReason[5],gNdsTickHudNativeOwnerFallbackByReason[6],gNdsTickHudNativeOwnerFallbackByReason[7],gNdsTickHudNativeOwnerFallbackByReason[8],gNdsTickHudNativeOwnerFallbackByReason[9],gNdsTickHudNativeOwnerFallbackByReason[10],gNdsTickHudNativeOwnerFallbackByReason[11],gNdsTickHudNativeOwnerFallbackByReason[12]',
        'printf "FALCON_PRE_BASE reject=%u planbuild=%u planhit=%u\n", gNdsFtrPreValidateReject, gNdsFtrPlanBuild, gNdsFtrPlanHit',
        'printf "FALCON_MODELPART_EVENT status=%d joint=%d id=%d detail=%d expectdl=%p expectflags=%#x\n", ((FTStruct *)((GObj *)$r0)->user_data.p)->status_id, (int)$r1, (int)$r2, ((FTStruct *)((GObj *)$r0)->user_data.p)->detail_curr, ((FTStruct *)((GObj *)$r0)->user_data.p)->attr->modelparts_container->modelparts_desc[10 - nFTPartsJointCommonStart]->modelparts[2][((FTStruct *)((GObj *)$r0)->user_data.p)->detail_curr - nFTPartsDetailStart].dl, ((FTStruct *)((GObj *)$r0)->user_data.p)->attr->modelparts_container->modelparts_desc[10 - nFTPartsJointCommonStart]->modelparts[2][((FTStruct *)((GObj *)$r0)->user_data.p)->detail_curr - nFTPartsDetailStart].flags',
        'finish',
        'printf "FALCON_MODELPART_APPLIED dl=%p expect=%p flags=%#x expectflags=%#x current=%d modified=%d\n", ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->joints[10]->dl, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->attr->modelparts_container->modelparts_desc[10 - nFTPartsJointCommonStart]->modelparts[2][((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->detail_curr - nFTPartsDetailStart].dl, ((FTParts *)((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->joints[10]->user_data.p)->flags, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->attr->modelparts_container->modelparts_desc[10 - nFTPartsJointCommonStart]->modelparts[2][((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->detail_curr - nFTPartsDetailStart].flags, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->modelpart_status[10 - nFTPartsJointCommonStart].modelpart_id_curr, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_modelpart_modify',
        'printf "FALCON_JUMP_STATUS status=%d motion=%d\n", ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id, ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->motion_id',
        'if ((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id != nFTCommonStatusJumpB',
        'printf "FALCON_FAIL code=1\n"',
        'detach',
        'quit 1',
        'end',
        # Let the ordinary source frame present the mutated DObj once.
        ('tbreak *0x{0:x8}' -f $frameComplete),
        'continue',
        'printf "FALCON_NATIVE_AFTER calls=%u eligible=%u fallback=%u displaylist=%u validate=%u\n", gNdsTickHudNativeOwnerFallbackByReason[0], gNdsTickHudNativeOwnerFallbackByReason[1], gNdsTickHudNativeOwnerFallbackCount, gNdsTickHudNativeOwnerFallbackByReason[4], gNdsTickHudNativeOwnerFallbackByReason[6]',
        'printf "FALCON_REASON_AFTER=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTickHudNativeOwnerFallbackByReason[2],gNdsTickHudNativeOwnerFallbackByReason[3],gNdsTickHudNativeOwnerFallbackByReason[4],gNdsTickHudNativeOwnerFallbackByReason[5],gNdsTickHudNativeOwnerFallbackByReason[6],gNdsTickHudNativeOwnerFallbackByReason[7],gNdsTickHudNativeOwnerFallbackByReason[8],gNdsTickHudNativeOwnerFallbackByReason[9],gNdsTickHudNativeOwnerFallbackByReason[10],gNdsTickHudNativeOwnerFallbackByReason[11],gNdsTickHudNativeOwnerFallbackByReason[12]',
        'printf "FALCON_PRE_AFTER reject=%u planbuild=%u planhit=%u\n", gNdsFtrPreValidateReject, gNdsFtrPlanBuild, gNdsFtrPlanHit',
        'printf "FALCON_REJECT code=%u slot=%u low=%u root=%u observed=%#x expected=%#x\n", gNdsNativeFighterValidateRejectCode, gNdsNativeFighterValidateRejectSlot, gNdsNativeFighterValidateRejectLow, gNdsNativeFighterValidateRejectRoot, gNdsNativeFighterValidateRejectObserved, gNdsNativeFighterValidateRejectExpected',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-falcon-modelpart.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-falcon-modelpart.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $stdout

    $ready = [regex]::Match($stdout, 'FALCON_READY status=(-?\d+) fkind=(\d+) lr=(-?\d+) player=(\d+)')
    $select = [regex]::Match($stdout, 'FALCON_JUMP_SELECT prestatus=(-?\d+) lr=(-?\d+) stickx=(-?\d+) product=(-?\d+)')
    $jump = [regex]::Match($stdout, 'FALCON_JUMP_STATUS status=(-?\d+) motion=(-?\d+)')
    $event = [regex]::Match($stdout, 'FALCON_MODELPART_EVENT status=(-?\d+) joint=(\d+) id=(-?\d+) detail=(-?\d+) expectdl=(0x[0-9a-fA-F]+) expectflags=(0x[0-9a-fA-F]+|0)')
    $applied = [regex]::Match($stdout, 'FALCON_MODELPART_APPLIED dl=(0x[0-9a-fA-F]+) expect=(0x[0-9a-fA-F]+) flags=(0x[0-9a-fA-F]+|0) expectflags=(0x[0-9a-fA-F]+|0) current=(-?\d+) modified=(-?\d+)')
    $nativeBase = [regex]::Match($stdout, 'FALCON_NATIVE_BASE calls=(\d+) eligible=(\d+) fallback=(\d+) displaylist=(\d+) validate=(\d+)')
    $nativeAfter = [regex]::Match($stdout, 'FALCON_NATIVE_AFTER calls=(\d+) eligible=(\d+) fallback=(\d+) displaylist=(\d+) validate=(\d+)')

    Assert-FalconModelPart ($ready.Success -and [int]$ready.Groups[2].Value -eq 7) `
        'Falcon proof never reached controller-owned source Wait.' $stdout
    Assert-FalconModelPart ($select.Success -and [int]$select.Groups[4].Value -lt 0) `
        'Falcon proof did not supply a backward horizontal jump input.' $stdout
    Assert-FalconModelPart ($jump.Success) 'BattleShip JumpB status marker is missing.' $stdout
    Assert-FalconModelPart ($event.Success -and [int]$event.Groups[2].Value -eq 10 -and [int]$event.Groups[3].Value -eq 2) `
        'Captain JumpB did not emit source SetModelPartID(10,2).' $stdout
    Assert-FalconModelPart ($applied.Success -and
        $applied.Groups[1].Value -eq $applied.Groups[2].Value -and
        $applied.Groups[3].Value -eq $applied.Groups[4].Value) `
        'Falcon live DObj/MObj model-part state diverged from BattleShip.' $stdout
    Assert-FalconModelPart ($nativeBase.Success -and $nativeAfter.Success) `
        'Falcon native-owner baseline/after markers are missing.' $stdout
    $nativeCalls = [uint32]$nativeAfter.Groups[1].Value - [uint32]$nativeBase.Groups[1].Value
    $nativeEligible = [uint32]$nativeAfter.Groups[2].Value - [uint32]$nativeBase.Groups[2].Value
    $nativeFallback = [uint32]$nativeAfter.Groups[3].Value - [uint32]$nativeBase.Groups[3].Value
    $nativeDisplayList = [uint32]$nativeAfter.Groups[4].Value - [uint32]$nativeBase.Groups[4].Value
    $nativeValidate = [uint32]$nativeAfter.Groups[5].Value - [uint32]$nativeBase.Groups[5].Value
    Assert-FalconModelPart ($nativeCalls -gt [uint32]0 -and
        $nativeEligible -gt [uint32]0 -and
        $nativeFallback -eq [uint32]0 -and
        $nativeDisplayList -eq [uint32]0 -and
        $nativeValidate -eq [uint32]0) `
        'Falcon JumpB model-part frame left the native owner path.' $stdout

    Write-Output ('P2 Falcon model-part native proof passed: JumpB source joint10/id2; ' +
        'native calls={0} eligible={1} fallback=0.' -f
        $nativeCalls, $nativeEligible)
    Write-Output $event.Value
    Write-Output $applied.Value
    Write-Output ('FALCON_NATIVE calls={0} eligible={1} fallback={2} displaylist={3} validate={4}' -f
        $nativeCalls, $nativeEligible, $nativeFallback, $nativeDisplayList, $nativeValidate)
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
