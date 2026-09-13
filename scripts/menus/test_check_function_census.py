#!/usr/bin/env python3
"""Unit tests for check_function_census.py: the comment-and-string stripper
and the definition/call classifier on synthetic bodies."""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import check_function_census as census  # noqa: E402


def test_stripper_hides_comments_and_strings():
    body = (
        '// mpFooHiddenOne(\n'
        '/* ftFooHiddenTwo(\n'
        '   still comment */\n'
        'void mpFooReal(void)\n'
        '{\n'
        '    const char *s = "mpFooHiddenThree("; /* ftFooHiddenFour( */\n'
        '    char c = \'(\';\n'
        '    mpFooCallee();\n'
        '}\n'
    )
    stripped = census.strip_comments_strings(body)
    assert len(stripped) == len(body)
    assert stripped.count("\n") == body.count("\n")
    defined, called, _ = census.scan_body(stripped)
    assert "mpFooReal" in defined
    assert "mpFooCallee" in called
    for hidden in ("mpFooHiddenOne", "ftFooHiddenTwo", "mpFooHiddenThree",
                   "ftFooHiddenFour"):
        assert hidden not in defined
        assert hidden not in called


def test_synthetic_undefined_call():
    defs = "void mpFooDefined(GObj *g)\n{\n}\n"
    calls = ("void mpFooCaller(void)\n"
             "{\n"
             "    mpFooDefined();\n"
             "    mpFooMissing();\n"
             "}\n")
    d_defined, _, _ = census.scan_body(census.strip_comments_strings(defs))
    _, d_called, _ = census.scan_body(census.strip_comments_strings(calls))
    assert "mpFooDefined" in d_defined
    assert "mpFooDefined" in d_called
    assert "mpFooMissing" in d_called
    assert "mpFooMissing" not in d_defined


def test_prototypes_and_pointer_assignments_are_neither():
    body = ("void mpFooProto(int x);\n"
            "void mpFooAssign(void)\n"
            "{\n"
            "    void (*fn)(void) = mpFooProto;\n"
            "    (void)fn;\n"
            "}\n")
    defined, called, _ = census.scan_body(
        census.strip_comments_strings(body))
    assert "mpFooProto" not in defined
    assert "mpFooProto" not in called
    assert "mpFooAssign" in defined


def test_multiline_definition_attribute_and_trailing_comment():
    body = ("s32\n"
            "mpFooMultiline(s32 a,\n"
            "               s32 b)\n"
            "{\n"
            "    return a;\n"
            "}\n"
            "void __attribute__((noinline))\n"
            "mpFooAttr(DObj *joint)\n"
            "{\n"
            "}\n"
            "void mpFooTrail(GObj *g) // trailing comment\n"
            "{\n"
            "}\n")
    defined, called, _ = census.scan_body(
        census.strip_comments_strings(body))
    assert "mpFooMultiline" in defined
    assert "mpFooAttr" in defined
    assert "mpFooTrail" in defined
    assert "mpFooMultiline" not in called


def test_definition_generating_macro_invocation():
    body = ("#define NDS_SCENE_STUB(name) void name(void) { ndsSceneBoundary(); }\n"
            "NDS_SCENE_STUB(mpFooStubbed)\n"
            "void mpFooUser(void)\n"
            "{\n"
            "    mpFooStubbed();\n"
            "}\n")
    stripped = census.strip_comments_strings(body)
    generators = census.generator_macros(stripped)
    assert generators == {"NDS_SCENE_STUB": [0]}
    defined, called, _ = census.scan_body(stripped)
    defined |= census.generated_definitions(stripped, generators)
    assert "mpFooStubbed" in defined
    assert "mpFooStubbed" in called
    assert "mpFooUser" in defined
