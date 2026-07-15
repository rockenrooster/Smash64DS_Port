param(
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\builds\build-battle-playable-canonical-hwtri-harness'),
    [string]$Elf = (Join-Path $PSScriptRoot '..\smash64ds-battle-playable-hwtri.elf'),
    [string]$Nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe'
)
$ErrorActionPreference = 'Stop'

function Fail-Check {
    param([string]$Message)
    throw "mpprocess live-link check failed: $Message"
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
    $actualUnique = @($actualText | Sort-Object -CaseSensitive -Unique)
    $expectedUnique = @($expectedText | Sort-Object -CaseSensitive -Unique)
    Assert-Check ($actualText.Count -eq $actualUnique.Count) "$Label contains duplicate actual entries"
    Assert-Check ($expectedText.Count -eq $expectedUnique.Count) "$Label contains duplicate expected entries"
    $delta = @(Compare-Object -CaseSensitive -ReferenceObject $expectedUnique -DifferenceObject $actualUnique)
    if ($delta.Count -ne 0) {
        $detail = @($delta | ForEach-Object {
            "$($_.SideIndicator)$($_.InputObject)"
        }) -join ', '
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

function Get-AllDefinitions {
    param(
        [string]$Object,
        [switch]$ExternalOnly,
        [ValidateRange(0, 1)][int]$ExpectedAnonymousAbsolute = 0
    )
    $arguments = @('--defined-only')
    if ($ExternalOnly) { $arguments += '-g' } else { $arguments += '-a' }
    $arguments += $Object
    $records = @()
    $anonymousAbsoluteCount = 0
    foreach ($line in @(Invoke-Tool $Nm $arguments "nm complete definitions for $Object")) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -match '^[0-9a-fA-F]+\s+[aA]\s*$') {
            $anonymousAbsoluteCount++
            continue
        }
        if ($line -notmatch '^([0-9a-fA-F]+)\s+(\S)\s+(\S+)$') {
            Fail-Check "unparsed complete nm definition line for $Object`: $line"
        }
        $records += [pscustomobject]@{
            Address = [Convert]::ToUInt64($Matches[1], 16)
            Name = $Matches[3]
            Type = $Matches[2]
        }
    }
    Assert-Check ($anonymousAbsoluteCount -eq $ExpectedAnonymousAbsolute) `
        "complete nm definitions for $Object contain $anonymousAbsoluteCount anonymous absolute records, expected $ExpectedAnonymousAbsolute"
    return $records
}

function Get-Undefined {
    param([string]$Object)
    $names = @()
    foreach ($line in @(Invoke-Tool $Nm @('-u', $Object) "nm undefined symbols for $Object")) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^\s+U\s+(\S+)\s*$') {
            Fail-Check "unparsed or non-strong undefined symbol for $Object`: $line"
        }
        $names += $Matches[1]
    }
    return $names
}

function Get-ObjectSetGlobalDefinitions {
    param([string[]]$Objects)
    Assert-Check ($Objects.Count -gt 0) 'link map did not provide an object set'
    $lines = @(Invoke-Tool $Nm (@('-A', '-g', '--defined-only') + $Objects) 'link-map object definitions')
    $records = @()
    foreach ($line in $lines) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^(.+):([0-9a-fA-F]+)\s+(\S)\s+(\S+)\s*$') {
            Fail-Check "unparsed link-map object definition line: $line"
        }
        $objectPath = [IO.Path]::GetFullPath($Matches[1])
        $records += [pscustomobject]@{
            Address = [Convert]::ToUInt64($Matches[2], 16)
            Object = $objectPath
            Type = $Matches[3]
            Name = $Matches[4]
        }
    }
    return $records
}

function Get-ObjectSetAllDefinitions {
    param([string[]]$Objects)
    Assert-Check ($Objects.Count -gt 0) 'link map did not provide an all-definition object set'
    $lines = @(Invoke-Tool $Nm (@('-A', '-a', '--defined-only') + $Objects) `
        'link-map object complete definitions')
    $records = @()
    foreach ($line in $lines) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        Assert-Check ($line -notmatch '^.+:[0-9a-fA-F]+\s+[aA]\s*$') `
            "direct link object has an anonymous absolute definition: $line"
        if ($line -notmatch '^(.+):([0-9a-fA-F]+)\s+(\S)\s+(\S+)$') {
            Fail-Check "unparsed link-map object complete definition line: $line"
        }
        $records += [pscustomobject]@{
            Address = [Convert]::ToUInt64($Matches[2], 16)
            Object = [IO.Path]::GetFullPath($Matches[1])
            Type = $Matches[3]
            Name = $Matches[4]
        }
    }
    return $records
}

function Get-ArchiveAllDefinitions {
    param([string[]]$Archives)
    $records = @()
    foreach ($archive in $Archives) {
        foreach ($line in @(Invoke-Tool $Nm `
            @('-A', '-a', '--defined-only', $archive) `
            "archive complete definitions for $archive")) {
            if ([string]::IsNullOrWhiteSpace($line)) { continue }
            Assert-Check ($line -notmatch '^.+:[0-9a-fA-F]+\s+[aA]\s*$') `
                "link archive has an anonymous absolute definition: $line"
            if ($line -notmatch '^(.+):([0-9a-fA-F]+)\s+(\S)\s+(\S+)$') {
                Fail-Check "unparsed archive complete definition line for $archive`: $line"
            }
            $records += [pscustomobject]@{
                Address = [Convert]::ToUInt64($Matches[2], 16)
                Location = $Matches[1]
                Type = $Matches[3]
                Name = $Matches[4]
            }
        }
    }
    return $records
}

