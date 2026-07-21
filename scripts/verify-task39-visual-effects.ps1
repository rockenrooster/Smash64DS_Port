param(
    [Parameter(Mandatory=$true)][string]$Rom,
    [Parameter(Mandatory=$true)][string]$Elf,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [ValidateRange(10,60)][int]$TimeoutSeconds = 30,
    [string]$Screenshot = '',
    [ValidateRange(0,7)][int]$ExpectedEngagementMask = 7,
    [ValidateSet(0,1)][int]$FoxCpuMode = 1,
    [switch]$ProbeOnly
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$romPath = (Resolve-Path $Rom).Path
$elfPath = (Resolve-Path $Elf).Path
$melonDsPath = Resolve-MelonDSRepoExecutablePath -Root $root -MelonDS $MelonDS
$melonDsDir = Split-Path -Parent $melonDsPath
$config = Join-Path $melonDsDir 'melonDS.toml'
$originalConfig = Get-Content -LiteralPath $config -Raw
$captureEnabled = -not [string]::IsNullOrWhiteSpace($Screenshot)
$emulator = $null
$gdbProcess = $null

try {
    $runtimeConfig = Set-MelonDSDualScreenLayout -Text $originalConfig
    $runtimeConfig = Set-MelonDSTomlValue -Text $runtimeConfig `
        -Section 'Instance0.Gdb' -Key 'Enable' -Value 'true'
    $runtimeConfig = Set-MelonDSTomlValue -Text $runtimeConfig `
        -Section 'Instance0.Gdb' -Key 'Enabled' -Value 'true'
    $runtimeConfig = Set-MelonDSTomlValue -Text $runtimeConfig `
        -Section 'Instance0.Gdb.ARM9' -Key 'BreakOnStartup' -Value 'true'
    $runtimeConfig = Set-MelonDSTomlValue -Text $runtimeConfig `
        -Section 'Instance0.Gdb.ARM9' -Key 'Port' -Value "$GdbPort"
    $runtimeConfig = Set-MelonDSTomlValue -Text $runtimeConfig `
        -Section 'Instance0.Gdb.ARM7' -Key 'BreakOnStartup' -Value 'false'
    $runtimeConfig = Set-MelonDSTomlValue -Text $runtimeConfig `
        -Section 'Instance0.Gdb.ARM7' -Key 'Port' -Value "$($GdbPort + 1)"
    $runtimeConfig = Set-MelonDSTomlRootValue -Text $runtimeConfig `
        -Key 'LimitFPS' -Value $(if ($captureEnabled) { 'true' } else { 'false' })
    $runtimeConfig = $runtimeConfig -replace
        '(?ms)(\[JIT\].*?^Enable\s*=\s*)true\s*$', '${1}false'
    Set-Content -LiteralPath $config -Value $runtimeConfig -NoNewline

    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList "`"$romPath`"" -WorkingDirectory $melonDsDir `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $GdbPort | Out-Null

    $screenshotPath = $null
    if ($captureEnabled) {
        $screenshotPath = if ([System.IO.Path]::IsPathRooted($Screenshot)) {
            [System.IO.Path]::GetFullPath($Screenshot)
        } else {
            [System.IO.Path]::GetFullPath((Join-Path $root $Screenshot))
        }
        $visibilityDir = [System.IO.Path]::GetFullPath(
            (Join-Path $root 'artifacts\visibility'))
        $visibilityPrefix = $visibilityDir.TrimEnd('\', '/') +
            [System.IO.Path]::DirectorySeparatorChar
        if (-not $screenshotPath.StartsWith(
                $visibilityPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Task 39 screenshots must stay under '$visibilityDir'."
        }
        $captureScript = [System.IO.Path]::GetFullPath(
            (Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'))
        Remove-Item -LiteralPath $screenshotPath -Force `
            -ErrorAction SilentlyContinue
    }

    $arguments = @(
        '-q', '-batch', $elfPath,
        '-ex', 'set pagination off',
        '-ex', "target remote localhost:$GdbPort",
        '-ex', 'tbreak scVSBattleStartBattle',
        '-ex', 'continue',
        '-ex', "set variable gNdsBattlePlayableFoxCpuEnabled = $FoxCpuMode"
    )
    if ($ProbeOnly) {
        $arguments += @(
            '-ex', 'tbreak ndsFighterMarioFoxNaturalMotionUpdateEnabled',
            '-ex', 'continue',
            '-ex', 'printf "FXPROBE=%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsBattlePlayableFoxCpuEnabled, gNdsFighterNaturalMotionPrepared, gNdsFighterNaturalMotionUpdateCount, gNdsFighterNaturalMotionResult, gNdsFighterNaturalMotionManagerMask, gNdsFighterNaturalCombatPhase, gNdsFighterNaturalCombatPhaseFrames, gNdsFighterNaturalCombatAttackerSlot, gNdsFighterNaturalCombatVictimSlot, gNdsSceneHarnessMode, gNdsSceneHarnessResult, gNdsTask39FxEngagementMask'
        )
    } else {
        $arguments += @(
            '-ex', "tbreak ndsFighterMarioFoxNaturalMotionUpdateEnabled if gNdsTask39FxEngagementMask == $ExpectedEngagementMask || gNdsFighterNaturalCombatGuardFrames != 0",
            '-ex', 'continue'
        )
    }
    $arguments += @(
        '-ex', 'printf "FXSUMMARY=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsBattlePlayableFoxCpuEnabled, gNdsFighterNaturalCombatAttackerSlot, gNdsFighterNaturalCombatVictimSlot, gNdsFighterNaturalCombatVictimStartPercent, gNdsFighterNaturalCombatVictimFinalPercent, gNdsFighterNaturalCombatGuardOnFrames, gNdsFighterNaturalCombatGuardFrames, gNdsFighterNaturalCombatGuardOffFrames, gNdsTask39FxEngagementMask, gNdsTask39FxHitSparkSpawnCount, gNdsTask39FxFlashDrawCount, gNdsTask39FxShieldDrawCount, gNdsTask39FxArenaRejectCount',
        '-ex', 'printf "FXBUDGET=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsTask39FxSpawnTicks, gNdsTask39FxUpdateTicks, gNdsTask39FxDrawTicks, gNdsTask39FxFrameTicks, gNdsTask39FxMaxFrameTicks, gNdsTask39FxObjVramBytes, gNdsTask39FxObjVramRemaining, gNdsTaskmanArenaAllocFailCount',
        '-ex', 'printf "FXSTATIC=%u,%u,%u,%u,%u,%u,%u,%#x,%u\n", gNdsRendererBattleStaticTextureEnabled, gNdsRendererBattleStaticTexturePrepareCount, gNdsRendererBattleStaticTexturePrepareFailCount, gNdsRendererBattleStaticTexturePreparedCount, gNdsRendererBattleStaticTexturePreparedBytes, gNdsRendererBattleStaticTextureArmCount, gNdsRendererBattleStaticTexturePinnedHitCount, gNdsRendererBattleStaticTextureOwnerMask, gNdsRendererBattleStaticTextureViolationCount',
        '-ex', 'printf "FXARENA=%u,%u,%u\n", gNdsTask39FxArenaBootSize, gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount',
        '-ex', 'echo FXROUTES=\n',
        '-ex', 'x/111ub &gNdsTask39EffectRoutes[0]',
        '-ex', 'echo FXSPAWN=',
        '-ex', 'print gNdsTask39EffectSpawnCount',
        '-ex', 'echo FXORIGINAL=',
        '-ex', 'print gNdsTask39EffectOriginalCount',
        '-ex', 'echo FXSUBSTITUTE=',
        '-ex', 'print gNdsTask39EffectSubstituteCount',
        '-ex', 'echo FXSKIPPED=',
        '-ex', 'print gNdsTask39EffectSkippedCount',
        '-ex', 'detach',
        '-ex', 'quit'
    )
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Gdb
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $arguments) {
        [void]$startInfo.ArgumentList.Add($argument)
    }
    $gdbProcess = [System.Diagnostics.Process]::new()
    $gdbProcess.StartInfo = $startInfo
    [void]$gdbProcess.Start()
    $stdoutTask = $gdbProcess.StandardOutput.ReadToEndAsync()
    $stderrTask = $gdbProcess.StandardError.ReadToEndAsync()
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        $gdbProcess.Kill($true)
        $gdbProcess.WaitForExit()
        $timeoutStdout = $stdoutTask.Result
        $timeoutStderr = $stderrTask.Result
        if ($timeoutStdout) { Write-Output $timeoutStdout.TrimEnd() }
        if ($timeoutStderr) { Write-Warning $timeoutStderr.TrimEnd() }
        throw "Task 39 runtime census timed out after $TimeoutSeconds seconds."
    }
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    if ($stdout) { Write-Output $stdout.TrimEnd() }
    if ($stderr) { Write-Warning $stderr.TrimEnd() }
    if (($gdbProcess.ExitCode -ne 0) -or
        -not $stdout.Contains('FXSUMMARY=') -or
        -not $stdout.Contains('FXSKIPPED=')) {
        throw "Task 39 GDB census failed with exit $($gdbProcess.ExitCode)."
    }
    $summary = [regex]::Match(
        $stdout, '(?m)^FXSUMMARY=([0-9]+(?:,[0-9]+){12})\r?$')
    if (-not $summary.Success) {
        throw 'Task 39 runtime census did not emit a parseable summary.'
    }
    $summaryValues = @($summary.Groups[1].Value.Split(',') |
        ForEach-Object { [uint32]$_ })
    if ($summaryValues[0] -ne [uint32]$FoxCpuMode) {
        throw "Task 39 Fox AI mode mismatch: $($summary.Groups[1].Value)"
    }
    if (-not $ProbeOnly -and
        ($summaryValues[8] -ne [uint32]$ExpectedEngagementMask)) {
        throw "Task 39 engagement mask $($summaryValues[8]) != $ExpectedEngagementMask."
    }
    if ($null -ne $screenshotPath) {
        & $captureScript -EmulatorProcessId $emulator.Id `
            -Output $screenshotPath | Write-Output
    }
    if (($null -ne $screenshotPath) -and
        (-not (Test-Path -LiteralPath $screenshotPath -PathType Leaf))) {
        throw "Task 39 screenshot was not captured: $screenshotPath"
    }
    Write-Output 'TASK39_EFFECT_RUNTIME_CENSUS=PASS'
} finally {
    if (($null -ne $gdbProcess) -and -not $gdbProcess.HasExited) {
        $gdbProcess.Kill($true)
    }
    if (($null -ne $emulator) -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force
        $emulator.WaitForExit()
    }
    Set-Content -LiteralPath $config -Value $originalConfig -NoNewline
}
