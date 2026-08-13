[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4613,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 1200,
    # Bucket samples taken while BOTH fireballs are live and rolling, starting
    # $Fb2Delay presented frames after the second wpMarioFireballMakeWeapon.
    # Fireball lifetime is 140 ticks; with the second throw ~30 frames after
    # the first, both are alive for ~110 frames, so a 64-sample window at
    # +45..+109 keeps every sample inside the two-live span.
    #
    # Samples come from the ROM's own 128-entry sBattleTickHudRing in one dump
    # at the window END (plus a head stop that anchors the baseline), not from
    # per-frame GDB stops -- per-frame stops raced the frame marker on the
    # quad arm and dropped a frame (see the ring note in Run-Arm).
    [ValidateRange(16, 128)][int]$Samples = 64,
    [int]$Fb2Delay = 45,
    # 2 = drive two fireballs via the playback pads and sample while both are
    # live (the cliff state). 0 = no input at all, sampling the SAME window
    # frames the 2-fireball runs land on, so the pair differs ONLY in the
    # fireballs. The 2-fireball runs make the second ball at frame ~45, so
    # the 0-fireball window starts at ~90, matching fb2 + Fb2Delay.
    [ValidateSet(0, 2)][int]$Fireballs = 2,
    [int]$ZeroFireballStartFrame = 90,
    [string]$JsonOut = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

# MUST match enum NDSTickHudBucket in include/nds/nds_startup.h, in order and
# in count (see the same warning at the top of sample-tick-hud-buckets.ps1).
$bucketNames = @('ALL', 'FTR', 'STG', 'BG', 'AUD', 'HUD', 'SRC', 'MISC', 'OTHR',
                 'WAIT', 'WORK', 'SHDT', 'SWRM',
                 'GCRA', 'SCPU', 'SCAT', 'SPRM',
                 'SINT', 'SPHD', 'SPHC')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

function Get-ArmElfSymbolAddress {
    param([string]$Elf, [string]$Name)
    $line = @(& $script:nm -a $Elf) |
        Where-Object { $_ -match "^([0-9a-fA-F]+)\s+\S\s+$([regex]::Escape($Name))$" } |
        Select-Object -First 1
    if (-not $line) { throw "ELF symbol not found in $([System.IO.Path]::GetFileName($Elf)): $Name" }
    $match = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($match.Groups[1].Value, 16))
}

