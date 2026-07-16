param(
    [int]$LargeSourceWarnLines = 10000,
    [int]$LargeHeaderWarnLines = 3000
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\harness-registry.ps1')
$failures = [System.Collections.Generic.List[string]]::new()
$warnings = [System.Collections.Generic.List[string]]::new()
function Add-Failure {
    param([string]$Message)
    $failures.Add($Message) | Out-Null
}
function Add-Warning {
    param([string]$Message)
    $warnings.Add($Message) | Out-Null
}
function Get-RelativePath {
    param([string]$Path)
    return ([System.IO.Path]::GetRelativePath($root, $Path) -replace '\\', '/')
}
function Invoke-GitLines {
    param([string[]]$Arguments)
    $output = & git @Arguments 2>$null
    if ($LASTEXITCODE -ne 0) { return @() }
    return @($output | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}
if (Test-Path -LiteralPath (Join-Path $root '.git')) {
    $decompStatus = Invoke-GitLines @('status', '--porcelain=v1', '--', 'decomp')
    if ($decompStatus.Count -gt 0) {
        Add-Failure ("decomp/ has working-tree changes, but it is read-only: {0}" -f ($decompStatus -join '; '))
    }
    $tracked = Invoke-GitLines @('ls-files')
    $forbiddenTracked = @($tracked | Where-Object {
        ($_ -match '^(build|artifacts)/') -or
        ($_ -match '\.(nds|elf|map|sym|sav|o|d)$') -or
        (($_ -match '^emulators/') -and ($_ -notin @('emulators/README.md', 'emulators/melonds/.gitkeep', 'emulators/nogba/.gitkeep')))
    })
    if ($forbiddenTracked.Count -gt 0) {
        Add-Failure ("generated/local output is tracked: {0}" -f (($forbiddenTracked | Select-Object -First 20) -join ', '))
    }
}
$srcRoot = Join-Path $root 'src'
$allowedSrcDirs = @('import', 'nds', 'port')
Get-ChildItem -LiteralPath $srcRoot -Directory | ForEach-Object {
    if ($allowedSrcDirs -notcontains $_.Name) {
        Add-Failure "unexpected source directory '$($_.Name)'; use src/import, src/nds, or src/port"
    }
}
$sourceFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -Include *.c,*.h -File) +
    @(Get-ChildItem -LiteralPath (Join-Path $root 'include') -Recurse -Include *.h -File)
$allowedDecompHeaderIncludes = @(
    # Runtime slice 2 imports the original descriptor tables through these
    # narrow wrappers; see docs/FT_ANIM_STATUS_SCOUT.md.
    'include/ft/ftcommon/ftcommonstatus.h',
    'include/ft/ftchar/ftmario/ftmariostatus.h',
    'include/ft/ftchar/ftfox/ftfoxstatus.h'
)
foreach ($file in $sourceFiles) {
    $relative = Get-RelativePath $file.FullName
    $text = Get-Content -LiteralPath $file.FullName -Raw
    $hasDecompInclude = ($text -match 'decomp/BattleShip-main/decomp/src') -or ($text -match '\.\./\.\./decomp/')
    if ($hasDecompInclude -and ($relative -notmatch '^src/import/') -and
        ($allowedDecompHeaderIncludes -notcontains $relative)) {
        Add-Failure "decomp source include outside src/import: $relative"
    }
}
Get-ChildItem -LiteralPath (Join-Path $root 'src/import') -Filter '*.c' -File | ForEach-Object {
    $relative = Get-RelativePath $_.FullName
    $allowedImportHelpers = @(
        # Weak callback aliases that let original descriptor-table headers link
        # against macro-renamed imported status callbacks, plus documented
        # inactive/map/physics seams for not-yet-imported status dependencies.
        'src/import/battleship_ftstatus_callback_aliases.c',
        'src/import/battleship_ftstatus_inactive_stubs.c',
        'src/import/battleship_ftstatus_map_physics_shims.c'
    )
    $text = Get-Content -LiteralPath $_.FullName -Raw
    if (($text -notmatch 'decomp/BattleShip-main') -and
        ($allowedImportHelpers -notcontains $relative)) {
        Add-Failure "import wrapper lacks original BattleShip source path: $relative"
    }
}
$registry = @(Get-Smash64DSHarnessRegistry)
$architecture = Get-Content -LiteralPath (Join-Path $root 'docs/ARCHITECTURE.md') -Raw
$hasLargeFilePlan = $architecture.Contains('## Large Backend File Split Plan')
$largeFiles = @()
foreach ($file in @(
    'src/port/reloc_backend.c',
    'src/port/taskman_seam.c',
    'include/nds/nds_startup.h',
    'docs/DIAGNOSTIC_REFERENCE.md',
    'docs/PORTING.md'
)) {
    $path = Join-Path $root $file
    if (-not (Test-Path -LiteralPath $path)) { continue }
    $lineCount = @(Get-Content -LiteralPath $path).Count
    $budget = if ($file -match '\.h$') { $LargeHeaderWarnLines } else { $LargeSourceWarnLines }
    if (($file -match '^docs/') -and ($file -ne 'docs/DIAGNOSTIC_REFERENCE.md')) { $budget = 8000 }
    if ($lineCount -gt $budget) {
        $largeFiles += "$file=$lineCount"
    }
}
if (($largeFiles.Count -gt 0) -and (-not $hasLargeFilePlan)) {
    Add-Failure ("large project files require docs/ARCHITECTURE.md split plan: {0}" -f ($largeFiles -join ', '))
} elseif ($largeFiles.Count -gt 0) {
    Add-Warning ("large file split plan present for: {0}" -f ($largeFiles -join ', '))
}
$generatedStatus = @()
foreach ($path in @('build')) {
    if (Test-Path -LiteralPath (Join-Path $root $path)) {
        $generatedStatus += $path
    }
}
Get-ChildItem -LiteralPath $root -File -Filter 'smash64ds*.nds' -ErrorAction SilentlyContinue | ForEach-Object {
    $generatedStatus += $_.Name
}
if ($generatedStatus.Count -gt 0) {
    Add-Warning ("generated outputs exist locally; clean before release snapshots if needed: {0}" -f (($generatedStatus | Select-Object -First 10) -join ', '))
}
if ($warnings.Count -gt 0) {
    foreach ($warning in $warnings) {
        Write-Warning $warning
    }
}
if ($failures.Count -gt 0) {
    foreach ($failure in $failures) {
        Write-Error $failure
    }
    throw "Architecture check failed with $($failures.Count) issue(s)."
}
Write-Output "Architecture check passed: imports=$(@(Get-ChildItem -LiteralPath (Join-Path $root 'src/import') -Filter '*.c' -File).Count), registryEntries=$($registry.Count), warnings=$($warnings.Count)."
