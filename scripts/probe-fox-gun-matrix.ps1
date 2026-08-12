[CmdletBinding()]
param(
    [string]$Build = 'build-c128-foxgun',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
    [string]$EvidenceLabel = '2026-08-12_fox-gun-matrix'
)

# BUGS.md "Fox's pistol model is missing" -- RE-OPENED 2026-08-12 after the
# owner playtested build-c128-foxgun and still saw no pistol.
#
# The previous acceptance was invalid twice over, and both mistakes are visible
# in artifacts/visibility/2026-08-12_fox-gun-overlay-shot.png:
#   * The bright magenta bar beside Fox is NDS_R2_FOX_BLASTER_QUAD's debug quad
#     (nds_renderer.c, `glColor(RGB15(27, 0, 16))`), not the gun.
#   * Its "pre-fix control", 2026-08-09_fox-blaster-native-promoted.png, is a
#     different camera and pose entirely -- Fox is not even in the same region
#     of the frame -- so nothing was stacked against anything.
# `tris = 22 x draws` proves the submit ran. It cannot prove the triangles
# landed on screen, and this probe exists to answer exactly that.
#
# WHAT IT READS, and why this is the first divergence worth measuring: the whole
# overlay reduces to one matrix. ndsRendererSubmitFoxGun loads `composed`
# straight into GL_MODELVIEW and the DS projection matrix is identity for the
# whole run (nds_platform.c:372), so `composed` IS the model-view-projection.
# Its row 3 is therefore joint 17's own clip-space position, and the 44 baked
# vertices (times NDS_FOX_GUN_VERTEX_SCALE) give the rest. Dump the sixteen
# words and the gun's screen rectangle follows by arithmetic on the host --
# no build, no guessing about where the mesh went.
#
# Predictions written before the run, so a surprise is legible:
#   * If the chain is sound, row 3 divided by its w lands on Fox's hand in the
#     screenshot this probe takes, and the 44 transformed corners span a few
#     tens of pixels around it.
#   * If joint 17's transform never got rebuilt, row 3 is the world origin
#     projected -- off to one side, or behind the camera with w <= 0.
#   * If the scale convention is wrong, the rectangle is either sub-pixel or
#     hundreds of pixels wide. (Static evidence already says it is not: every
#     x/y/z in nds_native_fighter_owner.generated.inc's prepared dense table is
#     an exact multiple of 16, which is the same x16 the gun submit applies.)
#
# THE CAPTURE DELIBERATELY DOES NOT CHASE THE SHOT. An earlier cut ran to
# battleship_wpFoxBlasterMakeWeapon first, and the frame that produces is the
# one frame where the pistol cannot be seen: the muzzle flash is a large pink
# quad centred on the barrel and it covers the whole gun
# (artifacts/visibility/2026-08-12_fox-gun-c129.png). The model-part window is
# much longer than the flash, so any gun-out frame is better evidence, and the
# third submit is one the overlay has already presented.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$captureHelper = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$shot = Join-Path $root ('artifacts\visibility\' + $EvidenceLabel + '.png')
$shotA = Join-Path $root ('artifacts\visibility\' + $EvidenceLabel + '-a.png')
$artifact = Join-Path $root ('artifacts\verification\' + $EvidenceLabel + '.txt')
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.fox-gun-matrix.stdout.log'
$stderr = Join-Path $log_dir 'melonds.fox-gun-matrix.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'ndsRendererSubmitFoxGun',
    'gNdsRendererFoxGunDrawCount',
    'gNdsRendererFoxGunTriangleCount',
    'gNdsRendererFoxGunFailCount',
    'gNdsRendererFoxGunPrepareCount',
    'gNdsRendererFoxGunBytes',
    'gSCManagerBattleState',
    'ndsBattlePlayableFrameCompleteMarker'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Fox gun matrix probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr, $shot, $shotA `
        -Force -ErrorAction SilentlyContinue
    # WindowStyle: visible-by-design -- capture-running-melonds-window.ps1 needs
    # a real top-level window handle, and Hidden leaves MainWindowHandle zero so
    # the screenshot dies as a black PNG instead of as an error.
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Normal `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $captureCommand =
        'shell pwsh.exe -NoProfile -File "{0}" -EmulatorProcessId {1} -Output "{2}" 2>&1' -f
        $captureHelper.Replace('\', '/'), $emulator.Id, $shot.Replace('\', '/')
    $captureCommandA =
        'shell pwsh.exe -NoProfile -File "{0}" -EmulatorProcessId {1} -Output "{2}" 2>&1' -f
        $captureHelper.Replace('\', '/'), $emulator.Id, $shotA.Replace('\', '/')

    # `x/16dw $r0` rather than a typed member read: at the function entry the ARM
    # ABI guarantees r0 is `composed`, and a raw dump cannot be defeated by a
    # missing typedef or an -O2 location list. Reading it as a struct is the
    # convenience that would abort the whole commands block on one bad name.
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'break ndsRendererSubmitFoxGun',
        'continue',
        'printf "FOXMTX n=1 composed=%p tr=%d\n", $r0, gSCManagerBattleState->time_remain',
        'x/16dw $r0',
        # THE COMPARATOR. The gun submit runs inside Fox's own draw, so the
        # production workspace still holds the matrices the visible body was
        # just drawn with. Same camera, same frame, same units -- if the gun's
        # linear part is orders of magnitude smaller than the body's, the gun is
        # a sub-pixel speck and every counter can still read healthy.
        'printf "FOXBODY composed_matrices[0]\n"',
        'x/16dw &sNdsRendererAdapterNativeOwnerWorkspace.composed_matrices[0]',
        'printf "FOXCAM count=%u projection then modelview\n", sNdsRendererAdapterCameraCacheCount',
        'x/16dw &sNdsRendererAdapterCameraCache[0].projection',
        'x/16dw &sNdsRendererAdapterCameraCache[0].modelview',
        'continue',
        'printf "FOXMTX n=2 composed=%p tr=%d\n", $r0, gSCManagerBattleState->time_remain',
        'x/16dw $r0',
        'printf "FOXBODY composed_matrices[0]\n"',
        'x/16dw &sNdsRendererAdapterNativeOwnerWorkspace.composed_matrices[0]',
        'continue',
        'printf "FOXMTX n=3 composed=%p tr=%d\n", $r0, gSCManagerBattleState->time_remain',
        'x/16dw $r0',
        # CAPTURE AT A FRAME BOUNDARY, not on a submit. Halting inside the 3D
        # draw and grabbing the window gave a half-drawn frame with a black
        # wedge across Fox -- unusable as pixel evidence, and exactly the
        # "gdb-halted melonDS looks like a frame drop" trap. The frame-complete
        # marker is noinline+used precisely so it can be broken on.
        # TWO captures, one frame apart. The DS presents the 3D buffer a frame
        # after it is built, and Fox's model-part window is only about four
        # frames long, so a single guess at the offset either shows the frame
        # before the gun or lands past the end of the window. Take both and let
        # the reader pick; the cost is one extra PNG.
        'delete breakpoints',
        'break ndsBattlePlayableFrameCompleteMarker',
        'continue',
        $captureCommandA,
        'continue',
        $captureCommand,
        'printf "FOXGUNDONE draws=%u tris=%u fail=%u bytes=%u prepare=%u\n", gNdsRendererFoxGunDrawCount, gNdsRendererFoxGunTriangleCount, gNdsRendererFoxGunFailCount, gNdsRendererFoxGunBytes, gNdsRendererFoxGunPrepareCount',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'fox_gun_matrix_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the helper's capture file, not its return value: a timed-out run holds
    # the same evidence as a completed one and only the second throws.
    $captured = Join-Path $log_temp 'fox_gun_matrix_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $artifact -Force
        Get-Content -LiteralPath $artifact |
            Where-Object { $_ -match '^(FOX|0x)' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $artifact"
        if (Test-Path -LiteralPath $shot) {
            Write-Output "screenshot: $shot"
        } else {
            Write-Output 'screenshot: ABSENT'
        }
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
