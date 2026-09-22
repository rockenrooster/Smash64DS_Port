#!/usr/bin/env python3
"""Both DIF_AMB derivations must agree, clamp included (P04/K02/J01).

The fighter shade word ``diffuse | (ambient << 16)`` is derived in TWO places
in ``src/nds/nds_renderer_native_common.c``:

    production   ndsRendererNativeShadeProductionActions writes the live word
    replay       ndsFighterPacketApplyTint RE-DERIVES the recorded word when
                 the prim colour or the colour modulate moves

They must produce the same bits for the same inputs, and for most of this
project's life they did not: the replay derivation called
``ndsRendererR2MaterialColor15`` twice and stopped, omitting
``ndsRendererR2ClampDiffuseToMaterial``.  A packet is RECORDED holding the
clamped word, so the divergence only appeared after the first tint move --
damage flash, invincibility blink, team colour -- which replaced the clamped
word with an unclamped one and then latched it, because the function's own
early-out records the new prim hash and does not derive again.

The clamp is a no-op for ``use_material == 0`` and for a white prim, so the
only fighters it can move are the tinted ones it was written for: Pikachu
0xFFD933, Kirby 0x00FF5A and Purin 0xFFCDD8 -- which are exactly the three
face/body rows in docs/BUGS.md.  An untinted run held its colour while the
tinted run beside it jumped, and that is a seam on one model.

This checker reads source only.  It runs nothing and writes nothing.

CLAIM 1  Every function that composes a DIF_AMB word assigns ``diffuse`` from
         ndsRendererR2ClampDiffuseToMaterial first.  This is the invariant the
         bug broke, and it is the one a third derivation site would break
         again.
CLAIM 2  There are exactly the two known derivation sites.  A new one is not
         an error, but it must be read and added here deliberately rather than
         inheriting a silent pass.
CLAIM 3  The clamp still holds both of its exemptions -- ``use_material == 0``
         and a white prim return the diffuse untouched.  Without them the
         clamp itself reintroduces a seam between a no-material run and a
         white-prim run, which is how it was first mis-tuned.
CLAIM 4  The clamp's argument lists match between the two sites, so the two
         derivations cannot drift by passing a different material colour or a
         different modulate.

Usage:  python scripts/check-r2-shade-twin.py [-v]
Exit 0 on success, 1 on the first failed claim.
"""

import os
import re
import sys

SOURCE = os.path.join("src", "nds", "nds_renderer_native_common.c")

CLAMP = "ndsRendererR2ClampDiffuseToMaterial"
FIGHTER_FOLD = "ndsRendererR2MaterialColor15"
COMPOSE = re.compile(r"diffuse\s*\|\s*\(\s*ambient\s*<<\s*16\s*\)")
ASSIGN = re.compile(r"\bdiffuse\s*=\s*" + CLAMP + r"\s*\(")

# The two fighter derivations this file is allowed to contain.  Adding a third
# is a deliberate act; see CLAIM 2.
EXPECTED_SITES = (
    "ndsRendererR2ResolveEpochShade",
    "ndsFighterPacketApplyTint",
)

# Sites that compose the word out of a resolver's RETURN rather than deriving
# it.  ndsRendererNativeShadeProductionActions used to derive it inline; the
# 2026-09-22 tint route moved the derivation into
# ndsRendererR2ResolveEpochShade so the bodies could leave the fighter ITCM
# section, and what is left here is `diffuse = shade & 0xffff` plus the
# recompose.  It is not a third derivation and must not be treated as one --
# but it is also not an exemption to be handed out freely, which is why it is
# named rather than pattern-matched.
DELEGATING_SITES = (
    "ndsRendererNativeShadeProductionActions",
)

# The live resolver may skip the clamp, but only on the branch that installs a
# solid-prim tile instead -- and that branch MUST refuse the packet, because
# the replay has no tile and would shade the same run from the fold.  This is
# the invariant that keeps the twin honest now that the two paths can legally
# differ: they can only differ on frames the packet will not be used for.
TINT_BRANCH_GUARD = "sNdsFighterPacketRecorder.fault"

# Derivations that compose a DIF_AMB word WITHOUT the fighter fold, and are
# therefore outside CLAIM 1.  The discriminator is the material function, not
# the name: a site that folds through ndsRendererR2MaterialColor15 is shading
# a fighter and must clamp.
#
# The rebirth halo has its own fold, ndsRendererRebirthHaloMaterialColor15, on
# its own model -- the respawn platform, not a fighter part.  It is the SAME
# CLASS of omission and is recorded as such in docs/p2/BUG_NOTES.md, but it is
# not one of the reported face/body rows, so it is left alone here rather than
# changed unverified.  If the halo is ever brought under the fighter fold, it
# must move into EXPECTED_SITES and clamp with the others.
KNOWN_UNFOLDED_SITES = (
    "ndsRendererSubmitNativeRebirthHalo",
)


