#!/usr/bin/env python3
"""Host checks for the typed EXTRA source seam; never run a build producer."""

from __future__ import annotations

import argparse
import copy
from dataclasses import replace
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys
import tempfile
import unittest

import generate_nds_native_owners as owners
import generate_nds_native_owner_images as images
import extra_resource_adapter as extra


REPO_ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR: Path | None = None
ASSET_ID = 60000
MODEL_ASSET_ID = 5456
OWNER_NAME = "extra_synthetic_provider"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def global_maps() -> dict:
    """Capture legacy declarations and the source layout cache independently."""
    return copy.deepcopy({
        name: value for name, value in vars(owners).items()
        if isinstance(value, dict) and (
            name.startswith(("P2_", "OWNER_", "DETAIL_")) or name in (
                "BASE_MODEL_PART_ROOT_VARIANTS", "SOURCE_EXPORT_HASHES",
                "LOW_SOURCE_EXPORT_HASHES", "_PAIR_LAYOUT_CACHE",
            )
        )
    })


def synthetic_resource(*, image=False, vertex_asset=ASSET_ID,
                       vertex_only=False):
    """A source-authored triangle and two control roots, backed by real Vtx bytes."""
    commands = [
        (0xD9FFFFFF, 0x00020000),  # Source LIGHTING geometry state.
        (0xFCFFFE05, 0xFF167DFF),  # Source untextured SHADE/ENV combiner.
    ]
    if image:
        commands.append((0xFD100000, 0))
    vertex_slot = len(commands) * 8 + 4
    commands.append((0x01003006, 0xFFFF0020))  # Three vertices at byte 0x80.
    if not vertex_only:
        commands.append((0x05000204, 0))  # Source triangle slots 0, 1, 2.
    commands.append((0xDF000000, 0))
    data = bytearray(0xB0)
    for index, (w0, w1) in enumerate(commands):
        struct.pack_into(">II", data, index * 8, w0, w1)
    struct.pack_into(">II", data, 0x40, 0xDF000000, 0)
    struct.pack_into(">IIII", data, 0x48, 0xD9FFFFFF, 0x00020000, 0xDF000000, 0)
    for index, xyz in enumerate(((0, 0, 0), (10, 0, 0), (0, 10, 0))):
        struct.pack_into(">hhhHhhBBBB", data, 0x80 + index * 16,
                         *xyz, 0, 0, 0, 0, 0, 127, 255)
    source = bytes(data)
    ref = owners.stage_manifest.PointerRef(vertex_asset, 0x80)
    internal = {vertex_slot: ref} if vertex_asset == ASSET_ID else {}
    external = {vertex_slot: ref} if vertex_asset != ASSET_ID else {}
    spec = owners.stage_manifest.InputSpec(
        "synthetic-extra.raw", digest(source), ASSET_ID,
        len(internal), len(external), digest(source),
    )
    return owners.stage_manifest.O2RResource(
        spec, source, source, ASSET_ID, internal, external,
    )


