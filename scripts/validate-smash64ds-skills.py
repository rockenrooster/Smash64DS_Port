#!/usr/bin/env python3
"""Validate the project-local Nintendo DS Agent Skills pack."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
AGENTS = ROOT / ".agents" / "skills"
CLAUDE = ROOT / ".claude" / "skills"

EXPECTED_SKILLS = {
    "n64-to-nds-asset-conversion",
    "nds-manage-memory-vram",
    "nds-measure-performance",
    "nds-optimize-2d-display",
    "nds-optimize-arm7-audio",
    "nds-optimize-arm9",
    "nds-optimize-dma-storage",
    "nds-optimize-fixed-math",
    "nds-optimize-gx-3d",
    "nds-platform-runtime",
    "nds-port-and-optimize",
    "nds-review-low-level-change",
    "smash64ds-opus-guardrails",
    "smash64ds-project-context",
}

NAME_RE = re.compile(r"^[a-z0-9-]{1,64}$")
FRONTMATTER_RE = re.compile(
    r"\A---\nname:\s*([^\n]+)\ndescription:\s*(.+?)\n---\n",
    re.DOTALL,
)
META_VALUE_RE = re.compile(
    r'^  (display_name|short_description|default_prompt): "([^"\n]*)"$',
    re.MULTILINE,
)
REFERENCE_RE = re.compile(r"references/[A-Za-z0-9._/-]+\.md")
SKILL_REF_RE = re.compile(r"\$([a-z0-9][a-z0-9-]{0,63})")
GENERIC_PROJECT_TOKENS = (
    "Smash64DS",
    "BattleShip",
    "Dream Land",
    "mode 163",
)
CONTEXT_DYNAMIC_RE = (
    re.compile(r"\bmode\s+\d+\b", re.IGNORECASE),
    re.compile(r"\bTask\s+\d+\b"),
    re.compile(r"\bWORK-H\b"),
    re.compile(r"\b[0-9A-Fa-f]{64}\b"),
)
REQUIRED_SNIPPETS = {
    "n64-to-nds-asset-conversion": (
        "Do not reuse another asset's constants without proof.",
        "matching project-context skill provides asset-specific baking rules",
    ),
    "nds-optimize-arm7-audio": (
        "Qualification alone is read-only.",
        "references/audio-qualification.md",
    ),
    "nds-platform-runtime": (
        "Do not assume a frozen picture is an IRQ or platform failure.",
    ),
    "nds-review-low-level-change": (
        "Review requests are read-only unless the user separately authorizes implementation.",
    ),
    "smash64ds-project-context": (
        "Do not copy current mode numbers",
        "docs/optimization/TASK_STANDING_RULES.md",
        "docs/VERIFYING.md",
        "references/ifcommon-assets.md",
    ),
}

errors: list[str] = []


def directory_names(path: Path) -> set[str]:
    if not path.is_dir():
        errors.append(f"Missing directory: {path}")
        return set()
    return {entry.name for entry in path.iterdir() if entry.is_dir()}


canonical_names = directory_names(AGENTS)
bridge_names = directory_names(CLAUDE)

for missing in sorted(EXPECTED_SKILLS - canonical_names):
    errors.append(f"Missing canonical skill: {missing}")
for extra in sorted(canonical_names - EXPECTED_SKILLS):
    errors.append(f"Unexpected canonical skill: {extra}")
for missing in sorted(EXPECTED_SKILLS - bridge_names):
    errors.append(f"Missing Claude bridge: {missing}")
for extra in sorted(bridge_names - EXPECTED_SKILLS):
    errors.append(f"Unexpected Claude bridge: {extra}")

for skill_name in sorted(EXPECTED_SKILLS & canonical_names):
    skill_dir = AGENTS / skill_name
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
    normalized_text = " ".join(text.split())
    if name != skill_name:
        errors.append(f"{skill_file}: name '{name}' does not match folder '{skill_name}'")
    if not NAME_RE.fullmatch(name):
        errors.append(f"{skill_file}: invalid skill name '{name}'")
    if len(description) < 80:
        errors.append(f"{skill_file}: description is too short to trigger reliably")
    if text.count("\n") + 1 > 500:
        errors.append(f"{skill_file}: exceeds the 500-line progressive-disclosure limit")
    if "TODO" in text or "[TODO" in text:
        errors.append(f"{skill_file}: contains a placeholder")

    for snippet in REQUIRED_SNIPPETS.get(name, ()):
        if snippet not in normalized_text:
            errors.append(f"{skill_file}: missing required contract '{snippet}'")

    if name.startswith("nds-"):
        for token in GENERIC_PROJECT_TOKENS:
            if token in text:
                errors.append(f"{skill_file}: generic skill contains project token '{token}'")

    if name == "smash64ds-project-context":
        for pattern in CONTEXT_DYNAMIC_RE:
            if pattern.search(text):
                errors.append(
                    f"{skill_file}: context adapter duplicates dynamic truth matching "
                    f"{pattern.pattern}"
                )

    for reference in sorted(set(REFERENCE_RE.findall(text))):
        if not (skill_dir / reference).is_file():
            errors.append(f"{skill_file}: missing referenced file {reference}")

    referenced_skills = set(SKILL_REF_RE.findall(text))
    if name == "nds-port-and-optimize":
        required_routes = EXPECTED_SKILLS - {"smash64ds-project-context"}
        for missing_route in sorted(required_routes - referenced_skills):
            errors.append(
                f"{skill_file}: entry point does not route $" + missing_route
            )

    for referenced_skill in sorted(referenced_skills):
        if referenced_skill not in EXPECTED_SKILLS:
            errors.append(
                f"{skill_file}: references unknown skill $" + referenced_skill
            )

    metadata = skill_dir / "agents" / "openai.yaml"
    if not metadata.is_file():
        errors.append(f"{skill_dir}: missing agents/openai.yaml")
    else:
        meta_text = metadata.read_text(encoding="utf-8")
        values = dict(META_VALUE_RE.findall(meta_text))
        for key in ("display_name", "short_description", "default_prompt"):
            if key not in values:
                errors.append(f"{metadata}: missing quoted {key}")
        short_description = values.get("short_description", "")
        if short_description and not 25 <= len(short_description) <= 64:
            errors.append(
                f"{metadata}: short_description length "
                f"{len(short_description)} is outside 25-64"
            )
        default_prompt = values.get("default_prompt", "")
        if default_prompt and "$" + name not in default_prompt:
            errors.append(f"{metadata}: default_prompt must mention $" + name)

    bridge = CLAUDE / skill_name / "SKILL.md"
    if not bridge.is_file():
        continue
    bridge_text = bridge.read_text(encoding="utf-8")
    expected_path = f"../../../.agents/skills/{skill_name}/SKILL.md"
    if expected_path not in bridge_text:
        errors.append(f"{bridge}: does not point to {expected_path}")
    bridge_match = FRONTMATTER_RE.match(bridge_text)
    if not bridge_match:
        errors.append(f"{bridge}: invalid or missing YAML frontmatter")
        continue
    bridge_name = bridge_match.group(1).strip()
    bridge_description = " ".join(bridge_match.group(2).split())
    if bridge_name != skill_name:
        errors.append(f"{bridge}: name '{bridge_name}' does not match folder '{skill_name}'")
    if bridge_description != description:
        errors.append(f"{bridge}: description drifted from canonical skill")

if errors:
    print("Skill validation failed:")
    for error in errors:
        print(f"  - {error}")
    sys.exit(1)

print(
    f"Validated {len(EXPECTED_SKILLS)} canonical skills, metadata files, "
    f"references, and Claude bridges."
)
