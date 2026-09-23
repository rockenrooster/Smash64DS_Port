param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$RunnerSlot = 7,
    [int]$DelaySeconds = 90,
    [string]$Out = ''
)
# Let the ROM run unattended, then attach once: backtrace, registers and the
# compaction / heap-halt counters. For a load that never reaches a frame.
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
    Start-Sleep -Seconds $DelaySeconds
    $script = Join-Path $temp 'ifc-late.gdb'
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'bt 16',
        'info registers pc lr sp cpsr',
        'printf "IFC stage=%u count=%u move=%u slots=%u src=%u bytes=%u nulled=%u baked=%u bakefail=%u\n", gNdsRelocIfCompactFailStage, gNdsRelocIfCompactCount, gNdsRelocIfCompactMoveCount, gNdsRelocIfCompactSlots, gNdsRelocIfCompactSourceBytes, gNdsRelocIfCompactBytes, gNdsRelocIfCompactNulled, gNdsIFCommonEndBakedBytes, gNdsIFCommonEndBakedFailCount',
        'printf "HEAP ptr=0x%08x end=0x%08x start=0x%08x halt=%u req=%u headroom=%u callerlr=0x%08x\n", (unsigned)gSYTaskmanGeneralHeap.ptr, (unsigned)gSYTaskmanGeneralHeap.end, (unsigned)gSYTaskmanGeneralHeap.start, gNdsSyMallocOverflowCount, gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, gNdsSyMallocOverflowCallerLR',
        'info symbol gNdsSyMallocOverflowCallerLR',
        'printf "FRAME presented=%u\n", gNdsRendererProfileFrameCount',
        'detach', 'quit'
    ))
    $result = & $gdb -q -batch -x $script $elf 2>&1
    if ($Out -ne '') { $result | Set-Content -LiteralPath $Out }
    $result | Select-Object -Last 40
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