def read_source(root):
    path = os.path.join(root, SOURCE)
    if not os.path.isfile(path):
        return None, "%s not found (run from the repository root)" % SOURCE
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read().split("\n"), None


ATTRIBUTE = re.compile(r"__attribute__\s*\(\(.*?\)\)", re.S)
SIGNATURE_STOP = re.compile(r"(;|\}|\*/)\s*$")


def signature_name(lines, brace_index):
    """Name the definition whose opening brace is on ``lines[brace_index]``.

    Walk backwards over the signature, which may span several lines, until a
    line that plainly ends something else -- a statement, a previous body, a
    comment -- or a preprocessor directive.  Join what is left, drop
    ``__attribute__((...))`` so its own parentheses cannot be mistaken for the
    parameter list, and take the first ``identifier(``.  ITCM/section macros
    carry no parentheses and fall out on their own.
    """
    parts = []
    index = brace_index
    while index >= 0 and len(parts) < 12:
        stripped = lines[index].strip()
        if index != brace_index:
            if not stripped or stripped.startswith("#") or \
                    stripped.startswith("*") or stripped.startswith("//") or \
                    SIGNATURE_STOP.search(stripped):
                break
        parts.append(stripped)
        index -= 1
    text = " ".join(reversed(parts))
    text = text.split("{")[0]
    text = ATTRIBUTE.sub(" ", text)
    match = re.search(r"([A-Za-z_][A-Za-z_0-9]*)\s*\(", text)
    return match.group(1) if match else "<anonymous>"


def split_functions(lines):
    """Yield (name, first_line, body_lines) for every top-level definition.

    Brace depth, not a C parser.  That is enough for this file, which is plain
    C with no nested function definitions.
    """
    functions = []
    depth = 0
    start = None
    for index, line in enumerate(lines):
        opens = line.count("{")
        closes = line.count("}")
        if depth == 0 and opens > 0:
            start = index
        depth += opens - closes
        if depth < 0:
            depth = 0
        if depth == 0 and start is not None and closes > 0:
            functions.append(
                (signature_name(lines, start), start + 1,
                 lines[start:index + 1]))
            start = None
    return functions


def clamp_arguments(body):
    """Return the argument text of each clamp call in a function body."""
    text = "\n".join(body)
    calls = []
    for match in re.finditer(CLAMP + r"\s*\(", text):
        depth = 0
        for offset in range(match.end() - 1, len(text)):
            if text[offset] == "(":
                depth += 1
            elif text[offset] == ")":
                depth -= 1
                if depth == 0:
                    inner = text[match.end():offset]
                    calls.append(re.sub(r"\s+", " ", inner).strip())
                    break
    return calls


# The clamp's parameter list, by ROLE and by the spellings each site is
# allowed to use for it.  The two sites read their inputs from different
# structures -- the live draw from ``state``/``stats``, the replay from
# ``site``/``inputs`` -- so the same quantity has two names.  Pinning the
# accepted spellings per position keeps that from hiding a genuinely wrong
# argument, which comparing the sites only to each other would not: passing
# the prim colour where the modulate belongs at BOTH sites would agree.
ROLES = (
    ("diffuse", ("diffuse",)),
    ("ambient", ("ambient",)),
    ("material colour", ("material_color",)),
    ("use_material", ("use_material",)),
    ("colour modulate", ("color_modulate", "modulate")),
)


def normalise_arguments(arguments):
    """Reduce each argument expression to its leaf identifier."""
    parts = [part.strip() for part in arguments.split(",")]
    roles = []
    for part in parts:
        leaf = part.split("->")[-1].split(".")[-1]
        leaf = re.sub(r"[^A-Za-z_0-9]", "", leaf)
        roles.append(leaf)
    return roles


