"""Zebes light beams and acid alpha, proved from source bytes; no ROM.

Owner (docs/BUGS.md, Stages): the acid's blending is "visibly too HARD", and
the ground-floor stage lights are "a flat/hard transparency (hard upsidown
trapezoid shape) instead of ... a gradient that tapers to fully transparent
towards the top". Reply to r37's subdivision: "i don't think more triangles
is the answer ... the issue is the hard triangle edges are super visible".

The DS has per-vertex colour but only per-POLYGON alpha. This file decodes
the source display lists straight from the pinned O2R payloads -- its own
walk and vertex decode, not the generator's tables -- and proves:

* each light beam (head-1 DLLinks 0x57A8 -> 0x5840, 0x58B8, 0x58F0) is ONE
  G_CC_SHADE triangle with alpha 0/0/160 in the source, and ONE ramp run in
  the packet whose corners carry T = T_peak * alpha / 160, POLY alpha 160, and
  the source's own positions and colours;
* the acid (157:0x9D8) is its seven source triangles in one run at one
  alpha, its source peak 220;
* the lamp body 0x5870 (r37's "cone") is back to its five source triangles;
* the runtime mirror (flag bit, ramp size, T scale, the admitted flag mask and
  the PrepareRun branch) agrees with the generator, and the descriptor rows
  fail closed.
"""
import re
import struct
import sys
import unittest
from dataclasses import replace
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import generate_nds_native_stage as generator
from native_stage_descriptors import get_descriptor

ROOT = Path(__file__).resolve().parents[2]
OWNERS_C = ROOT / "src/nds/nds_renderer_native_owners.c"

BEAM_ROOTS = (0x57A8, 0x58B8, 0x58F0)
G_CC_SHADE_SHADE = (0xFCFFFFFF, 0xFFFE793C)


