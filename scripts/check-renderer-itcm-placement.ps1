param(
    [Parameter(Mandatory = $true)]
    [string[]]$Elf,
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [switch]$BenchmarkAblation,
    [ValidateRange(1, 32768)]
    [int]$MaxItcmBytes = 32768
)

$ErrorActionPreference = 'Stop'

$hotFunctions = @(
    'ndsRendererApplyVertexCommand',
    'ndsRendererHardwareLitShadeColorPrepared',
    'ndsRendererHardwareConvertTexel01Ci4Direct',
    'ndsRendererHardwareSubmitVertex',
    'ndsRendererSubmitHardwareTriangle',
    'ndsRendererScanList'
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

if (-not (Test-Path -LiteralPath $Objdump -PathType Leaf)) {
    throw "arm-none-eabi-objdump was not found at '$Objdump'."
}

foreach ($elfPath in $Elf) {
    $resolvedElf = (Resolve-Path -LiteralPath $elfPath).Path
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
    foreach ($baseName in $hotFunctions) {
        $matches = @($functionSymbols | Where-Object {
            ($_.Name -eq $baseName) -or $_.Name.StartsWith("$baseName.")
        })
        if (($requiredEmittedFunctions -contains $baseName) -and
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

    Write-Output ("Renderer ITCM placement passed: elf={0} itcm={1}/{2} renderer={3} symbols=[{4}]" -f
        (Split-Path -Leaf $resolvedElf), $itcmBytes, $MaxItcmBytes,
        $rendererItcmBytes, ($emittedNames -join ', '))
}
