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
Assert-True ($renderer.Contains('ndsRendererMtxCellS16p16')) 'Renderer matrix unpack helper is missing.'
Assert-True ($renderer.Contains('ndsRendererTransformVertex20p12')) 'Renderer vertex transform helper is missing.'
Write-Output 'GBI decode fixtures passed: F3DEX2 VTX/TRI1/TRI2 packing, N64 matrix to DS 20.12 vertex transform, and source snippets verified.'
