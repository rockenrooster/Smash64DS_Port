[CmdletBinding()]
param(
    [string]$Build = 'build-c132-stress5',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 1500,
    [ValidateRange(1, 32)][int]$Hits = 6,
    [string]$Artifact = ''
)

# WHO hands the DObj joint parser a bad script? (KNOWN_ISSUES "Something hands
# the joint parser a misaligned animation script"; HANDOFF's open
# gNdsObjAnimRunawayCount row.)
#
# The counter records the SCRIPT and the OPCODE and nothing else, so five
# reports across three cycles have all ended at "mask 1, opcode 100, address
# N". This probe reads the one thing the counter cannot: the DObj the fault
# happened on, and through it the GObj and its func_run -- i.e. the owner.
#
# NO REBUILD. The fault block is already compiled into the ROM; it is just not
# instrumented enough. Everything below is a register read at that block plus
# pointer-derefs from it.
#
# THE BREAKPOINT IS AN INTERIOR ADDRESS, so it is DERIVED, not typed in, and
# the derivation is checked. In gcParseDObjAnimJoint the two fault blocks are
# the only code that touches gNdsObjAnimRunawayCount, and each opens with the
# `movs r1, #MASK` that supplies its own mask bit:
#
#   2068100:  2101       movs r1, #1          <- mask bit 0, unknown opcode
#   2068102:  4aa3       ldr  r2, [pc, #652]  @ &gNdsObjAnimRunawayCount
#   2068104:  6813       ldr  r3, [r2, #0]
#   2068106:  3301       adds r3, #1
#   2068108:  6013       str  r3, [r2, #0]
#   206810a:  4aa2       ldr  r2, [pc, #648]  @ &gNdsObjAnimRunawayMask
#   ...
#   2068112:  4ba1       ldr  r3, [pc, #644]  @ &gNdsObjAnimRunawayScript
#   2068114:  601c       str  r4, [r3, #0]    <- r4 IS dobj->anim_joint.event32
#   2068116:  4ba1       ldr  r3, [pc, #644]  @ &gNdsObjAnimRunawayOpcode
#   2068118:  601e       str  r6, [r3, #0]    <- r6 IS command_kind
#   206811a:  4b9a       ldr  r3, [pc, #616]  @ 0xff7fffff = AOBJ_ANIM_NULL
#   206811c:  676b       str  r3, [r5, #116]  <- r5 IS dobj (+0x74 = anim_wait)
#
# So r4/r5/r6 are live at the block's FIRST instruction and the store trail
# proves which is which. r5 is still validated independently at the hit --
# dobj->anim_joint.event32 must equal r4 -- because a register claim read off a
# disassembly is exactly the kind of thing that has been wrong here before.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_objanim-runaway.txt')
}

