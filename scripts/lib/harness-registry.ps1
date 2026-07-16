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
        New-HarnessRecord 'battle_playable_realtime' 163 'battle_playable_realtime' 'verify-battle-playable-realtime-harness.ps1' 'smash64ds-battle-playable-hwtri' 'build-battle-playable-canonical-hwtri-harness' @('latest','battle_playable','hardware','live_input','realtime')
        New-HarnessRecord 'battle_playable_match_lifecycle' 163 'battle_playable_match_lifecycle' 'verify-battle-playable-match-lifecycle-harness.ps1' 'smash64ds-battle-playable-cpu-proof' 'build-battle-playable-cpu-proof-harness' @('battle_playable','cpu','timer','match_end','results')
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
    $names = if ($Profile -eq 'Latest') {
        @('runtime', 'battle_playable_realtime')
    } else {
        @('battle_playable_realtime')
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
