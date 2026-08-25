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
    # P2-1M (2026-08-19): `artifacts/` WAS blanket-forbidden here, and that
    # contradicted the repository's own standing rule -- AGENTS.md: "`artifacts/
    # performance` and `artifacts/visibility` are permanent evidence" -- so this
    # check has been red against 1,830 deliberately tracked evidence files
    # (performance 1,208, visibility 462, bugs 20, verification 85, task37 55).
    # `/artifacts/` is gitignored wholesale; anything tracked under it was
    # force-added as cited evidence and is meant to survive. What must still
    # never be tracked is genuinely ephemeral run output -- the verifier's own
    # temp/log trees -- and any binary build product, which the extension rule
    # below already covers wherever it lands.
    # `decomp/` is read-only upstream reference (AGENTS.md hard rule) and
    # `decomp/sm64/` is tracked in-tree, all 26,260 files of it. Two of them --
    # an IDO `crt1.o` and an asm-processor `.d` fixture -- are upstream's own
    # files that happen to match the build-product extensions below. This rule
    # polices OUR generated output; `check-decomp-pristine.ps1` owns that tree.
    $forbiddenTracked = @($tracked | Where-Object {
        ($_ -notmatch '^decomp/') -and (
        ($_ -match '^build/') -or
        ($_ -match '^artifacts/(verifier-temp|verifier-logs|logs)/') -or
        ($_ -match '\.(nds|elf|map|sym|sav|o|d)$') -or
        (($_ -match '^emulators/') -and ($_ -notin @('emulators/README.md', 'emulators/melonds/.gitkeep', 'emulators/nogba/.gitkeep'))))
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
    #
    # This list is PER FIGHTER and every new production fighter adds one, so it
    # must be extended when the fighter lands, not later: Luigi and Donkey Kong
    # shipped their wrappers on 2026-08-22 without it and this checker has been
    # red ever since (found 2026-08-25, board row P2-3r10). Each wrapper is the
    # same narrow shape -- forward to the decomp descriptor table under
    # `NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_P2_<KIND>`, and supply
    # NDS_FT_STATUS_STUB16 otherwise -- so a new entry here is only correct if
    # the file has that shape.
    'include/ft/ftcommon/ftcommonstatus.h',
    'include/ft/ftchar/ftmario/ftmariostatus.h',
    'include/ft/ftchar/ftfox/ftfoxstatus.h',
    'include/ft/ftchar/ftluigi/ftluigistatus.h',
    'include/ft/ftchar/ftdonkey/ftdonkeystatus.h'
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
        'src/import/battleship_ftstatus_map_physics_shims.c',
        # DS-side STORAGE for two original symbols, not wrappers around original
        # code: both replace an N64 software buffer the DS rasterizes in
        # hardware, so there is no decomp translation unit for them to include.
        # Their derivations live on the externs they pair with.
        'src/import/battleship_sys_framebuffer.c',
        'src/import/battleship_sys_zbuffer.c'
    )
    $text = Get-Content -LiteralPath $_.FullName -Raw
    # P2-1M (2026-08-19): `decomp/BattleShip-main` is no longer the only correct
    # spelling. Import wrappers now include the overlay copy the build applies
    # its patches to -- `<battleship_overlay/src/...>`, materialised into
    # $(BUILD)/battleship_overlay/ (CLAUDE.md) -- and eight wrappers had moved to
    # it, so this rule reds on the modern form while passing the legacy one.
    if (($text -notmatch 'decomp/BattleShip-main') -and
        ($text -notmatch 'battleship_overlay/') -and
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
