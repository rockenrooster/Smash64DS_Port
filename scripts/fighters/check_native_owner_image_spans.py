#!/usr/bin/env python3
"""Every root's spans must fit the OWNER IMAGE's tables, not the in-binary ones.

`ndsRendererValidateNativeFighterOwner` clause 6
(`src/nds/nds_renderer_native_fighter_production.c:1232`) checks three spans per
root against the tables that are actually live.  When
`NDS_NATIVE_OWNER_IMAGE_<KIND>` is set -- which is the default,
`NDS_NATIVE_OWNER_IMAGE ?= 1` -- the in-binary `sNdsNative<Kind>Fighter*` arrays
are compiled OUT and the tables are bound at load from
`nitro:/fighters/<kind>_<detail>.bin`, whose element counts are the
`NDS_NATIVE_IMAGE_<KIND>_<DETAIL>_*_COUNT` macros.

The ROOT TABLE is not in the image.  `sNdsNative<Kind>Roots[]` stays compiled in
whatever the image flag says, so a root row and the image it indexes come from
two different emissions and nothing was comparing them.  When they disagree the
validator rejects, the adapter clears `native_owner_enabled`, and a packed
preview then reaches `ndsPreviewPackLoadHalt(20u, ...)` -- a deliberate
`for (;;)`.  That is a HANG, in the shipping configuration, on whichever
character select entry loads that owner.

Found by exactly that hang on Ness: `sNdsNativeNessRoots` rows 0, 4, 9, 10, 12
and 13 carry `tail_state_first` 188..193, the in-binary
`sNdsNativeNessFighterStateSequence` has 195 entries, and
`NDS_NATIVE_IMAGE_NESS_HIGH_STATE_SEQUENCE_COUNT` is 177.

CLAIM 1  Every root's tail-state span fits the image's state-sequence count.
CLAIM 2  Every root's epoch span fits the image's epoch count.
CLAIM 3  Where an in-binary array still exists beside an image, the two agree.
         A disagreement is the generator drift itself, visible before any root
         indexes into the gap.

Usage:  python scripts/fighters/check_native_owner_image_spans.py [-v]
Exit 0 on success, 1 on the first failed claim.
"""

import os
import re
import sys

OWNERS = os.path.join("src", "nds", "nds_native_fighter_owner.generated.inc")
IMAGES = os.path.join("include", "nds", "generated",
                      "nds_native_fighter_image.generated.h")

STATE_NONE = 0xffff

# Roots are PER DETAIL, and getting this wrong is the first thing this checker
# did: `sNdsNative<Kind>Roots` is the HIGH owner's table and
# `sNdsNative<Kind>RootsLow` is the LOW owner's (nds_renderer_assets.c:1866 and
# :1870 bind them to the two owners separately). Testing one array against both
# images manufactures failures on every kind whose two details have different
# table lengths -- which is most of them.
ROOT_ARRAY = re.compile(
    r"static\s+const\s+NDSNativeRoot\s+sNdsNative(\w+?)Roots(Low)?\s*\[(\d+)\]"
    r"\s*=\s*\{", re.M)
ROOT_ROW = re.compile(
    r"\{\s*0x([0-9a-fA-F]+)u\s*,\s*(\d+)u\s*,\s*(\d+)u\s*,\s*(\d+)u\s*,"
    r"\s*(\d+)u\s*,\s*(\d+)u\s*,\s*(\d+)u\s*,\s*(\d+)u\s*\}")
IMAGE_COUNT = re.compile(
    r"#define\s+NDS_NATIVE_IMAGE_(\w+?)_(HIGH|LOW)_(\w+?)_COUNT\s+(\d+)u")
INBIN_ARRAY = re.compile(
    r"static\s+const\s+(?:u8|NDSNativeEpoch|NDSNativeStateDelta|NDSNativeRun)\s+"
    r"sNdsNative(\w+?)Fighter(StateSequence|Epochs|StateDeltas|Runs)"
    r"(Low)?\s*\[(\d+)\]")

# Root struct field order, src/nds/nds_renderer_assets.c:201.
FIELDS = ("root_offset", "first_epoch", "tail_state_first",
          "source_command_count", "epoch_count", "tail_state_count",
          "tail_sync_count", "light_preamble")

# Image macro suffix -> the in-binary array suffix naming the same table.
SAME_TABLE = {
    "STATE_SEQUENCE": "StateSequence",
    "EPOCHS": "Epochs",
    "STATE_DELTAS": "StateDeltas",
    "RUNS": "Runs",
}


def read(root, relative):
    path = os.path.join(root, relative)
    if not os.path.isfile(path):
        return None, "%s not found (run from the repository root)" % relative
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read(), None


