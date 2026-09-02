[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\\emulators\\melonds\\melonDS.exe'),
    [string]$Gdb = 'C:\\devkitPro\\devkitARM\\bin\\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4334,
    [int]$RunnerSlot = 8,
    [string]$Build = 'build-p2-link-special-tour-fast',
    [string]$Artifact = '',
    [switch]$NoBuild,
    [ValidateRange(30,1200)][int]$TimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\\gdb-markers.ps1')

function Assert-LinkSpecial {
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
        'NDS_P2_LINK_BOMB_TOUR=0' 'NDS_P2_LINK_SPECIAL_TOUR=1' `
        'NDS_HARNESS_FAST_PRESENT_ON_REQUEST=1' 'NDS_R2_FOX_CPU_DEFAULT=0'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-LinkSpecial (Test-Path -LiteralPath $path -PathType Leaf) `
        "Link special proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_HARNESS_FAST_PRESENT_ON_REQUEST 1',
    '#define NDS_P2_LINK 1',
    '#define NDS_P2_PROOF_FIGHTER0 5',
    '#define NDS_P2_LINK_BOMB_TOUR 0',
    '#define NDS_P2_LINK_SPECIAL_TOUR 1',
        '#define NDS_R2_FOX_CPU_DEFAULT 0',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-LinkSpecial $configText.Contains($definition) `
        "Link special proof build is missing required definition: $definition" $config
}
Assert-LinkSpecial $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Link special proof must run the mode-163 battle_playable harness.' $sceneConfig

$movementPath = Join-Path $root 'src\\port\\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$entryPath = Join-Path $root 'src\\import\\battleship_ftcommon_entry.c'
$entryText = Get-Content -LiteralPath $entryPath -Raw
Assert-LinkSpecial ($entryText.Contains('#if !NDS_P2_LINK_BOMB_TOUR')) `
    'Integrated Link special proof must leave the source Wave + Beam entry makers enabled.'
Assert-LinkSpecial (-not $entryText.Contains(
    '#if !NDS_P2_LINK_BOMB_TOUR && !NDS_P2_LINK_SPECIAL_TOUR')) `
    'Link special proof still suppresses the source entry makers.'
$fixtureComment = '/* The bounded Bomb verifier isolates Link''s item state machine'
$fixtureStart = $movementText.IndexOf($fixtureComment)
Assert-LinkSpecial ($fixtureStart -gt 0) 'Could not locate the Bomb-only source-Wait fixture.'
$fixturePrefix = $movementText.Substring([Math]::Max(0, $fixtureStart - 96),
    [Math]::Min(96, $fixtureStart))
Assert-LinkSpecial $fixturePrefix.Contains('#if NDS_P2_LINK_BOMB_TOUR') `
    'Link special proof still shares the Bomb tour source-Wait fixture.'
$tourStart = $movementText.IndexOf('void ndsLinkSpecialTourProofStop')
$tourEnd = $movementText.IndexOf('static sb32 ndsFighterNaturalCombatRecoverTeeter', $tourStart)
Assert-LinkSpecial (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the Link special guest proof driver for injection guards.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
foreach ($forbidden in @(
    'ftMainSetStatus\s*\(',
    'ftLinkSpecial[A-Za-z0-9_]*SetStatus\s*\(',
    'wpLink[A-Za-z0-9_]*MakeWeapon\s*\(',
    'efManager[A-Za-z0-9_]*MakeEffect\s*\(',
    'gcEjectGObj\s*\(',
    'ftCommonWaitSetStatus\s*\(',
    'ftParamUnlockPlayerControl\s*\(',
    'link->status_id\s*=(?!=)',
    'link->motion_id\s*=(?!=)'
)) {
    Assert-LinkSpecial ($tourText -notmatch $forbidden) `
        "Link special guest proof contains forbidden state injection: $forbidden"
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
        'set $link_stop = 0',
        'tbreak ndsLinkSpecialTourProofStop',
        'commands',
        'silent',
        'set $link_stop = 1',
        'end',
        'continue',
        'printf "LINK_SPECIAL_STOPPED=%u\n", $link_stop',
        'printf "LINK_SPECIAL_TOUR=%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsLinkSpecialTourPhase,gNdsLinkSpecialTourFrames,gNdsLinkSpecialTourInputCount,gNdsLinkSpecialTourStatusMask,gNdsLinkSpecialTourBoomerangObserved,gNdsLinkSpecialTourBoomerangDrawObserved,gNdsLinkSpecialTourBoomerangCatchObserved,gNdsLinkSpecialTourSpinObserved,gNdsLinkSpecialTourSpinDrawObserved,gNdsLinkSpecialTourDone,gNdsLinkSpecialTourFixtureCount',
        'printf "LINK_ENTRY=%#x,%#x,%#x,%#x,%u,%u,%u,%u\n", gNdsLinkSpecialTourEntrySeenMask,gNdsLinkSpecialTourEntryDrawMask,gNdsLinkSpecialTourEntryEjectMask,gNdsLinkSpecialTourEntryStatusMask,gNdsLinkSpecialTourEntryInitialAnim[0],gNdsLinkSpecialTourEntryInitialAnim[1],gNdsLinkSpecialTourEntryLiveFrames[0],gNdsLinkSpecialTourEntryLiveFrames[1]',
        'printf "LINK_SPECIAL_RENDER=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsEntryEffectNativeDrawCount,gNdsEntryEffectNativeFallbackCount,gNdsEntryEffectNativeTexturePrepareCount,gNdsEntryEffectNativeTextureBindCount,gNdsWeaponRendererCaptureCount,gNdsWeaponRendererDObjDrawCount,gNdsWeaponRendererSubmitCount,gNdsWeaponRendererVisibleDrawCount,gNdsWeaponRendererTriangleCount,gNdsWeaponRendererTextureRejectCount,gNdsHarnessFastPresentRequestCount,gNdsHarnessFastPresentConsumeCount,gNdsWeaponRendererCallbackKind,gNdsWeaponRendererRejectedDrawCount',
        'printf "LINK_SPECIAL_MATRIX=%u,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsRendererAdapterEffectPrepCount,gNdsRendererAdapterEffectPrepMask,gNdsEffectDLCfgMask,gNdsEffectDLMatrixSeed,gNdsEffectDLMatrixCmd,gNdsEffectDLXformVertexCount,gNdsEffectDLHwVertexCount,gNdsEffectDLBlocker',
        'printf "LINK_SPECIAL_ROOTS=%u,%u,%u,%u,%u,%u,%#x\n", gNdsEntryEffectNativeRootDraws[23],gNdsEntryEffectNativeRootDraws[24],gNdsEntryEffectNativeRootDraws[25],gNdsEntryEffectNativeRootDraws[26],gNdsEntryEffectNativeRootDraws[27],gNdsEntryEffectNativeRootDraws[28],gNdsWeaponRendererKindMask',
        'printf "LINK_SPECIAL_BOOMERANG_PROJECT=%u,%f,%f,%f,%f,%d,%d\n", gNdsLinkBoomerangProjectCount,gNdsLinkBoomerangProjectWorldX,gNdsLinkBoomerangProjectWorldY,gNdsLinkBoomerangProjectScreenX,gNdsLinkBoomerangProjectScreenY,gGMCameraStruct.viewport_width,gGMCameraStruct.viewport_height',
        'printf "LINK_SPECIAL_BOOMERANG_DISPLAY=%u,%u,%u,%u\n", gNdsLinkBoomerangDisplayCallCount,gNdsLinkBoomerangDisplayTreeCount,gNdsLinkBoomerangDisplayMode,gNdsLinkBoomerangDisplayAttackState',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-link-specials.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-link-specials.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    Assert-LinkSpecial ($stdout -match 'LINK_SPECIAL_STOPPED=1') `
        'Link special proof stopped before its cache-coherent terminal marker.' $stdout

    $tourMatch = [regex]::Match($stdout, 'LINK_SPECIAL_TOUR=([^\r\n]+)')
    Assert-LinkSpecial $tourMatch.Success 'Link special terminal marker is missing.' $stdout
    $v = @($tourMatch.Groups[1].Value.Split(','))
    Assert-LinkSpecial ($v.Count -eq 11) `
        "Link special terminal marker has $($v.Count) fields; expected 11." $tourMatch.Value
    $u = { param([int]$i)
        if ($v[$i].StartsWith('0x')) { [Convert]::ToUInt32($v[$i].Substring(2), 16) }
        else { [uint32]$v[$i] }
    }
    $statusMask = & $u 3
    Assert-LinkSpecial ((& $u 0) -eq 11) 'Link special proof did not reach Done.' $tourMatch.Value
    Assert-LinkSpecial ((& $u 2) -eq 2) 'Link special proof must issue exactly B + Up-B.' $tourMatch.Value
    Assert-LinkSpecial (($statusMask -band 0xf) -eq 0xf) `
        'Link did not naturally reach Neutral-B/Get and grounded Spin/End statuses.' $tourMatch.Value
    foreach ($i in 4..9) {
        Assert-LinkSpecial ((& $u $i) -eq 1) 'Link special runtime observation is incomplete.' $tourMatch.Value
    }
    Assert-LinkSpecial ((& $u 10) -eq 0) `
        'Integrated Link special proof must not use the source-Wait fixture.' $tourMatch.Value

    $entryMatch = [regex]::Match($stdout, 'LINK_ENTRY=([^\r\n]+)')
    Assert-LinkSpecial $entryMatch.Success 'Link entry runtime evidence is missing.' $stdout
    $e = @($entryMatch.Groups[1].Value.Split(','))
    Assert-LinkSpecial ($e.Count -eq 8) `
        "Link entry marker has $($e.Count) fields; expected 8." $entryMatch.Value
    $eu = { param([int]$i)
        if ($e[$i].StartsWith('0x')) { [Convert]::ToUInt32($e[$i].Substring(2), 16) }
        else { [uint32]$e[$i] }
    }
    foreach ($i in 0..3) {
        Assert-LinkSpecial ((& $eu $i) -eq 3) `
            'Link Wave + Beam did not complete natural create/draw/eject/status coverage.' $entryMatch.Value
    }
    Assert-LinkSpecial (((& $eu 4) -eq 0) -and ((& $eu 5) -eq 0)) `
        'Link Wave + Beam maker return should precede the first gcPlayAnimAll update.' $entryMatch.Value
    Assert-LinkSpecial (((& $eu 6) -eq 60) -and ((& $eu 7) -eq 60)) `
        'Fast-logic Link Wave + Beam must remain linked for 60 controller samples (120 source updates).' $entryMatch.Value

    $renderMatch = [regex]::Match($stdout, 'LINK_SPECIAL_RENDER=([^\r\n]+)')
    Assert-LinkSpecial $renderMatch.Success 'Link special render evidence is missing.' $stdout
    $r = @($renderMatch.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    Assert-LinkSpecial ($r.Count -eq 14) `
        "Link special render marker has $($r.Count) fields; expected 14." $renderMatch.Value
    Assert-LinkSpecial (($r[0] -ge 6) -and ($r[1] -eq 0) -and
        ($r[2] -gt 0) -and ($r[3] -gt 0)) `
        'Link native special-effect renderer did not draw cleanly without fallback.' $renderMatch.Value
    Assert-LinkSpecial (($r[4] -gt 0) -and ($r[5] -gt 0) -and
        ($r[6] -gt 0) -and ($r[7] -gt 0) -and ($r[8] -gt 0) -and
        ($r[9] -eq 0)) `
        'Link Boomerang/Spin weapon display path did not render cleanly.' $renderMatch.Value
    Assert-LinkSpecial (($r[10] -ge 3) -and ($r[11] -eq $r[10])) `
        'Link special proof must consume every requested entry/Boomerang/Spin hardware draw.' $renderMatch.Value

    $rootsMatch = [regex]::Match($stdout, 'LINK_SPECIAL_ROOTS=([^\r\n]+)')
    Assert-LinkSpecial $rootsMatch.Success 'Link native root evidence is missing.' $stdout
    $roots = @($rootsMatch.Groups[1].Value.Split(','))
    Assert-LinkSpecial ($roots.Count -eq 7) `
        "Link native-root marker has $($roots.Count) fields; expected 7." $rootsMatch.Value
    foreach ($i in 0..5) {
        Assert-LinkSpecial ([uint32]$roots[$i] -gt 0) `
            'Link entry/Boomerang/Spin did not execute every exact generated native root.' $rootsMatch.Value
    }

    $summary = 'P2-3 Link specials passed: natural 120-tick Wave+Beam entry/eject -> controller Neutral-B Boomerang native draw/catch -> grounded Up-B native Spin effect/LinkModel weapon/end.'
    Write-Output $summary
    Write-Output $entryMatch.Value
    Write-Output $tourMatch.Value
    Write-Output $renderMatch.Value
    Write-Output $rootsMatch.Value
    if (-not [string]::IsNullOrWhiteSpace($Artifact)) {
        $artifactPath = if ([IO.Path]::IsPathRooted($Artifact)) { $Artifact } else { Join-Path $root $Artifact }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifactPath) | Out-Null
        Set-Content -LiteralPath $artifactPath -Value @(
            $summary, $entryMatch.Value, $tourMatch.Value,
            $renderMatch.Value, $rootsMatch.Value)
    }
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
