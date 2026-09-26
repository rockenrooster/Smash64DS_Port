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

            EFFECT is the one deliberate exception for VERTEX16 itself. M1
            moves projection of ImpactWave/DamageSlash vertices from ARM9 into
            GX, so route 0 writes already-projected 4.12 vertices while route 1
            writes object-space 4.12 vertices. EFFECT therefore grades every
            other non-matrix word at Tier 1 and moves VERTEX16 positions into
            Tier 2, where the active projection/modelview pair is applied.

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

# GX command classes (include/nds/nds_renderer.h:293-319).
# Matrix classes: MATRIX_MODE(7) .. MATRIX_RESTORE(14); LOAD4X4=9, MULT4X4=10,
# PUSH=11, POP=12, STORE=13, RESTORE=14. Tier 1 is everything NON-matrix.
# MULT4x3 (Task 51: 12-word model matrix) is appended at class 22 so the
# existing classes stay byte-stable; it is a matrix class for Tier-2 purposes.
$script:matrixClasses = 7,8,9,10,11,12,13,14,22   # MATRIX_MODE..MATRIX_RESTORE + MULT4x3
$script:load4x4Class = 9                          # NDS_TASK29_GX_MATRIX_LOAD4X4
$script:matrixMult4x3Class = 22                   # NDS_TASK29_GX_MATRIX_MULT4x3 (Task 51, appended)
$script:vertex16Class = 20                        # NDS_TASK29_GX_VERTEX16

# DS screen resolution.
$script:screenW = 256
$script:screenH = 192
# 20.12 fixed-point reciprocal.
$script:one20p12 = [double]4096.0
$script:one4p12 = [double]4096.0
$script:effectScreenThresholdPx = [double]1.0
$script:effectDepthThresholdNdc = [double](1.0 / 4096.0)

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
        [Parameter(Mandatory=$true)]$CapB,
        [switch]$ExcludeVertex16
    )
    # Compare non-matrix classes word-for-word, entry-by-entry. The capture is
    # per-owner, one frame, so the two streams must align entry-for-entry.
    $aNonMatrix = @($CapA.entries | Where-Object {
        -not ($script:matrixClasses -contains [int]$_.command_class) -and
        (-not $ExcludeVertex16 -or
         ([int]$_.command_class -ne $script:vertex16Class))
    })
    $bNonMatrix = @($CapB.entries | Where-Object {
        -not ($script:matrixClasses -contains [int]$_.command_class) -and
        (-not $ExcludeVertex16 -or
         ([int]$_.command_class -ne $script:vertex16Class))
    })

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
            if ($va -eq $vb) { $wordsMatched++ } else {
                $divergences.Add([pscustomobject]@{
                    entry=$i; class="cls$($ea.command_class)"; reason='word-mismatch'
                    word_index=$w; a_value=('0x{0:X8}' -f $va); b_value=('0x{0:X8}' -f $vb) })
            }
        }
    }
    return [pscustomobject]@{
        tier = 1
        standard = if ($ExcludeVertex16) {
            'bit-exact, zero tolerance; EFFECT VERTEX16 positions are Tier 2'
        } else {
            'bit-exact, zero tolerance'
        }
        entries_compared = $maxLen
        words_compared = $wordsCompared
        words_matched = $wordsMatched
        divergence_count = $divergences.Count
        divergences = $divergences
        verdict = if ($divergences.Count -eq 0 -and $wordsCompared -gt 0) { 'PASS' }
                  elseif ($wordsCompared -eq 0) { 'EMPTY' } else { 'FAIL' }
    }
}

function New-Task49IdentityMatrix {
    $m = [double[]]::new(16)
    $m[0] = 1.0
    $m[5] = 1.0
    $m[10] = 1.0
    $m[15] = 1.0
    , $m
}

function Invoke-Task49RowVectorTransform {
    param(
        [Parameter(Mandatory=$true)][double[]]$V,
        [Parameter(Mandatory=$true)][double[]]$M
    )
    $out = [double[]]::new(4)
    for ($col = 0; $col -lt 4; $col++) {
        $out[$col] =
            $V[0] * $M[$col] +
            $V[1] * $M[4 + $col] +
            $V[2] * $M[8 + $col] +
            $V[3] * $M[12 + $col]
    }
    , $out
}

function ConvertFrom-Task49Vertex16 {
    param([Parameter(Mandatory=$true)]$Words)
    $w = @($Words)
    if ($w.Count -ne 2) { return $null }
    $xy = [uint32][int64]$w[0]
    $zWord = [uint32][int64]$w[1]
    $raw = @(
        [uint16]($xy -band 0xffffu),
        [uint16](($xy -shr 16) -band 0xffffu),
        [uint16]($zWord -band 0xffffu)
    )
    $v = [double[]]::new(4)
    for ($i = 0; $i -lt 3; $i++) {
        $signed = if ($raw[$i] -ge 0x8000) {
            [double]$raw[$i] - 65536.0
        } else {
            [double]$raw[$i]
        }
        $v[$i] = $signed / $script:one4p12
    }
    $v[3] = 1.0
    , $v
}