function Get-SectionNames {
    param([string]$Object)
    $names = @()
    foreach ($line in @(Invoke-Tool $Objdump @('-h', $Object) "section table for $Object")) {
        if ($line -match '^\s*\d+\s+(\S+)\s+[0-9a-fA-F]+\s+') {
            $names += $Matches[1]
        }
    }
    Assert-Check ($names.Count -gt 0) "no sections parsed from $Object"
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
    $match = [regex]::Match(
        $logical.ToString(),
        ('^\s*' + [regex]::Escape($Target) + '[ \t]*:[ \t]*(.*)$'),
        [Text.RegularExpressions.RegexOptions]::Singleline
    )
    Assert-Check $match.Success "dependency file does not start with exact $Target rule: $DependencyFile"
    $words = @(ConvertFrom-MakeWords $match.Groups[1].Value "$Target dependency rule")
    Assert-Check ($words.Count -gt 0) "$Target dependency rule has no prerequisites"
    $paths = New-Object 'System.Collections.Generic.List[string]'
    $keys = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($word in $words) {
        Assert-Check ($word -cne '|') "$Target dependency rule contains an order-only separator"
        $candidate = if ([IO.Path]::IsPathRooted($word)) {
            [IO.Path]::GetFullPath($word)
        } else {
            [IO.Path]::GetFullPath((Join-Path $ProjectRoot $word))
        }
        Assert-Check (Test-Path -LiteralPath $candidate -PathType Leaf) "$Target dependency prerequisite is missing: $candidate"
        $resolved = (Resolve-Path -LiteralPath $candidate).Path
        Assert-Check ($keys.Add($resolved)) "$Target dependency rule contains a duplicate prerequisite: $resolved"
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
    $actual = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $Rule.Prerequisites) { [void]$actual.Add($path) }
    foreach ($path in $Required) {
        $requiredPath = (Resolve-Path -LiteralPath $path).Path
        Assert-Check ($actual.Contains($requiredPath)) "$Label dependency is missing exact prerequisite '$requiredPath'"
    }
    foreach ($path in $Forbidden) {
        $forbiddenPath = (Resolve-Path -LiteralPath $path).Path
        Assert-Check (-not $actual.Contains($forbiddenPath)) "$Label unexpectedly depends on '$forbiddenPath'"
    }
}

function Get-BuildConfigMacroValue {
    param([string]$Text, [string]$Name, [string]$Label)
    $matches = @([regex]::Matches(
        $Text,
        ('(?m)^[ \t]*#define[ \t]+' + [regex]::Escape($Name) +
         '[ \t]+([^\r\n]*?)[ \t]*\r?$')
    ))
    Assert-Check ($matches.Count -eq 1) "$Label must define $Name exactly once"
    return $matches[0].Groups[1].Value.Trim()
}

function Get-CFunctionText {
    param([string]$Text, [string]$Name, [string]$Label)
    $matches = @([regex]::Matches(
        $Text,
        ('(?m)^[ \t]*(?:static[ \t]+)?(?:sb32|void)[ \t]+' +
         [regex]::Escape($Name) + '[ \t]*\(')
    ))
    Assert-Check ($matches.Count -eq 1) "$Label must define $Name exactly once"
    $openBrace = $Text.IndexOf('{', $matches[0].Index)
    Assert-Check ($openBrace -ge 0) "$Label function $Name has no body"
    $depth = 0
    for ($i = $openBrace; $i -lt $Text.Length; $i++) {
        if ($Text[$i] -eq '{') { $depth++ }
        elseif ($Text[$i] -eq '}') {
            $depth--
            if ($depth -eq 0) {
                return $Text.Substring($matches[0].Index, ($i - $matches[0].Index) + 1)
            }
        }
    }
    Fail-Check "$Label function $Name has an unterminated body"
}

function Assert-FreshObject {
    param(
        [string]$Object,
        [object]$Rule,
        [string]$Makefile,
        [string]$ElfPath,
        [string]$MapPath,
        [string]$Label
    )
    $inputs = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    [void]$inputs.Add($Makefile)
    foreach ($path in $Rule.Prerequisites) { [void]$inputs.Add($path) }
    $newestInput = @($inputs | ForEach-Object { Get-Item -LiteralPath $_ } |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1)[0]
    $objectItem = Get-Item -LiteralPath $Object
    Assert-Check ($objectItem.Length -gt 0) "$Label object is empty"
    Assert-Check ($objectItem.LastWriteTimeUtc -ge $newestInput.LastWriteTimeUtc) "$Label object is stale: older than $($newestInput.Name)"
    Assert-Check ((Get-Item -LiteralPath $ElfPath).LastWriteTimeUtc -ge $objectItem.LastWriteTimeUtc) "ELF is older than $Label object"
    Assert-Check ((Get-Item -LiteralPath $MapPath).LastWriteTimeUtc -ge $objectItem.LastWriteTimeUtc) "link map is older than $Label object"
}

function Assert-SoleProvider {
    param(
        [object[]]$Definitions,
        [string]$Name,
        [string]$ExpectedObject,
        [string]$ExpectedType,
        [string]$Label
    )
    $providers = @($Definitions | Where-Object { $_.Name -ceq $Name })
    Assert-Check ($providers.Count -eq 1) "$Label '$Name' has $($providers.Count) providers, expected one"
    Assert-Check ([string]::Equals(
        [IO.Path]::GetFullPath($providers[0].Object),
        [IO.Path]::GetFullPath($ExpectedObject),
        [StringComparison]::OrdinalIgnoreCase
    )) "$Label '$Name' is provided by $($providers[0].Object), expected $ExpectedObject"
    Assert-Check ($providers[0].Type -ceq $ExpectedType) "$Label '$Name' has type $($providers[0].Type), expected $ExpectedType"
}

