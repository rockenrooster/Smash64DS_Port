# Independent audit of retained helper work (2026-09-26)

Root performed this audit after the owner's instruction to stop using subagents.
All helpers were already terminal; no new delegated work was started. Scope is
the retained work of this chat's four helpers and its integration, not a claim
that the whole port or every unrun scenario has been reviewed.

## Confirmed issues corrected

1. **Wrong motion owner could pass the pack checker.** A negative fixture moved
   Captain's unreferenced clip `0x67d` into Mario's bank, rebuilding directory,
   kind spans and stream offsets consistently. The old checker still reported
   1,570/1,570 exact decodes and PASS. That would omit a required clip from a
   Captain-only admission set. The checker now compares each owner with its
   source corpus owner independently of the main-user mask.
2. **Unclaimed stream bytes could pass.** Appending four bytes and extending
   `data_bytes` also passed. The checker now requires the last stream to end
   exactly at the declared data boundary and rejects metadata errors before
   calling the C decode bridge.
3. **Absolute ELF aliases could hide overlay references.** A linked absolute
   symbol pointing into the overlay was ignored because its section index was
   SHN_ABS. Address-range classification now covers this case.
4. **ABS32 addends could hide overlay references.** A relocation based on a
   resident symbol could resolve into the overlay and pass. The checker now
   examines the linked ABS32 word (or explicit addend) and rejects unnamed
   cross-boundary addresses; mapping symbols cannot authorize an exemption.

Both corrupted packs and both ELF cases failed their new tests before the
repairs, demonstrating false PASS results. They are rejected after the repairs.
The current real ELF still passes: 155 reviewed pairs, zero unknowns.

I also tightened my own budget-tool integration: it now prices the binary
directory rather than trusting same-size manifest metadata. It requires a
successful check tied to the exact pack, current validator sources and source
corpus. Stale/wrong receipts are tested and rejected.

## Source and implementation review

- Traced `ftMainSetStatus`: normal VS statuses select the main motion table;
  opening/demo statuses select the separate submotion table. Shield poses use
  their existing separate resident provider. Borrower masks come from actual
  main-table references, and mirror opponents are computed per slot. Unmapped
  non-NULL motion symbols fail rather than disappear.
- Compared ARM table-size accounting with `mfWalkTables`: the layout charges
  the backing blob, 212-byte ARM table records, 312-byte table-pointer object,
  symbol/successor arrays, selected descriptors and streams, and aligned raw
  exceptions. These are bank costs, not proof of available shipping memory.
- Reviewed overlay dispatch, scene returns, task tables, shared frame guards,
  and gameplay callbacks. The Boss and bonus backends and shared death/camera
  closures stay resident. The source dispatcher loads other scene code before
  entry; VS loans only the separate linker range, never a malloc header.
- Rechecked DamageFlyMDust against BattleShip and the pinned asset: IA16 source,
  seven current-image frames, material flags at MObjSub+0x30, exact quad, and
  GL_RGB8_A5 with eight palette entries. Word swapping is applied only to the
  runtime layout. Actual C conversion covers all 65,536 IA16 values and 7,168
  source texels with disjoint intensity bands. No additional defect found in
  this scoped source/host review; target visual/lifecycle acceptance remains due.
- Mutated the particle fixture's generated C without changing repository runtime
  code. Disabling a mirror split was caught by the C quad-count assertion;
  changing a UV endpoint while preserving count was caught by the independent
  Python FIFO comparison. The canonical fixture passes.

Earlier integration corrections are also retained: raw-allocation alignment,
complete corpus cache invalidation, missing-symbol rejection, ELF Thumb bounds,
and preserving shared callers that the initial overlay audit overlooked.

## Validation and limits

The combined focused suite passes **31 tests**. The stronger motion checker
still validates **1,570/1,570** exact C decodes and **29** raw exceptions. Pricing
the checked binary still gives canonical/heavy/worst banks of
**566,336 / 651,824 / 697,760 B**, across 1,365 four-slot rosters.

Audited receipts: `mf2-audited-check.json`, `mf2-audited-budgets.json`,
`overlay-audited-crossings.json`, `audit-summary.json`, and
`particle-fixture-audit.json`. Raw before/after logs remain under `builds/`.

This closes the scoped independent code/tooling audit. It does not accept the
runtime optimization. ROM `9F7C46CD` loads four heavy fighters and reaches two
initial updates but has only **12,556 B** free versus the **25,600 B** floor.
MF is not linked. GO/full-match, visual/native engagement, CSS reserve/captures,
menu reload, timing and all roster-stage acceptance remain unrun or open.
