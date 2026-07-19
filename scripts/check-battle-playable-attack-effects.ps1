$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$battleShip = Join-Path $root 'decomp/BattleShip-main/decomp/src'

$calls = foreach ($relative in @(
        'relocData/202_MarioMainMotion.c',
        'relocData/208_FoxMainMotion.c'
    )) {
    $source = Get-Content -LiteralPath (Join-Path $battleShip $relative) -Raw
    [regex]::Matches($source,
        'ftMotionCommandEffect\(\s*[^,]+,\s*(nEFKind[A-Za-z0-9_]+)') |
        ForEach-Object { $_.Groups[1].Value }
}
$actual = @($calls | Group-Object | Sort-Object Name |
    ForEach-Object { '{0}:{1}' -f $_.Name, $_.Count })
$expected = @(
    'nEFKindDamageFlyMDustReverse:1',
    'nEFKindDustDashSmall:25',
    'nEFKindDustHeavy:3',
    'nEFKindDustHeavyDouble:33',
    'nEFKindDustHeavyReverse:13',
    'nEFKindDustLight:32',
    'nEFKindFlashMiddle:4',
    'nEFKindFuraSparkle:2',
    'nEFKindImpactWave:5',
    'nEFKindMusicNote:2',
    'nEFKindQuakeMag0:5',
    'nEFKindQuakeMag1:8',
    'nEFKindQuakeMag2:3',
    'nEFKindShockSmall:2',
    'nEFKindSparkleWhite:2',
    'nEFKindSparkleWhiteMultiExplode:4',
    'nEFKindSparkleWhiteScale:34'
)
if (($actual -join ',') -ne ($expected -join ',')) {
    throw "Mario/Fox source motion-effect census changed: $($actual -join ',')"
}

$shim = Get-Content -LiteralPath (
    Join-Path $root 'src/port/reloc_backend_compat_shims.c') -Raw
$route = [regex]::Match($shim,
    '(?s)void \*ftParamMakeEffect\(.*?(?=static f32 ndsVisualDamageScale)')
if (-not $route.Success) {
    throw 'The live ftParamMakeEffect route was not found.'
}
foreach ($kind in ($calls | Sort-Object -Unique)) {
    $case = [regex]::Match($route.Value,
        ('(?s)case\s+{0}:.*?(?:break;|return\s+[^;]+;)' -f
            [regex]::Escape($kind)))
    if (-not $case.Success -or
        $case.Value -notmatch
            'ndsEFManagerMakeVisualEffect\(|efManagerQuakeMakeEffect\(') {
        throw "Source motion effect lacks a bounded DS visual route: $kind"
    }
}

$direct = @(
    @{ File = 'src/import/battleship_efmanager.c';
       Function = 'efManagerFoxReflectorMakeEffect';
       Kind = 'nNDSVisualEffectReflector' },
    @{ File = 'src/import/battleship_fox_blaster.c';
       Function = 'efManagerFoxBlasterGlowMakeEffect';
       Kind = 'nNDSVisualEffectHitElectric' },
    @{ File = 'src/port/reloc_backend_compat_shims.c';
       Function = 'efManagerDustExpandSmallMakeEffect';
       Kind = 'nNDSVisualEffectDust' },
    @{ File = 'src/port/reloc_backend_compat_shims.c';
       Function = 'efManagerFireGrindMakeEffect';
       Kind = 'nNDSVisualEffectHitFire' },
    @{ File = 'src/port/reloc_backend_compat_shims.c';
       Function = 'efManagerSparkleWhiteMakeEffect';
       Kind = 'nNDSVisualEffectSparkle' }
)
foreach ($fixture in $direct) {
    $text = Get-Content -LiteralPath (Join-Path $root $fixture.File) -Raw
    $pattern = '(?s)\b{0}\s*\([^)]*\)\s*\{{.*?{1}.*?\n\}}' -f
        [regex]::Escape($fixture.Function), [regex]::Escape($fixture.Kind)
    if ($text -notmatch $pattern) {
        throw "Direct source effect route changed: $($fixture.Function)"
    }
}

$fireball = Get-Content -LiteralPath (
    Join-Path $battleShip 'wp/wpmario/wpmariofireball.c') -Raw
foreach ($fixture in @(
        @{ Function = 'efManagerDustExpandSmallMakeEffect'; Count = 2 },
        @{ Function = 'efManagerFireGrindMakeEffect'; Count = 1 },
        @{ Function = 'efManagerSparkleWhiteMakeEffect'; Count = 1 }
    )) {
    if ([regex]::Matches($fireball,
            ('\b{0}\s*\(' -f [regex]::Escape($fixture.Function))).Count -ne
        $fixture.Count) {
        throw "Mario fireball source effect count changed: $($fixture.Function)"
    }
}
$foxReflector = Get-Content -LiteralPath (
    Join-Path $battleShip 'ft/ftchar/ftfox/ftfoxspeciallw.c') -Raw
$foxBlaster = Get-Content -LiteralPath (
    Join-Path $battleShip 'wp/wpfox/wpfoxblaster.c') -Raw
if ([regex]::Matches($foxReflector,
        '\befManagerFoxReflectorMakeEffect\s*\(').Count -ne 1 -or
    [regex]::Matches($foxBlaster,
        '\befManagerFoxBlasterGlowMakeEffect\s*\(').Count -ne 4) {
    throw 'Fox special source effect callsites changed.'
}

Write-Output ('Attack visual effects passed: 178/178 Mario/Fox motion calls ' +
    'across 17 source kinds plus reflector, blaster glow, and three fireball ' +
    'routes have bounded DS presentation.')
