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
    [int]$MarginBytes = 1024,
    # The linker script is an INPUT to this check, not a constant, because the
    # P2-2p8 hot-scalar block below is derived from the names it lists.
    [string]$LinkerScript = (Join-Path $PSScriptRoot '..\linker\nds_hot_text.ld')
)

$ErrorActionPreference = 'Stop'

function Get-Task20DTCMHotScalarNames {
    <#
    .SYNOPSIS
    The statics the linker script deliberately places in DTCM, by name.

    .DESCRIPTION
    Parsed from the script's own DTCM-RESIDENT HOT SCALARS markers so this
    check and the placement cannot drift apart. There is more than one block
    on purpose: an initialised static belongs in the LOADED .dtcm section and a
    zero-initialised one in NOLOAD .dtcm.bss, and putting a .data static in the
    NOLOAD section links cleanly while silently dropping its initialiser.

    Returns an empty list when the block is absent, so this is safe on a tree
    that predates the lane.
    #>
    param([string]$LinkerScript)

    if (-not (Test-Path -LiteralPath $LinkerScript)) {
        throw "check-task20-dtcm-layout: missing linker script '$LinkerScript'."
    }
    $text = Get-Content -LiteralPath $LinkerScript -Raw
    $begin = '/* DTCM-RESIDENT HOT SCALARS: BEGIN */'
    $end = '/* DTCM-RESIDENT HOT SCALARS: END */'
    $beginCount = ([regex]::Matches($text, [regex]::Escape($begin))).Count
    $endCount = ([regex]::Matches($text, [regex]::Escape($end))).Count
    if ($beginCount -ne $endCount) {
        throw ("check-task20-dtcm-layout: '$LinkerScript' has $beginCount " +
            "BEGIN and $endCount END hot-scalar markers.")
    }
    if ($beginCount -eq 0) { return @() }

    $names = New-Object System.Collections.Generic.List[string]
    foreach ($chunk in ($text -split [regex]::Escape($begin))[1..$beginCount]) {
        $body = ($chunk -split [regex]::Escape($end))[0]
        foreach ($line in ($body -split "`n")) {
            $trimmed = ($line -split '/\*')[0].Trim()
            if ([string]::IsNullOrWhiteSpace($trimmed)) { continue }
            $m = [regex]::Match($trimmed, '^\*\(\s*([^)\s]+)\s*\)')
            if (-not $m.Success) {
                throw ("check-task20-dtcm-layout: cannot parse hot-scalar " +
                    "input section from '$trimmed'.")
            }
            $section = $m.Groups[1].Value
            if ($section -match '[*?]') {
                throw ("check-task20-dtcm-layout: '$section' is a wildcard. " +
                    "This block names one section per entry so a gather that " +
                    "matches nothing can be detected.")
            }
            $names.Add(($section -split '\.')[-1])
        }
    }
    return $names.ToArray()
}

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

    # R2-03 E29's Mario dense tables left DTCM for main RAM (P2-2p8): every VS
    # kind draws through the lean list, which reads them only while it
    # materializes a Mario list, and their bytes now hold the DTCM hot stack.
    # They must not come back.
    foreach ($name in @('sNdsNativeFighterDenseNormals',
                        'sNdsNativeFighterPreparedDense')) {
        if (@($owners | Where-Object { $_.Name -eq $name }).Count -ne 0) {
            throw ("DTCM owner '$name' is back in DTCM in '$resolvedElf'; " +
                'its bytes hold the hot stack.')
        }
    }
    # Named .dtcm owners, sizes pinned, order the linker's. The renderer's
    # per-frame counter block (cycle 110: FTR -7,378 / STG -2,776 when it left
    # main RAM; ARM9 renderer code only, never a DMA endpoint, never visible
    # to the ARM7 or IPC) and the lean / VRAM lab control words, which are
    # read on every draw and poked by the lab harnesses. Each is optional:
    # configurations differ.
    $dtcmNamedSizes = [ordered]@{
        'sNdsRendererRuntimeFrameSummary' = 108
        'gNdsFtrAdmitIdentCensus' = 4
        'gNdsFtrLeanSlow' = 4
        'gNdsVramCensusEnable' = 4
        'gNdsFtrLeanAdmit' = 4
        'gNdsFtrLeanRoute' = 4
    }
    foreach ($entry in $dtcmNamedSizes.GetEnumerator()) {
        $owner = @($owners | Where-Object { $_.Name -eq $entry.Key })
        if (($owner.Count -ne 0) -and
            (($owner[0].Bytes -ne $entry.Value) -or
             ($owner[0].Section -ne '.dtcm'))) {
            throw ("DTCM owner '$($entry.Key)' is $($owner[0].Bytes) bytes " +
                "in $($owner[0].Section), expected $($entry.Value) in .dtcm, " +
                "in '$resolvedElf'.")
        }
    }
    # The DTCM hot stack (src/port/coroutine.c) leads .dtcm.bss by its own
    # linker line. ARM9 code only: the subtrees that run on it do no I/O and
    # hand no stack address to DMA, the ARM7 or storage.
    $hotStack = @($owners | Where-Object { $_.Name -eq 'gNdsDtcmHotStack' })
    if ($hotStack.Count -ne 1) {
        throw "DTCM hot stack gNdsDtcmHotStack missing in '$resolvedElf'."
    }
    $hotStackBytes = [int]$hotStack[0].Bytes
    if (($hotStackBytes -lt 4096) -or (($hotStackBytes % 32) -ne 0)) {
        throw ("DTCM hot stack is $hotStackBytes bytes (needs >= 4096 and a " +
            "multiple of 32 to keep __irq_table aligned) in '$resolvedElf'.")
    }
    # P2-2p8 hot scalar statics. 112 scattered 4-byte-ish statics moved into
    # DTCM by linker script alone, worth -43,200 WORK-H P50 confirmed per-PC
    # (artifacts/performance/2026-09-17_p2-2p8-dtcm-hot-scalars/).
    #
    # Their bytes are DERIVED, not a constant added to the expectation. The
    # linker script names each one between DTCM-RESIDENT HOT SCALARS markers,
    # and the size comes from the ELF's own symbol table, so this pin keeps its
    # whole strength: a DTCM symbol that is neither in the models above nor in
    # that block still breaks the layout assertion, and a symbol the script
    # claims but the ELF does not carry is reported here. Only the intended,
    # reviewed block is accounted for.
    $hotScalarNames = Get-Task20DTCMHotScalarNames -LinkerScript $LinkerScript
    $hotScalarBySection = @{ '.dtcm' = @(); '.dtcm.bss' = @() }
    foreach ($name in $hotScalarNames) {
        $owner = @($owners | Where-Object { $_.Name -eq $name })
        if ($owner.Count -eq 0) {
            # NOT a failure. Roster and feature gates mean a configuration need
            # not compile every one of these -- the shell build carries none of
            # the tick-HUD counters, so 7 of the 112 are simply absent there.
            # A symbol that does not exist has nothing to gather and adds no
            # bytes. A MISTYPED input section still leaves its symbol in the
            # ELF, in .main.bss, where scripts/check-dtcm-residency.py reports
            # it as stranded, so the guard against a silent gather is unharmed.
            continue
        }
        if (-not $hotScalarBySection.ContainsKey($owner[0].Section)) {
            throw ("DTCM hot scalar '$name' landed in section " +
                "'$($owner[0].Section)', not .dtcm or .dtcm.bss, in " +
                "'$resolvedElf'.")
        }
        $hotScalarBySection[$owner[0].Section] += $owner[0]
    }
    # SPAN, not the sum of the sizes. These are 1- and 2-byte statics among
    # 4-byte ones, so the linker pads between them and the sum is smaller than
    # the bytes the section actually grew -- 34 against 40, and 454 against 468,
    # in the arm that first measured this. The span is what the section sees.
    # It is also a contiguity assertion: the block is gathered last in each
    # output section, so anything landing inside its address range that is not
    # one of these names widens the span and breaks the layout check below.
    # Rounded to each block's own trailing ALIGN in the linker script. .dtcm
    # ends ALIGN(32) so that Calico's __irq_table keeps its 32-byte boundary --
    # .dtcm's total size is what positions .dtcm.bss, so a block appended there
    # with only ALIGN(4) drags the IRQ table off alignment, which is exactly
    # what this check caught the first time the lane was built. .dtcm.bss is
    # the section tail and ends ALIGN(4).
    $hotScalarTrailingAlign = @{ '.dtcm' = 32; '.dtcm.bss' = 4 }
    $hotScalarSpan = @{}
    foreach ($section in @('.dtcm', '.dtcm.bss')) {
        $rows = @($hotScalarBySection[$section])
        if ($rows.Count -eq 0) { $hotScalarSpan[$section] = 0; continue }
        $low = ($rows | Measure-Object -Property Address -Minimum).Minimum
        $high = ($rows | ForEach-Object { $_.Address + $_.Bytes } |
            Measure-Object -Maximum).Maximum
        $align = $hotScalarTrailingAlign[$section]
        $hotScalarSpan[$section] =
            [int]([math]::Ceiling(($high - $low) / [double]$align) * $align)
    }
    $hotScalarData = $hotScalarSpan['.dtcm']
    $hotScalarBss = $hotScalarSpan['.dtcm.bss']

    $dtcmOwnerRows = @($owners | Where-Object { $_.Section -eq '.dtcm' })
    $dtcmHigh = if ($dtcmOwnerRows.Count -eq 0) { $expectedBase } else {
        ($dtcmOwnerRows | ForEach-Object { $_.Address + $_.Bytes } |
            Measure-Object -Maximum).Maximum
    }
    $dtcmBytes = [int]([math]::Ceiling(($dtcmHigh - $expectedBase) / 32.0) * 32)
    # Everything in .dtcm is modelled below or a named hot scalar; the pin
    # loop throws on anything else, so the span cannot hide a stranger.
    $null = $hotScalarData
    # Native ShieldPose publishes a synchronous ARM9-only DObjDesc lookup:
    # 32 source rows * 44 bytes. The repo linker maps .sbss.shield_pose before
    # Calico's BSS. RefreshBaseRow writes transforms; guard evaluation
    # reads them on ARM9. This is neither DMA data nor ARM7/IPC shared storage.
    $shieldPoseOwners = @($owners | Where-Object {
        $_.Name -eq 'sNdsShieldPoseDObjScratch'
    })
    $shieldPoseBytes = if ($shieldPoseOwners.Count -eq 0) { 0 } else { 1408 }
    $dtcmBssBytes = $hotStackBytes + $shieldPoseBytes + $hotScalarBss + 152
    # NOT shifted by $hotScalarBss. The hot-scalar bss block is gathered LAST
    # in .dtcm.bss, after Calico's own BSS, so it grows the section tail and
    # leaves __irq_table where it was. Calico does move by $hotScalarData,
    # because that lands in .dtcm and pushes .dtcm.bss's base -- and that is
    # already carried in $dtcmBytes.
    $calicoBase = $expectedBase + $dtcmBytes + $hotStackBytes + $shieldPoseBytes

    if ($dtcm.Address -ne $expectedBase -or
        $dtcmBss.Address -ne ($expectedBase + $dtcmBytes) -or
        $dtcmStart -ne $expectedBase -or
        $dtcm.Bytes -ne $dtcmBytes -or
        $dtcmEnd -ne ($expectedBase + $dtcmBytes) -or
        $dtcmBss.Bytes -ne $dtcmBssBytes -or
        $dtcmBssEnd -ne ($expectedBase + $dtcmBytes + $dtcmBssBytes) -or
        $spUsr -ne ($expectedBase + 0x3e80) -or
        $spIrq -ne ($spUsr + 0x100) -or
        $spSvc -ne ($spIrq + 0x40) -or
        $irqFlags -ne ($spSvc + 0x38) -or
        $physicalEnd -ne ($spSvc + 0x40)) {
        throw "DTCM section or Calico user/IRQ/SVC/BIOS boundary changed in '$resolvedElf'."
    }

    $expectedOwners = @{
        '__irq_table' = [PSCustomObject]@{
            Address = $calicoBase
            Section = '.dtcm.bss'
            Bytes = 128
            Alignment = 32
        }
        '__sched_state' = [PSCustomObject]@{
            Address = $calicoBase + 128
            Section = '.dtcm.bss'
            Bytes = 24
            Alignment = 32
        }
    }
    $expectedOwners['gNdsDtcmHotStack'] = [PSCustomObject]@{
        Address = $expectedBase + $dtcmBytes
        Section = '.dtcm.bss'
        Bytes = $hotStackBytes
        Alignment = 8
    }
    if ($shieldPoseBytes -ne 0) {
        $expectedOwners['sNdsShieldPoseDObjScratch'] = [PSCustomObject]@{
            Address = $expectedBase + $dtcmBytes + $hotStackBytes
            Section = '.dtcm.bss'
            Bytes = 1408
            Alignment = 4
        }
    }
    $namedDtcm = @($dtcmNamedSizes.Keys) + $playbackOwnerNames
    foreach ($owner in @($owners | Where-Object { $namedDtcm -contains $_.Name })) {
        $expectedOwners[$owner.Name] = [PSCustomObject]@{
            Address = $owner.Address
            Section = '.dtcm'
            Bytes = $owner.Bytes
            Alignment = 4
        }
    }
    # The hot-scalar block is admitted BY NAME and excluded from the exact
    # address pin below. Pinning 112 individual addresses would be brittle
    # rather than protective -- every one of them shifts whenever anything
    # earlier in the section changes size -- and their placement inside the
    # block is the linker's business. What still holds for them: each must
    # exist, each must be in .dtcm or .dtcm.bss, and the span they occupy is
    # part of the section-size assertion above, so an unnamed object landing
    # among them widens that span and fails.
    #
    # Everything the pin covered before is unchanged. In particular an
    # application object that is in NEITHER set still fails as an unexpected
    # DTCM owner, which is the guard that matters.
    $hotScalarNameSet = @{}
    foreach ($name in $hotScalarNames) { $hotScalarNameSet[$name] = $true }
    $pinnedOwners = @($owners | Where-Object {
        -not $hotScalarNameSet.ContainsKey($_.Name)
    })
    if ($pinnedOwners.Count -ne $expectedOwners.Count) {
        throw ("Unexpected DTCM object count $($pinnedOwners.Count) " +
            "(plus $($hotScalarNames.Count) named hot scalars) in " +
            "'$resolvedElf'.")
    }
    foreach ($owner in $pinnedOwners) {
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