$required = @(
    'gcParseDObjAnimJoint',
    'gNdsObjAnimRunawayCount',
    'gNdsObjAnimRunawayMask',
    'gNdsObjAnimRunawayScript',
    'gNdsObjAnimRunawayOpcode',
    'sNdsRelocLoadedFiles',
    'sNdsRelocLoadedFileCount'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Runaway probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

function Get-SymbolAddress {
    param([string[]]$Lines, [string]$Name)
    foreach ($line in $Lines) {
        $parts = $line -split '\s+'
        if ($parts.Count -ge 3 -and $parts[-1] -eq $Name) {
            return [uint32]('0x' + $parts[0])
        }
    }
    throw "symbol $Name not found"
}

$parse_addr = Get-SymbolAddress -Lines $nm_lines -Name 'gcParseDObjAnimJoint'
$count_addr = Get-SymbolAddress -Lines $nm_lines -Name 'gNdsObjAnimRunawayCount'

# Derive the two fault blocks. objdump prints a literal-pool load as
#   ldr r2, [pc, #652]  @ (2068390 <gcParseDObjAnimJoint+0x318>)
# and the pool word itself on its own line, so the pool entry that holds
# &gNdsObjAnimRunawayCount names the loads that reach the counter.
$stop_addr = $parse_addr + 0x1000
$asm = & $objdump -d ('--start-address=0x{0:x}' -f $parse_addr) `
    ('--stop-address=0x{0:x}' -f $stop_addr) $elf
$pool = @{}
$code = @()
foreach ($line in $asm) {
    if ($line -match '^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+\.word\s+0x([0-9a-f]+)') {
        $pool[[uint32]('0x' + $matches[1])] = [uint32]('0x' + $matches[3])
        continue
    }
    if ($line -match '^\s*([0-9a-f]+):\s+[0-9a-f ]+\t(.*)$') {
        $code += [pscustomobject]@{
            Addr = [uint32]('0x' + $matches[1])
            Text = $matches[2].Trim()
        }
    }
}
$fault_sites = @()
for ($i = 1; $i -lt $code.Count; $i++) {
    if ($code[$i].Text -notmatch 'ldr\s+r2,\s*\[pc.*\(([0-9a-f]+)\s*<') { continue }
    $slot = [uint32]('0x' + $matches[1])
    if (-not $pool.ContainsKey($slot)) { continue }
    if ($pool[$slot] -ne $count_addr) { continue }
    if ($code[$i - 1].Text -notmatch '^movs\s+r1,\s*#(\d+)') { continue }
    $fault_sites += [pscustomobject]@{
        Addr = $code[$i - 1].Addr
        Mask = [int]$matches[1]
    }
}
$site = @($fault_sites | Where-Object { $_.Mask -eq 1 })
if ($site.Count -ne 1) {
    throw ("Expected exactly one mask-bit-0 fault block in gcParseDObjAnimJoint, found {0}" -f $site.Count)
}
$break_addr = $site[0].Addr
Write-Output ("fault block (mask 1, unknown opcode) at 0x{0:x8}" -f $break_addr)

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.objanim-runaway.stdout.log'
$stderr = Join-Path $log_dir 'melonds.objanim-runaway.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        'set $dumped = 0',
        # The disassembly goes into the capture so the reader can check the
        # register claims against the instructions rather than against a
        # comment.
        ('printf "RWSITE addr=0x%08x\n", ' + ('0x{0:x}' -f $break_addr)),
        ('x/14i ' + ('0x{0:x}' -f $break_addr)),
        ('break *' + ('0x{0:x}' -f $break_addr)),
        'commands',
        'silent',
        'set $n = $n + 1',
        'set $scr = (unsigned int)$r4',
        'set $dob = (DObj *)$r5',
        'set $opc = (unsigned int)$r6',
        # Independent validation of the r5 claim: the DObj the fault is about
        # must be the one whose joint script is the faulting pointer.
        'set $ok = ((unsigned int)$dob->anim_joint.event32 == $scr)',
        'printf "RW hit=%d count=%u script=0x%08x align=%u opcode=%u dobj=%p r5ok=%d wait=%f frame=%f speed=%f root=%d\n", $n, gNdsObjAnimRunawayCount, $scr, $scr & 3, $opc, $dob, $ok, $dob->anim_wait, $dob->anim_frame, $dob->anim_speed, $dob->is_anim_root',
        'set $gob = $dob->parent_gobj',
        'printf "  RWG gobj=%p id=%u link=%u objkind=%u gframe=%f flags=0x%08x\n", $gob, ($gob != 0) ? $gob->id : 0, ($gob != 0) ? $gob->link_id : 0, ($gob != 0) ? $gob->obj_kind : 0, ($gob != 0) ? $gob->anim_frame : -1, ($gob != 0) ? $gob->flags : 0',
        'if $gob != 0',
        'output $gob->func_run',
        'printf "  <- RWRUN\n"',
        'output $gob->func_anim',
        'printf "  <- RWANIM\n"',
        'end',
        # The words either side of the faulting pointer, read ALIGNED. If the
        # pointer is 2 mod 4 the parser read a rotation of the word at
        # (script & ~3), and the neighbourhood says whether that word is script
        # data at all.
        'set $al = (unsigned int *)($scr & ~3)',
        'printf "  RWMEM aligned=%p [-2]=%08x [-1]=%08x [0]=%08x [+1]=%08x [+2]=%08x [+3]=%08x\n", $al, $al[-2], $al[-1], $al[0], $al[1], $al[2], $al[3]',
        # WHO called the 32-bit parser. The counter records the data and never
        # the caller, which is why three cycles of reports all stopped at "mask
        # 1, opcode 100": the owning seam is a CALL SITE, and it is one `bt`
        # away. Printed before the file table so a run that dies mid-dump still
        # carries it.
        'printf "  RWBT\n"',
        'bt 5',
        # The loaded-file table names the OWNER of that address. Dumped once,
        # on the first hit, so a run that ends at its timeout still carries it.
        'if $dumped == 0',
        'set $dumped = 1',
        'printf "  RWFILES count=%u\n", sNdsRelocLoadedFileCount',
        'set $i = 0',
        'while $i < sNdsRelocLoadedFileCount',
        'set $f = &sNdsRelocLoadedFiles[$i]',
        'set $lo = (unsigned int)$f->data',
        'set $hi = $lo + $f->data_size',
        'printf "  RWF %3u asset=%u data=0x%08x size=%u scene=%u gen=%u hit=%d\n", $i, $f->asset_id, $lo, $f->data_size, $f->owner_scene, $f->owner_generation, (($scr >= $lo) && ($scr < $hi)) ? 1 : 0',
        'set $i = $i + 1',
        'end',
        'end',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        'continue',
        'printf "RWDONE n=%d count=%u mask=%u script=0x%08x opcode=%u\n", $n, gNdsObjAnimRunawayCount, gNdsObjAnimRunawayMask, gNdsObjAnimRunawayScript, gNdsObjAnimRunawayOpcode',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'objanim_runaway_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the capture file, not the helper's return value: this probe is
    # WAITING for a rare fault, so a timeout is an expected exit and it holds
    # every hit that did occur.
    $captured = Join-Path $log_temp 'objanim_runaway_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match 'RW' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
