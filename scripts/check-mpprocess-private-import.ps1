param(
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\builds\build-battle-playable-canonical-hwtri-harness'),
    [string]$Elf = (Join-Path $PSScriptRoot '..\smash64ds-battle-playable-hwtri.elf'),
    [string]$CanonicalRom = (Join-Path $PSScriptRoot '..\smash64ds-battle-playable-hwtri.nds'),
    [string]$ShippedRom = (Join-Path $PSScriptRoot '..\smash64ds-battle-playable-hwtri.nds'),
    [string]$LiveElf = '',
    [string]$LiveCanonicalRom = '',
    [string]$BaselineBuildDir = '',
    [string]$BaselineElf = '',
    [string]$BaselineRom = '',
    [switch]$RequireAB,
    [string]$Nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
)
$ErrorActionPreference = 'Stop'

function Fail-Check {
    param([string]$Message)
    throw "mpprocess private import check failed: $Message"
}

function Assert-Check {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { Fail-Check $Message }
}

function Resolve-Leaf {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Fail-Check "$Label is missing: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Resolve-Container {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        Fail-Check "$Label is missing: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Assert-DistinctPath {
    param([string]$Left, [string]$Right, [string]$Label)
    Assert-Check (-not [string]::Equals(
        [IO.Path]::GetFullPath($Left),
        [IO.Path]::GetFullPath($Right),
        [StringComparison]::OrdinalIgnoreCase)) `
        "$Label must use distinct paths"
}

function Invoke-Tool {
    param([string]$Tool, [string[]]$Arguments, [string]$Label)
    $lines = @(& $Tool @Arguments 2>&1 | ForEach-Object { "$_" })
    if ($LASTEXITCODE -ne 0) {
        Fail-Check "$Label failed with exit code $LASTEXITCODE`: $($lines -join ' | ')"
    }
    return $lines
}

function Assert-ExactSet {
    param([object[]]$Actual, [object[]]$Expected, [string]$Label)
    $actualText = @($Actual | ForEach-Object { "$_" })
    $expectedText = @($Expected | ForEach-Object { "$_" })
    $actualUnique = @($actualText | Sort-Object -Unique)
    $expectedUnique = @($expectedText | Sort-Object -Unique)
    Assert-Check ($actualText.Count -eq $actualUnique.Count) "$Label contains duplicate actual entries"
    Assert-Check ($expectedText.Count -eq $expectedUnique.Count) "$Label contains duplicate expected entries"
    $delta = @(Compare-Object -ReferenceObject $expectedUnique -DifferenceObject $actualUnique)
    if ($delta.Count -ne 0) {
        $detail = @($delta | ForEach-Object { "$($_.SideIndicator)$($_.InputObject)" }) -join ', '
        Fail-Check "$Label mismatch: $detail"
    }
}

function Get-Definitions {
    param([string]$Object, [switch]$ExternalOnly)
    $arguments = @('-S', '--size-sort', '--defined-only')
    if ($ExternalOnly) { $arguments += '-g' }
    $arguments += $Object
    $records = @()
    foreach ($line in @(Invoke-Tool $Nm $arguments "nm definitions for $Object")) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+(\S)\s+(\S+)$') {
            Fail-Check "unparsed nm definition line for $Object`: $line"
        }
        $records += [pscustomobject]@{
            Name = $Matches[4]
            Type = $Matches[3]
            Size = [Convert]::ToUInt64($Matches[2], 16)
        }
    }
    return $records
}

function Get-Undefined {
    param([string]$Object)
    $names = @()
    foreach ($line in @(Invoke-Tool $Nm @('-u', $Object) "nm undefined symbols for $Object")) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^\s+U\s+(\S+)\s*$') {
            Fail-Check "unparsed nm undefined line for $Object`: $line"
        }
        $names += $Matches[1]
    }
    return $names
}

function ConvertFrom-MakeWords {
    param([string]$Text, [string]$Label)
    $tokens = New-Object 'System.Collections.Generic.List[string]'
    $token = New-Object System.Text.StringBuilder
    $escaped = $false
    foreach ($character in $Text.ToCharArray()) {
        if ($escaped) {
            [void]$token.Append($character)
            $escaped = $false
            continue
        }
        if ($character -eq '\') {
            $escaped = $true
            continue
        }
        if ([char]::IsWhiteSpace($character)) {
            if ($token.Length -gt 0) {
                [void]$tokens.Add($token.ToString().Replace('$$', '$'))
                [void]$token.Clear()
            }
            continue
        }
        [void]$token.Append($character)
    }
    Assert-Check (-not $escaped) "$Label ends with an unterminated make escape"
    if ($token.Length -gt 0) {
        [void]$tokens.Add($token.ToString().Replace('$$', '$'))
    }
    return @($tokens)
}

