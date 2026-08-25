[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Build,
    [Parameter(Mandatory = $true)][string]$Target,
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 900)][int]$TimeoutSeconds = 180,
    [string]$Artifact = ''
)

# THE SILENT-FREEZE IDENTIFIER.
#
# `src/import/battleship_sys_malloc.c` replaced decomp malloc.c's anonymous
# `while (TRUE);` with a named halt that first PUBLISHES the arena id, the
# request, the alignment, the headroom and the caller LR. Nothing read those
# globals automatically, so every arena exhaustion still presented as "the ROM
# boots and then nothing happens" and had to be re-bisected by hand -- three
# times now (2026-07-29 twice, 2026-08-24 board row P2-3r11).
#
# This is the one run that ends that: break on `ndsSyMallocOverflowHalt`, read
# what the halt already recorded, and print a backtrace through the allocating
# caller. A ROM that does NOT hit the halt within the ceiling reaches the end
# of the capture with `overflow=0`, which is itself the answer -- the freeze is
# something else and this seam is exonerated.
#
# It is deliberately configuration-agnostic: any target whose ELF carries the
# halt symbol can be its patient.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_arena-overflow.txt')
}

$required = @(
    'ndsSyMallocOverflowHalt',
    'gNdsSyMallocOverflowCount',
    'gNdsSyMallocOverflowArenaID',
    'gNdsSyMallocOverflowRequest',
    'gNdsSyMallocOverflowAlignment',
    'gNdsSyMallocOverflowHeadroom',
    'gNdsSyMallocOverflowCallerLR',
    'gNdsTaskmanArenaChosenSize',
    'gNdsTaskmanArenaAllocFailCount',
    'gNdsTaskmanHeapGeneration',
    'gSYTaskmanGeneralHeap'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw "arena-overflow probe symbols absent from ${elf}: $($missing -join ', ')"
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.arena-overflow.stdout.log'
$stderr = Join-Path $log_dir 'melonds.arena-overflow.stderr.log'
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

    $commands = [System.Collections.Generic.List[string]]::new()
    $commands.AddRange([string[]]@(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break ndsSyMallocOverflowHalt',
        'continue',
        ('printf "ARENA HALT count=%u arena_id=%u request=%u align=%u ' +
         'headroom=%u caller_lr=0x%08x\n", gNdsSyMallocOverflowCount, ' +
         'gNdsSyMallocOverflowArenaID, gNdsSyMallocOverflowRequest, ' +
         'gNdsSyMallocOverflowAlignment, gNdsSyMallocOverflowHeadroom, ' +
         'gNdsSyMallocOverflowCallerLR'),
        ('printf "ARENA STATE chosen=%u allocfail=%u heapgen=%u ' +
         'heap_id=%u heap_start=0x%08x heap_ptr=0x%08x heap_end=0x%08x\n", ' +
         'gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount, ' +
         'gNdsTaskmanHeapGeneration, gSYTaskmanGeneralHeap.id, ' +
         '(unsigned)gSYTaskmanGeneralHeap.start, ' +
         '(unsigned)gSYTaskmanGeneralHeap.ptr, ' +
         '(unsigned)gSYTaskmanGeneralHeap.end'),
        'info symbol gNdsSyMallocOverflowCallerLR',
        'bt 16',
        'detach',
        'quit'
    ))

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
            -ScriptName 'arena_overflow_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    } catch {
        Write-Output ('arena-overflow capture ended: ' +
            $_.Exception.Message.Split("`n")[0])
    }
}
finally {
    $captured = Join-Path $log_temp 'arena_overflow_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force `
            -Path (Split-Path -Parent $Artifact) | Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(ARENA |#\d|\S+ in section)' }
        Write-Output ("probe capture: " + $Artifact)
    }
    if ($null -ne $emulator) {
        # Ask first, kill second. A forced kill skips melonDS's own SD flush and
        # can leave emulators/melonds/dldi.bin corrupt, which then reads as a
        # harness TIMEOUT rather than a crash (2026-08-14, board row P2-3r12).
        try { $emulator.CloseMainWindow() | Out-Null } catch { }
        if (-not $emulator.WaitForExit(4000)) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state | Out-Null
    }
}
