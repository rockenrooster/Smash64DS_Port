#!/usr/bin/env python3
"""R01-B: the Results winner-emblem must keep a native dispatch path.

The emblem was invisible because `gcAddGObjDisplay(gobj, gcDrawDObjTreeForGObj,
33, ...)` reached no geometry on this target: three separate guards -- two
`gNdsSceneManagerCurrIsBattle == 0` gates in the stage recorders and an
`nSCKindOpeningRoom` gate in the opening-room recorder -- turned the source's
own display callback into a pure observation. Nothing failed loudly, so the
absence was invisible to every counter and every build.

This test fails if that route is removed again, and it fails in the other
direction too: it refuses a "fix" that simply opens the battle gate, because
that would admit every Results DObj tree into the battle stage loop instead of
the one GObj the scene actually needs.

    python scripts/menus/test_vs_emblem_dispatch.py
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CAPTURE = ROOT / "src" / "port" / "opening_movie_backend.c"
OWNER = ROOT / "src" / "port" / "reloc_backend_movement.c"
IMPORT_TU = ROOT / "src" / "import" / "battleship_mnvsresults.c"

OWNER_FN = "ndsResultsEmblemRecordCapturedDisplay"
PUBLISH_FN = "ndsVSResultsEmblemGObj"

failures: list[str] = []


def check(condition: bool, message: str) -> None:
    if not condition:
        failures.append(message)


def body_of(text: str, signature: str) -> str:
    """Return the brace-balanced body of the function whose text starts at
    `signature`.  Cheap but exact enough for a source-shape guard."""
    start = text.index(signature)
    open_brace = text.index("{", start)
    depth = 0
    for i in range(open_brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace:i + 1]
    raise AssertionError(f"unbalanced braces after {signature!r}")


def check_capture_site(text: str) -> None:
    """The owner must be called from gcCaptureCameraGObj, before the source
    display proc, and its verdict must suppress that proc."""
    check(OWNER_FN in text, f"{CAPTURE.name}: {OWNER_FN} is never called")
    if OWNER_FN not in text:
        return

    body = body_of(text, "gcCaptureCameraGObj(GObj *camera_gobj")
    check(OWNER_FN in body,
          f"{CAPTURE.name}: {OWNER_FN} is not called from gcCaptureCameraGObj; "
          "the emblem's camera is only in hand there")

    call = body.index(OWNER_FN)
    proc = body.index("current_gobj->proc_display(current_gobj)")
    check(call < proc,
          f"{CAPTURE.name}: {OWNER_FN} is called after proc_display; the "
          "source proc would run first and the emblem would draw twice or "
          "not at all")

    # The result has to reach the handled flag, or the source proc runs anyway.
    assign = re.search(
        r"native_stage_handled\s*=\s*\n?\s*" + OWNER_FN, body)
    check(assign is not None,
          f"{CAPTURE.name}: {OWNER_FN}'s result is discarded; it must feed "
          "native_stage_handled so the observation-only proc is skipped")


def check_owner(text: str) -> None:
    check(f"sb32 {OWNER_FN}(" in text or f"{OWNER_FN}(void *camera_gobj" in text,
          f"{OWNER.name}: {OWNER_FN} is not defined here")
    if OWNER_FN not in text:
        return

    body = body_of(text, f"{OWNER_FN}(void *camera_gobj")

    # It must actually submit geometry, not just record.
    check("ndsRendererAdapterSubmitStageDObj" in body,
          f"{OWNER.name}: {OWNER_FN} submits no geometry; a recorder is what "
          "made the emblem invisible in the first place")
    check("ndsRendererAdapterBeginStageTraversal" in body and
          "ndsRendererAdapterEndStageTraversal" in body,
          f"{OWNER.name}: {OWNER_FN} does not bracket its submit with a "
          "traversal; Results has no battle-loop bracket to inherit")

    # It must claim ONE GObj by identity, not a whole scene.
    check(PUBLISH_FN in body,
          f"{OWNER.name}: {OWNER_FN} does not compare against the published "
          f"emblem GObj ({PUBLISH_FN}); id and DL link alone are a guess")
    check("NDS_NATIVE_VS_EMBLEM_RESULTS_DL_LINK" in body,
          f"{OWNER.name}: {OWNER_FN} does not restrict itself to the source's "
          "Results emblem DL link")

    # An empty draw is a failure under the native-only contract.
    check("ndsRendererRecordNativeFailure" in body,
          f"{OWNER.name}: {OWNER_FN} can claim a GObj having emitted no "
          "triangles without reporting it")

    # THE NEGATIVE CONTROL. Both battle gates must still be closed: the repair
    # is an owner, not a wider gate.
    for fn, sig in (("ndsStageGCDrawAllLoopRecordCapturedDisplay",
                     "ndsStageGCDrawAllLoopRecordCapturedDisplay(void *camera_gobj"),
                    ("ndsStageGCDrawAllLoopRecordDObjDraw",
                     "ndsStageGCDrawAllLoopRecordDObjDraw(void *gobj, u32 kind)")):
        guarded = body_of(text, sig)
        check("gNdsSceneManagerCurrIsBattle == 0u" in guarded,
              f"{OWNER.name}: {fn} no longer refuses non-battle scenes. The "
              "emblem repair must not widen that gate -- it would admit every "
              "Results DObj tree into the battle stage loop.")


def check_identity_publisher(text: str) -> None:
    check(f"void *{PUBLISH_FN}(void)" in text,
          f"{IMPORT_TU.name}: {PUBLISH_FN} is gone; the owner has no identity "
          "to compare against")
    check("#define mnVSResultsEmblemProcUpdate(gobj)" in text,
          f"{IMPORT_TU.name}: the mnVSResultsEmblemProcUpdate seam is gone; "
          "it is the only wrapper that yields the emblem GObj by pointer")
    check("ndsBaseMNVSResultsEmblemProcUpdate(gobj);" in text,
          f"{IMPORT_TU.name}: the source emblem updater is no longer called; "
          "its scale-down and rise are the source's, not the renderer's")

    # Lifetime: the pointer must be dropped when the arena is reused.
    start = body_of(text, "void mnVSResultsStartScene(void)")
    check("sNdsVSResultsEmblemGObj = NULL;" in start,
          f"{IMPORT_TU.name}: the emblem GObj is not cleared in "
          "mnVSResultsStartScene; a Results re-entry would compare against a "
          "pointer into a rewound arena")


def main() -> int:
    for path in (CAPTURE, OWNER, IMPORT_TU):
        if not path.is_file():
            print(f"FAIL: missing {path}", file=sys.stderr)
            return 1

    check_capture_site(CAPTURE.read_text(encoding="utf-8"))
    check_owner(OWNER.read_text(encoding="utf-8"))
    check_identity_publisher(IMPORT_TU.read_text(encoding="utf-8"))

    if failures:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1
    print("PASS: Results winner-emblem native dispatch "
          "(capture site, owner, identity, lifetime, battle gates closed)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