def source_triangles(resource, root):
    """Every triangle one source DL draws, in order, as (xyz, st, rgba)
    corners, plus the SETCOMBINE words it sets. Decoded here from bytes."""
    payload = resource.payload
    slots = {}
    triangles = []
    combines = []

    def corners(word):
        return tuple(slots[((word >> shift) & 0xFF) // 2]
                     for shift in (16, 8, 0))

    def walk(pc):
        while True:
            w0, w1 = struct.unpack_from(">II", payload, pc)
            op = w0 >> 24
            if op == 0x01:
                count = (w0 >> 12) & 0xFF
                end = (w0 >> 1) & 0x7F
                ref = resource.pointer_at(pc + 4)
                for index in range(count):
                    offset = ref.offset + 16 * index
                    x, y, z, _flag, s, t = struct.unpack_from(
                        ">hhhhhh", payload, offset)
                    slots[end - count + index] = (
                        (x, y, z), (s, t),
                        tuple(payload[offset + 12:offset + 16]))
            elif op == 0x05:
                triangles.append(corners(w0))
            elif op == 0x06:
                triangles.append(corners(w0))
                triangles.append(corners(w1))
            elif op == 0xFC:
                combines.append((w0, w1))
            elif op == 0xDE:
                ref = resource.pointer_at(pc + 4)
                if ref is not None:
                    walk(ref.offset)
                    if w0 & (1 << 16):
                        return
            elif op == 0xDF:
                return
            pc += 8

    walk(root)
    return triangles, combines


def packet_corners(packet, run):
    return [packet.vertices[packet.corners[run.first_corner + k]]
            for k in range(3 * run.triangle_count)]


class ZebesAlphaTreatmentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.desc = get_descriptor("zebes")
        cls.packet = generator.generate(ROOT, cls.desc)
        specs = generator._o2r_inputs_from_descriptor(cls.desc)
        cls.geometry = generator.load_o2r(ROOT, specs["stage_geometry"])
        cls.actors = generator.load_o2r(ROOT, specs["stage_actors"])
        cls.owners = OWNERS_C.read_text(encoding="utf-8")

    def binding_runs(self, asset_id, root):
        packet = self.packet
        matches = [
            index for index, binding in enumerate(packet.bindings)
            if binding.root_offset == root
            and packet.assets[binding.asset_index].asset_id == asset_id]
        self.assertEqual(len(matches), 1, f"{asset_id}:0x{root:x}")
        binding = packet.bindings[matches[0]]
        return binding, packet.runs[
            binding.first_run:binding.first_run + binding.run_count]

    def test_each_beam_is_one_source_triangle_fading_to_zero_at_the_top(self):
        for root in BEAM_ROOTS:
            triangles, combines = source_triangles(self.geometry, root)
            self.assertEqual(len(triangles), 1, hex(root))
            alphas = sorted(corner[2][3] for corner in triangles[0])
            self.assertEqual(alphas, [0, 0, 160], hex(root))
            tip = max(triangles[0], key=lambda corner: corner[2][3])
            tops = [c for c in triangles[0] if c[2][3] == 0]
            # The opaque end is the bottom tip; both zero corners are above it.
            self.assertTrue(all(c[0][1] > tip[0][1] for c in tops), hex(root))
            if root != 0x58F0:  # 0x58F0 inherits 0x58B8's combine in head 1
                self.assertEqual(combines[-1], G_CC_SHADE_SHADE, hex(root))

    def test_each_beam_is_one_ramp_run_carrying_the_source_alpha_on_t(self):
        peak_t = generator.ALPHA_RAMP_T_PEAK
        for root in BEAM_ROOTS:
            (source,), _ = source_triangles(self.geometry, root)
            _binding, runs = self.binding_runs(105, root)
            self.assertEqual(len(runs), 1, hex(root))
            run = runs[0]
            self.assertEqual(run.triangle_count, 1)
            self.assertEqual(run.flags, generator.RUN_FLAG_VERTEX_ALPHA_RAMP)
            policy = self.packet.policies[run.state_policy]
            self.assertEqual((policy.combine_w0, policy.combine_w1),
                             G_CC_SHADE_SHADE, hex(root))
            for (xyz, _st, rgba), vertex in zip(
                    source, packet_corners(self.packet, run)):
                self.assertEqual((vertex.x, vertex.y, vertex.z), xyz)
                self.assertEqual(vertex.rgba >> 8,
                                 (rgba[0] << 16) | (rgba[1] << 8) | rgba[2])
                self.assertEqual(vertex.rgba & 0xFF, 160)  # POLY alpha: peak
                self.assertEqual(vertex.s, generator.ALPHA_RAMP_S)
                self.assertEqual(vertex.t, peak_t * rgba[3] // 160)

    def test_ramp_reproduces_the_source_linear_alpha_field(self):
        # Affine in the triangle on both sides: the ramp coordinate at any
        # barycentric point, read back through the ramp, is the source's own
        # interpolated vertex alpha.
        peak_t = generator.ALPHA_RAMP_T_PEAK
        for root in BEAM_ROOTS:
            (source,), _ = source_triangles(self.geometry, root)
            _binding, (run,) = self.binding_runs(105, root)
            ts = [v.t for v in packet_corners(self.packet, run)]
            for weights in ((1, 0, 0), (0, 1, 0), (0, 0, 1), (1, 1, 1),
                            (2, 1, 1), (1, 3, 0), (5, 2, 9)):
                total = sum(weights)
                source_alpha = sum(w * c[2][3] for w, c in
                                   zip(weights, source)) / total
                t = sum(w * value for w, value in zip(weights, ts)) / total
                self.assertAlmostEqual(160 * t / peak_t, source_alpha, places=9)
        # The peak T is the ramp's last row once the hardware clamps, and the
        # constant S is inside the 8-texel width.
        self.assertEqual(peak_t >> 5, generator.ALPHA_RAMP_TEXELS)
        self.assertTrue(0 <= generator.ALPHA_RAMP_S >> 5 < 8)

    def test_acid_is_its_seven_source_triangles_at_one_alpha(self):
        triangles, _ = source_triangles(self.actors, 0x9D8)
        self.assertEqual(len(triangles), 7)
        peak = max(corner[2][3] for tri in triangles for corner in tri)
        self.assertEqual(peak, 220)
        self.assertEqual(
            sorted({corner[2][3] for tri in triangles for corner in tri}),
            [0, 220])
        _binding, runs = self.binding_runs(157, 0x9D8)
        self.assertEqual(len(runs), 1)
        self.assertEqual(runs[0].flags, 0)
        vertices = packet_corners(self.packet, runs[0])
        self.assertEqual(len(vertices), 21)
        for vertex, (xyz, st, _rgba) in zip(
                vertices, [corner for tri in triangles for corner in tri]):
            self.assertEqual((vertex.x, vertex.y, vertex.z), xyz)
            self.assertEqual((vertex.s, vertex.t), st)
            self.assertEqual(vertex.rgba & 0xFF, peak)

    def test_lamp_body_is_back_to_its_five_source_triangles(self):
        triangles, _ = source_triangles(self.geometry, 0x5870)
        self.assertEqual(len(triangles), 5)
        binding, runs = self.binding_runs(105, 0x5870)
        self.assertEqual(binding.triangle_count, 5)
        self.assertTrue(all(run.flags == 0 for run in runs))
        vertices = [v for run in runs for v in packet_corners(self.packet, run)]
        self.assertEqual([(v.x, v.y, v.z) for v in vertices],
                         [corner[0] for tri in triangles for corner in tri])

    def test_no_zebes_triangle_is_subdivided(self):
        self.assertEqual(tuple(self.desc.alpha_subdivide_roots), ())
        self.assertEqual(len(self.packet.corners) // 3, 151)
        self.assertEqual(
            sum(run.triangle_count for run in self.packet.runs
                if run.flags & generator.RUN_FLAG_VERTEX_ALPHA_RAMP), 3)

    def test_runtime_mirror_matches_the_generator(self):
        flag = re.search(
            r"#define NDS_NATIVE_STAGE_RUN_FLAG_ALPHA_RAMP \(1u << (\d+)\)",
            self.owners)
        self.assertIsNotNone(flag)
        self.assertEqual(1 << int(flag.group(1)),
                         generator.RUN_FLAG_VERTEX_ALPHA_RAMP)
        self.assertNotEqual(generator.RUN_FLAG_VERTEX_ALPHA_RAMP,
                            generator.RUN_FLAG_PROJECTED_CROSS_MATRIX)
        dims = {name: int(value) for name, value in re.findall(
            r"#define NDS_NATIVE_STAGE_ALPHA_RAMP_(WIDTH|HEIGHT) (\d+)u",
            self.owners)}
        self.assertEqual(dims, {"WIDTH": 8,
                                "HEIGHT": generator.ALPHA_RAMP_TEXELS})
        # S10.5 -> DS 12.4 with no other scale or origin: gSPTexture 1.0.
        self.assertIn("#define NDS_NATIVE_STAGE_ALPHA_RAMP_SCALE 0x10000u",
                      self.owners)
        topology = generator.named_c_closure(
            self.owners, "ndsRendererNativeStageValidateTopologyFull")
        self.assertIn("NDS_NATIVE_STAGE_RUN_FLAG_ALPHA_RAMP", topology)
        prepare = generator.named_c_closure(
            self.owners, "ndsRendererNativeStagePrepareRun")
        self.assertIn("ndsRendererNativeStageAlphaRampResolve(&resolved)",
                      prepare)
        fill = generator.named_c_closure(
            self.owners, "ndsRendererNativeStageAlphaRampFill")
        self.assertIn("/ NDS_NATIVE_STAGE_ALPHA_RAMP_WIDTH) << 3", fill)

    def test_descriptor_rows_fail_closed(self):
        cases = (
            # The lamp body samples TEXEL0: it has no free texture slot.
            (dict(alpha_ramp_roots=((105, 0x5870),)), "untextured"),
            # A listed beam that stops ramping must not ship flat.
            (dict(alpha_ramp_roots=((105, 0x57A8), (105, 0x58B8))),
             "alpha ramp/uniform"),
            (dict(alpha_uniform_roots=((157, 0x9D8), (105, 0x57A8))),
             "two alpha treatments"),
        )
        for fields, message in cases:
            with self.assertRaises(generator.Falsifier) as caught:
                generator.generate(ROOT, replace(self.desc, **fields))
            self.assertIn(message, str(caught.exception), fields)


if __name__ == "__main__":
    unittest.main()
