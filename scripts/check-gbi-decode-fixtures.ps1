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
function Add-TextureLookupFixtureEntry {
    param(
        [byte[]]$Table,
        [object[]]$Entries,
        [int]$EntryIndex
    )
    $slotValue = $EntryIndex + 1
    $slot = [int]([uint32]$Entries[$EntryIndex].Hash -band 127u)
    for ($probe = 0; $probe -lt 128; $probe++) {
        $value = [int]$Table[$slot]
        if ($value -eq $slotValue) { return }
        if ($value -eq 0) {
            $Table[$slot] = [byte]$slotValue
            return
        }
        $slot = ($slot + 1) -band 127
    }
}
function Remove-TextureLookupFixtureEntry {
    param(
        [byte[]]$Table,
        [object[]]$Entries,
        [int]$EntryIndex
    )
    $slotValue = $EntryIndex + 1
    $slot = [int]([uint32]$Entries[$EntryIndex].Hash -band 127u)
    for ($probe = 0; $probe -lt 128; $probe++) {
        $value = [int]$Table[$slot]
        if ($value -eq 0) { return }
        if ($value -eq $slotValue) {
            $Table[$slot] = 0
            $slot = ($slot + 1) -band 127
            while ($Table[$slot] -ne 0) {
                $value = [int]$Table[$slot]
                $Table[$slot] = 0
                Add-TextureLookupFixtureEntry $Table $Entries ($value - 1)
                $slot = ($slot + 1) -band 127
            }
            return
        }
        $slot = ($slot + 1) -band 127
    }
}
function Find-TextureLookupFixtureEntry {
    param(
        [byte[]]$Table,
        [object[]]$Entries,
        [uint32]$Hash,
        [string]$Key
    )
    $slot = [int]($Hash -band 127u)
    for ($probe = 0; $probe -lt 128; $probe++) {
        $value = [int]$Table[$slot]
        if ($value -eq 0) { return -1 }
        $entry = $Entries[$value - 1]
        if ($entry.Ready -and ([uint32]$entry.Hash -eq $Hash) -and
            ([string]$entry.Key -ceq $Key)) {
            return $value - 1
        }
        $slot = ($slot + 1) -band 127
    }
    return -1
}
function Get-WallpaperForwardLastWriterMap {
    param(
        [int]$Length,
        [uint32]$ScaleQ16,
        [int]$Origin,
        [int]$Viewport
    )
    $map = [int[]]::new($Viewport)
    for ($i = 0; $i -lt $Viewport; $i++) { $map[$i] = -1 }
    for ($source = 0; $source -lt $Length; $source++) {
        $start = $Origin + [int]((([int64]$source * $ScaleQ16) -shr 16))
        $end = $Origin + [int]((((([int64]($source + 1) * $ScaleQ16) +
            0xffff) -shr 16)))
        for ($destination = $start; $destination -lt $end; $destination++) {
            if (($destination -ge 0) -and ($destination -lt $Viewport)) {
                $map[$destination] = $source
            }
        }
    }
    return ,$map
}
function Get-WallpaperInverseLastWriterMap {
    param(
        [int]$Length,
        [uint32]$ScaleQ16,
        [int]$Origin,
        [int]$Viewport
    )
    $map = [int[]]::new($Viewport)
    for ($i = 0; $i -lt $Viewport; $i++) { $map[$i] = -1 }
    $start = [Math]::Max($Origin, 0)
    $end = [Math]::Min(
        $Origin + [int]((((([int64]$Length * $ScaleQ16) + 0xffff) -shr 16))),
        $Viewport)
    for ($destination = $start; $destination -lt $end; $destination++) {
        $relative = $destination - $Origin
        $source = [int][Math]::Floor(
            [double]((((([int64]$relative + 1) -shl 16) - 1)) /
                [double]$ScaleQ16))
        $map[$destination] = [Math]::Min($source, $Length - 1)
    }
    return ,$map
}
function Get-WallpaperStagedFinalMap {
    param(
        [int]$Length,
        [uint32]$ScaleQ16,
        [int]$Origin,
        [int]$Viewport,
        [int]$Output
    )
    $staged = Get-WallpaperForwardLastWriterMap `
        -Length $Length -ScaleQ16 $ScaleQ16 -Origin $Origin `
        -Viewport $Viewport
    $map = [int[]]::new($Output)
    $step = [int][Math]::Floor(([int64]$Viewport -shl 16) / $Output)
    $sourceQ16 = [int64]($step -shr 1)
    for ($destination = 0; $destination -lt $Output; $destination++) {
        $preview = [int]($sourceQ16 -shr 16)
        if ($preview -ge $Viewport) { $preview = $Viewport - 1 }
        $map[$destination] = $staged[$preview]
        $sourceQ16 += $step
    }
    return ,$map
}
function Get-WallpaperDirectFinalMap {
    param(
        [int]$Length,
        [uint32]$ScaleQ16,
        [int]$Origin,
        [int]$Viewport,
        [int]$Output
    )
    $map = [int[]]::new($Output)
    for ($i = 0; $i -lt $Output; $i++) { $map[$i] = -1 }
    $step = [int][Math]::Floor(([int64]$Viewport -shl 16) / $Output)
    $previewQ16 = [int64]($step -shr 1)
    $drawEnd = $Origin + [int][Math]::Floor(
        (([int64]$Length * $ScaleQ16) + 0xffff) / 65536.0)
    for ($destination = 0; $destination -lt $Output; $destination++) {
        $preview = [int]($previewQ16 -shr 16)
        if (($preview -ge $Origin) -and ($preview -lt $drawEnd)) {
            $relative = $preview - $Origin
            $source = [int][Math]::Floor(
                [double]((((([int64]$relative + 1) -shl 16) - 1)) /
                    [double]$ScaleQ16))
            $map[$destination] = [Math]::Min($source, $Length - 1)
        }
        $previewQ16 += $step
    }
    return ,$map
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
function Get-Texel01Ci4LutValue {
    param(
        [uint16]$Texel0,
        [uint16]$Texel1,
        [uint32]$Fraction
    )
    if ($Fraction -gt 0xff) { $Fraction = 0xff }
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
    return [uint32]($red -bor ($green -shl 5) -bor ($blue -shl 10) -bor
        ($alphaCoverage -shl 15))
}
function Resolve-Texel01Ci4LutValue {
    param(
        [uint32]$Value,
        [uint32]$X,
        [uint32]$Y
    )
    $bayer = @(0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5)
    $threshold = ($bayer[(($Y -band 3) -shl 2) -bor ($X -band 3)] -shl 4) + 8
    $alpha = if (($Value -shr 15) -gt $threshold) { 1 } else { 0 }
    return [uint16](($Value -band 0x7fff) -bor ($alpha -shl 15))
}
function Get-Ci4HashedRepresentatives {
    param(
        [int[]]$Source0,
        [int[]]$Source1
    )
    $table = [uint32[]]::new(256)
    $representatives = [int[]]::new($Source0.Count)
    $unique = 0
    $collisionProbes = 0

    Assert-Equal $Source1.Count $Source0.Count 'CI4 hashed representative maps have different lengths.'
    for ($i = 0; $i -lt $Source0.Count; $i++) {
        $key = [uint32]((($i -band 3) -shl 16) -bor
            (($Source1[$i] -band 0xff) -shl 8) -bor
            ($Source0[$i] -band 0xff))
        $storedKey = [uint32]($key + 1)
        $product = ([uint64]$key * [uint64]2654435761) -band [uint64]4294967295
        $slot = [int](($product -shr 24) -band 0xff)

        while ($table[$slot] -ne 0) {
            $entry = $table[$slot]
            if (($entry -band 0x7ffff) -eq $storedKey) {
                $representatives[$i] = [int]($entry -shr 19)
                break
            }
            $collisionProbes++
            $slot = ($slot + 1) -band 0xff
        }
        if ($table[$slot] -eq 0) {
            $table[$slot] = [uint32](([uint32]$i -shl 19) -bor $storedKey)
            $representatives[$i] = $i
            $unique++
        }
    }
    return [PSCustomObject]@{
        Representatives = $representatives
        Unique = $unique
        CollisionProbes = $collisionProbes
    }
}
function Test-Ci4RepresentativeExpansion {
    param(
        [int[]]$Source0S,
        [int[]]$Source1S,
        [int[]]$Source0T,
        [int[]]$Source1T
    )
    $width = $Source0S.Count
    $height = $Source0T.Count
    $representativeS = [int[]]::new($width)
    $representativeT = [int[]]::new($height)
    $uniqueS = 0
    $uniqueT = 0

    Assert-Equal $Source1S.Count $width 'CI4 representative S maps have different widths.'
    Assert-Equal $Source1T.Count $height 'CI4 representative T maps have different heights.'
    for ($x = 0; $x -lt $width; $x++) {
        for ($prior = 0; $prior -lt $x; $prior++) {
            if (($Source0S[$x] -eq $Source0S[$prior]) -and
                ($Source1S[$x] -eq $Source1S[$prior]) -and
                (($x -band 3) -eq ($prior -band 3))) {
                break
            }
        }
        $representativeS[$x] = $prior
        if ($prior -eq $x) { $uniqueS++ }
    }
    for ($y = 0; $y -lt $height; $y++) {
        for ($prior = 0; $prior -lt $y; $prior++) {
            if (($Source0T[$y] -eq $Source0T[$prior]) -and
                ($Source1T[$y] -eq $Source1T[$prior]) -and
                (($y -band 3) -eq ($prior -band 3))) {
                break
            }
        }
        $representativeT[$y] = $prior
        if ($prior -eq $y) { $uniqueT++ }
    }
    $hashedS = Get-Ci4HashedRepresentatives -Source0 $Source0S -Source1 $Source1S
    $hashedT = Get-Ci4HashedRepresentatives -Source0 $Source0T -Source1 $Source1T
    Assert-Equal ([string]::Join(',', $hashedS.Representatives)) `
        ([string]::Join(',', $representativeS)) `
        'CI4 hashed S representatives diverged from first exact matches.'
    Assert-Equal ([string]::Join(',', $hashedT.Representatives)) `
        ([string]::Join(',', $representativeT)) `
        'CI4 hashed T representatives diverged from first exact matches.'
    $representativeS = $hashedS.Representatives
    $representativeT = $hashedT.Representatives
    $uniqueS = $hashedS.Unique
    $uniqueT = $hashedT.Unique

    $direct = [uint16[]]::new($width * $height)
    $expanded = [uint16[]]::new($width * $height)
    for ($y = 0; $y -lt $height; $y++) {
        for ($x = 0; $x -lt $width; $x++) {
            $value = (($Source0S[$x] * 109) +
                ($Source1S[$x] * 521) +
                ($Source0T[$y] * 1237) +
                ($Source1T[$y] * 3571) +
                (($x -band 3) * 8191) +
                (($y -band 3) * 16381)) -band 0xffff
            $direct[($y * $width) + $x] = [uint16]$value
        }
    }
    for ($y = 0; $y -lt $height; $y++) {
        if ($representativeT[$y] -ne $y) { continue }
        $row = $y * $width
        for ($x = 0; $x -lt $width; $x++) {
            if ($representativeS[$x] -eq $x) {
                $expanded[$row + $x] = $direct[$row + $x]
            } else {
                $expanded[$row + $x] =
                    $expanded[$row + $representativeS[$x]]
            }
        }
    }
    for ($y = $height - 1; $y -ge 0; $y--) {
        $representativeY = $representativeT[$y]
        if ($representativeY -eq $y) { continue }
        for ($x = 0; $x -lt $width; $x++) {
            $expanded[($y * $width) + $x] =
                $expanded[($representativeY * $width) + $x]
        }
    }

    Assert-Equal ([string]::Join(',', $expanded)) ([string]::Join(',', $direct)) `
        'CI4 representative expansion diverged from direct per-pixel evaluation.'
    return [PSCustomObject]@{
        UniqueS = $uniqueS
        UniqueT = $uniqueT
        UniquePixels = $uniqueS * $uniqueT
        ReusedPixels = ($width * $height) - ($uniqueS * $uniqueT)
        HashCollisionProbes = $hashedS.CollisionProbes +
            $hashedT.CollisionProbes
    }
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
function Convert-ToRawGXCombinedMtx20p12 {
    param([object[]]$Mtx)
    $out = @()
    for ($row = 0; $row -lt 4; $row++) {
        $outRow = @()
        for ($col = 0; $col -lt 4; $col++) {
            $value = [Int64]$Mtx[$row][$col]
            if ($row -eq 3) {
                $value = Round-Shift-S64 -Value $value -Shift 8
            }
            $outRow += [int]$value
        }
        $out += ,$outRow
    }
    return $out
}
function Transform-RawGXVertex20p12 {
    param(
        [object[]]$Mtx,
        [int]$X,
        [int]$Y,
        [int]$Z
    )
    $raw = @(
        ([Int64]$X -shl 4),
        ([Int64]$Y -shl 4),
        ([Int64]$Z -shl 4)
    )
    $result = @()
    for ($col = 0; $col -lt 4; $col++) {
        [Int64]$sum = 0
        for ($row = 0; $row -lt 3; $row++) {
            $sum += [Int64]$Mtx[$row][$col] * $raw[$row]
        }
        $result += [Int64](Round-Shift-S64 -Value $sum -Shift 12) +
            [Int64]$Mtx[3][$col]
    }
    return @{ X=$result[0]; Y=$result[1]; Z=$result[2]; W=$result[3] }
}
function Assert-RawGXHomogeneousEquivalent {
    param(
        [object[]]$Mtx,
        [int]$X,
        [int]$Y,
        [int]$Z,
        [string]$Label
    )
    $cpu = Transform-Vertex20p12 -Mtx $Mtx -X $X -Y $Y -Z $Z
    $hardwareMtx = Convert-ToRawGXCombinedMtx20p12 -Mtx $Mtx
    $hardware = Transform-RawGXVertex20p12 -Mtx $hardwareMtx -X $X -Y $Y -Z $Z
    foreach ($axis in @('X','Y','Z')) {
        [Int64]$lhs = [Int64]$hardware[$axis] * [Int64]$cpu.W
        [Int64]$rhs = [Int64]$cpu[$axis] * [Int64]$hardware.W
        [Int64]$delta = [Math]::Abs($lhs - $rhs)
        [Int64]$scale = [Math]::Abs([Int64]$cpu[$axis]) + [Math]::Abs([Int64]$cpu.W)
        if ($scale -eq 0) { $scale = 1 }
        [Int64]$error = [Math]::Ceiling([double]$delta / [double]$scale)
        Assert-True ($error -le 16) "$Label raw GX $axis/W cross-product error exceeded 16 fixed LSBs."
    }
    Assert-True (([Math]::Sign([Int64]$hardware.W) -eq [Math]::Sign([Int64]$cpu.W))) `
        "$Label raw GX W sign drifted."
}
function Get-HybridSubmitClass {
    param(
        [bool]$SourceZ,
        [bool]$Decal,
        [bool]$PrimDepth,
        [bool]$MatrixCompatible,
        [bool]$CurrentSlots,
        [bool]$SameSnapshot,
        [bool]$RawCoordinatesFit
    )
    if (-not $SourceZ) { return 'projected_no_z' }
    if ($Decal) { return 'projected_decal' }
    if ($PrimDepth) { return 'projected_prim_depth' }
    if (-not $RawCoordinatesFit) { return 'projected_range_or_matrix' }
    if ($CurrentSlots) {
        if (-not $MatrixCompatible) { return 'projected_range_or_matrix' }
        return 'raw_z_current_matrix'
    }
    if ($SameSnapshot) { return 'raw_z_snapshot_matrix' }
    return 'projected_cross_matrix'
}
function Get-HybridProjectedDivisions {
    param([string]$SubmitClass)
    if (($SubmitClass -eq 'raw_z_current_matrix') -or
        ($SubmitClass -eq 'raw_z_snapshot_matrix')) { return 0 }
    if ($SubmitClass -eq 'projected_no_z') { return 6 }
    return 9
}
function Clamp-ProjectedV16 {
    param([Int64]$Value)
    if ($Value -lt -32768) { return -32768 }
    if ($Value -gt 32767) { return 32767 }
    return [int]$Value
}
function Get-ProjectedV16Reference {
    param([Int64]$Numerator, [int]$Denominator)
    if ($Denominator -eq 0) { return 0 }
    [Int64]$quotient = [Math]::Truncate(
        ([decimal]$Numerator) / ([decimal]$Denominator))
    return Clamp-ProjectedV16 $quotient
}
function Get-ProjectedV16Preclamped {
    param([Int64]$Numerator, [int]$Denominator)
    if ($Denominator -eq 0) { return 0 }
    [Int64]$lowProduct = [Int64]-32768 * [Int64]$Denominator
    [Int64]$highProduct = [Int64]32767 * [Int64]$Denominator
    if ((($Denominator -gt 0) -and ($Numerator -lt $lowProduct)) -or
        (($Denominator -lt 0) -and ($Numerator -gt $lowProduct))) {
        return -32768
    }
    if ((($Denominator -gt 0) -and ($Numerator -gt $highProduct)) -or
        (($Denominator -lt 0) -and ($Numerator -lt $highProduct))) {
        return 32767
    }
    return Get-ProjectedV16Reference $Numerator $Denominator
}
function Get-VertexLoadTransformPolicy {
    param([int]$ProfileLevel, [bool]$SnapshotAvailable)
    if (($ProfileLevel -ge 2) -or (-not $SnapshotAvailable)) {
        return 'eager_transform'
    }
    return 'defer_transform'
}
function Get-ProjectedTransformPolicy {
    param([bool]$ClipValid, [bool]$ClipMatchesVertexSnapshot)
    if ($ClipValid -and $ClipMatchesVertexSnapshot) {
        return 'cache_hit'
    }
    return 'transform_snapshot'
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
Assert-RawGXHomogeneousEquivalent -Mtx $identity -X -2048 -Y 0 -Z 2047 -Label 'Identity/range-edge'
Assert-RawGXHomogeneousEquivalent -Mtx $transform -X 12 -Y -3 -Z 8 -Label 'Scale/translation'
Assert-RawGXHomogeneousEquivalent -Mtx $combined -X 12 -Y -3 -Z 8 -Label 'Composed modelview/projection'
$perspectiveRows = @(
    @(1.25, 0.0, 0.0, 0.0),
    @(0.0, -0.75, 0.0, 0.0),
    @(0.0, 0.0, 1.5, -1.0),
    @(3.123, -2.251, 0.503, 7.0)
)
$perspective = Convert-N64PackedMtxTo20p12 (New-N64PackedMtx -Rows $perspectiveRows)
Assert-RawGXHomogeneousEquivalent -Mtx $perspective -X 100 -Y -200 -Z 2 -Label 'Perspective/positive-W'
Assert-RawGXHomogeneousEquivalent -Mtx $perspective -X -300 -Y 400 -Z 10 -Label 'Perspective/negative-W'
Assert-Equal (Get-HybridSubmitClass $false $false $false $true $true $false $true) 'projected_no_z' 'Hybrid raw policy did not preserve no-Z projection.'
Assert-Equal (Get-HybridSubmitClass $true $true $false $true $true $false $true) 'projected_decal' 'Hybrid raw policy did not preserve decal projection.'
Assert-Equal (Get-HybridSubmitClass $true $false $true $true $true $false $true) 'projected_prim_depth' 'Hybrid raw policy did not preserve primitive-depth projection.'
Assert-Equal (Get-HybridSubmitClass $true $false $false $false $true $false $true) 'projected_range_or_matrix' 'Hybrid raw policy accepted an incompatible current matrix.'
Assert-Equal (Get-HybridSubmitClass $true $false $false $true $false $false $true) 'projected_cross_matrix' 'Hybrid raw policy accepted mixed matrix snapshots.'
Assert-Equal (Get-HybridSubmitClass $true $false $false $true $false $true $true) 'raw_z_snapshot_matrix' 'Hybrid raw policy rejected three slots from one retained matrix snapshot.'
Assert-Equal (Get-HybridSubmitClass $true $false $false $true $true $false $false) 'projected_range_or_matrix' 'Hybrid raw policy accepted saturated coordinates.'
Assert-Equal (Get-HybridSubmitClass $true $false $false $true $true $false $true) 'raw_z_current_matrix' 'Hybrid raw policy rejected compatible current-matrix source-Z.'
Assert-Equal (Get-HybridProjectedDivisions 'raw_z_current_matrix') 0 'Raw-current class retained projected divisions.'
Assert-Equal (Get-HybridProjectedDivisions 'raw_z_snapshot_matrix') 0 'Raw-snapshot class retained projected divisions.'
Assert-Equal (Get-HybridProjectedDivisions 'projected_no_z') 6 'No-Z projected division accounting drifted.'
Assert-Equal (Get-HybridProjectedDivisions 'projected_cross_matrix') 9 'Source-Z projected division accounting drifted.'
$projectedDivisionCases = @(
    @{ Numerator = [Int64]0; Denominator = 1 },
    @{ Numerator = [Int64]-1; Denominator = 2 },
    @{ Numerator = [Int64]1; Denominator = -2 },
    @{ Numerator = ([Int64]32767 * 4095); Denominator = 4095 },
    @{ Numerator = (([Int64]32767 * 4095) + 1); Denominator = 4095 },
    @{ Numerator = ([Int64]-32768 * 4095); Denominator = 4095 },
    @{ Numerator = (([Int64]-32768 * 4095) - 1); Denominator = 4095 },
    @{ Numerator = ([Int64]-32768 * -4095); Denominator = -4095 },
    @{ Numerator = (([Int64]-32768 * -4095) + 1); Denominator = -4095 },
    @{ Numerator = ([Int64]32767 * -4095); Denominator = -4095 },
    @{ Numerator = (([Int64]32767 * -4095) - 1); Denominator = -4095 },
    @{ Numerator = ([Int64][Int32]::MaxValue * 4096); Denominator = 1 },
    @{ Numerator = ([Int64][Int32]::MinValue * 4096); Denominator = 1 },
    @{ Numerator = ([Int64]32767 * [Int32]::MinValue); Denominator = [Int32]::MinValue },
    @{ Numerator = ([Int64]-32768 * [Int32]::MinValue); Denominator = [Int32]::MinValue }
)
foreach ($divideCase in $projectedDivisionCases) {
    Assert-Equal (Get-ProjectedV16Preclamped `
        -Numerator $divideCase.Numerator `
        -Denominator $divideCase.Denominator) `
        (Get-ProjectedV16Reference `
            -Numerator $divideCase.Numerator `
            -Denominator $divideCase.Denominator) `
        "Projected signed pre-clamp drifted for $($divideCase.Numerator)/$($divideCase.Denominator)."
}
Assert-Equal (Get-VertexLoadTransformPolicy 0 $true) 'defer_transform' 'Performance profile did not defer a snapshotted source vertex.'
Assert-Equal (Get-VertexLoadTransformPolicy 1 $true) 'defer_transform' 'Coarse profile did not defer a snapshotted source vertex.'
Assert-Equal (Get-VertexLoadTransformPolicy 2 $true) 'eager_transform' 'Forensic profile did not retain eager oracle transforms.'
Assert-Equal (Get-VertexLoadTransformPolicy 0 $false) 'eager_transform' 'Snapshot-overflow fallback did not preserve a usable clip vertex.'
Assert-Equal (Get-ProjectedTransformPolicy $true $true) 'cache_hit' 'Matching projected snapshot did not reuse its clip transform.'
Assert-Equal (Get-ProjectedTransformPolicy $true $false) 'transform_snapshot' 'A stale clip transform survived matrix-snapshot invalidation.'
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
foreach ($layout in @('Native', 'O2R')) {
    $packedCi4 = if ($layout -eq 'O2R') { $o2rCi4 } else { $nativeCi4 }
    for ($i = 0; $i -lt 8; $i += 2) {
        $packed = Get-TextureByte $packedCi4 ($i -shr 1) $layout
        Assert-Equal ($packed -shr 4) (Get-TextureNibble $packedCi4 $i $layout) "$layout CI4 paired high nibble diverged at texel $i."
        Assert-Equal ($packed -band 0x0f) (Get-TextureNibble $packedCi4 ($i + 1) $layout) "$layout CI4 paired low nibble diverged at texel $($i + 1)."
    }
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
$lutPalette0 = [uint16[]](0..15 | ForEach-Object {
    (($_ * 3) -band 0x1f) -bor (((($_ * 5) -band 0x1f)) -shl 5) -bor
        (((($_ * 7) -band 0x1f)) -shl 10) -bor ((($_ -band 1)) -shl 15)
})
$lutPalette1 = [uint16[]](0..15 | ForEach-Object {
    (((31 - $_ * 2) -band 0x1f)) -bor (((($_ * 11) -band 0x1f)) -shl 5) -bor
        (((($_ * 13) -band 0x1f)) -shl 10) -bor (((($_ + 1) -band 1)) -shl 15)
})
$alphaPhasePrefix = [uint16[]](
    0x0000, 0x0001, 0x0401, 0x0405, 0x0505, 0x0525,
    0x8525, 0x85a5, 0xa5a5, 0xa5a7, 0xada7, 0xadaf,
    0xafaf, 0xafbf, 0xefbf, 0xefff, 0xffff
)
foreach ($lutFraction in @(0, 1, 0x40, 0x72, 0xff)) {
    for ($lutIndex0 = 0; $lutIndex0 -lt 16; $lutIndex0++) {
        for ($lutIndex1 = 0; $lutIndex1 -lt 16; $lutIndex1++) {
            $lutValue = Get-Texel01Ci4LutValue `
                -Texel0 $lutPalette0[$lutIndex0] `
                -Texel1 $lutPalette1[$lutIndex1] `
                -Fraction $lutFraction
            for ($lutY = 0; $lutY -lt 4; $lutY++) {
                for ($lutX = 0; $lutX -lt 4; $lutX++) {
                    $lutExpected = Blend-Texel01Rgb5551 `
                        -Texel0 $lutPalette0[$lutIndex0] `
                        -Texel1 $lutPalette1[$lutIndex1] `
                        -Fraction $lutFraction -X $lutX -Y $lutY
                    $lutActual = Resolve-Texel01Ci4LutValue `
                        -Value $lutValue -X $lutX -Y $lutY
                    if ($lutActual -ne $lutExpected) {
                        throw "TEXEL0/TEXEL1 CI4 lookup diverged at pair $lutIndex0/$lutIndex1, fraction $lutFraction, pixel $lutX/${lutY}: expected $lutExpected, got $lutActual."
                    }
                    $alphaCoverage = $lutValue -shr 15
                    $alphaPrefixCount = [Math]::Min(16, (($alphaCoverage + 7) -shr 4))
                    $phase = (($lutY -band 3) -shl 2) -bor ($lutX -band 3)
                    $phaseAlpha = ($alphaPhasePrefix[$alphaPrefixCount] -shr $phase) -band 1
                    $phaseActual = ($lutValue -band 0x7fff) -bor ($phaseAlpha -shl 15)
                    if ($phaseActual -ne $lutExpected) {
                        throw "TEXEL0/TEXEL1 CI4 phase mask diverged at pair $lutIndex0/$lutIndex1, fraction $lutFraction, pixel $lutX/${lutY}: expected $lutExpected, got $phaseActual."
                    }
                }
            }
        }
    }
}
$phaseOnlyMap = Test-Ci4RepresentativeExpansion `
    -Source0S ([int[]](0..7 | ForEach-Object { 0 })) `
    -Source1S ([int[]](0..7 | ForEach-Object { 0 })) `
    -Source0T ([int[]](0..7 | ForEach-Object { 0 })) `
    -Source1T ([int[]](0..7 | ForEach-Object { 0 }))
Assert-Equal $phaseOnlyMap.UniqueS 4 'CI4 representative S map collapsed distinct ordered-coverage phases.'
Assert-Equal $phaseOnlyMap.UniqueT 4 'CI4 representative T map collapsed distinct ordered-coverage phases.'
$nonperiodicSource0S = [int[]](0..127 | ForEach-Object { [Math]::Min($_, 31) })
$nonperiodicSource1S = [int[]](0..127 | ForEach-Object { ($_ + 13) -band 31 })
$nonperiodicSource0T = [int[]](0..127 | ForEach-Object { [Math]::Min($_, 30) })
$nonperiodicSource1T = [int[]](0..127 | ForEach-Object { ($_ + 7) -band 31 })
$nonperiodicMap = Test-Ci4RepresentativeExpansion `
    -Source0S $nonperiodicSource0S -Source1S $nonperiodicSource1S `
    -Source0T $nonperiodicSource0T -Source1T $nonperiodicSource1T
Assert-True ($nonperiodicMap.UniquePixels -gt 0 -and
    ($nonperiodicMap.UniquePixels * 2) -le (128 * 128) -and
    $nonperiodicMap.ReusedPixels -ge $nonperiodicMap.UniquePixels -and
    $nonperiodicMap.HashCollisionProbes -gt 0) `
    'CI4 nonperiodic representative fixture no longer exercises the retained large-texture reuse threshold.'
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
$wallpaperMapCases = @(
    @{ Length = 300; Scale = 65536; Origin = 0; Viewport = 320 },
    @{ Length = 300; Scale = 65798; Origin = -17; Viewport = 320 },
    @{ Length = 300; Scale = 65798; Origin = 9; Viewport = 320 },
    @{ Length = 300; Scale = 98304; Origin = -83; Viewport = 320 },
    @{ Length = 300; Scale = 131072; Origin = -150; Viewport = 320 },
    @{ Length = 220; Scale = 65798; Origin = -11; Viewport = 240 },
    @{ Length = 220; Scale = 131072; Origin = 7; Viewport = 240 }
)
foreach ($case in $wallpaperMapCases) {
    $forward = Get-WallpaperForwardLastWriterMap @case
    $inverse = Get-WallpaperInverseLastWriterMap @case
    Assert-Equal ($inverse -join ',') ($forward -join ',') `
        "Wallpaper inverse last-writer mapping drifted at length=$($case.Length) scale=$($case.Scale) origin=$($case.Origin)."
    $output = if ($case.Viewport -eq 320) { 256 } else { 192 }
    $stagedFinal = Get-WallpaperStagedFinalMap @case -Output $output
    $directFinal = Get-WallpaperDirectFinalMap @case -Output $output
    Assert-Equal ($directFinal -join ',') ($stagedFinal -join ',') `
        "Wallpaper final-resolution composition drifted from the exact two-stage map at length=$($case.Length) scale=$($case.Scale) origin=$($case.Origin)."
}
# Synthetic six-row/two-strip input with a one-row logical overlap. The later
# strip's first row must replace the earlier strip's last row before scaling.
$overlapRows = @(
    @{ LogicalY = 0; Color = 10 },
    @{ LogicalY = 1; Color = 11 },
    @{ LogicalY = 1; Color = 20 },
    @{ LogicalY = 2; Color = 21 }
)
foreach ($origin in @(-1, 1)) {
    $viewport = 6
    $scale = 65798
    $forwardColors = [int[]]::new($viewport)
    foreach ($row in $overlapRows) {
        $start = $origin + [int]((([int64]$row.LogicalY * $scale) -shr 16))
        $end = $origin + [int]((((([int64]($row.LogicalY + 1) * $scale) +
            0xffff) -shr 16)))
        for ($destination = $start; $destination -lt $end; $destination++) {
            if (($destination -ge 0) -and ($destination -lt $viewport)) {
                $forwardColors[$destination] = $row.Color
            }
        }
    }
    $inverseSources = Get-WallpaperInverseLastWriterMap `
        -Length 3 -ScaleQ16 $scale -Origin $origin -Viewport $viewport
    $flattenedColors = @(10, 20, 21)
    $inverseColors = [int[]]::new($viewport)
    for ($destination = 0; $destination -lt $viewport; $destination++) {
        if ($inverseSources[$destination] -ge 0) {
            $inverseColors[$destination] =
                $flattenedColors[$inverseSources[$destination]]
        }
    }
    Assert-Equal ($inverseColors -join ',') ($forwardColors -join ',') `
        "Wallpaper overlapping-strip flattening drifted at origin=$origin."
}
$spritePreview = Get-Content (Join-Path $root 'src/port/sprite_preview_backend.c') -Raw
Assert-True ($spritePreview.Contains('return ((((relative + 1u) << 16) - 1u) / scale_q16);')) 'Wallpaper inverse helper no longer uses the proven last-writer equation.'
Assert-True ([regex]::Matches($spritePreview, 'ndsSObjWallpaperLastSource\(').Count -ge 3) 'Wallpaper fast path no longer routes both axes through the proven inverse helper.'
Assert-True ($spritePreview.Contains('ndsSObjDrawOpaqueWallpaperFinal')) 'Dream Land wallpaper no longer has a direct final-resolution renderer.'
Assert-True ($spritePreview.Contains('(preview_width << 16) / overlay_width')) 'Direct wallpaper X mapping no longer composes the exact preview-to-screen nearest step.'
Assert-True ($spritePreview.Contains('(preview_height << 16) / overlay_height')) 'Direct wallpaper Y mapping no longer composes the exact preview-to-screen nearest step.'
Assert-True ($spritePreview.Contains('ndsSObjWallpaperFinalKeyMatches')) 'Direct wallpaper output no longer keys retained BG2 ownership and live source state.'
Assert-True ($spritePreview.Contains('sNdsSObjFramePendingWallpaper')) 'Layered SObj path no longer defers the isolated wallpaper until its background boundary.'
Assert-True ($spritePreview.Contains('ndsSObjPreviewBasicSupported(sobj) == FALSE')) 'Layered SObj path again clears staging before rejecting unsupported source formats.'
Assert-True ($spritePreview.Contains('sNdsSObjOverlayForegroundPopulated')) 'Layered SObj path no longer retains BG3 until a populated foreground becomes empty.'
Assert-True ($spritePreview.Contains('u32 *dst_pairs = (u32 *)dst;')) 'Opaque Dream Land wallpaper no longer packs aligned final BG2 rows into word stores.'
Assert-True ($spritePreview.Contains('((u32)src[source_x_map[x + 1u]] << 16)')) 'Packed wallpaper row path no longer preserves both exact RGB5A1 samples.'
Assert-True ($spritePreview.Contains('static s32 __attribute__((hot, optimize("O3")))')) 'Measured wallpaper hot path lost its targeted latency optimization.'
$platform = Get-Content (Join-Path $root 'src/nds/nds_platform.c') -Raw
Assert-True ($platform.Contains('ndsPlatformGetOriginalSpriteOverlayLayer')) 'DS platform no longer exposes bounded final-layer ownership for direct wallpaper composition.'
Assert-True ($platform.Contains('ndsPlatformCommitOriginalSpriteFinalLayer')) 'DS platform no longer publishes direct final-layer commits and ownership epochs.'
$taskman = Get-Content (Join-Path $root 'src/port/taskman_seam.c') -Raw
$harnessScript = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1') -Raw
$rendererUploadPair = [regex]::Match(
    $harnessScript, '(?s)function Test-RendererUploadPair \{.*?\n\}').Value
Assert-True (-not [string]::IsNullOrWhiteSpace($rendererUploadPair) -and $rendererUploadPair.Contains('$Count -eq 0 -and $Bytes -eq 0') -and $rendererUploadPair.Contains('$Count -eq 1 -and ($Bytes -eq 4096 -or $Bytes -eq 32768)') -and $rendererUploadPair.Contains('$Count -eq 2 -and $Bytes -eq 36864') -and -not $rendererUploadPair.Contains('ProfileLevel')) 'Renderer upload-pair gate no longer accepts every exact animated-water phase uniformly across profiles.'
Assert-True ($renderer.Contains('ndsRendererMtxCellS16p16')) 'Renderer matrix unpack helper is missing.'
Assert-True ($relocRendererDL.Contains('ndsRendererAdapterGetFrameCameraMatrices')) 'Renderer adapter does not reuse immutable camera matrices within one BattleShip draw frame.'
Assert-True ($relocRendererDL.Contains('sNdsRendererAdapterCameraCacheFrame != frame')) 'Renderer adapter camera cache is not bounded to one presented frame.'
Assert-True ($relocRendererDL.Contains('ndsRendererAdapterFindDObjWorldMatrix')) 'Renderer adapter does not reuse parent DObj world matrices within one BattleShip draw frame.'
Assert-True ($relocRendererDL.Contains('sNdsRendererAdapterDObjWorldCacheFrame != frame')) 'Renderer adapter DObj world cache is not bounded to one presented frame.'
Assert-True ($relocRendererDL.Contains('NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT 128u')) 'Renderer adapter DObj world cache lost its measured fixed-capacity fallback bound.'
Assert-True ($relocRendererDL.Contains('syTaskmanMalloc(bytes, 0x10u)')) 'Renderer adapter DObj world cache returned to scarce static memory instead of scene-owned taskman storage.'
Assert-True ($relocRendererDL.Contains('ndsRelocUpdateMemoryLedger();')) 'Renderer adapter scene-heap cache is absent from the P1 reserve ledger.'
Assert-True ($relocAssets.Contains('ndsRendererAdapterResetSceneCaches();')) 'Renderer adapter scene-owned cache is not invalidated before taskman heap reuse.'
Assert-True ($rendererHeader.Contains('NDSRendererImmutableCommandSpan immutable_command_span')) 'Renderer config cannot distinguish immutable source spans from dynamic task-heap lists.'
Assert-True ($renderer.Contains('config->immutable_command_span(dl, config->user)')) 'Renderer does not query one validated contiguous span for immutable source display lists.'
Assert-True ($renderer.Contains('i >= immutable_command_count')) 'Renderer returned to per-command validation inside proven immutable source spans.'
Assert-True ($renderer.Contains('(i + 1u) < immutable_command_count')) 'Renderer does not bound exact TRI-run replay to the proven immutable source span.'
Assert-True ($renderer.Contains('sNdsRendererProfileTriangleRunReuseCount++')) 'Renderer does not prove immutable adjacent TRI-run replay at runtime.'
Assert-True ($renderer.Contains('prepared_vertex_colors[NDS_RENDERER_MAX_VTX]')) 'Renderer no longer retains exact derived vertex colors within one unchanged TRI run.'
Assert-True ($renderer.Contains('prepared_texcoord_s[NDS_RENDERER_MAX_VTX]')) 'Renderer no longer retains exact scaled texture coordinates within one unchanged TRI run.'
Assert-True ($renderer.Contains('prepared_projected_x[NDS_RENDERER_MAX_VTX]')) 'Renderer no longer retains exact projected coordinates within one unchanged TRI run.'
Assert-True ($renderer.Contains('state->prepared_projected_source_z_valid_mask = 0u;')) 'Renderer does not invalidate derived projected-depth values at source-command boundaries.'
Assert-True ([regex]::Matches($renderer, 'static (?:void|u32) NDS_RENDERER_HOT_CODE').Count -eq 6) 'Renderer hot-code set drifted from the six measured texture/VTX/shade/vertex/triangle/scan paths.'
Assert-True ($renderer -match '(?s)#define NDS_RENDERER_HOT_CODE.*?optimize\("O3"\).*?target\("arm"\).*?section\("\.itcm"\)') 'Renderer hot-code policy no longer combines targeted O3, ARM state, and ITCM placement.'
Assert-True ($renderer -match '(?s)#define NDS_RENDERER_FAST_RUN_CODE.*?noinline.*?optimize\("O3"\).*?target\("arm"\)' -and $renderer -match 'static void NDS_RENDERER_FAST_RUN_CODE ndsRendererExecuteFastRawCurrentRun') 'Renderer shared raw-current run kernel is not isolated as one noinline ARM/O3 lab call.'
Assert-True ((Get-Content -LiteralPath (Join-Path $root 'scripts\check-renderer-itcm-placement.ps1') -Raw) -match "Hot renderer symbol.*escaped") 'Renderer ITCM post-link placement assertion is missing.'
Assert-True ($harnessScript.Contains("check-renderer-itcm-placement.ps1")) 'Canonical/coarse/forensic verifier no longer enforces renderer ITCM placement.'
Assert-True ($relocRendererDL.Contains('ndsRendererAdapterImmutableCommandSpan')) 'Battle renderer does not classify reloc-backed display-list topology as immutable.'
Assert-True ($relocRendererDL -match '(?s)static s32 ndsRendererAdapterStageValidateRange.*?ndsFighterDLScanRangeInTaskmanArena\(dl, bytes\) == FALSE\).*?ndsRelocFindLoadedFileContaining\(dl, bytes\) == NULL\).*?ndsRendererAdapterRangeIsEmptySegmentEDL\(dl, bytes\) == FALSE') 'Live stage validation no longer short-circuits taskman-arena commands before searching the reloc ledger.'
Assert-True ($relocRendererDL -match '(?s)static s32 ndsFighterDLAllDrawValidateRange.*?ndsFighterDLScanRangeInTaskmanArena\(dl, bytes\) == FALSE\).*?ndsRelocFindLoadedFileContaining\(dl, bytes\) == NULL\).*?ndsRendererAdapterRangeIsEmptySegmentEDL\(dl, bytes\) == FALSE') 'Live fighter validation no longer short-circuits taskman-arena commands before searching the reloc ledger.'
Assert-True ($harnessScript.Contains('RENDER_TOPOLOGY=')) 'Mode 163 does not prove immutable-span validation coverage and dynamic fallback.'
Assert-True ($harnessScript.Contains('RENDER_COST=')) 'Mode 163 does not expose aggregate triangle and vertex submission cost.'
Assert-True (-not $renderer.Contains('typedef struct NDSRendererHardwareVertexContext')) 'Renderer restored the measured per-triangle vertex-context restaging.'
Assert-True ($renderer.Contains('u32 texture_prepare_origin_s;') -and $renderer.Contains('u32 texture_prepare_origin_t;') -and $renderer.Contains('u32 texture_prepare_vertex_flags;')) 'Renderer traversal state no longer retains the exact texture/vertex context across a prepared epoch.'
Assert-True ($renderer.Contains('NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH')) 'Renderer compact vertex context no longer carries all exact depth/source-clip modes.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareSubmitVertex\(\s*NDSRendererStats \*stats,\s*NDSRendererTraversalState \*state,\s*u32 vertex_index,\s*s32 projected_z\s*#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*, NDSRendererSemanticVertex \*semantic_vertex\s*#endif\s*\)') 'Renderer no longer preprocesses profile 0/1 to the original four-argument vertex ABI with one profile-2-only semantic pointer.'
Assert-True ($renderer -match '(?s)if \(state->texture_prepare_valid == 0u\).*?state->texture_prepare_scale_s = texture_scale_s;\s*state->texture_prepare_scale_t = texture_scale_t;\s*state->texture_prepare_origin_s = render_tile->uls;\s*state->texture_prepare_origin_t = render_tile->ult;\s*state->texture_prepare_offset = texture_offset;') 'Renderer no longer prepares invariant texture coordinates once per exact state epoch.'
Assert-True ($renderer -match '(?s)state->texture_prepare_vertex_flags =\s*\(state->texture_prepare_vertex_flags &\s*NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK\).*?#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*vertex_submit_start = cpuGetTiming\(\);.*?ndsRendererHardwareSubmitVertex\(stats, state, i0, projected_z\[0\]\s*#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*, &semantic_event\.vertex\[0\]\s*#endif\s*\);\s*ndsRendererHardwareSubmitVertex\(stats, state, i1, projected_z\[1\]\s*#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*, &semantic_event\.vertex\[1\]\s*#endif\s*\);\s*ndsRendererHardwareSubmitVertex\(stats, state, i2, projected_z\[2\]\s*#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*, &semantic_event\.vertex\[2\]\s*#endif\s*\);') 'Renderer does not retain one exact prepared context before the measured three-vertex sequence with profile-2-only timing and semantic capture.'
Assert-True ([regex]::Matches($renderer, '(?s)ndsRendererHardwareSubmitVertex\(stats, state, i[0-2], projected_z\[[0-2]\]\s*#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*, &semantic_event\.vertex\[[0-2]\]\s*#endif\s*\);').Count -eq 3 -and $renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*NDSRendererSemanticEvent semantic_event;\s*#endif' -and -not ($renderer -match 's32 projected_z\s*,\s*NDSRendererSemanticVertex') -and -not ($renderer -match 'projected_z\[[0-2]\]\s*,\s*&semantic_event')) 'Renderer exposes semantic vertex argument/storage outside profile-2 guards.'
Assert-True ($harnessScript.Contains('RENDER_CI4LUT=')) 'Mode 163 does not expose exact animated CI4 LUT build/reuse coverage.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_CI4_INDEX_CACHE_COUNT 2u') -and $renderer.Contains('NDS_RENDERER_HW_CI4_INDEX_CACHE_TEXELS 1024u')) 'Performance renderer CI4 source-index cache lost its exact two-plane 32x32 bound.'
Assert-True ($renderer.Contains('sizeof(sNdsRendererHardwareCi4IndexCache) == 2080u')) 'Performance renderer CI4 source-index cache exceeded its measured 2080-byte DS bound.'
Assert-True ($renderer.Contains('== 512u') -and $renderer.Contains('CI4 representative maps must stay within 512 bytes')) 'Performance renderer CI4 representative maps exceeded their measured 512-byte DS bound.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareGetCi4Indices.*?entry->source == source.*?entry->source_texels == source_texels.*?entry->byte_lane_xor == byte_lane_xor.*?entry->indices\[i\] = ndsRendererHardwareReadCi4Direct') 'Performance renderer CI4 source-index cache no longer keys and decodes the exact immutable packed source plane.'
Assert-True ($renderer -match '(?s)replace_index = sNdsRendererHardwareCi4IndexCacheNext;.*?indices ==\s*protected_indices.*?replace_index = \(replace_index \+ 1u\).*?sNdsRendererHardwareCi4IndexCacheNext =\s*\(replace_index \+ 1u\)') 'CI4 pair acquisition can evict the first returned source plane during the second lookup.'
Assert-True ($renderer -match '(?s)if \(\(indices0 != NULL\) && \(indices1 != NULL\)\).*?index0 = indices0\[.*?index1 = indices1\[.*?return;') 'Performance CI4 source-index cache no longer preserves live tile addressing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_CI4_CLASS_TABLE_COUNT 256u') -and $renderer.Contains('CI4 representative class table must stay within 1 KiB')) 'Performance CI4 representative class table lost its measured half-full 1 KiB bound.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareBuildCi4RepresentativeMap.*?key = \(\(i & 3u\) << 16\).*?source1\[i\] << 8.*?source0\[i\].*?entry & NDS_RENDERER_HW_CI4_CLASS_KEY_MASK.*?stored_key.*?slot = \(slot \+ 1u\).*?representative\[i\] = \(u8\)i') 'Performance CI4 representative hash no longer preserves both source addresses, ordered-coverage phase, collision probing, and first exact match.'
Assert-True ($renderer -match '(?s)unique_s = ndsRendererHardwareBuildCi4RepresentativeMap\(\s*sNdsRendererHardwareTexel01Ci4Source0S.*?unique_t = ndsRendererHardwareBuildCi4RepresentativeMap\(\s*sNdsRendererHardwareTexel01Ci4Source0T') 'Performance CI4 S/T representative maps no longer share the exact bounded class index.'
Assert-True ($renderer -match '(?s)unique_texels \* 2u\) <= texels.*?representative_x != x.*?dst_index \+ representative_x.*?for \(y = height; y != 0u; \).*?memcpy\(') 'Performance CI4 representative path no longer applies its measured threshold or expands from already-written first representatives.'
Assert-True ($harnessScript.Contains('RENDER_CI4MAP=') -and $harnessScript.Contains('Forensic renderer unexpectedly reused performance CI4 representative maps.')) 'Mode 163 no longer proves representative-map reuse while keeping the forensic decoder independent.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_TEXTURE_LOOKUP_COUNT 128u') -and $renderer.Contains('texture lookup must retain an empty cluster terminator')) 'Performance texture lookup lost its measured 128-slot byte table or half-full terminator bound.'
Assert-True ($renderer.Contains('sizeof(NDSRendererHardwareTextureKey) == 236u') -and $renderer.Contains('entry->key_hash == key_hash') -and $renderer.Contains('ndsRendererHardwareTextureKeyEqual(&entry->key, key)')) 'Performance texture lookup no longer pairs its compact fingerprint with the full exact key oracle.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL < 2.*?entry = .*?sNdsRendererHardwareActiveTextureEntry.*?sNdsRendererHardwareTextureLookup\[slot\].*?value - 1u.*?#else\s*\(void\)key_hash;\s*for \(i = 0u; i < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT; i\+\+\)') 'Texture lookup no longer keeps the open-address performance path independent from the forensic linear oracle.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareTextureLookupRemove.*?leaving tombstones.*?while \(sNdsRendererHardwareTextureLookup\[slot\] !=.*?NDS_RENDERER_HW_TEXTURE_LOOKUP_EMPTY\).*?ndsRendererHardwareTextureLookupInsert') 'Texture lookup deletion no longer repairs its collision cluster to prevent long-match probe decay.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareTextureLookupRemove\(entry\);\s*#endif\s*entry->key = key;.*?entry->ready = TRUE;\s*#if NDS_RENDERER_PROFILE_LEVEL < 2\s*ndsRendererHardwareTextureLookupInsert\(entry\);') 'Texture refresh no longer removes the old hash mapping before publishing and indexing the exact replacement key.'
Assert-True ($harnessScript.Contains('RENDER_TEXHASH=') -and $harnessScript.Contains('bounded probes') -and $harnessScript.Contains('Forensic renderer unexpectedly used the performance texture hash lookup.')) 'Canonical verifier no longer proves bounded performance hash coverage and forensic independence.'
$textureLookupTable = [byte[]]::new(128)
$textureLookupEntries = @(
    [pscustomobject]@{ Hash = [uint32]5; Key = 'A'; Ready = $true },
    [pscustomobject]@{ Hash = [uint32]133; Key = 'B'; Ready = $true },
    [pscustomobject]@{ Hash = [uint32]5; Key = 'C'; Ready = $true },
    [pscustomobject]@{ Hash = [uint32]261; Key = 'D'; Ready = $true }
)
Add-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 0
Add-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 1
Add-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 2
Assert-Equal (Find-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 5 'C') 2 'Texture lookup did not pass an equal fingerprint through full-key comparison.'
Assert-Equal (Find-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 5 'Z') -1 'Texture lookup accepted a fingerprint collision without exact key equality.'
Remove-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 1
Assert-Equal (Find-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 5 'C') 2 'Texture lookup deletion did not retain a later collision-chain entry.'
Assert-Equal $textureLookupTable[6] 3 'Texture lookup deletion did not repair the collision-chain hole.'
Assert-Equal $textureLookupTable[7] 0 'Texture lookup deletion left the repaired collision chain fragmented.'
Add-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 3
Assert-Equal $textureLookupTable[7] 4 'Texture lookup did not append a new collision after cluster repair.'
Remove-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 0
Assert-Equal (Find-TextureLookupFixtureEntry $textureLookupTable $textureLookupEntries 261 'D') 3 'Texture lookup lost a replacement after deleting the collision-chain head.'
Assert-True ($renderer -match '(?s)#endif\s*for \(y = 0u; y < height; y\+\+\).*?index0 = ndsRendererHardwareReadCi4Direct.*?index1 = ndsRendererHardwareReadCi4Direct') 'Performance CI4 cache no longer retains the independent bytewise fallback.'
Assert-True ($rendererHeader.Contains('ndsRendererHardwareResetSourceCaches')) 'Renderer API cannot invalidate source-pointer caches before reloc scene storage is reused.'
Assert-True ($relocRendererDL -match '(?s)static void ndsRendererAdapterResetSceneCaches.*?ndsRendererHardwareResetSourceCaches\(\)') 'Reloc scene reset no longer invalidates immutable renderer source caches before pointer reuse.'
Assert-True ($harnessScript.Contains('Forensic renderer unexpectedly bypassed its independent bytewise CI4 source decoder.')) 'Mode 163 no longer pins the forensic bytewise CI4 decoder independently of the performance cache.'
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
Assert-True ($renderer.Contains('state->texture_prepare_origin_s = render_tile->uls;') -and $renderer.Contains('state->texture_prepare_origin_t = render_tile->ult;')) 'Renderer hardware texcoord submission is not retaining the active render-tile origin.'
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
Assert-True ($renderer.Contains('ndsRendererHardwareBuildTexel01Ci4Lut')) 'Renderer does not precompute the exact 16x16 CI4 palette-pair blend table.'
Assert-True ($renderer.Contains('sizeof(sNdsRendererHardwareTexel01Ci4PhaseLut) == 8192u')) 'Renderer phase-resolved CI4 lookup lost its measured 8 KiB bound.'
Assert-True ($renderer.Contains('(phase * NDS_RENDERER_HW_TEXEL01_CI4_LUT_COUNT) +') -and $renderer.Contains('lut_index] =')) 'Renderer CI4 build does not populate every palette-pair entry in every ordered-coverage phase plane.'
Assert-True ($renderer.Contains('alpha_phase_mask = alpha_phase_prefix[alpha_prefix_count]') -and $renderer.Contains('(alpha_phase_mask >> phase) & 1u')) 'Renderer CI4 lookup does not pre-resolve exact ordered-coverage phase masks.'
Assert-True ($renderer.Contains('sNdsRendererHardwareTexel01Ci4Source1S[x]') -and $renderer.Contains('ndsRendererHardwareTextureAddressCoord(')) 'Renderer does not precompute animated TEXEL1 addressing through the exact generic address function.'
Assert-True ($renderer.Contains('phase_lut_index = phase_row | ((x & 3u) << 8) |') -and $renderer.Contains('(index0 << 4) | index1;')) 'Renderer direct CI4 loop is not keyed by both source indices and the exact 4x4 phase.'
Assert-True (-not $renderer.Contains('ndsRendererHardwareResolveCi4PackedPair')) 'Renderer restored the mostly failing adjacent-pair branch instead of the measured single-pixel CI4 loop.'
Assert-True ($renderer.Contains('ndsRendererHardwareConvertTexel01Ci4Direct')) 'Renderer does not directly resolve the two CI4 source nibbles.'
Assert-True ($renderer.Contains('ndsRendererProfileRecordTextureCi4Direct(width * height)')) 'Renderer does not publish direct CI4 conversion coverage once per texture.'
Assert-True ($taskman.Contains('gNdsRendererProfileTextureCi4DirectPixels = 0;')) 'Renderer direct CI4 conversion coverage is not reset with the frame profile.'
Assert-True ($taskman.Contains('gNdsRendererProfileCi4RepresentativePixelCount = 0;') -and $taskman.Contains('gNdsRendererProfileCi4ReusePixelCount = 0;')) 'Renderer CI4 representative coverage is not reset with the frame profile.'
Assert-True ($harnessScript.Contains('gNdsRendererProfileTextureCi4DirectPixels')) 'Canonical renderer verifier does not require the direct CI4 water path.'
Assert-True ($harnessScript.Contains('RENDER_ADAPTER_CACHE=') -and $harnessScript.Contains('gNdsRendererProfileDObjWorldCacheOverflowCount')) 'Forensic renderer verifier does not prove bounded frame-local matrix-cache reuse.'
Assert-True ($renderer.Contains('texel1_palette_entries =') -and $renderer.Contains('texel1_source.palette_base + 16u')) 'Renderer TLUT pointer validation does not cover the TEXEL1 CI4 palette bank.'
Assert-True ($renderer.Contains('texel1_palette_entries > palette_entries')) 'Renderer does not resolve the maximum TEXEL0/TEXEL1 CI4 palette span.'
Assert-True ($renderer.Contains('use_texel1_ci4_lut != FALSE')) 'Renderer upload loop does not select the CI4 TEXEL0/TEXEL1 lookup path.'
Assert-True ($renderer.Contains('upload_width * upload_height * sizeof(u16)')) 'Renderer texture conversion still clears the worst-case scratch arena for smaller uploads.'
Assert-True ($renderer.Contains('color, color1, stats->prim_lod_fraction')) 'Renderer retained generic TEXEL0/TEXEL1 fallback does not consume both source texels and the original fraction.'
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
Assert-True ($renderer.Contains('return RGB15(31u, 31u, 31u)')) 'Renderer hardware path does not tint non-shade combine output white.'
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
Assert-True ($renderer.Contains('NDS_RENDERER_MWO_POINT_ST')) 'Renderer G_MWO_POINT_ST target is missing.'
Assert-True ($renderer.Contains('ndsRendererApplyModifyVertexCommand')) 'Renderer G_MODIFYVTX state handler is missing.'
Assert-True ($renderer.Contains('state->input_vertices[index].s = (s16)(w1 >> 16)')) 'Renderer G_MODIFYVTX does not replace cached vertex S.'
Assert-True ($renderer.Contains('state->input_vertices[index].t = (s16)(w1 & 0xffffu)')) 'Renderer G_MODIFYVTX does not replace cached vertex T.'
$modifyVertexSource = [regex]::Match($renderer, '(?s)static void ndsRendererApplyModifyVertexCommand\(.*?\n}\s*\n\s*static s32 ndsRendererTransformedTriangleReady').Value
Assert-True (-not [string]::IsNullOrWhiteSpace($modifyVertexSource)) 'Renderer G_MODIFYVTX fixture could not isolate the handler.'
Assert-True (-not $modifyVertexSource.Contains('vertex_matrix_snapshot')) 'G_MODIFYVTX ST incorrectly invalidates the cached position matrix snapshot.'
Assert-True (-not $modifyVertexSource.Contains('vertex_clip_snapshot')) 'G_MODIFYVTX ST incorrectly invalidates the cached clip transform snapshot.'
Assert-True ($renderer.Contains('ndsRendererHardwareClipVertex')) 'Renderer hardware no-z projected vertex path is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START')) 'Renderer hardware background projected-depth counter is missing.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START')) 'Renderer hardware foreground projected-depth counter is missing.'
Assert-True ($renderer.Contains('ndsRendererHardwareEnterProjectedForeground')) 'Renderer source-Z projected-depth phase transition is missing.'
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
$litPrepare = $renderer.LastIndexOf('static void ndsRendererHardwarePrepareLitDirection(')
$litDiffuse = $renderer.IndexOf('static u32 ndsRendererHardwareLitDiffuseNumer(', $litPrepare)
$litSqrt = $renderer.IndexOf('sqrtf(', $litPrepare)
$litCacheGuard = $renderer.IndexOf('if (state->prepared_light_direction_valid == 0u)')
$litPreparedCall = $renderer.IndexOf('prepared_light_direction = &state->prepared_light_direction;')
$litVertexLoop = $renderer.IndexOf('for (i = 0u; i < count; i++)', $litPreparedCall)
Assert-True ($litPrepare -ge 0 -and $litDiffuse -gt $litPrepare -and $litSqrt -gt $litPrepare -and $litSqrt -lt $litDiffuse) 'Renderer light normalization is not isolated from the per-vertex diffuse dot product.'
Assert-True ($litCacheGuard -ge 0 -and $litPreparedCall -gt $litCacheGuard -and $litVertexLoop -gt $litPreparedCall) 'Renderer does not reuse one exact prepared light direction before each source G_VTX loop.'
Assert-Equal ([regex]::Matches($renderer, 'sqrtf\(').Count) 1 'Renderer restored more than one light-normalization square root site.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_HW_TRIANGLES.*?#define NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION\(state\).*?prepared_light_direction_valid = FALSE.*?#else.*?#define NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION\(state\) \(\(void\)\(state\)\)') 'Renderer light-direction invalidation is not hardware-only with a normal-build no-op.'
Assert-True ([regex]::Matches($renderer, 'NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION\(state\);').Count -ge 3) 'Renderer light-direction cache is not invalidated by composed/matrix-word/light-state mutations.'
Assert-True ($renderer.Contains('state->prepared_light_direction_valid = TRUE;')) 'Renderer never validates its exact prepared light direction.'
Assert-True ($renderer -match 'static void NDS_RENDERER_HOT_CODE\s+ndsRendererApplyVertexCommand') 'Renderer source G_VTX handler is not retained in the measured ARM/O3 ITCM path.'
Assert-True ($renderer -match 'static u32 NDS_RENDERER_HOT_CODE\s+ndsRendererHardwareLitShadeColorPrepared') 'Renderer generic lit-shade fallback is not retained with the measured VTX hot path.'
Assert-True ($renderer.Contains('typedef u32 NDSRendererAliasedU32 __attribute__((__may_alias__))')) 'Renderer aligned VTX decode lost its strict-alias-safe source word type.'
Assert-True ($renderer.Contains('if ((((uintptr_t)src) & 3u) == 0u)') -and $renderer.Contains('xy = words[0];') -and $renderer.Contains('rgba = words[3];')) 'Renderer does not use the measured aligned four-word VTX decode.'
Assert-True ($renderer.Contains('xy = ndsRendererReadU32(src);') -and $renderer.Contains('rgba = ndsRendererReadU32((const u8 *)src + 12);')) 'Renderer aligned VTX decode lost the exact bytewise unaligned fallback.'
Assert-True ($renderer.Contains('NDSRendererInputVertex *input = &state->input_vertices[index];')) 'Renderer hardware VTX path does not decode directly into the persistent 32-slot source cache.'
Assert-True (-not $renderer.Contains('state->input_vertices[index] = input;')) 'Renderer restored the redundant temporary-to-cache VTX copy.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT 4u') -and $renderer.Contains('NDS_RENDERER_HW_LIGHT_SHADE_LUT_COUNT 128u')) 'Renderer exact light-shade cache lost its four-entry/128-step bound.'
Assert-True ($renderer.Contains('sizeof(sNdsRendererHardwareLightShadeCache) == 2096u')) 'Renderer exact light-shade cache exceeds its measured 2,096-byte bound.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL < 2\s*static NDSRendererHardwareLightShadeCacheEntry\s*sNdsRendererHardwareLightShadeCache.*?sizeof\(sNdsRendererHardwareLightShadeCache\) == 2096u.*?#endif') 'Forensic renderer restored the production light-shade cache instead of its independent exact shade path.'
Assert-True ($renderer -match '(?s)entry->diffuse == diffuse.*?entry->ambient == ambient.*?return entry->rgb;') 'Renderer light-shade cache is not content-keyed by both source light colors.'
Assert-True ($renderer.Contains('(ndsRendererHardwareColorByte(diffuse, 24) * i) / 127u') -and $renderer.Contains('return rgb_lut[diffuse_numer] | (u32)vtx->a;')) 'Renderer light-shade LUT does not preserve the exact per-channel integer result and source alpha.'
Assert-True ($renderer -match '(?s)stats->light_color_mask.*?NDS_RENDERER_LIGHT_COLOR_1_MASK.*?NDS_RENDERER_LIGHT_COLOR_2_MASK.*?prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut') 'Renderer light-shade LUT bypasses the generic fallback for incomplete source light state.'
function Get-LitShadeFixtureChannel {
    param([int]$Diffuse, [int]$Ambient, [int]$Numer)
    return [Math]::Min(255, $Ambient + [int][Math]::Floor(
        ([double]$Diffuse * $Numer) / 127.0))
}
Assert-Equal (Get-LitShadeFixtureChannel 131 36 0) 36 'Mario shoe LUT ambient endpoint drifted.'
Assert-Equal (Get-LitShadeFixtureChannel 131 36 64) 102 'Mario shoe LUT midpoint drifted.'
Assert-Equal (Get-LitShadeFixtureChannel 131 36 127) 167 'Mario shoe LUT diffuse endpoint drifted.'
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
$battlePlayableVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-playable-harness.ps1') -Raw
Assert-True ($makefile.Contains('NDS_RENDERER_HW_TRIANGLES ?= 0')) 'Makefile hardware renderer flag default is missing.'
Assert-True ($makefile.Contains('NDS_RENDERER_PROFILE_LEVEL ?= 2')) 'Makefile renderer profile default is not forensic-safe.'
Assert-True ($makefile.Contains('NDS_RENDERER_BENCHMARK_MODE ?= 0')) 'Makefile renderer benchmark modes no longer default off.'
Assert-True ($makefile -match '(?s)ifeq \(\$\(TARGET\),smash64ds-battle-playable-coarse-triangle-noop-hwtri\).*?override NDS_RENDERER_PROFILE_LEVEL := 1.*?override NDS_RENDERER_BENCHMARK_MODE := 1') 'TRIANGLE_NOOP cost-floor target is not a dedicated profile-1 build.'
Assert-True ($makefile -match '(?s)ifeq \(\$\(TARGET\),smash64ds-battle-playable-coarse-cpu-prep-no-gx-hwtri\).*?override NDS_RENDERER_PROFILE_LEVEL := 1.*?override NDS_RENDERER_BENCHMARK_MODE := 2') 'CPU_PREP_NO_GX cost-floor target is not a dedicated profile-1 build.'
Assert-True ($makefile -match '(?s)ifeq \(\$\(TARGET\),smash64ds-battle-playable-coarse-warm-no-upload-hwtri\).*?override NDS_RENDERER_PROFILE_LEVEL := 1.*?override NDS_RENDERER_BENCHMARK_MODE := 4') 'WARM_NO_UPLOAD cost-floor target is not a dedicated profile-1 build.'
Assert-True ($makefile -match '(?s)ifeq \(\$\(TARGET\),smash64ds-battle-playable-canonical-hwtri\).*?override NDS_RENDERER_PROFILE_LEVEL := 0') 'Canonical/shipped renderer is not forced to performance profile 0.'
Assert-True ($makefile -match '(?s)ifeq \(\$\(TARGET\),smash64ds-battle-playable-coarse-hwtri\).*?override NDS_RENDERER_PROFILE_LEVEL := 1') 'Internal coarse renderer target is not forced to profile 1.'
Assert-True ($makefile -match '(?s)ifeq \(\$\(TARGET\),smash64ds-battle-playable-forensic-hwtri\).*?override NDS_RENDERER_PROFILE_LEVEL := 2') 'Internal forensic renderer target is not forced to profile 2.'
Assert-True ([regex]::Matches($makefile, 'NDS_DEV_SCENE_HARNESS_ID := 163\r?\n(?:#[^\r\n]*\r?\n)*CFLAGS \+= -O2').Count -eq 1) 'Canonical mode 163 no longer retains the measured O2 latency policy.'
Assert-True ([regex]::Matches($makefile, 'NDS_DEV_SCENE_HARNESS_ID := 163\r?\n(?:#[^\r\n]*\r?\n)*CFLAGS \+= -Os').Count -eq 2) 'Mode 163 diagnostic variants no longer preserve their scene-reserve size policy.'
Assert-True ($makefile -match '(?s)ifeq \(\$\(NDS_DEV_SCENE_HARNESS_ID\),163\).*?nds_renderer\.o: CFLAGS \+= -marm.*?endif') 'Mode 163 no longer compiles the renderer TU in measured ARM state.'
Assert-True ($makefile.Contains("echo '#define NDS_RENDERER_PROFILE_LEVEL `$(NDS_RENDERER_PROFILE_LEVEL)';")) 'Generated build config omits the renderer profile level.'
Assert-True ($makefile.Contains('echo ''#define NDS_BUILD_HARNESS_VARIANT "$(NDS_DEV_SCENE_HARNESS)"'';')) 'Generated build config cannot invalidate objects when equal-ID harness variants change compiler policy.'
Assert-True ($battlePlayableVerifier -match '(?s)elseif \(\$RealtimePresentation -and \(\$RendererProfileLevel -lt 2\)\).*?\$harness = ''battle_playable_realtime''.*?-Harness \$harness') 'Direct canonical verifier no longer routes profiles 0/1 through the O2 battle_playable_realtime build variant.'
Assert-True ($battlePlayableVerifier -match '(?s)if \(\$MatchLifecycleProof\).*?\$harness = ''battle_playable_match_lifecycle''') 'Direct lifecycle verifier no longer routes through its size-optimized mode-163 build variant.'
Assert-True ($battlePlayableVerifier.Contains('[ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0')) 'Mode-163 benchmark wrapper no longer accepts the 128-frame decision window or changed its opt-in default.'
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
Assert-True ($rendererHeader.Contains('NDS_RENDERER_PROFILE_LEVEL must be 0, 1, or 2')) 'Renderer profile-level compile-time validation is missing.'
Assert-True ($rendererHeader.Contains('NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY 64u')) 'Renderer matrix snapshot table lost its measured bounded capacity.'
Assert-True ($rendererHeader.Contains('matrix_snapshots[NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY]')) 'Persistent renderer vertex cache does not own its matrix snapshot table.'
Assert-True ($rendererHeader.Contains('vertex_matrix_snapshot[NDS_RENDERER_VERTEX_CACHE_SIZE]')) 'Persistent renderer vertex cache lacks per-slot matrix snapshot IDs.'
Assert-True ($rendererHeader.Contains('vertex_clip_snapshot[NDS_RENDERER_VERTEX_CACHE_SIZE]')) 'Persistent renderer vertex cache lacks per-slot clip transform IDs.'
Assert-True ($rendererHeader.Contains('ndsRendererInitVertexCache')) 'Renderer vertex-cache boundary initializer is missing.'
$startupHeader = Get-Content (Join-Path $root 'include/nds/nds_startup.h') -Raw
Assert-True ($startupHeader.Contains('gNdsFighterDLAllDrawHardwareTextureReadyCount')) 'All-DL hardware texture diagnostics are missing from the startup header.'
Assert-True ($startupHeader.Contains('gNdsFighterDLAllDrawP0HardwareZBufferTriangleCount')) 'All-DL hardware z-buffer diagnostics are missing from the startup header.'
Assert-True ($startupHeader.Contains('gNdsStageGCDrawAllLoopHardwareTextureReadyCount')) 'Stage gcDrawAll hardware texture startup diagnostics are missing.'
Assert-True ($startupHeader.Contains('gNdsStageGCDrawAllLoopHardwareFighterTriangleCount')) 'Stage gcDrawAll hardware fighter diagnostics are missing from the startup header.'
Assert-True ($startupHeader.Contains('gNdsRendererProfileSourceVertexLoadCount')) 'Renderer lazy source-load diagnostic is missing from the startup header.'
Assert-True ($startupHeader.Contains('gNdsRendererProfileMatrixSnapshotOverflowCount')) 'Renderer snapshot-overflow diagnostic is missing from the startup header.'
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
Assert-True ($renderer -match '(?s)source_zbuffered = zbuffered;.*?if \(source_zbuffered != FALSE\)\s*\{\s*/\* Source-Z projected submissions use the composed clip Z below and\s*\* must not consume the synthetic no-Z painter counter\. \*/\s*projected_z\[0\].*?else.*?ndsRendererHardwareNextProjectedDepth\(\);') 'Renderer source-Z submissions consume the synthetic no-Z painter counter.'
Assert-True ($renderer.Contains('ndsRendererHardwareBeginTriangleBatch')) 'Renderer adjacent source-triangle batch helper is missing.'
Assert-True ($renderer.Contains('gNdsRendererProfileHardwareBatchReuseCount++')) 'Renderer adjacent source-triangle reuse diagnostic is missing.'
Assert-True ($renderer -match '(?s)if \(state->texture_prepare_valid == 0u\).*?ndsRendererHardwareBindTexture\(stats, config\).*?ndsRendererProfileRecordTexturePrepare\(\);.*?else.*?ndsRendererProfileRecordTexturePrepareReuse\(\);') 'Renderer does not reuse the dynamic texture/material/depth preparation within one exact state epoch.'
Assert-True ($renderer -match '(?s)u32 texture_prepare_alpha_constant;\s*u32 texture_prepare_poly_alpha;\s*u32 texture_prepare_poly_fmt;') 'Traversal state no longer retains exact constant-alpha/poly-format preparation for generic and forensic fast-run comparison.'
Assert-True ($renderer -match '(?s)static s32 ndsRendererHardwareAlphaUsesVertex.*?ndsRendererHardwareBlendAlphaUsesMemory\(stats\).*?NDS_RENDERER_ACMUX_PRIMITIVE.*?NDS_RENDERER_ACMUX_ENVIRONMENT.*?NDS_RENDERER_ACMUX_TEXEL0.*?NDS_RENDERER_ACMUX_SHADE') 'Renderer constant-alpha classifier no longer mirrors the exact blend/combine sources before permitting epoch reuse.'
Assert-True ($renderer -match '(?s)if \(state->texture_prepare_valid == 0u\).*?texture_prepare_alpha_constant =.*?ndsRendererHardwareAlphaUsesVertex\(stats\) == FALSE.*?texture_prepare_poly_fmt =\s*ndsRendererHardwarePolyFmt\(stats, poly_alpha\);.*?else if \(state->texture_prepare_alpha_constant != 0u\).*?poly_alpha = state->texture_prepare_poly_alpha;') 'Renderer does not limit prepared alpha/poly-format reuse to proven state-constant epochs in generic and forensic comparison paths.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_HW_TRIANGLES\s*#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE\(state\).*?texture_prepare_valid = FALSE.*?#else\s*#define NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE\(state\) \(\(void\)\(state\)\)') 'Renderer selective texture invalidation is not hardware-only with a normal-build no-op.'
Assert-True (-not ($renderer -match '(?s)if \(\(op != NDS_RENDERER_OP_TRI1\).*?ndsRendererHardwareEndBatch\(\);.*?NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE\(state\);.*?state->prepared_projected_source_z_valid_mask = 0u;')) 'Renderer still discards exact texture preparation at every VTX/matrix/non-TRI boundary.'
foreach ($texturePrepareOpcode in @('TEXTURE', 'GEOMETRYMODE', 'SETCOMBINE', 'SETTIMG', 'SETTILE', 'LOADTILE', 'LOADBLOCK', 'LOADTLUT', 'SETTILESIZE')) {
    Assert-True ($renderer -match ("(?s)case NDS_RENDERER_OP_{0}:\s*NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE\(state\);" -f $texturePrepareOpcode)) "Renderer does not invalidate exact texture preparation for $texturePrepareOpcode."
}
Assert-True ($renderer -match '(?s)case NDS_RENDERER_OP_SETENVCOLOR:.*?case NDS_RENDERER_OP_SETPRIMCOLOR:.*?op != NDS_RENDERER_OP_SETBLENDCOLOR.*?NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE\(state\);') 'Renderer does not invalidate material preparation for primitive/environment color changes.'
Assert-True ($renderer -match '(?s)case NDS_RENDERER_OP_SETOTHERMODE_H:.*?case NDS_RENDERER_OP_RDPSETOTHERMODE:.*?NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE\(state\);') 'Renderer does not invalidate depth/filter preparation for other-mode changes.'
Assert-True ($renderer.Contains('matrix_generation = ndsRendererNextMatrixGeneration()')) 'Renderer composed matrix generations are not refreshed on matrix changes.'
Assert-True ($renderer -match '(?s)sNdsRendererMatrixGenerationSerial == 0u.*?sNdsRendererHardwareMatrixLoaded = FALSE.*?sNdsRendererHardwareMatrixGeneration = 0u') 'Renderer matrix-generation wrap does not invalidate the GX matrix cache.'
Assert-True (-not $renderer.Contains('sNdsRendererHardwareMatrixProjection')) 'Renderer still compares full projection matrices in the triangle hot path.'
Assert-True (-not $renderer.Contains('sNdsRendererHardwareMatrixModelview')) 'Renderer still compares full modelview matrices in the triangle hot path.'
Assert-True ($renderer -match '(?s)ndsRendererLoadHardwareRawComposedMatrix\(.*?sNdsRendererHardwareMatrixMode ==\s*NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED.*?sNdsRendererHardwareMatrixGeneration == generation.*?return;.*?ndsRendererMtxIdentity20p12') 'Renderer raw matrix cache no longer returns before temporary matrix construction.'
Assert-True ($renderer -match '(?s)ndsRendererLoadHardwareMatrices\(.*?NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY.*?sNdsRendererHardwareMatrixGeneration == 0u.*?return;.*?ndsRendererMtxIdentity20p12') 'Renderer projected-identity cache no longer returns before temporary matrix construction.'
Assert-True ($renderer -match '(?s)ndsRendererBuildRawHardwareMatrix\(.*?\*hardware = \*composed;.*?for \(col = 0u; col < 4u; col\+\+\).*?NDS_RENDERER_HW_WORLD_UNIT_SHIFT') 'Renderer corrected raw matrix does not scale the complete homogeneous row.'
Assert-True ($renderer -match '(?s)ndsRendererApplyVertexCommand.*?ndsRendererHardwareRawVertexFits\(input\).*?raw_vertex_fit_mask \|= mask;.*?raw_vertex_fit_mask &= ~mask;') 'Renderer does not cache exact raw-coordinate eligibility when an RSP vertex slot is loaded.'
Assert-True ($renderer -match '(?s)raw_vertex_fit_mask & mask\) != mask.*?current_transform_vertex_mask & mask.*?ndsRendererHardwareRawMatrixCompatible.*?ndsRendererProfileRecordRawCurrentCandidate') 'Renderer raw-current classification does not require cached unclamped coordinates, current slots, and a compatible current matrix.'
Assert-True ($renderer -match '(?s)state\.raw_vertex_fit_mask = vertex_cache->raw_vertex_fit_mask.*?vertex_cache->raw_vertex_fit_mask = state\.raw_vertex_fit_mask') 'Renderer persistent vertex cache does not carry raw-coordinate eligibility across ordered source lists.'
Assert-True ($renderer.Contains('state.vertices = vertex_cache->transformed_vertices;') -and $renderer.Contains('state.input_vertices = vertex_cache->input_vertices;') -and $renderer.Contains('state.vertex_colors = vertex_cache->vertex_colors;') -and $renderer.Contains('state.vertex_matrix_snapshot = vertex_cache->vertex_matrix_snapshot;') -and $renderer.Contains('state.vertex_clip_snapshot = vertex_cache->vertex_clip_snapshot;')) 'Renderer traversal no longer operates directly on the persistent BattleShip RSP vertex planes.'
Assert-True (-not ($renderer -match 'memcpy\(state\.(vertices|input_vertices|vertex_colors|vertex_matrix_snapshot|vertex_clip_snapshot),\s*vertex_cache->') -and -not ($renderer -match 'memcpy\(vertex_cache->(transformed_vertices|input_vertices|vertex_colors|vertex_matrix_snapshot|vertex_clip_snapshot),\s*state\.')) 'Renderer restored full per-list copies around the persistent 32-slot RSP cache.'
Assert-True ($renderer.Contains('Valid masks and stack depth own every scratch read.') -and -not $renderer.Contains('memset(state, 0, sizeof(*state));')) 'Renderer traversal restored broad per-list scratch clearing instead of initializing the exact validity control plane.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX')) 'Renderer hybrid submission-class enum is missing raw-current source-Z.'
Assert-True ($renderer.Contains('NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX')) 'Renderer hybrid submission-class enum is missing retained-snapshot source-Z.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareClassifySubmit.*?NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z.*?NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL.*?NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH.*?raw_vertex_fit_mask.*?NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX.*?vertex_matrix_snapshot.*?NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX.*?NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX') 'Renderer hybrid submit classifier no longer preserves exceptional classes before cached raw current/snapshot eligibility and mixed-snapshot projection.'
Assert-True ($renderer -match '(?s)if \(submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX\).*?ndsRendererLoadHardwareMatrices\(state, scale_world\);.*?else if \(raw_snapshot != NULL\).*?ndsRendererLoadHardwareRawComposedMatrix.*?else.*?ndsRendererLoadHardwareMatrices\(NULL, FALSE\);') 'Renderer raw-current, raw-snapshot, and projected classes do not load their explicit GX matrices.'
Assert-True ($renderer -match '(?s)sNdsRendererHardwareTriangleBatchMatrixMode == matrix_mode.*?sNdsRendererHardwareTriangleBatchMatrixGeneration ==\s*matrix_generation') 'Renderer triangle batch key omits matrix representation or generation.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareBeginTriangleBatch.*?ndsRendererProfileRecordBatchReuse\(\);\s*return;\s*\}\s*alpha_key = ndsRendererHardwareAlphaStateKey') 'Renderer adjacent TRI reuse no longer bypasses invariant alpha/fog key construction.'
Assert-True ($renderer -match '(?s)ndsRendererProfileRecordSubmitClass\(submit_class\).*?ndsRendererProfileRecordHardwareTriangle') 'Renderer does not account hybrid submit classes before final hardware triangle accounting.'
Assert-True (-not ($renderer.Contains('ndsRendererProfileRecordProjectedDivisions'))) 'Renderer restored redundant hot-loop logical division accounting instead of deriving it from submitted classes.'
Assert-True ($renderer -match '(?s)static inline v16 ndsRendererHardwareProjectToV16.*?low_product = \(s64\)-32768 \* \(s64\)denominator;.*?high_product = \(s64\)32767 \* \(s64\)denominator;.*?#if defined\(__arm__\)\s*result = \(v16\)div64\(numerator, denominator\);') 'Renderer projected helper no longer bounds the exact signed quotient before using the DS 64/32 divider.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*static v16 __attribute__\(\(noinline, optimize\("Os"\)\)\)\s*ndsRendererHardwareProjectToV16.*?#else\s*static inline v16 ndsRendererHardwareProjectToV16') 'Forensic projected-divider oracle no longer keeps one size-optimized helper copy outside the hot submit loop.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*if \(result != ndsRendererHardwareClampS64ToV16\(\s*numerator / denominator\)\).*?sNdsRendererHardwareDivideSummary \|=\s*NDS_RENDERER_HW_DIVISION_MISMATCH;') 'Forensic renderer no longer compares every hardware quotient with the former exact C division.'
Assert-True (-not ($renderer -match 'static u32 sNdsRendererHardwareDivide(?:Call|Preclamp|Zero|Mismatch)')) 'Hardware-divider evidence restored separate forensic BSS counters instead of one packed summary.'
Assert-True (-not ($renderer -match 'NDS_RENDERER_HW_PROJECTED_VERTEX\) / (?:vtx|clip_vtx)->w')) 'Renderer restored direct software 64-bit division in projected vertex submission.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareConsumeSubmittedFrame.*?ndsRendererProfilePublishSubmitSummary\(\).*?ndsRendererProfileResetSubmitSummary\(\)') 'Renderer does not publish and reset hybrid submit accounting at the shared hardware-frame boundary.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2.*?ndsRendererHardwareQueueRawMatrixPosTest.*?#endif') 'Renderer raw GX PosTest capture is not forensic-only.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareQueueMatrixWordPosTestFixture.*?ndsRendererApplyMvpRecalcCommand.*?ndsRendererApplyMatrixMoveWordCommand') 'Renderer device PosTest coverage omits the MVP-recalc matrix-word reconstruction case.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareEndBatch\(\);\s*#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*ndsRendererHardwareRunRawMatrixPosTests\(\);') 'Renderer raw GX PosTests do not run after the final source triangle batch closes.'
Assert-True ($renderer.Contains('NDSRendererRuntimeFrameSummary')) 'Performance renderer compact frame summary is missing.'
Assert-True ($renderer.Contains('ndsRendererProfileFramePublish')) 'Performance renderer frame-summary publication is missing.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*\(void\)ndsRendererTransformCachedVertex.*?#else.*?matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID.*?ndsRendererTransformCachedVertex') 'Renderer VTX path does not keep forensic transforms eager while deferring bounded production snapshots.'
Assert-True ($renderer -match '(?s)if \(raw_submit == FALSE\).*?ndsRendererEnsureTransformedVertex\(stats, state, i0\).*?ndsRendererEnsureTransformedVertex\(stats, state, i1\).*?ndsRendererEnsureTransformedVertex\(stats, state, i2\)') 'Renderer projected exceptions do not lazily materialize all three clip vertices.'
Assert-True ($renderer -match '(?s)vertex_clip_snapshot\[index\] == snapshot_id.*?ndsRendererProfileRecordTransformCacheHit') 'Renderer lazy clip-transform cache does not require the vertex matrix snapshot ID.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*if \(sNdsRendererHardwareNoOracle == 0u\).*?ndsRendererHardwareRecordOracleTriangle') 'Renderer oracle is not compile-time excluded from profile 0/1 or does not honor no-oracle.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*ndsRendererProfileTextureCoord\(s, t\);\s*ndsRendererProfileTextureSample\(s, t\);.*?#endif\s*glTexCoord2t16\(s, t\);') 'Per-vertex texture range/sample diagnostics are not forensic-only.'
Assert-True ($renderer -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL >= 2\s*s32 depth =.*?hardware_projected_depth_sample_count\+\+;\s*#endif\s*projected_z = clip_vtx->z;') 'Projected-depth ranges are not forensic-only or alter runtime depth.'
$rendererMovement = Get-Content (Join-Path $root 'src/port/reloc_backend_movement.c') -Raw
Assert-True ($rendererMovement -match '(?s)#if NDS_RENDERER_PROFILE_LEVEL < 2\s*ndsRendererHardwareSetNoOracle\(TRUE\);.*?#if NDS_RENDERER_PROFILE_LEVEL < 2\s*ndsRendererHardwareSetNoOracle\(FALSE\);') 'Performance/coarse frame does not suppress the renderer command oracle.'
$rendererVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1') -Raw
Assert-True ($rendererVerifier.Contains('RENDER_PROFILE_LEVEL=%u')) 'Realtime verifier does not identify the compiled renderer profile.'
Assert-True ($rendererVerifier.Contains('RENDER_BENCH=%u')) 'Realtime verifier lacks the optional warm-frame renderer sampler.'
Assert-True ($rendererVerifier.Contains('COARSE_BENCH=%u') -and $rendererVerifier.Contains('OWNER_BENCH=%u') -and $rendererVerifier.Contains('GX_BOUNDARY=%u') -and $rendererVerifier.Contains('STAGE0_BENCH=%u')) 'Realtime verifier does not sample synchronized coarse phases, owner census records, GX flush boundaries, and stage layer-0 wall time.'
Assert-True ($rendererVerifier.Contains('RENDER_SEMANTIC={0}') -and $rendererVerifier.Contains('$format = ((1..38') -and $rendererVerifier.Contains('$coarseBenchmarkCommands += Get-RendererSemanticBenchmarkCommand')) 'Forensic benchmark lacks its exact 38-field synchronized semantic trace marker.'
Assert-True ($rendererVerifier -match '(?s)if \(\$RendererProfileLevel -ge 1\).*?COARSE_BENCH=%u.*?STAGE0_BENCH=%u.*?Get-RendererOwnerBenchmarkCommand') 'Realtime verifier references profile-1-only coarse/owner/stage-layer globals outside the guarded GDB command path.'
Assert-True ($rendererVerifier.Contains("-Name 'COARSE_BENCH' -FieldCount 24") -and $rendererVerifier.Contains("-Name 'OWNER_BENCH' -FieldCount 37") -and $rendererVerifier.Contains("-Name 'GX_BOUNDARY' -FieldCount 7") -and $rendererVerifier.Contains("-Name 'STAGE0_BENCH' -FieldCount 2")) 'Realtime verifier does not parse every coarse, owner, GX-boundary, and stage-layer benchmark field.'
Assert-True ($rendererVerifier -match '(?s)if \(\$RendererProfileLevel -ge 2\) \{.*?Get-RendererOwnerBenchmarkCommand.*?\$coarseBenchmarkCommands \+= Get-RendererSemanticBenchmarkCommand\s*\}' -and $rendererVerifier.Contains("-Name 'RENDER_SEMANTIC' -FieldCount 38")) 'Forensic semantic marker command/parser is not strictly profile-2-only or does not consume all 38 fields.'
Assert-True ($rendererVerifier.Contains('gNdsRendererSemanticOutputHash') -and $rendererVerifier.Contains('gNdsRendererSemanticOutputHash2') -and $rendererVerifier.Contains('gNdsRendererSemanticEventCount') -and $rendererVerifier.Contains('gNdsRendererSemanticOverflowCount')) 'Forensic semantic marker omits frame hash/count/overflow publication.'
Assert-True ($rendererVerifier.Contains('$owner.semantic_output_hash2') -and $rendererVerifier.Contains('$owner.semantic_event_count') -and $rendererVerifier.Contains('$owner.semantic_overflow_count') -and $rendererVerifier.Contains('$owner.semantic_occurrence_count') -and $rendererVerifier.Contains('$owner.semantic_first_owner_occurrence') -and $rendererVerifier.Contains('$owner.semantic_first_list_ordinal') -and $rendererVerifier.Contains('$owner.semantic_first_branch_path') -and $rendererVerifier.Contains('$owner.semantic_first_command_index') -and $rendererVerifier.Contains('$owner.semantic_first_tri2_half') -and $rendererVerifier.Contains('$owner.semantic_first_outcome')) 'Forensic semantic marker omits appended per-owner hash/count/occurrence/provenance fields.'
Assert-True ($rendererVerifier.Contains('[ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0')) 'Realtime renderer benchmark no longer accepts a 128-frame decision window or changed its opt-in default.'
Assert-True ($rendererVerifier.Contains('$sampleFrame -eq ($previousFrame + 1)')) 'Realtime renderer benchmark does not reject unsynchronized warm-frame windows.'
Assert-True ($rendererVerifier.Contains('$coarse[23] -eq ($previousCoarse[23] + 1)') -and $rendererVerifier.Contains('$ownerBenchmark.Count -eq (3 * $RendererBenchmarkSamples)')) 'Realtime coarse/owner benchmark does not require contiguous logic frames and an exact three-owner census.'
Assert-True ($rendererVerifier.Contains('$coarse[22] -eq $expectedConservationError') -and $rendererVerifier.Contains('($coarse[22] * 100) -le ($loopWall * 2)')) 'Realtime coarse benchmark does not enforce exact residual equations and the two-percent conservation ceiling.'
Assert-True ($rendererVerifier.Contains('($coarse[20] * 100) -le ($presentActive * 2)') -and $rendererVerifier.Contains('Renderer coarse residual ratios (median/p95 basis points)')) 'Realtime coarse benchmark does not cap present residual at two percent or publish draw/present/loop residual ratios.'
Assert-True ($rendererVerifier.Contains('$gxBoundaryBenchmark.Count -eq $RendererBenchmarkSamples') -and $rendererVerifier.Contains('$gxBoundary[0] -eq $frame') -and $rendererVerifier.Contains('$gxBoundary[4] -ne 0') -and $rendererVerifier.Contains('$gxBoundary[5] -ne 0') -and $rendererVerifier.Contains('$gxBoundary[6] -eq $coarse[16]')) 'Realtime GX boundary benchmark does not require exact count, frame alignment, post-VBlank completion, and synchronized flush ticks.'
Assert-True ($rendererVerifier.Contains('$stage0Benchmark.Count -eq $RendererBenchmarkSamples') -and $rendererVerifier.Contains('$stage0[0] -eq $frame') -and $rendererVerifier.Contains('$stage0[1] -gt 0') -and $rendererVerifier.Contains('Renderer stage layer-0 benchmark:')) 'Realtime stage layer-0 benchmark does not require exact count/frame alignment or publish median/P95 churn.'
Assert-True ($rendererVerifier.Contains('$rendererSemanticBenchmark.Count -eq $RendererBenchmarkSamples') -and $rendererVerifier.Contains('$semantic[0] -eq $frame')) 'Forensic semantic benchmark does not require exact synchronized frame samples.'
Assert-True ($rendererVerifier.Contains('$semantic[4] -eq 0') -and $rendererVerifier.Contains('$semantic[$semanticBase + 3] -eq 0') -and $rendererVerifier.Contains('$ownerOverflowCount -eq $semantic[4]')) 'Forensic semantic benchmark does not require zero and exactly conserved frame/owner overflow.'
Assert-True ($rendererVerifier.Contains('$ownerEventCount -eq $semantic[3]') -and $rendererVerifier.Contains('$semantic[$semanticBase] -ne 0') -and $rendererVerifier.Contains('$semantic[$semanticBase + 1] -ne 0') -and $rendererVerifier.Contains('$semantic[$semanticBase + 4] -gt 0')) 'Forensic semantic benchmark does not require exact event conservation, nonzero dual hashes, and positive occurrences.'
Assert-True ($rendererVerifier.Contains('$semantic[$semanticBase + 5] -lt $semantic[$semanticBase + 4]') -and $rendererVerifier.Contains('$semantic[$semanticBase + 6] -lt $ownerProfile[3]') -and $rendererVerifier.Contains('$semantic[$semanticBase + 8] -lt $ownerProfile[4]') -and $rendererVerifier.Contains('$semantic[$semanticBase + 9] -le 1') -and $rendererVerifier.Contains('$semantic[$semanticBase + 10] -le 3')) 'Forensic semantic benchmark does not bound first provenance, TRI2 half, and outcome.'
Assert-True ($rendererVerifier.Contains('Renderer semantic benchmark:') -and $rendererVerifier.Contains('Renderer semantic hash churn') -and $rendererVerifier.Contains('Renderer semantic first provenance')) 'Forensic semantic benchmark does not publish count, dual-hash churn, and first-provenance summaries.'
Assert-True ($rendererVerifier.Contains('median/p95 ticks')) 'Realtime renderer benchmark does not emit its warm-frame median/P95 distribution cleanly.'
Assert-True ($rendererVerifier.Contains('adjacent changes/distinct values')) 'Realtime renderer benchmark does not emit timing churn separately from the correctness summary.'
Assert-True ($rendererVerifier.Contains('Renderer coarse benchmark:') -and $rendererVerifier.Contains('active=$(Get-MedianP95 $coarseActive)') -and $rendererVerifier.Contains('wait=$(Get-MedianP95 $coarseWait)')) 'Realtime coarse benchmark does not publish active/wait and exclusive-phase median/P95 distributions.'
Assert-True ($rendererVerifier.Contains("`$ownerLabels = @('stage', 'Mario', 'Fox')") -and $rendererVerifier.Contains('Renderer owner census') -and $rendererVerifier.Contains('class$classIndex=')) 'Realtime benchmark does not publish stage/Mario/Fox census counts and all eight submit classes.'
Assert-True ($rendererVerifier.Contains('.entry_global_hash') -and $rendererVerifier.Contains('.exit_global_hash') -and $rendererVerifier.Contains('globalEntry=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('globalExit=$(Get-AdjacentChurn')) 'Realtime owner benchmark does not publish entry/exit global-state hash churn.'
Assert-True ($rendererVerifier.Contains('topology=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('selected=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('camera=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('DObj=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('material=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('light=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('texture=$(Get-AdjacentChurn') -and $rendererVerifier.Contains('semantic=$(Get-AdjacentChurn')) 'Realtime owner benchmark does not publish decision-window churn for all semantic signatures.'
Assert-True ($rendererVerifier.Contains('gNdsRendererProfileTexturePrepareReuseCount')) 'Realtime verifier does not report unchanged-TRI texture preparation reuse.'
Assert-True ($rendererVerifier.Contains('RENDER_RAW_MATRIX=%u')) 'Realtime verifier does not report raw-candidate and GX PosTest diagnostics.'
Assert-True ($rendererVerifier.Contains('RENDER_SUBMIT=%u')) 'Realtime verifier does not report hybrid raw/projected class accounting.'
Assert-True ($rendererVerifier.Contains('RENDER_HWDIV=%u')) 'Realtime verifier does not report actual hardware divides, pre-clamps, zero denominators, and oracle mismatches.'
Assert-True ($rendererVerifier.Contains('RENDER_LAZY=%u')) 'Realtime verifier does not report source loads, lazy transforms, snapshot reuse, and overflow.'
Assert-True ($rendererVerifier.Contains('defer a majority of CPU vertex transforms')) 'Realtime performance verifier does not require the measured lazy-transform majority.'
Assert-True ($rendererVerifier.Contains('sharply reduce and exactly account projected division demand')) 'Realtime verifier does not require the projected-division collapse.'
Assert-True ($rendererVerifier.Contains('Shipping-equivalent profile retained hot-loop hardware-divider telemetry')) 'Realtime verifier does not keep actual-divide telemetry out of profile 0/1.'
Assert-True ($rendererVerifier.Contains('Forensic renderer did not compare every live DS hardware quotient')) 'Realtime verifier does not require full exact hardware-divider oracle coverage.'
Assert-True ($rendererVerifier.Contains('corrected composed GX matrix PosTest')) 'Realtime verifier does not require the corrected GX matrix oracle.'
Assert-True ($rendererVerifier.Contains('still performed forensic oracle transforms')) 'Realtime performance verifier does not require zero oracle work.'
$forensicVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-playable-renderer-forensic.ps1') -Raw
Assert-True ($forensicVerifier.Contains('-RendererProfileLevel 2')) 'Separate profile-2 canonical oracle verifier is missing.'
Assert-True ($forensicVerifier.Contains('[ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0')) 'Forensic renderer wrapper no longer accepts the 128-frame decision window or changed its opt-in default.'
$realtimeVerifier = Get-Content (Join-Path $root 'scripts/verify-battle-playable-realtime-harness.ps1') -Raw
Assert-True ($realtimeVerifier.Contains('[ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0') -and $realtimeVerifier.Contains('$harnessArgs += @(''-RendererBenchmarkSamples'', "$RendererBenchmarkSamples")')) 'Canonical realtime wrapper does not pass the opt-in 128-frame benchmark window through to mode 163.'
Assert-True ($renderer -match '(?s)if \(sNdsRendererHardwareActiveTextureEntry != entry\).*?ndsRendererHardwareApplyTextureParams\(entry->params\);') 'Renderer cache-hit texture parameters are still programmed inside an unchanged triangle batch.'
Assert-True ($renderer -match '(?s)if \(\(op != NDS_RENDERER_OP_TRI1\) &&\s*\(op != NDS_RENDERER_OP_TRI2\)\)\s*\{\s*ndsRendererHardwareEndBatch\(\);') 'Renderer does not terminate a triangle batch before every non-triangle source command.'
Assert-True ($renderer -match '(?s)ndsRendererScanList\(dl, config, stats, &state, 0, NULL, NULL\);\s*#if NDS_RENDERER_HW_TRIANGLES\s*ndsRendererHardwareEndBatch\(\);') 'Renderer scan-only display-list entry point can leak an open triangle batch.'
Assert-True ($renderer -match '(?s)ndsRendererHardwareBeginTriangleBatch\(.*?ndsRendererHardwareSubmitVertex\(.*?ndsRendererHardwareSubmitVertex\(.*?ndsRendererHardwareSubmitVertex\(.*?if \(source_zbuffered != FALSE\)\s*\{\s*ndsRendererHardwareEnterProjectedForeground\(\);') 'Renderer batch submission or projected foreground phase no longer follows all three source vertices.'
Assert-True (-not $renderer.Contains('glEnd();')) 'Renderer restored libnds dummy glEnd FIFO writes.'
$projectedDepthStart = 0x1000 * 6
$projectedForegroundStart = (128 - 0x1000) * 6
$projectedDepthStep = 6
Assert-Equal ([math]::Floor(($projectedDepthStart - 1) / $projectedDepthStep)) 4095 'Renderer first synthetic no-Z depth must be the far signed 20.12 NDC endpoint.'
Assert-Equal ([math]::Truncate(($projectedForegroundStart - 1) / $projectedDepthStep)) -3968 'Renderer first post-source-Z no-Z depth must be the near signed 20.12 NDC foreground endpoint.'
$modifyVtxW0 = 0x02140002
Assert-Equal (($modifyVtxW0 -shr 16) -band 0xff) 0x14 'G_MODIFYVTX fixture no longer decodes G_MWO_POINT_ST.'
Assert-Equal (($modifyVtxW0 -band 0xffff) / 2) 1 'G_MODIFYVTX fixture no longer targets cached vertex 1.'
Assert-True ($rendererAdapter.Contains('NDS_RENDERER_ADAPTER_G_MWO_POINT_ST')) 'Battle DL adapter G_MWO_POINT_ST target is missing.'
Assert-True ($rendererAdapter.Contains('state->vertices[index].s = (s16)(command->w1 >> 16)')) 'Battle DL adapter diagnostics do not replace cached vertex S.'
Assert-True ($rendererAdapter.Contains('state->vertices[index].t = (s16)(command->w1 & 0xffffu)')) 'Battle DL adapter diagnostics do not replace cached vertex T.'
Assert-True ($rendererAdapter.Contains('NDSRendererVertexCache sNdsRendererAdapterStageVertexCache')) 'Stage traversal renderer vertex cache is missing.'
Assert-True ($rendererAdapter -match '(?s)ndsRendererAdapterBeginStageTraversal\(void\).*?ndsRendererInitVertexCache\(&sNdsRendererAdapterStageVertexCache\)') 'Stage traversal does not reset source RSP vertex-cache validity and snapshot ownership at its boundary.'
Assert-True ($rendererAdapter -match '(?s)ndsRendererExecuteDisplayListWithVertexCache\(.*?&sNdsRendererAdapterStageVertexCache') 'Stage DObjs do not share the source RSP vertex cache.'
Assert-True ($rendererAdapter -match '(?s)#if NDS_RENDERER_HW_TRIANGLES && \(NDS_RENDERER_PROFILE_LEVEL < 2\).*?ndsFighterDLDrawResetTransientRendererStats.*?offsetof\(NDSRendererStats, othermode_h\).*?prim_depth_command_count = 0u;') 'Profiles 0/1 no longer reset the exact transient renderer fields before reusing live ordered-list state.'
Assert-True ($rendererAdapter -match '(?s)ndsFighterDLDrawResetRuntimeRendererStats.*?blocker = NDS_RENDERER_BLOCKER_NONE;.*?command_count = 0u;.*?unsupported_command_count = 0u;.*?end_command_count = 0u;.*?hardware_triangle_count = 0u;.*?hardware_texture_reject_count = 0u;') 'Profiles 0/1 null-callback traversal no longer clears only execution guards and owner-level hardware totals.'
Assert-True ($rendererAdapter -match '(?s)render_stats = &sNdsRendererAdapterStagePersistentStats;.*?ndsFighterDLDrawResetTransientRendererStats\(render_stats\).*?#if NDS_RENDERER_PROFILE_LEVEL >= 2.*?ndsFighterDLDrawCopyPersistentRendererState') 'Stage runtime/forensic renderer-state ownership split regressed.'
Assert-True ($rendererAdapter -match '(?s)detailed_output = \(ndsRendererHardwareNoOracleEnabled\(\) == FALSE\).*?state\.segment_e_base = NULL;.*?state\.segment_e_base =\s*sNdsRendererAdapterStagePersistentState\.segment_e_base;.*?sNdsRendererAdapterStagePersistentState\.segment_e_base =\s*state\.segment_e_base;') 'Profiles 0/1 stage traversal no longer keeps null-callback resolver state separate from profile-2 software-preview vertex carry.'
Assert-True ($rendererAdapter -match '(?s)detailed_output = \(\(pixels != NULL\) \|\| \(no_oracle == FALSE\)\).*?persistent_state\.segment_e_base = NULL;.*?current_state = \(detailed_output != FALSE\) \? &states\[i\] :\s*&persistent_state;.*?runtime_hardware_triangle_count \+=\s*current_stats->hardware_triangle_count;.*?gNdsFighterDLAllDrawP0HardwareTriangleCount \+=\s*runtime_hardware_triangle_count;') 'Profiles 0/1 no longer keep null-callback fighter traversal out of the per-part software-preview/proof ledger while publishing one owner-level triangle total.'
Assert-True ($rendererAdapter.Contains('ndsRendererInitVertexCache(&persistent_renderer_vertices)')) 'Fighter traversal does not reset its persistent RSP vertex/snapshot cache at the fighter boundary.'
Assert-True ($rendererAdapter.Contains('static NDSRendererVertexCache persistent_renderer_vertices')) 'Fighter matrix snapshots returned to BattleShip task-stack storage.'
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
Assert-True ($rendererAdapter.Contains('for (; i != 0u; i--)') -and $rendererAdapter.Contains('ndsRendererMtxMul20p12(&local, out, out)')) 'Battle DL renderer DObj parent-chain composition order regressed.'
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
Assert-True ($relocAssets -match '(?s)#if !NDS_RENDERER_HW_TRIANGLES \|\| \(NDS_RENDERER_PROFILE_LEVEL >= 2\)\s*static NDSRendererStats\s*sNdsFighterDLAllDrawStats') 'Runtime builds again allocate the profile-2 per-part forensic stats array.'
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
Assert-True ($buildProfile.Contains('if ($stamp.profile -ne $Profile)')) 'Profile prebuild stamp validation does not reject a stamp from another profile.'
Assert-True ($buildProfile.Contains('Prebuild stamp profile mismatch: stamp={0} requested={1}')) 'Profile prebuild stamp mismatch does not identify the stamped and requested profiles.'
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
Write-Output 'GBI decode fixtures passed: F3DEX2 VTX/TRI/MTX/POPMTX packing, F3DEX MVP-recalc matrix move-word packing, transformed and hardware raw/material triangle readiness, N64 matrix to DS 20.12 modelview-projection/modelview-stack vertex transform, wallpaper inverse/overlap oracle, and source snippets verified.'
