param(
    [string]$Build = 'walk-u16',
    [int]$Slot = 11,
    [string]$Tag = 'g1',
    [int]$Seconds = 120,
    # gdb lines run right after main (pokes)
    [string[]]$Pokes = @(),
    # gdb lines after the pokes (breakpoints with commands ... end)
    [string[]]$Lines = @(),
    # a window capture every $Every seconds (0: none)
    [int]$Every = 0,
    [string]$Rom = 'smash64ds',
    [string]$WaitFile = '', [string]$Wait = '',
    # A private, writable SD image (the shared one is read-only for runners),
    # so the save and its siblings persist; -Fresh deletes it first.
    [string]$Image = '', [switch]$Fresh,
    # End the run when gdb exits (a record taken at one breakpoint that then
    # quits) instead of waiting out $Seconds.
    [switch]$StopWithGdb
)
# One gdb session for a whole emulator run on runner slot $Slot (the melonDS
# stub takes one attach per instance): main, the pokes, the caller's
# breakpoints, continue; the emulator stops after $Seconds. The record is
# builds/probes/<Tag>.log; captures go to artifacts/visibility/probes/<Tag>/.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
if ($WaitFile -ne '') {
    while (-not ((Test-Path $WaitFile) -and
                 (Select-String -Path $WaitFile -Pattern ([regex]::Escape($Wait)) -Quiet))) {
        Start-Sleep -Seconds 20
    }
}
. (Join-Path $S64Repo 'scripts\lib\melonds.ps1')
$shots = Join-Path $S64Shots $Tag
New-Item -ItemType Directory -Force $shots | Out-Null
$context = Initialize-MelonDSVerifierContext -Root $S64Repo -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$romPath = Join-Path $S64Repo "builds\$Build\$Rom.nds"
$elfPath = Join-Path $S64Repo "builds\$Build\$Rom.elf"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port `
    -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
if ($Image -ne '') {
    if ($Fresh) {
        New-Item -ItemType Directory -Force (Split-Path $Image) | Out-Null
        Remove-Item -LiteralPath $Image, "$Image.idx" -Force -ErrorAction SilentlyContinue
    }
    $cfgText = Get-Content -LiteralPath $state.Config -Raw
    $cfgText = Set-MelonDSTomlValue -Text $cfgText -Section 'DLDI' -Key 'ImagePath' -Value ('"' + ($Image -replace '\\', '/') + '"')
    $cfgText = Set-MelonDSTomlValue -Text $cfgText -Section 'DLDI' -Key 'ReadOnly' -Value 'false'
    Set-Content -LiteralPath $state.Config -Value $cfgText -NoNewline
}
$p = $null
$g = $null
$n = 0
$log = Join-Path $S64Probes "$Tag.log"
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($romPath) -WorkingDirectory (Split-Path $melon) -WindowStyle Minimized -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $cmd = Join-Path $S64Probes "$Tag.gdb"
    (@('set pagination off', 'set confirm off', 'set remote X-packet off', "target remote localhost:$port",
       'break main', 'continue', 'delete') + $Pokes + $Lines + @('continue')) |
        Set-Content -LiteralPath $cmd -Encoding ascii
    $g = Start-Process -FilePath $S64Gdb -ArgumentList @('-batch', '-x', $cmd, $elfPath) -RedirectStandardOutput $log `
        -RedirectStandardError "$log.err" -WindowStyle Hidden -PassThru
    $t0 = Get-Date
    while (((Get-Date) - $t0).TotalSeconds -lt $Seconds) {
        if ($p.HasExited) { break }
        if ($StopWithGdb -and $g.HasExited) { break }
        if ($Every -gt 0) {
            Start-Sleep -Seconds $Every
            if ($p.HasExited) { break }
            $png = Join-Path $shots ('{0}-{1:d3}.png' -f $Tag, $n)
            try { & (Join-Path $S64Repo 'scripts\capture-running-melonds-window.ps1') -EmulatorProcessId $p.Id -Output $png *> $null } catch { }
            $n++
        } else { Start-Sleep -Seconds 2 }
    }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Start-Sleep -Seconds 2
    if ($g -and -not $g.HasExited) { Stop-Process -Id $g.Id -Force }
    if ($Image -ne '') {
        $cfgText = Get-Content -LiteralPath $state.Config -Raw
        $cfgText = Set-MelonDSDldiProfile -Text $cfgText -ReadOnly
        Set-Content -LiteralPath $state.Config -Value $cfgText -NoNewline
    }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content $log | Select-String -Pattern '^(X |Program|Breakpoint [0-9]+,|#[0-9])' | Select-Object -First 80 |
    ForEach-Object { $_.Line }
"log=$log shots=$n"
