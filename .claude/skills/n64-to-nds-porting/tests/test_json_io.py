from pathlib import Path
import json
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from json_io import read_json, write_json_atomic


class StrictJsonTests(unittest.TestCase):
    def test_duplicates_nonfinite_overflow_and_utf8_rejected(self):
        bad = [b'{"op":"tri","op":"end"}', b'{"nested":{"x":1,"x":2}}',
               b'{"x":NaN}', b'{"x":Infinity}', b'{"x":-Infinity}', b'{"x":1e999}', b'\xff']
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"in.json"
            for raw in bad:
                path.write_bytes(raw)
                with self.subTest(raw=raw), self.assertRaises(ValueError): read_json(path)

    def test_actual_read_is_bounded(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"in.json"
            path.write_bytes(b" "*17)
            with patch("json_io.MAX_INPUT_BYTES", 16), self.assertRaises(ValueError): read_json(path)

    def test_replacement_is_deterministic(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"out.json"
            path.write_text("old")
            write_json_atomic(path, {"b":2,"a":1})
            first = path.read_bytes()
            write_json_atomic(path, {"a":1,"b":2})
            self.assertEqual(first,path.read_bytes())
            self.assertEqual(read_json(path),{"a":1,"b":2})
            self.assertEqual(len(list(Path(tmp).iterdir())),1)

    def test_write_or_replace_failure_preserves_existing_output(self):
        for operation in ("json_io.os.fsync", "json_io.os.replace"):
            with tempfile.TemporaryDirectory() as tmp:
                path = Path(tmp)/"out.json"
                path.write_text("original")
                with patch(operation, side_effect=OSError("injected")), self.assertRaises(OSError):
                    write_json_atomic(path,{"value":3})
                self.assertEqual(path.read_text(),"original")
                self.assertEqual(len(list(Path(tmp).iterdir())),1)

    def test_serialization_failure_preserves_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"out.json"; path.write_text("original")
            with self.assertRaises(ValueError): write_json_atomic(path,{"x":float("nan")})
            self.assertEqual(path.read_text(),"original")

    def test_both_clis_reject_duplicate_fields_without_replacing_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            inp,out = Path(tmp)/"in.json",Path(tmp)/"out.json"
            for tool,fixture in (("compile_vertex_plan.py","vertex_history.json"),("live_set.py","live_set.json")):
                data = (ROOT/"examples"/fixture).read_text()
                # Duplicate an otherwise valid schema field. A permissive parser
                # would accept this; strict parsing must reject it before writing.
                inp.write_text(data.replace('"schema": 1','"schema": 1, "schema": 1',1))
                out.write_text("original")
                result = subprocess.run([sys.executable,str(ROOT/"tools"/tool),str(inp),str(out)],capture_output=True,text=True)
                self.assertEqual(result.returncode,2,result.stdout+result.stderr)
                self.assertIn("duplicate JSON key",result.stderr)
                self.assertEqual(out.read_text(),"original")

if __name__ == "__main__": unittest.main()