function Run-Arm {
    param(
        [string]$ArmName,
        [string]$Build,
        [string]$Rom,
        [string]$Elf,
        # Cumulative counters read at each ring stop (window head and window
        # end); the delta across the stops is the window total. They differ per
        # arm: the quad counters only exist in the candidate ELF. Control reads
        # the generic fireball census instead. Same shape as
        # sample-tick-hud-buckets.ps1 -PerStopGlobals.
        [string[]]$PerStopGlobals,
        # Engagement counters read ONCE at the end of the run.
        [string[]]$EndGlobals
    )

    if (-not (Test-Path -LiteralPath $Rom -PathType Leaf) -or
        -not (Test-Path -LiteralPath $Elf -PathType Leaf)) {
        throw "$ArmName arm build did not produce the expected ROM and ELF."
    }
    $context = Initialize-MelonDSVerifierContext `
        -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
        -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
        -NoBuild:$NoBuild
    $temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
    $gdbScript = Join-Path $temp "fireball-quad-$ArmName.gdb"
    $gdbOut = Join-Path $temp "fireball-quad-$ArmName.gdb.out"
    $gdbErr = Join-Path $temp "fireball-quad-$ArmName.gdb.err"
    $emulatorOut = Join-Path $temp "fireball-quad-$ArmName.melonds.out"
    $emulatorErr = Join-Path $temp "fireball-quad-$ArmName.melonds.err"
    $configState = $null
    $emulator = $null

    try {
        if (-not (Test-Path -LiteralPath $Gdb)) { throw "GDB executable not found: $Gdb" }
        if (-not (Test-Path -LiteralPath $MelonDS)) {
            throw "melonDS executable not found: $MelonDS"
        }
        $padsAddress = Get-ArmElfSymbolAddress $Elf 'sControllerPlaybackPads'
        $connectedAddress = Get-ArmElfSymbolAddress $Elf 'sControllerPlaybackConnectedMask'
        $enabledAddress = Get-ArmElfSymbolAddress $Elf 'sControllerPlaybackEnabled'

        $configState = Enable-MelonDSGdbConfig `
            -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
            -Arm7Port $context.Arm7Port `
            -Persistent:([bool]$context.PersistentConfig) -MuteAudio
        Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
            -Force -ErrorAction SilentlyContinue
        $emulator = Start-Process -FilePath $context.MelonDSPath `
            -ArgumentList $rom `
            -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
            -RedirectStandardOutput $emulatorOut `
            -RedirectStandardError $emulatorErr `
            -WindowStyle Hidden -PassThru
        Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

        # Two fireballs, both forced by the playback pads. First throw: B held
        # until ftMarioSpecialNSetStatus enters (exactly the fireball verifier's
        # proven sequence), released, weapon made. Second throw: B pulsed every
        # 15 frames from +25 so a press edge lands whenever Mario is back in
        # Wait -- holding B across the status would consume the edge and the
        # second tbreak would never fire, hanging the run to its timeout. The
        # second status-entry stop releases and arms the make-weapon stop.
        $throwLines = if ($Fireballs -eq 2) {
            @(
                ('set {{unsigned short}}0x{0:x8} = 0x4000' -f $padsAddress),
                ('set {{unsigned int}}0x{0:x8} = 3' -f $connectedAddress),
                ('set {{unsigned int}}0x{0:x8} = 1' -f $enabledAddress),
                'tbreak ftmariospecialn.c:ftMarioSpecialNSetStatus',
                'continue',
                ('set {{unsigned short}}0x{0:x8} = 0' -f $padsAddress),
                'tbreak battleship_mario_fireball.c:wpMarioFireballMakeWeapon',
                'continue',
                'set $fb1 = gNdsBattlePlayablePacingPresentedFrames',
                # Pulse B every 15 frames from fb1+25 until the status re-enters.
                ('break ndsBattlePlayableFrameCompleteMarker if ' +
                    '(gNdsBattlePlayablePacingPresentedFrames >= ($fb1 + 25)) && ' +
                    '((gNdsBattlePlayablePacingPresentedFrames - $fb1) % 15 == 0)'),
                'commands',
                'silent',
                ('set {{unsigned short}}0x{0:x8} = 0x4000' -f $padsAddress),
                'continue',
                'end',
                ('break ndsBattlePlayableFrameCompleteMarker if ' +
                    '(gNdsBattlePlayablePacingPresentedFrames >= ($fb1 + 25)) && ' +
                    '((gNdsBattlePlayablePacingPresentedFrames - $fb1) % 15 == 2)'),
                'commands',
                'silent',
                ('set {{unsigned short}}0x{0:x8} = 0' -f $padsAddress),
                'continue',
                'end',
                'tbreak ftmariospecialn.c:ftMarioSpecialNSetStatus',
                'continue',
                ('set {{unsigned short}}0x{0:x8} = 0' -f $padsAddress),
                'delete',
                'tbreak battleship_mario_fireball.c:wpMarioFireballMakeWeapon',
                'continue',
                'set $fb2 = gNdsBattlePlayablePacingPresentedFrames',
                ("set `$sample_start = (`$fb2 + $Fb2Delay)")
            )
        } else {
            @(
                # No input at all; anchor the sample window at the fixed frame
                # the 2-fireball runs reach (fb2 ~45 + Fb2Delay 45 = 90).
                'set $fb1 = 0',
                'set $fb2 = 0',
                ("set `$sample_start = $ZeroFireballStartFrame")
            )
        }
        $ringWindow = 128
        $ringBytes = $bucketNames.Count * $ringWindow * 4
        $ringPath = Join-Path $temp "fireball-quad-$ArmName-ring.bin"
        $ringStopEnd = "`$sample_start + $Samples"
        $gdbLines = @(
            'set pagination off',
            'set confirm off',
            'set remotetimeout 30',
            "target remote 127.0.0.1:$($context.GdbPort)",
            # Experiment arm: the whole point of this A/B is the fireball draw
            # path, and a live Fox adds SRC/CPU noise neither arm needs.
            'tbreak scVSBattleStartBattle',
            'continue',
            'set gNdsBattlePlayableFoxCpuEnabled = 0',
            $throwLines,
            'printf "FIREBALL_FRAMES=%u,%u\n", $fb1, $fb2',
            # Both fireballs live and rolling: sample buckets from the ROM's own
            # 128-entry ring (sBattleTickHudRing), one stop at the window head
            # (baseline for the cumulative split counters) and one at the end.
            # Per-frame GDB stops raced the marker on the quad arm and dropped
            # a frame -- the ring is indexed by finalized iteration instead, so
            # it is not exposed to whatever makes the presented-frame counter
            # disagree with the marker (sample-tick-hud-buckets.ps1 documented
            # the same failure and the same fix).
            'break ndsBattlePlayableFrameCompleteMarker',
            'commands',
            'silent',
            "if gNdsBattlePlayablePacingPresentedFrames < `$sample_start",
            'continue',
            'end',
            'end',
            'continue',
            'printf "TICKRINGHEAD=%u,%u,%u\n", ' +
                'sBattleTickHudRingHead, sBattleTickHudRingCount, ' +
                'gNdsBattlePlayablePacingPresentedFrames',
            $(if ($PerStopGlobals.Count -ne 0) {
                "printf `"TICKSTOP0=$((, '%u' * $PerStopGlobals.Count) -join ',')\n`", " +
                    ($PerStopGlobals -join ', ')
            }),
            'delete',
            'break ndsBattlePlayableFrameCompleteMarker',
            'commands',
            'silent',
            "if gNdsBattlePlayablePacingPresentedFrames < $ringStopEnd",
            'continue',
            'end',
            'end',
            'continue',
            'printf "TICKRINGEND=%u,%u,%u\n", ' +
                'sBattleTickHudRingHead, sBattleTickHudRingCount, ' +
                'gNdsBattlePlayablePacingPresentedFrames',
            $(if ($PerStopGlobals.Count -ne 0) {
                "printf `"TICKSTOP1=$((, '%u' * $PerStopGlobals.Count) -join ',')\n`", " +
                    ($PerStopGlobals -join ', ')
            }),
            # dump binary memory splits its arguments on whitespace, so the
            # bounds have to be single tokens -- index one past the last bucket
            # row for the end bound (same form as the Task 34 stage-stream
            # dump and sample-tick-hud-buckets.ps1).
            ("dump binary memory $ringPath " +
                "&sBattleTickHudRing[0][0] " +
                "&sBattleTickHudRing[$($bucketNames.Count)][0]"),
            'delete',
            ('printf "TICKVBI=%u,%u,%u,%u,%u\n", ' +
                'gNdsBattlePlayablePacingPresentIntervalBucket[2], ' +
                'gNdsBattlePlayablePacingPresentIntervalBucket[3], ' +
                'gNdsBattlePlayablePacingPresentIntervalBucket[4], ' +
                'gNdsBattlePlayablePacingPresentIntervalBucket[5], ' +
                'gNdsBattlePlayablePacingPresentIntervalMax'),
            'printf "TICKSLIP=%u\n", gNdsBattlePlayablePacingCadenceViolationCount',
            $(if ($EndGlobals.Count -ne 0) {
                "printf `"TICKEND=$((, '%u' * $EndGlobals.Count) -join ',')\n`", " +
                    ($EndGlobals -join ', ')
            }),
            'detach'
        ) | Where-Object { -not [string]::IsNullOrEmpty($_) }
        [System.IO.File]::WriteAllLines($gdbScript,
            @($gdbLines | ForEach-Object { $_ }))

        $gdbProcess = Start-Process -FilePath $Gdb `
            -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
            -WorkingDirectory $root `
            -RedirectStandardOutput $gdbOut `
            -RedirectStandardError $gdbErr `
            -WindowStyle Hidden -PassThru
        if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
            Stop-Process -Id $gdbProcess.Id -Force
            throw ("$ArmName arm GDB run exceeded ${TimeoutSeconds}s. The fireball " +
                'sequence or sample window did not complete. GDB stderr: ' +
                (Get-Content $gdbErr -Raw -ErrorAction SilentlyContinue))
        }
        if ($gdbProcess.ExitCode -ne 0) {
            throw "$ArmName arm GDB run failed: $(Get-Content $gdbErr -Raw)"
        }
        $output = Get-Content $gdbOut -Raw

        $fbMatch = [regex]::Match($output, 'FIREBALL_FRAMES=([0-9]+),([0-9]+)')
        if (-not $fbMatch.Success) {
            throw ("$ArmName arm never recorded the two fireball make frames. " +
                "GDB output:`n$output")
        }
        $fb1 = [uint64]$fbMatch.Groups[1].Value
        $fb2 = [uint64]$fbMatch.Groups[2].Value
        # Ring stitch. The ROM records every finalized iteration into
        # sBattleTickHudRing (ndsPlatformTickHudSample), so the buckets come
        # from ONE dump at the window end, not from per-frame stops that raced
        # the marker. The ring is indexed by iteration, and a presented frame
        # can carry TWO iterations, so iterations >= presented frames ALWAYS;
        # that inequality is what turns a silent wrap into a hard failure.
        $headMatch = [regex]::Match($output,
            'TICKRINGHEAD=([0-9]+),([0-9]+),([0-9]+)')
        $endMatch = [regex]::Match($output,
            'TICKRINGEND=([0-9]+),([0-9]+),([0-9]+)')
        if (-not $headMatch.Success -or -not $endMatch.Success) {
            throw ("$ArmName arm produced no ring stop lines. GDB output:`n$output")
        }
        $headFrame = [uint64]$headMatch.Groups[3].Value
        $endHead = [int]$endMatch.Groups[1].Value
        $endCount = [int]$endMatch.Groups[2].Value
        $endFrame = [uint64]$endMatch.Groups[3].Value
        $presentedDelta = [int]($endFrame - $headFrame)
        if ($presentedDelta -gt $ringWindow) {
            throw ("$ArmName arm advanced $presentedDelta presented frames " +
                "(frames $headFrame..$endFrame), more than the $ringWindow-entry " +
                'ring can hold, so frames were overwritten before they were ' +
                'read. Lower -Samples.')
        }
        if (-not (Test-Path -LiteralPath $ringPath -PathType Leaf)) {
            throw "$ArmName arm ring dump wrote no file at $ringPath."
        }
        $raw = [System.IO.File]::ReadAllBytes($ringPath)
        if ($raw.Length -ne $ringBytes) {
            throw ("$ArmName arm ring dump is $($raw.Length) bytes, expected " +
                "$ringBytes ($($bucketNames.Count) buckets x $ringWindow).")
        }
        if ($endCount -lt $Samples) {
            throw ("$ArmName arm ring holds $endCount iterations, fewer than the " +
                "$Samples requested. Lower -Samples or start the window later.")
        }
        # Oldest-first walk of the newest $Samples slots, ending at the head.
        # The window is the newest slots because the dump was taken at the
        # window END; the head stop only anchored the frame span.
        $start = (($endHead - $Samples) % $ringWindow)
        if ($start -lt 0) { $start += $ringWindow }
        $rows = @(for ($j = 0; $j -lt $Samples; $j++) {
            $slot = ($start + $j) % $ringWindow
            $row = New-Object 'System.Collections.Generic.List[uint64]'
            $row.Add([uint64]($endFrame - ($Samples - 1 - $j)))
            for ($b = 0; $b -lt $bucketNames.Count; $b++) {
                $row.Add([BitConverter]::ToUInt32($raw,
                    ((($b * $ringWindow) + $slot) * 4)))
            }
            , [uint64[]]$row.ToArray()
        })
        $stop0Values = @()
        $stop1Values = @()
        if ($PerStopGlobals.Count -ne 0) {
            $s0 = [regex]::Match($output,
                "TICKSTOP0=([0-9]+(?:,[0-9]+){$($PerStopGlobals.Count - 1)})")
            $s1 = [regex]::Match($output,
                "TICKSTOP1=([0-9]+(?:,[0-9]+){$($PerStopGlobals.Count - 1)})")
            if (-not $s0.Success -or -not $s1.Success) {
                throw ("$ArmName arm produced no TICKSTOP baseline lines. " +
                    "GDB output:`n$output")
            }
            $stop0Values = @($s0.Groups[1].Value -split ',' | ForEach-Object {
                [uint64]$_ })
            $stop1Values = @($s1.Groups[1].Value -split ',' | ForEach-Object {
                [uint64]$_ })
        }
        $vbiMatch = [regex]::Match($output, 'TICKVBI=([0-9]+(?:,[0-9]+){4})')
        $vbi = if ($vbiMatch.Success) {
            [uint64[]]($vbiMatch.Groups[1].Value -split ',')
        } else { @(0, 0, 0, 0, 0) }
        $slipMatch = [regex]::Match($output, 'TICKSLIP=([0-9]+)')
        $endValues = if ($EndGlobals.Count -ne 0) {
            $endMatch = [regex]::Match($output,
                "TICKEND=([0-9]+(?:,[0-9]+){$($EndGlobals.Count - 1)})")
            if (-not $endMatch.Success) {
                throw ("$ArmName arm produced no TICKEND line. GDB output:`n$output")
            }
            [uint64[]]($endMatch.Groups[1].Value -split ',')
        } else { @() }

        $stats = @(0..($bucketNames.Count - 1) | ForEach-Object {
            $bucket = $_
            $values = @($rows | ForEach-Object { $_[$bucket + 1] })
            $sorted = @($values | Sort-Object)
            [PSCustomObject]@{
                bucket = $bucketNames[$bucket]
                p50 = [uint64]$sorted[[int][Math]::Floor(($sorted.Count - 1) * 0.50)]
                p95 = [uint64]$sorted[[int][Math]::Floor(($sorted.Count - 1) * 0.95)]
                min = [uint64]($values | Measure-Object -Minimum).Minimum
                max = [uint64]($values | Measure-Object -Maximum).Maximum
            }
        })
        $workIndex = [array]::IndexOf($bucketNames, 'WORK') + 1
        $hudIndex = [array]::IndexOf($bucketNames, 'HUD') + 1
        $workNoHud = @($rows | ForEach-Object {
            [uint64]$_[$workIndex] - [uint64]$_[$hudIndex] })
        $sortedWorkNoHud = @($workNoHud | Sort-Object)
        $stats += [PSCustomObject]@{
            bucket = 'WORK-H'
            p50 = [uint64]$sortedWorkNoHud[
                [int][Math]::Floor(($sortedWorkNoHud.Count - 1) * 0.50)]
            p95 = [uint64]$sortedWorkNoHud[
                [int][Math]::Floor(($sortedWorkNoHud.Count - 1) * 0.95)]
            min = [uint64]($workNoHud | Measure-Object -Minimum).Minimum
            max = [uint64]($workNoHud | Measure-Object -Maximum).Maximum
        }
        # MISC-split counters are CUMULATIVE; the delta between the head stop
        # (baseline) and the end stop is the cost of the window.
        $splitDelta = @(foreach ($g in 0..($PerStopGlobals.Count - 1)) {
            [PSCustomObject]@{
                name = $PerStopGlobals[$g]
                first = $stop0Values[$g]
                last = $stop1Values[$g]
                delta = $stop1Values[$g] - $stop0Values[$g]
            }
        })

        return [PSCustomObject]@{
            arm = $ArmName
            rom = $Rom
            fb1 = $fb1
            fb2 = $fb2
            stats = $stats
            vbi = $vbi
            slip = if ($slipMatch.Success) { [uint64]$slipMatch.Groups[1].Value }
                   else { [uint64]0 }
            endGlobals = $endValues
            splitDelta = $splitDelta
            rows = $rows
        }
    } finally {
        if ($null -ne $emulator) {
            $emulator.Refresh()
            if (-not $emulator.HasExited) {
                Stop-Process -Id $emulator.Id -Force
                $emulator.WaitForExit()
            }
        }
        Restore-MelonDSGdbConfig -State $configState
        Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
            -Force -ErrorAction SilentlyContinue
    }
}

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$script:nm = Join-Path $env:DEVKITARM 'bin\arm-none-eabi-nm.exe'

