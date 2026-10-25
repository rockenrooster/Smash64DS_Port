param(
    # melonDS runner slots baked in parallel, one boot per still.
    [int[]]$Slots = @(9, 10, 11, 12),
    # The lab ROM: TARGET=smash64ds NDS_MENU_WALK=1 NDS_1P_INTRO_BAKE=1.
    [string]$RomDir = 'builds\build-p2-intro-bake',
    # Raw 256x192 RGB15 captures, one <name>.raw per still.
    [string]$OutDir = 'artifacts\intro_bake\raw',
    # Only jobs whose name matches this regex; empty = all.
    [string]$Only = '',
    [switch]$SkipExisting,
    [int]$Tic = 240,
    [int]$TimeoutSeconds = 300,
    # P7 C1: also the player's cards of these contents (p<card>_c<NN>_<C>,
    # their four royal costumes) and of Metal Mario (p<card>_13_0); a P4 bake
    # ROM (the NDS_P4_* and NDS_P5 flags beside NDS_1P_INTRO_BAKE=1).
    [int[]]$Contents = @(),
    [switch]$Metal,
    # Only the content and Metal Mario cards.
    [switch]$CardsOnly,
    # P7 C1: these contents' congratulations stills (studio_c<NN>): the player
    # card in the studio framing (gNdsIntroBakeStudio*), for
    # scripts/p6/p6_congra_plates.py.
    [int[]]$Studio = @(),
    # The full figure at the card camera's 11-degree lens: ~5,500 units away,
    # aimed at the hips; -StudioStatus a demo status for the pose (65537 =
    # nFTDemoStatusWin1, the Results victory poses), 0 the card's.
    [int]$StudioDist = 5500, [int]$StudioY = 0, [int]$StudioYaw = 25, [int]$StudioStatus = 0,
    # The content cards' pose (-Contents): 0 the card's own (the content's
    # IntroL), otherwise a demo status as -StudioStatus (owner 2026-10-19:
    # Palutena's took her base's intro pose, the staff through her head).
    [int]$CardStatus = 0,
    # Debug: gdb lines added to every job's session before it runs.
    [string[]]$ExtraGdb = @()
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
if ($CardsOnly) { $jobs.Clear() }
# The player's card is made as the content at the bake (nds_p4.c
# ndsP4MatchContent, gNdsIntroBakePlayerCard): its parent's kind, the
# content poked beside it, each of its royal costumes (default_costumes[0..3]).
$kindIds = @{ nFTKindMario = 0; nFTKindFox = 1; nFTKindDonkey = 2; nFTKindSamus = 3; nFTKindLuigi = 4;
              nFTKindLink = 5; nFTKindYoshi = 6; nFTKindCaptain = 7; nFTKindKirby = 8; nFTKindPikachu = 9;
              nFTKindPurin = 10; nFTKindNess = 11 }
if ($Contents.Count -gt 0) {
    $gen = Join-Path $root "$RomDir\p4"
    $rows = @{}
    foreach ($m in (Select-String -Path (Join-Path $gen 'nds_p4_contents.generated.h') -Pattern '^\s+X\((\d+), \w+, \w+, (\w+), (nFTKind\w+),')) {
        $rows[[int]$m.Matches[0].Groups[1].Value] = @{ Name = $m.Matches[0].Groups[2].Value; Parent = $kindIds[$m.Matches[0].Groups[3].Value] }
    }
    foreach ($c in $Contents) {
        if (-not $rows.ContainsKey($c)) { throw "content $c is not in this bake ROM" }
        $r = $rows[$c]
        $line = Select-String -Path (Join-Path $gen "$($r.Name)\nds_p4_$($r.Name).generated.c") -Pattern '\.default_costumes = \{ ([\d, ]+) \}' | Select-Object -First 1
        $royal = @($line.Matches[0].Groups[1].Value.Split(',')[0..3] | ForEach-Object { [int]$_.Trim() } | Select-Object -Unique)
        foreach ($card in @(@(0, 0), @(1, 4), @(3, 6))) {
            foreach ($costume in $royal) {
                $jobs.Add(@{ Name = ('p{0}_c{1:00}_{2}' -f $card[0], $c, $costume); Stage = $card[1]; Show = (1 -shl $card[0]);
                             PlayerKind = $r.Parent; PlayerCostume = $costume; Content = $c;
                             CardStatus = $CardStatus })
            }
        }
    }
}
foreach ($c in $Studio) {
    if (-not $rows) { throw "-Studio needs -Contents with the same ids" }
    $r = $rows[$c]
    $jobs.Add(@{ Name = ('studio_c{0:00}' -f $c); Stage = 0; Show = 1; PlayerKind = $r.Parent; PlayerCostume = 0;
                 Content = $c; StudioDist = $StudioDist; StudioY = $StudioY; StudioYaw = $StudioYaw; StudioStatus = $StudioStatus })
}
if ($Metal) {
    foreach ($card in @(@(0, 0), @(1, 4), @(3, 6))) {
        # Metal Mario plays the campaign as Mario's kind with the metal flag
        # (nds_menu_shell_css.c ndsMenuShellCssOnePCommit) and is made as
        # himself: the intro's camera and tables stop at the twelve kinds.
        $jobs.Add(@{ Name = ('p{0}_13_0' -f $card[0]); Stage = $card[1]; Show = (1 -shl $card[0]);
                     PlayerKind = 0; PlayerCostume = 0; Metal = 1 })
    }
}
if ($Only -ne '') { $jobs = @($jobs | Where-Object { $_.Name -match $Only }) }
if ($SkipExisting) { $jobs = @($jobs | Where-Object { -not (Test-Path (Join-Path $OutDir ($_.Name + '.raw'))) }) }
"jobs: $($jobs.Count)"

$bakeOne = {
    param($root, $romDir, $outDir, $slot, $tic, $timeout, $job, $extraGdb)
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
            -WorkingDirectory (Split-Path $melon) -WindowStyle Minimized -PassThru
        Wait-MelonDSGdbListener -Process $p -Port $port | Out-Null
        $g = @(
            'set pagination off', 'set confirm off', "target remote localhost:$port", 'set $poked = 0',
            'break sc1PManagerUpdateScene', 'commands', 'silent', 'if $poked == 0',
            "set variable gSCManagerSceneData.spgame_stage = $($job.Stage)", 'set $poked = 1', 'end', 'continue', 'end',
            # The walk skips the intro; let this one run.
            'break ndsSceneManagerEnter', 'commands', 'silent',
            'if gSCManagerSceneData.scene_curr == 14', 'set variable gNdsMenuShellWalkRoute = 0',
            'else', 'set variable gNdsMenuShellWalkRoute = 1', 'end'
        )
        if ($job.ContainsKey('Content')) {
            # The 1P select picks the content as a player would (its walk's
            # target, ndsMenuShellCssWalkOnePTarget): its commit writes the
            # parent's kind and the content, and its preview the content's
            # images, as the campaign's own intro sees them.
            $g += @('if gSCManagerSceneData.scene_curr == 17',
                    'set $ca = (unsigned)&gNdsP4OnePlayerContent', 'set $cw = *(unsigned*)($ca & ~3)',
                    'set $cs = ($ca & 3) * 8',
                    "set variable *(unsigned*)(`$ca & ~3) = (`$cw & ~(0xff << `$cs)) | ($($job.Content) << `$cs)",
                    'end')
        }
        $g += @(
            'continue', 'end',
            'break sc1PIntroInitVars', 'commands', 'silent',
            "set variable $player.fkind = $($job.PlayerKind)", "set variable $player.costume = $($job.PlayerCostume)",
            "set variable $player.shade = 0"
        )
        if ($job.ContainsKey('Content')) {
            # gNdsP4OnePlayerContent is a byte: a word read-modify-write
            # (a gdb byte write stalls melonDS).
            $g += @('set $ca = (unsigned)&gNdsP4OnePlayerContent', 'set $cw = *(unsigned*)($ca & ~3)',
                    'set $cs = ($ca & 3) * 8',
                    "set variable *(unsigned*)(`$ca & ~3) = (`$cw & ~(0xff << `$cs)) | ($($job.Content) << `$cs)",
                    # The scene's kind the 1P select wrote beside it: the parent.
                    'set $ka = (unsigned)&gSCManagerSceneData.fkind', 'set $kw = *(unsigned*)($ka & ~3)',
                    'set $ks = ($ka & 3) * 8',
                    "set variable *(unsigned*)(`$ka & ~3) = (`$kw & ~(0xff << `$ks)) | ($($job.PlayerKind) << `$ks)")
        }
        if ($job.ContainsKey('Metal')) {
            # gNdsP7OnePlayerMetal is a byte: a word read-modify-write.
            $g += @('set $ma = (unsigned)&gNdsP7OnePlayerMetal', 'set $mw = *(unsigned*)($ma & ~3)',
                    'set $ms = ($ma & 3) * 8',
                    'set variable *(unsigned*)($ma & ~3) = ($mw & ~(0xff << $ms)) | (1 << $ms)')
        }
        if ($job.ContainsKey('Ally1Kind')) {
            $g += @("set variable $ally0.fkind = $($job.Ally1Kind)", "set variable $ally0.costume = $($job.Ally1Costume)",
                    "set variable $ally0.shade = 0")
        }
        if ($job.ContainsKey('Ally2Kind')) {
            $g += @("set variable $ally1.fkind = $($job.Ally2Kind)", "set variable $ally1.costume = $($job.Ally2Costume)",
                    "set variable $ally1.shade = 0")
        }
        if ($job.ContainsKey('CardStatus') -and ($job.CardStatus -ne 0)) {
            $g += @("set variable gNdsIntroBakeStudioStatus = $($job.CardStatus)")
        }
        if ($job.ContainsKey('StudioDist')) {
            $g += @("set variable gNdsIntroBakeStudioDist = $($job.StudioDist)",
                    "set variable gNdsIntroBakeStudioY = $($job.StudioY)",
                    "set variable gNdsIntroBakeStudioYaw = $($job.StudioYaw)",
                    "set variable gNdsIntroBakeStudioStatus = $($job.StudioStatus)")
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
            'bt 6', 'kill', 'quit', 'end'
        ) + $extraGdb + @('continue')
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
                -ArgumentList $root, $RomDir, $OutDir, $slot, $Tic, $TimeoutSeconds, $queue.Dequeue(), $ExtraGdb
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
