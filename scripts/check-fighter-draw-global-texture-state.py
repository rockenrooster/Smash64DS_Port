#!/usr/bin/env python3
"""The fighter draw walk must not mutate global texture state.

2026-09-22, twice in one day, in the same change. The face/body repair bound a
solid-prim tile from inside the fighter run prepare, and the owner reported
every fighter losing body parts. r43 invalidated the texture prepare afterwards
on the theory that the bind alone was the problem; the owner reported the same
regression, and the route was removed.

The arithmetic in that repair was right -- the DS has exactly one post-shade
multiply and it is the texture, so a tinted prim folded into diffuse/ambient
cannot reproduce clamp8(l2 + l1*dot) * prim, and the measured worst-case error
fell from 6/31 to 1/31 when it did.  What was wrong was the PLUMBING, three
times over:

  1. it BOUND a texture behind ndsRendererHardwareBindTexture's tracker, which
     only re-binds on a FULL prepare, so a run reusing its prepare drew with
     the wrong texture;
  2. it CREATED textures mid-frame, and PrepareIFCommonAtlas calls
     ndsRendererHardwareEvictTexture(NULL) on an upload failure -- which takes
     entries out of the GENERIC cache that other fighters' textures live in;
  3. it wrote the texture trackers by hand.  r42 bound with a raw
     glBindTexture and set the bound-name tracker itself; r43 added a null of
     sNdsRendererHardwareActiveTextureEntry.  The null turned out to be
     REQUIRED, not a violation: the cache's binds elide on `ActiveTextureEntry
     != entry`, so a foreign bind that leaves the pointer naming the previous
     entry makes the next run wanting that entry skip its bind and draw with
     the foreign texels.  What was wrong was doing it ad hoc from inside the
     walk.  The impact-wave, rebirth-halo and entry-effect owners perform that
     bind-plus-null pair beside each of their own binds; a fighter-walk route
     that needs one must add it as a named seam below, with that reason.

Nothing in the tree caught any of it.  check-r2-shade-twin proved the two shade
derivations agreed; check-alpha-mux-blast-radius proved no fighter family
changed opacity.  Both were green while every fighter lost body parts, because
neither watched the thing that broke.  This is that guard.

It follows CALLS, not just mentions, because a mention check would have passed
the very change it exists for: the allocator was three hops away, through a
helper deliberately placed outside ITCM.  It does not model control flow --
reaching a forbidden symbol on any path is enough -- and it reads the whole
unity translation unit, since a static call crosses those files freely.

A repair that needs a texture must bring it in through the prepare's own seam,
or create it at scene setup where the impact-wave and rebirth-halo tiles are
created, not from inside the walk.  An exemption has to be named below with its
reason, which is the point.

Read-only; runs no build and no emulator.

Usage:
    python scripts/check-fighter-draw-global-texture-state.py [-v]
"""

from __future__ import annotations

import os
import re
import sys

# The unity translation unit: src/nds/nds_renderer.c #includes these in order,
# so a fighter-code function reaches a helper in any of them with an ordinary
# static call.  That is exactly how the withdrawn repair reached the texture
# allocator -- fighter code -> ndsRendererR2ResolveEpochShade ->
# ndsRendererR2FighterTintTexture -> PrepareIFCommonPal16Atlas -- so a checker
# reading one file would have passed it.
SOURCES = (
    os.path.join("src", "nds", "nds_renderer_preamble.c"),
    os.path.join("src", "nds", "nds_renderer_assets.c"),
    os.path.join("src", "nds", "nds_renderer_dl_core.c"),
    os.path.join("src", "nds", "nds_renderer_textures_effects.c"),
    os.path.join("src", "nds", "nds_renderer_native_common.c"),
)

# How many call hops to follow from a fighter-code entry.  The withdrawn repair
# needed three; six leaves room without becoming a whole-program analysis.
MAX_HOPS = 6

# The marker that puts a function in the fighter's ITCM draw section.
FIGHTER_CODE = "NDS_RENDERER_NATIVE_FIGHTER_CODE"

