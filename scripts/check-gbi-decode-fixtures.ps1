param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
function Assert-Equal {
    param(
        [object]$Actual,
        [object]$Expected,
        [string]$Message
    )
    if ($Actual -ne $Expected) {
        throw "$Message actual=$Actual expected=$Expected"
    }
}
function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}
function Get-TextureByte {
    param(
        [byte[]]$Bytes,
        [int]$LogicalIndex,
        [string]$Layout
    )
    $physical = if ($Layout -eq 'O2R') { $LogicalIndex -bxor 3 } else { $LogicalIndex }
    return $Bytes[$physical]
}
function Get-TextureNibble {
    param(
        [byte[]]$Bytes,
        [int]$LogicalTexelIndex,
        [string]$Layout
    )
    $packed = Get-TextureByte $Bytes ([Math]::Floor($LogicalTexelIndex / 2)) $Layout
    if (($LogicalTexelIndex -band 1) -eq 0) {
        return ($packed -shr 4)
    }
    return ($packed -band 0x0f)
}
function Get-TextureHalfword {
    param(
        [uint16[]]$Halfwords,
        [int]$LogicalIndex,
        [string]$Layout
    )
    $physical = if ($Layout -eq 'O2R') { $LogicalIndex -bxor 1 } else { $LogicalIndex }
    return $Halfwords[$physical]
}
function Get-LoadBlockDxtSourceWidth {
    param(
        [int]$Size,
        [int]$Dxt,
        [int]$FallbackWidth
    )
    if ($Dxt -eq 0) {
        return $FallbackWidth
    }
    $qwords = [Math]::Floor((2048 + $Dxt - 1) / $Dxt)
    switch ($Size) {
        0 { return $qwords * 16 }
        1 { return $qwords * 8 }
        2 { return $qwords * 4 }
        3 { return $qwords * 2 }
        default { return 0 }
    }
}
function Test-Texel01LerpCombine {
    param(
        [uint32]$W0,
        [uint32]$W1
    )
    return ((($W0 -shr 20) -band 0x0f) -eq 2) -and
        ((($W1 -shr 28) -band 0x0f) -eq 1) -and
        ((($W0 -shr 15) -band 0x1f) -eq 14) -and
        ((($W1 -shr 15) -band 0x07) -eq 1) -and
        ((($W0 -shr 12) -band 0x07) -eq 2) -and
        ((($W1 -shr 12) -band 0x07) -eq 1) -and
        ((($W0 -shr 9) -band 0x07) -eq 6) -and
        ((($W1 -shr 9) -band 0x07) -eq 1) -and
        ((($W0 -shr 5) -band 0x0f) -eq 0) -and
        ((($W1 -shr 24) -band 0x0f) -eq 15) -and
        (($W0 -band 0x1f) -eq 4) -and
        ((($W1 -shr 6) -band 0x07) -eq 7) -and
        ((($W1 -shr 21) -band 0x07) -eq 0) -and
        ((($W1 -shr 18) -band 0x07) -eq 3)
}
function Blend-Texel01Rgb5551 {
    param(
        [uint16]$Texel0,
        [uint16]$Texel1,
        [uint32]$Fraction,
        [uint32]$X = 0,
        [uint32]$Y = 0
    )
    $inverse = 0x100 - $Fraction
    $r0 = (((($Texel0 -shr 0) -band 0x1f) -shl 3) -bor ((($Texel0 -shr 0) -band 0x1f) -shr 2))
    $g0 = (((($Texel0 -shr 5) -band 0x1f) -shl 3) -bor ((($Texel0 -shr 5) -band 0x1f) -shr 2))
    $b0 = (((($Texel0 -shr 10) -band 0x1f) -shl 3) -bor ((($Texel0 -shr 10) -band 0x1f) -shr 2))
    $r1 = (((($Texel1 -shr 0) -band 0x1f) -shl 3) -bor ((($Texel1 -shr 0) -band 0x1f) -shr 2))
    $g1 = (((($Texel1 -shr 5) -band 0x1f) -shl 3) -bor ((($Texel1 -shr 5) -band 0x1f) -shr 2))
    $b1 = (((($Texel1 -shr 10) -band 0x1f) -shl 3) -bor ((($Texel1 -shr 10) -band 0x1f) -shr 2))
    $red = ((($r0 * $inverse) + ($r1 * $Fraction)) -shr 8) -shr 3
    $green = ((($g0 * $inverse) + ($g1 * $Fraction)) -shr 8) -shr 3
    $blue = ((($b0 * $inverse) + ($b1 * $Fraction)) -shr 8) -shr 3
    $alphaCoverage = (((($Texel0 -shr 15) -band 1) * 0x100 * $inverse) +
        ((($Texel1 -shr 15) -band 1) * 0x100 * $Fraction)) -shr 8
    $bayer = @(0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5)
    $threshold = ($bayer[(($Y -band 3) -shl 2) -bor ($X -band 3)] -shl 4) + 8
    $alpha = if ($alphaCoverage -gt $threshold) { 1 } else { 0 }
    return [uint16]($red -bor ($green -shl 5) -bor ($blue -shl 10) -bor
        ($alpha -shl 15))
}
function Convert-N64Rgba16ToDs {
    param(
        [uint16]$Color,
        [bool]$PreserveTransparentRgb = $false
    )
    if ((($Color -band 1) -eq 0) -and -not $PreserveTransparentRgb) {
        return [uint16]0
    }
    $red = ($Color -shr 11) -band 0x1f
    $green = ($Color -shr 6) -band 0x1f
    $blue = ($Color -shr 1) -band 0x1f
    $alpha = if (($Color -band 1) -ne 0) { 1 } else { 0 }
    return [uint16]($red -bor ($green -shl 5) -bor ($blue -shl 10) -bor
        ($alpha -shl 15))
}
function Find-LatestTmemLoad {
    param(
        [object[]]$Loads,
        [uint32]$Tmem
    )
    return $Loads | Where-Object { $_.Valid -and $_.Tmem -eq $Tmem } |
        Sort-Object Sequence -Descending | Select-Object -First 1
}

function Get-CompactTextureLoadTexels {
    param([uint32]$Texels)

    if ($Texels -gt 0xffff) { return [uint16]0 }
    return [uint16]$Texels
}
function Convert-MObjSubMixedFields {
    param(
        [uint16]$Pad00,
        [byte]$Fmt,
        [byte]$Siz,
        [uint16]$Flags,
        [byte]$BlockFmt,
        [byte]$BlockSiz
    )
    return [pscustomobject]@{
        Pad00 = [uint16](([uint16]$Siz -shl 8) -bor $Fmt)
        Fmt = [byte]($Pad00 -shr 8)
        Siz = [byte]($Pad00 -band 0xff)
        Flags = [uint16](([uint16]$BlockSiz -shl 8) -bor $BlockFmt)
        BlockFmt = [byte]($Flags -shr 8)
        BlockSiz = [byte]($Flags -band 0xff)
    }
}
function Convert-N64TexCoordToDsT16 {
    param(
        [int]$Coord,
        [uint32]$Scale,
        [uint32]$Origin,
        [int]$Offset = 0
    )
    [Int64]$scaledT16 = ([Int64]$Coord * [Int64]$Scale) -shr 17
    [Int64]$originT16 = [Int64]$Origin -shl 2
    return [int]($scaledT16 - $originT16 + $Offset)
}
function Test-N64MaskedClampNeedsWrap {
    param(
        [uint32]$Mode,
        [uint32]$Mask,
        [uint32]$UploadExtent,
        [uint32]$TileExtent
    )
    if ((($Mode -band 2) -eq 0) -or ($Mask -eq 0) -or
        ($Mask -ge 31) -or ($UploadExtent -eq 0) -or
        ($TileExtent -eq 0)) {
        return $false
    }
    [uint32]$maskExtent = 1 -shl $Mask
    if ($UploadExtent -ne $maskExtent) { return $false }
    [uint32]$samplerExtent = $UploadExtent
    if (($Mode -band 1) -ne 0) { $samplerExtent = $samplerExtent -shl 1 }
    return (($Mode -band 1) -ne 0) -or ($samplerExtent -ne $TileExtent)
}
function Convert-N64MaskedTextureAddress {
    param(
        [uint32]$Coord,
        [uint32]$Mode,
        [uint32]$Mask
    )
    [uint32]$extent = 1 -shl $Mask
    [uint32]$period = $Coord -shr $Mask
    [uint32]$local = $Coord -band ($extent - 1)
    if ((($Mode -band 1) -ne 0) -and (($period -band 1) -ne 0)) {
        $local = $extent - 1 - $local
    }
    return $local
}
function Test-N64MaskedClampMaterialization {
    param(
        [uint32]$Mode,
        [uint32]$Mask,
        [uint32]$SourceExtent,
        [uint32]$TileExtent
    )
    if ((($Mode -band 2) -eq 0) -or ($Mask -eq 0) -or
        ($Mask -ge 31) -or ($SourceExtent -eq 0) -or
        ($TileExtent -gt 128)) {
        return $false
    }
    [uint32]$maskExtent = 1 -shl $Mask
    return ($TileExtent -gt $maskExtent) -and
        ($SourceExtent -ge $maskExtent) -and
        ($SourceExtent -le $TileExtent)
}
function New-F3DEX2VtxW0 {
    param(
        [int]$Count,
        [int]$V0
    )
    return ((0x01 -shl 24) -bor (($Count -band 0xff) -shl 12) -bor ((($V0 + $Count) -band 0x7f) -shl 1))
}
function Decode-F3DEX2Vtx {
    param(
        [uint32]$W0,
        [uint32]$MaxVtx
    )
    $count = ($W0 -shr 12) -band 0xff
    $end = ($W0 -shr 1) -band 0x7f
    if (($count -eq 0) -or ($count -gt $MaxVtx)) { return $null }
    if (($end -lt $count) -or ($end -gt $MaxVtx)) { return $null }
    return @{ V0 = ($end - $count); Count = $count }
}
function New-F3DEX2TriPacked {
    param(
        [int]$V0,
        [int]$V1,
        [int]$V2,
        [int]$Flag = 0
    )
    return (($Flag -band 0xff) -shl 24) -bor ((($V0 * 2) -band 0xff) -shl 16) -bor ((($V1 * 2) -band 0xff) -shl 8) -bor (($V2 * 2) -band 0xff)
}
function Decode-F3DEX2TriPacked {
    param([uint32]$Packed)
    return @{
        V0 = ((($Packed -shr 16) -band 0xff) / 2)
        V1 = ((($Packed -shr 8) -band 0xff) / 2)
        V2 = (($Packed -band 0xff) / 2)
    }
}
function Test-LitPrimitiveModulate {
    param(
        [uint32]$W0,
        [uint32]$W1,
        [bool]$TwoCycle = $false,
        [uint32]$EnvColor = [uint32]::MaxValue
    )
    if ($TwoCycle) {
        $a1 = ($W0 -shr 5) -band 0x0f
        $b1 = ($W1 -shr 24) -band 0x0f
        $c1 = $W0 -band 0x1f
        $d1 = ($W1 -shr 6) -band 0x07
        if (($EnvColor -ne [uint32]::MaxValue) -or
            ($a1 -ne 0) -or ($b1 -ne 15) -or
            ($c1 -ne 5) -or ($d1 -ne 7)) {
            return 0
        }
    }
    $a = ($W0 -shr 20) -band 0x0f
    $b = ($W1 -shr 28) -band 0x0f
    $c = ($W0 -shr 15) -band 0x1f
    $d = ($W1 -shr 15) -band 0x07
    if (($b -ne 15) -or ($d -ne 7)) { return 0 }
    return (($a -eq 3) -and ($c -eq 4)) -or
        (($a -eq 4) -and ($c -eq 3))
}
function New-F3DEX2MtxW0 {
    param([int]$Flags)
    return [uint32](([int64]0xda -shl 24) -bor ([int64]7 -shl 19) -bor (($Flags -bxor 1) -band 0xff))
}
function New-F3DEXMvpRecalcW0 {
    return [uint32](([int64]0xd5 -shl 24) -bor 1)
}
function New-F3DEXMoveWdW0 {
    param(
        [int]$Index,
        [int]$Offset
    )
    return [uint32](([int64]0xdb -shl 24) -bor (([int64]$Offset -band 0xffff) -shl 8) -bor ($Index -band 0xff))
}
function Decode-F3DEX2MtxFlags {
    param([uint32]$W0)
    return (($W0 -band 0xff) -bxor 1)
}
function New-N64PackedMtx {
    param([double[][]]$Rows)
    $words = @(0) * 16
    $ai = 0
    $af = 8
    for ($i = 0; $i -lt 4; $i++) {
        for ($j = 0; $j -lt 2; $j++) {
            $e1 = [int][Math]::Round($Rows[$i][$j * 2] * 65536.0)
            $e2 = [int][Math]::Round($Rows[$i][$j * 2 + 1] * 65536.0)
            $u1 = [uint32]([int64]$e1 -band 0xffffffffL)
            $u2 = [uint32]([int64]$e2 -band 0xffffffffL)
            $words[$ai++] = (($u1 -band 0xffff0000) -bor (($u2 -shr 16) -band 0xffff))
            $words[$af++] = ((($u1 -shl 16) -band 0xffff0000) -bor ($u2 -band 0xffff))
        }
    }
    return $words
}
function Get-N64PackedMtxCellS16p16 {
    param(
        [uint32[]]$Words,
        [int]$Row,
        [int]$Col
    )
    $pair = ($Row * 2) + [Math]::Floor($Col / 2)
    $hi = $Words[$pair]
    $lo = $Words[8 + $pair]
    if (($Col -band 1) -eq 0) {
        $raw = (($hi -band 0xffff0000) -bor (($lo -shr 16) -band 0xffff))
    } else {
        $raw = ((($hi -shl 16) -band 0xffff0000) -bor ($lo -band 0xffff))
    }
    if (($raw -band 0x80000000) -ne 0) {
        return [int]([int64]$raw - 0x100000000L)
    }
    return [int]$raw
}
function Convert-S16p16To20p12 {
    param([int]$Value)
    if ($Value -lt 0) {
        return -([int](((-$Value) + 8) -shr 4))
    }
    return [int](($Value + 8) -shr 4)
}
function Round-Shift-S64 {
    param(
        [Int64]$Value,
        [int]$Shift
    )
    if ($Shift -eq 0) { return $Value }
    $bias = [Int64]1 -shl ($Shift - 1)
    if ($Value -lt 0) {
        return -([Int64](((-$Value) + $bias) -shr $Shift))
    }
    return [Int64](($Value + $bias) -shr $Shift)
}
function Multiply-Mtx20p12 {
    param(
        [object[]]$Lhs,
        [object[]]$Rhs
    )
    $out = @()
    for ($row = 0; $row -lt 4; $row++) {
        $outRow = @()
        for ($col = 0; $col -lt 4; $col++) {
            [Int64]$sum = 0
            for ($k = 0; $k -lt 4; $k++) {
                $sum += [Int64]$Lhs[$row][$k] * [Int64]$Rhs[$k][$col]
            }
            $outRow += [int](Round-Shift-S64 -Value $sum -Shift 12)
        }
        $out += ,$outRow
    }
    return $out
}
function Convert-N64PackedMtxTo20p12 {
    param([uint32[]]$Words)
    $out = @()
    for ($row = 0; $row -lt 4; $row++) {
        $outRow = @()
        for ($col = 0; $col -lt 4; $col++) {
            $outRow += Convert-S16p16To20p12 (Get-N64PackedMtxCellS16p16 -Words $Words -Row $row -Col $col)
        }
        $out += ,$outRow
    }
    return $out
}
function Transform-Vertex20p12 {
    param(
        [object[]]$Mtx,
        [int]$X,
        [int]$Y,
        [int]$Z
    )
    return @{
        X = $Mtx[0][0] * $X + $Mtx[1][0] * $Y + $Mtx[2][0] * $Z + $Mtx[3][0]
        Y = $Mtx[0][1] * $X + $Mtx[1][1] * $Y + $Mtx[2][1] * $Z + $Mtx[3][1]
        Z = $Mtx[0][2] * $X + $Mtx[1][2] * $Y + $Mtx[2][2] * $Z + $Mtx[3][2]
        W = $Mtx[0][3] * $X + $Mtx[1][3] * $Y + $Mtx[2][3] * $Z + $Mtx[3][3]
    }
}
$vtxW0 = New-F3DEX2VtxW0 -Count 4 -V0 12
Assert-Equal $vtxW0 0x01004020 'F3DEX2 gSPVertex(4,12) fixture packed word mismatch.'
$decodedVtx = Decode-F3DEX2Vtx -W0 $vtxW0 -MaxVtx 32
Assert-True ($null -ne $decodedVtx) 'F3DEX2 gSPVertex fixture did not decode.'
Assert-Equal $decodedVtx.V0 12 'F3DEX2 gSPVertex fixture decoded v0 mismatch.'
Assert-Equal $decodedVtx.Count 4 'F3DEX2 gSPVertex fixture decoded count mismatch.'
$tri1Packed = New-F3DEX2TriPacked -V0 1 -V1 2 -V2 3
$tri1W0 = (0x05 -shl 24) -bor $tri1Packed
$tri1W1 = 0
Assert-Equal $tri1W0 0x05020406 'F3DEX2 gSP1Triangle fixture w0 mismatch.'
Assert-Equal $tri1W1 0 'F3DEX2 gSP1Triangle fixture w1 should be zero.'
$decodedTri1 = Decode-F3DEX2TriPacked -Packed ($tri1W0 -band 0x00ffffff)
Assert-Equal $decodedTri1.V0 1 'F3DEX2 gSP1Triangle decoded v0 mismatch.'
Assert-Equal $decodedTri1.V1 2 'F3DEX2 gSP1Triangle decoded v1 mismatch.'
Assert-Equal $decodedTri1.V2 3 'F3DEX2 gSP1Triangle decoded v2 mismatch.'
$tri2First = New-F3DEX2TriPacked -V0 4 -V1 5 -V2 6
$tri2Second = New-F3DEX2TriPacked -V0 7 -V1 8 -V2 9
$tri2W0 = (0x06 -shl 24) -bor $tri2First
$tri2W1 = $tri2Second
Assert-Equal $tri2W0 0x06080A0C 'F3DEX2 gSP2Triangles fixture w0 mismatch.'
Assert-Equal $tri2W1 0x000E1012 'F3DEX2 gSP2Triangles fixture w1 mismatch.'
$decodedTri2First = Decode-F3DEX2TriPacked -Packed ($tri2W0 -band 0x00ffffff)
$decodedTri2Second = Decode-F3DEX2TriPacked -Packed $tri2W1
Assert-Equal $decodedTri2First.V0 4 'F3DEX2 gSP2Triangles first decoded v0 mismatch.'
Assert-Equal $decodedTri2First.V1 5 'F3DEX2 gSP2Triangles first decoded v1 mismatch.'
Assert-Equal $decodedTri2First.V2 6 'F3DEX2 gSP2Triangles first decoded v2 mismatch.'
Assert-Equal $decodedTri2Second.V0 7 'F3DEX2 gSP2Triangles second decoded v0 mismatch.'
Assert-Equal $decodedTri2Second.V1 8 'F3DEX2 gSP2Triangles second decoded v1 mismatch.'
Assert-Equal $decodedTri2Second.V2 9 'F3DEX2 gSP2Triangles second decoded v2 mismatch.'
$validMask = (1 -shl 4) -bor (1 -shl 5) -bor (1 -shl 6) -bor (1 -shl 8)
$triReadyMask = (1 -shl $decodedTri2First.V0) -bor (1 -shl $decodedTri2First.V1) -bor (1 -shl $decodedTri2First.V2)
Assert-True (($validMask -band $triReadyMask) -eq $triReadyMask) 'Transformed triangle readiness mask did not accept the first TRI2 fixture.'
$triMissingMask = (1 -shl $decodedTri2Second.V0) -bor (1 -shl $decodedTri2Second.V1) -bor (1 -shl $decodedTri2Second.V2)
Assert-True (($validMask -band $triMissingMask) -ne $triMissingMask) 'Transformed triangle readiness mask did not reject the second TRI2 fixture.'
$marioJointVtx = Decode-F3DEX2Vtx -W0 0x01004010 -MaxVtx 32
Assert-Equal $marioJointVtx.V0 4 'Mario joint cache fixture decoded v0 mismatch.'
Assert-Equal $marioJointVtx.Count 4 'Mario joint cache fixture decoded count mismatch.'
$marioPriorMask = 0x0f
$marioJointMask = $marioPriorMask
for ($i = 0; $i -lt $marioJointVtx.Count; $i++) {
    $marioJointMask = $marioJointMask -bor (1 -shl ($marioJointVtx.V0 + $i))
}
$marioJointTriangles = @(0x06040e, 0x0c060e, 0x0e0402, 0x060c0a)
foreach ($packed in $marioJointTriangles) {
    $tri = Decode-F3DEX2TriPacked -Packed $packed
    $mask = (1 -shl $tri.V0) -bor (1 -shl $tri.V1) -bor (1 -shl $tri.V2)
    Assert-True (($marioJointMask -band $mask) -eq $mask) 'Mario cross-part vertex-cache fixture rejected a source triangle.'
}
$firstMarioJointTri = Decode-F3DEX2TriPacked -Packed $marioJointTriangles[0]
$firstMarioJointMask = (1 -shl $firstMarioJointTri.V0) -bor
    (1 -shl $firstMarioJointTri.V1) -bor
    (1 -shl $firstMarioJointTri.V2)
