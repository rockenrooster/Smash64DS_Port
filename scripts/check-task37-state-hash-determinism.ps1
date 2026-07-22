param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [int]$DelaySeconds = 0
)

# Control experiment for the Task 9 state-hash gate.
#
# The Task 37 bisect ran three unrelated code changes -- the libc memory leaves,
# a single 236-byte float leaf, and three port functions -- against the same
# baseline. All three diverged in EXACTLY the same 692 of 3,892 records, with
# byte-identical burst boundaries (1412..1733, 1992..2327, 2501..2534). Three
# unrelated changes cannot produce one identical failure signature; that is the
# signature of a gate whose own output is not reproducible.
#
# So this runs the SAME ROM twice and diffs the two exports. Nothing about the
# build differs between the runs. Any record that differs here is noise in the
# instrument, and every Task 37 "failure" measured against it is worth exactly
# nothing until it is fixed.
#
# This control should have been run before the gate was ever used to judge a
# change. It is cheap, and it is the difference between "the change is broken"
# and "the ruler is broken".

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$owner = Join-Path $PSScriptRoot `
    'verify-battle-mariofox-gcrunall-loop-harness.ps1'

function Invoke-DeterminismRun {
    param([Parameter(Mandatory=$true)][string]$ExportPath)

    [Environment]::SetEnvironmentVariable('NDS_TASK37_ITCM_LEAVES', '0', 'Process')
    & $owner `
        -MelonDS $MelonDS -Gdb $Gdb -GdbPort $GdbPort `
        -RunnerSlot $RunnerSlot -DelaySeconds $DelaySeconds `
        -BattlePlayable -LiveInputPreview -CPUOpponentProof -MatchLifecycleProof `
        -HardwareTriangles -RendererProfileLevel 0 -IFCommonHybridOamMode 0 `
        -FoxCpuMode 1 -RendererBenchmarkTimeoutSeconds 600 `
        -Task9StateHashMode 1 -Task9StateHashExportPath $ExportPath `
        -Harness 'battle_playable_match_lifecycle' `
        -Target 'smash64ds-task37-state-mask0-lab' `
        -Build 'builds/build-task37-state-mask0-lab' `
        -ExpectedMode 163 -ExpectedHarnessSceneCurr 22 -ExpectedHarnessScenePrev 21 `
        -Label 'Task 9 state hash determinism control' `
        -HarnessSelectMessage 'Determinism control did not select Pupupu VSBattle from Maps.'
    if ($LASTEXITCODE -ne 0) { throw "Determinism control run failed." }
}

$environment = @{
    NDS_RENDERER_FAST_RUN_DEFAULT = '9'
    NDS_SCENE_MIP_CACHE_LAB = '0'
    NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT = '1'
    NDS_DEBUG_HUD = '0'
}
$saved = @{}
foreach ($n in @($environment.Keys) + @('NDS_TASK37_ITCM_LEAVES')) {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n, 'Process')
}
foreach ($n in $environment.Keys) {
    [Environment]::SetEnvironmentVariable($n, $environment[$n], 'Process')
}

$a = Join-Path $root 'artifacts\performance\2026-07-22_task37-determinism-runA.json'
$b = Join-Path $root 'artifacts\performance\2026-07-22_task37-determinism-runB.json'
try {
    Invoke-DeterminismRun -ExportPath $a
    Invoke-DeterminismRun -ExportPath $b
} finally {
    foreach ($n in $saved.Keys) {
        if ($null -eq $saved[$n]) { Remove-Item "Env:$n" -ErrorAction SilentlyContinue }
        else { [Environment]::SetEnvironmentVariable($n, $saved[$n], 'Process') }
    }
}

$ra = (Get-Content -LiteralPath $a -Raw | ConvertFrom-Json)
$rb = (Get-Content -LiteralPath $b -Raw | ConvertFrom-Json)
if ($ra.artifacts.rom.sha256 -cne $rb.artifacts.rom.sha256) {
    throw 'Determinism control accidentally used two different ROMs.'
}
if ($ra.rows.Count -ne $rb.rows.Count) {
    throw "Record count differs between two runs of the SAME ROM: $($ra.rows.Count) vs $($rb.rows.Count)."
}

$first = -1
$differing = 0
for ($i = 0; $i -lt $ra.rows.Count; $i++) {
    if ((@($ra.rows[$i]) -join ',') -cne (@($rb.rows[$i]) -join ',')) {
        if ($first -lt 0) { $first = $i }
        $differing++
    }
}
if ($differing -eq 0) {
    Write-Output ("DETERMINISTIC: two runs of the same ROM produced " +
        "$($ra.rows.Count) identical records. The gate is a valid oracle, and " +
        'the Task 37 divergences are real.')
} else {
    Write-Output ("NOT DETERMINISTIC: $differing of $($ra.rows.Count) records " +
        "($([Math]::Round(100.0*$differing/$ra.rows.Count,1))%) differ between " +
        "two runs of the SAME ROM, first at update $first. The gate cannot " +
        'referee any change until this is fixed, and every Task 37 verdict ' +
        'measured against it is void.')
    exit 2
}