function Get-Task49EffectVertices {
    param([Parameter(Mandatory=$true)]$Cap)

    # DS geometry uses row vectors for the renderer's m[4][4] convention:
    # v' = v * modelview * projection.  M1's split loader writes projection in
    # mode 0 and modelview in mode 2.  Route 0 writes identity/identity because
    # ARM9 already projected its VERTEX16 positions.  Replaying the captured
    # state here therefore compares what reaches rasterization, not unlike
    # operands.
    $projection = New-Task49IdentityMatrix
    $modelview = New-Task49IdentityMatrix
    $mode = 2
    $vertices = [Collections.Generic.List[object]]::new()
    $faults = [Collections.Generic.List[object]]::new()

    for ($entryIndex = 0; $entryIndex -lt $Cap.entries.Count; $entryIndex++) {
        $entry = $Cap.entries[$entryIndex]
        $class = [int]$entry.command_class
        if ($class -eq 7) {
            $words = @($entry.words)
            if ($words.Count -ne 1) {
                $faults.Add([pscustomobject]@{
                    entry=$entryIndex; reason='matrix-mode-word-count'
                })
                continue
            }
            $mode = [int][uint32][int64]$words[0]
            continue
        }
        if ($class -eq 8) {
            if ($mode -eq 0) {
                $projection = New-Task49IdentityMatrix
            } elseif ($mode -eq 2) {
                $modelview = New-Task49IdentityMatrix
            }
            continue
        }
        if ($class -eq $script:load4x4Class) {
            $loaded = ConvertTo-ClipMatrix @($entry.words)
            if ($null -eq $loaded) {
                $faults.Add([pscustomobject]@{
                    entry=$entryIndex; reason='load4x4-word-count'
                })
                continue
            }
            if ($mode -eq 0) {
                $projection = $loaded
            } elseif ($mode -eq 2) {
                $modelview = $loaded
            } else {
                $faults.Add([pscustomobject]@{
                    entry=$entryIndex; reason='unsupported-matrix-mode'; mode=$mode
                })
            }
            continue
        }
        if (($script:matrixClasses -contains $class) -and
            ($class -notin @(7,8,$script:load4x4Class))) {
            $faults.Add([pscustomobject]@{
                entry=$entryIndex; reason='unsupported-matrix-command'; class=$class
            })
            continue
        }
        if ($class -ne $script:vertex16Class) { continue }

        $source = ConvertFrom-Task49Vertex16 @($entry.words)
        if ($null -eq $source) {
            $faults.Add([pscustomobject]@{
                entry=$entryIndex; reason='vertex16-word-count'
            })
            continue
        }
        $view = Invoke-Task49RowVectorTransform $source $modelview
        $clip = Invoke-Task49RowVectorTransform $view $projection
        if ([Math]::Abs($clip[3]) -lt 1e-9) {
            $faults.Add([pscustomobject]@{
                entry=$entryIndex; reason='zero-clip-w'
            })
            continue
        }
        $vertices.Add([pscustomobject]@{
            entry = $entryIndex
            binding = [int]$entry.binding_index
            ndc_x = $clip[0] / $clip[3]
            ndc_y = $clip[1] / $clip[3]
            ndc_z = $clip[2] / $clip[3]
        })
    }
    [pscustomobject]@{
        vertices = $vertices
        faults = $faults
    }
}

