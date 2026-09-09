param(
    [switch]$IncludeSlow,
    [switch]$ReportAll,
    [ValidateSet('All','Core','Native','Fighters')]
    [string]$Group = 'All'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script:GeneratorFailures = @()

function Invoke-GeneratorCheck {
    param(
        [string]$Label,
        [string]$Script,
        [string[]]$Arguments
    )
    $scriptPath = Join-Path $root $Script
    if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
        throw "Generator staleness sweep inventory is stale: missing $Script"
    }
    $watch = [System.Diagnostics.Stopwatch]::StartNew()
    $output = @(& python $scriptPath @Arguments 2>&1 | ForEach-Object { "$_" })
    $exitCode = $LASTEXITCODE
    $watch.Stop()
    foreach ($line in $output) { Write-Output $line }
    Write-Output ("GENERATOR-CHECK {0}: exit={1} seconds={2:N2}" -f `
        $Label, $exitCode, $watch.Elapsed.TotalSeconds)
    if ($exitCode -ne 0) {
        $script:GeneratorFailures += [PSCustomObject]@{
            Label = $Label
            ExitCode = $exitCode
        }
        if ($ReportAll) {
            Write-Output "GENERATOR-CHECK FAILURE: $Label (exit $exitCode)"
            return
        }
        throw "Generator staleness sweep failed at '$Label' (exit $exitCode)."
    }
}

function Test-GeneratorHasCheckArm {
    param([string]$Script)
    $scriptPath = Join-Path $root $Script
    $help = @(& python $scriptPath --help 2>&1 | ForEach-Object { "$_" })
    if ($LASTEXITCODE -ne 0) {
        throw "Generator staleness sweep could not inspect '$Script' --help (exit $LASTEXITCODE)."
    }
    return (($help -join "`n") -match '(?m)(^|\s)--check(?:\s|$)')
}

$sweepWatch = [System.Diagnostics.Stopwatch]::StartNew()
$previousYoster = $env:NDS_P2_STAGE_YOSTER
try {
    if ($Group -in @('All','Core')) {
        # The particle generator's output is feature-stamped. Running its --check arm
        # without the stamp produces a false stale result when Yoster is enabled.
        $particleStamp = Join-Path $root 'src\nds\generated\nds_particle_banks.flags.stamp'
        if (Test-Path -LiteralPath $particleStamp) {
            $stampText = (Get-Content -LiteralPath $particleStamp -Raw).Trim()
            if ($stampText -match '(^|=)1($|\s)') {
                $env:NDS_P2_STAGE_YOSTER = '1'
            } else {
                Remove-Item Env:\NDS_P2_STAGE_YOSTER -ErrorAction SilentlyContinue
            }
        }
        Invoke-GeneratorCheck 'particle-banks' 'scripts/generate_nds_particle_banks.py' @('--check')

        $fastChecks = @(
            [PSCustomObject]@{ Label='fighter-owners'; Script='scripts/fighters/generate_nds_native_owners.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='fighter-owner-images'; Script='scripts/fighters/generate_nds_native_owner_images.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='entry-effects'; Script='scripts/3d_vfx/generate_nds_entry_effects.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='fighter-production-manifest'; Script='scripts/fighters/generate_fighter_production_manifest.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='battle-static-textures'; Script='scripts/generate_battle_playable_static_textures.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='battle-texture-census'; Script='scripts/generate_battle_playable_texture_census.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='renderer-parity-corpus'; Script='scripts/generate_renderer_parity_corpus.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='dreamland-ds-mesh'; Script='scripts/stages/dreamland/generate_dreamland_ds_mesh.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='fgm-cue-decode'; Script='scripts/sfx/export-fgm-cue-wav.py'; Args=@('--check') },
            [PSCustomObject]@{ Label='stage-runtime-rows-dreamland'; Script='scripts/stages/emit_native_stage_runtime_rows.py'; Args=@('--stage', 'dreamland', '--check') },
            [PSCustomObject]@{ Label='fox-gun-source-tables'; Script='scripts/fox_gun_bake.py'; Args=@('--check', 'src/nds/nds_fox_gun.c') }
        )
        foreach ($check in $fastChecks) {
            Invoke-GeneratorCheck $check.Label $check.Script $check.Args
        }
    }

    # Native owner generators are numerous and keep growing. The file set is the
    # inventory; --help decides whether a new one supplies an artifact check arm.
    $nativeGapScripts = @()
    if ($Group -in @('All','Native')) {
        $nativeScripts = @(Get-ChildItem -LiteralPath (Join-Path $root 'scripts\stages') `
            -Filter 'generate_nds_native_*.py' -File | Sort-Object Name)
        foreach ($file in $nativeScripts) {
            $relative = 'scripts/stages/' + $file.Name
            if ($file.Name -eq 'generate_nds_native_stage.py') { continue }
            if (Test-GeneratorHasCheckArm $relative) {
                Invoke-GeneratorCheck ("native-" + $file.BaseName.Substring('generate_nds_native_'.Length)) `
                    $relative @('--check')
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
        Invoke-GeneratorCheck 'fighter-admission-pickup-fileids' `
            'scripts/fighters/admit_fighter.py' @('--fighter', 'pickup-fileids', '--check')
        Write-Output ("GENERATOR-CHECK GAP admit_fighter --check unsupported targets ({0}): {1}" -f `
            $fighterAdmissionNoCheck.Count, ($fighterAdmissionNoCheck -join ', '))
    }

    if ($IncludeSlow -and $Group -in @('All','Core')) {
        Invoke-GeneratorCheck 'native-stage-dreamland-SLOW' `
            'scripts/stages/generate_nds_native_stage.py' @('--check')
        Invoke-GeneratorCheck 'fgm-phase-pack-SLOW' `
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
