[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4615,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-vsresults-blit',
    [switch]$NoBuild,
    # R0's expensive window is results tics 131..181, where an iteration costs
    # 39 VBlanks. Skip to its start rather than measuring the cheap early tics:
    # cost climbs by a factor of twenty across the scene, so where the window
    # sits changes the answer more than anything this script measures.
    [ValidateRange(1,4000)][int]$SkipIterations = 130,
    [ValidateRange(1,400)][int]$Iterations = 40,
    [ValidateRange(60,3600)][int]$TimeoutSeconds = 1800,
    # Also break on the staging-layer boundary, so the two-layer 320x240
    # pipeline stops being charged to whichever blit happens to precede it.
    # R0e made this necessary: once the wallpaper's pixel loop dropped to 9
    # instructions per pixel, its interval was still 5.15 VBlanks, and the loop
    # can only account for about a seventh of that. Off by default so a run
    # stays comparable with the R0/R0c/R0d/R0e artifacts.
    [switch]$Phases,
    [string]$JsonOut = ''
)

# R2-07 R0c. Splits scene_draw's 19.55M ticks/iteration across the individual
# ndsDrawSObjIntoPreview calls, which R0a said to do before specializing the
# per-pixel format dispatch: 8.3 calls/frame at 2.34M average may be one
# dominant full-screen wallpaper rather than eight comparable sprites, and that
# changes which arm of the blitter is worth touching.
#
# Cost unit is VBlanks, not ticks, and deliberately so. The Results loop carries
# no instrument at all -- no ndsPlatformBeginFrame, no HUD, no pacing, and it
# never increments gNdsBattlePlayablePacingPresentedFrames -- so there is no tick
# accumulator to read. GDB stops freeze the emulator, and sVBlankCount freezes
# with it, so the delta in sVBlankCount between consecutive breakpoint hits is
# exactly the guest time the previous call consumed, independent of host speed.
# That is the same property R0 used to turn phase boundaries into a timeline.
#
# Reading sobj->sprite rather than the function's locals keeps this robust under
# -Os: width/height/nbitmaps are copied into locals at :1536-1538, but the
# struct fields they come from are live at function entry and cannot be
# reordered away.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'vsresults-blit.gdb'
$gdbOut = Join-Path $temp 'vsresults-blit.gdb.out'
$gdbErr = Join-Path $temp 'vsresults-blit.gdb.err'
$emulatorOut = Join-Path $temp 'vsresults-blit.melonds.out'
$emulatorErr = Join-Path $temp 'vsresults-blit.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required VS Results blit census file is missing: $path"
        }
    }

    # Fail before launching, not thirty minutes in. A `break` on a name GDB
    # cannot resolve is a warning, not an error: the script would emulate the
    # whole one-minute match, reach the Results scene, and then report "No BLIT
    # records" as if the finding were about the runtime. Two full runs were lost
    # to invented global names before `sample-tick-hud-buckets.ps1` got the same
    # pre-flight; this is that check, applied to breakpoint symbols.
    $breakSymbols = @('ndsMNVSResultsRecordFrame', 'ndsDrawSObjIntoPreview')
    if ($Phases) {
        $breakSymbols += @('ndsPlatformBeginOriginalSpritePreview',
                           'ndsPlatformCommitOriginalSpritePreviewLayer',
                           'ndsPlatformReadInput')
    }
    $nm = Join-Path (Split-Path -Parent $Gdb) 'arm-none-eabi-nm.exe'
    if (Test-Path -LiteralPath $nm -PathType Leaf) {
        $defined = [System.Collections.Generic.HashSet[string]]::new(
            [string[]](& $nm --defined-only $elf |
                ForEach-Object { ($_ -split '\s+')[-1] }))
        $absent = @($breakSymbols | Where-Object { -not $defined.Contains($_) })
        if ($absent.Count -ne 0) {
            throw ("These breakpoint symbols are not in $elf, so the census " +
                   "would run to completion and report nothing: " +
                   ($absent -join ', '))
        }
    }

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

    # Breakpoint 1 gates on Results iterations and is then deleted; 2 logs every
    # blit and continues; 3 sits on the same iteration boundary as 1 but carries
    # no commands, so when its ignore count expires GDB stops for good and the
    # script falls through to detach. 2 and 3 are at different addresses, so
    # 2's `continue` cannot swallow 3's stop.
    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 60',
        'set height 0',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsMNVSResultsRecordFrame',
        "ignore 1 $SkipIterations",
        'continue',
        'delete 1',
        'printf "WINDOWSTART=%u\n", sVBlankCount',
        'break ndsDrawSObjIntoPreview',
        'commands',
        'silent',
        ('printf "BLIT=%u,%d,%d,%d,%d,%d,%d\n", sVBlankCount, ' +
            'sobj->sprite.bmfmt, sobj->sprite.bmsiz, ' +
            '(int)(unsigned short)sobj->sprite.width, ' +
            '(int)(unsigned short)sobj->sprite.height, ' +
            '(int)(unsigned short)sobj->sprite.nbitmaps, ' +
            'sobj->sprite.attr'),
        'continue',
        'end',
        'break ndsMNVSResultsRecordFrame',
        "ignore 3 $Iterations"
    )
    if ($Phases) {
        # Breakpoints 4 and 5. They must be created before the `continue` that
        # runs the window, and after breakpoint 3 so its ignore count still
        # refers to the right number.
        $gdbLines += @(
            'break ndsPlatformBeginOriginalSpritePreview',
            'commands',
            'silent',
            'printf "PHASE=%u,begin\n", sVBlankCount',
            'continue',
            'end',
            'break ndsPlatformCommitOriginalSpritePreviewLayer',
            'commands',
            'silent',
            'printf "PHASE=%u,commit\n", sVBlankCount',
            'continue',
            'end',
            # The frame boundary, so the commit interval stops absorbing
            # everything else the scene does -- including the two 3D fighter
            # GObjs, which draw at display link 9, ahead of the wallpaper's 26.
            #
            # `ndsPlatformReadInput`, NOT a second breakpoint on
            # `ndsMNVSResultsRecordFrame`. Two breakpoints at one address are not
            # independent: this one's `continue` swallows breakpoint 3's stop, so
            # the window never closes. R0g lost a run that way -- it ran to the
            # 1800 s timeout, 6,169 iterations instead of 40, straight past the
            # Results screen into sprites no other arm ever saw. Same trap the
            # comment above breakpoint 3 already described, re-learned the hard
            # way: every logging breakpoint must sit at its OWN address.
            'break ndsPlatformReadInput',
            'commands',
            'silent',
            'printf "PHASE=%u,frame\n", sVBlankCount',
            'continue',
            'end'
        )
    }
    $gdbLines += @(
        'continue',
        'printf "WINDOWEND=%u\n", sVBlankCount',
        'detach'
    )

    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        # Partial output is still usable -- the blit lines up to the kill are
        # real measurements -- so report rather than discarding them.
        Write-Warning "VS Results blit census exceeded ${TimeoutSeconds}s; using partial output."
    }

    $lines = @(Get-Content $gdbOut -ErrorAction SilentlyContinue)
    $fmtNames = @{ 0 = 'RGBA'; 1 = 'YUV'; 2 = 'CI'; 3 = 'IA'; 4 = 'I' }
    $sizNames = @{ 0 = '4b'; 1 = '8b'; 2 = '16b'; 3 = '32b' }
    # One ordered event stream, because GDB prints in hit order and an interval
    # ends at the next hit of ANY breakpoint. Without -Phases there are no PHASE
    # records and this reduces exactly to the original blit-to-blit deltas.
    $events = @()
    $blits = @()
    foreach ($line in ($lines | Where-Object { $_ -match '^(BLIT|PHASE)=' })) {
        if ($line -match '^PHASE=') {
            $p = ($line -replace '^PHASE=', '') -split ','
            if ($p.Count -lt 2) { continue }
            $events += [PSCustomObject]@{
                vblank = [uint32]$p[0]
                label = "(layer $($p[1]))"
                pixels = 0
                blit = $null
                vblanksCost = $null
            }
            continue
        }
        $f = ($line -replace '^BLIT=', '') -split ','
        if ($f.Count -lt 7) { continue }
        # PSCustomObject, not [ordered]@{}: `Measure-Object -Property x` cannot
        # read a key off a dictionary, so an ordered hashtable here silently
        # summed to $null and every share printed 0.0% while the per-record data
        # was correct. vblanksCost is declared up front so the later assignment
        # lands on an existing property.
        $blit = [PSCustomObject]@{
            vblank = [uint32]$f[0]
            bmfmt = [int]$f[1]
            bmsiz = [int]$f[2]
            width = [int]$f[3]
            height = [int]$f[4]
            nbitmaps = [int]$f[5]
            attr = [int]$f[6]
            pixels = ([int]$f[3]) * ([int]$f[4])
            vblanksCost = $null
        }
        $blits += $blit
        $events += [PSCustomObject]@{
            vblank = $blit.vblank
            label = "{0}/{1} {2}x{3}" -f `
                ($fmtNames[$blit.bmfmt] ?? $blit.bmfmt),
                ($sizNames[$blit.bmsiz] ?? $blit.bmsiz),
                $blit.width, $blit.height
            pixels = $blit.pixels
            blit = $blit
            vblanksCost = $null
        }
    }
    if ($blits.Count -eq 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw ("No BLIT records. Either the Results window was never reached " +
               "(raise -TimeoutSeconds, or lower -SkipIterations) or " +
               "ndsDrawSObjIntoPreview never ran there, which would itself be " +
               "the finding. GDB output head:`n" +
               (($lines | Select-Object -First 15) -join "`n"))
    }

    # The VBlank delta to the NEXT hit is what the work at this hit cost. The
    # last record has no successor, so it is reported but excluded from totals.
    for ($i = 0; $i -lt $events.Count - 1; $i++) {
        $cost = [int]($events[$i + 1].vblank - $events[$i].vblank)
        $events[$i].vblanksCost = $cost
        if ($null -ne $events[$i].blit) { $events[$i].blit.vblanksCost = $cost }
    }
    $costed = @($events | Where-Object { $null -ne $_.vblanksCost })
    $groups = $costed | Group-Object -Property label
    $totalCost = ($costed | Measure-Object -Property vblanksCost -Sum).Sum

    Write-Host ""
    Write-Host ("VS Results blit census -- {0} intervals with a cost, {1} VBlanks total" -f `
        $costed.Count, $totalCost)
    Write-Host ""
    Write-Host ("{0,-22} {1,6} {2,10} {3,9} {4,8} {5,7}" -f `
        'fmt/siz  dimensions', 'hits', 'VBlanks', 'share', 'px/call', 'VB/call')
    Write-Host ('-' * 68)
    foreach ($g in ($groups | Sort-Object { -(($_.Group | Measure-Object -Property vblanksCost -Sum).Sum) })) {
        $sum = ($g.Group | Measure-Object -Property vblanksCost -Sum).Sum
        Write-Host ("{0,-22} {1,6} {2,10} {3,8:P1} {4,8} {5,7:N1}" -f `
            $g.Name, $g.Count, $sum,
            ($(if ($totalCost -gt 0) { $sum / $totalCost } else { 0 })),
            $g.Group[0].pixels,
            ($sum / $g.Count))
    }
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'R2-07 - VS Results per-interval cost split'
            target = $target
            rom = $rom
            romSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
            skipIterations = $SkipIterations
            iterations = $Iterations
            phases = [bool]$Phases
            capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
            windowStart = (($lines | Where-Object { $_ -match '^WINDOWSTART=' }) -replace '\D', '')
            windowEnd = (($lines | Where-Object { $_ -match '^WINDOWEND=' }) -replace '\D', '')
            totalCostedVblanks = $totalCost
            blits = $blits
            intervals = @($costed | Select-Object vblank, label, pixels, vblanksCost)
        }
        $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut }
                    else { Join-Path $root $JsonOut }
        $payload | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath
        Write-Host "Wrote $JsonOut"
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