function Test-EffectTier2EffectiveVertices {
    param(
        [Parameter(Mandatory=$true)]$CapA,
        [Parameter(Mandatory=$true)]$CapB
    )

    $a = Get-Task49EffectVertices $CapA
    $b = Get-Task49EffectVertices $CapB
    $n = [Math]::Min($a.vertices.Count, $b.vertices.Count)
    $maxPx = 0.0
    $sumPx = 0.0
    $maxDepth = 0.0
    $perVertex = [Collections.Generic.List[object]]::new()

    for ($i = 0; $i -lt $n; $i++) {
        $va = $a.vertices[$i]
        $vb = $b.vertices[$i]
        $dxPx = ([double]$va.ndc_x - [double]$vb.ndc_x) *
            ($script:screenW / 2.0)
        $dyPx = ([double]$va.ndc_y - [double]$vb.ndc_y) *
            ($script:screenH / 2.0)
        $screenPx = [Math]::Sqrt($dxPx*$dxPx + $dyPx*$dyPx)
        $depthNdc = [Math]::Abs([double]$va.ndc_z - [double]$vb.ndc_z)
        if ($screenPx -gt $maxPx) { $maxPx = $screenPx }
        if ($depthNdc -gt $maxDepth) { $maxDepth = $depthNdc }
        $sumPx += $screenPx
        $perVertex.Add([pscustomobject]@{
            vertex = $i
            a_entry = $va.entry
            b_entry = $vb.entry
            screen_px = [Math]::Round($screenPx,6)
            depth_ndc = [Math]::Round($depthNdc,9)
        })
    }

    $faults = @($a.faults) + @($b.faults)
    $countMismatch = ($a.vertices.Count -ne $b.vertices.Count)
    $pass = (
        $n -gt 0 -and
        -not $countMismatch -and
        $faults.Count -eq 0 -and
        $maxPx -le $script:effectScreenThresholdPx -and
        $maxDepth -le $script:effectDepthThresholdNdc
    )
    [pscustomobject]@{
        tier = 2
        standard = 'EFFECT effective VERTEX16 transform; <=1 px screen, <=1 4.12 LSB depth'
        vertices_compared = $n
        vertices_a = $a.vertices.Count
        vertices_b = $b.vertices.Count
        screen_threshold_px = $script:effectScreenThresholdPx
        depth_threshold_ndc = $script:effectDepthThresholdNdc
        max_screen_px = [Math]::Round($maxPx,6)
        mean_screen_px = if ($n -gt 0) {
            [Math]::Round($sumPx / $n,6)
        } else { 0.0 }
        max_depth_ndc = [Math]::Round($maxDepth,9)
        fault_count = $faults.Count
        faults = $faults
        per_vertex = $perVertex
        verdict = if ($pass) { 'PASS' } else { 'FAIL' }
    }
}

function ConvertTo-ClipMatrix {
    param([Parameter(Mandatory=$true)]$Words)
    # 16 words = s32 m[4][4] in 20.12 fixed-point, row-major. Convert each to a
    # signed double in a FLAT 16-element array (PowerShell unwraps 2D arrays on
    # function return; a flat array indexed [r*4+c] avoids that). uint32 ->
    # int32 reinterpretation. Accepts the Object[] JSON deserialization produces.
    $w = @($Words)
    if ($w.Count -ne 16) { return $null }
    $m = [double[]]::new(16)
    for ($i = 0; $i -lt 16; $i++) {
        $u = [uint32][int64]$w[$i]
        # Reinterpret as signed int32.
        # PowerShell parses bare 0x80000000 as signed Int32 (-2147483648),
        # which makes every UInt32 compare >= it and corrupts all positive
        # matrix elements. Compare in double precision against 2^31 instead.
        $s = if ([double]$u -ge 2147483648.0) {
            [double]$u - 4294967296.0
        } else {
            [double]$u
        }
        $m[$i] = $s / $script:one20p12
    }
    # Wrap in a single-element array so PowerShell returns the array object
    # itself rather than unrolling it.
    , $m
}

function Invoke-ClipTransform {
    param(
        [Parameter(Mandatory=$true)][double[]]$M,   # flat 16, row-major
        [Parameter(Mandatory=$true)]$V              # x,y,z (w=1)
    )
    # M is the projection*view*model clip matrix. Transform a model-space point.
    $v = @($V)
    $cx = $M[0]*$v[0]  + $M[1]*$v[1]  + $M[2]*$v[2]  + $M[3]
    $cy = $M[4]*$v[0]  + $M[5]*$v[1]  + $M[6]*$v[2]  + $M[7]
    $cz = $M[8]*$v[0]  + $M[9]*$v[1]  + $M[10]*$v[2] + $M[11]
    $cw = $M[12]*$v[0] + $M[13]*$v[1] + $M[14]*$v[2] + $M[15]
    , @([double]$cx,[double]$cy,[double]$cz,[double]$cw)
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
        @(-1.0,-1.0,-1.0), @(1.0,-1.0,-1.0), @(-1.0,1.0,-1.0), @(1.0,1.0,-1.0),
        @(-1.0,-1.0,1.0), @(1.0,-1.0,1.0), @(-1.0,1.0,1.0), @(1.0,1.0,1.0))

    $n = [Math]::Min($aBindings.Count, $bBindings.Count)
    $perBinding = [Collections.Generic.List[object]]::new()
    $maxDev = 0.0
    $sumDev = 0.0
    $devCount = 0
    for ($bi = 0; $bi -lt $n; $bi++) {
        $ma = ConvertTo-ClipMatrix @($aBindings[$bi].words)
        $mb = ConvertTo-ClipMatrix @($bBindings[$bi].words)
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

$effectOwner = ($OwnerName.ToUpperInvariant() -eq 'EFFECT')
$tier1 = Test-Tier1BitExact $capA $capB -ExcludeVertex16:$effectOwner
$tier2 = if ($effectOwner) {
    Test-EffectTier2EffectiveVertices $capA $capB
} else {
    Test-Tier2EffectiveTransform $capA $capB
}

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
