#!/usr/bin/env python3
"""Validate the Smash64DS project-local Agent Skills pack."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
AGENTS = ROOT / ".agents" / "skills"
CLAUDE = ROOT / ".claude" / "skills"

NAME_RE = re.compile(r"^[a-z0-9-]{1,64}$")
FRONTMATTER_RE = re.compile(
    r"\A---\nname:\s*([^\n]+)\ndescription:\s*(.+?)\n---\n",
    re.DOTALL,
)

errors: list[str] = []

if not AGENTS.is_dir():
    errors.append(f"Missing canonical directory: {AGENTS}")
else:
    for skill_dir in sorted(p for p in AGENTS.iterdir() if p.is_dir()):
        skill_file = skill_dir / "SKILL.md"
        if not skill_file.is_file():
            errors.append(f"{skill_dir}: missing SKILL.md")
            continue

        text = skill_file.read_text(encoding="utf-8")
        match = FRONTMATTER_RE.match(text)
        if not match:
            errors.append(f"{skill_file}: invalid or missing YAML frontmatter")
            continue

        name = match.group(1).strip()
        description = " ".join(match.group(2).split())
        if name != skill_dir.name:
            errors.append(f"{skill_file}: name '{name}' does not match folder '{skill_dir.name}'")
        if not NAME_RE.fullmatch(name):
            errors.append(f"{skill_file}: invalid skill name '{name}'")
        if len(description) < 80:
            errors.append(f"{skill_file}: description is too short to trigger reliably")
        if "TODO" in text or "<Smash64DS_Port>" in text:
            errors.append(f"{skill_file}: contains a placeholder")

        metadata = skill_dir / "agents" / "openai.yaml"
        if not metadata.is_file():
            errors.append(f"{skill_dir}: missing agents/openai.yaml")
        else:
            meta_text = metadata.read_text(encoding="utf-8")
            for key in ("display_name:", "short_description:", "default_prompt:"):
                if key not in meta_text:
                    errors.append(f"{metadata}: missing {key}")

        bridge = CLAUDE / skill_dir.name / "SKILL.md"
        if not bridge.is_file():
            errors.append(f"{skill_dir}: missing Claude bridge")
        else:
            bridge_text = bridge.read_text(encoding="utf-8")
            expected = f"../../../.agents/skills/{skill_dir.name}/SKILL.md"
            if expected not in bridge_text:
                errors.append(f"{bridge}: does not point to {expected}")
            bridge_match = FRONTMATTER_RE.match(bridge_text)
            if not bridge_match:
                errors.append(f"{bridge}: invalid or missing YAML frontmatter")
            else:
                bridge_name = bridge_match.group(1).strip()
                bridge_description = " ".join(bridge_match.group(2).split())
                if bridge_name != skill_dir.name:
                    errors.append(
                        f"{bridge}: name '{bridge_name}' does not match folder '{skill_dir.name}'"
                    )
                if bridge_description != description:
                    errors.append(
                        f"{bridge}: description drifted from the canonical skill description"
                    )

if CLAUDE.is_dir() and AGENTS.is_dir():
    canonical = {p.name for p in AGENTS.iterdir() if p.is_dir()}
    bridges = {p.name for p in CLAUDE.iterdir() if p.is_dir()}
    for extra in sorted(bridges - canonical):
        errors.append(f"Orphan Claude bridge: {extra}")
    for missing in sorted(canonical - bridges):
        errors.append(f"Missing Claude bridge: {missing}")

if errors:
    print("Skill validation failed:")
    for error in errors:
        print(f"  - {error}")
    sys.exit(1)

count = len([p for p in AGENTS.iterdir() if p.is_dir()])
print(f"Validated {count} canonical skills and {count} Claude bridges.")