function Get-DependencyRule {
    param(
        [string]$DependencyFile,
        [string]$Target,
        [string]$ProjectRoot
    )
    $lines = @((Get-Content -LiteralPath $DependencyFile) | ForEach-Object { "$_" })
    Assert-Check ($lines.Count -gt 0) "dependency file is empty: $DependencyFile"

    $logical = New-Object System.Text.StringBuilder
    $terminated = $false
    foreach ($line in $lines) {
        $trimmed = $line.TrimEnd()
        $trailingSlashes = 0
        for ($i = $trimmed.Length - 1; ($i -ge 0) -and ($trimmed[$i] -eq '\'); $i--) {
            $trailingSlashes++
        }
        $continued = (($trailingSlashes % 2) -eq 1)
        $piece = if ($continued) {
            $trimmed.Substring(0, $trimmed.Length - 1)
        } else {
            $trimmed
        }
        if ($logical.Length -gt 0) { [void]$logical.Append(' ') }
        [void]$logical.Append($piece)
        if (-not $continued) {
            $terminated = $true
            break
        }
    }
    Assert-Check $terminated "first dependency rule is unterminated: $DependencyFile"

    $match = [regex]::Match($logical.ToString(),
        ('^\s*' + [regex]::Escape($Target) + '[ \t]*:[ \t]*(.*)$'),
        [Text.RegularExpressions.RegexOptions]::Singleline)
    Assert-Check $match.Success `
        "dependency file does not start with the exact $Target rule: $DependencyFile"
    $words = @(ConvertFrom-MakeWords $match.Groups[1].Value "$Target dependency rule")
    Assert-Check ($words.Count -gt 0) "$Target dependency rule has no prerequisites"

    $paths = New-Object 'System.Collections.Generic.List[string]'
    $keys = [System.Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($word in $words) {
        Assert-Check ($word -cne '|') "$Target dependency rule contains an order-only separator"
        $candidate = if ([IO.Path]::IsPathRooted($word)) {
            [IO.Path]::GetFullPath($word)
        } else {
            [IO.Path]::GetFullPath((Join-Path $ProjectRoot $word))
        }
        Assert-Check (Test-Path -LiteralPath $candidate -PathType Leaf) `
            "$Target dependency prerequisite is missing: $candidate"
        $resolved = (Resolve-Path -LiteralPath $candidate).Path
        Assert-Check ($keys.Add($resolved)) `
            "$Target dependency rule contains a duplicate prerequisite: $resolved"
        [void]$paths.Add($resolved)
    }
    return [pscustomobject]@{
        Target = $Target
        Prerequisites = @($paths)
    }
}

function Assert-DependencyRule {
    param(
        [object]$Rule,
        [string[]]$Required,
        [string[]]$Forbidden,
        [string]$Label
    )
    $actual = [System.Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $Rule.Prerequisites) { [void]$actual.Add($path) }
    foreach ($path in $Required) {
        $requiredPath = [IO.Path]::GetFullPath($path)
        Assert-Check ($actual.Contains($requiredPath)) `
            "$Label dependency is missing exact prerequisite '$requiredPath'"
    }
    foreach ($path in $Forbidden) {
        $forbiddenPath = [IO.Path]::GetFullPath($path)
        Assert-Check (-not $actual.Contains($forbiddenPath)) `
            "$Label unexpectedly depends on '$forbiddenPath'"
    }
}

function Get-CFunctionText {
    param([string]$Text, [string]$Name, [string]$Label)
    $matches = @([regex]::Matches($Text,
        ('(?m)^[ \t]*(?:static[ \t]+)?(?:sb32|void)[ \t]+' +
         [regex]::Escape($Name) + '[ \t]*\(')))
    Assert-Check ($matches.Count -eq 1) `
        "$Label must define $Name exactly once"
    $openBrace = $Text.IndexOf('{', $matches[0].Index)
    Assert-Check ($openBrace -ge 0) "$Label function $Name has no body"
    $depth = 0
    for ($i = $openBrace; $i -lt $Text.Length; $i++) {
        if ($Text[$i] -eq '{') { $depth++ }
        elseif ($Text[$i] -eq '}') {
            $depth--
            if ($depth -eq 0) {
                return $Text.Substring($matches[0].Index,
                    ($i - $matches[0].Index) + 1)
            }
        }
    }
    Fail-Check "$Label function $Name has an unterminated body"
}

function Get-BuildConfigMacroValue {
    param([string]$Text, [string]$Name, [string]$Label)
    $matches = @([regex]::Matches($Text,
        ('(?m)^[ \t]*#define[ \t]+' + [regex]::Escape($Name) +
         '[ \t]+([^\r\n]*?)[ \t]*\r?$')))
    Assert-Check ($matches.Count -eq 1) `
        "$Label must define $Name exactly once"
    return $matches[0].Groups[1].Value.Trim()
}

function Get-BuildConfigGate {
    param([string]$Text, [string]$Label)
    return Get-BuildConfigMacroValue $Text `
        'NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE' $Label
}

function Remove-BuildConfigGateLine {
    param([string]$Text, [string]$Label)
    $pattern = '(?m)^[ \t]*#define[ \t]+NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE[ \t]+[^\r\n]*(?:\r?\n|$)'
    $matches = @([regex]::Matches($Text, $pattern))
    Assert-Check ($matches.Count -eq 1) `
        "$Label must contain exactly one removable private-gate line"
    return [regex]::Replace($Text, $pattern, '', 1)
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Get-BytesSha256 {
    param([byte[]]$Bytes)
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        return (($hasher.ComputeHash($Bytes) | ForEach-Object { '{0:x2}' -f $_ }) -join '').ToUpperInvariant()
    } finally {
        $hasher.Dispose()
    }
}

function Get-AllocFingerprint {
    param([string]$Path)
    $lines = @(Invoke-Tool $Objdump @('-h', $Path) "objdump section table for $Path")
    $fileBytes = [System.IO.File]::ReadAllBytes($Path)
    $records = @()
    for ($i = 0; $i -lt ($lines.Count - 1); $i++) {
        if ($lines[$i] -notmatch '^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+2\*\*(\d+)\s*$') {
            continue
        }
        $name = $Matches[1]
        $size = [Convert]::ToUInt64($Matches[2], 16)
        $vma = $Matches[3].ToLowerInvariant()
        $lma = $Matches[4].ToLowerInvariant()
        $offset = [Convert]::ToUInt64($Matches[5], 16)
        $alignment = $Matches[6]
        $flags = $lines[$i + 1].Trim()
        if ($flags -notmatch '(^|,\s*)ALLOC(,|$)') { continue }
        $contentHash = '-'
        if (($flags -match '(^|,\s*)CONTENTS(,|$)') -and ($size -gt 0)) {
            Assert-Check (($offset + $size) -le $fileBytes.LongLength) `
                "allocated section $name exceeds ELF bounds in $Path"
            $sectionBytes = New-Object byte[] ([int]$size)
            [Array]::Copy($fileBytes, [int64]$offset, $sectionBytes, 0, [int64]$size)
            $contentHash = Get-BytesSha256 $sectionBytes
        }
        $records += ('{0}|{1:x}|{2}|{3}|{4}|{5}|{6}' -f
            $name, $size, $vma, $lma, $alignment,
            ($flags -replace '\s+', ''), $contentHash)
    }
    Assert-Check ($records.Count -gt 0) "no allocated sections were parsed from $Path"
    return $records
}

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$buildPath = if (Test-Path -LiteralPath $BuildDir -PathType Container) {
    (Resolve-Path -LiteralPath $BuildDir).Path
} else {
    Fail-Check "canonical build directory is missing: $BuildDir"
}
$makefilePath = Resolve-Leaf (Join-Path $root 'Makefile') 'Makefile'
$wrapperPath = Resolve-Leaf (Join-Path $root 'src\import\battleship_mpprocess.c') 'mpprocess wrapper'
$supportSourcePath = Resolve-Leaf (Join-Path $root 'src\port\battleship_mpprocess_edge_support.c') 'mpprocess support source'
$collisionSourcePath = Resolve-Leaf (Join-Path $root 'src\port\reloc_backend_mp_collision.c') 'collision backend source'
$privateHeaderPath = Resolve-Leaf (Join-Path $root 'include\nds\nds_mpprocess_source.h') 'mpprocess private header'
$upstreamPath = Resolve-Leaf (Join-Path $root 'decomp\BattleShip-main\decomp\src\mp\mpprocess.c') 'BattleShip mpprocess source'
$upstreamCollisionPath = Resolve-Leaf (Join-Path $root 'decomp\BattleShip-main\decomp\src\mp\mpcollision.c') 'BattleShip mpcollision source'
$upstreamDefinitionPath = Resolve-Leaf (Join-Path $root 'decomp\BattleShip-main\decomp\src\mp\mpdef.h') 'BattleShip mpdef source'
$mapHeaderPath = Resolve-Leaf (Join-Path $root 'include\mp\map.h') 'public map header'
$objectPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess.o') 'mpprocess object'
$supportObjectPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess_edge_support.o') 'mpprocess support object'
$dependencyPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess.d') 'mpprocess dependency file'
$supportDependencyPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess_edge_support.d') 'mpprocess support dependency file'
$sceneObjectPath = Resolve-Leaf (Join-Path $buildPath 'scene_backend.o') 'scene backend object'
$sceneDependencyPath = Resolve-Leaf (Join-Path $buildPath 'scene_backend.d') 'scene backend dependency file'
$configPath = Resolve-Leaf (Join-Path $buildPath 'nds_build_config.h') 'canonical build config'
$mapPath = Resolve-Leaf (Join-Path $buildPath '.map') 'canonical link map'
$elfPath = Resolve-Leaf $Elf 'canonical ELF'
$canonicalRomPath = Resolve-Leaf $CanonicalRom 'canonical ROM'
$shippedRomPath = Resolve-Leaf $ShippedRom 'shipped ROM'
$nmPath = Resolve-Leaf $Nm 'arm-none-eabi-nm'
$objdumpPath = Resolve-Leaf $Objdump 'arm-none-eabi-objdump'
$Nm = $nmPath
$Objdump = $objdumpPath