$controlBuild = 'build-fireball-ab-control'
$quadBuild = 'build-fireball-ab-quad'
if (-not $NoBuild) {
    # One build at a time: the asset generators share paths outside $(BUILD).
    make -C $root "TARGET=$target" "BUILD=$controlBuild" NDS_R2_FIREBALL_QUAD=0
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    make -C $root "TARGET=$target" "BUILD=$quadBuild" NDS_R2_FIREBALL_QUAD=1
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$controlRom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $controlBuild -Extension '.nds'
$controlElf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $controlBuild -Extension '.elf'
$quadRom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $quadBuild -Extension '.nds'
$quadElf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $quadBuild -Extension '.elf'

$control = Run-Arm `
    -ArmName 'control' -Build $controlBuild `
    -Rom $controlRom -Elf $controlElf `
    -PerStopGlobals @('gNdsMiscWeaponDrawTicks', 'gNdsMiscEffectDrawTicks',
                      'gNdsWeaponRendererFireballSubmitCount') `
    -EndGlobals @('gNdsWeaponRendererFireballSubmitCount',
                  'gNdsWeaponRendererFireballTriangleCount')
$quad = Run-Arm `
    -ArmName 'quad' -Build $quadBuild `
    -Rom $quadRom -Elf $quadElf `
    -PerStopGlobals @('gNdsMiscWeaponDrawTicks', 'gNdsMiscEffectDrawTicks',
                      'gNdsFireballQuadDrawCount', 'gNdsFireballQuadFallbackCount') `
    -EndGlobals @('gNdsFireballQuadDrawCount', 'gNdsFireballQuadFallbackCount',
                  'gNdsWeaponRendererFireballSubmitCount')

$stateLabel = if ($Fireballs -eq 2) { 'two fireballs live' } else { 'no fireballs' }
$rows = @('arm,fb1,fb2,' + ($bucketNames -join ',') + ',WORK-H' +
          $(if ($control.splitDelta.Count -gt $quad.splitDelta.Count) {
                ',weaponTicks,effectTicks,fireballDraws'
            } else { ',weaponTicks,effectTicks,quadDraws,quadFallbacks' }),
          ('control,' + (($control.fb1, $control.fb2) -join ',') + ',' +
           (($control.stats | ForEach-Object { $_.p50 }) -join ',') + ',' +
           (($control.stats | Select-Object -Last 1).p50)),
          ('quad,' + (($quad.fb1, $quad.fb2) -join ',') + ',' +
           (($quad.stats | ForEach-Object { $_.p50 }) -join ',') + ',' +
           (($quad.stats | Select-Object -Last 1).p50)))
Write-Host ''
Write-Host ("========== FIREBALL QUAD A/B ({0}, Fox CPU off) ==========" -f $stateLabel)
Write-Host ("window: control frames {0}..{1}  quad frames {2}..{3}" -f
    $control.rows[0][0], $control.rows[$control.rows.Count - 1][0],
    $quad.rows[0][0], $quad.rows[$quad.rows.Count - 1][0])
Write-Host ''
Write-Host ('{0,-7} {1,9} {2,9} {3,9} {4,9}' -f 'bucket', 'ctl P50', 'quad P50',
    'ctl P95', 'quad P95')
foreach ($s in $control.stats) {
    $q = $quad.stats | Where-Object { $_.bucket -eq $s.bucket }
    if ($null -eq $q) { continue }
    Write-Host ('{0,-7} {1,9} {2,9} {3,9} {4,9}' -f $s.bucket, $s.p50, $q.p50,
        $s.p95, $q.p95)
}
Write-Host ''
Write-Host 'MISC-split deltas across the sample window (cumulative counters, ring stops):'
foreach ($d in $control.splitDelta) {
    $qd = $quad.splitDelta | Where-Object { $_.name -eq $d.name }
    Write-Host ('{0,-40} control {1,9}  quad {2,9}' -f $d.name, $d.delta,
        $(if ($null -ne $qd) { $qd.delta } else { 'n/a' }))
}
Write-Host ''
Write-Host ('VBI 2/3/4/5+max: control {0}/{1}/{2}/{3}/{4}  quad {5}/{6}/{7}/{8}/{9}' -f
    $control.vbi[0], $control.vbi[1], $control.vbi[2], $control.vbi[3],
    $control.vbi[4], $quad.vbi[0], $quad.vbi[1], $quad.vbi[2], $quad.vbi[3],
    $quad.vbi[4])
Write-Host ('cadence slips: control {0}  quad {1}' -f $control.slip, $quad.slip)
Write-Host ('engagement: control generic fireball submits={0}/{1}tris  quad draws={2} fallbacks={3} generic-submits={4}' -f
    $control.endGlobals[0], $control.endGlobals[1],
    $quad.endGlobals[0], $quad.endGlobals[1], $quad.endGlobals[2])
Write-Host '====================================================================='

if (-not [string]::IsNullOrWhiteSpace($JsonOut)) {
    [System.IO.File]::WriteAllText($JsonOut,
        ($control, $quad | ConvertTo-Json -Depth 6), [System.Text.Encoding]::UTF8)
    Write-Host "Wrote $JsonOut"
}
