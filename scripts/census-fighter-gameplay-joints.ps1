[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4614,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-fighter-joint-census',
    [switch]$NoBuild,
    [ValidateRange(1,1000000)][int]$StartFrame = 439,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# Task 77 E1. Reads the per-fighter joint bindings out of FTAttributes for both
# battle fighters and classifies every joint as gameplay-load-bearing or
# cosmetic, which is the flag COMPILER_FIRST_ARCHITECTURE.md makes mandatory in
# Fighter IR before the animation compiler may quantize anything.
#
# The runtime already holds fp->attr, so this needs no host-side asset parser --
# one stop on a settled battle frame is ground truth. That is deliberate: the
# same data exists in the o2r payloads, but re-deriving it host-side would mean
# a second struct-layout definition that can silently disagree with the one the
# game actually runs.
#
# Hurtboxes, foot joints and item joints are read directly. animlock is read as
# the two raw words and decoded host-side, because the encoding is only
# discoverable from its reader (ftParamSetAnimLocks,
# src/port/reloc_backend_compat_shims.c:7843) and belongs written down once:
#
#   joints run from nFTPartsJointCommonStart (4) to nFTPartsJointNumMax-1 (36)
#   bit 31 is consumed FIRST, shifting left each joint
#   flags0 serves i < ARRAY_COUNT(fp->joints)-1 (i.e. joints 4..35, 32 bits)
#   flags1 serves the remainder (joint 36, bit 31)
#
# An animation-locked joint is placed by code rather than by the animation, so
# it is gameplay-load-bearing by construction -- it is exactly the class the
# animation compiler must not quantize, because the animation is not what puts
# it where it is.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

# include/ft/fighter.h:163-170. Mirrored here only to decode the animlock words;
# the run also reports the runtime's own sizeof so a drift is caught rather than
# silently mis-decoded.
$jointCommonStart = 4
$jointNumMax = 37
$hurtboxCount = 11   # FTDAMAGECOLL_NUM_MAX
$effectJointCount = 5

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'fighter-joint-census.gdb'
$gdbOut = Join-Path $temp 'fighter-joint-census.gdb.out'
$gdbErr = Join-Path $temp 'fighter-joint-census.gdb.err'
$emulatorOut = Join-Path $temp 'fighter-joint-census.melonds.out'
$emulatorErr = Join-Path $temp 'fighter-joint-census.melonds.err'
$configState = $null
$emulator = $null

function New-FighterReadCommands {
    param([int]$Slot)

    $hurtFields = (0..($hurtboxCount - 1) | ForEach-Object {
        "`$ftst->attr->damage_coll_descs[$_].joint_id" }) -join ', '
    $hurtFormat = (, '%d' * $hurtboxCount) -join ','
    $effectFields = (0..($effectJointCount - 1) | ForEach-Object {
        "`$ftst->attr->effect_joint_ids[$_]" }) -join ', '
    $effectFormat = (, '%d' * $effectJointCount) -join ','

    @(
        "set `$gobj = (GObj *)gSCManagerBattleState->players[$Slot].fighter_gobj",
        'set $ftst = 0',
        'if $gobj != 0',
        'set $ftst = (FTStruct *)$gobj->user_data.p',
        'end',
        # Every pointer on this path is guarded: a NULL deref inside a -batch
        # script aborts the whole run, which would look like a harness failure
        # rather than the absent fighter it actually is.
        # $ftst, not $fp: $fp is GDB's ARM frame-pointer register and is not an
        # assignable convenience variable.
        'if ($ftst != 0) && ($ftst->attr != 0)',
        'set $al0 = 0',
        'set $al1 = 0',
        'set $stp0 = 0',
        'set $stp1 = 0',
        'if $ftst->attr->animlock != 0',
        'set $al0 = $ftst->attr->animlock[0]',
        'set $al1 = $ftst->attr->animlock[1]',
        'end',
        'if $ftst->attr->setup_parts != 0',
        'set $stp0 = $ftst->attr->setup_parts[0]',
        'set $stp1 = $ftst->attr->setup_parts[1]',
        'end',
        ("printf `"FTJOINT=$Slot,%d,%u,%u,%u,%u,%d,%d,%d,%d,%d\n`", " +
            '$ftst->fkind, $al0, $al1, $stp0, $stp1, ' +
            '$ftst->attr->joint_rfoot_id, $ftst->attr->joint_lfoot_id, ' +
            '$ftst->attr->joint_itemheavy_id, $ftst->attr->joint_itemlight_id, ' +
            '(int)(sizeof($ftst->joints) / sizeof($ftst->joints[0]))'),
        ("printf `"FTHURT=$Slot,$hurtFormat\n`", $hurtFields"),
        ("printf `"FTEFFECT=$Slot,$effectFormat\n`", $effectFields"),
        'else',
        "printf `"FTMISSING=$Slot\n`"",
        'end'
    )
}

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required fighter-joint census file is missing: $path"
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

    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        # One stop, on a settled combat frame -- both fighters exist and their
        # attributes are fully relocated by then. FTAttributes is immutable for
        # the match, so a later frame would read the same words.
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        'end',
        'continue'
    ) + (New-FighterReadCommands -Slot 0) +
        (New-FighterReadCommands -Slot 1) + @('detach')

    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "Fighter-joint census exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "Fighter-joint census GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $lines = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $fighters = @()
    foreach ($slot in 0, 1) {
        $jointLine = $lines | Where-Object { $_ -match "^FTJOINT=$slot," } | Select-Object -First 1
        if (-not $jointLine) {
            throw "Fighter slot $slot produced no FTJOINT record (missing or NULL attributes)."
        }
        $j = ($jointLine -replace '^FTJOINT=', '') -split ','
        $hurtLine = ($lines | Where-Object { $_ -match "^FTHURT=$slot," } | Select-Object -First 1)
        $effectLine = ($lines | Where-Object { $_ -match "^FTEFFECT=$slot," } | Select-Object -First 1)
        $hurt = (($hurtLine -replace '^FTHURT=', '') -split ',') | Select-Object -Skip 1 |
            ForEach-Object { [int]$_ }
        $effect = (($effectLine -replace '^FTEFFECT=', '') -split ',') | Select-Object -Skip 1 |
            ForEach-Object { [int]$_ }

        # $j[0] is the slot the printf leads with, so every payload field is
        # one past its printf position.
        $runtimeJointCount = [int]$j[10]
        if ($runtimeJointCount -ne $jointNumMax) {
            throw ("Joint array is $runtimeJointCount at runtime but this script " +
                   "decodes animlock against $jointNumMax; update both together.")
        }

        $animlock = @([uint32]$j[2], [uint32]$j[3])
        $setupParts = @([uint32]$j[4], [uint32]$j[5])

        # MSB-first, flags0 for joints below the last array slot, flags1 after.
        # setup_parts uses the same encoding, so one decoder serves both: it
        # marks the joints initialized on fighter creation, which is the closest
        # thing to "this joint exists for this fighter" the data offers, and
        # without it the unused tail reads as unclassified rather than absent.
        $decodeJointMask = {
            param($w0, $w1)
            $out = @()
            for ($i = $jointCommonStart; $i -lt $jointNumMax; $i++) {
                if ($i -lt ($jointNumMax - 1)) {
                    $bit = 31 - ($i - $jointCommonStart)
                    if ($bit -ge 0 -and (($w0 -shr $bit) -band 1) -ne 0) { $out += $i }
                } elseif ((($w1 -shr 31) -band 1) -ne 0) {
                    $out += $i
                }
            }
            , $out
        }
        $locked = & $decodeJointMask $animlock[0] $animlock[1]
        $present = & $decodeJointMask $setupParts[0] $setupParts[1]

        $gameplay = [System.Collections.Generic.SortedSet[int]]::new()
        foreach ($v in $hurt) { if ($v -ge 0) { [void]$gameplay.Add($v) } }
        foreach ($v in @([int]$j[6], [int]$j[7], [int]$j[8], [int]$j[9])) {
            if ($v -ge 0) { [void]$gameplay.Add($v) }
        }
        foreach ($v in $locked) { [void]$gameplay.Add($v) }

        $cosmeticOnly = [System.Collections.Generic.SortedSet[int]]::new()
        foreach ($v in $effect) {
            if ($v -ge 0 -and -not $gameplay.Contains($v)) { [void]$cosmeticOnly.Add($v) }
        }

        $fighters += [ordered]@{
            slot = $slot
            fkind = [int]$j[1]
            animlockWords = $animlock
            setupPartsWords = $setupParts
            animlockJoints = @($locked)
            presentJoints = @($present)
            unclassifiedPresentJoints = @($present | Where-Object { -not $gameplay.Contains($_) })
            hurtboxJoints = @($hurt)
            footJoints = @([int]$j[6], [int]$j[7])
            itemJoints = @([int]$j[8], [int]$j[9])
            effectJoints = @($effect)
            gameplayJoints = @($gameplay)
            cosmeticOnlyJoints = @($cosmeticOnly)
            runtimeJointCount = $runtimeJointCount
        }
    }

    Write-Host ""
    Write-Host ("slot fkind  gameplay joints (of {0}, indices {1}..{2})" -f `
        $jointNumMax, $jointCommonStart, ($jointNumMax - 1))
    Write-Host "---- -----  -----------------------------------------"
    foreach ($f in $fighters) {
        Write-Host ("{0,-4} {1,-6} {2}" -f $f.slot, $f.fkind,
            (($f.gameplayJoints) -join ' '))
        Write-Host ("       animlock  {0}" -f (($f.animlockJoints) -join ' '))
        Write-Host ("       hurtbox   {0}" -f (($f.hurtboxJoints) -join ' '))
        Write-Host ("       effect    {0}" -f (($f.effectJoints) -join ' '))
        Write-Host ("       cosmetic-only {0}" -f (($f.cosmeticOnlyJoints) -join ' '))
        Write-Host ("       present   {0} joints; unclassified {1}: {2}" -f `
            $f.presentJoints.Count, $f.unclassifiedPresentJoints.Count,
            (($f.unclassifiedPresentJoints) -join ' '))
    }
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'Task 77 E1 - fighter gameplay-joint classification'
            target = $target
            rom = $rom
            romSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
            startFrame = $StartFrame
            jointCommonStart = $jointCommonStart
            jointNumMax = $jointNumMax
            capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
            fighters = $fighters
        }
        $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut }
                    else { Join-Path $root $JsonOut }
        $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
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
