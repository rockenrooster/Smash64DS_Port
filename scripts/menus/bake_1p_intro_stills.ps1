param(
    # melonDS runner slots baked in parallel, one boot per still.
    [int[]]$Slots = @(9, 10, 11, 12),
    # The lab ROM: TARGET=smash64ds NDS_P2_MENU_WALK=1 NDS_1P_INTRO_BAKE=1.
    [string]$RomDir = 'builds\build-p2-intro-bake',
    # Raw 256x192 RGB15 captures, one <name>.raw per still.
    [string]$OutDir = 'artifacts\intro_bake\raw',
    # Only jobs whose name matches this regex; empty = all.
    [string]$Only = '',
    [switch]$SkipExisting,
    [int]$Tic = 240,
    [int]$TimeoutSeconds = 300
)
# Bakes the 1P intro's fighter stills from the DS renderer (P2-6; the owner
# chose rendered stills for the shipping intro, which runs static).
#
# Each still is one boot of the bake ROM: the menu walk reaches the 1P game,
# the campaign stage is poked at the manager's first update, the player and
# allies are poked into gSCManager1PGameBattleState when the intro reads them
# (sc1PIntroInitVars), the intro runs unskipped, and at intro tic $Tic the
# ROM display-captures the 3D layer alone into bank D with every other fighter
# group hidden (battleship_sc1pintro.c, NDS_1P_INTRO_BAKE). gdb dumps the
# capture. scripts/menus/pack_1p_intro_stills.py turns the dumps into the
# assets/intro/*.s1i files the static intro blits.
#
# Names (ndsSC1PIntroStillPath):
#   p<card>_<FF>_<C>  the player's card: card 0 on the solo stages, 1 beside
#                     one ally (Mario Bros), 3 beside two (Giant DK)
#   a<card>_<FF>_<C>  an ally's card: 2 (Mario Bros), 4 and 5 (Giant DK)
#   o<SS>             stage SS's VS fighters
#   o<SS>_<FF>_<C>    the same when the player's kind/costume makes the source
#                     recolour them (sc1PIntroInitVSFighters)
# FF is the fighter kind, C the costume ID (ftparam.c dFTParamCostumeIDs).
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not [System.IO.Path]::IsPathRooted($OutDir)) { $OutDir = Join-Path $root $OutDir }
New-Item -ItemType Directory -Force $OutDir | Out-Null

# decomp ft/ftparam.c dFTParamCostumeIDs[kind].royal
$royal = @(@(0,1,2,3), @(0,1,2,3), @(0,1,2,3), @(0,1,2,3), @(0,1,2,3), @(0,2,3,1),
           @(0,1,2,3), @(0,4,1,3), @(0,1,2,3), @(0,1,2,3), @(0,1,2,3), @(0,1,2,3))
