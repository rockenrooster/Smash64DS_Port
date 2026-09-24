param([string]$Label = 'seg57-gx-oracle', [switch]$Matrices, [int]$Frame = 0, [switch]$RamCounts)
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
# Historical recorder comparisons are preserved with their old ELF/scripts.
if (-not $Matrices -and -not $RamCounts) { $Matrices = $true }
$commands = @()
if ($Matrices) {
    $stop = if ($Frame -gt 0) { "gNdsBattlePlayablePacingPresentedFrames >= $($Frame-1)" } else {
        'gSCManagerBattleState != 0 && gSCManagerBattleState->time_remain > 0 && gSCManagerBattleState->time_remain <= 3300'
    }
    $commands = @(
        'set pagination off', 'set confirm off', 'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)", 'break main', 'continue',
        'delete breakpoints',
        "break ndsRendererFinishNativeStageOwner if $stop",
        'continue',
        'printf "GX_MATRIX tic=%u presented=%u draws=%u near=%u declines=%u gxstat=%u\n", gSCManagerBattleState->time_remain, gNdsBattlePlayablePacingPresentedFrames, gNdsP2StageProgDraws, gNdsP2StageProgNearRuns, gNdsP2StageProgDeclines, *(unsigned int*)0x04000600',
        'if gNdsP2StageProgDraws == 0 || sNdsNativeStageOwnerExecution.binding_composed == 0 || sNdsNativeStageOwnerExecution.active == 0', 'quit 2', 'end',
        ('dump binary memory {0}-new.bin sNdsStageGxWords (sNdsStageGxWords+{1})' -f $gdbOut, $words),
        ('dump binary memory {0}-composed.bin sNdsNativeStageOwnerExecution.binding_composed (sNdsNativeStageOwnerExecution.binding_composed+42)' -f $gdbOut),
        ('dump binary memory {0}-rigid.bin &sNdsNativeStageOwnerExecution.rigid_binding_mask ((char*)&sNdsNativeStageOwnerExecution.rigid_binding_mask+8)' -f $gdbOut),
        ('dump binary memory {0}-hidden.bin &sNdsNativeStageOwnerExecution.hidden_binding_mask ((char*)&sNdsNativeStageOwnerExecution.hidden_binding_mask+8)' -f $gdbOut),
        'detach', 'quit'
    )
}
if ($RamCounts) {
    $matchDigests = '(sNdsReplayDigest[0] == 492430630 && sNdsReplayDigest[1] == 3853537372) || (sNdsReplayDigest[0] == 731477600 && sNdsReplayDigest[1] == 2288996805)'
    $commands = @(
        'set pagination off', 'set confirm off', 'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)", 'break main', 'continue',
        'delete breakpoints'
    )
    if ($Matrices) {
        $commands += @("break ndsRendererFinishNativeStageOwner if $matchDigests", 'commands', 'silent')
        foreach ($case in @(@('a', '492430630'), @('b', '731477600'))) {
            $prefix = "$gdbOut-$($case[0])"
            $commands += @(
                "if sNdsReplayDigest[0] == $($case[1])",
                ('dump binary memory {0}-new.bin sNdsStageGxWords (sNdsStageGxWords+{1})' -f $prefix, $words),
                ('dump binary memory {0}-composed.bin sNdsNativeStageOwnerExecution.binding_composed (sNdsNativeStageOwnerExecution.binding_composed+42)' -f $prefix),
                ('dump binary memory {0}-rigid.bin &sNdsNativeStageOwnerExecution.rigid_binding_mask ((char*)&sNdsNativeStageOwnerExecution.rigid_binding_mask+8)' -f $prefix),
                ('dump binary memory {0}-hidden.bin &sNdsNativeStageOwnerExecution.hidden_binding_mask ((char*)&sNdsNativeStageOwnerExecution.hidden_binding_mask+8)' -f $prefix),
                'end'
            )
        }
        $commands += @('continue', 'end')
    }
    $countSite = @(Select-String -LiteralPath (Join-Path $root 'src/nds/nds_platform.c') `
        -SimpleMatch 'gNdsHardwareRendererPolyRamCount = GFX_POLYGON_RAM_USAGE;')
    if ($countSite.Count -ne 1) { throw 'Hardware-count stop must resolve uniquely' }
    $commands += @(
        "break nds_platform.c:$($countSite[0].LineNumber) if $matchDigests",
        'commands', 'silent',
        'set $polys = *(unsigned short*)0x04000604',
        'set $verts = *(unsigned short*)0x04000606',
        'set $dma = *(unsigned int*)0x040000b8',
        'set $gx = *(unsigned int*)0x04000600',
        'printf "GX_RAM frame=%u digest=%u/%u early=%u/%u after_status=%u/%u dma=%u gx=%u\n", gNdsBattlePlayablePacingPresentedFrames+1, sNdsReplayDigest[0], sNdsReplayDigest[1], $polys, $verts, *(unsigned short*)0x04000604, *(unsigned short*)0x04000606, $dma, $gx',
        'if sNdsReplayDigest[0] == 731477600', 'detach', 'quit', 'end',
        'continue', 'end', 'continue'
    )
}
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
    if (-not $RamCounts -and -not (Test-Path -LiteralPath "$out-new.bin")) { throw 'Missing compiled GX capture' }
    $captured = if ($RamCounts -and $Matrices) {
        foreach ($case in 'a','b') { foreach ($part in 'new','composed','rigid','hidden') { "$out-$case-$part.bin" } }
    } elseif ($RamCounts) { @() } elseif ($Matrices) { @("$out-new.bin", "$out-composed.bin", "$out-rigid.bin", "$out-hidden.bin") } else { @("$out-new.bin") }
    Get-FileHash -LiteralPath (@($rom) + $captured) | ConvertTo-Json |
        Set-Content -LiteralPath "$out-identity.json"
    Select-String -LiteralPath "$out.gdb.out" -Pattern '^GX_MATRIX|^GX_RAM'
} finally {
    foreach ($process in @($debugger, $emulator)) {
        if ($null -ne $process -and -not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    }
    Restore-MelonDSGdbConfig -State $state
}
