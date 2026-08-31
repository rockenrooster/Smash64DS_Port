[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = 5,
    [string]$Build = 'build-bug-fox-laser-colanim',
    [switch]$NoBuild,
    [ValidateRange(30,300)][int]$TimeoutSeconds = 180,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-FoxLaser {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if ($Condition) { return }
    if ($Evidence) { throw "$Message`n$Evidence" }
    throw $Message
}

$target = 'smash64ds-battle-playable-proof-hwtri'
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir "$target.nds"
$elf = Join-Path $buildDir "$target.elf"
$config = Join-Path $buildDir 'nds_build_config.h'
$sceneConfig = Join-Path $buildDir 'nds_scene_harness_config.h'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$Build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_PROOF_FIGHTER0=1' 'NDS_TASK68_FALLBACK_CENSUS=1'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig, $nm)) {
    Assert-FoxLaser (Test-Path -LiteralPath $path -PathType Leaf) `
        "Fox laser proof input is missing: $path"
}
$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_P2_PROOF_FIGHTER0 1',
    '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1',
    '#define NDS_IMPORT_BATTLESHIP_FOX_BLASTER 1'
)) {
    Assert-FoxLaser $configText.Contains($definition) `
        "Fox laser proof build is missing required definition: $definition" $config
}
Assert-FoxLaser $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Fox laser proof must run the mode-163 BattleShip battle harness.' $sceneConfig

$elfSymbols = @(& $nm -a $elf)
Assert-FoxLaser ($LASTEXITCODE -eq 0) "Could not read ELF symbols: $elf"
function Get-ElfSymbolAddress {
    param([Parameter(Mandatory=$true)][string]$Name)
    $escaped = [regex]::Escape($Name)
    $line = $elfSymbols | Where-Object {
        $_ -match "^([0-9a-fA-F]+)\s+\S\s+$escaped$"
    } | Select-Object -First 1
    Assert-FoxLaser ($null -ne $line) "ELF symbol not found: $Name"
    $m = [regex]::Match($line, '^([0-9a-fA-F]+)')
    return [uint32]([Convert]::ToUInt32($m.Groups[1].Value, 16))
}

