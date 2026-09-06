#!/usr/bin/env python3
"""Controls for audit_mn_screen_coverage.py's imported-screen arm.

The arm claims three separate things about every 1P/modes screen -- its
sprites' container is STAGED, the port ROWS the symbol, and the (asset,
offset) pair is DRAWABLE through the Sprite geometry manifest.  A checker that
can only ever say PASS is worthless, so each of those links is exercised here
against a state where it is broken, and the whole gate is exercised with its
allowlist taken away.  `test_tree_is_green` is the positive arm; every other
test is a control that must be able to fail.
"""
from __future__ import annotations

import dataclasses
import importlib.util
import sys
from pathlib import Path

import pytest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent

_spec = importlib.util.spec_from_file_location(
    "mn_screen_coverage_audit", HERE / "audit_mn_screen_coverage.py")
audit = importlib.util.module_from_spec(_spec)
sys.modules["mn_screen_coverage_audit"] = audit
_spec.loader.exec_module(audit)


@pytest.fixture(scope="module")
def reloc():
    return audit.scan_port_reloc(ROOT)


def test_tree_is_green():
    """The shipped tree explains every delta it has, on both screen classes."""
    deltas, allow, report = audit.run_audit(ROOT)
    unexplained = [d for d in deltas if d.allowed is None]
    stale = [e for e in allow if not e.used]
    assert not unexplained, unexplained[:5]
    assert not stale, stale[:5]
    for screen in audit.IMPORTED_SCREENS:
        assert screen.key in report["screens"]


def test_every_imported_screen_draws_something(reloc):
    """A screen whose whole inventory is a delta usually means the audit lost
    the source, not that the screen is empty.  `ending` is the one legitimate
    zero: mvending.c constructs no Sprite at all."""
    for screen in audit.IMPORTED_SCREENS:
        _deltas, info = audit.audit_imported_screen(ROOT, screen, reloc)
        if screen.key == "ending":
            assert info["source_sprites_live"] == 0
            continue
        assert info["source_sprites_live"] > 0, screen.key


def test_missing_geometry_row_is_reported_as_nodraw(reloc):
    """CONTROL for the DRAWABLE link.  `characters` is fully drawable today;
    empty the geometry manifest and every one of its sprites must turn into a
    NODRAW naming the row that would fix it."""
    screen = next(s for s in audit.IMPORTED_SCREENS if s.key == "characters")
    before, info = audit.audit_imported_screen(ROOT, screen, reloc)
    assert before == []
    assert info["source_sprites_live"] > 0

    broken = dataclasses.replace(reloc, geometry=set())
    after, _info = audit.audit_imported_screen(ROOT, screen, broken)
    assert len(after) == info["source_sprites_live"]
    assert {d.kind for d in after} == {"NODRAW"}
    assert "sNdsBattleInterfaceSpriteDescs" in after[0].detail


def test_unstaged_container_is_reported_as_missing(reloc):
    """CONTROL for the STAGED link, and for the report being actionable: the
    detail must name the Makefile row and the NitroFS path row."""
    screen = next(s for s in audit.IMPORTED_SCREENS if s.key == "characters")
    broken = dataclasses.replace(reloc, nitrofs=set(), staged=set())
    deltas, _info = audit.audit_imported_screen(ROOT, screen, broken)
    assert deltas and {d.kind for d in deltas} == {"MISSING"}
    detail = deltas[0].detail
    assert "nitro:/reloc/" in detail
    assert "stage_reloc_file.py" in detail


def test_unrowed_symbol_is_reported_as_missing(reloc):
    """CONTROL for the ROWED link."""
    screen = next(s for s in audit.IMPORTED_SCREENS if s.key == "characters")
    broken = dataclasses.replace(reloc, offsets={})
    deltas, _info = audit.audit_imported_screen(ROOT, screen, broken)
    assert deltas and {d.kind for d in deltas} == {"MISSING"}
    assert "no offset row" in deltas[0].detail


def test_gate_is_red_without_its_allowlist(monkeypatch):
    """CONTROL for the gate itself.  The ruled shell-screen entries are load
    bearing: take the allowlist away and every delta must read unexplained
    and the run must go red, or the arm is reporting nothing.  The imported
    screens contribute zero here because their geometry rows landed (the
    sprite-geometry oracles pin that); a future NODRAW regression reappears
    as unexplained deltas and keeps this red."""
    monkeypatch.setattr(audit, "load_allowlist", lambda repo_root: [])
    deltas, _allow, _report = audit.run_audit(ROOT)
    unexplained = [d for d in deltas if d.allowed is None]
    assert len(unexplained) == len(deltas) > 40
    assert {d.screen for d in unexplained} & {s.key for s in audit.SCREENS}

    monkeypatch.setattr(sys, "argv", ["audit_mn_screen_coverage.py"])
    assert audit.main() == 1


def test_import_tu_rot_is_a_hard_error(reloc):
    """A renamed or deleted src/import wrapper must stop the audit, not empty
    a screen quietly."""
    ghost = audit.ImportedScreenSpec(
        "ghost", "Ghost screen", ("mn/mndata/mncharacters.c",),
        "battleship_this_tu_does_not_exist.c")
    with pytest.raises(audit.AuditError):
        audit.audit_imported_screen(ROOT, ghost, reloc)

    wrong = audit.ImportedScreenSpec(
        "wrong", "Wrong source", ("mn/mnmaps/mnmaps.c",),
        "battleship_mncharacters.c")
    with pytest.raises(audit.AuditError):
        audit.audit_imported_screen(ROOT, wrong, reloc)
