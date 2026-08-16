[CmdletBinding()]
param(
    [string]$Build = 'build-c205-camtoggle',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 3000,
    # HITS MUST BE LESS THAN THE MATCH'S PRESENTED-FRAME COUNT. The counted
    # loop ends by NOT continuing on hit number $Hits, which is what returns
    # control to the script; if the battle loop exits first the marker never
    # fires again and the run sits until -TimeoutSeconds. Mode 163 presents
    # ~2,043 frames from scVSBattleStartBattle, so 1,900 leaves margin.
    [ValidateRange(1, 4000)][int]$Hits = 1900,
    # Comma-separated frame indices (1-based, counted from
    # scVSBattleStartBattle) at which the route word is toggled. Empty means
    # "never touch it" -- the ROM's own default arm for the whole match, which
    # is the minimum-perturbation form.
    #
    # A STRING, NOT AN INT ARRAY, ON PURPOSE. Long runs are launched detached
    # through `cmd /c "pwsh -File ..."` because that is the only redirect a
    # force-kill cannot eat, and pwsh -File binds `-FlipAt 400 800 1200 1600`
    # by taking the first token and shifting the rest onto POSITIONAL
    # parameters -- 1600 landed in -RouteInitial and only its ValidateRange
    # caught it. An int[] parameter cannot be passed correctly through the one
    # launch form the measurement rules require, so it is not offered.
    [string]$FlipAt = '',
    [string]$RouteGlobal = 'gNdsR2CameraFixedEnabled',
    # Comma-separated u32 globals appended to every -PerFrame row as
    # `x0=.. x1=..`. Same reason as sample-tick-hud-buckets.ps1's flag: a
    # cadence row without an engagement or cache counter beside it cannot tell
    # a slower arm from an arm that invalidated something.
    [string]$ExtraGlobals = '',
    # -1 leaves the boot value alone. 0/1 forces the arm at the FIRST frame
    # stop, which is the same seam sample-tick-hud-buckets.ps1 -SetGlobals uses.
    [ValidateRange(-1, 1)][int]$RouteInitial = -1,
    # Off: the frame breakpoint still fires but reads nothing. On: one gdb
    # memory read set per presented frame, which is what yields the per-frame
    # interval series and the post-flip transient.
    [switch]$PerFrame,
    # A symbol reached AFTER the battle loop exits. With `-Hits 1` this makes
    # the whole match run on TWO stops -- one to select the arm, one to read
    # the guest's own cumulative histogram -- which is the control for "do the
    # per-frame stops move the pacing they are measuring?".
    [string]$EndBreak = '',
    [string]$Artifact = ''
)

# IS THE PRESENTED CADENCE WORSE ON THIS ARM?
#
# AGENTS.md requires every device A/B report to carry the 2/3/4/5+ VBlank
# interval histogram, the max interval, and P50/P95. Nothing in the tree
# produced that for an arbitrary arm on a proof ROM: probe-battlepack-pacing.ps1
# reads the same counters but is wired to the pack's own globals, and the tick
# HUD instrument is a different (and differently paced) binary. This is the
# general form.
#
# ndsBattlePlayablePresentRealtimeFrame (taskman_seam.c:5032) schedules each
# present at last+NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS and then measures the
# interval it actually got, so a 2 is on-cadence 30 Hz and every 3/4/5+ is a
# slip. gNdsBattlePlayablePacingVBlanks is cumulative and published for the
# debugger at the frame-complete seam, so differencing it across consecutive
# ndsBattlePlayableFrameCompleteMarker stops gives that frame's interval; the
# guest's own bucket array is read at the same stops as an independent check on
# the differenced series.
#
# HALTING THE GUEST DOES NOT PERTURB THESE NUMBERS. melonDS stops the whole
# emulated machine at a breakpoint, so neither the VBlank counter nor
# cpuGetTiming advances during a stop -- a halt costs host wall time and no
# guest time. -PerFrame:$false exists to test exactly that claim rather than
# assert it: the same arm run with two stops must produce the same histogram as
# the same arm run with two thousand.
#
# THE HISTOGRAM WAS PUBLISHED BY ACCIDENT UNTIL 2026-08-16, AND NOW IS NOT.
# gNdsBattlePlayablePacingPresentIntervalBucket is not a member of
# NDS_BATTLE_PLAYABLE_PACING_GROUP and cannot be -- that list is pinned
# bilaterally against the BPLAY_PACE printf by check-gbi-decode-fixtures.ps1 and
# this array is not in that marker. Until 2026-08-16 it stayed readable only
# because DC_FlushRange cleans whole 32-byte lines and two group members happened
# to share the array's two lines (CadenceViolationCount at ...220 covering
# buckets 0-2 at ...234/238/23c, IntervalMax/Min at ...24c/250 covering buckets
# 3-5 at ...240/244/248), so any relayout of diagnostics.c silently returned the
# one histogram AGENTS.md requires in every device A/B to stale reads. It now has
# its own NDS_BATTLE_PLAYABLE_PACING_HISTOGRAM_GROUP, published at the same
# frame-complete seam and pinned by the same checker.

