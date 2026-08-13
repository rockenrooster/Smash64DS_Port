[CmdletBinding()]
param(
    [string]$Build = 'build-c132-flamejoint',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600,
    [string]$Artifact = ''
)

# BUGS.md: Fox's visible gun is now present, but the owner reports that the
# muzzle flash / beam are at the wrong Y. Do not tune an offset. BattleShip
# gives us a closed invariant instead:
#
#   visible gun local point {60,0,0} through joint 17's draw matrix
#       ==
#   wpFoxBlasterMakeWeapon input position through the weapon camera
#       ==
#   FoxBlasterGlow input position through the particle camera.
#
# This probe captures the three production inputs from ONE natural Neutral-B.
# scripts/fox_muzzle_alignment.py then performs the exact DS fixed-point replay
# for X/Y. A non-zero screen delta identifies a renderer/camera seam; a zero
# delta proves that changing the source spawn position would be a fidelity bug.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_fox-muzzle-alignment.txt')
}

$required = @(
    'battleship_wpFoxBlasterMakeWeapon',
    'ndsRendererSubmitFoxGun',
    'ndsRendererSubmitFoxBlasterQuad',
    'sNdsRendererParticleProjection',
    'sNdsRendererParticleModelview',
    'gNdsFoxSpawnAX', 'gNdsFoxSpawnAY', 'gNdsFoxSpawnAZ',
    'gNdsFoxSpawnWorldMtx',
    'gNdsFoxSpawnChainDepth', 'gNdsFoxSpawnChainDObj',
    'gNdsFoxSpawnChainMode', 'gNdsFoxSpawnChainLocal',
    'gGCCurrentCamera', 'sNdsStageGCDrawAllLoopCurrentCameraGObj',
    'gNdsFoxGunWorldProbeCount', 'gNdsFoxGunWorldProbeFloatMtx',
    'gNdsFoxGunWorldProbeMtx',
    'gNdsFoxGunWorldProbeShotQ12', 'gNdsFoxGunChainDepth',
    'gNdsFoxGunChainDObj', 'gNdsFoxGunChainMode', 'gNdsFoxGunChainKind0',
    'gNdsFoxGunChainSourceLocal', 'gNdsFoxGunChainRendererLocal',
    'gNdsBattlePlayablePacingLogicFrames',
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsPositionProbeUpdateInPresent'
    # The BEAM_PRESENT / GLOW_PRESENT rows are deliberately absent. They read
    # gNdsFox{Beam,Glow}Presentation*, which only exist alongside the WIP
    # presentation latch withheld on 2026-08-13
    # (artifacts/bugs/2026-08-12_fox-crouch/wip-presentation-latch.patch, and
    # the "Why the WIP presentation latch is inert" section of CONTRACT.md).
    # Re-apply that patch before re-adding them, and add the matched/not-matched
    # engagement counter CONTRACT.md asks for at the same time -- source == draw
    # on its own cannot distinguish a zero delta from a FALSE return.
)
$symbols = & $nm -a $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Fox muzzle probe symbols absent from {0}: {1}" -f
        $elf, ($missing -join ', '))
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir 'melonds.fox-muzzle-alignment.stdout.log'
$stderr = Join-Path $logDir 'melonds.fox-muzzle-alignment.stderr.log'
$logTemp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$configState = $null
$emulator = $null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDir `
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

        'set $beams = 0',
        # Keep all three breakpoints armed together. That lets the log correlate
        # gun and weapon draws by gNdsFrameCounter instead of accidentally
        # comparing different frames after a debugger stop/resume.
        'break battleship_wpFoxBlasterMakeWeapon',
        'commands 1',
        'silent',
        'printf "FOXALIGN SPAWN frame=%u logic=%u presented=%u sub=%u %f %f %f\n", gNdsFrameCounter, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePlayablePacingPresentedFrames, gNdsPositionProbeUpdateInPresent, ((Vec3f *)$r1)->x, ((Vec3f *)$r1)->y, ((Vec3f *)$r1)->z',
        'continue',
        'end',

        'break ndsRendererSubmitFoxGun',
        'commands 2',
        'silent',
        'printf "FOXALIGN GUNMATRIX frame=%u logic=%u presented=%u\n", gNdsFrameCounter, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePlayablePacingPresentedFrames',
        'printf "FOXALIGN GUNCAMERA frame=%u current=0x%x loop=0x%x\n", gNdsFrameCounter, gGCCurrentCamera, sNdsStageGCDrawAllLoopCurrentCameraGObj',
        # The gun composes joint x FIGHTER camera cache; the beam loads the
        # PARTICLE camera. Capture the fighter cache HERE, per frame, so the two
        # can be compared on the same frame instead of against a single delayed
        # read taken frames later -- that mismatch alone can fake a several-px
        # divergence while the camera pans.
        'printf "FOXALIGN GUNFIGHTERCAM frame=%u count=%u cacheframe=%u pvalid=%u mvalid=%u\n", gNdsFrameCounter, sNdsRendererAdapterCameraCacheCount, sNdsRendererAdapterCameraCacheFrame, sNdsRendererAdapterCameraCache[0].projection_valid, sNdsRendererAdapterCameraCache[0].modelview_valid',
        'x/16dw &sNdsRendererAdapterCameraCache[0].projection',
        # Joint 17's LIVE world matrix at the gun-draw site. Compared against the
        # identical read at the beam site (below) this says whether the two draws
        # in one present sample the SAME Fox pose. Plain memory reads only.
        'set $fx = (FTStruct *)gSCManagerBattleState->players[1].fighter_gobj->user_data.p',
        'set $fj = (FTParts *)$fx->joints[17]->user_data.p',
        'printf "FOXALIGN JOINT17 frame=%u site=gun t=%f,%f,%f xrow=%f,%f,%f muzzleY=%f\n", gNdsFrameCounter, $fj->mtx_translate[3][0], $fj->mtx_translate[3][1], $fj->mtx_translate[3][2], $fj->mtx_translate[0][0], $fj->mtx_translate[0][1], $fj->mtx_translate[0][2], ($fj->mtx_translate[0][1] * 60) + $fj->mtx_translate[3][1]',
        'printf "FOXALIGN GUNCOMPOSED frame=%u\n", gNdsFrameCounter',
        'x/16dw $r0',
        'continue',
        'end',

        'break ndsRendererSubmitFoxBlasterQuad',
        'commands 3',
        'silent',
        'set $beams = $beams + 1',
        'printf "FOXALIGN BEAM frame=%u logic=%u presented=%u %f %f %f sx=0x%x sy=0x%x facing=%d\n", gNdsFrameCounter, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePlayablePacingPresentedFrames, ((Vec3f *)$r0)->x, ((Vec3f *)$r0)->y, ((Vec3f *)$r0)->z, $r1, $r2, $r3',
        'printf "FOXALIGN BEAMCAMERA frame=%u current=0x%x loop=0x%x\n", gNdsFrameCounter, gGCCurrentCamera, sNdsStageGCDrawAllLoopCurrentCameraGObj',
        # Same read as the gun site. If muzzleY differs between the two sites on
        # one frame, the stage draw and the fighter draw see different poses and
        # a latch sampled at THIS site can only ever compute a zero delta.
        'set $fx = (FTStruct *)gSCManagerBattleState->players[1].fighter_gobj->user_data.p',
        'set $fj = (FTParts *)$fx->joints[17]->user_data.p',
        'printf "FOXALIGN JOINT17 frame=%u site=beam t=%f,%f,%f xrow=%f,%f,%f muzzleY=%f\n", gNdsFrameCounter, $fj->mtx_translate[3][0], $fj->mtx_translate[3][1], $fj->mtx_translate[3][2], $fj->mtx_translate[0][0], $fj->mtx_translate[0][1], $fj->mtx_translate[0][2], ($fj->mtx_translate[0][1] * 60) + $fj->mtx_translate[3][1]',
        'printf "FOXALIGN BEAMCAM PROJECTION frame=%u\n", gNdsFrameCounter',
        'x/16dw &sNdsRendererParticleProjection',
        'printf "FOXALIGN BEAMCAM MODELVIEW frame=%u\n", gNdsFrameCounter',
        'x/16dw &sNdsRendererParticleModelview',
        'if $beams >= 4',
        # Read the ROM-internal position probe only after several presented
        # frames. A direct dereference of the maker's stack pointer can see
        # stale ARM9 D-cache (it did on c133); these probe globals are the same
        # source value copied in-ROM and have had time to drain before GDB reads.
        'printf "FOXALIGN SPAWN_SAVED %f %f %f\n", gNdsFoxSpawnAX, gNdsFoxSpawnAY, gNdsFoxSpawnAZ',
        'printf "FOXALIGN WORLDPROBE count=%u\n", gNdsFoxGunWorldProbeCount',
        'printf "FOXALIGN SPAWN_FLOAT_WORLD\n"',
        'x/16fw &gNdsFoxSpawnWorldMtx',
        'printf "FOXALIGN GUN_FLOAT_WORLD\n"',
        'x/16fw &gNdsFoxGunWorldProbeFloatMtx',
        'printf "FOXALIGN GUN_FIXED_WORLD\n"',
        'x/16dw &gNdsFoxGunWorldProbeMtx',
        'printf "FOXALIGN WORLDPOINTQ12\n"',
        'x/4dw &gNdsFoxGunWorldProbeShotQ12',
        'printf "FOXALIGN SPAWN_CHAIN depth=%u\n", gNdsFoxSpawnChainDepth',
        'x/18uw &gNdsFoxSpawnChainDObj',
        'x/18uw &gNdsFoxSpawnChainMode',
        'x/288fw &gNdsFoxSpawnChainLocal',
        'printf "FOXALIGN GUN_CHAIN depth=%u\n", gNdsFoxGunChainDepth',
        'x/18uw &gNdsFoxGunChainDObj',
        'x/18uw &gNdsFoxGunChainMode',
        'x/18uw &gNdsFoxGunChainKind0',
        'x/288fw &gNdsFoxGunChainSourceLocal',
        'x/288dw &gNdsFoxGunChainRendererLocal',
        # The gun composes joint x FIGHTER camera cache; the beam loads the
        # PARTICLE camera. A host replay of the two put the same world muzzle
        # 3.35-3.70 px apart in Y with X agreeing to 0.05 px, and the delta
        # VARIES per frame -- the signature of a stale cached camera, not of a
        # constant matrix bug. Dump both caches plus the particle cache's key
        # and hit/miss counters so the divergence is read, not inferred.
        'printf "FOXALIGN FIGHTERCAM count=%u frame=%u\n", sNdsRendererAdapterCameraCacheCount, sNdsRendererAdapterCameraCacheFrame',
        'printf "FOXALIGN FIGHTERCAM ENTRY0 cobj=0x%x pvalid=%u mvalid=%u\n", sNdsRendererAdapterCameraCache[0].cobj, sNdsRendererAdapterCameraCache[0].projection_valid, sNdsRendererAdapterCameraCache[0].modelview_valid',
        'printf "FOXALIGN FIGHTERCAM PROJECTION\n"',
        'x/16dw &sNdsRendererAdapterCameraCache[0].projection',
        'printf "FOXALIGN FIGHTERCAM MODELVIEW\n"',
        'x/16dw &sNdsRendererAdapterCameraCache[0].modelview',
        'printf "FOXALIGN PARTICLECACHE enabled=%u valid=%u key=0x%x hit=%u miss=%u loads=%u\n", gNdsParticleCameraCacheEnabled, sNdsParticleCameraValid, sNdsParticleCameraKey, gNdsParticleCameraCacheHitCount, gNdsParticleCameraCacheMissCount, gNdsParticleCameraLoads',
        'printf "FOXALIGN PARTICLECACHE PROJECTION\n"',
        'x/16dw &sNdsParticleCameraProjection',
        'printf "FOXALIGN PARTICLECACHE MODELVIEW\n"',
        'x/16dw &sNdsParticleCameraModelview',
        # BEAM_PRESENT / GLOW_PRESENT removed with the WIP presentation latch
        # (2026-08-13) -- see the $required note above.
        'detach',
        'quit',
        'end',
        'continue',
        'end',
        'continue'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'fox_muzzle_alignment_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    $captured = Join-Path $logTemp 'fox_muzzle_alignment_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(FOXALIGN|0x)' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $configState) {
        Restore-MelonDSGdbConfig -State $configState
    }
}