function Test-RelevantName {
    param([string]$Name)
    return (($Name -cmatch '^(?:mpProcess|ndsBaseMPProcess|sNdsBaseMPProcess)\w+$') -or
        ($script:SupportApiNames -ccontains $Name))
}

function Test-MPProcessNamespaceName {
    param([string]$Name)
    return (($Name -cmatch `
            '^(?:mpProcess|ndsBaseMPProcess|sNdsBaseMPProcess)\w+(?:\..+)?$') -or
        ($Name -cmatch `
            '^(?:ndsPortWeak|ndsLegacy)MPProcess\w+(?:\..+)?$') -or
        ($Name -cmatch `
            '^mpCollisionGet[LR]WallEdgeD(?:\..+)?$') -or
        ($Name -cmatch `
            '^ndsPrivateMPGetWallEdgeD(?:\..+)?$'))
}

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$buildPath = Resolve-Container $BuildDir 'live build directory'
$elfPath = Resolve-Leaf $Elf 'live ELF'
$Nm = Resolve-Leaf $Nm 'arm-none-eabi-nm'
$Objdump = Resolve-Leaf $Objdump 'arm-none-eabi-objdump'
$mapPath = Resolve-Leaf (Join-Path $buildPath '.map') 'live link map'
$configPath = Resolve-Leaf (Join-Path $buildPath 'nds_build_config.h') 'live build config'
$makefilePath = Resolve-Leaf (Join-Path $root 'Makefile') 'Makefile'

$wrapperPath = Resolve-Leaf (Join-Path $root 'src\import\battleship_mpprocess.c') 'mpprocess source wrapper'
$supportSourcePath = Resolve-Leaf (Join-Path $root 'src\port\battleship_mpprocess_edge_support.c') 'mpprocess edge support source'
$bridgeSourcePath = Resolve-Leaf (Join-Path $root 'src\port\battleship_mpprocess_live_bridge.c') 'mpprocess live bridge source'
$sceneSourcePath = Resolve-Leaf (Join-Path $root 'src\port\scene_backend.c') 'scene backend source'
$collisionSourcePath = Resolve-Leaf (Join-Path $root 'src\port\reloc_backend_mp_collision.c') 'collision backend source'
$wpSourcePath = Resolve-Leaf (Join-Path $root 'src\import\battleship_wpmanager_core.c') 'wpmanager wrapper source'
$privateHeaderPath = Resolve-Leaf (Join-Path $root 'include\nds\nds_mpprocess_source.h') 'mpprocess source header'
$mapHeaderPath = Resolve-Leaf (Join-Path $root 'include\mp\map.h') 'public map header'
$upstreamPath = Resolve-Leaf (Join-Path $root 'decomp\BattleShip-main\decomp\src\mp\mpprocess.c') 'BattleShip mpprocess source'
$wpUpstreamPath = Resolve-Leaf (Join-Path $root 'decomp\BattleShip-main\decomp\src\wp\wpmanager.c') 'BattleShip wpmanager source'

$importObjectPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess.o') 'mpprocess import object'
$supportObjectPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess_edge_support.o') 'mpprocess edge support object'
$bridgeObjectPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess_live_bridge.o') 'mpprocess live bridge object'
$sceneObjectPath = Resolve-Leaf (Join-Path $buildPath 'scene_backend.o') 'scene backend object'
$wpObjectPath = Resolve-Leaf (Join-Path $buildPath 'battleship_wpmanager_core.o') 'wpmanager object'
$importDependencyPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess.d') 'mpprocess import dependency file'
$supportDependencyPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess_edge_support.d') 'mpprocess support dependency file'
$bridgeDependencyPath = Resolve-Leaf (Join-Path $buildPath 'battleship_mpprocess_live_bridge.d') 'mpprocess bridge dependency file'
$sceneDependencyPath = Resolve-Leaf (Join-Path $buildPath 'scene_backend.d') 'scene backend dependency file'
$wpDependencyPath = Resolve-Leaf (Join-Path $buildPath 'battleship_wpmanager_core.d') 'wpmanager dependency file'

$configText = Get-Content -LiteralPath $configPath -Raw
$expectedConfig = [ordered]@{
    NDS_BUILD_HARNESS_VARIANT = '"battle_playable_realtime"'
    NDS_RENDERER_HW_TRIANGLES = '1'
    NDS_RENDERER_PROFILE_LEVEL = '0'
    NDS_IMPORT_BATTLESHIP_FTMANAGER = '1'
    NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER = '1'
    NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE = '1'
    NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE = '0'
    NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE = '1'
}
foreach ($name in $expectedConfig.Keys) {
    $actual = Get-BuildConfigMacroValue $configText $name 'live build config'
    Assert-Check ($actual -ceq $expectedConfig[$name]) "live build config $name is '$actual', expected '$($expectedConfig[$name])'"
}

$expectedUpstreamHash = '42639625B85ACD7CFC50416C378BBDE59747A8DD1CFCBCA4AACADF949405C3B9'
$expectedWpUpstreamHash = 'E3F026C7C056155A4DB75013B1FCB8EBF52194A4A8CD9FA8FFC46AA536FAEB99'
Assert-Check ((Get-FileHash -LiteralPath $upstreamPath -Algorithm SHA256).Hash -ceq $expectedUpstreamHash) 'BattleShip mpprocess.c SHA256 drifted'
Assert-Check ((Get-FileHash -LiteralPath $wpUpstreamPath -Algorithm SHA256).Hash -ceq $expectedWpUpstreamHash) 'BattleShip wpmanager.c SHA256 drifted'