$ErrorActionPreference = 'Stop'
$flips = @()
if (-not [string]::IsNullOrWhiteSpace($FlipAt)) {
    $flips = @($FlipAt -split '[,;/ ]+' |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object {
            $value = 0
            if (-not [int]::TryParse($_.Trim(), [ref]$value)) {
                throw "FlipAt token '$_' is not an integer frame index."
            }
            if (($value -lt 1) -or ($value -gt 4000)) {
                throw "FlipAt frame index $value is outside 1..4000."
            }
            $value
        })
}
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\performance\' +
        (Get-Date -Format 'yyyy-MM-dd') + "_present-cadence-$Build.txt")
}

$required = @(
    'ndsBattlePlayableFrameCompleteMarker',
    'scVSBattleStartBattle',
    'gNdsBattlePlayablePacingVBlanks',
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsBattlePlayablePacingTimerTicks',
    'gNdsBattlePlayablePacingPresentIntervalBucket',
    'gNdsBattlePlayablePacingPresentIntervalMax',
    'gNdsBattlePlayablePacingPresentIntervalMin',
    'gNdsBattlePlayablePacingCadenceViolationCount'
)
if (($flips.Count -gt 0) -or ($RouteInitial -ge 0)) {
    $required += $RouteGlobal
}
if (-not [string]::IsNullOrWhiteSpace($EndBreak)) {
    $required += $EndBreak
}
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Present-cadence probe symbols absent from {0}: {1}" -f $elf,
        ($missing -join ', '))
}
# Engagement columns are arm-specific. Print them when the ELF has them and
# print literal zeros when it does not, so the row shape never changes.
$engage = @('gNdsR2CameraFixedLookAtCalls', 'gNdsR2CameraFixedFloatLookAtCalls',
            'gNdsR2CameraFixedPerspCalls', 'gNdsR2CameraFixedFloatPerspCalls')
