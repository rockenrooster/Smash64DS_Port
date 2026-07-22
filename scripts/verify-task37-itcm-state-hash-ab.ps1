param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 0,
    # First entry is the baseline every other mask is compared against.
    [int[]]$Masks = @(0, 1, 2, 4),
    # Falsify BGM (NDS_BGM_FALSIFIER_OFF=1) so the asynchronous audio stream is
    # removed from the comparison. BGM refill is driven by real elapsed time, not
    # by logic ticks, and this harness runs fast-logic with no realtime pacing --
    # so a build that runs faster reaches a different BGM position at the same
    # logic update. If the gate passes with this on and fails with it off, the
    # divergence was never gameplay.
    [switch]$BgmOff,
    # Bytes of never-executed padding to add to .main instead of moving code.
    # Run this against mask 0 to tell "ITCM residency breaks it" apart from
    # "any layout change breaks it".
    [int]$LayoutProbe = 0,
    # Put the probe padding in .itcm instead of .main.
    [switch]$ProbeItcm,
    # Run VBlank-paced (NDS_HARNESS_FAST_LOGIC=0) instead of fast-logic.
    #
    # Under fast logic the ARM9 runs unpaced while the ARM7 -- input and IPC --
    # keeps real time, so a faster ARM9 samples gSYControllerDevices at a
    # different phase. That state IS hashed. Realtime pacing pins the phase to
    # VBlank, which is how the game actually runs, so a faster ARM9 simply idles
    # longer. If the gate passes here and fails under fast logic, the divergence
    # is the harness sampling input differently, not the port computing a
    # different match.
    [switch]$Realtime,
    # Drop gSYControllerDevices from the hash. Diagnostic only: if the gate
    # passes with this and fails without, the divergence is the harness sampling
    # ARM7 input at a speed-dependent phase, not the port computing a different
    # match.
    [switch]$NoControllers,
    # Hex bitmask of NDSTask9StateRecordKind values to include in the hash.
    # Splits the state so the divergent half identifies itself instead of being
    # guessed at one region per run.
    [string]$RegionMask = ''
)

# Task 37 exactness gate, bisecting.
#
# Task 37 moves seven measured hot leaves from .main into ITCM and claims to
# change nothing else. The 0-vs-7 run on 2026-07-22 disproved that: 692 of 3,892
# per-update game-state records differed, and the owner independently confirmed
# the enabled lab build does not play correctly.
#
# NDS_TASK37_ITCM_LEAVES is therefore a bitmask, and this runs one full match
# lifecycle per mask so the responsible group names itself:
#   1  libc leaves   memset, memcpy, memcmp
#   2  libm leaf     __ieee754_sqrtf
#   4  port leaves   TextureSourceBytes, PolyFmt, FTParamsInvalidateFighterParts
#
# A clean group is one whose every per-update record matches the baseline. Do not
# weaken that to "mostly matches": the 0-vs-7 failure re-converged three times,
# which is exactly how a real defect can look like noise if you squint.

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$owner = Join-Path $PSScriptRoot `
    'verify-battle-mariofox-gcrunall-loop-harness.ps1'

$suffix = if ($BgmOff) { '-bgmoff' } else { '' }
if ($ProbeItcm) { $suffix += '-itcmpad' }
if ($Realtime) { $suffix += '-rt' }
if ($NoControllers) { $suffix += '-noctl' }
if ($RegionMask) { $suffix += "-rm$($RegionMask -replace '^0x','')" }

function Get-Task37ExportPath {
    param([int]$Mask, [int]$Probe = 0)
    $tag = if ($Probe -gt 0) { "$suffix-p$Probe" } else { $suffix }
    return (Join-Path $root "artifacts\performance\2026-07-22_task37-state-mask$Mask$tag.json")
}

function Invoke-Task37StateRun {
    param(
        [Parameter(Mandatory=$true)][int]$Mask,
        [int]$Probe = 0
    )

    [Environment]::SetEnvironmentVariable('NDS_TASK37_LAYOUT_PROBE', "$Probe", 'Process')
    $tag = if ($Probe -gt 0) { "$suffix-p$Probe" } else { $suffix }
    $target = "smash64ds-task37-state-mask$Mask$tag-lab"
    $build = "builds/build-task37-state-mask$Mask$tag-lab"
    $export = Get-Task37ExportPath -Mask $Mask -Probe $Probe

    # NDS_TASK37_ITCM_LEAVES is declared with ?= so the environment wins.
    [Environment]::SetEnvironmentVariable(
        'NDS_TASK37_ITCM_LEAVES', "$Mask", 'Process')
    & $owner `
        -MelonDS $MelonDS `
        -Gdb $Gdb `
        -GdbPort $GdbPort `
        -RunnerSlot $RunnerSlot `
        -NoBuild:$NoBuild `
        -DelaySeconds $DelaySeconds `
        -BattlePlayable `
        -LiveInputPreview `
        -CPUOpponentProof `
        -MatchLifecycleProof `
        -RealtimePresentation:$Realtime `
        -HardwareTriangles `
        -RendererProfileLevel 0 `
        -IFCommonHybridOamMode 0 `
        -FoxCpuMode 1 `
        -RendererBenchmarkTimeoutSeconds 600 `
        -Task9StateHashMode 1 `
        -Task9StateHashExportPath $export `
        -Harness 'battle_playable_match_lifecycle' `
        -Target $target `
        -Build $build `
        -ExpectedMode 163 `
        -ExpectedHarnessSceneCurr 22 `
        -ExpectedHarnessScenePrev 21 `
        -Label "Task 37 state hash ITCM mask=$Mask" `
        -HarnessSelectMessage 'Task 37 state hash run did not select Pupupu VSBattle from Maps.'
    if ($LASTEXITCODE -ne 0) {
        throw "Task 37 state hash run mask=$Mask failed."
    }
    return $export
}

$environment = @{
    NDS_BGM_FALSIFIER_OFF = $(if ($BgmOff) { '1' } else { '0' })
    NDS_TASK9_STATE_HASH_SKIP_CONTROLLERS = $(if ($NoControllers) { '1' } else { '0' })
    NDS_TASK9_STATE_HASH_REGION_MASK = $(if ($RegionMask) { $RegionMask } else { '0xFFFFFFFF' })
    NDS_TASK37_LAYOUT_PROBE = "$LayoutProbe"
    NDS_TASK37_LAYOUT_PROBE_ITCM = $(if ($ProbeItcm) { '1' } else { '0' })
    NDS_RENDERER_FAST_RUN_DEFAULT = '9'
    NDS_SCENE_MIP_CACHE_LAB = '0'
    NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT = '1'
    NDS_DEBUG_HUD = '0'
}
$savedEnvironment = @{}
foreach ($name in @($environment.Keys) + @('NDS_TASK37_ITCM_LEAVES')) {
    $savedEnvironment[$name] =
        [Environment]::GetEnvironmentVariable($name, 'Process')
}
foreach ($name in $environment.Keys) {
    [Environment]::SetEnvironmentVariable(
        $name, $environment[$name], 'Process')
}
try {
    foreach ($mask in $Masks) { Invoke-Task37StateRun -Mask $mask | Out-Null }
    if ($LayoutProbe -gt 0) {
        Invoke-Task37StateRun -Mask $Masks[0] -Probe $LayoutProbe | Out-Null
    }
} finally {
    foreach ($name in $savedEnvironment.Keys) {
        if ($null -eq $savedEnvironment[$name]) {
            Remove-Item "Env:$name" -ErrorAction SilentlyContinue
        } else {
            [Environment]::SetEnvironmentVariable(
                $name, $savedEnvironment[$name], 'Process')
        }
    }
}

$baselineMask = $Masks[0]
$baseline = Get-Content -LiteralPath (Get-Task37ExportPath -Mask $baselineMask) `
    -Raw | ConvertFrom-Json