$wrapperText = Get-Content -LiteralPath $wrapperPath -Raw
$wrapperNormalized = $wrapperText -replace '\\\r?\n[ \t]*', ' '
$upstreamText = Get-Content -LiteralPath $upstreamPath -Raw
$functionMaps = @([regex]::Matches($wrapperNormalized, '(?m)^#define[ \t]+(mpProcess\w+)[ \t]+(ndsBaseMPProcess\w+)[ \t]*\r?$'))
$globalMaps = @([regex]::Matches($wrapperNormalized, '(?m)^#define[ \t]+(sMPProcess\w+)[ \t]+(sNdsBaseMPProcess\w+)[ \t]*\r?$'))
Assert-Check ($functionMaps.Count -eq 36) "mpprocess wrapper function map count is $($functionMaps.Count), expected 36"
Assert-Check ($globalMaps.Count -eq 7) "mpprocess wrapper data map count is $($globalMaps.Count), expected 7"
$functionSources = @($functionMaps | ForEach-Object { $_.Groups[1].Value })
$functionTargets = @($functionMaps | ForEach-Object { $_.Groups[2].Value })
$globalSources = @($globalMaps | ForEach-Object { $_.Groups[1].Value })
$globalTargets = @($globalMaps | ForEach-Object { $_.Groups[2].Value })
$upstreamFunctions = @([regex]::Matches($upstreamText, '(?m)^(?:void|sb32)\s+(mpProcess\w+)\s*\(') | ForEach-Object { $_.Groups[1].Value })
$upstreamGlobals = @([regex]::Matches($upstreamText, '(?m)^(?:s32|f32|u32|Vec3f)\s+(sMPProcess\w+)(?:\[[^\]]+\])?\s*;') | ForEach-Object { $_.Groups[1].Value })
Assert-ExactSet $functionSources $upstreamFunctions 'BattleShip mpprocess function mappings'
Assert-ExactSet $globalSources $upstreamGlobals 'BattleShip mpprocess data mappings'
for ($i = 0; $i -lt $functionSources.Count; $i++) {
    $expected = 'ndsBaseMPProcess' + $functionSources[$i].Substring('mpProcess'.Length)
    Assert-Check ($functionTargets[$i] -ceq $expected) "noncanonical mpprocess function mapping for $($functionSources[$i])"
}
for ($i = 0; $i -lt $globalSources.Count; $i++) {
    $expected = 'sNdsBaseMPProcess' + $globalSources[$i].Substring('sMPProcess'.Length)
    Assert-Check ($globalTargets[$i] -ceq $expected) "noncanonical mpprocess data mapping for $($globalSources[$i])"
}

$bridgePublic = @'
mpProcessSetCollProjectFloorID
mpProcessResetMultiWallCount
mpProcessSetMultiWallLineID
mpProcessCheckTestLWallCollision
mpProcessRunLWallCollision
mpProcessCheckTestRWallCollision
mpProcessRunRWallCollision
mpProcessCheckTestFloorCollisionNew
mpProcessCheckTestFloorCollision
mpProcessCheckTestLCliffCollision
mpProcessCheckTestRCliffCollision
mpProcessCheckTestLWallCollisionAdjNew
mpProcessRunLWallCollisionAdjNew
mpProcessCheckTestRWallCollisionAdjNew
mpProcessRunRWallCollisionAdjNew
mpProcessSetLandingFloor
mpProcessSetCollideFloor
mpProcessCheckTestCeilCollisionAdjNew
mpProcessRunCeilCollisionAdjNew
mpProcessRunCeilEdgeAdjust
mpProcessCheckTestFloorCollisionAdjNew
mpProcessRunFloorCollisionAdjNewNULL
mpProcessCheckFloorEdgeCollisionL
mpProcessFloorEdgeLAdjust
mpProcessCheckFloorEdgeCollisionR
mpProcessFloorEdgeRAdjust
mpProcessRunFloorEdgeAdjust
mpProcessUpdateMain
'@ -split '\r?\n' | Where-Object { $_ }
Assert-Check ($bridgePublic.Count -eq 28) 'internal bridge public-symbol census is not 28'
$bridgeBase = @($bridgePublic | ForEach-Object {
    'ndsBaseMPProcess' + $_.Substring('mpProcess'.Length)
})
foreach ($base in $bridgeBase) {
    Assert-Check ($functionTargets -ccontains $base) "bridge calls non-source function $base"
}
$bridgeText = Get-Content -LiteralPath $bridgeSourcePath -Raw
$bridgeSourceDefinitions = @([regex]::Matches(
    $bridgeText,
    '(?m)^(?:void|sb32)\s+(mpProcess\w+)\s*\('
) | ForEach-Object { $_.Groups[1].Value })
Assert-ExactSet $bridgeSourceDefinitions $bridgePublic 'live bridge source definitions'
for ($i = 0; $i -lt $bridgePublic.Count; $i++) {
    $body = Get-CFunctionText $bridgeText $bridgePublic[$i] 'live bridge source'
    $calls = @([regex]::Matches($body, '\b(ndsBaseMPProcess\w+)\s*\(') | ForEach-Object { $_.Groups[1].Value })
    Assert-ExactSet $calls @($bridgeBase[$i]) "live bridge forwarding body $($bridgePublic[$i])"
}

