param(
    [switch]$IncludeSlow,
    [switch]$ReportAll,
    [ValidateSet('All','Core','Native','Fighters')]
    [string]$Group = 'All'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script:GeneratorChecks = @()
$script:GeneratorFailures = @()

function Add-GeneratorCheck {
    param(
        [string]$Label,
        [string]$Script,
        [string[]]$Arguments
    )
    $scriptPath = Join-Path $root $Script
    if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
        throw "Generator staleness sweep inventory is stale: missing $Script"
    }
    $script:GeneratorChecks += [PSCustomObject]@{
        Order = $script:GeneratorChecks.Count
        Label = $Label
        Script = $Script
        Arguments = @($Arguments)
    }
}

function Invoke-QueuedGeneratorChecks {
    if ($script:GeneratorChecks.Count -eq 0) { return }
    $checks = @($script:GeneratorChecks)
    $results = @($checks | ForEach-Object -Parallel {
        $check = $_
        $scriptPath = Join-Path $using:root $check.Script
        $generatorArgs = @($check.Arguments)
        $watch = [System.Diagnostics.Stopwatch]::StartNew()
        $output = @(& python $scriptPath @generatorArgs 2>&1 | ForEach-Object { "$_" })
        $exitCode = $LASTEXITCODE
        $watch.Stop()
        [PSCustomObject]@{
            Order = $check.Order
            Label = $check.Label
            ExitCode = $exitCode
            Seconds = $watch.Elapsed.TotalSeconds
            Output = @($output)
        }
    } -ThrottleLimit 8)

    foreach ($result in @($results | Sort-Object Order)) {
        foreach ($line in @($result.Output)) { Write-Output $line }
        Write-Output ("GENERATOR-CHECK {0}: exit={1} seconds={2:N2}" -f `
            $result.Label, $result.ExitCode, $result.Seconds)
        if ($result.ExitCode -ne 0) {
            $script:GeneratorFailures += [PSCustomObject]@{
                Label = $result.Label
                ExitCode = $result.ExitCode
            }
            if ($ReportAll) {
                Write-Output "GENERATOR-CHECK FAILURE: $($result.Label) (exit $($result.ExitCode))"
            } else {
                throw "Generator staleness sweep failed at '$($result.Label)' (exit $($result.ExitCode))."
            }
        }
    }
}

function Test-GeneratorDeclaresCheckArm {
    param([string]$Script)
    $scriptPath = Join-Path $root $Script
    $source = Get-Content -LiteralPath $scriptPath -Raw
    if ($source -match '(?m)add_argument\(\s*["'']--check["'']') {
        return $true
    }
    # Thin wave-1 scripts delegate their complete CLI to the shared generator.
    # Follow that positive import instead of starting every Python interpreter
    # merely to ask for --help during the front gate.
    $runImport = [regex]::Match(
        $source,
        '(?m)^\s*from\s+([A-Za-z0-9_]+)\s+import\s+[^\r\n]*\brun\b[^\r\n]*$'
    )
    if ($runImport.Success) {
        $sharedPath = Join-Path (Split-Path -Parent $scriptPath) ($runImport.Groups[1].Value + '.py')
        if (Test-Path -LiteralPath $sharedPath -PathType Leaf) {
            $sharedSource = Get-Content -LiteralPath $sharedPath -Raw
            return ($sharedSource -match '(?m)add_argument\(\s*["'']--check["'']')
        }
    }
    return $false
}

$sweepWatch = [System.Diagnostics.Stopwatch]::StartNew()
$previousYoster = $env:NDS_P2_STAGE_YOSTER
try {
    if ($Group -in @('All','Core')) {
        $fastChecks = @(
            [PSCustomObject]@{ Label='fighter-owner-images'; Script='scripts/fighters/generate_nds_native_owner_images.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='entry-effects'; Script='scripts/3d_vfx/generate_nds_entry_effects.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='fighter-production-manifest'; Script='scripts/fighters/generate_fighter_production_manifest.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='battle-static-textures'; Script='scripts/generate_battle_playable_static_textures.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='battle-texture-census'; Script='scripts/generate_battle_playable_texture_census.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='renderer-parity-corpus'; Script='scripts/generate_renderer_parity_corpus.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='dreamland-ds-mesh'; Script='scripts/stages/dreamland/generate_dreamland_ds_mesh.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='stage-runtime-rows-dreamland'; Script='scripts/stages/emit_native_stage_runtime_rows.py'; Args=@('--stage', 'dreamland', '--check') },
            [PSCustomObject]@{ Label='fox-gun-source-tables'; Script='scripts/fox_gun_bake.py'; Args=@('--check', 'src/nds/nds_fox_gun.c') }
        )
        foreach ($check in $fastChecks) {
            Add-GeneratorCheck $check.Label $check.Script $check.Args
        }
    }

    # Native owner generators are numerous and keep growing. The file set is the
    # inventory. The front gate checks the declared --check surface without
    # starting dozens of Python processes; -IncludeSlow executes every arm.
    $nativeGapScripts = @()
    if ($Group -in @('All','Native')) {
        $nativeScripts = @(Get-ChildItem -LiteralPath (Join-Path $root 'scripts\stages') `
            -Filter 'generate_nds_native_*.py' -File | Sort-Object Name)
        foreach ($file in $nativeScripts) {
            $relative = 'scripts/stages/' + $file.Name
            if ($file.Name -eq 'generate_nds_native_stage.py') { continue }
            if (Test-GeneratorDeclaresCheckArm $relative) {
                if ($IncludeSlow) {
                    Add-GeneratorCheck ("native-" + $file.BaseName.Substring('generate_nds_native_'.Length)) `
                        $relative @('--check')
                }
            } else {
                $nativeGapScripts += $relative
            }
        }
    }

    $fighterAdmissionNoCheck = @(
        'ness','purin','kirby','gdonkey','mmario','nmario','nfox','ndonkey','nsamus','nluigi',
        'nlink','nyoshi','ncaptain','nkirby','npikachu','npurin','nness','boss','polygons'
    )
    if ($Group -in @('All','Fighters')) {
        Add-GeneratorCheck 'fighter-admission-pickup-fileids' `
            'scripts/fighters/admit_fighter.py' @('--fighter', 'pickup-fileids', '--check')
    }

    if ($IncludeSlow -and $Group -in @('All','Core')) {
        # These are useful reproducibility checks, but each costs roughly
        # 5-7 seconds on this host. Keep the default front gate to checks whose
        # whole parallel batch finishes in a few seconds.
        $particleStamp = Join-Path $root 'src\nds\generated\nds_particle_banks.flags.stamp'
        if (Test-Path -LiteralPath $particleStamp) {
            $stampText = (Get-Content -LiteralPath $particleStamp -Raw).Trim()
            if ($stampText -match '(^|=)1($|\s)') {
                $env:NDS_P2_STAGE_YOSTER = '1'
            } else {
                Remove-Item Env:\NDS_P2_STAGE_YOSTER -ErrorAction SilentlyContinue
            }
        }
        Add-GeneratorCheck 'particle-banks-SLOW' `
            'scripts/generate_nds_particle_banks.py' @('--check')
        Add-GeneratorCheck 'fighter-owners-SLOW' `
            'scripts/fighters/generate_nds_native_owners.py' @('--check')
        Add-GeneratorCheck 'fgm-cue-decode-SLOW' `
            'scripts/sfx/export-fgm-cue-wav.py' @('--check')
        Add-GeneratorCheck 'native-stage-dreamland-SLOW' `
            'scripts/stages/generate_nds_native_stage.py' @('--check')
        Add-GeneratorCheck 'fgm-phase-pack-SLOW' `
            'scripts/sfx/render-audio-fgm-phase-pack.py' @('--check')
    }

    $knownNoCheck = @(
        'scripts/3d_vfx/generate_nds_rebirth_halo.py',
        'scripts/2d_vfx/generate_task39_effect_census.py',
        'scripts/2d_vfx/generate_task39_hit_sparks.py',
        'scripts/menus/generate_battle_hud.py',
        'scripts/menus/generate_mn_ui_kit.py',
        'scripts/stages/generate_native_wallpapers.py',
        'scripts/menus/generate_nds_banner_icon.py',
        'scripts/stages/dreamland/generate_pupupu_water_aot.py',
        'scripts/fighters/generate_preview_core_packs.py',
        'scripts/stages/register_native_stage_runtime.py'
    )
    $verifyOnly = @(
        'scripts/generate_ftanim_dense_bank.py',
        'scripts/generate_ftanim_track_pack.py',
        'scripts/generate_battlepack_anim.py'
    )
    Invoke-QueuedGeneratorChecks
    if ($Group -in @('All','Fighters')) {
        Write-Output ("GENERATOR-CHECK GAP admit_fighter --check unsupported targets ({0}): {1}" -f `
            $fighterAdmissionNoCheck.Count, ($fighterAdmissionNoCheck -join ', '))
    }
    if ($Group -in @('All','Core')) {
        $allGaps = @($knownNoCheck + $nativeGapScripts | Sort-Object -Unique)
        Write-Output ("GENERATOR-CHECK GAP no --check artifact arm ({0}): {1}" -f `
            $allGaps.Count, ($allGaps -join ', '))
        Write-Output ("GENERATOR-CHECK GAP --verify only ({0}): {1}" -f `
            $verifyOnly.Count, ($verifyOnly -join ', '))
    } elseif ($Group -eq 'Native' -and $nativeGapScripts.Count -gt 0) {
        Write-Output ("GENERATOR-CHECK GAP native no --check artifact arm ({0}): {1}" -f `
            $nativeGapScripts.Count, ($nativeGapScripts -join ', '))
    }
} finally {
    if ($null -eq $previousYoster) {
        Remove-Item Env:\NDS_P2_STAGE_YOSTER -ErrorAction SilentlyContinue
    } else {
        $env:NDS_P2_STAGE_YOSTER = $previousYoster
    }
}
$sweepWatch.Stop()
$mode = if ($IncludeSlow) { 'fast+slow' } else { 'fast' }
if ($script:GeneratorFailures.Count -gt 0) {
    $failedLabels = @($script:GeneratorFailures | ForEach-Object { $_.Label })
    throw ("Generator staleness sweep failed: mode={0} failures={1} [{2}] seconds={3:N2}" -f `
        "$mode/$Group", $script:GeneratorFailures.Count, ($failedLabels -join ', '), `
        $sweepWatch.Elapsed.TotalSeconds)
}
Write-Output ("Generator staleness sweep passed: mode={0} seconds={1:N2}" -f `
    "$mode/$Group", $sweepWatch.Elapsed.TotalSeconds)