if ($baseline.rows.Count -lt 3600) {
    throw "Task 37 baseline covered only $($baseline.rows.Count) updates."
}

$failed = @()
$arms = @()
foreach ($m in ($Masks | Select-Object -Skip 1)) { $arms += ,@($m, 0) }
if ($LayoutProbe -gt 0) { $arms += ,@($Masks[0], $LayoutProbe) }
foreach ($arm in $arms) {
    $mask = $arm[0]; $probe = $arm[1]
    $where = if ($ProbeItcm) { '.itcm' } else { '.main' }
    $label = if ($probe -gt 0) { "mask $mask + ${probe}B dead $where padding (no function moved)" } else { "mask $mask" }
    $candidate = Get-Content -LiteralPath (Get-Task37ExportPath -Mask $mask -Probe $probe) `
        -Raw | ConvertFrom-Json
    if ($baseline.artifacts.rom.sha256 -ceq $candidate.artifacts.rom.sha256) {
        throw "Task 37 arm '$label' produced the same ROM as the baseline."
    }
    if ($baseline.rows.Count -ne $candidate.rows.Count) {
        Write-Output ("$label  FAIL  record count " +
            "$($candidate.rows.Count) vs baseline $($baseline.rows.Count)")
        $failed += $mask
        continue
    }
    $first = -1
    $differing = 0
    for ($i = 0; $i -lt $baseline.rows.Count; $i++) {
        if ((@($baseline.rows[$i]) -join ',') -cne (@($candidate.rows[$i]) -join ',')) {
            if ($first -lt 0) { $first = $i }
            $differing++
        }
    }
    if ($differing -eq 0) {
        Write-Output ("$label  PASS  $($baseline.rows.Count) records identical")
    } else {
        Write-Output ("$label  FAIL  $differing of $($baseline.rows.Count) " +
            "records differ, first at update $first")
        $failed += $label
    }
}

if ($failed.Count -gt 0) {
    throw ("Task 37 placement is not behaviour-neutral for mask(s): " +
        ($failed -join ', '))
}
Write-Output 'Task 37 placement exactness gate PASS for every mask tested.'
