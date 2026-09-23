param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    [int]$RunnerSlot = 7,
    [int]$TimeoutSeconds = 120,
    [string]$Out = ''
)
# CSS heap budget on a MENU_WALK ROM: free taskman heap at the CSS animation
# working-set reservation, the reservation's verdict, and free heap when the
# walk leaves the character select (the scene's low-water: the general heap
# is a bump allocator between rewinds), with the tour's coverage masks.
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
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $free = '(unsigned)gSYTaskmanGeneralHeap.end - (unsigned)gSYTaskmanGeneralHeap.ptr'
    $script = Join-Path $temp 'css-heap.gdb'
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'tbreak ndsR2AnimCacheReserveCSSWorkingSet',
        'continue',
        "printf `"RESERVE_AT free=%u start=0x%x end=0x%x\n`", $free, (unsigned)gSYTaskmanGeneralHeap.start, (unsigned)gSYTaskmanGeneralHeap.end",
        'finish',
        "printf `"RESERVE_DONE free=%u reserved=%u fail=%u\n`", $free, gNdsR2AnimCacheArenaReservedBytes, gNdsR2AnimCacheArenaReserveFailCount",
        'tbreak ndsMNPlayersVSPreviewExit',
        'continue',
        "printf `"CSS_EXIT free=%u\n`", $free",
        'printf "TOUR kind=0x%03x drew=0x%03x\n", gNdsMenuShellCssWalkTourKindMask, gNdsMenuShellCssWalkTourDrewMask',
        'printf "ACQ load=%u fail=%u cap=%u\n", gNdsPlayersVSPreviewAcquireLoadCount, gNdsPlayersVSPreviewAcquireFailCount, gNdsPlayersVSPreviewResidentCapacityFailCount',
        'printf "FIGATREE size=%u\n", gFTManagerFigatreeHeapSize',
        'printf "SHARED used=%u\n", (unsigned)sNdsPlayersVSSharedResidentArena.ptr - (unsigned)sNdsPlayersVSSharedResidentArena.start',
        'printf "BLOCK0 kind=%d used=%u\n", sNdsPlayersVSResidentBlocks[0].fkind, (unsigned)sNdsPlayersVSResidentBlocks[0].arena.ptr - (unsigned)sNdsPlayersVSResidentBlocks[0].arena.start',
        'printf "BLOCK1 kind=%d used=%u\n", sNdsPlayersVSResidentBlocks[1].fkind, (unsigned)sNdsPlayersVSResidentBlocks[1].arena.ptr - (unsigned)sNdsPlayersVSResidentBlocks[1].arena.start',
        'printf "BLOCK2 kind=%d used=%u\n", sNdsPlayersVSResidentBlocks[2].fkind, (unsigned)sNdsPlayersVSResidentBlocks[2].arena.ptr - (unsigned)sNdsPlayersVSResidentBlocks[2].arena.start',
        'printf "BLOCK3 kind=%d used=%u\n", sNdsPlayersVSResidentBlocks[3].fkind, (unsigned)sNdsPlayersVSResidentBlocks[3].arena.ptr - (unsigned)sNdsPlayersVSResidentBlocks[3].arena.start',
        'printf "ANIM used=%u reserved=%u\n", gNdsR2AnimCacheArenaUsedBytes, gNdsR2AnimCacheArenaReservedBytes',
        'detach', 'quit'
    ))
    $job = Start-Job -ScriptBlock { param($g, $s, $e) & $g -q -batch -x $s $e 2>&1 } -ArgumentList $gdb, $script, $elf
    if (-not (Wait-Job $job -Timeout $TimeoutSeconds)) { Stop-Job $job; 'TIMEOUT' }
    $result = Receive-Job $job
    if ($Out -ne '') { $result | Set-Content -LiteralPath $Out }
    $result | Where-Object { $_ -match '^(RESERVE|CSS_EXIT|TOUR|ACQ|FIGATREE|SHARED|BLOCK|ANIM|Value returned)' }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
