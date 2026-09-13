#!/usr/bin/env python3
"""Check portable pack structure and local Markdown links (no network)."""
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[1]
errors = []
skill = (ROOT / "SKILL.md").read_text(encoding="utf-8")
parts = skill.split("---", 2)
if len(parts) != 3 or parts[0].strip():
    errors.append("missing YAML frontmatter")
else:
    if "name: n64-to-nds-porting" not in parts[1].splitlines():
        errors.append("wrong skill name")
    descriptions = [s[len("description: "):] for s in parts[1].splitlines() if s.startswith("description: ")]
    if len(descriptions) != 1 or not 1 <= len(descriptions[0]) <= 1024:
        errors.append("description missing or over 1024 characters")
    if len(parts[2].split()) > 5000:
        errors.append("entrypoint over 5000 words")
for relative in ["README.md", "LICENSE", "agents/openai.yaml", "references/SOURCES.md",
                 "examples/README.md", "tests/REVIEW_RESULTS.md"]:
    if not (ROOT / relative).is_file(): errors.append(f"missing {relative}")
for path in ROOT.rglob("*.md"):
    text = path.read_text(encoding="utf-8")
    if "\ufffd" in text: errors.append(f"replacement character in {path.relative_to(ROOT)}")
    for target in re.findall(r"\[[^\]\n]*\]\(([^)\s]+)\)", text):
        if "://" in target or target.startswith(("#", "mailto:")): continue
        target = target.split("#", 1)[0]
        if target and not (path.parent / target).is_file():
            errors.append(f"broken local link in {path.relative_to(ROOT)}: {target}")
for path in ROOT.rglob("*"):
    if path.is_file() and path.suffix in {".rom", ".z64", ".v64", ".nds", ".ttf", ".otf"}:
        errors.append(f"unexpected bundled asset: {path.name}")
if errors:
    print("\n".join(errors), file=sys.stderr)
    raise SystemExit(1)
print("PASS: frontmatter, structure, local Markdown links, asset exclusions")
