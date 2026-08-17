param(
    [Parameter(Mandatory = $true)]
    [string[]]$Elf,
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [switch]$BenchmarkAblation,
    [ValidateRange(1, 32768)]
    [int]$MaxItcmBytes = 32768
)

$ErrorActionPreference = 'Stop'

# Task 82: ndsRendererHardwareConvertTexel01Ci4Direct was evicted from ITCM on
# the owner's decision -- it measures zero cycles while Dream Land water is
# frozen at source frame 0. Restore it here if a stage ships with live water.
#
# Campaign 01 re-knapsack, 2026-08-17. This guard used to pin the generic
# display-list interpreter and its submit chain into ITCM. That set was measured
# on a GX=1-era c200 capture; on the v4-c238 shipping census, masked by the
# gate's own rank-80 frames, it rents ITCM at 0.1-1.1 ticks/frame per byte
# against residents running 20-200:
#
#   ndsRendererScanList                       6,188 B    599 tk/fr   0.1/B
#   ndsRendererSubmitHardwareTriangle         3,212 B    291 tk/fr   0.1/B
#   ndsRendererHardwareSubmitVertex           2,256 B    775 tk/fr   0.3/B
#   ndsRendererHardwareLitShadeColorPrepared    460 B     61 tk/fr   0.1/B
#   ndsRendererMtxMulAffine20p12                616 B    676 tk/fr   1.1/B
#   ndsRendererLoadHardwareSplitMatrices        392 B      8 tk/fr   0.0/B
#
# So the guard now pins BOTH directions: the admitted pack must be resident, and
# the evicted set must not silently come back. The generic renderer is still
# required to be EMITTED -- it is the fallback path, it just lives in main RAM.
# artifacts/performance/2026-08-17_itcm-repack2/ carries the ranking.
$hotFunctions = @(
    'ndsRendererCommitNativeStageSegment',
    'ndsRendererNativeStageBeginRun',
    'ndsRendererLoadHardwareGxComposedMatrices',
    'ndsRendererNativeStageEmitNoZVertex',
    'ndsRendererHardwareEndBatch',
    'ndsRendererHardwareApplyTextureParams',
    'ndsRendererR2MaterialColor15',
    'ndsRendererNativeApplyProductionPreamble'
)
$evictedFunctions = @(
    'ndsRendererScanList',
    'ndsRendererSubmitHardwareTriangle',
    'ndsRendererHardwareSubmitVertex',
    'ndsRendererHardwareLitShadeColorPrepared',
    'ndsRendererMtxMulAffine20p12',
    'ndsRendererLoadHardwareSplitMatrices'
)
$requiredEmittedFunctions = if ($BenchmarkAblation) {
    @('ndsRendererSubmitHardwareTriangle', 'ndsRendererScanList')
} else {
    @(
        'ndsRendererHardwareSubmitVertex',
        'ndsRendererSubmitHardwareTriangle',
        'ndsRendererScanList'
    )
}
$nativeFighterFunctions = @(
    'ndsRendererNativeShadeProductionActions',
    'ndsRendererNativePrepareProductionRun',
    'ndsRendererExecuteNativeFighterOwnerProduction'
)

if (-not (Test-Path -LiteralPath $Objdump -PathType Leaf)) {
    throw "arm-none-eabi-objdump was not found at '$Objdump'."
}

