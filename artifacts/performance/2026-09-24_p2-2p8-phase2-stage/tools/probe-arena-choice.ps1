param([Parameter(Mandatory)][string]$Build, [Parameter(Mandatory)][string]$Label)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$art = Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'scripts/lib/melonds.ps1')
. (Join-Path $root 'scripts/lib/gdb-markers.ps1')
$context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot 9 -NoBuild
$rom = Join-Path $root "builds/$Build/smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elf = [IO.Path]::ChangeExtension($rom, '.elf')
$out = Join-Path $art $Label
if (Test-Path "$out.txt") { throw 'Use a new label' }
$commands = @(
    'set pagination off', 'set confirm off', 'set remotetimeout 30',
    "target remote 127.0.0.1:$($context.GdbPort)",
    'break main', 'continue', 'delete breakpoints',
    'break _sbrk_r', 'commands', 'silent',
    'printf "SBRK bytes=%d caller=%08x\n", $r1, $lr', 'continue', 'end',
    'break ndsTaskmanArenaBytes', 'commands', 'silent',
    'printf "ARENA_ENTER start=%08x end=%08x chosen=%u\n", *(unsigned int*)&fake_heap_start, *(unsigned int*)&fake_heap_end, gNdsTaskmanArenaChosenSize',
    'disable 3 4', 'continue', 'end',
    'break _malloc_r if $r1 >= 1024', 'commands', 'silent',
    'printf "MALLOC bytes=%u caller=%08x\n", $r1, $lr', 'bt 4', 'continue', 'end',
    'break ndsRendererFinishNativeStageOwner', 'continue',
    'printf "ARENA_READY start=%08x end=%08x chosen=%u failed=%u refined=%u base=%08x top=%u\n", *(unsigned int*)&fake_heap_start, *(unsigned int*)&fake_heap_end, gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount, gNdsTaskmanArenaRefineBytes, sNdsTaskmanArenaBytes, gNdsTaskmanLibcTopChunkMin',
    'detach', 'quit'
)
$state = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
    -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$emulator = $null
try {
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList ('"{0}"' -f $rom) `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput "$out.melon.out" -RedirectStandardError "$out.melon.err"
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $result = Invoke-GdbMarkerScript -Gdb 'C:/devkitPro/devkitARM/bin/arm-none-eabi-gdb.exe' `
        -Elf $elf -Root $root -Commands $commands -ScriptName "$Label.gdb" -TimeoutSeconds 180
    $result.Stdout | Set-Content "$out.txt"
    $result.Stdout -split "`n" | Select-String '^ARENA_'
    Get-FileHash $rom, $elf | ConvertTo-Json | Set-Content "$out-identity.json"
} finally {
    if ($null -ne $emulator -and -not $emulator.HasExited) { $emulator.Kill(); $emulator.WaitForExit() }
    Restore-MelonDSGdbConfig -State $state
}
