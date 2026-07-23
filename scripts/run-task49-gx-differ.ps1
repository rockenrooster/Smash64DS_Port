<#
.SYNOPSIS
Task 49 GX equivalence differ capture harness.

.DESCRIPTION
Drives the NDS_TASK49_GX_DIFFER=1 lab ROM under repo-owned melonDS + GDB to
capture one owner's per-frame GX command stream. The recorder is armed before
the target window, the frame-complete marker is hit once per sample, and the
entry + word buffers and counters are dumped via `dump binary memory`. The
binary dumps are parsed into the JSON the analyzer
(scripts/analyze-task49-gx-differ.ps1) consumes.

One owner per run (the whole-frame capture does not fit; the stage owner alone
is ~2,557 entries / 6,894 words, Task 34's measured count). The 8-frame window
438-445 matches Task 48's contract.

Modeled on scripts/capture-fighter-animation-audit.ps1's GDB flow.
#>
[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task49-lab',
    [ValidateRange(0,2)][int]$Owner = 0,    # 0=STAGE, 1=MARIO, 2=FOX
    [ValidateRange(1,1000000)][int]$StartFrame = 438,
    [ValidateRange(1,64)][int]$Frames = 8,
    [ValidateRange(60,1800)][int]$TimeoutSeconds = 900,
    [string]$OutDir = '',
    [string]$Label = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-task49-differ-hwtri'

if ([string]::IsNullOrWhiteSpace($MelonDS)) {
    $MelonDS = Join-Path $root 'emulators\melonds\melonDS.exe'
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot -NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $root 'artifacts\performance'
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$tag = if ($Label) { $Label } else { "owner$Owner-{0}-{1}" -f $StartFrame, ($StartFrame + $Frames - 1) }
$json = Join-Path $OutDir ("2026-07-23_task49-differ-$tag.json")

$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbPort = $context.GdbPort
$emulatorOut = Join-Path $temp 'task49-differ.melonds.out'
$emulatorErr = Join-Path $temp 'task49-differ.melonds.err'
$gdbScript = Join-Path $temp 'task49-differ.gdb'
# Binary dump files (GDB writes these; we parse them).
$entriesBin = Join-Path $temp 'task49-differ-entries.bin'
$wordsBin = Join-Path $temp 'task49-differ-words.bin'

# Buffer sizes from include/nds/nds_task49_gx_differ.h.
$entryCapacity = 2800
$wordCapacity = 7000
$entryBytes = $entryCapacity * 8     # NDSRendererTask49GxDifferEntry = 8 bytes
$wordBytes = $wordCapacity * 4

# --- Build the GDB command sequence ---
# Arm the capture at StartFrame-1 (set owner + enabled), then break on the
# frame-complete marker for the LAST frame in the window and dump.
$lastFrame = $StartFrame + $Frames - 1
# Per-counter dump files: the symbols are NOT contiguous (the linker reorders
# them), so dump each named symbol individually rather than a contiguous span.
$ctrFiles = @{
    frame = (Join-Path $temp 't49-ctr-frame.bin')
    enabled = (Join-Path $temp 't49-ctr-enabled.bin')
    entryCount = (Join-Path $temp 't49-ctr-entrycount.bin')
    wordCount = (Join-Path $temp 't49-ctr-wordcount.bin')
    overflow = (Join-Path $temp 't49-ctr-overflow.bin')
    fault = (Join-Path $temp 't49-ctr-fault.bin')
    binding = (Join-Path $temp 't49-ctr-binding.bin')
}
$commands = @(
    'set pagination off',
    'set confirm off',
    'set remotetimeout 15',
    "target remote 127.0.0.1:$gdbPort",
    'break ndsBattlePlayableFrameCompleteMarker',
    'continue'   # first hit
)
for ($i = 2; $i -le ($StartFrame - 1); $i++) {
    $commands += 'continue'
}
$commands += @(
    "set variable gNdsTask49GxDifferSelectedOwner = $Owner",
    'set variable gNdsTask49GxDifferCaptureEnabled = 1',
    'continue'   # run the armed frame (StartFrame)
)
for ($i = ($StartFrame + 1); $i -le $lastFrame; $i++) {
    $commands += 'continue'
}
# Dump buffers + each counter symbol individually.
$commands += @(
    "dump binary memory $entriesBin &gNdsTask49GxDifferEntries[0] ((char *)&gNdsTask49GxDifferEntries[0] + $entryBytes)",
    "dump binary memory $wordsBin &gNdsTask49GxDifferWords[0] ((char *)&gNdsTask49GxDifferWords[0] + $wordBytes)",
    "dump binary memory $($ctrFiles.frame) &gNdsTask49GxDifferFrame ((char *)&gNdsTask49GxDifferFrame + 4)",
    "dump binary memory $($ctrFiles.enabled) &gNdsTask49GxDifferCaptureEnabled ((char *)&gNdsTask49GxDifferCaptureEnabled + 4)",
    "dump binary memory $($ctrFiles.entryCount) &gNdsTask49GxDifferEntryCount ((char *)&gNdsTask49GxDifferEntryCount + 4)",
    "dump binary memory $($ctrFiles.wordCount) &gNdsTask49GxDifferWordCount ((char *)&gNdsTask49GxDifferWordCount + 4)",
    "dump binary memory $($ctrFiles.overflow) &gNdsTask49GxDifferOverflowCount ((char *)&gNdsTask49GxDifferOverflowCount + 4)",
    "dump binary memory $($ctrFiles.fault) &gNdsTask49GxDifferFaultCount ((char *)&gNdsTask49GxDifferFaultCount + 4)",
    "dump binary memory $($ctrFiles.binding) &gNdsTask49GxDifferBindingCount ((char *)&gNdsTask49GxDifferBindingCount + 4)",
    'detach',
    'quit'
)
Set-Content -LiteralPath $gdbScript -Value ($commands -join "`n")

# --- Start melonDS, then GDB ---
Remove-Item $entriesBin, $wordsBin, $emulatorOut, $emulatorErr `
    -Force -ErrorAction SilentlyContinue
$configState = Enable-MelonDSGdbConfig `
    -MelonDSPath $context.MelonDSPath -GdbPort $gdbPort `
    -Arm7Port $context.Arm7Port `
    -Persistent:([bool]$context.PersistentConfig) -MuteAudio

$emulator = $null
$gdb = $null
try {
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru

    $gdbExe = Join-Path 'C:/devkitPro/devkitARM/bin' 'arm-none-eabi-gdb.exe'
    if (-not (Test-Path $gdbExe)) { $gdbExe = 'arm-none-eabi-gdb' }
    # Single argument string with proper quoting: Start-Process -ArgumentList
    # arrays mis-split '-ex set confirm off' into separate tokens.
    $gdbArgs = '-batch -ex "set confirm off" "{0}" -x "{1}"' -f $elf, $gdbScript
    $gdb = Start-Process -FilePath $gdbExe `
        -ArgumentList $gdbArgs `
        -RedirectStandardOutput (Join-Path $temp 'task49-differ.gdb.out') `
        -RedirectStandardError (Join-Path $temp 'task49-differ.gdb.err') `
        -WindowStyle Hidden -PassThru

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $gdb.Refresh()
        if ($gdb.HasExited) { break }
        Start-Sleep -Milliseconds 500
    }
    if (-not $gdb.HasExited) {
        try { $gdb.Kill() } catch {}
        throw "GDB did not finish within $TimeoutSeconds s."
    }
    $emulator.Refresh()
    if ($emulator.HasExited) {
        throw "melonDS exited before the capture completed (exit $($emulator.ExitCode))."
    }
} finally {
    if ($null -ne $gdb) { try { $gdb.Refresh(); if (-not $gdb.HasExited) { $gdb.Kill() } } catch {} }
    if ($null -ne $emulator) { try { $emulator.Refresh(); if (-not $emulator.HasExited) { $emulator.Kill() } } catch {} }
    Restore-MelonDSGdbConfig -State $configState
}

# --- Parse the binary dumps into JSON ---
function Read-U32File([string]$path) {
    if (-not (Test-Path $path)) { return 0 }
    $s = [IO.File]::OpenRead($path)
    try { return [BitConverter]::ToUInt32((New-Object IO.BinaryReader($s)).ReadBytes(4), 0) }
    finally { $s.Close() }
}
$cFrame = Read-U32File $ctrFiles.frame
$cEnabled = Read-U32File $ctrFiles.enabled
$cEntryCount = Read-U32File $ctrFiles.entryCount
$cWordCount = Read-U32File $ctrFiles.wordCount
$cOverflow = Read-U32File $ctrFiles.overflow
$cFault = Read-U32File $ctrFiles.fault
$cBinding = Read-U32File $ctrFiles.binding

$entries = @()
if ((Test-Path $entriesBin) -and $cEntryCount -le $entryCapacity) {
    $es = [IO.File]::OpenRead($entriesBin)
    $er = New-Object IO.BinaryReader($es)
    for ($i = 0; $i -lt $cEntryCount; $i++) {
        $wordOffset = [BitConverter]::ToUInt16($er.ReadBytes(2), 0)
        $class = $er.ReadByte()
        $wc = $er.ReadByte()
        $binding = [BitConverter]::ToUInt16($er.ReadBytes(2), 0)
        $eo = $er.ReadByte()
        $null = $er.ReadByte()  # reserved
        $entries += [pscustomobject]@{
            word_offset = $wordOffset
            command_class = $class
            word_count = $wc
            binding_index = $binding
            owner = $eo
        }
    }
    $er.Close(); $es.Close()
}

# Attach each entry's words by slicing the word buffer.
$wordValues = @()
if (Test-Path $wordsBin) {
    $ws = [IO.File]::OpenRead($wordsBin)
    $wr = New-Object IO.BinaryReader($ws)
    for ($i = 0; $i -lt $cWordCount; $i++) {
        $wordValues += [BitConverter]::ToUInt32($wr.ReadBytes(4), 0)
    }
    $wr.Close(); $ws.Close()
}
foreach ($e in $entries) {
    $slice = @($wordValues[$e.word_offset..([Math]::Min($e.word_offset + $e.word_count - 1, $wordValues.Count - 1))])
    $e | Add-Member -NotePropertyName words -NotePropertyValue $slice
}

$result = [ordered]@{
    meta = [ordered]@{
        frame = $cFrame
        owner = $Owner
        capture_enabled = $cEnabled
        entry_count = $cEntryCount
        word_count = $cWordCount
        overflow = $cOverflow
        fault = $cFault
        binding_count = $cBinding
        window_start = $StartFrame
        window_frames = $Frames
    }
    entries = $entries
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $json -Encoding utf8
Write-Output "wrote $json"
Write-Output ("meta: frame={0} owner={1} entries={2} words={3} overflow={4} fault={5} bindings={6}" -f `
    $cFrame, $cOwner, $cEntryCount, $cWordCount, $cOverflow, $cFault, $cBinding)
