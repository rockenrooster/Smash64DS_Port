#!/usr/bin/env python3
"""Fail when committed dependency references resolve only to untracked files."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".inc", ".py", ".s", ".S"}


def git(*args: str, allow_no_match: bool = False) -> str:
    proc = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if proc.returncode != 0 and not (allow_no_match and proc.returncode == 1):
        raise RuntimeError(
            f"git {' '.join(args)} failed ({proc.returncode}): {proc.stderr.strip()}"
        )
    return proc.stdout


def normalize(path: str) -> str:
    return path.replace("\\", "/").lstrip("./")


def git_grep(pattern: str, paths: tuple[str, ...]) -> list[str]:
    out = git(
        "grep",
        "--cached",
        "-n",
        "-E",
        pattern,
        "--",
        *paths,
        allow_no_match=True,
    )
    return [line for line in out.splitlines() if line]


def logical_make_lines(text: str) -> list[str]:
    return re.sub(r"\\\r?\n[ \t]*", " ", text).splitlines()


def rom_include_dirs(raw_makefile: str) -> tuple[str, ...]:
    """Return repository-local include roots declared by the indexed Makefile."""
    roots: set[str] = set()
    for line in logical_make_lines(raw_makefile):
        match = re.match(r"^\s*INCLUDES\s*(?::|\?|\+)?=\s*(.*)$", line)
        if match:
            for token in match.group(1).split():
                if "$" not in token:
                    roots.add(normalize(token))
        for match in re.finditer(r"-I\$\(CURDIR\)/([^\s\\]+)", line):
            token = match.group(1)
            if "$" not in token:
                roots.add(normalize(token))
    return tuple(sorted(root for root in roots if root))


def build_generated_includes(raw_makefile: str) -> set[str]:
    """Return include tokens generated under $(BUILD), a real ROM -I root."""
    return {
        normalize(match)
        for match in re.findall(
            r"\$\(BUILD\)/([A-Za-z0-9_./-]+\.(?:h|hpp|inc))\b",
            raw_makefile,
        )
    }


def resolve_include(
    source_rel: str,
    token: str,
    untracked: set[str],
    include_dirs: tuple[str, ...],
    generated_includes: set[str],
) -> list[str]:
    token_norm = normalize(token)
    if token_norm in generated_includes:
        return []

    source_dir = Path(source_rel).parent
    candidates = [
        normalize(str(source_dir / token)),
        *(normalize(str(Path(root) / token)) for root in include_dirs),
    ]
    return sorted({candidate for candidate in candidates if candidate in untracked})


def main() -> int:
    untracked = {
        normalize(path)
        for path in git(
            "ls-files", "--others", "--exclude-standard", "--", "src", "include", "scripts"
        ).splitlines()
        if path and Path(path).suffix in SOURCE_SUFFIXES
    }

    failures: set[tuple[str, str]] = set()

    # Makefile references are read from the index. This catches a committed link or
    # generator prerequisite that names a file which exists only in the worktree.
    raw_makefile = git("show", ":Makefile")
    include_dirs = rom_include_dirs(raw_makefile)
    generated_includes = build_generated_includes(raw_makefile)
    makefile = re.sub(r"\\\r?\n[ \t]*", " ", raw_makefile)
    makefile = "\n".join(
        line.split("#", 1)[0] for line in makefile.splitlines() if line.strip()
    )
    bare_link_inputs = set(
        re.findall(
            r"(?mi)^[ \t]*([A-Za-z0-9_.-]+\.(?:c|cc|cpp|s|S))[ \t]*\\?[ \t]*$",
            raw_makefile,
        )
    )
    for rel in sorted(untracked):
        path_token = re.escape(rel)
        if re.search(rf"(?<![A-Za-z0-9_.-]){path_token}(?![A-Za-z0-9_.-])", makefile):
            failures.add((rel, "Makefile(index)"))
        elif Path(rel).name in bare_link_inputs:
            failures.add((rel, "Makefile(index link-input row)"))

    # Only quoted repository includes are relevant; system/toolchain headers are
    # deliberately outside the repository and outside this guard.
    include_lines = git_grep(
        r'^[[:space:]]*#[[:space:]]*include[[:space:]]*"[^"]+"',
        ("src", "include"),
    )
    include_re = re.compile(r'#\s*include\s*"([^"]+)"')
    for row in include_lines:
        parts = row.split(":", 2)
        if len(parts) != 3:
            continue
        source_rel, line_no, text = parts
        match = include_re.search(text)
        if not match:
            continue
        for dep in resolve_include(
            source_rel,
            match.group(1),
            untracked,
            include_dirs,
            generated_includes,
        ):
            failures.add((dep, f"{source_rel}:{line_no} (index #include)"))

    # Generator helpers are Python dependencies rather than C #includes. Check the
    # same committed-reference/untracked-file relation so an untracked helper cannot
    # make a clean checkout die with ModuleNotFoundError.
    untracked_modules: dict[str, list[str]] = {}
    for rel in untracked:
        if rel.startswith("scripts/") and rel.endswith(".py"):
            untracked_modules.setdefault(Path(rel).stem, []).append(rel)
    import_lines = git_grep(
        r'^[[:space:]]*(from|import)[[:space:]]+[A-Za-z0-9_.]+',
        ("scripts",),
    )
    import_re = re.compile(r"^\s*(?:from|import)\s+([A-Za-z0-9_.]+)")
    for row in import_lines:
        parts = row.split(":", 2)
        if len(parts) != 3:
            continue
        source_rel, line_no, text = parts
        match = import_re.search(text)
        if not match:
            continue
        module_leaf = match.group(1).split(".")[-1]
        for dep in untracked_modules.get(module_leaf, []):
            failures.add((dep, f"{source_rel}:{line_no} (index import)"))

    if failures:
        for dep, owner in sorted(failures):
            print(f"UNTRACKED-DEPENDENCY: {dep} referenced by {owner}")
        print(
            f"Untracked dependency check failed: {len(failures)} committed reference(s) "
            "resolve to files outside the git index."
        )
        return 1

    print(
        "Untracked dependency check passed: "
        f"untracked_source_files={len(untracked)} committed_refs_clean=YES"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # fail closed: an unreadable index is not a pass
        print(f"Untracked dependency check failed: {exc}", file=sys.stderr)
        raise SystemExit(2)
