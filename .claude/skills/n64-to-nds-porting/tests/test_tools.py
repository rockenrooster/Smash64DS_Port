from __future__ import annotations
import copy
import json
from pathlib import Path
import random
import sys
import unittest
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from compile_vertex_plan import compile_plan, PlanError
from live_set import analyze, LiveSetError


class VertexPlanTests(unittest.TestCase):
    def setUp(self):
        self.doc = json.loads((ROOT / "examples/vertex_history.json").read_text())

    def test_load_history_and_immutable_patch(self):
        plan = compile_plan(self.doc)
        self.assertEqual(len(plan["triangles"]), 3)
        self.assertEqual(plan["mixed_position_triangles"], 2)
        self.assertEqual(plan["triangles"][0]["vertices"], [0, 1, 2])
        self.assertEqual(plan["triangles"][1]["vertices"], [0, 1, 3])
        self.assertEqual(plan["triangles"][2]["vertices"], [4, 1, 3])
        self.assertEqual(plan["vertex_versions"][0]["overrides"], {})
        self.assertEqual(plan["vertex_versions"][4]["overrides"], {"st": [-32, 64]})
        self.assertEqual(plan["vertex_versions"][0]["transform"], "joint_A_pose_expression_v1")
        self.assertEqual(plan["vertex_versions"][3]["transform"], "joint_B_pose_expression_v1")
        self.assertEqual(plan, compile_plan(self.doc))

    def test_call_state_is_not_implicitly_restored(self):
        self.doc["lists"]["root"].insert(-1, {"op": "load", "first": 4, "sources": ["extra"]})
        plan = compile_plan(self.doc)
        self.assertEqual(plan["vertex_versions"][-1]["transform"], "joint_B_pose_expression_v1")

    def test_unloaded_vertex_is_not_success(self):
        self.doc["lists"]["root"][3]["slots"][0] = 15
        with self.assertRaises(PlanError): compile_plan(self.doc)

    def test_unknown_op_and_screen_patch_rejected(self):
        for operation in ({"op": "unknown"}, {"op": "patch", "slot": 0, "field": "screen_xy", "value": [1, 2]}):
            doc = copy.deepcopy(self.doc)
            doc["lists"]["root"].insert(-1, operation)
            with self.assertRaises(PlanError): compile_plan(doc)

    def test_cycles_and_limits(self):
        self.doc["lists"]["child"].insert(0, {"op": "call", "list": "root"})
        with self.assertRaises(PlanError): compile_plan(self.doc)
        self.setUp()
        with self.assertRaises(PlanError): compile_plan(self.doc, max_steps=2)
        with self.assertRaises(PlanError): compile_plan(self.doc, max_depth=1)

    def test_branch_stops_caller(self):
        self.doc["lists"]["root"][4]["op"] = "branch"
        result = compile_plan(self.doc)
        self.assertEqual(len(result["triangles"]), 2)
        self.assertEqual(len(result["vertex_versions"]), 4)

    def test_missing_end_and_bad_span(self):
        self.doc["lists"]["root"].pop()
        with self.assertRaises(PlanError): compile_plan(self.doc)
        self.setUp()
        self.doc["lists"]["root"][2]["first"] = 15
        with self.assertRaises(PlanError): compile_plan(self.doc)

    def test_strict_schema_and_missing_state(self):
        self.doc["cache_slots"] = True
        with self.assertRaises(PlanError): compile_plan(self.doc)
        self.setUp()
        self.doc["lists"]["root"].pop(1)
        with self.assertRaises(PlanError): compile_plan(self.doc)
        self.setUp()
        self.doc["lists"]["root"][0]["ignored_semantic_field"] = 1
        with self.assertRaises(PlanError): compile_plan(self.doc)


    def test_random_history_against_eager_position_oracle(self):
        rng = random.Random(946)
        for _ in range(100):
            translations = {f"T{i}": (i * 7, -i * 3, i + 1) for i in range(5)}
            source_positions = {f"V{i}": (i, i * i, -i) for i in range(30)}
            commands = [{"op": "material", "id": "M"}]
            eager_cache = [None] * 16
            expected = []
            current = "T0"
            commands.append({"op": "state", "transform": current, "vertex_state": "S"})
            for step in range(150):
                choice = rng.randrange(3)
                if choice == 0:
                    current = f"T{rng.randrange(5)}"
                    commands.append({"op": "state", "transform": current, "vertex_state": "S"})
                elif choice == 1:
                    slot, source = rng.randrange(16), f"V{rng.randrange(30)}"
                    commands.append({"op": "load", "first": slot, "sources": [source]})
                    eager_cache[slot] = tuple(a + b for a, b in zip(source_positions[source], translations[current]))
                else:
                    live = [i for i, v in enumerate(eager_cache) if v is not None]
                    if len(live) >= 3:
                        selected = rng.sample(live, 3)
                        commands.append({"op": "tri", "slots": selected})
                        expected.append([eager_cache[i] for i in selected])
            commands.append({"op": "end"})
            plan = compile_plan({"schema": 1, "cache_slots": 16, "entry": "root", "lists": {"root": commands}})
            observed = []
            for triangle in plan["triangles"]:
                corners = []
                for version in triangle["vertices"]:
                    vertex = plan["vertex_versions"][version]
                    corners.append(tuple(a + b for a, b in zip(source_positions[vertex["source"]], translations[vertex["transform"]])))
                observed.append(corners)
            self.assertEqual(observed, expected)