Assert-True ((0xf0 -band $firstMarioJointMask) -ne $firstMarioJointMask) 'Mario cross-part fixture must require a prior cached vertex.'
$mtxProjectionLoadW0 = New-F3DEX2MtxW0 -Flags (0x04 -bor 0x02)
$mtxModelViewLoadW0 = New-F3DEX2MtxW0 -Flags 0x02
$mtxModelViewPushMulW0 = New-F3DEX2MtxW0 -Flags 0x01
$popMtxW1 = 64
Assert-Equal $mtxProjectionLoadW0 3661103111 'F3DEX2 gSPMatrix projection-load fixture w0 mismatch.'
Assert-Equal $mtxModelViewLoadW0 3661103107 'F3DEX2 gSPMatrix modelview-load fixture w0 mismatch.'
Assert-Equal $mtxModelViewPushMulW0 3661103104 'F3DEX2 gSPMatrix modelview-push-mul fixture w0 mismatch.'
Assert-Equal ($popMtxW1 / 64) 1 'F3DEX2 gSPPopMatrix fixture pop count mismatch.'
Assert-Equal (Decode-F3DEX2MtxFlags -W0 $mtxProjectionLoadW0) 0x06 'F3DEX2 gSPMatrix projection-load flags decode mismatch.'
Assert-Equal (Decode-F3DEX2MtxFlags -W0 $mtxModelViewLoadW0) 0x02 'F3DEX2 gSPMatrix modelview-load flags decode mismatch.'
Assert-Equal (Decode-F3DEX2MtxFlags -W0 $mtxModelViewPushMulW0) 0x01 'F3DEX2 gSPMatrix modelview-push-mul flags decode mismatch.'
$mvpRecalcW0 = New-F3DEXMvpRecalcW0
$matrixMoveWdW0 = New-F3DEXMoveWdW0 -Index 0 -Offset 0x20
Assert-Equal $mvpRecalcW0 ([uint32](([int64]0xd5 -shl 24) -bor 1)) 'F3DEX gSPMvpRecalc fixture w0 mismatch.'
Assert-Equal $matrixMoveWdW0 ([uint32](([int64]0xdb -shl 24) -bor ([int64]0x20 -shl 8))) 'F3DEX gMoveWd(G_MW_MATRIX, 0x20) fixture w0 mismatch.'
Assert-Equal (($matrixMoveWdW0 -shr 24) -band 0xff) 0xdb 'F3DEX gMoveWd opcode decode mismatch.'
Assert-Equal (($matrixMoveWdW0 -shr 8) -band 0xffff) 0x20 'F3DEX gMoveWd offset decode mismatch.'
Assert-Equal ($matrixMoveWdW0 -band 0xff) 0 'F3DEX gMoveWd index decode mismatch.'
$identityRows = @(
    @(1.0, 0.0, 0.0, 0.0),
    @(0.0, 1.0, 0.0, 0.0),
    @(0.0, 0.0, 1.0, 0.0),
    @(0.0, 0.0, 0.0, 1.0)
)
$identity = Convert-N64PackedMtxTo20p12 (New-N64PackedMtx -Rows $identityRows)
Assert-Equal $identity[0][0] 4096 'N64 identity matrix did not convert to DS 20.12 one.'
Assert-Equal $identity[3][3] 4096 'N64 identity matrix W did not convert to DS 20.12 one.'
$identityVtx = Transform-Vertex20p12 -Mtx $identity -X 10 -Y -20 -Z 30
Assert-Equal $identityVtx.X 40960 'Identity transformed X mismatch.'
Assert-Equal $identityVtx.Y -81920 'Identity transformed Y mismatch.'
Assert-Equal $identityVtx.Z 122880 'Identity transformed Z mismatch.'
Assert-Equal $identityVtx.W 4096 'Identity transformed W mismatch.'
$transformRows = @(
    @(2.0, 0.0, 0.0, 0.0),
    @(0.0, -3.0, 0.0, 0.0),
    @(0.0, 0.0, 0.5, 0.0),
    @(5.0, -7.0, 11.0, 1.0)
)
$transform = Convert-N64PackedMtxTo20p12 (New-N64PackedMtx -Rows $transformRows)
$transformedVtx = Transform-Vertex20p12 -Mtx $transform -X 12 -Y -3 -Z 8
Assert-Equal $transform[2][2] 2048 'N64 0.5 scale did not convert to DS 20.12 half.'
Assert-Equal $transformedVtx.X 118784 'Scale/translate transformed X mismatch.'
Assert-Equal $transformedVtx.Y 8192 'Scale/translate transformed Y mismatch.'
Assert-Equal $transformedVtx.Z 61440 'Scale/translate transformed Z mismatch.'
Assert-Equal $transformedVtx.W 4096 'Scale/translate transformed W mismatch.'
$projectionRows = @(
    @(2.0, 0.0, 0.0, 0.0),
    @(0.0, 2.0, 0.0, 0.0),
    @(0.0, 0.0, 1.0, 0.0),
    @(0.0, 0.0, 0.0, 1.0)
)
$projection = Convert-N64PackedMtxTo20p12 (New-N64PackedMtx -Rows $projectionRows)
$combined = Multiply-Mtx20p12 -Lhs $transform -Rhs $projection
$combinedVtx = Transform-Vertex20p12 -Mtx $combined -X 12 -Y -3 -Z 8
Assert-Equal $combinedVtx.X 237568 'Modelview-projection composed transformed X mismatch.'
Assert-Equal $combinedVtx.Y 16384 'Modelview-projection composed transformed Y mismatch.'
Assert-Equal $combinedVtx.Z 61440 'Modelview-projection composed transformed Z mismatch.'
Assert-Equal $combinedVtx.W 4096 'Modelview-projection composed transformed W mismatch.'
$modelviewStack = @()
$modelviewStack += ,$transform
$innerRows = @(
    @(1.0, 0.0, 0.0, 0.0),
    @(0.0, 1.0, 0.0, 0.0),
    @(0.0, 0.0, 1.0, 0.0),
    @(100.0, 0.0, 0.0, 1.0)
)
$inner = Convert-N64PackedMtxTo20p12 (New-N64PackedMtx -Rows $innerRows)
$pushedModelview = Multiply-Mtx20p12 -Lhs $transform -Rhs $inner
$pushedVtx = Transform-Vertex20p12 -Mtx $pushedModelview -X 12 -Y -3 -Z 8
$restoredModelview = $modelviewStack[$modelviewStack.Count - 1]
$restoredVtx = Transform-Vertex20p12 -Mtx $restoredModelview -X 12 -Y -3 -Z 8
Assert-True ($pushedVtx.X -ne $restoredVtx.X) 'Modelview push fixture did not change transformed X.'
Assert-Equal $restoredVtx.X $transformedVtx.X 'Modelview pop fixture did not restore transformed X.'
Assert-Equal $restoredVtx.Y $transformedVtx.Y 'Modelview pop fixture did not restore transformed Y.'
Assert-Equal $restoredVtx.Z $transformedVtx.Z 'Modelview pop fixture did not restore transformed Z.'
Assert-Equal $restoredVtx.W $transformedVtx.W 'Modelview pop fixture did not restore transformed W.'
$rawValidMask = (1 -shl 1) -bor (1 -shl 2) -bor (1 -shl 3)
$rawTri = New-F3DEX2TriPacked -V0 1 -V1 2 -V2 3
$decodedRawTri = Decode-F3DEX2TriPacked -Packed $rawTri
$rawTriMask = (1 -shl $decodedRawTri.V0) -bor
    (1 -shl $decodedRawTri.V1) -bor
    (1 -shl $decodedRawTri.V2)
