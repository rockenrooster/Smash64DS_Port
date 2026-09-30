# Reproduce: build the lab ROM with NDS_INTERP_FRAC_CAPTURE=1 into builds/build-p2p8-lab-cap
# (four-CPU lab target, NDS_LAB_FOURCPU_SWEEP=1, all stages), then run once per
# pattern, e.g. -Slot 9 -FirstPattern 2 -Flights 1, and feed the dumps to
# scripts/stages/generate_sector_arwing_frac.py.
param(
    [int]$Slot = 9,
    [Parameter(Mandatory)][int]$FirstPattern,
    [int]$Flights = 3,
    [string]$Build = 'build-p2p8-lab-cap',
    [string]$Kinds = '151388424',
    [int]$TimeoutSeconds = 900
)
# Sector Z Arwing spline capture: forces flight patterns FirstPattern,
# FirstPattern+1, ... at each flight start, shortens the wait between flights,
# and dumps gNdsInterpCapture after every finished flight.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
$sp = Join-Path $root 'artifacts\performance\2026-09-30_p2-2p8-arwing-frac\capture'
$out = Join-Path $root 'artifacts\performance\2026-09-30_p2-2p8-arwing-frac\capture'
New-Item -ItemType Directory -Force $out | Out-Null
$tag = "p$FirstPattern"
$port = 3300 + 10 * $Slot
$melon = Join-Path $root "emulators\melonds-runners\slot$Slot\melonDS.exe"
$rom = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.nds"
$elf = Join-Path $root "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.elf"
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$dump = (Join-Path $out "$tag.bin") -replace '\\', '/'
$cmd = Join-Path $sp "arwcap-$tag.gdb"
@"
set pagination off
set confirm off
set `$pattern = $FirstPattern
set `$flights = 0
target remote localhost:$port
break main
continue
delete
set var gNdsLabFourCpuGkind = 1
set var gNdsLabFourCpuKinds = $Kinds
break grSectorArwingUpdateWait if gGRCommonStruct.sector.arwing_appear_timer > 30
commands
  silent
  set var gGRCommonStruct.sector.arwing_appear_timer = 30
  if `$flights > 0
    printf "FLIGHT_END %d count=%u\n", `$flights, gNdsInterpCaptureCount
    dump binary memory $dump &gNdsInterpCapture[0][0] &gNdsInterpCapture[8192][0]
  end
  if `$flights >= $Flights
    printf "DONE count=%u\n", gNdsInterpCaptureCount
    kill
    quit
  end
  continue
end
break func_ovl2_80107D50 if gGRCommonStruct.sector.arwing_flight_pattern != -1
commands
  silent
  printf "FLIGHT_START %d source_pattern=%d forced=%d count=%u\n", `$flights + 1, gGRCommonStruct.sector.arwing_flight_pattern, `$pattern, gNdsInterpCaptureCount
  set var gGRCommonStruct.sector.arwing_flight_pattern = `$pattern
  set `$pattern = `$pattern + 1
  set `$flights = `$flights + 1
  continue
end
continue
"@ | Set-Content -LiteralPath $cmd -Encoding ascii
$state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port ($port + 1) -Persistent -BreakOnStartup -MuteAudio
$p = $null
try {
    $p = Start-Process -FilePath $melon -ArgumentList @($rom) -WorkingDirectory (Split-Path $melon) -PassThru -WindowStyle Hidden
    Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
    $g = Start-Process -FilePath $gdb -ArgumentList @('-batch', '-x', $cmd, $elf) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $out "$tag.log") -RedirectStandardError (Join-Path $out "$tag.err")
    if (-not $g.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $g.Id -Force; "TIMEOUT" }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    Restore-MelonDSGdbConfig -State $state
}
Get-Content (Join-Path $out "$tag.log") | Select-String -Pattern 'FLIGHT|DONE' | ForEach-Object { $_.Line }
