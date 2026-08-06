"""The object cache keys on exact inputs, and refuses to key on unknown ones.

The failure this guards against is a false hit: reusing an object whose source or
headers have moved on would let the ROM build report a green module for code it
never compiled, and the PR validator publishes that verdict.
"""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import rombuild_cache as RBK  # noqa: E402


def compiler_path(path):
    """Render a real path the way mwccarm writes it into a .d file.

    It always emits Windows syntax: a real drive letter when run natively, and Wine's
    Z: -- which stands in for the filesystem root -- on the Linux build box. Deriving
    the shape from the host keeps this test exercising the branch that host uses.
    """
    text = str(path)
    if text.startswith("/"):
        return "Z:" + text.replace("/", "\\")
    return text


class DepfileParsing(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = pathlib.Path(self.tmp.name).resolve()
        (self.repo / "include" / "math").mkdir(parents=True)
        (self.repo / "src").mkdir()
        self.header = self.repo / "include" / "Animation.h"
        self.header.write_text("struct Animation;\n", encoding="utf-8")
        self.nested = self.repo / "include" / "math" / "Fix12.h"
        self.nested.write_text("typedef int fx32;\n", encoding="utf-8")
        self.source = self.repo / "src" / "Anim.c"
        self.source.write_text("int f(void) { return 0; }\n", encoding="utf-8")
        self.scratch = self.repo / "scratch"
        self.scratch.mkdir()

    def tearDown(self):
        self.tmp.cleanup()

    def parse(self, text):
        return RBK.parse_depfile(text, str(self.scratch), self.repo)

    def test_absolute_target_and_headers(self):
        """A drive letter's colon must not be read as the target separator."""
        text = (f"{compiler_path(self.scratch / 'x.o')}: "
                f"{compiler_path(self.source)} \\\n"
                f"\t{compiler_path(self.header)} \\\n"
                f"\t{compiler_path(self.nested)} \n")
        self.assertEqual(self.parse(text),
                         ["include/Animation.h", "include/math/Fix12.h", "src/Anim.c"])

    def test_relative_target(self):
        """What a miss actually produces: mwccarm names the target relative to cwd."""
        text = f"x.o: {compiler_path(self.header)} \n"
        self.assertEqual(self.parse(text), ["include/Animation.h"])

    def test_crlf_line_endings(self):
        """mwccarm is a Windows tool and ends its .d lines with CRLF.

        Regression: handling the "\\<newline>" continuation before stripping CR left
        every multi-line depfile unparsed, which made every compile uncacheable and
        turned the whole cache into an expensive no-op that still looked like it
        worked -- builds were correct, just never faster.
        """
        text = (f"x.o: {compiler_path(self.source)} \\\r\n"
                f"\t{compiler_path(self.header)} \\\r\n"
                f"\t{compiler_path(self.nested)} \r\n")
        self.assertEqual(self.parse(text),
                         ["include/Animation.h", "include/math/Fix12.h", "src/Anim.c"])

    def test_headers_outside_the_repo_are_dropped(self):
        """A compiler-install header is pinned by the version in the source key."""
        outside = pathlib.Path(self.tmp.name).parent / "elsewhere.h"
        outside.write_text("/* not ours */\n", encoding="utf-8")
        try:
            text = (f"x.o: {compiler_path(self.header)} "
                    f"{compiler_path(outside)} \n")
            self.assertEqual(self.parse(text), ["include/Animation.h"])
        finally:
            outside.unlink()

    def test_unresolvable_dependency_refuses_the_entry(self):
        """An input we cannot hash makes the compile uncacheable, not half-keyed."""
        text = f"x.o: {compiler_path(self.header)} Z:\\nowhere\\ghost.h \n"
        self.assertIsNone(self.parse(text))

    def test_no_target_separator(self):
        self.assertIsNone(self.parse("garbage without a colon\n"))

    def test_header_symlinked_out_of_the_repo_refuses_the_entry(self):
        """A header that lives in the tree but resolves outside it is not keyable.

        Dropping it as "not ours" would leave every later edit of its target
        invisible to the key, so the compile has to become uncacheable instead."""
        target = pathlib.Path(self.tmp.name).parent / "outside_target.h"
        target.write_text("int outside;\n", encoding="utf-8")
        link = self.repo / "include" / "Escapes.h"
        try:
            link.symlink_to(target)
        except (OSError, NotImplementedError):
            self.skipTest("symlink creation not permitted on this host")
        try:
            text = f"x.o: {compiler_path(link)} \n"
            self.assertIsNone(self.parse(text))
        finally:
            link.unlink()
            target.unlink()


class Keys(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = pathlib.Path(self.tmp.name).resolve()
        (self.repo / "include").mkdir()
        (self.repo / "src").mkdir()
        (self.repo / "src" / "A.c").write_text("int a;\n", encoding="utf-8")
        (self.repo / "include" / "A.h").write_text("int b;\n", encoding="utf-8")
        self.exe = self.repo / "mwccarm.exe"
        self.exe.write_bytes(b"MZ-compiler-v1")
        self.cache = RBK.ObjectCache(self.repo / "cache", self.repo)

    def tearDown(self):
        self.tmp.cleanup()

    def key(self, version="2004/b56", flags="-O4"):
        return self.cache.source_key("src/A.c", version, flags, self.exe)

    def test_source_key_covers_version_flags_and_bytes(self):
        base = self.key()
        self.assertNotEqual(base, self.key(version="1.2/sp2p3"))
        self.assertNotEqual(base, self.key(flags="-O4 -lang c++"))
        (self.repo / "src" / "A.c").write_text("int a2;\n", encoding="utf-8")
        self.assertNotEqual(base, self.key())

    def test_source_key_is_none_for_a_missing_source(self):
        self.assertIsNone(self.cache.source_key("src/gone.c", "v", "-O4", self.exe))

    def test_source_key_covers_the_compiler_binary(self):
        """The version is a directory name; tools/mwccarm is in no repository.

        Re-provisioning the build box under the same version string must not let a
        surviving cache serve objects the new compiler never produced."""
        base = self.key()
        self.exe.write_bytes(b"MZ-compiler-v2")
        self.cache._tools.clear()  # a fresh build starts with a cold memo
        self.assertNotEqual(base, self.key())

    def test_missing_compiler_still_yields_a_key(self):
        """A build with no compiler fails loudly later; keying must not crash first."""
        self.assertIsNotNone(
            self.cache.source_key("src/A.c", "v", "-O4", self.repo / "absent.exe"))

    def test_object_key_follows_dependency_contents(self):
        """The whole point: editing a header must move the key of every dependent."""
        sk = self.key()
        before = self.cache.object_key(sk, ["include/A.h"])
        self.assertEqual(before, self.cache.object_key(sk, ["include/A.h"]))
        (self.repo / "include" / "A.h").write_text("int b2;\n", encoding="utf-8")
        self.cache._hashes.clear()  # a fresh build starts with a cold memo
        self.assertNotEqual(before, self.cache.object_key(sk, ["include/A.h"]))

    def test_object_key_separates_path_from_content(self):
        """Same bytes at a different path is a different build input."""
        sk = self.key()
        (self.repo / "include" / "B.h").write_text("int b;\n", encoding="utf-8")
        self.assertNotEqual(self.cache.object_key(sk, ["include/A.h"]),
                            self.cache.object_key(sk, ["include/B.h"]))


class Entries(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = pathlib.Path(self.tmp.name).resolve()
        (self.repo / "src").mkdir()
        (self.repo / "include").mkdir()
        (self.repo / "src" / "A.c").write_text("int a;\n", encoding="utf-8")
        (self.repo / "include" / "A.h").write_text("int b;\n", encoding="utf-8")
        self.exe = self.repo / "mwccarm.exe"
        self.exe.write_bytes(b"MZ-compiler-v1")
        self.cache = RBK.ObjectCache(self.repo / "cache", self.repo)
        self.obj = self.repo / "A.o"
        self.obj.write_bytes(b"\x7fELF-object-bytes")

    def tearDown(self):
        self.tmp.cleanup()

    def test_store_then_fetch_round_trips(self):
        sk = self.cache.source_key("src/A.c", "2004/b56", "-O4", self.exe)
        self.cache.put(sk, ["include/A.h"], self.obj)
        deps = self.cache.manifest(sk)
        self.assertEqual(deps, ["include/A.h"])
        dest = self.repo / "fetched.o"
        self.assertTrue(self.cache.fetch(self.cache.object_key(sk, deps), dest))
        self.assertEqual(dest.read_bytes(), self.obj.read_bytes())

    def test_edited_header_misses(self):
        sk = self.cache.source_key("src/A.c", "2004/b56", "-O4", self.exe)
        self.cache.put(sk, ["include/A.h"], self.obj)
        (self.repo / "include" / "A.h").write_text("int b2;\n", encoding="utf-8")
        self.cache._hashes.clear()
        deps = self.cache.manifest(sk)
        self.assertFalse(self.cache.fetch(self.cache.object_key(sk, deps),
                                          self.repo / "fetched.o"))

    def test_unseen_source_has_no_manifest(self):
        self.assertIsNone(self.cache.manifest("0" * 64))

    def test_deps_from_requires_exactly_one_depfile(self):
        scratch = self.repo / "scratch"
        scratch.mkdir()
        self.assertIsNone(self.cache.deps_from(scratch))
        (scratch / "a.d").write_text(f"x.o: {compiler_path(self.repo / 'include' / 'A.h')}\n",
                                     encoding="utf-8")
        self.assertEqual(self.cache.deps_from(scratch), ["include/A.h"])
        (scratch / "b.d").write_text("x.o: whatever\n", encoding="utf-8")
        self.assertIsNone(self.cache.deps_from(scratch))

    def test_prune_evicts_oldest_first(self):
        import os
        keys = []
        for i in range(4):
            key = f"{i:064d}"
            path = self.cache._shard("objects", key, ".o")
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"x" * 1000)
            os.utime(path, (1_000_000 + i, 1_000_000 + i))
            keys.append(path)
        self.assertEqual(self.cache.prune(10_000), 0)  # under the cap: untouched
        self.cache.prune(2_000)
        self.assertFalse(keys[0].exists())
        self.assertTrue(keys[3].exists())

    def test_disabled_cache_reports_nothing_reused(self):
        cache = RBK.ObjectCache(self.repo / "off", self.repo, enabled=False)
        self.assertFalse(cache.enabled)
        self.assertEqual(cache.summary({"miss": 3})["reused"], 0)
        self.assertEqual(cache.summary({"miss": 3})["compiled"], 3)


if __name__ == "__main__":
    unittest.main()