# Calls that mutate state the GENERIC texture cache owns, or that the
# texture-prepare tracker assumes only it performs.
FORBIDDEN = (
    ("ndsRendererHardwarePrepareIFCommonAtlas",
     "creates a texture mid-frame; its upload-failure path evicts generic "
     "cache entries other fighters depend on"),
    ("ndsRendererHardwarePrepareIFCommonPal16Atlas",
     "same, via the PAL16 wrapper"),
    ("ndsRendererHardwarePrepareIFCommonA3I5Atlas",
     "same, via the A3I5 wrapper"),
    ("ndsRendererHardwarePrepareIFCommonCloudAtlas",
     "same, via the cloud wrapper"),
    ("ndsRendererHardwareEvictTexture",
     "takes an entry out of the generic cache; the fighter walk cannot know "
     "what else is mid-flight on it"),
    ("glTexImage2D", "raw upload; see PrepareIFCommonAtlas"),
    ("glGenTextures", "raw name allocation inside the walk"),
    # BARE binds. Both move the hardware without keeping
    # sNdsRendererHardwareActiveTextureEntry honest, and the cache's own binds
    # elide against that pointer -- so the next run wanting the previously
    # active entry skips its bind and samples whatever was bound here. The
    # cache binds its own entries through the SEAMS below; anything else needs
    # a seam of its own that also nulls the active entry.
    ("ndsRendererHardwareBindTextureName",
     "a bare name bind leaves the cache's active entry stale"),
    ("ndsRendererHardwareBindTextureState",
     "a raw glBindTexture updates neither tracker"),
    # WRITE-only. The packet recorder READS this pointer to note which entry a
    # bind used, which is observation and is fine; assigning it is the generic
    # cache's own bookkeeping and is not the walk's to do. A mention check
    # cannot tell those apart and went red on the shipped tree for the read.
    ("sNdsRendererHardwareActiveTextureEntry=",
     "the generic cache's bookkeeping for the active entry; the walk may read "
     "it but must not assign it"),
)

# THE SANCTIONED SEAMS, and why this is a seam list rather than a symbol list.
#
# The first cut of this check simply forbade the texture-mutating symbols. It
# went red on the SHIPPED tree, because the fighter walk legitimately reaches
# eviction and the active-entry pointer -- through the texture cache's OWN
# resolve path, which is how a fighter's own textures become resident. Blunt
# forbidding cannot tell that apart from an outsider smuggling in a tile.
#
# So the rule is about ROUTE, not symbol: the walk may reach texture machinery
# ONLY through these entry points, whose interiors are the cache's business and
# are not searched. Anything else in the walk that reaches a creation API has
# introduced a texture of its own, which is exactly what the withdrawn
# face/body repair did and exactly what nothing caught.
#
# Adding a name here says "this is part of the sanctioned texture seam". That
# is a much stronger claim than "this call is fine", and it is meant to be.
SEAMS = (
    # The prepare's own resolve/bind path for a run's declared texture.
    "ndsRendererHardwareResolveResidentTexture",
    "ndsRendererHardwareResolveOrBindTexture",
    "ndsRendererHardwareBindTexture",
    # The untextured half of the same seam: BeginDirectBatch calls it for a run
    # whose policy says untextured, and it clears the active entry as part of
    # doing so. Same machinery, opposite branch.
    "ndsRendererHardwareBindNoTexture",
    # The run-texture memo: records the params of a bind the seam above already
    # performed, and replays them. It IS the tracker, not a caller past it.
    "ndsRendererR2RunTextureMemoApply",
    "ndsRendererR2RunTextureMemoFill",
)

# name -> the one symbol it may reach, for cases a seam cannot express.
EXEMPT = {}

ATTRIBUTE = re.compile(r"__attribute__\s*\(\(.*?\)\)", re.S)
SIGNATURE_STOP = re.compile(r"(;|\}|\*/)\s*$")
CALL = re.compile(r"\b([A-Za-z_][A-Za-z_0-9]*)\s*\(")


