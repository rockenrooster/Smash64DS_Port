param(
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$RendererObject,
    [ValidateSet('Arm', 'Thumb')][string]$ExpectedMainMode = 'Thumb',
    [ValidateRange(0, 32768)][int]$ExpectedItcmBytes = 0,
    [switch]$RequireHotText,
    [string[]]$ExpectedHotFunction = @(),
    [string]$Readelf =
        'C:\devkitPro\devkitARM\bin\arm-none-eabi-readelf.exe'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $Readelf -PathType Leaf)) {
    throw "arm-none-eabi-readelf was not found at '$Readelf'."
}
$resolvedElf = (Resolve-Path -LiteralPath $Elf).Path
$resolvedObject = (Resolve-Path -LiteralPath $RendererObject).Path

function Get-ArmSections {
    param([Parameter(Mandatory = $true)][string]$Path)

    $result = @{}
    foreach ($line in @(& $Readelf -SW $Path)) {
        if ($line -notmatch (
                '^\s*\[\s*(\d+)\]\s+(\S+)\s+\S+\s+' +
                '([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+' +
                '([0-9a-fA-F]+)\s+')) {
            continue
        }
        $index = [int]$Matches[1]
        $result[$index] = [pscustomobject]@{
            Index = $index
            Name = $Matches[2]
            Address = [Convert]::ToUInt64($Matches[3], 16)
            Bytes = [Convert]::ToUInt64($Matches[4], 16)
        }
    }
    if ($LASTEXITCODE -ne 0 -or $result.Count -eq 0) {
        throw "readelf section-table read failed for '$Path'."
    }
    return $result
}

function Get-ArmSymbols {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][hashtable]$Sections
    )

    $result = @()
    foreach ($line in @(& $Readelf -sW $Path)) {
        if ($line -notmatch (
                '^\s*\d+:\s+([0-9a-fA-F]+)\s+(\d+)\s+' +
                '(\S+)\s+\S+\s+\S+\s+(\S+)\s+(\S+)\s*$')) {
            continue
        }
        $sectionIndex = 0
        if (-not [int]::TryParse($Matches[4], [ref]$sectionIndex) -or
            -not $Sections.ContainsKey($sectionIndex)) {
            continue
        }
        $result += [pscustomobject]@{
            Value = [Convert]::ToUInt64($Matches[1], 16)
            Bytes = [Convert]::ToUInt64($Matches[2], 10)
            Type = $Matches[3]
            Section = $Sections[$sectionIndex]
            Name = $Matches[5]
        }
    }
    if ($LASTEXITCODE -ne 0) {
        throw "readelf symbol-table read failed for '$Path'."
    }
    return $result
}

function Test-ThumbSymbol {
    param([Parameter(Mandatory = $true)]$Symbol)
    return (($Symbol.Value -band 1u) -ne 0u)
}

$objectSections = Get-ArmSections -Path $resolvedObject
$objectSymbols = Get-ArmSymbols -Path $resolvedObject -Sections $objectSections
$objectFunctions = @($objectSymbols | Where-Object {
    $_.Type -eq 'FUNC' -and $_.Bytes -gt 0u -and
    ($_.Section.Name -eq '.text' -or
     $_.Section.Name.StartsWith('.text.') -or
     $_.Section.Name -eq '.itcm' -or
     $_.Section.Name.StartsWith('.itcm.'))
})
if ($objectFunctions.Count -eq 0) {
    throw 'Renderer object contains no auditable text functions.'
}

$objectItcmFunctions = @($objectFunctions | Where-Object {
    $_.Section.Name -eq '.itcm' -or $_.Section.Name.StartsWith('.itcm.')
})
if ($objectItcmFunctions.Count -eq 0) {
    throw 'Renderer object contains no ITCM functions to audit.'
}
$wrongItcm = @($objectItcmFunctions | Where-Object { Test-ThumbSymbol $_ })
if ($wrongItcm.Count -ne 0) {
    throw ("Renderer ITCM function became Thumb: {0}." -f
        (($wrongItcm | ForEach-Object Name) -join ', '))
}

$objectMainFunctions = @($objectFunctions | Where-Object {
    $_.Section.Name -eq '.text' -or $_.Section.Name.StartsWith('.text.')
})
$expectThumb = $ExpectedMainMode -eq 'Thumb'
$wrongMain = @($objectMainFunctions | Where-Object {
    (Test-ThumbSymbol $_) -ne $expectThumb
})
if ($wrongMain.Count -ne 0) {
    throw ("Renderer main-text function mode is not {0}: {1}." -f
        $ExpectedMainMode,
        (($wrongMain | Select-Object -First 12 | ForEach-Object Name) -join ', '))
}

