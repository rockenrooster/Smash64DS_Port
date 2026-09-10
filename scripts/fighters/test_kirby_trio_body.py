"""Kirby trio-body regression: faithful bake, live joint-6 key, no drift.

Proves the owned KIRBY_TRIO_* seam in the LIVE generator/checker (never the
scratch oracle): for each detail x reachable head (mp1/mp14, 228 motions),
the [canon0, head, body@binding2, canon3..6] program passes all six live
geometry closures, its body verts equal the independent source call path,
the old appendix position fails, head1 vs head14 bakes differ (key needed),
unknown heads raise, and the canonical kirby program is untouched.
"""
import contextlib
import io
import os
import unittest
from pathlib import Path

import check_native_owner_geometry_closure as closure
import generate_nds_native_owners as live

REACHABLE_HEADS = (1, 14)


def _reference_body_xyz(detail, head_mp):
    """Independent source call path: raw payload walk, true draw order."""
    payload = live.load_o2r_payload(closure.REPO, "kirby")
    canon = live.unpack_many(
        "<IHHHBBBB2x",
        live.build_p2_owner_source_export(
            closure.REPO, "kirby", detail)["kirby_roots"])
    slots = [None] * live.VERTEX_CACHE_SIZE
    geometry = [0x00020000]
    closure.walk_root_cache(payload, canon[0][0], slots, geometry)
    closure.walk_root_cache(
        payload, live.kirby_trio_head_offset(detail, head_mp), slots, geometry)
    body = closure.walk_root_cache(
        payload, live.KIRBY_TRIO_BODY_MP0, slots, geometry)
    out = []
    for tri in body:
        for (src, s_ov, t_ov, _lit) in tri:
            x, y, z, s, t, _rgba = live.decode_source_vertex(payload, src)
            if s_ov is not None:
                s, t = s_ov, t_ov
            out.append((x, y, z, s, t))
    return out


def _program_body_xyz(program, ordinal):
    dense = program["dense_vertices"]
    corners = program["packed_corners"]
    first_corner = program["run_first_corner"]
    root = program["roots"][ordinal]
    out = []
    for epoch_index in range(root[1], root[1] + root[4]):
        epoch = program["epochs"][epoch_index]
        for run_index in range(epoch[3], epoch[3] + epoch[9]):
            base = first_corner[run_index]
            for k in range(program["runs"][run_index][1] * 3):
                record = dense[corners[base + k] & closure.DENSE_ID_MASK]
                out.append((record[0], record[1], record[2],
                            record[3], record[4]))
    return out


def _quiet(call, *args):
    with contextlib.redirect_stdout(io.StringIO()):
        return call(*args)


class KirbyTrioBodyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # The seam under test is the LIVE generator/checker, not scratch.
        assert "kirby-trio" not in live.__file__.replace("\\", "/")
        assert "kirby-trio" not in closure.__file__.replace("\\", "/")
        cls.programs = {
            (detail, head): closure.kirby_trio_context_program(detail, head)
            for detail in closure.DETAILS for head in REACHABLE_HEADS
        }

    def test_specs_replace_in_place_at_source_bindings(self):
        for (detail, head), program in self.programs.items():
            with self.subTest(detail=detail, head=head):
                self.assertEqual(len(program["roots"]), 7)
                self.assertEqual(program["root_bindings"], [0, 1, 2, 3, 4, 5, 6])
                self.assertEqual(program["body_ordinal"], 2)
                self.assertEqual(program["roots"][2][0], 0x40A0)
                self.assertEqual(
                    program["roots"][1][0],
                    live.kirby_trio_head_offset(detail, head))

    def test_all_live_closures_pass(self):
        for (detail, head), program in self.programs.items():
            with self.subTest(detail=detail, head=head):
                offsets = closure.kirby_trio_source_offsets(detail, head)
                self.assertEqual(
                    closure.kirby_trio_variant_source_closure(
                        detail, program, offsets), [])
                self.assertEqual(
                    _quiet(closure.vertex_closure, "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.matrix_routing_closure,
                           "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.facing_closure, "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.winding_closure, "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.primitive_closure,
                           "kirby", detail, program), [])

    def test_faithful_body_matches_source_call_path(self):
        for (detail, head), program in self.programs.items():
            with self.subTest(detail=detail, head=head):
                self.assertEqual(
                    _program_body_xyz(program, 2),
                    _reference_body_xyz(detail, head))

    def test_old_appendix_position_fails(self):
        for detail in closure.DETAILS:
            canon = live.unpack_many(
                "<IHHHBBBB2x",
                live.build_p2_owner_source_export(
                    closure.REPO, "kirby", detail)["kirby_roots"])
            specs = (tuple((root[0], binding)
                           for binding, root in enumerate(canon)) +
                     ((live.KIRBY_TRIO_BODY_MP0, 2),))
            with self.subTest(detail=detail):
                # Negative control: the old body-after-canon6 position must
                # not reach tables (it reads binding-6 verts, which own no GX
                # palette slot), while the faithful bake routes cleanly.
                canon_roots = live.unpack_many(
                    "<IHHHBBBB2x",
                    live.build_p2_owner_source_export(
                        closure.REPO, "kirby", detail)["kirby_roots"])
                with self.assertRaises(ValueError):
                    live._bake_kirby_specs_program(
                        closure.REPO, detail, specs, canon_roots)

    def test_reachable_face_heads_preserve_body_geometry(self):
        for detail in closure.DETAILS:
            with self.subTest(detail=detail):
                self.assertEqual(
                    _reference_body_xyz(detail, 1),
                    _reference_body_xyz(detail, 14))

    def test_unknown_heads_reject(self):
        for bad in (0, 2, 3, 13, 15, -1):
            with self.subTest(head=bad):
                with self.assertRaises(ValueError):
                    live.build_kirby_trio_faithful_specs(
                        [(0,)] * 7, "high", bad)
                with self.assertRaises(ValueError):
                    live.build_kirby_trio_context_program(
                        closure.REPO, "high", bad)

    def test_canonical_kirby_program_untouched(self):
        for detail in closure.DETAILS:
            with self.subTest(detail=detail):
                program = closure.owner_program("kirby", detail)
                self.assertEqual(program["canonical_root_count"], 7)
                self.assertEqual(
                    _quiet(closure.source_closure, "kirby", detail, program),
                    [])
                # Census pins hold: the generator edit moved no canonical byte.
                live.build_p2_owner_runtime_context(
                    closure.REPO, "kirby", detail)

    def test_variant_schema_names_the_main_seam(self):
        schema = live.kirby_trio_variant_schema()
        self.assertEqual(schema["body_offset"], 0x40A0)
        self.assertEqual(schema["body_binding"], 2)
        self.assertEqual(
            {(row["detail"], row["head_mp"]) for row in schema["contexts"]},
            {(detail, head) for detail in closure.DETAILS
             for head in REACHABLE_HEADS})
        self.assertEqual(
            schema["present_macro"], "NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT")


def _shipped_body_xyz(program):
    dense = program["dense_vertices"]
    corners = program["packed_corners"]
    first_corner = program["run_first_corner"]
    root = program["roots"][2]
    out = []
    for epoch_index in range(root[1], root[1] + root[4]):
        epoch = program["epochs"][epoch_index]
        for run_index in range(epoch[3], epoch[3] + epoch[9]):
            base = first_corner[run_index]
            for k in range(program["runs"][run_index][1] * 3):
                record = dense[corners[base + k] & closure.DENSE_ID_MASK]
                out.append((record[0], record[1], record[2],
                            record[3], record[4]))
    return out


