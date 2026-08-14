[CmdletBinding()]
param(
    [string]$Build = 'build-c144-ledgeridx',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(60, 1800)][int]$TimeoutSeconds = 900,
    [ValidateRange(1, 400)][int]$Rounds = 48,
    [ValidateRange(1, 120)][int]$FrameStride = 24,
    # Presented frames to skip before the first sample. Collision inverses only
    # happen once the match is live and someone is attacking, so a probe that
    # samples from the GO countdown reads zero joints and looks exactly like a
    # dead latch. The tick-HUD gate window itself starts at frame 438.
    [ValidateRange(0, 4000)][int]$WarmupFrames = 600,
    # Collision entries to let past before sampling. The latches are per-frame
    # and are set by the hit path itself, so a stop taken at the FIRST collision
    # entry of a frame sees an empty tree. Skipping N entries lands the stop at a
    # point where N joints have already been prepared -- and it self-selects the
    # frames that have collision activity at all, which is exactly the domain the
    # kernel has to hold on.
    [ValidateRange(0, 256)][int]$CollisionSkip = 16,
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
    [string]$EvidenceLabel = '2026-08-13_collision-fixed-domain'
)

# R2-07 cycle B, checklist item 6: the LIVE input domain of the fixed-point
# world->local frame, and the live decline rate of its guards.
#
# What it reads and why that is the honest read. The cofactor frame is built
# from FTParts::mtx_translate -- the joint's float world matrix, which the seam
# analysis (artifacts/performance/2026-08-13_c-collision-fixed/DESIGN.md section
# 6) leaves float because the renderer and ~30 other readers consume it. So the
# quantity that decides whether a guard declines is that matrix, and it is
# reachable as a pointer-deref chain from two globals. No register is read, no
# breakpoint lands inside an optimized body, and no guest function is called:
# CLAUDE.OPUS.md's probe rails rule out all three, and the crouch probe already
# paid for the guest-call lesson (a `call func_ovl2_800EDBA4` hung the target).
#
# unk_dobjtrans_0x7 is the invert latch and unk_dobjtrans_0x6 the scale latch.
# ndsFTParamsInvalidateFighterParts zeroes unk_dobjtrans_word for every joint of
# every fighter each frame, so a joint with 0x7 set at the frame-complete marker
# is one this frame's collision actually inverted -- not a stale entry.
#
# This probe changes nothing. It samples, prints, and detaches; the grading is
# done on the host by scripts/grade-r2-collision-live-domain.c, which compiles
# the SHIPPING header rather than a transcription of it.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$artifact = Join-Path $root ('artifacts\verification\' + $EvidenceLabel + '.txt')

# The gate arm is NDS_R2_BOTH_CPU=1. A one-CPU ROM leaves Mario standing still,
# which samples one pose family and reads as a flatteringly narrow domain.
$configHeader = Join-Path $root ('builds\' + $Build + '\nds_build_config.h')
if (-not (Test-Path -LiteralPath $configHeader -PathType Leaf)) {
    throw "Build config header is missing: $configHeader"
}
$configText = [System.IO.File]::ReadAllText($configHeader)
if ($configText -notmatch '(?m)^#define\s+NDS_R2_BOTH_CPU\s+1\s*$') {
    throw ("This ROM is not NDS_R2_BOTH_CPU=1. The collision domain probe must " +
           "run on the gate arm, where both fighters are level-3 CPUs and the " +
           "joint poses are the ones the hit path actually inverts.")
}

$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$required = @(
    'scVSBattleStartBattle',
    'ndsBattlePlayableFrameCompleteMarker',
    'gSCManagerBattleState',
    'gNdsFrameCounter'
)
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ('Collision domain probe symbols absent: ' + ($missing -join ', '))
}
# The seam reinterprets unk_dobjtrans_0x9C. The port writer that would fill it
# with a float identity lives behind sNdsFighterPartsPool; DESIGN section 6
# asserts that pool is not linked. Assert it here rather than inherit it.
$forbidden = @('sNdsFighterPartsPool', 'ndsFighterPartsSyncDObj')
$present = @($forbidden | Where-Object { $symbols -contains $_ })
if ($present.Count -gt 0) {
    throw ('The FTParts pool writer IS linked in this ROM (' +
           ($present -join ', ') + '), so unk_dobjtrans_0x9C has a second ' +
           'writer and the seam assumption is false.')
}

$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir 'melonds.collision-fixed-domain.stdout.log'
$stderr = Join-Path $logDir 'melonds.collision-fixed-domain.stderr.log'
$configState = $null
$emulator = $null

