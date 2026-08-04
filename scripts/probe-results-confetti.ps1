[CmdletBinding()]
param(
    [string]$Build = 'build-r2-a5i3',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [string]$Label = 'probe',
    [ValidateRange(60, 600)][int]$TimeoutSeconds = 420
)

# THE RESULTS-PHASE HALF OF THE PARTICLE/VFX CLUSTER. probe-vfx-contracts.ps1
# is the battle-phase half and deliberately never waits for Results; this one
# does nothing else, because reaching mnVSResultsMakeConfetti means playing out
# the whole one-minute match and that is the only trigger it has.
#
# It answers BUGS row 10 -- "Confetti pieces do not look like there are large
# enough and don't look like they are falling freely". The source makes TWO
# sheets (mnvsresults.c:3210): pos0 (0, 1000, -1000) with is_genlink_mask FALSE,
# which despite the name is the one that DOES take LBPARTICLE_MASK_GENLINK(3),
# and pos1 (0, 1000, -400) with TRUE, which takes none. So one sheet lands in
# alloc-link slot 4 and the other in slot 0, and efdisplay.c:85/97 draws those
# through two DIFFERENT GObjs -- the CLD DL-18 one and the AA_ZB_XLU DL-10 one.
# A sheet missing entirely, or drawn at the wrong depth, is therefore a
# per-slot question, which is why both slots are read separately below.
#
# "Falling freely" is pos.y sampled twice, 60 frames apart: gravity is in the
# script, so a y that does not move is an interpreter finding and a y that moves
# is not.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$build_dir = Join-Path $root "builds\$Build"
$rom = Join-Path $build_dir "$Target.nds"
$elf = Join-Path $build_dir "$Target.elf"
$capture_helper = Join-Path $root 'scripts\capture-running-melonds-window.ps1'
$screenshot = Join-Path $root "artifacts\visibility\2026-08-01_results-confetti-$Label.png"
$artifact = Join-Path $root "artifacts\verification\results-confetti-$Label.txt"
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot 7 -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot 7
$stdout = Join-Path $log_dir "melonds.bugs-results-confetti-$Label.stdout.log"
$stderr = Join-Path $log_dir "melonds.bugs-results-confetti-$Label.stderr.log"
$config_state = $null
$emulator = $null

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr `
        -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener `
        -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 15',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $confetti_calls = 0',
        'set $confetti_false = 0',
        'set $confetti_true = 0',
        'set $p0x = 0.0',
        'set $p0y = 0.0',
        'set $p0z = 0.0',
        'set $p1x = 0.0',
        'set $p1y = 0.0',
        'set $p1z = 0.0',
        'set $drawx = 0.0',
        'set $drawy = 0.0',
        'set $drawz = 0.0',
        'break efManagerConfettiMakeEffect',
        'commands',
        'silent',
        'if $confetti_calls == 0',
        'set $p0x = *(float *)$r0',
        'set $p0y = *(float *)($r0 + 4)',
        'set $p0z = *(float *)($r0 + 8)',
        'else',
        'set $p1x = *(float *)$r0',
        'set $p1y = *(float *)($r0 + 4)',
        'set $p1z = *(float *)($r0 + 8)',
        'end',
        'if is_genlink_mask == 0',
        'set $confetti_false = $confetti_false + 1',
        'else',
        'set $confetti_true = $confetti_true + 1',
        'end',
        'set $confetti_calls = $confetti_calls + 1',
        'continue',
        'end',
        'tbreak mnVSResultsMakeConfetti',
        'continue',
        'set $script0 = gNdsParticleScriptStartCount',
        'set $reject0 = gNdsParticleRejectCount',
        'set $emit0 = gNdsParticleQuadEmitCount',
        'set $miss0 = gNdsParticleQuadMissCount',
        # First sample: 40 ticks in, while the sheets are still near y=1000.
        'tbreak mnVSResultsFuncRun if sMNVSResultsTotalTimeTics >= 40',
        'continue',
        'set $s0n = 0',
        'set $s4n = 0',
        'set $s0y0 = 0.0',
        'set $s4y0 = 0.0',
        'set $s0size = 0.0',
        'set $s4size = 0.0',
        'set $s0tex = -1',
        'set $s4tex = -1',
        'if sLBParticleStructsAllocLinks[0] != 0',
        'set $s0n = 1',
        'set $s0y0 = sLBParticleStructsAllocLinks[0]->pos.y',
        'set $s0size = sLBParticleStructsAllocLinks[0]->size',
        'set $s0tex = sLBParticleStructsAllocLinks[0]->texture_id',
        'end',
        'if sLBParticleStructsAllocLinks[4] != 0',
        'set $s4n = 1',
        'set $s4y0 = sLBParticleStructsAllocLinks[4]->pos.y',
        'set $s4size = sLBParticleStructsAllocLinks[4]->size',
        'set $s4tex = sLBParticleStructsAllocLinks[4]->texture_id',
        'end',

        # Second sample, 120 ticks later. The head of each list is not
        # guaranteed to be the same particle, so this is the sheet's leading
        # edge over time rather than one piece's trajectory -- which is the
        # right question anyway: a sheet whose head y never leaves 1000 is not
        # falling, whoever is at the head.
        'tbreak mnVSResultsFuncRun if sMNVSResultsTotalTimeTics >= 160',
        'continue',
        'set $s0y1 = $s0y0',
        'set $s4y1 = $s4y0',
        # Walk each list rather than reading only its head. The first run of
        # this probe read the head alone and reported s0_live=0 at tick 40 with
        # s0_size never assigned, which answered neither half of the owner's
        # report. The COUNT is the half that matters most: the pool is 48
        # structs (battleship_lbparticle.c:194) and a battle peaks at 23, so a
        # confetti shower of eight pieces is not a pool limit, it is the script
        # or its generator not producing.
        # Size of EVERY particle in the list, not just the head. The list is
        # LIFO (lbparticle.c:324 pushes at the front), so the head is the most
        # RECENT spawn -- and the previous run read the head's size as 0.000000
        # on both sheets. Script 0x70's own header carries size 0.0 because it
        # is a pure spawner (efcommon_scb.c script_112: four lbpMakeGenerator
        # calls and lbpEnd), while the confetti scripts it starts, 108 to 111,
        # each carry size 20.0. So a size-0 head is either the invisible root or
        # a real piece that lost its size, and lbParticleDrawTextures skips
        # `pc->size == 0.0F` before it ever reaches the atlas -- which is why
        # such a piece costs no QuadMiss and would be invisible with every
        # counter clean. Counting sized vs unsized separates the two.
        # X extent is stochastic in the source: scripts 108..111 spawn children
        # in a radial field around the two x=0 roots. Seed from the head rather
        # than a sentinel so the first comparison remains meaningful.
        'set $s0count = 0',
        'set $s0sized = 0',
        'set $s0maxsize = 0.0',
        'set $s0minx = 0.0',
        'set $s0maxx = 0.0',
        'set $p = sLBParticleStructsAllocLinks[0]',
        'while $p != 0',
        'if $s0count == 0',
        'set $s0minx = $p->pos.x',
        'set $s0maxx = $p->pos.x',
        'end',
        'if $p->pos.x < $s0minx',
        'set $s0minx = $p->pos.x',
        'end',
        'if $p->pos.x > $s0maxx',
        'set $s0maxx = $p->pos.x',
        'end',
        'set $s0count = $s0count + 1',
        'if $p->size != 0',
        'set $s0sized = $s0sized + 1',
        'end',
        'if $p->size > $s0maxsize',
        'set $s0maxsize = $p->size',
        'end',
        'if $s0count == 1',
        'set $s0y1 = $p->pos.y',
        'set $s0size = $p->size',
        'set $s0tex = $p->texture_id',
        'end',
        'set $p = $p->next',
        'end',
        'set $s4count = 0',
        'set $s4sized = 0',
        'set $s4maxsize = 0.0',
        'set $s4minx = 0.0',
        'set $s4maxx = 0.0',
        'set $p = sLBParticleStructsAllocLinks[4]',
        'while $p != 0',
        'if $s4count == 0',
        'set $s4minx = $p->pos.x',
        'set $s4maxx = $p->pos.x',
        'end',
        'if $p->pos.x < $s4minx',
        'set $s4minx = $p->pos.x',
        'end',
        'if $p->pos.x > $s4maxx',
        'set $s4maxx = $p->pos.x',
        'end',
        'set $s4count = $s4count + 1',
        'if $p->size != 0',
        'set $s4sized = $s4sized + 1',
        'end',
        'if $p->size > $s4maxsize',
        'set $s4maxsize = $p->size',
        'end',
        'if $s4count == 1',
        'set $s4y1 = $p->pos.y',
        'set $s4size = $p->size',
        'set $s4tex = $p->texture_id',
        'end',
        'set $p = $p->next',
        'end',
        ('printf "CONFETTISLOTS s0_count=%d s0_sized=%d s0_maxsize=%f ' +
            's0_y1=%f s0_headsize=%f s0_tex=%d s0_minx=%f s0_maxx=%f ' +
            's4_count=%d s4_sized=%d s4_maxsize=%f ' +
            's4_y1=%f s4_headsize=%f s4_tex=%d s4_minx=%f s4_maxx=%f ' +
            'ratepatch=%u sizepatch=%u ' +
            'gens_used=%u gens_highwater=%u structs_used=%u\n", ' +
            '$s0count, $s0sized, $s0maxsize, $s0y1, $s0size, $s0tex, ' +
            '$s0minx, $s0maxx, ' +
            '$s4count, $s4sized, $s4maxsize, $s4y1, $s4size, $s4tex, ' +
            '$s4minx, $s4maxx, gNdsConfettiDensityPatchCount, ' +
            'gNdsConfettiSizePatchCount, ' +
            'gLBParticleGeneratorsUsedNum, gNdsParticleGeneratorsMax, ' +
            'gLBParticleStructsUsedNum'),
        ('printf "CONFETTICONTRACT roots=%d false=%d true=%d generators=%u slot0=%d slot4=%d\n", ' +
            '$confetti_calls, $confetti_false, $confetti_true, ' +
            'gLBParticleGeneratorsUsedNum, $s0count, $s4count'),
        ('printf "CONFETTI=%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", ' +
            '$confetti_calls, $confetti_false, $confetti_true, ' +
            'gNdsParticleScriptStartCount-$script0, ' +
            'gNdsParticleRejectCount-$reject0, ' +
            'gNdsParticleQuadEmitCount-$emit0, ' +
            'gNdsParticleQuadMissCount-$miss0, ' +
            'gNdsRendererParticleAtlasPrepareCount, ' +
            'gNdsRendererParticleAtlasFailCount, ' +
            'gNdsParticleTextureUseMask[0], gNdsParticleQuadMissMask[0], ' +
            'gNdsParticleStructsLive, gNdsParticleStructsMax, ' +
            'gNdsParticleTransformsMax, gNdsParticleDrawVisibleMax, ' +
            'gNdsVSResultsTickCount, ' +
            '$p0x, $p0y, $p0z, $p1x, $p1y, $p1z, ' +
            '$drawx, $drawy, $drawz'),
        'detach',
        'quit'
    )
    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName "results_confetti_$Label.gdb" `
        -TimeoutSeconds $TimeoutSeconds
    if (($capture.Stdout -notmatch
            'CONFETTICONTRACT roots=2 false=1 true=1 generators=8 slot0=[1-9][0-9]* slot4=[1-9][0-9]*') -or
        ($capture.Stdout -notmatch
            '(?m)^CONFETTI=(?:[^,\r\n]+,){16}0\.000000,1000\.000000,-1000\.000000,0\.000000,1000\.000000,-400\.000000,')) {
        throw 'Results confetti source contract failed: expected ordered source roots, two branches, eight generators, and live slots 0/4.'
    }
    Set-Content -LiteralPath $artifact -Value $capture.Stdout
    & $capture_helper -EmulatorProcessId $emulator.Id -Output $screenshot
    if ((-not (Test-Path -LiteralPath $screenshot -PathType Leaf)) -or
        ((Get-Item -LiteralPath $screenshot).Length -le 1024)) {
        throw "Results confetti capture failed: $screenshot"
    }
    $capture.Stdout
}
finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
