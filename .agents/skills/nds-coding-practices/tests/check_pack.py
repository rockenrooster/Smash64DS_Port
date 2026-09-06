#!/usr/bin/env python3
"""Local packaging/link checks. No network and no behavioral validation."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def main() -> None:
    problems = []
    skill = (ROOT/"SKILL.md").read_text()
    if not skill.startswith("---\nname: nds-coding-practices\n"):
        problems.append("invalid skill frontmatter/name")
    count = 0
    for path in ROOT.rglob("*"):
        if not path.is_file(): continue
        if "__pycache__" in path.parts: continue
        if path.suffix not in (".md", ".c", ".h", ".py", ".yaml"): continue
        text = path.read_text(encoding="utf-8")
        if not text.endswith("\n"): problems.append(f"missing final newline: {path}")
        if path.suffix != ".md": continue
        count += 1
        for dest in re.findall(r"\[[^\]]*\]\(([^)]+)\)", text):
            dest = dest.split("#",1)[0]
            if not dest or "://" in dest or dest.startswith("mailto:"): continue
            if not (path.parent/dest).exists():
                problems.append(f"broken internal link: {path.relative_to(ROOT)} -> {dest}")
    for dest in re.findall(r"`((?:references|examples|tests)/[^`\n]+)`", skill):
        if not (ROOT/dest).exists(): problems.append(f"missing root route: {dest}")
    if problems: raise SystemExit("\n".join(problems))
    print(f"PASS: frontmatter, UTF-8/newlines, root routes, and internal links ({count} Markdown files)")

if __name__ == "__main__":
    main()
