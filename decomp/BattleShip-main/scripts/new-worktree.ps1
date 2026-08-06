# new-worktree.ps1 — spin up an isolated git worktree for a parallel Claude
#                    session on Windows. PowerShell port of new-worktree.sh.
#
# Usage:
#   scripts\new-worktree.ps1 <slug> [-Base <branch>] [-Build] [-Release | -DebugCfg]
#
# Example:
#   scripts\new-worktree.ps1 fighter-hitbox-review
#   scripts\new-worktree.ps1 ui-refactor -Base main -Build
#
# What it does (mirrors new-worktree.sh):
#   1. Creates a worktree at .claude\worktrees\<slug> on new branch agent/<slug>
#   2. Copies every baserom.{us,jp}.{z64,n64,v64} that exists in the main
#      tree (Windows symlinks need Developer Mode or elevation — a copy is
#      ~12 MB per ROM and avoids that requirement).
#   3. Clones libultraship, torch, and decomp as independent repos inside
#      the worktree:
#        - Source = main tree's local submodule checkout (picks up pinned SHAs
#          that may only exist locally, never pushed to the fork)
#        - Origin URL reset to the real SSH upstream from .gitmodules, so
#          `git push` inside the worktree's submodule goes to GitHub.
#   4. Regenerates gitignored codegen (reloc stubs, yamls, reloc table, credits).
#   5. Runs `cmake -B build` (configure only; pass -Build to also compile).
#   6. Copies extracted BattleShip.o2r / f3d.o2r from the main tree if present.
#
# Parallel windows in separate worktrees never collide on source, build
# outputs, or submodule checkouts.

[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)][string]$Slug,
    [string]$Base = "main",
    [switch]$Build,
    [switch]$Release,
    [switch]$DebugCfg   # `-Debug` is reserved by CmdletBinding; use -DebugCfg
)

$ErrorActionPreference = 'Stop'

function Step([string]$msg) { Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Fail([string]$msg) { Write-Host "ERROR: $msg" -ForegroundColor Red; exit 1 }

$Config = if ($Release) { 'Release' } elseif ($DebugCfg) { 'Debug' } else { 'Debug' }

# ── Paths ──
$Root = (git rev-parse --show-toplevel).Trim()
if (-not $Root) { Fail "not inside a git repository" }
$WtDir  = Join-Path $Root ".claude\worktrees\$Slug"
$Branch = "agent/$Slug"

if (Test-Path $WtDir) { Fail "$WtDir already exists. Remove it or pick a different slug." }

# Discover every baserom variant present in the main tree. The worktree
# inherits whatever the user has, so a US-only or JP-only setup still
# works — only a tree with neither US nor JP fails.
$Roms = @()
foreach ($region in 'us', 'jp') {
    foreach ($ext in 'z64', 'n64', 'v64') {
        $candidate = Join-Path $Root "baserom.$region.$ext"
        if (Test-Path $candidate) {
            $Roms += "baserom.$region.$ext"
            break  # first extension wins per region
        }
    }
}
if ($Roms.Count -eq 0) {
    Fail "no baserom.{us,jp}.{z64,n64,v64} in $Root"
}

# ── 1. Worktree + branch ──
Step "Creating worktree $WtDir on branch $Branch (base: $Base)"
git -C $Root worktree add $WtDir -b $Branch $Base
if ($LASTEXITCODE -ne 0) { Fail "git worktree add failed" }

# ── 2. ROM copies (gitignored, ~12 MB each) ──
# Windows symlinks (mklink, New-Item -Type SymbolicLink) need either
# Developer Mode enabled or an elevated shell. A flat copy works everywhere
# and is durable across the worktree's lifetime — the ROM is read-only at
# runtime so the duplicate isn't a coherency hazard.
Step "Copying baseroms ($($Roms -join ', '))"
foreach ($rom in $Roms) {
    Copy-Item -Path (Join-Path $Root $rom) -Destination (Join-Path $WtDir $rom) -Force
}

# ── 3. Independent submodule clones ──
# Submodules pin SHAs to JRickey/libultraship and JRickey/Torch on `ssb64`.
# Those SHAs frequently exist only in the main checkout's local .git/modules
# (branch hasn't been pushed), so `git submodule update --init` inside a
# fresh worktree fails with "upload-pack: not our ref".
#
# Instead: clone from the main tree's working submodule (git follows the
# .git gitfile → .git/modules/<name>), then reset origin to the real SSH
# upstream so pushes from the worktree go to GitHub.
foreach ($sm in 'libultraship', 'torch', 'decomp') {
    # Some submodules may not be registered on older base branches
    # (e.g. `decomp` was added on agent/decomp-submodule). Skip silently
    # if the main tree doesn't track it yet.
    $smPath = & git -C $WtDir config -f .gitmodules "submodule.$sm.path" 2>$null
    if (-not $smPath) {
        Write-Host "  Skipping submodule $sm (not configured in main tree .gitmodules)" -ForegroundColor Yellow
        continue
    }

    $pinnedSha = (git -C $WtDir rev-parse "HEAD:$sm").Trim()
    # Prefer the main tree's configured origin (often SSH) over .gitmodules URL
    # (often HTTPS) so the worktree inherits whatever auth method the user has
    # set up for push.
    $originUrl = & git -C (Join-Path $Root $sm) remote get-url origin 2>$null
    if (-not $originUrl) {
        $originUrl = (git -C $Root config -f .gitmodules "submodule.$sm.url").Trim()
    }
    $smBranch = & git -C $Root config -f .gitmodules "submodule.$sm.branch" 2>$null

    Step "Submodule $sm → $pinnedSha (origin: $originUrl)"
    $smTarget = Join-Path $WtDir $sm
    if (Test-Path $smTarget) { Remove-Item -Recurse -Force $smTarget }
    git clone --no-local --quiet (Join-Path $Root $sm) $smTarget
    if ($LASTEXITCODE -ne 0) { Fail "git clone $sm failed" }

    git -C $smTarget remote set-url origin $originUrl
    # The local source clone only carries refs/heads/* of the main checkout, so
    # SHAs reachable only via remote-tracking branches (e.g. an older tag's pin
    # that lives on a feature branch) won't be in the new clone. If the
    # checkout misses, fetch from the real fork and retry.
    git -C $smTarget checkout --quiet $pinnedSha 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  Pinned SHA not in local clone; fetching from $originUrl"
        git -C $smTarget fetch --quiet origin
        git -C $smTarget checkout --quiet $pinnedSha
        if ($LASTEXITCODE -ne 0) { Fail "submodule $sm checkout $pinnedSha failed even after fetch" }
    }
    if ($smBranch) {
        # Re-create the tracking branch so `git push` from detached HEAD is obvious.
        git -C $smTarget checkout -B $smBranch --quiet
    }
}

