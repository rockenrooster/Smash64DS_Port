param(
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$Label = 'r0',
    [int]$Admit = 2,
    [int]$Route = 0,
    [int]$Frames = 1990,
    [int]$TimeoutSeconds = 3000
)
# P2-2p8 Phase 1 slice 4: dump every distinct runtime-recorded fighter packet
# of one four-CPU stress match (runner slot 9 only).
#
# The dump point is the first instruction after TryReplay's DC flush of the
# packet words (found in the ELF by disassembly), so the words are read from
# memory after the cache has written them back: a GDB read sees memory, not
# the D-cache. At that instruction r6 holds the battle slot, r9 the word count
# and [sp+20] the words pointer (asserted from the disassembly below). A packet
# is dumped when its (key[0..4], word_count) differs from the last one dumped
# for its slot -- every structural variant the match records, once.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice4'
$out = Join-Path $art ("dumps\" + $Label)
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-ChildItem -LiteralPath $out -File | Remove-Item -Force

$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
$gdbExe = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'

# Locate the post-flush instruction in ndsFighterPacketTryReplay.
$dis = & $objdump -d --no-show-raw-insn $elf | Out-String
$m = [regex]::Match($dis, '(?s)<ndsFighterPacketTryReplay>:\r?\n(.*?)\r?\n\r?\n')
if (-not $m.Success) { throw 'TryReplay not found' }
$body = $m.Groups[1].Value -split '\r?\n'
$flushIdx = -1
for ($i = 0; $i -lt $body.Count; $i++) {
    if ($body[$i] -match 'bl\s+[0-9a-f]+ <armDCacheFlush>') { $flushIdx = $i; break }
}
if ($flushIdx -lt 0) { throw 'no armDCacheFlush call in TryReplay' }
# Assert the register contract: ldr r0, [sp, #20] and lsl r1, r9, #2 feed the call.
$pre = ($body[($flushIdx - 3)..($flushIdx - 1)] -join "`n")
if (($pre -notmatch 'ldr\s+r0, \[sp, #20\]') -or ($pre -notmatch 'lsl\s+r1, r9, #2')) {
    throw "TryReplay flush register contract changed:`n$pre"
}
$post = [regex]::Match($body[$flushIdx + 1], '^\s*([0-9a-f]+):').Groups[1].Value
if (-not $post) { throw 'post-flush address not found' }
Write-Output "post-flush break *0x$post"

$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' -RunnerSlot 9 -NoBuild
$port = $context.GdbPort
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot 9
$gdbScript = Join-Path $temp 'slice4-dump-packets.gdb'
$gdbOut = Join-Path $out 'gdb.out'
$fwd = ($out -replace '\\', '/')

$lines = @(
    'set pagination off',
    'set confirm off',
    'set print elements 0',
    'set remotetimeout 30',
    "target remote 127.0.0.1:$port",
    'break ndsBattlePlayableFrameCompleteMarker',
    'continue',
    "set variable gNdsFtrLeanAdmit = $Admit",
    "set variable gNdsFtrLeanRoute = $Route",
    'delete',
    'set $n = 0'
)
foreach ($s in 0..3) {
    $lines += @("set `$s${s}k0 = 0xffffffff", "set `$s${s}k1 = 0", "set `$s${s}k2 = 0",
        "set `$s${s}k3 = 0", "set `$s${s}k4 = 0", "set `$s${s}wc = 0")
}
$lines += @(
    "break *0x$post",
    'commands',
    'silent',
    'set $slot = $r6',
    'set $p = &sNdsFighterPackets[$slot]',
    'set $dump = 0'
)
foreach ($s in 0..3) {
    $lines += @(
        "if `$slot == $s",
        "if (`$p->key[0] != `$s${s}k0) || (`$p->key[1] != `$s${s}k1) || (`$p->key[2] != `$s${s}k2) || (`$p->key[3] != `$s${s}k3) || (`$p->key[4] != `$s${s}k4) || (`$r9 != `$s${s}wc)",
        "set `$s${s}k0 = `$p->key[0]",
        "set `$s${s}k1 = `$p->key[1]",
        "set `$s${s}k2 = `$p->key[2]",
        "set `$s${s}k3 = `$p->key[3]",
        "set `$s${s}k4 = `$p->key[4]",
        "set `$s${s}wc = `$r9",
        'set $dump = 1',
        'end',
        'end'
    )
}
$lines += @(
    'if $dump',
    'set $words = (unsigned int)$p->words',
    "eval `"dump binary memory $fwd/p%03d-s%d-struct.bin 0x%x 0x%x`", `$n, `$slot, `$p, (char *)`$p + sizeof(sNdsFighterPackets[0])",
    "eval `"dump binary memory $fwd/p%03d-s%d-words.bin 0x%x 0x%x`", `$n, `$slot, `$words, `$words + (`$r9 * 4)",
    'printf "PKT n=%d slot=%d frame=%u wc=%u pwc=%u words=0x%x key=%08x %08x %08x %08x %08x %08x roots=%u\n", $n, $slot, gNdsRendererProfileFrameCount, $r9, $p->word_count, $words,$p->key[0], $p->key[1], $p->key[2], $p->key[3], $p->key[4], $p->key[5], $p->root_count',
    'set $n = $n + 1',
    'end',
    'continue',
    'end',
    'break ndsBattlePlayableFrameCompleteMarker',
    "ignore `$bpnum $Frames",
    'commands',
    'silent',
    'printf "DONE n=%d frame=%u\n", $n, gNdsRendererProfileFrameCount',
    'detach',
    'quit',
    'end',
    'continue'
)
Set-Content -LiteralPath $gdbScript -Value ($lines -join "`n")
Copy-Item -LiteralPath $gdbScript -Destination (Join-Path $out 'dump.gdb') -Force

$configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath -GdbPort $port `
    -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$emulator = $null
$gdb = $null
$start = Get-Date
try {
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList "`"$rom`"" `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $port | Out-Null
    $gdbArgs = '-batch -ex "set confirm off" "{0}" -x "{1}"' -f $elf, $gdbScript
    $gdb = Start-Process -FilePath $gdbExe -ArgumentList $gdbArgs `
        -RedirectStandardOutput $gdbOut -RedirectStandardError (Join-Path $out 'gdb.err') `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $gdb.Refresh()
        if ($gdb.HasExited) { break }
        Start-Sleep -Milliseconds 500
    }
    if (-not $gdb.HasExited) { try { $gdb.Kill() } catch {}; throw "GDB did not finish within $TimeoutSeconds s" }
} finally {
    if ($null -ne $gdb) { try { $gdb.Refresh(); if (-not $gdb.HasExited) { $gdb.Kill() } } catch {} }
    if ($null -ne $emulator) { try { $emulator.Refresh(); if (-not $emulator.HasExited) { $emulator.Kill() } } catch {} }
    Restore-MelonDSGdbConfig -State $configState
}
$pk = @(Select-String -LiteralPath $gdbOut -Pattern '^PKT ').Count
$done = Select-String -LiteralPath $gdbOut -Pattern '^DONE ' | Select-Object -First 1
Write-Output ("dumped {0} packets; {1}; elapsed {2:N0} s" -f $pk, $done, ((Get-Date) - $start).TotalSeconds)
