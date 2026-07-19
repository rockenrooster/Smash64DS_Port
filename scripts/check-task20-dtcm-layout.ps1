param(
    [Parameter(Mandatory = $true)]
    [string[]]$Elf,
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [switch]$StackEvidence,
    [ValidateRange(0, 32768)]
    [int]$GameplayHighWater = 0,
    [ValidateRange(0, 32768)]
    [int]$MainHighWater = 0,
    [ValidateRange(1, 4096)]
    [int]$GuardBytes = 64,
    [ValidateRange(0, 4096)]
    [int]$MarginBytes = 1024
)

$ErrorActionPreference = 'Stop'

function Get-Task20Section {
    param(
        [string[]]$Lines,
        [string]$Name,
        [string]$ElfPath
    )

    $escapedName = [regex]::Escape($Name)
    $matches = @($Lines | Where-Object {
        $_ -match "^\s*\d+\s+$escapedName\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+.*?2\*\*(\d+)\s*$"
    })
    if ($matches.Count -ne 1) {
        throw "Expected exactly one $Name section in '$ElfPath'."
    }
    $null = $matches[0] -match "^\s*\d+\s+$escapedName\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+.*?2\*\*(\d+)\s*$"
    return [PSCustomObject]@{
        Name = $Name
        Bytes = [Convert]::ToUInt32($Matches[1], 16)
        Address = [Convert]::ToUInt32($Matches[2], 16)
        Alignment = 1 -shl [int]$Matches[3]
    }
}

function Get-Task20SymbolAddress {
    param(
        [string[]]$Lines,
        [string]$Name,
        [string]$ElfPath
    )

    $escapedName = [regex]::Escape($Name)
    $matches = @($Lines | Where-Object {
        $_ -match "^\s*([0-9a-fA-F]+)\s+.*\s+$escapedName\s*$"
    })
    if ($matches.Count -ne 1) {
        throw "Expected exactly one $Name symbol in '$ElfPath'."
    }
    $null = $matches[0] -match '^\s*([0-9a-fA-F]+)\s+'
    return [Convert]::ToUInt32($Matches[1], 16)
}

if (-not (Test-Path -LiteralPath $Objdump -PathType Leaf)) {
    throw "arm-none-eabi-objdump was not found at '$Objdump'."
}
if ($StackEvidence -and
    (-not $PSBoundParameters.ContainsKey('GameplayHighWater') -or
     -not $PSBoundParameters.ContainsKey('MainHighWater'))) {
    throw 'StackEvidence requires explicit GameplayHighWater and MainHighWater values.'
}

