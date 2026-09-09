#!/usr/bin/env python3
"""Static wiring guard for every native owner executor present in src/nds."""

from __future__ import annotations

import re
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
    renderer_path = ROOT / "src/nds/nds_renderer.c"
    adapter_path = ROOT / "src/port/renderer_adapter_stage.c"
    renderer = renderer_path.read_text(encoding="utf-8", errors="replace")
    adapter = adapter_path.read_text(encoding="utf-8", errors="replace")
    program_arms = no_program_arms(adapter)

    exec_paths = sorted((ROOT / "src/nds").glob("nds_native_*.exec.inc"))
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
        if not packet_match:
            add_failure(
                failures,
                stem,
                "Makefile PACKET variable",
                f"Makefile lacks a *_PACKET assignment for nds_native_{stem}.generated.inc (breaks at generation)",
            )
            failed_owners.add(stem)

        header_re = re.compile(rf"^\s*{re.escape(prefix)}_HEADER\s*(?::|\?|\+)?=")
        header_line = next((line for line in make_lines if header_re.search(line)), None)
        if header_line is None:
            add_failure(
                failures,
                stem,
                "Makefile HEADER variable",
                f"Makefile lacks {prefix}_HEADER (breaks at generation)",
            )
            failed_owners.add(stem)

        prereq_re = re.compile(rf"^\s*{re.escape(prefix)}_PREREQ\s*(?::|\?|\+)?=")
        if not any(prereq_re.search(line) for line in make_lines):
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
        if not any(rule_re.search(line) for line in make_lines):
            add_failure(
                failures,
                stem,
                "grouped emit rule",
                f"Makefile lacks $({prefix}_PACKET) $({prefix}_HEADER) &: $({prefix}_PREREQ) (breaks at generation)",
            )
            failed_owners.add(stem)

        renderer_rules = [line for line in make_lines if re.search(r"(?:^|[/\\])nds_renderer\.o\s*:", line)]
        renderer_tokens = (f"$({prefix}_PACKET)", f"$({prefix}_HEADER)")
        if not any(all(token in line for token in renderer_tokens) for line in renderer_rules):
            add_failure(
                failures,
                stem,
                "nds_renderer.o prerequisite",
                f"Makefile nds_renderer.o rule lacks {' '.join(renderer_tokens)} (breaks at generation)",
            )
            failed_owners.add(stem)

        scene_rules = [line for line in make_lines if re.search(r"(?:^|[/\\])scene_backend\.o\s*:", line)]
        header_token = f"$({prefix}_HEADER)"
        if not any(header_token in line for line in scene_rules):
            add_failure(
                failures,
                stem,
                "scene_backend.o prerequisite",
                f"Makefile scene_backend.o rule lacks {header_token} (breaks at generation)",
            )
            failed_owners.add(stem)

        packet_name_match = re.search(r"([A-Za-z0-9_]+\.generated\.inc)\b", packet_line or "")
        packet_name = (
            packet_name_match.group(1)
            if packet_name_match
            else f"nds_native_{stem}.generated.inc"
        )
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
        elif not any(any("NDS_" in guard for guard in guards) for _, guards in packet_sites):
            add_failure(
                failures,
                stem,
                "renderer packet build guard",
                f"src/nds/nds_renderer.c include {packet_name} is not under an NDS_* preprocessor guard",
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
        elif not any(any("NDS_" in guard for guard in guards) for _, guards in exec_sites):
            add_failure(
                failures,
                stem,
                "renderer build guard",
                f"src/nds/nds_renderer.c include {exec_path.name} is not under an NDS_* preprocessor guard",
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
        if header_count != 1:
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
