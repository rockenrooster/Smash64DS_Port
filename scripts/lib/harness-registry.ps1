$ErrorActionPreference = 'Stop'

function New-HarnessRecord {
    param(
        [string]$Name,
        [Nullable[int]]$Mode,
        [string]$Harness,
        [string]$Script,
        [string]$Target,
        [string]$Build,
        [string[]]$Tags
    )

    [PSCustomObject]@{
        Name = $Name
        Mode = $Mode
        Harness = $Harness
        Script = $Script
        Target = $Target
        Build = $Build
        Tags = $Tags
    }
}

function Get-Smash64DSHarnessRegistry {
    @(
        New-HarnessRecord 'runtime' $null $null 'verify-runtime.ps1' $null $null @('latest','normal_runtime')
        New-HarnessRecord 'battle_playable' 163 'battle_playable' 'verify-battle-playable-harness.ps1' 'smash64ds-battle-playable-fast-hwtri' 'build-battle-playable-hwtri-harness' @('battle_playable','hardware','fast_logic')
        # P2-1M (owner, 2026-08-19). THE ROW WAS `battle_playable_realtime` AND
        # THE SCENE STILL IS -- `Harness` is the scene name the header and
        # Makefile agree on (mode 163) and is deliberately unchanged; `Name` is
        # the verifier row, and the row now reaches that scene THROUGH the VS
        # shell instead of booting into it. The match is identical (Mario human
        # vs level-3 CPU Fox, Dream Land, one-minute Time), so this stays the
        # P1 regression guard docs/P2_PLAN.md law 4 requires.
        #
        # Target/Build moved off the P1 names onto the P2 shell lab pair. The
        # old Target metadata named `smash64ds-battle-playable-hwtri` -- the
        # FROZEN P1 artifact -- which this arm never built and only screenshotted;
        # the pair below is the ROM both halves of the arm now actually use.
        New-HarnessRecord 'p2_battle_realtime' 163 'battle_playable_realtime' 'verify-battle-playable-realtime-harness.ps1' 'smash64ds-p2-shell-hwtri' 'build-p2-shell' @('latest','boundary','battle_playable','hardware','live_input','realtime','p2_shell')
        New-HarnessRecord 'battle_playable_match_lifecycle' 163 'battle_playable_match_lifecycle' 'verify-battle-playable-match-lifecycle-harness.ps1' 'smash64ds-battle-playable-cpu-proof' 'build-battle-playable-cpu-proof-harness' @('battle_playable','cpu','timer','match_end','results')
        # P2-1g. The VS shell's full-loop walk, and the second arm of Boundary
        # from the P2-1 phase close (docs/P2_PLAN.md law 4). It has no
        # NDS_DEV_SCENE_HARNESS mode of its own -- it builds the battle_playable
        # _realtime harness with the shell flags on, so `Harness`/`Mode` are
        # null and check-harness-registry's header/Makefile mode cross-check
        # skips it by design, the same way the `runtime` record is skipped.
        New-HarnessRecord 'p2_shell_loop' $null $null 'verify-p2-shell-loop.ps1' 'smash64ds-p2-shell-loop-hwtri' 'build-p2-shell-loop' @('latest','boundary','p2_shell','scene_loop','menus')
        # P2-2 phase close. This is a configuration verifier, not a new scene
        # harness: it boots source VSBattle directly with the dedicated
        # NDS_P2_FOUR_CPU_STRESS target, proves 0 humans / 4 level-3 CPUs / four
        # live fighter GObjs, covers the complete one-minute guest clock, and
        # carries the memory/effect/particle/AObj hard-failure pins. Timing is
        # reported as debt by P2-2; it is not silently converted into a looser
        # version of P1's 1.12M target. See docs/p2/P2-2-four-fighters.md.
        New-HarnessRecord 'p2_fourcpu_stress' $null $null 'verify-p2-four-fighter-stress.ps1' 'smash64ds-p2-fourcpu-tickhud-hwtri' 'build-p2-fourcpu-tickhud' @('latest','boundary','p2','four_cpu','stress','hardware','realtime','memory')
    )
}

function Select-Smash64DSRegistryEntriesByName {
    param(
        [object[]]$Registry,
        [string[]]$Names
    )

    $entries = @()
    foreach ($name in $Names) {
        $record = $Registry | Where-Object { $_.Name -eq $name } | Select-Object -First 1
        if (-not $record) { throw "Registry entry '$name' was not found." }
        $entries += $record
    }
    return @($entries)
}

function Get-Smash64DSVerifyPlan {
    param(
        [ValidateSet('Latest','Boundary')]
        [string]$Profile = 'Boundary',
        [string[]]$Only,
        [string]$From
    )

    $registry = @(Get-Smash64DSHarnessRegistry)
    # BOUNDARY AFTER P2-2. The P2-1 shell loop and 2-fighter realtime regression
    # guard stay intact; P2-2 adds the standing 4-CPU stress configuration as
    # law 4 requires. The registry still exposes only Latest and Boundary; the
    # retired diagnostic fleet does not return.
    $names = if ($Profile -eq 'Latest') {
        @('runtime', 'p2_shell_loop', 'p2_battle_realtime', 'p2_fourcpu_stress')
    } else {
        @('p2_shell_loop', 'p2_battle_realtime', 'p2_fourcpu_stress')
    }
    $plan = @(Select-Smash64DSRegistryEntriesByName $registry $names)

    if ($Only -and $Only.Count -gt 0) {
        $wanted = @($Only | ForEach-Object { $_.Trim() } | Where-Object { $_ })
        $plan = @($registry | Where-Object { $wanted -contains $_.Name })
        $missing = @($wanted | Where-Object { $name = $_; -not ($registry | Where-Object { $_.Name -eq $name }) })
        if ($missing.Count -gt 0) { throw "Unknown verifier registry name(s): $($missing -join ', ')" }
    } elseif ($From) {
        $index = -1
        for ($i = 0; $i -lt $plan.Count; $i++) {
            if ($plan[$i].Name -eq $From) { $index = $i; break }
        }
        if ($index -lt 0) { throw "Registry entry '$From' is not in profile '$Profile'." }
        $plan = @($plan[$index..($plan.Count - 1)])
    }

    return @($plan)
}