class KirbyTrioShippedTests(unittest.TestCase):
    """The SHIPPED tables (grown kirby context + per-head body section).

    Every test below reads the real emitted context -- the same arrays
    render_p2_owner_runtime_program emits and the image payload carries --
    assembled into the live 7-root draw order, never the scratch bake.
    """

    @classmethod
    def setUpClass(cls):
        assert "kirby-trio" not in live.__file__.replace("\\", "/")
        assert "kirby-trio" not in closure.__file__.replace("\\", "/")
        cls.shipped = {
            (detail, head): closure.kirby_trio_shipped_program(detail, head)
            for detail in closure.DETAILS for head in REACHABLE_HEADS
        }
        cls.faithful = {
            (detail, head): closure.kirby_trio_context_program(detail, head)
            for detail in closure.DETAILS for head in REACHABLE_HEADS
        }

    def test_shipped_roots_replace_in_place_at_source_bindings(self):
        for (detail, head), program in self.shipped.items():
            with self.subTest(detail=detail, head=head):
                self.assertEqual(len(program["roots"]), 7)
                self.assertEqual(program["root_bindings"], [0, 1, 2, 3, 4, 5, 6])
                self.assertEqual(program["roots"][2][0], 0x40A0)
                self.assertEqual(
                    program["roots"][1][0],
                    live.kirby_trio_head_offset(detail, head))

    def test_shipped_all_live_closures_pass(self):
        for (detail, head), program in self.shipped.items():
            with self.subTest(detail=detail, head=head):
                offsets = closure.kirby_trio_source_offsets(detail, head)
                self.assertEqual(
                    closure.kirby_trio_variant_source_closure(
                        detail, program, offsets), [])
                self.assertEqual(
                    _quiet(closure.vertex_closure, "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.matrix_routing_closure,
                           "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.facing_closure, "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.winding_closure, "kirby", detail, program), [])
                self.assertEqual(
                    _quiet(closure.primitive_closure,
                           "kirby", detail, program), [])

    def test_shipped_body_matches_source_call_path(self):
        for (detail, head), program in self.shipped.items():
            with self.subTest(detail=detail, head=head):
                self.assertEqual(
                    _shipped_body_xyz(program),
                    _reference_body_xyz(detail, head))

    def test_shipped_body_matches_faithful_bake(self):
        # The append remap is pure offset arithmetic: shipped corners must
        # equal the faithful bake corner for corner (positions AND normals).
        for key in self.shipped:
            with self.subTest(detail=key[0], head=key[1]):
                self.assertEqual(
                    _shipped_body_xyz(self.shipped[key]),
                    _program_body_xyz(self.faithful[key], 2))

    def test_shipped_reachable_face_heads_preserve_body_geometry(self):
        for detail in closure.DETAILS:
            with self.subTest(detail=detail):
                self.assertEqual(
                    _shipped_body_xyz(self.shipped[(detail, 1)]),
                    _shipped_body_xyz(self.shipped[(detail, 14)]))

    def test_unknown_heads_reject_shipped(self):
        for bad in (0, 2, 3, 13, 15, -1):
            with self.subTest(head=bad):
                with self.assertRaises(ValueError):
                    closure.kirby_trio_shipped_program("high", bad)

    def test_append_is_purely_additive(self):
        # trio sections only ever grow kirby tables at the tail: with the
        # flag off every array (and both primitive modes) is an exact
        # prefix of the shipped one, and all other owners ignore the flag.
        for detail in closure.DETAILS:
            with self.subTest(detail=detail):
                off = live.build_p2_owner_runtime_context(
                    closure.REPO, "kirby", detail, kirby_trio=False)
                on = live.build_p2_owner_runtime_context(
                    closure.REPO, "kirby", detail, kirby_trio=True)
                self.assertNotIn("kirby_trio_bodies", off)
                for key in ("state", "sequence", "vertex", "triangles",
                            "runs", "epochs", "dense_vertices",
                            "dense_color_sources", "packed_corners",
                            "run_first_corner", "run_first_unique",
                            "run_unique_count", "run_unique_dense",
                            "action_dense_spans", "direct_epoch_policies",
                            "gx_positions"):
                    self.assertEqual(
                        list(on[key])[:len(list(off[key]))], list(off[key]),
                        f"kirby {detail} {key} moved existing bytes")
                self.assertEqual(off["roots"], on["roots"])
                self.assertEqual(off["root_bindings"], on["root_bindings"])
                self.assertEqual(
                    off["light_preamble_indices"],
                    on["light_preamble_indices"])
                for mode in (1, 2):
                    for old, new in zip(off["primitive_streams"][mode],
                                        on["primitive_streams"][mode]):
                        self.assertEqual(
                            list(new)[:len(list(old))], list(old))
        for owner in ("luigi", "donkey", "link", "samus"):
            with self.subTest(owner=owner):
                for detail in closure.DETAILS:
                    off = live.build_p2_owner_runtime_context(
                        closure.REPO, owner, detail, kirby_trio=False)
                    on = live.build_p2_owner_runtime_context(
                        closure.REPO, owner, detail, kirby_trio=True)
                    self.assertEqual(off, on)

    def test_full14_variants_untouched(self):
        # The landed head/face appendix keeps its rows: the first 16
        # variant specs and their baked roots are identical on/off, and the
        # emitted variant blocks match line for line.
        for detail in closure.DETAILS:
            with self.subTest(detail=detail):
                off = live.build_p2_owner_runtime_context(
                    closure.REPO, "kirby", detail, kirby_trio=False)
                on = live.build_p2_owner_runtime_context(
                    closure.REPO, "kirby", detail, kirby_trio=True)
                self.assertEqual(off["variant_specs"], on["variant_specs"])
                self.assertEqual(off["roots"], on["roots"])
                old_lines = live.render_p2_owner_runtime_program(off)
                new_lines = live.render_p2_owner_runtime_program(on)

                def _block(lines, symbol):
                    start = next(
                        i for i, ln in enumerate(lines) if symbol in ln)
                    end = next(
                        i for i in range(start, len(lines))
                        if lines[i].strip() == "};")
                    return lines[start:end + 1]

                suffix = "Low" if detail == "low" else ""
                self.assertEqual(
                    _block(old_lines,
                           f"sNdsNativeKirbyRootVariants{suffix}"),
                    _block(new_lines,
                           f"sNdsNativeKirbyRootVariants{suffix}"))
                # Activation follows the variant pattern: the high render
                # defines the macro once for both details, so stale incs
                # fail closed instead of failing to link.
                if detail == "high":
                    self.assertIn(
                        "NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT",
                        "\n".join(new_lines))
                else:
                    self.assertNotIn(
                        "NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT",
                        "\n".join(new_lines))
                self.assertNotIn(
                    "NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT",
                    "\n".join(old_lines))

    def test_emission_carries_trio_roots(self):
        import generate_nds_native_owner_images as images
        for detail in closure.DETAILS:
            with self.subTest(detail=detail):
                context = live.build_p2_owner_runtime_context(
                    closure.REPO, "kirby", detail, kirby_trio=True)
                lines = live.render_p2_owner_runtime_program(context)
                text = "\n".join(lines)
                suffix = "Low" if detail == "low" else ""
                for head in REACHABLE_HEADS:
                    symbol = (f"sNdsNativeKirbyTrioBodyRootHead{head}{suffix}")
                    self.assertIn(symbol, text)
                # Both resident roots name the reachable body at binding 2.
                self.assertEqual(text.count("0x000040a0u"), 2)
                # The image payload grows (stale exact-size reads reject),
                # and every other owner's member set is byte-identical.
                for owner in ("luigi", "donkey"):
                    for det in ("high", "low"):
                        a = live.build_p2_owner_runtime_context(
                            closure.REPO, owner, det, kirby_trio=False)
                        b = live.build_p2_owner_runtime_context(
                            closure.REPO, owner, det, kirby_trio=True)
                        ma = images.render_image(owner, det, a)
                        mb = images.render_image(owner, det, b)
                        self.assertEqual(ma, mb)
                grown = images.render_image("kirby", detail, context)
                plain = images.render_image(
                    "kirby", detail, live.build_p2_owner_runtime_context(
                        closure.REPO, "kirby", detail, kirby_trio=False))
                self.assertGreater(len(grown), len(plain))
                self.assertIn(".abi_tag", grown.split(".state_deltas")[0])


