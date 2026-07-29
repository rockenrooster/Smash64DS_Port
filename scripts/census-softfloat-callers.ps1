[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [string]$Addr2Line = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-addr2line.exe',
    [string]$Nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe',
    [int]$GdbPort = 4618,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task81-recensus',
    [ValidateRange(1,1000000)][int]$StartFrame = 439,
    [ValidateRange(10,600)][int]$SampleSeconds = 90,
    [string[]]$Helpers = @('__aeabi_fadd', '__aeabi_fmul'),
    [string]$JsonOut = ''
)

# Task 92 E0. Attributes the soft-float class to its callers.
#
# Task 81's partition found soft-float is 191,810 ticks/frame at 19.1% stall and
# 155,151 instructions -- the only instruction-bound class in the frame, and the
# second largest. It shrinks only by executing fewer float operations, which
# means knowing who executes them. The per-PC profiler cannot say: a leaf helper
# is charged to itself, never to its caller.
#
# Method, and the reason it works now when Task 84's attempt did not: GDB's
# `break <symbol>` lands PAST the prologue, by which point lr has often been
# spilled or clobbered, and Task 84's memcpy attribution resolved 82% of its
# samples into BSS data objects -- addresses that cannot be return addresses.
# `break *<exact address>` from nm stops on the first instruction, where lr is
# still the caller's return address. Task 85 proved this out.
#
# R2-03 E60 CORRECTS THE READING OF THIS CENSUS. The gameplay side is not
# collision: `gcPlayDObjAnimJoint` alone is 58% of the class and the whole
# collision family is under 4,000 ticks/frame. Animation inclusive is 146,942
# ticks/frame, 15.2% of WORK, larger than the gap to the gate. Read the gate
# column as "which contract governs a change", not as "this cannot be changed" --
# PROJECT_GOAL.md requires mechanical equivalence, NOT bit-exactness, and lists
# precomputed animation data and fixed-point replacement as allowed techniques.
# The state-hash freeze is a verifier's assertion, not the product contract, and
# a load-time pose table computed with the same float arithmetic is bit-exact
# anyway. See docs/optimization/ClaudeOpus5_R203_E60_AnimationIsTheGate_20260729.md.
#
# The split that matters is gameplay versus renderer. Float in gmcollision, mp*,
# ftMain* and ftComputer is frozen by the Task 9 state hash and by
# PROJECT_GOAL.md's mechanical-equivalence contract, so it cannot be converted
# to fixed point whatever it costs. Float in the renderer gates on the fidelity
# budget instead, where a fixed-point equivalent is the owner's call. Task 50
# already closed the divide/sqrt sites on exactly this reasoning; this run asks
# the same question of add and multiply, which Task 50 did not cover.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

# The gate, decided by CALLER NAME rather than by source path. addr2line
# resolves the function reliably but the source column comes back as a bare
# drive letter for the imported and decomp translation units, so a path-prefix
# classifier reports ~96% unresolved. Names are what this codebase actually
# keys its gates on anyway.
#
# GAMEPLAY is frozen by the Task 9 state hash and PROJECT_GOAL.md's mechanical
# equivalence contract -- float there cannot become fixed point whatever it
# costs. Animation joints belong here: Task 77 E1 measured the cosmetic-only
# joint set as EMPTY for both Mario and Fox, so no joint is safe to approximate.
# RENDERER gates on the fidelity budget instead, where a fixed-point equivalent
# is the owner's call.
$gameplayCallers = @(
    'gcPlayDObjAnimJoint', 'battleship_ftAnimParseDObjFigatree',
    'ndsBaseGcPlayMObjMatAnim', 'ndsBaseGcPlayAnimAll', 'gcParseDObjAnimJoint',
    'gcParseMObjMatAnimJoint', 'ndsStageMPSegmentIntersection2D',
    'ndsMPFloorSegmentCrossesDownwardKernel', 'ftComputerCheckDetectTarget',
    'mpProcessUpdateMain', 'gmCameraUpdateInterests', 'ftMainProcPhysicsMap',
    'mpCommonUpdateFighterSlopeContour'
)
$rendererCallers = @(
    'syMatrixLookAtReflectF', 'guMtxCatF', 'syMatrixF2L', 'syMatrixPerspFastF',
    'ndsRendererHardwarePrepareLitDirection', 'ndsRendererTransformVertex20p12',
    'ndsFighterDisplayContractProjectTarget',
    'ndsRendererAdapterBuildDObjLocalMatrix',
    'ndsRendererAdapterBuildFighterTraRotRpyExact'
)
# A float helper calling another float helper is not a lever -- its cost belongs
# to whoever called IT, and double-counting it would inflate both.
$secondOrderCallers = @(
    '__kernel_cosf', '__kernel_sinf', '__ieee754_sqrtf', 'cosf', 'sinf',
    'sqrtf', '__aeabi_fdiv'
)