[uint64]$objectTextBytes = 0u
[uint64]$objectItcmBytes = 0u
foreach ($section in $objectSections.Values) {
    if ($section.Name -eq '.text' -or $section.Name.StartsWith('.text.')) {
        $objectTextBytes += $section.Bytes
    } elseif ($section.Name -eq '.itcm' -or
              $section.Name.StartsWith('.itcm.')) {
        $objectItcmBytes += $section.Bytes
    }
}

$elfSections = Get-ArmSections -Path $resolvedElf
$elfSymbols = Get-ArmSymbols -Path $resolvedElf -Sections $elfSections
$elfItcmSections = @($elfSections.Values | Where-Object Name -eq '.itcm')
if ($elfItcmSections.Count -ne 1) {
    throw "Expected exactly one final .itcm section; found $($elfItcmSections.Count)."
}
$elfItcmSection = $elfItcmSections[0]
if ($ExpectedItcmBytes -ne 0 -and
    $elfItcmSection.Bytes -ne [uint64]$ExpectedItcmBytes) {
    throw ("Final ITCM size is {0}; expected {1}." -f
        $elfItcmSection.Bytes, $ExpectedItcmBytes)
}
$badFinalItcm = @($elfSymbols | Where-Object {
    $_.Section.Index -eq $elfItcmSection.Index -and
    (($_.Type -eq 'FUNC' -and $_.Bytes -gt 0u -and (Test-ThumbSymbol $_)) -or
     ($_.Type -eq 'NOTYPE' -and $_.Name -eq '$t'))
})
if ($badFinalItcm.Count -ne 0) {
    throw ("Final .itcm contains Thumb code: {0}." -f
        (($badFinalItcm | ForEach-Object Name) -join ', '))
}

$hotBytes = 0u
$hotNames = @()
if ($RequireHotText) {
    $hotSections = @($elfSections.Values | Where-Object Name -eq '.text.hot')
    if ($hotSections.Count -ne 1) {
        throw "Expected exactly one final .text.hot section; found $($hotSections.Count)."
    }
    $hotSection = $hotSections[0]
    $hotBytes = $hotSection.Bytes
    if ($hotBytes -eq 0u -or $hotBytes -gt 8192u) {
        throw "Final .text.hot is $hotBytes bytes; expected 1..8192."
    }
    $mainText = @($elfSections.Values | Where-Object Name -eq '.text')
    if ($mainText.Count -ne 1 -or
        ($hotSection.Address + $hotSection.Bytes) -gt $mainText[0].Address) {
        throw 'Final .text.hot is not contiguous immediately before .text.'
    }

    $hotFunctions = @($elfSymbols | Where-Object {
        $_.Type -eq 'FUNC' -and $_.Bytes -gt 0u -and
        $_.Section.Index -eq $hotSection.Index
    } | Sort-Object { $_.Value -band (-bnot 1u) })
    if (@($hotFunctions | Where-Object { -not (Test-ThumbSymbol $_) }).Count) {
        throw 'Final .text.hot contains a non-Thumb function.'
    }
    foreach ($expected in $ExpectedHotFunction) {
        $matches = @($hotFunctions | Where-Object {
            $_.Name -eq $expected -or $_.Name.StartsWith("$expected.")
        })
        if ($matches.Count -ne 1) {
            throw "Expected one hot function '$expected'; found $($matches.Count)."
        }
    }
    $hotNames = @($hotFunctions | ForEach-Object Name)
    if ($ExpectedHotFunction.Count -ne 0) {
        $actualExpectedOrder = @($hotFunctions | Where-Object {
            $name = $_.Name
            @($ExpectedHotFunction | Where-Object {
                $name -eq $_ -or $name.StartsWith("$_.")
            }).Count -ne 0
        } | ForEach-Object {
            $name = $_.Name
            @($ExpectedHotFunction | Where-Object {
                $name -eq $_ -or $name.StartsWith("$_.")
            })[0]
        })
        if (($actualExpectedOrder -join ',') -cne
            ($ExpectedHotFunction -join ',')) {
            throw ("Hot function order is '{0}'; expected '{1}'." -f
                ($actualExpectedOrder -join ', '),
                ($ExpectedHotFunction -join ', '))
        }
    }
} elseif (@($elfSections.Values | Where-Object Name -eq '.text.hot').Count) {
    throw 'Unexpected final .text.hot section before Task 12 Phase B.'
}

Write-Output (
    "Task 12 renderer codegen passed: main=$ExpectedMainMode " +
    "objectText=$objectTextBytes objectItcm=$objectItcmBytes " +
    "finalItcm=$($elfItcmSection.Bytes)/32768 hot=$hotBytes " +
    "hotFunctions=[$($hotNames -join ', ')]")