$jobs = New-Object System.Collections.Generic.List[object]
foreach ($card in @(@(0, 0), @(1, 4), @(3, 6))) {
    foreach ($kind in 0..11) {
        foreach ($color in 0..3) {
            $jobs.Add(@{ Name = ('p{0}_{1:00}_{2}' -f $card[0], $kind, $royal[$kind][$color]);
                         Stage = $card[1]; Show = (1 -shl $card[0]); PlayerKind = $kind;
                         PlayerCostume = $royal[$kind][$color] })
        }
    }
}
# Mario Bros: the ally is never Mario (sc1pmanager.c masks him out), and a
# Luigi ally wears costume 1.
foreach ($kind in 1..11) {
    $costume = if ($kind -eq 4) { 1 } else { 0 }
    $player = if ($kind -eq 11) { 5 } else { 11 }
    $jobs.Add(@{ Name = ('a2_{0:00}_{1}' -f $kind, $costume); Stage = 4; Show = 4; PlayerKind = $player;
                 PlayerCostume = 0; Ally1Kind = $kind; Ally1Costume = $costume })
}
foreach ($kind in 0..11) {
    $player = if ($kind -eq 11) { 5 } else { 11 }
    $other = if ($kind -eq 0) { 1 } else { 0 }
    $jobs.Add(@{ Name = ('a4_{0:00}_0' -f $kind); Stage = 6; Show = 16; PlayerKind = $player; PlayerCostume = 0;
                 Ally1Kind = $kind; Ally1Costume = 0; Ally2Kind = $other; Ally2Costume = 0 })
    $jobs.Add(@{ Name = ('a5_{0:00}_0' -f $kind); Stage = 6; Show = 32; PlayerKind = $player; PlayerCostume = 0;
                 Ally1Kind = $other; Ally1Costume = 0; Ally2Kind = $kind; Ally2Costume = 0 })
}
# The player is Ness except where Ness is the point; no stage recolours for him.
foreach ($stage in @(0, 2, 4, 5, 6, 9, 10, 13)) {
    $jobs.Add(@{ Name = ('o{0:00}' -f $stage); Stage = $stage; Show = 64; PlayerKind = 11; PlayerCostume = 0 })
}
# sc1PIntroInitVSFighters' recolours: the stage's own kind in its default
# costume (Link, Fox, Pikachu, Samus; Mario/Luigi at Mario Bros).
$variants = @(@(0, 5, 0), @(2, 1, 0), @(4, 0, 0), @(4, 4, 0), @(5, 9, 0), @(9, 3, 0))
foreach ($v in $variants) {
    $jobs.Add(@{ Name = ('o{0:00}_{1:00}_{2}' -f $v[0], $v[1], $v[2]); Stage = $v[0]; Show = 64;
                 PlayerKind = $v[1]; PlayerCostume = $v[2] })
}
# The three teams bake one member per boot (gNdsIntroBakeMember: a whole team
# does not fit the scene heap); pack_1p_intro_stills.py composites them far to
# near into o01 / o08 / o12 and their recolours.
#   o01m<ii>   Yoshi i; o01m<ii>s Yoshi i shaded (the player is a Yoshi in
#              costume i % 6, which only costumes 0-3 can be)
#   o08m<ii>   Kirby i with its hat; o08m<ii>k the same when the player is a
#              Kirby in costume 0 (the team recolours)
#   o12m<ii>   polygon kind nFTKindNStart + i (no NLuigi 4, no NPurin 10)
foreach ($i in 0..17) {
    $jobs.Add(@{ Name = ('o01m{0:00}' -f $i); Stage = 1; Show = 64; PlayerKind = 11; PlayerCostume = 0; Member = $i })
    if (($i % 6) -le 3) {
        $jobs.Add(@{ Name = ('o01m{0:00}s' -f $i); Stage = 1; Show = 64; PlayerKind = 6;
                     PlayerCostume = ($i % 6); Member = $i })
    }
}
foreach ($i in 0..7) {
    $jobs.Add(@{ Name = ('o08m{0:00}' -f $i); Stage = 8; Show = 64; PlayerKind = 11; PlayerCostume = 0; Member = $i })
    $jobs.Add(@{ Name = ('o08m{0:00}k' -f $i); Stage = 8; Show = 64; PlayerKind = 8; PlayerCostume = 0; Member = $i })
}
foreach ($i in @(0, 1, 2, 3, 5, 6, 7, 8, 9, 11)) {
    $jobs.Add(@{ Name = ('o12m{0:00}' -f $i); Stage = 12; Show = 64; PlayerKind = 11; PlayerCostume = 0; Member = $i })
}
if ($Only -ne '') { $jobs = @($jobs | Where-Object { $_.Name -match $Only }) }
if ($SkipExisting) { $jobs = @($jobs | Where-Object { -not (Test-Path (Join-Path $OutDir ($_.Name + '.raw'))) }) }
"jobs: $($jobs.Count)"