Assert-True (($rawValidMask -band $rawTriMask) -eq $rawTriMask) 'Hardware raw triangle readiness fixture failed.'
$forbiddenSnippets = @(
    'u32 v0 = ((w0 >> 16) & 0xFFu) / 2u;',
    'u32 count = (w0 & 0xFFu) / 2u;',
    'u32 v0 = ((command->w0 >> 16) & 0xffu) / 2u;',
    'u32 count = (command->w0 & 0xffu) / 2u;',
    'ndsFighterDLExecTriangleValid(state, command->w1)',
    'ndsFighterDLDrawAppendTriangle(state, command->w1);'
)
$files = @(
    'src/nds/nds_renderer.c',
    'src/port/opening_movie_backend.c',
    'src/port/reloc_backend.c'
)
foreach ($file in $files) {
    $content = Get-Content (Join-Path $root $file) -Raw
    foreach ($snippet in $forbiddenSnippets) {
        if ($content.Contains($snippet)) {
            throw "Old non-F3DEX2 decode snippet remains in ${file}: ${snippet}"
        }
    }
}
$renderer = Get-Content (Join-Path $root 'src/nds/nds_renderer.c') -Raw
$rendererHeader = Get-Content (Join-Path $root 'include/nds/nds_renderer.h') -Raw
$relocAssets = Get-Content (Join-Path $root 'src/port/reloc_backend_assets.c') -Raw
$relocRendererDL = Get-Content (Join-Path $root 'src/port/reloc_backend_renderer_dl.c') -Raw
$relocMPCollision = Get-Content (Join-Path $root 'src/port/reloc_backend_mp_collision.c') -Raw
$objAnimImport = Get-Content (Join-Path $root 'src/import/battleship_sys_objanim.c') -Raw
$o2rCi8 = [byte[]](3, 2, 1, 0)
Assert-Equal (Get-TextureByte $o2rCi8 0 'O2R') 0 'O2R CI8 logical byte 0 lane decode failed.'
Assert-Equal (Get-TextureByte $o2rCi8 1 'O2R') 1 'O2R CI8 logical byte 1 lane decode failed.'
Assert-Equal (Get-TextureByte $o2rCi8 2 'O2R') 2 'O2R CI8 logical byte 2 lane decode failed.'
Assert-Equal (Get-TextureByte $o2rCi8 3 'O2R') 3 'O2R CI8 logical byte 3 lane decode failed.'
$nativeCi8 = [byte[]](0, 1, 2, 3)
Assert-Equal (Get-TextureByte $nativeCi8 0 'Native') 0 'Native CI8 byte 0 should not lane-remap.'
Assert-Equal (Get-TextureByte $nativeCi8 3 'Native') 3 'Native CI8 byte 3 should not lane-remap.'
$o2rCi4 = [byte[]](0x78, 0x56, 0x34, 0x12)
for ($i = 0; $i -lt 8; $i++) {
    Assert-Equal (Get-TextureNibble $o2rCi4 $i 'O2R') ($i + 1) "O2R CI4 nibble $i lane decode failed."
}
$nativeCi4 = [byte[]](0x12, 0x34, 0x56, 0x78)
for ($i = 0; $i -lt 8; $i++) {
    Assert-Equal (Get-TextureNibble $nativeCi4 $i 'Native') ($i + 1) "Native CI4 nibble $i should not lane-remap."
}
Assert-Equal (Get-TextureNibble $o2rCi4 0 'O2R') 1 'O2R IA4/I4 share the CI4 packed-byte lane rule.'
Assert-Equal (Get-TextureByte $o2rCi8 2 'O2R') 2 'O2R IA8/I8 share the CI8 byte lane rule.'
$o2rHalfwords = [uint16[]](0x3344, 0x1122)
Assert-Equal (Get-TextureHalfword $o2rHalfwords 0 'O2R') 0x1122 'O2R RGBA16/TLUT halfword 0 lane decode failed.'
Assert-Equal (Get-TextureHalfword $o2rHalfwords 1 'O2R') 0x3344 'O2R RGBA16/TLUT halfword 1 lane decode failed.'
$nativeHalfwords = [uint16[]](0x1122, 0x3344)
Assert-Equal (Get-TextureHalfword $nativeHalfwords 0 'Native') 0x1122 'Native halfword 0 should not lane-remap.'
Assert-Equal (Get-TextureHalfword $nativeHalfwords 1 'Native') 0x3344 'Native halfword 1 should not lane-remap.'
$rgba32Control = [uint32[]](0x11223344)
Assert-Equal $rgba32Control[0] 0x11223344 'RGBA32 control must stay on the native 32-bit read path.'
# BattleShip gbi.h:3291,3309-3317 defines DXT as the rounded 1.11
# reciprocal of 64-bit words per source row. At dxt=1024, two qwords give
# these logical row strides even when the render tile is narrower.
Assert-Equal (Get-LoadBlockDxtSourceWidth -Size 0 -Dxt 1024 -FallbackWidth 8) 32 'CI4 LOADBLOCK DXT source width mismatch.'
Assert-Equal (Get-LoadBlockDxtSourceWidth -Size 1 -Dxt 1024 -FallbackWidth 4) 16 'CI8 LOADBLOCK DXT source width mismatch.'
Assert-Equal (Get-LoadBlockDxtSourceWidth -Size 2 -Dxt 1024 -FallbackWidth 2) 8 'RGBA16 LOADBLOCK DXT source width mismatch.'
Assert-Equal (Get-LoadBlockDxtSourceWidth -Size 3 -Dxt 1024 -FallbackWidth 1) 4 'RGBA32 LOADBLOCK DXT source width mismatch.'
Assert-Equal (Get-LoadBlockDxtSourceWidth -Size 0 -Dxt 0 -FallbackWidth 8) 8 'DXT-zero LOADBLOCK must retain its bounded fallback width.'
# StagePupupuFile2 DL 0x22D0 uses G_CC_TEMPLERP in cycle 1 and
# COMBINED*SHADE / COMBINED*PRIMITIVE in cycle 2. Its generated MObj branch
# loads next through tile 6/TMEM 0x40 before current through tile 7/TMEM 0.
$pondCombineW0 = [Convert]::ToUInt32('fc272c04', 16)
$pondCombineW1 = [Convert]::ToUInt32('1f0c93ff', 16)
Assert-True (Test-Texel01LerpCombine $pondCombineW0 $pondCombineW1) 'Dream Land pond TEXEL0/TEXEL1 combine mux did not decode as source TEMPLERP.'
$pondLoads = @(
    [pscustomobject]@{ Valid = $true; Sequence = 1; Tile = 6; Tmem = 0x40; Image = 0x1e10; Texels = 256; Dxt = 1024 },
    [pscustomobject]@{ Valid = $true; Sequence = 2; Tile = 7; Tmem = 0x00; Image = 0x1be0; Texels = 256; Dxt = 1024 }
)
$pondTexel1Load = Find-LatestTmemLoad -Loads $pondLoads -Tmem 0x40
$pondTexel0Load = Find-LatestTmemLoad -Loads $pondLoads -Tmem 0x00
Assert-True ($null -ne $pondTexel1Load -and $pondTexel1Load.Tile -eq 6 -and $pondTexel1Load.Image -eq 0x1e10) 'Dream Land TEXEL1 did not resolve the tile-6 TMEM 0x40 load.'
Assert-True ($null -ne $pondTexel0Load -and $pondTexel0Load.Tile -eq 7 -and $pondTexel0Load.Image -eq 0x1be0) 'Dream Land TEXEL0 did not retain the later tile-7 TMEM 0 load.'
Assert-Equal (Get-LoadBlockDxtSourceWidth -Size 0 -Dxt $pondTexel1Load.Dxt -FallbackWidth 128) 32 'Dream Land TEXEL1 transfer-size load did not decode through its CI4 render tile.'
Assert-Equal (Get-CompactTextureLoadTexels 0xffff) 0xffff 'Largest compact TMEM load was rejected.'
Assert-Equal (Get-CompactTextureLoadTexels 0x10000) 0 'Oversized TMEM load did not fail closed before u16 truncation.'
$pondTexel0 = Convert-N64Rgba16ToDs -Color 0x0221
$pondTexel1 = Convert-N64Rgba16ToDs -Color 0xFE10 -PreserveTransparentRgb $true
Assert-Equal (Convert-N64Rgba16ToDs -Color 0xFE10) 0 'Ordinary transparent RGBA16 conversion no longer clears hidden RGB.'
Assert-Equal $pondTexel1 ([uint16](31 -bor (24 -shl 5) -bor (8 -shl 10))) 'Composite RGBA16 conversion discarded transparent source RGB.'
Assert-Equal (Blend-Texel01Rgb5551 -Texel0 $pondTexel0 -Texel1 $pondTexel1 -Fraction 0x72) 0xB1EE 'Dream Land primitive-LOD texture blend changed.'
$coverageCount = 0
for ($coverageY = 0; $coverageY -lt 4; $coverageY++) {
    for ($coverageX = 0; $coverageX -lt 4; $coverageX++) {
        $coverageColor = Blend-Texel01Rgb5551 -Texel0 0 -Texel1 0x8000 -Fraction 0x40 -X $coverageX -Y $coverageY
        if (($coverageColor -band 0x8000) -ne 0) { $coverageCount++ }
    }
}
Assert-Equal $coverageCount 4 'Dream Land A1 coverage approximation did not retain the source quarter-alpha mean.'
$pondMObj = Convert-MObjSubMixedFields -Pad00 0x0202 -Fmt 0 -Siz 0 -Flags 0x0200 -BlockFmt 0x6b -BlockSiz 0
Assert-True ($pondMObj.Pad00 -eq 0 -and $pondMObj.Fmt -eq 2 -and $pondMObj.Siz -eq 2 -and $pondMObj.Flags -eq 0x006b -and $pondMObj.BlockFmt -eq 2 -and $pondMObj.BlockSiz -eq 0) 'Dream Land water MObjSub mixed fields did not recover the source layout.'
$whispyMObj = Convert-MObjSubMixedFields -Pad00 0x0202 -Fmt 0 -Siz 0 -Flags 0x0200 -BlockFmt 1 -BlockSiz 0
Assert-True ($whispyMObj.Pad00 -eq 0 -and $whispyMObj.Fmt -eq 2 -and $whispyMObj.Siz -eq 2 -and $whispyMObj.Flags -eq 1 -and $whispyMObj.BlockFmt -eq 2 -and $whispyMObj.BlockSiz -eq 0) 'Dream Land Whispy MObjSub false-native layout was not disambiguated.'
# FoxModel DLs 0x2718/0x2818 use an 8x8 CI4 render tile over a physical
# 16-texel row (dxt=0x800). The right half is row padding; compact width-8
# stepping would incorrectly alternate real and zero rows.
$foxTailRow = [byte[]](0x55, 0x8f, 0xff, 0x23, 0, 0, 0, 0)
$foxTailNative = [byte[]]($foxTailRow + $foxTailRow)
$foxTailO2R = [byte[]](0x23, 0xff, 0x8f, 0x55, 0, 0, 0, 0,
                       0x23, 0xff, 0x8f, 0x55, 0, 0, 0, 0)