class LiveSetTests(unittest.TestCase):
    def setUp(self):
        self.doc = json.loads((ROOT / "examples/live_set.json").read_text())

    def test_late_spawn_and_dedup(self):
        result = analyze(self.doc)
        self.assertEqual(result["dropped"], ["setup_preview"])
        self.assertIn("projectile", result["kept"])
        self.assertEqual(result["kept_object_bytes"], 96 + 2048 + 32 + 24 + 1024)
        self.assertEqual(len(result["layout"]), 5)
        for obj in result["layout"]:
            self.assertEqual(obj["offset"] % obj["align"], 0)
        self.assertEqual(result, analyze(self.doc))

    def test_unknown_reachable_fails(self):
        self.doc["roots"].append("setup_preview")
        with self.assertRaises(LiveSetError): analyze(self.doc)

    def test_incomplete_roots_and_missing_target(self):
        self.doc["roots_complete"] = False
        with self.assertRaises(LiveSetError): analyze(self.doc)
        self.setUp()
        self.doc["objects"][0]["edges"].append("missing")
        with self.assertRaises(LiveSetError): analyze(self.doc)

    def test_cycles_terminate_without_double_counting(self):
        self.doc["objects"][2]["edges"].append("actor_runtime")
        result = analyze(self.doc)
        self.assertEqual(len(result["kept"]), 5)

    def test_input_order_does_not_change_layout(self):
        expected = analyze(self.doc)
        self.doc["objects"].reverse()
        self.doc["roots"].reverse()
        self.assertEqual(expected, analyze(self.doc))

    def test_strict_fields_alignment_and_duplicates(self):
        for mutate in (lambda d: d["objects"][0].update(align=3),
                       lambda d: d["objects"].append(copy.deepcopy(d["objects"][0])),
                       lambda d: d["objects"][0].update(size=True),
                       lambda d: d.update(roots_complete=1)):
            doc = copy.deepcopy(self.doc)
            mutate(doc)
            with self.assertRaises(LiveSetError): analyze(doc)

    def test_layout_overflow(self):
        self.doc["objects"][0]["size"] = 0xffffffff
        with self.assertRaises(LiveSetError): analyze(self.doc)

    def test_random_graph_closure_against_fixed_point(self):
        rng = random.Random(64)
        for _ in range(100):
            count = 20
            objects = [{"id": str(i), "size": rng.randrange(1, 100), "align": 1 << rng.randrange(5),
                        "edge_status": "complete", "edges": [str(j) for j in range(count) if rng.randrange(8) == 0]}
                       for i in range(count)]
            doc = {"schema": 1, "roots_complete": True, "roots": ["0", "1"], "objects": objects}
            expected = {"0", "1"}
            while True:
                expanded = expected | {edge for obj in objects if obj["id"] in expected for edge in obj["edges"]}
                if expanded == expected: break
                expected = expanded
            self.assertEqual(set(analyze(doc)["kept"]), expected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