$script:SupportApiNames = @('mpCollisionGetLWallEdgeD', 'mpCollisionGetRWallEdgeD')
$supportLocalName = 'ndsPrivateMPGetWallEdgeD'
$expectedFinalPublic = @'
mpProcessCheckTestCeilCollisionAdjNew
mpProcessCheckTestFloorCollision
mpProcessCheckTestFloorCollisionAdjNew
mpProcessCheckTestFloorCollisionNew
mpProcessCheckTestLCliffCollision
mpProcessCheckTestLWallCollisionAdjNew
mpProcessCheckTestRCliffCollision
mpProcessCheckTestRWallCollisionAdjNew
mpProcessRunCeilCollisionAdjNew
mpProcessRunCeilEdgeAdjust
mpProcessRunFloorCollisionAdjNewNULL
mpProcessRunFloorEdgeAdjust
mpProcessRunLWallCollisionAdjNew
mpProcessRunRWallCollisionAdjNew
mpProcessSetCollideFloor
mpProcessSetCollProjectFloorID
mpProcessSetLandingFloor
mpProcessUpdateMain
'@ -split '\r?\n' | Where-Object { $_ }
$expectedFinalInternalBase = @'
ndsBaseMPProcessCeilEdgeAdjustLeft
ndsBaseMPProcessCeilEdgeAdjustRight
ndsBaseMPProcessCheckCeilEdgeCollisionL
ndsBaseMPProcessCheckCeilEdgeCollisionR
ndsBaseMPProcessCheckFloorEdgeCollisionL
ndsBaseMPProcessCheckFloorEdgeCollisionR
ndsBaseMPProcessFloorEdgeLAdjust
ndsBaseMPProcessFloorEdgeRAdjust
'@ -split '\r?\n' | Where-Object { $_ }
$expectedFinalBaseFunctions = @(
    @($expectedFinalPublic | ForEach-Object {
        'ndsBaseMPProcess' + $_.Substring('mpProcess'.Length)
    }) + $expectedFinalInternalBase
)
$expectedFinalData = @'
sNdsBaseMPProcessLastWallAngle
sNdsBaseMPProcessLastWallCollidePosition
sNdsBaseMPProcessLastWallFlags
sNdsBaseMPProcessLastWallLineID
sNdsBaseMPProcessMultiWallCollideLineIDs
sNdsBaseMPProcessMultiWallCollidesNum
'@ -split '\r?\n' | Where-Object { $_ }
$expectedFinalSupport = @($script:SupportApiNames)
$expectedFinalNames = @(
    $expectedFinalPublic + $expectedFinalBaseFunctions +
    $expectedFinalData + $expectedFinalSupport
)
Assert-Check ($expectedFinalPublic.Count -eq 18) `
    'canonical final public mpprocess set is not 18'
Assert-Check ($expectedFinalBaseFunctions.Count -eq 26) `
    'canonical final source-function set is not 26'
Assert-Check ($expectedFinalData.Count -eq 6) `
    'canonical final source-data set is not 6'
Assert-Check ($expectedFinalSupport.Count -eq 2) `
    'canonical final edge-support set is not 2'
foreach ($name in $expectedFinalPublic) {
    Assert-Check ($bridgePublic -ccontains $name) `
        "canonical final public symbol is absent from the bridge census: $name"
}
foreach ($name in $expectedFinalBaseFunctions) {
    Assert-Check ($functionTargets -ccontains $name) `
        "canonical final source function is absent from the imported census: $name"
}
foreach ($name in $expectedFinalData) {
    Assert-Check ($globalTargets -ccontains $name) `
        "canonical final source data is absent from the imported census: $name"
}
$runtimeCriticalNames = @(
    'mpProcessUpdateMain',
    'ndsBaseMPProcessUpdateMain',
    'mpProcessCheckTestFloorCollisionNew',
    'ndsBaseMPProcessCheckTestFloorCollisionNew',
    'mpProcessSetLandingFloor',
    'ndsBaseMPProcessSetLandingFloor',
    'mpProcessSetCollideFloor',
    'ndsBaseMPProcessSetCollideFloor',
    'mpCollisionGetLWallEdgeD',
    'mpCollisionGetRWallEdgeD'
)
foreach ($name in $runtimeCriticalNames) {
    Assert-Check ($expectedFinalNames -ccontains $name) `
        "runtime-critical mpprocess symbol is absent from the canonical closure: $name"
}
$expectedImportUndefined = @'
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
    '__aeabi_fcmplt',
    'mpCollisionGetVertexCountLineID',
    'mpCollisionGetVertexPositionID'
)

$importDefinitions = @(Get-Definitions $importObjectPath)
Assert-ExactSet @($importDefinitions.Name) @($functionTargets + $globalTargets) 'mpprocess import definitions'
foreach ($definition in $importDefinitions) {
    Assert-Check ($definition.Size -gt 0) "zero-sized mpprocess import definition: $($definition.Name)"
    $expectedType = if ($functionTargets -ccontains $definition.Name) { 'T' } else { 'B' }
    Assert-Check ($definition.Type -ceq $expectedType) "mpprocess import $($definition.Name) has type $($definition.Type), expected $expectedType"
}
Assert-ExactSet @(Get-Undefined $importObjectPath) $expectedImportUndefined 'mpprocess import undefined symbols'

