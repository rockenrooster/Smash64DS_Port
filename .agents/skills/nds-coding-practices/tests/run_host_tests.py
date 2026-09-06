#!/usr/bin/env python3
"""Host logic/call-contract checks, not libnds target or device verification."""
from pathlib import Path
import os
import re
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CC = shlex.split(os.environ.get("CC", "cc"))
CXX = shlex.split(os.environ.get("CXX", "c++"))

def run(command: list[str]) -> None:
    subprocess.run(command, check=True)

def main() -> None:
    if not CC or shutil.which(CC[0]) is None:
        raise SystemExit("Host C compiler not found. Set CC to a GCC/Clang-compatible compiler.")
    with tempfile.TemporaryDirectory(prefix="nds-skill-tests-") as temporary:
        temp = Path(temporary)
        common = CC + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT/"tests/mocks")]
        for name, flags in [
            ("debug", ["-O0"]),
            ("optimized", ["-O2"]),
            ("release", ["-O2", "-DNDEBUG"]),
            ("ubsan", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
        ]:
            executable = temp / name
            run(common + flags + ["-c", str(ROOT/"tests/portable_codegen.c"), "-o", str(temp/(name+"-probes.o"))])
            run(common + flags + [str(ROOT/"tests/test_helpers.c"), "-o", str(executable)])
            print(f"[{name}]", flush=True)
            run([str(executable)])
        chapter = (ROOT/"references/11-storage-filesystems-streaming.md").read_text()
        snippet = next(s for s in re.findall(r"```c\n(.*?)```", chapter, re.S) if "static bool read_exact" in s)
        source = temp / "read_exact.c"
        source.write_text(snippet + r"""
int main(void) {
    FILE *file = tmpfile();
    if (file == NULL) return 1;
    unsigned char data[4] = {0};
    if (fwrite("ABCD", 1, 4, file) != 4) return 2;
    rewind(file);
    if (!read_exact(file, data, 3) || data[0] != 'A' || data[2] != 'C') return 3;
    if (read_exact(file, data, 2)) return 4; /* Only one byte remains. */
    if (!read_exact(file, data, 0)) return 5;
    return fclose(file) != 0;
}
""")
        executable = temp / "read_exact"
        run(common + [str(source), "-o", str(executable)])
        run([str(executable)])
        print("PASS: standalone C11 read_exact snippet, exact/short/zero reads")
        if CXX and shutil.which(CXX[0]):
            source = temp / "headers.cpp"
            source.write_text('#include "examples/shared_mailbox.h"\n#include "examples/fixed_math.h"\n#include "examples/video_copy16.h"\n#include "examples/pxi/protocol.h"\n#include "examples/pxi/demo.h"\nint main() { return 0; }\n')
            run(CXX + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT), str(source), "-o", str(temp/"headers")])
            print("PASS: reusable headers compile as C++17")
        else:
            print("SKIP: C++ compiler unavailable")
    print("No DS target build, DMA/cache emulation, device run, or speedup measured.")

if __name__ == "__main__":
    main()
