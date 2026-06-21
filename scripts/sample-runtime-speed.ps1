param(
    [switch]$Build,
    [switch]$Visible,
    [switch]$RequireTitle,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$DelaySeconds = 15,
    [uint32]$ExpectedTitleMarker = 0x54494457,
    [int]$MinRoomTick = 0,
    [int]$MinActionScenes = 0,
    [int]$MinActionFrames = 0,
    [double]$MinHostFps = 0.0
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$rom = Join-Path $root 'smash64ds.nds'
$elf = Join-Path $root 'smash64ds.elf'
$melonDsPath = if ([System.IO.Path]::IsPathRooted($MelonDS)) {
    $MelonDS
} else {
    Join-Path $root $MelonDS
}
$melonDsDir = Split-Path -Parent $melonDsPath
$config = Join-Path $melonDsDir 'melonDS.toml'
$logDir = Join-Path $root 'artifacts\emulator-logs'
$stdout = Join-Path $logDir 'melonds.speed.stdout.log'
$stderr = Join-Path $logDir 'melonds.speed.stderr.log'
$gdbScriptPath = Join-Path $root '_speed_sample.gdb'
$gdbStdoutPath = Join-Path $root '_speed_sample.gdb.out'
$gdbStderrPath = Join-Path $root '_speed_sample.gdb.err'
$originalConfig = $null
$emulator = $null
$stopwatch = $null

function Get-MatchValue {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Fallback = '0'
    )

    $match = [regex]::Match($Text, $Pattern)
    if ($match.Success) {
        return $match.Groups[1].Value
    }
    return $Fallback
}

function Convert-MarkerUInt32 {
    param([string]$Value)

    if ($Value.StartsWith('0x')) {
        return [Convert]::ToUInt32($Value.Substring(2), 16)
    }
    return [Convert]::ToUInt32($Value, 10)
}

if ($Build) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C $root -j4
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Build smash64ds.nds and smash64ds.elf before runtime speed sampling.'
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
if (-not (Test-Path $Gdb)) {
    throw "GDB executable not found: $Gdb"
}
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