$pads = Get-ElfSymbolAddress 'sControllerPlaybackPads'
$connected = Get-ElfSymbolAddress 'sControllerPlaybackConnectedMask'
$enabled = Get-ElfSymbolAddress 'sControllerPlaybackEnabled'
$frameComplete = Get-ElfSymbolAddress 'ndsBattlePlayableFrameCompleteMarker'
$specialNSetStatus = Get-ElfSymbolAddress 'ftFoxSpecialNSetStatus'
foreach ($symbol in @(
    'gNdsFoxLaserColAnimSuppressCount',
    'gNdsFoxLaserColAnimPassCount',
    'gSCManagerBattleState'
)) { [void](Get-ElfSymbolAddress $symbol) }

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\verification\2026-08-31_bug-fox-laser-colanim.txt'
} elseif (-not [System.IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

# Source contract:
#   relocData/208_FoxMainMotion.c:1442-1456 Laser and :1458-1466
#   LaserAerial each wait, then issue exactly
#     SetColAnim(nGMColAnimFighterFoxSpecialHiStart, 0)
#   while ftfoxstatus.h maps them to SpecialN (225) / SpecialAirN (226).
# The owner explicitly rejects that cosmetic flash on DS.  This proof changes
# only the external controller pad. BattleShip must select SpecialN itself and
# execute the original motion event; the DS gate must consume that exact event.
$ctx = Initialize-MelonDSVerifierContext -Root $root -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot -NoBuild
$state = $null
$emu = $null
try {
    $state = Enable-MelonDSGdbConfig -MelonDSPath $ctx.MelonDSPath `
        -GdbPort $ctx.GdbPort -Persistent -MuteAudio
    $emu = Start-Process -FilePath $ctx.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path $ctx.MelonDSPath) `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emu -Port $ctx.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $ctx.GdbPort),
        # Install a neutral controller-owned P0 pad, then wait for source Wait.
        ('set {{unsigned int}}0x{0:x8} = 0' -f $pads),
        ('set {{unsigned int}}0x{0:x8} = 0' -f ($pads + 4)),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $connected),
        ('set {{unsigned int}}0x{0:x8} = 1' -f $enabled),
        ('tbreak *0x{0:x8} if (gSCManagerBattleState != 0) && (gSCManagerBattleState->players[0].fighter_gobj != 0) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->fkind == 1) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->status_id == nFTCommonStatusWait) && (((FTStruct *)gSCManagerBattleState->players[0].fighter_gobj->user_data.p)->is_control_disable == 0)' -f $frameComplete),
        'continue',
        'set $fox = (GObj *)gSCManagerBattleState->players[0].fighter_gobj',
        'set $ffp = (FTStruct *)$fox->user_data.p',
        'set $suppress0 = gNdsFoxLaserColAnimSuppressCount',
        'set $pass0 = gNdsFoxLaserColAnimPassCount',
        'printf "FOX_LASER_READY=%d,%d,%u,%u\n", $ffp->status_id, $ffp->motion_id, $suppress0, $pass0',
        # B_BUTTON is 0x4000 in the DS playback pad. Neutral stick forces N,
        # not Up-B/Down-B, and source ftCommonSpecialN chooses the Fox setter.
        ('set {{unsigned short}}0x{0:x8} = 0x4000' -f $pads),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 2)),
        ('set {{signed char}}0x{0:x8} = 0' -f ($pads + 3)),
        ('tbreak *0x{0:x8} if ((GObj *)$r0 == $fox)' -f $specialNSetStatus),
        'continue',
        'printf "FOX_LASER_SETTER=%d,%d\n", $ffp->status_id, $ffp->motion_id',
        # The current input is already latched. Release external B so this is a
        # single natural laser rather than a proof-created repeated-fire loop.
        ('set {{unsigned int}}0x{0:x8} = 0' -f $pads),
        'finish',
        # The tiny static gate is deliberately inlineable and may have no ELF
        # symbol.  Observe its durable counter at the once-per-presented-frame
        # boundary instead: that counter can increment only for Fox/NFox,
        # status SpecialN/SpecialAirN, and the exact source
        # nGMColAnimFighterFoxSpecialHiStart id.
        ('tbreak *0x{0:x8} if (gNdsFoxLaserColAnimSuppressCount > $suppress0) && (($ffp->status_id == 225) || ($ffp->status_id == 226))' -f $frameComplete),
        'continue',
        'printf "FOX_LASER_EVENT=%d,%d,%u,%u\n", $ffp->status_id, $ffp->motion_id, gNdsFoxLaserColAnimSuppressCount-$suppress0, gNdsFoxLaserColAnimPassCount-$pass0',
        # Prove consuming the cosmetic event did not stall the source script.
        ('tbreak *0x{0:x8} if ($ffp->status_id == nFTCommonStatusWait) && (gNdsFoxLaserColAnimSuppressCount > $suppress0)' -f $frameComplete),
        'continue',
        'printf "FOX_LASER_DONE=%d,%d,%u,%u\n", $ffp->status_id, $ffp->motion_id, gNdsFoxLaserColAnimSuppressCount-$suppress0, gNdsFoxLaserColAnimPassCount-$pass0',
        ('set {{unsigned int}}0x{0:x8} = 0' -f $enabled),
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-fox-laser-colanim.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-fox-laser-colanim.gdb.out'
    $stdout = Get-Content -LiteralPath $stdoutPath -Raw
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $stdout

    $ready = [regex]::Match($stdout, 'FOX_LASER_READY=(-?\d+),(-?\d+),(\d+),(\d+)')
    $setter = [regex]::Match($stdout, 'FOX_LASER_SETTER=(-?\d+),(-?\d+)')
    $event = [regex]::Match($stdout, 'FOX_LASER_EVENT=(-?\d+),(-?\d+),(\d+),(\d+)')
    $done = [regex]::Match($stdout, 'FOX_LASER_DONE=(-?\d+),(-?\d+),(\d+),(\d+)')
    Assert-FoxLaser ($ready.Success -and [int]$ready.Groups[1].Value -eq 10) `
        'Fox laser proof never reached controller-owned source Wait.' $stdout
    Assert-FoxLaser $setter.Success 'Neutral B did not call BattleShip ftFoxSpecialNSetStatus.' $stdout
    Assert-FoxLaser ($event.Success -and ([int]$event.Groups[1].Value -in @(225,226)) -and `
        [int]$event.Groups[3].Value -eq 1) `
        'Fox laser did not consume its exact source ColAnim event.' $stdout
    Assert-FoxLaser ($done.Success -and [int]$done.Groups[1].Value -eq 10 -and `
        [int]$done.Groups[3].Value -eq 1) `
        'Fox laser did not return to source Wait after consuming the cosmetic ColAnim event.' $stdout

    Write-Output ('P2 Fox laser ColAnim proof passed: source SpecialN event reached; ' +
        'suppressed exactly once; status returned to Wait.')
    Write-Output $event.Value
    Write-Output $done.Value
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
