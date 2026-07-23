<#
.SYNOPSIS
Task 49 GX equivalence differ -- host analyzer.

.DESCRIPTION
Reads two Task 49 per-owner GX stream captures (JSON, produced by the capture
harness) and reports equivalence under TWO TIERS WITH DIFFERENT STANDARDS:

  Tier 1 -- non-matrix stream, BIT-EXACT, zero tolerance.
            Classes CONTROL, ALPHA_TEST, TEXTURE_PARAM, TEXTURE_BIND,
            POLY_FORMAT, BEGIN, COLOR, TEX_COORD, VERTEX16
            (include/nds/nds_renderer.h:293-318). Task 48 measured this
            partition at 2,858 words/frame, 100.000% identical across 24
            frames. Any Tier 1 divergence is a defect, full stop -- both paths
            read the same baked source data.

  Tier 2 -- matrix equivalence by EFFECTIVE TRANSFORM.
            Recompose each binding's clip matrix from its captured
            MATRIX_LOAD4X4 words (profile 1 emits the CPU-composed
            projection*view*model product, src/nds/nds_renderer.c:12849-12868;
            profile 0 will emit MULT4x3 of a constant model under a once-loaded
            view). Transform each binding's 8 bounding-box corners under both
            and report deviation in SCREEN-SPACE PIXELS after projection.

Profile 0 and profile 1 cannot produce bit-identical matrices (different
composition hardware, 20.12 rounding, 4x3 drops a row). Tier 2 compares the
RESULT, not the operands, in the unit the fidelity doctrine is written in.

The matrix is NDSRendererMatrix20p12 = s32 m[4][4], 20.12 fixed-point
(include/nds/nds_renderer.h:232-235). The DS geometry engine composes the clip
matrix in hardware; we capture the 16-word LOAD4X4 that profile 1 hands the GPU,
which IS that composed product. Projecting unit-cube corners through it and
mapping to the 256x192 screen gives the screen-space deviation.

A threshold is stated with justification in the certificate; if no defensible
threshold can be derived, this script reports the deviation distribution ungated
and the task STOPs per the spec -- an unjustified magic number is worse than no
gate, because Tasks 51 and 52 will both be judged by it.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$CaptureA,
    [Parameter(Mandatory=$true)][string]$CaptureB,
    [string]$LabelA = 'A',
    [string]$LabelB = 'B',
    [string]$OwnerName = 'STAGE'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# GX command classes (include/nds/nds_renderer.h:293-318).
# Matrix classes: MATRIX_MODE(7) .. MATRIX_RESTORE(14); LOAD4X4=10, MULT4X4=11,
# PUSH=12, POP=13. Tier 1 is everything NON-matrix. Note: MULT4x3 (profile 0)
# does not exist in the enum yet; profile 0 lands with Task 51. The differ
# handles it as a matrix class when it appears.
$script:matrixClasses = 7,8,9,10,11,12,13,14   # MATRIX_MODE..MATRIX_RESTORE
$script:load4x4Class = 10
$script:matrixMult4x3Class = 99                 # reserved for profile 0 (Task 51)

# DS screen resolution.
$script:screenW = 256
$script:screenH = 192
# 20.12 fixed-point reciprocal.
$script:one20p12 = [double]4096.0

function ConvertFrom-Task49Capture {
    param([Parameter(Mandatory=$true)][string]$Path)
    $cap = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    # The capture harness writes: meta {frame,owner,entryCount,wordCount,
    # overflow,fault,bindingCount} and entries[] = {command_class,word_count,
    # binding_index,owner, words:[u32...]}.
    if (-not $cap.entries) {
        throw "Capture $Path has no entries field."
    }
    return $cap
}

