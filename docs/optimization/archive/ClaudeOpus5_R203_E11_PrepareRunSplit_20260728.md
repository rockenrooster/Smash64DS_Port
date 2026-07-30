# R2-03 E11 — the memo E5 proved is worth 46,000, and not where I said

**Date:** 2026-07-28
**Phase:** R2-03. Sizing only, no runtime change.
**Verdict:** the E5 memo is live and large. Its target is the texture prepare,
not the policy validation.

## 1. Why this was reopened

E5 proved `ndsRendererNativePrepareProductionRun` is a pure function of
`run_index` — 0 misses over 112,300 calls, entries == successes so it never
rejects, 0 changes over 208,874 UV writes — and then declined to build the memo,
on the reasoning that ~119 UV writes/frame cannot explain the bucket.

That reasoning was sound about the UV loop and wrong about the function. The
frame census (2026-07-28, `artifacts/r2-04-census`) puts the symbol at 22,205
ticks/frame, and a census row is **self time**: it excludes the texture
resolution the function calls out to, which the same census charges separately
to `ndsRendererHardwareResolveOrBindTexture` (18,803) and
`ndsRendererSyncTextureTile` (12,004).

## 2. The split

Four brackets inside the function, 128 presented frames:

| phase | ticks/frame | calls/frame | ticks/call | share |
|---|---:|---:|---:|---:|
| validate | 8,308 | 60.9 | 136 | 10.1% |
| **texture prepare** | **45,952** | **45.3** | **1,013** | **56.0%** |
| texture reuse | 1,090 | 15.6 | 70 | 1.3% |
| uv loop | 11,853 | 60.9 | 195 | 14.4% |
| tail | 14,838 | 60.9 | 244 | 18.1% |
| **inclusive total** | **82,042** | | | |

Rejects: **0** of 60.9 entries, reconfirming E5 on this build.

Three things move as a result.

**The function is 82,042 inclusive, not 22,205.** Nearly four times its census
row. Any future sizing off a census row alone will make the same error; the row
is self time and this function's cost is mostly in its callee.

**The lever is the texture prepare.** 45.3 of 60.9 calls take the full path and
pay 1,013 ticks each. Only 15.6 hit the caller's per-DObj reuse flag, because
`texture_prepare_valid` is reset per DObj (`nds_renderer.c`, production
executor) — so almost every run re-resolves a texture for a fighter whose
texture identity E5 proved never changes across the entire match.

**The validation is not the lever.** 8,308 ticks, 10% of the function. I had
assumed it was the bulk because it is ~30 comparisons; it is 136 ticks/call.
Skipping it outright — which E5's zero-reject result licenses — is worth 8,308,
not the 20,000 the census row suggested.

## 3. What the memo may and may not cache

E5's STABLE hash already enumerates exactly the fields a memo would restore, and
measured all of them constant: `texture_prepare_name`, the resolved `params`,
`format`, `width`, `height`, `poly_fmt`, `scale_s`, `scale_t`, `origin_s`,
`origin_t`, `offset`, `vertex_flags`, `enabled`. The per-run digest additionally
folds `material_color` and still had 0 misses.

One field is deliberately excluded and must not be cached: `resolved->entry`,
recorded at the time as *"a pointer into the hardware texture cache, which
rotates for reasons unrelated to what is drawn."* The proof that the identity is
stable is therefore **not** a proof that the residency is.

So the memo is: cache the stable identity per `run_index`, skip the resolution,
and still issue the bind — `ndsRendererHardwareBindTextureName` and
`ndsRendererHardwareApplyTextureParams` both already early-out when the state
matches, so the bind is cheap when it is redundant and correct when it is not.
Residency has to be revalidated, not assumed, and a failed revalidation falls
back to the full path.

That is the difference from E8, which was refuted at +16,301: E8's key was 16
floats and cost as much as the work it skipped. This key is an integer index
into a 67-entry table.

## 4. Expected size, stated before building

Full elimination of the resolution is not available, because the bind and the
revalidation stay. Against 45,952 + 8,308 = 54,260 of skippable work, a
realistic figure is **35,000–45,000 ticks/frame**, against a gap of 147,872.

Recorded here so the result is judged against a number chosen in advance.

## 5. For the standing rules

**A census row is self time; a function whose cost is in its callee will be
mis-sized by it.** E5 read 21,504 for this function, the census read 22,205,
and both were consistent — and both were 27% of the real inclusive 82,042. When
a candidate is rejected for being too small, check whether the instrument
measured the function or the work.