try {
    if (Test-Path $config) {
        $originalConfig = Get-Content $config -Raw
    } else {
        $seed = Start-Process -FilePath $melonDsPath -ArgumentList $rom `
            -WorkingDirectory $melonDsDir -WindowStyle Hidden -PassThru
        Start-Sleep -Seconds 1
        if (-not $seed.HasExited) { Stop-Process -Id $seed.Id -Force }
        Start-Sleep -Milliseconds 250
    }

    $text = Get-Content $config -Raw
    $gdbSectionPattern = '(?s)\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?'
    $enabled = $text -replace `
        '(?s)(\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?\bEnabled\s*=\s*)false', `
        '${1}true'
    $enabled = $enabled -replace `
        '(?s)(\[Instance0\.Gdb\](?:(?!\r?\n\[).)*?\bEnable\s*=\s*)false', `
        '${1}true'
    if ($enabled -notmatch "$gdbSectionPattern\bEnabled\s*=\s*true") {
        $enabled = $enabled -replace `
            '(?m)(^\[Instance0\.Gdb\]\s*\r?\n)', `
            "`$1Enabled = true`r`n"
    }
    if ($enabled -notmatch "$gdbSectionPattern\bEnable\s*=\s*true") {
        $enabled = $enabled -replace `
            '(?m)(^\[Instance0\.Gdb\]\s*\r?\n)', `
            "`$1Enable = true`r`n"
    }
    Set-Content $config -Value $enabled -NoNewline

    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $startInfo = @{
        FilePath = $melonDsPath
        ArgumentList = $rom
        WorkingDirectory = $melonDsDir
        RedirectStandardOutput = $stdout
        RedirectStandardError = $stderr
        PassThru = $true
    }
    if (-not $Visible) {
        $startInfo.WindowStyle = 'Hidden'
    }
    $emulator = Start-Process @startInfo
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

    $listener = $null
    for ($i = 0; $i -lt 60; $i++) {
        $emulator.Refresh()
        if ($emulator.HasExited) {
            throw "melonDS exited before speed sample (exit $($emulator.ExitCode))."
        }
        $listener = Get-NetTCPConnection -LocalPort 3333 -State Listen `
            -ErrorAction SilentlyContinue |
            Where-Object { $_.OwningProcess -eq $emulator.Id } |
            Select-Object -First 1
        if ($null -ne $listener) {
            break
        }
        Start-Sleep -Milliseconds 250
    }
    if ($null -eq $listener) {
        throw 'melonDS did not open the ARM9 GDB listener on 127.0.0.1:3333.'
    }

    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, 1))

    $gdbCommands = @(
        'set pagination off',
        'set remotetimeout 5',
        'target remote 127.0.0.1:3333',
        'printf "FRAMES=%u\n", gNdsFrameCounter',
        'printf "PRESENT=%u\n", gNdsOpeningMoviePresentFrameCount',
        'printf "PERF_FPS=%u,%u,%u,%u,%u\n", gNdsPerfPresentFps, gNdsPerfLogicFps, gNdsPerfDLDrawFps, gNdsPerfSampleCount, gNdsPerfSampleWindowTicks',
        'printf "PERF_CONTENT=%u,%u,%u,%u\n", gNdsOriginalSpritePreviewCommitCount, gNdsOriginalDLPreviewCommitCount, gNdsPerfPreviewCommitFps, gNdsPerfPreviewCommitCount',
        'printf "ROOM_DRAW=%u,%u\n", gNdsOpeningRoomDrawProbeCount, gNdsOpeningRoomDrawReuseCount',
        'printf "ROOM=%u\n", gNdsOpeningRoomTickCount',
        'printf "PORTRAITS=%u\n", gNdsOpeningPortraitsTickCount',
        'printf "MARIO=%u\n", gNdsOpeningMarioTickCount',
        'printf "ACTION=%u,%u\n", gNdsOpeningMovieActionPreviewCount, gNdsOpeningMovieActionPreviewFrameCount',
        'printf "TITLE=%#x\n", gNdsTitleDrawResult',
        'detach'
    )
    [System.IO.File]::WriteAllLines($gdbScriptPath, $gdbCommands)

    Remove-Item $gdbStdoutPath, $gdbStderrPath -Force -ErrorAction SilentlyContinue
    $gdbproc = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScriptPath, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbStdoutPath `
        -RedirectStandardError $gdbStderrPath `
        -PassThru
    $gdbproc.WaitForExit()
    $gdbStdout =
        if (Test-Path $gdbStdoutPath) { Get-Content $gdbStdoutPath -Raw }
        else { '' }
    $gdbStderr =
        if (Test-Path $gdbStderrPath) { Get-Content $gdbStderrPath -Raw }
        else { '' }
    if ($gdbproc.ExitCode -ne 0 -or [string]::IsNullOrEmpty($gdbStdout)) {
        throw ("GDB did not complete successfully (exit $($gdbproc.ExitCode))." +
               "`nstdout:$gdbStdout`nstderr:$gdbStderr")
    }

    $elapsed = [Math]::Max($stopwatch.Elapsed.TotalSeconds, 0.001)
    $frames = [int](Get-MatchValue $gdbStdout 'FRAMES=([0-9]+)')
    $present = [int](Get-MatchValue $gdbStdout 'PRESENT=([0-9]+)')
    $perf = [regex]::Match($gdbStdout, 'PERF_FPS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $content = [regex]::Match($gdbStdout, 'PERF_CONTENT=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomDraw = [regex]::Match($gdbStdout, 'ROOM_DRAW=([0-9]+),([0-9]+)')
    $room = Get-MatchValue $gdbStdout 'ROOM=([0-9]+)'
    $portraits = Get-MatchValue $gdbStdout 'PORTRAITS=([0-9]+)'
    $mario = Get-MatchValue $gdbStdout 'MARIO=([0-9]+)'
    $action = [regex]::Match($gdbStdout, 'ACTION=([0-9]+),([0-9]+)')
    $title = Get-MatchValue $gdbStdout 'TITLE=(0x[0-9a-fA-F]+|0)'

    $romFps = if ($perf.Success) { $perf.Groups[1].Value } else { '0' }
    $logicFps = if ($perf.Success) { $perf.Groups[2].Value } else { '0' }
    $dlFps = if ($perf.Success) { $perf.Groups[3].Value } else { '0' }
    $contentFps = if ($content.Success) { $content.Groups[3].Value } else { '0' }
    $contentTotal = if ($content.Success) { $content.Groups[4].Value } else { '0' }
    $roomDrawProbes = if ($roomDraw.Success) { $roomDraw.Groups[1].Value } else { '0' }
    $roomDrawReuse = if ($roomDraw.Success) { $roomDraw.Groups[2].Value } else { '0' }
    $actionCount = if ($action.Success) { $action.Groups[1].Value } else { '0' }
    $actionFrames = if ($action.Success) { $action.Groups[2].Value } else { '0' }
    $hostFps = [double]$frames / $elapsed

    $summaryFormat =
        "Runtime speed sample ({0:N1}s): frames={1} hostfps={2:N2} " +
        "romfps={3} up={4} dl={5} cv={6} ch={7} present={8} " +
        "room={9} rdraw={10}/{11} portraits={12} mario={13} " +
        "action={14}/{15} title={16}"
    Write-Output ($summaryFormat -f
                  $elapsed, $frames, $hostFps, $romFps, $logicFps, $dlFps,
                  $contentFps, $contentTotal, $present, $room,
                  $roomDrawProbes, $roomDrawReuse, $portraits, $mario,
                  $actionCount, $actionFrames, $title)

    $titleValue = Convert-MarkerUInt32 $title
    if ($RequireTitle -and $titleValue -ne $ExpectedTitleMarker) {
        throw ("Expected Title marker 0x{0:x8}, got {1}." -f
               $ExpectedTitleMarker, $title)
    }
    if ($MinRoomTick -gt 0 -and [int]$room -lt $MinRoomTick) {
        throw "Expected Opening Room tick >= $MinRoomTick, got $room."
    }
    if ($MinActionScenes -gt 0 -and [int]$actionCount -lt $MinActionScenes) {
        throw "Expected action preview count >= $MinActionScenes, got $actionCount."
    }
    if ($MinActionFrames -gt 0 -and [int]$actionFrames -lt $MinActionFrames) {
        throw "Expected action preview frames >= $MinActionFrames, got $actionFrames."
    }
    if ($MinHostFps -gt 0.0 -and $hostFps -lt $MinHostFps) {
        throw ("Expected hostfps >= {0:N2}, got {1:N2}." -f
               $MinHostFps, $hostFps)
    }
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    if ($null -ne $originalConfig) {
        Set-Content $config -Value $originalConfig -NoNewline
    } else {
        Remove-Item $config -Force -ErrorAction SilentlyContinue
    }
    Remove-Item $stdout, $stderr, $gdbScriptPath, $gdbStdoutPath, $gdbStderrPath `
        -Force -ErrorAction SilentlyContinue
}
