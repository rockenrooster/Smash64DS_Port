[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 7,
    [string]$Build = 'build-bugs-falcon-effects',
    [switch]$NoBuild,
    [ValidateRange(30,600)][int]$TimeoutSeconds = 240,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-FalconEffect {
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
        'NDS_P2_PROOF_FIGHTER0=7' 'NDS_TASK68_FALLBACK_CENSUS=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig, $nm)) {
    Assert-FalconEffect (Test-Path -LiteralPath $path -PathType Leaf) `
        "Falcon effect proof input is missing: $path"
}
$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_P2_CAPTAIN 1',
    '#define NDS_P2_PROOF_FIGHTER0 7',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-FalconEffect $configText.Contains($definition) `
        "Falcon effect build is missing required definition: $definition" $config
}
Assert-FalconEffect $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Falcon effect proof must run the mode-163 BattleShip battle harness.' $sceneConfig

$symbols = @(& $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] })
foreach ($symbol in @(
    'sControllerPlaybackPads', 'sControllerPlaybackConnectedMask',
    'sControllerPlaybackEnabled', 'ndsBattlePlayableFrameCompleteMarker',
    'ftCaptainSpecialNSetStatus', 'ftCaptainSpecialLwSetStatus',
    'efManagerCaptainFalconPunchMakeEffect', 'efManagerCaptainFalconKickMakeEffect',
    'ndsRendererAdapterSubmitStageDL',
    'gcDrawDObjDLHead1', 'gcDrawDObjTreeForGObj', 'gSCManagerBattleState',
    'gNdsEffectRendererTriangleCount', 'gNdsEffectRendererTextureRejectCount',
    'gNdsEffectRendererRejectedDrawCount', 'gNdsEffectRendererSubmitCount',
    'gNdsEffectDLPublishCount', 'gNdsEffectDLBlocker',
    'gNdsEffectDLCommandCount', 'gNdsEffectDLFirstOpcode',
    'gNdsEffectDLUnsupportedOpcode', 'gNdsEffectDLHwTriangleCount',
    'gNdsEffectDLHwVertexCount', 'gNdsEffectDLMatrixSeed',
    'gNdsEffectDLMatrixCmd', 'gNdsEffectDLXformVertexCount',
    'gNdsEffectDLCfgMask', 'gNdsEffectDLCfgMvT',
    'gmCollisionGetFighterPartsWorldPosition', 'syMatrixTra',
    'dEFManagerCaptainFalconPunchEffectDesc', 'dEFManagerCaptainFalconKickEffectDesc'
)) {
    Assert-FalconEffect ($symbols -contains $symbol) "Falcon effect ELF symbol missing: $symbol"
}

function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = @(& $nm -a $elf) | Where-Object {
        $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$"
    } | Select-Object -First 1
    Assert-FalconEffect ($null -ne $line) "ELF symbol not found: $Name"
    return [uint32]([Convert]::ToUInt32(([regex]::Match($line, '^([0-9a-fA-F]+)')).Groups[1].Value, 16))
}