$engage_args = ($engage | ForEach-Object {
    if ($symbols -contains $_) { $_ } else { '0' }
}) -join ', '
$route_arg = if ($symbols -contains $RouteGlobal) { $RouteGlobal } else { '0' }
$extra = @()
if (-not [string]::IsNullOrWhiteSpace($ExtraGlobals)) {
    $extra = @($ExtraGlobals -split '[,; ]+' |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $absent = @($extra | Where-Object { $symbols -notcontains $_ })
    if ($absent.Count -gt 0) {
        throw ("ExtraGlobals absent from {0}: {1}" -f $elf, ($absent -join ', '))
    }
}
$extra_fmt = ''
$extra_args = ''
for ($i = 0; $i -lt $extra.Count; $i++) {
    $extra_fmt += (' x{0}=%u' -f $i)
    $extra_args += (', ' + $extra[$i])
}

$bucket_fmt = 'PCADHIST %s n=%d 2=%u 3=%u 4=%u 5=%u max=%u min=%u viol=%u ' +
    'vbl=%u pres=%u tk=%u\n'
$bucket_args = '$n, ' +
    'gNdsBattlePlayablePacingPresentIntervalBucket[2], ' +
    'gNdsBattlePlayablePacingPresentIntervalBucket[3], ' +
    'gNdsBattlePlayablePacingPresentIntervalBucket[4], ' +
    'gNdsBattlePlayablePacingPresentIntervalBucket[5], ' +
    'gNdsBattlePlayablePacingPresentIntervalMax, ' +
    'gNdsBattlePlayablePacingPresentIntervalMin, ' +
    'gNdsBattlePlayablePacingCadenceViolationCount, ' +
    'gNdsBattlePlayablePacingVBlanks, ' +
    'gNdsBattlePlayablePacingPresentedFrames, ' +
    'gNdsBattlePlayablePacingTimerTicks'

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.present-cadence.stdout.log'
$stderr = Join-Path $log_dir 'melonds.present-cadence.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = [System.Collections.Generic.List[string]]::new()
    $commands.Add('set pagination off')
    $commands.Add('set confirm off')
    $commands.Add('set remotetimeout 30')
    # Incremental logging is not optional: a whole-match per-frame run is long
    # enough to be killed or time out, and gdb's redirected stdout is buffered
    # by the process and DISCARDED by a forced terminate.
    $commands.Add(("set logging file {0}" -f ($Artifact -replace '\\', '/')))
    $commands.Add('set logging overwrite on')
    $commands.Add('set logging enabled on')
    $commands.Add(("target remote 127.0.0.1:{0}" -f $context.GdbPort))
    $commands.Add('set $n = 0')
    $commands.Add(('printf "PCAD build=' + $Build + ' route=' + $RouteGlobal +
        ' initial=' + $RouteInitial + ' perframe=' +
        ([int]$PerFrame.IsPresent) + ' flips=' + (($flips | ForEach-Object {
            [string]$_ }) -join '/') + '\n"'))
    $commands.Add('tbreak scVSBattleStartBattle')
    $commands.Add('continue')
    $commands.Add('break ndsBattlePlayableFrameCompleteMarker')
    $commands.Add('commands')
    $commands.Add('silent')
    $commands.Add('set $n = $n + 1')
    if ($RouteInitial -ge 0) {
        $commands.Add('if $n == 1')
        $commands.Add(('set var {0} = {1}' -f $RouteGlobal, $RouteInitial))
        $commands.Add(('printf "PCADFLIP %d -> ' + $RouteInitial + '\n", $n'))
        $commands.Add(('printf "' + $bucket_fmt + '", "INIT", ' + $bucket_args))
        $commands.Add('end')
    }
    if ($PerFrame.IsPresent) {
        $commands.Add('printf "PCADF %d vbl=%u pres=%u tk=%u rt=%u ' +
            'fx=%u fl=%u px=%u pl=%u' + $extra_fmt + '\n", $n, ' +
            'gNdsBattlePlayablePacingVBlanks, ' +
            'gNdsBattlePlayablePacingPresentedFrames, ' +
            'gNdsBattlePlayablePacingTimerTicks, ' + $route_arg + ', ' +
            $engage_args + $extra_args)
    }
    foreach ($flip in $flips) {
        $commands.Add(('if $n == {0}' -f $flip))
        $commands.Add(('printf "' + $bucket_fmt + '", "PREFLIP", ' + $bucket_args))
        $commands.Add(('set var {0} = ({0} == 0) ? 1 : 0' -f $RouteGlobal))
        $commands.Add(('printf "PCADFLIP %d -> %u\n", $n, {0}' -f $RouteGlobal))
        $commands.Add('end')
    }
    # The summary is printed from INSIDE the breakpoint's command block, not
    # after the loop, so a run that is killed or times out still carries it.
    $commands.Add('if $n == ' + $Hits)
    $commands.Add(('printf "' + $bucket_fmt + '", "FINAL", ' + $bucket_args))
    $commands.Add('end')
    $commands.Add('if $n < ' + $Hits)
    $commands.Add('continue')
    $commands.Add('end')
    $commands.Add('end')
    $commands.Add('continue')
    if (-not [string]::IsNullOrWhiteSpace($EndBreak)) {
        $commands.Add('delete breakpoints')
        $commands.Add(('tbreak {0}' -f $EndBreak))
        $commands.Add('continue')
    }
    $commands.Add(('printf "' + $bucket_fmt + '", "DONE", ' + $bucket_args))
    $commands.Add('detach')
    $commands.Add('quit')

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
        -ScriptName 'present_cadence_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # `set logging` above wrote $Artifact through, so it is authoritative even
    # for a killed run. The helper's own .out replaces it ONLY when larger.
    $captured = Join-Path $log_temp 'present_cadence_probe.gdb.out'
    if ((Test-Path -LiteralPath $captured) -and
        ((-not (Test-Path -LiteralPath $Artifact)) -or
         ((Get-Item -LiteralPath $captured).Length -gt
          (Get-Item -LiteralPath $Artifact).Length))) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
    }
    if (Test-Path -LiteralPath $Artifact) {
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match 'PCADHIST|PCADFLIP' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
