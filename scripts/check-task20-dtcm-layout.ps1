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

    $playbackOwnerNames = @(
        'sControllerPlaybackEnabled'
        'sControllerPlaybackConnectedMask'
        'sControllerPlaybackPads'
    )
    $applicationOwners = @($owners | Where-Object {
        $playbackOwnerNames -contains $_.Name
    })
    if ($applicationOwners.Count -ne 0 -and
        $applicationOwners.Count -ne $playbackOwnerNames.Count) {
        throw "Controller playback DTCM owners must be present all-or-none in '$resolvedElf'."
    }
    $playbackBytes = if ($applicationOwners.Count -eq 0) { 0 } else { 32 }

    # R2-03 E29. The two hot fighter vertex tables. Both are randomly indexed by
    # 1,878 corners a frame and did not fit the 4 KB data cache in main RAM.
    # Audited for this gate's DMA/IPC/ARM7 requirement: both are written and read
    # only by ARM9 code in the fighter draw, neither is ever a DMA source or
    # destination (the renderer's only GXFIFO DMA is the stage replay's
    # owner->words buffer in main RAM), and neither is visible to the ARM7 or
    # IPC. They lead the section, so everything below shifts up by their size.
    $fighterOwnerSizes = [ordered]@{
        'sNdsNativeFighterDenseNormals'  = 2164
        'sNdsNativeFighterPreparedDense' = 6492
    }
    $fighterOwners = @($owners | Where-Object {
        $fighterOwnerSizes.Contains($_.Name)
    })
    if ($fighterOwners.Count -ne 0 -and
        $fighterOwners.Count -ne $fighterOwnerSizes.Count) {
        throw "Fighter DTCM owners must be present all-or-none in '$resolvedElf'."
    }
    # Cycle 110 puts the renderer's per-frame counter block in the same section
    # on the same terms. 108 bytes of u32 counters read-modify-written on every
    # hardware batch, every matrix load and every texture prepare; compiling
    # them out measured FTR -7,378 and STG -2,776, and they cannot be compiled
    # out because verify-battle-mariofox-gcrunall-loop-harness.ps1 asserts exact
    # batch and texture-prepare accounting off them. DTCM keeps the evidence and
    # stops paying main-RAM latency and a cache line for it. Same audit as the
    # two tables above: ARM9 renderer code only, never a DMA source or
    # destination, never visible to the ARM7 or IPC.
    #
    # OPTIONAL, unlike the pair above, because it only exists at
    # NDS_RENDERER_PROFILE_LEVEL < 2 -- an all-or-none rule would reject a
    # level-2 ELF for a reason that has nothing to do with the layout. Its size
    # is still pinned when it is there.
    $rendererOwnerSizes = [ordered]@{
        'sNdsRendererRuntimeFrameSummary' = 108
    }

    $fighterBytes = 0
    if ($fighterOwners.Count -ne 0) {
        foreach ($size in $fighterOwnerSizes.Values) { $fighterBytes += $size }
        foreach ($name in $rendererOwnerSizes.Keys) {
            $owner = @($owners | Where-Object { $_.Name -eq $name })
            if ($owner.Count -eq 0) { continue }
            if ($owner[0].Bytes -ne $rendererOwnerSizes[$name]) {
                throw ("DTCM owner '$name' is $($owner[0].Bytes) bytes, " +
                    "expected $($rendererOwnerSizes[$name]), in '$resolvedElf'.")
            }
            $fighterBytes += $rendererOwnerSizes[$name]
        }
        # The linker realigns to 32 after .dtcm.fighter so that Calico's
        # __irq_table keeps its 32-byte boundary no matter how the data-driven
        # fighter table sizes come out.
        $fighterBytes = [int](([math]::Ceiling($fighterBytes / 32.0)) * 32)
    }
    $dtcmBytes = $fighterBytes + $playbackBytes

    if ($dtcm.Address -ne $expectedBase -or
        $dtcmBss.Address -ne ($expectedBase + $dtcmBytes) -or
        $dtcmStart -ne $expectedBase -or
        $dtcm.Bytes -ne $dtcmBytes -or
        $dtcmEnd -ne ($expectedBase + $dtcmBytes) -or
        $dtcmBss.Bytes -ne 152 -or
        $dtcmBssEnd -ne ($expectedBase + $dtcmBytes + 152) -or
        $spUsr -ne ($expectedBase + 0x3e80) -or
        $spIrq -ne ($spUsr + 0x100) -or
        $spSvc -ne ($spIrq + 0x40) -or
        $irqFlags -ne ($spSvc + 0x38) -or
        $physicalEnd -ne ($spSvc + 0x40)) {
        throw "DTCM section or Calico user/IRQ/SVC/BIOS boundary changed in '$resolvedElf'."
    }

    $expectedOwners = @{
        '__irq_table' = [PSCustomObject]@{
            Address = $expectedBase + $dtcmBytes
            Section = '.dtcm.bss'
            Bytes = 128
            Alignment = 32
        }
        '__sched_state' = [PSCustomObject]@{
            Address = $expectedBase + $dtcmBytes + 128
            Section = '.dtcm.bss'
            Bytes = 24
            Alignment = 32
        }
    }
    if ($fighterBytes -ne 0) {
        $fighterAddress = $expectedBase
        foreach ($entry in $fighterOwnerSizes.GetEnumerator()) {
            $expectedOwners[$entry.Key] = [PSCustomObject]@{
                Address = $fighterAddress
                Section = '.dtcm'
                Bytes = $entry.Value
                Alignment = 4
            }
            $fighterAddress += $entry.Value
        }
        # Optional renderer owners follow the pinned pair, in declaration order.
        foreach ($entry in $rendererOwnerSizes.GetEnumerator()) {
            if (@($owners | Where-Object { $_.Name -eq $entry.Key }).Count -eq 0) {
                continue
            }
            $expectedOwners[$entry.Key] = [PSCustomObject]@{
                Address = $fighterAddress
                Section = '.dtcm'
                Bytes = $entry.Value
                Alignment = 4
            }
            $fighterAddress += $entry.Value
        }
    }
    if ($playbackBytes -ne 0) {
        # The fighter tables lead the section, so playback starts above them.
        $playbackBase = $expectedBase + $fighterBytes
        $expectedOwners['sControllerPlaybackEnabled'] = [PSCustomObject]@{
            Address = $playbackBase
            Section = '.dtcm'
            Bytes = 4
            Alignment = 4
        }
        $expectedOwners['sControllerPlaybackConnectedMask'] =
            [PSCustomObject]@{
                Address = $playbackBase + 4
                Section = '.dtcm'
                Bytes = 4
                Alignment = 4
            }
        $expectedOwners['sControllerPlaybackPads'] = [PSCustomObject]@{
            Address = $playbackBase + 8
            Section = '.dtcm'
            Bytes = 24
            Alignment = 4
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
            ($owner.Address % $expected.Alignment) -ne 0) {
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

    Write-Output ("Task 20 DTCM layout passed: elf={0} sections={1}/{2} owners=[{3}] sharedGap=0x{4:x8}..0x{5:x8}/{6} stackTops=usr:0x{7:x8},irq:0x{8:x8},svc:0x{9:x8} reserves=irq:256,svc:64,bios:64 forbiddenDmaRefs=0(applicationOwners={10}) stackEvidence=[{11}]" -f
        (Split-Path -Leaf $resolvedElf), $dtcm.Bytes, $dtcmBss.Bytes,
        $ownerText, $dtcmBssEnd, $spUsr, $sharedGap, $spUsr, $spIrq,
        $spSvc, $applicationOwners.Count, $stackText)
}