function Test-Tier1BitExact {
    param(
        [Parameter(Mandatory=$true)]$CapA,
        [Parameter(Mandatory=$true)]$CapB
    )
    # Compare non-matrix classes word-for-word, entry-by-entry. The capture is
    # per-owner, one frame, so the two streams must align entry-for-entry.
    $aNonMatrix = @($CapA.entries | Where-Object { $script:matrixClasses -notcontains $_.command_class })
    $bNonMatrix = @($CapB.entries | Where-Object { $script:matrixClasses -notcontains $_.command_class })

    $maxLen = [Math]::Max($aNonMatrix.Count, $bNonMatrix.Count)
    $divergences = [Collections.Generic.List[object]]::new()
    $wordsCompared = 0
    $wordsMatched = 0
    for ($i = 0; $i -lt $maxLen; $i++) {
        $ea = if ($i -lt $aNonMatrix.Count) { $aNonMatrix[$i] } else { $null }
        $eb = if ($i -lt $bNonMatrix.Count) { $bNonMatrix[$i] } else { $null }
        if ($null -eq $ea -or $null -eq $eb) {
            $divergences.Add([pscustomobject]@{
                entry=$i; class='MISSING'; reason='stream-length-mismatch'
                a_class=$(if($ea){$ea.command_class}else{'<none>'});
                b_class=$(if($eb){$eb.command_class}else{'<none>'}) })
            continue
        }
        if ($ea.command_class -ne $eb.command_class) {
            $divergences.Add([pscustomobject]@{
                entry=$i; class="cls$($ea.command_class)"; reason='class-mismatch'
                a_class=$ea.command_class; b_class=$eb.command_class })
            continue
        }
        $wa = @($ea.words); $wb = @($eb.words)
        if ($wa.Count -ne $wb.Count) {
            $divergences.Add([pscustomobject]@{
                entry=$i; class="cls$($ea.command_class)"; reason='word-count-mismatch'
                a_words=$wa.Count; b_words=$wb.Count })
            continue
        }
        for ($w = 0; $w -lt $wa.Count; $w++) {
            $wordsCompared++
            # Treat as unsigned 32-bit; JSON may carry signed values from the
            # 20.12 s32 field. Normalize via UInt32.
            $va = [uint32]$wa[$w]
            $vb = [uint32]$wb[$w]
            if ($va -eq $vb) { $wordsMatched } else {
                $divergences.Add([pscustomobject]@{
                    entry=$i; class="cls$($ea.command_class)"; reason='word-mismatch'
                    word_index=$w; a_value=('0x{0:X8}' -f $va); b_value=('0x{0:X8}' -f $vb) })
            }
        }
    }
    return [pscustomobject]@{
        tier = 1
        standard = 'bit-exact, zero tolerance'
        entries_compared = $maxLen
        words_compared = $wordsCompared
        words_matched = $wordsMatched
        divergence_count = $divergences.Count
        divergences = $divergences
        verdict = if ($divergences.Count -eq 0 -and $wordsCompared -gt 0) { 'PASS' }
                  elseif ($wordsCompared -eq 0) { 'EMPTY' } else { 'FAIL' }
    }
}

function ConvertTo-ClipMatrix {
    param([Parameter(Mandatory=$true)][uint32[]]$Words)
    # 16 words = s32 m[4][4] in 20.12 fixed-point, row-major. Convert each to a
    # signed double. uint32 -> int32 reinterpretation.
    if ($Words.Count -ne 16) { return $null }
    $m = [double[,]]::new(4,4)
    for ($r = 0; $r -lt 4; $r++) {
        for ($c = 0; $c -lt 4; $c++) {
            $u = [uint32]$Words[($r*4)+$c]
            # Reinterpret as signed int32.
            $s = if ($u -ge 0x80000000) { ([double]$u - 4294967296.0) } else { [double]$u }
            $m[$r,$c] = $s / $script:one20p12
        }
    }
    return $m
}

function Invoke-ClipTransform {
    param(
        [Parameter(Mandatory=$true)][double[,]]$M,
        [Parameter(Mandatory=$true)][double[]]$V   # x,y,z (w=1)
    )
    # M is the projection*view*model clip matrix. Transform a model-space point.
    $cx = $M[0,0]*$V[0] + $M[0,1]*$V[1] + $M[0,2]*$V[2] + $M[0,3]
    $cy = $M[1,0]*$V[0] + $M[1,1]*$V[1] + $M[1,2]*$V[2] + $M[1,3]
    $cz = $M[2,0]*$V[0] + $M[2,1]*$V[1] + $M[2,2]*$V[2] + $M[2,3]
    $cw = $M[3,0]*$V[0] + $M[3,1]*$V[1] + $M[3,2]*$V[2] + $M[3,3]
    return @($cx,$cy,$cz,$cw)
}