$expectedUpstreamHash = '42639625B85ACD7CFC50416C378BBDE59747A8DD1CFCBCA4AACADF949405C3B9'
$expectedDefinitionHash = '2A7E4E42BA6DCCDEFC99DD4B33BB2A52EA5B02C61A32022A2D0E0CC58DDF260E'
$expectedCollisionHash = 'AF7B73F3BB0AA5E1670AB53C82666263C518C0344A0D30CF58DEC3D0CCB4EF53'
$upstreamHash = Get-Sha256 $upstreamPath
$upstreamDefinitionHash = Get-Sha256 $upstreamDefinitionPath
$upstreamCollisionHash = Get-Sha256 $upstreamCollisionPath
Assert-Check ($upstreamHash -ceq $expectedUpstreamHash) `
    "BattleShip mpprocess.c SHA256 drifted: $upstreamHash"
Assert-Check ($upstreamDefinitionHash -ceq $expectedDefinitionHash) `
    "BattleShip mpdef.h SHA256 drifted: $upstreamDefinitionHash"
Assert-Check ($upstreamCollisionHash -ceq $expectedCollisionHash) `
    "BattleShip mpcollision.c SHA256 drifted: $upstreamCollisionHash"

$collisionText = Get-Content -LiteralPath $collisionSourcePath -Raw
$supportSourceText = Get-Content -LiteralPath $supportSourcePath -Raw
$upstreamCollisionText = Get-Content -LiteralPath $upstreamCollisionPath -Raw
$localToWorldBody = Get-CFunctionText $collisionText `
    'ndsMPLinePositionLocalToWorld' 'collision backend'
$worldToLocalBody = Get-CFunctionText $collisionText `
    'ndsMPLinePositionWorldToLocal' 'collision backend'
Assert-Check ($localToWorldBody -match
    'position->x\s*\+=\s*yakumono_dobj->translate\.vec\.f\.x') `
    'collision backend local-to-world helper does not add yakumono X'
Assert-Check ($localToWorldBody -match
    'position->y\s*\+=\s*yakumono_dobj->translate\.vec\.f\.y') `
    'collision backend local-to-world helper does not add yakumono Y'
Assert-Check ($worldToLocalBody -match
    'local_position->x\s*-=\s*yakumono_dobj->translate\.vec\.f\.x') `
    'collision backend world-to-local helper does not subtract yakumono X'
Assert-Check ($worldToLocalBody -match
    'local_position->y\s*-=\s*yakumono_dobj->translate\.vec\.f\.y') `
    'collision backend world-to-local helper does not subtract yakumono Y'

foreach ($name in @(
    'mpCollisionGetVertexPositionID',
    'mpCollisionGetFloorEdgeL',
    'mpCollisionGetFloorEdgeR',
    'mpCollisionGetCeilEdgeL',
    'mpCollisionGetCeilEdgeR',
    'ndsMPGetWallEdgeU'
)) {
    $body = Get-CFunctionText $collisionText $name 'collision backend'
    Assert-Check ($body -match 'ndsMPLinePositionLocalToWorld\s*\(') `
        "$name does not publish a world-space endpoint/vertex"
}
$ceilCommonBody = Get-CFunctionText $collisionText `
    'mpCollisionGetFCCommonCeil' 'collision backend'
Assert-Check (($ceilCommonBody -match
        'ndsMPLinePositionWorldToLocal\s*\(') -and
    ($ceilCommonBody -match
        'ndsMPLineDistanceFC\s*\(\s*object_local\.x') -and
    ($ceilCommonBody -match 'ceil_y\s*-\s*object_local\.y')) `
    'FCCommonCeil does not evaluate local vertices from a world-space input'
$ceilSweepBody = Get-CFunctionText $collisionText `
    'ndsStageMPCeilFloorLoopSweep' 'collision backend'
Assert-Check (($ceilSweepBody -match
        'mpCollisionGetFCCommonCeil\s*\(\s*line_id\s*,\s*translate\s*,') -and
    ($ceilSweepBody -notmatch
        'mpCollisionGetFCCommonCeil\s*\(\s*line_id\s*,\s*&sweep_translate')) `
    'ceil sweep still passes its line-local intersection point to FCCommonCeil'
