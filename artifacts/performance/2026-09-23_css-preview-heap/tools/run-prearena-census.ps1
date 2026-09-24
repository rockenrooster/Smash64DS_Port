param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    [int]$RunnerSlot = 7,
    [int]$MinBytes = 2048,
    [int]$TimeoutSeconds = 180,
    [string]$Out = ''
)
# Who holds the libc heap below the taskman arena? Every newlib allocation of
# at least MinBytes made before the arena chooser first runs is logged with its
# size and caller (lr), then the chooser's first call prints the heap state.
# The arena takes the largest block libc can still give, so every byte listed
# here is a byte the taskman arena (and the character select's margin) lacks.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') `
    -RunnerSlot $RunnerSlot -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) `
        -BreakOnStartup -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $script = Join-Path $temp 'prearena.gdb'
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        "break *malloc if `$r0 >= $MinBytes",
        'commands', 'silent',
        'printf "ALLOC malloc size=%u lr=0x%08x\n", $r0, $lr',
        'info symbol $lr',
        'continue', 'end',
        "break *calloc if (`$r0 * `$r1) >= $MinBytes",
        'commands', 'silent',
        'printf "ALLOC calloc size=%u lr=0x%08x\n", $r0 * $r1, $lr',
        'info symbol $lr',
        'continue', 'end',
        "break *aligned_alloc if `$r1 >= $MinBytes",
        'commands', 'silent',
        'printf "ALLOC aligned_alloc size=%u align=%u lr=0x%08x\n", $r1, $r0, $lr',
        'info symbol $lr',
        'continue', 'end',
        "break *realloc if `$r1 >= $MinBytes",
        'commands', 'silent',
        'printf "ALLOC realloc size=%u lr=0x%08x\n", $r1, $lr',
        'info symbol $lr',
        'continue', 'end',
        'tbreak ndsTaskmanArenaBytes',
        'continue',
        'printf "ARENA_PROBE sbrk_base=0x%08x fake_start=0x%08x fake_end=0x%08x top=0x%08x maxsbrk=%u\n", (unsigned)__malloc_sbrk_base, (unsigned)fake_heap_start, (unsigned)fake_heap_end, ((unsigned *)&__malloc_av_)[2], __malloc_max_sbrked_mem',
        'info symbol $lr',
        'bt 6',
        'detach', 'quit'
    ))
    $job = Start-Job -ScriptBlock { param($g, $s, $e) & $g -q -batch -x $s $e 2>&1 } -ArgumentList $gdb, $script, $elf
    if (-not (Wait-Job $job -Timeout $TimeoutSeconds)) { Stop-Job $job; 'TIMEOUT' }
    $result = Receive-Job $job
    if ($Out -ne '') { $result | Set-Content -LiteralPath $Out }
    $result | Where-Object { $_ -match '^(ALLOC|ARENA_PROBE)|in section|^#' }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