$pads = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$connected = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$enabled = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
$frameComplete = Get-ElfSymbolAddress 'ndsBattlePlayableFrameCompleteMarker'
foreach ($address in @($pads, $connected, $enabled)) {
    Assert-FalconEffect (($address -band 0xFFFF0000) -eq 0x02FF0000) `
        ('Falcon proof playback state must be in uncached ARM9 DTCM; ' +
         ('GDB writes to cached main RAM are invalid (address 0x{0:x8}).' -f $address))
}

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\verification\2026-08-31_bug-falcon-effects.txt'
} elseif (-not [System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

# BattleShip owns both effect triggers. Neutral B reaches Falcon Punch; B with
# stick Y=-80 reaches grounded Falcon Kick. The proof writes only controller
# playback. It then observes the source makers, their source attachment joints
# (Punch joint 16, Kick joint 23), and the actual source display callbacks.
$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path $ctx.MelonDSPath) -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        ('set {{unsigned int}}0x{0:x8} = 0' -f $pads),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $connected),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $enabled),
        ('tbreak *0x{0:x8} if (gSCManagerBattleState != 0) && (gSCManagerBattleState->players[0].fighter_gobj != 0) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->fkind == 7) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id == nFTCommonStatusWait) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_control_disable == 0)' -f $frameComplete),
        'continue',
        'set $falcon = gSCManagerBattleState->players[0].fighter_gobj',
        'set $fst = (FTStruct *)$falcon->user_data.p',
        'printf "FALCON_EFFECT_READY status=%d fkind=%d lr=%d\n", $fst->status_id, $fst->fkind, $fst->lr',

        # Neutral B -> source Falcon Punch.
        ('set {{unsigned short}}0x{0:x8} = 0x4000' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 3)),
        'tbreak ftCaptainSpecialNSetStatus',
        'continue',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $pads),
        'tbreak efManagerCaptainFalconPunchMakeEffect',
        'continue',
        'printf "FALCON_PUNCH_TRIGGER status=%d motion=%d fighter=%p\n", ((FTStruct *)((GObj *)$r0)->user_data.p)->status_id, ((FTStruct *)((GObj *)$r0)->user_data.p)->motion_id, (GObj *)$r0',
        'finish',
        'set $punch = (GObj *)$r0',
        'set $punchroot = ($punch != 0) ? (DObj *)$punch->obj : (DObj *)0',
        'printf "FALCON_PUNCH_MADE effect=%p root=%p attach=%p expect=%p proc=%p expectproc=%p dl=%p\n", $punch, $punchroot, ($punchroot != 0) ? $punchroot->user_data.p : 0, $fst->joints[16], dEFManagerCaptainFalconPunchEffectDesc.proc_display, gcDrawDObjDLHead1, ($punchroot != 0) ? $punchroot->dl : 0',
        'tbreak gcDrawDObjDLHead1 if $r0 == $punch',
        'continue',
        'printf "FALCON_PUNCH_DRAW effect=%p status=%d\n", (GObj *)$r0, $fst->status_id',
        'tbreak gmCollisionGetFighterPartsWorldPosition if $r0 == $fst->joints[16]',
        'continue',
        'finish',
        'tbreak syMatrixTra',
        'continue',
        'printf "FALCON_PUNCH_POS_BITS x=%#x y=%#x z=%#x\n", $r1, $r2, $r3',
        'finish',
        'tbreak ndsRendererAdapterSubmitStageDL if ((DObj *)$r0)->parent_gobj == $punch',
        'continue',
        'set $punchdl = (Gfx *)$r1',
        'printf "FALCON_PUNCH_SUBMIT effect=%p dobj=%p dl=%p attach=%p expect=%p\n", $punch, (DObj *)$r0, $punchdl, ((DObj *)$r0)->user_data.p, $fst->joints[16]',
        'finish',
        'printf "FALCON_PUNCH_DL publish=%u blocker=%u commands=%u first=%#x unsupported=%#x hwtri=%u hwvtx=%u seed=%u mcmd=%u xform=%u cfg=%u mvt=%d,%d,%d\n", gNdsEffectDLPublishCount, gNdsEffectDLBlocker, gNdsEffectDLCommandCount, gNdsEffectDLFirstOpcode, gNdsEffectDLUnsupportedOpcode, gNdsEffectDLHwTriangleCount, gNdsEffectDLHwVertexCount, gNdsEffectDLMatrixSeed, gNdsEffectDLMatrixCmd, gNdsEffectDLXformVertexCount, gNdsEffectDLCfgMask, gNdsEffectDLCfgMvT[0], gNdsEffectDLCfgMvT[1], gNdsEffectDLCfgMvT[2]',

        # Wait for the source move to finish before asking for the next special.
        ('tbreak *0x{0:x8} if ($fst->status_id == nFTCommonStatusWait)' -f $frameComplete),
        'continue',

        # Down+B -> source grounded Falcon Kick.
        ('set {{unsigned short}}0x{0:x8} = 0x4000' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 2)),
        ('set {{signed char}}0x{0:x8} = -80' -f ($pads + 3)),
        'tbreak ftCaptainSpecialLwSetStatus',
        'continue',
        ('set {{unsigned short}}0x{0:x8} = 0' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 3)),
        'tbreak efManagerCaptainFalconKickMakeEffect',
        'continue',
        'printf "FALCON_KICK_TRIGGER status=%d motion=%d fighter=%p\n", ((FTStruct *)((GObj *)$r0)->user_data.p)->status_id, ((FTStruct *)((GObj *)$r0)->user_data.p)->motion_id, (GObj *)$r0',
        'finish',
        'set $kick = (GObj *)$r0',
        'set $kickroot = ($kick != 0) ? (DObj *)$kick->obj : (DObj *)0',
        'printf "FALCON_KICK_MADE effect=%p root=%p attach=%p expect=%p x0=%#x proc=%p expectproc=%p ry=%f rz=%f\n", $kick, $kickroot, ($kickroot != 0) ? $kickroot->user_data.p : 0, $fst->joints[23], (($kickroot != 0) && ($kickroot->xobjs_num > 0)) ? $kickroot->xobjs[0]->kind : 0, dEFManagerCaptainFalconKickEffectDesc.proc_display, gcDrawDObjTreeForGObj, ($kickroot != 0) ? $kickroot->rotate.vec.f.y : 0.0, ($kickroot != 0) ? $kickroot->rotate.vec.f.z : 0.0',
        'tbreak gcDrawDObjTreeForGObj if $r0 == $kick',
        'continue',
        'printf "FALCON_KICK_DRAW effect=%p status=%d attach=%p expect=%p\n", (GObj *)$r0, $fst->status_id, ((DObj *)((GObj *)$r0)->obj)->user_data.p, $fst->joints[23]',
        'tbreak gmCollisionGetFighterPartsWorldPosition if $r0 == $fst->joints[23]',
        'continue',
        'finish',
        'tbreak syMatrixTra',
        'continue',
        'printf "FALCON_KICK_POS_BITS x=%#x y=%#x z=%#x\n", $r1, $r2, $r3',
        'finish',
        'tbreak ndsRendererAdapterSubmitStageDL if ((DObj *)$r0)->parent_gobj == $kick',
        'continue',
        'set $kickdl = (Gfx *)$r1',
        'printf "FALCON_KICK_SUBMIT effect=%p dobj=%p dl=%p attach=%p expect=%p\n", $kick, (DObj *)$r0, $kickdl, ((DObj *)$r0)->user_data.p, $fst->joints[23]',
        'finish',
        'printf "FALCON_KICK_DL publish=%u blocker=%u commands=%u first=%#x unsupported=%#x hwtri=%u hwvtx=%u seed=%u mcmd=%u xform=%u cfg=%u mvt=%d,%d,%d\n", gNdsEffectDLPublishCount, gNdsEffectDLBlocker, gNdsEffectDLCommandCount, gNdsEffectDLFirstOpcode, gNdsEffectDLUnsupportedOpcode, gNdsEffectDLHwTriangleCount, gNdsEffectDLHwVertexCount, gNdsEffectDLMatrixSeed, gNdsEffectDLMatrixCmd, gNdsEffectDLXformVertexCount, gNdsEffectDLCfgMask, gNdsEffectDLCfgMvT[0], gNdsEffectDLCfgMvT[1], gNdsEffectDLCfgMvT[2]',
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-falcon-effects.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    $stdout = $capture.Stdout
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $stdout

    $ready = [regex]::Match($stdout, 'FALCON_EFFECT_READY status=(-?\d+) fkind=(\d+) lr=(-?\d+)')
    $ptr = '(?:0x[0-9a-fA-F]+|\(nil\))'
    $punchMade = [regex]::Match($stdout, "FALCON_PUNCH_MADE effect=(0x[0-9a-fA-F]+) root=($ptr) attach=($ptr) expect=(0x[0-9a-fA-F]+) proc=(0x[0-9a-fA-F]+) expectproc=(0x[0-9a-fA-F]+) dl=($ptr)")
    $punchDraw = [regex]::Match($stdout, 'FALCON_PUNCH_DRAW effect=(0x[0-9a-fA-F]+) status=(-?\d+)')
    $punchPos = [regex]::Match($stdout, 'FALCON_PUNCH_POS_BITS x=(0x[0-9a-fA-F]+) y=(0x[0-9a-fA-F]+) z=(0x[0-9a-fA-F]+)')
    $punchSubmit = [regex]::Match($stdout, 'FALCON_PUNCH_SUBMIT effect=(0x[0-9a-fA-F]+) dobj=(0x[0-9a-fA-F]+) dl=(0x[0-9a-fA-F]+) attach=(0x[0-9a-fA-F]+) expect=(0x[0-9a-fA-F]+)')
    $punchDL = [regex]::Match($stdout, 'FALCON_PUNCH_DL publish=(\d+) blocker=(\d+) commands=(\d+) first=(0x[0-9a-fA-F]+) unsupported=(0x[0-9a-fA-F]+|0) hwtri=(\d+) hwvtx=(\d+) seed=(\d+) mcmd=(\d+) xform=(\d+) cfg=(\d+) mvt=(-?\d+),(-?\d+),(-?\d+)')
    $kickMade = [regex]::Match($stdout, "FALCON_KICK_MADE effect=(0x[0-9a-fA-F]+) root=(0x[0-9a-fA-F]+) attach=($ptr) expect=(0x[0-9a-fA-F]+) x0=(0x[0-9a-fA-F]+) proc=(0x[0-9a-fA-F]+) expectproc=(0x[0-9a-fA-F]+) ry=([-+0-9.eE]+) rz=([-+0-9.eE]+)")
    $kickDraw = [regex]::Match($stdout, 'FALCON_KICK_DRAW effect=(0x[0-9a-fA-F]+) status=(-?\d+) attach=(0x[0-9a-fA-F]+) expect=(0x[0-9a-fA-F]+)')
    $kickPos = [regex]::Match($stdout, 'FALCON_KICK_POS_BITS x=(0x[0-9a-fA-F]+) y=(0x[0-9a-fA-F]+) z=(0x[0-9a-fA-F]+)')
    $kickSubmit = [regex]::Match($stdout, "FALCON_KICK_SUBMIT effect=(0x[0-9a-fA-F]+) dobj=(0x[0-9a-fA-F]+) dl=(0x[0-9a-fA-F]+) attach=($ptr) expect=(0x[0-9a-fA-F]+)")
    $kickDL = [regex]::Match($stdout, 'FALCON_KICK_DL publish=(\d+) blocker=(\d+) commands=(\d+) first=(0x[0-9a-fA-F]+) unsupported=(0x[0-9a-fA-F]+|0) hwtri=(\d+) hwvtx=(\d+) seed=(\d+) mcmd=(\d+) xform=(\d+) cfg=(\d+) mvt=(-?\d+),(-?\d+),(-?\d+)')

    Assert-FalconEffect ($ready.Success -and [int]$ready.Groups[2].Value -eq 7) `
        'Falcon effect proof never reached controller-owned source Wait.' $stdout
    Assert-FalconEffect ($punchMade.Success -and $punchDraw.Success -and $punchPos.Success -and $punchSubmit.Success -and $punchDL.Success) `
        'Natural Falcon Punch did not create and draw its source effect.' $stdout
    Assert-FalconEffect ($punchSubmit.Groups[4].Value -eq $punchSubmit.Groups[5].Value) `
        'Falcon Punch renderer submit was not attached to BattleShip joint 16.' $stdout
    $punchProc = [Convert]::ToUInt32($punchMade.Groups[5].Value.Substring(2), 16) -band 0xFFFFFFFE
    $punchExpectProc = [Convert]::ToUInt32($punchMade.Groups[6].Value.Substring(2), 16) -band 0xFFFFFFFE
    Assert-FalconEffect ($punchProc -eq $punchExpectProc) `
        'Falcon Punch descriptor is not routed through the DS DLHead1 source-display seam.' $stdout
    Assert-FalconEffect ([int]$punchDL.Groups[2].Value -eq 0 -and
        [Convert]::ToUInt32(($punchDL.Groups[5].Value -replace '^0x',''), 16) -eq 0 -and
        [int]$punchDL.Groups[6].Value -gt 0 -and
        [int]$punchDL.Groups[7].Value -gt 0 -and
        (([int]$punchDL.Groups[11].Value -band 2) -ne 0) -and
        [int]$punchDL.Groups[8].Value -gt 0 -and
        (([math]::Abs([int64]$punchDL.Groups[12].Value) +
          [math]::Abs([int64]$punchDL.Groups[13].Value) +
          [math]::Abs([int64]$punchDL.Groups[14].Value)) -gt 4096)) `
        'Falcon Punch source display callback reached the DS renderer but emitted no accepted triangles.' $stdout
    Assert-FalconEffect ($kickMade.Success -and $kickDraw.Success -and $kickPos.Success -and $kickSubmit.Success -and $kickDL.Success) `
        'Natural Falcon Kick did not create and draw its source effect.' $stdout
    Assert-FalconEffect ($kickDraw.Groups[3].Value -eq $kickDraw.Groups[4].Value) `
        'Falcon Kick source effect did not remain attached to BattleShip joint 23.' $stdout
    Assert-FalconEffect ([Convert]::ToUInt32($kickMade.Groups[5].Value.Substring(2), 16) -eq 0x50) `
        'Falcon Kick root did not carry source matrix kind 0x50 (joint-position attachment).' $stdout
    # Kick is a source DObj TREE.  The root owns matrix kind 0x50 and joint23;
    # drawable children inherit that parent transform and therefore correctly
    # carry no user_data attachment of their own.  Assert the live root at the
    # source display callback above, not a duplicated attachment on each child.
    $kickProc = [Convert]::ToUInt32($kickMade.Groups[6].Value.Substring(2), 16) -band 0xFFFFFFFE
    $kickExpectProc = [Convert]::ToUInt32($kickMade.Groups[7].Value.Substring(2), 16) -band 0xFFFFFFFE
    Assert-FalconEffect ($kickProc -eq $kickExpectProc) `
        'Falcon Kick descriptor did not use BattleShip tree display.' $stdout
    Assert-FalconEffect ([int]$kickDL.Groups[2].Value -eq 0 -and
        [Convert]::ToUInt32(($kickDL.Groups[5].Value -replace '^0x',''), 16) -eq 0 -and
        [int]$kickDL.Groups[6].Value -gt 0 -and
        [int]$kickDL.Groups[7].Value -gt 0 -and
        (([int]$kickDL.Groups[11].Value -band 2) -ne 0) -and
        [int]$kickDL.Groups[8].Value -gt 0 -and
        (([math]::Abs([int64]$kickDL.Groups[12].Value) +
          [math]::Abs([int64]$kickDL.Groups[13].Value) +
          [math]::Abs([int64]$kickDL.Groups[14].Value)) -gt 4096)) `
        'Falcon Kick source effect reached its display callback but emitted no accepted triangles.' $stdout

    foreach ($entry in @(@('Punch', $punchPos), @('Kick', $kickPos))) {
        $label = $entry[0]
        $match = $entry[1]
        $joint = @(1,2,3 | ForEach-Object {
            $bits = [Convert]::ToUInt32($match.Groups[$_].Value.Substring(2), 16)
            [BitConverter]::ToSingle([BitConverter]::GetBytes($bits), 0)
        })
        $magnitude = 0.0
        for ($axis = 0; $axis -lt 3; $axis++) {
            Assert-FalconEffect (-not [double]::IsNaN($joint[$axis]) -and
                -not [double]::IsInfinity($joint[$axis])) `
                "Falcon $label source joint position is not finite." $stdout
            $magnitude += [math]::Abs($joint[$axis])
        }
        Assert-FalconEffect (($magnitude -gt 64.0) -and ($magnitude -lt 100000.0)) `
            "Falcon $label matrix kind 0x50 collapsed to/near world origin or an invalid world position." $stdout
    }

    Write-Output (('P2 Falcon effects proof passed: natural Punch rendered {0} triangles; ' +
        'natural Kick rendered {1} triangles on source joint23 with matrix kind 0x50.') -f
        $punchDL.Groups[6].Value, $kickDL.Groups[6].Value)
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