# ── 4. Regenerate gitignored codegen ──
# reloc_data.h, yamls/us/reloc_*.yml, credits .encoded/.metadata are all
# gitignored and must be rebuilt on every fresh checkout before CMake runs.
$Python = if (Get-Command python3 -ErrorAction SilentlyContinue) { 'python3' }
          elseif (Get-Command python  -ErrorAction SilentlyContinue) { 'python' }
          else { Fail "python3 (or python) not found in PATH" }

Step "Regenerating reloc codegen"
& $Python (Join-Path $WtDir "tools\generate_reloc_stubs.py")
Push-Location $WtDir
try {
    & $Python "tools\generate_yamls.py"
    & $Python "tools\generate_reloc_table.py"
} finally { Pop-Location }

Step "Encoding credits text"
Push-Location (Join-Path $WtDir "decomp\src\credits")
try {
    foreach ($f in 'staff.credits.us.txt', 'titles.credits.us.txt') {
        & $Python (Join-Path $WtDir "tools\creditsTextConverter.py") $f | Out-Null
    }
    foreach ($f in 'info.credits.us.txt', 'companies.credits.us.txt') {
        & $Python (Join-Path $WtDir "tools\creditsTextConverter.py") -paragraphFont $f | Out-Null
    }
} finally { Pop-Location }

# ── 5. CMake configure ──
$Gen = if (Get-Command ninja -ErrorAction SilentlyContinue) { 'Ninja' } else { 'Visual Studio 17 2022' }

Step "Configuring CMake ($Gen, $Config)"
cmake -S $WtDir -B (Join-Path $WtDir 'build') -G $Gen -DCMAKE_BUILD_TYPE=$Config
if ($LASTEXITCODE -ne 0) { Fail "cmake configure failed" }

# ── 5b. Copy extracted assets ──
# Torch extraction (BattleShip.o2r) is slow and produces bytewise-identical
# output for a given baserom. The binary loads BattleShip.o2r (ROM-derived)
# and f3d.o2r (shaders) from its CWD on launch — without them the game
# prints "archive ... does not exist" and exits. Reuse the main tree's
# extracted assets via copy so parallel worktrees don't each re-extract.
Step "Copying extracted assets (BattleShip.o2r / f3d.o2r)"
$linkedAny = $false
foreach ($asset in 'BattleShip.o2r', 'BattleShip-JP.o2r', 'f3d.o2r') {
    $src = $null
    foreach ($cand in (Join-Path $Root "build\$asset"), (Join-Path $Root $asset)) {
        if (Test-Path $cand) { $src = $cand; break }
    }
    if ($src) {
        Copy-Item -Path $src -Destination (Join-Path $WtDir "build\$asset") -Force
        $linkedAny = $true
    } else {
        Write-Host ("  warn: {0} not found in main tree ({1}\build or {1}) — extract it there first" -f $asset, $Root) -ForegroundColor Yellow
    }
}
if (-not $linkedAny) {
    Write-Host "  No assets copied. Build + extract in the main tree first (cmake --build $Root\build --target ExtractAssets) before launching the worktree binary." -ForegroundColor Yellow
}

# ── 6. Optional: compile ──
if ($Build) {
    $Jobs = $env:NUMBER_OF_PROCESSORS
    if (-not $Jobs) { $Jobs = 4 }
    Step "Building ssb64 ($Jobs jobs)"
    cmake --build (Join-Path $WtDir 'build') --target ssb64 --config $Config -j $Jobs
    if ($LASTEXITCODE -ne 0) { Fail "cmake --build failed" }
}

# ── Done ──
Step "Worktree ready"
@"
  Path:     $WtDir
  Branch:   $Branch  (base: $Base)
  Build:    $WtDir\build       ($Gen, $Config)
  ROMs:     $($Roms -join ', ')  (copied, not symlinked — Windows)
  Submods:  libultraship, torch, decomp — independent clones,
            origin set to fork

  Point a new Claude window at: $WtDir
  Build:    cmake --build $WtDir\build --target ssb64 -j
  Remove:   git worktree remove $WtDir; git branch -D $Branch
"@