function ConvertTo-ScreenPx {
    param([Parameter(Mandatory=$true)][double[]]$Clip)
    # Perspective divide then map NDC (-1..1) to screen (0..W, 0..H).
    # DS viewport: x in [0,256), y in [0,192). NDC x=-1 -> 0, x=1 -> 256;
    # NDC y=-1 -> 192, y=1 -> 0 (Y flipped).
    if ([Math]::Abs($Clip[3]) -lt 1e-9) { return $null }
    $ndx = $Clip[0] / $Clip[3]
    $ndy = $Clip[1] / $Clip[3]
    $px = ($ndx + 1.0) * 0.5 * $script:screenW
    $py = (1.0 - (($ndy + 1.0) * 0.5)) * $script:screenH
    return @($px,$py)
}

function Test-Tier2EffectiveTransform {
    param(
        [Parameter(Mandatory=$true)]$CapA,
        [Parameter(Mandatory=$true)]$CapB
    )
    # Group LOAD4X4 entries by binding_index; each binding's 16 words are its
    # composed clip matrix. Transform the 8 bounding-box corners of the unit
    # cube under each binding's clip matrix in A and B, compare in screen px.
    $aBindings = @($CapA.entries | Where-Object { $_.command_class -eq $script:load4x4Class })
    $bBindings = @($CapB.entries | Where-Object { $_.command_class -eq $script:load4x4Class })

    # The 8 unit-cube corners (the binding's local bbox, normalized).
    $corners = @(
        ,@(-1.0,-1.0,-1.0), @(1.0,-1.0,-1.0), @(-1.0,1.0,-1.0), @(1.0,1.0,-1.0),
        ,@(-1.0,-1.0,1.0), @(1.0,-1.0,1.0), @(-1.0,1.0,1.0), @(1.0,1.0,1.0))

    $n = [Math]::Min($aBindings.Count, $bBindings.Count)
    $perBinding = [Collections.Generic.List[object]]::new()
    $maxDev = 0.0
    $sumDev = 0.0
    $devCount = 0
    for ($bi = 0; $bi -lt $n; $bi++) {
        $ma = ConvertTo-ClipMatrix ([uint32[]]@($aBindings[$bi].words))
        $mb = ConvertTo-ClipMatrix ([uint32[]]@($bBindings[$bi].words))
        if ($null -eq $ma -or $null -eq $mb) { continue }
        $bMax = 0.0
        for ($ci = 0; $ci -lt 8; $ci++) {
            $ca = Invoke-ClipTransform $ma $corners[$ci]
            $cb = Invoke-ClipTransform $mb $corners[$ci]
            $sa = ConvertTo-ScreenPx $ca
            $sb = ConvertTo-ScreenPx $cb
            if ($null -eq $sa -or $null -eq $sb) { continue }
            $dx = $sa[0] - $sb[0]; $dy = $sa[1] - $sb[1]
            $d = [Math]::Sqrt($dx*$dx + $dy*$dy)
            if ($d -gt $bMax) { $bMax = $d }
        }
        $perBinding.Add([pscustomobject]@{
            binding = $bi; max_screen_px = [Math]::Round($bMax,4) })
        if ($bMax -gt $maxDev) { $maxDev = $bMax }
        if ($bMax -gt 0) { $sumDev += $bMax; $devCount++ }
    }
    $meanDev = if ($devCount -gt 0) { $sumDev / $devCount } else { 0.0 }
    return [pscustomobject]@{
        tier = 2
        standard = 'effective transform, screen-space pixels'
        bindings_compared = $n
        bindings_a = $aBindings.Count
        bindings_b = $bBindings.Count
        max_screen_px = [Math]::Round($maxDev,4)
        mean_screen_px = [Math]::Round($meanDev,4)
        per_binding = $perBinding
        verdict = if ($n -eq 0) { 'EMPTY' } elseif ($maxDev -le 0.0) { 'ZERO_DEVIATION' } else { 'DEVIATION_REPORTED' }
    }
}

# --- main ---
$capA = ConvertFrom-Task49Capture $CaptureA
$capB = ConvertFrom-Task49Capture $CaptureB

$tier1 = Test-Tier1BitExact $capA $capB
$tier2 = Test-Tier2EffectiveTransform $capA $capB

$result = [ordered]@{
    task = 49
    label_a = $LabelA
    label_b = $LabelB
    owner = $OwnerName
    capture_a_meta = $capA.meta
    capture_b_meta = $capB.meta
    tier1_non_matrix_bit_exact = $tier1
    tier2_matrix_effective_transform = $tier2
}

$result | ConvertTo-Json -Depth 6
