param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$DelaySeconds = 5
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$rom = Join-Path $root 'smash64ds-title.nds'
$elf = Join-Path $root 'smash64ds-title.elf'
$melonDsPath = if ([System.IO.Path]::IsPathRooted($MelonDS)) {
    $MelonDS
} else {
    Join-Path $root $MelonDS
}
$melonDsDir = Split-Path -Parent $melonDsPath
$config = Join-Path $melonDsDir 'melonDS.toml'
$logDir = Join-Path $root 'artifacts\emulator-logs'
$stdout = Join-Path $logDir 'melonds.title-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.title-harness.stderr.log'
$gdbScriptPath = Join-Path $root '_title_harness.gdb'
$gdbStdoutPath = Join-Path $root '_title_harness.gdb.out'
$gdbStderrPath = Join-Path $root '_title_harness.gdb.err'
$originalConfig = $null
$emulator = $null

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

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

& make -C $root TARGET=smash64ds-title BUILD=build-title-harness NDS_DEV_SCENE_HARNESS=title -j4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Title harness build did not produce smash64ds-title.nds and smash64ds-title.elf.'
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
    if ($enabled -notmatch "$gdbSectionPattern\bEnabled\s*=\s*true" -or
        $enabled -notmatch "$gdbSectionPattern\bEnable\s*=\s*true") {
        throw 'Could not enable the melonDS ARM9 GDB stub.'
    }
    Set-Content $config -Value $enabled -NoNewline

    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru

    $listener = $null
    for ($i = 0; $i -lt 60; $i++) {
        $emulator.Refresh()
        if ($emulator.HasExited) {
            throw "melonDS exited before title harness sample (exit $($emulator.ExitCode))."
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
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev',
        'printf "ROOM=%u\n", gNdsOpeningRoomTickCount',
        'printf "OPENING_ACTION=%u,%u\n", gNdsOpeningMovieActionPreviewCount, gNdsOpeningMovieActionPreviewFrameCount',
        'printf "TITLE_DRAW=%#x\n", gNdsTitleDrawResult',
        'printf "TITLE_ORIGINAL=%#x,%#x,%#x,%u,%u,%u,%#x\n", gNdsTitleOriginalStartResult, gNdsTitleOriginalFuncStartResult, gNdsTitleOriginalSetupMask, gNdsTitleOriginalLoadedFileCount, gNdsTitleOriginalGObjCount, gNdsTitleOriginalCameraCount, gNdsTitleOriginalDeferredMask',
        'printf "TITLE_UPDATE=%#x,%u,%u,%u,%u,%u,%u\n", gNdsTitleOriginalUpdateResult, gNdsTitleOriginalUpdateCount, gNdsTitleOriginalLayout, gNdsTitleOriginalTransitionTics, gNdsTitleOriginalStartActorProcess, gNdsTitleOriginalProceedScene, gNdsTitleOriginalProceedWait',
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

    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+)')
    $room = [int](Get-MatchValue $gdbStdout 'ROOM=([0-9]+)')
    $openingAction = [regex]::Match($gdbStdout, 'OPENING_ACTION=([0-9]+),([0-9]+)')
    $titleDraw = Get-MatchValue $gdbStdout 'TITLE_DRAW=(0x[0-9a-fA-F]+|0)'
    $titleOriginal = [regex]::Match($gdbStdout, 'TITLE_ORIGINAL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $titleUpdate = [regex]::Match($gdbStdout, 'TITLE_UPDATE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')

    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 1 -or
        [int]$harn.Groups[3].Value -ne 1 -or
        [int]$harn.Groups[4].Value -ne 46 -or
        (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Title harness marker did not select the imported Title boundary.`n$gdbStdout"
    }
    if (-not $scene.Success -or
        [int]$scene.Groups[1].Value -ne 1 -or
        [int]$scene.Groups[2].Value -ne 46) {
        throw "Live scene state is not Title from OpeningNewcomers.`n$gdbStdout"
    }
    if ($room -ne 0) {
        throw "Title harness replayed Opening Room before Title (room tick $room).`n$gdbStdout"
    }
    if (-not $openingAction.Success -or
        [int]$openingAction.Groups[1].Value -ne 0 -or
        [int]$openingAction.Groups[2].Value -ne 0) {
        throw "Title harness unexpectedly ran opening movie action previews.`n$gdbStdout"
    }
    if ((Convert-MarkerUInt32 $titleDraw) -ne 0x54494457) {
        throw "Title harness did not reach bounded Title draw marker.`n$gdbStdout"
    }
    if (-not $titleOriginal.Success -or
        $titleOriginal.Groups[1].Value.ToLowerInvariant() -ne '0x54495354' -or
        $titleOriginal.Groups[2].Value.ToLowerInvariant() -ne '0x54494653' -or
        (Convert-MarkerUInt32 $titleOriginal.Groups[3].Value) -ne 0xf -or
        [int]$titleOriginal.Groups[4].Value -ne 2 -or
        [int]$titleOriginal.Groups[5].Value -lt 2 -or
        [int]$titleOriginal.Groups[6].Value -ne 4) {
        throw "Imported original Title setup did not run from the harness.`n$gdbStdout"
    }
    if (-not $titleUpdate.Success -or
        $titleUpdate.Groups[1].Value.ToLowerInvariant() -ne '0x54495550' -or
        [int]$titleUpdate.Groups[2].Value -ne 1) {
        throw "Harness did not run one bounded original Title update.`n$gdbStdout"
    }

    Write-Output ("Title harness passed: scene={0}/{1} room={2} title={3}" -f
        $scene.Groups[1].Value, $scene.Groups[2].Value, $room, $titleDraw)
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
    } elseif (Test-Path $config) {
        Remove-Item $config -Force -ErrorAction SilentlyContinue
    }
    Remove-Item $stdout, $stderr, $gdbScriptPath, $gdbStdoutPath, $gdbStderrPath `
        -Force -ErrorAction SilentlyContinue
}