try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr, $artifact -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory $melonDir -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set print repeats 0',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'tbreak scVSBattleStartBattle',
        'continue',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'delete breakpoints',
        'break ndsBattlePlayableFrameCompleteMarker',
        'set $fbp = $bpnum',
        # The latch is cleared by ftParamsUpdateFighterPartsTransformAll, and a
        # first draft of this probe sampled at the frame-complete marker and read
        # latch5=latch6=latch7=0 on 52 live joints three rounds running. That is
        # not a dead latch, it is the wrong instant: the invalidate has already
        # run by the time the frame completes. Its ENTRY is the one point where
        # the whole previous frame's collision state is still intact, so that is
        # where the sample is taken.
        'break gmCollisionCheckFighterAttackDamageCollide',
        'set $ibp = $bpnum',
        'disable $ibp',
        'printf "CFX_READY frame_bp=%d collision_bp=%d\n", $fbp, $ibp',
        ('ignore $fbp {0}' -f $WarmupFrames),
        'continue',
        'printf "CFX_WARM frame=%u\n", gNdsFrameCounter',
        'set $round = 0',
        ('while $round < {0}' -f $Rounds),
        '  disable $ibp',
        '  enable $fbp',
        ('  ignore $fbp {0}' -f ($FrameStride - 1)),
        '  continue',
        '  disable $fbp',
        '  enable $ibp',
        ('  ignore $ibp {0}' -f $CollisionSkip),
        '  continue',
        '  set $p = 0',
        '  set $n5 = 0',
        '  set $n6 = 0',
        '  set $n7 = 0',
        '  set $njoint = 0',
        '  while $p < 2',
        '    set $g = (GObj *)gSCManagerBattleState->players[$p].fighter_gobj',
        '    if $g != 0',
        '      set $fst = (FTStruct *)$g->user_data.p',
        '      set $j = 0',
        '      while $j < 37',
        '        set $d = $fst->joints[$j]',
        '        if $d != 0',
        '          set $q = (FTParts *)$d->user_data.p',
        '          if $q != 0',
        '            set $njoint = $njoint + 1',
        '            if $q->unk_dobjtrans_0x5 != 0',
        '              set $n5 = $n5 + 1',
        '            end',
        '            if $q->unk_dobjtrans_0x6 != 0',
        '              set $n6 = $n6 + 1',
        '            end',
        '            if $q->unk_dobjtrans_0x7 != 0',
        '              set $n7 = $n7 + 1',
        '            end',
        '            if $q->unk_dobjtrans_0x7 != 0',
        '              printf "CFXM %u %d %d %d %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %d\n", gNdsFrameCounter, $round, $p, $j, $q->mtx_translate[0][0], $q->mtx_translate[0][1], $q->mtx_translate[0][2], $q->mtx_translate[1][0], $q->mtx_translate[1][1], $q->mtx_translate[1][2], $q->mtx_translate[2][0], $q->mtx_translate[2][1], $q->mtx_translate[2][2], $q->mtx_translate[3][0], $q->mtx_translate[3][1], $q->mtx_translate[3][2], $q->vec_scale.x, $q->vec_scale.y, $q->vec_scale.z, $q->unk_dobjtrans_0x6',
        '            end',
        '          end',
        '        end',
        '        set $j = $j + 1',
        '      end',
        '    end',
        '    set $p = $p + 1',
        '  end',
        '  printf "CFXC %u %d joints=%d latch5=%d latch6=%d latch7=%d\n", gNdsFrameCounter, $round, $njoint, $n5, $n6, $n7',
        '  set $round = $round + 1',
        'end',
        'printf "CFX_DONE rounds=%d frame=%u\n", $round, gNdsFrameCounter',
        'detach',
        'quit'
    )

    # A round costs as long as it takes forty collision entries to happen, which
    # depends on how often the two CPUs actually swing. -Rounds therefore does
    # NOT have a fixed wall cost, and a large one exits by timeout. That must not
    # cost the capture: CLAUDE.OPUS.md's rail is "write artifacts before the
    # wait", and the helper's timeout throw carries the partial stdout, so the
    # rows already sampled are recovered and graded rather than discarded.
    $timedOut = $false
    $captureText = ''
    try {
        $capture = Invoke-GdbMarkerScript -Gdb $gdb -Elf $elf -Root $root `
            -Commands $commands -ScriptName 'collision_fixed_domain_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds
        $captureText = $capture.Stdout
    } catch {
        $message = $_.Exception.Message
        if ($message -notmatch 'GDB marker capture timed out') {
            throw
        }
        $timedOut = $true
        $captureText = $message
    }
    [System.IO.Directory]::CreateDirectory((Split-Path -Parent $artifact)) | Out-Null
    [System.IO.File]::WriteAllText($artifact, $captureText, [System.Text.Encoding]::UTF8)

    $lines = @($captureText -split "`r?`n")
    $rows = @($lines | Where-Object { $_ -match '^CFXM ' })
    $ready = @($lines | Where-Object { $_ -match '^CFX_(READY|DONE)' })
    $ready | ForEach-Object { Write-Output $_ }
    if ($rows.Count -eq 0) {
        throw 'Collision domain probe captured no inverted joints.'
    }
    if ($timedOut) {
        Write-Output ("CFX_PARTIAL the run hit -TimeoutSeconds {0}; the rows " +
                      "below are what it sampled before the kill." -f $TimeoutSeconds)
    }
    Write-Output ("CFX_ROWS {0}" -f $rows.Count)
    Write-Output "probe capture: $artifact"
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
            $emulator.WaitForExit()
        }
    }
    if ($null -ne $configState) {
        Restore-MelonDSGdbConfig -State $configState
    }
}