class KirbyTrioResolveTests(unittest.TestCase):
    """The RUNTIME resolve path, host-compiled from the shipped C file.

    Extracts the real ndsRendererNativeKirbyTrioBodyRoot + head-key setter
    from src/nds/nds_renderer_assets.c and runs the head selection live
    (no make, ROM, or emulator; temp dir only). A second test pins the
    ResolveRoot trio branch to the same symbols so the validator and the
    production preflight both route through the keyed resolve.
    """

    @classmethod
    def setUpClass(cls):
        here = Path(__file__).resolve().parent
        repo = here
        while not (repo / "src" / "nds" / "nds_renderer_assets.c").is_file():
            repo = repo.parent
        cls.repo = repo
        src = (repo / "src" / "nds" / "nds_renderer_assets.c").read_text()
        key = ("static const NDSNativeRoot "
               "*ndsRendererNativeKirbyTrioBodyRoot(")
        i = src.find(key)
        assert i >= 0, "trio resolve function missing from runtime"
        brace = src.find("{", i)
        depth = 0
        for j in range(brace, len(src)):
            if src[j] == "{":
                depth += 1
            elif src[j] == "}":
                depth -= 1
                if depth == 0:
                    cls.fn_text = src[i:j + 1]
                    break
        else:
            raise AssertionError("unbalanced braces in trio resolve")
        assert "sNdsNativeKirbyTrioBodyRootHead1" in cls.fn_text
        assert "sNdsNativeKirbyTrioBodyRootHead14" in cls.fn_text
        # The adapter publishes through this setter every kirby draw.
        assert ("void ndsRendererNativeKirbyTrioSetHeadKey(u32 head_mp)"
                in src)
        adapter = (repo / "src" / "port" /
                   "renderer_adapter_fighter.c").read_text()
        assert "ndsRendererNativeKirbyTrioSetHeadKey(" in adapter
        assert "ndsFighterKirbyTrioHeadKey(fp, &kirby_trio_head)" in adapter
        assert "ndsFighterKirbyTrioBodyActive(fp)" in adapter
        # The shared resolve routes binding-2/0x40A0 through the keyed
        # function BEFORE the generic variant loop, for validator and
        # preflight alike.
        resolve = src[src.find(
            "static const NDSNativeRoot *ndsRendererNativeFighterResolveRoot("):]
        assert "sNdsKirbyTrioHeadMp" in resolve
        assert "ndsRendererNativeKirbyTrioBodyRoot(" in resolve
        assert "0x40A0u" in resolve
        trio_branch = resolve.find("NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT")
        variant_loop = resolve.find("i < variant_count")
        assert 0 <= trio_branch < variant_loop

    def test_head_key_selects_live_bake(self):
        import shutil
        import subprocess
        import tempfile
        harness = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT 1
