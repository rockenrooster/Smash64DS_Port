[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 180,
    [string]$Build = 'build-p2-ness-special-tour-fast',
    [string]$Artifact = '',
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Assert-NessSpecial {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if (-not $Condition) {
        if ($Evidence) { throw "$Message`n$Evidence" }
        throw $Message
    }
}

$target = 'smash64ds-battle-playable-fast-hwtri'
$buildDir = Join-Path $root (Join-Path 'builds' $Build)
$rom = Join-Path $buildDir ($target + '.nds')
$elf = Join-Path $buildDir ($target + '.elf')
$config = Join-Path $buildDir 'nds_build_config.h'
$sceneConfig = Join-Path $buildDir 'nds_scene_harness_config.h'

if (-not $NoBuild) {
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    & make -C $root "TARGET=$target" "BUILD=builds/$Build" `
        'NDS_P2_NESS=1' 'NDS_P2_PROOF_FIGHTER0=11' `
        'NDS_HARNESS_FAST_PRESENT_ON_REQUEST=1' 'NDS_R2_FOX_CPU_DEFAULT=0'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $config, $sceneConfig)) {
    Assert-NessSpecial (Test-Path -LiteralPath $path -PathType Leaf) `
        "Ness special proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $config -Raw
$sceneText = Get-Content -LiteralPath $sceneConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 0',
    '#define NDS_HARNESS_FAST_LOGIC 1',
    '#define NDS_HARNESS_FAST_PRESENT_ON_REQUEST 1',
    '#define NDS_P2_NESS 1',
    '#define NDS_P2_PROOF_FIGHTER0 11',
    '#define NDS_R2_FOX_CPU_DEFAULT 0',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1'
)) {
    Assert-NessSpecial $configText.Contains($definition) `
        "Ness special proof build is missing required definition: $definition" $config
}
Assert-NessSpecial $sceneText.Contains('#define NDS_DEV_SCENE_HARNESS 163') `
    'Ness special proof must run the mode-163 battle_playable harness.' $sceneConfig

# The verifier may supply only controller input and observe state. This static
# guard prevents the bounded tour from quietly becoming a state-injection test.
$movementPath = Join-Path $root 'src\port\reloc_backend_movement.c'
$movementText = Get-Content -LiteralPath $movementPath -Raw
$tourStart = $movementText.IndexOf('static sb32 ndsNessSpecialTourApplyInput')
$tourEnd = $movementText.IndexOf(
    'static sb32 ndsFighterNaturalCombatRecoverTeeter', $tourStart)
Assert-NessSpecial (($tourStart -ge 0) -and ($tourEnd -gt $tourStart)) `
    'Could not isolate the Ness special guest proof driver.'
