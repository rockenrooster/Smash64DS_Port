---
name: smash64ds-opus-guardrails
description: Load and enforce the Opus-specific rails for this repo (CLAUDE.OPUS.md + AGENTS.OPUS.md). Use at the start of any Opus session, when authoring a brief for an Opus subagent, or the moment an Opus agent drifts into unverified claims, scope creep, or budget overruns.
---

# smash64ds-opus-guardrails

Use when an Opus model operates in this repository — at session start, when
writing a brief for an Opus subagent, or the moment one of the failure modes
in `AGENTS.OPUS.md` appears in live work.

## On invocation

1. Read `CLAUDE.OPUS.md` (the rails) and `AGENTS.OPUS.md` (the cycle loop) at
   the repo root, then `CLAUDE.md`/`AGENTS.md` if not already loaded. The
   overlays add constraints; the originals own the truth and always win a
   conflict.
2. State the cycle objective in one line before any tool call.
3. Run the pre-flight checklist. Do not start work that fails it.

## Pre-flight checklist

- [ ] Objective is one bounded step with a verifiable finish inside this cycle.
- [ ] The discriminating measurement is identified and scheduled FIRST.
- [ ] Root-ROM hashes recorded if anything will build.
- [ ] The owner's dirty tree noted; commit plan is explicit-paths-only.

## Mid-cycle tripwires — stop and re-read the rails before you

- claim an absence from a single grep, or any conclusion whose only evidence
  is that you did not find something;
- validate a pointer through itself, or headline a number whose window or
  pairing you have not stated;
- start an edit whose build + run + verify chain no longer fits your context;
- touch a file outside the row you own, anything in `decomp/`, or the owner's
  dirty working-tree state;
- pass `-j`, touch `MAKEFLAGS`, build a published target for lab work, push,
  or run the snapshot;
- mark a visual row FIXED — the owner's eye is the gate; rows ask.

## Pre-commit checklist

- [ ] Engagement proof AND negative control both present, with numbers.
- [ ] Explicit paths; every blob name-scanned; trailer present.
- [ ] Packet states what was NOT done and what the next cycle inherits.
- [ ] Root-ROM hashes re-verified if anything built.

## When reining in a live agent

Quote the specific rail broken, require the missing evidence or the clean
stop, and do not accept a re-assertion in place of a measurement. Apply the
meta-rule: a documented lesson gets broken twice — when a trap repeats, fix
the helper so the wrong form is inexpressible, then document it.
