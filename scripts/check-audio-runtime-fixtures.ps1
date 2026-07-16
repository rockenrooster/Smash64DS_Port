param(
    [string]$BattleShipRoot = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if ([string]::IsNullOrWhiteSpace($BattleShipRoot)) {
    $BattleShipRoot = Join-Path $root 'decomp/BattleShip-main/decomp'
}
$BattleShipRoot = (Resolve-Path -LiteralPath $BattleShipRoot).Path

function Find-BytePattern {
    param(
        [byte[]]$Data,
        [byte[]]$Pattern
    )

    $hits = [System.Collections.Generic.List[int]]::new()
    for ($offset = 0; $offset -le ($Data.Length - $Pattern.Length);
         $offset++) {
        $matches = $true
        for ($index = 0; $index -lt $Pattern.Length; $index++) {
            if ($Data[$offset + $index] -ne $Pattern[$index]) {
                $matches = $false
                break
            }
        }
        if ($matches) {
            $hits.Add($offset)
        }
    }
    return $hits.ToArray()
}

function Read-NormalizedU16Word {
    param(
        [byte[]]$Data,
        [int]$Offset
    )

    # The loader's blanket u32 byte swap makes [3,2,1,0]. The audited
    # mixed-u16 repair then exchanges those two native lanes.
    $wordSwap = @(
        $Data[$Offset + 3], $Data[$Offset + 2],
        $Data[$Offset + 1], $Data[$Offset]
    )
    $laneRepair = @(
        $wordSwap[2], $wordSwap[3], $wordSwap[0], $wordSwap[1]
    )
    return @(
        ([int]$laneRepair[0] -bor ([int]$laneRepair[1] -shl 8)),
        ([int]$laneRepair[2] -bor ([int]$laneRepair[3] -shl 8))
    )
}

function Assert-EqualList {
    param(
        [string]$Label,
        [int[]]$Actual,
        [int[]]$Expected
    )

    if (($Actual -join ',') -ne ($Expected -join ',')) {
        throw ('{0} differs: got [{1}], expected [{2}].' -f
            $Label, ($Actual -join ', '), ($Expected -join ', '))
    }
}

function Get-SourceFunctionBlock {
    param(
        [string]$Source,
        [string]$Name
    )

    $match = [regex]::Match(
        $Source,
        ('(?s)void\s+{0}\s*\([^)]*\)\s*\{{.*?(?=\r?\n// 0x|\z)' -f
            [regex]::Escape($Name))
    )
    if (-not $match.Success) {
        throw "BattleShip source function was not found: $Name"
    }
    return $match.Value
}

function Assert-OrderedTokens {
    param(
        [string]$Label,
        [string]$Source,
        [string[]]$Tokens
    )

    $cursor = 0
    foreach ($token in $Tokens) {
        $found = $Source.IndexOf($token, $cursor,
            [StringComparison]::Ordinal)
        if ($found -lt 0) {
            throw "$Label source order is missing: $token"
        }
        $cursor = $found + $token.Length
    }
}

$fixtures = @(
    @{
        Name = 'MarioMain'
        Prefix = [byte[]]@(
            0x01, 0xB7, 0x01, 0x24, 0x01, 0xB1, 0x01, 0xB8,
            0x01, 0xAD, 0x01, 0xAE, 0x01, 0xAF, 0x00, 0x00
        )
        Audio = [int[]]@(439, 292, 433, 440, 429, 430, 431, 0)
        Heavy = [int[]]@(438, 0)
    },
    @{
        Name = 'FoxMain'
        Prefix = [byte[]]@(
            0x01, 0x72, 0x01, 0x21, 0x01, 0x68, 0x01, 0x77,
            0x01, 0x76, 0x01, 0x74, 0x01, 0x75, 0x00, 0x00
        )
        Audio = [int[]]@(370, 289, 360, 375, 374, 372, 373, 0)
        Heavy = [int[]]@(0x2B7, 0)
    }
)

foreach ($fixture in $fixtures) {
    $assetPath = Join-Path $BattleShipRoot (
        'BattleShip_o2r/reloc_fighters_main/{0}' -f $fixture.Name)
    if (-not (Test-Path -LiteralPath $assetPath -PathType Leaf)) {
        throw "BattleShip O2R fighter fixture not found: $assetPath"
    }
    $data = [IO.File]::ReadAllBytes($assetPath)
    $hits = @(Find-BytePattern -Data $data -Pattern $fixture.Prefix)
    if ($hits.Count -ne 1) {
        throw ('{0} FTAttributes signature count is {1}, expected 1.' -f
            $fixture.Name, $hits.Count)
    }

    $audio = [System.Collections.Generic.List[int]]::new()
    foreach ($relativeOffset in @(0, 4, 8, 12)) {
        foreach ($value in (Read-NormalizedU16Word `
                -Data $data -Offset ($hits[0] + $relativeOffset))) {
            $audio.Add($value)
        }
    }
    Assert-EqualList -Label "$($fixture.Name) normalized audio attributes" `
        -Actual $audio.ToArray() -Expected $fixture.Audio
    Assert-EqualList -Label "$($fixture.Name) normalized item-throw pair" `
        -Actual (Read-NormalizedU16Word -Data $data `
            -Offset ($hits[0] + 0x30)) -Expected ([int[]]@(100, 100))
    Assert-EqualList -Label "$($fixture.Name) normalized heavy-get word" `
        -Actual (Read-NormalizedU16Word -Data $data `
        -Offset ($hits[0] + 0x34)) -Expected $fixture.Heavy
}

$ftDataText = Get-Content -LiteralPath (
    Join-Path $BattleShipRoot 'src/ft/ftdata.c') -Raw
foreach ($sourceOffset in @(
        @{ Fighter = 'Mario'; Offset = '0x00000428' },
        @{ Fighter = 'Fox'; Offset = '0x0000046C' }
    )) {
    $dataBlock = [regex]::Match(
        $ftDataText,
        ('(?s)FTData dFT{0}Data\s*=\s*\{{.*?\}};' -f
            $sourceOffset.Fighter)
    )
    if (-not $dataBlock.Success -or
        -not $dataBlock.Value.Contains($sourceOffset.Offset)) {
        throw ('BattleShip dFT{0}Data does not retain attribute offset {1}.' -f
            $sourceOffset.Fighter, $sourceOffset.Offset)
    }
}

$marioMotionText = Get-Content -LiteralPath (Join-Path $BattleShipRoot `
    'src/relocData/202_MarioMainMotion.c') -Raw
foreach ($fixture in @(
        @{ Call = 'ftMotionPlayFGM(nSYAudioFGMMarioLanding)'; Count = 7 },
        @{ Call = 'ftMotionPlayVoice(nSYAudioVoiceMarioSmash1)'; Count = 4 },
        @{ Call = 'ftMotionPlayVoice(nSYAudioVoiceMarioJump)'; Count = 3 }
    )) {
    $count = [regex]::Matches(
        $marioMotionText, [regex]::Escape($fixture.Call)).Count
    if ($count -ne $fixture.Count) {
        throw ("BattleShip Mario motion trigger count changed: {0} ({1})" -f
            $fixture.Call, $count)
    }
}

$marioMainText = Get-Content -LiteralPath (Join-Path $BattleShipRoot `
    'src/relocData/203_MarioMain.c') -Raw
if (-not $marioMainText.Contains(
        '{ nSYAudioVoiceMarioSmash1, nSYAudioVoiceMarioSmash2, nSYAudioVoiceMarioSmash3 }, /* smash_sfx */')) {
    throw 'BattleShip Mario random smash-voice table changed.'
}
$ftMainText = Get-Content -LiteralPath (Join-Path $BattleShipRoot `
    'src/ft/ftmain.c') -Raw
if (-not $ftMainText.Contains(
        'ftParamPlayVoice(fp, fp->attr->smash_sfx[syUtilsRandIntRange(ARRAY_COUNT(fp->attr->smash_sfx))]);')) {
    throw 'BattleShip random smash-voice dispatch changed.'
}

$deadSourcePath = Join-Path $BattleShipRoot 'src/ft/ftcommon/ftcommondead.c'
$deadSourceText = Get-Content -LiteralPath $deadSourcePath -Raw
$deadInit = Get-SourceFunctionBlock -Source $deadSourceText `
    -Name 'ftCommonDeadInitStatusVars'
Assert-OrderedTokens -Label 'BattleShip regular-KO fighter audio' `
    -Source $deadInit -Tokens @(
        'ftCommonDeadAddDeadSFXSoundQueue(fp->attr->dead_fgm_ids[0]);',
        'ftCommonDeadAddDeadSFXSoundQueue(fp->attr->dead_fgm_ids[1]);'
    )
foreach ($functionName in @(
        'ftCommonDeadDownSetStatus',
        'ftCommonDeadRightSetStatus',
        'ftCommonDeadLeftSetStatus'
    )) {
    $block = Get-SourceFunctionBlock -Source $deadSourceText `
        -Name $functionName
    Assert-OrderedTokens -Label "BattleShip $functionName audio" `
        -Source $block -Tokens @(
            'ftCommonDeadInitStatusVars(fighter_gobj);',
            'else sfx_id = nSYAudioFGMDeadExplodeL;',
            'ftCommonDeadAddDeadSFXSoundQueue(sfx_id);'
        )
}
$rebirthBlock = Get-SourceFunctionBlock -Source $deadSourceText `
    -Name 'ftCommonDeadCheckRebirth'
foreach ($unexpected in @(
        'func_800269C0_275C0(',
        'ftCommonDeadAddDeadSFXSoundQueue('
    )) {
    if ($rebirthBlock.Contains($unexpected)) {
        throw "BattleShip rebirth path gained an explicit audio call: $unexpected"
    }
}

$attackMotionFixtures = @(
    @{
        Path = 'src/relocData/202_MarioMainMotion.c'
        Tokens = @(
            'ftMotionPlayFGM(nSYAudioFGMCatch)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMLightSwingL)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMLightSwingM)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMLightSwingS)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMMarioUnkSwing2)',
            'ftMotionPlayFGM(nSYAudioFGMMarioSpecialN)',
            'ftMotionPlayFGM(nSYAudioFGMMarioSpecialHiJump)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMMarioUnkSwing1)'
        )
    },
    @{
        Path = 'src/relocData/208_FoxMainMotion.c'
        Tokens = @(
            'ftMotionPlayFGM(nSYAudioFGMCatch)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMLightSwingL)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMLightSwingM)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMLightSwingS)',
            'ftMotionCommandPlayFGMStoreInfo(nSYAudioFGMFoxAttackAirLw)',
            'ftMotionPlayFGM(nSYAudioFGMFoxSpecialN)',
            'ftMotionPlayFGM(nSYAudioFGMFoxSpecialHiStart)',
            'ftMotionPlayFGM(nSYAudioFGMFoxSpecialHiFly)',
            'ftMotionPlayFGM(nSYAudioFGMFoxSpecialLwStart)'
        )
    }
)
foreach ($fixture in $attackMotionFixtures) {
    $source = Get-Content -LiteralPath (
        Join-Path $BattleShipRoot $fixture.Path) -Raw
    foreach ($token in $fixture.Tokens) {
        if (-not $source.Contains($token)) {
            throw "BattleShip attack motion fixture is missing: $token"
        }
    }
}

$metadataPath = Join-Path $root 'assets/audio/fgm_phase_pack_ima.json'
$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$koIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'ko'
    } | ForEach-Object { [int]$_.id })
$marioRegularKo = [int[]]@($fixtures[0].Audio[0],
    $fixtures[0].Audio[1], 154)
$foxRegularKo = [int[]]@($fixtures[1].Audio[0],
    $fixtures[1].Audio[1], 154)
Assert-EqualList -Label 'Mario regular-KO source sequence' `
    -Actual $marioRegularKo -Expected ([int[]]@(439, 292, 154))
Assert-EqualList -Label 'Fox regular-KO source sequence' `
    -Actual $foxRegularKo -Expected ([int[]]@(370, 289, 154))
Assert-EqualList -Label 'Resident regular-KO ID set' -Actual $koIDs `
    -Expected ([int[]]@(439, 292, 370, 289, 154))
$attackIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'attack'
    } | ForEach-Object { [int]$_.id })
Assert-EqualList -Label 'Resident attack/activation ID set' `
    -Actual $attackIDs -Expected ([int[]]@(215))
$qualification = $metadata.attack_activation_qualification
Assert-EqualList -Label 'Source-qualified attack/activation ID set' `
    -Actual ([int[]]@($qualification.qualified_ids)) `
    -Expected ([int[]]@(215))
Assert-EqualList -Label 'Fail-closed attack/activation exclusion set' `
    -Actual ([int[]]@($qualification.excluded_ids)) `
    -Expected ([int[]]@(19, 41, 42, 43, 185, 186, 187, 189, 190,
            217, 218, 219))
if (($qualification.source_action_audit.sha256 -ne
        'ae7690adc1d646e8c0a755510064a324c6ff59f4f578a2f6fdd719351744c601') -or
    ($qualification.sha256 -ne
        '8e520123996038b06edbd9cd2c3194734b9d7d08bde89159271ff3872a15e69e') -or
    ($qualification.source_action_audit.region -ne 'REGION_US') -or
    ([int]$qualification.source_action_audit.callsite_count -ne 60) -or
    ([int]$qualification.source_action_audit.action_count -ne 66) -or
    (@($qualification.source_action_audit.callsites).Count -ne 60) -or
    (@($qualification.source_action_audit.actions).Count -ne 66)) {
    throw 'Attack/activation action or source-program qualification changed.'
}
foreach ($entry in $metadata.entries | Where-Object {
        $_.entry_kind -eq 'ko'
    }) {
    if ($entry.name -match '(?i)rebirth|respawn|halo') {
        throw "Regular-KO pack incorrectly claims respawn audio: $($entry.name)"
    }
}

$relocPath = Join-Path $root 'src/port/reloc_backend_assets.c'
$fgmPath = Join-Path $root 'src/nds/nds_audio_fgm.c'
$deadImportPath = Join-Path $root 'src/import/battleship_ftcommon_dead.c'
$relocText = Get-Content -LiteralPath $relocPath -Raw
$fgmText = Get-Content -LiteralPath $fgmPath -Raw
$deadImportText = Get-Content -LiteralPath $deadImportPath -Raw

if (-not $deadImportText.Contains(
        '../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c')) {
    throw 'The live dead-state TU no longer imports BattleShip ftcommondead.c.'
}

$normalize = [regex]::Match(
    $relocText,
    '(?s)static s32 ndsRelocNormalizeFighterAttributesFile\(.*?' +
        '(?=static size_t ndsRelocAssetAllocSize)'
)
if (-not $normalize.Success) {
    throw 'FTAttributes normalizer implementation was not found.'
}
$repairCount = [regex]::Matches(
    $normalize.Value, 'ndsRelocSwapNativeU16WordLanes\('
).Count
if ($repairCount -ne 6) {
    throw "FTAttributes normalizer repairs $repairCount words, expected 6."
}
foreach ($required in @(
        'NDS_RELOC_SYMBOL_MARIO_MAIN_ATTRIBUTES 0x428u',
        'NDS_RELOC_SYMBOL_FOX_MAIN_ATTRIBUTES 0x46cu',
        'offsetof(FTAttributes, dead_fgm_ids)',
        'offsetof(FTAttributes, deadup_sfx)',
        'offsetof(FTAttributes, smash_sfx)',
        'offsetof(FTAttributes, itemthrow_vel_scale)',
        'offsetof(FTAttributes, heavyget_sfx)',
        'ndsRelocFighterAttributesMatchSource(loaded->asset_id, attr)',
        'ndsRelocNormalizeFighterAttributesFile(loaded)'
    )) {
    if (-not $relocText.Contains($required)) {
        throw "FTAttributes production fixture is missing: $required"
    }
}

foreach ($required in @(
        'u16 fgm_id;',
        'u16 loop_point_words;',
        'entry->loop_point_words = ndsAudioFgmReadLe16(&raw[30]);',
        '(ndsAudioFgmReadLe16(&header[4]) != 3u)',
        '&sNdsAudioFgmPack[entry->data_offset], SoundFormat_ADPCM,',
        'entry->data_bytes - ((u32)entry->loop_point_words * 4u)',
        '((entry->flags & 1u) != 0u), entry->loop_point_words);',
        'handle->effect.sfx_id = ndsAudioFgmNextInstanceToken();',
        'handle->fgm_id = fgm_id;',
        'handle->effect.sfx_id = 0u;',
        'handle->allocated = FALSE;',
        'gNdsAudioFgmHandleRecycleCount++;',
        'gNdsAudioFgmHandleReleaseCount++;',
        'gNdsAudioFgmInstanceTokenWrapCount++;',
        'gNdsAudioFgmKoPlayCounts[ko_index]++;',
        'gNdsAudioFgmKoTrace[gNdsAudioFgmKoTraceCount++] = fgm_id;'
    )) {
    if (-not $fgmText.Contains($required)) {
        throw "FGM token-lifecycle fixture is missing: $required"
    }
}
foreach ($forbidden in @(
        'u16 reserved;',
        '(ndsAudioFgmReadLe16(&header[4]) != 2u)',
        '((entry->flags & 1u) != 0u), 0u);',
        'handle->effect.sfx_id = fgm_id;',
        'handle->effect.sfx_max = fgm_id;',
        'NDS_AUDIO_FGM_NONREUSE_HANDLE_CAPACITY',
        'never reused during one'
    )) {
    if ($fgmText.Contains($forbidden)) {
        throw "Obsolete FGM handle behavior remains: $forbidden"
    }
}

$devkitArm = if ([string]::IsNullOrWhiteSpace($env:DEVKITARM)) {
    'C:/devkitPro/devkitARM'
}
else {
    $env:DEVKITARM
}
$compiler = Join-Path $devkitArm 'bin/arm-none-eabi-gcc.exe'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
    throw "ARM compiler is required for layout fixtures: $compiler"
}
$layoutSource = @'
#include <stddef.h>
#include <ft/fighter.h>
#include <sys/audio.h>
_Static_assert(offsetof(FTAttributes, dead_fgm_ids) == 0xb4u, "dead");
_Static_assert(offsetof(FTAttributes, deadup_sfx) == 0xb8u, "deadup");
_Static_assert(offsetof(FTAttributes, damage_sfx) == 0xbau, "damage");
_Static_assert(offsetof(FTAttributes, smash_sfx) == 0xbcu, "smash");
_Static_assert(offsetof(FTAttributes, item_pickup) == 0xc4u, "pickup");
_Static_assert(offsetof(FTAttributes, itemthrow_vel_scale) == 0xe4u, "throw");
_Static_assert(offsetof(FTAttributes, heavyget_sfx) == 0xe8u, "heavy");
_Static_assert(offsetof(FTAttributes, halo_size) == 0xecu, "halo");
_Static_assert(offsetof(alSoundEffect, sfx_id) == 0x26u, "token");
int main(void) { return 0; }
'@
$compilerArgs = @(
    '-std=gnu11', '-fsyntax-only', '-x', 'c', '-',
    '-DREGION_US', '-D_LANGUAGE_C', '-DARM9', '-DSSB64_TARGET_NDS',
    "-I$(Join-Path $root 'include')",
    "-I$(Join-Path $BattleShipRoot 'src')",
    "-I$(Join-Path $BattleShipRoot 'src/sys')"
)
$compilerOutput = $layoutSource | & $compiler @compilerArgs 2>&1
if ($LASTEXITCODE -ne 0) {
    $details = ($compilerOutput | Out-String).Trim()
    throw "Target layout fixtures failed.`n$details"
}

& (Join-Path $PSScriptRoot 'check-audio-id-fixtures.ps1') `
    -BattleShipRoot $BattleShipRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Output (
    'Audio runtime fixtures passed: 2 source FTAttributes blocks, 6 audited ' +
    'mixed-u16 words each, 14 Mario motion callsites across 1 included and 2 ' +
    'source-audited excluded cues plus the random smash table/dispatch, ' +
    'exact Mario/Fox ' +
    'regular-KO call order, no rebirth-audio claim, source attack-motion ' +
    'callsites, target layouts, ' +
    'source IDs, persisted DS loop point/length, and recyclable BattleShip ' +
    'FGM tokens.'
)