$supportDefinitions = @(Get-Definitions $supportObjectPath)
$supportExports = @(Get-Definitions $supportObjectPath -ExternalOnly)
Assert-ExactSet @($supportDefinitions.Name) @($script:SupportApiNames + $supportLocalName) 'mpprocess support definitions'
Assert-ExactSet @($supportExports.Name) $script:SupportApiNames 'mpprocess support exports'
foreach ($definition in $supportDefinitions) {
    Assert-Check ($definition.Size -gt 0) "zero-sized support definition: $($definition.Name)"
    $expectedType = if ($script:SupportApiNames -ccontains $definition.Name) { 'T' } else { 't' }
    Assert-Check ($definition.Type -ceq $expectedType) "support $($definition.Name) has type $($definition.Type), expected $expectedType"
}
Assert-ExactSet @(Get-Undefined $supportObjectPath) $expectedSupportUndefined 'mpprocess support undefined symbols'

$bridgeDefinitions = @(Get-Definitions $bridgeObjectPath)
Assert-ExactSet @($bridgeDefinitions.Name) $bridgePublic 'mpprocess live bridge definitions'
foreach ($definition in $bridgeDefinitions) {
    Assert-Check ($definition.Size -gt 0) "zero-sized bridge definition: $($definition.Name)"
    Assert-Check ($definition.Type -ceq 'T') "bridge $($definition.Name) has type $($definition.Type), expected T"
}
Assert-ExactSet @(Get-Undefined $bridgeObjectPath) $bridgeBase 'mpprocess live bridge undefined symbols'

$importSections = @(Get-SectionNames $importObjectPath)
$supportSections = @(Get-SectionNames $supportObjectPath)
$bridgeSections = @(Get-SectionNames $bridgeObjectPath)
Assert-ExactSet @($importSections | Where-Object { $_ -cmatch '^\.text\.ndsBaseMPProcess\w+$' }) @($functionTargets | ForEach-Object { ".text.$_" }) 'mpprocess import function sections'
Assert-ExactSet @($importSections | Where-Object { $_ -cmatch '^\.(?:bss|data)\.sNdsBaseMPProcess\w+$' }) @($globalTargets | ForEach-Object { ".bss.$_" }) 'mpprocess import data sections'
Assert-ExactSet @($supportSections | Where-Object { $_ -cmatch '^\.text\.(?:mpCollisionGet[LR]WallEdgeD|ndsPrivateMPGetWallEdgeD)$' }) @(($script:SupportApiNames + $supportLocalName) | ForEach-Object { ".text.$_" }) 'mpprocess support function sections'
Assert-ExactSet @($bridgeSections | Where-Object { $_ -cmatch '^\.text\.mpProcess\w+$' }) @($bridgePublic | ForEach-Object { ".text.$_" }) 'mpprocess bridge function sections'

foreach ($objectInfo in @(
    [pscustomobject]@{ Path = $sceneObjectPath; Label = 'scene backend' },
    [pscustomobject]@{ Path = $wpObjectPath; Label = 'wpmanager' }
)) {
    $legacyDefinitions = @(Get-AllDefinitions $objectInfo.Path | Where-Object {
        Test-MPProcessNamespaceName $_.Name
    })
    Assert-Check ($legacyDefinitions.Count -eq 0) "$($objectInfo.Label) still defines mpprocess symbols (including local or weak): $(@($legacyDefinitions.Name) -join ',')"
}

$importRule = Get-DependencyRule $importDependencyPath 'battleship_mpprocess.o' $root
$supportRule = Get-DependencyRule $supportDependencyPath 'battleship_mpprocess_edge_support.o' $root
$bridgeRule = Get-DependencyRule $bridgeDependencyPath 'battleship_mpprocess_live_bridge.o' $root
$sceneRule = Get-DependencyRule $sceneDependencyPath 'scene_backend.o' $root
$wpRule = Get-DependencyRule $wpDependencyPath 'battleship_wpmanager_core.o' $root
Assert-DependencyRule $importRule @($wrapperPath, $configPath, $privateHeaderPath, $mapHeaderPath, $upstreamPath) @($supportSourcePath, $bridgeSourcePath) 'mpprocess import object'
Assert-DependencyRule $supportRule @($supportSourcePath, $configPath, $privateHeaderPath, $mapHeaderPath) @($wrapperPath, $bridgeSourcePath, $upstreamPath) 'mpprocess support object'
Assert-DependencyRule $bridgeRule @($bridgeSourcePath, $configPath, $privateHeaderPath, $mapHeaderPath) @($wrapperPath, $supportSourcePath, $upstreamPath) 'mpprocess bridge object'
Assert-DependencyRule $sceneRule @($sceneSourcePath, $collisionSourcePath, $configPath) @($wrapperPath, $supportSourcePath, $bridgeSourcePath, $upstreamPath) 'scene backend object'
Assert-DependencyRule $wpRule @($wpSourcePath, $wpUpstreamPath, $configPath) @($wrapperPath, $supportSourcePath, $bridgeSourcePath, $upstreamPath) 'wpmanager object'

foreach ($fresh in @(
    [pscustomobject]@{ Object = $importObjectPath; Rule = $importRule; Label = 'mpprocess import' },
    [pscustomobject]@{ Object = $supportObjectPath; Rule = $supportRule; Label = 'mpprocess support' },
    [pscustomobject]@{ Object = $bridgeObjectPath; Rule = $bridgeRule; Label = 'mpprocess bridge' },
    [pscustomobject]@{ Object = $sceneObjectPath; Rule = $sceneRule; Label = 'scene backend' },
    [pscustomobject]@{ Object = $wpObjectPath; Rule = $wpRule; Label = 'wpmanager' }
)) {
    Assert-FreshObject $fresh.Object $fresh.Rule $makefilePath $elfPath $mapPath $fresh.Label
}