$tourText = $movementText.Substring($tourStart, $tourEnd - $tourStart)
foreach ($forbidden in @(
    'ftMainSetStatus\s*\(',
    'ftNess[A-Za-z0-9_]*SetStatus\s*\(',
    'wpNess[A-Za-z0-9_]*MakeWeapon\s*\(',
    'itNess[A-Za-z0-9_]*MakeItem\s*\(',
    'efManager[A-Za-z0-9_]*MakeEffect\s*\(',
    'gcEjectGObj\s*\(',
    'ftCommonWaitSetStatus\s*\(',
    'ftParamUnlockPlayerControl\s*\(',
    'ness->status_id\s*=(?!=)',
    'ness->motion_id\s*=(?!=)'
)) {
    Assert-NessSpecial ($tourText -notmatch $forbidden) `
        "Ness special proof contains forbidden state injection: $forbidden"
}

$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$requiredSymbols = @(
    'ndsNessSpecialTourProofStop',
    'gNdsNessSpecialTourDone',
    'gNdsNessSpecialTourStatusMask',
    'gNdsNessSpecialTourPKFireParticleDrawObserved',
    'gNdsNessSpecialTourPKThunderHeadObserved',
    'gNdsNessSpecialTourPKThunderTrailObserved',
    'gNdsNessSpecialTourPsychicMagnetCollOffset',
    'gNdsParticleBankNessID',
    'gNdsParticleNessScriptsPacked'
)
$missing = @($requiredSymbols | Where-Object { $symbols -notcontains $_ })
Assert-NessSpecial ($missing.Count -eq 0) `
    ('Ness special proof symbols are missing: ' + ($missing -join ', '))

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_ness-specials.txt')
} elseif (-not [IO.Path]::IsPathRooted($Artifact)) {
    $Artifact = Join-Path $root $Artifact
}

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
        'set $tour_stop = 0',
        'set $fault = 0',
        'break __excpt_entry',
        'commands',
        'silent',
        'set $fault = 1',
        'printf "NESS_EXCEPTION pc=%#x lr=%#x phase=%u frames=%u input=%u mask=%#x\n", $pc,$lr,gNdsNessSpecialTourPhase,gNdsNessSpecialTourFrames,gNdsNessSpecialTourInputCount,gNdsNessSpecialTourStatusMask',
        'bt 12',
        'end',
        'tbreak ndsNessSpecialTourProofStop',
        'commands',
        'silent',
        'set $tour_stop = 1',
        'end',
        'continue',
        'printf "NESS_STOPPED=%u fault=%u\n", $tour_stop,$fault',
        'printf "NESS_TOUR=%u,%u,%u,%#x,%u\n", gNdsNessSpecialTourPhase,gNdsNessSpecialTourFrames,gNdsNessSpecialTourInputCount,gNdsNessSpecialTourStatusMask,gNdsNessSpecialTourDone',
        'printf "NESS_PKFIRE=%u,%u,%u,%u,%u,%u,%u\n", gNdsNessSpecialTourPKFireWeaponObserved,gNdsNessSpecialTourPKFireWeaponMObjObserved,gNdsNessSpecialTourPKFireWeaponDrawObserved,gNdsNessSpecialTourPKFireItemObserved,gNdsNessSpecialTourPKFireItemDrawObserved,gNdsNessSpecialTourPKFireParticleObserved,gNdsNessSpecialTourPKFireParticleDrawObserved',
        'printf "NESS_PKTHUNDER=%u,%u,%u,%u\n", gNdsNessSpecialTourPKThunderHeadObserved,gNdsNessSpecialTourPKThunderHeadMObjObserved,gNdsNessSpecialTourPKThunderTrailObserved,gNdsNessSpecialTourPKThunderTrailMObjObserved',
        'printf "NESS_MAGNET=%u,%#x,%u\n", gNdsNessSpecialTourPsychicMagnetObserved,gNdsNessSpecialTourPsychicMagnetCollOffset,gNdsNessSpecialTourEffectAttachObserved',
        'printf "NESS_SAFETY=%u,%u,%u,%u,%u,%u,%u\n", gNdsNessSpecialTourWeaponRefusalDelta,gNdsNessSpecialTourWeaponHighWater,gNdsNessSpecialTourNativeFailureDelta,gNdsNessSpecialTourFighterRejectDelta,gNdsNessSpecialTourWeaponRejectDelta,gNdsNessSpecialTourItemRejectDelta,gNdsNessSpecialTourEffectRejectDelta',
        'printf "NESS_RENDER=%u,%u,%u,%u\n", gNdsParticleSubmitOkCount,gNdsParticleSubmitFailCount,gNdsHarnessFastPresentRequestCount,gNdsHarnessFastPresentConsumeCount',
        'printf "NESS_EFDESC=%u,%u,%u,%u,%u\n", gNdsEFDescDeferRecoverCount,gNdsEFDescDisabledCount,gNdsEFDescUnknownFileCount,gNdsEFDescDeferOverflowCount,gNdsEFDescResolveCount',
        'printf "NESS_PARTICLE_BANK=%u,%u\n", gNdsParticleBankNessID,gNdsParticleNessScriptsPacked',
        'detach',
        'quit'
    )
    Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
        -Commands $commands -ScriptName 'p2-ness-specials.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null

    $outPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-ness-specials.gdb.out'
    $errPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR 'p2-ness-specials.gdb.err'
    $stdout = Get-Content -LiteralPath $outPath -Raw
    $stderr = if (Test-Path -LiteralPath $errPath) { Get-Content -LiteralPath $errPath -Raw } else { '' }

    Assert-NessSpecial ($stdout -match 'NESS_STOPPED=1 fault=0') `
        'Ness special proof did not finish without an ARM exception.' ($stdout + $stderr)

    $tourMatch = [regex]::Match($stdout, 'NESS_TOUR=([^\r\n]+)')
    Assert-NessSpecial $tourMatch.Success 'Ness tour marker is missing.' $stdout
    $tour = @($tourMatch.Groups[1].Value.Split(','))
    Assert-NessSpecial ($tour.Count -eq 5) 'Ness tour marker field count changed.' $tourMatch.Value
    $mask = if ($tour[3].StartsWith('0x')) { [Convert]::ToUInt32($tour[3].Substring(2), 16) } else { [uint32]$tour[3] }
    Assert-NessSpecial (([uint32]$tour[0] -eq 12) -and
        ([uint32]$tour[2] -eq 3) -and (($mask -band 0x7f) -eq 0x7f) -and
        ([uint32]$tour[4] -eq 1)) `
        'Ness did not naturally complete Neutral-B -> Up-B -> Down-B.' $tourMatch.Value

    $pkFireMatch = [regex]::Match($stdout, 'NESS_PKFIRE=([^\r\n]+)')
    Assert-NessSpecial $pkFireMatch.Success 'PK Fire marker is missing.' $stdout
    $pk = @($pkFireMatch.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    Assert-NessSpecial (($pk.Count -eq 7) -and ($pk[3] -eq 1) -and
        ($pk[5] -eq 1) -and ($pk[6] -eq 1)) `
        'PK Fire did not create its source pillar item and visible particle path.' $pkFireMatch.Value

    $thunderMatch = [regex]::Match($stdout, 'NESS_PKTHUNDER=([^\r\n]+)')
    Assert-NessSpecial $thunderMatch.Success 'PK Thunder marker is missing.' $stdout
    $thunder = @($thunderMatch.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    Assert-NessSpecial (($thunder.Count -eq 4) -and ($thunder[0] -eq 1) -and
        ($thunder[2] -eq 1) -and ($thunder[3] -eq 1)) `
        'PK Thunder did not survive through source head/trail ownership.' $thunderMatch.Value

    $magnetMatch = [regex]::Match($stdout, 'NESS_MAGNET=([^\r\n]+)')
    Assert-NessSpecial $magnetMatch.Success 'PSI Magnet marker is missing.' $stdout
    $mag = @($magnetMatch.Groups[1].Value.Split(','))
    $collOffset = if ($mag[1].StartsWith('0x')) { [Convert]::ToUInt32($mag[1].Substring(2), 16) } else { [uint32]$mag[1] }
    Assert-NessSpecial (($mag.Count -eq 3) -and ([uint32]$mag[0] -eq 1) -and
        ($collOffset -eq 0x16d4) -and ([uint32]$mag[2] -eq 1)) `
        'PSI Magnet did not reach Hold with the source absorb collision/effect.' $magnetMatch.Value

    $safetyMatch = [regex]::Match($stdout, 'NESS_SAFETY=([^\r\n]+)')
    Assert-NessSpecial $safetyMatch.Success 'Ness safety marker is missing.' $stdout
    $safety = @($safetyMatch.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    Assert-NessSpecial (($safety.Count -eq 7) -and ($safety[0] -eq 0)) `
        'Ness special tour exhausted/refused the weapon pool.' $safetyMatch.Value

    $renderMatch = [regex]::Match($stdout, 'NESS_RENDER=([^\r\n]+)')
    Assert-NessSpecial $renderMatch.Success 'Ness particle render marker is missing.' $stdout
    $render = @($renderMatch.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    Assert-NessSpecial (($render.Count -eq 4) -and ($render[0] -gt 0) -and
        ($render[1] -eq 0) -and ($render[2] -gt 0) -and
        ($render[3] -eq $render[2])) `
        'Ness particle/presentation path did not submit cleanly.' $renderMatch.Value

    $efMatch = [regex]::Match($stdout, 'NESS_EFDESC=([^\r\n]+)')
    Assert-NessSpecial $efMatch.Success 'Ness EFDesc marker is missing.' $stdout
    $ef = @($efMatch.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    Assert-NessSpecial (($ef.Count -eq 5) -and ($ef[0] -gt 0) -and ($ef[3] -eq 0)) `
        'Ness late-loaded effect descriptors did not recover cleanly.' $efMatch.Value

    $bankMatch = [regex]::Match($stdout, 'NESS_PARTICLE_BANK=([^\r\n]+)')
    Assert-NessSpecial $bankMatch.Success 'Ness particle-bank marker is missing.' $stdout
    $bank = @($bankMatch.Groups[1].Value.Split(',') | ForEach-Object { [uint32]$_ })
    Assert-NessSpecial (($bank.Count -eq 2) -and ($bank[0] -ne 0xff) -and ($bank[1] -eq 4)) `
        'Ness particle bank did not register all four source scripts.' $bankMatch.Value

    $summary = 'P2-3 Ness specials passed: controller Neutral-B produced the source PK Fire pillar particle; grounded Up-B completed PK Thunder head/trail; grounded Down-B held/released PSI Magnet with source collision offset 0x16D4.'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    @(
        $summary,
        "ROM_SHA256=$((Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash)",
        "BUILD=$Build",
        "TARGET=$target",
        $tourMatch.Value,
        $pkFireMatch.Value,
        $thunderMatch.Value,
        $magnetMatch.Value,
        $safetyMatch.Value,
        $renderMatch.Value,
        $efMatch.Value,
        $bankMatch.Value
    ) | Set-Content -LiteralPath $Artifact
    Write-Output $summary
    Get-Content -LiteralPath $Artifact | Select-Object -Skip 4
}
finally {
    if (($null -ne $emu) -and -not $emu.HasExited) {
        Stop-Process -Id $emu.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $state) { Restore-MelonDSGdbConfig -State $state }
}
