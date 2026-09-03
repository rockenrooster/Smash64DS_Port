[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Build,
    [Parameter(Mandatory=$true)][string]$Target,
    # Symbols to trace, each hit prints TRACE <symbol> and continues. Optional
    # `symbol=N` stops after N hits of that symbol (the run ends there).
    [Parameter(Mandatory=$true)][string[]]$Symbols,
    # Optional integer globals to print at every trace stop, as one extra
    # `TRACE GLOBALS <name>=<value> ...` line. The point is to read a counter AT
    # a named seam rather than after a free run, which is the difference between
    # "the pool was full at battle setup" and "the pool was full at some point".
    # Statics resolve too -- they are in the ELF symbol table -- so a file-local
    # count is readable by name. Values are printed unsigned; a pointer or a
    # struct is not expressible here and the ELF check will pass it anyway, so
    # name scalars.
    [string[]]$Globals = @(),
    # Optional fixed DS addresses to print as u32 at every trace hit. Use this
    # for hardware/environment state with no ELF symbol (argv header, EXMEMCNT),
    # not as a general-purpose memory dumper.
    [string[]]$MemoryWords = @(),
    # Name one of $Symbols to hold every OTHER breakpoint disabled until that
    # one is first hit. Without this, tracing a per-frame function to find a
    # freeze deep in a run is impossible: `gcRunAll` and `gcDrawAll` hit on
    # every frame from boot, and thousands of gdb round trips at ~10 ms each
    # mean the guest never reaches the scene under investigation. Arm on
    # something that only happens there -- a stage's own process function --
    # and the flood costs nothing until it matters. The armer keeps tracing
    # itself, so its own hits still bracket the ones it enabled.
    [string]$ArmAfter = '',
    # When a `symbol=N` spec reaches its Nth hit, single-step this many guest
    # instructions and print where the CPU ended up, with a backtrace. This is
    # how you find a FREEZE rather than a crash: a hang never returns to gdb,
    # so `continue` blocks forever and the batch's own trailing stop is never
    # reached, which is why a plain trace can say a function was entered and
    # never say where inside it the CPU stayed. Stepping is bounded, so it
    # always terminates. Each step is a stub round trip -- budget roughly a
    # millisecond each -- and a tight loop is entered almost immediately, so a
    # few thousand is usually enough and 20,000 is already slow.
    [ValidateRange(0, 200000)][int]$StepOnLimit = 0,
    # Some failures happen before an ordinary post-launch GDB attach can arm a
    # breakpoint. Opt in to melonDS's ARM9 startup halt for those cases; keep
    # the default off so normal traces preserve the existing launch behavior.
    [switch]$BreakOnStartup,
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    # Wall-clock ceiling. A crash wander never returns to gdb, so the trace is
    # read back from whatever printed before the ceiling.
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 120,
    [string]$Artifact = ''
)