foreach ($elfPath in $Elf) {
    $resolvedElf = (Resolve-Path -LiteralPath $elfPath).Path
    $sectionLines = @(& $Objdump -h $resolvedElf)
    if ($LASTEXITCODE -ne 0) {
        throw "objdump section listing failed for '$resolvedElf'."
    }
    $symbolLines = @(& $Objdump -t $resolvedElf)
    if ($LASTEXITCODE -ne 0) {
        throw "objdump symbol listing failed for '$resolvedElf'."
    }

    $dtcm = Get-Task20Section -Lines $sectionLines -Name '.dtcm' `
        -ElfPath $resolvedElf
    $dtcmBss = Get-Task20Section -Lines $sectionLines -Name '.dtcm.bss' `
        -ElfPath $resolvedElf

    $dtcmStart = Get-Task20SymbolAddress -Lines $symbolLines `
        -Name '__dtcm_start' -ElfPath $resolvedElf
    $dtcmEnd = Get-Task20SymbolAddress -Lines $symbolLines `
        -Name '__dtcm_end' -ElfPath $resolvedElf
    $dtcmBssEnd = Get-Task20SymbolAddress -Lines $symbolLines `
        -Name '__dtcm_bss_end' -ElfPath $resolvedElf
    $spUsr = Get-Task20SymbolAddress -Lines $symbolLines `
        -Name '__sp_usr' -ElfPath $resolvedElf
    $spIrq = Get-Task20SymbolAddress -Lines $symbolLines `
        -Name '__sp_irq' -ElfPath $resolvedElf
    $spSvc = Get-Task20SymbolAddress -Lines $symbolLines `
        -Name '__sp_svc' -ElfPath $resolvedElf
    $irqFlags = Get-Task20SymbolAddress -Lines $symbolLines `
        -Name '__irq_flags' -ElfPath $resolvedElf

    $expectedBase = [uint32]0x02ff0000
    $physicalEnd = [uint32]0x02ff4000
    if ($dtcm.Address -ne $expectedBase -or
        $dtcmBss.Address -ne $expectedBase -or
        $dtcmStart -ne $expectedBase -or
        $dtcm.Bytes -ne 0 -or
        $dtcmEnd -ne $expectedBase -or
        $dtcmBss.Bytes -ne 152 -or
        $dtcmBssEnd -ne ($expectedBase + 152) -or
        $spUsr -ne ($expectedBase + 0x3e80) -or
        $spIrq -ne ($spUsr + 0x100) -or
        $spSvc -ne ($spIrq + 0x40) -or
        $irqFlags -ne ($spSvc + 0x38) -or
        $physicalEnd -ne ($spSvc + 0x40)) {
        throw "DTCM section or Calico user/IRQ/SVC/BIOS boundary changed in '$resolvedElf'."
    }

    $owners = [System.Collections.Generic.List[object]]::new()
    foreach ($line in $symbolLines) {
        if ($line -notmatch '^\s*([0-9a-fA-F]+)\s+\S+\s+O\s+(\.dtcm(?:\.bss)?)\s+([0-9a-fA-F]+)\s+(\S+)\s*$') {
            continue
        }
        $owners.Add([PSCustomObject]@{
            Address = [Convert]::ToUInt32($Matches[1], 16)
            Section = $Matches[2]
            Bytes = [Convert]::ToUInt32($Matches[3], 16)
            Name = $Matches[4]
        }) | Out-Null
    }

    $expectedOwners = @{
        '__irq_table' = [PSCustomObject]@{
            Address = $expectedBase
            Section = '.dtcm.bss'
            Bytes = 128
        }
        '__sched_state' = [PSCustomObject]@{
            Address = $expectedBase + 128
            Section = '.dtcm.bss'
            Bytes = 24
        }
    }
    if ($owners.Count -ne $expectedOwners.Count) {
        throw "Unexpected DTCM object count $($owners.Count) in '$resolvedElf'."
    }
    foreach ($owner in $owners) {
        if (-not $expectedOwners.ContainsKey($owner.Name)) {
            throw "Unexpected application DTCM object '$($owner.Name)' in '$resolvedElf'; audit DMA/IPC/ARM7 access before placement."
        }
        $expected = $expectedOwners[$owner.Name]
        if ($owner.Address -ne $expected.Address -or
            $owner.Section -ne $expected.Section -or
            $owner.Bytes -ne $expected.Bytes -or
            ($owner.Address % 32) -ne 0) {
            throw "DTCM owner '$($owner.Name)' changed address, section, size, or observed alignment in '$resolvedElf'."
        }
    }

    $sharedGap = [int64]$spUsr - [int64]$dtcmBssEnd
    $ownerText = @($owners | Sort-Object Address | ForEach-Object {
        '{0}=0x{1:x8}+{2}' -f $_.Name, $_.Address, $_.Bytes
    }) -join ', '
    $stackText = 'not-supplied'
    if ($StackEvidence) {
        $rawNeed = [int64]$GameplayHighWater + $MainHighWater + $GuardBytes
        $marginNeed = $rawNeed + (2 * $MarginBytes)
        $rawVerdict = if ($rawNeed -le $sharedGap) {
            "FIT+$($sharedGap - $rawNeed)"
        } else {
            "NO_FIT-$($rawNeed - $sharedGap)"
        }
        $marginVerdict = if ($marginNeed -le $sharedGap) {
            "FIT+$($sharedGap - $marginNeed)"
        } else {
            "NO_FIT-$($marginNeed - $sharedGap)"
        }
        $stackText = "gameplayHwm=$GameplayHighWater mainHwm=$MainHighWater guard=$GuardBytes raw=$rawNeed/$rawVerdict margins=2x$MarginBytes marginNeed=$marginNeed/$marginVerdict"
    }

    Write-Output ("Task 20 DTCM layout passed: elf={0} sections={1}/{2} owners=[{3}] sharedGap=0x{4:x8}..0x{5:x8}/{6} stackTops=usr:0x{7:x8},irq:0x{8:x8},svc:0x{9:x8} reserves=irq:256,svc:64,bios:64 forbiddenDmaRefs=0(applicationOwners=0) stackEvidence=[{10}]" -f
        (Split-Path -Leaf $resolvedElf), $dtcm.Bytes, $dtcmBss.Bytes,
        $ownerText, $dtcmBssEnd, $spUsr, $sharedGap, $spUsr, $spIrq,
        $spSvc, $stackText)
}