foreach ($elfPath in $Elf) {
    $resolvedElf = (Resolve-Path -LiteralPath $elfPath).Path
    $elfName = Split-Path -Leaf $resolvedElf
    $requiresNativeFighter = $elfName -match
        '^smash64ds-battle-playable-(?:hwtri|coarse-hwtri)\.elf$'
    # The one-minute lifecycle target deliberately omits the native
    # stage/fighter routes. At O3 that leaves one caller chain, and GCC folds
    # SubmitVertex -> SubmitHardwareTriangle completely into the ITCM-resident
    # ScanList. No out-of-line symbol is the optimized result, not a placement
    # escape. Published/native targets retain the stricter emitted-symbol gate.
    $allowsInlineCollapsedSubmitChain =
        $elfName -eq 'smash64ds-battle-playable-one-minute-match-hwtri.elf'
    $requiredForElf = @($requiredEmittedFunctions)
    if ($allowsInlineCollapsedSubmitChain) {
        $requiredForElf = @($requiredForElf | Where-Object {
            $_ -notin @('ndsRendererHardwareSubmitVertex',
                        'ndsRendererSubmitHardwareTriangle')
        })
    }
    $sectionLines = @(& $Objdump -h $resolvedElf)
    if ($LASTEXITCODE -ne 0) {
        throw "objdump section listing failed for '$resolvedElf'."
    }
    $itcmSection = @($sectionLines | Where-Object {
        $_ -match '^\s*\d+\s+\.itcm\s+([0-9a-fA-F]+)\s+'
    })
    if ($itcmSection.Count -ne 1) {
        throw "Expected exactly one .itcm section in '$resolvedElf'."
    }
    $null = $itcmSection[0] -match '^\s*\d+\s+\.itcm\s+([0-9a-fA-F]+)\s+'
    $itcmBytes = [Convert]::ToUInt32($Matches[1], 16)
    if (($itcmBytes -eq 0) -or ($itcmBytes -gt $MaxItcmBytes)) {
        throw ".itcm size $itcmBytes is outside 1..$MaxItcmBytes bytes in '$resolvedElf'."
    }

    $symbolLines = @(& $Objdump -t $resolvedElf)
    if ($LASTEXITCODE -ne 0) {
        throw "objdump symbol listing failed for '$resolvedElf'."
    }
    $functionSymbols = [System.Collections.Generic.List[object]]::new()
    foreach ($line in $symbolLines) {
        if ($line -notmatch '^\s*([0-9a-fA-F]+)\s+\S+\s+F\s+(\S+)\s+([0-9a-fA-F]+)\s+(\S+)\s*$') {
            continue
        }
        $functionSymbols.Add([PSCustomObject]@{
            Address = [Convert]::ToUInt32($Matches[1], 16)
            Section = $Matches[2]
            Bytes = [Convert]::ToUInt32($Matches[3], 16)
            Name = $Matches[4]
        }) | Out-Null
    }

    [uint32]$rendererItcmBytes = 0
    $emittedNames = [System.Collections.Generic.List[string]]::new()
    $inlineCollapsedNames = [System.Collections.Generic.List[string]]::new()
    $functionsToCheck = @($hotFunctions)
    if ($requiresNativeFighter) {
        $functionsToCheck += $nativeFighterFunctions
    }
    foreach ($baseName in $functionsToCheck) {
        $matches = @($functionSymbols | Where-Object {
            ($_.Name -eq $baseName) -or $_.Name.StartsWith("$baseName.")
        })
        if ($requiresNativeFighter -and
            ($nativeFighterFunctions -contains $baseName) -and
            ($matches.Count -eq 0)) {
            throw "Required hot renderer function '$baseName' was not emitted in '$resolvedElf'."
        }
        foreach ($symbol in $matches) {
            if ($symbol.Section -ne '.itcm') {
                throw "Hot renderer symbol '$($symbol.Name)' escaped to '$($symbol.Section)' in '$resolvedElf'."
            }
            $rendererItcmBytes += $symbol.Bytes
            $emittedNames.Add("$($symbol.Name)=$($symbol.Bytes)") | Out-Null
        }
    }

    # The other half of the 2026-08-17 re-knapsack: the generic display-list
    # renderer must still EXIST and must NOT be back in ITCM. A re-admission
    # here silently costs the admitted pack the bytes it was measured on.
    [uint32]$evictedBytes = 0
    $evictedSeen = [System.Collections.Generic.List[string]]::new()
    foreach ($baseName in $evictedFunctions) {
        $matches = @($functionSymbols | Where-Object {
            ($_.Name -eq $baseName) -or $_.Name.StartsWith("$baseName.")
        })
        if (($requiredForElf -contains $baseName) -and ($matches.Count -eq 0)) {
            throw "Required hot renderer function '$baseName' was not emitted in '$resolvedElf'."
        }
        if ($allowsInlineCollapsedSubmitChain -and
            ($matches.Count -eq 0) -and
            ($baseName -in @('ndsRendererHardwareSubmitVertex',
                             'ndsRendererSubmitHardwareTriangle'))) {
            $inlineCollapsedNames.Add($baseName) | Out-Null
        }
        foreach ($symbol in $matches) {
            if ($symbol.Section -eq '.itcm') {
                throw "Evicted renderer symbol '$($symbol.Name)' returned to ITCM in '$resolvedElf'."
            }
            $evictedBytes += $symbol.Bytes
            $evictedSeen.Add("$($symbol.Name)=$($symbol.Section)") | Out-Null
        }
    }

    Write-Output ("Renderer ITCM placement passed: elf={0} itcm={1}/{2} renderer={3} symbols=[{4}] inlineCollapsed=[{5}] evicted={6} [{7}]" -f
        $elfName, $itcmBytes, $MaxItcmBytes, $rendererItcmBytes,
        ($emittedNames -join ', '), ($inlineCollapsedNames -join ', '),
        $evictedBytes, ($evictedSeen -join ', '))
}