def read_sources(root):
    out = []
    for relative in SOURCES:
        path = os.path.join(root, relative)
        if not os.path.isfile(path):
            return None, "%s not found (run from the repo root)" % relative
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            out.append((relative, handle.read().splitlines()))
    return out, None


def signature_name(lines, brace_index):
    parts = []
    index = brace_index
    while index >= 0 and len(parts) < 12:
        stripped = lines[index].strip()
        if index != brace_index:
            if (not stripped or stripped.startswith("#")
                    or stripped.startswith("*") or stripped.startswith("//")
                    or SIGNATURE_STOP.search(stripped)):
                break
        parts.append(stripped)
        index -= 1
    text = " ".join(reversed(parts))
    head = ATTRIBUTE.sub(" ", text.split("{")[0])
    match = re.search(r"([A-Za-z_][A-Za-z_0-9]*)\s*\(", head)
    return (match.group(1) if match else "<anonymous>"), text


def split_functions(lines):
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
            name, signature = signature_name(lines, start)
            functions.append((name, signature, start + 1,
                              lines[start:index + 1]))
            start = None
    return functions


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def mentions(text, symbol):
    """A trailing '=' in the symbol means "assignment only", not any mention.

    Reading a cache pointer is observation; assigning it is bookkeeping. The
    first cut of this check could not tell those apart and failed on the
    packet recorder, which only reads.
    """
    if symbol.endswith("="):
        pattern = (r"\b" + re.escape(symbol[:-1])
                   + r"\s*=(?!=)")
    else:
        pattern = r"\b" + re.escape(symbol) + r"\b"
    return re.search(pattern, text) is not None


def main(argv):
    verbose = "-v" in argv
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sources, error = read_sources(root)
    if error:
        print("FAIL:", error)
        return 1

    bodies = {}
    entries = []
    for relative, lines in sources:
        for name, signature, first_line, body in split_functions(lines):
            bodies[name] = strip_comments("\n".join(body))
            if FIGHTER_CODE in signature:
                entries.append((name, relative, first_line))

    def reach(entry):
        seen = {entry}
        queue = [(entry, [entry], 0)]
        while queue:
            name, path, hops = queue.pop(0)
            text = bodies.get(name)
            if text is None:
                continue
            for symbol, why in FORBIDDEN:
                if mentions(text, symbol) and EXEMPT.get(name) != symbol:
                    return symbol, why, path
            if hops >= MAX_HOPS:
                continue
            for callee in CALL.findall(text):
                # A seam terminates the walk: its interior is the texture
                # cache's own business, and reaching it is the sanctioned way.
                if callee in SEAMS:
                    continue
                if callee in bodies and callee not in seen:
                    seen.add(callee)
                    queue.append((callee, path + [callee], hops + 1))
        return None

    failures = []
    for name, relative, first_line in entries:
        hit = reach(name)
        if verbose:
            print("fighter-code %s (%s:%d): %s"
                  % (name, relative, first_line,
                     "clean" if hit is None else "SUSPECT"))
        if hit is None:
            continue
        symbol, why, path = hit
        failures.append(
            "%s (%s:%d) is %s and reaches %s via %s -- %s. Bring the texture "
            "in through the prepare's own seam, or create it at scene setup "
            "beside the impact-wave tiles; not from inside the fighter walk."
            % (name, relative, first_line, FIGHTER_CODE, symbol,
               " -> ".join(path), why))

    if not entries:
        failures.append(
            "no %s functions found across the unity TU -- the marker was "
            "renamed and this check now watches nothing" % FIGHTER_CODE)

    if failures:
        for failure in failures:
            print("FAIL:", failure)
        print("%d failure(s)" % len(failures))
        return 1
    print("check-fighter-draw-global-texture-state: OK -- %d fighter-code "
          "entries over %d functions, none reaches global texture state"
          % (len(entries), len(bodies)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