# Bisection-by-breakpoint for wander crashes (P2-3r3, 2026-08-23): melonDS
# raises no ARM exception when the CPU walks into zeroed RAM, so a crash in a
# long setup function cannot be caught at the fault. This prints a marker at
# every listed symbol entry and lets the run go until the ceiling; the last
# marker before silence names the region, and the list is refined from there.
# Every hit costs one gdb round trip (~5-10 ms), so list entry points, not
# leaves.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_trace-symbols.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.trace-symbols.stdout.log'
$stderr = Join-Path $log_dir 'melonds.trace-symbols.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$symbolTable = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$specs = @()
foreach ($entry in $Symbols) {
    $name = $entry
    $limit = 0
    if ($entry -match '^(.+)=(\d+)$') { $name = $Matches[1]; $limit = [int]$Matches[2] }
    if ($symbolTable -notcontains $name) {
        throw "Trace symbol '$name' absent from $elf."
    }
    $specs += @{ Name = $name; Limit = $limit }
}
$armIndex = 0
$armedIndices = @()
if (-not [string]::IsNullOrWhiteSpace($ArmAfter)) {
    for ($i = 0; $i -lt $specs.Count; $i++) {
        if ($specs[$i].Name -eq $ArmAfter) { $armIndex = $i + 1 }
    }
    if ($armIndex -eq 0) {
        throw ("-ArmAfter '$ArmAfter' is not one of -Symbols: " +
            (($specs | ForEach-Object { $_.Name }) -join ', '))
    }
    if ($specs.Count -lt 2) {
        throw '-ArmAfter needs at least one other symbol to arm.'
    }
    $armedIndices = @(1..$specs.Count | Where-Object { $_ -ne $armIndex })
}
$missingGlobals = @($Globals | Where-Object { $symbolTable -notcontains $_ })
if ($missingGlobals.Count -gt 0) {
    throw ("Trace globals absent from ${elf}: " + ($missingGlobals -join ', '))
}
$badMemoryWords = @($MemoryWords | Where-Object { $_ -notmatch '^0x[0-9a-fA-F]+$' })
if ($badMemoryWords.Count -gt 0) {
    throw ("Trace memory words must be literal hex addresses: " +
        ($badMemoryWords -join ', '))
}
$globalsPrintf = ''
if ($Globals.Count -gt 0) {
    $globalsPrintf = 'printf "TRACE GLOBALS ' +
        (($Globals | ForEach-Object { $_ + '=%u' }) -join ' ') + '\n", ' +
        ($Globals -join ', ')
}

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent `
        -BreakOnStartup:$BreakOnStartup -MuteAudio
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
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort)
    ))
    $index = 1
    foreach ($spec in $specs) {
        $commands.Add(('break {0}' -f $spec.Name))
        $commands.Add(('set $h{0} = 0' -f $index))
        $commands.Add(('commands {0}' -f $index))
        $commands.Add('silent')
        $commands.Add(('set $h{0} = $h{0} + 1' -f $index))
        $commands.Add(('printf "TRACE {0} hit=%d pc=0x%x lr=0x%x sp=0x%x\n", $h{1}, $pc, $lr, $sp' -f $spec.Name, $index))
        $commands.Add('printf "TRACE ARGS r0=0x%x r1=0x%x r2=0x%x r3=0x%x\n", $r0, $r1, $r2, $r3')
        if ($globalsPrintf -ne '') { $commands.Add($globalsPrintf) }
        foreach ($address in $MemoryWords) {
            $commands.Add(('printf "TRACE MEM {0}=%#x\n", *(unsigned int*){0}' -f $address))
        }
        if (($armIndex -ne 0) -and ($index -eq $armIndex)) {
            $commands.Add(('enable {0}' -f ($armedIndices -join ' ')))
            $commands.Add('printf "TRACE ARMED\n"')
        }
        if ($spec.Limit -gt 0) {
            $commands.Add(('if $h{0} < {1}' -f $index, $spec.Limit))
            $commands.Add('continue')
            if ($StepOnLimit -gt 0) {
                $commands.Add('else')
                $commands.Add(('stepi {0}' -f $StepOnLimit))
                $commands.Add('printf "TRACE FROZEN pc=0x%x lr=0x%x sp=0x%x\n", $pc, $lr, $sp')
                $commands.Add('bt 12')
            }
            $commands.Add('end')
        } else {
            $commands.Add('continue')
        }
        $commands.Add('end')
        $index++
    }
    if ($armIndex -ne 0) {
        # Emitted after every `break`, because gdb cannot disable a breakpoint
        # that does not exist yet.
        $commands.Add(('disable {0}' -f ($armedIndices -join ' ')))
    }
    $commands.Add('continue')
    $commands.Add('printf "TRACE STOP pc=0x%x lr=0x%x\n", $pc, $lr')
    $commands.Add('bt 12')
    $commands.AddRange([string[]]@('detach', 'quit'))

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
            -ScriptName 'trace_symbols_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    } catch {
        Write-Output ("trace ended: " + $_.Exception.Message.Split("`n")[0])
    }
}
finally {
    $captured = Join-Path $log_temp 'trace_symbols_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        # `Breakpoint N at ...` lines are printed too, and that is not cosmetic.
        # This probe nm-verifies that a symbol EXISTS and then breaks on it by
        # NAME, which is a debug-info resolution: gdb can answer with
        # `Breakpoint 5 at 0x0: <name>. (6 locations)` when the body was inlined
        # into several callers. A zero hit count on such a breakpoint is not
        # evidence of absence, and with the summary filtered to TRACE lines
        # there was nothing on screen to say so (P2-3r10, 2026-08-25, where
        # ftDonkeyThrowFWaitSetStatus resolved exactly that way). Read the
        # resolution before you read the counts; prefer a symbol with one
        # location -- an out-of-line wrapper or a status callback -- as the
        # indicator for a transition you intend to prove absent.
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(TRACE|#|Breakpoint )' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        # A forced kill can skip melonDS's SD flush and corrupt the shared DLDI
        # image. Ask the emulator to close first, matching the newer long-run
        # probes; force is only the bounded fallback.
        try { $emulator.CloseMainWindow() | Out-Null } catch { }
        if (-not $emulator.WaitForExit(4000)) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