$mapText = Get-Content -LiteralPath $mapPath -Raw
$allLoadInputs = @([regex]::Matches($mapText, '(?m)^LOAD (.+?)\r?$') | ForEach-Object {
    $_.Groups[1].Value
})
$loadObjects = @($allLoadInputs | Where-Object { $_ -cmatch '(?i)\.o$' })
$loadArchives = @($allLoadInputs | Where-Object { $_ -cmatch '(?i)\.a$' })
$loadSynthetic = @($allLoadInputs | Where-Object { $_ -ceq 'linker stubs' })
Assert-Check ($loadObjects.Count -gt 0) `
    'live link map has no direct object LOAD records'
Assert-ExactSet $loadSynthetic @('linker stubs') `
    'live link map synthetic LOAD inputs'
Assert-Check (($loadObjects.Count + $loadArchives.Count +
    $loadSynthetic.Count) -eq
    $allLoadInputs.Count) `
    'live link map has an unclassified LOAD input outside .o/.a/linker-stub ownership census'
$mapObjects = @()
foreach ($loadObject in $loadObjects) {
    $candidate = if ([IO.Path]::IsPathRooted($loadObject)) {
        $loadObject
    } else {
        Join-Path $buildPath $loadObject
    }
    Assert-Check (Test-Path -LiteralPath $candidate -PathType Leaf) "live map object is missing: $loadObject"
    $mapObjects += (Resolve-Path -LiteralPath $candidate).Path
}
Assert-ExactSet $mapObjects @($mapObjects | Sort-Object -CaseSensitive -Unique) 'live map object paths'
$mapArchives = @()
foreach ($loadArchive in $loadArchives) {
    $candidate = if ([IO.Path]::IsPathRooted($loadArchive)) {
        $loadArchive
    } else {
        Join-Path $buildPath $loadArchive
    }
    Assert-Check (Test-Path -LiteralPath $candidate -PathType Leaf) `
        "live map archive is missing: $loadArchive"
    $mapArchives += (Resolve-Path -LiteralPath $candidate).Path
}
Assert-ExactSet $mapArchives `
    @($mapArchives | Sort-Object -CaseSensitive -Unique) `
    'live map archive paths'
foreach ($requiredObject in @($importObjectPath, $supportObjectPath, $bridgeObjectPath, $sceneObjectPath, $wpObjectPath)) {
    Assert-Check (@($mapObjects | Where-Object {
        [string]::Equals($_, $requiredObject, [StringComparison]::OrdinalIgnoreCase)
    }).Count -eq 1) "required live object is absent from link map: $requiredObject"
}
foreach ($linkInput in @($mapObjects + $mapArchives)) {
    $inputItem = Get-Item -LiteralPath $linkInput
    Assert-Check ((Get-Item -LiteralPath $elfPath).LastWriteTimeUtc -ge
        $inputItem.LastWriteTimeUtc) `
        "live ELF predates link input: $linkInput"
    Assert-Check ((Get-Item -LiteralPath $mapPath).LastWriteTimeUtc -ge
        $inputItem.LastWriteTimeUtc) `
        "live link map predates link input: $linkInput"
}

$mapObjectDefinitions = @(Get-ObjectSetGlobalDefinitions $mapObjects)
$mapObjectAllDefinitions = @(Get-ObjectSetAllDefinitions $mapObjects)
$archiveAllDefinitions = @(Get-ArchiveAllDefinitions $mapArchives)
$expectedOwnedNames = @($bridgePublic + $functionTargets + $globalTargets + $script:SupportApiNames)
$actualOwnedDefinitions = @($mapObjectDefinitions | Where-Object { Test-RelevantName $_.Name })
Assert-ExactSet @($actualOwnedDefinitions.Name) $expectedOwnedNames 'live map mpprocess ownership namespace'
$directNamespaceDefinitions = @($mapObjectAllDefinitions | Where-Object {
    Test-MPProcessNamespaceName $_.Name
})
Assert-ExactSet @($directNamespaceDefinitions.Name) `
    @($expectedOwnedNames + $supportLocalName) `
    'live direct-object complete mpprocess namespace'
$archiveNamespaceDefinitions = @($archiveAllDefinitions | Where-Object {
    Test-MPProcessNamespaceName $_.Name
})
Assert-Check ($archiveNamespaceDefinitions.Count -eq 0) `
    "link archives define mpprocess providers: $(@($archiveNamespaceDefinitions | ForEach-Object { "$($_.Location):$($_.Type):$($_.Name)" }) -join ',')"
$weakInputDefinitions = @(
    @($directNamespaceDefinitions + $archiveNamespaceDefinitions) |
        Where-Object { $_.Type -cmatch '^[WwVv]$' }
)
Assert-Check ($weakInputDefinitions.Count -eq 0) `
    "link inputs retain weak mpprocess providers: $(@($weakInputDefinitions.Name) -join ',')"
foreach ($name in $bridgePublic) {
    Assert-SoleProvider $mapObjectDefinitions $name $bridgeObjectPath 'T' 'public bridge ownership'
}
foreach ($name in $functionTargets) {
    Assert-SoleProvider $mapObjectDefinitions $name $importObjectPath 'T' 'source function ownership'
}
foreach ($name in $globalTargets) {
    Assert-SoleProvider $mapObjectDefinitions $name $importObjectPath 'B' 'source data ownership'
}
foreach ($name in $script:SupportApiNames) {
    Assert-SoleProvider $mapObjectDefinitions $name $supportObjectPath 'T' 'edge support ownership'
}