foreach ($name in @('ndsStageMPPlatformSpeedFloorLoopChooseCeilLine',
                     'ndsStageMPCeilFloorLoopChooseLine')) {
    $body = Get-CFunctionText $collisionText $name 'collision backend'
    Assert-Check (($body -match
            'ndsMPLinePositionLocalToWorld\s*\(\s*candidate\s*,\s*&world_probe') -and
        ($body -match
            'mpCollisionGetFCCommonCeil\s*\(\s*candidate\s*,\s*&world_probe')) `
        "$name still passes a raw line-local diagnostic probe to FCCommonCeil"
}
$lrCommonBody = Get-CFunctionText $collisionText `
    'ndsMPGetLRCommonWall' 'collision backend'
Assert-Check (($lrCommonBody -match
        'ndsMPLinePositionWorldToLocal\s*\(') -and
    ($lrCommonBody -match 'object_local\.y\s*-\s*a\.y') -and
    ($lrCommonBody -match 'wall_x\s*-\s*object_local\.x')) `
    'LRCommon wall helper does not evaluate local vertices from a world-space input'

$lowerEdgeBody = Get-CFunctionText $supportSourceText `
    'ndsPrivateMPGetWallEdgeD' 'mpprocess support source'
Assert-Check ([regex]::Matches($lowerEdgeBody,
    'mpCollisionGetVertexPositionID\s*\(').Count -eq 2) `
    'lower-wall support must select exactly two public world-space vertices'
Assert-Check ($lowerEdgeBody -notmatch
    'gMPCollisionVertexInfo|gMPCollisionYakumonoDObjs|yakumono_dobj|translate\.vec') `
    'lower-wall support still applies a second yakumono translation'

foreach ($name in @('mpCollisionGetLREdge', 'mpCollisionGetUDEdge',
                     'mpCollisionGetVertexPositionID')) {
    $body = Get-CFunctionText $upstreamCollisionText $name `
        'BattleShip mpcollision source'
    Assert-Check (($body -match 'translate\.vec\.f\.x') -and
        ($body -match 'translate\.vec\.f\.y')) `
        "BattleShip $name no longer defines the endpoint world-space contract"
}
foreach ($name in @('mpCollisionGetFCCommon', 'mpCollisionGetLRCommon')) {
    $body = Get-CFunctionText $upstreamCollisionText $name `
        'BattleShip mpcollision source'
    Assert-Check (($body -match
            'object_pos->x\s*-\s*yakumono_dobj->translate\.vec\.f\.x') -and
        ($body -match
            'object_pos->y\s*-\s*yakumono_dobj->translate\.vec\.f\.y')) `
        "BattleShip $name no longer defines the world-to-local common contract"
}

$liveElfPath = $null
$liveCanonicalRomPath = $null
if ($RequireAB) {
    Assert-Check (-not [string]::IsNullOrWhiteSpace($LiveElf)) `
        '-RequireAB requires -LiveElf'
    Assert-Check (-not [string]::IsNullOrWhiteSpace($LiveCanonicalRom)) `
        '-RequireAB requires -LiveCanonicalRom'
    $liveElfPath = Resolve-Leaf $LiveElf 'unstaged private-ON ELF'
    $liveCanonicalRomPath = Resolve-Leaf $LiveCanonicalRom `
        'unstaged private-ON canonical ROM'
}

$configText = Get-Content -LiteralPath $configPath -Raw
$expectedConfigMacros = [ordered]@{
    NDS_BUILD_HARNESS_VARIANT = '"battle_playable_realtime"'
    NDS_RENDERER_HW_TRIANGLES = '1'
    NDS_RENDERER_PROFILE_LEVEL = '0'
    NDS_IMPORT_BATTLESHIP_FTMANAGER = '1'
    NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE = '0'
    NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE = '1'
}
foreach ($name in $expectedConfigMacros.Keys) {
    $actualValue = Get-BuildConfigMacroValue $configText $name `
        'candidate build config'
    $expectedValue = $expectedConfigMacros[$name]
    Assert-Check ($actualValue -ceq $expectedValue) `
        "candidate build config $name is '$actualValue', expected '$expectedValue'"
}

$wrapperText = Get-Content -LiteralPath $wrapperPath -Raw
$upstreamText = Get-Content -LiteralPath $upstreamPath -Raw
$wrapperNormalized = $wrapperText -replace '\\\r?\n[ \t]*', ' '
$functionMaps = @([regex]::Matches($wrapperNormalized,
    '(?m)^#define[ \t]+(mpProcess\w+)[ \t]+(ndsBaseMPProcess\w+)[ \t]*\r?$'))
$globalMaps = @([regex]::Matches($wrapperNormalized,
    '(?m)^#define[ \t]+(sMPProcess\w+)[ \t]+(sNdsBaseMPProcess\w+)[ \t]*\r?$'))
Assert-Check ($functionMaps.Count -eq 36) "wrapper function mapping count is $($functionMaps.Count), expected 36"
Assert-Check ($globalMaps.Count -eq 7) "wrapper global mapping count is $($globalMaps.Count), expected 7"

$functionSources = @($functionMaps | ForEach-Object { $_.Groups[1].Value })
$functionTargets = @($functionMaps | ForEach-Object { $_.Groups[2].Value })
$globalSources = @($globalMaps | ForEach-Object { $_.Groups[1].Value })
$globalTargets = @($globalMaps | ForEach-Object { $_.Groups[2].Value })
Assert-ExactSet $functionSources @([regex]::Matches($upstreamText,
    '(?m)^(?:void|sb32)\s+(mpProcess\w+)\s*\(') | ForEach-Object { $_.Groups[1].Value }) `
    'BattleShip function-to-wrapper source names'
Assert-ExactSet $globalSources @([regex]::Matches($upstreamText,
    '(?m)^(?:s32|f32|u32|Vec3f)\s+(sMPProcess\w+)(?:\[[^\]]+\])?\s*;') | ForEach-Object { $_.Groups[1].Value }) `
    'BattleShip global-to-wrapper source names'
for ($i = 0; $i -lt $functionMaps.Count; $i++) {
    $expected = 'ndsBaseMPProcess' + $functionSources[$i].Substring('mpProcess'.Length)
    Assert-Check ($functionTargets[$i] -ceq $expected) "noncanonical function mapping for $($functionSources[$i])"
}
for ($i = 0; $i -lt $globalMaps.Count; $i++) {
    $expected = 'sNdsBaseMPProcess' + $globalSources[$i].Substring('sMPProcess'.Length)
    Assert-Check ($globalTargets[$i] -ceq $expected) "noncanonical global mapping for $($globalSources[$i])"
}

$includeMatches = @([regex]::Matches($wrapperNormalized,
    '(?m)^\s*#include\s+"\.\./\.\./decomp/BattleShip-main/decomp/src/mp/mpprocess\.c"\s*\r?$'))
Assert-Check ($includeMatches.Count -eq 1) "wrapper must include the exact BattleShip mpprocess source once"
$compatibilityMacroValues = [ordered]@{
    MAP_VERTEX_COLL_CLIFF = '0x00008000u'
    MAP_FLAG_LCLIFF = '0x00001000u'
    MAP_FLAG_RCLIFF = '0x00002000u'
    MAP_FLAG_CEILHEAVY = '0x00004000u'
    MAP_FLAG_FLOOREDGE = '0x00008000u'
    MAP_FLAG_CLIFF_MASK = '(MAP_FLAG_LCLIFF | MAP_FLAG_RCLIFF)'
    MAP_VERTEX_MAT_MASK = '(~0x0000ff00u)'
    nMPMaterial4 = '4'
}
$compatibilityMacros = @($compatibilityMacroValues.Keys)
$defineMatches = @([regex]::Matches($wrapperNormalized,
    '(?m)^#define[ \t]+([A-Za-z_]\w+)(?:[ \t]+([^\r\n]*?))?[ \t]*\r?$'))
$undefMatches = @([regex]::Matches($wrapperNormalized,
    '(?m)^#undef[ \t]+([A-Za-z_]\w+)[ \t]*\r?$'))
$definedNames = @($defineMatches | ForEach-Object { $_.Groups[1].Value })
$undefinedNames = @($undefMatches | ForEach-Object { $_.Groups[1].Value })
$expectedMacros = @($functionSources + $globalSources + $compatibilityMacros)
Assert-ExactSet $definedNames $expectedMacros 'wrapper macro definitions'
Assert-ExactSet $undefinedNames $expectedMacros 'wrapper macro undef hygiene'
foreach ($name in $compatibilityMacros) {
    $matches = @($defineMatches | Where-Object { $_.Groups[1].Value -ceq $name })
    Assert-Check ($matches.Count -eq 1) `
        "wrapper compatibility macro $name must be defined exactly once"
    $actualValue = ($matches[0].Groups[2].Value -replace '\s+', ' ').Trim()
    $expectedValue = $compatibilityMacroValues[$name]
    Assert-Check ($actualValue -ceq $expectedValue) `
        "wrapper compatibility macro $name is '$actualValue', expected '$expectedValue'"
}
foreach ($match in $defineMatches) {
    Assert-Check ($match.Index -lt $includeMatches[0].Index) "macro '$($match.Groups[1].Value)' is defined after the source include"
}
foreach ($match in $undefMatches) {
    Assert-Check ($match.Index -gt $includeMatches[0].Index) "macro '$($match.Groups[1].Value)' is undefined before the source include"
}

$definitions = @(Get-Definitions $objectPath)
Assert-ExactSet @($definitions.Name) @($functionTargets + $globalTargets) 'mpprocess object definitions'
foreach ($definition in $definitions) {
    Assert-Check ($definition.Size -gt 0) "zero-sized mpprocess definition: $($definition.Name)"
    $expectedType = if ($functionTargets -ccontains $definition.Name) { 'T' } else { 'B' }
    Assert-Check ($definition.Type -ceq $expectedType) "mpprocess definition $($definition.Name) has type $($definition.Type), expected $expectedType"
}

$supportDefinitions = @(Get-Definitions $supportObjectPath)
$supportExports = @(Get-Definitions $supportObjectPath -ExternalOnly)
$supportApiNames = @('mpCollisionGetLWallEdgeD', 'mpCollisionGetRWallEdgeD')
Assert-ExactSet @($supportDefinitions.Name) @($supportApiNames + 'ndsPrivateMPGetWallEdgeD') 'support object definitions'
Assert-ExactSet @($supportExports.Name) $supportApiNames 'support object exported APIs'
foreach ($definition in $supportDefinitions) {
    Assert-Check ($definition.Size -gt 0) "zero-sized support definition: $($definition.Name)"
    $expectedType = if ($supportApiNames -ccontains $definition.Name) { 'T' } else { 't' }
    Assert-Check ($definition.Type -ceq $expectedType) "support definition $($definition.Name) has type $($definition.Type), expected $expectedType"
}

$expectedUndefined = @'
__aeabi_f2iz
__aeabi_fadd
__aeabi_fcmpge
__aeabi_fcmpgt
__aeabi_fcmple
__aeabi_fcmplt
__aeabi_fdiv
__aeabi_fmul
__aeabi_fsub
__aeabi_i2f
gMPCollisionUpdateTic
mpCollisionCheckCeilLineCollisionDiff
mpCollisionCheckCeilLineCollisionSame
mpCollisionCheckExistLineID
mpCollisionCheckFloorLineCollisionDiff
mpCollisionCheckFloorLineCollisionSame
mpCollisionCheckLWallLineCollisionDiff
mpCollisionCheckLWallLineCollisionSame
mpCollisionCheckProjectFloor
mpCollisionCheckRWallLineCollisionDiff
mpCollisionCheckRWallLineCollisionSame
mpCollisionGetCeilEdgeL
mpCollisionGetCeilEdgeR
mpCollisionGetEdgeLeftDLineID
mpCollisionGetEdgeLeftULineID
mpCollisionGetEdgeRightDLineID
mpCollisionGetEdgeRightULineID
mpCollisionGetEdgeUnderLLineID
mpCollisionGetEdgeUnderRLineID
mpCollisionGetEdgeUpperLLineID
mpCollisionGetEdgeUpperRLineID
mpCollisionGetFCCommonCeil
mpCollisionGetFCCommonFloor
mpCollisionGetFloorEdgeL
mpCollisionGetFloorEdgeR
mpCollisionGetLineTypeID
mpCollisionGetLRCommonLWall
mpCollisionGetLRCommonRWall
mpCollisionGetLWallEdgeD
mpCollisionGetLWallEdgeU
mpCollisionGetRWallEdgeD
mpCollisionGetRWallEdgeU
mpCollisionGetVertexCountLineID
mpCollisionGetVertexPositionID
'@ -split '\r?\n' | Where-Object { $_ }
$expectedSupportUndefined = @(
    '__aeabi_fcmplt', 'mpCollisionGetVertexCountLineID',
    'mpCollisionGetVertexPositionID'
)
$undefined = @(Get-Undefined $objectPath)
$supportUndefined = @(Get-Undefined $supportObjectPath)
Assert-ExactSet $undefined $expectedUndefined 'mpprocess undefined allowlist'
Assert-ExactSet $supportUndefined $expectedSupportUndefined 'support undefined allowlist'

$mapText = Get-Content -LiteralPath $mapPath -Raw
$leakPattern = '(?:battleship_mpprocess(?:_edge_support|_live_bridge)?\.o|ndsBaseMPProcess\w*|sNdsBaseMPProcess\w*|ndsPrivateMPGetWallEdgeD|mpCollisionGet[LR]WallEdgeD)'
Assert-Check ($mapText -notmatch $leakPattern) 'private/support object or symbol leaked into the canonical link map'
$loadObjects = @([regex]::Matches($mapText, '(?m)^LOAD (.+\.o)\r?$') |
    ForEach-Object { $_.Groups[1].Value })
Assert-Check ($loadObjects.Count -gt 0) 'canonical link map has no direct object LOAD records'
$canonicalObjects = @()
foreach ($loadObject in $loadObjects) {
    $candidate = if ([System.IO.Path]::IsPathRooted($loadObject)) {
        $loadObject
    } else {
        Join-Path $buildPath $loadObject
    }
    Assert-Check (Test-Path -LiteralPath $candidate -PathType Leaf) `
        "canonical map object is missing: $loadObject"
    $canonicalObjects += (Resolve-Path -LiteralPath $candidate).Path
}
Assert-ExactSet $canonicalObjects @($canonicalObjects | Sort-Object -Unique) `
    'canonical map object paths'

$canonicalDefinitionLines = @(Invoke-Tool -Tool $Nm `
    -Arguments (@('-A', '-g', '--defined-only') + $canonicalObjects) `
    -Label 'canonical object-set definitions')
$canonicalDefinitions = @()
foreach ($line in $canonicalDefinitionLines) {
    if ($line -notmatch '^(.+):([0-9a-fA-F]+)\s+(\S)\s+(\S+)\s*$') {
        Fail-Check "unparsed canonical object definition line: $line"
    }
    $canonicalDefinitions += [pscustomobject]@{
        Object = $Matches[1]
        Type = $Matches[3]
        Name = $Matches[4]
    }
}
$providerDefinitions = @($canonicalDefinitions + @($supportExports | ForEach-Object {
    [pscustomobject]@{ Object = $supportObjectPath; Type = $_.Type; Name = $_.Name }
}))
$strongDuplicates = @($providerDefinitions |
    Where-Object { $_.Type -notmatch '^[WwVv]$' } |
    Group-Object Name |
    Where-Object { $_.Count -gt 1 })
if ($strongDuplicates.Count -ne 0) {
    $detail = @($strongDuplicates | ForEach-Object {
        '{0}=[{1}]' -f $_.Name, (@($_.Group.Object | Sort-Object -Unique) -join ',')
    }) -join '; '
    Fail-Check "duplicate strong dependency providers: $detail"
}
$resolvers = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($name in $providerDefinitions.Name) { [void]$resolvers.Add($name) }
foreach ($name in @($undefined + $supportUndefined | Sort-Object -Unique)) {
    if ($name -match '^__aeabi_') { continue }
    Assert-Check ($resolvers.Contains($name)) "non-EABI dependency has no support-object or canonical-object definition: $name"
}

$objectSections = @(Invoke-Tool $Objdump @('-h', $objectPath) 'mpprocess object section table') -join "`n"
$supportSections = @(Invoke-Tool $Objdump @('-h', $supportObjectPath) 'support object section table') -join "`n"
Assert-Check ([regex]::Matches($objectSections,
    '(?m)^\s*\d+\s+\.text\.ndsBaseMPProcess\w+\b').Count -eq 36) `
    'mpprocess object does not have exactly 36 separate function sections'
Assert-Check ([regex]::Matches($objectSections,
    '(?m)^\s*\d+\s+\.(?:bss|data)\.sNdsBaseMPProcess\w+\b').Count -eq 7) `
    'mpprocess object does not have exactly 7 separate data sections'
foreach ($name in @($supportApiNames + 'ndsPrivateMPGetWallEdgeD')) {
    Assert-Check ($supportSections -match ('(?m)^\s*\d+\s+\.text\.' + [regex]::Escape($name) + '\b')) `
        "support definition is not in its own function section: $name"
}

$mainRule = Get-DependencyRule $dependencyPath 'battleship_mpprocess.o' $root
$supportRule = Get-DependencyRule $supportDependencyPath 'battleship_mpprocess_edge_support.o' $root
$sceneRule = Get-DependencyRule $sceneDependencyPath 'scene_backend.o' $root
Assert-DependencyRule $mainRule @(
    $wrapperPath,
    $configPath,
    $privateHeaderPath,
    $mapHeaderPath,
    $upstreamPath
) @($supportSourcePath) 'mpprocess object'
Assert-DependencyRule $supportRule @(
    $supportSourcePath,
    $configPath,
    $privateHeaderPath,
    $mapHeaderPath
) @($wrapperPath, $upstreamPath) 'support object'
Assert-DependencyRule $sceneRule @(
    $collisionSourcePath,
    $configPath,
    $privateHeaderPath
) @() 'scene backend object'

$makefileText = Get-Content -LiteralPath $makefilePath -Raw
$makefileLogical = $makefileText -replace '\\\r?\n[ \t]*', ' '
$sourceListMatches = @([regex]::Matches($makefileLogical,
    '(?m)^NDS_MPPROCESS_SOURCE_CFILES\s*:=\s*([^\r\n]+?)\s*\r?$'))
Assert-Check ($sourceListMatches.Count -eq 1) `
    'Makefile must have one shared mpprocess source list assignment'
$sourceList = ($sourceListMatches[0].Groups[1].Value `
    -replace '\s+', ' ').Trim()
Assert-Check ($sourceList -ceq
    'battleship_mpprocess_edge_support.c battleship_mpprocess.c') `
    'Makefile shared mpprocess source list is not the exact support/wrapper pair'
Assert-Check ($makefileText -match
    '(?m)^NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE\s*\?=\s*0\s*\r?$') `
    'Makefile live import gate is missing or not disabled by default'
Assert-Check ($makefileText -match
    '(?m)^NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE\s*\?=\s*1\s*\r?$') `
    'Makefile private import gate is missing or not enabled by default'
$mutualExclusionPattern =
    '(?m)^ifeq \(\$\(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE\)' +
    '\$\(NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE\),11\)\r?\n' +
    '^\$\(error NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE=1 requires ' +
    'NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE=0\)\r?\n^endif\s*$'
Assert-Check ($makefileText -match $mutualExclusionPattern) `
    'Makefile does not hard-reject LIVE=1 plus PRIVATE=1'
$routingText = (($makefileLogical -split '\r?\n') |
    ForEach-Object { (($_.Trim()) -replace '[ \t]+', ' ') }) -join "`n"
$routingPattern =
    '(?m)^ifeq \(\$\(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE\),1\)\n' +
    '^CFILES \+= \$\(NDS_MPPROCESS_SOURCE_CFILES\) ' +
    'battleship_mpprocess_live_bridge\.c\n' +
    '^else ifeq \(\$\(NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE\),1\)\n' +
    '^NDS_PRIVATE_CHECK_CFILES \+= ' +
    '\$\(NDS_MPPROCESS_SOURCE_CFILES\)\n^endif$'
Assert-Check ($routingText -match $routingPattern) `
    'Makefile mpprocess LIVE/private source routing is not exact and mutually exclusive'
Assert-Check ($makefileLogical -match
    '(?m)^\$\(OUTPUT\)\.elf:\s+\$\(OFILES\)\s+\$\(NDS_PRIVATE_CHECK_OFILES\)\s*\r?$') `
    'private-check objects are not final ELF prerequisites'
Assert-Check ($makefileLogical -match
    '(?m)^export OFILES\s*:=\s*\$\(CPPFILES:\.cpp=\.o\)\s+\$\(CFILES:\.c=\.o\)\s+\$\(SFILES:\.s=\.o\)\s*\r?$') `
    'OFILES is no longer isolated from private-check objects'
$privateObjectStatements = @($makefileLogical -split '\r?\n' |
    Where-Object { $_ -match 'NDS_PRIVATE_CHECK_OFILES' } |
    ForEach-Object { (($_.Trim()) -replace '\s+', ' ') })
Assert-ExactSet $privateObjectStatements @(
    'export NDS_PRIVATE_CHECK_OFILES := $(NDS_PRIVATE_CHECK_CFILES:.c=.o)',
    ('export NDS_MPPROCESS_STRICT_OFILES := $(NDS_PRIVATE_CHECK_OFILES) ' +
     '$(if $(filter 1,$(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE)),' +
     '$(NDS_MPPROCESS_SOURCE_CFILES:.c=.o) battleship_mpprocess_live_bridge.o)'),
    'DEPENDS := $(OFILES:.o=.d) $(NDS_PRIVATE_CHECK_OFILES:.o=.d)',
    '$(OUTPUT).elf: $(OFILES) $(NDS_PRIVATE_CHECK_OFILES)',
    '$(OFILES) $(NDS_PRIVATE_CHECK_OFILES): $(PROJECT_ROOT)/Makefile $(NDS_BUILD_CONFIG)'
) 'Makefile private-object compile-only uses'
$strictObjectStatements = @($makefileLogical -split '\r?\n' |
    Where-Object { $_ -match 'NDS_MPPROCESS_STRICT_OFILES' } |
    ForEach-Object { (($_.Trim()) -replace '\s+', ' ') })
Assert-ExactSet $strictObjectStatements @(
    ('export NDS_MPPROCESS_STRICT_OFILES := $(NDS_PRIVATE_CHECK_OFILES) ' +
     '$(if $(filter 1,$(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE)),' +
     '$(NDS_MPPROCESS_SOURCE_CFILES:.c=.o) battleship_mpprocess_live_bridge.o)'),
    'ifneq ($(strip $(NDS_MPPROCESS_STRICT_OFILES)),)',
    ('$(NDS_MPPROCESS_STRICT_OFILES): CFLAGS += ' +
     '-Werror=implicit-function-declaration -Werror=incompatible-pointer-types ' +
     '-Werror=int-conversion -Werror=return-type')
) 'Makefile mpprocess strict-warning routing'
foreach ($sourceName in @('battleship_mpprocess_edge_support.c', 'battleship_mpprocess.c')) {
    Assert-Check ([regex]::Matches($makefileLogical, [regex]::Escape($sourceName)).Count -eq 1) `
        "Makefile must list $sourceName exactly once"
}
Assert-Check ([regex]::Matches($makefileLogical,
    'battleship_mpprocess_live_bridge\.c').Count -eq 1) `
    'Makefile must list battleship_mpprocess_live_bridge.c exactly once'

$elfSymbols = @(Invoke-Tool $Nm @('-a', '--defined-only', $elfPath) 'candidate ELF full symbol table') -join "`n"
Assert-Check ($elfSymbols -notmatch $leakPattern) 'private/support symbol leaked into the canonical ELF'

$freshnessElfPath = if ($RequireAB) { $liveElfPath } else { $elfPath }
$freshnessElfLabel = if ($RequireAB) { 'unstaged private-ON ELF' } else { 'candidate ELF' }
foreach ($pair in @(
    [pscustomobject]@{ Object = $objectPath; Rule = $mainRule },
    [pscustomobject]@{ Object = $supportObjectPath; Rule = $supportRule }
)) {
    $objectItem = Get-Item -LiteralPath $pair.Object
    $inputPaths = [System.Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    [void]$inputPaths.Add($makefilePath)
    foreach ($path in $pair.Rule.Prerequisites) { [void]$inputPaths.Add($path) }
    $newestInput = @($inputPaths | ForEach-Object { Get-Item -LiteralPath $_ } |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1)[0]
    Assert-Check ($objectItem.LastWriteTimeUtc -ge $newestInput.LastWriteTimeUtc) `
        "private-check object is stale: $($objectItem.Name) is older than $($newestInput.Name)"
    Assert-Check ((Get-Item -LiteralPath $freshnessElfPath).LastWriteTimeUtc -ge $objectItem.LastWriteTimeUtc) `
        "$freshnessElfLabel is older than $($objectItem.Name)"
}
$sceneObjectItem = Get-Item -LiteralPath $sceneObjectPath
$sceneInputPaths = [System.Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase)
[void]$sceneInputPaths.Add($makefilePath)
foreach ($path in $sceneRule.Prerequisites) { [void]$sceneInputPaths.Add($path) }
$newestSceneInput = @($sceneInputPaths | ForEach-Object {
        Get-Item -LiteralPath $_
    } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1)[0]
Assert-Check ($sceneObjectItem.LastWriteTimeUtc -ge
    $newestSceneInput.LastWriteTimeUtc) `
    "scene backend object is stale: $($sceneObjectItem.Name) is older than $($newestSceneInput.Name)"
Assert-Check ((Get-Item -LiteralPath $freshnessElfPath).LastWriteTimeUtc -ge
    $sceneObjectItem.LastWriteTimeUtc) `
    "$freshnessElfLabel is older than $($sceneObjectItem.Name)"
foreach ($privateObject in @($objectPath, $supportObjectPath)) {
    Assert-Check ((Get-Item -LiteralPath $mapPath).LastWriteTimeUtc -ge
        (Get-Item -LiteralPath $privateObject).LastWriteTimeUtc) `
        "canonical link map predates private-check object: $privateObject"
}

$canonicalFile = Get-Item -LiteralPath $canonicalRomPath
$shippedFile = Get-Item -LiteralPath $shippedRomPath
Assert-Check ($canonicalFile.Length -eq $shippedFile.Length) `
    "canonical/shipped ROM length mismatch: $($canonicalFile.Length) != $($shippedFile.Length)"
$canonicalHash = Get-Sha256 $canonicalRomPath
$shippedHash = Get-Sha256 $shippedRomPath
Assert-Check ($canonicalHash -ceq $shippedHash) `
    "canonical/shipped ROM SHA256 mismatch: $canonicalHash != $shippedHash"

$baselineArguments = @($LiveElf, $LiveCanonicalRom, $BaselineBuildDir,
    $BaselineElf, $BaselineRom) |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
if ((-not $RequireAB) -and ($baselineArguments.Count -ne 0)) {
    Fail-Check 'baseline arguments require -RequireAB and a complete OFF/ON artifact set'
}

$candidateLiveGate = Get-BuildConfigMacroValue $configText `
    'NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE' 'candidate build config'
Assert-Check ($candidateLiveGate -ceq '0') `
    "candidate live gate is $candidateLiveGate, expected 0"
$candidateGate = Get-BuildConfigGate $configText 'candidate build config'
Assert-Check ($candidateGate -ceq '1') "candidate private gate is $candidateGate, expected 1"
$provenanceSuffix = 'provenance=single-build live=0 compile-checkpoint-ab-qualified=false'
if ($RequireAB) {
    foreach ($required in @(
        [pscustomobject]@{ Value = $BaselineBuildDir; Label = 'BaselineBuildDir' },
        [pscustomobject]@{ Value = $BaselineElf; Label = 'BaselineElf' },
        [pscustomobject]@{ Value = $BaselineRom; Label = 'BaselineRom' },
        [pscustomobject]@{ Value = $LiveElf; Label = 'LiveElf' },
        [pscustomobject]@{ Value = $LiveCanonicalRom; Label = 'LiveCanonicalRom' }
    )) {
        Assert-Check (-not [string]::IsNullOrWhiteSpace($required.Value)) `
            "-RequireAB requires -$($required.Label)"
    }

    $baselineBuildPath = Resolve-Container $BaselineBuildDir 'baseline build directory'
    $baselineConfigPath = Resolve-Leaf (Join-Path $baselineBuildPath 'nds_build_config.h') `
        'baseline build config'
    $baselineElfPath = Resolve-Leaf $BaselineElf 'baseline ELF'
    $baselineRomPath = Resolve-Leaf $BaselineRom 'baseline ROM'
    Assert-DistinctPath $buildPath $baselineBuildPath 'candidate/baseline build directories'
    Assert-DistinctPath $elfPath $baselineElfPath 'candidate/baseline ELF artifacts'
    Assert-DistinctPath $canonicalRomPath $baselineRomPath 'candidate/baseline ROM artifacts'
    Assert-DistinctPath $elfPath $liveElfPath `
        'staged/unstaged private-ON ELF artifacts'
    Assert-DistinctPath $canonicalRomPath $liveCanonicalRomPath `
        'staged/unstaged private-ON ROM artifacts'

    $candidateElfHash = Get-Sha256 $elfPath
    $liveElfHash = Get-Sha256 $liveElfPath
    Assert-Check ($candidateElfHash -ceq $liveElfHash) `
        "staged/unstaged private-ON ELF SHA256 mismatch: $candidateElfHash != $liveElfHash"
    $liveCanonicalFile = Get-Item -LiteralPath $liveCanonicalRomPath
    $liveCanonicalHash = Get-Sha256 $liveCanonicalRomPath
    Assert-Check ($liveCanonicalFile.Length -eq $canonicalFile.Length) `
        "staged/unstaged private-ON ROM length mismatch: $($canonicalFile.Length) != $($liveCanonicalFile.Length)"
    Assert-Check ($liveCanonicalHash -ceq $canonicalHash) `
        "staged/unstaged private-ON ROM SHA256 mismatch: $canonicalHash != $liveCanonicalHash"

    $baselineConfigText = Get-Content -LiteralPath $baselineConfigPath -Raw
    $baselineLiveGate = Get-BuildConfigMacroValue $baselineConfigText `
        'NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE' 'baseline build config'
    Assert-Check ($baselineLiveGate -ceq '0') `
        "baseline live gate is $baselineLiveGate, expected 0"
    $baselineGate = Get-BuildConfigGate $baselineConfigText 'baseline build config'
    Assert-Check ($baselineGate -ceq '0') "baseline private gate is $baselineGate, expected 0"
    $candidateConfigWithoutGate = Remove-BuildConfigGateLine $configText 'candidate build config'
    $baselineConfigWithoutGate = Remove-BuildConfigGateLine $baselineConfigText 'baseline build config'
    Assert-Check ($candidateConfigWithoutGate -ceq $baselineConfigWithoutGate) `
        'OFF/ON build configs differ outside NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE'

    $baselineRomFile = Get-Item -LiteralPath $baselineRomPath
    $baselineRomHash = Get-Sha256 $baselineRomPath
    Assert-Check ($baselineRomFile.Length -eq $canonicalFile.Length) `
        "baseline/candidate ROM length mismatch: $($baselineRomFile.Length) != $($canonicalFile.Length)"
    Assert-Check ($baselineRomHash -ceq $canonicalHash) `
        "baseline/candidate ROM SHA256 mismatch: $baselineRomHash != $canonicalHash"

    $baselineAlloc = @(Get-AllocFingerprint $baselineElfPath)
    $candidateAlloc = @(Get-AllocFingerprint $elfPath)
    if (($baselineAlloc -join "`n") -cne ($candidateAlloc -join "`n")) {
        $delta = @(Compare-Object -ReferenceObject $baselineAlloc -DifferenceObject $candidateAlloc)
        $detail = @($delta | ForEach-Object { "$($_.SideIndicator)$($_.InputObject)" }) -join ', '
        Fail-Check "baseline/candidate allocated ELF sections differ: $detail"
    }
    $provenanceSuffix = 'provenance=deterministic-ab live=0 private-gates=0/1 staged-private-on=identical rom=identical elf-alloc=identical compile-checkpoint-ab-qualified=true'
}

Write-Output (('compile-checkpoint=PASS source=36+7 object=43 support-apis=2 ' +
    'dependencies=resolved-and-fresh upstream=sha256-pinned map=clean rom-bytes={0} sha256={1} {2}') -f
    $canonicalFile.Length, $canonicalHash, $provenanceSuffix)
Write-Output ('semantic-checkpoint=PASS contract=endpoint-world-plus-common-world-to-local ' +
    'providers=mpCollisionGetVertexPositionID,mpCollisionGetFloorEdgeL,mpCollisionGetFloorEdgeR,' +
    'mpCollisionGetCeilEdgeL,mpCollisionGetCeilEdgeR,mpCollisionGetLWallEdgeU,mpCollisionGetRWallEdgeU,' +
    'mpCollisionGetLWallEdgeD,mpCollisionGetRWallEdgeD,mpCollisionGetFCCommonCeil,' +
    'mpCollisionGetLRCommonLWall,mpCollisionGetLRCommonRWall transition=lower-edge-support-rebased')
Write-Output 'live-link=NOT-CLAIMED private-ab-live-gate=0 next=dedicated-live-link-checker-and-natural-runtime-gates'
exit 0
