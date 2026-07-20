[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$Map,
    [ValidateSet(0, 1)][int]$ExpectedEnabled,
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [string]$Nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
)

$ErrorActionPreference = 'Stop'

foreach ($path in @($Elf, $Map, $Objdump, $Nm)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Task 32 input is missing: $path"
    }
}

$expected = @(
    @('ndsRendererAdapterFindDObjWorldMatrix', '.text.ndsRendererAdapterFindDObjWorldMatrix', 'scene_backend.o'),
    @('ndsRendererAdapterBuildDObjWorldMatrix', '.text.ndsRendererAdapterBuildDObjWorldMatrix', 'scene_backend.o'),
    @('ndsRendererNativeStagePrepareRun.constprop.0', '.text.ndsRendererNativeStagePrepareRun.constprop.0', 'nds_renderer.o'),
    @('ndsRendererMtxMul20p12', '.text.ndsRendererMtxMul20p12', 'nds_renderer.o'),
    @('ndsRendererMtxMulAffine20p12', '.text.ndsRendererMtxMulAffine20p12', 'nds_renderer.o'),
    @('ndsRendererMtxLoadN64ToDS20p12', '.text.ndsRendererMtxLoadN64ToDS20p12', 'nds_renderer.o'),
    @('ndsRendererLoadHardwareMatrixPair.isra.0', '.text.ndsRendererLoadHardwareMatrixPair.isra.0', 'nds_renderer.o'),
    @('ndsRendererCommitNativeStageSegment', '.text.ndsRendererCommitNativeStageSegment', 'nds_renderer.o'),
    @('ndsRendererNativeStageLoadNoZMatrix', '.text.ndsRendererNativeStageLoadNoZMatrix', 'nds_renderer.o'),
    @('ndsRendererNativeStageEmitNoZTriangle', '.text.ndsRendererNativeStageEmitNoZTriangle', 'nds_renderer.o'),
    @('ndsRendererNativeApplyStateDelta.part.0', '.text.ndsRendererNativeApplyStateDelta.part.0', 'nds_renderer.o'),
    @('ndsRendererNativeApplyStateSpan', '.text.ndsRendererNativeApplyStateSpan', 'nds_renderer.o'),
    @('ndsRendererSyncTextureTile', '.text.ndsRendererSyncTextureTile', 'nds_renderer.o')
)

$sections = @{}
foreach ($line in (& $Objdump -h $Elf)) {
    if ($line -match '^\s*([0-9]+)\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)') {
        $sections[$matches[2]] = [pscustomobject]@{
            Index = [int]$matches[1]
            Size = [Convert]::ToUInt32($matches[3], 16)
            Vma = [Convert]::ToUInt32($matches[4], 16)
            Lma = [Convert]::ToUInt32($matches[5], 16)
        }
    }
}
if (-not $sections.ContainsKey('.text.hot') -or -not $sections.ContainsKey('.main')) {
    throw 'Task 17 hot text or main text section is absent.'
}
$update = $sections['.text.hot']
$main = $sections['.main']
if ($update.Size -ne 5016) {
    throw "Task 17 update hot text changed from 5,016 to $($update.Size) bytes."
}

$nmRows = @{}
foreach ($line in (& $Nm -S -n $Elf)) {
    if ($line -match '^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+[A-Za-z]\s+(.+)$') {
        $nmRows[$matches[3]] = [pscustomobject]@{
            Address = [Convert]::ToUInt32($matches[1], 16)
            Size = [Convert]::ToUInt32($matches[2], 16)
        }
    }
}

$mapText = Get-Content -LiteralPath $Map -Raw
$mainStartMatch = [regex]::Match(
    $mapText,
    '(?m)^\s*(0x[0-9a-fA-F]+)\s+PROVIDE \(__main_start = ADDR \(\.text\.hot\)\)')
if (-not $mainStartMatch.Success) {
    throw 'Map does not publish __main_start from .text.hot.'
}
$mainStart = [Convert]::ToUInt32($mainStartMatch.Groups[1].Value.Substring(2), 16)
if ($mainStart -ne $update.Vma) {
    throw ('Main load start moved away from .text.hot: 0x{0:x8} != 0x{1:x8}.' -f
        $mainStart, $update.Vma)
}

$draw = if ($sections.ContainsKey('.text.hot.draw')) {
    $sections['.text.hot.draw']
} else {
    $null
}
$expectedBytes = 0u
foreach ($item in $expected) {
    $symbol = [string]$item[0]
    if (-not $nmRows.ContainsKey($symbol)) {
        throw "Task 32 symbol vanished: $symbol"
    }
    $expectedBytes += $nmRows[$symbol].Size
}
if ($ExpectedEnabled -eq 0) {
    if (($null -ne $draw) -and ($draw.Size -ne 0)) {
        throw "Disabled Task 32 build retained $($draw.Size) draw-hot bytes."
    }
    $disabledGap = $main.Vma - ($update.Vma + $update.Size)
    if (($disabledGap -lt 0) -or ($disabledGap -ge 8)) {
        throw 'Disabled Task 32 build inserted bytes between update hot text and main.'
    }
    $mainEnd = $main.Vma + $main.Size
    foreach ($item in $expected) {
        $row = $nmRows[[string]$item[0]]
        if (($row.Address -lt $main.Vma) -or
            ($row.Address + $row.Size -gt $mainEnd)) {
            throw "Disabled Task 32 symbol escaped .main: $($item[0])"
        }
    }
    Write-Output ('Task 32 draw hot text disabled: update={0} bytes; main-start=0x{1:x8}; candidate symbols remain in .main.' -f
        $update.Size, $mainStart)
    exit 0
}

if ($null -eq $draw) {
    throw 'Enabled Task 32 build has no .text.hot.draw section.'
}
if ($draw.Size -ne $expectedBytes -or $draw.Size -gt 8192) {
    throw "Task 32 draw hot text is $($draw.Size) bytes; expected $expectedBytes and at most 8,192."
}
$mainGap = $main.Vma - ($draw.Vma + $draw.Size)
if (($draw.Index -ne ($update.Index + 1)) -or
    ($draw.Vma -ne ($update.Vma + $update.Size)) -or
    ($mainGap -lt 0) -or ($mainGap -ge 8)) {
    throw 'Task 32 draw hot text is not immediately between update hot text and main.'
}

$drawEnd = $draw.Vma + $draw.Size
$drawMap = [regex]::Match(
    $mapText,
    '(?ms)^\.text\.hot\.draw\s+.*?(?=^\.main\s+)').Value
if ([string]::IsNullOrWhiteSpace($drawMap)) {
    throw 'Map has no Task 32 draw-hot block.'
}
foreach ($item in $expected) {
    $symbol = [string]$item[0]
    $inputSection = [string]$item[1]
    $object = [string]$item[2]
    $row = $nmRows[$symbol]
    if (($row.Address -lt $draw.Vma) -or ($row.Address + $row.Size -gt $drawEnd)) {
        throw "Task 32 symbol is outside .text.hot.draw: $symbol"
    }
    if (($drawMap -notmatch [regex]::Escape($inputSection)) -or
        ($drawMap -notmatch [regex]::Escape($object))) {
        throw "Task 32 map lost $object($inputSection)."
    }
}

Write-Output ('Task 32 draw hot text enabled: {0} symbols, {1} bytes, VMA 0x{2:x8}-0x{3:x8}; update remains {4} bytes at main-load start 0x{5:x8}.' -f
    $expected.Count, $draw.Size, $draw.Vma, $drawEnd, $update.Size, $mainStart)
