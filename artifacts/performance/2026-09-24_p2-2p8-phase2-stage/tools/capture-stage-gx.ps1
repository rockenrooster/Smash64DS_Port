param([string]$Label = 'seg57-gx-oracle')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$art = Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'scripts/lib/melonds.ps1')
$context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot 9 -NoBuild
$build = Join-Path $root 'builds/build-p2p8-s7'
$rom = Join-Path $build 'smash64ds-p2-fourcpu-tickhud-hwtri.nds'
$elf = [IO.Path]::ChangeExtension($rom, '.elf')
$payload = [IO.File]::ReadAllBytes((Join-Path $build 'nitrofs/stages/dreamland.gxp'))
$words = [BitConverter]::ToUInt32($payload, 16)
$out = Join-Path $art $Label
if (Test-Path -LiteralPath "$out.gdb.out") { throw 'Use a new label to preserve the previous capture' }
$gdbOut = $out.Replace('\','/')
$commands = @(
    'set pagination off', 'set confirm off', 'set remotetimeout 30',
    "target remote 127.0.0.1:$($context.GdbPort)", 'break main', 'continue',
    'set var gNdsP2StageProg = 0', 'delete breakpoints',
    'break ndsBattlePlayableFrameCompleteMarker', 'ignore 2 31', 'continue',
    'printf "GX_OLD words=%u state=%u\n", sNdsRendererTask36ReplayOwner.word_count, sNdsRendererTask36ReplayOwner.state',
    ('dump binary memory {0}-old.bin &sNdsRendererTask36ReplayOwner.words[0] (&sNdsRendererTask36ReplayOwner.words[0]+sNdsRendererTask36ReplayOwner.word_count)' -f $gdbOut),
    'set $i = 0', 'while $i < 54',
    'printf "GX_RUN=%u,%u,%u,%u\n", $i, sNdsRendererTask36ReplayOwner.runs[$i].word_offset, sNdsRendererTask36ReplayOwner.runs[$i].word_count, sNdsRendererTask36ReplayOwner.runs[$i].prepared.textured',
    'set $i = $i + 1', 'end', 'set var gNdsP2StageProg = 1', 'continue',
    'printf "GX_NEW draws=%u declines=%u reason=%u gxstat=%u\n", gNdsP2StageProgDraws, gNdsP2StageProgDeclines, gNdsP2StageProgReason, *(unsigned int*)0x04000600',
    'if gNdsP2StageProgDraws == 0', 'quit 2', 'end',
    ('dump binary memory {0}-new.bin sNdsStageGxWords (sNdsStageGxWords+{1})' -f $gdbOut, $words),
    'detach', 'quit'
)
Set-Content -LiteralPath "$out.gdb" -Value $commands
$state = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
    -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$emulator = $null
$debugger = $null
try {
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList ('"{0}"' -f $rom) `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput "$out.melon.out" -RedirectStandardError "$out.melon.err"
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $debugger = Start-Process -FilePath 'C:/devkitPro/devkitARM/bin/arm-none-eabi-gdb.exe' `
        -ArgumentList ('-batch "{0}" -x "{1}.gdb"' -f $elf, $out) -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput "$out.gdb.out" -RedirectStandardError "$out.gdb.err"
    if (-not $debugger.WaitForExit(300000)) { throw 'Stage GX capture timed out' }
    if ($debugger.ExitCode -ne 0) { throw "Stage GX capture failed: $($debugger.ExitCode)" }
    if (-not (Test-Path -LiteralPath "$out-new.bin")) { throw 'Missing compiled GX capture' }
    Get-FileHash -LiteralPath $rom,"$out-old.bin","$out-new.bin" | ConvertTo-Json |
        Set-Content -LiteralPath "$out-identity.json"
    Select-String -LiteralPath "$out.gdb.out" -Pattern '^GX_OLD|^GX_NEW'
} finally {
    foreach ($process in @($debugger, $emulator)) {
        if ($null -ne $process -and -not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    }
    Restore-MelonDSGdbConfig -State $state
}
