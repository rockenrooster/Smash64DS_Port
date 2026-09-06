"""The per-commit revision must not recompile every C TU; no ROM or make needed."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
OWNERS = {
    "src/nds/nds_platform.c",
    "src/nds/nds_task10_hardware_calibration.c",
}


def emit_revision(short: str) -> bytes:
    # Mirrors the $(NDS_BUILD_REVISION) Makefile recipe exactly.
    return (
        "#ifndef NDS_BUILD_REVISION_H\n"
        "#define NDS_BUILD_REVISION_H\n"
        f'#define NDS_TASK10_GIT_SHORT "{short}"\n'
        "#endif\n"
    ).encode()


def load_build_commit(script: str):
    spec = importlib.util.spec_from_file_location(
        script.replace("-", "_"), ROOT / "scripts" / script)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.build_commit


class BuildRevisionSplitTests(unittest.TestCase):
    def test_makefile_split(self):
        text = (ROOT / "Makefile").read_text(errors="replace")
        config_block = text.split("$(NDS_BUILD_CONFIG): FORCE")[1].split(
            "$(NDS_BUILD_REVISION): FORCE")[0]
        self.assertNotIn("NDS_TASK10_GIT_SHORT", config_block)
        rev_block = text.split("$(NDS_BUILD_REVISION): FORCE")[1].split(
            "$(NDS_SCENE_HARNESS_CONFIG)")[0]
        self.assertIn('#define NDS_TASK10_GIT_SHORT "$(NDS_TASK10_GIT_SHORT)"', rev_block)
        self.assertIn("cmp -s", rev_block)
        self.assertIn(
            "nds_platform.o nds_task10_hardware_calibration.o: $(NDS_BUILD_REVISION)",
            text)
        ofiles_line = next(
            line for line in text.splitlines()
            if line.startswith("$(OFILES) $(NDS_PRIVATE_CHECK_OFILES):"))
        self.assertIn("$(NDS_BUILD_CONFIG)", ofiles_line)
        self.assertNotIn("NDS_BUILD_REVISION", ofiles_line)

    def test_owners_include_revision_with_fallback(self):
        includers = set()
        for path in (ROOT / "src").rglob("*.c"):
            try:
                body = path.read_text(errors="ignore")
            except OSError:
                continue
            if "nds_build_revision.h" in body:
                includers.add(path.relative_to(ROOT).as_posix())
        self.assertEqual(includers, OWNERS)
        for rel in OWNERS:
            body = (ROOT / rel).read_text(errors="ignore")
            self.assertIn('#include "nds_build_revision.h"', body)
            self.assertIn("#ifndef NDS_TASK10_GIT_SHORT", body)
            self.assertIn('#define NDS_TASK10_GIT_SHORT "unknown"', body)

    def test_readers_revision_first_with_config_fallback(self):
        readers = [
            load_build_commit("analyze-symbol-line-profile.py"),
            load_build_commit("analyze-inline-attribution.py"),
        ]
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            for fn in readers:
                with self.subTest(reader=fn.__module__):
                    build = Path(tmp) / fn.__module__
                    build.mkdir(exist_ok=True)
                    for stale in ("nds_build_revision.h", "nds_build_config.h"):
                        stale_path = build / stale
                        if stale_path.exists():
                            stale_path.unlink()
                    self.assertIsNone(fn(build))
                    (build / "nds_build_config.h").write_text(
                        '#define NDS_TASK10_GIT_SHORT "1111111"\n')
                    self.assertEqual(fn(build), "1111111")
                    (build / "nds_build_revision.h").write_text(
                        emit_revision("2222222").decode())
                    self.assertEqual(fn(build), "2222222")
                    (build / "nds_build_revision.h").unlink()
                    self.assertEqual(fn(build), "1111111")


if __name__ == "__main__":
    unittest.main()