$elfUndefined = @(Get-Undefined $elfPath)
Assert-Check ($elfUndefined.Count -eq 0) "final ELF has unresolved strong symbols: $($elfUndefined -join ',')"
$elfGlobalDefinitions = @(Get-AllDefinitions $elfPath -ExternalOnly)
$elfAllDefinitions = @(Get-AllDefinitions $elfPath `
    -ExpectedAnonymousAbsolute 1)
$elfNamespaceDefinitions = @($elfAllDefinitions | Where-Object {
    Test-MPProcessNamespaceName $_.Name
})
$elfRelevant = @($elfGlobalDefinitions | Where-Object { Test-RelevantName $_.Name })
Assert-ExactSet @($elfRelevant.Name) $expectedFinalNames `
    'canonical final ELF global mpprocess closure'
Assert-ExactSet @($elfNamespaceDefinitions.Name) `
    @($expectedFinalNames + $supportLocalName) `
    'canonical final ELF complete mpprocess namespace'
$finalLocalSupport = @($elfNamespaceDefinitions | Where-Object {
    $_.Name -ceq $supportLocalName
})
Assert-Check (($finalLocalSupport.Count -eq 1) -and
    ($finalLocalSupport[0].Type -ceq 't')) `
    "final ELF local support helper is not one strong local text definition: $supportLocalName"
$weakElfDefinitions = @($elfNamespaceDefinitions | Where-Object {
    $_.Type -cmatch '^[WwVv]$'
})
Assert-Check ($weakElfDefinitions.Count -eq 0) `
    "final ELF retains weak mpprocess definitions: $(@($weakElfDefinitions.Name) -join ',')"
foreach ($definition in $elfRelevant) {
    $expectedType = if ($expectedFinalData -ccontains $definition.Name) {
        'B'
    } else {
        'T'
    }
    Assert-Check ($definition.Type -ceq $expectedType) "final ELF $($definition.Name) has legacy/weak type $($definition.Type), expected $expectedType"
}

$mapSymbolRecords = @([regex]::Matches(
    $mapText,
    '(?m)^\s+0x([0-9a-fA-F]+)\s+(\S+)\s*$'
) | ForEach-Object {
    [pscustomobject]@{
        Address = [Convert]::ToUInt64($_.Groups[1].Value, 16)
        Name = $_.Groups[2].Value
    }
})
$mapRelevant = @($mapSymbolRecords | Where-Object {
    Test-RelevantName $_.Name
})
Assert-ExactSet @($mapRelevant.Name) @($elfRelevant.Name) `
    'allocated link-map/final-ELF mpprocess closure'
foreach ($definition in $elfRelevant) {
    $mapDefinitions = @($mapRelevant | Where-Object {
        $_.Name -ceq $definition.Name
    })
    Assert-Check ($mapDefinitions.Count -eq 1) `
        "link map does not contain exactly one address for $($definition.Name)"
    Assert-Check ($mapDefinitions[0].Address -eq $definition.Address) `
        ("map/ELF address mismatch for {0}: 0x{1:x} != 0x{2:x}" -f
            $definition.Name, $mapDefinitions[0].Address,
            $definition.Address)
}

$finalPublic = @($elfRelevant.Name | Where-Object { $bridgePublic -ccontains $_ })
Assert-ExactSet $finalPublic $expectedFinalPublic `
    'canonical final public mpprocess bridge closure'
$requiredFinalBase = @($finalPublic | ForEach-Object {
    'ndsBaseMPProcess' + $_.Substring('mpProcess'.Length)
})
foreach ($name in $requiredFinalBase) {
    Assert-Check (@($elfRelevant | Where-Object {
        ($_.Name -ceq $name) -and ($_.Type -ceq 'T')
    }).Count -eq 1) "reachable bridge closure is missing source target $name"
}
$finalBaseFunctions = @($elfRelevant.Name | Where-Object { $functionTargets -ccontains $_ })
$finalBaseData = @($elfRelevant.Name | Where-Object { $globalTargets -ccontains $_ })
$finalSupport = @($elfRelevant.Name | Where-Object { $script:SupportApiNames -ccontains $_ })
Assert-ExactSet $finalBaseFunctions $expectedFinalBaseFunctions `
    'canonical final source-function closure'
Assert-ExactSet $finalBaseData $expectedFinalData `
    'canonical final source-data closure'
Assert-ExactSet $finalSupport $expectedFinalSupport `
    'canonical final edge-support closure'
foreach ($name in $runtimeCriticalNames) {
    Assert-Check (@($elfRelevant | Where-Object {
        $_.Name -ceq $name
    }).Count -eq 1) `
        "canonical final closure is missing runtime-critical symbol $name"
}

$elfItem = Get-Item -LiteralPath $elfPath
$mapItem = Get-Item -LiteralPath $mapPath
Assert-Check ($elfItem.Length -gt 0) 'live ELF is empty'
Assert-Check ($mapItem.Length -gt 0) 'live link map is empty'
Write-Output (
    'mpprocess-live-link=PASS config=realtime-hw-profile0-private0-live1 ' +
    'objects=import36+7,support2+1,bridge28 map-ownership=exclusive ' +
    "closure=public$($finalPublic.Count),base$($finalBaseFunctions.Count),data$($finalBaseData.Count),edge$($finalSupport.Count) " +
    'unresolved=0 legacy=0 weak=0 dependencies=resolved-and-fresh provenance=BattleShip-sha256-pinned'
)
exit 0