Assert-Equal (Get-TextureNibble $foxTailNative 16 'Native') 5 'Fox tail native row 1 did not start at the DXT-derived stride.'
Assert-Equal (Get-TextureNibble $foxTailO2R 16 'O2R') 5 'Fox tail O2R row 1 did not preserve CI4 lane decoding at the DXT-derived stride.'
Assert-Equal (Get-TextureNibble $foxTailNative 8 'Native') 0 'Fox tail compact-width control no longer reaches row padding.'
Assert-Equal (Get-TextureNibble $foxTailO2R 8 'O2R') 0 'Fox tail O2R compact-width control no longer reaches row padding.'
# StagePupupuFile2.c:651-679 and objdisplay.c:1432-1499 produce this
# render-tile origin and gSPTexture scale for the 32x64 CI4 surface.
Assert-Equal (Convert-N64TexCoordToDsT16 -Coord 2048 -Scale 0xCCCC -Origin 205) -1 'Dream Land CI4 vertex S=2048 did not land at the source tile edge.'
Assert-Equal (Convert-N64TexCoordToDsT16 -Coord 3072 -Scale 0xCCCC -Origin 205) 408 'Dream Land CI4 vertex S=3072 did not preserve its source tile-relative coordinate.'
# Dream Land file 104 DL 0x0820 uses a 64-wide CI4 upload, mask 6,
# MIRROR|CLAMP, and a 128-wide SetTileSize extent. The physical mask period
# repeats inside the logical tile; clamp still applies at the logical edge.
Assert-True (Test-N64MaskedClampNeedsWrap -Mode 3 -Mask 6 -UploadExtent 64 -TileExtent 128) 'Dream Land mirrored canopy axis did not enable masked repeat inside its logical tile.'
Assert-True (Test-N64MaskedClampNeedsWrap -Mode 2 -Mask 5 -UploadExtent 32 -TileExtent 192) 'Dream Land repeated stage axis did not enable masked repeat inside its logical tile.'
Assert-True (-not (Test-N64MaskedClampNeedsWrap -Mode 2 -Mask 5 -UploadExtent 32 -TileExtent 32)) 'Ordinary 32-wide clamped texture incorrectly enabled masked repeat.'
# File 104's first star deliberately extends beyond its 16x16 tile. The RSP
# keeps these vertex varyings linear; logical clamp belongs after interpolation.
Assert-Equal (Convert-N64TexCoordToDsT16 -Coord -134 -Scale 0xFFFF -Origin 0 -Offset 16) -51 'Dream Land star negative S was clamped before interpolation.'
Assert-Equal (Convert-N64TexCoordToDsT16 -Coord 2 -Scale 0xFFFF -Origin 0 -Offset 16) 16 'Dream Land star low T conversion changed.'
Assert-Equal (Convert-N64TexCoordToDsT16 -Coord 256 -Scale 0xFFFF -Origin 0 -Offset 16) 143 'Dream Land star center S conversion changed.'
Assert-Equal (Convert-N64TexCoordToDsT16 -Coord 784 -Scale 0xFFFF -Origin 0 -Offset 16) 407 'Dream Land star high T was clamped before interpolation.'
Assert-Equal (Convert-N64TexCoordToDsT16 -Coord 646 -Scale 0xFFFF -Origin 0 -Offset 16) 338 'Dream Land star high S was clamped before interpolation.'
# The star's 8-wide MIRROR mask is materialized across its independent
# 16-wide logical clamp extent, so DS clamp can happen after interpolation.
for ($i = 0; $i -lt 8; $i++) {
    Assert-Equal (Convert-N64MaskedTextureAddress -Coord $i -Mode 3 -Mask 3) $i "Dream Land star first mask period changed at texel $i."
    Assert-Equal (Convert-N64MaskedTextureAddress -Coord ($i + 8) -Mode 3 -Mask 3) (7 - $i) "Dream Land star mirrored mask period changed at texel $($i + 8)."
}
Assert-True (Test-N64MaskedClampMaterialization -Mode 3 -Mask 3 -SourceExtent 16 -TileExtent 16) 'Dream Land 8-in-16 star logical clamp was not materialized.'
Assert-True (Test-N64MaskedClampMaterialization -Mode 3 -Mask 5 -SourceExtent 32 -TileExtent 64) 'Dream Land 32-in-64 platform logical clamp was not materialized.'
Assert-True (Test-N64MaskedClampMaterialization -Mode 3 -Mask 6 -SourceExtent 64 -TileExtent 128) 'Dream Land 64-in-128 canopy logical clamp was not materialized.'
Assert-True (-not (Test-N64MaskedClampMaterialization -Mode 2 -Mask 5 -SourceExtent 32 -TileExtent 192)) 'Dream Land 192-wide island silently exceeded the bounded logical-clamp upload path.'
Assert-Equal (Convert-N64MaskedTextureAddress -Coord 32 -Mode 3 -Mask 5) 31 'Dream Land platform mirror boundary did not reverse the second mask period.'
Assert-Equal (Convert-N64MaskedTextureAddress -Coord 63 -Mode 3 -Mask 5) 0 'Dream Land platform logical edge did not resolve to the mirrored source edge.'
Assert-Equal (Convert-N64MaskedTextureAddress -Coord 127 -Mode 3 -Mask 6) 0 'Dream Land canopy logical edge did not resolve to the mirrored source edge.'
$taskman = Get-Content (Join-Path $root 'src/port/taskman_seam.c') -Raw
Assert-True ($renderer.Contains('ndsRendererMtxCellS16p16')) 'Renderer matrix unpack helper is missing.'
Assert-True ($renderer.Contains('ndsRendererTransformVertex20p12')) 'Renderer vertex transform helper is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_OP_MTX 0xdau')) 'Renderer G_MTX opcode support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_OP_POPMTX 0xd8u')) 'Renderer G_POPMTX opcode support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_MTX_PUSH_XOR')) 'Renderer G_MTX push-flag decode is missing.'
Assert-True ($renderer.Contains('ndsRendererApplyMatrixCommand')) 'Renderer G_MTX traversal state handler is missing.'
Assert-True ($renderer.Contains('ndsRendererApplyPopMatrixCommand')) 'Renderer G_POPMTX traversal state handler is missing.'
Assert-True ($renderer.Contains('ndsRendererApplyMvpRecalcCommand')) 'Renderer G_SPECIAL_1 MVP-recalc traversal state handler is missing.'
Assert-True ($renderer.Contains('ndsRendererApplyMatrixMoveWordCommand')) 'Renderer G_MOVEWORD matrix traversal state handler is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_OP_SPECIAL_1 0xd5u')) 'Renderer G_SPECIAL_1 opcode support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_MOVEWORD_MATRIX')) 'Renderer G_MOVEWORD matrix target support is missing.'
Assert-True ($renderer.Contains('modelview_stack')) 'Renderer modelview stack state is missing.'
Assert-True ($renderer.Contains('ndsRendererDecodeInputVertex')) 'Renderer Vtx payload decode helper is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordTransformedTriangle')) 'Renderer transformed triangle readiness handler is missing.'
Assert-True ($renderer.Contains('ndsRendererSubmitHardwareTriangle')) 'Renderer hardware triangle submission handler is missing.'
Assert-True ($renderer.Contains('ndsRendererInitTraversalState')) 'Renderer initial matrix seed path is missing.'
Assert-True ($renderer.Contains('config->initial_projection')) 'Renderer projection seed config is missing.'
Assert-True ($renderer.Contains('config->initial_modelview')) 'Renderer modelview seed config is missing.'
Assert-True ($renderer.Contains('hardware_matrix_seed_count')) 'Renderer hardware matrix seed stats are missing.'
Assert-True ($renderer.Contains('glVertex3v16')) 'Renderer hardware path does not submit raw vertices through GX.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_WORLD_UNIT_SHIFT')) 'Renderer hardware world-unit conversion is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareVertexCoord')) 'Renderer hardware v16 vertex encoder is missing.'
Assert-True ($renderer.Contains('glTexImage2D')) 'Renderer hardware texture upload path is missing.'
Assert-True ($renderer.Contains('case 128u: value = TEXTURE_SIZE_128')) 'Renderer cannot materialize a source logical texture extent at its declared 128-texel bound.'
Assert-True ($renderer.Contains('glTexCoord2t16')) 'Renderer hardware texture coordinates are missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_FMT_RGBA16')) 'Renderer RGBA16 texture format support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_SIZ_32B')) 'Renderer RGBA32 texture size support is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareConvertRgba32')) 'Renderer RGBA32 texture conversion helper is missing.'
Assert-True ($renderer.Contains('texels * sizeof(u32)')) 'Renderer RGBA32 texture source-byte sizing is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_FMT_CI')) 'Renderer CI texture format support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_FMT_IA')) 'Renderer IA texture format support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_FMT_I16')) 'Renderer I16 texture format support is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareConvertI(')) 'Renderer I texture conversion helper is missing.'
Assert-True ($renderer.Contains('intensity4 * 0x11u')) 'Renderer I4 texture expansion is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareConvertIA')) 'Renderer IA texture conversion helper is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordLoadTlut')) 'Renderer TLUT state tracking is missing.'
Assert-True ($renderer.Contains('glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA')) 'Renderer hardware upload does not preserve converted texture alpha.'
Assert-True ($renderer.Contains('texture_tlut_image')) 'Renderer CI texture palette pointer tracking is missing.'
Assert-True ($renderer.Contains('texture_render_tile_size')) 'Renderer render-tile pixel size tracking is missing.'
Assert-True ($renderer.Contains('hardware_texture_upload_count')) 'Renderer hardware texture upload stats are missing.'
Assert-True ($rendererHeader.Contains('NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED')) 'Renderer texture data-layout enum is missing.'
Assert-True ($rendererHeader.Contains('texture_data_layout')) 'Renderer config does not carry texture data layout.'
Assert-True ($renderer.Contains('ndsRendererReadTextureByte')) 'Renderer byte-packed texture reader is not centralized.'
Assert-True ($renderer.Contains('ndsRendererReadTexturePackedNibble')) 'Renderer packed-nibble texture reader is not centralized.'
Assert-True ($renderer.Contains('ndsRendererReadTextureHalfword')) 'Renderer halfword texture reader is not centralized.'
Assert-True ($renderer.Contains('logical_index ^ 3u')) 'Renderer O2R byte-lane correction is missing.'
Assert-True ($renderer.Contains('logical_index ^ 1u')) 'Renderer O2R halfword-lane correction is missing.'
Assert-True ($renderer.Contains('key.data_layout')) 'Renderer texture cache key does not distinguish data layout.'
Assert-True ($renderer.Contains('source_physical_bytes')) 'Renderer texture source range does not validate the physical lane-mapped word span.'
Assert-True ($renderer.Contains('gNdsRendererProfileTextureLaneByteMap')) 'Renderer texture-lane diagnostics are missing.'
Assert-True (-not ($renderer -match 'texels\s*\[\s*index\s*\]')) 'Renderer byte-packed texture path returned to direct texels[index] reads.'
Assert-True (-not ($renderer -match 'texels\s*\[\s*index\s*>>\s*1\s*\]')) 'Renderer packed-nibble texture path returned to direct texels[index >> 1] reads.'
Assert-True ($renderer.Contains('key.render_tmem = render_tile->tmem')) 'Renderer hardware texture cache key is not wired directly to render-tile TMEM state.'
Assert-True ($renderer.Contains('key.image_width = primary_image_width')) 'Renderer hardware texture cache key is missing resolved TEXEL0 source image width.'
Assert-True ($renderer.Contains('palette_base = render_tile->palette * 16u')) 'Renderer CI4 palette bank is not applied from the active render tile.'
Assert-True ($renderer.Contains('key.load_tile = primary_load_tile')) 'Renderer hardware texture cache key is missing resolved TEXEL0 load-tile state.'
Assert-True ($renderer.Contains('key.load_uls = primary_load_uls')) 'Renderer hardware texture cache key is missing resolved TEXEL0 load ULS state.'
Assert-True ($renderer.Contains('key.load_dxt = primary_load_dxt')) 'Renderer hardware texture cache key is missing resolved TEXEL0 load DXT state.'
Assert-True ($renderer.Contains('NDS_RENDERER_G_TX_DXT_ONE + dxt - 1u')) 'Renderer LOADBLOCK source rows do not reconstruct 1.11 DXT qwords.'
Assert-True ($renderer.Contains('source_width = ndsRendererHardwareTextureLinePixels(size, qwords)')) 'Renderer LOADBLOCK source width does not use the DXT-derived logical row stride.'
Assert-True ($renderer.Contains('texture_load_kind = NDS_RENDERER_TEXTURE_LOADBLOCK')) 'Renderer G_LOADBLOCK does not mark the current texture load kind.'
Assert-True ($renderer.Contains('texture_load_kind = NDS_RENDERER_TEXTURE_LOADTILE')) 'Renderer G_LOADTILE does not mark the current texture load kind.'
Assert-True ($renderer.Contains('ndsRendererHardwareTextureSourceWidthPixels')) 'Renderer hardware upload does not derive source row stride.'
Assert-True ($renderer.Contains('primary_load_kind == NDS_RENDERER_TEXTURE_LOADTILE')) 'Renderer hardware upload does not limit resolved TEXEL0 source row stride to G_LOADTILE.'
Assert-True ($renderer.Contains('source_origin_s = primary_load_uls >> 2')) 'Renderer hardware upload does not honor resolved TEXEL0 LoadTile S origin.'
Assert-True ($renderer.Contains('source_origin_t = primary_load_ult >> 2')) 'Renderer hardware upload does not honor resolved TEXEL0 LoadTile T origin.'
Assert-True ($renderer.Contains('((source_origin_t + source_y) * source_width)') -and $renderer.Contains('source_origin_s + source_x')) 'Renderer hardware upload does not sample source sub-rect rows through resolved mask addressing.'
Assert-True ($renderer.Contains('((s64)coord * (s64)scale) >> 17')) 'Renderer hardware texcoords do not scale N64 vertex coordinates into DS t16 first.'
Assert-True ($renderer.Contains('(s64)origin << 2')) 'Renderer hardware texcoords do not convert the 10.2 tile origin directly to DS t16.'
Assert-True (-not $renderer.Contains('((s64)origin << 3)')) 'Renderer hardware texcoords still scale a tile origin converted into vertex-coordinate units.'
Assert-True ($renderer.Contains('render_tile->uls, render_tile->ult')) 'Renderer hardware texcoord submission is not using the active render-tile origin.'
Assert-True ($renderer.Contains('NDS_RENDERER_OP_LOADTILE 0xf4u')) 'Renderer G_LOADTILE opcode decode is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordLoadTile')) 'Renderer G_LOADTILE state recorder is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_TEXTURE_LOADTILE')) 'Renderer G_LOADTILE texture-state mask is missing.'
Assert-True ($renderer.Contains('(primary_load_kind << 8)')) 'Renderer texture cache key does not distinguish resolved TEXEL0 LOADBLOCK and LOADTILE state.'
Assert-True (-not $renderer.Contains('if (stats->texture_load_texels == 0)')) 'Renderer load-block state still freezes on the first texture load.'
Assert-True (-not $renderer.Contains('stats->texture_tile_width != 0) &&')) 'Renderer tile-size state still freezes on the first tile size.'
Assert-True ($rendererHeader.Contains('texture_tiles[NDS_RENDERER_TILE_COUNT]')) 'Renderer texture state is not tracked per GBI tile.'
Assert-True ($rendererHeader.Contains('texture_loads[NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT]')) 'Renderer does not retain the recent TEXEL0/TEXEL1 load pair.'
Assert-True ($rendererHeader.Contains('#define NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT 2u')) 'Renderer TMEM history grew beyond the source pond pair and legacy stack budget.'
Assert-True (-not $relocRendererDL.Contains('NDSRendererStats stats[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED]')) 'Fighter multi-DL fixture restored a stack array of enlarged renderer stats.'
Assert-True (-not $relocMPCollision.Contains('NDSRendererStats stats[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED]')) 'Inishie preview restored a stack array of enlarged renderer stats.'
Assert-True ($rendererHeader.Contains('prim_lod_fraction')) 'Renderer does not retain SETPRIMCOLOR primitive LOD fraction.'
Assert-True ($renderer.Contains('ndsRendererCaptureTextureLoad')) 'Renderer does not snapshot texture-image/load provenance when TMEM is filled.'
Assert-True ($renderer.Contains('primary_load = ndsRendererHardwareFindTextureLoadForTmem(')) 'Renderer does not resolve TEXEL0 through its render-tile TMEM load.'
Assert-True ($renderer.Contains('load = ndsRendererHardwareFindTextureLoadForTmem(stats, tile->tmem)')) 'Renderer does not resolve TEXEL1 through its render-tile TMEM load.'
Assert-True ($renderer.Contains('(uintptr_t)primary_image')) 'Renderer texture upload still resolves TEXEL0 through mutable SETTIMG state.'
Assert-True ($renderer.Contains('stats->texture_load_texels <= 0xffffu')) 'Renderer compact TMEM capture does not reject oversized load rectangles.'
Assert-True ($renderer.Contains('ndsRendererHardwareUsesTexel01Lerp')) 'Renderer does not recognize the source TEXEL0/TEXEL1 lerp contract.'
Assert-True ($renderer.Contains('ndsRendererHardwarePrepareTexel1Source')) 'Renderer does not prepare the second source texture independently.'
Assert-True ($renderer.Contains('key.texel1_image = load->image')) 'Renderer composite cache key omits the TEXEL1 source image.'
Assert-True ($renderer.Contains('key.prim_lod_fraction = stats->prim_lod_fraction')) 'Renderer composite cache key omits the source blend fraction.'
Assert-True ($renderer.Contains('ndsRendererHardwareBlendTexel01')) 'Renderer does not precompose the source TEXEL0/TEXEL1 result for DS hardware.'
Assert-True ($renderer.Contains('color, color1, stats->prim_lod_fraction')) 'Renderer texture upload loop does not consume both source texels and the original fraction.'
Assert-True ($renderer.Contains('preserve_transparent_rgb')) 'Renderer composite decode does not preserve transparent source RGB before color lerp.'
Assert-True ($renderer.Contains('ndsRendererHardwareFindTexel1RefreshTexture')) 'Renderer does not retain one allocation across source water animation updates.'
Assert-True ($renderer.Contains('entry->last_used_frame !=')) 'Renderer does not prevent incompatible same-frame composite allocation reuse.'
Assert-True ($renderer.Contains('glGetTexturePointer(entry->name)')) 'Renderer fraction refresh does not resolve the resident DS texture allocation.'
Assert-True ($renderer.Contains('dmaCopyWords(0, texture, vram_address, texture_bytes)')) 'Renderer fraction refresh does not update resident texture VRAM.'
Assert-True ($renderer.Contains('glDeleteTextures(1, &entry->name)')) 'Renderer cache eviction does not release the old libnds texture allocation.'
Assert-Equal ([regex]::Matches($taskman, 'gNdsRendererProfileTexel1FractionRefreshCount = 0;').Count) 1 'Renderer composite refresh proof is still cleared every presented frame.'
Assert-Equal ([regex]::Matches($taskman, 'gNdsRendererProfileTextureCacheEvictCount = 0;').Count) 1 'Renderer texture-eviction proof is still cleared every presented frame.'
Assert-True ($renderer.Contains('key.texel1_image_format = load->image_format')) 'Renderer composite cache key omits TEXEL1 image format.'
Assert-True ($renderer.Contains('key.texel1_image_size = load->image_size')) 'Renderer composite cache key omits TEXEL1 transfer size.'
Assert-True ($renderer.Contains('key.texel1_load_kind = load->load_kind')) 'Renderer composite cache key omits TEXEL1 load kind.'
Assert-True ($renderer.Contains('key.texel1_render_line = tile->line')) 'Renderer composite cache key omits TEXEL1 render line.'
Assert-True ($relocAssets.Contains('ndsRelocCopyMObjSubForAttachment')) 'Relocation backend does not expose mixed-width MObjSub attachment copies.'
Assert-True ($relocAssets.Contains('ndsRelocNormalizeMObjSubWordSwapped(dst)')) 'MObjSub attachment copy does not restore O2R mixed-width lanes.'
Assert-True ($relocAssets.Contains('loaded = ndsRelocFindLoadedFileContaining(src, sizeof(*src))')) 'MObjSub attachment normalization does not require O2R loaded-file provenance.'
Assert-True ($relocAssets.Contains('ndsRelocMObjSubAlreadyNormalized')) 'MObjSub attachment normalization does not preserve explicitly normalized loaded records.'
Assert-True ($objAnimImport.Contains('#define gcAddMObjAll ndsBaseGcAddMObjAll')) 'BattleShip objanim import does not fence the original MObj attachment entrypoint.'
Assert-True ($objAnimImport.Contains('gcAddMObjForDObj(dobj, &normalized_mobjsub)')) 'Live gcAddMObjAll traversal does not attach the normalized material copy.'
Assert-True ($objAnimImport -match '(?s)gNdsMObjSubAttachFailCount\+\+;.*?continue;.*?gcAddMObjForDObj') 'Invalid loaded MObjSub conversion no longer fails closed before live attachment.'
Assert-True ($renderer.Contains('ndsRendererSyncTextureTile(stats)')) 'Renderer per-tile texture state is not synced into the hardware bind view.'
Assert-True ($renderer.Contains('key.tile_lrs = render_tile->lrs') -and $renderer.Contains('key.tile_lrt = render_tile->lrt')) 'Renderer cache key is missing active render-tile rectangle end coordinates.'
Assert-True ($renderer.Contains('gNdsRendererProfileTextureCacheAliasAvoidCount')) 'Renderer cache alias diagnostic counter is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareRecordUseTextureReject')) 'Renderer texture-use rejection diagnostics are missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_USETEX_REJECT_NO_TEXEL0')) 'Renderer texture-use diagnostics do not distinguish missing TEXEL0 combine state.'
Assert-True ($renderer.Contains('ndsRendererHardwareTextureImplicitStateOn')) 'Renderer texture-use diagnostics/fix is missing strict implicit texture-on evidence.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE')) 'Renderer implicit texture-on path is missing a source-scale fallback.'
Assert-True ($renderer.Contains('tile->width = 0u;')) 'Renderer per-tile size state does not clear stale width.'
Assert-True ($renderer.Contains('tile->height = 0u;')) 'Renderer per-tile size state does not clear stale height.'
Assert-True ($renderer.Contains('ndsRendererHardwareColorSource')) 'Renderer hardware material color source helper is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareUseMaterialColor')) 'Renderer hardware material color presence helper is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareUsesLitPrimitiveModulate')) 'Renderer hardware direct lit-primitive modulation helper is missing.'
Assert-True (Test-LitPrimitiveModulate ([Convert]::ToUInt32('fc327e05', 16)) ([Convert]::ToUInt32('ff17fdff', 16))) 'Fox/Mario PRIMITIVE * SHADE combine did not retain primitive material color.'
Assert-True (-not (Test-LitPrimitiveModulate ([Convert]::ToUInt32('fc127e05', 16)) ([Convert]::ToUInt32('ff17f3ff', 16)))) 'TEXEL0 * SHADE combine incorrectly selected a material color.'
Assert-True (-not (Test-LitPrimitiveModulate ([Convert]::ToUInt32('fcfffe05', 16)) ([Convert]::ToUInt32('ff167dff', 16)))) 'SHADE-only combine incorrectly selected a material color.'
Assert-True (Test-LitPrimitiveModulate ([Convert]::ToUInt32('fc327e05', 16)) ([Convert]::ToUInt32('ff17fdff', 16)) $true) 'BattleShip white-ENV two-cycle pass-through discarded PRIMITIVE * SHADE.'
Assert-True (-not (Test-LitPrimitiveModulate ([Convert]::ToUInt32('fc327e05', 16)) ([Convert]::ToUInt32('ff17fdff', 16)) $true ([Convert]::ToUInt32('fffefeff', 16)))) 'Non-white two-cycle ENV incorrectly passed material modulation unchanged.'
Assert-True (-not (Test-LitPrimitiveModulate ([Convert]::ToUInt32('fc327e04', 16)) ([Convert]::ToUInt32('ff17fdff', 16)) $true)) 'Non-pass-through second cycle incorrectly retained first-cycle material modulation.'
Assert-True ($renderer.Contains('ndsRendererCombineOutputUsesColor')) 'Renderer hardware material color selection is not limited to combine output slots.'
Assert-True ($renderer.Contains('NDS_RENDERER_CYCLETYPE_MASK')) 'Renderer hardware cycle-type mask is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_CYC_2CYCLE')) 'Renderer hardware 2-cycle combiner constant is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_CCMUX_COMBINED')) 'Renderer hardware color combined mux fallback is missing.'
Assert-True ($renderer.Contains('ndsRendererCombineSecondOutputUsesColor')) 'Renderer hardware 2-cycle output color selection is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareOutputUsesColor')) 'Renderer hardware material color selection does not honor current cycle type.'
Assert-True ($renderer.Contains('w0, w1, NDS_RENDERER_CCMUX_COMBINED')) 'Renderer hardware 2-cycle color output does not fall back through COMBINED.'
Assert-True ($renderer.Contains('use_material_color != FALSE')) 'Renderer hardware material color selection still depends on a nonzero color value.'
Assert-True ($renderer.Contains('ndsRendererHardwareUseVertexColor')) 'Renderer hardware combine shade/white color selection helper is missing.'
Assert-True ($renderer.Contains('glColor3b(0xffu, 0xffu, 0xffu)')) 'Renderer hardware path does not tint non-shade combine output white.'
Assert-True ($renderer.Contains('ndsRendererHardwareAlpha')) 'Renderer hardware material alpha helper is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_ACMUX_COMBINED')) 'Renderer hardware alpha combined mux fallback is missing.'
Assert-True ($renderer.Contains('ndsRendererCombineSecondOutputUsesAlpha')) 'Renderer hardware 2-cycle output alpha selection is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareOutputUsesAlpha')) 'Renderer hardware material alpha selection does not honor current cycle type.'
Assert-True ($renderer.Contains('NDS_RENDERER_ACMUX_1')) 'Renderer hardware alpha constant-one mux is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_ACMUX_0')) 'Renderer hardware alpha constant-zero mux is missing.'
Assert-True ($renderer.Contains('0u : 0xffu')) 'Renderer hardware alpha does not map constant-zero mux to transparent alpha.'
Assert-True (-not $renderer.Contains('if (stats->texture_combine_w0 == 0)')) 'Renderer combine state recorder still keeps first combine instead of current combine.'
Assert-True ($renderer.Contains('ndsRendererHardwareUseDecal')) 'Renderer hardware combine decal helper is missing.'
Assert-True ($renderer -match '(?s)static s32 ndsRendererHardwareUseTexture.*ndsRendererHardwareOutputUsesAlpha\(\s*stats, NDS_RENDERER_ACMUX_TEXEL0\) != FALSE') 'Renderer hardware texture binding does not honor TEXEL0 alpha-only combines.'
Assert-True ($renderer.Contains('NDS_RENDERER_MDSFT_TEXTFILT')) 'Renderer hardware texture-filter othermode constant is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareTextureFilterOffset')) 'Renderer hardware texture-filter coordinate offset helper is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareTextureMaskedClampNeedsWrap')) 'Renderer masked clamp/repeat ownership helper is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareTextureMaterializesMaskedClamp')) 'Renderer masked logical-clamp materialization helper is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareTextureMaskedAddress')) 'Renderer masked logical-clamp source-address helper is missing.'
Assert-True ($renderer.Contains('(entry->params & GL_TEXTURE_WRAP_S) != 0u')) 'Renderer diagnostic S sampler does not use effective DS texture parameters.'
Assert-True ($renderer.Contains('(entry->params & GL_TEXTURE_WRAP_T) != 0u')) 'Renderer diagnostic T sampler does not use effective DS texture parameters.'
Assert-True ($renderer.Contains('render_tile->masks, upload_width')) 'Renderer S sampler state does not compare the N64 mask period with the uploaded extent.'
Assert-True (-not $renderer.Contains('max_t16 = ((s64)extent << 4) - 1')) 'Renderer still clamps vertex texture coordinates before raster interpolation.'
Assert-True ($renderer.Contains('NDS_RENDERER_MDSFT_TEXTPERSP')) 'Renderer hardware texture-perspective othermode constant is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareUseTextureMatrix')) 'Renderer hardware texture-perspective GX texgen helper is missing.'
Assert-True ($renderer.Contains('TEXGEN_TEXCOORD : TEXGEN_OFF')) 'Renderer hardware texture upload does not honor source texture-perspective state.'
Assert-True ($renderer.Contains('NDS_RENDERER_BLEND_ALPHA_CYCLE1_SHIFT')) 'Renderer hardware blend alpha-memory cycle-1 field is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_BLEND_ALPHA_CYCLE2_SHIFT')) 'Renderer hardware blend alpha-memory cycle-2 field is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareBlendAlphaUsesMemory')) 'Renderer hardware alpha does not honor cycle-aware blend alpha-memory state.'
Assert-True ($renderer.Contains('NDS_RENDERER_ALPHA_COMPARE_THRESHOLD')) 'Renderer hardware alpha-compare threshold constant is missing.'
Assert-True ($renderer.Contains('glEnable(GL_ALPHA_TEST)')) 'Renderer hardware path does not enable DS alpha test for source threshold compare.'
Assert-True ($renderer.Contains('stats->blend_color = w1')) 'Renderer SETBLENDCOLOR state is not recorded.'
Assert-True ($renderer.Contains('stats->blend_color & 0xffu')) 'Renderer hardware alpha-test threshold does not use source blend-color alpha.'
Assert-True ($renderer.Contains('glDisable(GL_ALPHA_TEST)')) 'Renderer hardware path does not clear DS alpha test when source threshold compare is inactive.'
Assert-True ($renderer.Contains('NDS_RENDERER_GEOM_RESET_MODE')) 'Renderer BattleShip reset geometry seed is missing.'
Assert-True ($renderer.Contains('stats->geometry_mode = NDS_RENDERER_GEOM_RESET_MODE')) 'Renderer stats do not start from BattleShip reset geometry state.'
Assert-True ($renderer.Contains('stats->othermode_h = NDS_RENDERER_TP_PERSP | NDS_RENDERER_TF_BILERP')) 'Renderer stats do not start from BattleShip reset texture-perspective/filter state.'
Assert-True ($renderer.Contains('ndsRendererHardwareClipVertex')) 'Renderer hardware no-z projected vertex path is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_PROJECTED_DEPTH_START')) 'Renderer hardware projected-depth counter is missing.'
Assert-True ($renderer.Contains('hardware_projected_depth_triangle_count')) 'Renderer hardware projected-depth stats are missing.'
Assert-True ($renderer.Contains('hardware_zbuffer_triangle_count')) 'Renderer hardware z-buffer stats are missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_ZMODE_DEC')) 'Renderer hardware decal depth mode constant is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_DECAL_DEPTH_BIAS')) 'Renderer hardware decal depth bias is missing.'
Assert-True ($renderer.Contains('hardware_decal_depth_triangle_count')) 'Renderer hardware decal-depth stats are missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_OP_SETPRIMDEPTH 0xeeu')) 'Renderer G_SETPRIMDEPTH opcode decode is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_ZSOURCE_PRIM')) 'Renderer G_ZS_PRIM source-depth state is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordPrimDepth')) 'Renderer primitive-depth state recorder is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareUsePrimDepth')) 'Renderer hardware primitive-depth branch is missing.'
Assert-True ($renderer.Contains('hardware_prim_depth_triangle_count')) 'Renderer hardware primitive-depth stats are missing.'
Assert-True ($renderer.Contains('stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER')) 'Renderer hardware path does not branch on source G_ZBUFFER state.'
Assert-True ($renderer.Contains('NDS_RENDERER_GEOM_FOG')) 'Renderer hardware fog geometry flag is missing.'
Assert-True ($renderer.Contains('poly_fmt |= POLY_FOG')) 'Renderer hardware path does not map source G_FOG to DS POLY_FOG.'
Assert-True ($renderer.Contains('NDS_RENDERER_GEOM_LIGHTING')) 'Renderer hardware lighting geometry flag is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_OP_MOVEMEM 0xdcu')) 'Renderer G_MOVEMEM opcode decode is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_MOVEMEM_LIGHT')) 'Renderer G_MV_LIGHT state target is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordLightMoveMem')) 'Renderer light MOVEMEM state recorder is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_MOVEWORD_LIGHTCOL')) 'Renderer G_MW_LIGHTCOL state target is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordLightColorMoveWord')) 'Renderer light-color move-word state recorder is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareLitShadeColor')) 'Renderer lit SHADE path is missing.'
$rendererAdapter = Get-Content (Join-Path $root 'src/port/reloc_backend_renderer_dl.c') -Raw
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterPackColor(&mobj->sub.light1color)')) 'Fighter material light 1 still depends on host-endian SYColorPack.pack layout.'
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterPackColor(&mobj->sub.light2color)')) 'Fighter material light 2 still depends on host-endian SYColorPack.pack layout.'
Assert-True ($rendererAdapter.Contains('((u32)color->s.r << 24)')) 'Fighter material color repack does not place R in the N64 high byte.'
Assert-True ($rendererAdapter.Contains('((u32)color->s.g << 16)')) 'Fighter material color repack does not place G in the N64 second byte.'
$marioModel = Get-Content (Join-Path $root 'decomp/BattleShip-main/decomp/src/relocData/296_MarioModel.c') -Raw
Assert-True ($marioModel.Contains('0x83271400,  /* RGBA(131, 39, 20, 0) */')) 'Mario source shoe diffuse-light fixture changed.'
Assert-True ($marioModel.Contains('0x240F1100,  /* RGBA(36, 15, 17, 0) */')) 'Mario source shoe ambient-light fixture changed.'
Assert-True ($renderer.Contains('NDS_RENDERER_MOVEWORD_FOG')) 'Renderer G_MW_FOG state target is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordFogMoveWord')) 'Renderer fog move-word state recorder is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordFogColor')) 'Renderer fog color recorder is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareApplyFog')) 'Renderer hardware fog apply helper is missing.'
Assert-True ($renderer.Contains('glFogColor')) 'Renderer hardware path does not program DS fog color.'
Assert-True ($renderer.Contains('glFogDensity')) 'Renderer hardware path does not program DS fog density table.'
Assert-True ($renderer.Contains('NDS_RENDERER_GEOM_CULL_BACK')) 'Renderer hardware cull-back geometry flag is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwarePolyFmt')) 'Renderer hardware poly-format helper is missing.'
Assert-True ($renderer.Contains('config->initial_geometry_mode')) 'Renderer hardware initial geometry-mode seed is missing.'
Assert-True ($renderer.Contains('POLY_ID(poly_id)')) 'Renderer hardware polygon ID state is missing.'
Assert-True ($renderer.Contains('stats->texture_combine_count & NDS_RENDERER_POLY_ID_MASK')) 'Renderer hardware polygon ID does not follow combine sequencing.'
Assert-True ($renderer.Contains('stats->env_color = w1')) 'Renderer env-color material state tracking is missing.'
Assert-True (-not $renderer.Contains('NDS_RENDERER_HW_FALLBACK_SCALE')) 'Renderer hardware fallback scale returned.'
Assert-True ($renderer.Contains('input_vertices')) 'Renderer hardware raw vertex cache is missing.'
Assert-True ($renderer.Contains('sNdsRendererHardwareSubmitted')) 'Renderer hardware frame-submit latch is missing.'
Assert-True ($renderer.Contains('matrix_command_count')) 'Renderer matrix stats are missing.'
Assert-True ($renderer.Contains('matrix_pop_count')) 'Renderer matrix pop stats are missing.'
Assert-True ($renderer.Contains('transformed_vertex_count')) 'Renderer transformed vertex stats are missing.'
Assert-True ($renderer.Contains('transformed_triangle_count')) 'Renderer transformed triangle stats are missing.'
Assert-True ($renderer.Contains('hardware_triangle_count')) 'Renderer hardware triangle stats are missing.'
$platform = Get-Content (Join-Path $root 'src/nds/nds_platform.c') -Raw
Assert-True ($platform.Contains('MODE_5_3D')) 'Platform hardware renderer/BG overlay mode init is missing.'
Assert-True ($platform.Contains('VRAM_A_TEXTURE')) 'Platform texture VRAM bank A init is missing.'
Assert-True ($platform.Contains('VRAM_E_TEX_PALETTE')) 'Platform texture palette VRAM bank init is missing.'
Assert-True ($platform.Contains('glFlush(GL_TRANS_MANUALSORT)')) 'Platform hardware renderer manual-sort/Z-buffer frame flush is missing.'
Assert-True (-not $platform.Contains('GL_WBUFFERING')) 'Projected source depth must not be replaced by identity-matrix W buffering.'
Assert-True ($platform.Contains('ndsRendererHardwareConsumeSubmittedFrame')) 'Platform does not guard hardware flushes with the renderer submit latch.'
Assert-True ($platform.Contains('gNdsHardwareRendererFlushCount')) 'Platform hardware renderer flush diagnostic is missing.'
Assert-True ($platform.Contains('gNdsHardwareRendererPolyRamCount')) 'Platform hardware renderer polygon RAM diagnostic is missing.'
Assert-True ($platform.Contains('gNdsHardwareRendererVertexRamCount')) 'Platform hardware renderer vertex RAM diagnostic is missing.'
$makefile = Get-Content (Join-Path $root 'Makefile') -Raw
Assert-True ($makefile.Contains('NDS_RENDERER_HW_TRIANGLES ?= 0')) 'Makefile hardware renderer flag default is missing.'
Assert-True ($makefile.Contains('battleship_sys_matrix.c')) 'Makefile original sys/matrix import is missing.'
Assert-True ($makefile.Contains('battleship_sys_sintable.c')) 'Makefile original sine table import is missing.'
$rendererHeader = Get-Content (Join-Path $root 'include/nds/nds_renderer.h') -Raw
Assert-True ($rendererHeader.Contains('NDSRendererResolveData')) 'Renderer data resolver hook is missing.'
Assert-True ($rendererHeader.Contains('initial_projection')) 'Renderer config initial projection field is missing.'
Assert-True ($rendererHeader.Contains('initial_modelview')) 'Renderer config initial modelview field is missing.'
Assert-True ($rendererHeader.Contains('hardware_texture_ready_count')) 'Renderer hardware texture stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('NDS_RENDERER_TEXTURE_LOADTILE')) 'Renderer G_LOADTILE texture-state mask is missing from the public header.'
Assert-True ($rendererHeader.Contains('prim_depth_command_count')) 'Renderer primitive-depth stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('hardware_prim_depth_triangle_count')) 'Renderer hardware primitive-depth stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('matrix_mvp_recalc_count')) 'Renderer MVP-recalc stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('matrix_move_word_count')) 'Renderer matrix move-word stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('othermode_h')) 'Renderer current othermode-H state is missing from the public stats.'
Assert-True ($rendererHeader.Contains('transformed_vertices')) 'Renderer command transformed vertex cache exposure is missing.'
Assert-True ($rendererHeader.Contains('ndsRendererHardwareConsumeSubmittedFrame')) 'Renderer hardware submit-latch API is missing.'
$startupHeader = Get-Content (Join-Path $root 'include/nds/nds_startup.h') -Raw
Assert-True ($startupHeader.Contains('gNdsFighterDLAllDrawHardwareTextureReadyCount')) 'All-DL hardware texture diagnostics are missing from the startup header.'
Assert-True ($startupHeader.Contains('gNdsFighterDLAllDrawP0HardwareZBufferTriangleCount')) 'All-DL hardware z-buffer diagnostics are missing from the startup header.'
Assert-True ($startupHeader.Contains('gNdsStageGCDrawAllLoopHardwareTextureReadyCount')) 'Stage gcDrawAll hardware texture startup diagnostics are missing.'
Assert-True ($startupHeader.Contains('gNdsStageGCDrawAllLoopHardwareFighterTriangleCount')) 'Stage gcDrawAll hardware fighter diagnostics are missing from the startup header.'
$rendererAdapter = Get-Content (Join-Path $root 'src/port/reloc_backend_renderer_dl.c') -Raw
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterPrepareInitialMatrices')) 'Battle DL renderer matrix adapter is missing.'
Assert-True ($rendererAdapter.Contains('syMatrixTraRotRpyRSca')) 'Battle DL renderer does not route DObj prep through original matrix helpers.'
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterBuildDefaultBattleCameraMatrices')) 'Battle DL renderer default battle camera seed is missing.'
Assert-True (-not $rendererAdapter.Contains('NDS_RENDERER_ADAPTER_HW_WORLD_SCALE')) 'Battle DL renderer must keep source camera near/far planes in the CPU projection domain.'
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterMulBefore(projection, &incoming')) 'Battle DL renderer camera look-at no longer left-multiplies the loaded projection.'
Assert-True (-not $renderer.Contains('ndsRendererHardwareSourceNdcDepth')) 'Renderer source depth still recomputes stale projection/modelview state.'
Assert-True ($renderer -match '(?s)source_clip_depth != FALSE.*projected_z = clip_vtx->z;.*ndsRendererHardwareClipVertex\(clip_vtx, projected_z\)') 'Renderer projected X/Y/Z no longer share the composed clip vertex.'
Assert-True ($renderer -match '(?s)if \(source_clip_depth != FALSE\).*?ndsRendererHardwareClipVertex\(clip_vtx, projected_z\);\s*}\s*else\s*{\s*ndsRendererHardwareClipVertexNdcDepth\(clip_vtx, projected_z\);') 'Renderer no-Z stage layers no longer submit synthetic depth directly in signed 20.12 NDC.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareNextProjectedDepth\(void\).*return sNdsRendererHardwareProjectedDepth /\s*NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;') 'Renderer synthetic no-Z depth regained the erroneous extra 12.4-to-20.12 shift.'
$projectedDepthStart = 0x1000 * 6
$projectedDepthStep = 6
Assert-Equal ([math]::Floor(($projectedDepthStart - 1) / $projectedDepthStep)) 4095 'Renderer first synthetic no-Z depth must be the far signed 20.12 NDC endpoint.'
Assert-True ($rendererAdapter.Contains('nGCMatrixKindVecTraRotRpyRSca')) 'Battle DL renderer vector DObj matrix cases are missing.'
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterBuildBillboardMtx')) 'Battle DL renderer billboard DObj matrix cases are missing.'
Assert-True ($rendererAdapter.Contains('nGCMatrixKindRecalcRotRpyRSca')) 'Battle DL renderer recalc DObj matrix cases are missing.'
Assert-True ($rendererAdapter.Contains('nGCMatrixKind50')) 'Battle DL renderer camera-mod recalc DObj matrix cases are missing.'
Assert-True ($rendererAdapter.Contains('NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND 0x4Bu')) 'Battle DL renderer common fighter-parts DObj matrix kind 0x4B is missing.'
Assert-True ($rendererAdapter.Contains('NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND 0x4Cu')) 'Battle DL renderer BattleShip camera matrix kind 0x4C is missing.'
Assert-True ($rendererAdapter.Contains('syMatrixLookAtReflect')) 'Battle DL renderer BattleShip camera matrix path no longer uses reflected look-at prep.'
Assert-True ($rendererAdapter.Contains('parts->transform_update_mode != 0')) 'Battle DL renderer fighter-parts cached matrix branch is missing.'
Assert-True ($rendererAdapter.Contains('parts->unk_dobjtrans_0x10')) 'Battle DL renderer fighter-parts matrix seed is missing.'
Assert-True ($rendererAdapter.Contains('value = (row == 3u) ? 1.0F : 0.0F;')) 'Battle DL renderer cached fighter-parts matrix conversion no longer matches syMatrixF2LFixedW W-column behavior.'
Assert-True ($rendererAdapter.Contains('for (i = depth; i != 0u; i--)')) 'Battle DL renderer DObj parent-chain composition order regressed.'
Assert-True ($rendererAdapter.Contains('ndsRendererMtxMul20p12(&dobj_world, &camera_modelview, modelview)')) 'Battle DL renderer DObj/camera modelview composition order regressed.'
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterPrepareMaterialSegment')) 'Battle DL renderer material segment emission is missing.'
Assert-True ($rendererAdapter.Contains('segment_e_base')) 'Battle DL renderer segment 0x0E material resolver is missing.'
Assert-True ($rendererAdapter.Contains('NDS_FIGHTER_DL_OP_LOADTLUT')) 'Battle DL renderer material TLUT packet emission is missing.'
Assert-True ($rendererAdapter.Contains('HardwareZBufferTriangleCount')) 'All-DL renderer does not expose hardware z-buffer stats.'
Assert-True ($rendererAdapter.Contains('HardwareProjectedDepthTriangleCount')) 'All-DL renderer does not expose hardware projected-depth stats.'
Assert-True ($rendererAdapter.Contains('HardwareDecalDepthTriangleCount')) 'All-DL renderer does not expose hardware decal-depth stats.'
Assert-True ($rendererAdapter.Contains('gNdsStageGCDrawAllLoopHardwareTextureReadyCount')) 'Stage gcDrawAll renderer does not expose hardware texture stats.'
$compatShims = Get-Content (Join-Path $root 'src/port/reloc_backend_compat_shims.c') -Raw
Assert-True ($compatShims.Contains('gcAddXObjForCamera(cobj, 0x4C, 0)')) 'VSBattle compatibility camera no longer exposes the BattleShip 0x4C matrix kind.'
Assert-True ($compatShims.Contains('MObjSub normalized_mobjsub = **mobjsubs;')) 'Fighter material attachment no longer copies the O2R MObjSub before lane normalization.'
Assert-True ($compatShims.Contains('ndsRelocNormalizeMObjSubWordSwapped(&normalized_mobjsub);')) 'Fighter material attachment no longer normalizes mixed-width O2R lanes.'
Assert-True ($compatShims.Contains('gcAddMObjForDObj(dobj, &normalized_mobjsub)')) 'Fighter material attachment no longer gives the normalized record to the original object manager.'
$relocAssets = Get-Content (Join-Path $root 'src/port/reloc_backend_assets.c') -Raw
Assert-True ($relocAssets.Contains('mobjsub->flags = ((u16)old_block_siz << 8) | old_block_fmt;')) 'MObjSub flags/block format lane restoration regressed.'
Assert-True ($relocAssets.Contains('ndsRelocReverseColorPackBytes(&mobjsub->primcolor);')) 'MObjSub primary-color lane restoration regressed.'
$openingBackend = Get-Content (Join-Path $root 'src/port/opening_movie_backend.c') -Raw
Assert-True ($openingBackend.Contains('ndsRendererAdapterPrepareInitialMatrices')) 'Opening-room renderer seed hook is missing.'
$allDLVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-dl-draw-all-harness.ps1') -Raw
Assert-True ($allDLVerifier.Contains('HardwareTriangles')) 'All-DL verifier hardware-triangle switch is missing.'
Assert-True ($allDLVerifier.Contains('SoftwarePreview')) 'All-DL verifier software-preview opt-out is missing.'
Assert-True ($allDLVerifier.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'All-DL verifier no longer defaults to hardware.'
Assert-True ($allDLVerifier.Contains('smash64ds-battle-mariofox-dl-draw-all-hwtri')) 'All-DL verifier hardware target is missing.'
Assert-True ($allDLVerifier.Contains("if (-not `$NoBuild)")) 'All-DL verifier no-build skip is missing.'
Assert-True ($allDLVerifier.Contains('FTR_DL_ALL_HWTEX')) 'All-DL verifier hardware texture marker is missing.'
Assert-True ($allDLVerifier.Contains('FTR_DL_ALL_HW')) 'All-DL verifier hardware triangle marker is missing.'
Assert-True ($allDLVerifier.Contains('FTR_DL_ALL_HW_DEPTH')) 'All-DL verifier hardware depth marker is missing.'
Assert-True ($allDLVerifier.Contains('hwdepth=z')) 'All-DL verifier summary does not report hardware depth stats.'
Assert-True ($allDLVerifier.Contains('gNdsFighterDLAllDrawP0HardwareMatrixSeedCount')) 'All-DL verifier hardware matrix-seed diagnostics are missing.'
$stageGCDrawVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-gcdrawall-loop-harness.ps1') -Raw
Assert-True ($stageGCDrawVerifier.Contains('PLATFORM_HW')) 'gcDrawAll verifier hardware frame-flush marker is missing.'
Assert-True ($stageGCDrawVerifier.Contains('hwflush=')) 'gcDrawAll verifier hardware frame-flush summary is missing.'
Assert-True ($stageGCDrawVerifier.Contains('STAGE_GCDRAWALL_HW')) 'gcDrawAll verifier hardware submit-count marker is missing.'
Assert-True ($stageGCDrawVerifier.Contains('hwsubmit=')) 'gcDrawAll verifier hardware submit-count summary is missing.'
Assert-True ($stageGCDrawVerifier.Contains('hwtri=')) 'gcDrawAll verifier hardware triangle-count summary is missing.'
Assert-True ($stageGCDrawVerifier.Contains('hwdepth=')) 'gcDrawAll verifier hardware depth summary is missing.'
Assert-True ($stageGCDrawVerifier.Contains('hwtex=')) 'gcDrawAll verifier hardware texture summary is missing.'
Assert-True ($stageGCDrawVerifier.Contains('STAGE_GCDRAWALL_HW_FTR')) 'gcDrawAll verifier hardware fighter marker is missing.'
Assert-True ($stageGCDrawVerifier.Contains('hwftr=')) 'gcDrawAll verifier hardware fighter summary is missing.'
Assert-True ($stageGCDrawVerifier.Contains("if (-not `$NoBuild)")) 'gcDrawAll verifier no-build skip is missing.'
$stageGCDrawWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1') -Raw
Assert-True ($stageGCDrawWrapper.Contains('HardwareTriangles')) 'Stage gcDrawAll verifier hardware switch is missing.'
Assert-True ($stageGCDrawWrapper.Contains('SoftwarePreview')) 'Stage gcDrawAll verifier software-preview opt-out is missing.'
Assert-True ($stageGCDrawWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage gcDrawAll verifier no longer defaults to hardware.'
Assert-True ($stageGCDrawWrapper.Contains('battle-mariofox-stage-gcdrawall-loop-hwtri')) 'Stage gcDrawAll verifier hardware target is missing.'
Assert-True ($stageGCDrawVerifier.Contains('NDS_RENDERER_HW_TRIANGLES=1')) 'Stage gcDrawAll verifier does not enable hardware renderer builds.'
$menuStageGCDrawWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1') -Raw
Assert-True ($menuStageGCDrawWrapper.Contains('HardwareTriangles')) 'Menu-chain stage gcDrawAll verifier hardware switch is missing.'
Assert-True ($menuStageGCDrawWrapper.Contains('SoftwarePreview')) 'Menu-chain stage gcDrawAll verifier software-preview opt-out is missing.'
Assert-True ($menuStageGCDrawWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage gcDrawAll verifier no longer defaults to hardware.'
Assert-True ($menuStageGCDrawWrapper.Contains('menu-chain-mariofox-stage-gcdrawall-loop-hwtri')) 'Menu-chain stage gcDrawAll verifier hardware target is missing.'
$gcRunAllVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1') -Raw
Assert-True ($gcRunAllVerifier.Contains('HardwareTriangles')) 'gcRunAll verifier hardware switch is missing.'
Assert-True ($gcRunAllVerifier.Contains('NDS_RENDERER_HW_TRIANGLES=1')) 'gcRunAll verifier does not enable hardware renderer builds.'
Assert-True ($gcRunAllVerifier.Contains('STAGE_GCDRAWALL_HW_FTR')) 'gcRunAll verifier hardware fighter marker is missing.'
Assert-True ($gcRunAllVerifier.Contains('RENDER_CLIP=')) 'gcRunAll verifier clipping/saturation marker is missing.'
Assert-True ($gcRunAllVerifier.Contains('RENDER_TEXUSE=')) 'gcRunAll verifier texture-use rejection marker is missing.'
Assert-True ($gcRunAllVerifier.Contains('hwftr=')) 'gcRunAll verifier hardware fighter summary is missing.'
$stageMPLiveHitStatusWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1') -Raw
Assert-True ($stageMPLiveHitStatusWrapper.Contains('HardwareTriangles')) 'Stage MP live-hit status verifier hardware switch is missing.'
Assert-True ($stageMPLiveHitStatusWrapper.Contains('SoftwarePreview')) 'Stage MP live-hit status verifier software-preview opt-out is missing.'
Assert-True ($stageMPLiveHitStatusWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP live-hit status verifier no longer defaults to hardware.'
Assert-True ($stageMPLiveHitStatusWrapper.Contains('battle-mariofox-stage-mplivehit-status-loop-hwtri')) 'Stage MP live-hit status verifier hardware target is missing.'
$menuStageMPLiveHitStatusWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1') -Raw
Assert-True ($menuStageMPLiveHitStatusWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP live-hit status verifier hardware switch is missing.'
Assert-True ($menuStageMPLiveHitStatusWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP live-hit status verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPLiveHitStatusWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP live-hit status verifier no longer defaults to hardware.'
Assert-True ($menuStageMPLiveHitStatusWrapper.Contains('menu-chain-mariofox-stage-mplivehit-status-loop-hwtri')) 'Menu-chain stage MP live-hit status verifier hardware target is missing.'
$registry = Get-Content (Join-Path $root 'scripts/lib/harness-registry.ps1') -Raw
Assert-True ($registry.Contains('smash64ds-battle-mariofox-dl-draw-all-hwtri')) 'Direct all-DL registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-dl-draw-all-hwtri')) 'Menu-chain all-DL registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-gcdrawall-loop-hwtri')) 'Stage gcDrawAll registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-gcdrawall-loop-hwtri')) 'Menu-chain stage gcDrawAll registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-collision-loop-hwtri')) 'Stage collision registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-collision-loop-hwtri')) 'Menu-chain stage collision registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-floor-follow-loop-hwtri')) 'Stage floor-follow registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-floor-follow-loop-hwtri')) 'Menu-chain stage floor-follow registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-floor-edge-loop-hwtri')) 'Stage floor-edge registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-floor-edge-loop-hwtri')) 'Menu-chain stage floor-edge registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpprocess-floor-loop-hwtri')) 'Stage MP process-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpprocess-floor-loop-hwtri')) 'Menu-chain stage MP process-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpupdate-floor-loop-hwtri')) 'Stage MP update-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpupdate-floor-loop-hwtri')) 'Menu-chain stage MP update-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpsweep-floor-loop-hwtri')) 'Stage MP sweep-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpsweep-floor-loop-hwtri')) 'Menu-chain stage MP sweep-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcross-floor-loop-hwtri')) 'Stage MP cross-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcross-floor-loop-hwtri')) 'Menu-chain stage MP cross-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpadjust-floor-loop-hwtri')) 'Stage MP adjust-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpadjust-floor-loop-hwtri')) 'Menu-chain stage MP adjust-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpedge-floor-loop-hwtri')) 'Stage MP edge-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpedge-floor-loop-hwtri')) 'Menu-chain stage MP edge-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpwall-floor-loop-hwtri')) 'Stage MP wall-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpwall-floor-loop-hwtri')) 'Menu-chain stage MP wall-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpstale-floor-loop-hwtri')) 'Stage MP stale-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpstale-floor-loop-hwtri')) 'Menu-chain stage MP stale-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mplivestale-floor-loop-hwtri')) 'Stage MP live-stale-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mplivestale-floor-loop-hwtri')) 'Menu-chain stage MP live-stale-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpmotionstale-floor-loop-hwtri')) 'Stage MP motion-stale-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpmotionstale-floor-loop-hwtri')) 'Menu-chain stage MP motion-stale-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffstatus-floor-loop-hwtri')) 'Stage MP cliff-status-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-hwtri')) 'Menu-chain stage MP cliff-status-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpclifftick-floor-loop-hwtri')) 'Stage MP cliff-tick-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpclifftick-floor-loop-hwtri')) 'Menu-chain stage MP cliff-tick-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpfallmap-floor-loop-hwtri')) 'Stage MP fall-map-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpfallmap-floor-loop-hwtri')) 'Menu-chain stage MP fall-map-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpfallland-floor-loop-hwtri')) 'Stage MP fall-landing-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpfallland-floor-loop-hwtri')) 'Menu-chain stage MP fall-landing-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpceil-floor-loop-hwtri')) 'Stage MP ceiling-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpceil-floor-loop-hwtri')) 'Menu-chain stage MP ceiling-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpceilstatus-floor-loop-hwtri')) 'Stage MP ceiling-status-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpceilstatus-floor-loop-hwtri')) 'Menu-chain stage MP ceiling-status-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffcatch-floor-loop-hwtri')) 'Stage MP cliff-catch-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffcatch-floor-loop-hwtri')) 'Menu-chain stage MP cliff-catch-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffwait-floor-loop-hwtri')) 'Stage MP cliff-wait-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffwait-floor-loop-hwtri')) 'Menu-chain stage MP cliff-wait-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffattack-floor-loop-hwtri')) 'Stage MP cliff-attack-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffattack-floor-loop-hwtri')) 'Menu-chain stage MP cliff-attack-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffattack-action-loop-hwtri')) 'Stage MP cliff-attack-action registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffattack-action-loop-hwtri')) 'Menu-chain stage MP cliff-attack-action registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffcommon2-loop-hwtri')) 'Stage MP cliff-common2 registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffcommon2-loop-hwtri')) 'Menu-chain stage MP cliff-common2 registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffescape-action-loop-hwtri')) 'Stage MP cliff-escape-action registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffescape-action-loop-hwtri')) 'Menu-chain stage MP cliff-escape-action registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffescape-common2-loop-hwtri')) 'Stage MP cliff-escape-common2 registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffescape-common2-loop-hwtri')) 'Menu-chain stage MP cliff-escape-common2 registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffclimb-floor-loop-hwtri')) 'Stage MP cliff-climb-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-hwtri')) 'Menu-chain stage MP cliff-climb-floor registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffclimb-action-loop-hwtri')) 'Stage MP cliff-climb-action registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffclimb-action-loop-hwtri')) 'Menu-chain stage MP cliff-climb-action registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffclimb-common2-loop-hwtri')) 'Stage MP cliff-climb-common2 registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-hwtri')) 'Menu-chain stage MP cliff-climb-common2 registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffclimb-finish-loop-hwtri')) 'Stage MP cliff-climb-finish registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-hwtri')) 'Menu-chain stage MP cliff-climb-finish registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mpcliffwait-damage-loop-hwtri')) 'Stage MP cliff-wait-damage registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mpcliffwait-damage-loop-hwtri')) 'Menu-chain stage MP cliff-wait-damage registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mppassive-loop-hwtri')) 'Stage MP passive registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mppassive-loop-hwtri')) 'Menu-chain stage MP passive registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-battle-mariofox-stage-mplivehit-status-loop-hwtri')) 'Boundary direct registry target is not hardware-renderer default.'
Assert-True ($registry.Contains('smash64ds-menu-chain-mariofox-stage-mplivehit-status-loop-hwtri')) 'Boundary menu registry target is not hardware-renderer default.'
$hwSubmitAllowlist = [regex]::Match($taskman, '(?s)NDS_RENDERER_HW_TRIANGLES.*?ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame\(\)')
Assert-True ($hwSubmitAllowlist.Success) 'Stage gcDrawAll hardware submit allowlist was not found.'
Assert-True ($hwSubmitAllowlist.Value.Contains('NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP')) 'Stage MP passive direct mode is not in the hardware submit allowlist.'
Assert-True ($hwSubmitAllowlist.Value.Contains('NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP')) 'Stage MP passive menu mode is not in the hardware submit allowlist.'
$buildProfile = Get-Content (Join-Path $root 'scripts/build-verify-profile.ps1') -Raw
Assert-True ($buildProfile.Contains("NDS_RENDERER_HW_TRIANGLES=1")) 'Profile prebuild does not enable hardware renderer for hwtri targets.'
Assert-True ($buildProfile.Contains('if (-not $NoSharedBuild)')) 'Profile prebuild -Force path disables shared builds.'
Assert-True ($buildProfile.Contains('$forcedSharedBuilds')) 'Profile prebuild no longer limits -Force to one full rebuild per shared slot.'
Assert-True ($buildProfile.Contains('$rendererSuffix')) 'Profile prebuild shared slots do not split hardware and software renderer object trees.'
$stageCollisionWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-collision-loop-harness.ps1') -Raw
Assert-True ($stageCollisionWrapper.Contains('HardwareTriangles')) 'Stage collision verifier hardware switch is missing.'
Assert-True ($stageCollisionWrapper.Contains('SoftwarePreview')) 'Stage collision verifier software-preview opt-out is missing.'
Assert-True ($stageCollisionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage collision verifier no longer defaults to hardware.'
Assert-True ($stageCollisionWrapper.Contains('battle-mariofox-stage-collision-loop-hwtri')) 'Stage collision verifier hardware target is missing.'
$menuStageCollisionWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-collision-loop-harness.ps1') -Raw
Assert-True ($menuStageCollisionWrapper.Contains('HardwareTriangles')) 'Menu-chain stage collision verifier hardware switch is missing.'
Assert-True ($menuStageCollisionWrapper.Contains('SoftwarePreview')) 'Menu-chain stage collision verifier software-preview opt-out is missing.'
Assert-True ($menuStageCollisionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage collision verifier no longer defaults to hardware.'
Assert-True ($menuStageCollisionWrapper.Contains('menu-chain-mariofox-stage-collision-loop-hwtri')) 'Menu-chain stage collision verifier hardware target is missing.'
$stageFloorFollowWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-floor-follow-loop-harness.ps1') -Raw
Assert-True ($stageFloorFollowWrapper.Contains('HardwareTriangles')) 'Stage floor-follow verifier hardware switch is missing.'
Assert-True ($stageFloorFollowWrapper.Contains('SoftwarePreview')) 'Stage floor-follow verifier software-preview opt-out is missing.'
Assert-True ($stageFloorFollowWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage floor-follow verifier no longer defaults to hardware.'
Assert-True ($stageFloorFollowWrapper.Contains('battle-mariofox-stage-floor-follow-loop-hwtri')) 'Stage floor-follow verifier hardware target is missing.'
$menuStageFloorFollowWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-floor-follow-loop-harness.ps1') -Raw
Assert-True ($menuStageFloorFollowWrapper.Contains('HardwareTriangles')) 'Menu-chain stage floor-follow verifier hardware switch is missing.'
Assert-True ($menuStageFloorFollowWrapper.Contains('SoftwarePreview')) 'Menu-chain stage floor-follow verifier software-preview opt-out is missing.'
Assert-True ($menuStageFloorFollowWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage floor-follow verifier no longer defaults to hardware.'
Assert-True ($menuStageFloorFollowWrapper.Contains('menu-chain-mariofox-stage-floor-follow-loop-hwtri')) 'Menu-chain stage floor-follow verifier hardware target is missing.'
$stageFloorEdgeWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-floor-edge-loop-harness.ps1') -Raw
Assert-True ($stageFloorEdgeWrapper.Contains('HardwareTriangles')) 'Stage floor-edge verifier hardware switch is missing.'
Assert-True ($stageFloorEdgeWrapper.Contains('SoftwarePreview')) 'Stage floor-edge verifier software-preview opt-out is missing.'
Assert-True ($stageFloorEdgeWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage floor-edge verifier no longer defaults to hardware.'
Assert-True ($stageFloorEdgeWrapper.Contains('battle-mariofox-stage-floor-edge-loop-hwtri')) 'Stage floor-edge verifier hardware target is missing.'
$menuStageFloorEdgeWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1') -Raw
Assert-True ($menuStageFloorEdgeWrapper.Contains('HardwareTriangles')) 'Menu-chain stage floor-edge verifier hardware switch is missing.'
Assert-True ($menuStageFloorEdgeWrapper.Contains('SoftwarePreview')) 'Menu-chain stage floor-edge verifier software-preview opt-out is missing.'
Assert-True ($menuStageFloorEdgeWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage floor-edge verifier no longer defaults to hardware.'
Assert-True ($menuStageFloorEdgeWrapper.Contains('menu-chain-mariofox-stage-floor-edge-loop-hwtri')) 'Menu-chain stage floor-edge verifier hardware target is missing.'
$stageMPProcessFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpprocess-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPProcessFloorWrapper.Contains('HardwareTriangles')) 'Stage MP process-floor verifier hardware switch is missing.'
Assert-True ($stageMPProcessFloorWrapper.Contains('SoftwarePreview')) 'Stage MP process-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPProcessFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP process-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPProcessFloorWrapper.Contains('battle-mariofox-stage-mpprocess-floor-loop-hwtri')) 'Stage MP process-floor verifier hardware target is missing.'
$menuStageMPProcessFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpprocess-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPProcessFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP process-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPProcessFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP process-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPProcessFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP process-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPProcessFloorWrapper.Contains('menu-chain-mariofox-stage-mpprocess-floor-loop-hwtri')) 'Menu-chain stage MP process-floor verifier hardware target is missing.'
$stageMPUpdateFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpupdate-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPUpdateFloorWrapper.Contains('HardwareTriangles')) 'Stage MP update-floor verifier hardware switch is missing.'
Assert-True ($stageMPUpdateFloorWrapper.Contains('SoftwarePreview')) 'Stage MP update-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPUpdateFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP update-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPUpdateFloorWrapper.Contains('battle-mariofox-stage-mpupdate-floor-loop-hwtri')) 'Stage MP update-floor verifier hardware target is missing.'
$menuStageMPUpdateFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpupdate-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPUpdateFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP update-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPUpdateFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP update-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPUpdateFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP update-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPUpdateFloorWrapper.Contains('menu-chain-mariofox-stage-mpupdate-floor-loop-hwtri')) 'Menu-chain stage MP update-floor verifier hardware target is missing.'
$stageMPSweepFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpsweep-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPSweepFloorWrapper.Contains('HardwareTriangles')) 'Stage MP sweep-floor verifier hardware switch is missing.'
Assert-True ($stageMPSweepFloorWrapper.Contains('SoftwarePreview')) 'Stage MP sweep-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPSweepFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP sweep-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPSweepFloorWrapper.Contains('battle-mariofox-stage-mpsweep-floor-loop-hwtri')) 'Stage MP sweep-floor verifier hardware target is missing.'
$menuStageMPSweepFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpsweep-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPSweepFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP sweep-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPSweepFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP sweep-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPSweepFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP sweep-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPSweepFloorWrapper.Contains('menu-chain-mariofox-stage-mpsweep-floor-loop-hwtri')) 'Menu-chain stage MP sweep-floor verifier hardware target is missing.'
$stageMPCrossFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcross-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCrossFloorWrapper.Contains('HardwareTriangles')) 'Stage MP cross-floor verifier hardware switch is missing.'
Assert-True ($stageMPCrossFloorWrapper.Contains('SoftwarePreview')) 'Stage MP cross-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCrossFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cross-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCrossFloorWrapper.Contains('battle-mariofox-stage-mpcross-floor-loop-hwtri')) 'Stage MP cross-floor verifier hardware target is missing.'
$menuStageMPCrossFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcross-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCrossFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cross-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCrossFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cross-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCrossFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cross-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCrossFloorWrapper.Contains('menu-chain-mariofox-stage-mpcross-floor-loop-hwtri')) 'Menu-chain stage MP cross-floor verifier hardware target is missing.'
$stageMPAdjustFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPAdjustFloorWrapper.Contains('HardwareTriangles')) 'Stage MP adjust-floor verifier hardware switch is missing.'
Assert-True ($stageMPAdjustFloorWrapper.Contains('SoftwarePreview')) 'Stage MP adjust-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPAdjustFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP adjust-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPAdjustFloorWrapper.Contains('battle-mariofox-stage-mpadjust-floor-loop-hwtri')) 'Stage MP adjust-floor verifier hardware target is missing.'
$menuStageMPAdjustFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPAdjustFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP adjust-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPAdjustFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP adjust-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPAdjustFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP adjust-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPAdjustFloorWrapper.Contains('menu-chain-mariofox-stage-mpadjust-floor-loop-hwtri')) 'Menu-chain stage MP adjust-floor verifier hardware target is missing.'
$stageMPEdgeFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPEdgeFloorWrapper.Contains('HardwareTriangles')) 'Stage MP edge-floor verifier hardware switch is missing.'
Assert-True ($stageMPEdgeFloorWrapper.Contains('SoftwarePreview')) 'Stage MP edge-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPEdgeFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP edge-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPEdgeFloorWrapper.Contains('battle-mariofox-stage-mpedge-floor-loop-hwtri')) 'Stage MP edge-floor verifier hardware target is missing.'
$menuStageMPEdgeFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPEdgeFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP edge-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPEdgeFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP edge-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPEdgeFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP edge-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPEdgeFloorWrapper.Contains('menu-chain-mariofox-stage-mpedge-floor-loop-hwtri')) 'Menu-chain stage MP edge-floor verifier hardware target is missing.'
$stageMPWallFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPWallFloorWrapper.Contains('HardwareTriangles')) 'Stage MP wall-floor verifier hardware switch is missing.'
Assert-True ($stageMPWallFloorWrapper.Contains('SoftwarePreview')) 'Stage MP wall-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPWallFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP wall-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPWallFloorWrapper.Contains('battle-mariofox-stage-mpwall-floor-loop-hwtri')) 'Stage MP wall-floor verifier hardware target is missing.'
$menuStageMPWallFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPWallFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP wall-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPWallFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP wall-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPWallFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP wall-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPWallFloorWrapper.Contains('menu-chain-mariofox-stage-mpwall-floor-loop-hwtri')) 'Menu-chain stage MP wall-floor verifier hardware target is missing.'
$stageMPStaleFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpstale-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPStaleFloorWrapper.Contains('HardwareTriangles')) 'Stage MP stale-floor verifier hardware switch is missing.'
Assert-True ($stageMPStaleFloorWrapper.Contains('SoftwarePreview')) 'Stage MP stale-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPStaleFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP stale-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPStaleFloorWrapper.Contains('battle-mariofox-stage-mpstale-floor-loop-hwtri')) 'Stage MP stale-floor verifier hardware target is missing.'
$menuStageMPStaleFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpstale-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPStaleFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP stale-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPStaleFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP stale-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPStaleFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP stale-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPStaleFloorWrapper.Contains('menu-chain-mariofox-stage-mpstale-floor-loop-hwtri')) 'Menu-chain stage MP stale-floor verifier hardware target is missing.'
$stageMPLiveStaleFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPLiveStaleFloorWrapper.Contains('HardwareTriangles')) 'Stage MP live-stale-floor verifier hardware switch is missing.'
Assert-True ($stageMPLiveStaleFloorWrapper.Contains('SoftwarePreview')) 'Stage MP live-stale-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPLiveStaleFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP live-stale-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPLiveStaleFloorWrapper.Contains('battle-mariofox-stage-mplivestale-floor-loop-hwtri')) 'Stage MP live-stale-floor verifier hardware target is missing.'
$menuStageMPLiveStaleFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPLiveStaleFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP live-stale-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPLiveStaleFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP live-stale-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPLiveStaleFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP live-stale-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPLiveStaleFloorWrapper.Contains('menu-chain-mariofox-stage-mplivestale-floor-loop-hwtri')) 'Menu-chain stage MP live-stale-floor verifier hardware target is missing.'
$stageMPMotionStaleFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPMotionStaleFloorWrapper.Contains('HardwareTriangles')) 'Stage MP motion-stale-floor verifier hardware switch is missing.'
Assert-True ($stageMPMotionStaleFloorWrapper.Contains('SoftwarePreview')) 'Stage MP motion-stale-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPMotionStaleFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP motion-stale-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPMotionStaleFloorWrapper.Contains('battle-mariofox-stage-mpmotionstale-floor-loop-hwtri')) 'Stage MP motion-stale-floor verifier hardware target is missing.'
$menuStageMPMotionStaleFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPMotionStaleFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP motion-stale-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPMotionStaleFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP motion-stale-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPMotionStaleFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP motion-stale-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPMotionStaleFloorWrapper.Contains('menu-chain-mariofox-stage-mpmotionstale-floor-loop-hwtri')) 'Menu-chain stage MP motion-stale-floor verifier hardware target is missing.'
$stageMPCliffStatusFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffStatusFloorWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-status-floor verifier hardware switch is missing.'
Assert-True ($stageMPCliffStatusFloorWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-status-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffStatusFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-status-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffStatusFloorWrapper.Contains('battle-mariofox-stage-mpcliffstatus-floor-loop-hwtri')) 'Stage MP cliff-status-floor verifier hardware target is missing.'
$menuStageMPCliffStatusFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffStatusFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-status-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffStatusFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-status-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffStatusFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-status-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffStatusFloorWrapper.Contains('menu-chain-mariofox-stage-mpcliffstatus-floor-loop-hwtri')) 'Menu-chain stage MP cliff-status-floor verifier hardware target is missing.'
$stageMPCliffTickFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpclifftick-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffTickFloorWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-tick-floor verifier hardware switch is missing.'
Assert-True ($stageMPCliffTickFloorWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-tick-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffTickFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-tick-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffTickFloorWrapper.Contains('battle-mariofox-stage-mpclifftick-floor-loop-hwtri')) 'Stage MP cliff-tick-floor verifier hardware target is missing.'
$menuStageMPCliffTickFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpclifftick-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffTickFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-tick-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffTickFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-tick-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffTickFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-tick-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffTickFloorWrapper.Contains('menu-chain-mariofox-stage-mpclifftick-floor-loop-hwtri')) 'Menu-chain stage MP cliff-tick-floor verifier hardware target is missing.'
$stageMPFallMapFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpfallmap-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPFallMapFloorWrapper.Contains('HardwareTriangles')) 'Stage MP fall-map-floor verifier hardware switch is missing.'
Assert-True ($stageMPFallMapFloorWrapper.Contains('SoftwarePreview')) 'Stage MP fall-map-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPFallMapFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP fall-map-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPFallMapFloorWrapper.Contains('battle-mariofox-stage-mpfallmap-floor-loop-hwtri')) 'Stage MP fall-map-floor verifier hardware target is missing.'
$menuStageMPFallMapFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpfallmap-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPFallMapFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP fall-map-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPFallMapFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP fall-map-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPFallMapFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP fall-map-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPFallMapFloorWrapper.Contains('menu-chain-mariofox-stage-mpfallmap-floor-loop-hwtri')) 'Menu-chain stage MP fall-map-floor verifier hardware target is missing.'
$stageMPFallLandFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpfallland-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPFallLandFloorWrapper.Contains('HardwareTriangles')) 'Stage MP fall-landing-floor verifier hardware switch is missing.'
Assert-True ($stageMPFallLandFloorWrapper.Contains('SoftwarePreview')) 'Stage MP fall-landing-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPFallLandFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP fall-landing-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPFallLandFloorWrapper.Contains('battle-mariofox-stage-mpfallland-floor-loop-hwtri')) 'Stage MP fall-landing-floor verifier hardware target is missing.'
$menuStageMPFallLandFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpfallland-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPFallLandFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP fall-landing-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPFallLandFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP fall-landing-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPFallLandFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP fall-landing-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPFallLandFloorWrapper.Contains('menu-chain-mariofox-stage-mpfallland-floor-loop-hwtri')) 'Menu-chain stage MP fall-landing-floor verifier hardware target is missing.'
$stageMPCeilFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCeilFloorWrapper.Contains('HardwareTriangles')) 'Stage MP ceiling-floor verifier hardware switch is missing.'
Assert-True ($stageMPCeilFloorWrapper.Contains('SoftwarePreview')) 'Stage MP ceiling-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCeilFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP ceiling-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCeilFloorWrapper.Contains('battle-mariofox-stage-mpceil-floor-loop-hwtri')) 'Stage MP ceiling-floor verifier hardware target is missing.'
$menuStageMPCeilFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCeilFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP ceiling-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCeilFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP ceiling-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCeilFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP ceiling-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCeilFloorWrapper.Contains('menu-chain-mariofox-stage-mpceil-floor-loop-hwtri')) 'Menu-chain stage MP ceiling-floor verifier hardware target is missing.'
$stageMPCeilStatusFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpceilstatus-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCeilStatusFloorWrapper.Contains('HardwareTriangles')) 'Stage MP ceiling-status-floor verifier hardware switch is missing.'
Assert-True ($stageMPCeilStatusFloorWrapper.Contains('SoftwarePreview')) 'Stage MP ceiling-status-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCeilStatusFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP ceiling-status-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCeilStatusFloorWrapper.Contains('battle-mariofox-stage-mpceilstatus-floor-loop-hwtri')) 'Stage MP ceiling-status-floor verifier hardware target is missing.'
$menuStageMPCeilStatusFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCeilStatusFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP ceiling-status-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCeilStatusFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP ceiling-status-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCeilStatusFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP ceiling-status-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCeilStatusFloorWrapper.Contains('menu-chain-mariofox-stage-mpceilstatus-floor-loop-hwtri')) 'Menu-chain stage MP ceiling-status-floor verifier hardware target is missing.'
$stageMPCliffCatchFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffCatchFloorWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-catch-floor verifier hardware switch is missing.'
Assert-True ($stageMPCliffCatchFloorWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-catch-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffCatchFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-catch-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffCatchFloorWrapper.Contains('battle-mariofox-stage-mpcliffcatch-floor-loop-hwtri')) 'Stage MP cliff-catch-floor verifier hardware target is missing.'
$menuStageMPCliffCatchFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffCatchFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-catch-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffCatchFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-catch-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffCatchFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-catch-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffCatchFloorWrapper.Contains('menu-chain-mariofox-stage-mpcliffcatch-floor-loop-hwtri')) 'Menu-chain stage MP cliff-catch-floor verifier hardware target is missing.'
$stageMPCliffWaitFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffwait-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffWaitFloorWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-wait-floor verifier hardware switch is missing.'
Assert-True ($stageMPCliffWaitFloorWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-wait-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffWaitFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-wait-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffWaitFloorWrapper.Contains('battle-mariofox-stage-mpcliffwait-floor-loop-hwtri')) 'Stage MP cliff-wait-floor verifier hardware target is missing.'
$menuStageMPCliffWaitFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffwait-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffWaitFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-wait-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffWaitFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-wait-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffWaitFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-wait-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffWaitFloorWrapper.Contains('menu-chain-mariofox-stage-mpcliffwait-floor-loop-hwtri')) 'Menu-chain stage MP cliff-wait-floor verifier hardware target is missing.'
$stageMPCliffAttackFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffattack-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffAttackFloorWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-attack-floor verifier hardware switch is missing.'
Assert-True ($stageMPCliffAttackFloorWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-attack-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffAttackFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-attack-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffAttackFloorWrapper.Contains('battle-mariofox-stage-mpcliffattack-floor-loop-hwtri')) 'Stage MP cliff-attack-floor verifier hardware target is missing.'
$menuStageMPCliffAttackFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffAttackFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-attack-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffAttackFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-attack-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffAttackFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-attack-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffAttackFloorWrapper.Contains('menu-chain-mariofox-stage-mpcliffattack-floor-loop-hwtri')) 'Menu-chain stage MP cliff-attack-floor verifier hardware target is missing.'
$stageMPCliffAttackActionWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffAttackActionWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-attack-action verifier hardware switch is missing.'
Assert-True ($stageMPCliffAttackActionWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-attack-action verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffAttackActionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-attack-action verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffAttackActionWrapper.Contains('battle-mariofox-stage-mpcliffattack-action-loop-hwtri')) 'Stage MP cliff-attack-action verifier hardware target is missing.'
$menuStageMPCliffAttackActionWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffAttackActionWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-attack-action verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffAttackActionWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-attack-action verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffAttackActionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-attack-action verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffAttackActionWrapper.Contains('menu-chain-mariofox-stage-mpcliffattack-action-loop-hwtri')) 'Menu-chain stage MP cliff-attack-action verifier hardware target is missing.'
$stageMPCliffCommon2Wrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffcommon2-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffCommon2Wrapper.Contains('HardwareTriangles')) 'Stage MP cliff-common2 verifier hardware switch is missing.'
Assert-True ($stageMPCliffCommon2Wrapper.Contains('SoftwarePreview')) 'Stage MP cliff-common2 verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffCommon2Wrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-common2 verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffCommon2Wrapper.Contains('battle-mariofox-stage-mpcliffcommon2-loop-hwtri')) 'Stage MP cliff-common2 verifier hardware target is missing.'
$menuStageMPCliffCommon2Wrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffcommon2-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffCommon2Wrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-common2 verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffCommon2Wrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-common2 verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffCommon2Wrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-common2 verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffCommon2Wrapper.Contains('menu-chain-mariofox-stage-mpcliffcommon2-loop-hwtri')) 'Menu-chain stage MP cliff-common2 verifier hardware target is missing.'
$stageMPCliffEscapeActionWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffEscapeActionWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-escape-action verifier hardware switch is missing.'
Assert-True ($stageMPCliffEscapeActionWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-escape-action verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffEscapeActionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-escape-action verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffEscapeActionWrapper.Contains('battle-mariofox-stage-mpcliffescape-action-loop-hwtri')) 'Stage MP cliff-escape-action verifier hardware target is missing.'
$menuStageMPCliffEscapeActionWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffEscapeActionWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-escape-action verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffEscapeActionWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-escape-action verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffEscapeActionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-escape-action verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffEscapeActionWrapper.Contains('menu-chain-mariofox-stage-mpcliffescape-action-loop-hwtri')) 'Menu-chain stage MP cliff-escape-action verifier hardware target is missing.'
$stageMPCliffEscapeCommon2Wrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffescape-common2-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffEscapeCommon2Wrapper.Contains('HardwareTriangles')) 'Stage MP cliff-escape-common2 verifier hardware switch is missing.'
Assert-True ($stageMPCliffEscapeCommon2Wrapper.Contains('SoftwarePreview')) 'Stage MP cliff-escape-common2 verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffEscapeCommon2Wrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-escape-common2 verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffEscapeCommon2Wrapper.Contains('battle-mariofox-stage-mpcliffescape-common2-loop-hwtri')) 'Stage MP cliff-escape-common2 verifier hardware target is missing.'
$menuStageMPCliffEscapeCommon2Wrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffescape-common2-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffEscapeCommon2Wrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-escape-common2 verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffEscapeCommon2Wrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-escape-common2 verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffEscapeCommon2Wrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-escape-common2 verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffEscapeCommon2Wrapper.Contains('menu-chain-mariofox-stage-mpcliffescape-common2-loop-hwtri')) 'Menu-chain stage MP cliff-escape-common2 verifier hardware target is missing.'
$stageMPCliffClimbFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffClimbFloorWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-climb-floor verifier hardware switch is missing.'
Assert-True ($stageMPCliffClimbFloorWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-climb-floor verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffClimbFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-climb-floor verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffClimbFloorWrapper.Contains('battle-mariofox-stage-mpcliffclimb-floor-loop-hwtri')) 'Stage MP cliff-climb-floor verifier hardware target is missing.'
$menuStageMPCliffClimbFloorWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffClimbFloorWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-climb-floor verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffClimbFloorWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-climb-floor verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffClimbFloorWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-climb-floor verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffClimbFloorWrapper.Contains('menu-chain-mariofox-stage-mpcliffclimb-floor-loop-hwtri')) 'Menu-chain stage MP cliff-climb-floor verifier hardware target is missing.'
$stageMPCliffClimbActionWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffClimbActionWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-climb-action verifier hardware switch is missing.'
Assert-True ($stageMPCliffClimbActionWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-climb-action verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffClimbActionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-climb-action verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffClimbActionWrapper.Contains('battle-mariofox-stage-mpcliffclimb-action-loop-hwtri')) 'Stage MP cliff-climb-action verifier hardware target is missing.'
$menuStageMPCliffClimbActionWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffClimbActionWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-climb-action verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffClimbActionWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-climb-action verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffClimbActionWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-climb-action verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffClimbActionWrapper.Contains('menu-chain-mariofox-stage-mpcliffclimb-action-loop-hwtri')) 'Menu-chain stage MP cliff-climb-action verifier hardware target is missing.'
$stageMPCliffClimbCommon2Wrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffClimbCommon2Wrapper.Contains('HardwareTriangles')) 'Stage MP cliff-climb-common2 verifier hardware switch is missing.'
Assert-True ($stageMPCliffClimbCommon2Wrapper.Contains('SoftwarePreview')) 'Stage MP cliff-climb-common2 verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffClimbCommon2Wrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-climb-common2 verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffClimbCommon2Wrapper.Contains('battle-mariofox-stage-mpcliffclimb-common2-loop-hwtri')) 'Stage MP cliff-climb-common2 verifier hardware target is missing.'
$menuStageMPCliffClimbCommon2Wrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffClimbCommon2Wrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-climb-common2 verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffClimbCommon2Wrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-climb-common2 verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffClimbCommon2Wrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-climb-common2 verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffClimbCommon2Wrapper.Contains('menu-chain-mariofox-stage-mpcliffclimb-common2-loop-hwtri')) 'Menu-chain stage MP cliff-climb-common2 verifier hardware target is missing.'
$stageMPCliffClimbFinishWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffClimbFinishWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-climb-finish verifier hardware switch is missing.'
Assert-True ($stageMPCliffClimbFinishWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-climb-finish verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffClimbFinishWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-climb-finish verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffClimbFinishWrapper.Contains('battle-mariofox-stage-mpcliffclimb-finish-loop-hwtri')) 'Stage MP cliff-climb-finish verifier hardware target is missing.'
$menuStageMPCliffClimbFinishWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffClimbFinishWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-climb-finish verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffClimbFinishWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-climb-finish verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffClimbFinishWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-climb-finish verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffClimbFinishWrapper.Contains('menu-chain-mariofox-stage-mpcliffclimb-finish-loop-hwtri')) 'Menu-chain stage MP cliff-climb-finish verifier hardware target is missing.'
$stageMPCliffWaitDamageWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1') -Raw
Assert-True ($stageMPCliffWaitDamageWrapper.Contains('HardwareTriangles')) 'Stage MP cliff-wait-damage verifier hardware switch is missing.'
Assert-True ($stageMPCliffWaitDamageWrapper.Contains('SoftwarePreview')) 'Stage MP cliff-wait-damage verifier software-preview opt-out is missing.'
Assert-True ($stageMPCliffWaitDamageWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP cliff-wait-damage verifier no longer defaults to hardware.'
Assert-True ($stageMPCliffWaitDamageWrapper.Contains('battle-mariofox-stage-mpcliffwait-damage-loop-hwtri')) 'Stage MP cliff-wait-damage verifier hardware target is missing.'
$menuStageMPCliffWaitDamageWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1') -Raw
Assert-True ($menuStageMPCliffWaitDamageWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP cliff-wait-damage verifier hardware switch is missing.'
Assert-True ($menuStageMPCliffWaitDamageWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP cliff-wait-damage verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPCliffWaitDamageWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP cliff-wait-damage verifier no longer defaults to hardware.'
Assert-True ($menuStageMPCliffWaitDamageWrapper.Contains('menu-chain-mariofox-stage-mpcliffwait-damage-loop-hwtri')) 'Menu-chain stage MP cliff-wait-damage verifier hardware target is missing.'
$stageMPPassiveWrapper = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-stage-mppassive-loop-harness.ps1') -Raw
Assert-True ($stageMPPassiveWrapper.Contains('HardwareTriangles')) 'Stage MP passive verifier hardware switch is missing.'
Assert-True ($stageMPPassiveWrapper.Contains('SoftwarePreview')) 'Stage MP passive verifier software-preview opt-out is missing.'
Assert-True ($stageMPPassiveWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Stage MP passive verifier no longer defaults to hardware.'
Assert-True ($stageMPPassiveWrapper.Contains('battle-mariofox-stage-mppassive-loop-hwtri')) 'Stage MP passive verifier hardware target is missing.'
$menuStageMPPassiveWrapper = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1') -Raw
Assert-True ($menuStageMPPassiveWrapper.Contains('HardwareTriangles')) 'Menu-chain stage MP passive verifier hardware switch is missing.'
Assert-True ($menuStageMPPassiveWrapper.Contains('SoftwarePreview')) 'Menu-chain stage MP passive verifier software-preview opt-out is missing.'
Assert-True ($menuStageMPPassiveWrapper.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain stage MP passive verifier no longer defaults to hardware.'
Assert-True ($menuStageMPPassiveWrapper.Contains('menu-chain-mariofox-stage-mppassive-loop-hwtri')) 'Menu-chain stage MP passive verifier hardware target is missing.'
$menuAllDLVerifier = Get-Content (Join-Path $root 'scripts/verify-menu-chain-mariofox-dl-draw-all-harness.ps1') -Raw
Assert-True ($menuAllDLVerifier.Contains('HardwareTriangles')) 'Menu-chain all-DL verifier hardware switch is missing.'
Assert-True ($menuAllDLVerifier.Contains('SoftwarePreview')) 'Menu-chain all-DL verifier software-preview opt-out is missing.'
Assert-True ($menuAllDLVerifier.Contains('$HardwareTriangles = -not $SoftwarePreview')) 'Menu-chain all-DL verifier no longer defaults to hardware.'
Assert-True ($menuAllDLVerifier.Contains('smash64ds-menu-chain-mariofox-dl-draw-all-hwtri')) 'Menu-chain all-DL verifier hardware target is missing.'
Assert-True ($menuAllDLVerifier.Contains("if (-not `$NoBuild)")) 'Menu-chain all-DL verifier no-build skip is missing.'
Assert-True ($menuAllDLVerifier.Contains('FTR_DL_ALL_HWTEX')) 'Menu-chain all-DL verifier hardware texture marker is missing.'
Assert-True ($menuAllDLVerifier.Contains('FTR_DL_ALL_HW_DEPTH')) 'Menu-chain all-DL verifier hardware depth marker is missing.'
$movement = Get-Content (Join-Path $root 'src/port/reloc_backend_movement.c') -Raw
Assert-True ($movement.Contains('ndsStageGCDrawAllLoopSubmitHardwareFrame')) 'Stage gcDrawAll hardware replay hook is missing.'
Assert-True (-not $movement.Contains('NDS_STAGE_GCDRAWALL_HW_SUBMIT_LIMIT')) 'Stage gcDrawAll hardware replay still has the old bounded submit limit.'
$decodeHeader = Get-Content (Join-Path $root 'include/nds/nds_gbi_decode.h') -Raw
Assert-True ($decodeHeader.Contains('/ 2u')) 'F3DEX2 packed triangle decode must stay on BattleShip index*2 packing.'
Assert-True (-not $decodeHeader.Contains('/ 10u')) 'Stale F3DEX2 packed triangle /10 decode returned.'
Write-Output 'GBI decode fixtures passed: F3DEX2 VTX/TRI/MTX/POPMTX packing, F3DEX MVP-recalc matrix move-word packing, transformed and hardware raw/material triangle readiness, N64 matrix to DS 20.12 modelview-projection/modelview-stack vertex transform, and source snippets verified.'
