#!/usr/bin/env python3
"""Who does the alpha-ignore predicate actually decide for?

`alpha_ignores_texels` (nds_renderer_textures_effects.c) decides whether an
uploaded texture's alpha is THROWN AWAY and forced opaque.  Widening it makes
previously-opaque surfaces honour their alpha, which on a surface that should
be solid means the surface disappears.

On 2026-09-22 that shipped in r37 and took body parts off every fighter, and
the commit message defending it carried a table decoded with TEXEL0 = 0.  The
real constants are in nds_renderer_preamble.c:

    COMBINED 0   TEXEL0 1   TEXEL1 2   PRIMITIVE 3
    SHADE    4   ENVIRONMENT 5   "1" 6   "0" 7

so every `Aa1 = COMBINED` read as `Aa1 = TEXEL0`, and a change that touches
exactly one combine looked like a change that touched four fighter families.

This check exists so that arithmetic is never done by hand again.  It decodes
the fighter policy families straight out of the generated owner table, plus the
named source combines, and reports for each one what every candidate predicate
says.  It FAILS if a predicate other than the shipped one would change the
answer for a FIGHTER family -- because that is the blast radius that matters
and the one I got wrong.

Read-only; runs no build and no emulator.

Usage:
    python scripts/check-alpha-mux-blast-radius.py [-v]
"""

from __future__ import annotations

import os
import re
import sys

SOURCE_POLICIES = "src/nds/nds_native_fighter_owner.generated.inc"
SOURCE_PREAMBLE = "src/nds/nds_renderer_preamble.c"
SOURCE_PREDICATE = "src/nds/nds_renderer_textures_effects.c"

# Alpha mux slot -> (word, shift).  gbi.h:3088-3102 packs alpha A/B separately
# from C/D, which is the whole reason a C/D-only test has a hole.
ALPHA_SLOTS = (
    ("Aa0", 0, 12),
    ("Ab0", 1, 12),
    ("Ac0", 0, 9),
    ("Ad0", 1, 9),
    ("Aa1", 1, 21),
    ("Ab1", 1, 3),
    ("Ac1", 1, 18),
    ("Ad1", 1, 0),
)

# Named source combines worth carrying, so a reader can see which ones the
# predicates separate.  G_CC_MODULATEIA is the Castle roof's.
NAMED_COMBINES = {
    "G_CC_MODULATEIA": (0xFC121824, 0xFF33FFFF),
}


def read_mux_names(root):
    """The ACMUX constants, from the source rather than from memory."""
    path = os.path.join(root, SOURCE_PREAMBLE)
    names = {}
    pattern = re.compile(
        r"#define\s+NDS_RENDERER_ACMUX_(\w+)\s+(\d+)u")
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            match = pattern.match(line.strip())
            if match:
                names[int(match.group(2))] = match.group(1)
    missing = [v for v in range(8) if v not in names]
    if missing:
        raise SystemExit(
            "FAIL: %s does not define all eight ACMUX values (missing %s). "
            "Decode them from the source, never from memory -- that is what "
            "this check is for." % (SOURCE_PREAMBLE, missing))
    return names


def read_policies(root):
    """The fighter policy families, from the generated owner table."""
    path = os.path.join(root, SOURCE_POLICIES)
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    block = re.search(
        r"sNdsNativeFighterDirectPolicies\[\d+\]\s*=\s*\{(.*?)\n\};",
        text, re.S)
    if block is None:
        raise SystemExit(
            "FAIL: sNdsNativeFighterDirectPolicies not found in %s"
            % SOURCE_POLICIES)
    rows = re.findall(r"\{\s*(0x[0-9a-fA-F]+)u,\s*(0x[0-9a-fA-F]+)u,",
                      block.group(1))
    if not rows:
        raise SystemExit(
            "FAIL: no policy rows parsed from %s" % SOURCE_POLICIES)
    return [(int(w0, 16), int(w1, 16)) for w0, w1 in rows]


def slots_of(w0, w1):
    words = (w0, w1)
    return [(name, (words[word] >> shift) & 0x07)
            for name, word, shift in ALPHA_SLOTS]


def uses_texel(slots, keep):
    """TRUE when any KEPT slot names a texel. texel values are 1 and 2."""
    return any(value in (1, 2) for name, value in slots if name in keep)


# The shipped predicate and the candidates, by which slots each one reads.
PREDICATES = {
    "cd-only (shipped)": {"Ac0", "Ad0"},
    "cd + cycle0 A/B": {"Ac0", "Ad0", "Aa0", "Ab0"},
    "all eight (r37, REVERTED)": {name for name, _, _ in ALPHA_SLOTS},
}
SHIPPED = "cd-only (shipped)"


def main(argv):
    verbose = "-v" in argv
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    names = read_mux_names(root)
    policies = read_policies(root)

    subjects = [("fighter family %d" % i, w0, w1, True)
                for i, (w0, w1) in enumerate(policies)]
    subjects += [(name, w0, w1, False)
                 for name, (w0, w1) in sorted(NAMED_COMBINES.items())]

    failures = []
    for label, w0, w1, is_fighter in subjects:
        slots = slots_of(w0, w1)
        answers = {key: uses_texel(slots, keep)
                   for key, keep in PREDICATES.items()}
        if verbose:
            print("%-26s %s" % (
                label,
                " ".join("%s=%s" % (n, names[v]) for n, v in slots)))
            print("%-26s   %s" % ("", "  ".join(
                "%s=%s" % (k, answers[k]) for k in PREDICATES)))
        if not is_fighter:
            continue
        for key, value in answers.items():
            if (key != SHIPPED) and (value != answers[SHIPPED]):
                failures.append(
                    "%s: predicate '%s' answers %s where the shipped "
                    "'%s' answers %s. A fighter family changing its "
                    "alpha-ignore answer means every surface drawn under it "
                    "changes opacity -- that is r37's missing body parts."
                    % (label, key, value, SHIPPED, answers[SHIPPED]))

    # The Castle roof's combine is the reason anyone wants a wider predicate;
    # if a candidate does not separate it from the shipped one, it buys nothing.
    modulateia = slots_of(*NAMED_COMBINES["G_CC_MODULATEIA"])
    if uses_texel(modulateia, PREDICATES[SHIPPED]):
        failures.append(
            "G_CC_MODULATEIA is already seen by the shipped predicate, so the "
            "premise of this whole file has changed. Re-read it.")
    if not uses_texel(modulateia, PREDICATES["cd + cycle0 A/B"]):
        failures.append(
            "G_CC_MODULATEIA is NOT seen by 'cd + cycle0 A/B', so that "
            "candidate would not repair the Castle roof either.")

    if failures:
        for failure in failures:
            print("FAIL:", failure)
        print("%d failure(s)" % len(failures))
        return 1
    print("check-alpha-mux-blast-radius: OK -- %d fighter families unchanged "
          "by every candidate predicate; G_CC_MODULATEIA separated by "
          "'cd + cycle0 A/B' only" % len(policies))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
