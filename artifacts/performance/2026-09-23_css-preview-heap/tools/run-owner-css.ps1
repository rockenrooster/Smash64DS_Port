param(
    [Parameter(Mandatory = $true)][string]$Rom,
    [Parameter(Mandatory = $true)][string]$Elf,
    [Parameter(Mandatory = $true)][string]$Tag,
    [int]$RunnerSlot = 7
)
# The owner's own CSS route on a ROM WITHOUT the menu walk (the published
# configuration): builds/resume-20260905/css-owner-feed.gdb drives Title ->
# VS -> character select through a keysHeld feed, selects Link, Yoshi and
# Pikachu and screenshots each (artifacts/visibility/2026-09-06_css-*-<Tag>.png).
# css-reserve-extra.gdb adds the scene heap at the animation working-set
# reservation. The ROM must not be named like a file on the DLDI image
# (smash64ds.nds): pass a lab copy under its target's own name.
$ErrorActionPreference = 'Stop'
$root = 'D:/Stuff/DevFolder/Smash64DS_Port'
. "$root/scripts/lib/melonds.ps1"
. "$root/scripts/lib/gdb-markers.ps1"
$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$emulator = $null; $state = $null
$probeName = 'owner_css'
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort -Persistent -BreakOnStartup -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList @($Rom) -WindowStyle Hidden -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $commands = @('set pagination off', 'set confirm off', "target remote 127.0.0.1:$($context.GdbPort)", 'set $phase=0', 'set $tick=0', 'set $vs_visits=0', 'set $option_visits=0', 'set $items_tour=0', 'set $hide_ui=0')
    foreach ($entry in @(@('ndsMenuShellRunTitle', 1), @('ndsMenuShellRunModeSelect', 2), @('ndsMenuShellRunVSMode', 3), @('mnPlayersVSStartScene', 4), @('mnVSItemSwitchStartScene', 5))) {
        $commands += @("break *$($entry[0])", 'commands', 'silent', ('set $phase=' + $entry[1]), 'set $tick=0', 'printf "VSOPT_ENTRY phase=%u\n",$phase', 'continue', 'end')
        if ($entry[1] -eq 3) { $commands = $commands[0..($commands.Count - 3)] + @('set $vs_visits=$vs_visits+1', 'continue', 'end') }
        if ($entry[1] -eq 4) { $commands = $commands[0..($commands.Count - 3)] + @('set $option_visits=$option_visits+1', 'continue', 'end') }
    }
    $feed = Get-Content -Raw -LiteralPath "$root/builds/resume-20260905/css-owner-feed.gdb"
    $extra = Get-Content -Raw -LiteralPath "$root/artifacts/performance/2026-09-23_css-preview-heap/tools/css-reserve-extra.gdb"
    $feed = $extra + "`n" + $feed
    $commands += $feed.Replace('@ROOT@', $root).Replace('@PID@', "$($emulator.Id)").Replace('@TAG@', $Tag) -split "`r?`n"
    Invoke-GdbMarkerScript -Gdb 'C:/devkitPro/devkitARM/bin/arm-none-eabi-gdb.exe' -Elf $Elf -Root $root -Commands $commands -ScriptName "$probeName.gdb" -TimeoutSeconds 90 | Out-Null
} finally {
    $capture = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR "$probeName.gdb.out"
    if (Test-Path -LiteralPath $capture) {
        Get-Content -LiteralPath $capture | Where-Object { $_ -match '^(CSSRESERVE|CSS_OWNER|VSOPT_STOP)' }
    }
    if ($emulator) { Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue }
    if ($state) { Restore-MelonDSGdbConfig -State $state }
}