typedef struct NDSNativeRoot {
    u32 root_offset; u16 first_epoch; u16 tail_state_first;
    u16 source_command_count; u8 epoch_count; u8 tail_state_count;
    u8 tail_sync_count; u8 light_preamble;
} NDSNativeRoot;
static const NDSNativeRoot sNdsNativeKirbyTrioBodyRootHead1 =
    { 0x40A0u, 107u, 0xFFFFu, 50u, 4u, 0u, 0u, 0u };
static const NDSNativeRoot sNdsNativeKirbyTrioBodyRootHead14 =
    { 0x40A0u, 111u, 0xFFFFu, 50u, 4u, 0u, 0u, 0u };
static const NDSNativeRoot sNdsNativeKirbyTrioBodyRootHead1Low =
    { 0x40A0u, 96u, 0xFFFFu, 50u, 4u, 0u, 0u, 0u };
static const NDSNativeRoot sNdsNativeKirbyTrioBodyRootHead14Low =
    { 0x40A0u, 100u, 0xFFFFu, 50u, 4u, 0u, 0u, 0u };
__FN__
static int fails = 0;
#define CHECK(name_, cond_) do { \
    printf("CASE %s: %s\n", name_, (cond_) ? "PASS" : "FAIL"); \
    if (!(cond_)) fails++; } while (0)
int main(void) {
    const NDSNativeRoot *r;
    r = ndsRendererNativeKirbyTrioBodyRoot(0u, 1u);
    CHECK("high_head1", r == &sNdsNativeKirbyTrioBodyRootHead1);
    r = ndsRendererNativeKirbyTrioBodyRoot(0u, 14u);
    CHECK("high_head14", r == &sNdsNativeKirbyTrioBodyRootHead14);
    r = ndsRendererNativeKirbyTrioBodyRoot(1u, 1u);
    CHECK("low_head1", r == &sNdsNativeKirbyTrioBodyRootHead1Low);
    r = ndsRendererNativeKirbyTrioBodyRoot(1u, 14u);
    CHECK("low_head14", r == &sNdsNativeKirbyTrioBodyRootHead14Low);
    CHECK("unknown_zero", ndsRendererNativeKirbyTrioBodyRoot(0u, 0u) == 0);
    CHECK("unknown_two", ndsRendererNativeKirbyTrioBodyRoot(0u, 2u) == 0);
    CHECK("unknown_13", ndsRendererNativeKirbyTrioBodyRoot(1u, 13u) == 0);
    CHECK("unknown_15", ndsRendererNativeKirbyTrioBodyRoot(0u, 15u) == 0);
    printf("DONE fails=%d\n", fails);
    return fails ? 1 : 0;
}
""".replace("__FN__", self.fn_text)
        tmp = Path(tempfile.mkdtemp(prefix="kirby_trio_resolve_"))
        try:
            c_path = tmp / "harness.c"
            c_path.write_text(harness, encoding="utf-8")
            exe = tmp / ("harness.exe" if os.name == "nt" else "harness")
            cc = (shutil.which("gcc") or shutil.which("cc") or
                  shutil.which("clang"))
            assert cc, "no host C compiler found"
            p = subprocess.run(
                [cc, "-std=c99", "-O1", "-w", str(c_path), "-o", str(exe)],
                capture_output=True, text=True, timeout=120)
            assert p.returncode == 0, \
                f"host compile failed: {(p.stderr or '')[:2000]}"
            q = subprocess.run([str(exe)], capture_output=True, text=True,
                               timeout=60)
            out = q.stdout or ""
            assert q.returncode == 0, f"host harness failed:\n{out[:2000]}"
            assert "DONE fails=0" in out, \
                f"harness reports failures:\n{out[:2000]}"
            for case in ("high_head1", "high_head14", "low_head1",
                         "low_head14", "unknown_zero", "unknown_two",
                         "unknown_13", "unknown_15"):
                self.assertIn(f"CASE {case}: PASS", out)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
