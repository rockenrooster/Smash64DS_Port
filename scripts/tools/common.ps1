# Shared by every tool in scripts/tools (dot-source it: `. "$PSScriptRoot\common.ps1"`):
# where the repository is, where runs write their logs, the devkitPro tools,
# and the content tables the drivers need. Nothing here starts a process.

$S64Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# Probe logs, gdb command files and run summaries: generated output, so under
# builds/ (never tracked).
$S64Probes = Join-Path $S64Repo 'builds\probes'
New-Item -ItemType Directory -Force $S64Probes | Out-Null
# Screenshots the probes take.
$S64Shots = Join-Path $S64Repo 'artifacts\visibility\probes'
$S64Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$S64Cp = 'C:\Program Files\Git\usr\bin\cp.exe'

function Use-S64DevkitPro {
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
}

# The freeze report's trips, for every probe whose ROM carries the report
# (P8 S5): gdb lines that print an X STALL line (trip count, scene, the one
# before it, heartbeat) whenever the watchdog trips; none for an ELF without
# the report, since an unknown breakpoint ends a gdb batch script.
function Get-S64StallLines([string]$Elf) {
    $nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
    if (-not (Test-Path -LiteralPath $Elf)) { return @() }
    if (-not (& $nm $Elf | Select-String -SimpleMatch ' ndsFreezeDiagnosticsStallMarker' -Quiet)) { return @() }
    return @('break ndsFreezeDiagnosticsStallMarker', 'commands', 'silent',
        'printf "X STALL trips=%u scene=%u prev=%u hb=%u\n", gNdsFreezeDiagnosticsWatchdogTripCount, gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsFreezeDiagnosticsHeartbeat',
        'continue', 'end')
}

# The ship layout: every P4 content, the P5 stages and the shipping front end
# (memory play-rom-needs-p5-flag, sweep-must-use-ship-layout).
$S64ContentFlags = @('FALCO', 'GANONDORF', 'WOLF', 'BOWSER', 'MARTH', 'ROY', 'WARIO', 'PEACH',
    'CRASH', 'LANKY', 'SHEIK', 'SONIC', 'DEDEDE', 'PIANO', 'METAKNIGHT', 'MRGAW', 'SNAKE', 'IKE',
    'PALUTENA') | ForEach-Object { "NDS_P4_$_=1" }
$S64PlayFlags = $S64ContentFlags + @('NDS_P5=1')
$S64ShipFlags = $S64PlayFlags + @('NDS_1P_GAME=1', 'NDS_MENU_SHELL=1', 'NDS_UI_KIT=1',
    'NDS_FREEZE_DIAGNOSTICS=1', 'NDS_SHIP_TELEMETRY=1', 'NDS_BOOT_DIAG_TEXT=0',
    'NDS_DEMO_FOX_CPU_LADDER=0')

# Ike and Palutena (the authored contents, P6) build from stored files that
# derive from the owner's rips and stay on the owner's PC (owner 2026-10-29).
# A build tool passes its flags through this, which leaves out, and names,
# each authored content scripts/p6/p6_local.py does not report ready, so a
# checkout without those files builds every other content. make itself still
# fails a content that is enabled without its inputs.
function Select-S64ContentFlags([string[]]$Flags) {
    $notReady = @(& python (Join-Path $S64Repo 'scripts\p6\p6_local.py') --not-ready)
    if ($LASTEXITCODE -ne 0) { throw 'scripts/p6/p6_local.py failed' }
    foreach ($name in $notReady) {
        Write-Host "Building without ${name}: its local files are absent (python scripts/p6/p6_local.py --why)" -ForegroundColor Yellow
    }
    $drop = @($notReady | ForEach-Object { "NDS_P4_$($_.ToUpper())=1" })
    return @($Flags | Where-Object { $drop -notcontains $_ })
}
# The sweep ROM's lab-only ITCM evictions (memory lab-build-needs-itcm-evictions).
$S64SweepEvictions = @('ndsMPWallSweepStaticMiss', 'ndsRendererInitVertexCache', 'ndsF32ToFixed')

# P4 content id -> the fighter kind it is set up as (its parent).
$S64ContentParent = @{ 1 = 1; 2 = 7; 3 = 1; 4 = 6; 5 = 7; 6 = 7; 7 = 0; 8 = 1; 9 = 0; 10 = 0;
    11 = 7; 13 = 1; 14 = 7; 15 = 0; 16 = 10; 17 = 0; 18 = 7; 19 = 7; 20 = 1 }

# One build at a time: the lab linker script and the UI-kit outputs are shared.
function Invoke-S64Serialized([scriptblock]$Body) {
    $mutex = [System.Threading.Mutex]::new($false, 'Global\smash64ds_build_lab')
    try { [void]$mutex.WaitOne() } catch [System.Threading.AbandonedMutexException] { }
    try { & $Body } finally { $mutex.ReleaseMutex() }
}

# Seed a new build directory from an earlier one (mtimes preserved, so the
# first build is incremental; memory copied-build-dir-needs-preserved-mtimes).
function Copy-S64BuildSeed([string]$Build, [string]$Seed, [string[]]$DropRoms = @()) {
    if (($Seed -eq '') -or (Test-Path (Join-Path $S64Repo "builds\$Build"))) { return }
    if (-not (Test-Path (Join-Path $S64Repo "builds\$Seed"))) { return }
    & $S64Cp -rp "builds/$Seed" "builds/$Build"
    foreach ($rom in $DropRoms) {
        Remove-Item (Join-Path $S64Repo "builds\$Build\$rom") -ErrorAction SilentlyContinue
    }
}

# A repo-owned melonDS other than a runner slot's (emulators/<dir>/melonDS.exe
# with its own SD image beside it, e.g. the GX trap build, gxtrap/README.md):
# the context Initialize-MelonDSVerifierContext returns, minus its repair of
# the shared SD image, which throws while any melonDS runs. gdb listens on
# 4380 + 2 * Slot. Needs scripts/lib/melonds.ps1 dot-sourced first.
function Initialize-S64AltMelonDSContext([string]$MelonPath, [int]$Slot) {
    $port = 4380 + 2 * $Slot
    Set-MelonDSVerifierRunContext -Root $S64Repo -RunnerSlot -1 -GdbPort $port
    $env:SMASH64DS_VERIFY_NO_BUILD = '1'
    return [pscustomobject]@{
        RunnerSlot = -1; GdbPort = $port; Arm7Port = $port + 1; PersistentConfig = $false
        MelonDSPath = Resolve-MelonDSPath -Root $S64Repo -MelonDS $MelonPath
    }
}
