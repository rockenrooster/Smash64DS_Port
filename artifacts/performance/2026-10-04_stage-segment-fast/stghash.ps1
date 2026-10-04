param([int]$Slot = 5, [string]$Tag = 'sh01', [string]$Build = 'build-lab-cull',
      [string]$Pokes = '', [int]$LogicLo = 600, [int]$LogicHi = 2400,
      [int]$TimeoutSeconds = 2400)
# Four-CPU lab ROM, gate configuration (+ $Pokes "word=value;..." at main): at
# the start of each presented frame with logic frame in [LogicLo, LogicHi],
# print and reset the stage GX hash (every word sent to the FIFO, span states,
# closing depth/counters of the frame before), keyed by logic frame so two
# arms of a same-ROM A/B compare the same game moments.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
$sp = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\31e5c78d-38d0-4d75-b683-91723230d2cc\scratchpad'
$context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $Slot -NoBuild
$port = $context.GdbPort
$melon = $context.MelonDSPath
$romPath = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elfPath = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.elf"
$gdbExe = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$cmd = Join-Path $sp "sh-$Tag.gdb"
$log = Join-Path $sp "sh-$Tag.log"
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
$p = $null; $gp = $null
$pokeLines = @()
foreach ($kv in ($Pokes -split ';' | Where-Object { $_ -ne '' })) { $pokeLines += "set variable $kv" }
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($romPath) -WorkingDirectory (Split-Path $melon) -WindowStyle Minimized -PassThru
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $g = @('set pagination off', 'set confirm off', "target remote localhost:$port",
        'tbreak main', 'commands', 'silent') + $pokeLines + @('continue', 'end',
        'set $lastl = 0',
        "break ndsPlatformBeginFrame if (gNdsBattlePlayablePacingLogicFrames != `$lastl) && (gNdsBattlePlayablePacingLogicFrames >= $LogicLo)", 'commands', 'silent',
        'set $lastl = gNdsBattlePlayablePacingLogicFrames',
        'printf "H l=%u h=%08x w=%u s=%08x fc=%u fd=%u nf=%u\n", gNdsBattlePlayablePacingLogicFrames, gNdsLabStageGxHash, gNdsLabStageGxHashWords, gNdsLabStageGxStateHash, gNdsStageGxFastCommits, gNdsStageGxFastDeclines, gNdsRendererNativeFailure.count',
        'set variable gNdsLabStageGxHash = 2166136261', 'set variable gNdsLabStageGxHashWords = 0', 'set variable gNdsLabStageGxStateHash = 2166136261',
        "if gNdsBattlePlayablePacingLogicFrames >= $LogicHi", 'kill', 'quit', 'end',
        'continue', 'end',
        'break __excpt_entry', 'commands', 'printf "HALT exception\n"', 'bt 8', 'kill', 'quit', 'end',
        'continue')
    $g | Set-Content -LiteralPath $cmd -Encoding ascii
    $gp = Start-Process -FilePath $gdbExe -ArgumentList @('-batch', '-x', $cmd, $elfPath) -PassThru -WindowStyle Hidden -RedirectStandardOutput $log -RedirectStandardError (Join-Path $sp "sh-$Tag.err")
    if (-not $gp.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $gp.Id -Force; 'TIMEOUT' }
} finally {
    if ($gp -and -not $gp.HasExited) { Stop-Process -Id $gp.Id -Force }
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
$h = @(Get-Content $log | Where-Object { $_ -like 'H *' })
"$Tag rows=$($h.Count) last=$($h[-1])"
Get-Content $log | Select-String '^(HALT|#)' | ForEach-Object { $_.Line }
Get-Content (Join-Path $sp "sh-$Tag.err") -ErrorAction SilentlyContinue | Select-String 'No symbol|Error|syntax' | Select-Object -First 4 | ForEach-Object { $_.Line }
