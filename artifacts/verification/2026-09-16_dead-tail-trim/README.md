# The 3,779-line shim trim changed zero instruction bytes

Cleanup item 1 deleted 166 unreachable tails from
`src/port/reloc_backend_compat_shims.c` (17,745 -> 13,966 lines, commit
`63cf6704c64`). This is the proof that it removed nothing the hardware was
executing.

## Method

Compare the linked ELF before and after the deletion, same target, same build
directory, same flags (`NDS_LAB_POSE_JOINT_CAP=0` on both):

```
python scripts/compare-elf-sections.py \
  --a builds/control-jointcap.elf \
  --b builds/build-p2-jointcap/smash64ds-p2-fourcpu-tickhud-hwtri.elf \
  --max-diff 1
```

## Result

| section | bytes | differing |
|---|---:|---:|
| `.itcm` | 32,632 | **0** |
| `.text.hot` | 3,984 | **0** |
| `.text.hot.draw` | 5,980 | **0** |
| `.main` | 1,371,980 | **5** |
| `.main.rw` | 202,020 | **0** |
| `.dtcm` | 8,736 | **0** |

The five bytes are at `.main` + 0x112D48 (vaddr `0x021167f8`), and they are the
build's own embedded git stamp, not code:

```
A: 35 2B 20 20 ... 00 00 37 38 33 35 33 35 32 00 47 49 54 20 25 73 ...
                        "7835352"           "GIT %s TICKHUD"
B: 35 2B 20 20 ... 00 00 32 36 33 63 63 38 32 00 47 49 54 20 25 73 ...
                        "263cc82"           "GIT %s TICKHUD"
```

`78353523152` was HEAD when the pre-trim binary was built; `263cc82ea42` when
the post-trim one was. Two of the seven characters collide, which is why the
diff reports five and not seven.

## Why this is the right check for a deletion of this kind

The deleted code sat after an unconditional `return` at brace depth 1, so GCC
had already discarded it at `-O2`. A deletion that is genuinely unreachable
therefore has a falsifiable signature: **the instruction bytes must not move.**
If any `.text` byte had changed, the code was reachable, or the deletion
perturbed inlining or layout — and either would mean every banked measurement
taken on the old binary no longer describes the new one.

This is stronger than running the game. A passing match shows the deletion did
not break anything the match exercised; a byte-identical `.text` shows there was
nothing to break, and that no measurement needs re-taking.

It is also cheap: one link and one section compare, against the ~20 minutes a
four-CPU match costs.

**Corollary:** the trim cannot have changed performance, so the four-CPU
baseline (WORK-H P50 1,588,928 with the joint-cap instrument, 1,584,128
without) carries forward unchanged. No re-baseline is owed.

## What the trim does change

197 diagnostic counters were written only from the deleted tails. All are
defined and read elsewhere, so nothing breaks at build or link and no script in
`scripts/` reads any of them, but those HUD and census fields now read zero
permanently. That is a documentation change, not a behaviour change — the
writes were already unreachable, so the fields were reading their initial values
before this commit too.
