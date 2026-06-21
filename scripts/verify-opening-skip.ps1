param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
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
$stdout = Join-Path $logDir 'melonds.skip.stdout.log'
$stderr = Join-Path $logDir 'melonds.skip.stderr.log'
$originalConfig = $null
$emulator = $null

function Invoke-Arm9Gdb {
    param(
        [string]$Name,
        [string[]]$Commands,
        [int]$TimeoutMilliseconds = 15000
    )

    $scriptPath = Join-Path $root "_verify_$Name.gdb"
    $stdoutPath = Join-Path $root "_verify_$Name.gdb.out"
    $stderrPath = Join-Path $root "_verify_$Name.gdb.err"
    [System.IO.File]::WriteAllLines($scriptPath, $Commands)

    Remove-Item $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
    $process = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $scriptPath, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath `
        -PassThru
    if (-not $process.WaitForExit($TimeoutMilliseconds)) {
        $process.Kill()
        $process.WaitForExit()
        throw "GDB phase '$Name' timed out."
    }
    $gdbOut =
        if (Test-Path $stdoutPath) { Get-Content $stdoutPath -Raw }
        else { '' }
    $gdbErr =
        if (Test-Path $stderrPath) { Get-Content $stderrPath -Raw }
    Remove-Item $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
    if ($process.ExitCode -ne 0) {
        throw "GDB phase '$Name' failed (exit $($process.ExitCode)).`n$gdbOut`n$gdbErr"
    }
    return $gdbOut
}

if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Build smash64ds.nds and smash64ds.elf before skip verification.'
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
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
    $emulator = Start-Process -FilePath $melonDsPath -ArgumentList $rom `
        -WorkingDirectory $melonDsDir -WindowStyle Hidden `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -PassThru

    # Attach while Startup/Opening Room is still advancing. Break immediately
    # before the tick-10 callback body, then inject an N64 A-button tap into the
    # real BattleShip controller device that the imported helper scans.
    Start-Sleep -Milliseconds 750
    $listener = $null
    for ($i = 0; $i -lt 20; $i++) {
        $emulator.Refresh()
        if ($emulator.HasExited) {
            throw "melonDS exited before the Opening Room skip GDB attach (exit $($emulator.ExitCode))."
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
    $result = Invoke-Arm9Gdb -Name 'opening_skip' -TimeoutMilliseconds 20000 -Commands @(
        'set pagination off',
        'target remote 127.0.0.1:3333',
        'break mvOpeningRoomFuncRun if sMVOpeningRoomTotalTimeTics >= 9',
        'continue',
        'set variable gSYControllerDevices[0].button_tap = 0x8000',
        'delete breakpoints',
        'break osStopThread if gSCManagerSceneData.scene_curr == 1',
        'continue',
        'printf "SKIP_COUNT=%u\n", gNdsOpeningRoomSkipToTitleCount',
        'printf "ROOM_TICKS=%u\n", gNdsOpeningRoomTickCount',
        'printf "ROOM_CHECKS=%u\n", gNdsOpeningRoomControllerCheckCount',
        'printf "ROOM_RELOC=%#x\n", gNdsOpeningRoomRelocResult',
        'printf "ROOM_RELOC_INIT=%u\n", gNdsOpeningRoomRelocInitCount',
        'printf "ROOM_RELOC_LOAD=%u\n", gNdsOpeningRoomRelocLoadCount',
        'printf "ROOM_RELOC_MASK=%#x\n", gNdsOpeningRoomRelocFileMask',
        'printf "ROOM_RELOC_HEADER_MASK=%#x\n", gNdsOpeningRoomRelocHeaderMask',
        'printf "ROOM_RELOC_PAYLOAD_MASK=%#x\n", gNdsOpeningRoomRelocPayloadMask',
        'printf "ROOM_RELOC_DATA=%u\n", gNdsOpeningRoomRelocContentReady',
        'printf "ROOM_RELOC_FIXUP=%u\n", gNdsOpeningRoomRelocFixupReady',
        'printf "ROOM_RELOC_BYTES=%u\n", gNdsOpeningRoomRelocBytesLoaded',
        'printf "ROOM_RELOC_WORD_SWAP_MASK=%#x\n", gNdsOpeningRoomRelocWordSwapMask',
        'printf "ROOM_RELOC_WORD_SWAP_COUNT=%u\n", gNdsOpeningRoomRelocWordSwapCount',
        'printf "ROOM_RELOC_WORD_SWAP_FAILS=%u\n", gNdsOpeningRoomRelocWordSwapFailCount',
        'printf "ROOM_RELOC_POINTER_MASK=%#x\n", gNdsOpeningRoomRelocPointerFixupMask',
        'printf "ROOM_RELOC_POINTER_COUNT=%u\n", gNdsOpeningRoomRelocPointerFixupCount',
        'printf "ROOM_RELOC_POINTER_FAILS=%u\n", gNdsOpeningRoomRelocPointerFixupFailCount',
        'printf "ROOM_RELOC_SYMBOL_COUNT=%u\n", gNdsOpeningRoomRelocSymbolResolveCount',
        'printf "ROOM_RELOC_SYMBOL_FAILS=%u\n", gNdsOpeningRoomRelocSymbolResolveFailCount',
        'printf "ROOM_RELOC_SYMBOL_PROBE_MASK=%#x\n", gNdsOpeningRoomRelocSymbolProbeMask',
        'printf "ROOM_FIRST_EVENT=%#x\n", gNdsOpeningRoomFirstEventResult',
        'printf "ROOM_FIRST_EVENT_TICK=%u\n", gNdsOpeningRoomFirstEventTick',
        'printf "ROOM_FIRST_EVENT_MASK=%#x\n", gNdsOpeningRoomFirstEventProbeMask',
        'printf "ROOM_FIRST_EVENT_PENCILS_DOBJ=%u\n", gNdsOpeningRoomFirstEventPencilsDObjOffset',
        'printf "ROOM_FIRST_EVENT_PENCILS_ANIM=%u\n", gNdsOpeningRoomFirstEventPencilsAnimOffset',
        'printf "ROOM_FIRST_EVENT_DATA=%#x\n", gNdsOpeningRoomFirstEventDataResult',
        'printf "ROOM_FIRST_EVENT_DATA_MASK=%#x\n", gNdsOpeningRoomFirstEventDataMask',
        'printf "ROOM_FIRST_EVENT_DOBJ_ENTRIES=%u\n", gNdsOpeningRoomFirstEventPencilsDObjEntries',
        'printf "ROOM_FIRST_EVENT_DOBJ_DLS=%u\n", gNdsOpeningRoomFirstEventPencilsDLPtrs',
        'printf "ROOM_FIRST_EVENT_ANIM_JOINTS=%u\n", gNdsOpeningRoomFirstEventPencilsAnimJoints',
        'printf "ROOM_FIRST_EVENT_ANIM_OPCODE=%u\n", gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode',
        'printf "RELOC_NITRO=%#x\n", gNdsRelocAssetInitResult',
        'printf "RELOC_PAYLOADS=%u\n", gNdsRelocAssetPayloadReadCount',
        'printf "SCENE_CURR=%u\n", gSCManagerSceneData.scene_curr',
        'printf "SCENE_PREV=%u\n", gSCManagerSceneData.scene_prev',
        'printf "BOUNDARY=%#x\n", gNdsSceneBoundaryResult',
        'printf "BOUNDARY_KIND=%u\n", gNdsSceneBoundaryKind',
        'printf "TASKMAN_RETURNS=%u\n", gNdsTaskmanReturnCount',
        'printf "PRE_ASSET=%#x\n", gNdsOpeningRoomPreAssetResult',
        'printf "TITLE_RELOC=%#x\n", gNdsTitleRelocResult',
        'printf "TITLE_PREVIEW=%#x\n", gNdsTitlePreviewResult',
        'printf "TITLE_DRAW=%#x\n", gNdsTitleDrawResult',
        'printf "TITLE_SPRITE_NORM=%u,%u\n", gNdsTitleSpriteNormalizeCount, gNdsTitleSpriteNormalizeFailCount',
        'printf "TITLE_FIRE_SPRITE_NORM=%u,%u\n", gNdsTitleFireSpriteNormalizeCount, gNdsTitleFireSpriteNormalizeFailCount',
        'printf "TITLE_DRAW_COUNTS=%u,%u,%u,%u\n", gNdsTitleDrawVisibleSObjCount, gNdsTitleDrawRenderableSObjCount, gNdsTitleDrawSObjCount, gNdsTitleDrawPixels',
        'printf "TITLE_LOGO_FIRE=%#x,%#x,%u,%u,%u,%u,%u\n", gNdsTitleOriginalLogoFireResult, gNdsTitleOriginalLogoFireMask, gNdsTitleOriginalLogoFireGObjDelta, gNdsTitleOriginalLogoFireLinkID, gNdsTitleOriginalLogoFireDLLinkID, gNdsTitleOriginalLogoFireCameraMaskLo, gNdsTitleOriginalLogoFireParticleBank',
        'printf "TITLE_FIRE=%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsTitleOriginalFireResult, gNdsTitleOriginalFireMask, gNdsTitleOriginalFireGObjDelta, gNdsTitleOriginalFireSObjDelta, gNdsTitleOriginalFireGObjFlags, gNdsTitleOriginalFireSObjCount, gNdsTitleOriginalFireFrames, gNdsTitleOriginalFireAlpha',
        'printf "TITLE_ORIGINAL_UPDATE=%#x,%u,%u,%u,%u,%u,%u\n", gNdsTitleOriginalUpdateResult, gNdsTitleOriginalUpdateCount, gNdsTitleOriginalLayout, gNdsTitleOriginalTransitionTics, gNdsTitleOriginalStartActorProcess, gNdsTitleOriginalProceedScene, gNdsTitleOriginalProceedWait',
        'detach'
    )

    $expected = @{
        SKIP_COUNT = '1'
        SCENE_CURR = '1'
        SCENE_PREV = '28'
        BOUNDARY = '0x53434e45'
        BOUNDARY_KIND = '1'
        TASKMAN_RETURNS = '2'
        PRE_ASSET = '0'
        ROOM_RELOC = '0x4f52524c'
        ROOM_RELOC_INIT = '1'
        ROOM_RELOC_LOAD = '8'
        ROOM_RELOC_MASK = '0xff'
        ROOM_RELOC_HEADER_MASK = '0xff'
        ROOM_RELOC_PAYLOAD_MASK = '0xff'
        ROOM_RELOC_DATA = '1'
        ROOM_RELOC_FIXUP = '0'
        ROOM_RELOC_BYTES = '329248'
        ROOM_RELOC_WORD_SWAP_MASK = '0xff'
        ROOM_RELOC_WORD_SWAP_COUNT = '82312'
        ROOM_RELOC_WORD_SWAP_FAILS = '0'
        ROOM_RELOC_POINTER_MASK = '0xff'
        ROOM_RELOC_POINTER_COUNT = '711'
        ROOM_RELOC_POINTER_FAILS = '0'
        ROOM_RELOC_SYMBOL_COUNT = '43'
        ROOM_RELOC_SYMBOL_FAILS = '0'
        ROOM_RELOC_SYMBOL_PROBE_MASK = '0x7ffff'
        ROOM_FIRST_EVENT = '0x4f524631'
        ROOM_FIRST_EVENT_TICK = '280'
        ROOM_FIRST_EVENT_MASK = '0x3'
        ROOM_FIRST_EVENT_PENCILS_DOBJ = '44728'
        ROOM_FIRST_EVENT_PENCILS_ANIM = '44912'
        ROOM_FIRST_EVENT_DATA = '0x4f524644'
        ROOM_FIRST_EVENT_DATA_MASK = '0xf'
        ROOM_FIRST_EVENT_DOBJ_ENTRIES = '4'
        ROOM_FIRST_EVENT_DOBJ_DLS = '3'
        ROOM_FIRST_EVENT_ANIM_JOINTS = '3'
        ROOM_FIRST_EVENT_ANIM_OPCODE = '3'
        RELOC_NITRO = '0x4e465349'
        RELOC_PAYLOADS = '12'
        TITLE_RELOC = '0x5449524c'
        TITLE_PREVIEW = '0x54495056'
        TITLE_DRAW = '0x54494457'
        TITLE_FIRE_SPRITE_NORM = '30,0'
        TITLE_LOGO_FIRE = '0x544c4643,0x3f,1,4,3,1,0'
        TITLE_FIRE = '0x54464952,0xfff,1,2,0,2,786432,255'
        TITLE_ORIGINAL_UPDATE = '0,0,1,169,0,0,3'
    }
    foreach ($name in $expected.Keys) {
        $match = [regex]::Match($result, "$name=([^\r\n]+)")
        if (-not $match.Success -or
            $match.Groups[1].Value.ToLowerInvariant() -ne $expected[$name]) {
            throw "Opening Room skip check '$name' failed.`n$result"
        }
    }

    $roomTicksMatch = [regex]::Match($result, 'ROOM_TICKS=([0-9]+)')
    $roomChecksMatch = [regex]::Match($result, 'ROOM_CHECKS=([0-9]+)')
    if (-not $roomTicksMatch.Success -or -not $roomChecksMatch.Success) {
        throw "Opening Room skip timing diagnostics were missing.`n$result"
    }
    $roomTicks = [int]$roomTicksMatch.Groups[1].Value
    $roomChecks = [int]$roomChecksMatch.Groups[1].Value
    if ($roomTicks -lt 10 -or $roomTicks -ge 279 -or
        $roomChecks -ne ($roomTicks - 9)) {
        throw "Opening Room skip was outside the original controller-gated interval.`n$result"
    }

    $titleSpriteNorm = [regex]::Match($result, 'TITLE_SPRITE_NORM=([0-9]+),([0-9]+)')
    $titleDrawCounts = [regex]::Match($result, 'TITLE_DRAW_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    if (-not $titleSpriteNorm.Success -or
        [int]$titleSpriteNorm.Groups[1].Value -ne 10 -or
        [int]$titleSpriteNorm.Groups[2].Value -ne 0 -or
        -not $titleDrawCounts.Success -or
        [int]$titleDrawCounts.Groups[1].Value -ne 10 -or
        [int]$titleDrawCounts.Groups[2].Value -ne 10 -or
        [int]$titleDrawCounts.Groups[3].Value -ne 10 -or
        [int]$titleDrawCounts.Groups[4].Value -le 1000) {
        throw "Opening Room skip reached Title but did not render the bounded MNTitle preview.`n$result"
    }

    Write-Output "Opening Room skip verification passed (tick $roomTicks -> Title)."
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
    Remove-Item $stdout, $stderr, `
        (Join-Path $root '_verify_opening_skip.gdb'), `
        (Join-Path $root '_verify_opening_skip.gdb.out'), `
        (Join-Path $root '_verify_opening_skip.gdb.err') `
        -Force -ErrorAction SilentlyContinue
}
