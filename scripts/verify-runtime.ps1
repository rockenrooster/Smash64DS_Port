[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [string]$Rom = (Join-Path $PSScriptRoot '..\smash64ds.nds'),
    [string]$Elf = (Join-Path $PSScriptRoot '..\smash64ds.elf'),
    [string]$BuildConfig = (Join-Path $PSScriptRoot '..\builds\build\nds_build_config.h'),
    [ValidateRange(15,300)][int]$TimeoutSeconds = 60
)

# Latest's normal-startup arm follows the shipping P2 boot directly to Title.
# The retired opening-movie chain and its fixed 135-second sleep did not run
# in that configuration. Synchronize on scene entry and actual title frames.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
foreach ($path in @($Rom, $Elf, $Gdb, $BuildConfig)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Runtime verification input is missing: $path"
    }
}
# melonDS starts in its runner directory. Resolve caller-relative paths before
# that directory change, otherwise a valid ROM is reported as a missing listener.
$Rom = (Resolve-Path -LiteralPath $Rom).Path
$Elf = (Resolve-Path -LiteralPath $Elf).Path
$Gdb = (Resolve-Path -LiteralPath $Gdb).Path
$BuildConfig = (Resolve-Path -LiteralPath $BuildConfig).Path
$buildFlags = Get-Content -LiteralPath $BuildConfig -Raw
$livePreview = [regex]::Match($buildFlags, '(?m)^#define NDS_DEV_LIVE_INPUT_PREVIEW\s+([01])\b')
if (-not $livePreview.Success) { throw 'Build config lacks the controller admission flag.' }
# controller_backend.c:osContInit exposes ports 0/1 for the live battle bridge.
$expectedControllers = 1 + [int]$livePreview.Groups[1].Value
$context = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') -NoBuild:$NoBuild
$emulator = $null
$configState = $null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent:($RunnerSlot -ge 0) `
        -BreakOnStartup -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList ('"{0}"' -f $Rom) -WorkingDirectory (Split-Path $context.MelonDSPath -Parent) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 10',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsMenuShellRunTitle',
        'continue',
        'delete',
        'break ndsPlatformEndFrame',
        'condition $bpnum gNdsMenuShellFrames[0] >= 120',
        'continue',
        'printf "P2BOOT self=%#x boot=%#x video=%#x nitro=%#x controllers=%u polls=%u\n", gNdsBootSelfTestResult, gNdsOriginalBootStage, gNdsVideoBootstrapResult, gNdsRelocAssetInitResult, gSYControllerConnectedNum, gNdsControllerPollCount',
        'printf "P2TITLE screen=%u frames=%u fire=%u animation=%u pack=%u\n", gNdsMenuShellScreen, gNdsMenuShellFrames[0], gNdsTitleFireFrameCount, gNdsUiKitTitleAnimFrameCount, gNdsUiKitPackBytesLoaded',
        'printf "P2FAIL open=%u format=%u short=%u pack=%u surface=%u animation=%u scene=%u cpsr=%#x\n", gNdsRelocAssetOpenFailCount, gNdsRelocAssetFormatFailCount, gNdsRelocAssetShortReadCount, gNdsUiKitPackReadFailCount, gNdsUiKitSurfaceReadFailCount, gNdsUiKitTitleAnimLoadFailCount, gNdsSceneManagerRejectCount, $cpsr',
        'detach',
        'quit'
    )
    $capture = Invoke-GdbMarkerScript -Gdb $Gdb -Elf $Elf -Root $root `
        -Commands $commands -ScriptName 'verify_p2_startup.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    $boot = [regex]::Match($capture.Stdout,
        'P2BOOT self=(0x[0-9a-f]+) boot=(0x[0-9a-f]+) video=(0x[0-9a-f]+) nitro=(0x[0-9a-f]+) controllers=(\d+) polls=(\d+)')
    $title = [regex]::Match($capture.Stdout,
        'P2TITLE screen=(\d+) frames=(\d+) fire=(\d+) animation=(\d+) pack=(\d+)')
    $fail = [regex]::Match($capture.Stdout,
        'P2FAIL open=(\d+) format=(\d+) short=(\d+) pack=(\d+) surface=(\d+) animation=(\d+) scene=(\d+) cpsr=(0x[0-9a-f]+)')
    if (-not $boot.Success -or -not $title.Success -or -not $fail.Success) {
        throw "Startup markers missing. See $($capture.StdoutPath) and $($capture.StderrPath)."
    }
    if ($boot.Groups[1].Value -ne '0x50415353' -or
        $boot.Groups[2].Value -ne '0x53430007' -or
        $boot.Groups[3].Value -ne '0x56494430' -or
        $boot.Groups[4].Value -ne '0x4e465349' -or
        [int]$boot.Groups[5].Value -ne $expectedControllers -or [int]$boot.Groups[6].Value -le 0) {
        throw "P2 boot/controller initialization failed: $($boot.Value)"
    }
    if ([int]$title.Groups[1].Value -ne 0 -or [int]$title.Groups[2].Value -lt 120 -or
        [int]$title.Groups[3].Value -le 0 -or [int]$title.Groups[4].Value -le 0 -or
        [int]$title.Groups[5].Value -le 0) {
        throw "P2 title did not present its loaded animation: $($title.Value)"
    }
    foreach ($index in 1..7) {
        if ([uint32]$fail.Groups[$index].Value -ne 0u) {
            throw "P2 startup reported an asset/scene failure: $($fail.Value)"
        }
    }
    $mode = (Convert-MarkerUInt32 $fail.Groups[8].Value) -band 0x1f
    if ($mode -in @(0x17, 0x1b)) {
        throw "P2 startup entered an ARM abort/undefined mode: $($fail.Value)"
    }
    Write-Host "P2_RUNTIME_OK $($boot.Value) $($title.Value) $($fail.Value)"
}
finally {
    if ($null -ne $emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $configState) { Restore-MelonDSGdbConfig -State $configState }
}
