[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\\emulators\\melonds\\melonDS.exe'),
    [string]$Gdb = 'C:\\devkitPro\\devkitARM\\bin\\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = 7,
    [string]$Build = 'build-p2-link-bomb-tour-fast',
    [string]$Artifact = '',
    [switch]$NoBuild,
    [ValidateRange(30,1200)][int]$TimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\\gdb-markers.ps1')

function Assert-LinkBomb {
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

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_SAMUS=1' 'NDS_P2_LINK=1' 'NDS_P2_PROOF_FIGHTER0=5' `
        'NDS_P2_SAMUS_STATE_TOUR=0' 'NDS_P2_SAMUS_TUMBLE_TOUR=0' `
        'NDS_P2_SAMUS_DAMAGEFLY_TOUR=0' 'NDS_P2_SAMUS_ATTACK_TOUR=0' `
        'NDS_P2_LINK_BOMB_TOUR=1' `
        'NDS_HARNESS_FAST_PRESENT_ON_REQUEST=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-LinkBomb (Test-Path -LiteralPath $path -PathType Leaf) `
        "LinkBomb proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_HARNESS_FAST_PRESENT_ON_REQUEST 1',
    '#define NDS_P2_LINK 1',
    '#define NDS_P2_PROOF_FIGHTER0 5',
    '#define NDS_P2_LINK_BOMB_TOUR 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-LinkBomb $configText.Contains($definition) `
        "LinkBomb proof build is missing required definition: $definition" $config
}
Assert-LinkBomb $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'LinkBomb proof must run the mode-163 battle_playable harness.' $sceneConfig

# The proof is allowed to feed controller state only. Link's own special
# interrupt must choose SpecialLw and the source item/throw machinery owns every
# lifecycle transition after that.
$movementPath = Join-Path $root 'src\\port\\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$tourStart = $movementText.IndexOf('void ndsLinkBombTourProofStop')
$tourEnd = $movementText.IndexOf('static sb32 ndsFighterNaturalCombatRecoverTeeter', $tourStart)
Assert-LinkBomb (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the LinkBomb guest proof driver for injection guards.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
foreach ($forbidden in @(
    'ftMainSetStatus\s*\(',
    'ftLinkSpecialLwSetStatus\s*\(',
    'itLinkBombMakeItem\s*\(',
    'link->status_id\s*=(?!=)',
    'link->motion_id\s*=(?!=)'
)) {
    Assert-LinkBomb ($tourText -notmatch $forbidden) `
        "LinkBomb guest proof contains forbidden state injection: $forbidden"
}

$contextParams = @{
    Root = $root
    MelonDS = $MelonDS
    RunnerSlot = $RunnerSlot
    GdbPort = $GdbPort
    NoBuild = $true
}
if ($PSBoundParameters.ContainsKey('GdbPort')) {
    $contextParams.GdbPortExplicit = $true
}
$ctx = Initialize-MelonDSVerifierContext @contextParams
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
        'tbreak ndsLinkBombTourProofStop',
        'continue',
        'printf "LINK_BOMB_TOUR=%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%#x,%#x,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u\n",gNdsLinkBombTourPhase,gNdsLinkBombTourFrames,gNdsLinkBombTourInputCount,gNdsLinkBombTourStatusMask,gNdsLinkBombTourAttrValidCount,gNdsLinkBombTourHoldObserved,gNdsLinkBombTourThrowObserved,gNdsLinkBombTourColAnimObserved,gNdsLinkBombTourExplodeObserved,gNdsLinkBombTourDestroyObserved,gNdsLinkBombTourHoldKind,gNdsLinkBombTourHoldLifetime,gNdsLinkBombTourThrowLifetime,gNdsLinkBombTourThrowVelXMilli,gNdsLinkBombTourThrowVelYMilli,gNdsLinkBombTourColAnimRGBA,gNdsLinkBombTourRenderEnvRGBA,gNdsLinkBombTourExplodeDamage,gNdsLinkBombTourExplodeSize,gNdsLinkBombTourExplodeAngle,gNdsLinkBombTourExplodeElement,gNdsLinkBombTourExplodeEventID,gNdsLinkBombTourExplodeMulti,gNdsLinkBombTourDestroyMulti,gNdsItemRendererCaptureCount,gNdsItemRendererDObjDrawCount,gNdsItemRendererSubmitCount,gNdsItemRendererVisibleDrawCount,gNdsItemRendererTriangleCount,gNdsItemRendererTextureReadyCount,gNdsItemRendererTextureRejectCount,gNdsItemRendererKindMask,gNdsItemRendererRejectedDrawCount,gNdsItemRendererAttach52BuildCount,gNdsFighterNaturalCombatStallCount,gNdsFtPoseTrackOverflow,gNdsLinkBombTourFixtureCount',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-link-bomb.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-link-bomb.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    $match = [regex]::Match($stdout, 'LINK_BOMB_TOUR=([^\r\n]+)')
    Assert-LinkBomb $match.Success 'LinkBomb terminal marker is missing.' $stdout
    $v = @($match.Groups[1].Value.Split(','))
    Assert-LinkBomb ($v.Count -eq 37) `
        "LinkBomb terminal marker has $($v.Count) fields; expected 37." $match.Value
    $u = { param([int]$i) [Convert]::ToUInt32($v[$i].Replace('0x',''), 16) }
    $n = { param([int]$i) [int64]$v[$i] }

    $statusMask = & $u 3
    $colRGBA = & $u 15
    $renderEnv = & $u 16
    $kindMask = & $u 31
    Assert-LinkBomb ((& $n 0) -eq 7) 'LinkBomb proof did not reach destruction.' $match.Value
    Assert-LinkBomb ((& $n 2) -eq 2) 'LinkBomb proof must issue exactly two Down+B taps.' $match.Value
    Assert-LinkBomb (($statusMask -band 0x3) -eq 0x3) `
        'BattleShip did not select both Link SpecialLw and common LightThrowF4.' $match.Value
    Assert-LinkBomb ((& $n 4) -gt 0) 'LinkBomb packed attributes were not source-validated.' $match.Value
    foreach ($i in 5..9) {
        Assert-LinkBomb ((& $n $i) -gt 0) 'LinkBomb lifecycle observation is incomplete.' $match.Value
    }
    Assert-LinkBomb ((& $n 10) -eq 21) 'Held object was not nITKindLinkBomb.' $match.Value
    Assert-LinkBomb ((& $n 11) -gt 0 -and (& $n 11) -le 300) 'Held LinkBomb lifetime is invalid.' $match.Value
    Assert-LinkBomb ((& $n 12) -gt 0 -and (& $n 12) -le 96) `
        'Source throw did not occur from the naturally reached critical-fuse window.' $match.Value
    Assert-LinkBomb (((& $n 13) -ne 0) -or ((& $n 14) -ne 0)) 'Thrown LinkBomb has no velocity.' $match.Value
    Assert-LinkBomb (($colRGBA -band 0xff) -eq 140 -and ($renderEnv -band 0xff) -eq 140) `
        'Critical-fuse alpha did not reach both source ColAnim and DS EnvColor.' $match.Value
    Assert-LinkBomb ((& $n 17) -eq 5 -and (& $n 18) -eq 300 -and `
        (& $n 19) -eq 361 -and (& $n 20) -eq 1 -and (& $n 21) -eq 1 -and `
        (& $n 22) -eq 0 -and (& $n 23) -eq 6) `
        'LinkBomb explosion/destruction did not consume the source event script.' $match.Value
    Assert-LinkBomb ((& $n 24) -gt 0 -and (& $n 25) -gt 0 -and `
        (& $n 26) -gt 0 -and (& $n 27) -gt 0 -and (& $n 28) -gt 0 -and `
        (& $n 30) -eq 0 -and (($kindMask -band (1u -shl 21)) -ne 0) -and `
        (& $n 32) -eq 0 -and (& $n 33) -gt 0) `
        'LinkBomb did not complete the generic DS item render/0x52 attachment path.' $match.Value
    Assert-LinkBomb ((& $n 35) -eq 0) 'LinkBomb proof exhausted the fighter-pose track pool.' $match.Value
    Assert-LinkBomb ((& $n 36) -eq 1) 'LinkBomb proof fixture must enter source Wait exactly once.' $match.Value

    $summary = 'P2-3 LinkBomb lifecycle passed: controller Down+B -> shared hold -> critical fuse -> source LightThrowF4 -> event-script explosion -> six-frame destroy.'
    Write-Output $summary
    Write-Output $match.Value
    if (-not [string]::IsNullOrWhiteSpace($Artifact)) {
        $artifactPath = if ([IO.Path]::IsPathRooted($Artifact)) { $Artifact } else { Join-Path $root $Artifact }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifactPath) | Out-Null
        Set-Content -LiteralPath $artifactPath -Value @($summary, $match.Value)
    }
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