def parse_roots(text):
    """{(kind, detail): (declared, [ {field: value}, ... ])}."""
    roots = {}
    for match in ROOT_ARRAY.finditer(text):
        kind = match.group(1)
        detail = "LOW" if match.group(2) else "HIGH"
        declared = int(match.group(3))
        end = text.find("};", match.end())
        body = text[match.end():end if end != -1 else match.end()]
        rows = []
        for row in ROOT_ROW.finditer(body):
            values = [int(row.group(1), 16)] + \
                [int(row.group(i)) for i in range(2, 9)]
            rows.append(dict(zip(FIELDS, values)))
        roots[(kind, detail)] = (declared, rows)
    return roots


def parse_image_counts(text):
    """{(KIND, DETAIL): {SUFFIX: count}}"""
    counts = {}
    for match in IMAGE_COUNT.finditer(text):
        kind, detail, suffix, value = match.groups()
        counts.setdefault((kind, detail), {})[suffix] = int(value)
    return counts


def parse_inbinary(text):
    """{(Kind, Suffix, DETAIL): length}. Low arrays serve the LOW owner."""
    lengths = {}
    for match in INBIN_ARRAY.finditer(text):
        kind, suffix, low, length = match.groups()
        detail = "LOW" if low else "HIGH"
        lengths[(kind, suffix, detail)] = int(length)
    return lengths


def span_fits(first, count, total):
    """ndsRendererNativeArraySpanFits, production:643."""
    if first > total:
        return False
    return count <= (total - first)


def main(argv):
    verbose = "-v" in argv
    root = os.path.abspath(
        os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))

    owners_text, error = read(root, OWNERS)
    if error:
        print("FAIL:", error)
        return 1
    images_text, error = read(root, IMAGES)
    if error:
        print("FAIL:", error)
        return 1

    roots = parse_roots(owners_text)
    image_counts = parse_image_counts(images_text)
    inbinary = parse_inbinary(owners_text)

    if not roots:
        print("FAIL: no sNdsNative<Kind>Roots arrays parsed from %s" % OWNERS)
        return 1
    if not image_counts:
        print("FAIL: no NDS_NATIVE_IMAGE_*_COUNT macros parsed from %s" % IMAGES)
        return 1

    failures = []
    checked = 0

    for (kind, detail), (declared, rows) in sorted(roots.items()):
        if declared != len(rows):
            failures.append(
                "%s %s: array declares %d roots, %d rows parsed -- the parser "
                "and the table disagree, so this check is not watching them"
                % (kind, detail, declared, len(rows)))
            continue
        counts = image_counts.get((kind.upper(), detail))
        if counts is None:
            continue
        sequence_total = counts.get("STATE_SEQUENCE")
        epoch_total = counts.get("EPOCHS")
        if verbose:
            print("%s %s: state_sequence=%s epochs=%s roots=%d"
                  % (kind, detail, sequence_total, epoch_total, len(rows)))
        for index, row in enumerate(rows):
            # ---- CLAIM 1: the tail-state span ------------------------
            first = row["tail_state_first"]
            if (sequence_total is not None) and (first != STATE_NONE):
                checked += 1
                if not span_fits(first, row["tail_state_count"],
                                 sequence_total):
                    failures.append(
                        "CLAIM 1: %s %s root %d wants state sequence "
                        "[%d, %d) but the image has %d entries -- "
                        "validator clause 6, which clears "
                        "native_owner_enabled and hangs a packed preview "
                        "in ndsPreviewPackLoadHalt(20)"
                        % (kind, detail, index, first,
                           first + row["tail_state_count"],
                           sequence_total))
            # ---- CLAIM 2: the epoch span -----------------------------
            if epoch_total is not None:
                checked += 1
                if not span_fits(row["first_epoch"], row["epoch_count"],
                                 epoch_total):
                    failures.append(
                        "CLAIM 2: %s %s root %d wants epochs [%d, %d) but "
                        "the image has %d -- same clause 6"
                        % (kind, detail, index, row["first_epoch"],
                           row["first_epoch"] + row["epoch_count"],
                           epoch_total))

    # ---- CLAIM 3: an image and a surviving in-binary array must agree ----
    for (kind, suffix, detail), length in sorted(inbinary.items()):
        counts = image_counts.get((kind.upper(), detail))
        if counts is None:
            continue
        for image_suffix, array_suffix in SAME_TABLE.items():
            if array_suffix != suffix:
                continue
            total = counts.get(image_suffix)
            if (total is None) or (total == length):
                continue
            failures.append(
                "CLAIM 3: %s %s %s is %d in the image and %d in the "
                "in-binary array. Both are emitted from the same source, so "
                "they came from different generator runs -- regenerate before "
                "trusting either"
                % (kind, detail, suffix, total, length))

    if failures:
        for failure in failures:
            print("FAIL:", failure)
        print("%d failure(s)" % len(failures))
        return 1
    print("check_native_owner_image_spans: OK (%d root spans across %d owners)"
          % (checked, len(roots)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
