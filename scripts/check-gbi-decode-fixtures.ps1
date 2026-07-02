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
Assert-True ($renderer.Contains('glTexCoord2t16')) 'Renderer hardware texture coordinates are missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_FMT_RGBA16')) 'Renderer RGBA16 texture format support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_FMT_CI')) 'Renderer CI texture format support is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_FMT_I16')) 'Renderer I16 texture format support is missing.'
Assert-True ($renderer.Contains('ndsRendererRecordLoadTlut')) 'Renderer TLUT state tracking is missing.'
Assert-True ($renderer.Contains('texture_tlut_image')) 'Renderer CI texture palette pointer tracking is missing.'
Assert-True ($renderer.Contains('texture_render_tile_size')) 'Renderer render-tile pixel size tracking is missing.'
Assert-True ($renderer.Contains('hardware_texture_upload_count')) 'Renderer hardware texture upload stats are missing.'
Assert-True ($renderer.Contains('key.render_tmem = stats->texture_render_tile_tmem')) 'Renderer hardware texture cache key is missing render TMEM state.'
Assert-True ($renderer.Contains('key.load_tile = stats->texture_load_tile')) 'Renderer hardware texture cache key is missing load-tile state.'
Assert-True ($renderer.Contains('ndsRendererHardwareColorSource')) 'Renderer hardware material color source helper is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareAlpha')) 'Renderer hardware material alpha helper is missing.'
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
Assert-True ($platform.Contains('MODE_0_3D')) 'Platform hardware renderer mode init is missing.'
Assert-True ($platform.Contains('VRAM_A_TEXTURE')) 'Platform texture VRAM bank A init is missing.'
Assert-True ($platform.Contains('VRAM_E_TEX_PALETTE')) 'Platform texture palette VRAM bank init is missing.'
Assert-True ($platform.Contains('glFlush(0)')) 'Platform hardware renderer frame flush is missing.'
Assert-True ($platform.Contains('ndsRendererHardwareConsumeSubmittedFrame')) 'Platform does not guard hardware flushes with the renderer submit latch.'
$makefile = Get-Content (Join-Path $root 'Makefile') -Raw
Assert-True ($makefile.Contains('NDS_RENDERER_HW_TRIANGLES ?= 0')) 'Makefile hardware renderer flag default is missing.'
Assert-True ($makefile.Contains('battleship_sys_matrix.c')) 'Makefile original sys/matrix import is missing.'
Assert-True ($makefile.Contains('battleship_sys_sintable.c')) 'Makefile original sine table import is missing.'
$rendererHeader = Get-Content (Join-Path $root 'include/nds/nds_renderer.h') -Raw
Assert-True ($rendererHeader.Contains('NDSRendererResolveData')) 'Renderer data resolver hook is missing.'
Assert-True ($rendererHeader.Contains('initial_projection')) 'Renderer config initial projection field is missing.'
Assert-True ($rendererHeader.Contains('initial_modelview')) 'Renderer config initial modelview field is missing.'
Assert-True ($rendererHeader.Contains('hardware_texture_ready_count')) 'Renderer hardware texture stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('matrix_mvp_recalc_count')) 'Renderer MVP-recalc stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('matrix_move_word_count')) 'Renderer matrix move-word stats are missing from the public header.'
Assert-True ($rendererHeader.Contains('transformed_vertices')) 'Renderer command transformed vertex cache exposure is missing.'
Assert-True ($rendererHeader.Contains('ndsRendererHardwareConsumeSubmittedFrame')) 'Renderer hardware submit-latch API is missing.'
$startupHeader = Get-Content (Join-Path $root 'include/nds/nds_startup.h') -Raw
Assert-True ($startupHeader.Contains('gNdsFighterDLAllDrawHardwareTextureReadyCount')) 'All-DL hardware texture diagnostics are missing from the startup header.'
$rendererAdapter = Get-Content (Join-Path $root 'src/port/reloc_backend_renderer_dl.c') -Raw
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterPrepareInitialMatrices')) 'Battle DL renderer matrix adapter is missing.'
Assert-True ($rendererAdapter.Contains('syMatrixTraRotRpyRSca')) 'Battle DL renderer does not route DObj prep through original matrix helpers.'
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterBuildDefaultBattleCameraMatrices')) 'Battle DL renderer default battle camera seed is missing.'
Assert-True ($rendererAdapter.Contains('ndsRendererAdapterProjectionDepth')) 'Battle DL renderer hardware projection-depth scaling is missing.'
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
$compatShims = Get-Content (Join-Path $root 'src/port/reloc_backend_compat_shims.c') -Raw
Assert-True ($compatShims.Contains('gcAddXObjForCamera(cobj, 0x4C, 0)')) 'VSBattle compatibility camera no longer exposes the BattleShip 0x4C matrix kind.'
$openingBackend = Get-Content (Join-Path $root 'src/port/opening_movie_backend.c') -Raw
Assert-True ($openingBackend.Contains('ndsRendererAdapterPrepareInitialMatrices')) 'Opening-room renderer seed hook is missing.'
$allDLVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-dl-draw-all-harness.ps1') -Raw
Assert-True ($allDLVerifier.Contains('HardwareTriangles')) 'All-DL verifier hardware-triangle switch is missing.'
Assert-True ($allDLVerifier.Contains('FTR_DL_ALL_HWTEX')) 'All-DL verifier hardware texture marker is missing.'
$decodeHeader = Get-Content (Join-Path $root 'include/nds/nds_gbi_decode.h') -Raw
Assert-True ($decodeHeader.Contains('/ 2u')) 'F3DEX2 packed triangle decode must stay on BattleShip index*2 packing.'
Assert-True (-not $decodeHeader.Contains('/ 10u')) 'Stale F3DEX2 packed triangle /10 decode returned.'
Write-Output 'GBI decode fixtures passed: F3DEX2 VTX/TRI/MTX/POPMTX packing, F3DEX MVP-recalc matrix move-word packing, transformed and hardware raw/material triangle readiness, N64 matrix to DS 20.12 modelview-projection/modelview-stack vertex transform, and source snippets verified.'