class TypedProviderTests(unittest.TestCase):
    def setUp(self) -> None:
        self.before = global_maps()

    def tearDown(self) -> None:
        self.assertEqual(global_maps(), self.before, "typed sources changed legacy P2 globals")

    def compile(self, resource=None, roots=(0,), bindings=(7,), detail="high") -> dict:
        resource = synthetic_resource() if resource is None else resource
        return owners.build_p2_root_set_runtime_context(
            REPO_ROOT, OWNER_NAME, detail, roots,
            source_provider=owners.NativeOwnerSource(resource),
            root_bindings=bindings,
        )

    def test_unknown_owner_compiles_source_vertices_without_global_registration(self) -> None:
        self.assertNotIn(OWNER_NAME, owners.P2_O2R_ASSETS)
        context = self.compile()
        self.assertEqual(context["owner_name"], OWNER_NAME)
        self.assertEqual(context["asset_data_size"], 0xB0)
        self.assertEqual(context["root_bindings"], [7])
        self.assertEqual(len(context["triangles"]), 1)
        self.assertEqual(len(context["dense_corners"]), 3)
        self.assertEqual(len(context["dense_vertices"]), 3)
        self.assertEqual({row[:3] for row in context["dense_vertices"]},
                         {(0, 0, 0), (10, 0, 0), (0, 10, 0)})
        self.assertEqual({row[5] for row in context["dense_vertices"]}, {7})
        self.assertEqual(context["source_asset_id"], ASSET_ID)
        self.assertIsNone(context["topology"])

    def test_provider_compiles_both_details(self) -> None:
        high = self.compile(detail="high")
        low = self.compile(detail="low")
        self.assertEqual(high["triangles"], low["triangles"])
        self.assertEqual(high["dense_vertices"], low["dense_vertices"])

    def alpha_alias_resource(self, alpha: int, combine=(0xFC121805, 0xFF17FFFF)):
        resource = synthetic_resource()
        source = bytearray(resource.payload)
        struct.pack_into(">II", source, 8, *combine)
        source[0xAF] = alpha  # Last vertex is reached by the source triangle.
        source = bytes(source)
        return replace(
            resource, source=source, payload=source,
            spec=replace(resource.spec, sha256=digest(source), payload_sha256=digest(source)),
        )

    def test_source_alpha_alias_is_exact_for_opaque_vertices(self) -> None:
        for alpha in (0, 255):
            with self.subTest(alpha=alpha):
                context = self.compile(self.alpha_alias_resource(alpha))
                combine = [row[:2] for row in context["state"] if row[0] >> 24 == 0xFC]
                self.assertEqual(combine, [(0xFC127E05, 0xFF17F3FF)])
                self.assertEqual(len(context["triangles"]), 1)

    def test_source_alpha_alias_rejects_nonopaque_reached_vertex(self) -> None:
        with self.assertRaises(ValueError):
            self.compile(self.alpha_alias_resource(128))

    def test_skeleton_source_alpha_alias_requires_opaque_reached_vertices(self) -> None:
        combine = (0xFC327E05, 0xFF17F9FF)
        for alpha in (0, 255):
            with self.subTest(alpha=alpha):
                context = self.compile(self.alpha_alias_resource(alpha, combine))
                actual = [row[:2] for row in context["state"] if row[0] >> 24 == 0xFC]
                self.assertEqual(actual, [(0xFC327E05, 0xFF17FDFF)])
        with self.assertRaises(ValueError):
            self.compile(self.alpha_alias_resource(128, combine))

    def white_material_resource(self, family: int, corrupt_channel=None):
        resource = synthetic_resource()
        source = bytearray(resource.payload)
        combine = {6: (0xFC123245, 0x00400087), 7: (0xFC321803, 0xFF17FFFF)}[family]
        struct.pack_into(">II", source, 8, *combine)
        for vertex in range(3):
            source[0x80 + vertex * 16 + 12:0x80 + vertex * 16 + 16] = b"\xff" * 4
        if corrupt_channel is not None:
            source[0xA0 + 12 + corrupt_channel] = 254
        source = bytes(source)
        return replace(
            resource, source=source, payload=source,
            spec=replace(resource.spec, sha256=digest(source), payload_sha256=digest(source)),
        )

    def test_p4_material_families_preserve_exact_combine_on_white_vertices(self) -> None:
        for family, combine in ((6, (0xFC123245, 0x00400087)), (7, (0xFC321803, 0xFF17FFFF))):
            with self.subTest(family=family):
                context = self.compile(self.white_material_resource(family))
                actual = [row[:2] for row in context["state"] if row[0] >> 24 == 0xFC]
                self.assertEqual(actual, [combine])
                self.assertEqual(context["direct_epoch_policies"][0] & 7, family)

    def test_p4_material_families_reject_any_nonwhite_reached_channel(self) -> None:
        for family in (6, 7):
            for channel in range(4):
                with self.subTest(family=family, channel=channel), self.assertRaises(ValueError):
                    self.compile(self.white_material_resource(family, channel))

    def test_empty_display_list_is_a_valid_state_only_root(self) -> None:
        context = self.compile(roots=(0x40,), bindings=(0,))
        self.assertEqual(len(context["roots"]), 1)
        self.assertEqual(context["roots"][0][0], 0x40)
        self.assertEqual(context["triangles"], [])
        self.assertEqual(context["dense_vertices"], [])

    def test_control_only_root_preserves_geometry_state(self) -> None:
        context = self.compile(roots=(0x48,), bindings=(0,))
        self.assertEqual(context["triangles"], [])
        self.assertTrue(any(row[:2] == (0xD9FFFFFF, 0x00020000) for row in context["state"]))
        self.assertEqual(context["roots"][0][5], 1)

    def test_incomplete_light_prefix_fails(self) -> None:
        resource = synthetic_resource()
        source = bytearray(resource.payload)
        color = 0x34567800
        struct.pack_into(">IIIIII", source, 0x40,
                         0xDB0A0018, color, 0xDB0A001C, color, 0xDF000000, 0)
        source = bytes(source)
        resource = replace(
            resource, source=source, payload=source,
            spec=replace(resource.spec, sha256=digest(source), payload_sha256=digest(source)),
        )
        with self.assertRaises(ValueError):
            self.compile(resource, roots=(0x40,), bindings=(0,))

    def repeated_light_resource(self, *, cross_vertex=False):
        resource = synthetic_resource()
        colors = [(0x11223300, 0x22334400), (0x44556600, 0x55667700)]
        groups = [
            [(0xDB0A0000, diffuse), (0xDB0A0004, diffuse),
             (0xDB0A0018, ambient), (0xDB0A001C, ambient)]
            for diffuse, ambient in colors
        ]
        controls = [(0xD9FFFFFF, 0x00020000), (0xFCFFFE05, 0xFF167DFF)]
        vertex = (0x01003006, 0xFFFF0020)
        commands = (groups[0] + controls + [vertex] + groups[1] if cross_vertex else
                    groups[0] + groups[1] + controls + [vertex])
        vertex_slot = commands.index(vertex) * 8 + 4
        commands += [(0x05000204, 0), (0xDF000000, 0)]
        source = bytearray(resource.payload)
        for index, (w0, w1) in enumerate(commands):
            struct.pack_into(">II", source, index * 8, w0, w1)
        source = bytes(source)
        return replace(
            resource, source=source, payload=source,
            internal={vertex_slot: owners.stage_manifest.PointerRef(ASSET_ID, 0x80)},
            spec=replace(resource.spec, sha256=digest(source), payload_sha256=digest(source)),
        )

    def test_complete_repeated_light_prefix_uses_final_source_colors(self) -> None:
        context = self.compile(self.repeated_light_resource())
        self.assertEqual(context["light_command_counts"], (8, 0))
        self.assertIn((0x44556600, 0x55667700), context["light_preambles"])
        self.assertEqual(context["roots"][0][3], 13)
        self.assertEqual(len(context["triangles"]), 1)

    def test_repeated_light_prefix_cannot_cross_vertex_loads(self) -> None:
        with self.assertRaises(ValueError):
            self.compile(self.repeated_light_resource(cross_vertex=True))

    def test_wrong_source_hash_fails_before_compilation(self) -> None:
        resource = synthetic_resource()
        resource = replace(resource, spec=replace(resource.spec, sha256="0" * 64))
        with self.assertRaises(ValueError):
            self.compile(resource)

    def test_wrong_payload_hash_fails_before_compilation(self) -> None:
        resource = synthetic_resource()
        resource = replace(resource, spec=replace(resource.spec, payload_sha256="0" * 64))
        with self.assertRaises(ValueError):
            self.compile(resource)

    def test_spec_resource_identity_disagreement_fails(self) -> None:
        resource = synthetic_resource()
        resource = replace(resource, spec=replace(resource.spec, file_id=ASSET_ID - 1))
        with self.assertRaises(ValueError):
            self.compile(resource)

    def test_declared_fixup_counts_must_match_typed_pointer_maps(self) -> None:
        resource = synthetic_resource()
        for field, count in (("internal_fixups", 0), ("internal_fixups", 2),
                             ("external_fixups", 1)):
            with self.subTest(field=field, count=count), self.assertRaises(ValueError):
                self.compile(replace(resource, spec=replace(resource.spec, **{field: count})))

    def test_provider_requires_a_typed_resource(self) -> None:
        with self.assertRaises(ValueError):
            owners.NativeOwnerSource(object())

    def test_provider_rejects_overlapping_pointer_domains(self) -> None:
        resource = synthetic_resource()
        with self.assertRaises(ValueError):
            self.compile(replace(resource, external=resource.internal.copy()))

    def test_relocation_map_must_match_source_target_words(self) -> None:
        resource = synthetic_resource()
        slot = next(iter(resource.internal))
        mismatched = {slot: owners.stage_manifest.PointerRef(ASSET_ID, 0x90)}
        with self.assertRaises(ValueError):
            self.compile(replace(resource, internal=mismatched))

    def test_pointer_slot_must_be_aligned_and_bounded(self) -> None:
        resource = synthetic_resource()
        ref = next(iter(resource.internal.values()))
        for slot in (-4, 1, len(resource.payload)):
            with self.subTest(slot=slot), self.assertRaises(ValueError):
                self.compile(replace(resource, internal={slot: ref}))

    def test_unresolved_image_fails(self) -> None:
        with self.assertRaises(ValueError):
            self.compile(synthetic_resource(image=True))

    def test_explicit_foreign_image_target_zero_survives_state_conversion(self) -> None:
        resource = synthetic_resource(image=True)
        resource = replace(
            resource,
            external={0x14: owners.stage_manifest.PointerRef(45000, 0)},
            spec=replace(resource.spec, external_fixups=1),
        )
        context = self.compile(resource)
        image_rows = [row for row in context["state"] if row[0] >> 24 == 0xFD]
        self.assertEqual(len(image_rows), 1)
        self.assertEqual(image_rows[0][1], 0)
        self.assertEqual(image_rows[0][3], 45001)

    def test_foreign_vertex_source_fails(self) -> None:
        with self.assertRaises(ValueError):
            self.compile(synthetic_resource(vertex_asset=ASSET_ID - 1))

    def test_unbound_vertex_load_fails(self) -> None:
        resource = synthetic_resource()
        resource = replace(resource, internal={}, spec=replace(resource.spec, internal_fixups=0))
        with self.assertRaises(ValueError):
            self.compile(resource)

    def test_vertex_only_root_retains_cache_actions_in_a_zero_run_epoch(self) -> None:
        context = self.compile(synthetic_resource(vertex_only=True))
        self.assertEqual(context["triangles"], [])
        self.assertEqual(context["runs"], [])
        self.assertEqual(len(context["vertex"]), 1)
        self.assertEqual(len(context["dense_vertices"]), 3)
        self.assertEqual(len(context["epochs"]), 1)
        self.assertEqual(context["epochs"][0][9], 0)

    def test_tail_vertex_cache_survives_into_next_joint_geometry(self) -> None:
        resource = synthetic_resource()
        source = bytearray(resource.payload)
        # The first source joint loads cache slots 3..5 after its triangle.
        struct.pack_into(">IIII", source, 0x20, 0x0100300C, 0xFFFF0020, 0xDF000000, 0)
        # The next joint loads its own slot 0 and reads the retained 3..4.
        struct.pack_into(">IIIIII", source, 0x40,
                         0x01001002, 0xFFFF0020, 0x05000608, 0, 0xDF000000, 0)
        source = bytes(source)
        internal = resource.internal.copy()
        for slot in (0x24, 0x44):
            internal[slot] = owners.stage_manifest.PointerRef(ASSET_ID, 0x80)
        resource = replace(
            resource, source=source, payload=source, internal=internal,
            spec=replace(resource.spec, sha256=digest(source), payload_sha256=digest(source),
                         internal_fixups=len(internal)),
        )
        context = self.compile(resource, roots=(0, 0x40), bindings=(7, 8))
        self.assertEqual(len(context["triangles"]), 2)
        self.assertEqual(len(context["epochs"]), 3)
        self.assertEqual(len(context["vertex"]), 3)
        self.assertEqual(context["epochs"][1][9], 0)
        self.assertEqual([row[5] for row in context["dense_vertices"]], [7] * 6 + [8])
        self.assertEqual(context["dense_corners"][-3:], [6, 3, 4])

    def test_empty_root_set_fails(self) -> None:
        with self.assertRaises(ValueError):
            self.compile(roots=(), bindings=())

    def test_binding_cardinality_must_match_roots(self) -> None:
        for bindings in ((), (0, 1)):
            with self.subTest(bindings=bindings), self.assertRaises(ValueError):
                self.compile(bindings=bindings)

    def test_bindings_must_be_explicit_compact_integers(self) -> None:
        for binding in (-1, 31, 32, "0", 0.5, True, False):
            with self.subTest(binding=binding), self.assertRaises(ValueError):
                self.compile(bindings=(binding,))

    def test_bad_root_spans_fail(self) -> None:
        for root in (-8, 1, 4, 0xB0):
            with self.subTest(root=root), self.assertRaises(ValueError):
                self.compile(roots=(root,))

    def test_repeated_source_root_preserves_distinct_joint_bindings(self) -> None:
        context = self.compile(roots=(0, 0), bindings=(0, 1))
        self.assertEqual(context["root_bindings"], [0, 1])
        self.assertEqual([row[5] for row in context["dense_vertices"]], [0, 0, 0, 1, 1, 1])
        self.assertEqual(len(context["triangles"]), 2)
        self.assertEqual(len(context["dense_vertices"]), 6)

    def test_distinct_roots_can_share_a_logical_binding(self) -> None:
        context = self.compile(roots=(0, 0x40), bindings=(7, 7))
        self.assertEqual(context["root_bindings"], [7, 7])
        self.assertEqual(len(context["triangles"]), 1)

    def test_default_bindings_preserve_existing_root_order(self) -> None:
        context = self.compile(roots=(0, 0x40), bindings=None)
        self.assertEqual(context["root_bindings"], [0, 1])


class PinnedMetaKnightProviderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        configured = SOURCE_DIR
        if configured is None and os.environ.get("META_KNIGHT_SOURCE_DIR"):
            configured = Path(os.environ["META_KNIGHT_SOURCE_DIR"])
        if configured is None:
            raise unittest.SkipTest("set META_KNIGHT_SOURCE_DIR or --source-dir for pinned model IR checks")
        cls.source_dir = configured.resolve()
        cls.before = global_maps()
        try:
            cls.output = extra.build_meta_knight_model_ir(cls.source_dir, MODEL_ASSET_ID)
        except Exception:
            if global_maps() != cls.before:
                raise AssertionError("failed Meta Knight conversion modified legacy P2 globals")
            raise

    @classmethod
    def tearDownClass(cls) -> None:
        if global_maps() != cls.before:
            raise AssertionError("Meta Knight provider modified legacy P2 globals")

    def test_ir_has_complete_tables_for_both_source_details(self) -> None:
        self.assertEqual(set(self.output["model_ir"]), {"high", "low"})
        hashes = {
            "high": "7280e482fa04e8ea835341f7015dd3270814360c26e71cddf010e02cd294e8ea",
            "low": "7f452f40d1a26d773b8097f6d8acd7b8b8a13067f4e5eb213aa6388e73b7bdb4",
        }
        for detail, program in self.output["model_ir"].items():
            with self.subTest(detail=detail):
                self.assertEqual(program["source_asset_id"], MODEL_ASSET_ID)
                self.assertEqual(program["counts"], {
                    "roots": 23, "triangles": 407, "dense_vertices": 713,
                    "state_deltas": 62, "epochs": 39, "runs": 35, "state_only_roots": 4,
                })
                self.assertEqual(program["ir_sha256"], hashes[detail])
                serialized = json.dumps(program["ir"], sort_keys=True,
                                        separators=(",", ":"), allow_nan=False).encode("utf-8")
                self.assertEqual(digest(serialized), hashes[detail])
                context = program["ir"]
                self.assertEqual(context["source_asset_id"], MODEL_ASSET_ID)
                self.assertIsNone(context["topology"])
                self.assertEqual(len(context["roots"]), 23)
                self.assertEqual(len(context["triangles"]), 407)
                self.assertEqual(len(context["dense_corners"]), 3 * 407)
                self.assertEqual(len(context["packed_corners"]), 3 * 407)
                self.assertEqual(len(context["dense_vertices"]), 713)
                self.assertEqual(len(context["dense_normals"]), 713)
                self.assertEqual(len(context["gx_positions"]), 713)
                self.assertEqual(set(context["primitive_streams"]), {1, 2})

    def test_electric_skeleton_ir_preserves_both_variants_and_body_bindings(self) -> None:
        hashes = {
            ("1", "high"): "cde729dcabf5aeff4236c27f5bac972afde2c5648d9025c4f3821552bc0c9c52",
            ("1", "low"): "31153679c6b45f799baba4f67d94da7ba61d3082b26806b840dfcadb96e968b5",
            ("2", "high"): "8a6eac3de1c9e81779c4892ed07eb4f01991cccc7894dbe8952ccd6ad2966245",
            ("2", "low"): "76f98838776e6375cfd66a8a1e2bbcac680f9324b7ac23f87975db38b5be10d0",
        }
        census = {
            "1": {"roots": 7, "triangles": 201, "dense_vertices": 178,
                  "state_deltas": 17, "epochs": 12, "runs": 22, "state_only_roots": 0},
            "2": {"roots": 7, "triangles": 186, "dense_vertices": 134,
                  "state_deltas": 18, "epochs": 11, "runs": 23, "state_only_roots": 0},
        }
        self.assertEqual(set(self.output["skeleton_ir"]), {"1", "2"})
        for sid, details in self.output["skeleton_ir"].items():
            self.assertEqual(set(details), {"high", "low"})
            for detail, program in details.items():
                with self.subTest(skeleton=sid, detail=detail):
                    self.assertEqual(program["counts"], census[sid])
                    self.assertEqual(program["ir_sha256"], hashes[(sid, detail)])
                    encoded = json.dumps(program["ir"], sort_keys=True,
                                         separators=(",", ":"), allow_nan=False).encode("utf-8")
                    self.assertEqual(digest(encoded), hashes[(sid, detail)])
                    self.assertEqual(program["source_asset_id"], MODEL_ASSET_ID)
                    self.assertEqual(program["binding_source_joints"],
                                     self.output["model_ir"][detail]["binding_source_joints"])
                    self.assertEqual([row["joint_id"] for row in program["source_roots"]],
                                     [6, 10, 11, 14, 15, 23, 28])
                    self.assertEqual([row["binding"] for row in program["source_roots"]],
                                     [0, 2, 3, 6, 7, 10, 12])
                    self.assertEqual(program["ir"]["owner_name"], f"metaknight_skeleton{sid}")
                    self.assertEqual([row["skeleton_flags"] for row in program["source_roots"]],
                                     [0x40 if sid == "2" else 0] + [0] * 6)
            self.assertEqual(details["high"]["source_roots"], details["low"]["source_roots"])

    def test_root_union_keeps_each_source_domain_and_compact_joint_mapping(self) -> None:
        source_joints = [6, 8, 10, 11, 12, 13, 14, 15, 16, 17, 23, 24,
                         28, 29, 30, 31, 32, 33, 34]
        for detail, program in self.output["model_ir"].items():
            with self.subTest(detail=detail):
                self.assertEqual(program["binding_source_joints"], source_joints)
                roots = program["source_roots"]
                self.assertEqual(len({(row["offset"], row["joint_id"]) for row in roots}), 23)
                for kind, count, triangles in (("canonical", 13, 291), ("hidden", 4, 6),
                                               ("modelpart", 5, 110), ("accessory", 1, 0)):
                    selected = [row for row in roots if row["kind"] == kind]
                    self.assertEqual(len(selected), count)
                    self.assertEqual(sum(row["direct_triangles"] for row in selected), triangles)
                for row in roots:
                    self.assertEqual(source_joints[row["binding"]], row["joint_id"])

    def test_qualified_source_union_renders_all_native_image_tables(self) -> None:
        for detail, program in self.output["model_ir"].items():
            with self.subTest(detail=detail):
                context = program["ir"]
                members = {name: values for _ctype, name, values, _guard in images._member_values(context)}
                self.assertEqual(len(members["state_deltas"]), 62)
                self.assertEqual(len(members["dense_vertices"]), 713)
                self.assertEqual(len(members["dense_normals"]), 713)
                self.assertEqual(len(members["prepared_dense"]), 713)
                self.assertEqual(len(members["packed_corners"]), 1221)
                rendered = images.render_image("metaknight", detail, context)
                self.assertIn(".abi_tag = { 0x364f444eu }", rendered)
                self.assertIn(".prepared_dense =", rendered)
                self.assertNotIn(".roots =", rendered)

    def load_image_fixture(self, data: dict, *, include_inventory=False):
        # This temporary input exercises the JSON boundary, never a producer output.
        with tempfile.TemporaryDirectory(prefix="meta_knight_image_loader_test_") as directory:
            path = Path(directory) / "model_ir_fixture.json"
            path.write_text(json.dumps(data, sort_keys=True, allow_nan=False), encoding="utf-8")
            return images.load_extra_model_ir(path, include_inventory=include_inventory)

    def test_saved_ir_restores_typed_modes_and_identical_image_initializers(self) -> None:
        contexts = self.load_image_fixture(self.output)
        self.assertEqual(set(contexts), {(owner, detail) for owner in
                         ("metaknight", "metaknight_skeleton1", "metaknight_skeleton2")
                         for detail in ("high", "low")})
        for detail in ("high", "low"):
            with self.subTest(detail=detail):
                context = contexts[("metaknight", detail)]
                self.assertEqual(set(context["primitive_streams"]), {1, 2})
                self.assertEqual(images._member_values(context),
                                 images._member_values(self.output["model_ir"][detail]["ir"]))
        for sid, details in self.output["skeleton_ir"].items():
            for detail, program in details.items():
                with self.subTest(skeleton=sid, detail=detail):
                    context = contexts[(f"metaknight_skeleton{sid}", detail)]
                    self.assertEqual(set(context["primitive_streams"]), {1, 2})
                    self.assertEqual(images._member_values(context), images._member_values(program["ir"]))

    def test_saved_inventory_produces_identical_source_runtime_metadata(self) -> None:
        inventory, contexts = self.load_image_fixture(self.output, include_inventory=True)
        self.assertEqual(len(contexts), 6)
        self.assertEqual(owners.render_explicit_model_runtime_metadata(inventory),
                         owners.render_explicit_model_runtime_metadata(self.output))

    def test_image_loader_rejects_inventory_identity_and_detail_tampering(self) -> None:
        for field, value in (("schema_version", 2), ("status", "SOURCE_INVENTORY_NOT_NATIVE_ADMISSION"),
                             ("character", "Jigglypuff")):
            data = copy.deepcopy(self.output)
            data[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.load_image_fixture(data)
        data = copy.deepcopy(self.output)
        del data["model_ir"]["low"]
        with self.assertRaises(ValueError):
            self.load_image_fixture(data)

    def test_image_loader_rejects_array_count_tampering(self) -> None:
        for count in ("roots", "triangles", "dense_vertices", "state_deltas", "epochs", "runs"):
            data = copy.deepcopy(self.output)
            data["model_ir"]["high"]["counts"][count] += 1
            with self.subTest(count=count), self.assertRaises(ValueError):
                self.load_image_fixture(data)

    def test_image_loader_rejects_ir_hash_or_contents_tampering(self) -> None:
        data = copy.deepcopy(self.output)
        data["model_ir"]["high"]["ir_sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            self.load_image_fixture(data)
        data = copy.deepcopy(self.output)
        data["model_ir"]["high"]["ir"]["source_asset_id"] += 1
        with self.assertRaises(ValueError):
            self.load_image_fixture(data)

    def test_image_loader_rejects_missing_or_unknown_primitive_modes(self) -> None:
        for modes in ({1: {}}, {1: {}, 2: {}, 3: {}}):
            data = copy.deepcopy(self.output)
            data["model_ir"]["high"]["ir"]["primitive_streams"] = modes
            with self.subTest(modes=sorted(modes)), self.assertRaises(ValueError):
                self.load_image_fixture(data)

    def test_extra_header_rejects_missing_guards_details_and_legacy_collisions(self) -> None:
        contexts = {("metaknight", detail): program["ir"]
                    for detail, program in self.output["model_ir"].items()}
        with self.assertRaises(ValueError):
            images.render_header(contexts, {})
        with self.assertRaises(ValueError):
            images.render_header({("metaknight", "high"): contexts[("metaknight", "high")]}, {},
                                 extra_owner_guards={"metaknight": "NDS_P4_METAKNIGHT"})
        with self.assertRaises(ValueError):
            images.render_header({}, {}, extra_owner_guards={"luigi": "NDS_P4_METAKNIGHT"})

    def test_image_loader_requires_complete_verified_electric_skeleton_ir(self) -> None:
        for sid, detail in (("1", "high"), ("2", "low")):
            data = copy.deepcopy(self.output)
            del data["skeleton_ir"][sid][detail]
            with self.subTest(skeleton=sid, detail=detail), self.assertRaises(ValueError):
                self.load_image_fixture(data)
        data = copy.deepcopy(self.output)
        data["skeleton_ir"]["2"]["high"]["ir_sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            self.load_image_fixture(data)

    def test_runtime_metadata_preserves_canonical_hierarchy_and_source_root_lookup(self) -> None:
        rendered = owners.render_explicit_model_runtime_metadata(self.output)
        self.assertIn("#define NDS_NATIVE_METAKNIGHT_MODEL_ASSET_ID 5456u", rendered)
        self.assertIn("#define NDS_NATIVE_METAKNIGHT_MODEL_DATA_SIZE 75296u", rendered)
        self.assertIn("#define NDS_NATIVE_METAKNIGHT_SOURCE_BINDING_COUNT 19u", rendered)
        for name in ("Purin", "Fox", "Kirby", "Mario"):
            self.assertNotIn(name, rendered)

        def array_rows(symbol: str) -> list[str]:
            match = re.search(rf"\b{symbol}\[\d+\] =\n\{{(.*?)\n\}};", rendered, re.S)
            self.assertIsNotNone(match, symbol)
            return re.findall(r"\{([^{}]+)\}", match[1])

        for detail, suffix in (("high", ""), ("low", "Low")):
            with self.subTest(detail=detail):
                self.assertIn(f"#define NDS_NATIVE_METAKNIGHT_{detail.upper()}_STORAGE_ROOT_COUNT 23u", rendered)
                self.assertIn(f"#define NDS_NATIVE_METAKNIGHT_{detail.upper()}_CANONICAL_ROOT_COUNT 13u", rendered)
                hierarchy = array_rows(f"sNdsNativeMetaknightCanonicalHierarchy{suffix}")
                self.assertEqual(len(hierarchy), 23)
                rows = [[int(value.strip().removesuffix("u"), 0) for value in row.split(",")]
                        for row in hierarchy]
                self.assertEqual(rows[0], [0, 255, 255, 255])
                for index, row in enumerate(rows[1:], 1):
                    self.assertLess(row[1], index)
                self.assertEqual(len(array_rows(f"sNdsNativeMetaknightSourceRoots{suffix}")), 23)
                self.assertIn(f"sNdsNativeMetaknightCanonicalRootIndices{suffix}[13]", rendered)
                self.assertIn(f"sNdsNativeMetaknightMaterialOffsets{suffix}[", rendered)
        self.assertIn("sNdsNativeMetaknightHiddenParts[", rendered)

    @staticmethod
    def metadata_integer_array(rendered: str, symbol: str) -> list[int]:
        match = re.search(rf"\b{symbol}\[\d+\] =\n\{{(.*?)\n\}};", rendered, re.S)
        if match is None:
            raise AssertionError(f"missing metadata array {symbol}")
        return [int(value.strip().removesuffix("u"), 0)
                for value in match[1].split(",") if value.strip()]

    def test_material_asset_arrays_match_source_reference_identity(self) -> None:
        rendered = owners.render_explicit_model_runtime_metadata(self.output)
        model = self.output["model"]
        for detail, suffix in (("high", ""), ("low", "Low")):
            with self.subTest(detail=detail):
                sources = model["details"][detail]["canonical_roots"] + model["details"][detail]["hidden_roots"]
                sources += model["modelparts"]["roots"][detail]
                accessory = model["accesspart"]
                if accessory is not None and "root" in accessory:
                    sources.append({"joint_id": accessory["joint_id"], **accessory["root"]})
                references = {(row["offset"], row["joint_id"]): row["materials"] for row in sources}
                expected = [reference for row in self.output["model_ir"][detail]["source_roots"]
                            for reference in references[(row["offset"], row["joint_id"])]]
                self.assertTrue(expected)
                self.assertTrue(all(row["resource"] == "CHARACTER" for row in expected))
                self.assertEqual(self.metadata_integer_array(rendered, f"sNdsNativeMetaknightMaterialOffsets{suffix}"),
                                 [row["offset"] for row in expected])
                self.assertEqual(self.metadata_integer_array(rendered, f"sNdsNativeMetaknightMaterialAssets{suffix}"),
                                 [MODEL_ASSET_ID] * len(expected))
                for sid, variant in model["skeletons"]["variants"].items():
                    skeleton_refs = [reference for row in variant["roots"]
                                     for reference in row["materials_by_detail"][detail]]
                    offsets = self.metadata_integer_array(
                        rendered, f"sNdsNativeMetaknightSkeleton{sid}MaterialOffsets{suffix}")
                    assets = self.metadata_integer_array(
                        rendered, f"sNdsNativeMetaknightSkeleton{sid}MaterialAssets{suffix}")
                    self.assertEqual(offsets, [row["offset"] for row in skeleton_refs])
                    self.assertEqual(assets, [MODEL_ASSET_ID] * len(skeleton_refs))

    def test_emitted_skeleton_flags_and_live_material_requirements_match_source(self) -> None:
        rendered = owners.render_explicit_model_runtime_metadata(self.output)
        for sid in ("1", "2"):
            for suffix in ("", "Low"):
                with self.subTest(skeleton=sid, detail=suffix or "high"):
                    stem = f"sNdsNativeMetaknightSkeleton{sid}"
                    self.assertEqual(self.metadata_integer_array(rendered, f"{stem}Flags{suffix}"),
                                     [0x40 if sid == "2" else 0] + [0] * 6)
                    self.assertEqual(self.metadata_integer_array(rendered, f"{stem}MaterialInherited{suffix}"),
                                     [0] * 7 if sid == "1" else [0, 0, 0, 1, 0, 1, 1])
                    self.assertEqual(self.metadata_integer_array(rendered, f"{stem}RequiredMaterialSlots{suffix}"),
                                     [0] * 7 if sid == "1" else [0, 1, 1, 1, 1, 1, 1])

    def test_material_assets_follow_declared_resource_even_across_offset_ranges(self) -> None:
        data = copy.deepcopy(self.output)
        data["source_resource_ids"] = {"MAIN": 5455, "CHARACTER": MODEL_ASSET_ID}
        root = next(row for row in data["model"]["details"]["high"]["canonical_roots"] if row["materials"])
        root["materials"] = [
            {"type": "MObjSub", "resource": "CHARACTER", "offset": 0x10, "bytes": 120},
            {"type": "MObjSub", "resource": "MAIN", "offset": 0x12000, "bytes": 120},
        ]
        rendered = owners.render_explicit_model_runtime_metadata(data)
        offsets = self.metadata_integer_array(rendered, "sNdsNativeMetaknightMaterialOffsets")
        assets = self.metadata_integer_array(rendered, "sNdsNativeMetaknightMaterialAssets")
        pairs = list(zip(offsets, assets))
        self.assertIn((0x10, MODEL_ASSET_ID), pairs)
        self.assertIn((0x12000, 5455), pairs)
        del data["source_resource_ids"]["MAIN"]
        with self.assertRaises(ValueError):
            owners.render_explicit_model_runtime_metadata(data)

    def test_ir_is_reproducible_without_global_source_registration(self) -> None:
        repeated = extra.build_meta_knight_model_ir(self.source_dir, MODEL_ASSET_ID)
        self.assertEqual(
            json.dumps(self.output, sort_keys=True, allow_nan=False),
            json.dumps(repeated, sort_keys=True, allow_nan=False),
        )
        self.assertEqual(global_maps(), self.before)


class NativeImageSerializationTests(unittest.TestCase):
    def test_explicit_context_renders_native_image_members_in_memory(self) -> None:
        before = global_maps()
        context = owners.build_p2_root_set_runtime_context(
            REPO_ROOT, OWNER_NAME, "high", (0,),
            source_provider=owners.NativeOwnerSource(synthetic_resource()), root_bindings=(7,),
        )
        members = {name: (ctype, values, guard) for ctype, name, values, guard in
                   images._member_values(context)}
        self.assertEqual(len(members["vertex_actions"][1]), 1)
        self.assertEqual(len(members["dense_vertices"][1]), 3)
        self.assertEqual(len(members["prepared_dense"][1]), 3)
        self.assertEqual(len(members["packed_corners"][1]), 3)
        self.assertEqual(members["prepared_dense"][2], "NDS_RENDERER_PROFILE_LEVEL < 2")
        self.assertEqual(members["packed_corners"][2], images.PACKED_CORNERS_GUARD)
        rendered = images.render_image(OWNER_NAME, "high", context)
        self.assertIn(".abi_tag = { 0x364f444eu }", rendered)
        self.assertIn('section(".fighter_image")', rendered)
        self.assertIn(".vertex_actions =", rendered)
        self.assertIn(".dense_vertices =", rendered)
        self.assertNotIn(".roots =", rendered)
        self.assertEqual(global_maps(), before)


class O2RSourceRootIsolationTests(unittest.TestCase):
    def test_same_file_identity_resolves_independently_in_two_source_roots(self) -> None:
        source = images.skeletons.source
        default_cache = source.O2R_ID_CACHE
        with tempfile.TemporaryDirectory(prefix="o2r_source_roots_test_") as directory:
            root_a, root_b = Path(directory) / "a", Path(directory) / "b"
            root_a.mkdir()
            root_b.mkdir()
            paths = [root_a / "lookup_asset_a", root_b / "lookup_asset_b"]
            # Resolver fixtures contain only the valid identity header it reads.
            for index, path in enumerate(paths):
                header = bytearray(0x50)
                header[4:8] = b"OLER"
                struct.pack_into("<I", header, 0x40, 12345)
                header[0x4F] = index + 1
                path.write_bytes(header)
            self.assertEqual(source.o2r_path_by_id(12345, o2r_root=root_a), paths[0])
            self.assertEqual(source.o2r_path_by_id(12345, o2r_root=root_b), paths[1])
            self.assertEqual(source.o2r_path_by_id(12345, o2r_root=root_a), paths[0])
            self.assertIsNone(source.o2r_path_by_id(12346, o2r_root=root_a))
            self.assertNotEqual(paths[0].read_bytes(), paths[1].read_bytes())
        self.assertIs(source.O2R_ID_CACHE, default_cache)

    def test_lookup_rejects_two_competing_source_roots(self) -> None:
        with self.assertRaises(ValueError):
            images.skeletons.source.o2r_path_by_id(12345, REPO_ROOT, o2r_root=REPO_ROOT)


class ExistingSourcePreservationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        configured = SOURCE_DIR
        if configured is None and os.environ.get("META_KNIGHT_SOURCE_DIR"):
            configured = Path(os.environ["META_KNIGHT_SOURCE_DIR"])
        if configured is None:
            raise unittest.SkipTest("set META_KNIGHT_SOURCE_DIR or --source-dir for existing source comparison")
        # MetaKnight -> extra_characters -> donor -> decomp -> repository root.
        cls.repo_root = configured.resolve().parents[3]

    def test_explicit_provider_preserves_existing_boomerang_executable_tables(self) -> None:
        before = global_maps()
        owner = "linkboomerang"
        path, asset_id, source_hash = owners.P2_O2R_ASSETS[owner]
        resource = owners.stage_manifest.load_o2r(
            self.repo_root,
            owners.stage_manifest.InputSpec(str(path), source_hash, asset_id),
        )
        provider = owners.NativeOwnerSource(resource)
        executable_tables = (
            "state", "sequence", "vertex", "triangles", "runs", "epochs", "roots",
            "direct_epoch_policies", "light_preambles", "light_preamble_indices",
            "light_command_counts", "dense_vertices", "dense_normals", "gx_positions",
            "dense_color_sources", "dense_owners", "dense_corners", "action_dense_first",
            "action_dense_spans", "packed_corners", "run_first_corner", "run_first_unique",
            "run_unique_count", "run_unique_dense", "primitive_streams", "run_metadata",
            "unlit_uniform_roots", "unlit_vertex_alpha_deltas",
        )
        try:
            for detail in ("high", "low"):
                with self.subTest(detail=detail):
                    baseline = owners.build_p2_root_set_runtime_context(
                        self.repo_root, owner, detail, (0xF8,),
                    )
                    explicit = owners.build_p2_root_set_runtime_context(
                        self.repo_root, owner, detail, (0xF8,), source_provider=provider,
                    )
                    self.assertEqual(baseline["asset_data_size"], explicit["asset_data_size"])
                    for table in executable_tables:
                        with self.subTest(detail=detail, table=table):
                            self.assertEqual(baseline[table], explicit[table])
        finally:
            self.assertEqual(global_maps(), before, "source comparison changed legacy P2 globals")

    def test_extra_header_appends_slot_and_preserves_real_legacy_types(self) -> None:
        contexts = {
            (owner, detail): owners.build_p2_owner_runtime_context(self.repo_root, owner, detail)
            for owner in images.P2_IMAGE_OWNERS for detail in images.DETAILS
        }
        contexts.update(images.skeletons.contexts(self.repo_root))
        hats = {
            (modelpart, detail): owners.build_p2_kirby_hat_runtime_context(self.repo_root, detail, modelpart)
            for modelpart in owners.KIRBY_COPY_HAT_MODEL_PART_IDS for detail in images.DETAILS
        }
        before = global_maps()  # Legacy pair-source preparation legitimately fills its own cache.
        registry = (images.IMAGE_OWNERS, images.P2_IMAGE_OWNERS, images.IMAGE_OWNER_GUARDS.copy())
        baseline = images.render_header(contexts, hats)
        source_dir = self.repo_root / "decomp" / "smashremix-plus-extra" / "extra_characters" / "MetaKnight"
        new_owner = extra.build_meta_knight_model_ir(source_dir, MODEL_ASSET_ID)
        expanded = dict(contexts)
        expanded.update({("metaknight", detail): program["ir"]
                         for detail, program in new_owner["model_ir"].items()})
        expanded.update({(f"metaknight_skeleton{sid}", detail): program["ir"]
                         for sid, details in new_owner["skeleton_ir"].items()
                         for detail, program in details.items()})
        augmented = images.render_header(
            expanded, hats, extra_owner_guards={name: "NDS_P4_METAKNIGHT" for name in
                                               ("metaknight", "metaknight_skeleton1", "metaknight_skeleton2")},
        )
        slots = re.findall(r"^#define NDS_NATIVE_IMAGE_SLOT_\w+ \d+u$", baseline, re.M)
        self.assertEqual(len(slots), 25)
        for declaration in slots:
            self.assertIn(declaration, augmented)
        self.assertIn("#define NDS_NATIVE_IMAGE_SLOT_METAKNIGHT 25u", augmented)
        self.assertIn("#define NDS_NATIVE_IMAGE_SLOT_METAKNIGHT_SKELETON1 26u", augmented)
        self.assertIn("#define NDS_NATIVE_IMAGE_SLOT_METAKNIGHT_SKELETON2 27u", augmented)
        self.assertIn("#define NDS_NATIVE_IMAGE_OWNER_SLOTS 28u", augmented)
        self.assertIn("#if NDS_P4_METAKNIGHT", augmented)
        self.assertNotIn("#if NDS_P2_METAKNIGHT", augmented)
        for detail in images.DETAILS:
            self.assertIn(
                f"#define NDS_NATIVE_IMAGE_METAKNIGHT_{detail.upper()}_BYTES "
                f"((u32)sizeof(NDSNativeMetaknight{detail.title()}Image))", augmented,
            )
            self.assertIn(f'"nitro:/fighters/metaknight_{detail}.bin"', augmented)
        old_types = list(re.finditer(r"typedef struct (\w+)\n\{\n.*?\n\} \1;", baseline, re.S))
        self.assertGreaterEqual(len(old_types), 50)
        for declaration in old_types:
            self.assertIn(declaration.group(0), augmented)
        self.assertEqual(registry, (images.IMAGE_OWNERS, images.P2_IMAGE_OWNERS, images.IMAGE_OWNER_GUARDS))
        self.assertEqual(global_maps(), before, "extra header production changed legacy P2 globals")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--source-dir", type=Path)
    options, remaining = parser.parse_known_args()
    SOURCE_DIR = options.source_dir
    unittest.main(argv=[sys.argv[0], *remaining])