$bakeOne = {
    param($root, $romDir, $outDir, $slot, $tic, $timeout, $job)
    $ErrorActionPreference = 'Stop'
    . (Join-Path $root 'scripts\lib\melonds.ps1')
    $context = Initialize-MelonDSVerifierContext -Root $root -RunnerSlot $slot -NoBuild
    $port = $context.GdbPort
    $melon = $context.MelonDSPath
    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) "intro-bake-$slot"
    New-Item -ItemType Directory -Force $tmp | Out-Null
    $cmd = Join-Path $tmp 'bake.gdb'
    $log = Join-Path $tmp 'bake.log'
    $raw = (Join-Path $outDir ($job.Name + '.raw')) -replace '\\', '/'
    $player = 'gSCManager1PGameBattleState.players[gSCManagerSceneData.player]'
    $ally0 = 'gSCManager1PGameBattleState.players[gSCManagerSceneData.ally_players[0]]'
    $ally1 = 'gSCManager1PGameBattleState.players[gSCManagerSceneData.ally_players[1]]'
    $state = Enable-MelonDSGdbConfig -MelonDSPath $melon -GdbPort $port -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -BreakOnStartup -MuteAudio
    $p = $null
    try {
        $p = Start-Process -FilePath $melon -ArgumentList @((Join-Path $root "$romDir\smash64ds.nds")) `
            -WorkingDirectory (Split-Path $melon) -PassThru
        Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
        $g = @(
            'set pagination off', 'set confirm off', "target remote localhost:$port", 'set $poked = 0',
            'break sc1PManagerUpdateScene', 'commands', 'silent', 'if $poked == 0',
            "set variable gSCManagerSceneData.spgame_stage = $($job.Stage)", 'set $poked = 1', 'end', 'continue', 'end',
            # The walk skips the intro; let this one run.
            'break ndsSceneManagerEnter', 'commands', 'silent',
            'if gSCManagerSceneData.scene_curr == 14', 'set variable gNdsMenuShellWalkRoute = 0',
            'else', 'set variable gNdsMenuShellWalkRoute = 1', 'end', 'continue', 'end',
            'break sc1PIntroInitVars', 'commands', 'silent',
            "set variable $player.fkind = $($job.PlayerKind)", "set variable $player.costume = $($job.PlayerCostume)",
            "set variable $player.shade = 0"
        )
        if ($job.ContainsKey('Ally1Kind')) {
            $g += @("set variable $ally0.fkind = $($job.Ally1Kind)", "set variable $ally0.costume = $($job.Ally1Costume)",
                    "set variable $ally0.shade = 0")
        }
        if ($job.ContainsKey('Ally2Kind')) {
            $g += @("set variable $ally1.fkind = $($job.Ally2Kind)", "set variable $ally1.costume = $($job.Ally2Costume)",
                    "set variable $ally1.shade = 0")
        }
        $member = if ($job.ContainsKey('Member')) { $job.Member } else { -1 }
        $depth = (Join-Path $outDir ($job.Name + '.depth')) -replace '\\', '/'
        $g += @(
            "set variable gNdsIntroBakeMember = $member",
            "set variable gNdsIntroBakeShow = $($job.Show)", "set variable gNdsIntroBakeTic = $tic",
            'set variable gNdsRendererNativeFailure.count = 0', 'continue', 'end',
            'break ndsIntroBakeCaptured', 'commands', 'silent',
            "dump binary memory $raw 0x06860000 0x06878000",
            "dump binary value $depth gNdsIntroBakeDepth",
            'printf "BAKED tic=%u nf=%u decline=%u depth=%d\n", sc1PIntroTotalTimeTics, gNdsRendererNativeFailure.count, gNdsFtrDeclineStage, gNdsIntroBakeDepth',
            'kill', 'quit', 'end',
            'break ndsSyMallocOverflowHalt', 'commands',
            'printf "HALT malloc-overflow scene=%u tic=%u\n", gSCManagerSceneData.scene_curr, sc1PIntroTotalTimeTics',
            'bt 8', 'kill', 'quit', 'end',
            'break __excpt_entry', 'commands', 'printf "HALT exception scene=%u\n", gSCManagerSceneData.scene_curr',
            'bt 6', 'kill', 'quit', 'end',
            'continue'
        )
        $g | Set-Content -LiteralPath $cmd -Encoding ascii
        $gp = Start-Process -FilePath 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe' `
            -ArgumentList @('-batch', '-x', $cmd, (Join-Path $root "$romDir\smash64ds.elf")) -PassThru `
            -WindowStyle Hidden -RedirectStandardOutput $log -RedirectStandardError (Join-Path $tmp 'bake.err')
        if (-not $gp.WaitForExit($timeout * 1000)) { Stop-Process -Id $gp.Id -Force; return "$($job.Name) TIMEOUT" }
    } finally {
        if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
        Restore-MelonDSGdbConfig -State $state
    }
    $result = (Get-Content $log | Select-String '^BAKED|^HALT|^#' | ForEach-Object { $_.Line }) -join ' | '
    if ($result -eq '') { $result = 'NO CAPTURE' }
    return "$($job.Name) $result"
}

$queue = [System.Collections.Queue]::new()
foreach ($j in $jobs) { $queue.Enqueue($j) }
$running = @{}
while (($queue.Count -gt 0) -or ($running.Count -gt 0)) {
    foreach ($slot in $Slots) {
        if ((-not $running.ContainsKey($slot)) -and ($queue.Count -gt 0)) {
            $running[$slot] = Start-Job -ScriptBlock $bakeOne `
                -ArgumentList $root, $RomDir, $OutDir, $slot, $Tic, $TimeoutSeconds, $queue.Dequeue()
        }
    }
    Start-Sleep -Milliseconds 500
    foreach ($slot in @($running.Keys)) {
        if ($running[$slot].State -ne 'Running') {
            Receive-Job $running[$slot] | ForEach-Object { $_ }
            Remove-Job $running[$slot]
            $running.Remove($slot)
        }
    }
}
