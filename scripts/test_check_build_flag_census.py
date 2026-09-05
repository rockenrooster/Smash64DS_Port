#!/usr/bin/env python3
"""check_build_flag_census.py must see a hole the way the build does: a flag a
source tests that no channel defines is a hole, a foreach-derived echo counts,
a unity-TU slice define covers its sibling slices, an echo whose variable is
never assigned is EMPTY, and the real tree reads clean under --strict."""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import check_build_flag_census as census  # noqa: E402

MAKEFILE = """\
NDS_P2_1P_GAME ?= 0
NDS_VENUES := ALPHA BETA
NDS_DEAD_FLAG ?= 0
CFLAGS := -DARM9 -DREGION_US
$(NDS_BUILD_CONFIG): FORCE
\t\techo '#define NDS_ON $(NDS_P2_1P_GAME)'; \\
\t\t$(foreach venue,$(NDS_VENUES),echo '#define NDS_P2_STAGE_$(venue) $(NDS_P2_1P_GAME)';) \\
\t\techo '#define NDS_DEAD_FLAG $(NDS_DEAD_FLAG)'; \\
\t\techo '#define NDS_NEVER_ASSIGNED $(NDS_NEVER_ASSIGNED)'; \\
"""

FILES = {
    "src/nds/unity.c": '#include "unity_defs.c"\n#include "unity_user.c"\n',
    "src/nds/unity_defs.c": "#define NDS_SLICE_LOCAL 1\n",
    "src/nds/unity_user.c": "#if NDS_SLICE_LOCAL\nint a;\n#endif\n#if NDS_P2_STAGE_BETA\nint b;\n#endif\n",
    "src/port/user.c": (
        "#if NDS_ON && !NDS_HOLE_ONE\nint c;\n#endif\n"
        "#ifdef NDS_ALLOWED_ONE\nint d;\n#endif\n"
        "#if NDS_SLICE_LOCAL\nint e;\n#endif\n"       # other TU: not covered
        "#if NDS_FROM_HEADER > 1\nint f;\n#endif\n"
        "#if NDS_FROM_GENERATOR\nint g;\n#endif\n"
    ),
    "include/nds/conf.h": "#ifndef NDS_CONF_H\n#define NDS_CONF_H\n#define NDS_FROM_HEADER 4\n#endif\n",
    "scripts/gen.py": 'out.append("#define NDS_FROM_GENERATOR 1u")\nout.append(f"#define NDS_TEMPLATED_{x} 1")\n',
}


def make_repo(root: Path) -> None:
    (root / "Makefile").write_text(MAKEFILE, encoding="utf-8")
    for rel, text in FILES.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")


def test_synthetic_repo(tmp_path, monkeypatch):
    make_repo(tmp_path)
    monkeypatch.setitem(census.ALLOW, "NDS_ALLOWED_ONE", "test: deliberately undefined")
    result = census.census(str(tmp_path))
    holes = result["holes"]
    # The undefined flag is a hole; the slice-local define is a hole from the
    # other TU only (its own unity TU is covered).
    assert set(holes) == {"NDS_HOLE_ONE", "NDS_SLICE_LOCAL"}, holes
    assert holes["NDS_SLICE_LOCAL"] == ["src/port/user.c:7"]
    assert holes["NDS_HOLE_ONE"] == ["src/port/user.c:1"]
    assert set(result["allowed"]) == {"NDS_ALLOWED_ONE"}
    # Foreach-derived, header, generator-literal and -D channels all define.
    for name in ("NDS_ON", "NDS_P2_STAGE_ALPHA", "NDS_P2_STAGE_BETA", "NDS_FROM_HEADER",
                 "NDS_FROM_GENERATOR", "ARM9", "REGION_US"):
        assert name in result["defined"], name
    assert result["defined"]["NDS_P2_STAGE_BETA"] == "Makefile:7"
    # A templated generator name is not a wildcard.
    assert not any(n.startswith("NDS_TEMPLATED_") for n in result["defined"])
    assert result["empty"] == ["Makefile:9 $(NDS_NEVER_ASSIGNED) is never assigned"]
    # ALPHA is echoed and never tested; BETA is tested by the unity TU.
    assert result["dead"] == {"NDS_DEAD_FLAG": "unused", "NDS_NEVER_ASSIGNED": "unused",
                              "NDS_P2_STAGE_ALPHA": "unused"}
    assert result["unresolved"] == []


def test_arena_regression_shape(tmp_path):
    """The 2026-09-05 defect: a source tests NDS_P2_STAGE_<VENUE> and the
    Makefile's foreach line is absent.  Every venue must read as a hole."""
    make_repo(tmp_path)
    mk = tmp_path / "Makefile"
    mk.write_text("\n".join(l for l in MAKEFILE.splitlines() if "foreach" not in l) + "\n",
                  encoding="utf-8")
    result = census.census(str(tmp_path))
    assert "NDS_P2_STAGE_BETA" in result["holes"]
    assert result["holes"]["NDS_P2_STAGE_BETA"] == ["src/nds/unity_user.c:4"]


def test_real_tree_strict():
    proc = subprocess.run(
        [sys.executable, str(HERE / "check_build_flag_census.py"), "--strict"],
        cwd=str(HERE.parent), capture_output=True, text=True,
    )
    assert proc.returncode == 0, proc.stdout + proc.stderr
    assert "BUILD_FLAG_CENSUS holes=0 empty=0" in proc.stdout, proc.stdout
