#!/usr/bin/env python3
"""Prove the laser colour-animation override covers Kirby's copied blaster.

BattleShip's Fox neutral-B motion scripts issue
`SetColAnim(nGMColAnimFighterFoxSpecialHiStart, 0)`, a cosmetic body flash that
reads on DS as a strobe. `ndsFTMainCheckSetFighterColAnimID` in
`src/import/battleship_ftmain.c` refuses that one event for Fox/NFox in the two
source laser statuses, on the owner's instruction.

Kirby does not run Fox's scripts. He carries his own copies -- 228
`dKirbyMainMotion_LaserGround` and `dKirbyMainMotion_LaserAir` -- which issue
the identical command while `fkind` is Kirby and the status is
`nFTKirbyStatusCopyFoxSpecialN` / `...CopyFoxSpecialAirN`. A gate keyed on the
Fox fighter kind therefore misses every one of them, which is the white-and-gold
flash reported over Kirby's body when the copied blaster fires.

The failure mode is an *omission*: the port compiles, Fox stays correct, and only
the copied ability shows the flash. Two earlier repairs on this exact bug guessed
a maker instead of finding the producer, so pin the pairing structurally:

  * the source scripts that issue the command are found, and every Kirby script
    carrying `nGMColAnimFighterFoxSpecialHiStart` is accounted for -- the copied
    Fox pair is suppressed, and the copied Samus/Donkey scripts that reuse the
    same colanim id are deliberately NOT;
  * the port's override has a Kirby clause naming both copied-Fox statuses and
    the same colanim constant, and still returns FALSE for it;
  * the original Fox clause survives, so widening coverage never silently drops
    the repair it was extended from.

Structural, not numerical: the point is that the two ends agree about which
statuses flash, not what the flash looks like.
"""

from __future__ import annotations

import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parents[1]
PORT = REPO / "src/import/battleship_ftmain.c"
SOURCE = REPO / "decomp/BattleShip-main/decomp/src/relocData/228_KirbyMainMotion.c"

PORT_FUNCTION = "ndsFTMainCheckSetFighterColAnimID"
COLANIM = "nGMColAnimFighterFoxSpecialHiStart"

# The copied blaster's ground script is named `dKirbyMainMotion_LaserGround` in
# REGION_JP and `dKirbyMainMotion_0x1E18` elsewhere, so a name list would pin the
# wrong region. Discriminate on content instead: a Kirby script that both sets
# this colanim and plays Fox's neutral-B cue IS the copied blaster. Kirby's
# copied Samus and Donkey scripts reuse the colanim id without that cue, and the
# owner's report is about the pistol only, so they must keep their flash.
BLASTER_CUE = "nSYAudioFGMFoxSpecialN"
BLASTER_SCRIPT_COUNT = 2  # ground and air

ARRAY_RE = re.compile(
    r"(?:ftMotionCommand|u32)\s+(dKirbyMainMotion_\w+)\s*\[\]\s*=\s*\{",
)

KIRBY_STATUSES = (
    "nFTKirbyStatusCopyFoxSpecialN",
    "nFTKirbyStatusCopyFoxSpecialAirN",
)
FOX_STATUSES = (
    "nFTFoxStatusSpecialN",
    "nFTFoxStatusSpecialAirN",
)


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    sys.exit(1)


def read(path: pathlib.Path) -> str:
    if not path.is_file():
        fail(f"{path.relative_to(REPO)} is missing")
    return path.read_text(encoding="utf-8", errors="replace")


def extract_function(text: str, name: str, path: pathlib.Path) -> str:
    """Return the body of `name`, brace-matched from its opening brace."""
    match = re.search(rf"\b{re.escape(name)}\s*\([^;{{]*\)\s*\{{", text)
    if match is None:
        fail(f"{path.relative_to(REPO)} no longer defines {name}")
    start = match.end() - 1
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    fail(f"{path.relative_to(REPO)}: unbalanced braces in {name}")
    raise AssertionError("unreachable")


def scripts_containing(text: str, needle: str) -> dict[str, str]:
    """Map motion-script name to body, for every script mentioning `needle`."""
    found: dict[str, str] = {}
    for match in ARRAY_RE.finditer(text):
        start = match.end() - 1
        depth = 0
        for index in range(start, len(text)):
            if text[index] == "{":
                depth += 1
            elif text[index] == "}":
                depth -= 1
                if depth == 0:
                    body = text[start : index + 1]
                    if needle in body:
                        found[match.group(1)] = body
                    break
    return found


def check_source(text: str) -> None:
    owners = scripts_containing(text, COLANIM)
    if not owners:
        fail(
            f"{SOURCE.relative_to(REPO)} no longer issues {COLANIM}; the port "
            "override may now be suppressing an event the source never sends"
        )

    blaster = sorted(name for name, body in owners.items() if BLASTER_CUE in body)
    if len(blaster) != BLASTER_SCRIPT_COUNT:
        fail(
            f"expected {BLASTER_SCRIPT_COUNT} Kirby scripts issuing {COLANIM} "
            f"alongside {BLASTER_CUE} (the copied blaster's ground and air "
            f"laser); found {len(blaster)}: {blaster or 'none'}. Either the "
            "source changed or another copied ability now plays Fox's cue -- "
            "re-derive which statuses the override must cover."
        )

    others = sorted(name for name in owners if name not in blaster)
    print(
        f"  source: {COLANIM} issued by copied-blaster scripts "
        f"{', '.join(blaster)}; left alone in {', '.join(others) or 'no other script'}"
    )


def check_port(body: str) -> None:
    for status in KIRBY_STATUSES:
        if status not in body:
            fail(
                f"{PORT_FUNCTION} does not test {status}. Kirby's copied "
                "blaster will flash white and gold again."
            )
    for status in FOX_STATUSES:
        if status not in body:
            fail(
                f"{PORT_FUNCTION} lost its original Fox clause ({status}); "
                "extending the override must not drop what it extended."
            )
    if body.count(COLANIM) < 2:
        fail(
            f"{PORT_FUNCTION} tests {COLANIM} fewer than twice: the Fox and "
            "Kirby clauses must each match the exact source event, not a "
            "broader colour-animation class."
        )
    if body.count("return FALSE;") < 2:
        fail(
            f"{PORT_FUNCTION} has fewer than two refusals; a clause that "
            "counts the event but still forwards it suppresses nothing."
        )
    if "#if" in body or "#ifdef" in body:
        fail(
            f"{PORT_FUNCTION} now contains a preprocessor conditional. Both "
            "Kirby status constants and both fighter kinds come from the "
            "unconditional enums in ft/fighter.h, so a guard here can only "
            "delete the clause in some configuration while this checker still "
            "reads green -- which is the silent-omission failure being fixed."
        )


def main() -> int:
    check_source(read(SOURCE))
    check_port(extract_function(read(PORT), PORT_FUNCTION, PORT))
    print(
        "OK: the copied-Fox laser colour animation is refused for Kirby in "
        f"{', '.join(KIRBY_STATUSES)}, Fox's original clause survives, and "
        "Kirby's other copied abilities keep theirs."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
