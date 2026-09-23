param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$RunnerSlot = 7,
    [int]$TimeoutSeconds = 240,
    [string]$Out = ''
)
# Attach at boot with a breakpoint on calico's exception entry: r0-r12 and
# lr still hold the faulting context there (lr = faulting pc + 8 for a data
# abort, + 4 for a prefetch abort).
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
    $script = Join-Path $temp 'ifc-abort.gdb'
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break __excpt_entry',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'printf "STOP pc=0x%08x lr=0x%08x sp=0x%08x cpsr=0x%08x\n", $pc, $lr, $sp, $cpsr',
        'printf "REGS r0=0x%08x r1=0x%08x r2=0x%08x r3=0x%08x r4=0x%08x r5=0x%08x r6=0x%08x r7=0x%08x\n", $r0, $r1, $r2, $r3, $r4, $r5, $r6, $r7',
        'printf "REGS r8=0x%08x r9=0x%08x r10=0x%08x r11=0x%08x r12=0x%08x\n", $r8, $r9, $r10, $r11, $r12',
        'info symbol ($lr - 8)',
        'info symbol ($lr - 4)',
        'info line *($lr - 8)',
        'x/4i ($lr - 12)',
        'printf "IFC stage=%u count=%u slots=%u baked=%u\n", gNdsRelocIfCompactFailStage, gNdsRelocIfCompactCount, gNdsRelocIfCompactSlots, gNdsIFCommonEndBakedBytes',
        'detach', 'quit'
    ))
    $job = Start-Job -ScriptBlock { param($g, $s, $e) & $g -q -batch -x $s $e 2>&1 } -ArgumentList $gdb, $script, $elf
    if (-not (Wait-Job $job -Timeout $TimeoutSeconds)) { Stop-Job $job; 'TIMEOUT' }
    $result = Receive-Job $job
    if ($Out -ne '') { $result | Set-Content -LiteralPath $Out }
    $result | Select-Object -Last 40
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
