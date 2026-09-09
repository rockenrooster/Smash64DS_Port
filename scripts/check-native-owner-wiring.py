#!/usr/bin/env python3
"""Static wiring guard for every native owner executor present in src/nds."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def logical_make_lines(text: str) -> list[str]:
    return re.sub(r"\\\r?\n[ \t]*", " ", text).splitlines()


def include_sites(text: str, include_name: str) -> list[tuple[int, tuple[str, ...]]]:
    sites: list[tuple[int, tuple[str, ...]]] = []
    stack: list[str] = []
    include_re = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
    for line_no, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if re.match(r"^#\s*(if|ifdef|ifndef)\b", stripped):
            stack.append(stripped)
        elif re.match(r"^#\s*(elif|else)\b", stripped):
            if stack:
                stack[-1] = stripped
        elif re.match(r"^#\s*endif\b", stripped):
            if stack:
                stack.pop()
            continue
        match = include_re.match(line)
        if match and Path(match.group(1)).name == include_name:
            sites.append((line_no, tuple(stack)))
    return sites


def owner_entrypoint(text: str) -> str | None:
    match = re.search(
        r"\b(?:sb32|s32)\s+(ndsRendererSubmitNative[A-Za-z0-9_]+)\s*\(", text
    )
    return match.group(1) if match else None


def guard_stack_at_symbol(text: str, symbol: str) -> tuple[str, ...]:
    stack: list[str] = []
    symbol_re = re.compile(rf"\b{re.escape(symbol)}\s*\(")
    for line in text.splitlines():
        stripped = line.strip()
        if re.match(r"^#\s*(if|ifdef|ifndef)\b", stripped):
            stack.append(stripped)
        elif re.match(r"^#\s*(elif|else)\b", stripped):
            if stack:
                stack[-1] = stripped
        elif re.match(r"^#\s*endif\b", stripped):
            if stack:
                stack.pop()
            continue
        if symbol_re.search(line):
            return tuple(stack)
    return ()


def owner_feature_guards(exec_text: str, entrypoint: str) -> set[str]:
    guards = guard_stack_at_symbol(exec_text, entrypoint)
    tokens = {
        token
        for guard in guards
        for token in re.findall(r"\bNDS_[A-Z0-9_]+\b", guard)
    }
    return {
        token
        for token in tokens
        if token != "NDS_RENDERER_HW_TRIANGLES" and not token.endswith("_EXEC_INC")
    }


def guard_tokens(guards: tuple[str, ...]) -> set[str]:
    return {
        token
        for guard in guards
        for token in re.findall(r"\bNDS_[A-Z0-9_]+\b", guard)
    }


def tracked_files() -> set[str]:
    proc = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"git ls-files failed ({proc.returncode}): "
            + proc.stderr.decode("utf-8", errors="replace").strip()
        )
    return {
        entry.decode("utf-8", errors="replace").replace("\\", "/")
        for entry in proc.stdout.split(b"\0")
        if entry
    }


def tracked_packet(packet_name: str, tracked: set[str]) -> str | None:
    for base in ("src/nds", "src/nds/generated"):
        candidate = f"{base}/{packet_name}"
        if candidate in tracked:
            return candidate
    return None


def target_rule_for(make_lines: list[str], token: str) -> str | None:
    for line in make_lines:
        if ":" not in line:
            continue
        target, prereqs = line.split(":", 1)
        if token in target and prereqs.strip():
            return line
    return None


def make_guard_implications(make_lines: list[str]) -> dict[str, set[str]]:
    """Map an enabling NDS_* flag to derived boolean flags it forces on."""
    implications: dict[str, set[str]] = {}
    derived_re = re.compile(
        r"^\s*(NDS_[A-Z0-9_]+)\s*(?::|\?|\+)?=\s*"
        r"\$\(if\s+\$\(filter\s+1,(.*)\),1,0\)\s*$"
    )
    for line in make_lines:
        match = derived_re.match(line)
        if not match:
            continue
        derived = match.group(1)
        for source in re.findall(r"\$\((NDS_[A-Z0-9_]+)\)", match.group(2)):
            implications.setdefault(source, set()).add(derived)
    return implications


def expand_guard_tokens(
    tokens: set[str], implications: dict[str, set[str]]
) -> set[str]:
    expanded = set(tokens)
    changed = True
    while changed:
        changed = False
        for source in tuple(expanded):
            for derived in implications.get(source, ()):
                if derived not in expanded:
                    expanded.add(derived)
                    changed = True
    return expanded


def owner_tokens(stem: str) -> set[str]:
    generic = {"item", "actor", "ef", "pikachu", "samus", "sector", "yamabuki", "inishie"}
    return {token for token in stem.split("_") if token and token not in generic}


def handled_variable(stem: str, adapter: str) -> str | None:
    aliases = {
        "pikachu_thunderground": "thunder_ground_native_handled",
        "pikachu_thunderjolt": "thunder_jolt_native_handled",
        "pikachu_thunderjolt_effect": "thunder_fx_native_handled",
        "samus_chargeshot": "charge_shot_native_handled",
        "sector_arwing_laser": "sector_laser_native_handled",
        "yamabuki_marumine": "marumine_native_handled",
    }
    if stem in aliases and re.search(rf"\b{re.escape(aliases[stem])}\b", adapter):
        return aliases[stem]
    variables = sorted(set(re.findall(r"\b([a-z][a-z0-9_]*_native_handled)\b", adapter)))
    wanted = owner_tokens(stem)
    if not wanted:
        return None
    synonym = {"effect": "fx", "fx": "effect", "cloud": "clouds", "clouds": "cloud"}
    owner_join = "_".join(sorted(wanted))
    scored: list[tuple[int, int, str]] = []
    for variable in variables:
        base = variable[: -len("_native_handled")]
        have = set(base.split("_"))
        score = 0
        for token in wanted:
            if token in have:
                score += 2
            elif synonym.get(token) in have:
                score += 1
        have_join = "_".join(sorted(have))
        exact_bonus = 4 if have_join == owner_join else 0
        scored.append((score + exact_bonus, -len(have ^ wanted), variable))
    scored.sort(reverse=True)
    if not scored or scored[0][0] <= 0:
        return None
    if len(scored) > 1 and scored[0][:2] == scored[1][:2]:
        return None
    return scored[0][2]


def add_failure(failures: list[str], stem: str, step: str, detail: str) -> None:
    failures.append(f"OWNER {stem}: missing {step} -- {detail}")


def no_program_arms(adapter: str) -> tuple[str, str, str]:
    start = adapter.index("#if NDS_R2_IMPACT_WAVE_NATIVE")
    submit = adapter.index(
        "impact_wave_native_handled = ndsRendererSubmitNativeImpactWave", start
    )
    on_failure = adapter.index("NDS_NATIVE_FAILURE_NO_PROGRAM", submit)
    off_start = adapter.index("#else", on_failure)
    off_failure = adapter.index("NDS_NATIVE_FAILURE_NO_PROGRAM", off_start)
    return (
        adapter[start:submit],
        adapter[submit:on_failure],
        adapter[off_start:off_failure],
    )


def main() -> int:
    makefile = (ROOT / "Makefile").read_text(encoding="utf-8", errors="replace")
    make_lines = logical_make_lines(makefile)
    guard_implications = make_guard_implications(make_lines)
    renderer_path = ROOT / "src/nds/nds_renderer.c"
    native_owners_path = ROOT / "src/nds/nds_renderer_native_owners.c"
    adapter_path = ROOT / "src/port/renderer_adapter_stage.c"
    matrix_adapter_path = ROOT / "src/port/renderer_adapter_matrix.c"
    movement_path = ROOT / "src/port/reloc_backend_movement.c"
    renderer = renderer_path.read_text(encoding="utf-8", errors="replace")
    native_owners = native_owners_path.read_text(encoding="utf-8", errors="replace")
    adapter = adapter_path.read_text(encoding="utf-8", errors="replace")
    matrix_adapter = matrix_adapter_path.read_text(encoding="utf-8", errors="replace")
    movement = movement_path.read_text(encoding="utf-8", errors="replace")
    program_arms = no_program_arms(adapter)
    tracked = tracked_files()

    exec_paths = [
        path
        for path in sorted((ROOT / "src/nds").glob("nds_native_*.exec.inc"))
        if owner_entrypoint(path.read_text(encoding="utf-8", errors="replace")) is not None
    ]
    failures: list[str] = []
    failed_owners: set[str] = set()

    for exec_path in exec_paths:
        stem = exec_path.name[len("nds_native_") : -len(".exec.inc")]
        stem_re = re.escape(stem)
        expected_prefix = "NDS_NATIVE_" + stem.upper()

        packet_re = re.compile(
            rf"^\s*([A-Z0-9_]+)_PACKET\s*(?::|\?|\+)?=\s*.*nds_native_{stem_re}\.generated\.inc\b"
        )
        packet_match = None
        packet_line = None
        for line in make_lines:
            match = packet_re.search(line)
            if match:
                packet_match = match
                packet_line = line
                break
        prefix = packet_match.group(1) if packet_match else expected_prefix
        packet_name_match = re.search(
            r"([A-Za-z0-9_]+\.generated\.inc)\b", packet_line or ""
        )
        packet_name = (
            packet_name_match.group(1)
            if packet_name_match
            else f"nds_native_{stem}.generated.inc"
        )
        tracked_packet_rel = tracked_packet(packet_name, tracked)
        exec_text = exec_path.read_text(encoding="utf-8", errors="replace")
        entrypoint = owner_entrypoint(exec_text)
        assert entrypoint is not None

        native_owner_exec_sites = include_sites(native_owners, exec_path.name)
        if native_owner_exec_sites:
            packet_sites = include_sites(native_owners, packet_name)
            packet_token = f"$({prefix}_PACKET)"
            if not packet_match:
                add_failure(
                    failures,
                    stem,
                    "Makefile PACKET variable",
                    f"Makefile lacks a *_PACKET assignment for {packet_name} (breaks at generation)",
                )
                failed_owners.add(stem)
            elif target_rule_for(make_lines, packet_token) is None:
                add_failure(
                    failures,
                    stem,
                    "Makefile packet emit rule",
                    f"Makefile has no prerequisite-backed target rule for {packet_token} (breaks at generation)",
                )
                failed_owners.add(stem)
            renderer_rules = [
                line
                for line in make_lines
                if re.search(r"(?:^|[/\\])nds_renderer\.o\s*:", line)
            ]
            if packet_match and not any(packet_token in line for line in renderer_rules):
                add_failure(
                    failures,
                    stem,
                    "nds_renderer.o packet prerequisite",
                    f"Makefile nds_renderer.o rule lacks {packet_token} (breaks at generation)",
                )
                failed_owners.add(stem)
            if len(packet_sites) != 1:
                add_failure(
                    failures,
                    stem,
                    "native-owner packet include",
                    f"src/nds/nds_renderer_native_owners.c includes {packet_name} {len(packet_sites)} time(s); want exactly 1 (breaks at compile)",
                )
                failed_owners.add(stem)
            if len(native_owner_exec_sites) != 1:
                add_failure(
                    failures,
                    stem,
                    "native-owner exec include",
                    f"src/nds/nds_renderer_native_owners.c includes {exec_path.name} {len(native_owner_exec_sites)} time(s); want exactly 1 (breaks at compile/link)",
                )
                failed_owners.add(stem)
            elif len(packet_sites) == 1 and packet_sites[0][1] != native_owner_exec_sites[0][1]:
                add_failure(
                    failures,
                    stem,
                    "matching native-owner build guard",
                    "src/nds/nds_renderer_native_owners.c packet/exec includes use different preprocessor guard stacks",
                )
                failed_owners.add(stem)
            required_guards = owner_feature_guards(exec_text, entrypoint)
            for label, sites in (("packet", packet_sites), ("exec", native_owner_exec_sites)):
                if len(sites) == 1:
                    present_guards = expand_guard_tokens(
                        guard_tokens(sites[0][1]), guard_implications
                    )
                    missing = sorted(required_guards - present_guards)
                    if missing:
                        add_failure(
                            failures,
                            stem,
                            f"native-owner {label} build guard",
                            "src/nds/nds_renderer_native_owners.c include is missing required owner guard(s) "
                            + ",".join(missing),
                        )
                        failed_owners.add(stem)
            adapter_entrypoint = entrypoint.replace(
                "ndsRendererSubmitNative", "ndsRendererAdapterSubmitNative", 1
            )
            if re.search(
                rf"\bsb32\s+{re.escape(adapter_entrypoint)}\s*\(", matrix_adapter
            ) is None:
                add_failure(
                    failures,
                    stem,
                    "native actor adapter",
                    f"src/port/renderer_adapter_matrix.c lacks {adapter_entrypoint} (breaks at runtime)",
                )
                failed_owners.add(stem)
            if re.search(rf"\b{re.escape(adapter_entrypoint)}\s*\(", movement) is None:
                add_failure(
                    failures,
                    stem,
                    "native actor runtime call",
                    f"src/port/reloc_backend_movement.c does not call {adapter_entrypoint} (breaks at runtime)",
                )
                failed_owners.add(stem)
            continue

        if not packet_match and tracked_packet_rel is None:
            add_failure(
                failures,
                stem,
                "Makefile PACKET variable",
                f"Makefile lacks a *_PACKET assignment for nds_native_{stem}.generated.inc (breaks at generation)",
            )
            failed_owners.add(stem)

        header_re = re.compile(rf"^\s*{re.escape(prefix)}_HEADER\s*(?::|\?|\+)?=")
        header_line = next((line for line in make_lines if header_re.search(line)), None)
        if header_line is None and tracked_packet_rel is None:
            add_failure(
                failures,
                stem,
                "Makefile HEADER variable",
                f"Makefile lacks {prefix}_HEADER (breaks at generation)",
            )
            failed_owners.add(stem)

        prereq_re = re.compile(rf"^\s*{re.escape(prefix)}_PREREQ\s*(?::|\?|\+)?=")
        if tracked_packet_rel is None and not any(prereq_re.search(line) for line in make_lines):
            add_failure(
                failures,
                stem,
                "Makefile PREREQ variable",
                f"Makefile lacks {prefix}_PREREQ (breaks at generation)",
            )
            failed_owners.add(stem)

        rule_re = re.compile(
            rf"\$\({re.escape(prefix)}_PACKET\)\s+\$\({re.escape(prefix)}_HEADER\)\s*&:\s*\$\({re.escape(prefix)}_PREREQ\)"
        )
        if tracked_packet_rel is None and not any(rule_re.search(line) for line in make_lines):
            add_failure(
                failures,
                stem,
                "grouped emit rule",
                f"Makefile lacks $({prefix}_PACKET) $({prefix}_HEADER) &: $({prefix}_PREREQ) (breaks at generation)",
            )
            failed_owners.add(stem)

        renderer_rules = [line for line in make_lines if re.search(r"(?:^|[/\\])nds_renderer\.o\s*:", line)]
        renderer_tokens = (f"$({prefix}_PACKET)", f"$({prefix}_HEADER)")
        if tracked_packet_rel is None and not any(
            all(token in line for token in renderer_tokens) for line in renderer_rules
        ):
            add_failure(
                failures,
                stem,
                "nds_renderer.o prerequisite",
                f"Makefile nds_renderer.o rule lacks {' '.join(renderer_tokens)} (breaks at generation)",
            )
            failed_owners.add(stem)

        scene_rules = [line for line in make_lines if re.search(r"(?:^|[/\\])scene_backend\.o\s*:", line)]
        header_token = f"$({prefix}_HEADER)"
        if tracked_packet_rel is None and not any(header_token in line for line in scene_rules):
            add_failure(
                failures,
                stem,
                "scene_backend.o prerequisite",
                f"Makefile scene_backend.o rule lacks {header_token} (breaks at generation)",
            )
            failed_owners.add(stem)

        packet_sites = include_sites(renderer, packet_name)
        exec_sites = include_sites(renderer, exec_path.name)
        if len(packet_sites) != 1:
            add_failure(
                failures,
                stem,
                "renderer packet include",
                f"src/nds/nds_renderer.c includes {packet_name} {len(packet_sites)} time(s); want exactly 1 (breaks at compile)",
            )
            failed_owners.add(stem)
        if len(exec_sites) != 1:
            add_failure(
                failures,
                stem,
                "renderer exec include",
                f"src/nds/nds_renderer.c includes {exec_path.name} {len(exec_sites)} time(s); want exactly 1 (breaks at compile/link)",
            )
            failed_owners.add(stem)
        elif len(packet_sites) == 1 and packet_sites[0][1] != exec_sites[0][1]:
            add_failure(
                failures,
                stem,
                "matching renderer build guard",
                f"src/nds/nds_renderer.c packet/exec includes use different preprocessor guard stacks",
            )
            failed_owners.add(stem)

        required_guards = owner_feature_guards(exec_text, entrypoint)
        for label, sites in (("renderer packet", packet_sites), ("renderer", exec_sites)):
            if len(sites) != 1:
                continue
            present_guards = expand_guard_tokens(
                guard_tokens(sites[0][1]), guard_implications
            )
            missing_guards = sorted(required_guards - present_guards)
            if missing_guards:
                add_failure(
                    failures,
                    stem,
                    f"{label} build guard",
                    "src/nds/nds_renderer.c include is missing required owner guard(s) "
                    + ",".join(missing_guards),
                )
                failed_owners.add(stem)

        header_name_match = re.search(r"([A-Za-z0-9_]+\.generated\.h)\b", header_line or "")
        header_name = (
            header_name_match.group(1)
            if header_name_match
            else f"nds_native_{stem}.generated.h"
        )
        header_count = len(
            re.findall(
                rf'#\s*include\s*[<"][^>"]*{re.escape(header_name)}[>"]',
                adapter,
            )
        )
        if tracked_packet_rel is None and header_count != 1:
            add_failure(
                failures,
                stem,
                "adapter generated-header include",
                f"src/port/renderer_adapter_stage.c includes {header_name} {header_count} time(s); want exactly 1 (breaks at compile)",
            )
            failed_owners.add(stem)

        handled = handled_variable(stem, adapter)
        if handled is None:
            add_failure(
                failures,
                stem,
                "NO_PROGRAM handled term",
                "renderer_adapter_stage.c has no uniquely matching *_native_handled variable (breaks at runtime)",
            )
            failed_owners.add(stem)
        else:
            condition_re = re.compile(
                rf"(?:\b{re.escape(handled)}\s*==\s*FALSE\b|\bFALSE\s*==\s*{re.escape(handled)}\b|!\s*\b{re.escape(handled)}\b)"
            )
            missing_arms = [
                index + 1
                for index, arm in enumerate(program_arms)
                if condition_re.search(arm) is None
            ]
            if missing_arms:
                add_failure(
                    failures,
                    stem,
                    "all three NO_PROGRAM arms",
                    f"renderer_adapter_stage.c lacks {handled} in arm(s) {','.join(map(str, missing_arms))} (breaks at runtime)",
                )
                failed_owners.add(stem)

    if failures:
        for failure in failures:
            print(failure)
        print(
            f"Native owner wiring check failed: owners={len(exec_paths)} failed_owners={len(failed_owners)} gaps={len(failures)}"
        )
        return 1

    print(f"Native owner wiring check passed: owners={len(exec_paths)} gaps=0")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"Native owner wiring check failed: {exc}", file=sys.stderr)
        raise SystemExit(2)