def main(argv):
    verbose = "-v" in argv
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
    lines, error = read_source(root)
    if error:
        print("FAIL:", error)
        return 1

    failures = []
    functions = split_functions(lines)

    # ---- CLAIM 1 and CLAIM 2: who composes a DIF_AMB word, and do they clamp
    composing = []
    for name, first_line, body in functions:
        text = "\n".join(body)
        if not COMPOSE.search(text):
            continue
        if FIGHTER_FOLD not in text:
            if name in DELEGATING_SITES:
                if verbose:
                    print("delegates to the resolver: %s line %d"
                          % (name, first_line))
                continue
            if name not in KNOWN_UNFOLDED_SITES:
                failures.append(
                    "CLAIM 2: %s (line %d) composes a DIF_AMB word without "
                    "the fighter fold %s and is not a listed exemption. Read "
                    "it: either it shades a fighter and belongs under CLAIM "
                    "1, or it is another model and belongs in "
                    "KNOWN_UNFOLDED_SITES with its reason"
                    % (name, first_line, FIGHTER_FOLD))
            elif verbose:
                print("exempt (own fold): %s line %d" % (name, first_line))
            continue
        composing.append((name, first_line, body))
        if not ASSIGN.search(text):
            failures.append(
                "CLAIM 1: %s (line %d) composes diffuse | (ambient << 16) "
                "without assigning diffuse from %s first -- a replayed frame "
                "would shade a tinted fighter differently from a prepared one"
                % (name, first_line, CLAMP))

    found = tuple(name for name, _, _ in composing)
    if verbose:
        print("derivation sites: %s" % ", ".join(found) if found else
              "derivation sites: none")
    if sorted(found) != sorted(EXPECTED_SITES):
        failures.append(
            "CLAIM 2: DIF_AMB derivation sites are %s, expected %s. A new "
            "site must be read and listed in EXPECTED_SITES, not inherit a "
            "pass; a vanished one means this checker is now watching nothing"
            % (list(found), list(EXPECTED_SITES)))

    # ---- CLAIM 3: the clamp keeps both exemptions --------------------------
    body = None
    for name, _, function_body in functions:
        if name == CLAMP:
            body = "\n".join(function_body)
            break
    if body is None:
        failures.append(
            "CLAIM 3: %s is not defined in %s" % (CLAMP, SOURCE))
    else:
        if not re.search(r"use_material\s*==\s*0u\s*\)\s*\{\s*return", body):
            failures.append(
                "CLAIM 3: %s no longer returns the diffuse untouched for "
                "use_material == 0; the source has no prim multiply there "
                "either, so capping it invents a difference" % CLAMP)
        if "0x00ffffffu" not in body:
            failures.append(
                "CLAIM 3: %s no longer exempts a white prim. Folding white is "
                "the identity, so a capped white-prim run and an uncapped "
                "no-material run beside it climb on different curves -- that "
                "seam is the defect this clamp exists to remove" % CLAMP)

    # ---- CLAIM 4: the two sites pass the same argument roles ---------------
    roles_by_site = {}
    for name, first_line, function_body in composing:
        calls = clamp_arguments(function_body)
        if not calls:
            continue
        roles = normalise_arguments(calls[0])
        roles_by_site[name] = roles
        if verbose:
            print("%s clamp roles: %s" % (name, roles))
        if len(roles) != len(ROLES):
            failures.append(
                "CLAIM 4: %s (line %d) calls %s with %d arguments, expected "
                "%d (%s)" % (name, first_line, CLAMP, len(roles), len(ROLES),
                             ", ".join(role for role, _ in ROLES)))
            continue
        for index, (role, accepted) in enumerate(ROLES):
            if roles[index] not in accepted:
                failures.append(
                    "CLAIM 4: %s (line %d) passes `%s` where %s belongs "
                    "(accepted: %s). The two derivations must fold the same "
                    "quantities or they cannot produce the same word"
                    % (name, first_line, roles[index], role,
                       ", ".join(accepted)))
    if len(roles_by_site) != 2:
        failures.append(
            "CLAIM 4: %d of the 2 fighter derivations call %s; both must"
            % (len(roles_by_site), CLAMP))

    # ---- CLAIM 5: the live resolver's unclamped branch refuses the packet ---
    #
    # The resolver is allowed to skip the clamp, but only where it installs a
    # solid-prim tile instead, and the replay has no tile.  So that branch has
    # to make the packet unusable, or a fighter would alternate between two
    # colours as frames were recorded and replayed.  Without this claim, CLAIM
    # 1 could be satisfied by a clamp the live path never actually reaches.
    resolver = None
    for name, first_line, body in functions:
        if name == "ndsRendererR2ResolveEpochShade":
            resolver = (first_line, "\n".join(body))
            break
    if resolver is None:
        failures.append(
            "CLAIM 5: ndsRendererR2ResolveEpochShade is not defined in %s. "
            "The live derivation moved or was renamed; re-read it before "
            "editing this checker" % SOURCE)
    else:
        first_line, text = resolver
        if TINT_BRANCH_GUARD not in text:
            failures.append(
                "CLAIM 5: ndsRendererR2ResolveEpochShade (line %d) can return "
                "an unclamped word without faulting the packet (`%s` absent). "
                "A replayed frame has no prim tile, so it would shade the "
                "same run from the fold and the fighter would change colour "
                "between recorded and live frames"
                % (first_line, TINT_BRANCH_GUARD))
        elif verbose:
            print("tint branch faults the packet: line %d" % first_line)

    if failures:
        for failure in failures:
            print("FAIL:", failure)
        print("%d failure(s)" % len(failures))
        return 1
    print("check-r2-shade-twin: OK (5 claims)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