function Get-Gate {
    param([string]$Source, [string]$Function)
    if ($gameplayCallers -contains $Function) { return 'GAMEPLAY (state-hash frozen)' }
    if ($rendererCallers -contains $Function) { return 'RENDERER (fidelity-gated)' }
    if ($secondOrderCallers -contains $Function) { return 'second-order (float calling float)' }
    if ($Function -match '^(gc|ft|gm|mp|wp|lb)' ) { return 'GAMEPLAY (state-hash frozen)' }
    if ($Function -match '^(ndsRenderer|ndsFighter|ndsStage|sy|gu)') { return 'RENDERER (fidelity-gated)' }
    return 'UNRESOLVED'
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'softfloat.gdb'
$gdbOut = Join-Path $temp 'softfloat.gdb.out'
$gdbErr = Join-Path $temp 'softfloat.gdb.err'
$emulatorOut = Join-Path $temp 'softfloat.melonds.out'
$emulatorErr = Join-Path $temp 'softfloat.melonds.err'
$configState = $null
$emulator = $null
$gdbProcess = $null

try {
    foreach ($path in @($rom, $elf, $Gdb, $Addr2Line, $Nm)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required soft-float census file is missing: $path"
        }
    }

    # Exact entry addresses. `break <symbol>` would land past the prologue and
    # lr would no longer hold the caller -- that is precisely how Task 84's
    # memcpy attribution produced 82% impossible answers.
    $symbols = & $Nm $elf
    $entries = @{}
    foreach ($helper in $Helpers) {
        $line = $symbols | Where-Object { $_ -match "^([0-9a-f]{8}) [Tt] $([regex]::Escape($helper))$" } |
            Select-Object -First 1
        if (-not $line) { throw "Could not find $helper in $elf" }
        $entries[$helper] = ($line -split ' ')[0]
    }
    Write-Host "Entry addresses:"
    foreach ($k in $entries.Keys) { Write-Host ("  {0,-18} 0x{1}" -f $k, $entries[$k]) }

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
        # Settle into battle first; boot and title would otherwise dominate.
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        'end',
        'continue',
        'delete breakpoints'
    )
    foreach ($helper in $Helpers) {
        $gdbLines += @(
            "break *0x$($entries[$helper])",
            'commands',
            'silent',
            ("printf `"LR,$helper,%x\n`", `$lr"),
            'continue',
            'end'
        )
    }
    $gdbLines += 'continue'

    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -PassThru
    # The helper breakpoints never stop firing, so the run is bounded by wall
    # clock and the samples are whatever contiguous window that yields. A
    # contiguous window starting at a settled battle frame is unbiased for the
    # gameplay/renderer split, which is the only question being asked.
    Write-Host "Sampling for ${SampleSeconds}s ..."
    if (-not $gdbProcess.WaitForExit($SampleSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        $gdbProcess.WaitForExit()
    }

    $raw = @(Get-Content $gdbOut -ErrorAction SilentlyContinue |
        Where-Object { $_ -match '^LR,' })
    if ($raw.Count -eq 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Select-Object -First 20 | Write-Host
        throw "Soft-float census collected no samples."
    }

    $counts = @{}
    foreach ($line in $raw) {
        $p = $line -split ','
        # Clear the Thumb bit: a bx lr return address carries it and it is not
        # part of the code address.
        $addr = [Convert]::ToUInt32($p[2].Trim(), 16) -band 0xfffffffe
        $key = "$($p[1])|$addr"
        if ($counts.ContainsKey($key)) { $counts[$key]++ } else { $counts[$key] = 1 }
    }

    $unique = @($counts.Keys | ForEach-Object { [uint32](($_ -split '\|')[1]) } |
        Sort-Object -Unique)
    $stdin = ($unique | ForEach-Object { '0x{0:x8}' -f $_ }) -join "`n"
    $resolved = @($stdin | & $Addr2Line -f -e $elf)
    $map = @{}
    for ($i = 0; $i -lt $unique.Count; $i++) {
        $fn = $resolved[2 * $i]
        $loc = $resolved[(2 * $i) + 1]
        $map[$unique[$i]] = @{
            fn = if ($fn -and $fn -ne '??') { $fn } else { ('pc_{0:x8}' -f $unique[$i]) }
            src = if ($loc) { ($loc -split ':')[0] } else { '??' }
        }
    }

    $rows = @()
    foreach ($key in $counts.Keys) {
        $parts = $key -split '\|'
        $addr = [uint32]$parts[1]
        $info = $map[$addr]
        $rows += [pscustomobject]@{
            Helper = $parts[0]
            Caller = $info.fn
            Source = $info.src
            Gate   = Get-Gate -Source $info.src -Function $info.fn
            Count  = $counts[$key]
        }
    }

    $total = ($rows | Measure-Object -Property Count -Sum).Sum
    # R2-03 E68. -Helpers is a parameter, so the class being attributed is
    # whatever the caller asked for. Every heading and closing note used to say
    # "soft-float" and "the fadd/fmul class" unconditionally, so an E68 run
    # attributing memset/memcpy printed a report that named the wrong class in
    # three places -- exactly the kind of label a later reader cites verbatim.
    $classLabel = ($Helpers -join ' + ')
    Write-Host ""
    Write-Host ("Helper caller attribution [$classLabel] -- $total samples from frame $StartFrame")
    Write-Host ""
    Write-Host "  count    share  gate                          caller"
    Write-Host "-------  -------  ----------------------------  ------------------------------"
    foreach ($r in ($rows | Sort-Object -Property Count -Descending | Select-Object -First 25)) {
        Write-Host ("{0,7}  {1,6:P1}  {2,-28}  {3}" -f `
            $r.Count, ($r.Count / $total), $r.Gate, $r.Caller)
    }
    Write-Host ""
    Write-Host "gate                          samples    share"
    Write-Host "----------------------------  -------  -------"
    $byGate = $rows | Group-Object -Property Gate |
        ForEach-Object {
            [pscustomobject]@{
                Gate = $_.Name
                Count = ($_.Group | Measure-Object -Property Count -Sum).Sum
            }
        } | Sort-Object -Property Count -Descending
    foreach ($g in $byGate) {
        Write-Host ("{0,-28}  {1,7}  {2,6:P1}" -f $g.Gate, $g.Count, ($g.Count / $total))
    }
    Write-Host ""
    $renderer = ($byGate | Where-Object { $_.Gate -like 'RENDERER*' } |
        Measure-Object -Property Count -Sum).Sum
    if ($null -eq $renderer) { $renderer = 0 }
    # R2-03 E60. This census measures SHARES; it cannot measure the class total,
    # so the absolute scale has to come from a profile. It used to multiply by a
    # hardcoded 191,810 from the Task 81 partition and present the product as a
    # measurement -- by 2026-07-29 that constant was 84% high, because R2-04 E5
    # and R2-03 E46 had graduated since and __aeabi_fadd + __aeabi_fmul had
    # fallen to 104,222. Report the share and name the scale's provenance;
    # multiply against a profile taken on the SAME build instead.
    Write-Host ("renderer-side share: {0:P1} of the $classLabel class." -f `
        ($renderer / $total))
    Write-Host ("For absolute ticks multiply by $classLabel from a profile of")
    Write-Host ("THIS build (run-task37-profile-census.ps1). Reference points, each")
    Write-Host ("valid only for the build that measured it: fadd+fmul was 104,222")
    Write-Host ("ticks/frame at R2-03 E60, and memset+memcpy 58,700 at E68. Do not")
    Write-Host ("reuse either after anything graduates.")
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            task = "Helper caller attribution [$classLabel]"
            helperClass = $classLabel
            target = $target
            rom = $rom
            romSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
            startFrame = $StartFrame
            helpers = $Helpers
            entryAddresses = $entries
            samples = $total
            # Deliberately NOT a class total. This census measures shares only,
            # and the key that used to live here (a hardcoded 191,810 from the
            # Task 81 partition) was 84% stale by the time anyone noticed. A
            # consumer must scale by a profile of the same build; there is no
            # number here to reuse by accident.
            classTotalTicksPerFrame = $null
            rendererShare = ($renderer / $total)
            capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
            byGate = @($byGate)
            rows = @($rows | Sort-Object -Property Count -Descending)
        }
        $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut }
                    else { Join-Path $root $JsonOut }
        $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
        Write-Host "Wrote $JsonOut"
    }
} finally {
    if ($null -ne $gdbProcess) {
        $gdbProcess.Refresh()
        if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }
    }
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
